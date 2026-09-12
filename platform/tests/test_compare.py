from pathlib import Path
import tempfile
import unittest

from ace_echo.compare import compare_files


class CompareTests(unittest.TestCase):
    def test_i8_normalizes_signed_values(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            expected = root / "expected.txt"
            actual = root / "actual.txt"
            expected.write_text("-1 0 127\n", encoding="utf-8")
            actual.write_text("255 0 127\n", encoding="utf-8")
            self.assertEqual(compare_files(expected, actual, "i8").status, "PASS")

    def test_reports_first_mismatch(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            expected = root / "expected.bin"
            actual = root / "actual.bin"
            expected.write_bytes(b"abc")
            actual.write_bytes(b"axc")
            result = compare_files(expected, actual, "raw")
            self.assertEqual(result.status, "FAIL")
            self.assertEqual(result.first_mismatch, 1)


if __name__ == "__main__":
    unittest.main()

