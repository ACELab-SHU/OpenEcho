# ACE-ECHO 1.0

Venus 1.0 development with Gem5 cycle-level simulation. See the
[1.0 release notes](docs/releases/1.0.md) for the pinned components,
measured RTL comparison, and validation boundaries.

## New users: start here

For one-file Venus1/Venus2 switching with independent personal compiler and RTL
paths, see [hardware selection](docs/HARDWARE_SELECTION.md).

See [the clean-checkout Forge guide](docs/GETTING_STARTED.md) for the paired
release branch, external compiler requirements, a source-built Gem5, explicit
host configuration, and the self-contained two-task smoke application.
`make smoke` now exercises only that Gem5 fast development loop; it does not
claim Scheduler firmware or RTL qualification. The `forge` CLI initializes a
run; an AI coding host must load the repository skill to drive development.

ACE-ECHO is a standalone, Gem5-first Venus software development and
validation platform. It owns its Gem5 model, Scheduler sources, CLI, run
manifests, and comparison tools. It does not execute VEMU and it does not
depend at runtime on the old Gem5, Scheduler, or RTL worktrees.

The DSL/BAS frontend and selected Echo Hub workloads are bundled source snapshots. See PUBLIC_SOURCE_MANIFEST.json for their pinned revisions.
The installed Venus LLVM and RISC-V compilers are external tools.
`Debug/Emulator` is not included and is never invoked.

## What is runnable now

| Scope | Command | Current evidence level |
| --- | --- | --- |
| Single task | `ace-echo run task` | Gem5 task execution from immutable JSON/BIN/task images |
| DAG | `ace-echo run dag` | One-process multi-task Gem5 execution with dependency and Scheduler model |
| Scheduler package | `ace-echo scheduler build` | Isolated build of ACE-ECHO's own Scheduler source and `l1.elf/l1.bin` |
| Application contract | `ace-echo run application` | Runtime DAG decoded from Scheduler `l1.elf`, then executed by Gem5's RTL-shaped Scheduler state machine |
| Full Scheduler firmware | reserved `--scheduler-engine firmware` | Deliberately blocked until the Scheduler CPU/peripheral execution backend is implemented |
| ACE-ECHO Forge | `ace-echo forge` | AI-first golden → Venus → Gem5 → immutable RTL workflow |

The application-contract mode is not advertised as full firmware validation.
That distinction is recorded in every `run.json` coverage field.

## Layout

```text
ACE-ECHO/
├── ace-echo                  # zero-install CLI
├── configs/local.toml        # external compiler and internal component paths
├── components/
│   ├── gem5/                 # owned source; opt/debug must be built or explicitly installed
│   ├── scheduler/            # owned Scheduler source
│   ├── toolchain/dsl/        # DSL/BAS frontend snapshot; no VEMU runtime
│   └── manifest.json         # snapshot provenance and binary digests
├── workloads/                # selected Echo Hub task/DAG source snapshot
├── src/ace_echo/             # orchestration, adapters, records, comparisons
├── docs/                     # architecture and coverage contract
├── tests/
└── runs/                     # generated, immutable-per-run output (gitignored)
```

## First checks

```bash
cd ACE-Echo/platform
# First configure and build tools as described in docs/GETTING_STARTED.md.
./ace-echo --config .ace-echo/host/local.toml doctor --scope fast \
  --backend .ace-echo/host/backend.json
make test
make smoke
```

`doctor` verifies the owned Gem5 binaries and source adapters, owned Scheduler,
external compiler root, and host tools.

## Common workflows

Dry-run any workflow first to inspect the exact commands:

```bash
./ace-echo --dry-run run task \
  --dag-json /path/dag1.json --combined-bin /path/dag1.bin \
  --case-dir /path/task-images --task 17 --mode verification

./ace-echo --dry-run run dag \
  --dag-json /path/dag1.json --combined-bin /path/dag1.bin \
  --case-dir /path/task-images --mode fast

./ace-echo scheduler build \
  --dag-name my_dag --dag-json /path/my_dag.json --dag-bin /path/my_dag.bin

# Generate a generic zero-runtime-input launcher for a fully static DAG.
./ace-echo scheduler build --backend V1 --dag-name nrPBCH \
  --dag-json /path/dag1.json --dag-bin /path/dag1.bin \
  --auto-static-main

./ace-echo run application \
  --l1-elf /path/l1.elf --scheduler-engine contract --mode fast
```

Compile C/BAS without VEMU execution:

```bash
./ace-echo compile dag --target my_dag
```

Optionally keep test inputs outside the BAS, using the same declaration syntax:

```bash
./ace-echo compile dag --target my_dag --params /path/to/case.params
```

Use `parameter short mode = {15}`; `'` starts a comment, not an input
declaration. With no external file the legacy BAS path is unchanged. See
[optional BAS parameter inputs](docs/BAS_PARAMETER_INPUTS.md) for precedence,
runtime-input handoff, and the migrated PDCCH TV9 example.

Compilation happens in a run-local copy of ACE-ECHO's DSL frontend. The
resulting JSON, combined BIN, and task images are copied into the immutable
ACE-ECHO run directory; the checked-in component source is not modified.

Bit-exact comparison:

```bash
./ace-echo compare expected.txt actual.txt --dtype i16 --output comparison.json
```

Start an AI-first Forge run from a validated request:

```bash
./ace-echo forge examples/forge-oai-ue.example.json --validate-only
./ace-echo forge examples/forge-oai-ue.example.json
```

The repository-local skill is
`.agents/skills/ace-echo-venus-forge/`. Backend generations are selected with
`configs/backends/*.json`; adding Venus3.0 or Venus4.0 requires a new manifest
and smoke probes, not a rewritten agent. Formal hardware and optimization
knowledge lives under `docs/knowledge/`.

Venus1.0 and Venus2.0 use the same CLI and adapters. Select the generation on
every architecture-sensitive step with one manifest; the compiler rows/lanes,
Gem5 profile, Scheduler descriptor ABI/linker layout, RTL root and RTL
simulation flow are then resolved as one unit.  In particular, Venus1.0 uses
its 0x10100000 uncached Scheduler payload aperture.  Its current Scheduler CPU
profile also leaves the unqualified `menvcfgh` dcache-boundary CSR untouched;
the repository-qualified V1 CCM command protocol and PLL/reset sequence are
selected from separate backend-owned bodies.  The V1 helper uses the hardware
`0x02/0x03` command heads and lock settling interval; Venus2.0 retains its
shared `0x52/0x7a/0x50` implementation.  V1 also restores the four control
output and eight control input PAD settings required by its RTL testbench.
None of these V1 settings are
inherited by Venus2.0.  The default
fresh RTL flow is the
functionally equivalent non-coverage target; `simcm` remains available for
explicit coverage work but is not required for functional qualification:

```bash
V1=configs/backends/venus1p0-64x512.json
V2=configs/backends/venus2p0-16x128.json

./ace-echo doctor --backend "$V1"
./ace-echo compile dag --target nrPBCH --backend "$V1"
./ace-echo run dag --dag-json /path/dag1.json --combined-bin /path/dag1.bin \
  --case-dir /path/tasks --mode verification --backend "$V1"
./ace-echo scheduler build --dag-name nrPBCH --dag-json /path/dag1.json \
  --dag-bin /path/dag1.bin --backend "$V1"

# Fresh RTL compiles once, then replays later cases from the same executable.
./ace-echo rtl run --backend "$V1" \
  --case PBCH=/path/to/PBCH/l1.bin \
  --case PDCCH=/path/to/PDCCH/l1.bin \
  --run-dir runs/venus1-pbch-pdcch-rtl

# GPIO completion is not an all-output oracle. `compare-dag-dma` accepts two
# semantic traces containing task-id/return-port metadata. The legacy V1
# testbench's raw CSV `L2_DMA_trx.log` is not that format and must first be
# converted with a same-key task/port mapping. Never compare a signed V1
# l1.bin against a reference generated for another input image merely because
# task names and output lengths happen to match.

# The same commands select Venus2.0 by replacing "$V1" with "$V2".
# `--backend V1` and `--backend V2` are equivalent short aliases.
```

Commands without `--backend` retain the legacy `configs/local.toml` fallback.
For release evidence, always pass an explicit backend so the run record hashes
the selected manifest and cannot silently mix generations.
The RTL command stages firmware only in a run-local source snapshot. Generated
IP dependencies, tool paths, firmware destinations and runtime case variables
come from the backend manifest; the large snapshot is removed after evidence
collection unless `--keep-build` is explicitly requested.

The V1 native PBCH/PDCCH flow currently qualifies platform boot, Scheduler
dispatch, DAG completion and the firmware CRC oracle. Legacy AXI monitor
unknown-data assertions are counted and retained as qualification warnings;
they are not hidden by a final `Test complete!`. Release-level all-visible
output qualification still requires a same-key software trace for the exact
signed `l1.bin` under test.

For source-scoped startup diagnostics, native return-identity reconciliation,
integrated Gem5 output checks and separate RTL build/simulation timeouts, see
[RTL evidence and lifecycle](docs/RTL_EVIDENCE_AND_LIFECYCLE.md). These checks
preserve raw warnings and do not modify RTL or infer padding masks from failures.

After an application-complete PASS report, remove heavy rejected-candidate
artifacts while retaining baseline, winner, RTL-qualified candidates, the top
three correct alternatives, and compact metadata:

```bash
./ace-echo forge --finalize-run runs/<run-id>
```

## Run contract

Every operation creates a new run directory with:

- `run.json`: scope, mode, coverage, component identities and file digests;
- `commands/*.json`: exact argv, cwd, return code, duration and log path;
- `artifacts/`: copied or generated build and simulation artifacts;
- Gem5 `m5out`, task/DAG traces and output dumps when produced.

Fast and verification modes share the same timing model. Fast mode suppresses
expensive per-VINS observations; it does not replace the hardware model.

See [architecture](docs/ARCHITECTURE.md) and
[coverage contract](docs/COVERAGE.md). Bootstrap evidence is recorded in
[validation](docs/VALIDATION.md).
