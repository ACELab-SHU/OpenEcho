# A32 R49 VFU result-generation hold

## Scope and preserved baseline

This checkpoint continues from the accepted A32 R48 binary:

`/tmp/a32_r49_task17_20260729/attempt-001/baseline/gem5.debug`

SHA-256:

`1b55e6a1d890f41ae3154a68098e80873e3170a2f4fb78c730ce6797d583afad`

The A47 dispatcher/barrier fixes and the R48 manifest, structured-LSU,
DMT-return, tagged operand/VRF response, and hazard-class retirement fixes
remain in place. No DAG-wide, VFU, or LSU fixed latency was changed.

The promoted R49 binary is:

`/tmp/a32_r49_task17_20260729/attempt-031/promoted/gem5.debug`

SHA-256:

`452e662e759a4585159ed7d7d89c901ceba42127805725d78e335ab0171273b6`

## Rejected hypothesis and first divergence

R48 admitted hazard-overlapped operand commands early only for CAU A-D.
RTL has four instruction slots for BitALU, CAU, and SerDiv, so the first R49
hypothesis extended tagged requester admission to BitALU A/B and SerDiv A/B.
The directed nrPDCCH task 17 became faster, but full nrPDCCH and PDSCH both
stopped. Therefore that admission-only change was rejected.

The first deadlock was reproduced with:

`/tmp/a32_r49_task17_20260729/attempt-027/nrpdcch_attempt003_stall_trace`

The terminal sequence on every lane was:

1. instruction 1402 completed its CAU writes;
2. BitALU instruction 1403 issued all 32 tagged reads and calculated all 32
   rows;
3. at tick `547472000`, instruction 1403 wrote result 31/32 while the same
   BitAlu_B requester accepted younger instruction 1404;
4. instruction 1403's final tagged result was present in `datapipe`, but the
   younger active-generation transition called `setPipeLength()`, which
   clears every pipe slot;
5. instruction 1404 then waited forever for same-VFU RAW producer 1403:
   `progress 31, credit eligible 0`.

This was a circular wait caused by result-generation loss, not an incorrect
fixed latency or an LSU issue.

RTL does not have this state collapse. Its BitALU has:

- a four-entry vector-instruction queue;
- an independently backpressured two-entry result queue;
- an instruction ID stored with every result entry.

The relevant source is
`venus_soc/hardware/venus_extension/venus_bitalu_wrapper.sv`.

## Structural correction

R49 keeps early tagged command admission for all arithmetic operand
requesters:

- BitALU A/B;
- CAU A-D;
- SerDiv A/B.

It also enforces a generation transition invariant in the current gem5
BitALU representation: a younger instruction cannot reset the active
pipeline length while either tagged result pipe still contains an older
beat. The old beat is advanced through the ordinary result/writeback
handshake first.

This is deliberately structural:

- no task ID, PC, DAG name, address, or payload branch;
- no global hazard bypass;
- no latency constant change;
- the result and instruction tags remain the ownership proof.

The current implementation serializes only the instruction-boundary
transition that the old `datapipe::setPipeLength()` cannot represent safely.
A future fully explicit RTL-style two-entry result queue may recover more
same-VFU overlap, but it must preserve this no-overwrite invariant.

## Directed and full validation

### nrPDCCH full

Run:

`/tmp/a32_r49_task17_20260729/attempt-028/nrpdcch_bitalu_result_hold_full`

Results:

- natural completion at tick `1,043,164,000`;
- 24/24 tasks released;
- 53/53 RTL-captured return ports pass;
- 10,253 VINS outputs are byte-identical to the accepted R48 functional
  baseline;
- LSU accept/request/response/complete are each `797`;
- 797/797 strict LSU lifecycles close, with no incomplete operation.

The previous admission-only deadlock at task 19 is gone. Task 20 starts at
tick `550752000`, after instruction 1403's final result drains normally.

Normalized allocation-to-last-release is:

- RTL: `1,043,224 ns`;
- gem5 R48: `1,055,260 ns`, delta `+12,036 ns` (`+1.154%`);
- gem5 R49: `1,043,112 ns`, delta `-112 ns` (`-0.011%`).

This near-zero whole-DAG delta is not proof of complete microtiming: positive
and negative per-task errors still cancel.

| Task | RTL ns | gem5 R49 ns | Delta | Error |
| ---: | ---: | ---: | ---: | ---: |
| 1 | 10,691 | 11,280 | +589 | +5.509% |
| 3 | 69,631 | 72,844 | +3,213 | +4.614% |
| 9 | 158,799 | 170,240 | +11,441 | +7.205% |
| 14 | 202,779 | 203,284 | +505 | +0.249% |
| 17 | 33,167 | 49,176 | +16,009 | +48.268% |
| 20 | 398,523 | 386,632 | -11,891 | -2.984% |
| 22 | 100,055 | 104,916 | +4,861 | +4.858% |
| 23 | 33,951 | 38,732 | +4,781 | +14.082% |

### Structured LSU matrix

Run:

`/tmp/a32_r49_task17_20260729/attempt-029/lsu_suite_bitalu_result_hold`

Results:

- 68/68 pass;
- RAW-chain, independent-stream throughput, and load/store capacity cliffs
  remain covered;
- suite binary SHA-256 matches the promoted R49 binary.

### PDSCHDag2 A47 held-out

Run:

`/tmp/a32_r49_task17_20260729/attempt-030/pdsch_bitalu_result_hold_heldout`

Results:

- natural completion at tick `1,525,460,000`;
- 9/9 tasks released;
- 22/22 RTL-captured return ports pass;
- 8,103/8,103 VINS outputs are byte-identical to the accepted R48 held-out;
- LSU accept/request/response/complete are each `1,312`;
- 1,312/1,312 strict LSU lifecycles close.

The held-out completion moved by `-548 ns` from R48. It was retained as an
observable structural timing consequence, not compensated with latency
tuning.

## Promotion and next first divergence

R49 is promoted because the admission-only failure is explained by a
cycle-level result ownership trace, the correction is architecture-wide,
and the directed LSU plus both full DAG regressions pass.

Complete RTL microtiming still cannot be claimed. The next work should use
RTL/gem5 request/result/retirement events to split:

1. nrPDCCH task 17's remaining `+16,009 ns`, especially instruction-boundary
   result-queue occupancy versus requester admission;
2. task 9 and task 22/23 positive tails;
3. task 20's `-11,891 ns` lead and the task 6/7 allocator tie.

Do not fit these errors with a global latency. The next structural step is an
explicit fixed-depth, tagged BitALU/CAU/SerDiv instruction/result queue model
with RTL enqueue/dequeue/full/empty events.
