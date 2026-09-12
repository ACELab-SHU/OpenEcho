#!/usr/bin/env python3
"""Compare the complete per-lane VRF bank arbiter vectors by cycle.

The RTL probe exposes four banks of twelve low-priority masters as packed
``req_lvl2`` and ``gnt_lvl2`` vectors plus the independent LSU request mask.
The gem5 trace exposes the same information as tagged pending requests,
accepted RR grants, Shuffle live intents, and LSU-priority observations.
This tool normalizes those representations without using instruction or task
identity, so it can also be used by directed micro tests.
"""

import argparse
import json
import re
from pathlib import Path


RTL_RE = re.compile(
    r"edge=(?P<edge>\d+).*?"
    r"lane0_req_lvl2='b(?P<request>[01]+).*?"
    r"lane0_gnt_lvl2='b(?P<grant>[01]+).*?"
    r"lane0_req_ls='b(?P<lsu>[01]+)"
)
PENDING_RE = re.compile(
    r"^\s*(?P<tick>\d+): .*?Venus VRF request pending "
    r"src (?P<source>\d+) bank (?P<bank>\d+) master (?P<master>\d+)"
)
GRANT_RE = re.compile(
    r"^\s*(?P<tick>\d+): .*?Venus VRF RR grant bank (?P<bank>\d+) "
    r".*?winner_master (?P<master>\d+) source (?P<source>\d+)"
)
SHUFFLE_RE = re.compile(
    r"^\s*(?P<tick>\d+): .*?SH_LIVE_INTENT lane=(?P<lane>\d+) "
    r".*?bank=(?P<bank>\d+)"
)
LSU_RE = re.compile(
    r"^\s*(?P<tick>\d+): .*?Venus VRF bank (?P<bank>\d+) "
    r"blocked by LSU priority"
)


def empty_record():
    return {"request": 0, "grant": 0, "lsu": 0}


def read_rtl(path: Path):
    records = {}
    with path.open(errors="replace") as stream:
        for raw in stream:
            match = RTL_RE.search(raw)
            if match:
                records[int(match.group("edge"))] = {
                    "request": int(match.group("request"), 2),
                    "grant": int(match.group("grant"), 2),
                    "lsu": int(match.group("lsu"), 2),
                }
    return records


def read_gem5(path: Path, lane: int):
    records = {}
    first_source = lane * 16
    first_bank = lane * 4

    def at(tick):
        return records.setdefault(tick, empty_record())

    def local_bank(bank):
        bank -= first_bank
        return bank if 0 <= bank < 4 else None

    with path.open(errors="replace") as stream:
        for raw in stream:
            match = PENDING_RE.search(raw)
            if match:
                source = int(match.group("source"))
                bank = local_bank(int(match.group("bank")))
                if first_source <= source < first_source + 16 and bank is not None:
                    at(int(match.group("tick")))["request"] |= (
                        1 << (bank * 12 + int(match.group("master")))
                    )
                continue

            match = SHUFFLE_RE.search(raw)
            if match and int(match.group("lane")) == lane:
                bank = local_bank(int(match.group("bank")))
                if bank is not None:
                    at(int(match.group("tick")))["request"] |= (
                        1 << (bank * 12 + 11)
                    )
                continue

            match = GRANT_RE.search(raw)
            if match:
                source = int(match.group("source"))
                bank = local_bank(int(match.group("bank")))
                if first_source <= source < first_source + 16 and bank is not None:
                    bit = 1 << (bank * 12 + int(match.group("master")))
                    # An immediately accepted timing request has a grant log
                    # but never enters the pending-request path.  The RTL
                    # req_lvl2 vector still contains that request during the
                    # grant edge, so reconstruct both sides of the handshake.
                    at(int(match.group("tick")))["request"] |= bit
                    at(int(match.group("tick")))["grant"] |= bit
                continue

            match = LSU_RE.search(raw)
            if match:
                bank = local_bank(int(match.group("bank")))
                if bank is not None:
                    at(int(match.group("tick")))["lsu"] |= 1 << bank
    return records


def rendered_record(record):
    return {key: f"0x{value:x}" for key, value in record.items()}


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--rtl-trace", type=Path, required=True)
    parser.add_argument("--gem5-trace", type=Path, required=True)
    parser.add_argument("--rtl-anchor-edge", type=int, required=True)
    parser.add_argument("--gem5-anchor-tick", type=int, required=True)
    parser.add_argument("--cycles", type=int, required=True)
    parser.add_argument("--lane", type=int, default=0, choices=range(16))
    parser.add_argument("--rtl-edge-stride", type=int, default=2)
    parser.add_argument("--gem5-tick-stride", type=int, default=2000)
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()

    rtl = read_rtl(args.rtl_trace)
    gem5 = read_gem5(args.gem5_trace, args.lane)
    comparisons = []
    first_divergence = None
    for cycle in range(args.cycles):
        rtl_edge = args.rtl_anchor_edge + cycle * args.rtl_edge_stride
        gem5_tick = args.gem5_anchor_tick + cycle * args.gem5_tick_stride
        rtl_record = rtl.get(rtl_edge, empty_record())
        gem5_record = gem5.get(gem5_tick, empty_record())
        differences = {
            field: {
                "rtl": f"0x{rtl_record[field]:x}",
                "gem5": f"0x{gem5_record[field]:x}",
            }
            for field in ("request", "grant", "lsu")
            if rtl_record[field] != gem5_record[field]
        }
        comparison = {
            "cycle": cycle,
            "rtl_edge": rtl_edge,
            "gem5_tick": gem5_tick,
            "rtl": rendered_record(rtl_record),
            "gem5": rendered_record(gem5_record),
            "differences": differences,
        }
        comparisons.append(comparison)
        if differences and first_divergence is None:
            first_divergence = comparison

    report = {
        "schema": "venus-vrf-bank-cycle-comparison/v1",
        "lane": args.lane,
        "cycles": args.cycles,
        "pass": first_divergence is None,
        "first_divergence": first_divergence,
        "comparisons": comparisons,
    }
    rendered = json.dumps(report, indent=2, sort_keys=True)
    if args.output:
        args.output.write_text(rendered + "\n")
    print(rendered)
    raise SystemExit(0 if first_divergence is None else 1)


if __name__ == "__main__":
    main()
