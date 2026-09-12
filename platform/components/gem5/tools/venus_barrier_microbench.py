#!/usr/bin/env python3
"""Build shared-image scalar600/Venus barrier timing oracles.

The four cases isolate the persistent ``vec_barrier_cnt``/``barrier_valid``
state from vector work.  RTL and gem5 execute the same task bytes; only the
post-synthesis case wrapper and termination transport differ.
"""

import argparse
from pathlib import Path

import venus_lsu_microbench as base


def emit_nops(count):
    return ["    nop" for _ in range(count)]


def barrier_case(args):
    ew = 0 if args.ew == 8 else 1
    lines = [
        '    .section .text, "ax", @progbits',
        "    .option norvc",
        "    .4byte 0x00000093",
        "    .global _start",
        "_start:",
        "    li sp, 0x24000",
        f"    li a1, {args.vl}",
        "    li a3, 37",
    ]

    if args.scenario == "idle_guard":
        lines += base.emit_venus(
            base.vbroadcast(base.REG["a1"], base.REG["a3"], 0, ew)
        )
        lines += emit_nops(args.scalar_gap)
        lines += [f"    .4byte 0x{base.VBARRIER:08x}"]
    elif args.scenario == "busy_wait":
        lines += base.emit_venus(
            base.vbroadcast(base.REG["a1"], base.REG["a3"], 0, ew)
        )
        lines += [f"    .4byte 0x{base.VBARRIER:08x}"]
    elif args.scenario == "zero_window":
        lines += base.emit_venus(
            base.vbroadcast(base.REG["a1"], base.REG["a3"], 0, ew)
        )
        lines += [
            f"    .4byte 0x{base.VBARRIER:08x}",
            f"    .4byte 0x{base.VBARRIER:08x}",
        ]
    else:
        # A multi-beat store is fully drained before the barrier enters ID,
        # matching the structural class around nrPDCCH task17 sequence679.
        lines += base.emit_venus(
            base.vbroadcast(base.REG["a1"], base.REG["a3"], 0, ew)
        )
        lines += [f"    .4byte 0x{base.VBARRIER:08x}"]
        lines += base.emit_lsu_base("a0", args.base_address)
        lines += base.emit_venus(
            base.vstore(base.REG["a1"], base.REG["a0"], 0, ew)
        )
        lines += emit_nops(args.scalar_gap)
        lines += [f"    .4byte 0x{base.VBARRIER:08x}"]

    # The common follower exposes the barrier-release -> second-word packet
    # -> sequencer fire -> LSU request/retirement boundary.
    lines += base.emit_lsu_base("a0", args.base_address + 0x4000)
    lines += base.emit_venus(
        base.vload(base.REG["a1"], base.REG["a0"], 32, ew)
    )
    lines += [f"    .4byte 0x{base.VBARRIER:08x}"]
    lines += base.emit_drain_delay()
    lines += base.emit_return()

    contract = {
        "schema": "venus-barrier-microbench/v1",
        "kind": "barrier",
        "scenario": args.scenario,
        "ew_bits": args.ew,
        "vl": args.vl,
        "scalar_gap_nops": args.scalar_gap,
        "base_address": args.base_address,
        "controlled_variables": {
            "lanes": 16,
            "rows": 128,
            "barrier_counter_reset": 6,
            "barrier_busy_synchronizer_depth": 3,
        },
        "timing_oracle": {
            "compare": [
                "ID barrier capture",
                "persistent counter and valid D/Q",
                "delayed vector busy",
                "ID release and scalar retirement",
                "follower packet admission",
                "VLOAD fire and retirement",
            ],
            "idle_guard": "counter-dominated release after prior vector idle",
            "busy_wait": "busy-dominated release after counter expiry",
            "zero_window": "second barrier pass-through before counter rearm",
            "store_idle": "drained multi-beat store then counter-dominated barrier",
        },
        "model_contract": {
            "gem5_profile": "venus-rtl-16x128",
            "gem5_environment": {
                "VENUS_GEM5_EXPERIMENTAL_VRF_RR": "1",
                "VENUS_GEM5_EXPERIMENTAL_REQUESTER_Q_VISIBILITY": "1",
            },
            "tile_period_ps": 2000,
        },
    }
    return "\n".join(lines) + "\n", contract


def parse_args():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output-dir", type=Path, required=True)
    parser.add_argument(
        "--scenario",
        choices=("idle_guard", "busy_wait", "zero_window", "store_idle"),
        required=True,
    )
    parser.add_argument("--ew", type=int, choices=(8, 16), default=8)
    parser.add_argument("--vl", type=int, default=432)
    parser.add_argument("--scalar-gap", type=int, default=160)
    parser.add_argument(
        "--base-address", type=lambda value: int(value, 0), default=0x70000
    )
    args = parser.parse_args()
    if args.vl <= 0 or args.scalar_gap < 0:
        parser.error("--vl must be positive and --scalar-gap non-negative")
    args.kind = "barrier"
    args.operation = args.scenario
    args.streams = 1
    args.blocker_vl = args.vl
    args.scheduler_launch_input = False
    args.gem5_termination = "scheduler_ebreak"
    return args


if __name__ == "__main__":
    base.build_case(parse_args(), case_builder=barrier_case,
                    stem_prefix="barrier")
