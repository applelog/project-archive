import fcntl
import hashlib
import json
import os
import re
import tempfile
from contextlib import contextmanager
from pathlib import Path
from urllib.parse import urlsplit

PROJECT_ROOT = Path(__file__).resolve().parent.parent


class CleanerError(Exception):
    """A safe, user-facing error. Never include credentials or raw HTTP bodies."""


def post_id(value):
    if isinstance(value, bool) or not isinstance(value, (str, int)):
        raise CleanerError("게시물/계정 ID는 정확한 정수 문자열이어야 합니다. 실수 ID는 거부합니다.")
    result = str(value)
    if not re.fullmatch(r"[1-9][0-9]{0,18}", result):
        raise CleanerError("ID는 0으로 시작하지 않는 1~19자리 숫자여야 합니다.")
    return result


def input_id(value):
    value = value.strip()
    if value.isascii() and value.isdigit():
        return post_id(value)
    url = urlsplit(value)
    if (url.scheme != "https" or url.netloc.lower() not in
            {"x.com", "www.x.com", "twitter.com", "www.twitter.com"}):
        raise CleanerError("X/Twitter HTTPS 게시물 링크 또는 ID를 입력하세요.")
    match = re.fullmatch(r"/(?:[A-Za-z0-9_]+/status|i/(?:web/)?status)/([0-9]+)(?:/(?:photo|video)/[0-9]+)?/?", url.path)
    if not match:
        raise CleanerError("게시물 링크의 /status/ID 형식을 확인하세요.")
    return post_id(match.group(1))


def canonical(value):
    return json.dumps(value, ensure_ascii=False, sort_keys=True, separators=(",", ":"))


def digest(value):
    return hashlib.sha256(canonical(value).encode()).hexdigest()


def project_path(path):
    candidate = Path(path).expanduser()
    if not candidate.is_absolute():
        candidate = PROJECT_ROOT / candidate
    candidate = Path(os.path.abspath(candidate))
    resolved = candidate.resolve()
    if resolved == PROJECT_ROOT or not is_within(resolved, PROJECT_ROOT):
        raise CleanerError("출력/작업 경로는 x-cleaner 프로젝트 내부의 하위 경로여야 합니다.")
    if not is_within(candidate, PROJECT_ROOT):
        raise CleanerError("출력 경로의 우회/심볼릭 링크는 허용하지 않습니다.")
    component = PROJECT_ROOT
    for part in candidate.relative_to(PROJECT_ROOT).parts:
        component = component / part
        if component.is_symlink():
            raise CleanerError("출력/작업 경로의 심볼릭 링크는 허용하지 않습니다.")
    return resolved


def is_within(path, root):
    """Path.is_relative_to equivalent compatible with the user's Python 3.8 env."""
    try:
        path.relative_to(root)
        return True
    except ValueError:
        return False


def private_directory(path):
    path.mkdir(parents=True, exist_ok=True, mode=0o700)
    os.chmod(path, 0o700)


def atomic_write(path, text):
    path = project_path(path)
    if path.is_symlink():
        raise CleanerError("출력 파일의 심볼릭 링크는 허용하지 않습니다.")
    private_directory(path.parent)
    fd, temporary = tempfile.mkstemp(dir=path.parent, prefix=".tmp-")
    try:
        with os.fdopen(fd, "w", encoding="utf-8", newline="") as stream:
            stream.write(text)
            stream.flush()
            os.fsync(stream.fileno())
        os.replace(temporary, path)
    finally:
        if os.path.exists(temporary):
            os.unlink(temporary)


def write_json(path, value):
    atomic_write(path, json.dumps(value, ensure_ascii=False, indent=2) + "\n")


@contextmanager
def lock(path):
    path = project_path(path)
    private_directory(path.parent)
    if path.is_symlink():
        raise CleanerError("잠금 파일의 심볼릭 링크는 허용하지 않습니다.")
    fd = os.open(path, os.O_CREAT | os.O_RDWR | os.O_NOFOLLOW, 0o600)
    try:
        try:
            fcntl.flock(fd, fcntl.LOCK_EX | fcntl.LOCK_NB)
        except BlockingIOError:
            raise CleanerError("같은 작업 또는 인증 정보를 사용하는 프로그램이 실행 중입니다.") from None
        yield
    finally:
        os.close(fd)
