from __future__ import annotations

from dataclasses import dataclass
import ast
from pathlib import Path

try:
    import tomllib  # type: ignore[attr-defined]
except ModuleNotFoundError:  # Python 3.8 on the current Venus host.
    tomllib = None


class ConfigError(ValueError):
    pass


def _fallback_toml(text: str) -> dict[str, object]:
    """Parse the deliberately small TOML subset used by local.toml.

    Keeping this reader local avoids adding a network-installed dependency to
    a platform that must run on the host's Python 3.8. It supports top-level
    keys, one-level tables, strings, integers, and booleans.
    """
    result: dict[str, object] = {}
    current = result
    for line_number, original in enumerate(text.splitlines(), 1):
        line = original.strip()
        if not line or line.startswith("#"):
            continue
        if line.startswith("[") and line.endswith("]"):
            name = line[1:-1].strip()
            if not name or "." in name:
                raise ConfigError(f"unsupported TOML table at line {line_number}")
            table: dict[str, object] = {}
            result[name] = table
            current = table
            continue
        if "=" not in line:
            raise ConfigError(f"invalid TOML assignment at line {line_number}")
        key, value_text = (part.strip() for part in line.split("=", 1))
        try:
            if value_text in ("true", "false"):
                value: object = value_text == "true"
            else:
                value = ast.literal_eval(value_text)
        except (SyntaxError, ValueError) as error:
            raise ConfigError(f"invalid TOML value at line {line_number}") from error
        current[key] = value
    return result


def _path(value: str, base: Path) -> Path:
    result = Path(value).expanduser()
    return (result if result.is_absolute() else base / result).resolve()


@dataclass(frozen=True)
class Projects:
    toolchain_root: Path
    workload_root: Path
    gem5_root: Path
    scheduler_root: Path


@dataclass(frozen=True)
class Compiler:
    venus_llvm_bin: Path
    riscv_gcc_root: Path
    scheduler_cross_prefix: str


@dataclass(frozen=True)
class Gem5:
    fast_binary: Path
    verification_binary: Path
    config_script: Path
    venus_config: str
    fast_mode: str
    verification_mode: str

    def binary(self, mode: str) -> Path:
        if mode == "fast":
            return self.fast_binary
        if mode == "verification":
            return self.verification_binary
        raise ConfigError(f"unsupported simulation mode: {mode}")

    def observation_mode(self, mode: str) -> str:
        if mode == "fast":
            return self.fast_mode
        if mode == "verification":
            return self.verification_mode
        raise ConfigError(f"unsupported simulation mode: {mode}")


@dataclass(frozen=True)
class Scheduler:
    default_target: str
    default_main: str


@dataclass(frozen=True)
class Timeouts:
    task_seconds: int
    dag_seconds: int
    application_seconds: int


@dataclass(frozen=True)
class PlatformConfig:
    source: Path
    run_root: Path
    projects: Projects
    compiler: Compiler
    gem5: Gem5
    scheduler: Scheduler
    timeouts: Timeouts


def load_config(path: Path) -> PlatformConfig:
    path = path.expanduser().resolve()
    try:
        text = path.read_text(encoding="utf-8")
        raw = tomllib.loads(text) if tomllib is not None else _fallback_toml(text)
    except OSError as error:
        raise ConfigError(f"cannot read config {path}: {error}") from error
    except Exception as error:
        if isinstance(error, ConfigError):
            raise
        raise ConfigError(f"invalid TOML in {path}: {error}") from error
    if raw.get("schema_version") != 1:
        raise ConfigError("config schema_version must be 1")
    try:
        project_raw = raw["projects"]
        gem5_raw = raw["gem5"]
        scheduler_raw = raw["scheduler"]
        timeout_raw = raw["timeouts"]
        projects = Projects(**{
            key: _path(project_raw[key], path.parent)
            for key in ("toolchain_root", "workload_root", "gem5_root", "scheduler_root")
        })
        compiler_raw = raw["compiler"]
        compiler = Compiler(
            venus_llvm_bin=_path(compiler_raw["venus_llvm_bin"], path.parent),
            riscv_gcc_root=_path(compiler_raw["riscv_gcc_root"], path.parent),
            scheduler_cross_prefix=str(compiler_raw["scheduler_cross_prefix"]),
        )
        gem5_root = projects.gem5_root
        gem5 = Gem5(
            fast_binary=_path(gem5_raw["fast_binary"], gem5_root),
            verification_binary=_path(gem5_raw["verification_binary"], gem5_root),
            config_script=_path(gem5_raw["config_script"], gem5_root),
            venus_config=str(gem5_raw["venus_config"]),
            fast_mode=str(gem5_raw.get("fast_mode", "fast")),
            verification_mode=str(gem5_raw.get("verification_mode", "verification")),
        )
        scheduler = Scheduler(
            default_target=str(scheduler_raw.get("default_target", "venus")),
            default_main=str(scheduler_raw.get("default_main", "src/main.c")),
        )
        timeouts = Timeouts(**{
            key: int(timeout_raw[key])
            for key in ("task_seconds", "dag_seconds", "application_seconds")
        })
    except (KeyError, TypeError, ValueError) as error:
        raise ConfigError(f"missing or invalid config field: {error}") from error
    if min(timeouts.task_seconds, timeouts.dag_seconds,
           timeouts.application_seconds) <= 0:
        raise ConfigError("all timeouts must be positive")
    return PlatformConfig(
        source=path,
        run_root=_path(str(raw.get("run_root", "../runs")), path.parent),
        projects=projects,
        compiler=compiler,
        gem5=gem5,
        scheduler=scheduler,
        timeouts=timeouts,
    )
