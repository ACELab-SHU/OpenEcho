#!/usr/bin/env python3
"""Summarize tagged Venus VRF-result retries and their winning requesters.

The lane prints a retry more than once while a selected packet is being
forwarded, and it may print a rejection followed by an acceptance in the same
tick.  A physical blocked edge is therefore identified by
``(tick, instruction, running-id, offset)`` after removing identities that
also succeed in that tick.  The bank first observed for a tagged packet is
retained across a downstream retry, matching the Xbar's selected owner.
"""

import argparse
import json
import re
from collections import Counter, defaultdict
from pathlib import Path


ATTEMPT_RE = re.compile(
    r"^\s*(?P<tick>\d+): system\.cpu\.VenusLane_(?P<lane>\d+): "
    r"VFU VRF grant passage -3 instr (?P<instr>\d+)/rid (?P<rid>\d+) "
    r"offset (?P<offset>\d+) size (?P<size>\d+) accepted "
    r"(?P<accepted>[01]) reason (?P<reason>\S+)"
)
PENDING_RE = re.compile(
    r"^\s*(?P<tick>\d+): system\.cpu\.XBAR: Venus VRF request pending "
    r"src (?P<src>\d+) bank (?P<bank>\d+) "
    r"(?:master (?P<master>\d+)|behind LSU priority)"
)
RR_RE = re.compile(
    r"^\s*(?P<tick>\d+): system\.cpu\.XBAR: Venus VRF RR grant bank "
    r"(?P<bank>\d+) old_rr (?P<old>\d+) winner_master (?P<winner>\d+) "
    r"source (?P<src>\d+) next_rr (?P<next>\d+) contenders (?P<count>\d+)"
)
LSU_RE = re.compile(
    r"^\s*(?P<tick>\d+): system\.cpu\.XBAR: Venus VRF bank "
    r"(?P<bank>\d+) blocked by LSU priority"
)

MASTER_NAMES = {
    0: "BitALU_A",
    1: "BitALU_B",
    2: "CAU_A",
    3: "CAU_B",
    8: "BitALU_result",
    9: "CAU_result",
    10: "SerDiv_result",
    11: "Shuffle",
}


def parse_instruction_ids(value):
    if value is None:
        return None
    path = Path(value)
    if path.is_file():
        data = json.loads(path.read_text())
        if isinstance(data, dict):
            data = data.get("instruction_ids", data.get("instructions"))
        return {int(item) for item in data}
    return {int(item) for item in value.split(",") if item.strip()}


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("trace", type=Path)
    parser.add_argument("--lane", type=int, default=0)
    parser.add_argument("--instructions", help="comma list or JSON file")
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()

    wanted = parse_instruction_ids(args.instructions)
    result_source = args.lane * 16 + 10
    lane_bank_lo = args.lane * 4
    lane_bank_hi = lane_bank_lo + 4

    # Events are recorded first and normalized after the whole trace is read,
    # because the grant that invalidates a same-cycle rejection can appear
    # later in the log.
    attempts = defaultdict(lambda: {"rejected": False, "accepted": False})
    tag_bank = {}
    pending_bank = {}
    winners = {}
    lsu_blocks = set()
    raw_rejected = 0

    with args.trace.open(errors="replace") as stream:
        for line in stream:
            match = PENDING_RE.match(line)
            if match:
                tick = int(match.group("tick"))
                src = int(match.group("src"))
                if src == result_source:
                    pending_bank[tick] = int(match.group("bank"))
                continue

            match = RR_RE.match(line)
            if match:
                bank = int(match.group("bank"))
                if lane_bank_lo <= bank < lane_bank_hi:
                    key = (int(match.group("tick")), bank)
                    winners[key] = int(match.group("winner"))
                continue

            match = LSU_RE.match(line)
            if match:
                bank = int(match.group("bank"))
                if lane_bank_lo <= bank < lane_bank_hi:
                    lsu_blocks.add((int(match.group("tick")), bank))
                continue

            match = ATTEMPT_RE.match(line)
            if not match or int(match.group("lane")) != args.lane:
                continue
            instr = int(match.group("instr"))
            if wanted is not None and instr not in wanted:
                continue
            tick = int(match.group("tick"))
            tag = (instr, int(match.group("rid")), int(match.group("offset")))
            identity = (tick,) + tag
            if tick in pending_bank:
                tag_bank[tag] = pending_bank[tick]
            record = attempts[identity]
            record["bank"] = tag_bank.get(tag)
            record["reason"] = match.group("reason")
            if match.group("accepted") == "1":
                record["accepted"] = True
            else:
                record["rejected"] = True
                raw_rejected += 1

    blocked = []
    by_instruction = Counter()
    by_winner = Counter()
    unknown = []
    for identity, record in sorted(attempts.items()):
        if not record["rejected"] or record["accepted"]:
            continue
        tick, instr, rid, offset = identity
        bank = record.get("bank")
        if bank is not None and (tick, bank) in lsu_blocks:
            winner = "LSU"
        elif bank is not None and (tick, bank) in winners:
            winner = MASTER_NAMES.get(
                winners[(tick, bank)], f"master_{winners[(tick, bank)]}"
            )
        else:
            winner = "unknown"
            unknown.append({"tick": tick, "instr": instr, "rid": rid,
                            "offset": offset, "bank": bank})
        event = {"tick": tick, "instr": instr, "rid": rid,
                 "offset": offset, "bank": bank, "winner": winner}
        blocked.append(event)
        by_instruction[instr] += 1
        by_winner[winner] += 1

    result = {
        "schema": "venus-vrf-tagged-arbiter-summary/v1",
        "trace": str(args.trace),
        "lane": args.lane,
        "instructions": sorted(wanted) if wanted is not None else None,
        "raw_rejected_trace_records": raw_rejected,
        "normalized_blocked": len(blocked),
        "blocked_by_winner": dict(sorted(by_winner.items())),
        "per_instruction_blocked": [
            {"instr": instr, "blocked": by_instruction[instr]}
            for instr in sorted(by_instruction)
        ],
        "blocked_events": blocked,
        "unknown_events": unknown,
    }
    payload = json.dumps(result, indent=2, sort_keys=False) + "\n"
    if args.output:
        args.output.write_text(payload)
    else:
        print(payload, end="")


if __name__ == "__main__":
    main()
