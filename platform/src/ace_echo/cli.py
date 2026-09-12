from __future__ import annotations

import argparse
from dataclasses import asdict, replace
import json
from pathlib import Path
import sys
import traceback

from . import __version__
from .adapters.gem5 import Gem5Adapter
from .adapters.rtl import RtlAdapter, RtlCase
from .adapters.scheduler import SchedulerAdapter
from .adapters.toolchain import ToolchainAdapter
from .artifacts import RunWorkspace
from .backend import BackendProfile, resolve_backend_profile
from .compare import compare_files, write_result
from .output_validity import compare_binary_output
from .dag_output_compare import (
    compare_dag_outputs,
    compare_dma_returns,
    write_dag_output_result,
)
from .config import ConfigError, PlatformConfig, load_config
from .doctor import run_doctor
from .forge import (
    ForgeContractError,
    prepare_forge_run,
    prune_completed_run,
    validate_backend_manifest,
    validate_forge_request,
)
from .process import CommandFailed, Runner
from .rtl_preflight import RtlPreflightError, run_rtl_preflight
from . import selection


PROJECT_ROOT = Path(__file__).resolve().parents[2]
DEFAULT_CONFIG = PROJECT_ROOT / "configs/local.toml"


def parser() -> argparse.ArgumentParser:
    root = argparse.ArgumentParser(
        prog="ace-echo",
        description="Gem5-first Venus task, DAG, and application platform",
    )
    root.add_argument("--version", action="version", version=__version__)
    root.add_argument("--config", type=Path,
                      help="explicit legacy/resolved config; otherwise use .ace-echo/project.toml when present")
    root.add_argument("--dry-run", action="store_true",
                      help="record commands and manifests without executing tools")
    commands = root.add_subparsers(dest="command", required=True)
    hardware = commands.add_parser("hardware", help="show active hardware, host paths and Forge context")
    hardware.add_argument("action", choices=["show"])

    doctor = commands.add_parser("doctor", help="validate all component paths and tools")
    doctor.add_argument("--json", action="store_true")
    doctor.add_argument("--scope", choices=("fast", "software", "full", "diagnostic"), default="full",
                        help="fast: fast tools; software: backend software policy; full: adds Scheduler/RTL paths; diagnostic: requires debug")
    doctor.add_argument(
        "--backend", type=Path,
        help="validate one complete compiler/Gem5/Scheduler/RTL backend profile",
    )

    compile_cmd = commands.add_parser("compile", help="compile without running VEMU")
    compile_sub = compile_cmd.add_subparsers(dest="compile_scope", required=True)
    compile_dag = compile_sub.add_parser("dag")
    compile_dag.add_argument("--target", required=True)
    compile_dag.add_argument("--params", type=Path,
                             help="optional BAS-syntax parameter input; apostrophes are comments")
    compile_dag.add_argument(
        "--backend", type=Path,
        help="backend manifest supplying compiler architecture parameters",
    )
    compile_dag.add_argument("--run-dir", type=Path)

    scheduler = commands.add_parser("scheduler", help="build Scheduler in an isolated copy")
    scheduler_sub = scheduler.add_subparsers(dest="scheduler_command", required=True)
    scheduler_build = scheduler_sub.add_parser("build")
    scheduler_build.add_argument("--target")
    scheduler_build.add_argument("--main-src")
    scheduler_build.add_argument("--dag-name")
    scheduler_build.add_argument("--dag-json", type=Path)
    scheduler_build.add_argument("--dag-bin", type=Path)
    scheduler_build.add_argument(
        "--backend", type=Path,
        help="backend manifest supplying the physical Scheduler/RTL ABI",
    )
    scheduler_build.add_argument("--run-dir", type=Path)
    scheduler_build.add_argument(
        "--allow-static-only-dag", action="store_true",
        help=("Allow a DAG with zero runtime inputs.  The isolated build "
              "emits inert one-element C arrays; Scheduler runtime semantics "
              "and the source component are unchanged."),
    )
    scheduler_build.add_argument(
        "--auto-static-main", action="store_true",
        help=("Generate a run-local zero-input DAG launcher.  It waits for "
              "all outputs and emits completion GPIOs; correctness must be "
              "checked externally from the complete DMA trace."),
    )

    run = commands.add_parser("run", help="run on Gem5")
    run_sub = run.add_subparsers(dest="scope", required=True)
    common = argparse.ArgumentParser(add_help=False)
    common.add_argument("--mode", choices=["fast", "verification"],
                        default="fast", help="fast (default): normal validation; verification: explicit debug diagnosis")
    common.add_argument("--run-dir", type=Path)
    common.add_argument(
        "--backend", type=Path,
        help="backend manifest selecting the Gem5 architecture profile",
    )

    task = run_sub.add_parser("task", parents=[common])
    task.add_argument("--dag-json", type=Path, required=True)
    task.add_argument("--combined-bin", type=Path, required=True)
    task.add_argument("--case-dir", type=Path, required=True)
    task.add_argument("--task", type=int, required=True)

    dag = run_sub.add_parser("dag", parents=[common])
    dag.add_argument("--dag-json", type=Path, required=True)
    dag.add_argument("--combined-bin", type=Path, required=True)
    dag.add_argument("--case-dir", type=Path, required=True)

    application = run_sub.add_parser("application", parents=[common])
    application.add_argument("--l1-elf", type=Path, required=True)
    application.add_argument("--prefix")
    application.add_argument("--l2-backing-spec", type=Path)
    application.add_argument(
        "--scheduler-engine", choices=["contract", "firmware"],
        default="contract",
        help=(
            "contract autostarts the decoded DAG; firmware executes the exact "
            "l1.elf through Scheduler CPU/MMIO/IRQ/DMA before firing it"
        ),
    )
    application.add_argument(
        "--firmware-completion-gpio-mask", type=lambda value: int(value, 0),
        default=0x8,
        help=(
            "external Scheduler-test completion GPIO mask (default: 0x8); "
            "set 0 to require firmware $stop or the command timeout"
        ),
    )

    compare = commands.add_parser("compare", help="bit-exact file comparison")
    compare.add_argument("expected", type=Path)
    compare.add_argument("actual", type=Path)
    compare.add_argument("--dtype", choices=["raw", "i8", "i16"], default="raw")
    compare.add_argument("--output", type=Path)
    compare.add_argument("--layout", type=Path,
                         help="explicit valid-bit output layout (raw dtype only); padding produces warnings")

    compare_dag = commands.add_parser(
        "compare-dag-outputs",
        help="compare all Gem5 task outputs with a Venus DMA golden trace",
    )
    compare_dag.add_argument("--expected-dma", type=Path, required=True)
    compare_dag.add_argument("--actual-dir", type=Path, required=True)
    compare_dag.add_argument(
        "--trace-template", type=Path,
        help=(
            "Gem5 venus_dag_trace.jsonl used to assign task/port identities "
            "to an unannotated legacy Venus1 RTL DMA trace"
        ),
    )
    compare_dag.add_argument("--output", type=Path)
    compare_dag.add_argument("--output-layouts", type=Path,
                             help="task/port valid-bit layouts; undeclared outputs stay fully strict")
    compare_dag.add_argument('--rtl-log', type=Path, help='reconcile native labels against accepted RTL requests')
    compare_dag.add_argument('--dag-json', type=Path)
    compare_dag.add_argument('--identity-backend', type=Path, help='backend manifest containing flow.return_identity')

    compare_dma = commands.add_parser(
        "compare-dag-dma",
        help="compare all task-return records in two RTL DMA traces",
    )
    compare_dma.add_argument("--expected-dma", type=Path, required=True)
    compare_dma.add_argument("--actual-dma", type=Path, required=True)
    compare_dma.add_argument("--output", type=Path)
    compare_dma.add_argument("--output-layouts", type=Path,
                             help="task/port valid-bit layouts; undeclared outputs stay fully strict")

    forge = commands.add_parser(
        "forge", help="start or finalize an AI-first ACE-ECHO Forge run"
    )
    forge.add_argument("request", type=Path, nargs="?")
    forge.add_argument("--run-dir", type=Path)
    forge.add_argument("--validate-only", action="store_true")
    forge.add_argument("--finalize-run", type=Path)
    forge.add_argument("--keep-all", action="store_true")

    rtl_preflight = commands.add_parser(
        "rtl-preflight",
        help="run configured read-only RTL/testbench compatibility checks",
    )
    rtl_preflight.add_argument("--backend", type=Path)
    rtl_preflight.add_argument("--rtl-root", type=Path)
    rtl_preflight.add_argument("--output", type=Path)

    rtl = commands.add_parser(
        "rtl", help="run an immutable, backend-declared fresh RTL flow"
    )
    rtl_sub = rtl.add_subparsers(dest="rtl_command", required=True)
    rtl_run = rtl_sub.add_parser("run")
    rtl_run.add_argument("--backend", type=Path)
    rtl_run.add_argument(
        "--case", action="append", required=True, metavar="NAME=L1_BIN",
        help=("RTL runtime case and Scheduler firmware; repeat to compile "
              "once and replay additional cases"),
    )
    rtl_run.add_argument("--run-dir", type=Path)
    rtl_run.add_argument('--build-timeout', type=int, help='RTL build wall-clock budget in seconds')
    rtl_run.add_argument('--simulation-timeout', type=int, help='per-case simulation wall-clock budget in seconds')
    rtl_run.add_argument('--dag-json', type=Path, help='compiled DAG for complete return identity/coverage checks')
    rtl_run.add_argument('--gem5-output-dir', type=Path, help='same-case Gem5 task output directory')
    rtl_run.add_argument('--output-layouts', type=Path, help='explicit valid-bit layouts; other bits stay strict')
    rtl_run.add_argument(
        "--keep-build", action="store_true",
        help="retain the large run-local RTL snapshot (off by default)",
    )
    rtl_run.add_argument(
        "--sim-horizon-us", type=int,
        help=("override the run-local UCLI horizon for diagnosis; omitted "
              "keeps the backend repository default"),
    )
    rtl_run.add_argument(
        "--task-boundary-trace", action="store_true",
        help=("use the backend-declared debug build and emit a waveform for "
              "matched per-task RTL execution timing"),
    )
    return root


def components(config: PlatformConfig,
               backend: BackendProfile | None = None) -> dict[str, Path]:
    if backend is not None:
        return backend.components
    return {
        "toolchain": config.projects.toolchain_root,
        "workloads": config.projects.workload_root,
        "gem5": config.projects.gem5_root,
        "scheduler": config.projects.scheduler_root,
    }


def workspace(config: PlatformConfig, scope: str, target: str, mode: str,
              explicit: Path | None,
              backend: BackendProfile | None = None) -> RunWorkspace:
    work = RunWorkspace(
        config.run_root, scope, target, mode, config.source,
        components(config, backend), explicit=explicit,
    )
    context = selection.read_context(config)
    if context:
        selection.save(work.path / "hardware-selection.json", context)
        work.add_input(config.source)
        work.add_input(config.source.parent / "setup-receipt.json")
    return work


def _require_inputs(paths: list[Path], dry_run: bool) -> None:
    if dry_run:
        return
    missing = [str(path) for path in paths if not path.exists()]
    if missing:
        raise FileNotFoundError("missing input(s): " + ", ".join(missing))


def _run_recorded(work: RunWorkspace, action, success_coverage: str,
                  dry_run: bool) -> int:
    try:
        outputs = action()
        for path in outputs:
            if path.exists():
                work.add_output(path)
        if not dry_run and work.record.scope in ("compile-dag", "scheduler-build"):
            selection.stamp(work)
        work.finish("PLANNED" if dry_run else "PASS",
                    "planned" if dry_run else success_coverage)
        print(work.path)
        return 0
    except KeyboardInterrupt:
        work.finish(
            "FAIL", "unproven",
            ["KeyboardInterrupt: run stopped before qualification completed"],
        )
        print("error: run interrupted before qualification completed",
              file=sys.stderr)
        print(f"run record: {work.path / 'run.json'}", file=sys.stderr)
        return 130
    except Exception as error:
        work.finish("FAIL", "unproven", [f"{type(error).__name__}: {error}"])
        print(f"error: {error}", file=sys.stderr)
        if not isinstance(error, (ConfigError, ValueError, PermissionError,
                                  FileNotFoundError, CommandFailed)):
            traceback.print_exc()
        print(f"run record: {work.path / 'run.json'}", file=sys.stderr)
        return 1


def command_compile(args, config: PlatformConfig) -> int:
    backend = (
        resolve_backend_profile(config, args.backend)
        if args.backend else None
    )
    work = workspace(
        config, "compile-dag", args.target, "compile", args.run_dir,
        backend,
    )
    if backend is not None:
        work.add_input(backend.source)
    if args.params is not None:
        work.add_input(args.params)
    runner = Runner(work.commands, args.dry_run)
    adapter = ToolchainAdapter(config, runner, backend)
    return _run_recorded(
        work,
        lambda: [adapter.compile_dag(
            target=args.target, output=work.artifacts,
            timeout=config.timeouts.dag_seconds,
            params=args.params,
        )],
        "compiled_dag_artifacts",
        args.dry_run,
    )


def command_scheduler(args, config: PlatformConfig) -> int:
    if args.auto_static_main and args.main_src:
        raise ValueError(
            "--auto-static-main and --main-src are mutually exclusive"
        )
    backend_profile = (
        resolve_backend_profile(config, args.backend)
        if args.backend else None
    )
    target = args.target or (
        backend_profile.scheduler_build_target
        if backend_profile is not None else config.scheduler.default_target
    )
    main_src = args.main_src or config.scheduler.default_main
    work = workspace(config, "scheduler-build", args.dag_name or target,
                     "build", args.run_dir, backend_profile)
    for item in (args.dag_json, args.dag_bin):
        if item:
            work.add_input(item)
    if backend_profile is not None:
        work.add_input(backend_profile.source)
        if backend_profile.scheduler_linker_script is not None:
            work.add_input(backend_profile.scheduler_linker_script)
        if backend_profile.scheduler_pll_helper_body is not None:
            work.add_input(backend_profile.scheduler_pll_helper_body)
        if backend_profile.scheduler_devctrl_init_body is not None:
            work.add_input(backend_profile.scheduler_devctrl_init_body)
    main_path = Path(main_src).expanduser()
    if main_path.is_file():
        work.add_input(main_path)
    runner = Runner(work.commands, args.dry_run)
    adapter = SchedulerAdapter(config, runner, backend_profile)
    task_input_type_bits = None
    logical_type_map = None
    unsupported_logical_types = None
    max_dag_outputs = None
    output_descriptor_lag_ports = None
    output_descriptor_transfer_chunk_bytes = None
    completion_poll_fallback = None
    completion_poll_interval = None
    task_container_spmd_fields = None
    if args.backend:
        backend = backend_profile.manifest
        descriptor = backend["abi"]["task_input_descriptor"]
        task_input_type_bits = descriptor["physical_type_bits"]
        logical_type_map = descriptor["logical_to_physical_type"]
        unsupported_logical_types = descriptor["rtl_unsupported_logical_types"]
        max_dag_outputs = backend["abi"]["scheduler_output"][
            "max_dag_outputs"
        ]
        output_descriptor_lag_ports = backend["abi"].get(
            "scheduler_output", {}
        ).get("descriptor_response_lag_ports", 0)
        output_descriptor_transfer_chunk_bytes = backend["abi"].get(
            "scheduler_output", {}
        ).get("descriptor_transfer_chunk_bytes", 64)
        completion_poll_fallback = backend["abi"].get(
            "scheduler_output", {}
        ).get("completion_poll_fallback", False)
        completion_poll_interval = backend["abi"].get(
            "scheduler_output", {}
        ).get("completion_poll_interval", 4096)
        task_container_spmd_fields = backend["abi"].get(
            "task_container", {}
        ).get("includes_spmd_fields", True)
    return _run_recorded(
        work,
        lambda: [adapter.build(
            workspace=work.artifacts,
            target=target,
            main_src=main_src,
            dag_name=args.dag_name,
            dag_json=args.dag_json,
            dag_bin=args.dag_bin,
            timeout=config.timeouts.application_seconds,
            task_input_type_bits=task_input_type_bits,
            logical_type_map=logical_type_map,
            unsupported_logical_types=unsupported_logical_types,
            max_dag_outputs=max_dag_outputs,
            output_descriptor_lag_ports=output_descriptor_lag_ports,
            output_descriptor_transfer_chunk_bytes=(
                output_descriptor_transfer_chunk_bytes
            ),
            completion_poll_fallback=completion_poll_fallback,
            completion_poll_interval=completion_poll_interval,
            task_container_spmd_fields=task_container_spmd_fields,
            allow_static_only_dag=args.allow_static_only_dag,
            auto_static_main=args.auto_static_main,
        )],
        "scheduler_l1_artifacts",
        args.dry_run,
    )


def command_run(args, config: PlatformConfig) -> int:
    backend_profile = (
        resolve_backend_profile(config, args.backend)
        if args.backend else None
    )
    if args.scope == "task":
        inputs = [args.dag_json, args.combined_bin, args.case_dir]
        target = f"task-{args.task}"
        timeout = config.timeouts.task_seconds
        coverage = "gem5_single_task_execution"
    elif args.scope == "dag":
        inputs = [args.dag_json, args.combined_bin, args.case_dir]
        target = args.dag_json.stem
        timeout = config.timeouts.dag_seconds
        coverage = "gem5_dag_scheduler_model"
    else:
        inputs = [args.l1_elf]
        if args.l2_backing_spec:
            inputs.append(args.l2_backing_spec)
        if backend_profile is not None:
            inputs.append(backend_profile.source)
        target = args.l1_elf.stem
        timeout = config.timeouts.application_seconds
        coverage = (
            "gem5_scheduler_firmware_application_model"
            if args.scheduler_engine == "firmware"
            else "gem5_scheduler_contract_model"
        )
    _require_inputs(inputs, args.dry_run)
    if backend_profile is not None and backend_profile.source not in inputs:
        inputs.append(backend_profile.source)
    work = workspace(
        config, args.scope, target, args.mode, args.run_dir, backend_profile
    )
    for item in inputs:
        if item.exists():
            work.add_input(item)
    runner = Runner(work.commands, args.dry_run)
    adapter = Gem5Adapter(config, runner, backend_profile)
    task_input_type_bits = 3
    task_container_spmd_fields = True
    if args.scope == "application" and args.backend:
        backend = backend_profile.manifest
        task_input_type_bits = backend["abi"]["task_input_descriptor"][
            "physical_type_bits"
        ]
        task_container_spmd_fields = backend["abi"].get(
            "task_container", {}
        ).get("includes_spmd_fields", True)

    def action() -> list[Path]:
        output = work.artifacts / "gem5"
        output.mkdir(parents=True, exist_ok=True)
        if args.scope == "task":
            manifest = adapter.run_task(
                dag_json=args.dag_json, combined_bin=args.combined_bin,
                case_dir=args.case_dir, task=args.task, output=output,
                mode=args.mode, timeout=timeout,
            )
        elif args.scope == "dag":
            manifest = adapter.run_dag(
                dag_json=args.dag_json, combined_bin=args.combined_bin,
                case_dir=args.case_dir,
                output=output, mode=args.mode, timeout=timeout,
            )
        else:
            manifest = adapter.run_application_contract(
                l1_elf=args.l1_elf, output=output, mode=args.mode,
                timeout=timeout, prefix=args.prefix,
                l2_backing_spec=args.l2_backing_spec,
                task_input_type_bits=task_input_type_bits,
                task_container_spmd_fields=task_container_spmd_fields,
                execute_firmware=args.scheduler_engine == "firmware",
                firmware_completion_gpio_mask=(
                    args.firmware_completion_gpio_mask
                    if args.scheduler_engine == "firmware" else 0
                ),
            )
        return [manifest, manifest.parent / "venus_dag_trace.jsonl",
                output / "venus_dag_trace.jsonl",
                output / "firmware-trace.jsonl",
                output / "m5out/stats.txt"]

    return _run_recorded(work, action, coverage, args.dry_run)


def command_forge(args, config: PlatformConfig) -> int:
    if args.finalize_run is not None:
        report = prune_completed_run(args.finalize_run, keep_all=args.keep_all)
        print(json.dumps(report, indent=2))
        return 0
    if args.request is None:
        raise ForgeContractError(
            "forge requires a request manifest or --finalize-run"
        )
    context = selection.read_context(config)
    override = Path(context["backend"]) if context else None
    request, backend = validate_forge_request(args.request, backend_override=override)
    if args.validate_only or args.dry_run:
        print(json.dumps({
            "status": "PASS",
            "request_schema": request["schema"],
            "backend_id": backend["backend_id"],
            "vemu_forbidden": True,
        }, indent=2))
        return 0
    run_path = prepare_forge_run(args.request, config.run_root, args.run_dir,
                                 backend_override=override)
    if context:
        selection.save(run_path / "hardware-selection.json", context)
    print(run_path)
    return 0


def command_rtl_preflight(args) -> int:
    report = run_rtl_preflight(args.backend, args.rtl_root)
    rendered = json.dumps(report, indent=2) + "\n"
    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(rendered, encoding="utf-8")
    print(rendered, end="")
    return 0 if report["status"] == "PASS" else 1


def _parse_rtl_cases(values: list[str]) -> list[RtlCase]:
    cases: list[RtlCase] = []
    names: set[str] = set()
    for value in values:
        if "=" not in value:
            raise ValueError("--case must use NAME=L1_BIN")
        name, path = value.split("=", 1)
        if name in names:
            raise ValueError(f"duplicate RTL case name: {name}")
        names.add(name)
        cases.append(RtlCase(name=name, l1_bin=Path(path).expanduser()))
    return cases


def command_rtl(args, config: PlatformConfig) -> int:
    from .output_layout_contract import discover_layouts
    args.output_layouts = discover_layouts(args.dag_json, args.output_layouts)
    backend = resolve_backend_profile(config, args.backend)
    cases = _parse_rtl_cases(args.case)
    work = workspace(
        config, "rtl", "+".join(case.name for case in cases), "fresh",
        args.run_dir, backend,
    )
    work.add_input(backend.source)
    for path in (args.dag_json, args.output_layouts):
        if path is not None:
            work.add_input(path)
    if args.dag_json is not None:
        for name in ('output-validity.json', 'output-layouts.binding.json'):
            candidate = args.dag_json.parent / name
            if candidate.is_file():
                work.add_input(candidate)
    if args.gem5_output_dir is not None:
        for path in sorted(args.gem5_output_dir.glob('task_*_port_*.bin')):
            work.add_input(path)
    for case in cases:
        if case.l1_bin.exists():
            work.add_input(case.l1_bin)
    runner = Runner(work.commands, args.dry_run)
    adapter = RtlAdapter(runner, backend)

    def action() -> list[Path]:
        if args.dry_run:
            preflight_path = work.artifacts / "rtl-preflight.planned.json"
        else:
            preflight = run_rtl_preflight(backend.source)
            preflight_path = work.artifacts / "rtl-preflight.json"
            preflight_path.write_text(
                json.dumps(preflight, indent=2) + "\n", encoding="utf-8"
            )
            if preflight["status"] != "PASS":
                raise RtlPreflightError(
                    "RTL preflight did not pass; see "
                    f"{preflight_path}"
                )
        report = adapter.run(
            workspace=work.artifacts, cases=cases,
            timeout=config.timeouts.application_seconds,
            keep_build=args.keep_build,
            sim_horizon_us=args.sim_horizon_us,
            task_boundary_trace=args.task_boundary_trace,
            build_timeout=args.build_timeout, simulation_timeout=args.simulation_timeout,
            dag_json=args.dag_json, gem5_output_dir=args.gem5_output_dir, output_layouts=args.output_layouts,
        )
        return [preflight_path, report]

    return _run_recorded(
        work,
        action,
        "fresh_rtl_testbench_oracle",
        args.dry_run,
    )


def selected_config(args):
    """Explicit --config is an escape to legacy mode; never silently mix profiles."""
    active = PROJECT_ROOT / ".ace-echo/project.toml"
    hardware_commands = {"hardware", "doctor", "compile", "scheduler", "run", "rtl", "rtl-preflight", "forge"}
    if args.config is None and active.exists() and args.command in hardware_commands:
        config, context = selection.prepare(PROJECT_ROOT)
    else:
        config = load_config(args.config or DEFAULT_CONFIG)
        context = selection.read_context(config)
    if context:
        config = replace(config, run_root=Path(context["run_root"]))
        if hasattr(args, "backend"):
            if args.backend is None:
                args.backend = Path(context["backend"])
            profile = resolve_backend_profile(config, args.backend)
            selection.check_backend(context, profile.manifest)
        print(f"hardware={context['hardware']} toolchain={context['toolchain']} "
              f"backend={context['backend_id']}", file=sys.stderr)
        guarded = []
        if args.command == "run":
            guarded = ([args.l1_elf] if args.scope == "application" else
                       [args.dag_json, args.combined_bin, args.case_dir])
        elif args.command == "scheduler":
            guarded = [p for p in (args.dag_json, args.dag_bin) if p]
        elif args.command == "rtl":
            guarded = [case.l1_bin for case in _parse_rtl_cases(args.case)]
        for path in guarded:
            if not args.dry_run or path.exists():
                selection.check_artifact(path, context)
        if args.command == "rtl-preflight" and args.rtl_root is not None:
            expected = Path(selection.object_file(Path(context["backend"]))["paths"]["rtl_root"])
            if args.rtl_root.resolve() != expected.resolve():
                raise ConfigError("Edit host-paths.json to change the selected RTL root")
    if args.command in ("rtl", "rtl-preflight") and args.backend is None:
        raise ConfigError("--backend is required without a project hardware selection")
    return config, context


def main(argv: list[str] | None = None) -> int:
    args = parser().parse_args(argv)
    try:
        config, context = selected_config(args)
        if args.command == "hardware":
            if context is None:
                raise ConfigError("Create .ace-echo/project.toml and host-paths.json; see docs/HARDWARE_SELECTION.md")
            report = dict(context)
            report["resolved_backend"] = selection.object_file(Path(context["backend"]))
            print(json.dumps(report, indent=2))
            return 0
        if args.command == "doctor":
            backend = (
                resolve_backend_profile(config, args.backend)
                if args.backend else None
            )
            report = run_doctor(config, backend, scope=args.scope)
            if args.json:
                print(json.dumps(report, indent=2))
            else:
                for check in report["checks"]:
                    location = f" {check['path']}" if check["path"] else ""
                    print(f"{check['status']:<4} {check['name']}{location}")
                print(f"overall: {report['status']}")
            return 0 if report["status"] == "PASS" else 1
        if args.command == "compile":
            return command_compile(args, config)
        if args.command == "scheduler":
            return command_scheduler(args, config)
        if args.command == "run":
            return command_run(args, config)
        if args.command == "compare":
            if args.layout is not None:
                if args.dtype != 'raw':
                    raise ValueError('--layout requires --dtype raw')
                report = compare_binary_output(args.expected, args.actual, args.layout)
                print(write_dag_output_result(report, args.output), end='')
                return 0 if report['status'] == 'PASS' else 1
            result = compare_files(args.expected, args.actual, args.dtype)
            print(write_result(result, args.output), end="")
            return 0 if result.status == "PASS" else 1
        if args.command == "compare-dag-outputs":
            identity_contract = None
            if args.identity_backend is not None:
                identity_contract = json.loads(args.identity_backend.read_text())['engines']['rtl']['flow']['return_identity']
            report = compare_dag_outputs(
                args.expected_dma, args.actual_dir, args.trace_template, args.output_layouts,
                rtl_log=args.rtl_log, dag_json=args.dag_json, identity_contract=identity_contract
            )
            print(write_dag_output_result(report, args.output), end="")
            return 0 if report["status"] == "PASS" else 1
        if args.command == "compare-dag-dma":
            report = compare_dma_returns(args.expected_dma, args.actual_dma, args.output_layouts)
            print(write_dag_output_result(report, args.output), end="")
            return 0 if report["status"] == "PASS" else 1
        if args.command == "forge":
            return command_forge(args, config)
        if args.command == "rtl-preflight":
            return command_rtl_preflight(args)
        if args.command == "rtl":
            return command_rtl(args, config)
    except (ConfigError, ForgeContractError, RtlPreflightError,
            OSError, ValueError) as error:
        print(f"error: {error}", file=sys.stderr)
        return 2
    return 2


if __name__ == "__main__":
    raise SystemExit(main())
