# A32 R814 current CCH/SCH timing audit

Date: 2026-08-14

## Verdict

The accepted model was rebuilt after rejecting an LDU AXI phase experiment,
then both complete workloads were rerun.  The binary SHA256 is
`60fb7f9cbbf847432a9c4195135fc80aff57976572e98d1fcc4123c1d0bca1c0`.

Per-task execution uses the same lifecycle boundary on both sides: RTL
`start execute -> execute complete` and gem5 `tile_start -> task_epilogue`.
Return DMA and tile release are excluded.  The complete DAG figures use the
retained RTL complete span and the gem5 `dag_complete` tick.

- CCH: RTL 1,042,151 ns, gem5 1,042,604 ns, **+453 ns (+0.043468%)**.
- SCH: RTL 1,534,187 ns, gem5 1,540,004 ns, **+5,817 ns (+0.379158%)**.

Neither aggregate is an acceptance criterion: SCH still contains strong
cancellation, chiefly task3 `+5,489 ns`, task4 `+927 ns`, and task8
`-1,091 ns`.

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
| 15 | 6,443 | 6,408 | -35 | -0.543% |
| 16 | 1,343 | 1,336 | -7 | -0.521% |
| 17 | 33,167 | 33,118 | -49 | -0.148% |
| 18 | 123,235 | 123,234 | -1 | -0.001% |
| 19 | 2,439 | 2,436 | -3 | -0.123% |
| 20 | 398,523 | 398,512 | -11 | -0.003% |
| 21 | 9,303 | 9,306 | +3 | +0.032% |
| 22 | 100,055 | 100,052 | -3 | -0.003% |
| 23 | 33,951 | 33,944 | -7 | -0.021% |

The largest independent CCH residuals are task3 `-251 ns`, task14
`+231 ns`, task9 `-111 ns`, task17 `-49 ns`, and task15 `-35 ns`.  The
common `-7..-11 ns` residual on very short tasks creates the largest
percentages and remains a scalar CPU/task-boundary issue; it is not evidence
for changing a vector fixed latency.

Task17's first VLOAD still lasts 35 gem5 cycles versus 34 RTL cycles.  A
generic global LDU AXI phase flip made task17 exact and changed its total from
`-49 ns` to `+1 ns`, but simultaneously changed task20's first VLOAD from the
RTL-exact 45 cycles to 44 and worsened task20 from `-11 ns` to `-257 ns`.
That candidate was rejected and removed.  The two RTL oracles show that the
missing rule is the source-pointer setup/sampling aperture at the asynchronous
AR CDC boundary, not a universal latency or phase constant.

## SCH / PDSCHDag2 held-out

| Task | RTL ns | gem5 ns | Delta ns | Error |
| ---: | ---: | ---: | ---: | ---: |
| 0 | 676,287 | 676,300 | +13 | +0.002% |
| 1 | 79,131 | 79,140 | +9 | +0.011% |
| 2 | 55,227 | 54,900 | -327 | -0.592% |
| 3 | 471,571 | 477,060 | +5,489 | +1.164% |
| 4 | 30,175 | 31,102 | +927 | +3.072% |
| 5 | 11,059 | 10,988 | -71 | -0.642% |
| 6 | 3,795 | 3,786 | -9 | -0.237% |
| 7 | 1,059 | 1,056 | -3 | -0.283% |
| 8 | 330,955 | 329,864 | -1,091 | -0.330% |

The main SCH gap is structural vector overlap rather than scalar task entry:

- task3 accumulates `+5,489 ns` through repeated CAU/SerDiv/LSU overlap;
- task4 accumulates `+927 ns` around SerDiv D/Q, result enqueue, per-bank VRF
  grant, and retirement;
- task8 is `1,091 ns` early through repeated Shuffle/LSU contention, where the
  remaining model needs stable tagged requesters, independent high-priority
  LSU request state, and persistent per-bank round-robin state;
- task2 is `327 ns` early, with its first material divergence around the
  VRANGE/Shuffle result path.

## Functional regression and evidence

The fresh runs are byte-exact to the accepted functional baselines:

- CCH: 10,253 VINS compared, zero mismatch;
- SCH: 8,103 VINS compared, zero mismatch.  Only the retained 7,256 RTL VINS
  are claimed as direct RTL coverage; the remaining 847 are baseline-only.

Evidence:

- `evidence/a32_r814_current_cch_sch_20260814/cch_task_timing_vs_rtl.json`
- `evidence/a32_r814_current_cch_sch_20260814/sch_task_timing_vs_rtl.json`
- `evidence/a32_r814_current_cch_sch_20260814/cch_vins_vs_r811.json`
- `evidence/a32_r814_current_cch_sch_20260814/sch_vins_vs_r811.json`

