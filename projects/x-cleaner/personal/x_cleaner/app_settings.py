"""Local GUI settings; secrets stay in a private, git-ignored directory."""
import json

from .auth import Settings
from .common import CleanerError, project_path, write_json

PROFILE = ".secrets/app-settings.json"


def read_profile():
    path = project_path(PROFILE)
    if not path.exists():
        return None
    if path.is_symlink() or path.stat().st_mode & 0o077:
        raise CleanerError("앱 설정 파일의 접근 권한이 안전하지 않습니다.")
    try:
        data = json.loads(path.read_text(encoding="utf-8"))
        if not isinstance(data, dict) or set(data) != {"client_id", "client_secret", "redirect_uri"}:
            raise ValueError
        if any(not isinstance(v, str) for v in data.values()):
            raise ValueError
        settings = Settings(**data)
        settings.validate()
        return settings
    except (ValueError, TypeError, OSError):
        raise CleanerError("앱 설정을 읽지 못했습니다. 설정 화면에서 다시 저장하세요.") from None


def save_profile(client_id, client_secret, redirect_uri):
    values = [client_id, client_secret, redirect_uri]
    if any(not isinstance(v, str) or "\n" in v or "\r" in v or "\x00" in v for v in values):
        raise CleanerError("설정 값은 한 줄의 문자열이어야 합니다.")
    settings = Settings(client_id.strip(), client_secret.strip(), redirect_uri.strip())
    settings.validate()
    write_json(project_path(PROFILE), vars(settings))
    return settings
