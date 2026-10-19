import json
import socket
from pathlib import Path

import httpx
import pytest

from x_cleaner import auth, common
from x_cleaner.archive import read_archive
from x_cleaner.api import XAPI
from x_cleaner.job import Job
from x_cleaner.workflow import make_plan

OWNER = "123456789"
A, B, C, D, E, F = (str(9007199254740993 + i) for i in range(6))
SOURCE, OTHER_SOURCE = "8000000000000002", "8000000000000004"


@pytest.fixture(autouse=True)
def isolated_workspace(tmp_path, monkeypatch):
    monkeypatch.setattr(common, "PROJECT_ROOT", tmp_path)
    monkeypatch.setattr(auth, "PROJECT_ROOT", tmp_path)

    def forbidden(*args, **kwargs):
        pytest.fail("A test attempted real network access")

    monkeypatch.setattr(socket.socket, "connect", forbidden)
    monkeypatch.setattr(socket, "create_connection", forbidden)


@pytest.fixture
def archive_dir(tmp_path):
    return write_archive(tmp_path / "archive")


def write_archive(path, rows=None, account=True):
    data = path / "data"
    data.mkdir(parents=True)
    if account:
        (data / "account.js").write_text('window.YTD.account.part0 = ' + json.dumps([
            {"account": {"accountId": OWNER, "username": "fixture"}}]) + ";", encoding="utf-8")
    rows = default_rows() if rows is None else rows
    (data / "tweets.js").write_text('window.YTD.tweets.part0 = ' + json.dumps(rows, ensure_ascii=False) + ";", encoding="utf-8")
    return path


def default_rows():
    return [
        {"tweet": {"id_str": A, "full_text": "selected post"}},
        {"tweet": {"id_str": B, "full_text": "selected reply", "in_reply_to_status_id_str": "801"}},
        {"tweet": {"id_str": C, "full_text": "RT @someone: selected", "retweeted_status": {"id_str": SOURCE}}},
        {"tweet": {"id_str": D, "full_text": "unselected post"}},
        {"tweet": {"id_str": E, "full_text": "unselected reply", "in_reply_to_status_id_str": "802"}},
        {"tweet": {"id_str": F, "full_text": "RT @another: unselected", "retweeted_status": {"id_str": OTHER_SOURCE}}},
    ]


@pytest.fixture
def job_factory(tmp_path):
    def factory(archive, selections=None, keep=None):
        selections = selections if selections is not None else [{"kind": "post", "id": A},
            {"kind": "reply", "id": B}, {"kind": "repost", "id": C}]
        inputs = {"version": 1, "archive": read_archive(archive), "selections": selections, "keep": keep or []}
        path = tmp_path / "jobs" / f"job-{len(list((tmp_path / 'jobs').glob('*'))) if (tmp_path / 'jobs').exists() else 0}"
        job = Job.create(path, inputs)
        with job.open():
            job.save_plan(make_plan(inputs))
            job.reports()
        return job
    return factory


class Clock:
    def __init__(self):
        self.now = 1800000000.0
        self.sleeps = []

    def time(self):
        return self.now

    def sleep(self, seconds):
        self.sleeps.append(seconds)
        self.now += seconds


class Tokens:
    def __init__(self):
        self.generation = 0
        self.account = None

    def access_token(self):
        return "test-secret-token"

    def remember_account(self, identifier):
        if self.account and self.account != identifier:
            raise common.CleanerError("account changed")
        self.account = identifier


class Server:
    def __init__(self, archive):
        self.owner = OWNER
        self.calls = []
        self.delete_calls = []
        self.failures = []
        self.mutate_before_reply = False
        self.after_delete = None
        self.lookup_errors = {}
        self.live = {}
        for record in read_archive(archive)["records"]:
            refs = []
            if record["repost_of"]:
                refs = [{"type": "retweeted", "id": record["repost_of"]}]
                self.live[record["repost_of"]] = {"id": record["repost_of"], "author_id": "999",
                    "edit_history_tweet_ids": [record["repost_of"]]}
            elif record["reply_to"]:
                refs = [{"type": "replied_to", "id": record["reply_to"]}]
            self.live[record["id"]] = {"id": record["id"], "author_id": OWNER,
                "edit_history_tweet_ids": record["edit_ids"], "referenced_tweets": refs}

    def mutate(self, path):
        identifier = path.rsplit("/", 1)[-1]
        if "/retweets/" in path:
            keys = [key for key, value in self.live.items() if
                {"type": "retweeted", "id": identifier} in value.get("referenced_tweets", [])]
        else:
            history = set(self.live.get(identifier, {}).get("edit_history_tweet_ids", [identifier]))
            keys = [key for key in self.live if key in history]
        for key in keys:
            self.live.pop(key, None)

    def handle(self, request):
        self.calls.append((request.method, request.url.path))
        assert request.url.host == "api.x.com"
        assert request.url.scheme == "https"
        if request.method == "GET" and request.url.path == "/2/users/me":
            return httpx.Response(200, json={"data": {"id": self.owner, "username": "current_name"}})
        if request.method == "GET" and request.url.path == "/2/tweets":
            ids = request.url.params["ids"].split(",")
            data, errors = [], []
            for identifier in ids:
                if identifier in self.lookup_errors:
                    errors.append({"resource_id": identifier, "type": "https://api.x.com/2/problems/" + self.lookup_errors[identifier]})
                elif identifier in self.live:
                    data.append(self.live[identifier])
                else:
                    errors.append({"resource_id": identifier, "type": "https://api.x.com/2/problems/resource-not-found"})
            return httpx.Response(200, json={"data": data, "errors": errors})
        if request.method == "DELETE":
            self.delete_calls.append(request.url.path)
            if self.failures:
                failure = self.failures.pop(0)
                if self.mutate_before_reply:
                    self.mutate(request.url.path)
                if isinstance(failure, BaseException):
                    raise failure
                return failure
            self.mutate(request.url.path)
            if self.after_delete:
                self.after_delete(request)
            data = {"retweeted": False} if "/retweets/" in request.url.path else {"deleted": True}
            return httpx.Response(200, json={"data": data})
        raise AssertionError("Unexpected request " + str(request.url))


@pytest.fixture
def api_factory():
    clients = []
    def factory(archive):
        server, clock, tokens = Server(archive), Clock(), Tokens()
        client = httpx.Client(transport=httpx.MockTransport(server.handle), trust_env=False)
        clients.append(client)
        api = XAPI(client, tokens, clock=clock.time, sleep=clock.sleep, notify=lambda _: None)
        return api, server, clock
    yield factory
    for client in clients:
        client.close()


def accept_phrase(prompt):
    return prompt.split(": ", 1)[1].split("\n")[0]
