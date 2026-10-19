"""Only the official HTTPS API; no cookies, passwords, or private X endpoints."""
import math
import re
import time
from datetime import datetime, timezone
from email.utils import parsedate_to_datetime

import httpx

from .common import CleanerError, post_id

API_BASE = "https://api.x.com/2"
MAX_ATTEMPTS = 4  # initial request + at most 3 retries


class APIError(CleanerError):
    def __init__(self, category, status=None, until=None):
        self.category, self.status, self.until = category, status, until
        super().__init__(f"공식 API 오류: {category}" + (f" (HTTP {status})" if status else ""))


def json_body(response):
    try:
        body = response.json()
        return body if isinstance(body, dict) else {}
    except ValueError:
        return {}


def wait_until(response, now):
    values = []
    try:
        value = float(response.headers.get("x-rate-limit-reset", ""))
        if math.isfinite(value):
            values.append(value + 1)
    except ValueError:
        pass
    retry = response.headers.get("retry-after", "")
    try:
        seconds = float(retry)
        if math.isfinite(seconds) and seconds >= 0:
            values.append(now + seconds + 1)
    except ValueError:
        try:
            values.append(parsedate_to_datetime(retry).timestamp() + 1)
        except (ValueError, TypeError, OverflowError):
            pass
    return max(values + [now + 1]) if values else now + 60


def check_response(response, now):
    status = response.status_code
    body = json_body(response)
    problems = [body] + [e for e in body.get("errors", []) if isinstance(e, dict)] if isinstance(body.get("errors", []), list) else [body]
    types = {str(e.get("type", "")).rsplit("/", 1)[-1] for e in problems}
    if status == 401:
        raise APIError("authentication", status)
    if status == 403:
        raise APIError("permission", status)
    if status == 402 or types & {"usage-capped", "credits-depleted", "insufficient-credits", "usage-cap-exceeded"}:
        raise APIError("billing_or_usage_cap", status)
    if status == 429:
        raise APIError("rate_limit", status, wait_until(response, now))
    if status >= 500:
        raise APIError("server", status)
    if status == 404:
        raise APIError("not_found", status)
    if status != 200:
        raise APIError("unexpected_response", status)
    if types & {"not-authorized-for-resource", "client-forbidden"}:
        raise APIError("permission", status)
    return body


def pause(until, clock=time.time, sleep=time.sleep, notify=print):
    remaining = until - clock()
    if remaining > 0:
        notify(f"서버 요청 제한/재시도로 약 {math.ceil(remaining)}초 대기합니다. Ctrl+C 후 이어할 수 있습니다.")
    while clock() < until:
        sleep(min(60, until - clock()))


class XAPI:
    def __init__(self, client, tokens, clock=time.time, sleep=time.sleep, notify=print):
        self.client, self.tokens = client, tokens
        self.clock, self.sleep, self.notify = clock, sleep, notify
        self.owner_id = None
        self.checked_generation = None

    def _send(self, method, path, token, params=None):
        try:
            return self.client.request(method, API_BASE + path, params=params,
                                       headers={"Authorization": "Bearer " + token})
        except httpx.HTTPError:
            raise APIError("network") from None

    def _read(self, path, params=None, token=None):
        for attempt in range(MAX_ATTEMPTS):
            try:
                response = (self._send("GET", path, token, params) if token is not None
                            else self.request("GET", path, params=params))
                return check_response(response, self.clock())
            except APIError as error:
                if error.category not in {"rate_limit", "network", "server"} or attempt == MAX_ATTEMPTS - 1:
                    raise
                pause(error.until or self.clock() + 2 ** (attempt + 1), self.clock, self.sleep, self.notify)
        raise AssertionError("unreachable")

    def request(self, method, path, params=None, guard=None):
        allowed_read = method == "GET" and path in {"/users/me", "/tweets"}
        allowed_delete = method == "DELETE" and (re.fullmatch(r"/tweets/[1-9][0-9]{0,18}", path) or
            re.fullmatch(r"/users/[1-9][0-9]{0,18}/retweets/[1-9][0-9]{0,18}", path))
        if not allowed_read and not allowed_delete:
            raise CleanerError("허용되지 않은 API 요청입니다.")
        if allowed_delete and (guard is None or self.owner_id is None):
            raise CleanerError("계정 검증과 보존/선택 검사가 없는 삭제 요청을 차단했습니다.")
        token = self.tokens.access_token()
        if self.owner_id and self.checked_generation != self.tokens.generation:
            # A refresh can occur during a long job. Recheck identity before using it.
            self._check_me(self._read("/users/me", token=token), self.owner_id)
            self.checked_generation = self.tokens.generation
        if allowed_delete:
            guard()  # Must run after refresh/wait and immediately before HTTP DELETE.
        return self._send(method, path, token, params)

    def _check_me(self, body, expected):
        try:
            data = body["data"]
            identifier = post_id(data["id"])
            if not isinstance(data, dict):
                raise TypeError
        except (KeyError, TypeError):
            raise CleanerError("인증된 계정 ID를 검증할 수 없습니다.") from None
        if identifier != expected:
            raise CleanerError("인증된 계정과 아카이브 소유 계정 ID가 다릅니다. 삭제를 차단합니다.")
        self.tokens.remember_account(identifier)
        return data

    def me(self, expected):
        if not expected:
            raise CleanerError("아카이브 소유 계정 ID가 없습니다. 계정 검증과 삭제를 차단합니다.")
        data = self._check_me(self._read("/users/me"), post_id(expected))
        self.owner_id = expected
        self.checked_generation = self.tokens.generation
        return data

    def lookup(self, ids):
        """Return authoritative metadata per requested archive ID; no new targets."""
        result = {}
        identifiers = sorted(set(post_id(i) for i in ids))
        for offset in range(0, len(identifiers), 100):
            batch = identifiers[offset:offset + 100]
            body = self._read("/tweets", {"ids": ",".join(batch),
                "tweet.fields": "author_id,created_at,referenced_tweets,edit_history_tweet_ids,edit_controls"})
            data = body.get("data", [])
            errors = body.get("errors", [])
            if not isinstance(data, list) or not isinstance(errors, list):
                raise CleanerError("게시물 조회 응답 형식이 올바르지 않습니다.")
            returned_ids = {post_id(item.get("id")) for item in data if isinstance(item, dict)}
            for item in data:
                if not isinstance(item, dict):
                    raise CleanerError("게시물 조회 응답 형식이 올바르지 않습니다.")
                identifier = post_id(item.get("id"))
                history = item.get("edit_history_tweet_ids")
                if history is None:
                    # Official docs allow missing edit metadata for pre-2022-09-29
                    # posts. Missing metadata on newer/undated posts is not proof
                    # that there is no edit chain.
                    try:
                        created = datetime.fromisoformat(item["created_at"].replace("Z", "+00:00"))
                        if created.tzinfo is None or created >= datetime(2022, 9, 29, tzinfo=timezone.utc):
                            raise ValueError
                    except (KeyError, AttributeError, ValueError, TypeError):
                        raise CleanerError("게시물 수정 이력이 누락되어 보존 보호를 검증할 수 없습니다.") from None
                    history = [identifier]
                if not isinstance(history, list) or not history:
                    raise CleanerError("게시물 수정 이력을 검증할 수 없습니다.")
                history = sorted(set(post_id(i) for i in history) | {identifier})
                author = post_id(item.get("author_id"))
                refs = item.get("referenced_tweets", [])
                if not isinstance(refs, list) or any(not isinstance(r, dict) for r in refs):
                    raise CleanerError("게시물 종류를 검증할 수 없습니다.")
                if any(r.get("type") not in {"retweeted", "replied_to", "quoted"} for r in refs):
                    raise CleanerError("알 수 없는 게시물 참조 종류입니다. 처리 방식을 추정하지 않습니다.")
                for reference in refs:
                    post_id(reference.get("id"))
                sources = {post_id(r.get("id")) for r in refs if r.get("type") == "retweeted"}
                if len(sources) > 1:
                    raise CleanerError("리포스트 원본 ID가 충돌합니다.")
                kind = "repost" if sources else ("reply" if any(r.get("type") == "replied_to" for r in refs) else "post")
                metadata = {"state": "present", "author_id": author, "kind": kind,
                            "repost_of": next(iter(sources), None), "edit_ids": history,
                            "edit_controls": item.get("edit_controls", {}),
                            "created_at": item.get("created_at")}
                # Prefer an exact returned version. Older and newer versions have
                # different creation times/controls and must not overwrite each
                # other's metadata merely because their edit chains overlap.
                matched = ({identifier} if identifier in batch else set()) | (set(batch) & set(history) - returned_ids)
                if not matched:
                    raise CleanerError("조회 응답 ID가 요청한 아카이브 ID와 연결되지 않습니다.")
                for requested in matched:
                    if requested in result and result[requested] != metadata:
                        raise CleanerError("조회 응답에 충돌하는 게시물 정보가 있습니다.")
                    result[requested] = metadata
            for error in errors:
                if not isinstance(error, dict):
                    raise CleanerError("게시물 조회 오류 형식을 검증할 수 없습니다.")
                category = str(error.get("type", "")).rsplit("/", 1)[-1]
                if category in {"not-authorized-for-resource", "client-forbidden"} or error.get("status") in {401, 403}:
                    raise APIError("permission", error.get("status"))
                if category in {"usage-capped", "credits-depleted"}:
                    raise APIError("billing_or_usage_cap")
                identifier = error.get("resource_id", error.get("value"))
                if identifier is None:
                    raise CleanerError("게시물 조회 오류의 대상 ID가 없습니다.")
                identifier = post_id(identifier)
                if identifier not in batch or identifier in result:
                    raise CleanerError("게시물 조회 결과가 충돌합니다.")
                result[identifier] = {"state": "absent" if category == "resource-not-found" else "unknown"}
            for identifier in batch:
                result.setdefault(identifier, {"state": "unknown"})
        return result

    def delete(self, action, guard):
        path = ("/tweets/" + post_id(action["request_id"]) if action["kind"] == "delete_post" else
                "/users/" + post_id(self.owner_id) + "/retweets/" + post_id(action["request_id"]))
        response = self.request("DELETE", path, guard=guard)
        if 200 <= response.status_code < 300 and response.status_code != 200:
            raise APIError("ambiguous_response", response.status_code)
        body = check_response(response, self.clock())
        # The unrepost reference currently contains an inconsistent retweeted:true
        # example. Accept only an explicit false; ambiguous 2xx is never success.
        field, expected = ("deleted", True) if action["kind"] == "delete_post" else ("retweeted", False)
        data = body.get("data")
        if body.get("errors") or not isinstance(data, dict) or data.get(field) is not expected:
            raise APIError("ambiguous_response", response.status_code)
        return response
