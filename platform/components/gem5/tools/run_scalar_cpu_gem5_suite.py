#!/usr/bin/env python3
"""Run a scalar600 CPU microbench suite against one gem5 binary."""

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


def run_case(entry, output_root, gem5, config, venus_config):
    contract_path = Path(entry["case"])
    contract = json.loads(contract_path.read_text(encoding="utf-8"))
    name = entry["name"]
    output = output_root / name
    output.mkdir()
    trace = output / "scalar_trace.log"
    environment = os.environ.copy()
    environment["VENUS_GEM5_DEBUG_DIR"] = str(output / "debug")
    environment["VENUS_GEM5_TASK_EBREAK_EXIT"] = "1"
    command = [
        str(gem5),
        "--listener-mode=off",
        f"--outdir={output}",
        "--debug-flags=MinorExecute,Exec",
        "--debug-file=scalar_trace.log",
        str(config),
        f"--binary={contract['gem5_elf']}",
        f"--venus-config={venus_config}",
    ]
    wake_tick = contract.get("external_events", {}).get("wfi_wake_tick")
    if wake_tick is not None:
        command.append(f"--wfi-wake-tick={int(wake_tick)}")
    completed = subprocess.run(
        command, env=environment, capture_output=True, text=True,
        check=False,
    )
    (output / "launcher.stdout").write_text(
        completed.stdout, encoding="utf-8"
    )
    (output / "launcher.stderr").write_text(
        completed.stderr, encoding="utf-8"
    )
    return {
        "name": name,
        "returncode": completed.returncode,
        "case_contract": str(contract_path),
        "trace": str(trace),
    }


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

    suite_path = args.suite.resolve()
    suite = json.loads(suite_path.read_text(encoding="utf-8"))
    output_root = args.output_dir.resolve()
    if output_root.exists() and any(output_root.iterdir()):
        parser.error(f"immutable output directory is not empty: {output_root}")
    output_root.mkdir(parents=True, exist_ok=True)
    gem5 = args.gem5.resolve()
    config = args.config.resolve()

    with concurrent.futures.ThreadPoolExecutor(
            max_workers=args.jobs) as executor:
        results = list(executor.map(
            lambda entry: run_case(
                entry, output_root, gem5, config, args.venus_config
            ),
            suite["cases"],
        ))

    report = {
        "schema": "venus-scalar600-cpu-gem5-suite-results/v1",
        "suite": str(suite_path),
        "gem5": str(gem5),
        "gem5_sha256": sha256(gem5),
        "cases": len(results),
        "passed": sum(item["returncode"] == 0 for item in results),
        "failed": [item for item in results if item["returncode"] != 0],
        "results": results,
    }
    result_path = output_root / "suite_results.json"
    result_path.write_text(
        json.dumps(report, indent=2) + "\n", encoding="utf-8"
    )
    print(result_path)
    raise SystemExit(0 if not report["failed"] else 1)


if __name__ == "__main__":
    main()
