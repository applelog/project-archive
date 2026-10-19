"""Attach terrain estimates to an existing OSM graph; never promote them to surveys.

Run in conda min with Tools/requirements-terrain.txt installed. All processing is
local. The input SHP/PRJ/DBF files stay unchanged, and output is written atomically.
"""

import argparse
import json
import math
from collections import Counter
from datetime import date
from pathlib import Path

import numpy as np
import osmium
import shapefile
from pyproj import CRS, Transformer
from scipy.interpolate import LinearNDInterpolator
from scipy.spatial import Delaunay, cKDTree

from import_seoul import write_json_atomic


SOURCE = "https://data.seoul.go.kr/dataList/OA-22241/F/1/datasetView.do"
LICENSE = "서울특별시 서울시 경사도·등고선; 공공누리 1유형 (출처표시, 상업적 이용·변경 가능)"


def sample_line(points, spacing):
    points = np.asarray(points, dtype=float)
    if points.ndim != 2 or points.shape[1] != 2 or len(points) < 2 or not math.isfinite(spacing) or spacing <= 0:
        raise ValueError("좌표 쌍 둘 이상과 양수 표본 간격이 필요합니다")
    lengths = np.linalg.norm(np.diff(points, axis=0), axis=1)
    keep = np.r_[True, lengths > 0]
    points = points[keep]
    if len(points) < 2 or not np.isfinite(points).all():
        raise ValueError("보행/등고선 좌표가 유효하지 않습니다")
    distances = np.r_[0, np.cumsum(np.linalg.norm(np.diff(points, axis=0), axis=1))]
    at = np.r_[np.arange(0, distances[-1], spacing), distances[-1]]
    return at, np.column_stack([np.interp(at, distances, points[:, axis]) for axis in (0, 1)])


class Terrain:
    def __init__(self, points, heights):
        points, heights = np.asarray(points), np.asarray(heights)
        if len(points) < 3 or not np.isfinite(points).all() or not np.isfinite(heights).all():
            raise ValueError("지형 표본이 부족하거나 비유한 값이 있습니다")
        unique, first = np.unique(np.round(points, 3), axis=0, return_index=True)
        self.triangles = Delaunay(unique)
        self.interpolator = LinearNDInterpolator(self.triangles, heights[first])
        self.nearest = cKDTree(unique)
        vertices = unique[self.triangles.simplices]
        self.spans = np.max([np.linalg.norm(vertices[:, a] - vertices[:, b], axis=1)
                            for a, b in [(0, 1), (1, 2), (2, 0)]], axis=0)
        self.sample_count = len(unique)

    def elevations(self, points):
        points = np.asarray(points)
        simplex = self.triangles.find_simplex(points)
        nearest, _ = self.nearest.query(points)
        valid = (simplex >= 0) & (nearest <= 125) & (self.spans[np.maximum(simplex, 0)] <= 300)
        heights = self.interpolator(points)
        heights[~valid] = np.nan
        return heights


def read_terrain(contours, spot_heights, bbox):
    crs = CRS.from_wkt(contours.with_suffix(".prj").read_text())
    if not crs.is_projected or any(abs(axis.unit_conversion_factor - 1) > 1e-9 for axis in crs.axis_info):
        raise ValueError("지형 좌표계는 미터 단위 투영 좌표계여야 합니다")
    if CRS.from_wkt(spot_heights.with_suffix(".prj").read_text()) != crs:
        raise ValueError("등고선과 표고점 좌표계가 다릅니다")
    transform = Transformer.from_crs(4326, crs, always_xy=True, allow_ballpark=False)
    left, bottom, right, top = transform.transform_bounds(*bbox)
    bounds = np.array([left - 300, bottom - 300, right + 300, top + 300])
    points, heights = [], []
    for path, expected in [(spot_heights, shapefile.POINT), (contours, shapefile.POLYLINE)]:
        reader = shapefile.Reader(str(path))
        if reader.shapeType != expected or "HEIGHT" not in [field[0] for field in reader.fields[1:]]:
            raise ValueError("Point/Polyline SHP와 HEIGHT 높이 필드가 필요합니다")
        for row in reader.iterShapeRecords():
            height = row.record["HEIGHT"]
            if height is None or not math.isfinite(height):
                raise ValueError("높이 자료에 결측/비유한 값이 있습니다")
            if expected == shapefile.POINT:
                sampled = np.asarray(row.shape.points, dtype=float)
            else:
                parts = list(row.shape.parts) + [len(row.shape.points)]
                chunks = [sample_line(row.shape.points[a:b], 20)[1]
                          for a, b in zip(parts, parts[1:]) if b - a >= 2]
                if not chunks:
                    continue
                sampled = np.concatenate(chunks)
            inside = ((sampled[:, 0] >= bounds[0]) & (sampled[:, 0] <= bounds[2]) &
                      (sampled[:, 1] >= bounds[1]) & (sampled[:, 1] <= bounds[3]))
            sampled = sampled[inside]
            if len(sampled):
                points.append(sampled)
                heights.append(np.full(len(sampled), height))
    if not points:
        raise ValueError("보행망 범위에 지형 표본이 없습니다")
    return Terrain(np.concatenate(points), np.concatenate(heights)), transform


class Structures(osmium.SimpleHandler):
    def __init__(self, way_ids):
        super().__init__()
        self.way_ids = way_ids
        self.eligible = set()
        self.structures = set()

    def way(self, way):
        if way.id not in self.way_ids:
            return
        tags = dict(way.tags)
        layer = tags.get("layer", "0")
        if tags.get("bridge", "no") != "no" or tags.get("tunnel", "no") != "no" or layer != "0":
            self.structures.add(way.id)
        else:
            self.eligible.add(way.id)


def slope_extrema(distances, heights):
    # shortcut: 30 m terrain windows miss short ramps/curbs; upgrade with surveyed sidewalk profiles.
    total = distances[-1]
    starts = distances[distances < total]
    ends = np.minimum(starts + 30, total)
    starts = np.maximum(0, ends - min(30, total))
    slopes = (np.interp(ends, distances, heights) - np.interp(starts, distances, heights)) / (ends - starts) * 100
    return math.ceil(max(0, float(slopes.max())) * 100) / 100, math.ceil(max(0, float(-slopes.min())) * 100) / 100


def attach_estimates(graph, terrain, transform, structures):
    counts = Counter()
    for index, edge in enumerate(graph["edges"]):
        if index % 20000 == 0:
            print("processed", index, "estimated", counts["estimated"], flush=True)
        if edge.get("gradeEvidence") in {"fieldChecked", "official"}:
            counts["keptConfirmed"] += 1
            continue
        for key in ["gradePercent", "forwardMaxUphillPercent", "forwardMaxDownhillPercent", "elevationProfile"]:
            edge.pop(key, None)
        edge["gradeEvidence"] = "unknown"
        way_id = int(edge["id"].split(":")[1])
        if edge.get("obstacle") != "none" or way_id in structures.structures:
            counts["excludedStructureOrObstacle"] += 1
            continue
        if way_id not in structures.eligible:
            counts["missingOSMWay"] += 1
            continue
        lon = [point["longitude"] for point in edge["coordinates"]]
        lat = [point["latitude"] for point in edge["coordinates"]]
        distances, xy = sample_line(np.column_stack(transform.transform(lon, lat)), 10)
        heights = terrain.elevations(xy)
        if not np.isfinite(heights).all():
            counts["outsideOrSparseTerrain"] += 1
            continue
        up, down = slope_extrema(distances, heights)
        positions = distances / distances[-1] * edge["length"]
        edge.update(gradeEvidence="estimated", forwardMaxUphillPercent=up,
                    forwardMaxDownhillPercent=down,
                    elevationProfile=[[float(x), round(float(z), 3)] for x, z in zip(positions, heights)])
        counts["estimated"] += 1
    graph["gradeSource"] = SOURCE
    graph["gradeSourceLicense"] = LICENSE
    graph["terrainImportSummary"] = dict(counts, terrainSamples=terrain.sample_count)
    graph["updatedAt"] = date.today().isoformat()
    assumptions = graph.setdefault("routingAssumptions", [])
    old_note = "OSM 보행 연결은 현장 검증 전입니다. 경사·턱·폭은 확인되지 않았습니다."
    if old_note in assumptions:
        assumptions.remove(old_note)
    note = "서울시 2023년 기준 표고점·등고선의 지형 추정 경사입니다. 실제 보도 경사·턱·폭은 미검증입니다."
    if note not in assumptions:
        assumptions.append(note)
    return counts


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ["network", "contours", "spot-heights", "osm-pbf", "output"]:
        parser.add_argument("--" + name, type=Path, required=True)
    args = parser.parse_args()
    if args.output.resolve() in {p.resolve() for p in [args.network, args.contours, args.spot_heights, args.osm_pbf]}:
        parser.error("원본과 다른 출력 경로를 지정하세요")
    graph = json.loads(args.network.read_text())
    bbox = graph.get("coverageBBox")
    if not bbox or len(bbox) != 4 or not all(math.isfinite(n) for n in bbox) or not (-180 <= bbox[0] < bbox[2] <= 180 and -90 <= bbox[1] < bbox[3] <= 90):
        raise ValueError("보행망의 위경도 범위가 필요합니다")
    way_ids = set()
    for edge in graph["edges"]:
        parts = edge["id"].split(":")
        if len(parts) != 4 or parts[0] != "osm" or not parts[1].isdigit():
            raise ValueError("OSM 보행망 링크 ID가 필요합니다")
        if not math.isfinite(edge["length"]) or edge["length"] <= 0:
            raise ValueError("보행로 길이가 유효하지 않습니다")
        way_ids.add(int(parts[1]))
    terrain, transform = read_terrain(args.contours, args.spot_heights, bbox)
    print("terrain samples", terrain.sample_count, flush=True)
    structures = Structures(way_ids)
    structures.apply_file(str(args.osm_pbf))
    counts = attach_estimates(graph, terrain, transform, structures)
    if not counts["estimated"]:
        raise ValueError("연결된 경사 표본이 없습니다")
    write_json_atomic(args.output, graph)
    print(json.dumps(dict(counts), ensure_ascii=False), flush=True)


if __name__ == "__main__":
    main()
