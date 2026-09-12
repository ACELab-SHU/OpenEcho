# A32 R767 physical operand-command validity alignment

## Scope and verdict

This round separates the functional operand placeholders used by gem5 from
the physical `operand_req_valid_o` registers implemented by RTL.  It does not
change CPU, LSU, VFU fixed latency and contains no task, sequence, PC, opcode,
address, or data special case.

The candidate binary SHA256 is
`12bf4940979ad2f2ad5536ee0292b0f59e87674aa9ccad40df982b3c81e56023`.
The verdict remains **INCONCLUSIVE for complete microtiming alignment**:
task20 sequence 78 is now exact, but the total CCH/SCH task times do not move
because later independent timing residuals remain.

Permanent evidence is under:

```
evidence/a32_r767_physical_operand_command_validity_20260814/
```

## RTL-backed structural change

`venus_lane_sequencer.sv` only pushes a physical operand command when the
corresponding operand is used (`use_vs1`, `use_vs2`, `use_vd1_op`,
`use_vd2_op`, or mask read/write).  gem5 still needs neutral placeholders in
its software queues for functional A/B pairing.  Treating those placeholders
as physical command-valid bits introduced false admission backpressure.

The lane now checks whether a queue contains a physically valid command when
deciding whether a one-entry RTL command register is occupied.  An unused
placeholder is also prevented from advancing the registered command-ack
boundary.  Functional placeholder consumption is unchanged.

## Focused microtiming result

For CCH task20, sequence 78 VSUB changes from fire `+1 cycle`, duration
`-1 cycle` to exact fire/duration/recycle timing.  The first remaining
lifecycle difference is sequence 608 VSGT recycle/duration `-1 cycle`; the
first remaining fire difference is sequence 612 `-1 cycle`.  All 8,837 VINS
remain exact.

Task17 retains its accepted first residuals: sequence 191 VLOAD duration and
recycle `-2 cycles`, followed by sequence 194 VLOAD fire `+1 cycle`; all 687
VINS remain exact.  Complete-DAG task18 remains `-1 ns`.

## Complete CCH result

The full-DAG span remains RTL 1,042,151 ns versus gem5 1,041,232 ns:
**-919 ns, -0.088%**.  This aggregate is not an acceptance criterion.

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
Task3 first diverges immediately at sequence 0 VLOAD (duration/recycle
`+8 cycles`) although its final task delta is negative, proving there is
later internal cancellation.  Task14 first diverges at sequence 0 VLOAD
(duration/recycle `+1 cycle`) and sequence 1 fire is also `+1 cycle`.

## Complete SCH held-out result

The full-DAG span remains RTL 1,534,187 ns versus gem5 1,539,012 ns:
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

## Regression gates and next boundaries

- LSU RAW/throughput/capacity: 68/68 pass; 382/382 VINS files are exact to
  the accepted run.
- BitALU, CAU, and SerDiv each reach depth 2/full; 1/4/64 ns capacity-case
  outputs are functionally exact to the 1 ns control.
- Next CCH boundary: task20 sequence 608 result enqueue/grant/retirement;
  independently retain task17 LSU sequence 191/194 and short-task CPU edges.
- Next SCH boundary: task3 CAU/SerDiv/LSU overlap first, task4 SerDiv D/Q
  second, and task8 Shuffle/LSU arbitration third.
