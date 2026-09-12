# A32 R739 tagged LSU priority and VSTU lookahead alignment

## Scope and verdict

This round continues the accepted CPU, requester, VRF arbitration, LSU, and
fixed-depth VFU queue model. It adds no task/sequence/PC/DAG/address/data
special case and changes no fixed CPU, LSU-memory, VFU, or DAG latency.

The candidate binary is:

```
build/RISCV/gem5.debug
SHA256 73f9592a115a4074e3bd69c737a6a4e1b0af2e71b110bc4dab595dd2ccfd820b
```

The verdict remains **INCONCLUSIVE for complete microtiming alignment**.
Functional output and the directed capacity gates pass, and task17 is within
3 ns, but task20 and the SCH task3/task4/task8 residuals remain independent
modeling gaps.

Permanent evidence is under:

```
evidence/a32_r739_tagged_lsu_stu_lookahead_20260814/
```

## RTL-backed structural changes

The task17 sequence-180 RTL oracle samples two consecutive positive tile
edges. All 16 lanes have `stu_req=1`, `stu_mask=1111`, and `ldu_req=0`; the
STU address advances from row 0 to row 1. Source inspection shows this is a
single short VSTORE's ping-pong prefetch, not a second VSTORE: while
`issue_cnt_bytes_q` remains non-zero and the alternate ping-pong row is empty,
`stu_operand_req_o` requests one row beyond the architectural payload.

The sequencer now reserves one additional STU operand row for this physical
request. LSU ownership is also represented as a stable `(tag, bank_mask)`
vector at each grant edge instead of an untyped global tick. Both LDU and STU
publish stable tags. Arbitration blocks only banks selected by an LSU vector,
and ordinary per-bank RR state advances only after an actual ordinary grant,
so LSU ownership no longer corrupts persistent RR state.

Active RTL `vldu.sv` sets every lane-valid bit when a result row enters the
queue; the older byte-enable-derived mask is commented out. LDU result rows
therefore intentionally publish all 64 data banks, including a partial tail.
An experimental tail-active-lane mask moved task17's first divergence
backward to sequence 104 and was rejected.

Relevant implementation:

- `src/venus/VenusSequencer.cc`: VSTU lookahead, LSU tags, full-bank LDU/STU
  vectors, and reservation publication;
- `src/venus/VenusSequencer.hh`: the LDU row's explicit bank vector;
- `src/mem/noncoherent_xbar.hh/.cc`: tagged per-tick bank vectors and
  independent per-bank priority checks.

## Focused first-divergence result

For CCH task17, sequence 180 VBRDCST is now exact. The first remaining
duration/recycle difference moves to sequence 191 VLOAD: gem5 is two cycles
faster. The first fire difference is sequence 194 VLOAD: gem5 is one cycle
later. The complete task is 33,170 ns versus RTL 33,167 ns, or +3 ns
(+0.009%). All 687 VINS outputs remain exact.

For CCH task20, the tagged LSU representation is timing-neutral because the
active RTL LDU/STU vectors currently own all 64 banks. The first remaining
duration/recycle difference is sequence 66 VSLE, 62 cycles in gem5 versus 63
in RTL. The first fire difference is sequence 70 VMUL, one cycle early in
gem5. The complete task is 397,072 ns versus RTL 398,523 ns, or -1,451 ns
(-0.364%). All 8,837 VINS outputs remain exact.

This separates a real structural repair from its immediate performance
effect: the tagged bank-vector mechanism is required for correct ownership,
but task20's current first divergence is still upstream/downstream of the LSU
priority decision.

## Full CCH result

The allocation-to-last-epilogue span is 1,040,080 ns versus RTL 1,042,151 ns:
-2,071 ns, -0.199%. This total is not an acceptance criterion.

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
| 20 | 398,523 | 397,072 | -1,451 | -0.364% |
| 21 | 9,303 | 9,306 | +3 | +0.032% |
| 22 | 100,055 | 100,052 | -3 | -0.003% |
| 23 | 33,951 | 33,944 | -7 | -0.021% |

All 24 tasks complete. The 10,253 emitted VINS outputs are byte-exact to the
accepted functional baseline. The largest absolute per-task residual is
task20, not the short tasks with the largest percentages. The repeated
-7/-9/-11 ns short-task shape remains a separate CPU/task-boundary issue.

## Full SCH held-out result

The allocation-to-last-epilogue span is 1,539,012 ns versus RTL 1,534,187 ns:
+4,825 ns, +0.314%. Task3 and task8 still partially cancel.

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

All nine tasks complete and 8,103 VINS outputs are exact to the accepted gem5
functional baseline. The retained RTL capture still covers only 7,256 VINS;
the other 847 have no retained RTL dump and are not represented as a direct
RTL comparison.

The first current lifecycle divergences are:

- task2 sequence 45 VRANGE: duration/recycle +3 cycles;
- task3 sequence 0 VLOAD: duration/recycle -1 cycle, followed by sequence 1
  fire +1 cycle; the long task later accumulates mixed CAU/SerDiv/LSU and
  arbitration differences;
- task4 sequence 0 VLOAD: duration/recycle -1 cycle, and sequence 8 VDIV fire
  -1 cycle; its +2.993% total points to SerDiv/result-DQ and grant overlap;
- task8 sequence 2 VSHUFFLE: fire/recycle -2 cycles, followed by sequence 3
  VSTORE duration -1 cycle; its long negative residual points to repeated
  Shuffle/LSU contention.

## Regression gates and next boundary

- LSU RAW/throughput/capacity suite: 68/68 pass;
- all 382 LSU result files have the same aggregate path/hash manifest as the
  accepted R653k control;
- CCH 10,253 and SCH 8,103 VINS are exact to the accepted baseline;
- BitALU and CAU reach depth=2/full at 4 ns and 64 ns grant gaps;
- SerDiv reaches depth=2/full at 64 ns;
- every capacity output is exact to the 1 ns control.

Next work should keep the now-exact task17 prefix and split task20 sequence
66--70 into stable request, VRF response data-valid, operand FIFO Q,
VFU valid/ready, result queue D/Q, and tagged retirement. SCH should be treated
independently: task3 for repeated CAU/SerDiv/LSU overlap, task4 for SerDiv
result D/Q and grants, and task8 for Shuffle/LSU arbitration. Neither the CCH
nor SCH full-DAG percentage may be used to accept cross-task cancellation.
