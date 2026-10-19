import json
import stat
import zipfile

import pytest

from x_cleaner.archive import read_archive, parse_data
from x_cleaner.common import CleanerError, input_id, post_id
from conftest import A, OWNER, write_archive


def test_folder_and_zip_identical_ids(archive_dir, tmp_path):
    folder = read_archive(archive_dir)
    zip_path = tmp_path / "export.zip"
    with zipfile.ZipFile(zip_path, "w") as archive:
        for path in archive_dir.rglob("*.js"):
            archive.write(path, "export/" + str(path.relative_to(archive_dir)))
    zipped = read_archive(zip_path)
    assert folder["records"] == zipped["records"]
    assert zipped["owner_id"] == OWNER
    assert folder["records"][0]["id"] == A
    assert isinstance(folder["records"][0]["id"], str)


def test_integer_over_2_to_53_is_exact(tmp_path):
    path = write_archive(tmp_path / "large", [{"tweet": {"id": int(A), "full_text": "exact"}}])
    assert read_archive(path)["records"][0]["id"] == A


@pytest.mark.parametrize("value", [9007199254740992.0, True, "9.007199254740993e15", "001", "0", "1" * 20])
def test_imprecise_or_invalid_id_rejected(value):
    with pytest.raises(CleanerError):
        post_id(value)


def test_conflicting_id_str_blocks(tmp_path):
    path = write_archive(tmp_path / "conflict", [{"tweet": {"id_str": A, "id": int(A) + 1}}])
    with pytest.raises(CleanerError, match="일치"):
        read_archive(path)


def test_split_parts_missing_and_duplicate_block(archive_dir):
    data = archive_dir / "data"
    original = (data / "tweets.js").read_text()
    (data / "tweets.js").unlink()
    (data / "tweets-part0.js").write_text(original)
    (data / "tweets-part2.js").write_text(original.replace("part0", "part2"))
    with pytest.raises(CleanerError, match="분할"):
        read_archive(archive_dir)


def test_split_parts_parse(archive_dir):
    data = archive_dir / "data"
    rows = json.loads((data / "tweets.js").read_text().split("=", 1)[1].strip().rstrip(";"))
    (data / "tweets.js").unlink()
    for index, part in enumerate((rows[:3], rows[3:])):
        (data / f"tweets-part{index}.js").write_text(f"window.YTD.tweets.part{index} = " + json.dumps(part))
    assert len(read_archive(archive_dir)["records"]) == 6


def test_base_file_plus_numbered_parts_parse(archive_dir):
    data = archive_dir / "data"
    rows = json.loads((data / "tweets.js").read_text().split("=", 1)[1].strip().rstrip(";"))
    (data / "tweets.js").write_text("window.YTD.tweets.part0 = " + json.dumps(rows[:3]))
    (data / "tweets-part1.js").write_text("window.YTD.tweets.part1 = " + json.dumps(rows[3:]))
    assert len(read_archive(archive_dir)["records"]) == 6


def test_duplicate_base_and_part0_blocked(archive_dir):
    data = archive_dir / "data"
    (data / "tweets-part0.js").write_text((data / "tweets.js").read_text())
    with pytest.raises(CleanerError, match="分割|분할"):
        read_archive(archive_dir)


@pytest.mark.parametrize("payload", [b"window.YTD.tweets.part0 = []; require('evil')", b"eval([])", b"{not-json}"])
def test_javascript_never_executed(payload):
    with pytest.raises(CleanerError):
        parse_data(payload, "tweet", 0)


def test_header_mismatch_blocks(archive_dir):
    (archive_dir / "data" / "tweet-headers.js").write_text('window.YTD.tweet_headers.part0 = [{"tweet":{"id":"123"}}]')
    with pytest.raises(CleanerError, match="tweet-headers"):
        read_archive(archive_dir)


def test_conflicting_duplicate_block(tmp_path):
    path = write_archive(tmp_path / "duplicates", [{"tweet": {"id_str": A, "text": "first"}}, {"tweet": {"id_str": A, "text": "second"}}])
    with pytest.raises(CleanerError, match="충돌"):
        read_archive(path)


@pytest.mark.parametrize("entry", ["../tweets.js", "/tweets.js", "data\\tweets.js"])
def test_zip_traversal_blocked(tmp_path, entry):
    path = tmp_path / "bad.zip"
    with zipfile.ZipFile(path, "w") as archive:
        archive.writestr(entry, "[]")
    with pytest.raises(CleanerError, match="경로"):
        read_archive(path)


def test_zip_symlink_blocked(tmp_path):
    path = tmp_path / "link.zip"
    info = zipfile.ZipInfo("data/tweets.js")
    info.create_system = 3
    info.external_attr = (stat.S_IFLNK | 0o777) << 16
    with zipfile.ZipFile(path, "w") as archive:
        archive.writestr(info, "/etc/passwd")
    with pytest.raises(CleanerError, match="심볼릭"):
        read_archive(path)


def test_no_account_is_visible_but_cannot_verify(tmp_path):
    path = write_archive(tmp_path / "no-account", account=False)
    assert read_archive(path)["owner_id"] is None


@pytest.mark.parametrize("link", [A, f"https://x.com/user/status/{A}?s=20", f"https://twitter.com/user/status/{A}/photo/1", f"https://x.com/i/web/status/{A}"])
def test_links_and_id(link):
    assert input_id(link) == A


@pytest.mark.parametrize("link", [f"https://evil.example/status/{A}", f"https://x.com.evil/status/{A}", f"https://user:secret@x.com/user/status/{A}", "https://x.com/user", "http://x.com/user/status/123"])
def test_bad_links_blocked(link):
    with pytest.raises(CleanerError):
        input_id(link)
