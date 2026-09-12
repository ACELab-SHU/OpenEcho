# A32 R871 multi-beat WLAST/addrgen alignment

Date: 2026-08-15

## Accepted structural change

The task17 sequence230 direct RTL oracle shows that the remaining two-cycle
VLOAD error was not in fixed VLOAD latency, result enqueue, per-bank grant, or
retirement. Relative to fire, gem5's return/result/retire chain already had the
same spacing as RTL; the internal AR was early at the STU-to-LDU direction
transition.

The paired task17 controls repeat throughout the same instruction stream:

- a single-beat VSTORE followed by the same VLOAD shape gives 104 cycles in
  both RTL and gem5;
- a beat-crossing, multi-beat VSTORE completed on the new VLOAD's availability
  edge gives 106 RTL cycles, while R868 gave 104.

R871 therefore models one registered WLAST/direction-select boundary only when
a multi-beat STU completes on the exact edge that an opposite-direction LSU
request would otherwise enter addrgen. Same-direction traffic, single-beat
stores, and later arrivals are unchanged. There is no task, PC, DAG, literal
address, or data match, and no LSU/VFU fixed latency changed.

## CCH / nrPDCCH

Allocation-to-last-release remains RTL `1,042.151 us`, gem5 `1,042.564 us`,
delta `+0.413 us` (`+0.039630%`), because task20 remains the critical path.
This total is not used as the acceptance criterion.

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
| 17 | 33.167 | 33.164 | -0.003 | -0.009% |
| 18 | 123.235 | 123.234 | -0.001 | -0.001% |
| 19 | 2.439 | 2.434 | -0.005 | -0.205% |
| 20 | 398.523 | 398.514 | -0.009 | -0.002% |
| 21 | 9.303 | 9.306 | +0.003 | +0.032% |
| 22 | 100.055 | 100.054 | -0.001 | -0.001% |
| 23 | 33.951 | 33.944 | -0.007 | -0.021% |

Task17 improves from `-55 ns` to `-3 ns`. Its 687 vector instructions are
exact through sequence679 in normalized fire/duration/recycle. Sequence680 is
now the first difference: the VLOAD fires two cycles late, but its own
61-cycle duration is exact. Since sequence679 VSTORE recycle is also exact,
the next target is the post-store sequence/request admission interval.

Task20's 8,837 vector instructions are all exact in normalized fire, duration,
and recycle. Its task envelope remains `-9 ns`; that residue is outside the
vector instruction lifecycle and must not be used to infer unverified internal
per-bank requester/grant equivalence.

## SCH / PDSCHDag2

Allocation-to-last-release is RTL `1,534.187 us`, gem5 `1,541.128 us`, delta
`+6.941 us` (`+0.452422%`). R871 intentionally adds 20 ns to task4 at five
structurally matching multi-beat store-to-load boundaries. Although its task
aggregate grows from `+0.929 us` to `+0.949 us`, rejecting the boundary on that
basis would be aggregate-error fitting.

| task | RTL (us) | gem5 (us) | delta (us) | error |
|---:|---:|---:|---:|---:|
| 0 | 676.287 | 676.300 | +0.013 | +0.002% |
| 1 | 79.131 | 79.142 | +0.011 | +0.014% |
| 2 | 55.227 | 54.900 | -0.327 | -0.592% |
| 3 | 471.571 | 477.098 | +5.527 | +1.172% |
| 4 | 30.175 | 31.124 | +0.949 | +3.145% |
| 5 | 11.059 | 10.990 | -0.069 | -0.624% |
| 6 | 3.795 | 3.786 | -0.009 | -0.237% |
| 7 | 1.059 | 1.056 | -0.003 | -0.283% |
| 8 | 330.955 | 330.974 | +0.019 | +0.006% |

The first SCH task3 difference is sequence1 VLOAD fire/recycle `+2` cycles
with exact duration. Larger recurring sources then appear at inter-sequence
gaps (`+215/+218` cycles), VDIV/SerDiv retirement (`+82` through `+455`
cycles), and the dependent VMUL/VSADD result/retirement wave. Task4 first
differs at sequence7 VSADD recycle `-1` cycle; its final vector recycle is
`+478` cycles (`+0.956 us`), dominated by repeated VDIV-to-dependent-VFU
phases and store/load boundaries rather than a single fixed latency.

## Regression gates

- CCH: 10,253 task-local VINS byte-exact against R868.
- SCH: 8,103 task-local VINS byte-exact against R868.
- scalar600 execution suite: 52/52; valid RTL oracle records: 1,258 retire and
  1,258 writeback events exact.
- vector LSU RAW/throughput/capacity suite: 68/68, 382 VINS.
- tested binary SHA-256:
  `baa230f69aac17824e1953486794963f862a73054453d88d4c7f0d72992dbb7e`.

Evidence: `evidence/a32_r871_multibeat_wlast_addrgen_20260815/`.

## Next boundaries

1. Task17 sequence680: split the exact sequence679 VSTORE recycle to next
   VLOAD fire interval into scalar sequence production, requester admission,
   and addrgen capture.
2. SCH task3: first isolate the recurring scalar/inter-sequence `+215`-cycle
   gaps, then SerDiv result enqueue/full, dependent VFU grants, and retire.
3. SCH task4: retain the five verified WLAST boundaries and localize the
   sequence7 VSADD/result-queue first difference before later VDIV accumulation.
4. Task20: keep the 8,837-instruction exact lifecycle as a regression gate and
   separately verify stable tagged requester vectors and persistent per-bank
   RR; task-total agreement alone is insufficient.

