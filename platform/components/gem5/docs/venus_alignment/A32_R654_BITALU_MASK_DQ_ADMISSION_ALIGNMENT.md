# A32 R654 BitALU mask D/Q and requester-admission alignment

Date: 2026-08-13

Status: **INCONCLUSIVE**. This round replaces two guessed masked-operation
timing effects with RTL-proven state boundaries and advances task20's first
microtiming divergence from sequence 60 to sequence 64. The new sequence-64
residual contains an instruction-local `fire -9 / duration +8` cancellation,
so neither its nearly exact recycle edge nor the full-DAG total is an
acceptance criterion.

## Accepted generic changes

### Independent tagged BitALU mask D/Q latch

`venus_bitalu_wrapper.sv` accepts its mask independently of the A/B operand
handshake. The mask enters `operand_mask_valid_d`, becomes visible through
`operand_mask_valid_q` on the next lane edge, remains valid for four EW8
arithmetic beats, and then crosses a registered clear/refill boundary.

`VenusLane` now represents that structure explicitly:

- mask rows carry instruction and running-ID generation tags;
- D capture is independent of A/B readiness;
- Q visibility occurs one lane edge after capture;
- a row is retained for four banks and cleared through D/Q;
- the next row cannot be consumed on the clear edge;
- failed A/B admission rolls back only the operands actually popped;
- masked BitALU operations no longer count a second synthetic ingress pipe
  stage on top of the explicit mask/result registers.

This is a VFU-local structural model. It has no task, sequence, PC, address,
data-value, or workload condition, and no LSU/VFU fixed latency was changed.

### Hazard-bearing mask requester admission

`venus_mask_operand_requester.sv` accepts a new command in `IDLE` even when
the command's captured hazard vector is nonzero. It then waits in
`REQUESTING` until the registered hazard clears. Unlike an ordinary operand
requester, it has no `raw_hazard_counter` or writeback-chaining path.

The gem5 mask requester now follows the same split:

- command admission does not wait for hazard clear;
- the full tagged hazard vector is still captured;
- mask requesters are excluded from producer chaining credit;
- VRF access remains blocked until the registered hazard becomes clear.

This corrects command-register occupancy and upstream lane ready timing
without weakening any dependency.

## BitALU RTL oracle and focused result

The direct lane-0 task20 oracle establishes both relevant mask shapes:

- sequence 57 captures its mask independently, exposes Q one edge later, and
  retains the row for four arithmetic beats;
- sequence 60 performs the same D/Q transition, inserts the real clear/refill
  bubble between row 0 and row 1, and uses the registered result queue.

After the independent latch change, sequence 57 and sequence 60 are exact.
After the requester-admission change, sequences 62 and 63 are also exact.
Task20 remains 8,837/8,837 VINS exact.

The first remaining divergence is sequence 64:

| dimension | operation | gem5 cycles | RTL cycles | delta |
|---|---|---:|---:|---:|
| fire | VSADD | 4713 | 4722 | -9 |
| duration | VSADD | 63 | 55 | +8 |
| recycle | VSADD | 4776 | 4777 | -1 |

This is not convergence: the early fire and long duration almost cancel at
recycle.

## New sequence-64 CAU oracle

A fresh RTL UCLI oracle samples lane 0 every 2 ns from 270.641 us through the
sequence-64 lifetime. Sequence 64 is assigned at 270.658994 us and follows
this registered path:

- edge 22: lane emits `vfu_operation_valid`, ID 3, VSADD;
- edge 23: CAU instruction Q becomes ID 3 with issue/process/commit count 16;
- edges 33--40: the mask queue still carries the preceding BitALU mask;
- edge 41: target changes to CAU, but no mask payload is valid yet;
- edge 48: CAU A is valid and the sequence-64 mask handshakes into D;
- edge 49: B and mask Q are valid and the first result enters result-queue D;
- edge 52: result queue reaches the real two-entry backpressure point;
- edge 55: row 0 clears and row 1 handshakes into mask D;
- edge 56: row 1 is Q-visible and arithmetic resumes;
- edge 59: ID 3 asserts done; RTL releases it at 270.762998 us.

The corresponding gem5 trace preserves the same downstream shape and retires
only one cycle early. Its large duration error begins earlier: the sequencer
records sequence 64 as fired while the lane fall-through register still waits
for sequence 63's shared mask-command register to clear. Thus the next target
is the boundary between main-sequencer issue visibility, lane fall-through
capture, and the shared one-entry mask command, not CAU arithmetic latency.

## Focused task17 control

Task17 remains 687/687 VINS exact. Its first duration/recycle divergence is
still sequence 180 VBRDCST at -1 cycle, and its first fire divergence remains
sequence 194 VLOAD at +1 cycle. The new masked requester rules do not perturb
this unmasked control.

## Full CCH timing

The comparison boundary is gem5 `tile_start -> task_epilogue` against RTL
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
| 20 | 398.523 | 395.340 | -3.183 | -0.799% |
| 21 | 9.303 | 9.306 | +0.003 | +0.032% |
| 22 | 100.055 | 100.052 | -0.003 | -0.003% |
| 23 | 33.951 | 33.944 | -0.007 | -0.021% |

The first-start to last-epilogue span is RTL 1,042.151 us versus gem5
1,038.300 us: **-3.851 us, -0.370%**. The preceding candidate was
+6.805 us/+0.653%; the 10.656 us movement is entirely task20. This aggregate
overshoot is retained because sequences 57, 60, 62, and 63 now satisfy the
direct RTL structure. It also proves why aggregate error cannot select a
model.

CCH emits 10,253 VINS and is task-local/value exact against the accepted
functional baseline.

## Full SCH timing

SCH timing and output are unchanged:

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
**+4.805 us, +0.313%**. Task3/4 being slow and task2/5/8 being fast still
cancel. SCH emits 8,103 VINS exact against the accepted gem5 functional
baseline. Of those, 7,256 have retained direct RTL dumps; the other 847 are
not claimed as an all-RTL comparison.

## Directed regressions

- LSU RAW/throughput/capacity: 68/68 pass.
- LSU result files: 382/382 byte-exact against the preceding accepted suite.
- BitALU result queue reaches depth 2/full at 4 ns and 64 ns grant gaps.
- CAU result queue reaches depth 2/full at 4 ns and 64 ns grant gaps.
- SerDiv result queue reaches depth 2/full at the 64 ns grant gap.
- All VFU capacity-case outputs are exact across 1/4/64 ns grant gaps.
- CCH: 10,253 VINS exact.
- SCH: 8,103 VINS exact.
- Final binary SHA256:
  `b1dd7e91485de08f70702deca3c5b7edac0678edbf1c9f00053b250a1526e469`.

## Remaining module priorities

1. Align the main-sequencer fire boundary with RTL `pe_req_valid_o`, then
   cross-check lane fall-through capture and shared mask-command release on
   task20 sequences 63--65. Sequence 64 may not be accepted based on recycle
   alone.
2. Preserve a stable tagged requester vector until per-bank grant; implement
   LSU-over-arithmetic priority and persistent independent RR state per bank.
3. Split CAU and SerDiv mask D/Q state only where a direct oracle proves the
   existing tagged operand-response boundary is insufficient.
4. Independently continue SCH task3 SerDiv/CAU/LSU overlap, task4 SerDiv
   result D/Q, and task8 Shuffle/LSU contention.
5. Isolate the common 7--11 ns short-task CPU/start/epilogue residual with
   scalar-only cases rather than folding it into vector latency.

## Evidence

- `evidence/a32_r654_bitalu_mask_dq_admission_20260813/rtl_oracle/`
- `evidence/a32_r654_bitalu_mask_dq_admission_20260813/focused/`
- `evidence/a32_r654_bitalu_mask_dq_admission_20260813/cch_full/`
- `evidence/a32_r654_bitalu_mask_dq_admission_20260813/sch_full/`
- `evidence/a32_r654_bitalu_mask_dq_admission_20260813/lsu_suite/`
- `evidence/a32_r654_bitalu_mask_dq_admission_20260813/vfu_capacity/`
