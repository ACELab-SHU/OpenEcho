# A32 R785 BitALU full-queue operand-admission alignment

## Scope and verdict

This round aligns the masked BitALU operand handshake with the RTL
`result_queue_full` boundary.  It does not change CPU, LSU, or VFU fixed
latency and contains no task, sequence, PC, opcode, address, or data special
case.

The candidate binary SHA256 is
`5a7e9b51265f92d0e3edffe0b4377790c77cf6cb2b0b9ee61af854f460ee9b73`.
The change fixes a real local timing error, but complete microtiming alignment
is still **not achieved**: task20's first lifecycle divergence moves from
sequence 608 to sequence 647, while complete-task timing remains unchanged.

Permanent evidence is under:

```
evidence/a32_r785_bitalu_full_admission_20260814/
```

## RTL-backed structural fix

In `venus_bitalu_wrapper.sv`, normal BitALU operand admission and the mask
operand D/Q handshake are both inside `!result_queue_full`.  gem5 previously
allowed a masked instruction to capture its mask-D while the two-entry result
queue was full.  That made mask-Q and the first arithmetic result visible one
lane edge before RTL.

`VenusLane.cc` now calls `prepareBitAluMaskOperand()` only when
`bitAluOperandAdmissionReady()` is true.  This also prevents A/B operands from
being consumed on that full-Q edge.  Reductions retain their existing path.

The compact RTL oracle for task20 sequence 608 directly observes:

- result-count Q is 2/full at oracle edge 25, with no mask handshake;
- result-count Q falls to 1 at edge 26 and mask-D is accepted;
- mask-Q and the first sequence-608 result appear at edge 27;
- the final mask result is D at edge 35, grant/done-D at edge 36, and done-Q
  at edge 37.

After the fix, sequence 608 VSGT fire, duration, and recycle are all exact.
The new first difference is sequence 647 VSTORE duration/recycle `-2 cycles`;
sequence 648 VLOAD is the first fire difference at `-2 cycles`.  All 8,837
task20 VINS remain byte-exact to the accepted run.

## Complete CCH timing

The complete-DAG span remains RTL 1,042,151 ns versus gem5 1,041,232 ns:
**-919 ns, -0.088%**.  The unchanged aggregate is not an acceptance result;
the local sequence-608 correction is absorbed before the task boundary.

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

All 24 tasks complete and all 10,253 VINS are exact to the accepted baseline.
In the concurrent full-DAG context, task17 first differs at sequence 1 VLOAD
duration/recycle `+1 cycle`; sequence 9 VSEQ is the first fire difference at
`+1 cycle`.  Task18 has no emitted VINS dump and its task boundary remains
`-1 ns`.

## Complete SCH held-out timing

The complete-DAG span remains RTL 1,534,187 ns versus gem5 1,539,012 ns:
**+4,825 ns, +0.314%**.

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

All nine tasks complete and all 8,103 gem5 VINS are exact to the accepted
baseline.  The retained RTL capture directly covers 7,256 VINS; 847 have no
retained RTL dump and are not claimed as an all-RTL comparison.

## Regression gates and next boundary

- LSU RAW/throughput/capacity: 68/68 processes pass; 382 VINS dumps emitted.
- BitALU, CAU, and SerDiv each reach depth 2/full; all 1/4/64 ns outputs are
  exact to the 1 ns control.
- Next CCH boundary: task20 sequence 647 VSTORE completion and sequence 648
  VLOAD issue; task17 LSU/VSEQ remains independent.
- Next SCH boundary: task3 CAU/SerDiv/LSU overlap first, task4 SerDiv D/Q
  second, and task8 Shuffle/LSU arbitration third.
