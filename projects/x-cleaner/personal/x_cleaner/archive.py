"""Read known YTD archive files as JSON, never as executable JavaScript."""
import hashlib
import json
import re
import stat
import zipfile
from pathlib import Path, PurePosixPath

from .common import CleanerError, is_within, post_id

MAX_FILE_BYTES = 256 * 1024 * 1024
MAX_TOTAL_BYTES = 1024 * 1024 * 1024
FILE_NAME = re.compile(r"(account|tweets?|tweet-headers)(?:-part([0-9]+))?\.(js|json)$")
ASSIGNMENT = re.compile(r"\s*window\.YTD\.([A-Za-z_]+)\.part([0-9]+)\s*=\s*")


def parse_data(raw, family, part):
    try:
        text = raw.decode("utf-8-sig")
        if text.lstrip().startswith("["):
            payload = text.lstrip()
        else:
            match = ASSIGNMENT.match(text)
            allowed = {"tweet", "tweets"} if family == "tweet" else {family.replace("-", "_")}
            if not match or match.group(1) not in allowed or int(match.group(2)) != part:
                raise CleanerError("알 수 없는 아카이브 JavaScript 형식입니다. 실행하지 않고 차단합니다.")
            payload = text[match.end():]
        # JSON integer tokens are read directly as strings: no floating-point round trip.
        decoder = json.JSONDecoder(parse_int=str)
        data, end = decoder.raw_decode(payload)
        if payload[end:].strip() not in {"", ";"} or not isinstance(data, list):
            raise CleanerError("아카이브에는 JSON 배열과 선택적인 세미콜론만 허용합니다.")
        return data
    except (UnicodeError, ValueError, RecursionError):
        raise CleanerError("아카이브 JSON을 읽지 못했습니다. 손상된 파일을 확인하세요.") from None


def field_id(data, key="id", required=False):
    string_key = key + "_str"
    values = [post_id(data[k]) for k in (string_key, key) if data.get(k) not in (None, "")]
    if values and len(set(values)) != 1:
        raise CleanerError("아카이브의 숫자 ID와 문자열 ID가 일치하지 않습니다.")
    if required and not values:
        raise CleanerError("아카이브 게시물 ID가 누락되었습니다.")
    return values[0] if values else None


def normalize(row):
    if not isinstance(row, dict):
        raise CleanerError("게시물 항목은 JSON 객체여야 합니다.")
    data = row.get("tweet", row)
    if not isinstance(data, dict):
        raise CleanerError("tweet 항목이 올바르지 않습니다.")
    identifier = field_id(data, required=True)
    text = data.get("full_text", data.get("text", ""))
    if not isinstance(text, str):
        raise CleanerError("게시물 본문이 문자열이 아닙니다.")
    source = None
    if data.get("retweeted_status") is not None:
        if not isinstance(data["retweeted_status"], dict):
            raise CleanerError("리포스트 원본 정보가 올바르지 않습니다.")
        source = field_id(data["retweeted_status"], required=True)
    references = data.get("referenced_tweets", [])
    if not isinstance(references, list):
        raise CleanerError("게시물 참조 정보가 올바르지 않습니다.")
    for ref in references:
        if not isinstance(ref, dict):
            raise CleanerError("게시물 참조 정보가 올바르지 않습니다.")
        if ref.get("type") == "retweeted":
            reference_id = field_id(ref, required=True)
            if source and source != reference_id:
                raise CleanerError("리포스트 원본 ID가 충돌합니다.")
            source = reference_id
    reply = field_id(data, "in_reply_to_status_id")
    if not reply:
        reply = next((field_id(r, required=True) for r in references if r.get("type") == "replied_to"), None)
    edits = data.get("edit_history_tweet_ids", [identifier])
    if not isinstance(edits, list) or not edits:
        raise CleanerError("수정 이력 정보가 올바르지 않습니다.")
    edits = sorted(set(post_id(i) for i in edits) | {identifier})
    suspect = bool(re.match(r"^RT @[A-Za-z0-9_]+:", text))
    kind = "repost" if source or suspect else ("reply" if reply else "post")
    return {"id": identifier, "text": text, "created_at": str(data.get("created_at", "")),
            "kind": kind, "repost_of": source, "suspected_repost": suspect and not source,
            "reply_to": reply, "edit_ids": edits}


def _entries(path):
    """Yield only recognized data files. ZIP members are never extracted."""
    if path.is_file():
        try:
            with zipfile.ZipFile(path) as archive:
                seen = set()
                total = 0
                for info in archive.infolist():
                    member = PurePosixPath(info.filename)
                    if member.is_absolute() or ".." in member.parts or "\\" in info.filename:
                        raise CleanerError("ZIP에 안전하지 않은 경로가 있습니다.")
                    if stat.S_ISLNK(info.external_attr >> 16):
                        raise CleanerError("ZIP의 심볼릭 링크는 허용하지 않습니다.")
                    if not FILE_NAME.fullmatch(member.name):
                        continue
                    if info.filename in seen:
                        raise CleanerError("ZIP에 중복 데이터 파일이 있습니다.")
                    seen.add(info.filename)
                    total += info.file_size
                    if info.file_size > MAX_FILE_BYTES or total > MAX_TOTAL_BYTES:
                        raise CleanerError("아카이브 데이터 읽기 크기 제한을 초과했습니다.")
                    with archive.open(info) as stream:
                        raw = stream.read(MAX_FILE_BYTES + 1)
                    if len(raw) > MAX_FILE_BYTES:
                        raise CleanerError("아카이브 데이터 파일이 너무 큽니다.")
                    yield member, raw
        except (zipfile.BadZipFile, RuntimeError, OSError):
            raise CleanerError("ZIP을 읽지 못했습니다. 암호화/손상 여부를 확인하세요.") from None
    elif path.is_dir():
        total = 0
        for file in sorted(path.rglob("*")):
            if not FILE_NAME.fullmatch(file.name):
                continue
            if file.is_symlink() or not is_within(file.resolve(), path):
                raise CleanerError("아카이브 데이터의 심볼릭 링크는 허용하지 않습니다.")
            if not file.is_file():
                continue
            total += file.stat().st_size
            if file.stat().st_size > MAX_FILE_BYTES or total > MAX_TOTAL_BYTES:
                raise CleanerError("아카이브 데이터 읽기 크기 제한을 초과했습니다.")
            with file.open("rb") as stream:
                raw = stream.read(MAX_FILE_BYTES + 1)
            if len(raw) > MAX_FILE_BYTES:
                raise CleanerError("아카이브 데이터 파일이 너무 큽니다.")
            yield PurePosixPath(file.relative_to(path).as_posix()), raw
    else:
        raise CleanerError("아카이브 ZIP 또는 폴더를 찾을 수 없습니다.")


def read_archive(value):
    path = Path(value).expanduser().resolve()
    entries = list(_entries(path))
    if not entries:
        raise CleanerError("tweet.js / tweets.js와 계정 데이터 파일을 찾지 못했습니다.")
    parents = {p.parent for p, _ in entries}
    if len(parents) != 1:
        raise CleanerError("여러 데이터 폴더가 발견되었습니다. 한 계정의 아카이브만 입력하세요.")
    groups = {}
    hashes = {}
    for name, raw in entries:
        match = FILE_NAME.fullmatch(name.name)
        family = "tweet" if match[1] in {"tweet", "tweets"} else match[1]
        part = int(match[2] or 0)
        groups.setdefault(family, []).append((part, match[2] is not None, name, raw))
        hashes[str(name)] = hashlib.sha256(raw).hexdigest()
    decoded = {}
    for family, parts in groups.items():
        numbers = sorted(p[0] for p in parts)
        # X exports may call part 0 tweets.js and subsequent parts tweets-part1.js.
        # Duplicate base/part0 files and gaps are still rejected by numbering.
        if numbers != list(range(len(parts))):
            raise CleanerError("아카이브 분할 파일이 누락되거나 중복되었습니다.")
        decoded[family] = [row for part, _, _, raw in sorted(parts) for row in parse_data(raw, family, part)]
    if "tweet" not in decoded:
        raise CleanerError("게시물 본문 파일이 없습니다. tweet-headers만으로 삭제하지 않습니다.")
    records = {}
    for row in decoded["tweet"]:
        record = normalize(row)
        if record["id"] in records and records[record["id"]] != record:
            raise CleanerError("동일 게시물 ID의 데이터가 충돌합니다.")
        records[record["id"]] = record
    if "tweet-headers" in decoded:
        if any(not isinstance(row, dict) or not isinstance(row.get("tweet", row), dict)
               for row in decoded["tweet-headers"]):
            raise CleanerError("tweet-headers 항목 형식이 올바르지 않습니다.")
        headers = {field_id(row.get("tweet", row), required=True) for row in decoded["tweet-headers"]}
        if headers != set(records):
            raise CleanerError("tweet-headers와 본문 ID 목록이 다릅니다. 파일 누락을 확인하세요.")
    owners = set()
    usernames = set()
    for row in decoded.get("account", []):
        if not isinstance(row, dict) or not isinstance(row.get("account", row), dict):
            raise CleanerError("아카이브 계정 정보가 올바르지 않습니다.")
        account = row.get("account", row)
        owner = account.get("accountId", account.get("id_str", account.get("id")))
        if owner not in (None, ""):
            owners.add(post_id(owner))
        usernames.add(str(account.get("username", "")))
    if len(owners) > 1 or len(usernames - {""}) > 1:
        raise CleanerError("아카이브에 서로 다른 계정 정보가 있습니다.")
    return {"source": str(path), "fingerprint": hashes, "owner_id": next(iter(owners), None),
            "username": next(iter(usernames - {""}), ""), "records": sorted(records.values(), key=lambda r: int(r["id"]))}
