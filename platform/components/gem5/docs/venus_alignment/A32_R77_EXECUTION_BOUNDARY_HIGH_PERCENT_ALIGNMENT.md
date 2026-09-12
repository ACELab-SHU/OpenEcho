# A32 R77 task execution boundary and high-percentage triage

Date: 2026-08-10

Verdict: `INCONCLUSIVE`

## What changed

The task timing comparator now pairs the same lifecycle boundary:

- RTL: `start execute -> execute complete`
- gem5: `tile_start -> task_epilogue`

Return DMA and tile release are recorded as a separate gem5 tail.  The prior
table paired RTL execute completion with gem5 measured release, so short tasks
with one or two output DMAs showed artificial errors of 30--111 percent.
No simulator latency or workload-specific rule was changed in this pass.
The R76 binary remains authoritative:

`41469f72c13a8b3814f259cb413f0c805527f2cd36282cdf1f4c9788fde1f699`

The reusable checker is `tools/compare_task_execution_timing.py`.

## Corrected CCH execution errors

The formerly largest short-task errors collapse as follows:

| Task | Old error | Matched-boundary error | Execution delta | gem5 return tail |
| ---: | ---: | ---: | ---: | ---: |
| 0 | +90.128% | -2.011% | -11 ns | 504 ns |
| 2 | +86.472% | -2.011% | -11 ns | 484 ns |
| 5 | +55.531% | -1.676% | -15 ns | 512 ns |
| 6 | +98.117% | +3.578% | +19 ns | 502 ns |
| 7 | +30.950% | -1.676% | -15 ns | 292 ns |
| 8 | +91.337% | +3.578% | +19 ns | 466 ns |
| 10 | +64.068% | -2.373% | -7 ns | 196 ns |
| 11 | +111.200% | -3.467% | -13 ns | 430 ns |
| 12 | +74.933% | -3.467% | -13 ns | 294 ns |
| 16 | +34.624% | +0.521% | +7 ns | 458 ns |

These tasks must not be "fixed" by changing CPU or VFU latency.  Their old
percentage was almost entirely the unmatched return tail.

The genuine high CCH execution residuals are now task13 +75 ns (+13.321%),
task23 +4,109 ns (+12.103%), and task4 +291 ns (+7.241%).  Larger absolute
residuals remain task9 +8,127 ns, task20 -13,401 ns, task22 +3,563 ns, and
task18 +2,545 ns; they remain independent alignment targets.

## Corrected SCH execution errors

| Task | RTL | gem5 | Delta | Error |
| ---: | ---: | ---: | ---: | ---: |
| 0 | 676,287 ns | 687,092 ns | +10,805 ns | +1.598% |
| 1 | 79,131 ns | 78,170 ns | -961 ns | -1.214% |
| 2 | 55,227 ns | 59,504 ns | +4,277 ns | +7.744% |
| 3 | 471,571 ns | 472,940 ns | +1,369 ns | +0.290% |
| 4 | 30,175 ns | 30,692 ns | +517 ns | +1.713% |
| 5 | 11,059 ns | 10,734 ns | -325 ns | -2.939% |
| 6 | 3,795 ns | 3,982 ns | +187 ns | +4.928% |
| 7 | 1,059 ns | 1,180 ns | +121 ns | +11.426% |
| 8 | 330,955 ns | 332,268 ns | +1,313 ns | +0.397% |

SCH task7 remains a real short-task percentage residual after removing its
84 ns return tail.  SCH task2 is the next larger execution target.

## task13 first real vector divergence

Task13 has 85 architectural scalar-retire records and six CAU instructions.
With the R76 default path, the six instruction fire cycles relative to the
first are RTL `0/6/12/18/56/62` and gem5 `0/6/12/18/94/100`.  The first four
fires are exact; the fifth is 38 tile cycles late.

The default first VMUL duration is gem5 41 versus RTL 34 cycles.  Enabling
the source-derived tagged/FairArb path makes the first two instruction
durations exactly 34/51 cycles, and moves the first duration mismatch to the
third instruction by only one cycle.  It does **not** repair the fifth fire:
that remains gem5 94 versus RTL 56.  Therefore reducing a fixed VMUL latency
would be an invalid fit.

The focused sequencer trace shows the fourth CAU command holding the
downstream all-lane handshake while individual lanes wait for their
operand-command registers/requesters.  The sequencer cannot accept the fifth
command during this interval.  The next implementation target is the exact
RTL `venus_lane_sequencer` fall-through command register and
`venus_operand_requester` same-edge consume/refill behavior, followed by
per-lane ready-vector comparison.  The experimental FairArb flags remain off
by default because they do not close this boundary.

## Functional and scope gates

Because the simulator binary did not change, R76 gates remain applicable:

- CCH: 10,253 VINS task-local exact to the accepted baseline.
- SCH: 8,103 VINS exact to the accepted baseline; only 7,256 have retained
  RTL dumps and the other 847 are not claimed as direct RTL comparison.
- LSU RAW/throughput/capacity: 68/68 pass.
- VEMU, scheduler, and RTL worktrees were read only in this pass.

## Evidence

- `evidence/a32_r77_execution_boundary_20260810/cch/task_execution_timing_vs_rtl.json`
- `evidence/a32_r77_execution_boundary_20260810/sch/task_execution_timing_vs_rtl.json`
- `evidence/a32_r77_execution_boundary_20260810/cch/task13_first_scalar_divergence.txt`
- `evidence/a32_r77_execution_boundary_20260810/cch/task13_scalar_summary.txt`
- `evidence/a32_r77_execution_boundary_20260810/cch/task13_vins_timing_default.json`
- `evidence/a32_r77_execution_boundary_20260810/cch/task13_vins_timing_fairarb.json`

The old release-inclusive tables are retained as historical scheduler/DMA
evidence, but they are no longer valid per-task execution comparisons.
