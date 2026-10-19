import base64
import hashlib
import json
import math
import os
import secrets
import time
import webbrowser
from dataclasses import dataclass
from http.server import BaseHTTPRequestHandler, HTTPServer
from urllib.parse import parse_qs, urlencode, urlsplit

import httpx
from dotenv import load_dotenv

from .common import CleanerError, PROJECT_ROOT, lock, post_id, project_path, write_json

SCOPES = {"tweet.read", "users.read", "tweet.write", "offline.access"}
TOKEN_URL = "https://api.x.com/2/oauth2/token"
AUTHORIZE_URL = "https://x.com/i/oauth2/authorize"


@dataclass
class Settings:
    client_id: str
    client_secret: str = ""
    redirect_uri: str = "http://127.0.0.1:8765/callback"

    @classmethod
    def load(cls):
        # GUI settings can be used by the existing CLI without copying secrets to code.
        from .app_settings import read_profile
        profile = read_profile()
        if profile is not None:
            return profile
        env = PROJECT_ROOT / ".env"
        if env.is_symlink():
            raise CleanerError(".env의 심볼릭 링크는 허용하지 않습니다.")
        load_dotenv(env, override=False)
        config = cls(os.getenv("X_CLIENT_ID", ""), os.getenv("X_CLIENT_SECRET", ""),
                     os.getenv("X_REDIRECT_URI", "http://127.0.0.1:8765/callback"))
        config.validate()
        return config

    def validate(self):
        if not self.client_id or any(c.isspace() for c in self.client_id):
            raise CleanerError("X_CLIENT_ID를 환경변수 또는 .env에 설정하세요.")
        try:
            uri = urlsplit(self.redirect_uri)
            valid = (uri.scheme == "http" and uri.hostname == "127.0.0.1" and
                     uri.port is not None and uri.path == "/callback" and
                     not uri.query and not uri.fragment and not uri.username and not uri.password)
        except ValueError:
            valid = False
        if not valid:
            raise CleanerError("콜백 주소는 http://127.0.0.1:포트/callback 형식이어야 합니다.")


def pkce():
    verifier = secrets.token_urlsafe(48)
    challenge = base64.urlsafe_b64encode(hashlib.sha256(verifier.encode("ascii")).digest()).decode().rstrip("=")
    return verifier, challenge, secrets.token_urlsafe(32)


def callback_code(target, state):
    uri = urlsplit(target)
    query = parse_qs(uri.query, keep_blank_values=True)
    states = query.get("state", [])
    if uri.path != "/callback" or len(states) != 1 or not secrets.compare_digest(states[0], state):
        raise CleanerError("OAuth 콜백 state 검증에 실패했습니다.")
    if query.get("error"):
        raise CleanerError("X 앱 승인이 거부되었습니다.")
    codes = query.get("code", [])
    if len(codes) != 1 or not codes[0]:
        raise CleanerError("OAuth 승인 코드가 없습니다.")
    return codes[0]


class TokenManager:
    """Use under the token lock; never print or return raw token responses to CLI."""

    def __init__(self, settings, client, clock=time.time):
        self.settings = settings
        self.client = client
        self.clock = clock
        self.path = project_path(".secrets/tokens.json")
        self.generation = 0
        self.data = None

    def load(self):
        if self.path.is_symlink():
            raise CleanerError("인증 파일의 심볼릭 링크는 허용하지 않습니다.")
        try:
            if self.path.stat().st_mode & 0o077:
                raise CleanerError("토큰 파일의 접근 권한이 너무 넓습니다. chmod 600 .secrets/tokens.json 을 실행하세요.")
            data = json.loads(self.path.read_text(encoding="utf-8"))
        except (OSError, ValueError):
            raise CleanerError("인증 정보가 없습니다/손상되었습니다. 먼저 auth 명령을 실행하세요.") from None
        if not isinstance(data, dict) or data.get("client_id") != self.settings.client_id:
            raise CleanerError("저장된 토큰의 앱과 현재 Client ID가 일치하지 않습니다. 다시 인증하세요.")
        try:
            if (not isinstance(data.get("access_token"), str) or not data["access_token"] or
                    not isinstance(data.get("refresh_token"), str) or not data["refresh_token"] or
                    not SCOPES.issubset(set(data["scope"].split())) or
                    not math.isfinite(float(data["expires_at"]))):
                raise ValueError
            if data.get("account_id"):
                post_id(data["account_id"])
        except (KeyError, TypeError, ValueError):
            raise CleanerError("저장된 토큰/권한 정보가 올바르지 않습니다. 다시 인증하세요.") from None
        self.data = data

    def exchange(self, payload):
        auth = None
        if self.settings.client_secret:
            auth = httpx.BasicAuth(self.settings.client_id, self.settings.client_secret)
        else:
            payload = {**payload, "client_id": self.settings.client_id}
        try:
            response = self.client.post(TOKEN_URL, data=payload, auth=auth)
        except httpx.HTTPError:
            raise CleanerError("OAuth 토큰 요청 연결 오류입니다. 승인/갱신 결과가 불명확하므로 재인증하세요.") from None
        if response.status_code != 200:
            raise CleanerError(f"OAuth 토큰 요청이 실패했습니다(HTTP {response.status_code}). 반복 요청하지 않습니다.")
        try:
            token = response.json()
            previous = self.data or {}
            if (not isinstance(token, dict) or token.get("token_type", "").lower() != "bearer" or
                    not isinstance(token.get("access_token"), str) or not token["access_token"]):
                raise ValueError
            scope = token.get("scope", previous.get("scope", ""))
            refresh = token.get("refresh_token", previous.get("refresh_token"))
            duration = float(token["expires_in"])
            if (not isinstance(scope, str) or not SCOPES.issubset(set(scope.split())) or
                    not isinstance(refresh, str) or not refresh or not math.isfinite(duration) or duration <= 0):
                raise ValueError
            data = {"client_id": self.settings.client_id, "access_token": token["access_token"],
                    "refresh_token": refresh, "scope": scope, "expires_at": self.clock() + duration,
                    "account_id": previous.get("account_id")}
        except (ValueError, TypeError, KeyError):
            raise CleanerError("OAuth 토큰 응답 또는 필수 권한이 올바르지 않습니다. 다시 인증하세요.") from None
        # Persist a rotated refresh token before making any further API calls.
        write_json(self.path, data)
        self.data = data
        self.generation += 1

    def access_token(self):
        if self.data is None:
            self.load()
        if self.clock() >= float(self.data["expires_at"]) - 60:
            self.exchange({"grant_type": "refresh_token", "refresh_token": self.data["refresh_token"]})
        return self.data["access_token"]

    def remember_account(self, identifier):
        identifier = post_id(identifier)
        if self.data.get("account_id") not in (None, identifier):
            raise CleanerError("인증 토큰의 계정이 이전 계정과 달라졌습니다. 중단합니다.")
        if self.data.get("account_id") != identifier:
            self.data["account_id"] = identifier
            write_json(self.path, self.data)


def authenticate(settings, client, notify=print, open_browser=webbrowser.open, timeout=300):
    settings.validate()
    verifier, challenge, state = pkce()
    uri = urlsplit(settings.redirect_uri)
    result = {}

    class Handler(BaseHTTPRequestHandler):
        def log_message(self, *_):
            pass  # Callback URLs contain auth codes: never log request lines.

        def do_GET(self):
            try:
                code = callback_code(self.path, state)
            except CleanerError:
                self.send_response(400)
                self.send_header("Cache-Control", "no-store")
                self.end_headers()
                self.wfile.write(b"Authorization callback rejected. Return to the terminal.")
                # A valid state with an OAuth error terminates authorization.
                query = parse_qs(urlsplit(self.path).query)
                if query.get("state") == [state] and query.get("error"):
                    result["denied"] = True
                return
            result["code"] = code
            self.send_response(200)
            self.send_header("Cache-Control", "no-store")
            self.send_header("Content-Type", "text/plain; charset=utf-8")
            self.end_headers()
            self.wfile.write("승인되었습니다. X Cleaner 앱으로 돌아가세요.".encode())

    with lock(".secrets/token.lock"):
        try:
            server = HTTPServer(("127.0.0.1", uri.port), Handler)
        except OSError:
            raise CleanerError("로컬 콜백 포트를 열지 못했습니다. 다른 실행/포트 사용을 확인하세요.") from None
        with server:
            server.timeout = 1
            url = AUTHORIZE_URL + "?" + urlencode({"response_type": "code", "client_id": settings.client_id,
                "redirect_uri": settings.redirect_uri, "scope": " ".join(sorted(SCOPES)), "state": state,
                "code_challenge": challenge, "code_challenge_method": "S256"})
            notify("X 공식 승인 화면을 엽니다. 열리지 않으면 아래 주소를 브라우저에서 여세요.")
            notify(url)
            open_browser(url)
            deadline = time.monotonic() + timeout
            while not result and time.monotonic() < deadline:
                server.handle_request()
        if not result or result.get("denied"):
            raise CleanerError("앱 승인이 거부되었거나 대기 시간이 끝났습니다.")
        manager = TokenManager(settings, client)
        manager.exchange({"grant_type": "authorization_code", "code": result["code"],
                          "redirect_uri": settings.redirect_uri, "code_verifier": verifier})
    notify("인증 정보를 .secrets/tokens.json 에 저장했습니다. 게시물을 삭제하지 않았습니다.")
