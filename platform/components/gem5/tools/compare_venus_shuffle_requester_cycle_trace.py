#!/usr/bin/env python3
"""Compare one Shuffle lane's two-level requester/grant signals by cycle.

The RTL debug dump deliberately exposes the two sides in their native packed
orientations: ``req_q`` is PE-major (16 lanes per PE), while ``gnt`` is
lane-major (16 PEs per lane).  This tool normalizes both to a 16-bit PE mask
for one lane and compares them with the gem5 live-requester sideband trace.

Only lane 0 has an RTL level-2 VRF-bank/LSU oracle.  The packed level-1
request/grant vectors cover all 16 lanes, so other lanes can still be checked
without pretending that their outer-bank signals were observed.  Anchors make
the comparison local to an instruction phase without assuming that full-DAG
and captured-replay preambles have equal length.
"""

import argparse
import json
import re
from pathlib import Path


RTL_RE = re.compile(
    r"RTLSH edge=(?P<edge>\d+).*?"
    r"req_q='b(?P<req>[01]+).*?"
    r"gnt='b(?P<gnt>[01]+).*?"
    r"lane0_req_lvl2='b(?P<outer_req>[01]+).*?"
    r"lane0_gnt_lvl2='b(?P<outer_gnt>[01]+).*?"
    r"lane0_req_ls='b(?P<lsu>[01]+)"
)
GEM_INTENT_RE = re.compile(
    r"^(?P<tick>\d+): .*?\[(?P=tick)\] SH_LIVE_INTENT "
    r"lane=(?P<lane>\d+) .*?req=(?P<req>0x[0-9a-fA-F]+) "
    r"winner=PE(?P<winner>\d+) .*?bank=(?P<bank>\d+)"
)
GEM_GRANT_RE = re.compile(
    r"^(?P<tick>\d+): .*?\[(?P=tick)\] SH_LIVE_GRANT "
    r"lane=(?P<lane>\d+) .*?req=(?P<req>0x[0-9a-fA-F]+) "
    r".*?grant=PE(?P<winner>\d+)"
)
GEM_LSU_BLOCK_RE = re.compile(
    r"^(?P<tick>\d+): .*?Venus VRF bank (?P<bank>\d+) "
    r"blocked by LSU priority"
)


def pe_major_lane_mask(value: int, lane: int) -> int:
    """Transpose [PE][lane] into a PE mask for one lane."""
    return sum(
        ((value >> (pe * 16 + lane)) & 1) << pe
        for pe in range(16)
    )


def lane_major_pe_mask(value: int, lane: int) -> int:
    """Select one [lane][PE] 16-bit chunk."""
    return (value >> (lane * 16)) & 0xFFFF


def shuffle_bank_mask(value: int) -> int:
    """Select master 11 (Shuffle) from four [bank][master] groups."""
    return sum(
        ((value >> (bank * 12 + 11)) & 1) << bank
        for bank in range(4)
    )


def read_rtl(path: Path, lane: int):
    records = {}
    with path.open(errors="replace") as stream:
        for raw in stream:
            match = RTL_RE.search(raw)
            if not match:
                continue
            edge = int(match.group("edge"))
            req = int(match.group("req"), 2)
            gnt = int(match.group("gnt"), 2)
            records[edge] = {
                "inner_request": pe_major_lane_mask(req, lane),
                "inner_grant": lane_major_pe_mask(gnt, lane),
                "outer_request_banks": shuffle_bank_mask(
                    int(match.group("outer_req"), 2)
                ),
                "outer_grant_banks": shuffle_bank_mask(
                    int(match.group("outer_gnt"), 2)
                ),
                "lsu_request_banks": int(match.group("lsu"), 2),
            }
    return records


def read_gem5(path: Path, lane: int):
    records = {}

    def at(tick):
        return records.setdefault(tick, {
            "inner_request": 0,
            "inner_grant": 0,
            "outer_request_banks": 0,
            "outer_grant_banks": 0,
            "lsu_request_banks": 0,
        })

    with path.open(errors="replace") as stream:
        for raw in stream:
            match = GEM_INTENT_RE.search(raw)
            if match and int(match.group("lane")) == lane:
                tick = int(match.group("tick"))
                record = at(tick)
                record["inner_request"] = int(match.group("req"), 16)
                bank = int(match.group("bank")) - lane * 4
                if 0 <= bank < 4:
                    record["outer_request_banks"] |= 1 << bank
                continue

            match = GEM_GRANT_RE.search(raw)
            if match and int(match.group("lane")) == lane:
                tick = int(match.group("tick"))
                record = at(tick)
                record["inner_grant"] |= 1 << int(match.group("winner"))
                # A live-grant callback is emitted only after the outer bank
                # accepts this lane's published intent.
                record["outer_grant_banks"] |= record[
                    "outer_request_banks"
                ]
                continue

            match = GEM_LSU_BLOCK_RE.search(raw)
            if match:
                bank = int(match.group("bank")) - lane * 4
                if 0 <= bank < 4:
                    at(int(match.group("tick")))[
                        "lsu_request_banks"
                    ] |= 1 << bank
    return records


def hex_record(record):
    return {key: f"0x{value:x}" for key, value in record.items()}


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--rtl-trace", type=Path, required=True)
    parser.add_argument("--gem5-trace", type=Path, required=True)
    parser.add_argument("--rtl-anchor-edge", type=int, required=True)
    parser.add_argument("--gem5-anchor-tick", type=int, required=True)
    parser.add_argument("--cycles", type=int, default=16)
    parser.add_argument("--rtl-edge-stride", type=int, default=2)
    parser.add_argument("--gem5-tick-stride", type=int, default=2000)
    parser.add_argument("--lane", type=int, default=0,
                        choices=range(16))
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()

    rtl = read_rtl(args.rtl_trace, args.lane)
    gem5 = read_gem5(args.gem5_trace, args.lane)
    empty = {
        "inner_request": 0,
        "inner_grant": 0,
        "outer_request_banks": 0,
        "outer_grant_banks": 0,
        "lsu_request_banks": 0,
    }
    compared_fields = (
        tuple(empty) if args.lane == 0 else
        ("inner_request", "inner_grant")
    )
    comparisons = []
    first_divergence = None
    for cycle in range(args.cycles):
        rtl_edge = args.rtl_anchor_edge + cycle * args.rtl_edge_stride
        gem5_tick = args.gem5_anchor_tick + cycle * args.gem5_tick_stride
        rtl_record = rtl.get(rtl_edge, empty)
        gem5_record = gem5.get(gem5_tick, empty)
        differences = {
            key: {
                "rtl": f"0x{rtl_record[key]:x}",
                "gem5": f"0x{gem5_record[key]:x}",
            }
            for key in compared_fields
            if rtl_record[key] != gem5_record[key]
        }
        comparison = {
            "cycle": cycle,
            "rtl_edge": rtl_edge,
            "gem5_tick": gem5_tick,
            "rtl": hex_record(rtl_record),
            "gem5": hex_record(gem5_record),
            "differences": differences,
        }
        comparisons.append(comparison)
        if differences and first_divergence is None:
            first_divergence = comparison

    report = {
        "schema": "venus-shuffle-requester-cycle-comparison/v1",
        "lane": args.lane,
        "compared_fields": list(compared_fields),
        "cycles": args.cycles,
        "pass": first_divergence is None,
        "first_divergence": first_divergence,
        "comparisons": comparisons,
    }
    rendered = json.dumps(report, indent=2, sort_keys=True)
    if args.output:
        args.output.write_text(rendered + "\n")
    print(rendered)
    raise SystemExit(0 if report["pass"] else 1)


if __name__ == "__main__":
    main()
