#!/usr/bin/env python3
"""Run a materialized Venus LSU suite against one immutable gem5 binary."""

import argparse
import concurrent.futures
import hashlib
import json
import os
import subprocess
from pathlib import Path


def sha256(path):
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def run_case(case_entry, output_root, gem5, config, venus_config):
    case_contract_path = Path(case_entry["case"])
    contract = json.loads(case_contract_path.read_text(encoding="utf-8"))
    name = case_entry["name"]
    case_output = output_root / name
    case_output.mkdir()

    trace_path = case_output / "lsu_events.jsonl"
    environment = os.environ.copy()
    environment.update({
        "VENUS_GEM5_ENABLE_OPERAND_HAZARDS": "1",
        "VENUS_GEM5_LSU_TRACE": str(trace_path),
        "VENUS_GEM5_DEBUG_DIR": str(case_output / "debug"),
        "VENUS_GEM5_TASK_EBREAK_EXIT": "1",
    })
    command = [
        str(gem5),
        "--listener-mode=off",
        f"--outdir={case_output}",
        "--redirect-stdout",
        "--redirect-stderr",
        str(config),
        f"--binary={contract['gem5_elf']}",
        f"--venus-config={venus_config}",
    ]
    completed = subprocess.run(
        command, env=environment, capture_output=True, text=True,
        check=False,
    )
    (case_output / "launcher.stdout").write_text(
        completed.stdout, encoding="utf-8"
    )
    (case_output / "launcher.stderr").write_text(
        completed.stderr, encoding="utf-8"
    )
    result = {
        "name": name,
        "returncode": completed.returncode,
        "case_contract": str(case_contract_path),
        "lsu_trace": str(trace_path),
        "simout": str(case_output / "simout.txt"),
        "simerr": str(case_output / "simerr.txt"),
    }
    (case_output / "result.json").write_text(
        json.dumps(result, indent=2) + "\n", encoding="utf-8"
    )
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--suite", type=Path, required=True)
    parser.add_argument("--output-dir", type=Path, required=True)
    parser.add_argument(
        "--gem5", type=Path, default=Path("build/RISCV/gem5.debug")
    )
    parser.add_argument(
        "--config", type=Path,
        default=Path("configs/tutorial/part1/packet_gen.py"),
    )
    parser.add_argument("--venus-config", default="venus-rtl-16x128")
    parser.add_argument("--jobs", type=int, default=4)
    args = parser.parse_args()

    suite = json.loads(args.suite.resolve().read_text(encoding="utf-8"))
    output_root = args.output_dir.resolve()
    if output_root.exists() and any(output_root.iterdir()):
        parser.error(f"immutable output directory is not empty: {output_root}")
    output_root.mkdir(parents=True, exist_ok=True)

    gem5 = args.gem5.resolve()
    config = args.config.resolve()
    with concurrent.futures.ThreadPoolExecutor(
            max_workers=args.jobs) as executor:
        futures = [
            executor.submit(
                run_case, case_entry, output_root, gem5, config,
                args.venus_config,
            )
            for case_entry in suite["cases"]
        ]
        results = [future.result() for future in futures]

    report = {
        "schema": "venus-lsu-suite-results/v1",
        "suite": str(args.suite.resolve()),
        "gem5": str(gem5),
        "gem5_sha256": sha256(gem5),
        "cases": len(results),
        "passed": sum(result["returncode"] == 0 for result in results),
        "failed": [
            result for result in results if result["returncode"] != 0
        ],
        "results": results,
    }
    report_path = output_root / "suite_results.json"
    report_path.write_text(
        json.dumps(report, indent=2) + "\n", encoding="utf-8"
    )
    print(report_path)
    raise SystemExit(0 if not report["failed"] else 1)


if __name__ == "__main__":
    main()
