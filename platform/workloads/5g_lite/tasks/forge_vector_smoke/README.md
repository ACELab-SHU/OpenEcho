# Forge vector smoke (software onboarding only)

This small, standalone example proves that a new checkout can generate a BAS
case, compile Venus C, transfer data between two tasks, execute Gem5 fast,
compare **every** task output and report a clock-normalized DAG latency.
It does not import another application or read an old `runs/` archive.

## Frozen numerical and ABI contract

- Inputs: two little-endian signed i16 arrays, 32 elements each, values in
  `[-1024, 1024]`. Contiguous, no aliasing or in-place updates.
- Stage 1: `sum[i] = a[i] + b[i]`; stage 2: `restored[i] = sum[i] - b[i]`.
  All intermediates fit signed i16; no rounding, saturation or shifts occur.
- Two vector input arguments per task; one 64-byte output per task.
- `sum` is both an observable output and the real producer of the second task.
- Three frozen cases: signed ramp, zero, domain boundaries. The BAS in this
  directory is the signed-ramp example; `reference.py` generates the matrix.
- Host golden state: `PROVISIONAL_GOLDEN`. Human digest approval is required
  before reusing it as an authoritative application/RTL reference.
- Intended tested profile: Venus1 64 lanes/512 rows, 300 MHz tile. Other
  profiles require their own software and hardware evidence.
- Operand-order probe: the V1 LLVM lowering binds the first C operand to
  `vs2` and second to `vs1`. In this backend `VSUB` is `vs1-vs2`, while
  `VRSUB` is `vs2-vs1`. Thus restoration uses `vrsub(sum,b)`. The asymmetric
  signed-ramp case rejects `vsub(sum,b)` even though the DAG completes.
  This is compiler/Gem5-scoped evidence, not a new cross-backend ISA rule.

## Run

Follow ACE-ECHO `docs/GETTING_STARTED.md`, then run `make smoke` there.
Or run this file directly with explicit `--platform-root`, `--config`,
`--backend`, and a **new** `--output` directory:

```bash
python3 run.py --help
```

Every case retains generated source, actual inputs, reference and actual
outputs, command logs, build receipts, simulator stats and task trace. A
successful process exit without exact output/length matches is a failure.
Only closed, allowlisted replay intermediates are compacted after all cases
pass; `--keep-heavy` disables this. No entire experiment directory is deleted.

Latency is the interval from the first task allocation to `dag_complete`.
Cycles are derived from the sequencer's actual `clk_domain` in Gem5's
`config.ini`; seconds use `simFreq` from stats. This includes modeled DAG
scheduling/returns but excludes the host-side comparison. It is **not** a
measurement of full L1 firmware, RF timing or RTL latency.

## Develop a new application

Copy this directory to a new target name under `5g_lite/tasks/`, rename its
BAS entrypoint, and add it to `5g_lite/task-index.json` for discoverability.
Replace the numerical contract, cases and host reference before changing the
Venus payload. Keep source/reference changes reviewable and retain failed
cases. Ask your AI host to use `$ace-echo-venus-forge`; the skill lives in the
platform repository, not in this task.
