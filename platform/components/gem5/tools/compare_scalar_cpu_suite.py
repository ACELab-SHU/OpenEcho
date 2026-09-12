#!/usr/bin/env python3
"""Compare a complete scalar600 gem5 suite with per-case RTL oracles."""

import argparse
import json
import re
import subprocess
import sys
from pathlib import Path


RETIRE_MATCH = re.compile(r"FIRST-DIVERGENCE: MATCH (\d+)")
WRITEBACK_MATCH = re.compile(r"WRITEBACK: MATCH (\d+)")


def run_comparison(command, pattern):
    completed = subprocess.run(
        command, capture_output=True, text=True, check=False,
    )
    output = completed.stdout + completed.stderr
    match = pattern.search(output)
    return completed.returncode, int(match.group(1)) if match else 0, output


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--suite", type=Path, required=True)
    parser.add_argument("--gem5-output", type=Path, required=True)
    parser.add_argument("--oracle-manifest", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--gem5-ticks-per-cycle", type=int, default=2000)
    parser.add_argument("--cycle-tolerance", type=int, default=0)
    parser.add_argument("--allow-gem5-tail", type=int, default=1)
    args = parser.parse_args()

    suite = json.loads(args.suite.read_text(encoding="utf-8"))
    manifest = json.loads(
        args.oracle_manifest.read_text(encoding="utf-8")
    )
    default_oracle = manifest["default_oracle"]
    overrides = manifest.get("overrides", {})
    rtl_task = int(manifest.get("rtl_task", 0))
    gem5_thread = int(manifest.get("gem5_thread", 0))
    tool_root = Path(__file__).resolve().parent

    results = []
    for entry in suite["cases"]:
        name = entry["name"]
        oracle = Path(overrides.get(name, default_oracle.format(case=name)))
        gem5_log = args.gem5_output / name / "scalar_trace.log"

        retire_command = [
            sys.executable, str(tool_root / "compare_scalar_retire.py"),
            "--rtl-jsonl", str(oracle),
            "--rtl-task", str(rtl_task),
            "--gem5-log", str(gem5_log),
            "--gem5-thread", str(gem5_thread),
            "--gem5-ticks-per-cycle", str(args.gem5_ticks_per_cycle),
            "--first-divergence",
            "--cycle-tolerance", str(args.cycle_tolerance),
            "--allow-gem5-tail", str(args.allow_gem5_tail),
        ]
        writeback_command = [
            sys.executable, str(tool_root / "compare_scalar_writeback.py"),
            "--rtl-jsonl", str(oracle),
            "--rtl-task", str(rtl_task),
            "--gem5-log", str(gem5_log),
            "--gem5-thread", str(gem5_thread),
            "--allow-gem5-tail", str(args.allow_gem5_tail),
        ]
        retire_rc, retire_count, retire_output = run_comparison(
            retire_command, RETIRE_MATCH
        )
        writeback_rc, writeback_count, writeback_output = run_comparison(
            writeback_command, WRITEBACK_MATCH
        )
        results.append({
            "name": name,
            "rtl_oracle": str(oracle),
            "gem5_log": str(gem5_log),
            "retire_exact": retire_count,
            "writeback_exact": writeback_count,
            "passed": retire_rc == 0 and writeback_rc == 0,
            "retire_output": retire_output.strip(),
            "writeback_output": writeback_output.strip(),
        })

    report = {
        "schema": "venus-scalar600-cpu-oracle-suite-results/v1",
        "suite": str(args.suite.resolve()),
        "gem5_output": str(args.gem5_output.resolve()),
        "oracle_manifest": str(args.oracle_manifest.resolve()),
        "cycle_tolerance": args.cycle_tolerance,
        "allow_gem5_tail": args.allow_gem5_tail,
        "cases": len(results),
        "passed": sum(item["passed"] for item in results),
        "retire_exact": sum(item["retire_exact"] for item in results),
        "writeback_exact": sum(item["writeback_exact"] for item in results),
        "failed": [item["name"] for item in results if not item["passed"]],
        "results": results,
    }
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(
        json.dumps(report, indent=2) + "\n", encoding="utf-8"
    )
    print(args.output)
    raise SystemExit(0 if not report["failed"] else 1)


if __name__ == "__main__":
    main()
