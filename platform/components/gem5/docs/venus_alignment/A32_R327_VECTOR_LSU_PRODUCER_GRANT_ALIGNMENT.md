# A32 R327 vector LSU producer-grant alignment

Date: 2026-08-12

Status: **INCONCLUSIVE**.  This checkpoint closes the first task17 VSTORE
divergence and preserves functional results, but it does not close task17 as a
whole, task20, task18, or the SCH held-out residuals.

## Scope and structural changes

This round makes three general, RTL-sourced changes.  It contains no task,
PC, DAG, address-value, payload-value, or workload special case, and it does
not change the fixed LSU/VFU response latencies.

1. `scheduleLsuDone()` now derives the number of 64-byte AXI words from
   `addr[5:0] + payload_bytes`.  This matches `addrgen.sv`, where
   `lookahead_len = (vl << vew) + addr[5:0]`; an unaligned transfer whose tail
   crosses a 64-byte boundary therefore emits another beat.
2. VSTU operand-ready to the first tile-local W handshake is one tile clock,
   not two.  The task17 sequence-96 RTL oracle observes `stu_req` at
   569573 ns and the local W handshake at 569575 ns.  After this correction,
   gem5 local-W to internal-B remains 44 ns, exactly matching RTL; no AXI B
   or return-CDC constant was changed.
3. A tagged lane-producer completion sideband separates the final destination
   bank grant from the later PE response/running-ID recycle.  It aggregates
   `{instruction generation, running ID, active lane}` final grants and only
   updates the sequencer-local producer tombstone used by LSU operand
   admission.  Short vectors wait only for the sequencer lane-use board, not
   all 16 physical lanes.  VFU retirement, lane done reports, VINS dump,
   running-ID release, and registered hazard broadcasts keep their previous
   boundaries.

The resulting binary is:

```text
f91a2a60f3a5670659c8f9308c7644a1efd63a6c2bd1b85c75c18884ee648030
```

## task17 first-divergence result

Before this round, task17 sequence 0--95 was lifecycle exact and sequence 96
VSTORE was 56 cycles in gem5 versus 52 in RTL.  It is now exact:

| sequence | op | gem5 fire/duration/recycle | RTL fire/duration/recycle |
| ---: | --- | --- | --- |
| 94 | VMUL | 713 / 37 / 750 | 713 / 37 / 750 |
| 95 | VSADD | 719 / 39 / 758 | 719 / 39 / 758 |
| 96 | VSTORE | 730 / 52 / 782 | 730 / 52 / 782 |
| 97 | VLOAD | 797 / 35 / 832 | 797 / 35 / 832 |
| 98 | VLOAD | 854 / 34 / 888 | 854 / 34 / 888 |
| 99 | VLOAD | 862 / 34 / 896 | 862 / 34 / 896 |
| 100 | VSHUFFLE | 907 / 66 / 973 | 907 / 66 / 973 |

Thus sequence 0--100 is lifecycle exact.  The first remaining divergence is
sequence 101 VXOR: gem5 duration/recycle are 63/980 cycles versus RTL 64/981,
both with fire at 917.  The first fire divergence is sequence 109, gem5 1044
versus RTL 1043.  This is the next bounded requester/result-queue target.

For sequence 96, the narrow trace proves the intended separation:

- all active-lane final grants for producer sequence 95: 3712 ns;
- producer VFU retirement: 3714 ns;
- sequence 96 StoreOperands observes the tombstone: 3714 ns;
- external final W / internal B / completion: 3732/3760/3764 ns.

The task17 CCH execution error changes from `+77 ns` to `-87 ns`; the exact
sequence result above, rather than that signed task total, is the acceptance
criterion.

## CCH task execution timing

Boundary: RTL `start execute -> execute complete`, gem5
`tile_start -> task_epilogue`.  Return DMA and tile release are excluded from
both execution durations.

| task | RTL ns | gem5 ns | delta ns | error |
| ---: | ---: | ---: | ---: | ---: |
| 0 | 547 | 540 | -7 | -1.280% |
| 1 | 10,691 | 10,890 | +199 | +1.861% |
| 2 | 547 | 540 | -7 | -1.280% |
| 3 | 69,631 | 69,820 | +189 | +0.271% |
| 4 | 4,019 | 4,014 | -5 | -0.124% |
| 5 | 895 | 886 | -9 | -1.006% |
| 6 | 531 | 526 | -5 | -0.942% |
| 7 | 895 | 886 | -9 | -1.006% |
| 8 | 531 | 526 | -5 | -0.942% |
| 9 | 158,799 | 158,914 | +115 | +0.072% |
| 10 | 295 | 288 | -7 | -2.373% |
| 11 | 375 | 370 | -5 | -1.333% |
| 12 | 375 | 370 | -5 | -1.333% |
| 13 | 563 | 612 | +49 | +8.703% |
| 14 | 202,779 | 203,796 | +1,017 | +0.502% |
| 15 | 6,443 | 6,430 | -13 | -0.202% |
| 16 | 1,343 | 1,358 | +15 | +1.117% |
| 17 | 33,167 | 33,080 | -87 | -0.262% |
| 18 | 123,235 | 126,700 | +3,465 | +2.812% |
| 19 | 2,439 | 2,460 | +21 | +0.861% |
| 20 | 398,523 | 388,514 | -10,009 | -2.512% |
| 21 | 9,303 | 9,434 | +131 | +1.408% |
| 22 | 100,055 | 100,060 | +5 | +0.005% |
| 23 | 33,951 | 33,944 | -7 | -0.021% |

Relative to the previous checkpoint, task17 changes by -164 ns, task14 by
-92 ns, and task20 by -296 ns.  task20 is not accepted: its initial scalar
loop still puts sequence 2 fire at +163 cycles, while the long vector/LSU body
then drifts in the opposite direction.  The more negative task total must not
be used to reject the now source-exact VSTU boundary or to hide the two
independent task20 defects.

## SCH held-out task execution timing

| task | RTL ns | gem5 ns | delta ns | error |
| ---: | ---: | ---: | ---: | ---: |
| 0 | 676,287 | 676,304 | +17 | +0.003% |
| 1 | 79,131 | 79,144 | +13 | +0.016% |
| 2 | 55,227 | 56,876 | +1,649 | +2.986% |
| 3 | 471,571 | 478,480 | +6,909 | +1.465% |
| 4 | 30,175 | 31,538 | +1,363 | +4.517% |
| 5 | 11,059 | 11,184 | +125 | +1.130% |
| 6 | 3,795 | 3,784 | -11 | -0.290% |
| 7 | 1,059 | 1,056 | -3 | -0.283% |
| 8 | 330,955 | 328,878 | -2,077 | -0.628% |

The structural changes reduce task3/task4/task5 by 108/20/8 ns and task8 by
1,394 ns.  task2/3/4 and task8 remain independent held-out targets; their
opposite signs are not an acceptance criterion.

## Regression gates and evidence

- task17: 687 VINS byte-exact; sequence 0--100 lifecycle exact;
- focused task20: 8,837 VINS byte-exact; initial sequence-2 fire divergence
  remains +163 cycles and is not mislabeled as a vector-LSU fix;
- LSU RAW/throughput/capacity: 68/68 processes pass, 382 VINS files byte-exact
  against the accepted suite;
- BitALU/CAU/SerDiv result queues pass the 1/4/16/32/64/128 ns grant-gap
  sweep; every queue reaches depth=2/full in the applicable stalled cases and
  all six functional outputs match the 1 ns control;
- CCH: 10,253 task-local VINS exact against the accepted checkpoint;
- SCH: 8,103 task-local VINS exact against the accepted checkpoint.

Permanent machine-readable evidence is in:

```text
evidence/a32_r327_vector_lsu_alignment_20260812/
```

The full run directories remain in `/tmp/a32_r324_*` through
`/tmp/a32_r328_*`.

## Next work

1. Split task17 sequence-101 VXOR into operand admission, result enqueue,
   per-bank grant, requester-q visibility, and retirement; retain sequence
   0--100 as a hard non-regression prefix.
2. Keep task20's early scalar `branch -> lhu -> dependent branch` defect
   separate from its later stable requester/LSU-priority/persistent per-bank
   RR drift.  Do not fit either with a VSTU or VFU fixed latency.
3. Analyze CCH task18 and SCH task2/3/4/8 independently, always retaining the
   68-case LSU and task-local VINS gates.
