# Valid output bits and padding warnings

Output correctness is determined by declared semantic bits, not by arbitrary
contents of alignment padding. ACE-ECHO supports domain-independent output
layout metadata for raw files and task-return DMA comparison. No task names,
radio modes, instance counts, backend paths or hardware generation assumptions
are part of a layout.

## Layout

```json
{
  "schema": "ace-echo-output-layout/v1",
  "transport_bytes": 64,
  "valid_bit_ranges": [[0, 8]]
}
```

Each range is `[bit_offset, bit_count]`. Bit 0 is the least significant bit of
the byte at offset 0; bit 8 is the least significant bit of byte 1. Ranges must
be ordered, nonempty, nonoverlapping and contained within the transfer. The
complement is declared padding. This example describes one 8-bit field in a
64-byte transport; it is not a rule automatically applied to scalar outputs.

Layouts must come from explicit ABI/output descriptions. They must not be
inferred from X locations or disagreements. The compiler does not infer C
semantic layouts: users or generators provide explicit declarations, which can
now be stored with the workload and propagated automatically as follows.
Arrays with dynamic valid lengths need an explicit layout for the selected
case. Changing consumers, sizes or layout requires reviewing this metadata.
Keep it with the build/case evidence; result reports record its SHA-256,
contents and compared input hashes. Build fingerprints are provenance, not
hard-coded interface semantics.

## Checked-in workload contract (automatic normal route)

Place `output-validity.json` beside the entry BAS file. Bind declarations to
producer/output names, not transient task IDs or an observed mismatch:

```json
{
  "schema": "ace-echo-output-contract/v1",
  "basis": "One short data field at byte 0 of a 64-byte aligned object.",
  "outputs": [{
    "task": "Task_example", "output": "count",
    "layout": {
      "schema": "ace-echo-output-layout/v1",
      "transport_bytes": 64, "valid_bit_ranges": [[0, 16]]
    }
  }]
}
```

Compilation resolves each name pair to exactly one task/port and checks its
allocation capacity. Missing, ambiguous or duplicate identities are errors.
It freezes `output-validity.json`, `output-layouts.json`, and
`output-layouts.binding.json` beside the compiled DAG JSON/BIN. The binding
covers the hashes of the contract, resolved layout, DAG JSON and combined BIN.
Retain/export this whole bundle together.

`rtl run --dag-json ...` and identity-aware `compare-dag-outputs --dag-json ...`
automatically load this verified bundle: no extra layout argument is required.
An explicit `--output-layouts` remains a supported case-specific override and
is recorded in comparison evidence. Legacy bundles without declarations remain
strict. Incomplete or stale bundles are errors, never silently ignored.

The declaration is an application ABI promise, not a compiler proof that the
consumer ignores padding. Review it when types, consumers or shapes change.
Do not generate a declaration by looking for unknown or mismatching bytes.

### Verdict policy

| Condition | Functional verdict | Diagnostic |
| --- | --- | --- |
| Valid bits known and equal; only declared padding differs/is unknown | PASS | WARNING: PADDING_* |
| Valid bits differ or are unknown | FAIL | Data comparison failure |
| Return exceeds allocation | Rejected | ERROR: capacity violation |
| Missing bytes, wrong lengths, invalid/stale contract, lost return identity | Rejected/FAIL | Never a padding warning |

`FAIL` is a verification outcome; `WARNING` and `ERROR` describe diagnostic
severity. Do not promote `transport_bit_exact_status: FAIL` to a functional
failure when only explicitly declared padding differs. Do not hide the raw
transport result either. Independent simulator/bus/source-identity gates still
apply. Rejudging saved logs creates a new report and records the new contract;
it never rewrites the historical report or counts as a fresh hardware run.

## Commands

```sh
./ace-echo compare expected.bin actual.bin --dtype raw \
  --layout output-layout.json --output comparison.json
```

For DAGs, select explicit task/port output identities using a map:

```json
{
  "schema": "ace-echo-output-layouts/v1",
  "outputs": [
    {
      "task_id": 2,
      "port": 0,
      "layout": {
        "schema": "ace-echo-output-layout/v1",
        "transport_bytes": 64,
        "valid_bit_ranges": [[0, 8]]
      }
    }
  ]
}
```

```sh
./ace-echo compare-dag-outputs --expected-dma rtl-dma.txt \
  --actual-dir gem5-outputs --output-layouts output-layouts.json \
  --output comparison.json
./ace-echo compare-dag-dma --expected-dma reference-dma.txt \
  --actual-dma rtl-dma.txt --output-layouts output-layouts.json \
  --output comparison.json
```

Undeclared outputs, or calls without a layout, compare **every requested bit**
strictly. Duplicate/unknown layout identities and malformed layouts are errors.
The existing `tid/tname/retid`, native `task_id/retid`, and legacy trace-template
paths retain identity checks. Do not relabel a return by comparing its data.
Missing data, changed lengths, duplicate returns and extra DMA identities do
not become padding warnings. No command changes the RTL execution gate.

## Results

- `status`: PASS only when every declared valid bit is known and exact, with
  matching transfer lengths and the comparison's required return coverage.
- `transport_bit_exact_status`: separate comparison of **all** requested bits,
  including padding. It can be FAIL while the functional comparison is PASS.
- `PADDING_UNKNOWN`: padding has X/Z in either compared trace.
- `PADDING_NONZERO`: at least one side has known nonzero padding; this is **not**
  proof of uninitialized memory.
- `PADDING_DIFFERENCE`: known padding differs between the two sides.

Warnings include affected bit counts and, for DAGs, task/port identity. Partial
unknown hex nibbles are retained: `x5` can match a valid low nibble of `5`, but
cannot pass when its unknown high nibble is also declared valid. Raw binary
files contain only known bits; they cannot prove whether memory was initialized.

Exit status 0 means functional comparison PASS (possibly with warnings), 1
means comparison FAIL and 2 means a CLI/metadata error. Never equate that exit
status with a complete RTL qualification: bus assertions, execution errors,
firmware self-checks and source-identity gates are independent. Padding warnings
do not waive AXI unknown-data assertions or modify existing validation reports.

Clearing padding is an optional transport-determinism improvement, not an
algorithm correction. No kernel, compiler, Scheduler or RTL changes are made
by these comparison options. The default remains strict: in-range X/Z is now
a failure instead of silently skipping unknown reference bytes.

## Allocation safety is not a padding warning

The compiler adapter exports occupied allocation bytes independently from BAS
declared length. Constant return lengths are checked after compilation; observed
returns are checked after Gem5 and again before an explicit RTL handoff. A
return exceeding the allocation is an **error**, regardless of its valid-bit
layout. A 2-byte field in a proven 64-byte slot is different from a 4096-byte
return into a 128-byte slot. See [output capacity gates](OUTPUT_CAPACITY.md).
