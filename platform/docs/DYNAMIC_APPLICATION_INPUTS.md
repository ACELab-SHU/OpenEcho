# Dynamic application inputs in Gem5

## Hardware authority and scope

For `venus2p0-16x128`, RTL commit
`bf82d804968672e04db31d105876092d78969b0e` explicitly constructs type-6
runtime pointer records in `hardware/L2_scheduler/task_manager.sv` and routes
them through the pointer DMA path in
`hardware/L2_scheduler/venus_task_lv_scheduler_dma_assignment_mux.sv`.
See [the pointer ABI](VENUS2_POINTER_ABI.md). This source evidence establishes
the intended descriptor mechanism; it is not a fresh full-DAG RTL PASS.

## Two execution modes

`run application --scheduler-engine firmware` executes the exact L1 ELF.
Its DAG registry validates code, descriptor tables, DMT routing and firmware
identity without requiring runtime payloads to exist in the embedded DAG blob.
The registry emits task code/data scaffolding with no preloaded task inputs.
The real L1 CPU, FIFO setup and DMA must supply data before firing the DAG.
The existing firmware runtime disables the shared-image time-zero preload.

Registry manifests carry `input_materialization: live-firmware-dma` and are
rejected without the firmware engine, including a single-DAG registry. Capture
overlays and preloaded fixtures are not permitted in this mode. This does not
infer an input from its symbol name, zero-fill a missing case, predict a fire,
change a descriptor, or modify Scheduler/RTL code.

`run application --scheduler-engine contract` does not execute L1. It requires
complete input bytes. Supply runtime payloads with `--l2-backing-spec` when they
are absent from or replace declared dynamic input slots in the canonical image.
The existing schema is:

```json
{
  "version": 1,
  "l2_backing_inputs": [
    {
      "address": "0x...",
      "length": 4096,
      "provenance": "Exact case payload; address/length from generated input table",
      "source": {"kind": "file", "file": "input.bin"}
    }
  ]
}
```

Use actual numeric addresses from the generated tables, not the placeholder.
Offsets address shared L2, not a CPU pointer or a tile destination. A type-6
pointer's backing is materialized at its declared global range, while the
pointer record itself remains a separate 64-byte transfer. A captured pointer
DMA record must never be mistaken for the backing bytes, even if both happen
to have length 64. A static type-5 pointer cannot authorize replacing embedded
canonical bytes. Code/data overwrite, undeclared canonical overwrite,
incomplete post-image input, invalid length and conflicting payloads remain
errors. The loader does not certify arbitrary kernel pointer accesses or the
selected hardware's physical SRAM capacity.

## Normal full-PDCCH example

After compiling the complete `nrPDCCH_2p0_full_ce_candidate_a5` and building
its TV9 self-checking L1 firmware with `src/main_nrPDCCH_tv9.c` (DAG name
`nrPDCCH_2p0` to match that launcher):

```bash
./ace-echo run application --mode fast --scheduler-engine firmware \
  --l1-elf runs/pdcch-scheduler/artifacts/scheduler-artifacts/l1.elf \
  --run-dir runs/pdcch-firmware-fast
```

Use fresh run directories and the selected hardware's artifact receipts.
No Gem5 C++ rebuild is needed for these Python-only loader changes, provided
the installed binary already matches the platform's required engine source.
Software completion alone does not authorize an RTL or numerical correctness
claim; retain every output and check the self-check result as well.

## Regression evidence (2026-09-09)

- 311 repository unit tests pass, including 13 new positive/negative loader
  tests and existing descriptor/registry tests.
- A5 TV9 regression uses the same previously built L1 ELF, SHA-256
  `f04463f68864dda542716d384d5318b372175152757ec09dcf3272af8f7538e1`.
- Both explicit-input contract and live-firmware Gem5 fast execution complete
  all 24 tasks. All 53 task-output transport dumps match between modes.
- All six final outputs match the existing TV9 expected bytes over their
  declared lengths, in both modes. Firmware emits completion GPIO value 8,
  without the failure bit. Padding is not promoted to golden data.
- Evidence directory: `nrPDCCH-input-infra-regression-20260909-a001` under the
  testing account's `runs/venus2.0/`; commands, logs, manifests, inputs and
  comparison records are retained.

This is an infrastructure regression, not a new hardware qualification.
No RTL, compiler, workload, hardware configuration or comparison masks were
changed. Fresh application compile/integration/RTL acceptance remains a
separate run. The patch is reversible independently of workload changes.
