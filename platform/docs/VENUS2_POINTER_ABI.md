# Venus2 pointer ABI and verification setup

## Corrected backend contract

`configs/backends/venus2p0-16x128.json` uses three-bit input descriptors with
the following RTL-scoped mapping:

| Logical type | Physical encoding | Meaning |
| --- | --- | --- |
| 0 | 000 | temporary value |
| 1 | 001 | global value |
| 2 | 010 | runtime/DFE value |
| 4 | 100 | temporary pointer record |
| 5 | 101 | global pointer record |
| 6 | 101 | runtime/DFE pointer record via global-parameter pointer path |

The prior manifest mapped 4/5/6 to 0/1/1 and rejected those logical types at
Scheduler handoff. That was stale for RTL commit
`bf82d804968672e04db31d105876092d78969b0e`. Its evidence is:

- `hardware/L2_scheduler/l2_scheduler_pkg.sv:48-49`: descriptor type width.
- `hardware/L2_scheduler/task_manager.sv`, `FSM_TASK_FIRING_1`: physical 5
  fetches the global parameter; physical 6 does not. Its following pointer
  construction branch reads the stale response from the preceding input.
- `hardware/L2_scheduler/venus_task_lv_scheduler_dma_assignment_mux.sv:51-58`:
  pointer transfer address handling.

The parser still rejects reserved/out-of-width types. Other backends retain
their own mappings and unsupported-type lists. No Scheduler core or RTL source
is changed by this correction. Do not generalize this profile to older RTL
trees merely because they are named Venus2.

### Runtime pointer correction (2026-09-10)

The earlier 6-to-6 mapping inferred complete support from pointer construction
alone. Fresh full-A5 TV9 RTL runs on both the FZQ 384 KiB tree and SYH Venus_3
512 KiB tree disproved it: task 3 received `0x33d80/64` instead of
`0x48e00/4096` for `csetSubcarriers`. All other 55 launch transfers matched.
The unchanged RTL task-manager SHA-256 was
`91d6a59e0aa01e8787c7e48ad6d89d4dae24938a58e3be1d5cd04c0e29d5f7e6`.
The failed firmware BIN SHA-256 was
`f40c3a5596fbe7aab375ac5bc4a3f8e6d09aada4702e1bef234896c48f39ad2b`.

Decoding historical passing A28 ELF
`d28f12a26dfbe9e21363c9cd1f8116444c3ee818888af4df9759cfe787b46cbf`
shows physical **5**, not 6, for the same task-3 descriptor. The backend now
restores that pointer-to-pointer mapping. Logical 6 still belongs to
`DAG_INPUT_TYPES`: L1 input offsets, lengths, deduplication and per-fire DMA
remain dynamic. The global table entry retains the declared payload length;
only its task descriptor encoding changes. This is not a static-input
substitution, a hardware edit, or permission to map a pointer to value type 1.

Unit tests cover actual packed descriptors, distinct adjacent global entries,
runtime-input ownership, declared payload lengths, reserved types, and an
explicit native-6 mapping for other backends. Fresh software/RTL qualification
and remaining transport/validity limitations are recorded separately; a
pointer fix alone does not waive those checks.

The fresh SYH Venus_3 regression is retained under
`runs/venus2-pointer-compat-20260910-a001/`:

- 312 unit tests passed; fresh compilation and firmware-driven Gem5 fast passed.
- Corrected L1 BIN SHA-256:
  `93fb3e913a58cf376668c69964760154a72feb5940ce22c9d30742ee56ed35af`.
- ELF audit: only three input descriptors (tasks 3, 5 and 7) changed from
  physical 6 to 5. Task code/data, global table and dynamic input offset/length
  tables remained byte-identical to the negative-control firmware.
- Fresh RTL task-3 launch DMA: 56/56 transfers exact. All 53 outputs passed
  source-declared valid-bit comparison; all six final goldens and the firmware
  self-check passed. The same comparison still fails the negative control.
- Validity layouts describe seven `short_struct` outputs: one 16-bit field
  inside a 64-byte aligned object. Every other requested bit remains strict.
  No layout was inferred from observed Xs or differing values.
- This validates the pointer compatibility fix, **not complete RTL acceptance**.
  Strict transport comparison remains 46/53 because of 434 unknown padding
  bytes; 37 AXI unknown assertions remain. The unchanged allocation gate blocks
  task20/port0 (128 declared versus 4096 returned), and three short final
  outputs (2 declared versus 64 returned). The official report remains FAIL /
  `OUTPUT_COMPARISON_BLOCKED`; neither checker nor RTL was modified.

Do not report this fixed TV9 regression as all-configuration qualification.
The allocation failures above describe that historical, pre-return-fix attempt.
The approved follow-up now returns 128 B from both Polar decoder branches and
exports actual occupied output allocations. Compiler-return and observed-size
checks reject overruns before RTL; aligned short-object transport is checked
against its proven allocation instead of its semantic length. See
[output capacity gates](OUTPUT_CAPACITY.md). This correction does not waive
AXI assertions, padding diagnostics or other qualification gates.

## What was verified, and what was not

### Pointer record length versus backing allocation

For static/global and runtime/DFE pointer inputs (types 5/6), the in-process
Gem5 launcher packs the compiled JSON `offset` and **declared `length`** into
the DMT pointer record. This follows `components/scheduler/dags/read_dag_json.py`:
the global/parameter entry is built from those fields, not the distance to
the next parameter. The record transport itself is 64 bytes; that is a third,
separate quantity. Type-4 dependency handling is unchanged.

A workspace can occupy more than 65,535 bytes while its declared descriptor
length is 64. Replacing that declaration with the whole allocation size both
changes the ABI and can incorrectly reject a valid record. Missing, zero,
negative or greater-than-uint16 declared lengths still fail; no clipping,
wrapping or implicit 64-byte fallback is allowed.

The launcher separately validates offsets against the combined image and the
declared length against the inferred backing span (next input offset or image
end). The manifest's `static_pointer_records` records the declared length,
backing span, offset and transport size. These checks do not certify arbitrary
kernel pointer accesses or physical SRAM fit; Scheduler/linker and RTL
resource checks are still required. Aliased inputs that do not fit this
conservative span check require an explicit layout contract, not a bypass.

The historical Venus_3 PDSCHDag2 record used offset `0xea40` and length `64`
(`0xea400040`). Historical hardware evidence used a different DAG binary and
512 KiB shared memory; it does not qualify a fresh 384 KiB/6144-row run.

### Earlier backend correction

The recorded nrPDSCHDag1_hw_2p0 diagnostic used these exact artifacts:

- DAG JSON SHA-256: `3c9e7cc08d3088ff77e37fd8f332b8d97a84dabb6953ab59078511d86449f1bf`
- Combined BIN SHA-256: `abe5ea4e1dc3a9f318fdc85d873142d8053fa0ebe4e749e28ba3a2220489a356`
- L1 BIN SHA-256: `0de31dea68d3c97eef7bf6e848735ad437dca55c83d64494a34f3c32f5f3dc27`

That attempt exercised 53 tasks, including 52 type-4 and 599 type-5 inputs.
A fresh RTL simulator executed all tasks and reached completion. Three final
returns totaling 15,552 bytes matched the software replay byte-for-byte.
This is historical software/RTL replay evidence, not an independent numerical
golden and not a new-account hardware qualification.

The native RTL acceptance report remained FAIL: time-zero devctrl assertion,
AXI unknown-data assertions, internal padding and duplicate return annotation
issues were not waived. The replay firmware's failure GPIO was not held until
its completion GPIO, so its completion marker alone is not a valid self-check
claim; the final-byte result above comes from independent DMA extraction.
Type 6 has parser/source evidence but was **not exercised by this DAG**.

The generic valid-bit comparison can report declared padding separately, but
does not repair missing/duplicate identity coverage or suppress bus assertions.
See [output validity](OUTPUT_VALIDITY.md).

## Updating an installation

Pull the branch/release containing this correction, then run:

```bash
./ace-echo hardware show
./ace-echo doctor --scope software --json
```

The normal route uses fast and does not require debug. For failure/divergence
diagnosis, build it with `python3 scripts/bootstrap.py build-gem5 --mode debug
--jobs 4`, then use `doctor --scope diagnostic` and `--mode verification`.
Git does not distribute a debug executable. Use an approved same-source receipt
for any prebuilt installation; never substitute opt for debug.
See [getting started](GETTING_STARTED.md).

The project selector creates a new backend/artifact identity after this ABI
change. Recompile the DAG; do not bypass the stale-artifact check. For a static
DAG such as this one, whose input payloads are embedded in the combined BIN:

```bash
./ace-echo compile dag --target nrPDSCHDag1_hw_2p0 \
  --run-dir runs/nrpdsch-compile-new
./ace-echo run dag --mode fast \
  --dag-json runs/nrpdsch-compile-new/artifacts/toolchain/dag1.json \
  --combined-bin runs/nrpdsch-compile-new/artifacts/toolchain/dag1.bin \
  --case-dir runs/nrpdsch-compile-new/artifacts/toolchain/tasks \
  --run-dir runs/nrpdsch-fast-new
```

The task binary directory (not its parent or an empty directory) is the signed
case directory. Compare the full declared case matrix and all required outputs
before building an integrated RTL handoff; debug is an optional diagnostic
branch, not an extra normal-path run. Other DAGs with external runtime inputs need their
own case payloads; do not assume the static recipe applies to them.

Only after software gates pass should a self-checking Scheduler image and
fresh immutable RTL attempt be built. Every user's RTL root remains in personal
host paths. Missing generated IP, file-list relocation and licenses are
separate deployment prerequisites, not fixed by changing pointer encodings.

The adapter fix in this change allows rewriting manifest-declared copied
metadata even when the copied file is read-only, restoring its mode afterward.
It rejects source HDL, symlinks outside the snapshot and shared hardlinks.
It does not auto-invent file-list rewrites, change RTL source permissions,
or blanket-ignore simulator errors.
