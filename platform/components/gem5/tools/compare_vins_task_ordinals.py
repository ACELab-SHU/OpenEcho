#!/usr/bin/env python3
"""Compare VINS dumps by task-local instruction order.

Concurrent DAG runs may allocate different global instruction IDs without
changing a task's instruction stream.  This comparator deliberately ignores
those global IDs while requiring the task names, opcode/suffix sequence, and
all emitted values to match.
"""

import argparse
import json
import re
from pathlib import Path


FILE_RE = re.compile(
    r"^(?P<op>[A-Za-z0-9]+)_(?P<id>[0-9]+)(?P<suffix>.*)\.txt$"
)
TASK_RE = re.compile(r"^task_(?P<id>[0-9]+)$")


def parse_args():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--reference", type=Path, required=True)
    parser.add_argument("--candidate", type=Path, required=True)
    parser.add_argument("--report", type=Path)
    parser.add_argument("--strict-x", action="store_true")
    parser.add_argument(
        "--normalize-shuffle",
        action="store_true",
        help=(
            "Treat RTL GATHER/SCATTER and gem5 VSHUFFLE dump names as the "
            "same VSHUFFLE instruction class"
        ),
    )
    return parser.parse_args()


def task_stream(root):
    streams = {}
    for task_dir in root.iterdir():
        task_match = TASK_RE.match(task_dir.name)
        if not task_dir.is_dir() or task_match is None:
            continue
        entries = []
        for path in task_dir.glob("*.txt"):
            match = FILE_RE.match(path.name)
            if match is None:
                continue
            values = [
                line.strip()
                for line in path.read_text(errors="replace").splitlines()
                if line.strip()
            ]
            entries.append({
                "id": int(match.group("id")),
                "op": match.group("op"),
                "suffix": match.group("suffix"),
                "path": path,
                "values": values,
            })
        streams[int(task_match.group("id"))] = sorted(
            entries, key=lambda entry: (
                entry["id"], entry["op"], entry["suffix"]
            )
        )
    return streams


def value_equal(reference, candidate, strict_x):
    if not strict_x and any(char in reference.lower() for char in ("x", "z")):
        return True
    return reference == candidate


def normalized_op(op, normalize_shuffle):
    if normalize_shuffle and op in ("GATHER", "SCATTER", "VSHUFFLE"):
        return "VSHUFFLE"
    return op


def first_stream_mismatch(
        reference, candidate, strict_x, normalize_shuffle):
    common = min(len(reference), len(candidate))
    for ordinal in range(common):
        ref_entry = reference[ordinal]
        cand_entry = candidate[ordinal]
        ref_key = (
            normalized_op(ref_entry["op"], normalize_shuffle),
            ref_entry["suffix"],
        )
        cand_key = (
            normalized_op(cand_entry["op"], normalize_shuffle),
            cand_entry["suffix"],
        )
        if ref_key != cand_key:
            return {
                "ordinal": ordinal,
                "kind": "opcode",
                "reference": ref_key,
                "candidate": cand_key,
            }
        ref_values = ref_entry["values"]
        cand_values = cand_entry["values"]
        value_common = min(len(ref_values), len(cand_values))
        for element in range(value_common):
            if not value_equal(
                    ref_values[element], cand_values[element], strict_x):
                return {
                    "ordinal": ordinal,
                    "kind": "value",
                    "op": ref_entry["op"] + ref_entry["suffix"],
                    "element": element,
                    "reference": ref_values[element],
                    "candidate": cand_values[element],
                }
        if len(ref_values) != len(cand_values):
            return {
                "ordinal": ordinal,
                "kind": "value_count",
                "op": ref_entry["op"] + ref_entry["suffix"],
                "reference": len(ref_values),
                "candidate": len(cand_values),
            }
    if len(reference) != len(candidate):
        return {
            "ordinal": common,
            "kind": "instruction_count",
            "reference": len(reference),
            "candidate": len(candidate),
        }
    return None


def main():
    args = parse_args()
    reference = task_stream(args.reference.resolve())
    candidate = task_stream(args.candidate.resolve())
    task_ids = sorted(set(reference) | set(candidate))
    mismatches = []
    compared = 0
    for task_id in task_ids:
        ref_stream = reference.get(task_id, [])
        cand_stream = candidate.get(task_id, [])
        mismatch = first_stream_mismatch(
            ref_stream,
            cand_stream,
            args.strict_x,
            args.normalize_shuffle,
        )
        if mismatch is not None:
            mismatches.append({"task": task_id, **mismatch})
        else:
            compared += len(ref_stream)

    report = {
        "schema": "venus-vins-task-ordinal-comparison/v1",
        "reference": str(args.reference.resolve()),
        "candidate": str(args.candidate.resolve()),
        "tasks": len(task_ids),
        "normalize_shuffle": args.normalize_shuffle,
        "instructions_compared": compared,
        "mismatches": mismatches,
        "match": not mismatches,
    }
    rendered = json.dumps(report, indent=2) + "\n"
    if args.report:
        args.report.write_text(rendered, encoding="utf-8")
    print(rendered, end="")
    return 0 if report["match"] else 1


if __name__ == "__main__":
    raise SystemExit(main())
