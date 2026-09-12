#!/usr/bin/env python3
"""Normalize LaneVFU CAU/SerDiv edge messages into JSONL.

The input is a plain gem5 debug trace.  Use ``gzip -cd`` first for a
compressed trace.  Structural command, operand, result, response, and
retirement edges are emitted so requester/result overlap can be compared
without inferring it from a final task duration.
"""

from __future__ import annotations

import argparse
import json
import re
import sys
from pathlib import Path


PREFIX = re.compile(
    r"^(?P<tick>\d+): system\.cpu\.VenusLane_(?P<lane>\d+): (?P<body>.*)$"
)
ADMIT = re.compile(
    r"VFU_(?P<vfu>CAU|SerDiv) Unit, has received a new instr\s*:\s*"
    r"instr (?P<instr>\d+) with runningID (?P<rid>\d+)"
)
QUEUE = re.compile(
    r"(?P<vfu>CAU|SerDiv) result-queue (?P<action>enqueue|dequeue) "
    r"instr (?P<instr>\d+)/rid (?P<rid>\d+) occupancy "
    r"(?P<occupancy>\d+)/(?P<capacity>\d+)"
)
GRANT = re.compile(
    r"VFU VRF grant passage (?P<passage>-?\d+) instr "
    r"(?P<instr>\d+)/rid (?P<rid>\d+) offset (?P<offset>\d+) "
    r"size (?P<size>\d+) accepted (?P<accepted>[01])"
    r"(?: reason (?P<reason>granted|master_busy|bank_backpressure))?"
)
RETIRE = re.compile(
    r"VFU retirement vfu (?P<vfu_id>\d+) instr "
    r"(?P<instr>\d+)/rid (?P<rid>\d+)"
)
OPERAND_ADMIT = re.compile(
    r"operand requester admit type (?P<type>-?\d+) instr "
    r"(?P<instr>\d+)/rid (?P<rid>\d+)"
)
OPERAND_REQUEST = re.compile(
    r"operand VRF request type (?P<type>-?\d+) instr "
    r"(?P<instr>\d+)/rid (?P<rid>\d+) offset (?P<offset>\d+) "
    r"size (?P<size>\d+) accepted (?P<accepted>[01])"
)
OPERAND_RESPONSE = re.compile(
    r"operand VRF response type (?P<type>-?\d+) instr "
    r"(?P<instr>\d+)/rid (?P<rid>\d+) offset (?P<offset>\d+) "
    r"pipelined (?P<pipelined>[01])"
)
OPERAND_RESPONSE_CONSUME = re.compile(
    r"operand response consume type (?P<type>-?\d+) instr "
    r"(?P<instr>\d+)/rid (?P<rid>\d+) offset (?P<offset>\d+)"
)
OPERAND_COMPLETE = re.compile(
    r"operand requester complete type (?P<type>-?\d+) instr "
    r"(?P<instr>\d+)/rid (?P<rid>\d+) rows (?P<rows>\d+)"
)
OPERAND_INSTR_ENQUEUE = re.compile(
    r"operand instruction enqueue type (?P<type>-?\d+) instr "
    r"(?P<instr>\d+)/rid (?P<rid>\d+) depth "
    r"(?P<occupancy>\d+)/(?P<capacity>\d+)"
)
WRITE_RESPONSE = re.compile(
    r"VFU VRF write response passage (?P<passage>-?\d+) instr "
    r"(?P<instr>\d+)/rid (?P<rid>\d+)"
)

VFU_FROM_PASSAGE = {
    -3: "CAU",
    -4: "CAU",
    -5: "SerDiv",
}
VFU_FROM_ID = {
    1: "CAU",
    2: "SerDiv",
}
VFU_FROM_OPERAND = {
    0: "BitALU",
    1: "BitALU",
    2: "CAU",
    3: "CAU",
    4: "CAU",
    5: "CAU",
    6: "SerDiv",
    7: "SerDiv",
    8: "Mask",
}


def parse_line(line: str) -> dict[str, object] | None:
    prefix = PREFIX.match(line)
    if not prefix:
        return None
    base: dict[str, object] = {
        "tick": int(prefix["tick"]),
        "lane": int(prefix["lane"]),
    }
    body = prefix["body"]

    match = ADMIT.search(body)
    if match:
        return {**base, **{
            "vfu": match["vfu"],
            "event": "admit",
            "instr": int(match["instr"]),
            "rid": int(match["rid"]),
        }}

    match = QUEUE.search(body)
    if match:
        return {**base, **{
            "vfu": match["vfu"],
            "event": match["action"],
            "instr": int(match["instr"]),
            "rid": int(match["rid"]),
            "occupancy": int(match["occupancy"]),
            "capacity": int(match["capacity"]),
        }}

    match = GRANT.search(body)
    if match:
        passage = int(match["passage"])
        accepted = bool(int(match["accepted"]))
        reason = match["reason"]
        if reason is None:
            reason = "granted" if accepted else "bank_backpressure"
        return {**base, **{
            "vfu": VFU_FROM_PASSAGE.get(passage, "unknown"),
            "event": "vrf_grant",
            "instr": int(match["instr"]),
            "rid": int(match["rid"]),
            "passage": passage,
            "offset": int(match["offset"]),
            "size": int(match["size"]),
            "accepted": accepted,
            "reason": reason,
        }}

    match = OPERAND_ADMIT.search(body)
    if match:
        operand_type = int(match["type"])
        return {**base, **{
            "vfu": VFU_FROM_OPERAND.get(operand_type, "unknown"),
            "event": "operand_admit",
            "operand_type": operand_type,
            "instr": int(match["instr"]),
            "rid": int(match["rid"]),
        }}

    match = OPERAND_REQUEST.search(body)
    if match:
        operand_type = int(match["type"])
        return {**base, **{
            "vfu": VFU_FROM_OPERAND.get(operand_type, "unknown"),
            "event": "operand_request",
            "operand_type": operand_type,
            "instr": int(match["instr"]),
            "rid": int(match["rid"]),
            "offset": int(match["offset"]),
            "size": int(match["size"]),
            "accepted": bool(int(match["accepted"])),
        }}

    match = OPERAND_RESPONSE.search(body)
    if match:
        operand_type = int(match["type"])
        return {**base, **{
            "vfu": VFU_FROM_OPERAND.get(operand_type, "unknown"),
            "event": "operand_response",
            "operand_type": operand_type,
            "instr": int(match["instr"]),
            "rid": int(match["rid"]),
            "offset": int(match["offset"]),
            "pipelined": bool(int(match["pipelined"])),
        }}

    match = OPERAND_RESPONSE_CONSUME.search(body)
    if match:
        operand_type = int(match["type"])
        return {**base, **{
            "vfu": VFU_FROM_OPERAND.get(operand_type, "unknown"),
            "event": "operand_response_consume",
            "operand_type": operand_type,
            "instr": int(match["instr"]),
            "rid": int(match["rid"]),
            "offset": int(match["offset"]),
        }}

    match = OPERAND_COMPLETE.search(body)
    if match:
        operand_type = int(match["type"])
        return {**base, **{
            "vfu": VFU_FROM_OPERAND.get(operand_type, "unknown"),
            "event": "operand_complete",
            "operand_type": operand_type,
            "instr": int(match["instr"]),
            "rid": int(match["rid"]),
            "rows": int(match["rows"]),
        }}

    match = OPERAND_INSTR_ENQUEUE.search(body)
    if match:
        operand_type = int(match["type"])
        return {**base, **{
            "vfu": VFU_FROM_OPERAND.get(operand_type, "unknown"),
            "event": "operand_instruction_enqueue",
            "operand_type": operand_type,
            "instr": int(match["instr"]),
            "rid": int(match["rid"]),
            "occupancy": int(match["occupancy"]),
            "capacity": int(match["capacity"]),
        }}

    match = WRITE_RESPONSE.search(body)
    if match:
        passage = int(match["passage"])
        return {**base, **{
            "vfu": VFU_FROM_PASSAGE.get(passage, "unknown"),
            "event": "vrf_write_response",
            "passage": passage,
            "instr": int(match["instr"]),
            "rid": int(match["rid"]),
        }}

    match = RETIRE.search(body)
    if match:
        vfu_id = int(match["vfu_id"])
        vfu = VFU_FROM_ID.get(vfu_id)
        if vfu is None:
            return None
        return {**base, **{
            "vfu": vfu,
            "event": "retire",
            "instr": int(match["instr"]),
            "rid": int(match["rid"]),
        }}

    return None


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("trace", type=Path, nargs="?", help="plain trace file")
    parser.add_argument("--lane", type=int, default=0)
    parser.add_argument("--vfu", choices=("CAU", "SerDiv"))
    parser.add_argument("--instr-min", type=int)
    parser.add_argument("--instr-max", type=int)
    parser.add_argument(
        "--summary",
        action="store_true",
        help="emit one first/last/count record per tagged instruction",
    )
    args = parser.parse_args()

    stream = args.trace.open(encoding="utf-8", errors="replace") if args.trace else sys.stdin
    summaries = {}
    try:
        for line in stream:
            event = parse_line(line)
            if event is None or event["lane"] != args.lane:
                continue
            if args.vfu is not None and event["vfu"] != args.vfu:
                continue
            if args.instr_min is not None and event["instr"] < args.instr_min:
                continue
            if args.instr_max is not None and event["instr"] > args.instr_max:
                continue
            if args.summary:
                key = (event["vfu"], event["instr"], event["rid"])
                summary = summaries.setdefault(
                    key,
                    {
                        "vfu": event["vfu"],
                        "instr": event["instr"],
                        "rid": event["rid"],
                        "lane": event["lane"],
                        "max_occupancy": 0,
                        "accepted_grants": 0,
                        "rejected_grants": 0,
                        "master_busy_grants": 0,
                        "bank_rejected_grants": 0,
                    },
                )
                name = event["event"]
                summary.setdefault("first_" + name + "_tick", event["tick"])
                summary["last_" + name + "_tick"] = event["tick"]
                summary[name + "_count"] = summary.get(name + "_count", 0) + 1
                if "occupancy" in event:
                    summary["max_occupancy"] = max(
                        summary["max_occupancy"], event["occupancy"]
                    )
                if name == "vrf_grant":
                    grant_key = (
                        "accepted_grants"
                        if event["accepted"]
                        else "rejected_grants"
                    )
                    summary[grant_key] += 1
                    if event["reason"] == "master_busy":
                        summary["master_busy_grants"] += 1
                    elif not event["accepted"]:
                        summary["bank_rejected_grants"] += 1
                continue
            try:
                print(json.dumps(event, sort_keys=True))
            except BrokenPipeError:
                return 0
    finally:
        if args.trace:
            stream.close()
    if args.summary:
        for key in sorted(summaries, key=lambda item: item[1]):
            try:
                print(json.dumps(summaries[key], sort_keys=True))
            except BrokenPipeError:
                return 0
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
