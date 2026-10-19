import json
import os

import pytest

from x_cleaner import common
from x_cleaner.app_settings import read_profile, save_profile
from x_cleaner.auth import Settings
from x_cleaner.common import CleanerError
from x_cleaner.gui import preview_selection, UIAPI


def test_gui_settings_saved_privately_and_used_by_cli(tmp_path, monkeypatch):
    monkeypatch.setattr(common, "PROJECT_ROOT", tmp_path)
    settings = save_profile("public-client", "mock-secret", "http://127.0.0.1:8765/callback")
    path = tmp_path / ".secrets/app-settings.json"
    assert path.stat().st_mode & 0o777 == 0o600
    assert path.parent.stat().st_mode & 0o777 == 0o700
    assert read_profile() == settings
    assert Settings.load() == settings
    assert not (tmp_path / ".env").exists()


@pytest.mark.parametrize("value", ["http://evil.example/callback", "https://127.0.0.1:8765/callback", "http://127.0.0.1:8765/other"])
def test_gui_settings_reject_non_loopback_callback(tmp_path, monkeypatch, value):
    monkeypatch.setattr(common, "PROJECT_ROOT", tmp_path)
    with pytest.raises(CleanerError):
        save_profile("client", "", value)
    assert not (tmp_path / ".secrets/app-settings.json").exists()


def test_gui_config_injection_and_unsafe_permissions_rejected(tmp_path, monkeypatch):
    monkeypatch.setattr(common, "PROJECT_ROOT", tmp_path)
    with pytest.raises(CleanerError):
        save_profile("client\nX_OTHER=bad", "", "http://127.0.0.1:8765/callback")
    save_profile("client", "", "http://127.0.0.1:8765/callback")
    path = tmp_path / ".secrets/app-settings.json"
    os.chmod(path, 0o644)
    with pytest.raises(CleanerError):
        read_profile()


def test_gui_selection_never_adds_unselected_records(archive_dir):
    from x_cleaner.archive import read_archive
    archive = read_archive(archive_dir)
    selected = archive["records"][0]["id"]
    inputs = preview_selection(archive, {selected}, "")
    assert [s["id"] for s in inputs["selections"]] == [selected]
    with pytest.raises(CleanerError):
        preview_selection(archive, set(), "")
    with pytest.raises(CleanerError):
        preview_selection(archive, {"123"}, "")


def test_gui_stop_blocks_network_before_next_request():
    import threading
    event = threading.Event()
    event.set()
    api = UIAPI(None, None, event, lambda _: None)
    with pytest.raises(CleanerError):
        api._send("DELETE", "/tweets/123", "mock-token")
    with pytest.raises(CleanerError):
        api.pause(300)
