# A32 R60 task20 scalar branch/load diagnosis

Date: 2026-08-10

Status: diagnostic complete; timing candidates rejected; executable model
restored to the accepted R59 behavior.

## Scope

This pass followed nrPDCCH task20 from VBRDCST sequence 1 to VSTORE
sequence 2.  It compared RTL `scalar_retire` events with gem5 `Exec` commit
events.  It did not use full-DAG cancellation as an acceptance criterion.

Frozen RTL scalar source:

`venus_soc/sim/build_gc0802_overall_nrPDCCH_tv9_nrPDCCH_full_structzero_a28_20260727/venus_full_dag_perf_cluster0_tile0.jsonl`

Accepted starting point: A32 R59.

## Exact localization

The dynamic scalar PC sequence is identical.  All 757 excess tile cycles
between task20 VBRDCST sequence 1 and VSTORE sequence 2 are explained by
three repeated edges in the 164-iteration loop:

| edge | count | RTL cycles/edge | R59 gem5 cycles/edge | total delta |
|---|---:|---:|---:|---:|
| `0x26c branch -> 0x270 lhu` | 164 | 6 | 8 | +328 |
| `0x270 lhu -> 0x274 dependent branch` | 164 | 2 | 4 | +328 |
| taken `0x274 branch -> 0x264 addi` | 101 | 3 | 4 | +101 |

The sum is exactly 757 cycles.  ALU, store, JAL, and loop-update gaps are
otherwise mostly exact.  This rules out task20 VSTORE latency, vector LSU,
VFU, VRF arbitration, CPU frequency, and control-flow correctness as the
source of the sequence-1-to-2 gap.

The detailed gem5 commit evidence is:

`/tmp/a32_r60_task20_exec_20260809/cch/m5out/task20_exec.log`

The internal Minor/LSQ trace additionally shows that with R59 behavior the
load enters the memory FU one cycle after the branch, but cannot be handed to
the LSQ until the older branch has retired and the next commit edge opens.
The request-to-response path itself is six cycles.

## Rejected candidates

### All ID-resolved controls on a FU-less path

This made `load -> dependent branch` 2 cycles, but moved delay into branch
followers and shifted the full CCH by about 11.6 us.  It was rejected as a
broad control-retirement remap.

### Global early memory issue

This made task20 `branch -> load` exactly 6 cycles.  It is not deployable:
PDSCH held-out compared only 892 instructions before functional divergence,
task3 stopped at ordinal 67 instead of 289, and task4/task5/task8 produced
wrong values.  The full SCH path collapsed from 1541.836 us to 1200.996 us.

Rejected evidence:

`/tmp/a32_r65_pdsch_heldout_20260810`

### Conditional or taken/not-taken commit-gate classification

Classifying every conditional control as a 6-cycle predecessor made SCH
task0 `-45.427 us / -6.717%`.  Refining this to not-taken=6 and taken=8
restored SCH and improved its full path to approximately `+1.292 us`, while
keeping all 8,103 VINS exact.  It nevertheless regressed CCH task18 to
`-3.879 us / -3.148%` and task20 to `-13.027 us / -3.269%`, so direction
alone is not a general boundary.

Cross-link functional evidence:

- accepted CCH candidate check: 10,253 instructions exact;
- accepted SCH direction experiment check: 8,103 instructions exact;
- the direction experiment is still rejected because of CCH task18 timing.

Evidence directories:

- `/tmp/a32_r69_taken_control_cch_20260810`
- `/tmp/a32_r69_taken_control_pdsch_20260810`

### First branch follower on a short path

This reduced the first follower gap, but moved the same bubble to the second
ALU (`0x278 -> 0x27c` and `0x264 -> 0x268`).  Task20 duration did not change.
It was rejected as delay relocation rather than pipeline alignment.

## Safe checkpoint and next action

The executable model is restored to the accepted R59 behavior:

- scalar early-memory issue disabled;
- control predecessor load-commit gate remains 8 cycles;
- non-control predecessor gate remains 6 cycles;
- no task ID, PC, operand-value, or workload special case remains;
- LSU/VFU fixed latencies are unchanged.

The next experiment must compare task18 and task20 at the same internal
boundaries: branch issue/retire, memory-FU arrival, LSQ request, response,
load retire, dependent-control admission, and target/fall-through admission.
The missing discriminator is a real pipeline/response state, not opcode,
taken direction, task identity, or total-DAG error.

Verdict: `INCONCLUSIVE` for complete RTL timing alignment; the R59 checkpoint
remains functionally case-validated and safe for continued diagnosis.
