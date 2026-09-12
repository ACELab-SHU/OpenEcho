from __future__ import annotations

from pathlib import Path
import json
import tempfile
import unittest

from ace_echo.dag_output_compare import compare_dag_outputs, compare_dma_returns


class DagOutputCompareTests(unittest.TestCase):
    def test_reverses_each_dma_beat_and_ignores_only_padding_x(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            expected = root / "dma.txt"
            actual = root / "actual"
            actual.mkdir()
            expected.write_text(
                "src: 0 | dst: 0 | len: 00000003 | tid: 2 | "
                "tname: Task_test | retid: 1\n"
                "data:\n"
                "xxxxxxxxxx030201\n\n",
                encoding="utf-8",
            )
            (actual / "task_2_port_1.bin").write_bytes(b"\x01\x02\x03")
            report = compare_dag_outputs(expected, actual)
            self.assertEqual(report["status"], "PASS")
            self.assertEqual(report["known_expected_bytes"], 3)

    def test_missing_output_fails(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            expected = root / "dma.txt"
            actual = root / "actual"
            actual.mkdir()
            expected.write_text(
                "src: 0 | dst: 0 | len: 00000001 | tid: 0 | "
                "tname: Task_test | retid: 0\n"
                "data:\n01\n",
                encoding="utf-8",
            )
            report = compare_dag_outputs(expected, actual)
            self.assertEqual(report["status"], "FAIL")

    def test_legacy_trace_uses_semantic_output_template(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            expected = root / "legacy-dma.txt"
            trace = root / "venus_dag_trace.jsonl"
            actual = root / "actual"
            actual.mkdir()
            expected.write_text(
                "src: 80000100 | dst: 82100000 | len: 00000002\n"
                "data:\n" + "00" * 62 + "bbaa\n\n"
                "src: 82101200 | dst: 80002000 | len: 00000003\n"
                "data:\n" + "00" * 61 + "030201\n\n",
                encoding="utf-8",
            )
            trace.write_text(json.dumps({
                "tick": 10,
                "event": "dma_output",
                "task_id": 4,
                "task_name": "Task_legacy",
                "output_port": 2,
                "source": "0x101200",
                "bytes": 3,
            }) + "\n", encoding="utf-8")
            (actual / "task_4_port_2.bin").write_bytes(b"\x01\x02\x03")
            report = compare_dag_outputs(expected, actual, trace)
            self.assertEqual(report["status"], "PASS")
            self.assertEqual(report["outputs"], 1)
            self.assertEqual(report["results"][0]["rtl_source"], "0x82101200")

    def test_legacy_trace_accepts_runtime_return_admit_event(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            expected = root / "legacy-dma.txt"
            trace = root / "venus_dag_trace.jsonl"
            actual = root / "actual"
            actual.mkdir()
            expected.write_text(
                "src: 82102000 | dst: 80004000 | len: 00000002\n"
                "data:\n" + "00" * 62 + "2211\n\n",
                encoding="utf-8",
            )
            trace.write_text(json.dumps({
                "tick": 10,
                "event": "return_admit",
                "task_id": 7,
                "task_name": "runtime_task_7",
                "retid": 3,
                "source": "0x82102000",
                "bytes": 2,
            }) + "\n", encoding="utf-8")
            (actual / "task_7_port_3.bin").write_bytes(b"\x11\x22")
            report = compare_dag_outputs(expected, actual, trace)
            self.assertEqual(report["status"], "PASS")
            self.assertEqual(report["outputs"], 1)

    def test_compares_two_dma_return_traces(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            expected = root / "expected.txt"
            actual = root / "actual.txt"
            record = (
                "src: 0 | dst: 0 | len: 00000004 | tid: 3 | "
                "tname: Task_three | retid: 1\n"
                "data:\n"
                + "00" * 60 + "04030201\n"
            )
            expected.write_text(record, encoding="utf-8")
            actual.write_text(record, encoding="utf-8")
            report = compare_dma_returns(expected, actual)
            self.assertEqual(report["status"], "PASS")
            self.assertEqual(report["passed_returns"], 1)
            self.assertEqual(report["compared_known_bytes"], 4)

    def test_dma_trace_rejects_missing_and_extra_returns(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            expected = root / "expected.txt"
            actual = root / "actual.txt"
            expected.write_text(
                "src: 0 | dst: 0 | len: 00000001 | tid: 1 | "
                "tname: Task_one | retid: 0\ndata:\n" + "00" * 63 + "01\n",
                encoding="utf-8",
            )
            actual.write_text(
                "src: 0 | dst: 0 | len: 00000001 | tid: 2 | "
                "tname: Task_two | retid: 0\ndata:\n" + "00" * 63 + "02\n",
                encoding="utf-8",
            )
            report = compare_dma_returns(expected, actual)
            self.assertEqual(report["status"], "FAIL")
            self.assertEqual(report["results"][0]["reason"], "missing_return")
            self.assertEqual(len(report["extra_returns"]), 1)


if __name__ == "__main__":
    unittest.main()
