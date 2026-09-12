# A32 R864 scalar ID queue and current CCH/SCH alignment

Date: 2026-08-15

Status: **functional regression gates pass; per-instruction timing is not
fully aligned.** Full-DAG error is reported only as context and is not an
acceptance criterion.

## Accepted model change

The scalar600 ID branch operand reader no longer evaluates an instruction
that is still queued in Minor Execute as if it already occupied RTL EX. A
matching queued producer now holds the control instruction until the producer
has actually issued; only the ordered `rtlIdWriters` EX-forward chain can then
supply the value. This is a generic pipeline-state rule and contains no task,
PC, DAG, address, payload or operand-value special case.

The direct task17 trace showed why this matters. At PC `0x120c`, the previous
model resolved a branch from a stale value and paid a later three-cycle
correction. The accepted rule removes that false prediction. It also exposes
an older two-cycle scalar/LSU admission gap rather than hiding it with the
wrong-path penalty.

## Rejected experiment

RTL scalar-memory traces split the remaining boundary as follows:

- task17: extension second word at tile cycle `193583`, scalar read request at
  `193586`, retire at `193591` (`3 + 5` cycles);
- SCH task8 representative: extension second word at `610548`, scalar read
  request at `610550`, retire at `610555` (`2 + 5` cycles).

A global or current-parity-conditioned commit hold is therefore wrong: it
changes a request-admission contract into a retirement delay, feeds back into
future phase, and moved SCH task8 from `-1.093 us` to `+1.135 us`. That code
was removed. The next implementation must carry explicit registered
extension-to-scalar-LSU admission state and validate request and retire edges
independently.

## Current timing

The task boundary is RTL `start execute -> execute complete` against gem5
`tile_start -> task_epilogue`. Return DMA and tile release are excluded from
individual task percentages.

### nrPDCCH / CCH

Full allocation-to-last-release span: RTL `1,042.151 us`, gem5
`1,042.564 us`, delta `+0.413 us` (`+0.039630%`).

| task | RTL (us) | gem5 (us) | gem5 - RTL | error |
|---:|---:|---:|---:|---:|
| 0 | 0.547 | 0.536 | -0.011 us | -2.011% |
| 1 | 10.691 | 10.680 | -0.011 us | -0.103% |
| 2 | 0.547 | 0.536 | -0.011 us | -2.011% |
| 3 | 69.631 | 69.378 | -0.253 us | -0.363% |
| 4 | 4.019 | 4.010 | -0.009 us | -0.224% |
| 5 | 0.895 | 0.886 | -0.009 us | -1.006% |
| 6 | 0.531 | 0.522 | -0.009 us | -1.695% |
| 7 | 0.895 | 0.886 | -0.009 us | -1.006% |
| 8 | 0.531 | 0.522 | -0.009 us | -1.695% |
| 9 | 158.799 | 158.690 | -0.109 us | -0.069% |
| 10 | 0.295 | 0.288 | -0.007 us | -2.373% |
| 11 | 0.375 | 0.366 | -0.009 us | -2.400% |
| 12 | 0.375 | 0.366 | -0.009 us | -2.400% |
| 13 | 0.563 | 0.556 | -0.007 us | -1.243% |
| 14 | 202.779 | 203.008 | +0.229 us | +0.113% |
| 15 | 6.443 | 6.410 | -0.033 us | -0.512% |
| 16 | 1.343 | 1.334 | -0.009 us | -0.670% |
| 17 | 33.167 | 33.108 | -0.059 us | -0.178% |
| 18 | 123.235 | 123.234 | -0.001 us | -0.001% |
| 19 | 2.439 | 2.434 | -0.005 us | -0.205% |
| 20 | 398.523 | 398.514 | -0.009 us | -0.002% |
| 21 | 9.303 | 9.306 | +0.003 us | +0.032% |
| 22 | 100.055 | 100.054 | -0.001 us | -0.001% |
| 23 | 33.951 | 33.944 | -0.007 us | -0.021% |

The small full-DAG number still contains positive/negative task cancellation.
Short-task percentages remain large even when the absolute gap is only
7--11 ns.

### PDSCHDag2 / SCH

Full allocation-to-last-release span: RTL `1,534.187 us`, gem5
`1,539.996 us`, delta `+5.809 us` (`+0.378637%`).

| task | RTL (us) | gem5 (us) | gem5 - RTL | error |
|---:|---:|---:|---:|---:|
| 0 | 676.287 | 676.300 | +0.013 us | +0.002% |
| 1 | 79.131 | 79.142 | +0.011 us | +0.014% |
| 2 | 55.227 | 54.900 | -0.327 us | -0.592% |
| 3 | 471.571 | 477.098 | +5.527 us | +1.172% |
| 4 | 30.175 | 31.104 | +0.929 us | +3.079% |
| 5 | 11.059 | 10.990 | -0.069 us | -0.624% |
| 6 | 3.795 | 3.786 | -0.009 us | -0.237% |
| 7 | 1.059 | 1.056 | -0.003 us | -0.283% |
| 8 | 330.955 | 329.862 | -1.093 us | -0.330% |

SCH remains dominated by task3 and task4 positive error, partly cancelled by
task8 and task2. It is not acceptable to tune only the total `+0.379%`.

## Micro-timing and regression gates

- task17 sequences 0--193 remain exact. The first fire/recycle divergence is
  sequence194 VLOAD: gem5 is 2 tile cycles early; sequence195 VBRDCST duration
  is 13 cycles versus RTL 12.
- The direct sequence194 RTL VLOAD path is: PE request, addrgen, registered AR,
  external R, internal result queue, per-bank VRF grant and commit. Its own
  RTL duration is 34 cycles; the remaining pre-fire gap is on the scalar LSU
  admission side, not justification for changing fixed vector-load latency.
- CCH 10,253 and SCH 8,103 task-local VINS are exact against the accepted
  functional baselines.
- LSU suite is 68/68 and all 382 VINS are byte-exact against R853.
- Scalar suite launches 52/52. Direct oracle comparison is 51/52 only because
  of the documented obsolete `ecall_nop` fixture; the other 1,258 retire and
  1,258 writeback records are exact.

Accepted binary SHA-256:

`f6c8eb43e658f2f2b31b8375b0fbca5a6fd0c60815291dba42b1d75ef905282c`

Evidence: `evidence/a32_r864_scalar_id_queue_20260815/`.

## Next boundary

Implement explicit registered `Venus extension -> scalar LSU request`
admission, with RTL-visible request and retire events as separate checks.
After task17 sequence194 is exact without changing SCH task8, continue from
task17 sequence230 VLOAD result/recycle and SCH task3/task4 VFU/SerDiv queues.
Total-DAG error and task-to-task cancellation remain non-acceptance metrics.
