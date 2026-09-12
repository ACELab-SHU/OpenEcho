# A32 R682: mask-bank RR and task20 sequence-66 localization

Date: 2026-08-13

## Scope

This step continues the cycle-level comparison after A32 R654.  It does not
fit total DAG time and does not change fixed LSU or VFU latency.  The focused
oracle is nrPDCCH task20, with sequence57 and sequence46 retained as already
exact protection points.

## Structural change retained

RTL `venus_mask_operand_requester.sv` gives each lane-local mask SRAM a
persistent two-input fair RR arbiter: mask read is input 0 and BitALU mask
write is input 1.  gem5 previously applied the persistent per-bank Venus RR
only to the 64 data banks; the 16 mask banks fell through the generic XBar
event order.

The XBar now models all 80 arbitrated banks.  A mask-bank request is decoded
from its lane and local port, with requester 0 for mask read and requester 1
for mask write.  Each bank retains independent RR state.  LSU priority and
the 12-requester data-bank tree remain data-bank-only.  There is no
task/opcode/PC/address/data special case.

## Focused result

With the correct `venus-rtl-16x128` profile and all accepted experimental
structure switches enabled:

- task20 completes at tick 397,080,000, unchanged from the accepted control;
- 8,837 VINS compare exact to R653e, with no mismatches;
- sequence57 and all earlier instruction timing remain unchanged;
- the first duration/recycle divergence remains sequence66 VSLE:
  gem5 62 cycles versus RTL 63 cycles, delta -1;
- the first fire divergence remains sequence70 VMUL, delta -1 cycle.

Thus the mask-bank RR is a real missing hardware structure but is not the
direct owner of the sequence66 first divergence in this trace.

## Rejected boundary experiments

Two wider timing hypotheses were tested and fully reverted:

1. Delaying every BitALU mask completion by one cycle moved the first
   divergence backward to sequence57 (+1 cycle).
2. Treating every newly received operand row as unavailable for another
   edge moved the first divergence backward to sequence33 VRANGE (+3 cycles)
   and increased task20 completion by 5.480 microseconds.  Restricting that
   rule to BitALU A/B still exposed sequence33 +3 cycles through an earlier
   local producer-consumer chain and increased completion by 1.448
   microseconds.

These failures are important: RTL `venus_operand_queue.sv` keeps requester
credit (`operand_issued_i` / `ibuf_usage_q`) distinct from actual data FIFO
arrival (`operand_valid_i`).  The next model must preserve those two tagged
events rather than translate either into a blanket fixed delay.

## Sequence66 edge diagnosis

The accepted gem5 trace and RTL oracle agree through the producer retirement
and first A-bank grant:

- gem5 sequence65 final CAU dequeue: 10,022 ns;
- gem5 local retirement: 10,024 ns;
- gem5 sequence66 A hazard clear/request/grant: 10,026 ns;
- RTL producer `pe_done`: oracle edge146;
- RTL sequence66 A grant: edge147.

The remaining one-cycle difference is after that grant.  RTL separately
advances requester issued/credit, VRF response data, `fifo_v3` data-valid,
BitALU operand handshake, result-queue D/Q, mask grant, and `pe_done`.  gem5
still coalesces part of the issued/data-valid path in some queue states.  A
correct fix therefore needs stable tagged response payloads and a separate
per-requester response-to-ibuf event, checked against both an empty and a
non-empty/full BitALU result queue.

## Evidence

- `evidence/a32_r682_mask_bank_rr_20260813/focused/task20_timing_vs_rtl.json`
- `evidence/a32_r682_mask_bank_rr_20260813/focused/task20_vins_vs_r653e.json`
- `/tmp/a32_r663_task20_bitalu_phase_oracle.txt`
- `/tmp/a32_r674_task20_bitalu_requester_dual_oracle.txt`

Accepted binary SHA256:

`363c1814e578f81cdaff3ff24b3328a0f30a1496dd74ed99fe5ec9d036c281bc`

## Next gate

Instrument one RTL/gem5 table for each BitALU A/B request row containing:
requester command tag, request/grant, `operand_issued`, VRF data-valid,
`fifo_v3` empty/usage, VFU valid/ready, result enqueue/dequeue, mask grant and
retirement.  First validate a minimal empty-queue case and a depth-2/full
case, then re-run sequence46, sequence57 and sequence66 before any full CCH or
SCH timing acceptance.

## 2026-08-13 follow-up: sequence57/66 internal boundary oracle

The missing oracle above has now been collected for lane 0 and cross-checked
against all 16 lanes.  All lanes are cycle-synchronous for sequence66, so the
one-cycle gap is not a slow-lane reduction artifact.

For sequence57 (B-only MVX), RTL observes the first B grant, response push,
non-fall-through data-FIFO Q, mask D/Q latch, ALU result D/Q, mask grants at
addresses 3 and 7, and lane retirement as separate edges.  For sequence66
(A+B MVV), B first fills the four-entry data FIFO while A is RAW/bank blocked;
after A is released, each grant is followed by response push and data-FIFO Q
on successive edges.  RTL's first result appears only after those boundaries,
and the final mask grant and lane/PE done are also registered separately.

The gem5 trace exposes a concrete model defect: `bitaluResultQueue` can enqueue
and dequeue a newly born entry in one event, while the registered result-count
bookkeeping still reflects the operand-handshake reservation.  This produces
diagnostic counts such as `-1/2`; capacity reservation and actual
`result_queue_q` occupancy are therefore not yet independent state variables.

Four candidate shortcuts were tested and rejected/reverted:

1. result-Q visibility for every BitALU instruction moved the first divergence
   to sequence1 and increased task20 to 412.708 microseconds;
2. restricting it to masked BitALU moved the first divergence to sequence57
   (25 to 26 cycles) and increased task20 to 399.672 microseconds;
3. conditioning it on asymmetric operand prefill moved the first divergence
   to sequence63 and made sequence66 64 rather than 63 cycles;
4. allowing at most one result dequeue per gem5 tick did not change task20 or
   remove the negative registered count.

All four experiments were workload-independent, but none satisfies the first-
divergence gate, so none is retained.  The accepted task20 point remains
397.080 microseconds with 8,837 VINS exact, sequence57 exact, and sequence66
62 versus RTL 63 cycles.  The next implementation must split (a) elastic
pipeline/capacity reservation, (b) result-queue D, and (c) result-queue Q
occupancy before changing timing again.

Additional transient evidence:

- `/tmp/a32_r695_task20_seq66_all_lanes_oracle.txt`
- `/tmp/a32_r696_task20_seq66_boundary_oracle.txt`
- `/tmp/a32_r699_task20_seq57_boundary_oracle.txt`
- `/tmp/a32_r703_one_dequeue_per_tick_task20/timing.json`
