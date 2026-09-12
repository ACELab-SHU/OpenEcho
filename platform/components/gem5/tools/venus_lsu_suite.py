#!/usr/bin/env python3
"""Materialize immutable RAW, throughput and capacity LSU test suites."""

import argparse
import json
import subprocess
import sys
from pathlib import Path


def case_name(kind, operation, ew, vl, streams, dependency):
    suffix = f"{kind}_{operation}_ew{ew}_vl{vl}_n{streams}"
    return f"{suffix}_{dependency}" if dependency else suffix


def matrix(profile):
    if profile == "smoke":
        raw_vls = (16, 256)
        throughput_vls = (16, 256)
        stream_counts = (1, 4, 5, 6)
        capacity_counts = (2, 3, 4, 5, 6)
    else:
        raw_vls = (1, 16, 63, 64, 65, 127, 128, 129, 256, 2048)
        throughput_vls = (16, 64, 128, 256, 2048)
        stream_counts = (1, 2, 3, 4, 5, 6)
        capacity_counts = (1, 2, 3, 4, 5, 6)

    cases = []
    for operation in ("load", "store"):
        for ew in (8, 16):
            for vl in raw_vls:
                cases.append(("raw", operation, ew, vl, 1, "barrier"))
                dependency = "consumer" if operation == "load" else "producer"
                cases.append(("raw", operation, ew, vl, 1, dependency))
            for vl in throughput_vls:
                for streams in stream_counts:
                    cases.append(
                        ("throughput", operation, ew, vl, streams, "")
                    )
            for streams in capacity_counts:
                cases.append(
                    ("capacity", operation, ew, 16, streams, "")
                )
    return cases


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output-dir", type=Path, required=True)
    parser.add_argument("--profile", choices=("smoke", "full"),
                        default="smoke")
    args = parser.parse_args()

    root = args.output_dir.resolve()
    if root.exists() and any(root.iterdir()):
        parser.error(f"immutable suite directory is not empty: {root}")
    root.mkdir(parents=True, exist_ok=True)
    generator = Path(__file__).with_name("venus_lsu_microbench.py")
    manifest = []
    for kind, operation, ew, vl, streams, dependency in matrix(args.profile):
        name = case_name(kind, operation, ew, vl, streams, dependency)
        command = [
            sys.executable,
            str(generator),
            "--output-dir",
            str(root / name),
            "--kind",
            kind,
            "--operation",
            operation,
            "--ew",
            str(ew),
            "--vl",
            str(vl),
            "--streams",
            str(streams),
        ]
        if kind == "capacity":
            command += [
                "--blocker-vl",
                "4096" if ew == 8 else "2048",
            ]
        if dependency == "consumer":
            command.append("--consumer")
        elif dependency == "producer":
            command.append("--producer")
        subprocess.run(command, check=True, capture_output=True, text=True)
        manifest.append({
            "name": name,
            "case": str(root / name / "case.json"),
        })
    (root / "suite.json").write_text(
        json.dumps({
            "schema": "venus-lsu-suite/v1",
            "profile": args.profile,
            "cases": manifest,
        }, indent=2) + "\n",
        encoding="utf-8",
    )
    print(root / "suite.json")


if __name__ == "__main__":
    main()
