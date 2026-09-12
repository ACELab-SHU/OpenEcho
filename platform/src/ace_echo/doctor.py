from __future__ import annotations

from dataclasses import asdict, dataclass
from pathlib import Path
import shutil

from .backend import BackendProfile
from .config import PlatformConfig
from .forge import software_validation_mode


@dataclass(frozen=True)
class Check:
    name: str
    status: str
    path: str | None
    detail: str


def _exists(name: str, path: Path, kind: str = "file") -> Check:
    predicate = path.is_dir if kind == "directory" else path.is_file
    okay = predicate()
    return Check(name, "PASS" if okay else "FAIL", str(path),
                 kind if okay else f"missing {kind}")


def run_doctor(config: PlatformConfig,
               backend: BackendProfile | None = None,
               scope: str = "full") -> dict[str, object]:
    if scope not in ("fast", "software", "full", "diagnostic"):
        raise ValueError(f"unsupported doctor scope: {scope}")
    p = config.projects
    dsl_root = (
        backend.dsl_root
        if backend is not None else p.toolchain_root / "dsl"
    )
    workload_root = (
        backend.workload_root if backend is not None else p.workload_root
    )
    gem5_root = backend.gem5_root if backend is not None else p.gem5_root
    scheduler_root = (
        backend.scheduler_root if backend is not None else p.scheduler_root
    )
    gem5 = backend.gem5 if backend is not None else config.gem5
    validation_mode = software_validation_mode(backend.manifest) if backend else "fast"
    venus_llvm_bin = (
        backend.venus_llvm_bin
        if backend is not None else config.compiler.venus_llvm_bin
    )
    checks = [
        _exists("DSL root", dsl_root, "directory"),
        _exists("DSL config", dsl_root / "config.mk"),
        _exists("workload root", workload_root, "directory"),
        _exists("Venus clang", venus_llvm_bin / "clang"),
        _exists("Venus opt", venus_llvm_bin / "opt"),
        _exists("Venus llc", venus_llvm_bin / "llc"),
        _exists("Scheduler cross GCC",
                Path(config.compiler.scheduler_cross_prefix + "gcc")),
        _exists("Gem5 root", gem5_root, "directory"),
        _exists("Gem5 fast binary", gem5.fast_binary),
        _exists("Gem5 verification binary", gem5.verification_binary),
        _exists("Gem5 config", gem5.config_script),
        _exists("single-task adapter", gem5_root / "tools/venus_single_task_manifest.py"),
        _exists("DAG adapter", gem5_root / "tools/venus_dag.py"),
        _exists("Scheduler/L1 adapter", gem5_root / "tools/venus_l1_dag.py"),
        _exists("Scheduler root", scheduler_root, "directory"),
        _exists("Scheduler Makefile", scheduler_root / "Makefile"),
    ]
    if scope != "diagnostic" and (scope == "fast" or validation_mode == "fast"):
        checks = [check for check in checks if check.name != "Gem5 verification binary"]
    if scope != "full":
        excluded = {"Scheduler cross GCC", "Scheduler root", "Scheduler Makefile", "Scheduler/L1 adapter"}
        if scope == "fast":
            excluded.add("Gem5 verification binary")
        checks = [check for check in checks if check.name not in excluded]
    if backend is not None and scope == "full":
        checks.extend([
            _exists("backend manifest", backend.source),
            _exists("RTL root", backend.rtl_root, "directory"),
            _exists("RTL config", backend.rtl_root / ".config"),
            _exists("RTL Makefile", backend.rtl_root / "Makefile"),
        ])
    for tool in ("git", "make", "riscv64-linux-gnu-objcopy", "riscv64-linux-gnu-ld"):
        resolved = shutil.which(tool)
        checks.append(Check(tool, "PASS" if resolved else "FAIL", resolved,
                            "executable" if resolved else "not found in PATH"))
    return {
        "schema": "ace-echo-doctor/v1",
        "scope": scope,
        "qualification": "ENVIRONMENT_PATH_CHECK_ONLY",
        "status": "PASS" if all(item.status == "PASS" for item in checks) else "FAIL",
        "backend_id": backend.backend_id if backend is not None else None,
        "backend_manifest": str(backend.source) if backend is not None else None,
        "clock_contract": (
            backend.clock_contract_id if backend is not None else None
        ),
        "system_axi_hz": backend.system_axi_hz if backend is not None else None,
        "tile_hz": backend.tile_hz if backend is not None else None,
        "checks": [asdict(item) for item in checks],
        "runtime_policy": {
            "full_software_engine": "gem5." + validation_mode,
            "diagnostic_engine": "gem5.verification",
            "vemu_emulator": "DISABLED",
            "owned_dsl_frontend": "ENABLED",
            "external_compilers": "ALLOWED",
            "scheduler_build": "ISOLATED_COPY",
            "gem5": "PRIMARY_EXECUTOR",
            "rtl_repository": "SELECTED" if backend is not None and scope == "full" else "NOT_REQUIRED",
            "rtl_evidence": "OPTIONAL_INPUT_ONLY",
        },
    }
