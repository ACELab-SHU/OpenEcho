#!/usr/bin/env python3

import sys
import tempfile
import unittest
from pathlib import Path


TOOLS = Path(__file__).resolve().parents[1] / "components/gem5/tools"
sys.path.insert(0, str(TOOLS))

from compare_venus_vrf_bank_cycle_trace import read_gem5  # noqa: E402
from parse_venus_vrf_arbiter_trace import (  # noqa: E402
    ATTEMPT_RE,
    LSU_RE,
    PENDING_RE,
    RR_RE,
)


class VenusVrfTraceParsersTest(unittest.TestCase):
    def test_debug_flag_indentation_does_not_hide_vrf_events(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            trace = Path(tmp) / "gem5.log"
            trace.write_text(
                "  12000: system.cpu.XBAR: Venus VRF request pending "
                "src 10 bank 0 master 9\n"
                "  12000: system.cpu.XBAR: Venus VRF RR grant bank 0 "
                "old_rr 8 winner_master 9 source 10 next_rr 10 "
                "contenders 1\n",
                encoding="utf-8",
            )

            records = read_gem5(trace, lane=0)

            expected_bit = 1 << 9
            self.assertEqual(records[12000]["request"], expected_bit)
            self.assertEqual(records[12000]["grant"], expected_bit)

    def test_all_arbiter_patterns_accept_gem5_leading_space(self) -> None:
        self.assertIsNotNone(ATTEMPT_RE.match(
            "  1: system.cpu.VenusLane_0: VFU VRF grant passage -3 "
            "instr 2/rid 3 offset 4 size 5 accepted 0 reason blocked"
        ))
        self.assertIsNotNone(PENDING_RE.match(
            "  1: system.cpu.XBAR: Venus VRF request pending "
            "src 10 bank 0 master 9"
        ))
        self.assertIsNotNone(RR_RE.match(
            "  1: system.cpu.XBAR: Venus VRF RR grant bank 0 old_rr 8 "
            "winner_master 9 source 10 next_rr 10 contenders 1"
        ))
        self.assertIsNotNone(LSU_RE.match(
            "  1: system.cpu.XBAR: Venus VRF bank 0 blocked by LSU priority"
        ))


if __name__ == "__main__":
    unittest.main()
