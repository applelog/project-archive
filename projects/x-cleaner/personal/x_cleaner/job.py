import csv
import io
import json
import os
import sqlite3
import time
from contextlib import contextmanager
from datetime import datetime, timezone

from .archive import read_archive
from .common import CleanerError, canonical, digest, lock, private_directory, project_path, write_json, atomic_write


class Job:
    def __init__(self, path):
        self.path = project_path(path)
        self.db = None

    @classmethod
    def create(cls, path, inputs):
        job = cls(path)
        if job.path.exists():
            raise CleanerError("작업 폴더가 이미 있습니다. 새 작업 경로를 지정하거나 기존 작업을 이어하세요.")
        private_directory(job.path)
        with job.open() as opened:
            opened.set_meta("input_hash", digest(inputs))
            write_json(opened.path / "input.json", inputs)
        return job

    @contextmanager
    def open(self):
        if not self.path.is_dir():
            raise CleanerError("작업 폴더가 없습니다. 먼저 preview를 실행하세요.")
        with lock(self.path / "job.lock"):
            db_path = self.path / "state.sqlite3"
            if db_path.is_symlink():
                raise CleanerError("작업 DB의 심볼릭 링크는 허용하지 않습니다.")
            fd = os.open(db_path, os.O_CREAT | os.O_RDWR | os.O_NOFOLLOW, 0o600)
            os.close(fd)
            os.chmod(db_path, 0o600)
            self.db = sqlite3.connect(db_path)
            self.db.row_factory = sqlite3.Row
            self.db.executescript("""
                PRAGMA synchronous=FULL;
                CREATE TABLE IF NOT EXISTS meta (key TEXT PRIMARY KEY, value TEXT NOT NULL);
                CREATE TABLE IF NOT EXISTS actions (
                    key TEXT PRIMARY KEY, status TEXT NOT NULL DEFAULT 'pending',
                    attempts INTEGER NOT NULL DEFAULT 0, next_at REAL NOT NULL DEFAULT 0,
                    error TEXT NOT NULL DEFAULT '', updated_at REAL NOT NULL DEFAULT 0
                );
                CREATE TABLE IF NOT EXISTS events (
                    seq INTEGER PRIMARY KEY, at REAL NOT NULL, action TEXT NOT NULL,
                    event TEXT NOT NULL, category TEXT NOT NULL
                );
            """)
            try:
                yield self
            finally:
                self.db.close()
                self.db = None

    def set_meta(self, key, value):
        with self.db:
            self.db.execute("INSERT OR REPLACE INTO meta VALUES (?, ?)", (key, canonical(value)))

    def get_meta(self, key, default=None):
        row = self.db.execute("SELECT value FROM meta WHERE key=?", (key,)).fetchone()
        return json.loads(row[0]) if row else default

    def read_bound(self, filename, expected_hash):
        path = self.path / filename
        if path.is_symlink():
            raise CleanerError("작업 파일의 심볼릭 링크는 허용하지 않습니다.")
        try:
            value = json.loads(path.read_text(encoding="utf-8"))
        except (OSError, ValueError):
            raise CleanerError("작업 파일을 읽지 못했습니다. preview/verify를 다시 실행하세요.") from None
        if not expected_hash or digest(value) != expected_hash:
            raise CleanerError("작업 입력/계획이 변경되었거나 기록과 일치하지 않습니다. 삭제를 차단합니다.")
        return value

    def inputs(self, check_source=True):
        inputs = self.read_bound("input.json", self.get_meta("input_hash"))
        if check_source:
            current = read_archive(inputs["archive"]["source"])
            if digest(current) != digest(inputs["archive"]):
                raise CleanerError("아카이브가 미리보기 이후 변경되었습니다. 새 preview 작업을 만드세요.")
        return inputs

    def plan(self):
        plan = self.read_bound("plan.json", self.get_meta("plan_hash"))
        if plan.get("input_hash") != self.get_meta("input_hash"):
            raise CleanerError("선택 목록과 삭제 계획이 일치하지 않습니다.")
        return plan

    def save_plan(self, plan):
        plan = {**plan, "input_hash": self.get_meta("input_hash")}
        write_json(self.path / "plan.json", plan)
        with self.db:
            self.db.execute("INSERT OR REPLACE INTO meta VALUES ('plan_hash', ?)", (canonical(digest(plan)),))
            for action in plan["actions"]:
                self.db.execute("INSERT OR IGNORE INTO actions (key) VALUES (?)", (action["key"],))

    def state(self, key):
        row = self.db.execute("SELECT * FROM actions WHERE key=?", (key,)).fetchone()
        if row is None:
            raise CleanerError("대상 실행 기록이 없습니다. verify를 다시 실행하세요.")
        return dict(row)

    def event(self, key, status, error="", next_at=0, increment=False):
        with self.db:
            self.db.execute("""UPDATE actions SET status=?, error=?, next_at=?,
                attempts=attempts+?, updated_at=? WHERE key=?""",
                (status, error, next_at, int(increment), time.time(), key))
            self.db.execute("INSERT INTO events (at, action, event, category) VALUES (?, ?, ?, ?)",
                            (time.time(), key, status, error))

    def has_started(self):
        return bool(self.db.execute("SELECT 1 FROM actions WHERE attempts>0 OR status!='pending' LIMIT 1").fetchone())

    def cooldown(self, kind):
        return self.get_meta("cooldown:" + kind, 0)

    def set_cooldown(self, kind, until):
        self.set_meta("cooldown:" + kind, max(until, self.cooldown(kind)))

    def reports(self):
        plan = self.plan()
        inputs = self.inputs(check_source=False)
        states = {a["key"]: self.state(a["key"]) for a in plan["actions"]}
        status_counts = {}
        for action in plan["actions"]:
            state = states[action["key"]]["status"]
            status_counts[state] = status_counts.get(state, 0) + 1
        records = inputs["archive"]["records"]
        summary = {"archive_total": len(records), "archive_owner_id": inputs["archive"]["owner_id"],
            "archive_username": inputs["archive"]["username"], "selected_input_count": len(inputs["selections"]),
            "selected_archive_count": len(plan["selected_ids"]),
            "untouched_archive_count": len(records) - len(plan["selected_ids"]),
            "keep_input_count": len(inputs["keep"]), "preserved_archive_count": len(plan["preserved_ids"]),
            "delete_target_count": len(plan["actions"]),
            "delete_post_count": sum(a["kind"] == "delete_post" and a["post_kind"] != "reply" for a in plan["actions"]),
            "delete_reply_count": sum(a["kind"] == "delete_post" and a["post_kind"] == "reply" for a in plan["actions"]),
            "unrepost_count": sum(a["kind"] == "unrepost" for a in plan["actions"]),
            "verified": plan["verified"], "execution_blocked": bool(plan["blocks"]),
            "execution_ready": plan["verified"] and not bool(plan["blocks"]),
            "blocks": plan["blocks"], "status_counts": status_counts,
            "last_error": self.get_meta("last_error"),
            "note": "선택하지 않은 게시물/댓글/리포스트는 대상에 추가하지 않습니다."}
        write_json(self.path / "summary.json", summary)
        targets = [{**a, "execution": states[a["key"]]} for a in plan["actions"]]
        write_json(self.path / "targets.json", targets)
        buffer = io.StringIO(newline="")
        writer = csv.DictWriter(buffer, fieldnames=["action", "archive_ids", "request_id", "post_kind",
            "link", "created_at", "text", "affected_edit_ids", "status", "attempts", "error"])
        writer.writeheader()
        for action in targets:
            writer.writerow({"action": action["kind"], "archive_ids": " ".join("'" + i for i in action["archive_ids"]),
                "request_id": "'" + action["request_id"], "post_kind": action["post_kind"],
                "link": "https://x.com/i/web/status/" + action["archive_ids"][0],
                "created_at": action["created_at"], "text": csv_text(action["text"]),
                "affected_edit_ids": " ".join("'" + i for i in action["edit_ids"]),
                "status": action["execution"]["status"], "attempts": action["execution"]["attempts"],
                "error": action["execution"]["error"]})
        atomic_write(self.path / "targets.csv", "\ufeff" + buffer.getvalue())
        rows = self.db.execute("SELECT * FROM events ORDER BY seq").fetchall()
        atomic_write(self.path / "events.jsonl", "".join(canonical({**dict(r),
            "utc": datetime.fromtimestamp(r["at"], timezone.utc).isoformat()}) + "\n" for r in rows))
        return summary


def csv_text(text):
    return "'" + text if text.lstrip().startswith(("=", "+", "-", "@")) else text
