import base64
import hashlib
import json
import os
from urllib.parse import parse_qs

import httpx
import pytest

from x_cleaner.auth import Settings, TokenManager, SCOPES, pkce, callback_code
from x_cleaner.common import CleanerError, project_path, write_json
from conftest import Clock, OWNER


def token_response(**overrides):
    return {"token_type": "bearer", "access_token": "secret-access", "refresh_token": "secret-refresh",
        "expires_in": 7200, "scope": " ".join(sorted(SCOPES)), **overrides}


def test_pkce_is_s256_random_and_has_no_extra_scopes():
    verifier, challenge, state = pkce()
    assert challenge == base64.urlsafe_b64encode(hashlib.sha256(verifier.encode()).digest()).decode().rstrip("=")
    assert 43 <= len(verifier) <= 128 and len(state) >= 32
    assert pkce() != (verifier, challenge, state)
    assert not any(scope.startswith(("dm.", "follows.", "like.")) for scope in SCOPES)


@pytest.mark.parametrize("target", ["/callback?state=wrong&code=secret", "/callback?state=ok&state=ok&code=secret", "/callback?state=ok", "/wrong?state=ok&code=secret", "/callback?state=ok&error=access_denied", "/callback?state=ok&code=one&code=two"])
def test_callback_validation_does_not_expose_codes(target):
    with pytest.raises(CleanerError) as failure:
        callback_code(target, "ok")
    assert "secret" not in str(failure.value)


def test_callback_accepts_matching_state():
    assert callback_code("/callback?state=ok&code=secret-code", "ok") == "secret-code"


@pytest.mark.parametrize("uri", ["https://evil.example/callback", "http://localhost:8765/callback", "http://0.0.0.0:8765/callback", "http://127.0.0.1:8765/wrong", "http://user:secret@127.0.0.1:8765/callback", "http://127.0.0.1:8765/callback?code=x"])
def test_callback_only_binds_loopback(uri):
    with pytest.raises(CleanerError):
        Settings("client-id", redirect_uri=uri).validate()


def test_public_exchange_refresh_rotation_and_permissions(tmp_path):
    calls = []
    responses = [token_response(), token_response(access_token="rotated-access", refresh_token="rotated-refresh")]

    def handle(request):
        calls.append(request)
        assert request.url == "https://api.x.com/2/oauth2/token"
        assert "authorization" not in request.headers
        assert parse_qs(request.content.decode())["client_id"] == ["client-id"]
        return httpx.Response(200, json=responses.pop(0))

    clock = Clock()
    with httpx.Client(transport=httpx.MockTransport(handle)) as client:
        manager = TokenManager(Settings("client-id"), client, clock.time)
        manager.exchange({"grant_type": "authorization_code", "code": "secret-code", "code_verifier": "secret-verifier", "redirect_uri": "http://127.0.0.1:8765/callback"})
        manager.remember_account(OWNER)
        path = tmp_path / ".secrets/tokens.json"
        assert path.stat().st_mode & 0o777 == 0o600
        assert path.parent.stat().st_mode & 0o777 == 0o700
        clock.now += 7200
        assert manager.access_token() == "rotated-access"
        saved = json.loads(path.read_text())
        assert saved["refresh_token"] == "rotated-refresh"
        assert saved["account_id"] == OWNER
        assert manager.generation == 2
        assert "code" not in saved and "code_verifier" not in saved
        second = parse_qs(calls[1].content.decode())
        assert second["refresh_token"] == ["secret-refresh"]
        loaded = TokenManager(Settings("client-id"), client, clock.time)
        assert loaded.access_token() == "rotated-access"
        assert len(calls) == 2


def test_confidential_client_uses_basic_auth():
    calls = []
    def handle(request):
        calls.append(request)
        assert request.headers["authorization"].startswith("Basic ")
        assert "client_id" not in parse_qs(request.content.decode())
        return httpx.Response(200, json=token_response())
    with httpx.Client(transport=httpx.MockTransport(handle)) as client:
        TokenManager(Settings("client-id", "secret-client"), client).exchange({"grant_type": "authorization_code", "code": "secret-code"})
    assert len(calls) == 1


@pytest.mark.parametrize("response", [httpx.Response(401, json={"detail": "secret-client"}), httpx.Response(200, json=token_response(scope="tweet.read")), httpx.Response(200, json=token_response(expires_in="nan")), httpx.Response(200, json=token_response(refresh_token=""))])
def test_bad_token_response_never_saved_or_logged(tmp_path, response, capsys):
    count = []
    def handle(request):
        count.append(request)
        return response
    with httpx.Client(transport=httpx.MockTransport(handle)) as client:
        with pytest.raises(CleanerError) as failure:
            TokenManager(Settings("client-id"), client).exchange({"grant_type": "authorization_code", "code": "secret-code"})
    assert len(count) == 1
    assert not (tmp_path / ".secrets/tokens.json").exists()
    assert "secret" not in str(failure.value)
    assert "secret" not in capsys.readouterr().out


def test_token_file_rejects_wrong_app_and_world_readable_permissions(tmp_path):
    clock = Clock()
    data = {"client_id": "another-client", "access_token": "secret", "refresh_token": "secret-refresh", "scope": " ".join(SCOPES), "expires_at": clock.now + 3600}
    write_json(project_path(".secrets/tokens.json"), data)
    with httpx.Client(transport=httpx.MockTransport(lambda _: pytest.fail("network"))) as client:
        with pytest.raises(CleanerError, match="Client ID"):
            TokenManager(Settings("client-id"), client, clock.time).access_token()
        path = tmp_path / ".secrets/tokens.json"
        os.chmod(path, 0o644)
        with pytest.raises(CleanerError, match="접근 권한"):
            TokenManager(Settings("client-id"), client, clock.time).access_token()


def test_token_account_cannot_silently_change():
    with httpx.Client(transport=httpx.MockTransport(lambda _: pytest.fail("network"))) as client:
        manager = TokenManager(Settings("client-id"), client)
        manager.data = {"account_id": OWNER}
        with pytest.raises(CleanerError, match="계정"):
            manager.remember_account("777")
