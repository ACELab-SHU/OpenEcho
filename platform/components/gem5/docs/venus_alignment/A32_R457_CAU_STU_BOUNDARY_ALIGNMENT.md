# A32 R457 CAU / STU registered-boundary alignment

Date: 2026-08-12

Status: **INCONCLUSIVE**.  This checkpoint accepts two RTL-derived local
boundaries and deliberately does not claim whole-task or whole-DAG timing
equivalence.

## Accepted model changes

### CAU arithmetic pipeline

`VenusLane::getCauPipeLength()` now uses the RTL values directly:

- add/sub: 0 cycles;
- multiply family: 2 cycles;
- complex multiply: 4 cycles.

These values come from `venus_pkg.sv` (`LatMul=2`, `LatCmxMul=4`) and
`venus_cau_wrapper.sv::cau_latency`.  Operand requester Q visibility and the
depth-2 result queue remain separate registered boundaries; they are not
folded into arithmetic latency.

For CCH task17 sequence 4 VMUL, the operand request/grant boundary was already
aligned: gem5 and RTL both issue the first bank request 20 lane cycles after
fire.  The old 4-cycle multiply pipe nevertheless made the first result two
cycles late.  With the 2-cycle RTL value, task17 sequences 0--95 are exact and
the first divergence moves to sequence 96 VSTORE (+2 cycles before the STU
fix).

### STU operand-ready to local-W

`VenusStuOperandToLocalWriteCycles` changes from 2 to 1.  The task17 sequence
96 RTL oracle has STU operand request at 569573 ns and the local W handshake at
569575 ns, one 500 MHz tile cycle later.  After this change sequence 96 is
52/52 cycles and task17 sequences 0--176 have exact fire/recycle/duration.

The first remaining task17 discrepancies are now:

- sequence 177 VXOR duration/recycle: gem5 68 cycles, RTL 69 cycles;
- sequence 180 VBRDCST fire: gem5 is 3 cycles late.

## Rejected experiment

Registering every operand requester with one uniform extra Q stage was tested
and reverted.  It did not improve task17's sequence-177 breakpoint and changed
187 of the first 419 task20 bank-vector cycles, including Shuffle master-bank
phase.  The result shows that current gem5 passage entry points already encode
different implicit phases.  The next fix must make those entry phases explicit
per RTL passage, rather than adding one global delay.

The accepted CAU change itself preserves the inspected task20 bank stream:
the first 419 candidate vectors are identical to the preceding BitALU/CAU-B
candidate.  This candidate-to-candidate statement is intentionally narrower
than claiming all 419 cycles RTL-exact.

## Full CCH result

Comparison boundary: RTL `start execute -> execute complete` versus gem5
`tile_start -> task_epilogue`; return DMA/release is excluded.

| Task | RTL ns | gem5 ns | Delta ns | Error |
| ---: | ---: | ---: | ---: | ---: |
| 0 | 547 | 540 | -7 | -1.280% |
| 1 | 10,691 | 10,740 | +49 | +0.458% |
| 2 | 547 | 540 | -7 | -1.280% |
| 3 | 69,631 | 69,276 | -355 | -0.510% |
| 4 | 4,019 | 4,014 | -5 | -0.124% |
| 5 | 895 | 886 | -9 | -1.006% |
| 6 | 531 | 526 | -5 | -0.942% |
| 7 | 895 | 886 | -9 | -1.006% |
| 8 | 531 | 526 | -5 | -0.942% |
| 9 | 158,799 | 158,832 | +33 | +0.021% |
| 10 | 295 | 288 | -7 | -2.373% |
| 11 | 375 | 370 | -5 | -1.333% |
| 12 | 375 | 370 | -5 | -1.333% |
| 13 | 563 | 612 | +49 | +8.703% |
| 14 | 202,779 | 203,802 | +1,023 | +0.504% |
| 15 | 6,443 | 6,400 | -43 | -0.667% |
| 16 | 1,343 | 1,346 | +3 | +0.223% |
| 17 | 33,167 | 33,314 | +147 | +0.443% |
| 18 | 123,235 | 126,700 | +3,465 | +2.812% |
| 19 | 2,439 | 2,420 | -19 | -0.779% |
| 20 | 398,523 | 393,784 | -4,739 | -1.189% |
| 21 | 9,303 | 9,434 | +131 | +1.408% |
| 22 | 100,055 | 100,058 | +3 | +0.003% |
| 23 | 33,951 | 33,944 | -7 | -0.021% |

CCH functional DAG span is 1,042,308 ns versus RTL 1,043,224 ns, or
-916 ns (-0.088%).  This small total is cancellation: task20 is 4,739 ns too
fast while task18 is 3,465 ns too slow.

Relative to the immediately preceding R442 full run, task17 improves from
+419 ns to +147 ns.  Task20 changes from +161 ns to -4,739 ns.  The latter is
not hidden or used to reject the RTL-exact CAU latency; it exposes missing
requester/result-queue passage boundaries elsewhere in task20.

## Full SCH result

| Task | RTL ns | gem5 ns | Delta ns | Error |
| ---: | ---: | ---: | ---: | ---: |
| 0 | 676,287 | 676,300 | +13 | +0.002% |
| 1 | 79,131 | 79,120 | -11 | -0.014% |
| 2 | 55,227 | 57,808 | +2,581 | +4.673% |
| 3 | 471,571 | 478,424 | +6,853 | +1.453% |
| 4 | 30,175 | 31,058 | +883 | +2.926% |
| 5 | 11,059 | 11,206 | +147 | +1.329% |
| 6 | 3,795 | 3,786 | -9 | -0.237% |
| 7 | 1,059 | 1,056 | -3 | -0.283% |
| 8 | 330,955 | 330,546 | -409 | -0.124% |

SCH functional DAG span is 1,542,204 ns versus RTL 1,535,100 ns, or
+7,104 ns (+0.463%).  The dominant independent residual is task3 +6,853 ns,
then task2 +2,581 ns and task4 +883 ns; task8 has the opposite sign.

## Functional and build gates

- CCH: 10,253 instructions, task-local VINS exact against the accepted R442
  baseline.
- SCH: 8,103 instructions, task-local VINS exact against the accepted R443
  baseline.
- Build completed successfully.
- `gem5.debug` SHA256:
  `c4b48ca7dea7dae74d15875ed55eec407023c6ad4e5fbbbc10b372e30eb6c01c`.

Permanent machine-readable evidence is under:

`evidence/a32_r457_cau_stu_alignment_20260812/`

## Next structural work

1. Capture a narrow RTL oracle for task17 sequence 177 and split VXOR result
   enqueue, registered writer request, per-bank grant, and retirement.  Then
   continue through the sequence-180 broadcast phase shift.
2. For task20, keep CAU arithmetic at RTL 0/2/4 and model the missing passage
   entry phases explicitly: stable tagged requester vector, vector-LSU
   independent priority, registered WB/RAW visibility, and persistent per-bank
   RR.  Do not recover the total with a CAU fixed-latency retune.
3. Split task18 and SCH task3/task2/task4 at their first instruction-level
   divergence.  Percent-only short tasks are diagnostic, not the priority over
   multi-microsecond absolute residuals.

