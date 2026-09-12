# A32 R909 TASK_DONE tile-manager boundary alignment

Date: 2026-08-15

## Verdict

`INCONCLUSIVE` for complete RTL equivalence. This round accepts one generic
hardware boundary: a scalar `TASK_DONE` store no longer causes a combinational
Gem5 task exit. It enters the tile manager, crosses two registered
selected-clock edges, and then asks Minor Execute to generate its native
`SuspendThread` stream redirect on a CPU edge.

There is no task ID, DAG name, PC, instruction ordinal, payload pattern beyond
the architectural zero write, or workload-specific duration adjustment. VEMU,
scheduler/L1 packaging, RTL, vector LSU fixed latency, and VFU fixed latency
were not changed.

## RTL evidence and old-model defect

`venus_tile_manager.sv` clears `venustile_softresetreg` on the control-register
write, propagates it through `venustile_softresetreg_z1`, and exposes
`soft_reset_n_o1`. At the same boundary,
`tile_soft_reset_n_for_logic_o1` changes the selected block clock back to the
250 MHz AXI clock. The lifecycle monitor observes the falling reset on the
following selected-clock edge.

All 33 held-out CCH/SCH tasks show the same boundary family:

| RTL boundary | Distribution |
|---|---:|
| TASK_DONE request to scalar store retire | 2 ns: 33/33 |
| TASK_DONE request to task complete | 7 ns: 5, 9 ns: 20, 11 ns: 8 |
| scalar store retire to task complete | 5 ns: 5, 7 ns: 20, 9 ns: 8 |

R908 called `requestTaskExit()` while Minor created the LSQ request. The thread
was suspended immediately; the later TASK_DONE store response was consequently
discarded as belonging to the old stream. That was both early and
architecturally wrong.

R909 adds `Running -> TaskDonePending -> Draining`. The pending deadline is the
second scheduler/AXI clock edge after the request. Direct suspension from the
scheduler was rejected: CCH task9 proved that a Default-priority external event
can precede the same-tick Minor commit and leave a matching-stream instruction
at the head. The accepted implementation presents a pending suspend request to
Minor Execute; Execute atomically suspends the context and emits the normal
`BranchData::SuspendThread` redirect on its CPU edge. Output drain/capture does
not begin before that edge.

The resulting task change is uniformly +8 or +10 ns, depending only on clock
phase. Already-slow tasks also move by the same amount, which is expected for a
shared hardware boundary and rules out error-directed task compensation.

## Final SCH task timing

Boundary: RTL `task_start -> task_complete`; Gem5 `tile_start -> task_epilogue`.
Return DMA and tile release are excluded.

| task | RTL ns | Gem5 ns | delta ns | error |
|---:|---:|---:|---:|---:|
| 0 | 676287 | 676300 | +13 | +0.001922% |
| 1 | 79131 | 79144 | +13 | +0.016428% |
| 2 | 55227 | 55228 | +1 | +0.001811% |
| 3 | 471571 | 471560 | -11 | -0.002333% |
| 4 | 30175 | 30164 | -11 | -0.036454% |
| 5 | 11059 | 11108 | +49 | +0.443078% |
| 6 | 3795 | 3792 | -3 | -0.079051% |
| 7 | 1059 | 1060 | +1 | +0.094429% |
| 8 | 330955 | 330976 | +21 | +0.006345% |

The sum of absolute per-task deltas improves from 131 ns to 123 ns. The SCH
execution envelope moves from `-343 ns/-0.022357%` to
`-267 ns/-0.017403%`. This aggregate is diagnostic only: task5 is now the
largest task-local residual at +49 ns.

## Final CCH task timing

| task | RTL ns | Gem5 ns | delta ns | error |
|---:|---:|---:|---:|---:|
| 0 | 547 | 540 | -7 | -1.279707% |
| 1 | 10691 | 10684 | -7 | -0.065476% |
| 2 | 547 | 540 | -7 | -1.279707% |
| 3 | 69631 | 69384 | -247 | -0.354727% |
| 4 | 4019 | 4016 | -3 | -0.074645% |
| 5 | 895 | 892 | -3 | -0.335196% |
| 6 | 531 | 528 | -3 | -0.564972% |
| 7 | 895 | 892 | -3 | -0.335196% |
| 8 | 531 | 528 | -3 | -0.564972% |
| 9 | 158799 | 158668 | -131 | -0.082494% |
| 10 | 295 | 292 | -3 | -1.016949% |
| 11 | 375 | 372 | -3 | -0.800000% |
| 12 | 375 | 372 | -3 | -0.800000% |
| 13 | 563 | 560 | -3 | -0.532860% |
| 14 | 202779 | 202804 | +25 | +0.012329% |
| 15 | 6443 | 6432 | -11 | -0.170728% |
| 16 | 1343 | 1340 | -3 | -0.223380% |
| 17 | 33167 | 33164 | -3 | -0.009045% |
| 18 | 123235 | 123236 | +1 | +0.000811% |
| 19 | 2439 | 2440 | +1 | +0.041000% |
| 20 | 398523 | 398520 | -3 | -0.000753% |
| 21 | 9303 | 9308 | +5 | +0.053746% |
| 22 | 100055 | 100056 | +1 | +0.000999% |
| 23 | 33951 | 33948 | -3 | -0.008836% |

Sixteen CCH tasks are now within 3 ns and twenty are within 7 ns. The sum of
absolute task deltas falls from 674 ns to 482 ns, a 28.49% improvement. The
CCH execution envelope moves from `-871 ns/-0.083577%` to
`-771 ns/-0.073982%`. The remaining task3 -247 ns and task9 -131 ns are not
task-completion effects and must be split earlier in their scalar/vector
lifecycles. Task14's +25 ns is a held-out counterexample against adding more
global tail latency.

## Regression gates

- SCH: 22/22 output files and 8,103/8,103 VINS dumps byte-exact to R908.
- CCH: 53/53 output files and 10,253/10,253 VINS dumps byte-exact to R908.
- scalar600: 52/52 executions; frozen RTL oracle 51/52, with the existing
  `ecall_nop` fixture exclusion; 1,258 retire and 1,258 writeback records exact.
- vector LSU: 68/68 RAW/throughput/capacity cases; 382/382 VINS exact to R908.
- BitALU, CAU, and SerDiv result queues all reached depth 2/full; functional
  output is exact across 1/4/64 ns grant-gap controls.
- final binary SHA256:
  `83036b069e406633f725acad38865f4222992a283d1a817a48234f7831632b41`.

Permanent evidence is under
`evidence/a32_r909_taskdone_tile_manager_20260815/`.

## Next breakpoints

1. SCH task5 +49 ns: split scalar entry, its vector region, and scalar epilogue;
   task2's exact vector lifecycle remains a held-out control.
2. CCH task3 -247 ns and task9 -131 ns: find the first scalar retire or VINS
   admission divergence rather than changing task tail or fixed VFU latency.
3. CCH task15 -11 ns: determine whether its remaining tail differs from the
   now-modeled TASK_DONE family or originates earlier.
4. Preserve task17/task20 at -3 ns while continuing direct per-bank requester,
   LSU priority, and persistent RR structural checks.
