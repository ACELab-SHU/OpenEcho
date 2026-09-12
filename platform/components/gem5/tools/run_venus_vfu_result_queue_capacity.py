#!/usr/bin/env python3
"""Sweep VRF grant gaps and verify all fixed-depth VFU result queues."""

import argparse
import concurrent.futures
import hashlib
import json
import os
import re
import subprocess
from pathlib import Path


UNITS = ("BitALU", "CAU", "SerDiv")
EXIT_TICK = re.compile(r"Exiting @ tick ([0-9]+)")


def sha256(path):
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def dump_hashes(root):
    return {
        str(path.relative_to(root)): sha256(path)
        for path in sorted(root.rglob("*.txt"))
    }


def count_events(trace, unit):
    text = trace.read_text(encoding="utf-8")
    enqueue_patterns = [rf"{unit} result-queue enqueue"]
    full_patterns = [rf"{unit} result-queue full.*occupancy 2/2"]
    if unit == "CAU":
        # The explicit CAU D/Q model names admission at the pipeline-D
        # boundary and reports backpressure across the combined elastic
        # pipeline/result-queue capacity.  These are the same structural
        # enqueue/full events that the older result-queue-only trace named.
        enqueue_patterns.append(r"CAU explicit pipeline D enqueue")
        full_patterns.append(
            r"CAU elastic pipeline/result queue full.*occupancy 2/2"
        )
    return {
        "enqueue": sum(
            len(re.findall(pattern, text)) for pattern in enqueue_patterns
        ),
        "dequeue": text.count(f"{unit} result-queue dequeue"),
        "occupancy_2": sum(len(re.findall(
            rf"{pattern}.*occupancy 2/2", text
        )) for pattern in enqueue_patterns),
        "full": sum(
            len(re.findall(pattern, text)) for pattern in full_patterns
        ),
    }


def run_gap(gap, output_root, gem5, config, elf, venus_config):
    case_dir = output_root / f"grant_gap_{gap}"
    out_dir = case_dir / "m5out"
    debug_dir = case_dir / "debug"
    out_dir.mkdir(parents=True)
    debug_dir.mkdir()
    environment = os.environ.copy()
    environment.update({
        "VENUS_GEM5_ENABLE_OPERAND_HAZARDS": "1",
        "VENUS_GEM5_DEBUG_DIR": str(debug_dir),
        "VENUS_GEM5_VRF_GRANT_GAP": gap,
        "VENUS_GEM5_TASK_EBREAK_EXIT": "1",
    })
    command = [
        str(gem5),
        f"--outdir={out_dir}",
        "--debug-flags=LaneVFU",
        "--debug-file=lane_vfu.trace",
        str(config),
        f"--binary={elf}",
        f"--venus-config={venus_config}",
    ]
    completed = subprocess.run(
        command,
        env=environment,
        capture_output=True,
        text=True,
        check=False,
    )
    (case_dir / "launcher.stdout").write_text(
        completed.stdout, encoding="utf-8"
    )
    (case_dir / "launcher.stderr").write_text(
        completed.stderr, encoding="utf-8"
    )
    trace = out_dir / "lane_vfu.trace"
    tick_match = EXIT_TICK.search(completed.stdout + completed.stderr)
    result = {
        "gap": gap,
        "returncode": completed.returncode,
        "exit_tick": int(tick_match.group(1)) if tick_match else None,
        "trace": str(trace),
        "queues": (
            {unit: count_events(trace, unit) for unit in UNITS}
            if completed.returncode == 0 and trace.exists()
            else {}
        ),
        "dump_hashes": dump_hashes(
            debug_dir / "venusgem5_vins_result"
        ),
    }
    (case_dir / "result.json").write_text(
        json.dumps(result, indent=2) + "\n", encoding="utf-8"
    )
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--case", type=Path, required=True)
    parser.add_argument("--output-dir", type=Path, required=True)
    parser.add_argument(
        "--gem5", type=Path, default=Path("build/RISCV/gem5.debug")
    )
    parser.add_argument(
        "--config", type=Path,
        default=Path("configs/tutorial/part1/packet_gen.py"),
    )
    parser.add_argument("--venus-config", default="venus-rtl-16x128")
    parser.add_argument(
        "--gaps", nargs="+",
        default=("1ns", "4ns", "16ns", "32ns", "64ns"),
    )
    parser.add_argument("--jobs", type=int, default=4)
    args = parser.parse_args()

    output_root = args.output_dir.resolve()
    if output_root.exists() and any(output_root.iterdir()):
        parser.error(f"immutable output directory is not empty: {output_root}")
    output_root.mkdir(parents=True, exist_ok=True)
    contract = json.loads(args.case.resolve().read_text(encoding="utf-8"))
    elf = Path(contract["gem5_elf"]).resolve()
    gem5 = args.gem5.resolve()
    config = args.config.resolve()
    gaps = list(dict.fromkeys(args.gaps))
    if "1ns" not in gaps:
        parser.error("--gaps must include the unchanged 1ns control")

    with concurrent.futures.ThreadPoolExecutor(
            max_workers=args.jobs) as executor:
        futures = [
            executor.submit(
                run_gap, gap, output_root, gem5, config, elf,
                args.venus_config,
            )
            for gap in gaps
        ]
        results = [future.result() for future in futures]

    by_gap = {result["gap"]: result for result in results}
    baseline_hashes = by_gap["1ns"]["dump_hashes"]
    functional_exact = {
        gap: result["dump_hashes"] == baseline_hashes
        for gap, result in by_gap.items()
    }
    queue_verdicts = {}
    for unit in UNITS:
        stressed = [
            result["gap"] for result in results
            if result["queues"].get(unit, {}).get("occupancy_2", 0) > 0
            and result["queues"].get(unit, {}).get("full", 0) > 0
        ]
        queue_verdicts[unit] = {
            "pass": bool(stressed),
            "full_gaps": stressed,
        }

    passed = (
        all(result["returncode"] == 0 for result in results)
        and all(functional_exact.values())
        and all(item["pass"] for item in queue_verdicts.values())
    )
    report = {
        "schema": "venus-vfu-result-queue-capacity/v1",
        "case": str(args.case.resolve()),
        "gem5": str(gem5),
        "gem5_sha256": sha256(gem5),
        "control_gap": "1ns",
        "functional_exact_to_control": functional_exact,
        "queue_verdicts": queue_verdicts,
        "passed": passed,
        "results": results,
    }
    report_path = output_root / "capacity_results.json"
    report_path.write_text(
        json.dumps(report, indent=2) + "\n", encoding="utf-8"
    )
    print(report_path)
    raise SystemExit(0 if passed else 1)


if __name__ == "__main__":
    main()
