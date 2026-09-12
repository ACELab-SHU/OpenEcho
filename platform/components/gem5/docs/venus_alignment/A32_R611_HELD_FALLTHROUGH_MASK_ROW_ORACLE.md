# A32 R611 held-request fall-through and BitALU mask-row oracle

Date: 2026-08-13

Verdict: `INCONCLUSIVE`

## Accepted structural change

The lane command boundary now distinguishes a fresh request from an upstream
request that was held stable while the lane command register was busy.  A
fresh request retains the registered one-cycle boundary.  A different request
that was already visible across a full lane cycle may refill the command
register on the same edge that the previous command is consumed.

This is the behavior observed in RTL around CCH task20: sequence 57 is held
upstream while the preceding command blocks, and then transfers on the edge
immediately following sequence 56.  The implementation is generic: it keys
only on stable instruction identity and elapsed lane edges.  It contains no
task, PC, opcode, address, data, or workload rule.

In the focused task20 replay, sequences 0--54 remain exact.  The old +1-cycle
duration/recycle errors on the two following VMUL instructions close.  The
first remaining duration/recycle divergence is the mask-producing VSGT at
comparison sequence 57: gem5 is one cycle early, while the first fire
divergence moves to sequence 63 at +1 cycle.  This exposes a previous local
compensation rather than completing the model.

Task17 remains functionally exact and does not regress: all three lifecycle
dimensions remain exact through sequence 179; the first duration/recycle
difference is sequence 180 VBRDCST at -1 cycle, and the first fire difference
is sequence 194 VLOAD at +1 cycle.

## RTL mask result structure

The focused RTL oracle proves that a BitALU `vm_w` result does not use the
ordinary per-bank data-result requester on every arithmetic micro-operation.

- `venus_bitalu_wrapper.sv` accumulates result bits in `mask_q`.
- The depth-2 result queue marks a mask entry externally valid only at a
  complete four-bank row boundary or at the final micro-operation.
- Interior entries leave the queue through `mask_jump_gnt_wr`; they do not
  issue a mask SRAM write.
- Boundary entries drive `bitalu_mask_result_req_o`.
- Mask reads and BitALU mask writes share a separate two-input RR in
  `venus_mask_operand_requester.sv`; this path is independent of the
  ordinary per-bank VRF arbiter.

For task20's first `VSGT vm_w`, RTL shows only two real mask-result
request/grant transactions (addresses 3 and 7), with interior result entries
jumped.  gem5 still presents each 16-bit arithmetic result as an individual
mask-memory write.  This is the first known structural gap after the accepted
fall-through fix.

## Rejected experiments

Three timing-only experiments were rejected and fully reverted:

1. delaying every `vm_w` result entry by one edge moved sequence 57 from
   -1 to +2 cycles by adding result-queue backpressure;
2. denying only the first new mask-writer generation did not change timing;
3. packing four results directly into an RTL-shaped mask row preserved the
   first timing divergence but corrupted task20's first mask VINS result.

The third failure is important: gem5's current mask backing layout exposes
16-bit micro-result storage, while RTL's requester writes an `mlen_t` row.
The next implementation must first define an explicit logical mask-row
interface and element mapping, then attach the row accumulator and independent
RR.  Directly widening the existing physical write is invalid.

## Full CCH timing

The boundary is RTL `start execute -> execute complete` versus gem5
`tile_start -> task_epilogue`; return DMA and tile release are excluded.

| Task | RTL ns | gem5 ns | Delta ns | Error |
| ---: | ---: | ---: | ---: | ---: |
| 0 | 547 | 536 | -11 | -2.011% |
| 1 | 10,691 | 10,680 | -11 | -0.103% |
| 2 | 547 | 536 | -11 | -2.011% |
| 3 | 69,631 | 68,736 | -895 | -1.285% |
| 4 | 4,019 | 4,010 | -9 | -0.224% |
| 5 | 895 | 886 | -9 | -1.006% |
| 6 | 531 | 522 | -9 | -1.695% |
| 7 | 895 | 886 | -9 | -1.006% |
| 8 | 531 | 522 | -9 | -1.695% |
| 9 | 158,799 | 158,710 | -89 | -0.056% |
| 10 | 295 | 288 | -7 | -2.373% |
| 11 | 375 | 366 | -9 | -2.400% |
| 12 | 375 | 366 | -9 | -2.400% |
| 13 | 563 | 556 | -7 | -1.243% |
| 14 | 202,779 | 202,994 | +215 | +0.106% |
| 15 | 6,443 | 6,368 | -75 | -1.164% |
| 16 | 1,343 | 1,336 | -7 | -0.521% |
| 17 | 33,167 | 33,172 | +5 | +0.015% |
| 18 | 123,235 | 123,234 | -1 | -0.001% |
| 19 | 2,439 | 2,418 | -21 | -0.861% |
| 20 | 398,523 | 394,764 | -3,759 | -0.943% |
| 21 | 9,303 | 9,306 | +3 | +0.032% |
| 22 | 100,055 | 100,052 | -3 | -0.003% |
| 23 | 33,951 | 33,944 | -7 | -0.021% |

The remaining absolute CCH priorities are task20, task3, then task14.  Task18
is now -1 ns, but that does not excuse task20's -3,759 ns.

## Full SCH timing

| Task | RTL ns | gem5 ns | Delta ns | Error |
| ---: | ---: | ---: | ---: | ---: |
| 0 | 676,287 | 676,300 | +13 | +0.002% |
| 1 | 79,131 | 79,140 | +9 | +0.011% |
| 2 | 55,227 | 54,930 | -297 | -0.538% |
| 3 | 471,571 | 477,072 | +5,501 | +1.167% |
| 4 | 30,175 | 31,018 | +843 | +2.794% |
| 5 | 11,059 | 10,964 | -95 | -0.859% |
| 6 | 3,795 | 3,786 | -9 | -0.237% |
| 7 | 1,059 | 1,056 | -3 | -0.283% |
| 8 | 330,955 | 329,864 | -1,091 | -0.330% |

The SCH priority is task3 by absolute delta, then task8 and task4.  Task4 has
the largest percentage, but changing it first would leave the larger
task3/task8 structural gaps untouched.

## Functional gates

- CCH: 10,253 instructions match the accepted control by strict task-local
  opcode/suffix order and every emitted element.
- SCH: 8,103 instructions match the accepted control by the same rule.
- LSU RAW/throughput/capacity: 68/68 cases pass; all 382 VINS files are
  byte-identical to the pre-change accepted suite.
- Task20 focused replay: 8,837/8,837 VINS exact for the accepted
  held-fall-through model.
- Task17 focused replay: 687/687 VINS exact.
- The rejected mask-row prototype is absent from source and the accepted
  binary.

CCH and SCH global dump filenames can differ when independent instructions
finish in another global order.  The task-local comparator is authoritative;
the global filename suffix is not architectural data.

## Evidence

- `evidence/a32_r611_held_fallthrough_20260813/cch_full/task_execution_timing_vs_rtl.json`
- `evidence/a32_r611_held_fallthrough_20260813/cch_full/vins_vs_control.json`
- `evidence/a32_r611_held_fallthrough_20260813/sch_full/task_execution_timing_vs_rtl.json`
- `evidence/a32_r611_held_fallthrough_20260813/sch_full/vins_vs_control.json`
- `evidence/a32_r611_held_fallthrough_20260813/focused/task20_timing_vs_rtl.json`
- `evidence/a32_r611_held_fallthrough_20260813/focused/task17_timing_vs_rtl.json`
- `evidence/a32_r611_held_fallthrough_20260813/lsu_suite/suite_results.json`
- `evidence/a32_r611_held_fallthrough_20260813/rtl_oracles/task20_bitalu_ready.txt`
- `evidence/a32_r611_held_fallthrough_20260813/rtl_oracles/task20_bitalu_commit.txt`
- `evidence/a32_r611_held_fallthrough_20260813/rtl_oracles/task20_mask_rr.txt`

## Next checkpoint

1. Introduce a logical 8-bit `mlen_t` mask-row representation, expanded to
   eight backing bytes for gem5's element dump semantics without changing
   values.
2. Accumulate BitALU mask bits by result-queue entry and perform interior
   `mask_jump` dequeue.
3. Model the separate mask read/write RR, including stable request ownership.
4. Re-run task20 first-divergence, task17, the 68-case LSU suite, then complete
   CCH/SCH.  No total-DAG or cross-task cancellation is an acceptance gate.
