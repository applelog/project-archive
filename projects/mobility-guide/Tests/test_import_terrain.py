import sys
import unittest
from pathlib import Path
from types import SimpleNamespace

import numpy as np

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "Tools"))
from import_terrain import Terrain, attach_estimates, sample_line, slope_extrema


class TerrainChecks(unittest.TestCase):
    def test_plane_and_no_extrapolation(self):
        terrain = Terrain([[0, 0], [0, 60], [60, 0], [60, 60]], [10, 10, 16, 16])
        heights = terrain.elevations([[0, 0], [30, 30], [60, 60], [61, 30]])
        np.testing.assert_allclose(heights[:3], [10, 13, 16])
        self.assertTrue(np.isnan(heights[3]))
        sparse = Terrain([[0, 0], [0, 1000], [1000, 0]], [0, 0, 100])
        self.assertTrue(np.isnan(sparse.elevations([[10, 10]])[0]))

    def test_directional_slopes_and_short_links(self):
        self.assertEqual(slope_extrema(np.array([0, 10, 20, 30]), np.array([0, 1, 2, 3])), (10, 0))
        self.assertEqual(slope_extrema(np.array([0, 10]), np.array([3, 2])), (0, 10))
        self.assertEqual(slope_extrema(np.array([0, 10, 20, 30, 40]), np.array([0, 1, 2, 3, 4])), (10, 0))
        distances, points = sample_line([[0, 0], [0, 0], [15, 0]], 10)
        np.testing.assert_allclose(distances, [0, 10, 15])
        np.testing.assert_allclose(points[:, 0], [0, 10, 15])

    def test_structures_and_surveys_are_preserved(self):
        terrain = Terrain([[0, 0], [0, 60], [60, 0], [60, 60]], [10, 10, 16, 16])
        class Identity:
            def transform(self, x, y):
                return x, y
        def link(way, evidence="unknown", obstacle="none"):
            return dict(id="osm:{}:0:0".format(way), length=30, gradeEvidence=evidence,
                        obstacle=obstacle, coordinates=[dict(longitude=0, latitude=0),
                                                       dict(longitude=30, latitude=0)])
        graph = dict(edges=[link(1), link(2), link(3, "fieldChecked"), link(4), link(5, obstacle="stairs")])
        graph["edges"][2]["gradePercent"] = 2
        counts = attach_estimates(graph, terrain, Identity(), SimpleNamespace(eligible={1, 2, 3, 5}, structures={2}))
        self.assertEqual(counts["estimated"], 1)
        self.assertEqual(graph["edges"][0]["forwardMaxUphillPercent"], 10)
        self.assertEqual(graph["edges"][0]["elevationProfile"][-1], [30, 13])
        self.assertEqual([e["gradeEvidence"] for e in graph["edges"]],
                         ["estimated", "unknown", "fieldChecked", "unknown", "unknown"])
        self.assertEqual(graph["edges"][2]["gradePercent"], 2)
        self.assertEqual(graph["edges"][4]["obstacle"], "stairs")


if __name__ == "__main__":
    unittest.main()
