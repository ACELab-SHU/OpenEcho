from __future__ import annotations

from pathlib import Path
import json
import re
import shutil

from ..backend import BackendProfile
from ..config import PlatformConfig
from ..process import Runner
from .. import bas_parameters


class SchedulerAdapter:
    """Build Scheduler in an isolated run-local source copy.

    The upstream Makefile performs a clean and rewrites generated DAG C files.
    Running it in the user's working tree would damage unrelated state, so the
    platform always builds a private copy.
    """

    def __init__(self, config: PlatformConfig, runner: Runner,
                 backend: BackendProfile | None = None):
        self.config = config
        self.runner = runner
        self.backend = backend
        self.root = (
            backend.scheduler_root
            if backend is not None else config.projects.scheduler_root
        )

    def build(self, *, workspace: Path, target: str, main_src: str,
              dag_name: str | None, dag_json: Path | None,
              dag_bin: Path | None, timeout: int,
              task_input_type_bits: int | None = None,
              logical_type_map: dict[str, int] | None = None,
              unsupported_logical_types: list[int] | None = None,
              max_dag_outputs: int | None = None,
              output_descriptor_lag_ports: int | None = None,
              output_descriptor_transfer_chunk_bytes: int | None = None,
              completion_poll_fallback: bool | None = None,
              completion_poll_interval: int | None = None,
              task_container_spmd_fields: bool | None = None,
              allow_static_only_dag: bool = False,
              auto_static_main: bool = False,
              ) -> Path:
        if (dag_json is None) != (dag_bin is None):
            raise ValueError("--dag-json and --dag-bin must be supplied together")
        if dag_json and not dag_name:
            raise ValueError("--dag-name is required when staging a DAG")
        if dag_json and max_dag_outputs is not None:
            self._reject_excess_dag_outputs(dag_json, max_dag_outputs)
        if dag_json and unsupported_logical_types:
            self._reject_unsupported_logical_types(
                dag_json, set(unsupported_logical_types)
            )

        source = workspace / "scheduler-source"
        parameter_binding = bas_parameters.read_binding(dag_json, dag_bin) if dag_json and dag_bin else None
        if parameter_binding and parameter_binding["runtime"] and auto_static_main:
            raise ValueError("external runtime inputs require a matching L1 main, not --auto-static-main")
        if not self.runner.dry_run:
            shutil.copytree(
                self.root, source,
                ignore=shutil.ignore_patterns(
                    ".git", "*.o", "*.d", "l1.bin", "l1.elf", "l1.map",
                    "l1_objdump.txt", "l1_bin.txt", "echo.txt",
                ),
            )
            # The source generator also works in a standalone Scheduler copy.
            (source / "python").mkdir(exist_ok=True)
            shutil.copy2(Path(bas_parameters.__file__), source / "python/bas_parameters.py")
            if dag_json and dag_bin and dag_name:
                (source / "dags/json").mkdir(parents=True, exist_ok=True)
                (source / "dags/bin").mkdir(parents=True, exist_ok=True)
                shutil.copy2(dag_json, source / "dags/json" / f"{dag_name}.json")
                shutil.copy2(dag_bin, source / "dags/bin" / f"{dag_name}.bin")
            if any(value is not None for value in (
                    task_input_type_bits, logical_type_map,
                    output_descriptor_lag_ports,
                    output_descriptor_transfer_chunk_bytes,
                    completion_poll_fallback, completion_poll_interval,
                    task_container_spmd_fields)):
                self._configure_backend_copy(source)
            if task_container_spmd_fields is False:
                self._omit_task_container_spmd_fields(source)
            if allow_static_only_dag or auto_static_main:
                self._enable_static_only_dag_codegen(source)
            if (self.backend is not None
                    and self.backend.scheduler_linker_script is not None):
                shutil.copy2(
                    self.backend.scheduler_linker_script,
                    source / "l1.ld",
                )
            if (self.backend is not None
                    and not self.backend.scheduler_configure_dcache_end):
                self._disable_dcache_end_configuration(source)
            if (self.backend is not None
                    and self.backend.scheduler_enable_ctrl_iopads):
                self._enable_ctrl_iopads(source)
            if (self.backend is not None
                    and self.backend.scheduler_pll_helper_body is not None):
                self._install_pll_helper_body(
                    source, self.backend.scheduler_pll_helper_body
                )
            if (self.backend is not None
                    and self.backend.scheduler_devctrl_init_body is not None):
                self._install_devctrl_init_body(
                    source, self.backend.scheduler_devctrl_init_body
                )
            if auto_static_main:
                if not (dag_name and dag_json):
                    raise ValueError(
                        "auto static Scheduler main requires --dag-name and "
                        "--dag-json"
                    )
                staged_main = source / "src/ace_echo_auto_static_main.c"
                self._write_auto_static_main(
                    dag_name=dag_name,
                    dag_json=dag_json,
                    destination=staged_main,
                )
                main_src = str(staged_main.relative_to(source))
            else:
                main_path = Path(main_src).expanduser()
            if not auto_static_main and main_path.is_file():
                staged_main = source / "src" / "ace_echo_external_main.c"
                shutil.copy2(main_path.resolve(), staged_main)
                # Workload-owned launchers commonly keep generated golden
                # vectors beside the C file.  Preserve that self-contained
                # layout in the isolated Scheduler source tree without
                # copying arbitrary source or build products.
                for pattern in ("*.h", "*.inc"):
                    for auxiliary in sorted(main_path.parent.glob(pattern)):
                        shutil.copy2(auxiliary.resolve(),
                                     staged_main.parent / auxiliary.name)
                main_src = str(staged_main.relative_to(source))
            if parameter_binding and parameter_binding["runtime"]:
                self._stage_parameter_inputs(source, main_src, dag_json, parameter_binding)
        command = [
            "make", "-C", source,
            f"CROSS_COMPILE={self.config.compiler.scheduler_cross_prefix}",
            f"PYTHON={__import__('sys').executable}",
            f"MAIN_SRC={main_src}",
        ]
        if task_input_type_bits is not None:
            command.append(f"DAG_INPUT_TYPE_BITS={task_input_type_bits}")
        if logical_type_map is not None:
            encoded_map = ",".join(
                f"{key}:{logical_type_map[key]}"
                for key in sorted(logical_type_map, key=int)
            )
            command.append(f"DAG_INPUT_TYPE_MAP={encoded_map}")
        if output_descriptor_lag_ports is not None:
            if output_descriptor_lag_ports < 0:
                raise ValueError(
                    "output descriptor response lag must be non-negative"
                )
            command.append(
                "DAG_OUTPUT_DESCRIPTOR_LAG_PORTS="
                f"{output_descriptor_lag_ports}"
            )
        if output_descriptor_transfer_chunk_bytes is not None:
            chunk_bytes = output_descriptor_transfer_chunk_bytes
            if chunk_bytes < 0 or (chunk_bytes and chunk_bytes % 64):
                raise ValueError(
                    "output descriptor transfer chunk bytes must be zero or "
                    "a positive multiple of 64"
                )
            command.append(
                "DAG_OUTPUT_DESCRIPTOR_TRANSFER_CHUNK_BYTES="
                f"{chunk_bytes}"
            )
        if completion_poll_fallback is not None:
            command.append(
                "DAG_COMPLETION_POLL_FALLBACK="
                f"{int(completion_poll_fallback)}"
            )
        if completion_poll_interval is not None:
            if completion_poll_interval <= 0:
                raise ValueError(
                    "completion poll interval must be a positive integer"
                )
            command.append(
                f"DAG_COMPLETION_POLL_INTERVAL={completion_poll_interval}"
            )
        command.append(target)
        self.runner.run(
            "build-scheduler", command,
            cwd=workspace, timeout=timeout,
        )
        result = workspace / "scheduler-artifacts"
        if not self.runner.dry_run:
            result.mkdir(parents=True, exist_ok=True)
            for name in ("l1.elf", "l1.bin", "l1.map", "l1_objdump.txt"):
                candidate = source / name
                if candidate.is_file():
                    shutil.copy2(candidate, result / name)
            payload = result / "payload"
            payload.mkdir()
            for candidate in sorted((source / "dags/json").glob("*.json")):
                shutil.copy2(candidate, payload / candidate.name)
            for candidate in sorted((source / "dags/bin").glob("*.bin")):
                shutil.copy2(candidate, payload / candidate.name)
            if not (result / "l1.elf").is_file():
                raise FileNotFoundError("Scheduler build produced no l1.elf")
        return result

    @staticmethod
    def _stage_parameter_inputs(source: Path, main_src: str, dag_json: Path,
                                binding: dict) -> None:
        main = (source / main_src).resolve()
        if source.resolve() not in main.parents:
            raise ValueError("external input main must stay inside the isolated Scheduler copy")
        text = main.read_text()
        if not re.search(r'^\s*#\s*include\s+"ace_echo_inputs.inc"', text, re.M):
            raise ValueError("external runtime inputs require main to include ace_echo_inputs.inc; refusing unused inputs")
        header = (dag_json.parent / "ace_echo_inputs.inc").read_bytes()
        if bas_parameters.sha256(header) != binding["runtime_include_sha256"]:
            raise ValueError("external runtime input include digest mismatch")
        (main.parent / "ace_echo_inputs.inc").write_bytes(header)
        main.write_text("#define ACE_ECHO_EXTERNAL_INPUTS 1\n" + text)
        shutil.copy2(dag_json.parent / "parameters.json", source / "external-parameters.json")

    @staticmethod
    def _write_auto_static_main(
            *, dag_name: str, dag_json: Path, destination: Path) -> None:
        """Generate a generic launcher for a zero-runtime-input DAG.

        This launcher deliberately contains no workload oracle.  It fires the
        staged DAG, waits for every returned DMA, and exposes the standard
        GPIO completion markers.  Qualification remains an external full-DMA
        comparison, so the launcher is reusable and cannot hide bad outputs.
        """
        if re.fullmatch(r"[A-Za-z_][A-Za-z0-9_]*", dag_name) is None:
            raise ValueError(
                f"DAG name is not a valid Scheduler C identifier: {dag_name}"
            )
        document = json.loads(dag_json.read_text(encoding="utf-8"))
        if not isinstance(document, list):
            raise ValueError("DAG JSON must be a top-level list")

        dynamic_inputs = []
        for task_index, task in enumerate(document):
            if not isinstance(task, dict):
                continue
            for input_index, entry in enumerate(task.get("all_input") or []):
                raw_type = entry.get("type")
                try:
                    logical_type = (
                        int(raw_type, 0)
                        if isinstance(raw_type, str) else int(raw_type)
                    )
                except (TypeError, ValueError):
                    continue
                if logical_type in {2, 6}:
                    dynamic_inputs.append(
                        f"task[{task_index}].input[{input_index}]"
                    )
        if dynamic_inputs:
            raise ValueError(
                "auto static Scheduler main cannot supply runtime DAG "
                "inputs: " + ", ".join(dynamic_inputs[:8])
            )

        return_nodes = [
            item for item in document
            if isinstance(item, dict) and "return_output" in item
        ]
        if len(return_nodes) != 1:
            raise ValueError(
                "DAG JSON must contain exactly one top-level return_output "
                "record"
            )
        outputs = return_nodes[0]["return_output"]
        if outputs in (None, "None"):
            output_count = 0
        elif isinstance(outputs, list):
            output_count = len(outputs)
        else:
            raise ValueError("DAG return_output must be a list or None")

        declarations = "".join(
            f"static stdata_t ace_echo_output_{index};\n"
            for index in range(output_count)
        )
        initializers = "".join(
            f"  init_fifo(&ace_echo_output_{index}.fifo);\n"
            for index in range(output_count)
        )
        output_arguments = "".join(
            f", &ace_echo_output_{index}"
            for index in range(output_count)
        )
        source = f'''#include "common.h"
#include "config.h"
#include "venus.h"

#include "irq_instr.S"

/* Generated by ACE-ECHO for an externally checked, static-input DAG. */
{declarations}
__attribute__((optimize("O0"))) void main(void) {{
  printf("ACE-ECHO static DAG launch: {dag_name}\\n");
  REG_WRITE(0x1fff4000, 0x2);
  REG_WRITE(0x1fff4000, 0x0);

{initializers}  fire_dag({dag_name}, 0, {output_count}{output_arguments});
  fire_dag_fence();

  REG_WRITE(0x1fff4000, 0x4);
  REG_WRITE(0x1fff4000, 0x0);
  REG_WRITE(0x1fff4000, 0x8);
  REG_WRITE(0x1fff4000, 0x0);

  while (1) {{
  }}
}}
'''
        destination.parent.mkdir(parents=True, exist_ok=True)
        destination.write_text(source, encoding="utf-8")

    @staticmethod
    def _replace_once(path: Path, old: str, new: str, label: str) -> None:
        text = path.read_text(encoding="utf-8")
        if old not in text:
            raise ValueError(
                f"Scheduler {label} adapter no longer matches {path}; "
                "refusing an unqualified patch"
            )
        path.write_text(text.replace(old, new, 1), encoding="utf-8")

    @classmethod
    def _disable_dcache_end_configuration(cls, source: Path) -> None:
        """Apply a backend-qualified Scheduler CPU startup capability.

        Some Scheduler CPUs do not implement the ``menvcfgh`` cache-boundary
        CSR used by later generations.  The authoritative V1 smoke firmware
        omits this write, while firmware that executes it traps before GPIO0.
        Keep the shared Scheduler source unchanged and make the difference
        explicit in the selected backend manifest.
        """
        cls._replace_once(
            source / "src/kernel.c",
            "  change_dcache_end_addr(&_cacheram_end_addr);\n",
            "  /* Backend Scheduler CPU has no qualified dcache-end CSR. */\n",
            "dcache-end startup capability",
        )

    @classmethod
    def _install_devctrl_init_body(cls, source: Path, body: Path) -> None:
        """Install a manifest-owned SoC clock/reset sequence in a private copy."""
        path = source / "src/peripheral_init.c"
        text = path.read_text(encoding="utf-8")
        signature = (
            '__attribute__((optimize("O0"))) void devctrl_init(void) {'
        )
        start = text.find(signature)
        if start < 0:
            raise ValueError(
                "Scheduler devctrl adapter no longer matches "
                f"{path}; refusing an unqualified patch"
            )
        # devctrl_init is deliberately the final function in this source.
        if text[text.rfind("}") + 1:].strip():
            raise ValueError(
                f"Scheduler devctrl adapter expected {path} to end at "
                "devctrl_init"
            )
        replacement = signature + "\n" + body.read_text(
            encoding="utf-8"
        ).rstrip() + "\n}\n"
        path.write_text(text[:start] + replacement, encoding="utf-8")

    @classmethod
    def _install_pll_helper_body(cls, source: Path, body: Path) -> None:
        """Install the backend-qualified CCM command protocol privately."""
        path = source / "src/peripheral_init.c"
        text = path.read_text(encoding="utf-8")
        signature = (
            '__attribute__((optimize("O0"))) void '
            'GC0802_change_pll_settings(uint32_t base_addr, '
            'uint32_t offset_addr, uint32_t target_settings) {'
        )
        start = text.find(signature)
        if start < 0:
            raise ValueError(
                "Scheduler PLL helper adapter no longer matches "
                f"{path}; refusing an unqualified patch"
            )
        next_signature = (
            '__attribute__((optimize("O0"))) void devctrl_init(void) {'
        )
        end = text.find(next_signature, start)
        if end < 0:
            raise ValueError(
                f"Scheduler PLL helper adapter cannot find devctrl_init in {path}"
            )
        replacement = signature + "\n" + body.read_text(
            encoding="utf-8"
        ).rstrip() + "\n}\n\n"
        path.write_text(text[:start] + replacement + text[end:],
                        encoding="utf-8")

    @classmethod
    def _enable_ctrl_iopads(cls, source: Path) -> None:
        """Restore V1 Scheduler GPIO control pads in the private source."""
        path = source / "src/peripheral_init.c"
        replacements = []
        for index in range(4):
            old_line = (
                "  CONFIG_GC0802_IOPAD_REG("
                f"GC0802_IOPAD_CTRL_OUT{index} * 0x4, 0x030);"
            )
            new_line = (
                f"  CONFIG_GC0802_IOPAD_REG(0x{index * 4:02x}, 0x030);"
                f"  /* V1 CTRL_OUT{index} */"
            )
            replacements.append(
                ("  // " + old_line.lstrip() + "\n", new_line + "\n")
            )
        for index in range(8):
            old_line = (
                "  CONFIG_GC0802_IOPAD_REG("
                f"GC0802_IOPAD_CTRL_IN{index} * 0x4, 0x039);"
            )
            offset = 0x10 + index * 4
            new_line = (
                f"  CONFIG_GC0802_IOPAD_REG(0x{offset:02x}, 0x039);"
                f"  /* V1 CTRL_IN{index} */"
            )
            replacements.append(
                ("  // " + old_line.lstrip() + "\n", new_line + "\n")
            )
        for old, new in replacements:
            cls._replace_once(path, old, new, "V1 control IOPAD capability")

    @classmethod
    def _omit_task_container_spmd_fields(cls, source: Path) -> None:
        """Match Scheduler task-container packing to pre-SPMD RTL.

        Venus1.0 places ``task_input_num`` immediately after the hardware
        requirement.  Later RTL generations inserted the one-bit SPMD flag
        and six-bit minimum-core count between those fields.  Apply this only
        to the run-local Scheduler parser selected by the backend manifest;
        the shared Scheduler source remains unchanged.
        """
        parser = source / "dags/read_dag_json.py"
        cls._replace_once(
            parser,
            """          hardware_info +
          is_spmd +
          min_core_num +
          inputnum +
""",
            """          hardware_info +
          inputnum +
""",
            "pre-SPMD task-container layout",
        )

    @classmethod
    def _configure_backend_copy(cls, source: Path) -> None:
        """Apply the qualified Venus backend overlay to a private copy.

        The overlay is the source form validated by the PBCH/DLSCH RTL runs.
        It deliberately leaves ``components/scheduler`` immutable and refuses
        to guess when the copied upstream source no longer matches.
        """
        parser = source / "dags/read_dag_json.py"
        cls._replace_once(
            parser,
            "TASK_INPUT_TYPE_BIT_CHOICES = (2, 3)\n\n",
            """TASK_INPUT_TYPE_BIT_CHOICES = (2, 3)

DEFAULT_LOGICAL_TO_PHYSICAL_TYPE = {
  2: {
    TYPE_TEMP: 0b00,
    TYPE_PARAM_GLOBAL: 0b01,
    TYPE_DAG_DFE: 0b01,
    TYPE_RESERVED: 0b11,
    TYPE_PTR_TEMP: 0b00,
    TYPE_PTR_PARAM_GLOBAL: 0b01,
    TYPE_PTR_DAG_DFE: 0b01,
  },
  3: {
    TYPE_TEMP: TYPE_TEMP,
    TYPE_PARAM_GLOBAL: TYPE_PARAM_GLOBAL,
    TYPE_DAG_DFE: TYPE_DAG_DFE,
    TYPE_RESERVED: TYPE_RESERVED,
    TYPE_PTR_TEMP: TYPE_PTR_TEMP,
    TYPE_PTR_PARAM_GLOBAL: TYPE_PTR_PARAM_GLOBAL,
    TYPE_PTR_DAG_DFE: TYPE_PTR_PARAM_GLOBAL,
  },
}

""",
            "logical input type map",
        )
        cls._replace_once(
            parser,
            """INDEX_INPUT_TYPES = {
  TYPE_PARAM_GLOBAL,
  TYPE_DAG_DFE,
  TYPE_PTR_PARAM_GLOBAL,
  TYPE_PTR_DAG_DFE,
}

""",
            """INDEX_INPUT_TYPES = {
  TYPE_PARAM_GLOBAL,
  TYPE_DAG_DFE,
  TYPE_PTR_PARAM_GLOBAL,
  TYPE_PTR_DAG_DFE,
}


def descriptor_transfer_length(item):
  \"\"\"Use a slice's physical payload, not its unsliced source extent.\"\"\"
  slice_length = item.get(\"slice_length\")
  if slice_length is not None:
    value = int(slice_length, 0) if isinstance(slice_length, str) else int(slice_length)
    if value > 0:
      return value
  length = item.get(\"length\")
  return int(length, 0) if isinstance(length, str) else int(length)

""",
            "physical slice transfer length",
        )
        cls._replace_once(
            parser,
            """  return task_input_type_bits

class DAG_parser(object):
  def __init__(self, dag_json_file, task_input_type_bits=DEFAULT_TASK_INPUT_TYPE_BITS):
    task_input_type_bits = _validate_task_input_type_bits(task_input_type_bits)
""",
            """  return task_input_type_bits


def _parse_logical_type_map(text, task_input_type_bits):
  if text is None:
    return dict(DEFAULT_LOGICAL_TO_PHYSICAL_TYPE[task_input_type_bits])
  mapping = {}
  for item in text.split(','):
    fields = item.split(':', 1)
    if len(fields) != 2:
      raise ValueError(
        \"logical type map must use comma-separated LOGICAL:PHYSICAL entries\"
      )
    logical = int(fields[0], 0)
    physical = int(fields[1], 0)
    if logical < 0 or logical > TYPE_RETURN_VALUE:
      raise ValueError(f\"logical input type {logical} is outside [0, 7]\")
    if physical < 0 or physical >= (1 << task_input_type_bits):
      raise ValueError(
        f\"physical input type {physical} does not fit in \"
        f\"{task_input_type_bits} bits\"
      )
    mapping[logical] = physical
  return mapping

class DAG_parser(object):
  def __init__(self, dag_json_file, task_input_type_bits=DEFAULT_TASK_INPUT_TYPE_BITS,
               logical_type_map=None, output_descriptor_lag_ports=0):
    task_input_type_bits = _validate_task_input_type_bits(task_input_type_bits)
    logical_type_map = _parse_logical_type_map(
      logical_type_map,
      task_input_type_bits
    )
    if output_descriptor_lag_ports < 0:
      raise ValueError(\"output descriptor lag must be non-negative\")
    self.output_descriptor_lag_ports = output_descriptor_lag_ports
""",
            "parser backend arguments",
        )
        cls._replace_once(
            parser,
            "    self.dag_json_file = dag_json_file\n",
            "    self.dag_json_file = dag_json_file\n    self.logical_type_map = logical_type_map\n",
            "logical map state",
        )
        cls._replace_once(
            parser,
            """    if input_type_value < 0 or input_type_value >= (1 << type_bits):
      raise ValueError(
        f\"{self.__field_location(task_idx, input_idx)} type={input_type} \"
        f\"does not fit in {type_bits} bits; use --task-input-type-bits 3 \"
        f\"for pointer type descriptors\"
      )
    return input_type_value, bin(input_type_value)[2:].zfill(type_bits)
""",
            """    if input_type_value not in self.logical_type_map:
      raise ValueError(
        f\"{self.__field_location(task_idx, input_idx)} logical type=\"
        f\"{input_type_value} has no physical mapping for {type_bits}-bit RTL\"
      )
    physical_type_value = self.logical_type_map[input_type_value]
    return (
      input_type_value,
      bin(physical_type_value)[2:].zfill(type_bits)
    )
""",
            "logical-to-physical encoding",
        )
        cls._replace_once(
            parser,
            """            if input_type_bin == \"110\":
              one_task_descriptor = \"101\" + port + desaddr
            else:
              one_task_descriptor = input_type_bin + port + desaddr
""",
            "            one_task_descriptor = input_type_bin + port + desaddr\n",
            "descriptor type encoding",
        )
        cls._replace_once(
            parser,
            """              length = all_inputs_item.get(\"length\")
              length_int = self.__parse_int_field(length, \"length\", self.task_num, i)
              length = self.__format_binary_field(
                length,
""",
            """              length_int = descriptor_transfer_length(all_inputs_item)
              length = self.__format_binary_field(
                length_int,
""",
            "descriptor physical length",
        )
        cls._replace_once(
            parser,
            """              else:
                  out = all_output[out_idx]
                  temp_offset = int(out[\"temp_offset\"])
""",
            """              else:
                  physical_idx = min(
                    out_idx + self.output_descriptor_lag_ports,
                    len(all_output) - 1
                  )
                  out = all_output[physical_idx]
                  temp_offset = int(out[\"temp_offset\"])
""",
            "output descriptor response lag",
        )
        cls._replace_once(
            parser,
            """    help=\"Task input typeid width. Use 3 for pointer-aware DSL JSON, or 2 for legacy JSON descriptors.\",
  )
  parser.add_argument(
    \"--legacy-task-input-type-bits\",
""",
            """    help=\"Physical task-input type width declared by the target RTL.\",
  )
  parser.add_argument(
    \"--logical-type-map\",
    help=(
      \"Optional comma-separated logical:physical mapping, for example \"
      \"0:0,1:1,2:1,4:0,5:1,6:1\"
    ),
  )
  parser.add_argument(
    \"--legacy-task-input-type-bits\",
""",
            "logical map CLI",
        )
        cls._replace_once(
            parser,
            """  parser.add_argument(\"dag_json_files\", nargs=\"+\")
  args = parser.parse_args(sys.argv[1:])

  for arg in args.dag_json_files:
    dag_parser = DAG_parser(arg, task_input_type_bits=args.task_input_type_bits)
""",
            """  parser.add_argument(
    \"--output-descriptor-lag-ports\",
    type=int,
    default=0,
    help=\"Pre-advance output descriptors for a backend response pipeline.\",
  )
  parser.add_argument(\"dag_json_files\", nargs=\"+\")
  args = parser.parse_args(sys.argv[1:])

  for arg in args.dag_json_files:
    dag_parser = DAG_parser(
      arg,
      task_input_type_bits=args.task_input_type_bits,
      logical_type_map=args.logical_type_map,
      output_descriptor_lag_ports=args.output_descriptor_lag_ports
    )
""",
            "output lag CLI",
        )

        dags_make = source / "dags/Makefile"
        cls._replace_once(
            dags_make,
            """DAG_INPUT_TYPE_BITS ?= 3

.PHONY: create_cbin
""",
            """DAG_INPUT_TYPE_BITS ?= 2
DAG_INPUT_TYPE_MAP ?=
DAG_OUTPUT_DESCRIPTOR_LAG_PORTS ?= 0

.PHONY: create_cbin
""",
            "DAG Make variables",
        )
        cls._replace_once(
            dags_make,
            "\t$(PYTHON) read_dag_json.py --task-input-type-bits $(DAG_INPUT_TYPE_BITS) $(JSON_FILES)\n",
            """\t$(PYTHON) read_dag_json.py --task-input-type-bits $(DAG_INPUT_TYPE_BITS) \\
\t\t$(if $(DAG_INPUT_TYPE_MAP),--logical-type-map $(DAG_INPUT_TYPE_MAP),) \\
\t\t--output-descriptor-lag-ports $(DAG_OUTPUT_DESCRIPTOR_LAG_PORTS) \\
\t\t$(JSON_FILES)
""",
            "DAG parser invocation",
        )

        top_make = source / "Makefile"
        cls._replace_once(
            top_make,
            "CFLAGS += -I$(BSP_DIR)/include -I$(ADC_DIR)\n",
            """CFLAGS += -I$(BSP_DIR)/include -I$(ADC_DIR)
DAG_OUTPUT_DESCRIPTOR_TRANSFER_CHUNK_BYTES ?= 64
CFLAGS += -DDAG_OUTPUT_DESCRIPTOR_TRANSFER_CHUNK_BYTES=$(DAG_OUTPUT_DESCRIPTOR_TRANSFER_CHUNK_BYTES)
DAG_COMPLETION_POLL_FALLBACK ?= 0
DAG_COMPLETION_POLL_INTERVAL ?= 4096
CFLAGS += -DDAG_COMPLETION_POLL_FALLBACK=$(DAG_COMPLETION_POLL_FALLBACK)
CFLAGS += -DDAG_COMPLETION_POLL_INTERVAL=$(DAG_COMPLETION_POLL_INTERVAL)
""",
            "Scheduler build variables",
        )

        dagfire = source / "include/dagfire.h"
        cls._replace_once(
            dagfire,
            "extern int dma_error_hpn;\n\n",
            """extern int dma_error_hpn;

#ifndef DAG_OUTPUT_DESCRIPTOR_TRANSFER_CHUNK_BYTES
#define DAG_OUTPUT_DESCRIPTOR_TRANSFER_CHUNK_BYTES 64
#endif

#if DAG_OUTPUT_DESCRIPTOR_TRANSFER_CHUNK_BYTES == 0
#define transfer_dag_output_descriptors(DAGname, DagProc)                    \\
  dma_transfer((uint32_t)(&CONCAT(DAGname, output_addr)),                    \\
               (uint32_t)(DagProc)->config.outputaddr,                       \\
               (uint32_t)(CONCAT(DAGname, output_addr_size)), 0)
#else
#define transfer_dag_output_descriptors(DAGname, DagProc)                    \\
  do {                                                                        \\
    for (uint32_t __output_addr_transfer_index__ = 0;                         \\
         __output_addr_transfer_index__ <                                     \\
             (uint32_t)(CONCAT(DAGname, output_addr_size));                   \\
         __output_addr_transfer_index__ +=                                    \\
             DAG_OUTPUT_DESCRIPTOR_TRANSFER_CHUNK_BYTES) {                    \\
      uint32_t __output_addr_remaining__ =                                    \\
          (uint32_t)(CONCAT(DAGname, output_addr_size)) -                     \\
          __output_addr_transfer_index__;                                     \\
      uint32_t __output_addr_transfer_size__ =                                \\
          __output_addr_remaining__ > DAG_OUTPUT_DESCRIPTOR_TRANSFER_CHUNK_BYTES \\
              ? DAG_OUTPUT_DESCRIPTOR_TRANSFER_CHUNK_BYTES                    \\
              : __output_addr_remaining__;                                    \\
      dma_transfer(                                                           \\
          (uint32_t)(&CONCAT(DAGname, output_addr)) +                         \\
              __output_addr_transfer_index__,                                 \\
          (uint32_t)(DagProc)->config.outputaddr +                            \\
              __output_addr_transfer_index__,                                 \\
          __output_addr_transfer_size__, 0);                                  \\
    }                                                                         \\
  } while (0)
#endif

""",
            "descriptor DMA helper",
        )
        start = "  for(uint32_t __output_addr_size_terans_index__ = 0;"
        end = "  transfer_dag_inputs(p, &CONCAT(DAGname, bin),"
        text = dagfire.read_text(encoding="utf-8")
        begin = text.find(start)
        finish = text.find(end, begin)
        if begin < 0 or finish < 0:
            raise ValueError(
                "Scheduler descriptor DMA adapter no longer matches "
                f"{dagfire}; refusing an unqualified patch"
            )
        text = text[:begin] + (
            "  transfer_dag_output_descriptors(DAGname, p);"
            "                                                                 "
            "                                 \\\n"
        ) + text[finish:]
        dagfire.write_text(text, encoding="utf-8")

        cls._adapt_completion_polling(source)

    @classmethod
    def _adapt_completion_polling(cls, source: Path) -> None:
        proven = source / "src/cluster.c"
        text = proven.read_text(encoding="utf-8")
        function_start = text.index("void cluster_interrupt_handler")
        irq_start = text.index("  if (cluster_intstatusreg &", function_start)
        irq_else = text.index("  } else if (cluster_intstatusreg &", irq_start)
        prefix = text[:function_start]
        helper = """int cluster_outputs_ready(uint32_t cluster_id) {
  cluster_t* c = &clusters[cluster_id];
  dagproc_t* p = &(c->dagproc[0]);
  if (p->state != RUN || p->outputnum <= 0) return 0;
  for (int i = 0; i < p->outputnum; i++) {
    uint32_t ret_addr = READ_BURST_32(VENUS_CLUSTER_L2_CFG(cluster_id),
        VENUS_CLUSTER_L2_DAG_RETURN_ADDR_OFFSET(i));
    uint32_t ret_len = READ_BURST_32(VENUS_CLUSTER_L2_CFG(cluster_id),
        VENUS_CLUSTER_L2_DAG_RETURN_LENGTH_OFFSET(i));
    if ((ret_addr | ret_len) == 0) return 0;
  }
  return 1;
}

int cluster_collect_outputs(uint32_t cluster_id) {
  cluster_t* c = &clusters[cluster_id];
  dagproc_t* p = &(c->dagproc[0]);
  if (p->state != RUN) return 0;
  WRITE_BURST_32(VENUS_CLUSTER_L2_CFG(cluster_id),
                 VENUS_CLUSTER_L2_RESET_REG_ADDR, 0);
  p->state = C2D;
  int outputnum = p->outputnum;
  int cluster_offset = p->config.prog;
  dmadsc_t dmadsc;
  dmadsc.ptr = (uint32_t)p;
  dmadsc.atr = DAGPROC_PSEUDO_DSC;
  dmapush_fifo(&dmafifo, dmadsc);
  for (int i = 0; i < outputnum; i++) {
    uint32_t ret_addr = READ_BURST_32(VENUS_CLUSTER_L2_CFG(cluster_id),
        VENUS_CLUSTER_L2_DAG_RETURN_ADDR_OFFSET(i));
    uint32_t ret_len = READ_BURST_32(VENUS_CLUSTER_L2_CFG(cluster_id),
        VENUS_CLUSTER_L2_DAG_RETURN_LENGTH_OFFSET(i));
    uint32_t ret_len_aligned = _align_up(ret_len, 64);
    uint32_t* malloc_ptr = malloc(ret_len_aligned);
    stdata_t* stdata = (stdata_t*)p->outputlist[i];
    stdata->atr = (int)malloc_ptr;
    dma_transfer((ret_addr + cluster_offset), (int)malloc_ptr, ret_len,
                 i == outputnum - 1);
  }
  return 1;
}

"""
        irq_header_end = text.index(" {", irq_start) + 2
        new_irq = text[function_start:irq_start] + (
            text[irq_start:irq_header_end]
            + "\n    cluster_collect_outputs(cluster_id);\n"
        ) + text[irq_else:]
        proven.write_text(prefix + helper + new_irq, encoding="utf-8")

        api = source / "src/api.c"
        cls._replace_once(
            api,
            '#include "common.h"\n',
            '#include "common.h"\n#include "dagproc.h"\n',
            "completion polling include",
        )
        cls._replace_once(
            api,
            '#include "venus.h"\n\n',
            """#include \"venus.h\"

#ifndef DAG_COMPLETION_POLL_FALLBACK
#define DAG_COMPLETION_POLL_FALLBACK 0
#endif
#ifndef DAG_COMPLETION_POLL_INTERVAL
#define DAG_COMPLETION_POLL_INTERVAL 4096
#endif
extern dagproc_t* p;
extern int cluster_outputs_ready(uint32_t cluster_id);
extern int cluster_collect_outputs(uint32_t cluster_id);

""",
            "completion polling declarations",
        )
        cls._replace_once(
            api,
            """__attribute__((optimize(\"O0\"))) void fire_dag_fence(void) {
  while (!MUTEX_dag_done) {}
}""",
            """__attribute__((optimize(\"O0\"))) void fire_dag_fence(void) {
  unsigned int poll_counter = 0;
  while (!MUTEX_dag_done) {
#if DAG_COMPLETION_POLL_FALLBACK
    poll_counter++;
    if (poll_counter >= DAG_COMPLETION_POLL_INTERVAL) {
      poll_counter = 0;
      if (p != 0 && cluster_outputs_ready(0)) {
        cluster_collect_outputs(0);
      }
    }
#endif
  }
}""",
            "completion polling fence",
        )

    @staticmethod
    def _enable_static_only_dag_codegen(source: Path) -> None:
        """Patch only the isolated build copy to emit legal zero-input C.

        The hardware descriptors and Scheduler runtime are untouched.  The
        generated placeholder arrays are never dereferenced because workload
        launchers pass DAGInputNum=0.
        """
        parser = source / "dags/read_dag_json.py"
        text = parser.read_text(encoding="utf-8")
        if "input_array_size = max(1, self.l1_input_num)" in text:
            return
        replacements = {
            'f"unsigned int {self.dag_name}_input_offset[{self.l1_input_num}] = {{" + ", ".join(map(str, self.input_offset)) + "};\\n"':
            'f"unsigned int {self.dag_name}_input_offset[{max(1, self.l1_input_num)}] = {{" + ", ".join(map(str, self.input_offset or [0])) + "};\\n"',
            'f"unsigned int {self.dag_name}_input_length[{self.l1_input_num}] = {{" + ", ".join(map(str, self.input_length)) + "};\\n"':
            'f"unsigned int {self.dag_name}_input_length[{max(1, self.l1_input_num)}] = {{" + ", ".join(map(str, self.input_length or [0])) + "};\\n"',
            "      if self.l1_input_num == 0:\n        raise ValueError(f'The DAG \"{self.dag_name}\" has no input data!')\n":
            "      # ACE-ECHO isolated-build adapter: zero runtime inputs are legal.\n",
        }
        for old, new in replacements.items():
            if old not in text:
                raise ValueError(
                    "Scheduler static-only adapter no longer matches "
                    f"{parser}; refusing an unqualified patch"
                )
            text = text.replace(old, new, 1)
        parser.write_text(text, encoding="utf-8")

    @staticmethod
    def _reject_excess_dag_outputs(
            dag_json: Path, max_dag_outputs: int) -> None:
        if max_dag_outputs <= 0:
            raise ValueError("max_dag_outputs must be a positive integer")
        document = json.loads(dag_json.read_text(encoding="utf-8"))
        if not isinstance(document, list):
            raise ValueError("DAG JSON must be a top-level list")
        return_nodes = [
            item for item in document
            if isinstance(item, dict) and "return_output" in item
        ]
        if len(return_nodes) != 1:
            raise ValueError(
                "DAG JSON must contain exactly one top-level return_output "
                "record"
            )
        outputs = return_nodes[0]["return_output"]
        if outputs in (None, "None"):
            output_count = 0
        elif isinstance(outputs, list):
            output_count = len(outputs)
        else:
            raise ValueError("DAG return_output must be a list or None")
        if output_count > max_dag_outputs:
            raise ValueError(
                f"backend RTL supports at most {max_dag_outputs} "
                f"hardware-visible DAG outputs, but the staged DAG declares "
                f"{output_count}; pack or reduce the top-level return ports "
                "without changing internal task-output semantics"
            )

    @staticmethod
    def _reject_unsupported_logical_types(
            dag_json: Path, unsupported: set[int]) -> None:
        tasks = json.loads(dag_json.read_text(encoding="utf-8"))
        violations = []
        for task_index, task in enumerate(tasks):
            for input_index, entry in enumerate(task.get("all_input") or []):
                raw_type = entry.get("type")
                try:
                    logical_type = int(raw_type, 0) if isinstance(
                        raw_type, str
                    ) else int(raw_type)
                except (TypeError, ValueError):
                    continue
                if logical_type in unsupported:
                    task_name = task.get("debug_task_name", task_index)
                    input_name = entry.get("name", input_index)
                    violations.append(
                        f"{task_name}.{input_name}=logical_type_{logical_type}"
                    )
        if violations:
            raise ValueError(
                "backend RTL cannot preserve these task-input semantics: "
                + ", ".join(violations)
                + "; use a backend-native value interface or a backend that "
                  "explicitly supports those logical types"
            )
