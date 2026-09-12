#!/usr/bin/env python3
"""Compile and check a fresh two-task DAG; never uses historical run inputs."""
from __future__ import annotations

import argparse
import configparser
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess

from reference import bas, cases, golden

HERE = Path(__file__).resolve().parent


def sha(path):
    h = hashlib.sha256()
    with Path(path).open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            h.update(chunk)
    return h.hexdigest()


def save(path, value):
    with Path(path).open("x") as stream:
        json.dump(value, stream, indent=2)
        stream.write("\n")


def compare(expected, actual):
    if expected != actual:
        first = next((i for i, (a, b) in enumerate(zip(expected, actual)) if a != b), min(len(expected), len(actual)))
        raise ValueError(f"bit-exact mismatch at byte {first}; expected {len(expected)} B, actual {len(actual)} B")


def allocated(root):
    total = 0
    for directory, dirs, files in os.walk(root):
        dirs[:] = [d for d in dirs if not (Path(directory) / d).is_symlink()]
        for name in files:
            total += (Path(directory) / name).lstat().st_blocks * 512
    return total


def timing(gem5):
    events = [json.loads(line) for line in (gem5 / "venus_dag_trace.jsonl").read_text().splitlines()]
    starts = [e for e in events if e["event"] == "tile_allocated"]
    ends = [e for e in events if e["event"] == "dag_complete"]
    if len(starts) != 2 or len(ends) != 1:
        raise ValueError("Expected two task allocations and one DAG completion")
    ini = configparser.ConfigParser(interpolation=None)
    ini.read(str(gem5 / "m5out/config.ini"))
    sequencers = [s for s in ini.sections() if ini.get(s, "type", fallback="") == "VenusSequencer"]
    domains = {ini[s]["clk_domain"] for s in sequencers}
    if len(domains) != 1:
        raise ValueError("Missing or ambiguous active Venus clock domain")
    domain = domains.pop()
    period = int(ini[domain]["clock"])
    if period <= 0:
        raise ValueError("Nonpositive clock period")
    stats = {}
    for line in (gem5 / "m5out/stats.txt").read_text().splitlines():
        fields = line.split()
        if len(fields) >= 2 and fields[0] in {"simFreq", "simTicks"}:
            stats[fields[0]] = int(fields[1])
    freq = stats["simFreq"]
    begin, end = min(e["tick"] for e in starts), ends[0]["tick"]
    if end <= begin or freq <= 0:
        raise ValueError("Invalid measured time interval")
    per_task = []
    for start in starts:
        released = [e for e in events if e["event"] == "tile_released" and e["task_id"] == start["task_id"]]
        if len(released) != 1 or released[0]["tick"] < start["tick"]:
            raise ValueError("Missing task release boundary")
        ticks = released[0]["tick"] - start["tick"]
        per_task.append({"task_id": start["task_id"], "raw_ticks": ticks, "cycles": ticks / period})
    return {"scope": "GEM5_DAG_FIRST_ALLOCATION_TO_COMPLETION", "start_tick": begin, "end_tick": end,
            "raw_ticks": end - begin, "ticks_per_second": freq, "ticks_per_cycle": period,
            "clock_domain": domain, "cycles": (end - begin) / period, "seconds": (end - begin) / freq,
            "per_task_allocation_to_release": per_task, "host_acceptance_included": False,
            "l1_firmware_or_rtl_latency_claim": False}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--platform-root", required=True, type=Path)
    parser.add_argument("--config", required=True, type=Path)
    parser.add_argument("--backend", required=True, type=Path)
    parser.add_argument("--output", required=True, type=Path)
    parser.add_argument("--keep-heavy", action="store_true")
    args = parser.parse_args()
    root, output = args.platform_root.resolve(), args.output.resolve()
    output.mkdir(parents=True, exist_ok=False)
    sources = output / "source"
    sources.mkdir()
    for source in HERE.iterdir():
        if source.is_file():
            shutil.copy2(source, sources / source.name)
    identities = {}
    for name, repository in {"platform": root, "hub": root / "workloads", "dsl": root / "components/toolchain/dsl"}.items():
        identities[name] = {"commit": subprocess.check_output(["git", "-C", str(repository), "rev-parse", "HEAD"], text=True).strip(),
                            "status": subprocess.check_output(["git", "-C", str(repository), "status", "--porcelain", "--untracked-files=no"], text=True)}
    save(output / "repository-identities.json", identities)
    shutil.copy2(args.config, output / "host-config.toml")
    shutil.copy2(args.backend, output / "host-backend.json")
    setup_receipt = args.config.resolve().parent / "setup-receipt.json"
    if setup_receipt.is_file():
        shutil.copy2(setup_receipt, output / "setup-receipt.json")
    reports, receipts = [], []
    def command(argv):
        before = allocated(output)
        log = output / f"command-{len(receipts):02d}.log"
        with log.open("x") as stream:
            result = subprocess.run(list(map(str, argv)), cwd=root, stdout=stream, stderr=subprocess.STDOUT, timeout=900)
        receipt = {"argv": list(map(str, argv)), "cwd": str(root), "exit_code": result.returncode, "log": str(log),
                   "storage_before_bytes": before, "storage_after_bytes": allocated(output)}
        receipts.append(receipt)
        save(output / f"command-{len(receipts)-1:02d}.json", receipt)
        if result.returncode:
            raise RuntimeError(f"Command failed; see {log}")
    prefix = [root / "ace-echo", "--config", args.config.resolve()]
    try:
        command(prefix + ["doctor", "--scope", "fast", "--backend", args.backend.resolve(), "--json"])
        for name, (a, b) in cases().items():
            case = output / name
            target = case / "workload/tasks/forge_vector_smoke"
            target.mkdir(parents=True)
            for source in HERE.glob("*.c"):
                shutil.copy2(source, target / source.name)
            (target / "forge_vector_smoke.bas").write_text(bas(a, b))
            save(case / "inputs.json", {"a": a, "b": b, "dtype": "little-endian signed i16", "shape": [32]})
            expected = golden(a, b)
            for port, payload in expected.items():
                (case / f"expected-{port}.bin").write_bytes(payload)
            backend = json.loads(args.backend.read_text())
            if any(not Path(v).is_absolute() for v in backend["paths"].values()):
                raise ValueError("Run bootstrap configure and pass its resolved backend")
            backend["paths"]["workload_root"] = str(case / "workload")
            save(case / "backend.json", backend)
            command(prefix + ["compile", "dag", "--target", "forge_vector_smoke", "--backend", case / "backend.json", "--run-dir", case / "compile"])
            compiled = case / "compile/artifacts/toolchain"
            command(prefix + ["run", "dag", "--mode", "fast", "--backend", case / "backend.json", "--dag-json", compiled / "dag1.json",
                              "--combined-bin", compiled / "dag1.bin", "--case-dir", compiled / "tasks", "--run-dir", case / "fast"])
            gem5 = case / "fast/artifacts/gem5"
            descriptors = json.loads((compiled / "dag1.json").read_text())
            outputs = []
            for task in descriptors:
                if "current_taskId" not in task:
                    continue
                task_name = task["debug_task_name"]
                port = "sum" if task_name == "Task_forgeAdd" else "restored" if task_name == "Task_forgeRestore" else None
                if port is None or len(task["all_output"]) != 1:
                    raise ValueError("Unexpected smoke task/port ABI")
                actual = gem5 / f"task_{task['current_taskId']}_port_0.bin"
                compare(expected[port], actual.read_bytes())
                outputs.append({"port": port, "path": str(actual), "bytes": len(expected[port]), "sha256": sha(actual)})
            if len(outputs) != 2 or len(list(gem5.glob("task_*_port_*.bin"))) != 2:
                raise ValueError("Unexpected or incomplete all-output coverage")
            report = {"case": name, "status": "PASS", "outputs": outputs, "timing": timing(gem5),
                      "input_sha256": sha(case / "inputs.json"), "backend_sha256": sha(case / "backend.json")}
            save(case / "comparison.json", report)
            reports.append(report)
            print(json.dumps(report), flush=True)
        removed = []
        retained = {str(p): sha(p) for p in output.rglob("*") if p.is_file() and
                    (p.suffix in {".json", ".jsonl", ".c", ".bas", ".py", ".toml", ".ini", ".ld"}
                     or p.name in {"stats.txt", "code.bin", "data.bin", "shared_l2.bin", "dag1.bin"}
                     or p.name.startswith("expected-") or p.parent.name == "tasks" and p.suffix == ".bin")}
        for report in reports:
            for item in report["outputs"]:
                retained[item["path"]] = item["sha256"]
        candidates = []
        if not args.keep_heavy:
            for name in cases():
                replay = output / name / "fast/artifacts/gem5/replay"
                for p in replay.rglob("*"):
                    if p.is_file() and not p.is_symlink() and p.name.endswith((".replay.o", ".replay.high.o", ".replay.elf", ".replay.bin", ".replay.high.bin")):
                        p.resolve().relative_to(output)
                        candidates.append({"path": str(p), "bytes": p.stat().st_blocks * 512, "sha256": sha(p)})
        save(output / "cleanup-manifest.json", {"targets": candidates, "retained_sha256": retained})
        for entry in candidates:
            p = Path(entry["path"])
            if sha(p) != entry["sha256"]:
                raise ValueError("Replay changed before cleanup")
            p.unlink()
            removed.append(entry)
        if any(sha(p) != expected_sha for p, expected_sha in retained.items()):
            raise ValueError("Retained evidence changed during cleanup")
        save(output / "report.json", {"schema": "ace-echo-onboarding-smoke/v1", "status": "PASS", "scope": "GEM5_FAST_SMOKE",
             "reference_state": "PROVISIONAL_GOLDEN", "rtl_qualified": False, "application_complete": False,
             "cases": reports, "commands": receipts, "compaction": {"deleted_files": len(removed), "freed_bytes": sum(p["bytes"] for p in removed)},
             "limitations": ["Not an approved application golden", "No L1 firmware or RTL qualification", "External proprietary compiler installation required"]})
    except Exception as error:
        save(output / "failure.json", {"status": "FAIL", "error": str(error), "completed_cases": reports, "commands": receipts})
        raise
    print(output / "report.json")


if __name__ == "__main__":
    main()
