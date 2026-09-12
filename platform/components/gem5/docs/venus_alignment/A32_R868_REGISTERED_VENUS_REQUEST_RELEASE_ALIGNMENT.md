# A32 R868 registered Venus-request release alignment

Date: 2026-08-15

## Accepted structural change

The scalar600 RTL holds the instruction following a backpressured two-word
Venus request in ID.  After the Venus ready handshake, that follower crosses
registered ID/EX, LSU-complete, and WB-visible boundaries.  Minor previously
allowed a younger scalar load to enter the LSU on the same edge that the older
Venus request retry succeeded.

R868 records whether each dynamic `VENUSEXT` request was actually
backpressured.  Only that dynamic state activates the registered follower
release: a following memory request cannot use the same-edge LSU shortcut, it
enters the LSU two scalar clocks after the handshake, and its architectural
completion is held to the RTL WB-visible edge.  Non-backpressured requests keep
their previous behavior.  There is no task, PC, DAG, address, or data match,
and no vector LSU/VFU fixed latency was changed.

The direct RTL oracle contains 420 one-nanosecond samples.  At the critical
task17 instance, the request handshakes with `valid=ready=1`, the next edge is
a bubble, the following `lw` enters EX/LSU on the second edge, LSU `complete`
arrives on the seventh edge, and load WB visibility is on the eighth edge.

## CCH / nrPDCCH

The allocation-to-last-release span is unchanged because task20 remains the
critical path: RTL `1,042.151 us`, gem5 `1,042.564 us`, delta `+0.413 us`
(`+0.039630%`).  This total still must not be used as the acceptance metric.

| task | RTL (us) | gem5 (us) | delta (us) | error |
|---:|---:|---:|---:|---:|
| 0 | 0.547 | 0.536 | -0.011 | -2.011% |
| 1 | 10.691 | 10.680 | -0.011 | -0.103% |
| 2 | 0.547 | 0.536 | -0.011 | -2.011% |
| 3 | 69.631 | 69.378 | -0.253 | -0.363% |
| 4 | 4.019 | 4.010 | -0.009 | -0.224% |
| 5 | 0.895 | 0.886 | -0.009 | -1.006% |
| 6 | 0.531 | 0.522 | -0.009 | -1.695% |
| 7 | 0.895 | 0.886 | -0.009 | -1.006% |
| 8 | 0.531 | 0.522 | -0.009 | -1.695% |
| 9 | 158.799 | 158.690 | -0.109 | -0.069% |
| 10 | 0.295 | 0.288 | -0.007 | -2.373% |
| 11 | 0.375 | 0.366 | -0.009 | -2.400% |
| 12 | 0.375 | 0.366 | -0.009 | -2.400% |
| 13 | 0.563 | 0.556 | -0.007 | -1.243% |
| 14 | 202.779 | 203.008 | +0.229 | +0.113% |
| 15 | 6.443 | 6.410 | -0.033 | -0.512% |
| 16 | 1.343 | 1.334 | -0.009 | -0.670% |
| 17 | 33.167 | 33.112 | -0.055 | -0.166% |
| 18 | 123.235 | 123.234 | -0.001 | -0.001% |
| 19 | 2.439 | 2.434 | -0.005 | -0.205% |
| 20 | 398.523 | 398.514 | -0.009 | -0.002% |
| 21 | 9.303 | 9.306 | +0.003 | +0.032% |
| 22 | 100.055 | 100.054 | -0.001 | -0.001% |
| 23 | 33.951 | 33.944 | -0.007 | -0.021% |

Task17 sequences 0--229 now have exact normalized fire/recycle timing.  The
old sequence194 VLOAD fire/recycle `-2` cycles and sequence195 VBRDCST
duration `+1` cycle are exact.  The new first divergence is sequence230
VLOAD duration/recycle `-2` cycles; the first fire divergence is sequence231
VBRDCST `-1` cycle.  Task17 execution improves from `-59 ns` to `-55 ns`.

## SCH / PDSCHDag2

The allocation-to-last-release span is RTL `1,534.187 us`, gem5
`1,541.108 us`, delta `+6.921 us` (`+0.451118%`).  The larger total is expected:
task8's former negative error no longer cancels task3/task4's positive error.

| task | RTL (us) | gem5 (us) | delta (us) | error |
|---:|---:|---:|---:|---:|
| 0 | 676.287 | 676.300 | +0.013 | +0.002% |
| 1 | 79.131 | 79.142 | +0.011 | +0.014% |
| 2 | 55.227 | 54.900 | -0.327 | -0.592% |
| 3 | 471.571 | 477.098 | +5.527 | +1.172% |
| 4 | 30.175 | 31.104 | +0.929 | +3.079% |
| 5 | 11.059 | 10.990 | -0.069 | -0.624% |
| 6 | 3.795 | 3.786 | -0.009 | -0.237% |
| 7 | 1.059 | 1.056 | -0.003 | -0.283% |
| 8 | 330.955 | 330.974 | +0.019 | +0.006% |

Task8 improves from `-1.093 us` (`-0.330%`) to `+0.019 us` (`+0.006%`).
The remaining SCH error is therefore exposed rather than hidden: task3 and
task4 dominate, followed by task2.

## Regression gates

- CCH: 10,253 task-local VINS byte-exact against R852.
- SCH: 8,103 task-local VINS byte-exact against R854.
- scalar600 CPU suite: 52/52 passed.
- vector LSU RAW/throughput/capacity suite: 68/68 passed, 382 VINS emitted.
- Tested binary SHA-256:
  `e6873fcf4ecec8ffdca00b3a79e7a431c39c35849288c8f22d5edcdfd7234d07`.

Evidence: `evidence/a32_r868_venus_request_release_20260815/`.

## Next boundaries

1. Task17 sequence230 VLOAD: split internal result enqueue, per-bank VRF
   grant, and recycle; fixed VLOAD latency remains frozen.
2. SCH task3/task4: split operand admission, SerDiv/VFU result enqueue/full,
   grant, and retirement.  They now account for nearly all positive SCH gap.
3. Task20 remains a structural audit target despite its `-9 ns` task total:
   stable tagged requester vectors, independent high-priority LSU service,
   and persistent per-bank RR must be checked without relying on its near-zero
   aggregate timing.

