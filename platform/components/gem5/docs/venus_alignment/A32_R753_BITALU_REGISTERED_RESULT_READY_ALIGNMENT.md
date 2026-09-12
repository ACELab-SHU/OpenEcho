# A32 R753 BitALU registered result-ready alignment

## Scope and verdict

This round aligns the ordinary BitALU operand-handshake/result-queue clock
boundary against `venus_bitalu_wrapper.sv`.  It does not change fixed CPU,
LSU-memory, VFU, or DAG latency and contains no task, sequence, PC, address,
or data special case.

The candidate binary is:

```
build/RISCV/gem5.debug
SHA256 bde3f1930c272e78f03539cb790c3ae2201f57c57c9f86ba60f83beff7b61377
```

The verdict remains **INCONCLUSIVE for complete microtiming alignment**.
Task20 improves materially and its previously divergent sequence 57--77
window is exact, but CCH still has independent LSU/CPU residuals and SCH is
unchanged with larger CAU/SerDiv/Shuffle/LSU residuals.

Permanent evidence is under:

```
evidence/a32_r753_bitalu_registered_result_ready_20260814/
```

## RTL-backed structural change

The RTL queue counter and operand FIFO usage are updated from one pre-edge
snapshot, but they have different consumers:

- the accepted BitALU A/B/mask handshake consumes operand FIFO-Q on that
  edge, so requester queue usage records the pop immediately;
- the combinational ordinary BitALU result writes result-queue D on that
  edge and becomes result-queue Q-visible on the following edge;
- a result grant updates result-queue D, while operand-ready on the same edge
  still reads result-queue Q.  Therefore a full-queue stall cannot be retried
  in the same gem5 tick after the grant.

The lane now represents these three facts separately.  The former same-tick
`serviceBitAluResultQueue()` admission retry was removed.  This prevents a
later serialized gem5 callback from observing result-count D as if it were Q,
while preserving the immediate operand FIFO pop needed by the stable per-bank
requester vector and persistent RR state.

The capacity checker was also taught that the explicit CAU D/Q model names
the same structural events `CAU explicit pipeline D enqueue` and
`CAU elastic pipeline/result queue full`.  This is a validation-parser update,
not a timing-model change.

## Focused task20 result

All 8,837 task20 VINS outputs are exact.  The preceding accepted point first
diverged at sequence 57/66; the new focused replay has exact fire, duration,
and recycle timing through sequence 77.  The first remaining lifecycle
difference is sequence 78 VSUB: fire is one cycle late and duration is one
cycle short, so its recycle realigns.  The first remaining recycle difference
is sequence 608 VSGT, one cycle early.

In the complete CCH DAG, task20 is 398,224 ns versus RTL 398,523 ns:
**-299 ns, -0.075%**.  The preceding accepted result was -1,451 ns/-0.364%,
so this structural change closes 1,152 ns without moving any other CCH task.

## Complete CCH result

The first-tile-start to last-task-epilogue span is 1,041,232 ns versus RTL
1,042,151 ns: **-919 ns, -0.088%**.  This aggregate is reported only as a
summary and is not an acceptance criterion.

| Task | RTL ns | gem5 ns | Delta ns | Error |
| ---: | ---: | ---: | ---: | ---: |
| 0 | 547 | 536 | -11 | -2.011% |
| 1 | 10,691 | 10,680 | -11 | -0.103% |
| 2 | 547 | 536 | -11 | -2.011% |
| 3 | 69,631 | 69,402 | -229 | -0.329% |
| 4 | 4,019 | 4,010 | -9 | -0.224% |
| 5 | 895 | 886 | -9 | -1.006% |
| 6 | 531 | 522 | -9 | -1.695% |
| 7 | 895 | 886 | -9 | -1.006% |
| 8 | 531 | 522 | -9 | -1.695% |
| 9 | 158,799 | 158,732 | -67 | -0.042% |
| 10 | 295 | 288 | -7 | -2.373% |
| 11 | 375 | 366 | -9 | -2.400% |
| 12 | 375 | 366 | -9 | -2.400% |
| 13 | 563 | 556 | -7 | -1.243% |
| 14 | 202,779 | 202,998 | +219 | +0.108% |
| 15 | 6,443 | 6,376 | -67 | -1.040% |
| 16 | 1,343 | 1,336 | -7 | -0.521% |
| 17 | 33,167 | 33,170 | +3 | +0.009% |
| 18 | 123,235 | 123,234 | -1 | -0.001% |
| 19 | 2,439 | 2,436 | -3 | -0.123% |
| 20 | 398,523 | 398,224 | -299 | -0.075% |
| 21 | 9,303 | 9,306 | +3 | +0.032% |
| 22 | 100,055 | 100,052 | -3 | -0.003% |
| 23 | 33,951 | 33,944 | -7 | -0.021% |

All 24 tasks complete and all 10,253 emitted VINS outputs are byte-exact to
the accepted functional baseline.  The largest remaining absolute task
residuals are task20 (-299 ns), task3 (-229 ns), and task14 (+219 ns).  The
larger percentage errors on tasks 0/2/10--13 are only 7--11 ns and retain the
common scalar CPU/task-boundary signature.

## Complete SCH held-out result

SCH is bit-for-bit and timing-identical to the preceding accepted point.  Its
span is 1,539,012 ns versus RTL 1,534,187 ns: **+4,825 ns, +0.314%**.

| Task | RTL ns | gem5 ns | Delta ns | Error |
| ---: | ---: | ---: | ---: | ---: |
| 0 | 676,287 | 676,300 | +13 | +0.002% |
| 1 | 79,131 | 79,140 | +9 | +0.011% |
| 2 | 55,227 | 54,930 | -297 | -0.538% |
| 3 | 471,571 | 477,032 | +5,461 | +1.158% |
| 4 | 30,175 | 31,078 | +903 | +2.993% |
| 5 | 11,059 | 10,964 | -95 | -0.859% |
| 6 | 3,795 | 3,786 | -9 | -0.237% |
| 7 | 1,059 | 1,056 | -3 | -0.283% |
| 8 | 330,955 | 329,864 | -1,091 | -0.330% |

All nine tasks complete and all 8,103 gem5 VINS outputs are exact to the
accepted functional baseline.  The retained RTL capture directly covers
7,256 VINS; the other 847 have no retained RTL dump and are not claimed as an
all-RTL comparison.

The first current lifecycle divergences remain:

- task2 sequence 45 VRANGE: duration/recycle +3 cycles;
- task3 sequence 0 VLOAD: duration/recycle -1 cycle, then sequence 1 fire
  +1 cycle; the long positive residual still accumulates mixed
  CAU/SerDiv/LSU and arbitration differences;
- task4 sequence 0 VLOAD: duration/recycle -1 cycle, then sequence 8 VDIV
  fire -1 cycle; the total points to SerDiv result D/Q and grant overlap;
- task8 sequence 2 VSHUFFLE: fire/recycle -2 cycles, then sequence 3 VSTORE
  duration -1 cycle; the repeated residual remains Shuffle/LSU contention.

## Regression gates

- task17 focused replay: 687 VINS exact; the accepted sequence-191 VLOAD
  duration -2-cycle and sequence-194 fire +1-cycle residuals do not regress;
- task18 complete-DAG execution: 123,234 ns versus RTL 123,235 ns (-1 ns);
- LSU RAW/throughput/capacity: 68/68 pass, and all 382 VINS files have the
  same path/content aggregate hash as R653k;
- BitALU, CAU, and SerDiv all reach depth 2/full; all 1/4/64 ns capacity-case
  outputs are exact to the 1 ns control;
- CCH 10,253 VINS and SCH 8,103 VINS are exact.

## Next boundaries

1. Preserve task20 sequence 34--77 and split sequence 78 into sequencer
   command acceptance, operand requester Q, four-bank grant, result D/Q, and
   retirement.  Sequence 78's fire/duration cancellation must not be counted
   as exact.
2. Independently resolve task20's sequence-0 VLOAD -1-cycle duration and the
   first persistent recycle difference at sequence 608.
3. Treat SCH independently: task3 CAU/SerDiv/LSU overlap first, task4 SerDiv
   D/Q second, and task8 Shuffle/LSU arbitration third.
4. Isolate the recurring 7--11 ns CCH short-task residual with scalar-only
   CPU/start/epilogue cases.  Do not fold it into vector latency.

