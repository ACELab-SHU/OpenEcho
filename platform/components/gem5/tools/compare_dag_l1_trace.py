#!/usr/bin/env python3
"""Compare RTL L1-DMA issue records with Gem5 scheduler-admission trace.

This intentionally checks only the externally visible transaction contract:
issue order, direction, task/tile/port identity, source, destination, and
byte length.  Completion timestamps are checked separately by
``compare_dag_lifecycle.py``.
"""

import argparse
import json
from pathlib import Path

from venus_l1_dag import parse_rtl_dma_transactions


def parse_gem5_trace(path):
    transactions = []
    for lineno, raw in enumerate(path.read_text(encoding="utf-8").splitlines(), 1):
        if not raw.strip():
            continue
        event = json.loads(raw)
        kind = event.get("event")
        if kind not in ("fire_admit", "return_admit"):
            continue
        try:
            transaction = {
                "direction": "fire" if kind == "fire_admit" else "return",
                "task": event["task_id"],
                "tile": event["tile_id"],
                "source": int(event["source"], 0),
                "destination": int(event["destination"], 0),
                "length": event["bytes"],
            }
            if kind == "fire_admit":
                transaction["fid"] = event["ordinal"]
            else:
                transaction["retid"] = event["retid"]
        except (KeyError, TypeError, ValueError) as error:
            raise ValueError(
                f"Gem5 trace line {lineno} is not an L1 admission record"
            ) from error
        transactions.append(transaction)
    return transactions


def format_transaction(transaction):
    port = transaction["fid"] if transaction["direction"] == "fire" \
        else transaction["retid"]
    return (
        f"{transaction['direction']} task={transaction['task']} "
        f"tile={transaction['tile']} port={port} "
        f"src=0x{transaction['source']:08x} "
        f"dst=0x{transaction['destination']:08x} "
        f"bytes={transaction['length']}"
    )


def compare(rtl, gem5):
    if len(rtl) != len(gem5):
        return (
            f"transaction count RTL={len(rtl)} Gem5={len(gem5)}",
            min(len(rtl), len(gem5)),
        )
    for index, (rtl_entry, gem5_entry) in enumerate(zip(rtl, gem5)):
        common_fields = ("direction", "task", "tile", "source",
                         "destination", "length")
        port_field = "fid" if rtl_entry["direction"] == "fire" else "retid"
        fields = common_fields + (port_field,)
        if any(rtl_entry[field] != gem5_entry[field] for field in fields):
            return (
                f"event {index}: RTL {format_transaction(rtl_entry)}; "
                f"Gem5 {format_transaction(gem5_entry)}",
                index,
            )
    return None, len(rtl)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--rtl-dma-log", type=Path, required=True)
    parser.add_argument("--gem5-trace", type=Path, required=True)
    args = parser.parse_args()

    rtl = parse_rtl_dma_transactions(args.rtl_dma_log)
    gem5 = parse_gem5_trace(args.gem5_trace)
    error, checks = compare(rtl, gem5)
    print("L1 admission transaction trace: " + ("PASS" if error is None else "MISMATCH"))
    print(f"- RTL={len(rtl)}, Gem5={len(gem5)}, checked={checks}")
    if error:
        print("- first divergence: " + error)
        return 1
    print("- issue order, IDs, raw bus addresses, and byte lengths match")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
