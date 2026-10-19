"""Convert official Seoul walking-network and optional elevator CSV exports."""

import argparse
import csv
import json
import math
import os
import re
import tempfile
from datetime import date
from pathlib import Path


WKT = re.compile(r"^(POINT|LINESTRING)\s*\((.*)\)$", re.IGNORECASE)
COLUMNS = {
    "NODE_TYPE": "노드링크 유형", "NODE_ID": "노드 ID", "NODE_WKT": "노드 WKT",
    "LNKG_ID": "링크 ID", "LNKG_WKT": "링크 WKT", "LNKG_TYPE": "링크 유형 코드",
    "BGNG_LNKG_ID": "시작노드 ID", "END_LNKG_ID": "종료노드 ID", "LNKG_LEN": "링크 길이",
    "SGG_NM": "시군구명", "OVRP": "고가도로", "SBWY_NTW": "지하철네트워크",
    "BRDG": "교량", "TUNNEL": "터널", "FOOTBRIDGE": "육교", "BLDG": "건물내",
}
HAZARD_FLAGS = ("OVRP", "BRDG", "TUNNEL", "FOOTBRIDGE")
OBSTACLES = {"none", "stairs", "escalator", "impassable", "unverifiedOverpass"}


def parse_wkt(value):
    match = WKT.fullmatch((value or "").strip())
    if not match:
        raise ValueError("지원하지 않는 좌표 형식")
    points = []
    for pair in match.group(2).split(","):
        parts = pair.strip().split()
        if len(parts) != 2:
            raise ValueError("좌표는 경도와 위도 두 값이어야 합니다")
        try:
            longitude, latitude = (float(part) for part in parts)
        except ValueError:
            raise ValueError("좌표가 숫자가 아닙니다") from None
        if not (math.isfinite(longitude) and math.isfinite(latitude) and
                -180 <= longitude <= 180 and -90 <= latitude <= 90):
            raise ValueError("좌표 범위가 올바르지 않습니다")
        points.append({"latitude": latitude, "longitude": longitude})
    if match.group(1).upper() == "POINT" and len(points) != 1:
        raise ValueError("POINT에는 좌표 하나가 필요합니다")
    if match.group(1).upper() == "LINESTRING" and len(points) < 2:
        raise ValueError("LINESTRING에는 좌표 둘 이상이 필요합니다")
    return points[0] if len(points) == 1 else points


def meters(a, b):
    first, second = math.radians(a["latitude"]), math.radians(b["latitude"])
    dy = second - first
    dx = math.radians(b["longitude"] - a["longitude"])
    haversine = math.sin(dy / 2) ** 2 + math.cos(first) * math.cos(second) * math.sin(dx / 2) ** 2
    return 6_371_008.8 * 2 * math.asin(math.sqrt(min(1, max(0, haversine))))


def parse_flag(value):
    if value.upper() in {"", "N", "0"}:
        return False
    if value.upper() in {"Y", "1"}:
        return True
    raise ValueError("속성 플래그는 0/1 또는 N/Y이어야 합니다")


def measured_percent(value, signed=False):
    value = (value or "").strip()
    if not value:
        return None
    percent = float(value)
    if not math.isfinite(percent) or not (-100 if signed else 0) <= percent <= 100:
        raise ValueError("현장 경사 값이 올바르지 않습니다")
    return percent


def build_graph(rows, elevator_rows, checks, *, bidirectional=False):
    summary = dict(sourceRows=0, districtCount=0, duplicateNodes=0, duplicateLinks=0,
                   excludedMissingEndpoints=0, excludedSelfLoops=0, excludedIndoor=0, excludedSubway=0,
                   excludedGeometry=0, alignedGeometry=0, reversedGeometry=0,
                   blockedUnverifiedStructures=0, blockedNonpedestrian=0,
                   maxEndpointDifferenceMeters=0)
    nodes, links, districts = {}, {}, set()
    for raw in rows:
        summary["sourceRows"] += 1
        row = {key: (raw.get(key) or raw.get(alias) or "").strip() for key, alias in COLUMNS.items()}
        row["LNKG_TYPE"] = row["LNKG_TYPE"] or (raw.get("LNKG_TYPE_CD") or "").strip()
        if row["SGG_NM"]:
            districts.add(row["SGG_NM"])
        if row["NODE_TYPE"].upper() == "NODE":
            node_id = row["NODE_ID"]
            if not node_id:
                raise ValueError("ID 없는 보행 노드")
            point = parse_wkt(row["NODE_WKT"])
            if not isinstance(point, dict):
                raise ValueError("노드 좌표가 POINT가 아닙니다")
            if node_id in nodes:
                if nodes[node_id] != point:
                    raise ValueError("중복된 노드 ID의 좌표가 다릅니다: " + node_id)
                summary["duplicateNodes"] += 1
            else:
                nodes[node_id] = point
        elif row["NODE_TYPE"].upper() == "LINK":
            edge_id = row["LNKG_ID"]
            if not edge_id:
                raise ValueError("ID 없는 보행 링크")
            if edge_id in links:
                if links[edge_id] != row:
                    raise ValueError("중복된 링크 ID의 속성이 다릅니다: " + edge_id)
                summary["duplicateLinks"] += 1
            else:
                links[edge_id] = row
        else:
            raise ValueError("노드링크 유형은 NODE 또는 LINK이어야 합니다")
    summary["districtCount"] = len(districts)

    edges = []
    for edge_id, row in links.items():
        flags = {key: parse_flag(row[key]) for key in (*HAZARD_FLAGS, "SBWY_NTW", "BLDG")}
        code = row["LNKG_TYPE"]
        if code and not re.fullmatch("[01]{4}", code):
            raise ValueError("링크 유형 코드는 네 자리 이진수이어야 합니다: " + edge_id)
        start, end = row["BGNG_LNKG_ID"], row["END_LNKG_ID"]
        if start not in nodes or end not in nodes:
            summary["excludedMissingEndpoints"] += 1
            continue
        if start == end:
            summary["excludedSelfLoops"] += 1
            continue
        if flags["BLDG"]:
            summary["excludedIndoor"] += 1
            continue
        if flags["SBWY_NTW"]:
            summary["excludedSubway"] += 1
            continue
        line = parse_wkt(row["LNKG_WKT"])
        if not isinstance(line, list):
            raise ValueError("링크 좌표가 LINESTRING이 아닙니다: " + edge_id)
        forward = (meters(line[0], nodes[start]), meters(line[-1], nodes[end]))
        reverse = (meters(line[-1], nodes[start]), meters(line[0], nodes[end]))
        if sum(reverse) < sum(forward):
            line.reverse()
            forward = reverse
            summary["reversedGeometry"] += 1
        delta = max(forward)
        summary["maxEndpointDifferenceMeters"] = max(summary["maxEndpointDifferenceMeters"], delta)
        if delta > 0.5:
            summary["excludedGeometry"] += 1
            continue
        if delta > 0:
            summary["alignedGeometry"] += 1
        # Only sub-meter export rounding may align to a node; never create a connector.
        line[0], line[-1] = nodes[start], nodes[end]
        geometric_length = sum(meters(a, b) for a, b in zip(line, line[1:]))
        official_length = float(row["LNKG_LEN"] or 0)
        if not math.isfinite(official_length) or official_length < 0:
            raise ValueError("링크 길이가 올바르지 않습니다: " + edge_id)
        length = official_length or geometric_length
        if not math.isfinite(length) or length <= 0:
            raise ValueError("링크 길이가 올바르지 않습니다: " + edge_id)

        obstacle = "unverifiedOverpass" if any(flags[key] for key in HAZARD_FLAGS) else "none"
        grade, uphill, downhill, evidence = None, None, None, "unknown"
        reverse_allowed = True if bidirectional else None
        if edge_id in checks:
            check = checks[edge_id]
            checked_at = (check.get("checked_at") or "").strip()
            if not re.fullmatch(r"\d{4}-\d{2}-\d{2}", checked_at):
                raise ValueError("현장 확인 날짜는 YYYY-MM-DD이어야 합니다: " + edge_id)
            date.fromisoformat(checked_at)
            checked_obstacle = (check.get("obstacle") or "").strip()
            if checked_obstacle:
                if checked_obstacle not in OBSTACLES:
                    raise ValueError("현장 확인 장애물 값이 올바르지 않습니다: " + edge_id)
                obstacle = checked_obstacle
            grade = measured_percent(check.get("grade_percent"), signed=True)
            uphill = measured_percent(check.get("forward_max_uphill_percent"))
            downhill = measured_percent(check.get("forward_max_downhill_percent"))
            if (uphill is None) != (downhill is None):
                raise ValueError("방향별 경사는 오르막과 내리막 최댓값을 함께 기록해야 합니다: " + edge_id)
            if grade is not None or uphill is not None:
                if not checked_obstacle:
                    raise ValueError("경사 확인에는 장애물 확인 값이 필요합니다: " + edge_id)
                evidence = "fieldChecked"
                # General-walking direction assumptions must not grant reverse access
                # to a link accepted by the strict step-free profile.
                reverse_allowed = None
            direction = (check.get("reverse_allowed") or "").strip().lower()
            if direction:
                if direction not in {"true", "false"}:
                    raise ValueError("reverse_allowed는 true/false이어야 합니다: " + edge_id)
                reverse_allowed = direction == "true"
        # Official codebook: the first bit means pedestrian permission, not stairs or direction.
        if code.startswith("0"):
            obstacle = "impassable"
            summary["blockedNonpedestrian"] += 1
        elif obstacle == "unverifiedOverpass":
            summary["blockedUnverifiedStructures"] += 1
        edges.append({
            "id": edge_id, "from": start, "to": end, "length": length,
            "coordinates": line, "gradePercent": grade, "gradeEvidence": evidence,
            "obstacle": obstacle, "reverseAllowed": reverse_allowed,
            "forwardMaxUphillPercent": uphill, "forwardMaxDownhillPercent": downhill,
        })

    unknown_checks = set(checks) - {edge["id"] for edge in edges}
    if unknown_checks:
        raise ValueError("보행망에 없거나 제외된 현장 확인 링크: " + ", ".join(sorted(unknown_checks)[:5]))
    entrances = []
    for row in elevator_rows:
        station = row.get("SBWY_STN_NM", "").strip()
        if station.endswith("역"):
            station = station[:-1]
        node_id = row.get("NODE_ID", "").strip()
        if not station or node_id not in nodes:
            raise ValueError("엘리베이터의 역 이름 또는 연결된 보행 노드가 없습니다")
        point = parse_wkt(row.get("NODE_WKT"))
        if not isinstance(point, dict) or meters(point, nodes[node_id]) > 0.5:
            raise ValueError("엘리베이터 위치와 보행망 노드가 일치하지 않습니다")
        entrances.append({
            "id": node_id, "name": station + "역 엘리베이터 " + str(len(entrances) + 1),
            "nodeID": node_id, "coordinate": nodes[node_id],
        })
    summary.update(importedNodes=len(nodes), importedLinks=len(edges), importedEntrances=len(entrances))
    return {
        "updatedAt": date.today().isoformat(),
        "nodes": [{"id": node_id, **point} for node_id, point in nodes.items()],
        "edges": edges, "entrances": entrances, "importSummary": summary,
        "routingAssumptions": ["일반 보행용 양방향 통행 가정; 공식 방향 정보 및 휠체어 통행 가능성은 미확인"] if bidirectional else [],
    }


def read_csv(path):
    encoding = "utf-8-sig"
    try:
        with open(path, encoding=encoding) as handle:
            while handle.read(1024 * 1024):
                pass
    except UnicodeDecodeError:
        encoding = "cp949"
    with open(path, newline="", encoding=encoding) as handle:
        reader = csv.DictReader(handle, strict=True)
        headers = reader.fieldnames
        if not headers or len(headers) != len(set(headers)) or any(not header.strip() for header in headers):
            raise ValueError("CSV 헤더가 없거나 중복됐습니다")
        for row in reader:
            if None in row or any(value is None for value in row.values()):
                raise ValueError("CSV 열 개수가 헤더와 다릅니다: " + str(reader.line_num))
            yield row


def write_json_atomic(path, value):
    path.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.NamedTemporaryFile("w", encoding="utf-8", dir=path.parent, delete=False) as handle:
        temporary = Path(handle.name)
        try:
            json.dump(value, handle, ensure_ascii=False, separators=(",", ":"), allow_nan=False)
        except Exception:
            temporary.unlink(missing_ok=True)
            raise
    os.replace(temporary, path)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--network-csv", required=True, type=Path)
    parser.add_argument("--elevator-csv", type=Path)
    parser.add_argument("--field-checks", type=Path)
    parser.add_argument("--bidirectional", action="store_true", help="일반 보행용 양방향 통행 근사; 휠체어 경사 확인을 대신하지 않습니다")
    parser.add_argument("--output", type=Path, default=Path("Data/network.json"))
    args = parser.parse_args()
    checks = {}
    if args.field_checks:
        for row in read_csv(args.field_checks):
            edge_id = row.get("edge_id", "").strip()
            if not edge_id or edge_id in checks:
                raise ValueError("현장 확인 링크 ID가 없거나 중복됐습니다")
            checks[edge_id] = row
    graph = build_graph(read_csv(args.network_csv), read_csv(args.elevator_csv) if args.elevator_csv else [],
                        checks, bidirectional=args.bidirectional)
    if not graph["nodes"] or not graph["edges"]:
        raise ValueError("연결된 보행 노드 또는 링크가 없습니다")
    write_json_atomic(args.output, graph)
    print(json.dumps(graph["importSummary"], ensure_ascii=False))
    for assumption in graph["routingAssumptions"]:
        print(assumption)


if __name__ == "__main__":
    try:
        main()
    except (OSError, ValueError, KeyError, csv.Error) as exc:
        raise SystemExit("자료 변환 실패: " + str(exc))
