import copy
import json

import httpx
import pytest

from x_cleaner.api import APIError, MAX_ATTEMPTS, wait_until
from x_cleaner.common import CleanerError, canonical
from x_cleaner.workflow import make_plan, verify, run, guard_action
from conftest import A, B, C, D, E, F, OWNER, SOURCE, OTHER_SOURCE, accept_phrase


def test_only_selected_posts_replies_reposts_change(archive_dir, job_factory, api_factory):
    job = job_factory(archive_dir)
    api, server, _ = api_factory(archive_dir)
    with job.open():
        summary = verify(job, api)
        assert not server.delete_calls
        assert (summary["archive_total"], summary["selected_archive_count"], summary["untouched_archive_count"]) == (6, 3, 3)
        assert (summary["delete_post_count"], summary["delete_reply_count"], summary["unrepost_count"]) == (1, 1, 1)
        result = run(job, api, confirm=accept_phrase, notify=lambda _: None)
        assert result["status_counts"] == {"success": 3}
    assert server.delete_calls == [f"/2/tweets/{A}", f"/2/tweets/{B}", f"/2/users/{OWNER}/retweets/{SOURCE}"]
    assert {D, E, F, SOURCE, OTHER_SOURCE}.issubset(server.live)
    assert {A, B, C}.isdisjoint(server.live)
    assert all("likes" not in path and "following" not in path and "dm" not in path for _, path in server.calls)


@pytest.mark.parametrize("keep", [[A], [C], [SOURCE], [A, B, C]])
def test_keep_wins_and_request_guard_rejects_forgery(archive_dir, job_factory, api_factory, keep):
    job = job_factory(archive_dir, keep=keep)
    api, server, _ = api_factory(archive_dir)
    with job.open():
        verify(job, api)
        inputs, plan = job.inputs(), job.plan()
        for action in plan["actions"]:
            assert not set(action["aliases"]) & set(keep)
        original = make_plan({**inputs, "keep": []}, plan["online"], verified=True)
        protected = [a for a in original["actions"] if set(a["aliases"]) & set(keep)]
        for action in protected:
            with pytest.raises(CleanerError, match="선택 목록에 없는"):
                guard_action(inputs, plan, action)
        run(job, api, confirm=accept_phrase, notify=lambda _: None)
    if A in keep:
        assert f"/2/tweets/{A}" not in server.delete_calls
    if C in keep or SOURCE in keep:
        assert f"/2/users/{OWNER}/retweets/{SOURCE}" not in server.delete_calls


@pytest.mark.parametrize("keep,selections", [(["7000000000000001"], None),
    ([], [{"kind": "post", "id": "7000000000000001"}]), ([], [])])
def test_missing_keep_selection_or_empty_selection_blocks_all(archive_dir, job_factory, api_factory, keep, selections):
    job = job_factory(archive_dir, selections=selections, keep=keep)
    api, server, _ = api_factory(archive_dir)
    with job.open():
        summary = verify(job, api)
        assert summary["execution_blocked"]
        with pytest.raises(CleanerError, match="차단"):
            run(job, api, confirm=accept_phrase)
    assert server.delete_calls == []


def test_wrong_kind_selection_blocks(archive_dir, job_factory, api_factory):
    job = job_factory(archive_dir, [{"kind": "reply", "id": A}])
    api, server, _ = api_factory(archive_dir)
    with job.open():
        assert verify(job, api)["execution_blocked"]
        with pytest.raises(CleanerError):
            run(job, api, confirm=accept_phrase)
    assert not server.delete_calls


def test_source_link_resolves_unknown_archive_repost(archive_dir, job_factory, api_factory):
    data_path = archive_dir / "data" / "tweets.js"
    text = data_path.read_text()
    rows = json.loads(text.split("=", 1)[1].strip().rstrip(";"))
    del rows[2]["tweet"]["retweeted_status"]
    data_path.write_text("window.YTD.tweets.part0 = " + json.dumps(rows))
    job = job_factory(archive_dir, [{"kind": "repost", "id": SOURCE}])
    api, server, _ = api_factory(archive_dir)
    server.live[C]["referenced_tweets"] = [{"type": "retweeted", "id": SOURCE}]
    server.live[SOURCE] = {"id": SOURCE, "author_id": "999", "edit_history_tweet_ids": [SOURCE]}
    with job.open():
        assert job.plan()["blocks"]
        assert not verify(job, api)["execution_blocked"]
        run(job, api, confirm=accept_phrase, notify=lambda _: None)
    assert server.delete_calls == [f"/2/users/{OWNER}/retweets/{SOURCE}"]
    assert F in server.live


def test_rt_prefix_alone_is_not_authoritative(archive_dir, job_factory, api_factory):
    data_path = archive_dir / "data" / "tweets.js"
    rows = json.loads(data_path.read_text().split("=", 1)[1].strip().rstrip(";"))
    rows[0]["tweet"]["full_text"] = "RT @someone: I manually wrote this"
    data_path.write_text("window.YTD.tweets.part0 = " + json.dumps(rows))
    job = job_factory(archive_dir, [{"kind": "post", "id": A}])
    api, server, _ = api_factory(archive_dir)
    with job.open():
        assert job.plan()["blocks"]
        assert not verify(job, api)["execution_blocked"]
        run(job, api, confirm=accept_phrase, notify=lambda _: None)
    assert server.delete_calls == [f"/2/tweets/{A}"]


def test_account_mismatch_blocks(archive_dir, job_factory, api_factory):
    job = job_factory(archive_dir)
    api, server, _ = api_factory(archive_dir)
    server.owner = "888"
    with job.open(), pytest.raises(CleanerError, match="계정"):
        verify(job, api)
    assert not server.delete_calls


def test_foreign_author_cannot_delete_other_peoples_comments(archive_dir, job_factory, api_factory):
    job = job_factory(archive_dir, [{"kind": "reply", "id": B}])
    api, server, _ = api_factory(archive_dir)
    server.live[B]["author_id"] = "777"
    with job.open():
        assert verify(job, api)["execution_blocked"]
        with pytest.raises(CleanerError):
            run(job, api, confirm=accept_phrase)
    assert not server.delete_calls


def test_account_rechecked_at_run_and_after_refresh(archive_dir, job_factory, api_factory):
    job = job_factory(archive_dir, [{"kind": "post", "id": A}])
    api, server, _ = api_factory(archive_dir)
    with job.open():
        verify(job, api)
        server.owner = "777"
        with pytest.raises(CleanerError):
            run(job, api, confirm=accept_phrase)
        server.owner = OWNER
        api.me(OWNER)
        api.tokens.generation += 1
        server.owner = "777"
        with pytest.raises(CleanerError):
            api.delete(job.plan()["actions"][0], lambda: None)
    assert not server.delete_calls


def test_confirmation_is_required_and_unverified_blocks(archive_dir, job_factory, api_factory):
    job = job_factory(archive_dir)
    api, server, _ = api_factory(archive_dir)
    with job.open():
        with pytest.raises(CleanerError):
            run(job, api, confirm=accept_phrase)
        verify(job, api)
        with pytest.raises(CleanerError, match="확인 문구"):
            run(job, api, confirm=lambda _: "yes")
    assert not server.delete_calls


def test_already_missing_item_does_not_send_delete(archive_dir, job_factory, api_factory):
    job = job_factory(archive_dir, [{"kind": "post", "id": A}])
    api, server, _ = api_factory(archive_dir)
    server.live.pop(A)
    with job.open():
        verify(job, api)
        summary = run(job, api, confirm=accept_phrase, notify=lambda _: None)
        assert summary["status_counts"] == {"already_absent": 1}
    assert not server.delete_calls


def test_new_unselected_repost_of_same_source_is_not_cancelled(archive_dir, job_factory, api_factory):
    job = job_factory(archive_dir, [{"kind": "repost", "id": C}])
    api, server, _ = api_factory(archive_dir)
    with job.open():
        verify(job, api)
        server.live["901"] = {**server.live.pop(C), "id": "901", "edit_history_tweet_ids": ["901"]}
        result = run(job, api, confirm=accept_phrase, notify=lambda _: None)
        assert result["status_counts"] == {"already_absent": 1}
    assert not server.delete_calls
    assert "901" in server.live


def test_edited_sibling_keep_protects_entire_chain(archive_dir, job_factory, api_factory):
    job = job_factory(archive_dir, [{"kind": "post", "id": A}], keep=[D])
    api, server, _ = api_factory(archive_dir)
    for identifier in (A, D):
        server.live[identifier]["edit_history_tweet_ids"] = [A, D]
    with job.open():
        result = verify(job, api)
        assert result["delete_target_count"] == 0
        assert result["preserved_archive_count"] == 2
        run(job, api, confirm=accept_phrase, notify=lambda _: None)
    assert not server.delete_calls


def test_changed_edit_chain_requires_reverification(archive_dir, job_factory, api_factory):
    job = job_factory(archive_dir, [{"kind": "post", "id": A}])
    api, server, _ = api_factory(archive_dir)
    with job.open():
        verify(job, api)
        server.live[A]["edit_history_tweet_ids"] = [A, D]
        with pytest.raises(CleanerError, match="변경"):
            run(job, api, confirm=accept_phrase)
    assert not server.delete_calls


def test_edit_window_must_close_before_deletion(archive_dir, job_factory, api_factory):
    job = job_factory(archive_dir, [{"kind": "post", "id": A}])
    api, server, clock = api_factory(archive_dir)
    server.live[A]["edit_controls"] = {"is_edit_eligible": True, "editable_until": "2099-01-01T00:00:00Z"}
    with job.open():
        verify(job, api)
        with pytest.raises(CleanerError, match="수정 가능한"):
            run(job, api, confirm=accept_phrase)
    assert not server.delete_calls


def test_archive_and_plan_changes_block(archive_dir, job_factory, api_factory):
    job = job_factory(archive_dir)
    api, server, _ = api_factory(archive_dir)
    with job.open():
        verify(job, api)
        path = job.path / "plan.json"
        original = path.read_text()
        changed = json.loads(original)
        changed["actions"][0]["request_id"] = D
        path.write_text(json.dumps(changed))
        with pytest.raises(CleanerError, match="変更|변경"):
            run(job, api, confirm=accept_phrase)
        path.write_text(original)
        (archive_dir / "data" / "tweets.js").write_text((archive_dir / "data" / "tweets.js").read_text() + "\n")
        with pytest.raises(CleanerError, match="아카이브"):
            run(job, api, confirm=accept_phrase)
    assert not server.delete_calls


def test_rate_limit_uses_server_time_and_respects_retry_bound(archive_dir, job_factory, api_factory):
    job = job_factory(archive_dir, [{"kind": "post", "id": A}])
    api, server, clock = api_factory(archive_dir)
    reset = clock.now + 90
    server.failures = [httpx.Response(429, headers={"x-rate-limit-reset": str(reset), "Retry-After": "120"},
        json={"type": "https://api.x.com/2/problems/rate-limit-exceeded"})]
    with job.open():
        verify(job, api)
        summary = run(job, api, confirm=accept_phrase, notify=lambda _: None)
        assert summary["status_counts"] == {"success": 1}
        assert job.state("delete_post:" + A)["attempts"] == 2
    assert sum(clock.sleeps) >= 121
    assert max(clock.sleeps) <= 60


@pytest.mark.parametrize("status,category", [(401, "authentication"), (403, "permission"), (402, "billing_or_usage_cap")])
def test_auth_permission_billing_stop_immediately(archive_dir, job_factory, api_factory, status, category):
    job = job_factory(archive_dir)
    api, server, clock = api_factory(archive_dir)
    server.failures = [httpx.Response(status, json={"detail": "test-secret-token must never be logged"})]
    with job.open():
        verify(job, api)
        with pytest.raises(APIError, match=category):
            run(job, api, confirm=accept_phrase, notify=lambda _: None)
        job.reports()
    assert len(server.delete_calls) == 1
    assert clock.sleeps == []
    assert "test-secret-token" not in (job.path / "events.jsonl").read_text()


def test_usage_cap_429_does_not_sleep(archive_dir, job_factory, api_factory):
    job = job_factory(archive_dir)
    api, server, clock = api_factory(archive_dir)
    server.failures = [httpx.Response(429, json={"type": "https://api.x.com/2/problems/usage-capped"})]
    with job.open():
        verify(job, api)
        with pytest.raises(APIError, match="billing"):
            run(job, api, confirm=accept_phrase, notify=lambda _: None)
    assert len(server.delete_calls) == 1 and not clock.sleeps


def test_retries_bounded_and_do_not_reset_on_resume(archive_dir, job_factory, api_factory):
    job = job_factory(archive_dir, [{"kind": "post", "id": A}])
    api, server, clock = api_factory(archive_dir)
    server.failures = [httpx.Response(503, json={}) for _ in range(10)]
    with job.open():
        verify(job, api)
        result = run(job, api, confirm=accept_phrase, notify=lambda _: None)
        assert result["status_counts"] == {"uncertain": 1}
        assert job.state("delete_post:" + A)["attempts"] == MAX_ATTEMPTS
    assert len(server.delete_calls) == MAX_ATTEMPTS
    with job.open():
        run(job, api, resume=True, confirm=accept_phrase, notify=lambda _: None)
    assert len(server.delete_calls) == MAX_ATTEMPTS


def test_crash_after_request_then_resume_without_redeleting(archive_dir, job_factory, api_factory):
    job = job_factory(archive_dir)
    api, server, _ = api_factory(archive_dir)

    def crash(request):
        server.after_delete = None
        raise KeyboardInterrupt

    server.after_delete = crash
    with job.open():
        verify(job, api)
        with pytest.raises(KeyboardInterrupt):
            run(job, api, confirm=accept_phrase, notify=lambda _: None)
        assert job.state("delete_post:" + A)["status"] == "in_flight"
    with job.open():
        with pytest.raises(CleanerError, match="resume"):
            run(job, api, confirm=accept_phrase)
        summary = run(job, api, resume=True, confirm=accept_phrase, notify=lambda _: None)
        assert summary["status_counts"] == {"already_absent": 1, "success": 2}
    assert server.delete_calls.count(f"/2/tweets/{A}") == 1
    assert {D, E, F}.issubset(server.live)


def test_wait_survives_interruption(archive_dir, job_factory, api_factory):
    job = job_factory(archive_dir, [{"kind": "post", "id": A}])
    api, server, clock = api_factory(archive_dir)
    until = clock.now + 60
    server.failures = [httpx.Response(429, headers={"x-rate-limit-reset": str(until)}, json={})]

    def interrupted_sleep(seconds):
        raise KeyboardInterrupt

    api.sleep = interrupted_sleep
    with job.open():
        verify(job, api)
        with pytest.raises(KeyboardInterrupt):
            run(job, api, confirm=accept_phrase, notify=lambda _: None)
        assert job.state("delete_post:" + A)["status"] == "retry_wait"
    api.sleep = clock.sleep
    with job.open():
        run(job, api, resume=True, confirm=accept_phrase, notify=lambda _: None)
    assert sum(clock.sleeps) >= 61
    assert len(server.delete_calls) == 2


def test_ambiguous_unrepost_response_is_not_success(archive_dir, job_factory, api_factory):
    job = job_factory(archive_dir, [{"kind": "repost", "id": C}])
    api, server, _ = api_factory(archive_dir)
    server.failures = [httpx.Response(200, json={"data": {"retweeted": True}})]
    with job.open():
        verify(job, api)
        with pytest.raises(APIError, match="ambiguous"):
            run(job, api, confirm=accept_phrase, notify=lambda _: None)
        assert job.state("unrepost:" + SOURCE)["status"] == "uncertain"
    assert len(server.delete_calls) == 1


def test_low_level_delete_requires_guard_and_account(archive_dir, api_factory):
    api, server, _ = api_factory(archive_dir)
    with pytest.raises(CleanerError):
        api.request("DELETE", "/tweets/" + A)
    api.me(OWNER)
    with pytest.raises(CleanerError):
        api.request("DELETE", "/tweets/" + A)
    with pytest.raises(CleanerError):
        api.request("DELETE", f"/users/{OWNER}/likes/{A}", guard=lambda: None)
    assert not server.delete_calls
