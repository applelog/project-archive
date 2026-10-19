import csv
import json
import os
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "Tools"))
from import_seoul import build_graph, parse_wkt, read_csv, write_json_atomic


def network_rows():
    return [
        {"노드링크 유형": "NODE", "노드 ID": "A", "노드 WKT": "POINT (127 37)", "시군구명": "구로구"},
        {"노드링크 유형": "NODE", "노드 ID": "B", "노드 WKT": "POINT (127.001 37)", "시군구명": "금천구"},
        {"노드링크 유형": "NODE", "노드 ID": "C", "노드 WKT": "POINT (127.002 37)", "시군구명": "강남구"},
        {"노드링크 유형": "LINK", "링크 ID": "AB", "시작노드 ID": "A", "종료노드 ID": "B", "링크 WKT": "LINESTRING (127 37, 127.001 37)", "링크 길이": "89", "시군구명": "금천구", "고가도로": "0", "지하철네트워크": "0", "건물내": "0"},
        {"노드링크 유형": "LINK", "링크 ID": "BC", "시작노드 ID": "B", "종료노드 ID": "C", "링크 WKT": "LINESTRING (127.001 37, 127.002 37)", "링크 길이": "89", "시군구명": "강남구", "고가도로": "0", "지하철네트워크": "0", "건물내": "0"},
    ]


class SeoulImportTests(unittest.TestCase):
    def test_all_districts_and_optional_elevators_keep_unknown_evidence(self):
        graph = build_graph(iter(network_rows()), [], {})
        self.assertEqual({node["id"] for node in graph["nodes"]}, {"A", "B", "C"})
        self.assertEqual(len(graph["edges"]), 2)
        self.assertEqual(graph["entrances"], [])
        self.assertEqual(graph["importSummary"]["districtCount"], 3)
        for edge in graph["edges"]:
            self.assertIsNone(edge["gradePercent"])
            self.assertEqual(edge["gradeEvidence"], "unknown")
            self.assertIsNone(edge["reverseAllowed"])

    def test_all_station_names_are_imported(self):
        lifts = [
            {"SBWY_STN_NM": "시청역", "NODE_ID": "A", "NODE_WKT": "POINT (127 37)"},
            {"SBWY_STN_NM": "강남", "NODE_ID": "C", "NODE_WKT": "POINT (127.002 37)"},
        ]
        graph = build_graph(network_rows(), lifts, {})
        self.assertEqual([entry["nodeID"] for entry in graph["entrances"]], ["A", "C"])
        self.assertIn("시청역", graph["entrances"][0]["name"])
        self.assertIn("강남역", graph["entrances"][1]["name"])

    def test_existing_english_schema_and_yn_flags_still_work(self):
        rows = [
            {"NODE_TYPE": "NODE", "NODE_ID": "A", "NODE_WKT": "POINT (127 37)", "SGG_NM": "중구"},
            {"NODE_TYPE": "NODE", "NODE_ID": "B", "NODE_WKT": "POINT (127.001 37)", "SGG_NM": "중구"},
            {"NODE_TYPE": "LINK", "LNKG_ID": "AB", "BGNG_LNKG_ID": "A", "END_LNKG_ID": "B", "LNKG_WKT": "LINESTRING (127 37, 127.001 37)", "LNKG_LEN": "89", "OVRP": "Y", "BLDG": "N"},
        ]
        edge = build_graph(rows, [], {})["edges"][0]
        self.assertEqual(edge["id"], "AB")
        self.assertEqual(edge["obstacle"], "unverifiedOverpass")

    def test_signed_legacy_grade_and_directional_checks_are_preserved(self):
        checks = {
            "AB": {"grade_percent": "-9", "obstacle": "none", "checked_at": "2026-09-25", "reverse_allowed": "false"},
            "BC": {"forward_max_uphill_percent": "2", "forward_max_downhill_percent": "8", "obstacle": "none", "checked_at": "2026-09-25", "reverse_allowed": "true"},
        }
        edges = build_graph(network_rows(), [], checks)["edges"]
        self.assertEqual(edges[0]["gradePercent"], -9)
        self.assertFalse(edges[0]["reverseAllowed"])
        self.assertEqual(edges[1]["forwardMaxUphillPercent"], 2)
        self.assertEqual(edges[1]["forwardMaxDownhillPercent"], 8)
        self.assertTrue(edges[1]["reverseAllowed"])
        self.assertTrue(all(edge["gradeEvidence"] == "fieldChecked" for edge in edges))

    def test_explicit_bidirectional_approximation_does_not_confirm_grade(self):
        graph = build_graph(network_rows(), [], {}, bidirectional=True)
        self.assertTrue(all(edge["reverseAllowed"] for edge in graph["edges"]))
        self.assertTrue(all(edge["gradeEvidence"] == "unknown" for edge in graph["edges"]))
        self.assertTrue(graph["routingAssumptions"])
        check = {"AB": {"checked_at": "2026-09-25", "reverse_allowed": "false"}}
        self.assertFalse(build_graph(network_rows(), [], check, bidirectional=True)["edges"][0]["reverseAllowed"])
        check = {"AB": {"grade_percent": "0", "obstacle": "none", "checked_at": "2026-09-25"}}
        self.assertIsNone(build_graph(network_rows(), [], check, bidirectional=True)["edges"][0]["reverseAllowed"])

    def test_numeric_hazard_flags_block_overpasses_and_exclude_indoor_subway(self):
        for column in ("고가도로", "육교", "교량", "터널"):
            with self.subTest(column=column):
                rows = network_rows()
                rows[3][column] = "1"
                self.assertEqual(build_graph(rows, [], {})["edges"][0]["obstacle"], "unverifiedOverpass")
        rows = network_rows()
        rows[3]["건물내"] = "1"
        rows[4]["지하철네트워크"] = "1"
        graph = build_graph(rows, [], {})
        self.assertEqual(graph["edges"], [])
        self.assertEqual(graph["importSummary"]["excludedIndoor"], 1)
        self.assertEqual(graph["importSummary"]["excludedSubway"], 1)

    def test_only_rounding_error_is_aligned_and_large_mismatch_is_excluded(self):
        rows = network_rows()
        rows[3]["링크 WKT"] = "LINESTRING (127.001 37, 127.000001 37)"
        rows[4]["링크 WKT"] = "LINESTRING (127.0011 37, 127.002 37)"
        graph = build_graph(rows, [], {})
        self.assertEqual(len(graph["edges"]), 1)
        self.assertEqual(graph["edges"][0]["coordinates"][0], parse_wkt("POINT (127 37)"))
        self.assertEqual(graph["importSummary"]["excludedGeometry"], 1)
        self.assertEqual(graph["importSummary"]["alignedGeometry"], 1)
        self.assertGreater(graph["importSummary"]["maxEndpointDifferenceMeters"], 8)

    def test_missing_endpoint_is_counted_without_inventing_a_node(self):
        rows = network_rows()
        rows[4]["종료노드 ID"] = "missing"
        graph = build_graph(rows, [], {})
        self.assertEqual(len(graph["edges"]), 1)
        self.assertEqual(graph["importSummary"]["excludedMissingEndpoints"], 1)
        rows[4]["종료노드 ID"] = "B"
        graph = build_graph(rows, [], {})
        self.assertEqual(graph["importSummary"]["excludedMissingEndpoints"], 0)
        self.assertEqual(graph["importSummary"]["excludedSelfLoops"], 1)

    def test_verified_transport_code_blocks_nonpedestrian_links(self):
        rows = network_rows()
        rows[3]["링크 유형 코드"] = "0001"
        rows[4]["링크 유형 코드"] = "1111"
        edges = build_graph(rows, [], {})["edges"]
        self.assertEqual(edges[0]["obstacle"], "impassable")
        self.assertEqual(edges[1]["obstacle"], "none")
        self.assertIsNone(edges[1]["reverseAllowed"])
        check = {"AB": {"obstacle": "none", "grade_percent": "0", "checked_at": "2026-09-25"}}
        self.assertEqual(build_graph(rows, [], check)["edges"][0]["obstacle"], "impassable")
        rows[3]["링크 유형 코드"] = "invalid"
        self.assertRaises(ValueError, build_graph, rows, [], {})

    def test_equivalent_duplicates_are_deduplicated_but_conflicts_fail(self):
        rows = network_rows()
        graph = build_graph(rows + [dict(rows[0]), dict(rows[3])], [], {})
        self.assertEqual(len(graph["nodes"]), 3)
        self.assertEqual(len(graph["edges"]), 2)
        for changed in ({**rows[0], "노드 WKT": "POINT (127.0001 37)"}, {**rows[3], "고가도로": "1"}):
            with self.subTest(changed=changed):
                self.assertRaises(ValueError, build_graph, rows + [changed], [], {})

    def test_bogus_coordinates_flags_lengths_and_directions_fail(self):
        for value in ("POINT (oops)", "POINT (nan 37)", "POINT (181 37)", "LINESTRING (127 37)"):
            with self.subTest(value=value):
                self.assertRaises(ValueError, parse_wkt, value)
        for column, value in (("고가도로", "maybe"), ("링크 길이", "nan"), ("링크 길이", "-1"), ("링크 WKT", "POINT (127 37)")):
            rows = network_rows()
            rows[3][column] = value
            with self.subTest(column=column, value=value):
                self.assertRaises(ValueError, build_graph, rows, [], {})
        for values in ({"reverse_allowed": "1"}, {"grade_percent": "nan"}, {"grade_percent": "-101"}, {"forward_max_uphill_percent": "2"}, {"forward_max_uphill_percent": "-1", "forward_max_downhill_percent": "1"}, {"checked_at": "bad"}):
            check = {"checked_at": "2026-09-25", **values}
            with self.subTest(values=values):
                self.assertRaises(ValueError, build_graph, network_rows(), [], {"AB": check})

    def test_unknown_check_and_unconnected_elevator_fail(self):
        self.assertRaises(ValueError, build_graph, network_rows(), [], {"missing": {"checked_at": "2026-09-25"}})
        self.assertRaises(ValueError, build_graph, network_rows(), [{"SBWY_STN_NM": "시청", "NODE_ID": "missing", "NODE_WKT": "POINT (127 37)"}], {})

    def test_cp949_csv_and_cli_without_elevators(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "network.csv"
            output = Path(directory) / "graph.json"
            rows = network_rows()
            headers = sorted({key for row in rows for key in row})
            with path.open("w", encoding="cp949", newline="") as handle:
                writer = csv.DictWriter(handle, headers)
                writer.writeheader()
                writer.writerows(rows)
            self.assertEqual(len(list(read_csv(path))), 5)
            result = subprocess.run([sys.executable, str(Path(__file__).resolve().parents[1] / "Tools/import_seoul.py"), "--network-csv", str(path), "--output", str(output), "--bidirectional"], capture_output=True, text=True)
            self.assertEqual(result.returncode, 0, result.stderr)
            self.assertEqual(len(json.loads(output.read_text())["edges"]), 2)

    def test_malformed_csv_columns_fail(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "bad.csv"
            for text in ("id,id\n1,2\n", "id,wkt\n1\n", "id\n1,extra\n"):
                path.write_text(text)
                with self.subTest(text=text):
                    self.assertRaises(ValueError, list, read_csv(path))

    def test_atomic_json_rejects_nonfinite_values_without_replacing_existing_file(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "graph.json"
            path.write_text("original")
            self.assertRaises(ValueError, write_json_atomic, path, {"length": float("nan")})
            self.assertEqual(path.read_text(), "original")
            self.assertEqual(list(Path(directory).iterdir()), [path])

    @unittest.skipUnless(os.environ.get("SEOUL_NETWORK_CSV"), "Set SEOUL_NETWORK_CSV to run the official full CSV smoke check")
    def test_official_full_csv_smoke(self):
        graph = build_graph(read_csv(os.environ["SEOUL_NETWORK_CSV"]), [], {})
        self.assertEqual(graph["importSummary"]["districtCount"], 25)
        self.assertEqual(graph["importSummary"]["sourceRows"], 491082)
        self.assertEqual(graph["importSummary"]["excludedMissingEndpoints"], 3663)
        self.assertEqual(graph["importSummary"]["excludedSelfLoops"], 470)
        self.assertGreater(len(graph["edges"]), 260000)
        self.assertEqual(graph["importSummary"]["maxEndpointDifferenceMeters"], 0)
        self.assertTrue(all(edge["gradeEvidence"] == "unknown" and edge["reverseAllowed"] is None for edge in graph["edges"]))


if __name__ == "__main__":
    unittest.main()
