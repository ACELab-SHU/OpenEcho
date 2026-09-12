# Get started with ACE-ECHO 1.0

## Public source distribution

```bash
git clone https://github.com/ACELab-SHU/ACE-Echo.git
cd ACE-Echo/platform
```

DSL and selected workloads are included as revision-pinned source snapshots. No submodule initialization or private Git access is required. See `PUBLIC_SOURCE_MANIFEST.json` for provenance and [the release notes](releases/1.0.md) for validation scope.

## Requirements

- Linux with Python 3.8+, Git, GNU Make, C/C++ build tools, Python development
  headers, SCons and zlib development headers. The vendored Gem5 build reports
  its supported compiler versions (GCC 11 through 14.2 in this snapshot).
  Additional optional dependencies are diagnosed by SCons. No root-level
  package installation is performed automatically.
- An authorized **Venus-custom LLVM** installation for the chosen backend,
  including `clang`, `opt`, `llc`, `llvm-objcopy`, `llvm-objdump`. Stock LLVM
  cannot replace the custom `zvenus` target and passes. Obtain the compiler
  build/revision from the platform owner; it is not distributable by these
  two repos alone. V1's recorded compiler identity is LLVM 15.0.7, source
  `27fdd688584d87b64ab8a565bbfb1e0f16406bd4`.
- A RISC-V bare-metal GCC toolchain with `bin/riscv32-unknown-elf-gcc` and its
  matching headers/libraries, plus host `riscv64-linux-gnu-objcopy` and
  `riscv64-linux-gnu-ld` for Gem5 replay packaging.
- An AI coding host capable of reading repository skills and running these
  tools. Its model access/login is separate from the repository.

**Not required for fast onboarding:** VEMU, an RTL checkout, VCS/VIP licenses,
Scheduler cross GCC, Gem5 debug, Damoda/OAI source checkouts, or historical runs.
VEMU is never used by this workflow.

## Explicit setup

Create `.ace-echo/`, copy `configs/host.example.json` to
`.ace-echo/host-tools.json`, and set your two approved compiler directories.
Do not edit the shared backend's rows, lanes, clock or ABI to fit a machine.
Tool directory names must be free of whitespace/shell metacharacters because
the legacy DSL's Make interface cannot quote them safely.

```bash
# Explicit network operation, pinned nlohmann/json v3.11.3 commit.
python3 scripts/bootstrap.py fetch-json

python3 scripts/bootstrap.py configure \
  --tools .ace-echo/host-tools.json \
  --backend configs/backends/venus1p0-64x512-300mhz.json

# Build this checkout's owned Gem5 source; no historic binary or build cache.
python3 scripts/bootstrap.py build-gem5 --jobs 4 --mode opt

./ace-echo --config .ace-echo/host/local.toml doctor \
  --backend .ace-echo/host/backend.json --scope fast --json
make test
make smoke
```

`configure` generates a host-specific backend and TOML plus a compiler-hash
receipt in `.ace-echo/host/`. It stages a copy of the pinned DSL with the pinned
JSON headers, avoiding the DSL's unpinned implicit download. It changes only
installation paths and the equivalent GCC search path flag; architecture,
numeric rules and correctness gates remain unchanged. The bundled DSL is
not edited. An existing host directory is rejected rather than overwritten;
use `--output .ace-echo/host-next` for another setup and pass its config/backend
explicitly to subsequent commands. Generated config is checkout-specific:
regenerate it after moving or cloning the repository.

Build logs and a source-tree/binary receipt are under `.ace-echo/`. The first
source build is substantially more expensive than a tiny DAG simulation.
The binary is intentionally ignored by Git. A future distributed binary
package must carry its source tree, build options and SHA-256; silently copying
an old `gem5.opt` is not a clean-checkout validation. To install an explicitly
supplied build from a trusted maintainer instead of rebuilding on every host:

```bash
python3 scripts/bootstrap.py install-gem5 --mode opt \
  --binary /path/to/new-build/gem5.opt --receipt /path/to/build-gem5-opt.json
```

The installer rejects dirty source builds, source-tree mismatch, wrong mode,
checksum mismatch and overwriting an existing binary. A receipt binds identity,
not trust: obtain both files through an approved channel. This does not solve
host shared-library/OS compatibility; the subsequent doctor and actual smoke
must still pass. The reference build receipt is produced by `build-gem5`.

The default smoke output is `runs/onboarding-smoke-a001`; it refuses to reuse
an existing directory. Repeat using:

```bash
python3 scripts/smoke.py --output runs/onboarding-smoke-a002
```

PASS means three freshly compiled cases, two connected tasks, all outputs
bit-exact against the provisional host model, and clock-derived cycle records.
It does **not** mean an approved application reference, full firmware execution,
or software/RTL qualification. A missing tool or failed command leaves logs
and failure evidence rather than a success placeholder.

## Use the Forge agent for a new application

Open the **platform root** in your AI host and ask it to read `AGENTS.md` and
`.agents/skills/ace-echo-venus-forge/SKILL.md` plus required references. Example:

> Use $ace-echo-venus-forge. Develop my new application in workloads/ using
> .ace-echo/host/backend.json and .ace-echo/host/local.toml. First freeze input,
> integer/ABI and all-output semantics. Use Gem5 fast for exploration, preserve
> inputs/outputs/cycles/config/conclusions, and report software-only scope.
> Do not invoke VEMU, use old runs as hidden dependencies, alter shared
> simulation semantics, or claim RTL qualification without running its gates.

`./ace-echo forge request.json --validate-only` validates a request;
`./ace-echo forge request.json` initializes a run. **Neither command itself
calls an LLM or autonomously executes the whole development/search loop.**
The AI host follows the skill and invokes compile/run/compare commands.
After setup, the self-contained request can also exercise the Forge entrance:

```bash
./ace-echo --config .ace-echo/host/local.toml forge \
  examples/forge-vector-smoke.example.json --validate-only
./ace-echo --config .ace-echo/host/local.toml forge \
  examples/forge-vector-smoke.example.json
```

Its golden gate remains `REVIEW_REQUIRED`; initializing a request must not
silently promote the provisional mathematical model to an approved golden.
Use the small `forge_vector_smoke` as the self-contained starting example;
the OAI request template requires external source and is not an install test.

## Fast validation, RTL and debug diagnosis

The shipped backends use this normal route: independent compile -> Gem5 fast
over the complete declared case matrix -> approved golden/all-required-output
comparison -> integrated revalidation -> fresh RTL -> software/RTL comparison.
`run task`, `run dag` and `run application` default to `--mode fast`.
Debug is not an extra prerequisite for this route. `full_software_engine`
selects the full-matrix executor, not reduced correctness coverage. Custom
backends selecting `gem5.verification` still require that engine.

Use `doctor --scope software` for the selected software policy and
`doctor --scope full` for additional Scheduler/RTL paths. Neither is proof of
numerical correctness. RTL license preflight, firmware/input identities,
immutable source, fresh build, output coverage, self-checks and bus assertions
remain required. A successful fast exit alone is insufficient.

Fast onboarding builds only `gem5.opt`. Pulling Git updates does not install
`gem5.debug`: executables are intentionally untracked. For a project-selector
installation, add the same-source debug build when diagnosis is needed:

```bash
python3 scripts/bootstrap.py build-gem5 --mode debug --jobs 4
./ace-echo doctor --scope diagnostic --json
```

For a legacy host installation, pass its `--config` and `--backend` to doctor,
as in the setup examples above. An approved prebuilt debug executable can also
be installed with `bootstrap.py install-gem5 --mode debug --binary ... --receipt ...`.
Never rename/symlink `gem5.opt` to `gem5.debug`, substitute another source build,
or silently fall back when `--mode verification` was requested. Existing
build receipts are immutable; if a build attempt failed, preserve its log and
receipt before arranging a separate build attempt.

For failure, assertion, hang or output divergence, preserve the failing
attempt and run `--mode verification` in a fresh directory. A debug PASS does
not override a failed fast/RTL comparison. Revalidate affected cases after a
fix; record unused debug as NOT_RUN, not PASS.
RTL additionally needs the exact separately supplied immutable RTL tree,
generated IP dependencies, Scheduler toolchain and licensed tools declared by
the backend. Follow the Forge RTL preflight/qualification procedure. Supplying
`rtl_root` alone does not install its IP or tools and does not qualify anything.
For the Venus2 pointer ABI correction, signed task-case directory, and remaining
RTL validation requirements, see [Venus2 pointer ABI](VENUS2_POINTER_ABI.md).

Historical Hub applications are source/knowledge examples, not all self-contained
reproduction packages: some still need external Damoda/OAI sources or recorded
artifacts. Those dependencies must be separately declared and resolved before
claiming their reproduction. This onboarding change does not certify them.
