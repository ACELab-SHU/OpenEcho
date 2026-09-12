from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path
import re
from typing import Any

from .config import ConfigError, Gem5, PlatformConfig
from .forge import validate_backend_manifest


_MAKE_VARIABLE = re.compile(r"^[A-Za-z_][A-Za-z0-9_]*$")
_PROTECTED_MAKE_VARIABLES = {"TARGET_DAG", "LLVM_PATH", "RVPATH"}


def _resolve_path(value: str, base: Path) -> Path:
    path = Path(value).expanduser()
    return (path if path.is_absolute() else base / path).resolve()


@dataclass(frozen=True)
class BackendProfile:
    """Resolved execution settings from one authoritative backend manifest."""

    source: Path
    backend_id: str
    manifest: dict[str, Any]
    dsl_root: Path
    workload_root: Path
    gem5_root: Path
    scheduler_root: Path
    rtl_root: Path
    venus_llvm_bin: Path
    riscv_gcc_root: Path
    gem5: Gem5
    scheduler_build_target: str
    scheduler_linker_script: Path | None
    scheduler_configure_dcache_end: bool
    scheduler_enable_ctrl_iopads: bool
    scheduler_pll_helper_body: Path | None
    scheduler_devctrl_init_body: Path | None
    toolchain_make_variables: dict[str, str]
    clock_contract_id: str | None
    system_axi_hz: int | None
    tile_hz: int | None

    @property
    def components(self) -> dict[str, Path]:
        return {
            "toolchain": self.dsl_root,
            "workloads": self.workload_root,
            "gem5": self.gem5_root,
            "scheduler": self.scheduler_root,
            "rtl": self.rtl_root,
        }


def _profile_field(profile: dict[str, Any], key: str, where: str) -> str:
    value = profile.get(key)
    if not isinstance(value, str) or not value:
        raise ConfigError(f"{where}.{key} must be a non-empty string")
    return value


def resolve_backend_profile(config: PlatformConfig,
                            path: Path) -> BackendProfile:
    """Validate and resolve a backend without mutating local host config.

    ``local.toml`` remains the installation fallback.  Once ``--backend`` is
    supplied, architecture-sensitive compiler, Gem5, Scheduler and RTL values
    come from the selected manifest so a command cannot silently mix Venus
    generations.
    """
    requested = path.expanduser()
    aliases = {
        "v1": "venus1p0-64x512-300mhz.json",
        "venus1": "venus1p0-64x512-300mhz.json",
        "venus1.0": "venus1p0-64x512-300mhz.json",
        "venus1p0-64x512": "venus1p0-64x512-300mhz.json",
        "v1-300": "venus1p0-64x512-300mhz.json",
        "venus1-300": "venus1p0-64x512-300mhz.json",
        "venus1p0-64x512-300mhz": "venus1p0-64x512-300mhz.json",
        "v1-150": "venus1p0-64x512.json",
        "venus1-150": "venus1p0-64x512.json",
        "venus1p0-64x512-150mhz": "venus1p0-64x512.json",
        "v2": "venus2p0-16x128.json",
        "venus2": "venus2p0-16x128.json",
        "venus2.0": "venus2p0-16x128.json",
        "venus2p0-16x128": "venus2p0-16x128.json",
    }
    if not requested.exists() and str(requested).lower() in aliases:
        requested = (
            config.source.parent / "backends"
            / aliases[str(requested).lower()]
        )
    source = requested.resolve()
    manifest = validate_backend_manifest(source)
    base = source.parent
    paths = manifest["paths"]
    dsl_root = _resolve_path(paths["dsl_root"], base)
    workload_root = _resolve_path(paths["workload_root"], base)
    gem5_root = _resolve_path(paths["gem5_root"], base)
    scheduler_text = paths.get("scheduler_root")
    scheduler_root = (
        _resolve_path(scheduler_text, base)
        if isinstance(scheduler_text, str) and scheduler_text
        else config.projects.scheduler_root
    )
    rtl_root = _resolve_path(paths["rtl_root"], base)

    gem5_profiles = manifest["engines"]["gem5"]
    fast = gem5_profiles["fast"]
    verification = gem5_profiles["verification"]
    fast_config = _resolve_path(
        _profile_field(fast, "config", "backend.engines.gem5.fast"),
        gem5_root,
    )
    verification_config = _resolve_path(
        _profile_field(
            verification, "config", "backend.engines.gem5.verification"
        ),
        gem5_root,
    )
    if fast_config != verification_config:
        raise ConfigError(
            "backend fast and verification Gem5 profiles must use the same "
            "config script"
        )
    fast_venus_config = _profile_field(
        fast, "venus_config", "backend.engines.gem5.fast"
    )
    verification_venus_config = _profile_field(
        verification, "venus_config", "backend.engines.gem5.verification"
    )
    if fast_venus_config != verification_venus_config:
        raise ConfigError(
            "backend fast and verification Gem5 profiles must use the same "
            "Venus architecture profile"
        )
    gem5 = Gem5(
        fast_binary=_resolve_path(
            _profile_field(fast, "binary", "backend.engines.gem5.fast"),
            gem5_root,
        ),
        verification_binary=_resolve_path(
            _profile_field(
                verification, "binary", "backend.engines.gem5.verification"
            ),
            gem5_root,
        ),
        config_script=fast_config,
        venus_config=fast_venus_config,
        fast_mode=str(fast.get("sim_mode", config.gem5.fast_mode)),
        verification_mode=str(
            verification.get(
                "sim_mode", config.gem5.verification_mode
            )
        ),
    )

    architecture = manifest["architecture"]
    make_variables = {
        "VENUSROW": str(architecture["rows"]),
        "VENUSLANE": str(architecture["lanes"]),
    }
    toolchain = manifest["engines"].get("toolchain", {})
    venus_llvm_text = toolchain.get("venus_llvm_bin")
    venus_llvm_bin = (
        _resolve_path(venus_llvm_text, base)
        if isinstance(venus_llvm_text, str) and venus_llvm_text
        else config.compiler.venus_llvm_bin
    )
    riscv_gcc_text = toolchain.get("riscv_gcc_root")
    riscv_gcc_root = (
        _resolve_path(riscv_gcc_text, base)
        if isinstance(riscv_gcc_text, str) and riscv_gcc_text
        else config.compiler.riscv_gcc_root
    )
    declared_variables = toolchain.get("make_variables", {})
    if not isinstance(declared_variables, dict):
        raise ConfigError(
            "backend.engines.toolchain.make_variables must be an object"
        )
    for key, value in declared_variables.items():
        if not isinstance(key, str) or not _MAKE_VARIABLE.fullmatch(key):
            raise ConfigError(f"invalid backend toolchain make variable: {key!r}")
        if key in _PROTECTED_MAKE_VARIABLES:
            raise ConfigError(
                f"backend toolchain cannot override protected variable {key}"
            )
        if not isinstance(value, (str, int)) or isinstance(value, bool):
            raise ConfigError(
                f"backend toolchain make variable {key} must be a string or integer"
            )
        make_variables[key] = str(value)
    for key, expected in (
        ("VENUSROW", str(architecture["rows"])),
        ("VENUSLANE", str(architecture["lanes"])),
    ):
        if make_variables[key] != expected:
            raise ConfigError(
                f"backend toolchain {key}={make_variables[key]} conflicts "
                f"with architecture value {expected}"
            )

    scheduler = manifest["engines"]["scheduler"]
    scheduler_linker_text = scheduler.get("linker_script")
    scheduler_linker_script = (
        _resolve_path(scheduler_linker_text, base)
        if isinstance(scheduler_linker_text, str) and scheduler_linker_text
        else None
    )
    if (scheduler_linker_script is not None
            and not scheduler_linker_script.is_file()):
        raise ConfigError(
            "backend Scheduler linker script does not exist: "
            f"{scheduler_linker_script}"
        )
    scheduler_configure_dcache_end = scheduler.get(
        "configure_dcache_end", True
    )
    if not isinstance(scheduler_configure_dcache_end, bool):
        raise ConfigError(
            "backend.engines.scheduler.configure_dcache_end must be a "
            "boolean"
        )
    scheduler_enable_ctrl_iopads = scheduler.get(
        "enable_ctrl_iopads", False
    )
    if not isinstance(scheduler_enable_ctrl_iopads, bool):
        raise ConfigError(
            "backend.engines.scheduler.enable_ctrl_iopads must be a boolean"
        )
    pll_helper_body_text = scheduler.get("pll_helper_body")
    scheduler_pll_helper_body = (
        _resolve_path(pll_helper_body_text, base)
        if isinstance(pll_helper_body_text, str) and pll_helper_body_text
        else None
    )
    if (scheduler_pll_helper_body is not None
            and not scheduler_pll_helper_body.is_file()):
        raise ConfigError(
            "backend Scheduler PLL helper body does not exist: "
            f"{scheduler_pll_helper_body}"
        )
    devctrl_body_text = scheduler.get("devctrl_init_body")
    scheduler_devctrl_init_body = (
        _resolve_path(devctrl_body_text, base)
        if isinstance(devctrl_body_text, str) and devctrl_body_text
        else None
    )
    if (scheduler_devctrl_init_body is not None
            and not scheduler_devctrl_init_body.is_file()):
        raise ConfigError(
            "backend Scheduler devctrl body does not exist: "
            f"{scheduler_devctrl_init_body}"
        )
    clock_contract_id = None
    system_axi_hz = None
    tile_hz = None
    clock_contracts = manifest.get("clock_contracts")
    if isinstance(clock_contracts, dict):
        active_clock = clock_contracts.get("active")
        if isinstance(active_clock, str):
            active_contract = clock_contracts[active_clock]
            clock_contract_id = active_clock
            system_axi_hz = int(active_contract["system_axi_hz"])
            tile_hz = int(active_contract["tile_hz"])
    return BackendProfile(
        source=source,
        backend_id=str(manifest["backend_id"]),
        manifest=manifest,
        dsl_root=dsl_root,
        workload_root=workload_root,
        gem5_root=gem5_root,
        scheduler_root=scheduler_root,
        rtl_root=rtl_root,
        venus_llvm_bin=venus_llvm_bin,
        riscv_gcc_root=riscv_gcc_root,
        gem5=gem5,
        scheduler_build_target=str(scheduler["build_target"]),
        scheduler_linker_script=scheduler_linker_script,
        scheduler_configure_dcache_end=scheduler_configure_dcache_end,
        scheduler_enable_ctrl_iopads=scheduler_enable_ctrl_iopads,
        scheduler_pll_helper_body=scheduler_pll_helper_body,
        scheduler_devctrl_init_body=scheduler_devctrl_init_body,
        toolchain_make_variables=make_variables,
        clock_contract_id=clock_contract_id,
        system_axi_hz=system_axi_hz,
        tile_hz=tile_hz,
    )
