"""Host-local hardware selection; does not change any hardware semantics."""
from __future__ import annotations

from dataclasses import replace
import contextlib
import hashlib
import importlib.util
import json
from pathlib import Path
import re
import sys

from .artifacts import sha256
from .config import ConfigError, _fallback_toml, load_config, tomllib


def digest(value):
    return hashlib.sha256(json.dumps(value, sort_keys=True).encode()).hexdigest()


def object_file(path):
    try:
        value = json.loads(path.read_text())
    except (OSError, ValueError) as error:
        raise ConfigError(f"Cannot read {path}: {error}") from error
    if not isinstance(value, dict):
        raise ConfigError(f"Expected JSON object: {path}")
    return value


def save(path, value):
    with path.open("x") as stream:
        json.dump(value, stream, indent=2)
        stream.write("\n")


def plan(root):
    project = root / ".ace-echo/project.toml"
    text = project.read_text()
    raw = tomllib.loads(text) if tomllib else _fallback_toml(text)
    if set(raw) != {"hardware", "toolchain"}:
        raise ConfigError("project.toml requires only hardware and toolchain")
    for key in raw:
        if not isinstance(raw[key], str) or not re.fullmatch(r"[A-Za-z0-9_.-]+", raw[key]):
            raise ConfigError(f"Invalid project {key}")
    catalog_path = root / "configs/hardware-profiles.json"
    catalog = object_file(catalog_path)
    if catalog.get("schema") != "ace-echo-hardware-profiles/v1":
        raise ConfigError("Invalid hardware catalog schema")
    entry = catalog.get("profiles", {}).get(raw["hardware"])
    if not isinstance(entry, dict) or not isinstance(entry.get("backend"), str):
        raise ConfigError(f"Unknown hardware: {raw['hardware']}")
    backend = (catalog_path.parent / entry["backend"]).resolve()
    backend.relative_to((root / "configs/backends").resolve())
    host = object_file(root / ".ace-echo/host-paths.json")
    if host.get("schema") != "ace-echo-host-paths/v1" or set(host) - {"schema", "toolchains", "rtl"}:
        raise ConfigError("Invalid host-paths schema or unknown fields")
    toolchains = host.get("toolchains")
    if not isinstance(toolchains, dict) or not isinstance(toolchains.get(raw["toolchain"]), dict):
        raise ConfigError(f"Unknown host toolchain: {raw['toolchain']}")
    tools = dict(toolchains[raw["toolchain"]])
    if set(tools) - {"venus_llvm_bin", "riscv_gcc_root", "scheduler_cross_prefix"}:
        raise ConfigError("Only compiler installation paths belong in toolchains")
    for key in ("venus_llvm_bin", "riscv_gcc_root"):
        if not isinstance(tools.get(key), str) or not tools[key]:
            raise ConfigError(f"Missing {key} in selected toolchain")
    rtl = host.get("rtl", {})
    if not isinstance(rtl, dict):
        raise ConfigError("host-paths.rtl must map hardware names to directories")
    if raw["hardware"] in rtl:
        tools["rtl_root"] = rtl[raw["hardware"]]
    for key, value in tools.items():
        if not isinstance(value, str) or not value or not Path(value).expanduser().is_absolute():
            raise ConfigError(f"Host path must be absolute: {key}")
        tools[key] = str(Path(value).expanduser())
    tools["schema"] = "ace-echo-host-tools/v1"
    return raw, backend, tools


def bootstrap_module(root):
    spec = importlib.util.spec_from_file_location("ace_echo_host_bootstrap", root / "scripts/bootstrap.py")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def file_identity(binary):
    stat = binary.stat()
    return [str(binary.resolve()), stat.st_size, stat.st_mtime_ns, stat.st_ctime_ns]


def prepare(root):
    root = root.resolve()
    selected, backend, tools = plan(root)
    bootstrap = bootstrap_module(root)
    dsl = root / "components/toolchain/dsl"
    if (bootstrap.git(dsl, "rev-parse", "HEAD") != bootstrap.git(root, "rev-parse", "HEAD:components/toolchain/dsl")
            or bootstrap.git(dsl, "status", "--porcelain", "--untracked-files=no")):
        raise ConfigError("Selected hardware requires the clean pinned DSL")
    binaries = [Path(tools["venus_llvm_bin"]) / name for name in
                ("clang", "opt", "llc", "llvm-objcopy", "llvm-objdump")]
    binaries += [Path(tools["riscv_gcc_root"]) / "bin/riscv32-unknown-elf-gcc"]
    # Stat identity invalidates the expensive compiler hash cache on replacement.
    # This is reproducibility bookkeeping, not protection against hostile edits.
    scheduler_gcc = Path(tools["scheduler_cross_prefix"] + "gcc") if tools.get("scheduler_cross_prefix") else None
    if scheduler_gcc and scheduler_gcc.is_file():
        binaries.append(scheduler_gcc)
    identities = [file_identity(binary) for binary in binaries]
    key = digest({"selection": selected, "tools": tools, "compiler_files": identities,
                  "backend": sha256(backend), "bootstrap": sha256(root / "scripts/bootstrap.py"),
                  "selector": sha256(Path(__file__)), "root": str(root),
                  "platform": bootstrap.git(root, "rev-parse", "HEAD")})
    parent = root / ".ace-echo/resolved" / selected["hardware"]
    parent.mkdir(parents=True, exist_ok=True)
    destination = parent / key
    destination.resolve().relative_to((root / ".ace-echo").resolve())
    # Linux host: serialize concurrent CLI/agent starts for this configuration.
    import fcntl
    with (parent / (key + ".lock")).open("a") as lock:
        fcntl.flock(lock, fcntl.LOCK_EX)
        if not destination.exists():
            tools_path = parent / (key + ".tools.json")
            if not tools_path.exists():
                save(tools_path, tools)
            with contextlib.redirect_stdout(sys.stderr):
                bootstrap.configure(root, tools_path, backend, destination)
            setup = object_file(destination / "setup-receipt.json")
            context = {
                "schema": "ace-echo-hardware-selection/v1", **selected,
                "backend_id": object_file(backend)["backend_id"],
                "backend": str(destination / "backend.json"),
                "config": str(destination / "local.toml"),
                "source_backend": str(backend), "source_backend_sha256": sha256(backend),
                "run_root": str(root / "runs" / selected["hardware"]),
                "artifact_key": digest({"hardware": selected["hardware"], "backend": sha256(backend),
                                        "tools": setup["tools"],
                                        "scheduler_prefix": tools.get("scheduler_cross_prefix"),
                                        "scheduler_sha256": sha256(scheduler_gcc) if scheduler_gcc and scheduler_gcc.is_file() else None}),
                "compiler_files": identities,
                "setup_receipt_sha256": sha256(destination / "setup-receipt.json"),
                "agent_skill": str(root / ".agents/skills/ace-echo-venus-forge/SKILL.md"),
                "knowledge": {k: str(root / v) for k, v in object_file(backend)["knowledge"].items()},
                "rtl_qualified": False,
            }
            save(destination / "selection.json", context)
        config = load_config(destination / "local.toml")
        context = read_context(config)
        if context is None:
            raise ConfigError(f"Incomplete selection cache: {destination}; inspect failed setup, do not reuse")
    return replace(config, run_root=Path(context["run_root"])), context


def read_context(config):
    path = config.source.parent / "selection.json"
    if not path.exists():
        return None
    context = object_file(path)
    receipt_path = path.parent / "setup-receipt.json"
    receipt = object_file(receipt_path)
    if (context.get("schema") != "ace-echo-hardware-selection/v1"
            or sha256(receipt_path) != context["setup_receipt_sha256"]
            or sha256(config.source) != receipt["config_sha256"]
            or sha256(Path(context["backend"])) != receipt["resolved_backend_sha256"]):
        raise ConfigError("Resolved configuration was modified; edit project/host-paths instead")
    if any(file_identity(Path(entry[0])) != entry for entry in context.get("compiler_files", [])):
        raise ConfigError("Compiler installation changed since resolution; select the project again")
    return context


def check_backend(context, actual):
    expected = object_file(Path(context["backend"]))
    # Workload-local smoke may relocate workload_root, but not hardware semantics.
    for key in ("backend_id", "architecture", "abi", "execution_policy", "static_gates"):
        if expected.get(key) != actual.get(key):
            raise ConfigError(f"Explicit backend conflicts with selected hardware: {key}")
    if expected["engines"]["gem5"] != actual["engines"]["gem5"]:
        raise ConfigError("Gem5 engine conflicts with selected hardware")
    if expected["engines"].get("toolchain") != actual["engines"].get("toolchain"):
        raise ConfigError("Compiler settings conflict with selected toolchain")
    if expected["paths"]["rtl_root"] != actual["paths"]["rtl_root"]:
        raise ConfigError("RTL path conflicts with selected host configuration")
    normalized = dict(actual)
    normalized["paths"] = dict(actual["paths"], workload_root=expected["paths"]["workload_root"])
    if normalized != expected:
        raise ConfigError("Explicit backend changes the selected hardware contract")


def stamp(work):
    context_path = work.path / "hardware-selection.json"
    if not context_path.exists():
        return
    context = object_file(context_path)
    handoff = work.artifacts / ("toolchain" if work.record.scope == "compile-dag" else "scheduler-artifacts")
    files = {str(p.relative_to(work.path)): sha256(p)
             for p in handoff.rglob("*") if p.is_file()}
    save(work.path / "hardware-artifacts.json", {
        "schema": "ace-echo-hardware-artifacts/v1", "artifact_key": context["artifact_key"],
        "backend_id": context["backend_id"], "files": files,
    })


def check_artifact(path, context):
    path = path.resolve()
    for parent in path.parents:
        marker = parent / "hardware-artifacts.json"
        if not marker.is_file():
            continue
        record = object_file(marker)
        producer = object_file(parent / "run.json")
        if producer.get("status") != "PASS" or record.get("schema") != "ace-echo-hardware-artifacts/v1":
            raise ConfigError(f"Producer is not a completed PASS: {parent}")
        if record.get("artifact_key") != context["artifact_key"]:
            raise ConfigError(f"Artifact hardware/toolchain mismatch: {path}; recompile for selected hardware")
        if path.is_dir():
            prefix = str(path.relative_to(parent)) + "/"
            expected = {name for name in record.get("files", {}) if name.startswith(prefix)}
            actual = {str(p.relative_to(parent)) for p in path.rglob("*") if p.is_file()}
            if not expected or actual != expected:
                raise ConfigError(f"Artifact directory membership changed: {path}")
        files = list(path.rglob("*")) if path.is_dir() else [path]
        for item in files:
            if item.is_file() and record.get("files", {}).get(str(item.relative_to(parent))) != sha256(item):
                raise ConfigError(f"Artifact missing from producer receipt or modified: {item}")
        return
    raise ConfigError(f"No hardware producer receipt for {path}; recompile in project-selection mode")
