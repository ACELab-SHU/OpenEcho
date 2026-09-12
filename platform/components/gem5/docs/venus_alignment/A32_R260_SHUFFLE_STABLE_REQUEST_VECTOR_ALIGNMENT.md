# A32 R260 Shuffle stable request-vector alignment

Date: 2026-08-12

## Scope and acceptance rule

This round continued the vector requester, per-bank VRF arbitration, and
vector-LSU alignment.  Acceptance is based on the first cycle-level
request/grant divergence plus directed functional/capacity regressions.  A
small total-DAG error or cancellation between task17/task18/task20 is not an
acceptance criterion.

No task ID, PC, DAG name, address, payload, or fixed-latency special case was
added.

## Accepted structural change

`VenusShufflePipline` now records the exact registered PE request vector which
selected a blocked lane request.  When the tagged request is eventually
granted, the lane FairArb next-priority state is computed from that saved
vector rather than from later PE requests which happened to become visible
while the downstream bank was blocked.

The implementation adds `laneBlockedRequests` and clears it only after the
matching tagged grant.  The required model configuration remains:

```sh
VENUS_GEM5_EXPERIMENTAL_VRF_RR=1
VENUS_GEM5_EXPERIMENTAL_REQUESTER_Q_VISIBILITY=1
```

## RTL oracle and first remaining divergence

A fresh internal task20 RTL trace was captured with:

- TCL: `/home/shenyihao/.codex/tmp/a32_task20_shuffle_internal.tcl`;
- decoded trace: `/tmp/a32_task20_shuffle_internal.txt`;
- RTL run directory:
  `/home/shenyihao/Project/Venus_3/venus_soc/sim/build_gc0802_overall_nrPDCCH_tv9_r52debug`.

The accepted candidate preserves the operand-queue exact window: RTL queue
cycle 68 versus gem5 tick 556136000, relative cycles 0--60, has zero event and
zero registered-usage mismatches.  The first per-bank request/grant mismatch
is still relative cycle 89:

- RTL cycle 157 grants lane0/bank0 to Shuffle PE0;
- gem5 first grants the corresponding PE0 write at relative cycle 91;
- RTL's next write is relative cycle 92, while gem5's is relative cycle 93.

The remaining cause is now explicit.  The Shuffle lane-local
`rr_arb_tree` is instantiated with `LockIn=0`.  At relative cycle 84 the
downstream LSU priority suppresses a PE8 request; at relative cycle 85 PE0 has
also become visible and RTL re-arbitrates the live `[PE0, PE8]` vector to PE0.
The current gem5 timing-port retry retains PE8, so PE0's DATA and WRITE phases
become one to two cycles late.  This is a requester/XBar handshake-model gap,
not a Shuffle, LSU, or VFU fixed-latency gap.

## Full-chain functional and timing result

The full-chain accepted-candidate binary was
`4ace07fbc4c49e13c931c74d9475a35c370ed76220a4e9a98c1a9a5caa8f44ec`.
The source-equivalent post-rejection rebuild is
`fd9392b560b3a52f2d417cf2b6f8abdce4642004df8e22f4dba930bacef323a6`.

### CCH / nrPDCCH

- 24/24 tasks complete;
- 10,253 task-local VINS are byte-exact to R253;
- machine-readable timing:
  `/tmp/a32_r254_stable_shuffle_cch/task_execution_timing_vs_rtl.json`;
- VINS comparison:
  `/tmp/a32_r254_stable_shuffle_cch/vins_ordinal_vs_r253.json`.

| task | delta ns | error |
|---:|---:|---:|
| 0 | -11 | -2.011% |
| 1 | +199 | +1.861% |
| 2 | -11 | -2.011% |
| 3 | +153 | +0.220% |
| 4 | -9 | -0.224% |
| 5 | -9 | -1.006% |
| 6 | -9 | -1.695% |
| 7 | -9 | -1.006% |
| 8 | -9 | -1.695% |
| 9 | +121 | +0.076% |
| 10 | -7 | -2.373% |
| 11 | -9 | -2.400% |
| 12 | -9 | -2.400% |
| 13 | +49 | +8.703% |
| 14 | +1,085 | +0.535% |
| 15 | +27 | +0.419% |
| 16 | +15 | +1.117% |
| 17 | +175 | +0.528% |
| 18 | +3,465 | +2.812% |
| 19 | +21 | +0.861% |
| 20 | -7,419 | -1.862% |
| 21 | +131 | +1.408% |
| 22 | +5 | +0.005% |
| 23 | -7 | -0.021% |

Relative to R253, task17 improves from +491 ns to +175 ns.  Task20 worsens
from -6,847 ns to -7,419 ns.  The accepted component-level correction is not
therefore presented as task20 timing convergence.

### SCH / nrPDSCHDag2 held-out

- 9/9 tasks complete;
- 8,103 task-local VINS are byte-exact to R253;
- machine-readable timing:
  `/tmp/a32_r254_stable_shuffle_sch/task_execution_timing_vs_rtl.json`;
- VINS comparison:
  `/tmp/a32_r254_stable_shuffle_sch/vins_ordinal_vs_r253.json`.

| task | delta ns | error |
|---:|---:|---:|
| 0 | +17 | +0.003% |
| 1 | +13 | +0.016% |
| 2 | +1,723 | +3.120% |
| 3 | +6,933 | +1.470% |
| 4 | +1,023 | +3.390% |
| 5 | +125 | +1.130% |
| 6 | -9 | -0.237% |
| 7 | -3 | -0.283% |
| 8 | +985 | +0.298% |

Only task2 changes by +6 ns and task6 by +2 ns; the principal SCH residuals
are not explained by this Shuffle correction.

## Directed regression on the final rebuild

- LSU RAW/throughput/capacity:
  `/tmp/a32_r260_rebuilt_stable_qrr_lsu_suite/suite_results.json`, 68/68;
- all 382 VINS files are byte-exact to the accepted R253 suite;
- VFU result queues:
  `/tmp/a32_r260_rebuilt_stable_qrr_capacity/capacity_results.json`;
- all grant-gap outputs are exact to the 1 ns control;
- BitALU, CAU, and SerDiv each directly reach occupancy 2 and an observed
  full condition; every enqueue count equals its dequeue count.

## Rejected follow-up experiments

Two attempts to implement `LockIn=0` by changing the PE owner only at gem5's
retry callback were rejected and fully removed:

1. cross-bank intent migration produced stale retry ownership and then
   live-lock;
2. restricting replacement to the same physical bank did not reach the
   task20 debug window after 12 minutes and changed earlier Shuffle progress.

The second run is retained only as rejected evidence in
`/tmp/a32_r259_same_bank_reselect`; it is not an accepted timing result.

## Next breakpoint

The next implementation must expose the lane-local combinational Shuffle
arbiter and downstream per-bank arbiter as one explicit intent/grant
interface.  A PE identity may change while `gnt_i=0`, but the XBar may commit
RR and packet ownership only for the PE/bank pair granted on that edge.  This
must be verified first against task20 relative cycles 84--93, then against
task17/task18 and the full LSU/CCH/SCH gates above.

