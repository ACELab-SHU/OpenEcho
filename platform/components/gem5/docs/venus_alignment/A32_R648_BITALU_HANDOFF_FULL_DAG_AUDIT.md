# A32 R648 BitALU tagged handoff and full-DAG audit

Date: 2026-08-13

Status: **INCONCLUSIVE**. Functional regressions pass, but CCH task20 and
SCH task3/task4 still have material instruction-level timing error. The full
DAG result is not an acceptance criterion.

## Change accepted in this round

RTL `venus_operand_requester.sv` changes requester ownership from the final
VRF grant. The corresponding SRAM response can still be in flight and is
routed by its command tag. gem5 previously kept ordinary BitALU A/B ownership
until the final response had been pushed into the operand queue.

The accepted implementation therefore:

- carries ordinary BitALU A/B reads through the tagged response path;
- releases the requester after the final grant;
- drains the old tagged response while the next command owns requester Q;
- retains the accepted single-outstanding BitALU read throughput;
- does not change LSU or VFU fixed latency and has no task, sequence, opcode,
  PC, address, or data-value special case.

The first prototype also changed ordinary BitALU outstanding depth to two.
It made task20 sequence 1 one cycle early and changed the per-bank request
phase throughout the task, so it was rejected. Two later attempts to make
same-VFU RAW dependencies wait unconditionally for the three-stage global
hazard broadcast caused a new sequence-36 divergence and delayed sequence-52
fire by three cycles; both were also rejected.

## RTL oracle: task20 sequence 59/60

The direct RTL samples show that persistent per-bank RR is not the first
error in this window. At the edge corresponding to gem5 9852 ns, RTL bank 0
contains only BitALU result master 8. gem5 contains result master 8 and
BitALU-A requester master 0/1 traffic in a different phase, so a result beat
is delayed even though the RR algorithm selects correctly for the vector it
receives.

The two independent phase errors are:

1. BitALU-B command handoff was late because gem5 waited for the final VRF
   response. RTL captures the replacement command from the final grant.
2. BitALU-A for sequence 60 is early. RTL retains its hazard through the
   requester/global-hazard registered boundary and first grants it at the
   edge corresponding to 9850 ns; the accepted gem5 baseline first grants it
   at 9846 ns.

These errors have opposite sign and partially cancel in nearby instruction
durations. They must not be replaced with a fixed latency adjustment.

## Focused regression

| workload | functional result | first duration divergence | first fire divergence |
|---|---:|---|---|
| CCH task17 | 687/687 VINS exact | seq180 VBRDCST, -1 cycle | seq194 VLOAD, +1 cycle |
| CCH task20 | 8,837/8,837 VINS exact | seq59 VBRDCST, +1 cycle | seq63 VXOR, +1 cycle |

For task20, sequence 0 through 58 remains exact for fire, duration, and
recycle. The final task20 fire/recycle phase is +4,605 tile cycles, showing
that the one-cycle first divergence repeats and amplifies through the later
loop rather than behaving as a single fixed offset.

## Full CCH timing

Comparison boundary is gem5 `tile_start -> task_epilogue` against RTL
`start execute -> execute complete`. Return DMA and tile release are excluded.

| task | RTL us | gem5 us | delta us | error |
|---:|---:|---:|---:|---:|
| 0 | 0.547 | 0.536 | -0.011 | -2.011% |
| 1 | 10.691 | 10.680 | -0.011 | -0.103% |
| 2 | 0.547 | 0.536 | -0.011 | -2.011% |
| 3 | 69.631 | 68.736 | -0.895 | -1.285% |
| 4 | 4.019 | 4.010 | -0.009 | -0.224% |
| 5 | 0.895 | 0.886 | -0.009 | -1.006% |
| 6 | 0.531 | 0.522 | -0.009 | -1.695% |
| 7 | 0.895 | 0.886 | -0.009 | -1.006% |
| 8 | 0.531 | 0.522 | -0.009 | -1.695% |
| 9 | 158.799 | 158.710 | -0.089 | -0.056% |
| 10 | 0.295 | 0.288 | -0.007 | -2.373% |
| 11 | 0.375 | 0.366 | -0.009 | -2.400% |
| 12 | 0.375 | 0.366 | -0.009 | -2.400% |
| 13 | 0.563 | 0.556 | -0.007 | -1.243% |
| 14 | 202.779 | 202.994 | +0.215 | +0.106% |
| 15 | 6.443 | 6.368 | -0.075 | -1.164% |
| 16 | 1.343 | 1.336 | -0.007 | -0.521% |
| 17 | 33.167 | 33.172 | +0.005 | +0.015% |
| 18 | 123.235 | 123.234 | -0.001 | -0.001% |
| 19 | 2.439 | 2.418 | -0.021 | -0.861% |
| 20 | 398.523 | 407.724 | +9.201 | +2.309% |
| 21 | 9.303 | 9.306 | +0.003 | +0.032% |
| 22 | 100.055 | 100.052 | -0.003 | -0.003% |
| 23 | 33.951 | 33.944 | -0.007 | -0.021% |

The aligned first-start to last-epilogue span is RTL 1,042.151 us versus gem5
1,050.684 us: +8.533 us, +0.819%. This value is dominated by task20 and is
not evidence of model completeness.

CCH emits 10,253 VINS files and is task-local byte-exact against the accepted
functional baseline. Task20 is now the dominant timing error. Task3 is the
next absolute error; its first divergence is sequence 0 VLOAD +8 cycles, but
its final phase is -442 cycles, so later CAU/Shuffle overlap reverses the
sign. The 7--11 ns residual shared by many short tasks is a separate
CPU/task-boundary issue and should not be mixed with vector timing.

## Full SCH timing

| task | RTL us | gem5 us | delta us | error |
|---:|---:|---:|---:|---:|
| 0 | 676.287 | 676.300 | +0.013 | +0.002% |
| 1 | 79.131 | 79.140 | +0.009 | +0.011% |
| 2 | 55.227 | 54.930 | -0.297 | -0.538% |
| 3 | 471.571 | 477.072 | +5.501 | +1.167% |
| 4 | 30.175 | 31.018 | +0.843 | +2.794% |
| 5 | 11.059 | 10.964 | -0.095 | -0.859% |
| 6 | 3.795 | 3.786 | -0.009 | -0.237% |
| 7 | 1.059 | 1.056 | -0.003 | -0.283% |
| 8 | 330.955 | 329.864 | -1.091 | -0.330% |

The aligned full-DAG span is RTL 1,534.187 us versus gem5 1,538.992 us:
+4.805 us, +0.313%. There is clear task cancellation: task3/task4 are slow
while task2/task5/task8 are fast.

SCH emits 8,103 VINS files and is task-local byte-exact against the accepted
functional baseline. The historical direct RTL dump covers 7,256 of these;
the other 847 still have no RTL dump and are not represented as an all-RTL
comparison.

The primary timing signatures are:

- task3: first duration difference sequence 0 VLOAD -1 cycle; first fire
  difference sequence 1 VLOAD +1 cycle; final phase +2,753 cycles. Repeated
  VDIV, VMUL, VSADD and store overlap dominate the accumulated delay.
- task4: first duration difference sequence 0 VLOAD -1 cycle; first fire
  difference sequence 8 VDIV -5 cycles; final phase +425 cycles. SerDiv/CAU
  requester and result arbitration remain incomplete.
- task8: first fire difference sequence 2 VSHUFFLE -2 cycles; final recycle
  phase -521 cycles. Shuffle admission/live intent and its interaction with
  LSU priority remain the leading source.

## Directed regressions

- LSU RAW/throughput/capacity: 68/68 processes pass.
- LSU VINS: 382/382 files byte-exact against the accepted suite.
- BitALU/CAU/SerDiv depth-2/full coverage from the preceding accepted
  baseline remains applicable; this round did not replace those queues.
- Final binary SHA256:
  `03831f6e152e2c6eb80360a8394e1518f5899db25ed239f10c2316e19f0428d4`.

## Next hardware boundary

The next change must not be a latency fit. Capture and compare, on the same
RTL edges, the following state for task20 sequence 60 and an earlier
non-regressing control window such as sequence 34--37:

1. requester command capture and source/destination hazard bitmap;
2. `raw_hazard_counter_q`, registered writer VFU/address/valid metadata, and
   `global_hazard_table_i`;
3. operand-queue ready and exact four-bank requester vector;
4. persistent per-bank RR Q, LSU high-priority request, winner, and result
   enqueue/dequeue;
5. tagged retirement generation.

gem5 currently folds source RAW and destination WAR/WAW requester state into
shared `chain_raw_hazard` bookkeeping. The rejected experiments show that a
single retirement shortcut or a uniform three-stage wait is insufficient.
Only an RTL oracle that distinguishes those bits may justify splitting the
model.

## Evidence

- `evidence/a32_r648_bitalu_handoff_20260813/cch_full/`
- `evidence/a32_r648_bitalu_handoff_20260813/sch_full/`
- `evidence/a32_r648_bitalu_handoff_20260813/focused/`
- `evidence/a32_r648_bitalu_handoff_20260813/lsu_suite/`
- `evidence/a32_r648_bitalu_handoff_20260813/rtl_oracle/`

