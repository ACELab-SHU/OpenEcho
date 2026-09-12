#!/usr/bin/env python3
"""Condense a raw VSTU RTL signal capture into tile-clock transitions.

The xsim capture scripts used by the Venus alignment work sample every 1 ns,
while the tile state advances every 2 ns.  Sampling exactly on a simulated
clock transition can make the printed clock level depend on delta-cycle
ordering, so this tool uses an explicit raw-edge anchor and stride instead of
guessing rising edges from ``clk``.  It deliberately does not infer a latency
or align to a gem5 task: the output is a reusable RTL contract for the
instruction queue, operand ping-pong rows, AXI W/B path, and registered
response boundary.
"""

import argparse
import json
import re
from pathlib import Path


PAIR_RE = re.compile(r"([A-Za-z_][A-Za-z0-9_]*)=([^\s]+)")
DEFAULT_FIELDS = (
    "seq_state",
    "seq_issue",
    "seq_pe_valid",
    "pe_valid",
    "pe_ready",
    "addr_ack",
    "ag_state",
    "aw_valid",
    "aw_ready",
    "ag_valid",
    "ag_stu_ready",
    "stu_req",
    "stu_valid",
    "pp_valid",
    "pp_valid_q",
    "active_row",
    "issue_pnt",
    "commit_pnt",
    "accept_pnt",
    "issue_bytes",
    "issue_bytes_d1",
    "w_valid",
    "w_ready",
    "wlast",
    "int_b_valid",
    "int_b_ready",
    "bus_aw_valid",
    "bus_aw_ready",
    "bus_w_valid",
    "bus_w_ready",
    "bus_wlast",
    "bus_b_valid",
    "bus_b_ready",
    "ext_aw_valid",
    "ext_aw_ready",
    "ext_w_valid",
    "ext_w_ready",
    "ext_wlast",
    "ext_b_valid",
    "ext_b_ready",
    "store_done",
    "resp_done",
    "issue_cnt",
    "commit_cnt",
)


def parse_value(raw):
    if raw.startswith("'b"):
        bits = raw[2:].lower()
        if any(bit not in "01" for bit in bits):
            return raw
        return int(bits, 2)
    try:
        return int(raw, 0)
    except ValueError:
        return raw


def read_rows(path):
    rows = []
    with path.open() as stream:
        for line_number, raw in enumerate(stream, 1):
            values = {
                key: parse_value(value)
                for key, value in PAIR_RE.findall(raw)
            }
            if "edge" not in values or "clk" not in values:
                continue
            values["_line"] = line_number
            rows.append(values)
    if not rows:
        raise ValueError(f"no STU signal rows found in {path}")
    return rows


def sampled_edges(rows, anchor, stride):
    return [
        row for row in rows
        if row["edge"] >= anchor and (row["edge"] - anchor) % stride == 0
    ]


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--trace", type=Path, required=True)
    parser.add_argument("--output", type=Path)
    parser.add_argument(
        "--sample-start-edge", type=int,
        help="first raw edge to retain (default: first edge in capture)",
    )
    parser.add_argument(
        "--edge-stride", type=int, default=2,
        help="raw-edge spacing between tile samples (default: 2)",
    )
    parser.add_argument(
        "--fields", nargs="+", default=DEFAULT_FIELDS,
        help="signal fields to include (default: all VSTU lifecycle fields)",
    )
    args = parser.parse_args()
    if args.edge_stride <= 0:
        parser.error("--edge-stride must be positive")

    rows = read_rows(args.trace)
    anchor = (
        rows[0]["edge"]
        if args.sample_start_edge is None else args.sample_start_edge
    )
    samples = sampled_edges(rows, anchor, args.edge_stride)
    if not samples:
        raise ValueError("capture contains no samples at the requested stride")

    missing = sorted(set(args.fields) - set().union(*(row.keys() for row in rows)))
    if missing:
        raise ValueError(f"capture is missing requested fields: {missing}")

    first_assertion = {}
    transitions = []
    previous = None
    for cycle, row in enumerate(samples):
        state = {field: row[field] for field in args.fields}
        for field, value in state.items():
            if value and field not in first_assertion:
                first_assertion[field] = {
                    "cycle": cycle,
                    "edge": row["edge"],
                    "value": value,
                }
        if previous is None:
            changed = {
                field: {"from": None, "to": value}
                for field, value in state.items()
            }
        else:
            changed = {
                field: {"from": previous[field], "to": value}
                for field, value in state.items()
                if previous[field] != value
            }
        if changed:
            transitions.append({
                "cycle": cycle,
                "edge": row["edge"],
                "changed": changed,
                "state": state,
            })
        previous = state

    report = {
        "schema": "venus-stu-rtl-cycle-summary/v1",
        "trace": str(args.trace.resolve()),
        "raw_samples": len(rows),
        "tile_samples": len(samples),
        "sample_start_edge": anchor,
        "edge_stride": args.edge_stride,
        "first_raw_edge": rows[0]["edge"],
        "last_raw_edge": rows[-1]["edge"],
        "first_assertion": first_assertion,
        "transitions": transitions,
    }
    rendered = json.dumps(report, indent=2, sort_keys=True)
    if args.output:
        args.output.write_text(rendered + "\n")
    print(rendered)


if __name__ == "__main__":
    main()
