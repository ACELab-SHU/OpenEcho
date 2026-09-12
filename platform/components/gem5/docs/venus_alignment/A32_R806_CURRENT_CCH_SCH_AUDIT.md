# A32 R806 current CCH/SCH timing audit

## Verdict

The current binary is functionally stable but is not cycle-complete against
RTL.  Its SHA256 is
`6ffbc43dc11efbac07b6f0b2ae5b8c2810cf43f76dfc41e74403bc0acab69567`.
Task execution is compared at the same boundary: RTL `start execute` to
`execute complete`, and gem5 `tile_start` to `task_epilogue`.  Return DMA and
tile release are excluded from per-task errors.

The complete CCH span is RTL 1,042,151 ns versus gem5 1,042,572 ns:
**+421 ns, +0.040397%**.  The complete SCH span is RTL 1,534,187 ns versus
gem5 1,539,924 ns: **+5,737 ns, +0.373944%**.  Neither aggregate is an
acceptance criterion because both contain cancellation.

## CCH / nrPDCCH

| Task | RTL ns | gem5 ns | Delta ns | Error |
| ---: | ---: | ---: | ---: | ---: |
| 0 | 547 | 536 | -11 | -2.011% |
| 1 | 10,691 | 10,680 | -11 | -0.103% |
| 2 | 547 | 536 | -11 | -2.011% |
| 3 | 69,631 | 69,380 | -251 | -0.360% |
| 4 | 4,019 | 4,010 | -9 | -0.224% |
| 5 | 895 | 886 | -9 | -1.006% |
| 6 | 531 | 522 | -9 | -1.695% |
| 7 | 895 | 886 | -9 | -1.006% |
| 8 | 531 | 522 | -9 | -1.695% |
| 9 | 158,799 | 158,688 | -111 | -0.070% |
| 10 | 295 | 288 | -7 | -2.373% |
| 11 | 375 | 366 | -9 | -2.400% |
| 12 | 375 | 366 | -9 | -2.400% |
| 13 | 563 | 556 | -7 | -1.243% |
| 14 | 202,779 | 203,010 | +231 | +0.114% |
| 15 | 6,443 | 6,376 | -67 | -1.040% |
| 16 | 1,343 | 1,336 | -7 | -0.521% |
| 17 | 33,167 | 33,118 | -49 | -0.148% |
| 18 | 123,235 | 123,234 | -1 | -0.001% |
| 19 | 2,439 | 2,436 | -3 | -0.123% |
| 20 | 398,523 | 398,512 | -11 | -0.003% |
| 21 | 9,303 | 9,306 | +3 | +0.032% |
| 22 | 100,055 | 100,052 | -3 | -0.003% |
| 23 | 33,951 | 33,944 | -7 | -0.021% |

The largest independent absolute residuals are task3 (-251 ns), task14
(+231 ns), task9 (-111 ns), task15 (-67 ns), and task17 (-49 ns).  The common
-7 to -11 ns residual on very short scalar-heavy tasks produces the largest
percentages but is a CPU/task-boundary problem, not evidence for changing a
vector fixed latency.

The current first VINS lifecycle differences are:

- task3: sequence 0 VLOAD duration/recycle +8 cycles; first fire difference is
  sequence 5 VLOAD +9 cycles;
- task9: sequence 1 VSTORE duration/recycle +1 cycle; sequence 5 fire +2;
- task14: sequence 0 VLOAD duration/recycle +1 cycle; sequence 1 fire +1;
- task15: sequence 0 VLOAD duration/recycle -1 cycle; sequence 1 VSTORE fire -1;
- task17: sequence 1 VLOAD duration/recycle +1 cycle; sequence 9 VSEQ is the
  first fire difference at +1 cycle;
- task20 in the concurrent DAG: sequence 0 VLOAD duration/recycle -1 cycle and
  sequence 2 VSTORE fire -1.  In isolated replay, sequences 0--8,834 are exact;
  the first remaining difference is sequence 8,835 VBRDCST, 38 versus 40
  cycles.  Operand admission and BitALU issue are already exact there; the gap
  is the tagged requester hazard lifetime between per-lane retirement and the
  command-wide registered completion boundary.

## SCH / PDSCHDag2 held-out

| Task | RTL ns | gem5 ns | Delta ns | Error |
| ---: | ---: | ---: | ---: | ---: |
| 0 | 676,287 | 676,300 | +13 | +0.002% |
| 1 | 79,131 | 79,140 | +9 | +0.011% |
| 2 | 55,227 | 54,900 | -327 | -0.592% |
| 3 | 471,571 | 477,016 | +5,445 | +1.155% |
| 4 | 30,175 | 31,078 | +903 | +2.993% |
| 5 | 11,059 | 10,974 | -85 | -0.769% |
| 6 | 3,795 | 3,786 | -9 | -0.237% |
| 7 | 1,059 | 1,056 | -3 | -0.283% |
| 8 | 330,955 | 329,864 | -1,091 | -0.330% |

The main residuals are independent and partially cancel: task3 +5,445 ns,
task8 -1,091 ns, task4 +903 ns, and task2 -327 ns.  The first differences are:

- task2: sequence 45 VRANGE duration/recycle +3 cycles; sequence 49 VXOR fire
  +3 cycles, pointing at Shuffle/result arbitration rather than task entry;
- task3: sequence 0 VLOAD -1 cycle and sequence 1 fire +1 cycle are small; the
  +5.445 us error accumulates later through repeated CAU/SerDiv/LSU overlap;
- task4: sequence 0 VLOAD -1 cycle; sequence 8 VDIV fire -1 cycle, with the
  large task residual still centered on SerDiv D/Q, result enqueue, bank grant,
  and retirement behavior;
- task5: sequence 0 VLOAD -1 cycle and sequence 9 VLOAD fire -23 cycles;
- task8: sequence 2 SCATTER/VSHUFFLE fire and recycle -2 cycles; sequence 3
  VSTORE duration +1 cycle.  Repetition exposes Shuffle/LSU contention,
  stable tagged requesters, LSU priority, and persistent per-bank RR.

## Regression evidence and next priority

- CCH: all 24 tasks complete; 10,253 VINS are task-local/value exact to the
  accepted R770 baseline.
- SCH: all 9 tasks complete; 8,103 VINS are exact to the accepted R771
  baseline.  The retained RTL dumps directly cover 7,256; the remaining 847
  are not claimed as direct RTL comparisons.
- LSU RAW/throughput/capacity: 68/68 cases pass and all 382 emitted VINS files
  are byte-exact to the retained R752 suite control.

The next highest-value work is SCH task3's first sustained slope change across
CAU/SerDiv/LSU overlap, then SCH task4 SerDiv result D/Q, then SCH task8's
persistent Shuffle/LSU bank contention.  CCH should proceed independently with
task3/task14 LSU boundaries and task17 LSU/VSEQ.  Task20's isolated sequence
8,835 must be fixed with a registered, tagged command-wide retirement token;
an attempted reuse of the existing global retirement table made task20
414,068 ns and was rejected and removed.

Compact evidence is stored under
`evidence/a32_r806_current_cch_sch_20260814/`.
