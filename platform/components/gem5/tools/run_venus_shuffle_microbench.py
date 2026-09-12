#!/usr/bin/env python3
"""Run one generated Shuffle oracle with the accepted RTL-model switches.

The runner deliberately owns the three structural switches used by the CCH
and SCH timing baseline.  A standalone run with any of them omitted is a
different model and must not be compared with the RTL edge oracle.
"""

import argparse
import hashlib
import json
import os
import subprocess
from pathlib import Path


REQUIRED_ENVIRONMENT = {
    "VENUS_GEM5_EXPERIMENTAL_VRF_RR": "1",
    "VENUS_GEM5_EXPERIMENTAL_REQUESTER_Q_VISIBILITY": "1",
    "VENUS_GEM5_EXPERIMENTAL_ONE_ENTRY_OPERAND_COMMANDS": "1",
}


def sha256(path):
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def normalized_lifecycle(monitor_path, tile_period_ps):
    payload = json.loads(monitor_path.read_text(encoding="utf-8"))
    instructions = payload[0]["Venus_instr"]
    lifecycle = []
    for sequence, instr in enumerate(instructions):
        duration = int(instr["consumed_ticks"])
        if duration % tile_period_ps:
            raise RuntimeError(
                f"sequence {sequence} duration {duration} ps is not tile "
                f"aligned to {tile_period_ps} ps"
            )
        lifecycle.append({
            "sequence": sequence,
            "op": instr["op_s"],
            "running_id": instr["id"],
            "fire_ps": int(instr["fire_tick"]),
            "recycle_ps": int(instr["recycle_tick"]),
            "duration_ps": duration,
            "duration_tile_cycles": duration // tile_period_ps,
        })
    return lifecycle


def rtl_lifecycle(perf_path, task):
    paths = (
        sorted(perf_path.glob("venus_full_dag_perf_cluster*_tile*.jsonl"))
        if perf_path.is_dir() else [perf_path]
    )
    records = []
    for path in paths:
        with path.open(encoding="utf-8") as stream:
            for raw in stream:
                event = json.loads(raw)
                if (event.get("event") == "venus_instr" and
                        event.get("task_id") == task):
                    records.append(event)
    records.sort(key=lambda event: event["task_instr_counter"])
    return [{
        "sequence": sequence,
        "op": event["op_s"],
        "running_id": event["id"],
        "fire_tile_cycle": int(event["fire_tick"]),
        "recycle_tile_cycle": int(event["recycle_tick"]),
        "duration_tile_cycles": int(event["consumed_ticks"]),
    } for sequence, event in enumerate(records)]


def canonical_op(op, shuffle_operation):
    if op == "VSHUFFLE":
        return shuffle_operation.upper()
    return op


def compare_lifecycle(rtl, gem5, tile_period_ps, shuffle_operation):
    if len(rtl) != len(gem5):
        return {
            "pass": False,
            "first_divergence": {
                "kind": "instruction_count",
                "rtl": len(rtl),
                "gem5": len(gem5),
            },
        }
    if not rtl:
        return {
            "pass": False,
            "first_divergence": {"kind": "empty_oracle"},
        }
    rtl_origin = rtl[0]["fire_tile_cycle"]
    gem5_origin = gem5[0]["fire_ps"] // tile_period_ps
    for sequence, (rtl_event, gem5_event) in enumerate(zip(rtl, gem5)):
        rtl_op = canonical_op(rtl_event["op"], shuffle_operation)
        gem5_op = canonical_op(gem5_event["op"], shuffle_operation)
        # The tile RTL logger predates the LSU opcode name table and emits
        # "Unknown OP" for a structurally decoded VLOAD/VSTORE.  Keep timing
        # comparison available without pretending that string is identity
        # evidence; all named opcodes still compare strictly.
        if (rtl_op != "Unknown OP" and gem5_op != "Unknown OP" and
                rtl_op != gem5_op):
            return {
                "pass": False,
                "first_divergence": {
                    "kind": "op",
                    "sequence": sequence,
                    "rtl": rtl_op,
                    "gem5": gem5_op,
                },
            }
        rtl_timing = {
            "fire": rtl_event["fire_tile_cycle"] - rtl_origin,
            "duration": rtl_event["duration_tile_cycles"],
            "recycle": rtl_event["recycle_tile_cycle"] - rtl_origin,
        }
        gem5_timing = {
            "fire": gem5_event["fire_ps"] // tile_period_ps - gem5_origin,
            "duration": gem5_event["duration_tile_cycles"],
            "recycle": (
                gem5_event["recycle_ps"] // tile_period_ps - gem5_origin
            ),
        }
        for dimension in ("fire", "duration", "recycle"):
            if rtl_timing[dimension] != gem5_timing[dimension]:
                return {
                    "pass": False,
                    "first_divergence": {
                        "kind": dimension,
                        "sequence": sequence,
                        "op": rtl_event["op"],
                        "rtl": rtl_timing[dimension],
                        "gem5": gem5_timing[dimension],
                        "delta": (
                            gem5_timing[dimension] -
                            rtl_timing[dimension]
                        ),
                    },
                }
    return {"pass": True, "first_divergence": None}


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
    parser.add_argument("--max-ticks", type=int, default=0)
    parser.add_argument(
        "--rtl-perf", type=Path,
        help="RTL perf JSONL file or directory for lifecycle comparison",
    )
    parser.add_argument("--rtl-task", type=int, default=0)
    parser.add_argument(
        "--edge-trace", action="store_true",
        help="Record requester/result/Shuffle debug events as edges.trace",
    )
    args = parser.parse_args()

    contract_path = args.case.resolve()
    contract = json.loads(contract_path.read_text(encoding="utf-8"))
    if contract.get("schema") != "venus-shuffle-microbench/v1":
        parser.error(f"not a Shuffle microbench contract: {contract_path}")

    output = args.output_dir.resolve()
    if output.exists() and any(output.iterdir()):
        parser.error(f"immutable output directory is not empty: {output}")
    output.mkdir(parents=True, exist_ok=True)
    debug_dir = output / "debug"

    gem5 = args.gem5.resolve()
    config = args.config.resolve()
    environment = os.environ.copy()
    environment.update(REQUIRED_ENVIRONMENT)
    environment.update({
        "VENUS_GEM5_DEBUG_DIR": str(debug_dir),
        "VENUS_GEM5_TASK_EBREAK_EXIT": "1",
    })

    command = [
        str(gem5),
        "--listener-mode=off",
        f"--outdir={output}",
        "--redirect-stdout",
        "--redirect-stderr",
    ]
    if args.edge_trace:
        command += [
            "--debug-flags=LaneVFU,LaneOperandRequester,LaneSequencer,"
            "ShuffleEngine,VenusSequencer",
            "--debug-file=edges.trace",
        ]
    command += [
        str(config),
        f"--binary={Path(contract['gem5_elf']).resolve()}",
        "--venus-config=venus-rtl-16x128",
    ]
    if args.max_ticks:
        command.append(f"--max-ticks={args.max_ticks}")

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

    monitor = debug_dir / "venusgem5_sequencer_monitor.json"
    lifecycle = []
    normalization_error = None
    if monitor.exists():
        try:
            lifecycle = normalized_lifecycle(
                monitor, contract["model_contract"]["tile_period_ps"]
            )
        except (KeyError, ValueError, RuntimeError) as error:
            normalization_error = str(error)

    rtl = []
    comparison = None
    if args.rtl_perf:
        rtl = rtl_lifecycle(args.rtl_perf.resolve(), args.rtl_task)
        comparison = compare_lifecycle(
            rtl, lifecycle, contract["model_contract"]["tile_period_ps"],
            contract["operation"],
        )

    report = {
        "schema": "venus-shuffle-microbench-result/v1",
        "case": str(contract_path),
        "case_name": contract["name"],
        "dependency": contract["dependency"],
        "gem5": str(gem5),
        "gem5_sha256": sha256(gem5),
        "required_environment": REQUIRED_ENVIRONMENT,
        "returncode": completed.returncode,
        "monitor": str(monitor),
        "normalization_error": normalization_error,
        "lifecycle": lifecycle,
        "rtl_perf": str(args.rtl_perf.resolve()) if args.rtl_perf else None,
        "rtl_task": args.rtl_task if args.rtl_perf else None,
        "rtl_lifecycle": rtl,
        "comparison": comparison,
    }
    report_path = output / "result.json"
    report_path.write_text(
        json.dumps(report, indent=2) + "\n", encoding="utf-8"
    )
    print(report_path)
    passed = completed.returncode == 0 and lifecycle and not normalization_error
    if comparison is not None:
        passed = passed and comparison["pass"]
    raise SystemExit(0 if passed else 1)


if __name__ == "__main__":
    main()
