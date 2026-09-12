#!/usr/bin/env python3

import json
import sys
import tempfile
import unittest
from pathlib import Path


TOOLS = Path(__file__).resolve().parents[1] / "components/gem5/tools"
sys.path.insert(0, str(TOOLS))

from compare_vins_lifecycle_timing import (  # noqa: E402
    compare,
    read_gem5,
    read_rtl,
    select_task,
)


class CompareVinsLifecycleTimingTest(unittest.TestCase):
    def test_focused_replay_task_ids_can_be_remapped(self) -> None:
        source = {34: [{"ordinal": 0}], 35: [{"ordinal": 1}]}

        selected = select_task(source, 34)

        self.assertEqual(selected, {0: [{"ordinal": 0}]})
        self.assertIs(select_task(source, None), source)

    def test_native_tile_started_event_is_accepted(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            trace = root / "trace.jsonl"
            trace.write_text(
                json.dumps({"event": "tile_started", "task_id": 7,
                            "tick": 1000}) + "\n" +
                json.dumps({"event": "task_epilogue", "task_id": 7,
                            "tick": 1600}) + "\n",
                encoding="utf-8",
            )
            monitor = root / "monitor.json"
            monitor.write_text(json.dumps([{"Venus_instr": [
                {"venus_instr_counter": 0, "id": 0, "op_s": "VADD",
                 "vfu_s": "VFU_CAU", "vl": 4, "vew_s": "EW8",
                 "fire_tick": 1100, "recycle_tick": 1200},
            ]}]), encoding="utf-8")

            records = read_gem5(trace, monitor, 1.0)

            self.assertEqual(set(records), {7})
            self.assertEqual(records[7][0]["relative_start_ps"], 100.0)

    def test_reused_running_id_is_paired_by_fifo(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            rtl = root / "rtl.log"
            rtl.write_text(
                "vins 0 complete at task 0 tile 0 at time 0\n"
                "task 0 start execute at tile 0 at time 1000\n"
                "vins 0 start at task 0 tile 0 at time 1100\n"
                "vins 0 complete at task 0 tile 0 at time 1200\n"
                "vins 0 start at task 0 tile 0 at time 1300\n"
                "vins 0 complete at task 0 tile 0 at time 1500\n",
                encoding="utf-8",
            )
            trace = root / "trace.jsonl"
            trace.write_text(
                json.dumps({"event": "tile_start", "task_id": 0,
                            "tick": 1000}) + "\n" +
                json.dumps({"event": "task_epilogue", "task_id": 0,
                            "tick": 1600}) + "\n",
                encoding="utf-8",
            )
            monitor = root / "monitor.json"
            monitor.write_text(json.dumps([{"Venus_instr": [
                {"venus_instr_counter": 0, "id": 0, "op_s": "VADD",
                 "vfu_s": "VFU_CAU", "vl": 4, "vew_s": "EW8",
                 "fire_tick": 1100, "recycle_tick": 1200},
                {"venus_instr_counter": 1, "id": 0, "op_s": "VADD",
                 "vfu_s": "VFU_CAU", "vl": 4, "vew_s": "EW8",
                 "fire_tick": 1300, "recycle_tick": 1500},
            ]}]), encoding="utf-8")

            report = compare(
                read_rtl(rtl, 1.0), read_gem5(trace, monitor, 1.0))

            rows = report["tasks"][0]["instructions"]
            self.assertEqual([row["ordinal"] for row in rows], [0, 1])
            self.assertEqual([row["duration_delta_ns"] for row in rows],
                             [0.0, 0.0])

    def test_bind_observer_edges_are_assigned_to_task_interval(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            task_log = root / "tasks.log"
            task_log.write_text(
                "task 3 start execute at tile 0 at time 1000\n"
                "task 3 execute complete at tile 0 at time 1800\n",
                encoding="utf-8",
            )
            vins_log = root / "sim.log"
            vins_log.write_text(
                "ACE_ECHO_VINS 0 complete tile 0 time 0\n"
                "ACE_ECHO_VINS 7 start tile 0 time 900\n"
                "ACE_ECHO_VINS 7 complete tile 0 time 950\n"
                "ACE_ECHO_VINS 0 start tile 0 time 1100\n"
                "ACE_ECHO_VINS 0 complete tile 0 time 1500\n",
                encoding="utf-8",
            )

            records = read_rtl(task_log, 1.0, vins_log)

            self.assertEqual(set(records), {3})
            self.assertEqual(records[3][0]["relative_start_ps"], 100.0)
            self.assertEqual(records[3][0]["complete_ps"], 1500.0)


if __name__ == "__main__":
    unittest.main()
