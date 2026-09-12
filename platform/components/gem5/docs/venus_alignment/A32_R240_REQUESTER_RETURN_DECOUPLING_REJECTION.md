# A32 R240: requester grant/return decoupling audit

Date: 2026-08-11

Verdict: `REJECTED`

## Question

RTL `venus_operand_requester.sv` advances `requester_q` from a VRF grant,
while the DSPM response and operand-queue data capture are later registered
boundaries.  The accepted gem5 model still completes a requester from returned
data.  R240 tested whether separating those ownership lifetimes closes the
remaining requester and bank-arbitration residuals.

The candidate was structural and generic: returned data retained instruction,
running-ID, operand-passage, and row tags; no task, PC, address, opcode, or
payload rule was added.  A per-requester same-edge grant guard was also needed
because RTL may accept the following command on the final-grant edge but its
new bank request is visible only after the next `requester_q` edge.

## Directed gates

The first task17 replay exposed a duplicate scheduled requester event after a
stable-owner retry.  Cancelling the superseded old-generation retry removed
that failure.  The first full CCH attempt then caught two issues to the same
operand queue on one tile edge (`usage 3 +2 > depth 4`).  Adding the registered
same-edge grant guard removed the overflow, and the complete 24-task CCH run
finished normally.

The completed candidate preserved all 10,253 CCH task-local VINS byte-exactly
against R237.  Functional equality was therefore not used as timing
acceptance.

## Timing rejection

Matched execution-boundary deltas changed as follows:

| Task | R237 | R240 candidate | Change |
| ---: | ---: | ---: | ---: |
| 0 | -7 ns | +93 ns | +100 ns |
| 2 | -7 ns | +93 ns | +100 ns |
| 3 | +105 ns | +9 ns | -96 ns |
| 6 | -5 ns | +83 ns | +88 ns |
| 8 | -5 ns | +83 ns | +88 ns |
| 9 | +125 ns | +195 ns | +70 ns |
| 14 | +1,093 ns | +1,213 ns | +120 ns |
| 17 | +501 ns | +573 ns | +72 ns |
| 20 | -5,393 ns | +4,153 ns | +9,546 ns |

The candidate moves task20 across RTL by 9.546 us, worsens task17, and creates
large percentage errors in short tasks.  Therefore grant/return decoupling
cannot be enabled uniformly.  The missing model includes requester-specific
registered ready/credit state and its interaction with the shared bank vector;
it is not a universal response-lifetime rule.

All R240 source changes were removed.  The retained source remains R237's
directional addrgen acknowledgement, stable tagged requester owner, independent
LSU priority intent, and persistent per-bank RTL-tree RR model.  The rejected
candidate evidence is under:

- `/tmp/a32_r240_decoupled_requester_task17`
- `/tmp/a32_r240b_decoupled_requester_task17`
- `/tmp/a32_r240_decoupled_requester_cch`
- `/tmp/a32_r240c_decoupled_requester_cch`

The rebuilt restored binary has SHA-256
`dd6afa1effebba599f7dcacee73d5d41eafd6f2072ed01734f578c24b34cf1f3`.
Post-restore gates pass:

- LSU RAW/throughput/capacity: 68/68 in
  `/tmp/a32_r241_restored_lsu_suite/suite_results.json`;
- result-queue capacity: passed in
  `/tmp/a32_r241_restored_capacity/capacity_results.json`;
- BitALU and CAU reach depth 2/full at 4--128 ns grant gaps;
- SerDiv reaches depth 2/full at 64 and 128 ns;
- every stressed-gap dump is exact to the 1 ns control.

## Next boundary

The next requester change must be driven by a per-requester RTL vector trace:
command valid/ready, operand-queue ready, stable bank request, selected owner,
grant, and registered occupancy on the same edges.  In particular, task20 must
be split at the first point where R237 and RTL choose different surviving
request vectors; task17's mostly single-contender early window should not be
used to tune the bank selector.
