# A32 R809 Shuffle LockIn=0 live-intent alignment

## Verdict

The current CCH and SCH results remain functionally exact to the accepted
gem5 baselines, but they are not cycle-complete against RTL.  The full CCH
critical path is RTL 1,042,151 ns versus gem5 1,042,316 ns, or **+165 ns /
+0.015833%**.  The full held-out SCH path is RTL 1,534,187 ns versus gem5
1,539,948 ns, or **+5,761 ns / +0.375508%**.  These totals are reported only
as context: task-local positive and negative residuals remain independent
acceptance failures.

Per-task execution uses the same boundaries as the prior audit: RTL
`start execute` to `execute complete`, and gem5 `tile_start` to
`task_epilogue`.  DMA return and tile release are excluded.

## Structural change and RTL oracle

The first task20 sequence-40 mismatch was reduced to a single bank-arbiter
edge.  RTL's local `rr_arb_tree` is instantiated with `LockIn=0`; while a
request is denied by the higher-priority LSU input, the requester vector may
change and the local winner is recomputed.  The old gem5 timing retry retained
the previously selected PE, so RTL selected PE0 while gem5 continued with PE8
at the first differing edge.

The existing generation-tagged Shuffle live-intent path now becomes the
default whenever the RTL profile also enables deferred per-bank RR.  A
standalone profile without per-bank RR keeps it disabled, preserving the LSU
micro-suite's topology contract.  No LSU/VFU fixed latency and no task, PC,
DAG, address, or data special case was added.

In isolated task20, sequence 40 through sequence 49 regain exact
fire/recycle/duration lifecycles.  The first remaining lifecycle difference
is sequence 50 `VBRDCST`: gem5 admits it 154 ns early, but recycle is already
equal, so the next work item is the command-register/tag visibility boundary,
not another Shuffle fixed-latency adjustment.

An attempted blanket rule that held every same arithmetic-VFU dependency in
the sequencer was rejected.  It moved task20's first difference backward to
sequence 31 and increased the replay from 398,232 ns to 440,036 ns.  The
candidate was removed and the accepted replay returned to 398,232 ns.

## CCH / nrPDCCH current task timing

| Task | RTL ns | gem5 ns | Delta ns | Error |
| ---: | ---: | ---: | ---: | ---: |
| 0 | 547 | 536 | -11 | -2.011% |
| 1 | 10,691 | 10,680 | -11 | -0.103% |
| 2 | 547 | 536 | -11 | -2.011% |
| 3 | 69,631 | 69,732 | +101 | +0.145% |
| 4 | 4,019 | 4,010 | -9 | -0.224% |
| 5 | 895 | 886 | -9 | -1.006% |
| 6 | 531 | 522 | -9 | -1.695% |
| 7 | 895 | 886 | -9 | -1.006% |
| 8 | 531 | 522 | -9 | -1.695% |
| 9 | 158,799 | 158,688 | -111 | -0.070% |
| 10 | 295 | 288 | -7 | -2.373% |
| 11 | 375 | 366 | -9 | -2.400% |
| 12 | 375 | 366 | -9 | -2.400% |
| 13 | 563 | 618 | +55 | +9.769% |
| 14 | 202,779 | 203,010 | +231 | +0.114% |
| 15 | 6,443 | 6,408 | -35 | -0.543% |
| 16 | 1,343 | 1,336 | -7 | -0.521% |
| 17 | 33,167 | 33,118 | -49 | -0.148% |
| 18 | 123,235 | 123,234 | -1 | -0.001% |
| 19 | 2,439 | 2,436 | -3 | -0.123% |
| 20 | 398,523 | 398,224 | -299 | -0.075% |
| 21 | 9,303 | 9,306 | +3 | +0.032% |
| 22 | 100,055 | 100,052 | -3 | -0.003% |
| 23 | 33,951 | 33,944 | -7 | -0.021% |

The largest independent absolute CCH residuals are task20 -299 ns, task14
+231 ns, task9 -111 ns, task3 +101 ns, task13 +55 ns, and task17 -49 ns.
Task13 has the largest percentage only because it is 563 ns long; its 55 ns
absolute gap still matters, but it does not outrank the longer task20/task14
micro-timing investigations.  The recurring -7 to -11 ns short-task offset
continues to point at the CPU/task-boundary surface.

## SCH / PDSCHDag2 held-out current task timing

| Task | RTL ns | gem5 ns | Delta ns | Error |
| ---: | ---: | ---: | ---: | ---: |
| 0 | 676,287 | 676,300 | +13 | +0.002% |
| 1 | 79,131 | 79,140 | +9 | +0.011% |
| 2 | 55,227 | 54,900 | -327 | -0.592% |
| 3 | 471,571 | 477,004 | +5,433 | +1.152% |
| 4 | 30,175 | 31,102 | +927 | +3.072% |
| 5 | 11,059 | 10,988 | -71 | -0.642% |
| 6 | 3,795 | 3,786 | -9 | -0.237% |
| 7 | 1,059 | 1,056 | -3 | -0.283% |
| 8 | 330,955 | 329,864 | -1,091 | -0.330% |

The main SCH residual is still task3 +5,433 ns, accumulated through sustained
CAU/SerDiv/LSU overlap.  Task4 +927 ns remains centered on SerDiv D/Q, result
enqueue, per-bank grant, and retirement.  Task8 -1,091 ns remains the best
oracle for persistent Shuffle/LSU contention, LSU priority, and per-bank RR.
Task2 -327 ns points at the later Shuffle/result-arbitration region.  These
opposite signs partially cancel in the full-DAG number and must be closed
independently.

## Regression evidence

- CCH: 24/24 tasks; 10,253 VINS exact to the accepted R770 baseline.
- SCH: 9/9 tasks; 8,103 VINS exact to the accepted R771 baseline.  Existing
  direct RTL dumps cover 7,256 of them; the other 847 are not presented as a
  direct RTL comparison.
- LSU RAW/throughput/capacity: 68/68; 382 VINS files byte-exact to the retained
  control.
- BitALU/CAU/SerDiv result queues each reached depth 2 and a real full state,
  with outputs exact to their control.

The full-chain runs used binary SHA256
`43e89c45c7c24e85c07e2d26e70542577a68bcb0d7754a2090ebc2eb35b63511`.
After rejecting and reverting the same-VFU candidate, the rebuilt binary is
`101f3ef799c8bd26216b3c4d89666e805e390d20ad1be8e564eb605511cc6df6`;
the accepted task20 replay returned to 398,232 ns and all 8,837 VINS remained
exact.  The hash differs because gem5 embeds build metadata.

Compact machine-readable timing and regression evidence is in
`evidence/a32_r809_shuffle_live_intent_20260814/`.
