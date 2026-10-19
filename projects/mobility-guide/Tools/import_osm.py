"""Convert OpenStreetMap ways into the app's outdoor walking graph.

Requires pyosmium in the project's conda min environment. The output is ODbL
derived data; display © OpenStreetMap contributors and the licence link.
"""

import argparse
from collections import Counter
from datetime import date
from pathlib import Path

import osmium

from import_seoul import meters, write_json_atomic


PEDESTRIAN = {"footway", "path", "pedestrian", "steps", "living_street"}
QUIET_ROADS = {"residential", "unclassified", "service"}
OTHER_ROADS = {"tertiary", "secondary", "primary", "cycleway", "track"}
FOOT_ALLOWED = {"yes", "designated", "official"}
SIDEWALK = {"yes", "both", "left", "right"}


def walkable(tags):
    highway = tags.get("highway", "")
    foot = tags.get("foot", "")
    if tags.get("area") == "yes" or tags.get("indoor") == "yes" or tags.get("ford") == "yes":
        return False
    if foot and foot not in FOOT_ALLOWED:
        return False
    if tags.get("access") in {"no", "private", "permissive", "customers", "destination", "conditional"} and foot not in FOOT_ALLOWED:
        return False
    if tags.get("sac_scale") not in {None, "hiking"}:
        return False
    if highway in QUIET_ROADS and tags.get("sidewalk") == "separate" and foot not in FOOT_ALLOWED:
        return False
    if highway in PEDESTRIAN | QUIET_ROADS:
        return True
    if highway in OTHER_ROADS:
        return foot in FOOT_ALLOWED or tags.get("sidewalk") in SIDEWALK
    return False


def foot_direction(tags):
    value = tags.get("oneway:foot", "no")
    if value in {"yes", "1", "true"}:
        return 1
    if value == "-1":
        return -1
    if value in {"no", "0", "false"}:
        return 0
    return None  # Unknown restrictions are excluded, not guessed.


class WalkingWays(osmium.SimpleHandler):
    def __init__(self, bbox):
        super().__init__()
        self.bbox = bbox
        self.ways = []
        self.references = Counter()
        self.skipped_missing_locations = 0
        self.skipped_unknown_direction = 0

    def inside(self, longitude, latitude):
        return self.bbox is None or (self.bbox[0] <= longitude <= self.bbox[2]
                                     and self.bbox[1] <= latitude <= self.bbox[3])

    def way(self, way):
        tags = dict(way.tags)
        if not walkable(tags):
            return
        direction = foot_direction(tags)
        if direction is None:
            self.skipped_unknown_direction += 1
            return
        conveying = tags.get("conveying")
        if conveying == "forward":
            direction = 1
        elif conveying == "backward":
            direction = -1
        elif conveying not in {None, "no"}:
            self.skipped_unknown_direction += 1
            return
        points = []
        for ref in way.nodes:
            if not ref.location.valid():
                self.skipped_missing_locations += 1
                return
            point = (ref.ref, ref.location.lat, ref.location.lon)
            if not points or points[-1][0] != point[0]:
                points.append(point)
        obstacle = ("escalator" if conveying in {"forward", "backward"}
                    else "stairs" if tags.get("highway") == "steps"
                    else "wheelchairRestricted" if tags.get("wheelchair") in {"no", "limited"}
                    else "none")
        run = []
        for point in points + [None]:
            if point is not None and self.inside(point[2], point[1]):
                run.append(point)
            else:
                if len(run) >= 2:
                    self.ways.append((way.id, len(self.ways), run, obstacle, direction))
                    self.references.update(item[0] for item in run)
                run = []


def build_graph(ways, *, extract_date=None):
    nodes = {}
    edges = []
    for way_id, run_id, points, obstacle, direction in ways.ways:
        split_at = [index for index, point in enumerate(points)
                    if index == 0 or index == len(points) - 1 or ways.references[point[0]] > 1]
        for part, (first, last) in enumerate(zip(split_at, split_at[1:])):
            piece = points[first:last + 1]
            if piece[0][0] == piece[-1][0]:
                continue
            if direction == -1:
                piece = piece[::-1]
            coordinates = [{"latitude": point[1], "longitude": point[2]} for point in piece]
            length = sum(meters(a, b) for a, b in zip(coordinates, coordinates[1:]))
            if length <= 0:
                continue
            start, end = piece[0], piece[-1]
            for point in (start, end):
                nodes[point[0]] = {"id": "osm:" + str(point[0]),
                                   "latitude": point[1], "longitude": point[2]}
            edges.append({
                "id": "osm:{}:{}:{}".format(way_id, run_id, part),
                "from": "osm:" + str(start[0]), "to": "osm:" + str(end[0]),
                "length": length, "coordinates": coordinates,
                "gradeEvidence": "unknown", "obstacle": obstacle,
                "reverseAllowed": direction == 0,
            })
    if not edges:
        raise ValueError("선택 범위에 연결된 보행로가 없습니다")
    return {
        "updatedAt": date.today().isoformat(),
        "routingAssumptions": [
            "OSM 보행 연결은 현장 검증 전입니다. 경사·턱·폭은 확인되지 않았습니다.",
            "계단은 OSM 표기를 기준으로 제외하며, 누락된 계단은 검출하지 못합니다.",
        ],
        "source": "https://download.geofabrik.de/asia/south-korea.html",
        "sourceExtractDate": extract_date,
        "sourceLicense": "ODbL 1.0; © OpenStreetMap contributors",
        "coverageBBox": ways.bbox,
        "nodes": list(nodes.values()), "edges": edges, "entrances": [],
        "importSummary": {
            "walkingWayRuns": len(ways.ways), "nodes": len(nodes), "edges": len(edges),
            "skippedMissingLocations": ways.skipped_missing_locations,
            "skippedUnknownDirection": ways.skipped_unknown_direction,
        },
    }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--pbf", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--bbox", type=float, nargs=4,
                        metavar=("MIN_LON", "MIN_LAT", "MAX_LON", "MAX_LAT"),
                        help="Optional geographic clip; omit to import all South Korea")
    parser.add_argument("--extract-date", help="Source extract date YYYY-MM-DD")
    args = parser.parse_args()
    if args.bbox and not (-180 <= args.bbox[0] < args.bbox[2] <= 180
                          and -90 <= args.bbox[1] < args.bbox[3] <= 90):
        parser.error("bbox 좌표 범위가 올바르지 않습니다")
    if args.extract_date:
        date.fromisoformat(args.extract_date)
    if args.pbf.resolve() == args.output.resolve():
        parser.error("입력 파일과 출력 파일이 같습니다")
    handler = WalkingWays(args.bbox)
    handler.apply_file(str(args.pbf), locations=True)
    graph = build_graph(handler, extract_date=args.extract_date)
    write_json_atomic(args.output, graph)
    print(graph["importSummary"])


if __name__ == "__main__":
    try:
        main()
    except (OSError, ValueError, RuntimeError) as exc:
        raise SystemExit("OSM 변환 실패: " + str(exc))
