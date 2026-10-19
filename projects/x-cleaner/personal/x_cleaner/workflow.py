import math
import time
from datetime import datetime, timezone

from .api import APIError, MAX_ATTEMPTS, pause, wait_until
from .common import CleanerError, digest

TERMINAL = {"success", "already_absent"}


def aliases(record):
    return set(record["edit_ids"]) | {record["id"]} | ({record["repost_of"]} if record["repost_of"] else set())


def matches(record, selection):
    identifier, kind = selection["id"], selection["kind"]
    if kind == "repost":
        return record["kind"] == "repost" and identifier in {record["id"], record["repost_of"]}
    return record["id"] == identifier and record["kind"] != "repost" and (kind != "reply" or record["kind"] == "reply")


def overlay(record, online):
    data = (online or {}).get(record["id"], {})
    result = {**record, "api_state": data.get("state", "unverified")}
    if data.get("state") == "present":
        result.update({k: data[k] for k in ("kind", "repost_of", "edit_ids", "author_id", "edit_controls")})
        result["suspected_repost"] = False
    return result


def make_plan(inputs, online=None, verified=False):
    archive = inputs["archive"]
    records = [overlay(r, online) for r in archive["records"]]
    keep = set(inputs["keep"])
    blocks = []
    if not archive["owner_id"]:
        blocks.append("아카이브 소유 계정 ID가 없습니다.")
    archive_aliases = set().union(*(aliases(r) for r in archive["records"])) if records else set()
    missing_keep = keep - archive_aliases
    if missing_keep:
        blocks.append("보존 ID가 아카이브에서 발견되지 않았습니다: " + ", ".join(sorted(missing_keep)))
    if not inputs["selections"]:
        blocks.append("삭제/취소 선택 목록이 비어 있습니다. 전체 삭제는 지원하지 않습니다.")
    # If one archived edit version is protected, protect its entire known chain.
    protected = set(keep)
    changed = True
    while changed:
        previous = set(protected)
        for record in records:
            if set(record["edit_ids"]) & protected:
                protected.update(record["edit_ids"])
        changed = previous != protected
    selected = {}
    for selection in inputs["selections"]:
        found = [r for r in records if matches(r, selection)]
        if not found:
            blocks.append(f"선택한 {selection['kind']} ID가 아카이브의 해당 종류에서 발견되지 않았습니다: {selection['id']}")
        for record in found:
            selected[record["id"]] = record
    preserved = sorted(r["id"] for r in records if aliases(r) & protected)
    actions = {}
    for record in selected.values():
        identifiers = aliases(record)
        if record["kind"] == "repost" and online and record["repost_of"]:
            source = online.get(record["repost_of"], {})
            if source.get("state") == "present":
                identifiers.update(source["edit_ids"])
        if identifiers & protected:
            continue  # An explicit keep always wins over a delete selection.
        if record["suspected_repost"] or (record["kind"] == "repost" and not record["repost_of"]):
            # A missing wrapper cannot prove whether its relationship still exists.
            blocks.append("리포스트 원본/종류를 확인하지 못했습니다: " + record["id"])
            continue
        if verified:
            if record["api_state"] not in {"present", "absent"}:
                blocks.append("선택 항목의 API 조회 결과를 확정하지 못했습니다: " + record["id"])
            if record["api_state"] == "present" and record.get("author_id") != archive["owner_id"]:
                blocks.append("선택 항목이 아카이브 소유 계정의 게시물이 아닙니다: " + record["id"])
        kind = "unrepost" if record["kind"] == "repost" else "delete_post"
        request_id = record["repost_of"] if kind == "unrepost" else record["id"]
        # One edited post is one destructive operation, even if several versions were selected.
        grouping = (kind, request_id if kind == "unrepost" else tuple(record["edit_ids"]))
        if grouping in actions:
            actions[grouping]["archive_ids"].append(record["id"])
            actions[grouping]["aliases"] = sorted(set(actions[grouping]["aliases"]) | identifiers)
            continue
        actions[grouping] = {"key": kind + ":" + request_id, "kind": kind, "request_id": request_id,
            "archive_ids": [record["id"]], "post_kind": record["kind"],
            "aliases": sorted(identifiers), "edit_ids": record["edit_ids"],
            "text": record["text"], "created_at": record["created_at"], "api_state": record["api_state"]}
    actions = sorted(actions.values(), key=lambda a: (a["kind"], int(a["request_id"])))
    return {"verified": verified, "verified_at": time.time() if verified else None,
            "blocks": sorted(set(blocks)), "selected_ids": sorted(selected),
            "preserved_ids": preserved, "actions": actions, "online": online or {}}


def verify(job, api):
    inputs = job.inputs()
    api.me(inputs["archive"]["owner_id"])
    records = inputs["archive"]["records"]
    selectors = inputs["selections"]
    # Only look up selected archive items. For a source-link selection whose source
    # is absent from the export, resolve archive repost wrappers, never fetch a timeline.
    ids = {r["id"] for r in records if any(s["id"] == r["id"] or matches(r, s) for s in selectors)}
    for selection in selectors:
        if selection["kind"] == "repost" and not any(matches(r, selection) for r in records):
            ids.update(r["id"] for r in records if r["kind"] == "repost")
    ids.update(r["id"] for r in records if aliases(r) & set(inputs["keep"]))
    online = api.lookup(ids)
    resolved = [overlay(r, online) for r in records]
    # Read source edit chains to avoid un-reposting a protected version's other ID.
    sources = {r["repost_of"] for r in resolved if r["repost_of"] and
               any(matches(r, s) for s in selectors)}
    if sources:
        online.update(api.lookup(sources - set(online)))
    plan = make_plan(inputs, online, verified=True)
    job.save_plan(plan)
    return job.reports()


def guard_action(inputs, plan, action):
    if not plan["verified"] or plan["blocks"]:
        raise CleanerError("계획이 검증되지 않았거나 차단 사유가 있습니다.")
    expected = make_plan(inputs, plan["online"], verified=True)
    if expected["blocks"]:
        raise CleanerError("선택/보존 검사를 통과하지 못했습니다.")
    permitted = {a["key"]: a for a in expected["actions"]}
    if action["key"] not in permitted or permitted[action["key"]] != action:
        raise CleanerError("선택 목록에 없는 요청 또는 변경된 요청을 차단했습니다.")
    if set(action["aliases"]) & set(inputs["keep"]):
        raise CleanerError("보존 ID에 대한 삭제 요청을 차단했습니다.")


def confirmation(job, plan, count):
    return f"DELETE {job.inputs(check_source=False)['archive']['owner_id']} {count} {job.get_meta('plan_hash')[:12]}"


def _check_fresh(plan, action, metadata, owner, now):
    if metadata.get("state") == "absent":
        # No DELETE is sent for a missing archive wrapper. A new repost of the
        # same source is a different, unselected item and must remain untouched.
        return "absent"
    if metadata.get("state") != "present" or metadata.get("author_id") != owner:
        raise CleanerError("선택 항목의 현재 상태/작성자를 검증할 수 없습니다.")
    if (metadata["kind"] != action["post_kind"] or set(metadata["edit_ids"]) != set(action["edit_ids"]) or
            (action["kind"] == "unrepost" and metadata["repost_of"] != action["request_id"])):
        raise CleanerError("검증 후 게시물/수정 이력이 변경되었습니다. verify와 미리보기를 다시 확인하세요.")
    controls = metadata.get("edit_controls") or {}
    if not isinstance(controls, dict):
        raise CleanerError("현재 수정 가능 여부를 검증할 수 없습니다.")
    until = controls.get("editable_until")
    if until:
        try:
            date = datetime.fromisoformat(until.replace("Z", "+00:00"))
            if date.tzinfo is None:
                raise ValueError
            timestamp = date.timestamp()
            if not math.isfinite(timestamp):
                raise ValueError
        except (ValueError, TypeError, AttributeError):
            raise CleanerError("게시물 수정 가능 시간을 검증할 수 없습니다.") from None
        if timestamp > now:
            raise CleanerError("아직 수정 가능한 게시물입니다. 수정 가능 시간이 끝난 뒤 다시 검증/실행하세요.")
    elif controls.get("is_edit_eligible") is True:
        raise CleanerError("수정 가능한 게시물의 종료 시간을 알 수 없어 삭제를 차단합니다.")
    elif metadata.get("created_at"):
        try:
            created = datetime.fromisoformat(metadata["created_at"].replace("Z", "+00:00"))
            if created.tzinfo is None:
                raise ValueError
        except (ValueError, TypeError, AttributeError):
            raise CleanerError("게시물 생성 시각을 검증할 수 없습니다.") from None
        if created.timestamp() + 30 * 60 > now:
            raise CleanerError("최근 작성 항목의 수정 가능 시간이 지나지 않았습니다. 30분 경과 후 다시 실행하세요.")
    return "present"


def run(job, api, *, resume=False, retry_failed=False, confirm=input, notify=print):
    inputs = job.inputs()
    plan = job.plan()
    if not plan["verified"] or plan["blocks"]:
        raise CleanerError("실행을 차단합니다. summary.json의 차단 사유를 확인하고 verify를 완료하세요.")
    if job.has_started() and not resume:
        raise CleanerError("실행 이력이 있습니다. --resume --execute 로 이어하세요.")
    api.me(inputs["archive"]["owner_id"])
    selected = []
    for action in plan["actions"]:
        state = job.state(action["key"])
        if state["status"] in TERMINAL:
            continue
        if state["status"] in {"failed", "uncertain"} and not retry_failed:
            continue
        guard_action(inputs, plan, action)
        selected.append(action)
    if not selected:
        notify("실행할 미완료 항목이 없습니다. 실패/미확정 항목은 --retry-failed 로 명시적으로 재시도하세요.")
        return job.reports()
    fresh = api.lookup({i for a in selected for i in a["archive_ids"]})
    for action in selected:
        for identifier in action["archive_ids"]:
            _check_fresh(plan, action, fresh.get(identifier, {}), inputs["archive"]["owner_id"], api.clock())
    phrase = confirmation(job, plan, len(selected))
    notify(f"선택한 미완료 작업 {len(selected)}건을 처리합니다. 수정 게시물은 관련 이력 전체가 삭제됩니다.")
    notify("리포스트는 원본을 삭제하지 않고 내 리포스트만 취소합니다. 선택하지 않은 항목은 추가하지 않습니다.")
    if confirm(f"시작하려면 다음 문구를 정확히 입력하세요: {phrase}\n> ") != phrase:
        raise CleanerError("확인 문구가 일치하지 않습니다. 삭제 요청을 보내지 않았습니다.")
    for action in selected:
        key = action["key"]
        state = job.state(key)
        if state["status"] in {"failed", "uncertain"} and retry_failed:
            with job.db:
                job.db.execute("UPDATE actions SET attempts=0 WHERE key=?", (key,))
            job.event(key, "pending", "explicit_retry")
        if all(fresh[i]["state"] == "absent" for i in action["archive_ids"]):
            job.event(key, "already_absent", "resource_not_found")
            continue
        while job.state(key)["attempts"] < MAX_ATTEMPTS:
            state = job.state(key)
            until = max(state["next_at"], job.cooldown(action["kind"]))
            pause(until, api.clock, api.sleep, notify)

            def guard():
                # Reload bindings on every retry. Public CSV files are never inputs.
                current_inputs = job.inputs()
                current_plan = job.plan()
                if digest(current_plan) != digest(plan):
                    raise CleanerError("실행 중 계획이 변경되었습니다.")
                guard_action(current_inputs, current_plan, action)
                latest = api.lookup(action["archive_ids"])
                for identifier in action["archive_ids"]:
                    if _check_fresh(plan, action, latest.get(identifier, {}),
                                    current_inputs["archive"]["owner_id"], api.clock()) == "absent":
                        raise APIError("not_found", 404)

            # Durable write BEFORE HTTP DELETE. A killed process leaves in_flight.
            job.event(key, "in_flight", increment=True)
            try:
                response = api.delete(action, guard)
            except APIError as error:
                attempts = job.state(key)["attempts"]
                if error.category == "rate_limit":
                    job.set_cooldown(action["kind"], error.until)
                if error.category == "not_found":
                    job.event(key, "already_absent", "resource_not_found")
                    break
                if error.category in {"authentication", "permission", "billing_or_usage_cap"}:
                    job.event(key, "failed", error.category)
                    raise
                if error.category == "ambiguous_response":
                    job.event(key, "uncertain", error.category)
                    raise
                if error.category in {"rate_limit", "server", "network"} and attempts < MAX_ATTEMPTS:
                    until = error.until or api.clock() + 2 ** attempts
                    job.event(key, "retry_wait", error.category, until)
                    continue
                status = "uncertain" if error.category in {"network", "server"} else "failed"
                job.event(key, status, error.category)
                break
            except CleanerError:
                job.event(key, "failed", "safety_check")
                raise
            else:
                if response.headers.get("x-rate-limit-remaining") == "0":
                    job.set_cooldown(action["kind"], wait_until(response, api.clock()))
                job.event(key, "success")
                notify(f"완료: {action['kind']} {action['request_id']}")
                break
        else:
            # Retry counters survive restarts; restarting never bypasses the bound.
            job.event(key, "uncertain", "attempt_limit_reached")
    return job.reports()
