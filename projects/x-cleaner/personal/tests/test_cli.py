import json

import pytest

from x_cleaner import cli
from x_cleaner.common import CleanerError, lock, project_path
from conftest import A, B, C, D


def test_preview_and_default_run_never_load_credentials(archive_dir, tmp_path, monkeypatch, capsys):
    def forbidden():
        pytest.fail("Offline commands attempted authentication")
    monkeypatch.setattr(cli, "api_session", forbidden)
    job = tmp_path / "jobs" / "offline"
    assert cli.main(["preview", "--archive", str(archive_dir), "--delete-post", A,
        "--delete-reply", B, "--delete-repost", C, "--job", str(job)]) == 0
    assert cli.main(["run", "--job", str(job)]) == 0
    summary = json.loads((job / "summary.json").read_text())
    assert summary["archive_total"] == 6 and summary["delete_target_count"] == 3
    assert D not in {i for a in json.loads((job / "targets.json").read_text()) for i in a["archive_ids"]}
    assert "API 요청 없음" in capsys.readouterr().out
    assert not (tmp_path / ".secrets").exists()


def test_empty_selection_is_never_delete_all(archive_dir, tmp_path):
    assert cli.main(["preview", "--archive", str(archive_dir), "--job", "jobs/empty"]) == 2
    assert not (tmp_path / "jobs" / "empty").exists()


def test_execution_without_verification_does_not_authenticate(archive_dir, tmp_path, monkeypatch):
    job = tmp_path / "jobs" / "offline"
    assert cli.main(["preview", "--archive", str(archive_dir), "--delete-post", A, "--job", str(job)]) == 0
    monkeypatch.setattr(cli, "api_session", lambda: pytest.fail("should not authenticate"))
    assert cli.main(["run", "--job", str(job), "--execute"]) == 2


def test_missing_keep_produces_blocked_report(archive_dir, tmp_path):
    assert cli.main(["preview", "--archive", str(archive_dir), "--delete-post", A,
        "--keep", "888888888", "--job", "jobs/missing"]) == 2
    summary = json.loads((tmp_path / "jobs/missing/summary.json").read_text())
    assert summary["execution_blocked"]
    assert "보존 ID" in summary["blocks"][0]


def test_selection_csv_and_keep_file(archive_dir, tmp_path):
    selections = tmp_path / "selection.csv"
    selections.write_text(f"kind,value\npost,{A}\nreply,https://x.com/test/status/{B}\nrepost,{C}\n")
    keep = tmp_path / "keep.txt"
    keep.write_text(A + "\n")
    assert cli.main(["preview", "--archive", str(archive_dir), "--selection-file", str(selections),
        "--keep-file", str(keep), "--job", "jobs/selection"]) == 0
    summary = json.loads((tmp_path / "jobs/selection/summary.json").read_text())
    assert summary["delete_target_count"] == 2 and summary["preserved_archive_count"] == 1


def test_inventory_search_is_offline(archive_dir, tmp_path):
    assert cli.main(["list", "--archive", str(archive_dir), "--kind", "reply", "--output", "jobs/replies.csv"]) == 0
    text = (tmp_path / "jobs/replies.csv").read_text(encoding="utf-8-sig")
    assert B in text and A not in text
    assert not (tmp_path / ".secrets").exists()


def test_job_lock_prevents_concurrent_run(tmp_path):
    path = tmp_path / "jobs/test/job.lock"
    with lock(path):
        with pytest.raises(CleanerError, match="실행 중"):
            with lock(path):
                pass


def test_output_cannot_escape_project(tmp_path):
    with pytest.raises(CleanerError, match="프로젝트 내부"):
        project_path(tmp_path.parent / "escape")


def test_internal_symlink_cannot_redirect_tokens_to_tracked_file(tmp_path):
    target = tmp_path / "README.md"
    target.write_text("must remain intact")
    secrets = tmp_path / ".secrets"
    secrets.mkdir()
    (secrets / "tokens.json").symlink_to(target)
    with pytest.raises(CleanerError, match="심볼릭"):
        project_path(".secrets/tokens.json")
    assert target.read_text() == "must remain intact"
