"""Import a reviewed KRIC entrance-to-platform route, never transfer routes."""

import argparse
import json
from datetime import date
from pathlib import Path

from import_seoul import write_json_atomic


def extract_steps(payload, route_id):
    records = []

    def collect(value):
        if isinstance(value, dict):
            if "mvContDtl" in value and "exitMvTpOrdr" in value:
                records.append(value)
                return
            for child in value.values():
                collect(child)
        elif isinstance(value, list):
            for child in value:
                collect(child)

    collect(payload)
    selected = [row for row in records if str(row.get("mvPathMgNo", "")) == route_id]
    if not selected:
        raise ValueError("선택한 공식 이동경로 번호가 응답에 없습니다")
    steps = []
    for row in selected:
        text = str(row.get("mvContDtl") or "").strip()
        if not text or len(text) > 1000:
            raise ValueError("이동 안내 문구가 비어 있거나 너무 깁니다")
        from_name = str(row.get("stMovePath") or "").strip()
        to_name = str(row.get("edMovePath") or "").strip()
        if any(word in part for part in (text, from_name, to_name) for word in ("에스컬레이터", "계단")):
            raise ValueError("선택한 경로에 계단 또는 에스컬레이터 안내가 있습니다")
        try:
            order = int(row["exitMvTpOrdr"])
        except (TypeError, ValueError):
            raise ValueError("공식 이동 단계 순서가 올바르지 않습니다") from None
        if order < 1:
            raise ValueError("공식 이동 단계 순서가 올바르지 않습니다")
        steps.append({
            "order": order, "text": text,
            "from": from_name,
            "to": to_name,
        })
    steps.sort(key=lambda item: item["order"])
    if len({step["order"] for step in steps}) != len(steps):
        raise ValueError("공식 이동 단계 순서가 중복됐습니다")
    if not any("엘리베이터" in step["text"] or "승강기" in step["text"] for step in steps):
        raise ValueError("선택한 경로에서 엘리베이터 사용을 확인할 수 없습니다")
    return steps


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--network", type=Path, default=Path("Data/network.json"))
    parser.add_argument("--guil-json", type=Path, required=True)
    parser.add_argument("--guil-route-id", required=True)
    parser.add_argument("--guil-entrance-node", required=True)
    parser.add_argument("--oryu-json", type=Path, required=True)
    parser.add_argument("--oryu-route-id", required=True)
    parser.add_argument("--oryu-entrance-node", required=True)
    parser.add_argument("--reviewed-at", required=True, help="공식 도면과 대조한 날짜 YYYY-MM-DD")
    parser.add_argument("--output", type=Path, default=Path("Data/station_steps.json"))
    args = parser.parse_args()
    date.fromisoformat(args.reviewed_at)
    network = json.loads(args.network.read_text(encoding="utf-8"))
    entrance_ids = {item["nodeID"] for item in network.get("entrances", [])}
    routes = []
    for direction, title, path, route_id, entrance in [
        ("guil", "구일·서울 방면", args.guil_json, args.guil_route_id, args.guil_entrance_node),
        ("oryu", "오류동·인천 방면", args.oryu_json, args.oryu_route_id, args.oryu_entrance_node),
    ]:
        if entrance not in entrance_ids:
            raise ValueError("공식 이동경로의 출입구가 보행망 엘리베이터와 연결되지 않았습니다: " + direction)
        payload = json.loads(path.read_text(encoding="utf-8"))
        routes.append({
            "id": direction, "title": title, "entranceNodeID": entrance,
            "officialRouteID": route_id, "steps": extract_steps(payload, route_id),
        })
    write_json_atomic(args.output, {
        "source": "https://data.kric.go.kr/rips/M_01_02/detail.do?id=429&operation=stationMovement&service=handicapped",
        "reviewedAt": args.reviewed_at,
        "routes": routes,
    })
    print("공식 동선 2개를 저장했습니다. 엘리베이터 운행 상태는 실시간 확인이 필요합니다.")


if __name__ == "__main__":
    try:
        main()
    except (OSError, ValueError, KeyError, json.JSONDecodeError) as exc:
        raise SystemExit("철도 자료 변환 실패: " + str(exc))
