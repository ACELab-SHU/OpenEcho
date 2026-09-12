# A32 R908 CPU load-to-DIV and inactive-lane RAW alignment

Date: 2026-08-15

## Verdict

`INCONCLUSIVE` for complete RTL equivalence.  This round closes two concrete,
RTL-observed boundaries without using the full-DAG total as an objective:

1. scalar600 independent load-to-DIV/REM D/Q replay;
2. command-wide tagged retirement for a dependent lane which did not
   participate in the short arithmetic producer.

SCH task2 now has 841/841 VINS with exact identity, relative fire, duration,
and recycle timing.  CCH task14 moved from `-1003 ns` to `+17 ns`.  Other
task-local residuals remain, so neither small execution envelopes nor task
cancellation are treated as proof of completeness.

VEMU, scheduler/L1 packaging, and RTL sources were not changed.  The frozen RTL
logs and accepted functional artifacts were reused as oracles because this
round changes gem5 timing only.

## R907: scalar load-to-DIV D/Q boundary

SCH task2's first vector command fired 169 tile cycles too early even though
its later vector divergence was only three cycles.  Direct scalar comparison
isolated an independent `lbu` at PC `0x3bc` followed by `remu` at PC `0x3c0`:

| boundary | RTL | old gem5 | accepted gem5 |
|---|---:|---:|---:|
| load retire to DIV/REM retire | 205 cycles | 35 cycles | 205 cycles |

The first candidate added 170 cycles to a generic FU minimum-commit boundary.
It produced 171 rather than 205 cycles because the delay ran in parallel with
the existing 35-cycle divider and was rejected.  The accepted model places the
170-cycle independent-load replay in the divider pending-ready D/Q state, so
the structural sequence is `35 + 170 = 205` cycles.  A true load-to-DIV data
dependency is excluded, and no PC, task, instruction ordinal, address, or data
special case is present.

The same condition occurs exactly three times in CCH task14, accounting for
the observed `3 * 170 cycles * 2 ns = 1020 ns` change from `-1003 ns` to
`+17 ns`.

## R908: inactive producer lane tagged completion

After the CPU correction, SCH task2's first vector mismatch was sequence 45
`VRANGE`: RTL 78 cycles versus gem5 81.  The same `VL=31`, `EW16` pattern
recurred at sequences 324 and 603.

The preceding sequence is an `EW8`, `VL=31` `VXOR`, so only lanes 0--3
participate.  The wider dependent `VRANGE` uses lanes 0--7.  Active lanes 0--3
cleared the cross-VFU RAW at the tagged local retirement boundary; lanes 4--7
had no local producer and waited six ticks for a later global generation
change, adding exactly three tile cycles.

The lane now records the accepted VINS generation for every running ID.  A
cross-VFU arithmetic requester may consume the registered all-lane command
completion only when all of the following hold:

- the completion generation matches the captured producer generation;
- this lane's accepted-generation tag proves it did not receive that producer;
- neither side is LSU or Shuffle.

Active lanes remain on their local cross-VFU chaining path.  This avoids the
rejected R805 behavior that reused global retirement for all lanes and moved
CCH task20 to 414068 ns.

The three short `VRANGE` lifecycles are now 78/78 cycles.  The complete task2
comparison reports 841/841 exact instructions with no first divergence.

## Remaining SCH task2 boundary

The vector region is now exact; the remaining `-9 ns` is outside it:

| component | RTL | gem5 | delta |
|---|---:|---:|---:|
| task start to first VINS fire | 618 cycles | 619 cycles | +2 ns |
| first VINS fire to last VINS recycle | 26961 cycles | 26961 cycles | 0 ns |
| last VINS recycle to task complete | 69 ns | 58 ns | -11 ns |
| whole task | 55227 ns | 55218 ns | -9 ns |

The next task2 work is therefore scalar entry/epilogue boundary modeling, not
VFU latency, requester admission, or VRF arbitration.

## Final task timing

The comparison boundary is RTL `start execute -> execute complete` versus gem5
`tile_start -> task_epilogue`.  Return DMA and tile release are excluded.

### SCH / PDSCHDag2

| task | RTL ns | gem5 ns | delta ns | error |
|---:|---:|---:|---:|---:|
| 0 | 676287 | 676292 | +5 | +0.000739% |
| 1 | 79131 | 79134 | +3 | +0.003791% |
| 2 | 55227 | 55218 | -9 | -0.016296% |
| 3 | 471571 | 471552 | -19 | -0.004029% |
| 4 | 30175 | 30154 | -21 | -0.069594% |
| 5 | 11059 | 11098 | +39 | +0.352654% |
| 6 | 3795 | 3782 | -13 | -0.342556% |
| 7 | 1059 | 1050 | -9 | -0.849858% |
| 8 | 330955 | 330968 | +13 | +0.003928% |

The execution envelope remains RTL/gem5 `1534187/1533844 ns`, or
`-343 ns/-0.022357%`, because task2 is not the critical completion.  It is a
diagnostic only.  The largest remaining absolute SCH task residual is task5
`+39 ns`; task7 has the largest percentage only because it lasts 1059 ns.

### CCH / nrPDCCH

| task | RTL ns | gem5 ns | delta ns | error |
|---:|---:|---:|---:|---:|
| 0 | 547 | 532 | -15 | -2.742230% |
| 1 | 10691 | 10676 | -15 | -0.140305% |
| 2 | 547 | 532 | -15 | -2.742230% |
| 3 | 69631 | 69374 | -257 | -0.369088% |
| 4 | 4019 | 4006 | -13 | -0.323464% |
| 5 | 895 | 882 | -13 | -1.452514% |
| 6 | 531 | 518 | -13 | -2.448211% |
| 7 | 895 | 882 | -13 | -1.452514% |
| 8 | 531 | 518 | -13 | -2.448211% |
| 9 | 158799 | 158658 | -141 | -0.088791% |
| 10 | 295 | 284 | -11 | -3.728814% |
| 11 | 375 | 362 | -13 | -3.466667% |
| 12 | 375 | 362 | -13 | -3.466667% |
| 13 | 563 | 552 | -11 | -1.953819% |
| 14 | 202779 | 202796 | +17 | +0.008384% |
| 15 | 6443 | 6422 | -21 | -0.325935% |
| 16 | 1343 | 1330 | -13 | -0.967982% |
| 17 | 33167 | 33154 | -13 | -0.039196% |
| 18 | 123235 | 123228 | -7 | -0.005680% |
| 19 | 2439 | 2430 | -9 | -0.369004% |
| 20 | 398523 | 398510 | -13 | -0.003262% |
| 21 | 9303 | 9298 | -5 | -0.053746% |
| 22 | 100055 | 100048 | -7 | -0.006996% |
| 23 | 33951 | 33938 | -13 | -0.038290% |

CCH task20 remains `-13 ns`; therefore the inactive-lane rule did not repeat
the old global-retirement regression.  The execution envelope improves from
R906 `-1891 ns` to `-871 ns` only because task14 changed by 1020 ns.  It is not
used as acceptance evidence.  The next independent absolute residuals are
task3 `-257 ns` and task9 `-141 ns`; the common short-task `-11..-15 ns` tail
should be investigated as one scalar boundary family.

## Correctness and regression gates

- SCH: 22/22 output files and 8103/8103 VINS byte-exact to R907.
- CCH: 53/53 output files and 10253/10253 VINS byte-exact to R907.
- SCH task2: 841/841 identity/fire/duration/recycle exact to RTL.
- LSU RAW/throughput/capacity: 68/68; 382/382 VINS exact to R907.
- BitALU, CAU, and SerDiv result queues all reached depth 2/full; all grant-gap
  functional outputs match their control.
- scalar suite: 52/52 executes; the frozen oracle compares 51/52 with 1258
  retire and 1258 writeback records exact.  `ecall_nop` remains the documented
  old fixture-length exclusion and is not represented as a pass.
- final binary SHA256:
  `052ef9883e6bc52a2658abab48a6400567987f34fb90b4cb5c05174948d47675`.

Permanent evidence:

- `evidence/a32_r908_cpu_div_inactive_lane_20260815/final_cch_sch_summary.json`
- `evidence/a32_r908_cpu_div_inactive_lane_20260815/sch_task_timing_vs_rtl.json`
- `evidence/a32_r908_cpu_div_inactive_lane_20260815/sch_task2_vins_timing_vs_rtl.json`
- `evidence/a32_r908_cpu_div_inactive_lane_20260815/sch_task2_boundary_decomposition.json`
- `evidence/a32_r908_cpu_div_inactive_lane_20260815/scalar_suite_results.json`
- `evidence/a32_r908_cpu_div_inactive_lane_20260815/scalar_oracle_results.json`
- `evidence/a32_r908_cpu_div_inactive_lane_20260815/lsu_suite_results.json`
- `evidence/a32_r908_cpu_div_inactive_lane_20260815/vfu_capacity_results.json`
- `evidence/a32_r908_cpu_div_inactive_lane_20260815/sch_venus_dag_trace.jsonl`
- `evidence/a32_r908_cpu_div_inactive_lane_20260815/cch_venus_dag_trace.jsonl`

## Next breakpoints

1. Split SCH task5 `+39 ns` at scalar entry, first VINS, last VINS, and epilogue;
   retain task2's now-exact vector lifecycle as a held-out control.
2. Split CCH task3 `-257 ns` and task9 `-141 ns` at their first scalar/vector
   divergence; do not modify VFU fixed latency from their task totals.
3. Audit the common CCH short-task `-11..-15 ns` epilogue family together with
   SCH task2's `-11 ns` post-vector tail.
4. Continue task17/task20 per-bank requester and LSU checks only from a new
   direct RTL edge oracle; task20's `-13 ns` is a held-out non-regression gate.
