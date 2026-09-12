#!/usr/bin/env python3
"""Compare ordered RTL/gem5 LSU request, response and completion phases."""

import argparse
import json
from pathlib import Path


def read_jsonl(path):
    records = []
    with path.open(encoding="utf-8") as stream:
        for line_number, line in enumerate(stream, 1):
            try:
                records.append(json.loads(line))
            except json.JSONDecodeError as error:
                raise RuntimeError(
                    f"{path}:{line_number}: invalid JSON: {error}"
                ) from error
    return records


def rtl_transactions(records):
    instructions = []
    for record in records:
        if record.get("event") != "venus_instr":
            continue
        if record.get("op") not in (50, 51):
            continue
        instructions.append({
            "ordinal": len(instructions),
            "kind": "store" if record["op"] == 50 else "load",
            "id": record["id"],
            "vl": record["vl"],
            "vew": record["vew"],
            "accept_cycle": record["fire_cycle"],
            "accept_time": record["fire_time"],
            "complete_cycle": record["recycle_cycle"],
        })

    requests = [
        record for record in records if record.get("event") == "lsu_request"
    ]
    responses = [
        record for record in records
        if record.get("event") == "lsu_response"
    ]
    request_index = {"load": 0, "store": 0}
    response_index = {"load": 0, "store": 0}
    requests_by_kind = {
        kind: [record for record in requests if record["kind"] == kind]
        for kind in ("load", "store")
    }
    responses_by_kind = {
        kind: [record for record in responses if record["kind"] == kind]
        for kind in ("load", "store")
    }

    for transaction in instructions:
        kind = transaction["kind"]
        index = request_index[kind]
        if index >= len(requests_by_kind[kind]):
            transaction["missing"] = "request"
            continue
        request = requests_by_kind[kind][index]
        request_index[kind] += 1
        transaction["beats"] = request["beats"]
        transaction["request_delta"] = (
            request["sim_time"] - transaction["accept_time"]
        ) // 2
        transaction["request_cycle"] = (
            transaction["accept_cycle"] + transaction["request_delta"]
        )

        response_ordinal = response_index[kind]
        if kind == "load":
            response_ordinal += transaction["beats"] - 1
        if response_ordinal >= len(responses_by_kind[kind]):
            transaction["missing"] = "response"
            continue
        response = responses_by_kind[kind][response_ordinal]
        response_index[kind] = response_ordinal + 1
        transaction["response_delta"] = (
            response["sim_time"] - transaction["accept_time"]
        ) // 2
        transaction["response_cycle"] = (
            transaction["accept_cycle"] + transaction["response_delta"]
        )
        transaction["complete_delta"] = (
            transaction["complete_cycle"] - transaction["accept_cycle"]
        )
    return instructions


def gem5_transactions(records):
    transactions = {}
    for record in records:
        event = record.get("event")
        if event not in {
            "lsu_accept", "lsu_request", "lsu_response", "lsu_complete"
        }:
            continue
        ordinal = record["instr"]
        transaction = transactions.setdefault(ordinal, {
            "ordinal": ordinal,
            "kind": "store" if record["op"] == "VSTORE" else "load",
            "id": record["id"],
            "vl": record["vl"],
            "vew": record["vew"],
        })
        if event == "lsu_accept":
            transaction["accept_cycle"] = record["cycle"]
        elif event == "lsu_request":
            transaction["beats"] = record["beats"]
            transaction["request_cycle"] = record["cycle"]
        elif event == "lsu_response":
            transaction["response_cycle"] = record["cycle"]
        else:
            transaction["complete_cycle"] = record["cycle"]

    ordered = [transactions[index] for index in sorted(transactions)]
    for transaction in ordered:
        accept = transaction["accept_cycle"]
        for phase in ("request", "response", "complete"):
            transaction[f"{phase}_delta"] = (
                transaction[f"{phase}_cycle"] - accept
            )
    return ordered


def compare(rtl, gem5, tolerance):
    local_fields = (
        "kind", "vl", "vew", "beats", "request_delta",
        "response_delta", "complete_delta",
    )
    pipeline_fields = (
        "kind", "vl", "vew", "beats", "request_from_first",
        "response_from_first", "complete_from_first",
    )

    for transactions in (rtl, gem5):
        if not transactions:
            continue
        anchor = transactions[0]["accept_cycle"]
        for transaction in transactions:
            transaction["accept_from_first"] = (
                transaction["accept_cycle"] - anchor
            )
            for phase in ("request", "response", "complete"):
                transaction[f"{phase}_from_first"] = (
                    transaction[f"{phase}_cycle"] - anchor
                )

    comparisons = []
    local_first_divergence = None
    pipeline_first_divergence = None
    for ordinal in range(max(len(rtl), len(gem5))):
        if ordinal >= len(rtl) or ordinal >= len(gem5):
            count_difference = {
                "ordinal": ordinal,
                "field": "transaction_count",
                "rtl": len(rtl),
                "gem5": len(gem5),
            }
            local_first_divergence = (
                local_first_divergence or count_difference
            )
            pipeline_first_divergence = (
                pipeline_first_divergence or count_difference
            )
            break
        item = {"ordinal": ordinal, "rtl": rtl[ordinal], "gem5": gem5[ordinal]}
        local_differences = []
        pipeline_differences = []
        for field in local_fields:
            rtl_value = rtl[ordinal].get(field)
            gem5_value = gem5[ordinal].get(field)
            different = rtl_value != gem5_value
            if (field.endswith("_delta") and rtl_value is not None and
                    gem5_value is not None):
                different = abs(rtl_value - gem5_value) > tolerance
            if different:
                local_differences.append({
                    "field": field,
                    "rtl": rtl_value,
                    "gem5": gem5_value,
                })
                if local_first_divergence is None:
                    local_first_divergence = {
                        "ordinal": ordinal,
                        **local_differences[-1],
                    }
        for field in pipeline_fields:
            rtl_value = rtl[ordinal].get(field)
            gem5_value = gem5[ordinal].get(field)
            different = rtl_value != gem5_value
            if (field.endswith("_from_first") and
                    rtl_value is not None and gem5_value is not None):
                different = abs(rtl_value - gem5_value) > tolerance
            if different:
                pipeline_differences.append({
                    "field": field,
                    "rtl": rtl_value,
                    "gem5": gem5_value,
                })
                if pipeline_first_divergence is None:
                    pipeline_first_divergence = {
                        "ordinal": ordinal,
                        **pipeline_differences[-1],
                    }
        item["local_differences"] = local_differences
        item["pipeline_differences"] = pipeline_differences
        comparisons.append(item)
    return {
        "schema": "venus-lsu-event-comparison/v2",
        "tolerance_cycles": tolerance,
        "rtl_transactions": len(rtl),
        "gem5_transactions": len(gem5),
        "first_divergence": pipeline_first_divergence,
        "local_first_divergence": local_first_divergence,
        "pass": pipeline_first_divergence is None,
        "comparisons": comparisons,
    }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--rtl", type=Path, required=True)
    parser.add_argument("--gem5", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--tolerance-cycles", type=int, default=0)
    args = parser.parse_args()

    report = compare(
        rtl_transactions(read_jsonl(args.rtl)),
        gem5_transactions(read_jsonl(args.gem5)),
        args.tolerance_cycles,
    )
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(
        json.dumps(report, indent=2) + "\n", encoding="utf-8"
    )
    print(json.dumps({
        "pass": report["pass"],
        "first_divergence": report["first_divergence"],
        "output": str(args.output),
    }))
    raise SystemExit(0 if report["pass"] else 1)


if __name__ == "__main__":
    main()
