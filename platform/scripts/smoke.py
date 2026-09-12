#!/usr/bin/env python3
"""Run the tracked Hub onboarding DAG, not a removed historical probe."""

from pathlib import Path
import subprocess
import argparse
import sys


ROOT = Path(__file__).resolve().parents[1]
def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--config", type=Path)
    parser.add_argument("--backend", type=Path)
    parser.add_argument("--output", type=Path)
    parser.add_argument("--keep-heavy", action="store_true")
    args = parser.parse_args()
    if args.config is None and (ROOT / ".ace-echo/project.toml").is_file():
        sys.path.insert(0, str(ROOT / "src"))
        from ace_echo.selection import prepare, check_backend, object_file
        config, context = prepare(ROOT)
        args.config = config.source
        if args.backend:
            check_backend(context, object_file(args.backend))
        else:
            args.backend = Path(context["backend"])
        args.output = args.output or config.run_root / "onboarding-smoke-a001"
    else:
        args.config = args.config or ROOT / ".ace-echo/host/local.toml"
        args.backend = args.backend or args.config.parent / "backend.json"
    args.output = args.output or ROOT / "runs/onboarding-smoke-a001"
    source = ROOT / "workloads/5g_lite/tasks/forge_vector_smoke/run.py"
    if not source.is_file() or not args.config.is_file() or not args.backend.is_file():
        parser.error("Initialize the pinned Hub submodule and run scripts/bootstrap.py configure; see docs/GETTING_STARTED.md")
    command = [sys.executable, str(source), "--platform-root", str(ROOT), "--config", str(args.config.resolve()),
               "--backend", str(args.backend.resolve()), "--output", str(args.output.resolve())]
    if args.keep_heavy:
        command.append("--keep-heavy")
    return subprocess.call(command)


if __name__ == "__main__":
    raise SystemExit(main())
