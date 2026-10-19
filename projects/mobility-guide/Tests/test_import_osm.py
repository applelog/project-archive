import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "Tools"))
from import_osm import WalkingWays, build_graph, walkable


OSM = """<osm version="0.6" generator="test">
  <node id="1" lat="37" lon="127"/>
  <node id="2" lat="37" lon="127.001"/>
  <node id="3" lat="37" lon="127.002"/>
  <node id="4" lat="37.001" lon="127.001"/>
  <way id="10"><nd ref="1"/><nd ref="2"/><nd ref="3"/><tag k="highway" v="footway"/></way>
  <way id="11"><nd ref="2"/><nd ref="4"/><tag k="highway" v="steps"/></way>
  <way id="12"><nd ref="4"/><nd ref="3"/><tag k="highway" v="footway"/><tag k="oneway:foot" v="-1"/></way>
  <way id="13"><nd ref="1"/><nd ref="4"/><tag k="highway" v="steps"/><tag k="conveying" v="backward"/></way>
  <way id="14"><nd ref="1"/><nd ref="3"/><tag k="highway" v="steps"/><tag k="conveying" v="yes"/></way>
</osm>"""


class OSMImportTests(unittest.TestCase):
    def test_shared_nodes_stairs_and_reverse_only(self):
        with tempfile.TemporaryDirectory() as folder:
            source = Path(folder) / "sample.osm"
            source.write_text(OSM, encoding="utf-8")
            handler = WalkingWays((126.9, 36.9, 127.1, 37.1))
            handler.apply_file(str(source), locations=True)
            graph = build_graph(handler)
        self.assertEqual(len(graph["edges"]), 5)
        self.assertEqual(graph["importSummary"]["skippedUnknownDirection"], 1)
        stairs = next(edge for edge in graph["edges"] if edge["id"].startswith("osm:11:"))
        self.assertEqual((stairs["from"], stairs["to"]), ("osm:2", "osm:4"))
        reverse_only = next(edge for edge in graph["edges"] if edge["id"].startswith("osm:12:"))
        self.assertEqual((reverse_only["from"], reverse_only["to"]), ("osm:3", "osm:4"))
        self.assertFalse(reverse_only["reverseAllowed"])
        escalator = next(edge for edge in graph["edges"] if edge["id"].startswith("osm:13:"))
        self.assertEqual((escalator["from"], escalator["to"]), ("osm:4", "osm:1"))
        self.assertEqual(escalator["obstacle"], "escalator")
        self.assertFalse(escalator["reverseAllowed"])
        self.assertTrue(all(edge["gradeEvidence"] == "unknown" for edge in graph["edges"]))
        self.assertEqual({node["id"] for node in graph["nodes"]}, {"osm:1", "osm:2", "osm:3", "osm:4"})

    def test_private_and_motor_roads_are_not_walkable(self):
        self.assertFalse(walkable({"highway": "footway", "foot": "no"}))
        self.assertFalse(walkable({"highway": "motorway", "foot": "yes"}))
        self.assertFalse(walkable({"highway": "primary", "sidewalk": "separate"}))
        self.assertFalse(walkable({"highway": "path", "access": "private"}))
        self.assertFalse(walkable({"highway": "path", "access": "customers"}))
        self.assertFalse(walkable({"highway": "footway", "foot": "conditional"}))
        self.assertFalse(walkable({"highway": "residential", "sidewalk": "separate"}))
        self.assertTrue(walkable({"highway": "primary", "sidewalk": "both"}))


if __name__ == "__main__":
    unittest.main()
