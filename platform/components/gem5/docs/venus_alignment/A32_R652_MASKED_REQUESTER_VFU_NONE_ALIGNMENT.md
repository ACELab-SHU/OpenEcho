# A32 R652 masked requester VFU_NONE alignment

Date: 2026-08-13

Status: **INCONCLUSIVE**. This round fixes one RTL-proven requester semantic
and improves CCH task20, but BitALU mask/result timing and the independent SCH
SerDiv/CAU/Shuffle paths are not yet cycle exact. Full-DAG cancellation is not
an acceptance criterion.

## Accepted generic change

`venus_lane_sequencer.sv` assigns `VFU_NONE` to every ordinary arithmetic
operand requester when `vm_r | vm_w` is true. The separately tagged mask
requester still targets the real VFU. gem5 previously tagged all requesters
with the instruction VFU, allowing a masked ordinary A/B/C/D requester to
consume producer chaining credit that RTL forbids.

The two requester eligibility sites in `VenusLane.cc` now use `VFU_NONE` for
non-mask requesters of masked instructions. The change applies to BitALU, CAU,
and SerDiv without task, sequence, opcode, PC, address, or data-value special
cases. LSU and VFU fixed latencies are unchanged.

An A/B control rebuilt from the pre-change source with the exact accepted
experimental flags produced a byte-identical task20 sequencer monitor. This
rules out compiler rebuild drift; the observed timing change comes from the
new requester rule.

## Direct RTL oracle and focused task20 result

The direct RTL task20 tail oracle establishes the following sequence-60 VXOR
lane-0 behavior:

- requester A captures hazard `0x24` with `vfu=7` (`VFU_NONE`);
- its registered hazard progresses `0x24 -> 0x20 -> 0` and cannot be cleared
  by producer chaining;
- the first A grant is the edge corresponding to gem5 9850 ns;
- the accepted gem5 result now grants both A and B first at 9850 ns.

Consequently sequence 59 VBRDCST changes from 31 to 30 cycles and is exact.
The first duration/recycle divergence advances to sequence 60:

| dimension | first sequence | operation | gem5 cycles | RTL cycles | delta |
|---|---:|---|---:|---:|---:|
| duration | 60 | VXOR | 32 | 34 | -2 |
| recycle | 60 | VXOR | 4717 | 4719 | -2 |
| fire | 63 | VXOR | 4708 | 4707 | +1 |

Task20 remains 8,837/8,837 VINS exact.

The remaining sequence-60 shape is downstream of requester admission. gem5
enqueues/grants result rows continuously, while RTL starts its result request
one cycle later and inserts one bubble between result rows 3 and 4. RTL
`venus_bitalu_wrapper.sv` explains both boundaries:

- arithmetic consumes only registered `operand_mask_valid_q`;
- mask handshake writes `operand_mask_valid_d` independently of A/B readiness;
- the latch clears on a full mask row or final issue;
- result payload/valid/count/pointers are explicit `result_queue_d/q` state.

A prototype that delayed mask consumption inside the combined A/B/mask VFU
consume path made sequence 60 duration exact but regressed previously exact
sequence 57 by two cycles and changed VINS values. It was rejected and fully
reverted. The RTL mask intake must instead be modeled as an independent
tagged D/Q latch that can prefetch while A/B wait.

## Focused task17 control

Task17 is unchanged and remains 687/687 VINS exact. Its first duration/recycle
difference is sequence 180 VBRDCST at -1 cycle; its first fire difference is
sequence 194 VLOAD at +1 cycle. This confirms that the new requester rule did
not perturb the unmasked control path.

## Full CCH timing

Comparison boundary is gem5 `tile_start -> task_epilogue` against RTL
`start execute -> execute complete`; return DMA and tile release are excluded.

| task | RTL us | gem5 us | delta us | error |
|---:|---:|---:|---:|---:|
| 0 | 0.547 | 0.536 | -0.011 | -2.011% |
| 1 | 10.691 | 10.680 | -0.011 | -0.103% |
| 2 | 0.547 | 0.536 | -0.011 | -2.011% |
| 3 | 69.631 | 68.736 | -0.895 | -1.285% |
| 4 | 4.019 | 4.010 | -0.009 | -0.224% |
| 5 | 0.895 | 0.886 | -0.009 | -1.006% |
| 6 | 0.531 | 0.522 | -0.009 | -1.695% |
| 7 | 0.895 | 0.886 | -0.009 | -1.006% |
| 8 | 0.531 | 0.522 | -0.009 | -1.695% |
| 9 | 158.799 | 158.710 | -0.089 | -0.056% |
| 10 | 0.295 | 0.288 | -0.007 | -2.373% |
| 11 | 0.375 | 0.366 | -0.009 | -2.400% |
| 12 | 0.375 | 0.366 | -0.009 | -2.400% |
| 13 | 0.563 | 0.556 | -0.007 | -1.243% |
| 14 | 202.779 | 202.994 | +0.215 | +0.106% |
| 15 | 6.443 | 6.368 | -0.075 | -1.164% |
| 16 | 1.343 | 1.336 | -0.007 | -0.521% |
| 17 | 33.167 | 33.172 | +0.005 | +0.015% |
| 18 | 123.235 | 123.234 | -0.001 | -0.001% |
| 19 | 2.439 | 2.418 | -0.021 | -0.861% |
| 20 | 398.523 | 405.132 | +6.609 | +1.658% |
| 21 | 9.303 | 9.306 | +0.003 | +0.032% |
| 22 | 100.055 | 100.052 | -0.003 | -0.003% |
| 23 | 33.951 | 33.944 | -0.007 | -0.021% |

The first-start to last-epilogue span is RTL 1,042.151 us versus gem5
1,048.092 us: **+5.941 us, +0.570%**. R648 was +8.533 us/+0.819%, so this
change removes 2.592 us from task20 and the same dominant loop reduces the
full-DAG error by 2.592 us. CCH emits 10,253 VINS and is task-local/value exact
against the accepted baseline.

Task20 remains the dominant absolute error. Task3 is next at -0.895 us. The
7--11 ns residual on many sub-microsecond tasks is a distinct CPU/task-boundary
signature; its percentage looks large only because the denominator is small.

## Full SCH timing

The masked-requester change does not alter SCH timing or output.

| task | RTL us | gem5 us | delta us | error |
|---:|---:|---:|---:|---:|
| 0 | 676.287 | 676.300 | +0.013 | +0.002% |
| 1 | 79.131 | 79.140 | +0.009 | +0.011% |
| 2 | 55.227 | 54.930 | -0.297 | -0.538% |
| 3 | 471.571 | 477.072 | +5.501 | +1.167% |
| 4 | 30.175 | 31.018 | +0.843 | +2.794% |
| 5 | 11.059 | 10.964 | -0.095 | -0.859% |
| 6 | 3.795 | 3.786 | -0.009 | -0.237% |
| 7 | 1.059 | 1.056 | -0.003 | -0.283% |
| 8 | 330.955 | 329.864 | -1.091 | -0.330% |

The full-DAG span remains RTL 1,534.187 us versus gem5 1,538.992 us:
**+4.805 us, +0.313%**. It still contains cancellation between slow task3/4
and fast task2/5/8. SCH emits 8,103 VINS and is exact against the accepted
functional baseline. Only 7,256 have retained direct RTL dumps; the other 847
are not claimed as an all-RTL comparison.

## Directed regressions

- LSU RAW/throughput/capacity: 68/68 cases pass.
- LSU result files: 382/382 byte-exact against the accepted suite.
- CCH: 10,253 VINS task-local/value exact.
- SCH: 8,103 VINS task-local/value exact.
- Final binary SHA256:
  `8ae9ce51e08bd4f125d3660df7ce4448c2c187d9cd46eea09a52f074417996ee`.

## Remaining module priorities

1. BitALU: implement independent tagged mask-latch D/Q and registered result
   row state, cross-checking task20 sequence 60 and the sequence 57 control.
2. CCH task20 after sequence 60: continue stable requester vector, explicit
   LSU high priority, persistent per-bank RR, and tagged retirement checks.
3. SCH task3: repeated VDIV/VMUL/VSADD/store overlap; current +5.501 us is the
   largest SCH absolute error.
4. SCH task4: SerDiv/CAU result D/Q and bank grants; current +2.794% is the
   largest material SCH relative error.
5. SCH task8: Shuffle admission/live intent and LSU contention; it is 1.091 us
   too fast.
6. CPU/task boundary: isolate the common 7--11 ns short-task residual with
   scalar-only start/epilogue cases. It must not be folded into vector latency.

## Evidence

- `evidence/a32_r652_masked_requester_vfu_none_20260813/rtl_oracle/`
- `evidence/a32_r652_masked_requester_vfu_none_20260813/focused/`
- `evidence/a32_r652_masked_requester_vfu_none_20260813/cch_full/`
- `evidence/a32_r652_masked_requester_vfu_none_20260813/sch_full/`
- `evidence/a32_r652_masked_requester_vfu_none_20260813/lsu_suite/`

