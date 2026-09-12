#!/usr/bin/env python3
"""Compare a symbolic Venus L1 transfer plan with RTL DMA issue evidence.

This checker covers the transaction-routing boundary only: DMA command issue
order, task/fid/retid, address space selection, destination tile, and
fixed/dynamically resolved length.  It deliberately does *not* accept RTL
issue timestamps as a GEM5 latency schedule.  DMA completion must be modeled
by a real event/response path separately.
"""

import argparse
import json
import sys
from pathlib import Path

from venus_l1_dag import parse_rtl_dma_transactions


SHARED_L2_BASE = 0x80000000
TILE_BASE = 0x82000000
TILE_STRIDE = 0x00200000
PTR_GLOBAL_BASE = 0x81FF8540
PTR_TEMP_BASE = 0x81FF8580


def hex_addr(value):
    return f"0x{value:08x}"


class Comparison:
    def __init__(self):
        self.errors = []
        self.checks = 0

    def require(self, condition, message):
        self.checks += 1
        if not condition and not self.errors:
            self.errors.append(message)


def plan_by_task_and_ordinal(fire_plan):
    result = {}
    for item in fire_plan:
        key = (item["task"], item["issue_ordinal"])
        if key in result:
            raise ValueError(f"duplicate fire plan entry {key}")
        result[key] = item
    return result


def return_slots(return_plan):
    result = set()
    for item in return_plan:
        key = (item["task"], item["retid"])
        if key in result:
            raise ValueError(f"duplicate return plan entry {key}")
        result.add(key)
    return result


def expected_source(plan, completed_returns):
    source = plan["source"]
    space = source["space"]
    if space == "shared_l2":
        return SHARED_L2_BASE + source["offset"], None
    if space in ("ptr_global", "ptr_dfe"):
        return PTR_GLOBAL_BASE, None
    if space == "ptr_temp":
        return PTR_TEMP_BASE, None
    if space == "dmt_return":
        producer = source["producer"]
        key = (producer["task"], producer["retid"])
        return_record = completed_returns.get(key)
        if return_record is None:
            return None, (
                f"requires completed DMT return {key}, but no earlier return "
                f"DMA was observed")
        return return_record["destination"], None
    raise ValueError(f"unsupported source space {space!r}")


def expected_length(plan, completed_returns):
    length = plan["length"]
    if length["mode"] == "fixed":
        return length["bytes"], None
    if length["mode"] == "dmt_return":
        producer = length["producer"]
        key = (producer["task"], producer["retid"])
        return_record = completed_returns.get(key)
        if return_record is None:
            return None, (
                f"requires dynamic DMT length from {key}, but no earlier return "
                f"DMA was observed")
        return return_record["length"], None
    raise ValueError(f"unsupported input length mode {length['mode']!r}")


def compare(manifest, observed):
    if manifest.get("version", 0) < 5:
        raise ValueError("manifest must be version 5 or newer")
    if manifest.get("address_profile") != "venus_l2_tile_v1":
        raise ValueError("unsupported or absent Venus address profile")

    plan = plan_by_task_and_ordinal(manifest["fire_plan"])
    slots = return_slots(manifest["return_plan"])
    comparison = Comparison()
    observed_fire = [entry for entry in observed if entry["direction"] == "fire"]
    observed_return = [entry for entry in observed if entry["direction"] == "return"]
    comparison.require(
        len(observed_fire) == len(plan),
        f"fire count mismatch: plan={len(plan)}, rtl={len(observed_fire)}")
    comparison.require(
        len(observed_return) == len(slots),
        f"return count mismatch: plan={len(slots)}, rtl={len(observed_return)}")

    completed_returns = {}
    seen_plan = set()
    seen_slots = set()
    for sequence, transaction in enumerate(observed):
        direction = transaction["direction"]
        task = transaction["task"]
        tile = transaction["tile"]
        if direction == "return":
            key = (task, transaction["retid"])
            comparison.require(
                key in slots,
                f"event {sequence}: unexpected RTL return slot {key}")
            comparison.require(
                key not in seen_slots,
                f"event {sequence}: duplicate RTL return slot {key}")
            comparison.require(
                TILE_BASE + tile * TILE_STRIDE <= transaction["source"] <
                TILE_BASE + (tile + 1) * TILE_STRIDE,
                f"event {sequence}: return {key} source "
                f"{hex_addr(transaction['source'])} is outside tile {tile}")
            comparison.require(
                transaction["destination"] >= SHARED_L2_BASE,
                f"event {sequence}: return {key} destination "
                f"{hex_addr(transaction['destination'])} is not shared L2")
            comparison.require(
                transaction["length"] > 0,
                f"event {sequence}: return {key} has zero length")
            seen_slots.add(key)
            completed_returns[key] = transaction
            continue

        if direction != "fire":
            comparison.require(False, f"event {sequence}: unknown direction {direction!r}")
            continue
        key = (task, transaction["fid"])
        plan_entry = plan.get(key)
        comparison.require(
            plan_entry is not None,
            f"event {sequence}: RTL fire task={task} fid={transaction['fid']} "
            "has no matching logical issue ordinal")
        if plan_entry is None:
            continue
        comparison.require(
            key not in seen_plan,
            f"event {sequence}: duplicate RTL fire task={task} fid={transaction['fid']}")
        seen_plan.add(key)

        destination = (TILE_BASE + tile * TILE_STRIDE +
                       plan_entry["destination"]["offset"])
        comparison.require(
            transaction["destination"] == destination,
            f"event {sequence}: task={task} fid={transaction['fid']} "
            f"destination RTL={hex_addr(transaction['destination'])}, "
            f"plan={hex_addr(destination)}")
        source, source_error = expected_source(plan_entry, completed_returns)
        comparison.require(
            source_error is None,
            f"event {sequence}: task={task} fid={transaction['fid']} {source_error}")
        if source_error is None:
            comparison.require(
                transaction["source"] == source,
                f"event {sequence}: task={task} fid={transaction['fid']} "
                f"source RTL={hex_addr(transaction['source'])}, "
                f"plan={hex_addr(source)}")
        length, length_error = expected_length(plan_entry, completed_returns)
        comparison.require(
            length_error is None,
            f"event {sequence}: task={task} fid={transaction['fid']} {length_error}")
        if length_error is None:
            comparison.require(
                transaction["length"] == length,
                f"event {sequence}: task={task} fid={transaction['fid']} "
                f"length RTL={transaction['length']}, plan={length}")

    missing_plan = sorted(set(plan) - seen_plan)
    missing_slots = sorted(slots - seen_slots)
    if missing_plan:
        comparison.require(
            False,
            f"RTL is missing planned fire transaction {missing_plan[0]}")
    if missing_slots:
        comparison.require(
            False,
            f"RTL is missing planned return slot {missing_slots[0]}")
    return comparison, len(observed_fire), len(observed_return)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--manifest", required=True, type=Path)
    parser.add_argument(
        "--rtl-dma-log", type=Path,
        help="Override manifest rtl_observed_dma with a fresh RTL dump")
    parser.add_argument("--report", type=Path)
    args = parser.parse_args()

    manifest = json.loads(args.manifest.read_text(encoding="utf-8"))
    observed = (parse_rtl_dma_transactions(args.rtl_dma_log)
                if args.rtl_dma_log else manifest.get("rtl_observed_dma"))
    if not observed:
        raise ValueError("no RTL DMA evidence: pass --rtl-dma-log or materialize with one")
    comparison, fire_count, return_count = compare(manifest, observed)
    lines = [
        "L1 transaction routing: " + ("PASS" if not comparison.errors else "MISMATCH"),
        f"- RTL fire={fire_count}, return={return_count}",
        f"- logical fire={len(manifest['fire_plan'])}, "
        f"return slots={len(manifest['return_plan'])}",
        f"- checks={comparison.checks}",
    ]
    if comparison.errors:
        lines.append("- first divergence: " + comparison.errors[0])
    else:
        lines.append(
            "- verified issue order/routing only; DMA completion timing is "
            "intentionally outside this checker")
    report = "\n".join(lines) + "\n"
    if args.report:
        args.report.write_text(report, encoding="utf-8")
    sys.stdout.write(report)
    raise SystemExit(1 if comparison.errors else 0)


if __name__ == "__main__":
    main()
