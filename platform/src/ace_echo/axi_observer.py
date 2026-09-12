"""Opt-in, independent read-only RTL AXI evidence launch.

Run with ``PYTHONPATH=src python3 -m ace_echo.axi_observer --help``. Existing
``ace-echo rtl run`` and its native judges remain unchanged. An observer run is
not a complete workload qualification: its bytes still need independent golden
and software comparisons.
"""
from __future__ import annotations

import argparse
import copy
import hashlib
import json
from pathlib import Path
import re
import sys

from .adapters.rtl import RtlAdapter, RtlCase
from .artifacts import git_identity
from .axi_capture import reconstruct
from .backend import resolve_backend_profile
from .config import load_config
from .process import Runner
from .rtl_preflight import run_rtl_preflight


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def checked(path: Path, digest: str) -> Path:
    if not re.fullmatch("[0-9a-f]{64}", digest) or sha256(path) != digest:
        raise ValueError(f"input identity mismatch: {path}")
    return path


def validate_contract(backend, path: Path, digest: str) -> dict:
    checked(path, digest)
    contract = json.loads(path.read_text())
    if (contract["schema"] != "ace-echo-readonly-axi-observer/v1"
            or contract["backend_id"] != backend.backend_id
            or contract["backend_sha256"] != sha256(backend.source)
            or contract["rtl_commit"] != git_identity(backend.rtl_root, "rtl")["head"]):
        raise ValueError("observer/backend/RTL identity mismatch")
    if not contract["rtl_sources"]:
        raise ValueError("empty observer source binding")
    for relative, expected in contract["rtl_sources"].items():
        p = Path(relative)
        if p.is_absolute() or ".." in p.parts:
            raise ValueError("unsafe observer source path")
        checked(backend.rtl_root / p, expected)
    for key in ("hierarchy", "ucli_hierarchy"):
        if not re.fullmatch(r"[A-Za-z_][A-Za-z0-9_]*(?:\.[A-Za-z_][A-Za-z0-9_]*)+", contract[key]):
            raise ValueError("unsafe observer hierarchy")
    if contract["hierarchy"] != "testbench." + contract["ucli_hierarchy"]:
        raise ValueError("UCLI and VCD hierarchy differ")
    if contract["data_bytes"] != 64 or contract["max_burst_beats"] != 64:
        raise ValueError("unqualified observer geometry")
    for name, width in contract["signals"].items():
        if not re.fullmatch("[A-Za-z_][A-Za-z0-9_]*", name) or not isinstance(width, int) or not 0 < width <= 4096:
            raise ValueError("unsafe/invalid observer signal")
    for signal, lo, width in contract["fields"].values():
        if signal not in contract["signals"] or not 0 <= lo < lo+width <= contract["signals"][signal]:
            raise ValueError("out-of-range observer field")
    return contract


class AxiObserverAdapter(RtlAdapter):
    """Only add debug metadata and narrow read-only dumping to a new snapshot."""
    def __init__(self, runner, backend, contract):
        super().__init__(runner, backend)
        self.flow = copy.deepcopy(self.flow)
        self.contract = contract
        source = (backend.rtl_root / "sim/Makefile").read_text()
        match = re.search(r'^VCS_FLAGS_NODEBUG="([^"\n]+)"$', source, re.M)
        if match is None or "debug_access" in match[1]:
            raise ValueError("unqualified source compile flags")
        self.flow["make_variables"]["VCS_FLAGS_NODEBUG"] = '"' + match[1] + ' -debug_access+all"'

    def _set_sim_horizon(self, snapshot, horizon):
        super()._set_sim_horizon(snapshot, horizon)
        path = snapshot / "sim/simv_ucli.tcl"
        text = path.read_text()
        signals = " ".join(self.contract["ucli_hierarchy"] + "." + name
                           for name in self.contract["signals"])
        capture = ("# ACE-ECHO read-only accepted-AXI observer\n"
                   "set axi_fid [dump -file l1-axi.vpd -type VPD]\n"
                   f"dump -add {{{signals}}} -fid $axi_fid -aggregates\n"
                   "dump -deltaCycle on -fid $axi_fid\n")
        path.write_text(capture + text)

    def _copy_evidence(self, build, output, extra_patterns=None):
        return super()._copy_evidence(build, output, [*(extra_patterns or []), "l1-axi.vpd"])


def run(args) -> dict:
    backend = resolve_backend_profile(load_config(args.config), args.backend)
    contract = validate_contract(backend, args.contract, args.contract_sha256)
    checked(args.firmware, args.firmware_sha256)
    RtlAdapter._validate_case_name(args.case)
    if not re.fullmatch(r"[A-Za-z0-9_/-]+", args.case):
        raise ValueError("unsafe runtime case name")
    if not 1 <= args.horizon_us <= 1000000 or not 1 <= args.timeout <= 86400:
        raise ValueError("invalid horizon/timeout")
    out = args.out.resolve()
    out.mkdir(parents=True, exist_ok=False)
    runner = Runner(out / "commands")
    artifacts = out / "artifacts"
    artifacts.mkdir()
    preflight = run_rtl_preflight(backend.source)
    (artifacts / "rtl-preflight.json").write_text(json.dumps(preflight, indent=2) + "\n")
    if preflight["status"] != "PASS":
        result = {"status": "BLOCKED", "reason": "RTL_PREFLIGHT", "application_qualified": False}
        (out / "observer-run.json").write_text(json.dumps(result, indent=2) + "\n")
        return result
    adapter = AxiObserverAdapter(runner, backend, contract)
    files = [args.config, args.contract, args.firmware, backend.source,
             backend.rtl_root / "sim/Makefile",
             *(backend.rtl_root / p for p in contract["rtl_sources"]),
             *Path(__file__).parent.glob("*.py"),
             *Path(__file__).parent.joinpath("adapters").glob("*.py")]
    inputs = {str(path.resolve()): sha256(path) for path in files}
    result = {"status": "RUNNING", "application_qualified": False,
              "scope": "read-only accepted-AXI observation; independent output comparison still required",
              "inputs": inputs, "make_variables": adapter.flow["make_variables"],
              "rtl_before": git_identity(backend.rtl_root, "rtl")}
    report = out / "observer-run.json"
    report.write_text(json.dumps(result, indent=2) + "\n")
    try:
        native_report = adapter.run(workspace=artifacts,
                                    cases=[RtlCase(args.case, args.firmware.resolve())],
                                    timeout=args.timeout, keep_build=True,
                                    sim_horizon_us=args.horizon_us)
        native = json.loads(native_report.read_text())
        result["native_report"] = {"path": str(native_report), "sha256": sha256(native_report)}
        case = native["cases"][0]
        if not native["fresh_compile"] or not native["source_unchanged"] or case["l1_sha256"] != args.firmware_sha256:
            raise ValueError("invalid fresh RTL/firmware handoff")
        evidence = artifacts / "rtl" / args.case.replace("/", "_")
        simlog = evidence / "sim.log"
        if re.search(r"Error-\[UCLI", simlog.read_text()):
            raise ValueError("UCLI observation command failed")
        vpd = evidence / "l1-axi.vpd"
        entries = [e for e in case["evidence"] if Path(e["path"]) == vpd]
        if len(entries) != 1:
            raise ValueError("missing or ambiguous raw VPD evidence")
        checked(vpd, entries[0]["sha256"])
        vcd = artifacts / "l1-axi.vcd"
        environment = adapter._environment()
        converter = Path(environment["VCS_HOME"]) / "bin/vpd2vcd"
        if not converter.is_file():
            raise ValueError("backend VPD converter missing")
        result["converter"] = {"path": str(converter), "sha256": sha256(converter)}
        runner.run("convert-axi-vpd", [converter, "-full64", vpd, vcd],
                   cwd=artifacts, timeout=args.timeout, env=environment)
        result["vcd"] = {"path": str(vcd), "sha256": sha256(vcd), "bytes": vcd.stat().st_size}
        transfers, samples = reconstruct(vcd, contract)
        capture_dir = artifacts / "accepted-dma"
        capture_dir.mkdir()
        records = []
        for record in transfers:
            payload = record.pop("payload")
            target = capture_dir / f"transfer-{record['index']:05d}.bin"
            target.write_bytes(payload)
            records.append({**record, "payload_path": str(target), "payload_sha256": sha256(target)})
        capture = capture_dir / "transactions.json"
        capture.write_text(json.dumps({"status": "PASS_CAPTURE_ONLY", "clock_samples": samples,
                                       "records": records}, indent=2) + "\n")
        result["capture"] = {"path": str(capture), "sha256": sha256(capture)}
        result["transfer_count"] = len(transfers)
        result["accepted_bytes"] = sum(r["length"] for r in records)
        result["status"] = "PASS_CAPTURE_ONLY"
    except Exception as exc:
        result["status"] = "FAIL"
        result["error"] = str(exc)
    finally:
        result["rtl_after"] = git_identity(backend.rtl_root, "rtl")
        result["inputs_unchanged"] = all(Path(p).is_file() and sha256(Path(p)) == value for p, value in inputs.items())
        if result["rtl_after"] != result["rtl_before"] or not result["inputs_unchanged"]:
            result["status"] = "FAIL"
            result["identity_error"] = "source/input changed during execution"
        report.write_text(json.dumps(result, indent=2) + "\n")
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--config", type=Path, required=True)
    parser.add_argument("--backend", type=Path, required=True)
    parser.add_argument("--contract", type=Path, required=True)
    parser.add_argument("--contract-sha256", required=True)
    parser.add_argument("--case", required=True)
    parser.add_argument("--firmware", type=Path, required=True)
    parser.add_argument("--firmware-sha256", required=True)
    parser.add_argument("--out", type=Path, required=True)
    parser.add_argument("--horizon-us", type=int, default=200000)
    parser.add_argument("--timeout", type=int, default=14400)
    args = parser.parse_args()
    result = run(args)
    print(json.dumps(result, indent=2))
    return 0 if result["status"] == "PASS_CAPTURE_ONLY" else 1


if __name__ == "__main__":
    sys.exit(main())
