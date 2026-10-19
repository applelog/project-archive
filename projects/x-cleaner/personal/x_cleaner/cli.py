import argparse
import csv
import io
import json
import sqlite3
import sys
from contextlib import contextmanager
from datetime import datetime
from pathlib import Path
from uuid import uuid4

import httpx

from .api import XAPI
from .archive import read_archive
from .auth import Settings, TokenManager, authenticate
from .common import CleanerError, atomic_write, input_id, lock, project_path
from .job import Job, csv_text
from .workflow import make_plan, run, verify


def parser():
    root = argparse.ArgumentParser(description="선택한 내 게시물·댓글만 삭제하고 선택한 리포스트만 취소합니다. 기본은 오프라인 미리보기입니다.")
    commands = root.add_subparsers(dest="command", required=True)
    listing = commands.add_parser("list", help="아카이브 목록을 CSV로 출력합니다. API 호출 없음")
    listing.add_argument("--archive", required=True)
    listing.add_argument("--search", default="", help="본문 검색")
    listing.add_argument("--kind", choices=["all", "post", "reply", "repost"], default="all")
    listing.add_argument("--output", default="jobs/inventory.csv")
    preview = commands.add_parser("preview", help="명시적으로 선택한 항목만 미리보기. API 호출 없음")
    preview.add_argument("--archive", required=True)
    preview.add_argument("--delete-post", action="append", default=[], metavar="ID_OR_LINK", help="내 게시물(댓글·인용 포함) 선택. 여러 번 지정 가능")
    preview.add_argument("--delete-reply", action="append", default=[], metavar="ID_OR_LINK", help="내 댓글/답글만 선택")
    preview.add_argument("--delete-repost", action="append", default=[], metavar="ID_OR_LINK", help="내 리포스트 취소 선택. 원본 링크 또는 아카이브 리포스트 ID")
    preview.add_argument("--selection-file", help="kind,value 헤더가 있는 CSV. kind: post/reply/repost")
    preview.add_argument("--keep", action="append", default=[], metavar="ID_OR_LINK", help="추가 보호 ID. 삭제 선택과 충돌하면 보존 우선")
    preview.add_argument("--keep-file", help="한 줄당 보존 ID/링크를 기록한 텍스트 파일")
    preview.add_argument("--job", help="새 작업 폴더. 생략하면 jobs 아래 자동 생성")
    commands.add_parser("auth", help="X 공식 OAuth 2.0 PKCE 승인. 삭제하지 않음")
    commands.add_parser("gui", help="화면 앱 열기: API 설정·로그인·파일 선택·미리보기")
    check = commands.add_parser("verify", help="공식 API 읽기로 계정/대상 검증. 조회 비용 발생 가능")
    check.add_argument("--job", required=True)
    execute = commands.add_parser("run", help="기본 미리보기. --execute와 확인 입력이 있어야 삭제")
    execute.add_argument("--job", required=True)
    execute.add_argument("--execute", action="store_true")
    execute.add_argument("--resume", action="store_true")
    execute.add_argument("--retry-failed", action="store_true", help="실패/미확정 항목 명시적 재시도. --resume --execute 필수")
    return root


def read_text(path):
    try:
        return Path(path).expanduser().read_text(encoding="utf-8-sig")
    except (OSError, UnicodeError):
        raise CleanerError("선택/보존 입력 파일을 읽지 못했습니다.") from None


def selections(args):
    result = [{"kind": kind, "id": input_id(value)} for kind, values in
        [("post", args.delete_post), ("reply", args.delete_reply), ("repost", args.delete_repost)] for value in values]
    if args.selection_file:
        rows = csv.DictReader(io.StringIO(read_text(args.selection_file)))
        if rows.fieldnames != ["kind", "value"]:
            raise CleanerError("선택 CSV의 헤더는 정확히 kind,value 이어야 합니다.")
        for row in rows:
            if set(row) != {"kind", "value"} or row["kind"] not in {"post", "reply", "repost"} or not row["value"]:
                raise CleanerError("선택 CSV에 잘못된 종류/빈 값/추가 열이 있습니다.")
            result.append({"kind": row["kind"], "id": input_id(row["value"])})
    result = [{"kind": kind, "id": identifier} for kind, identifier in sorted({(r["kind"], r["id"]) for r in result})]
    if not result:
        raise CleanerError("삭제할 게시물/댓글 또는 취소할 리포스트를 명시적으로 선택하세요. 전체 삭제 기능은 없습니다.")
    keep = [input_id(value) for value in args.keep]
    if args.keep_file:
        keep.extend(input_id(line) for line in read_text(args.keep_file).splitlines() if line.strip() and not line.lstrip().startswith("#"))
    return result, sorted(set(keep))


def show(summary, path):
    print(f"아카이브 {summary['archive_total']}건 | 선택 {summary['selected_archive_count']}건 | 선택 밖 {summary['untouched_archive_count']}건")
    print(f"보존 {summary['preserved_archive_count']}건 | 게시물 삭제 {summary['delete_post_count']}건 | 댓글 삭제 {summary['delete_reply_count']}건 | 리포스트 취소 {summary['unrepost_count']}건")
    print("온라인 검증: " + ("완료" if summary["verified"] else "미완료 — verify 필요"))
    for reason in summary["blocks"]:
        print("차단: " + reason)
    if summary["status_counts"]:
        print("실행 상태: " + json.dumps(summary["status_counts"], ensure_ascii=False))
    print(f"보고서: {path}/summary.json, targets.json, targets.csv, events.jsonl")


def http_client():
    return httpx.Client(timeout=httpx.Timeout(30, connect=10), follow_redirects=False, trust_env=False)


@contextmanager
def api_session():
    settings = Settings.load()
    with lock(".secrets/token.lock"), http_client() as client:
        yield XAPI(client, TokenManager(settings, client))


def inventory(args):
    archive = read_archive(args.archive)
    records = [r for r in archive["records"] if (args.kind == "all" or r["kind"] == args.kind)
               and args.search.casefold() in r["text"].casefold()]
    stream = io.StringIO(newline="")
    writer = csv.DictWriter(stream, fieldnames=["id", "kind", "source_id", "created_at", "text", "link", "needs_online_kind_check"])
    writer.writeheader()
    for record in records:
        writer.writerow({"id": "'" + record["id"], "kind": record["kind"],
            "source_id": "'" + record["repost_of"] if record["repost_of"] else "",
            "created_at": record["created_at"], "text": csv_text(record["text"]),
            "link": "https://x.com/i/web/status/" + record["id"], "needs_online_kind_check": record["suspected_repost"]})
    output = project_path(args.output)
    if output.exists():
        raise CleanerError("목록 출력 파일이 이미 있습니다. 다른 --output 경로를 지정하세요.")
    atomic_write(output, "\ufeff" + stream.getvalue())
    for record in records[:30]:
        print(f"{record['id']} [{record['kind']}] {record['text'][:80].replace(chr(10), ' ')}")
    print(f"검색 결과 {len(records)}건을 {output} 에 저장했습니다. API 요청 없음.")


def execute(args):
    if args.command == "gui":
        from .gui import launch
        launch()
        return 0
    if args.command == "list":
        inventory(args)
        return 0
    if args.command == "preview":
        selected, keep = selections(args)
        inputs = {"version": 1, "archive": read_archive(args.archive), "selections": selected, "keep": keep}
        path = args.job or f"jobs/{datetime.now():%Y%m%d-%H%M%S}-{uuid4().hex[:8]}"
        job = Job.create(path, inputs)
        with job.open():
            job.save_plan(make_plan(inputs))
            summary = job.reports()
        show(summary, job.path)
        print("미리보기만 수행했습니다. API 요청 없음.")
        return 2 if summary["execution_blocked"] else 0
    if args.command == "auth":
        with http_client() as client:
            authenticate(Settings.load(), client)
        return 0
    if args.command == "run" and args.retry_failed and not (args.resume and args.execute):
        raise CleanerError("--retry-failed 는 --resume --execute 와 함께 사용하세요.")
    job = Job(args.job)
    with job.open():
        job.inputs()
        try:
            if args.command == "run" and not args.execute:
                summary = job.reports()
                print("미리보기만 수행했습니다. API 요청 없음.")
            elif args.command == "verify":
                print("공식 API 조회 검증입니다. 조회 비용이 발생할 수 있습니다. 삭제 요청은 보내지 않습니다.")
                with api_session() as api:
                    summary = verify(job, api)
                job.set_meta("last_error", None)
            else:
                plan = job.plan()
                if plan["blocks"] or not plan["verified"]:
                    raise CleanerError("온라인 검증이 미완료이거나 차단 사유가 있습니다. summary.json을 확인하세요.")
                with api_session() as api:
                    summary = run(job, api, resume=args.resume, retry_failed=args.retry_failed)
                job.set_meta("last_error", None)
        except CleanerError as error:
            job.set_meta("last_error", str(error))
            raise
        finally:
            # Regenerate reports after interruption/errors; SQLite is authoritative.
            job.reports()
        show(summary, job.path)
        if summary["execution_blocked"] or any(summary["status_counts"].get(s, 0) for s in ("failed", "uncertain", "in_flight", "retry_wait")):
            return 2
        return 0


def main(argv=None):
    args = parser().parse_args(argv)
    try:
        return execute(args)
    except CleanerError as error:
        print("중단: " + str(error), file=sys.stderr)
        return 2
    except (KeyboardInterrupt, EOFError):
        print("중단했습니다. 삭제 요청의 응답이 유실되었을 수 있습니다. 기록을 확인한 뒤 --resume --execute 로 이어하세요.", file=sys.stderr)
        return 130
    except (OSError, ValueError, TypeError, KeyError, sqlite3.Error):
        print("중단: 로컬 파일/작업 기록 형식 또는 저장 오류입니다. 원본과 작업 파일을 확인하세요. 비밀값 보호를 위해 원문 오류는 출력하지 않습니다.", file=sys.stderr)
        return 2
