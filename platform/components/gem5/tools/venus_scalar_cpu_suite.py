#!/usr/bin/env python3
"""Materialize the deterministic scalar600 CPU conformance matrix."""

import argparse
import json
import subprocess
import sys
from pathlib import Path

import venus_scalar_cpu_microbench as microbench


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output-dir", type=Path, required=True)
    parser.add_argument(
        "--cases", nargs="*", choices=tuple(microbench.CASES),
        default=list(microbench.CASES),
    )
    args = parser.parse_args()

    root = args.output_dir.resolve()
    if root.exists() and any(root.iterdir()):
        parser.error(f"immutable suite directory is not empty: {root}")
    root.mkdir(parents=True, exist_ok=True)

    generator = Path(__file__).with_name("venus_scalar_cpu_microbench.py")
    cases = []
    for name in args.cases:
        case_root = root / name
        subprocess.run([
            sys.executable, str(generator),
            "--output-dir", str(case_root),
            "--case", name,
        ], check=True, capture_output=True, text=True)
        cases.append({
            "name": name,
            "case": str(case_root / "case.json"),
        })

    suite = root / "suite.json"
    suite.write_text(json.dumps({
        "schema": "venus-scalar600-cpu-suite/v1",
        "cases": cases,
    }, indent=2) + "\n", encoding="utf-8")
    print(suite)


if __name__ == "__main__":
    main()
