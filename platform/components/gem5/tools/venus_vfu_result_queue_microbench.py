#!/usr/bin/env python3
"""Build an isolated BitALU/CAU/SerDiv result-queue capacity probe.

The program initializes two vectors, then runs one result-producing command
per VFU with barriers between classes.  A runner can vary only the physical
VRF request grant gap and use LaneVFU queue events as the capacity oracle.
"""

import argparse
from pathlib import Path
from types import SimpleNamespace

import venus_lsu_microbench as base


def vdiv(avl_reg, vs1, vs2, vd, ew):
    return base.venus_low(0x0C, ew, 0, avl_reg, 0x2B), base.operand3(
        vs1, vs2, vd
    )


def queue_case(args):
    ew = 0 if args.ew == 8 else 1
    lines = [
        '    .section .text, "ax", @progbits',
        "    .option norvc",
        "    .4byte 0x00000093",
        "    .global _start",
        "_start:",
        "    li sp, 0x24000",
        f"    li a1, {args.vl}",
        "    li a3, 32",
    ]
    lines += base.emit_venus(
        base.vbroadcast(base.REG["a1"], base.REG["a3"], 0, ew)
    )
    lines += ["    li a4, 4"]
    lines += base.emit_venus(
        base.vbroadcast(base.REG["a1"], base.REG["a4"], 4, ew)
    )
    lines += [f"    .4byte 0x{base.VBARRIER:08x}"]
    lines += base.emit_venus(
        base.vadd(base.REG["a1"], 0, 4, 8, ew)
    )
    lines += [f"    .4byte 0x{base.VBARRIER:08x}"]
    lines += base.emit_venus(
        vdiv(base.REG["a1"], 8, 4, 12, ew)
    )
    lines += [f"    .4byte 0x{base.VBARRIER:08x}"]
    lines += base.emit_drain_delay()
    lines += base.emit_return()
    contract = {
        "schema": "venus-vfu-result-queue-microbench/v1",
        "kind": "result_queue",
        "operation": "bitalu_cau_serdiv",
        "ew_bits": args.ew,
        "vl": args.vl,
        "streams": 1,
        "dependency": "barrier_isolated_vfu_classes",
        "expected_values": {
            "bitalu_vd0": 32,
            "bitalu_vd4": 4,
            "cau_vd8": 36,
            "serdiv_vd12": 9,
        },
        "capacity_oracle": {
            "queue_depth": 2,
            "required_events": [
                "result-queue enqueue ... occupancy 2/2",
                "result-queue full ... occupancy 2/2",
            ],
            "functional_rule": "all VINS dumps match the 1ns control",
        },
        "controlled_variables": {
            "lanes": 16,
            "rows": 128,
            "masked": False,
            "vrf_response_latency": "unchanged",
            "only_swept_variable": "physical VRF request_gap",
        },
    }
    return "\n".join(lines) + "\n", contract


def parse_args():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output-dir", type=Path, required=True)
    parser.add_argument("--ew", type=int, choices=(8, 16), default=16)
    parser.add_argument("--vl", type=int, default=256)
    args = parser.parse_args()
    if args.vl <= 0:
        parser.error("--vl must be positive")
    return args


def main():
    parsed = parse_args()
    build_args = SimpleNamespace(
        output_dir=parsed.output_dir,
        kind="result_queue",
        operation="bitalu_cau_serdiv",
        ew=parsed.ew,
        vl=parsed.vl,
        streams=1,
    )
    base.build_case(
        build_args,
        case_builder=queue_case,
        stem_prefix="vfu",
    )


if __name__ == "__main__":
    main()
