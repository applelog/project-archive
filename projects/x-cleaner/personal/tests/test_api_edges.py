from datetime import datetime, timezone

import httpx
import pytest

from x_cleaner.api import APIError, MAX_ATTEMPTS
from x_cleaner.common import CleanerError
from x_cleaner.workflow import verify, run
from conftest import A, B, C, D, OWNER, SOURCE, accept_phrase


def test_legacy_post_without_edit_metadata_supported(archive_dir, api_factory):
    api, server, _ = api_factory(archive_dir)
    del server.live[A]["edit_history_tweet_ids"]
    server.live[A]["created_at"] = "2020-01-01T00:00:00.000Z"
    assert api.lookup([A])[A]["edit_ids"] == [A]


@pytest.mark.parametrize("created", [None, "2025-01-01T00:00:00.000Z", "2020-01-01T00:00:00", "not-a-date"])
def test_missing_new_or_undated_edit_metadata_blocks(archive_dir, api_factory, created):
    api, server, _ = api_factory(archive_dir)
    del server.live[A]["edit_history_tweet_ids"]
    if created is not None:
        server.live[A]["created_at"] = created
    with pytest.raises(CleanerError, match="수정 이력"):
        api.lookup([A])
    assert not server.delete_calls


def test_unknown_reference_type_blocks(archive_dir, api_factory):
    api, server, _ = api_factory(archive_dir)
    server.live[A]["referenced_tweets"] = [{"id": B, "type": "new-unknown-kind"}]
    with pytest.raises(CleanerError, match="참조 종류"):
        api.lookup([A])


@pytest.mark.parametrize("status,body", [(200, {"data": {"deleted": 1}}), (204, None), (200, {"data": {"deleted": True}, "errors": [{"title": "unexpected"}]})])
def test_ambiguous_delete_is_not_success(archive_dir, job_factory, api_factory, status, body):
    job = job_factory(archive_dir, [{"kind": "post", "id": A}])
    api, server, _ = api_factory(archive_dir)
    server.failures = [httpx.Response(status, json=body) if body is not None else httpx.Response(status)]
    with job.open():
        verify(job, api)
        with pytest.raises(APIError, match="ambiguous"):
            run(job, api, confirm=accept_phrase, notify=lambda _: None)
        assert job.state("delete_post:" + A)["status"] == "uncertain"
    assert len(server.delete_calls) == 1


def test_retry_exhaustion_preserves_cooldown_for_next_item(archive_dir, job_factory, api_factory):
    job = job_factory(archive_dir, [{"kind": "post", "id": A}, {"kind": "post", "id": D}])
    api, server, clock = api_factory(archive_dir)
    attempts_at = []
    original = server.handle
    def handle(request):
        if request.method == "DELETE":
            attempts_at.append(clock.now)
            if len(attempts_at) <= MAX_ATTEMPTS:
                server.delete_calls.append(request.url.path)
                return httpx.Response(429, headers={"x-rate-limit-reset": str(clock.now + 20)}, json={})
        return original(request)
    api.client = httpx.Client(transport=httpx.MockTransport(handle))
    try:
        with job.open():
            verify(job, api)
            result = run(job, api, confirm=accept_phrase, notify=lambda _: None)
            assert result["status_counts"] == {"failed": 1, "success": 1}
        assert len(attempts_at) == 5
        assert attempts_at[4] >= attempts_at[3] + 21
    finally:
        api.client.close()


def test_explicit_retry_failed_is_required_and_can_succeed(archive_dir, job_factory, api_factory):
    job = job_factory(archive_dir, [{"kind": "post", "id": A}])
    api, server, _ = api_factory(archive_dir)
    server.failures = [httpx.Response(400, json={})]
    with job.open():
        verify(job, api)
        assert run(job, api, confirm=accept_phrase, notify=lambda _: None)["status_counts"] == {"failed": 1}
    with job.open():
        run(job, api, resume=True, confirm=accept_phrase, notify=lambda _: None)
    assert len(server.delete_calls) == 1
    with job.open():
        assert run(job, api, resume=True, retry_failed=True, confirm=accept_phrase, notify=lambda _: None)["status_counts"] == {"success": 1}
    assert len(server.delete_calls) == 2


def test_successes_skipped_on_completed_resume(archive_dir, job_factory, api_factory):
    job = job_factory(archive_dir)
    api, server, _ = api_factory(archive_dir)
    with job.open():
        verify(job, api)
        run(job, api, confirm=accept_phrase, notify=lambda _: None)
    count = len(server.delete_calls)
    with job.open():
        run(job, api, resume=True, confirm=lambda _: pytest.fail("nothing to confirm"), notify=lambda _: None)
    assert len(server.delete_calls) == count


def test_crash_after_unrepost_does_not_unrepost_again(archive_dir, job_factory, api_factory):
    job = job_factory(archive_dir, [{"kind": "repost", "id": C}])
    api, server, _ = api_factory(archive_dir)
    server.after_delete = lambda _: (_ for _ in ()).throw(KeyboardInterrupt())
    with job.open():
        verify(job, api)
        with pytest.raises(KeyboardInterrupt):
            run(job, api, confirm=accept_phrase, notify=lambda _: None)
    server.after_delete = None
    with job.open():
        result = run(job, api, resume=True, confirm=accept_phrase, notify=lambda _: None)
        assert result["status_counts"] == {"already_absent": 1}
    assert server.delete_calls == [f"/2/users/{OWNER}/retweets/{SOURCE}"]


def test_lookup_permission_error_stops_without_delete(archive_dir, job_factory, api_factory):
    job = job_factory(archive_dir)
    api, server, _ = api_factory(archive_dir)
    server.lookup_errors[A] = "not-authorized-for-resource"
    with job.open(), pytest.raises(APIError, match="permission"):
        verify(job, api)
    assert not server.delete_calls


def test_proactive_wait_at_zero_remaining(archive_dir, job_factory, api_factory):
    job = job_factory(archive_dir, [{"kind": "post", "id": A}, {"kind": "reply", "id": B}])
    api, server, clock = api_factory(archive_dir)
    original = server.handle
    def handle(request):
        response = original(request)
        if request.method == "DELETE" and len(server.delete_calls) == 1:
            response.headers["x-rate-limit-remaining"] = "0"
            response.headers["x-rate-limit-reset"] = str(clock.now + 130)
        return response
    api.client = httpx.Client(transport=httpx.MockTransport(handle))
    try:
        with job.open():
            verify(job, api)
            run(job, api, confirm=accept_phrase, notify=lambda _: None)
        assert sum(clock.sleeps) >= 131
        assert max(clock.sleeps) <= 60
    finally:
        api.client.close()


def test_two_exact_edit_versions_have_distinct_creation_times(archive_dir, api_factory):
    api, server, _ = api_factory(archive_dir)
    for identifier, created in [(A, "2024-01-01T00:00:00Z"), (D, "2024-01-01T00:02:00Z")]:
        server.live[identifier]["edit_history_tweet_ids"] = [A, D]
        server.live[identifier]["created_at"] = created
    result = api.lookup([A, D])
    assert result[A]["created_at"] == "2024-01-01T00:00:00Z"
    assert result[D]["created_at"] == "2024-01-01T00:02:00Z"


def test_edited_versions_group_into_one_operation(archive_dir, job_factory, api_factory):
    job = job_factory(archive_dir, [{"kind": "post", "id": A}, {"kind": "post", "id": D}])
    api, server, _ = api_factory(archive_dir)
    for identifier in (A, D):
        server.live[identifier]["edit_history_tweet_ids"] = [A, D]
    with job.open():
        summary = verify(job, api)
        assert summary["delete_target_count"] == 1
        assert set(job.plan()["actions"][0]["archive_ids"]) == {A, D}
        run(job, api, confirm=accept_phrase, notify=lambda _: None)
    assert server.delete_calls == [f"/2/tweets/{A}"]
