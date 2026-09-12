#!/usr/bin/env python3
"""Build minimal, shared-image Shuffle timing probes for RTL and gem5.

The cases deliberately separate an already-drained Shuffle command from
producer- and LSU-dependent commands.  They are timing oracles for hazard
admission and requester visibility, not full-DAG latency fitting cases.
"""

import argparse
from pathlib import Path
from types import SimpleNamespace

import venus_lsu_microbench as base


def vshuffle(avl_reg, vs1, vs2, vd, ew, gather=False):
    vm_r = 1 if gather else 0
    return (
        base.venus_low(0x08, ew, 0, avl_reg, 0x5B) | (vm_r << 25),
        base.operand3(vs1, vs2, vd),
    )


def shuffle_case(args):
    ew = 0 if args.ew == 8 else 1
    index_row = 0
    data_row = 8
    dest_row = 16
    lines = [
        '    .section .text, "ax", @progbits',
        "    .option norvc",
        "    .4byte 0x00000093",
        "    .global _start",
        "_start:",
        "    li sp, 0x24000",
        f"    li a1, {args.vl}",
        "    li a3, 0",
    ]
    lines += base.emit_venus(
        base.vbroadcast(base.REG["a1"], base.REG["a3"], index_row, 1)
    )

    if args.dependency == "lsu":
        lines += base.emit_lsu_base("a0", args.base_address)
        lines += base.emit_venus(
            base.vload(base.REG["a1"], base.REG["a0"], data_row, ew)
        )
    else:
        lines += ["    li a4, 37"]
        lines += base.emit_venus(
            base.vbroadcast(base.REG["a1"], base.REG["a4"], data_row, ew)
        )

    if args.dependency == "independent":
        lines += [f"    .4byte 0x{base.VBARRIER:08x}"]

    lines += base.emit_venus(
        vshuffle(
            base.REG["a1"], data_row, index_row, dest_row, ew,
            gather=args.operation == "gather",
        )
    )
    lines += [f"    .4byte 0x{base.VBARRIER:08x}"]
    lines += base.emit_drain_delay()
    lines += base.emit_return()

    contract = {
        "schema": "venus-shuffle-microbench/v1",
        "kind": "shuffle",
        "operation": args.operation,
        "dependency": args.dependency,
        "ew_bits": args.ew,
        "vl": args.vl,
        "streams": 1,
        "rows": {
            "index": index_row,
            "data": data_row,
            "destination": dest_row,
        },
        "controlled_variables": {
            "lanes": 16,
            "rows": 128,
            "index_ew_bits": 16,
            "data_ew_bits": args.ew,
            "index_value": 0,
            "broadcast_data_value": 37,
            "unit_stride_lsu": args.dependency == "lsu",
        },
        "timing_oracle": {
            "independent": "empty captured hazard before Shuffle admission",
            "producer": "tagged VBRDCST completion clears captured hazard",
            "lsu": "tagged VLOAD completion clears captured hazard",
            "compare": [
                "issue admission",
                "hazard clear",
                "state q/d",
                "request q/d",
                "per-bank grant",
                "retirement",
            ],
        },
        "model_contract": {
            "gem5_profile": "venus-rtl-16x128",
            "gem5_environment": {
                "VENUS_GEM5_EXPERIMENTAL_VRF_RR": "1",
                "VENUS_GEM5_EXPERIMENTAL_REQUESTER_Q_VISIBILITY": "1",
                "VENUS_GEM5_EXPERIMENTAL_ONE_ENTRY_OPERAND_COMMANDS": "1",
            },
            "tile_period_ps": 2000,
            "rtl_scope": (
                "tile-local requester, result queue, VRF arbitration, "
                "Shuffle pipeline, and retirement edges"
            ),
            "lsu_topology_warning": (
                "The postsyn tile testbench external AXI BFM is not the "
                "full-cluster shared-L2 path. Never use its VLOAD/VSTORE "
                "memory-response latency to tune VenusSharedL2."
            ),
        },
    }
    return "\n".join(lines) + "\n", contract


def parse_args():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output-dir", type=Path, required=True)
    parser.add_argument("--operation", choices=("scatter", "gather"),
                        default="scatter")
    parser.add_argument("--dependency",
                        choices=("independent", "producer", "lsu"),
                        required=True)
    parser.add_argument("--ew", type=int, choices=(8, 16), default=8)
    parser.add_argument("--vl", type=int, default=63)
    parser.add_argument("--base-address", type=lambda value: int(value, 0),
                        default=0x70000)
    args = parser.parse_args()
    if args.vl <= 0:
        parser.error("--vl must be positive")
    return args


def main():
    parsed = parse_args()
    build_args = SimpleNamespace(
        output_dir=parsed.output_dir,
        kind="shuffle",
        operation=parsed.operation,
        dependency=parsed.dependency,
        ew=parsed.ew,
        vl=parsed.vl,
        streams=1,
        base_address=parsed.base_address,
        # The postsyn tile testbench consumes inline JSON input data, while
        # this probe needs no runtime payload.  Keep Input_Num at zero so the
        # exact same code-only image is valid in both RTL and gem5.
        scheduler_launch_input=False,
        gem5_termination="scheduler_ebreak",
    )
    base.build_case(
        build_args,
        case_builder=shuffle_case,
        stem_prefix=f"shuffle_{parsed.dependency}",
    )


if __name__ == "__main__":
    main()
