import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "Tools"))
from import_kric import extract_steps


class KricImportTests(unittest.TestCase):
    def test_only_chosen_official_route_is_ordered(self):
        payload = {"body": {"items": [
            {"mvPathMgNo": "lift-route", "exitMvTpOrdr": "2", "mvContDtl": "엘리베이터를 이용합니다", "stMovePath": "1번 출입구", "edMovePath": "승강장"},
            {"mvPathMgNo": "other", "exitMvTpOrdr": "1", "mvContDtl": "계단을 이용합니다"},
            {"mvPathMgNo": "lift-route", "exitMvTpOrdr": "1", "mvContDtl": "출입구에서 직진합니다", "stMovePath": "1번 출입구", "edMovePath": "엘리베이터"},
        ]}}
        steps = extract_steps(payload, "lift-route")
        self.assertEqual([step["order"] for step in steps], [1, 2])
        self.assertEqual(steps[1]["text"], "엘리베이터를 이용합니다")

    def test_stairs_or_missing_lift_are_rejected(self):
        stairs = [{"mvPathMgNo": "one", "exitMvTpOrdr": 1, "mvContDtl": "계단으로 내려갑니다"}]
        self.assertRaises(ValueError, extract_steps, stairs, "one")
        plain = [{"mvPathMgNo": "one", "exitMvTpOrdr": 1, "mvContDtl": "통로를 따라갑니다"}]
        self.assertRaises(ValueError, extract_steps, plain, "one")
        hidden_stairs = [{"mvPathMgNo": "one", "exitMvTpOrdr": 1, "mvContDtl": "엘리베이터를 이용합니다", "stMovePath": "계단 앞"}]
        self.assertRaises(ValueError, extract_steps, hidden_stairs, "one")


if __name__ == "__main__":
    unittest.main()
