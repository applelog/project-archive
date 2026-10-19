"""Download KRIC stationMovement JSON over HTTPS using KRIC_API_KEY from .env."""

import argparse
import json
import os
import ssl
import stat
import urllib.error
import urllib.parse
import urllib.request
from pathlib import Path

from import_seoul import write_json_atomic


class NoRedirect(urllib.request.HTTPRedirectHandler):
    def redirect_request(self, request, fp, code, message, headers, newurl):
        raise ValueError("철도 API가 다른 주소로 이동했습니다")


def read_key(path):
    mode = path.stat().st_mode
    if mode & (stat.S_IRWXG | stat.S_IRWXO):
        raise ValueError(".env 권한을 600으로 제한하세요: chmod 600 .env")
    for line in path.read_text(encoding="utf-8").splitlines():
        if line.startswith("KRIC_API_KEY="):
            value = line.split("=", 1)[1].strip()
            if len(value) >= 2 and value[0] == value[-1] and value[0] in "\"'":
                value = value[1:-1]
            if value:
                return value
    raise ValueError(".env에 KRIC_API_KEY가 없습니다")


def fetch(key, operator, line, station, next_station):
    query = urllib.parse.urlencode({
        "serviceKey": key, "format": "json", "railOprIsttCd": operator,
        "lnCd": line, "stinCd": station, "nextStinCd": next_station,
    })
    url = "https://openapi.kric.go.kr/openapi/handicapped/stationMovement?" + query
    opener = urllib.request.build_opener(
        urllib.request.HTTPSHandler(context=ssl.create_default_context()), NoRedirect()
    )
    try:
        with opener.open(url, timeout=20) as response:
            if response.status != 200 or response.headers.get_content_type() not in ("application/json", "text/json"):
                raise ValueError("철도 API 응답 형식이 올바르지 않습니다")
            body = response.read(2_000_001)
            if len(body) > 2_000_000:
                raise ValueError("철도 API 응답이 너무 큽니다")
            return json.loads(body)
    except (urllib.error.URLError, urllib.error.HTTPError):
        # urllib exceptions can contain the URL, including the secret key.
        raise ValueError("철도 API 접속에 실패했습니다. 키 승인 상태와 역 코드를 확인하세요") from None


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--env", type=Path, default=Path(".env"))
    parser.add_argument("--operator-code", required=True)
    parser.add_argument("--line-code", required=True)
    parser.add_argument("--station-code", required=True)
    parser.add_argument("--next-station-code", required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    key = read_key(args.env)
    for code in (args.operator_code, args.line_code, args.station_code, args.next_station_code):
        if not code or len(code) > 20 or not all(character.isalnum() or character == "-" for character in code):
            raise ValueError("역 코드 형식이 올바르지 않습니다")
    result = fetch(key, args.operator_code, args.line_code, args.station_code, args.next_station_code)
    write_json_atomic(args.output, result)
    print("철도 API 응답을 로컬 파일에 저장했습니다. 공식 도면과 대조한 뒤 가져오세요.")


if __name__ == "__main__":
    try:
        main()
    except (OSError, ValueError) as exc:
        raise SystemExit("철도 자료 요청 실패: " + str(exc))
