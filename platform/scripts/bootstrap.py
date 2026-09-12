#!/usr/bin/env python3
"""Explicit, checkout-local setup. Never downloads proprietary tools or runs RTL."""
from __future__ import annotations

import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
JSON_COMMIT = "9cca280a4d0ccf0c08f47a99aa71d1b0e52f8d03"
JSON_URL = "https://github.com/nlohmann/json.git"


def sha(path):
    digest = hashlib.sha256()
    with Path(path).open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def git(root, *args):
    return subprocess.check_output(["git", "-C", str(root), *args], text=True).strip()


def save(path, value):
    with Path(path).open("x", encoding="utf-8") as stream:
        json.dump(value, stream, indent=2)
        stream.write("\n")


def fetch_json(root):
    target = root / ".ace-echo/deps/json"
    if target.exists():
        raise FileExistsError(f"Refusing to replace {target}")
    target.parent.mkdir(parents=True, exist_ok=True)
    subprocess.run(["git", "clone", "--no-checkout", "--filter=blob:none", JSON_URL, str(target)], check=True)
    subprocess.run(["git", "-C", str(target), "checkout", "--detach", JSON_COMMIT], check=True)
    if git(target, "rev-parse", "HEAD") != JSON_COMMIT:
        raise ValueError("JSON dependency revision mismatch")
    print(target)


def configure(root, tools_file, backend_file, destination):
    """Relocate installation paths only; preserve architecture and validation gates."""
    tools = json.loads(tools_file.read_text())
    if tools.get("schema") != "ace-echo-host-tools/v1":
        raise ValueError("Unsupported host tools schema")
    for key in ("venus_llvm_bin", "riscv_gcc_root"):
        if not isinstance(tools.get(key), str) or not tools[key].strip():
            raise ValueError(f"Missing non-empty host tool path: {key}")
    for key in ("rtl_root", "scheduler_cross_prefix"):
        if key in tools and (not isinstance(tools[key], str) or not tools[key].strip()):
            raise ValueError(f"Invalid optional host path: {key}")
    destination = destination.resolve()
    destination.relative_to(root / ".ace-echo")
    if destination.exists():
        raise FileExistsError(f"Use a new host directory; refusing to overwrite {destination}")
    llvm = Path(tools["venus_llvm_bin"]).expanduser().resolve()
    gcc = Path(tools["riscv_gcc_root"]).expanduser().resolve()
    # The legacy DSL passes tool locations through Make; whitespace/metacharacters
    # are not supported by that interface. Reject rather than shell-interpolate.
    allowed = set("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789/._+-")
    for path in (llvm, gcc):
        if any(c not in allowed for c in str(path)):
            raise ValueError(f"DSL tool path contains unsupported characters: {path}")
    binaries = [llvm / name for name in ("clang", "opt", "llc", "llvm-objcopy", "llvm-objdump")]
    binaries += [gcc / "bin/riscv32-unknown-elf-gcc"]
    for path in binaries:
        if not path.is_file() or not os.access(path, os.X_OK):
            raise FileNotFoundError(f"Install the approved external tool first: {path}")
    dsl = root / "components/toolchain/dsl"
    hub = root / "workloads"
    for path in (dsl / "config.mk", hub / "5g_lite/task-index.json"):
        if not path.is_file():
            raise FileNotFoundError(f"Initialize pinned submodules first: {path}")
    if (git(dsl, "rev-parse", "HEAD") != git(root, "rev-parse", "HEAD:components/toolchain/dsl")
            or git(dsl, "status", "--porcelain", "--untracked-files=no")):
        raise ValueError("DSL must match the pinned commit with no tracked changes")
    json_source = root / ".ace-echo/deps/json"
    if not json_source.is_dir() or git(json_source, "rev-parse", "HEAD") != JSON_COMMIT:
        raise ValueError("Run bootstrap.py fetch-json first (pinned dependency)")
    if git(json_source, "status", "--porcelain", "--untracked-files=all"):
        raise ValueError("Pinned JSON dependency has modified or untracked files")
    sys.path.insert(0, str(root / "src"))
    from ace_echo.forge import validate_backend_manifest
    manifest = validate_backend_manifest(backend_file)
    destination.mkdir(parents=True)
    shadow = destination / "dsl"
    shadow.mkdir()
    tracked = subprocess.check_output(["git", "-C", str(dsl), "ls-files", "-z"]).decode().split("\0")
    for relative in filter(None, tracked):
        source = dsl / relative
        source.resolve().relative_to(dsl.resolve())
        if source.is_symlink() or not source.is_file():
            raise ValueError(f"Unsupported DSL source entry: {relative}")
        target = shadow / relative
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(source, target)
    # Supply a pinned dependency in the generated copy, never change the DSL submodule.
    json_dest = shadow / "heft_new/json/include"
    shutil.copytree(json_source / "include", json_dest)
    base = backend_file.resolve().parent
    for key, value in manifest["paths"].items():
        path = Path(value).expanduser()
        manifest["paths"][key] = str((base / path).resolve() if not path.is_absolute() else path)
    manifest["paths"]["dsl_root"] = str(shadow)
    # Absent RTL is deliberate for software onboarding, not a dummy qualification.
    manifest["paths"]["rtl_root"] = str(Path(tools.get("rtl_root", destination / "RTL_NOT_INSTALLED")).expanduser().resolve())
    toolchain = manifest["engines"].setdefault("toolchain", {})
    toolchain["venus_llvm_bin"] = str(llvm)
    toolchain["riscv_gcc_root"] = str(gcc)
    toolchain.setdefault("make_variables", {})["VENUSARCH"] = (
        f"--target=riscv32-unknown-elf --gcc-toolchain={gcc} -march=rv32imazvenus"
    )
    scheduler = manifest["engines"]["scheduler"]
    for key in ("linker_script", "pll_helper_body", "devctrl_init_body"):
        if key in scheduler:
            scheduler[key] = str((base / scheduler[key]).resolve())
    for contract in manifest.get("clock_contracts", {}).values():
        if isinstance(contract, dict) and "firmware" in contract:
            path = Path(contract["firmware"])
            if not path.is_absolute():
                path = base / path if (base / path).is_file() else root / path
            contract["firmware"] = str(path.resolve())
    backend_out = destination / "backend.json"
    save(backend_out, manifest)
    config = destination / "local.toml"
    values = {
        "projects": {"toolchain_root": str(destination), "workload_root": str(hub / "5g_lite"),
                     "gem5_root": str(root / "components/gem5"), "scheduler_root": str(root / "components/scheduler")},
        "compiler": {"venus_llvm_bin": str(llvm), "riscv_gcc_root": str(gcc),
                     "scheduler_cross_prefix": tools.get("scheduler_cross_prefix", str(destination / "SCHEDULER_NOT_INSTALLED-"))},
        "gem5": {"fast_binary": "build/RISCV/gem5.opt", "verification_binary": "build/RISCV/gem5.debug",
                 "config_script": "configs/tutorial/part1/packet_gen.py",
                 "venus_config": manifest["engines"]["gem5"]["fast"]["venus_config"],
                 "fast_mode": "fast", "verification_mode": "verification"},
        "scheduler": {"default_target": "venus", "default_main": "src/main.c"},
        "timeouts": {"task_seconds": 120, "dag_seconds": 300, "application_seconds": 600},
    }
    with config.open("x") as stream:
        stream.write('schema_version = 1\nrun_root = ' + json.dumps(str(root / "runs")) + '\n')
        for section, fields in values.items():
            stream.write(f"\n[{section}]\n")
            for key, value in fields.items():
                stream.write(f"{key} = {json.dumps(value)}\n")
    from ace_echo.config import load_config
    from ace_echo.backend import resolve_backend_profile
    resolve_backend_profile(load_config(config), backend_out)
    save(destination / "setup-receipt.json", {
        "schema": "ace-echo-onboarding-setup/v1", "scope": "GEM5_SOFTWARE_ONLY",
        "platform_commit": git(root, "rev-parse", "HEAD"), "hub_commit": git(hub, "rev-parse", "HEAD"),
        "dsl_commit": git(dsl, "rev-parse", "HEAD"), "json_commit": JSON_COMMIT,
        "source_backend": str(backend_file.resolve()), "source_backend_sha256": sha(backend_file),
        "resolved_backend_sha256": sha(backend_out), "config_sha256": sha(config),
        "tools": [{"path": str(p), "sha256": sha(p)} for p in binaries],
        "rtl_qualified": False,
    })
    print(json.dumps({"config": str(config), "backend": str(backend_out)}, indent=2))


def build_gem5(root, jobs, mode):
    if not 1 <= jobs <= 64:
        raise ValueError("jobs must be 1..64; choose a shared-host resource budget")
    receipts = root / ".ace-echo"
    receipts.mkdir(exist_ok=True)
    log = receipts / f"build-gem5-{mode}.log"
    command = ["scons", f"build/RISCV/gem5.{mode}", f"-j{jobs}", "--ignore-style"]
    source = root / "components/gem5"
    with log.open("x") as stream:
        result = subprocess.run(command, cwd=source, stdout=stream, stderr=subprocess.STDOUT)
    binary = source / f"build/RISCV/gem5.{mode}"
    save(receipts / f"build-gem5-{mode}.json", {
        "argv": command, "cwd": str(source), "exit_code": result.returncode,
        "source_tree": git(root, "rev-parse", "HEAD:components/gem5"),
        "source_status": git(root, "status", "--porcelain", "--", "components/gem5"),
        "binary_sha256": sha(binary) if result.returncode == 0 and binary.is_file() else None,
        "log": str(log),
    })
    print(log)
    return result.returncode


def install_gem5(root, binary, receipt_path, mode):
    """Install an explicitly supplied, same-source build; no silent old-binary fallback."""
    receipt = json.loads(receipt_path.read_text())
    if (receipt.get("exit_code") != 0 or receipt.get("source_status") != ""
            or receipt.get("source_tree") != git(root, "rev-parse", "HEAD:components/gem5")
            or receipt.get("binary_sha256") != sha(binary)
            or f"build/RISCV/gem5.{mode}" not in receipt.get("argv", [])):
        raise ValueError("Binary/receipt/source-tree/mode mismatch or dirty source build")
    if git(root, "status", "--porcelain", "--", "components/gem5"):
        raise ValueError("Target Gem5 source is modified")
    target = root / f"components/gem5/build/RISCV/gem5.{mode}"
    if target.exists():
        raise FileExistsError(target)
    target.parent.mkdir(parents=True, exist_ok=True)
    with binary.open("rb") as src, target.open("xb") as dst:
        shutil.copyfileobj(src, dst, 1024 * 1024)
    target.chmod(0o755)
    if sha(target) != receipt["binary_sha256"]:
        raise ValueError("Installed binary checksum mismatch")
    (root / ".ace-echo").mkdir(exist_ok=True)
    save(root / f".ace-echo/installed-gem5-{mode}.json", {
        "source_receipt": str(receipt_path.resolve()), "receipt_sha256": sha(receipt_path),
        "binary": str(target), "build_receipt": receipt,
    })
    print(target)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    sub = parser.add_subparsers(dest="action", required=True)
    sub.add_parser("fetch-json", help="explicit network fetch of pinned open-source JSON dependency")
    configure_cmd = sub.add_parser("configure")
    configure_cmd.add_argument("--tools", required=True, type=Path)
    configure_cmd.add_argument("--backend", required=True, type=Path)
    configure_cmd.add_argument("--output", type=Path, default=ROOT / ".ace-echo/host")
    build = sub.add_parser("build-gem5")
    build.add_argument("--jobs", type=int, default=4)
    build.add_argument("--mode", choices=("opt", "debug"), default="opt")
    install = sub.add_parser("install-gem5", help="explicit same-source binary installation with a build receipt")
    install.add_argument("--binary", type=Path, required=True)
    install.add_argument("--receipt", type=Path, required=True)
    install.add_argument("--mode", choices=("opt", "debug"), default="opt")
    args = parser.parse_args()
    if args.action == "fetch-json":
        fetch_json(ROOT)
    elif args.action == "configure":
        configure(ROOT, args.tools, args.backend, args.output)
    elif args.action == "install-gem5":
        install_gem5(ROOT, args.binary, args.receipt, args.mode)
    else:
        return build_gem5(ROOT, args.jobs, args.mode)
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (ValueError, OSError, subprocess.CalledProcessError) as error:
        print(f"BLOCKED: {error}", file=sys.stderr)
        raise SystemExit(2)
