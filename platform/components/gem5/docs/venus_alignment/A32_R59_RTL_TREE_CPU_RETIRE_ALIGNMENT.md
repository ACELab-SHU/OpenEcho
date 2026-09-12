# A32 R59 RTL tree winner / scalar retire alignment

Date: 2026-08-09

Status: **functional and directed gates pass; timing alignment is not complete**.

## Model changes

This iteration makes two profile-gated structural corrections. It does not
change a fixed LSU or VFU latency and contains no task, PC, DAG, address, or
payload special case.

1. The VRF arbiter now separates the two outputs of RTL `rr_arb_tree.sv`:
   the current winner is selected by the binary tree using current `rr_q`,
   while FairArb mask/LZC chooses the next `rr_q`. The chosen requester and
   next-RR value remain tagged until the request actually crosses the bank.
2. Under `venusRtlScalarTiming`, a `VenusStaticInst` consumes the configured
   one-instruction Minor commit slot even though it uses the no-cost FU.
   Before this correction several already-issued Venus instructions could
   call `sendVenusInstrPkt` in one gem5 edge; scalar600 has a single-retire
   boundary.

Candidate binary SHA256:

    34bbc854bb9f9002f7cd4788aed4898aa9eeda1cbb7e1237d120425e2a762c1a

## Local edge results

With the corrected RTL tree winner, task17 sequence 6 is edge-exact in the
R58 focused trace:

- fire: `426304000`
- operand admission: `+8/+14 ns`
- operand grants: `+50/+54/+56/+58 ns`
- result enqueue/grants: `+56/+60/+62/+64 ns`
- lane retirement/recycle: `+66/+68 ns`

The collision witness at `426314000` has `old_rr=8`, current winner master 8,
`next_rr=1`, and two contenders. This is the key case where using cyclic
selection for the current winner disagreed with RTL.

The scalar-retire change is independently justified but is not a task17 VFU
fix. In the R59 full run, task17's first duration difference moves from
sequence 9 to sequence 1. Recycle edges for the first ten instructions remain
mostly exact because the one-cycle fire and duration changes compensate.

## Full-chain timing

All values are allocation-to-last-release or measured task duration and use
the frozen R55 RTL references.

### nrPDCCH / CCH

- full DAG: RTL `1,043,224 ns`, gem5 `1,044,616 ns`, delta `+1,392 ns`
  (`+0.133%`)
- task9: `+11,729 ns` (`+7.386%`)
- task17: `+341 ns` (`+1.028%`)
- task20: `-12,371 ns` (`-3.104%`)
- task22: `+4,841 ns` (`+4.838%`)
- task23: `+4,625 ns` (`+13.623%`)

The full-DAG result is not an acceptance result. Task9 and task20 alone still
cancel by more than 12 us.

### PDSCH / SCH held-out

- full DAG: RTL `1,535,100 ns`, gem5 `1,541,808 ns`, delta `+6,708 ns`
  (`+0.437%`)
- task0: `+0.343%`
- task2: `+8.193%`
- task4: `+5.531%`
- task7: `+20.113%`
- task8: `+0.425%`

The CPU correction has mixed cross-workload timing impact: it improves task4
relative to R55, leaves task2/task7 essentially unresolved, and slightly
worsens the full-DAG number. It is retained because it closes an observed CPU
contract violation, not because of aggregate timing.

## Functional and capacity gates

- CCH: 10,253 task-local VINS are byte-exact to the accepted R55 gem5
  baseline.
- SCH: 8,103 task-local VINS are byte-exact to the accepted R55 gem5
  baseline. This does not upgrade the historical RTL claim: only 7,256 have
  retained RTL dumps; 847 still lack an RTL dump.
- LSU RAW/throughput/capacity suite: 68/68 pass.
- BitALU, CAU, and SerDiv result queues all reach occupancy 2 and an actual
  full condition; every grant-gap output is exact to its control.

## Next first-divergence work

Task20 has two distinct errors which must not be fitted against each other:

1. Sequence 2 `VSTORE` fires 757 tile cycles later than RTL (`3424` versus
   `2667` cycles from task-local origin). This is an initial scalar/LSU
   admission or dependency-release gap.
2. Across all 8,837 instructions, gem5 then advances from `+757` cycles to
   `-6370` cycles at the final fire. This is a recurrent issue/admission and
   bank-conflict phase gap. It cannot be repaired by changing the initial
   store latency.

The next iteration must capture those two windows separately. For the loop,
compare transition classes and the exact requester winner: large opposing
terms such as `VBRDCST->VSGT` and `VBRDCST->VBRDCST` already show another
within-task cancellation. Raw percentage and full-DAG totals remain
non-acceptance metrics.
