# A32 R721 BitALU handshake/result-Q negative-oracle audit

Date: 2026-08-14

## Accepted result

No timing candidate from this audit was promoted.  The workspace and binary
were restored to the accepted R682 behavior and rebuilt.  Task20 completes at
tick `397080000`; its 8,837 VINS files and complete sequencer monitor are
byte-identical to the restored R717 baseline.  The first lifecycle difference
remains sequence 66 `VSLE`: gem5 62 cycles versus RTL 63 cycles.  The first
fire difference remains sequence 70, gem5 one cycle early.

Restored binary SHA256:

    ec5a6d11bac91e39564928a11e27467eedb18881d5d44555beb3dfc91fbe4f41

Restored evidence:

    /tmp/a32_r721_task20_restored/

## Direct RTL facts added in this audit

The full-DAG sequence-1 `VBRDCST` oracle records the normal BitALU queue and
bank behavior without a synthetic micro-task phase offset:

- operand-B issue at edge 17 and FIFO push at edge 18;
- first combinational ALU/result-queue-D at edge 19;
- result-queue-Q at edge 20;
- one real bank conflict at edge 21, then the blocked result grants at edge 22;
- final grant at edge 32, combinational done at edge 32, and queue-Q empty at
  edge 33.

Oracle:

    /tmp/a32_r712_task20_seq1_vrf_result_oracle.txt

For sequence 66, RTL produces result-queue-D on edges 58, 59, 60, 62, 64, 65,
66, and 67.  The two external mask-row grants occur on edges 63 and 68: one
registered edge after the corresponding row-boundary result.  Gem5 produces
the same result-enqueue edge pattern but currently grants those two mask rows
on the enqueue edge.  This is the direct owner of the local `-1` endpoint, but
it is coupled to mask-latch clear/refill and combinational arithmetic.

Oracles:

    /tmp/a32_r695_task20_seq66_all_lanes_oracle.txt
    /tmp/a32_r696_task20_seq66_boundary_oracle.txt

## Rejected structural candidates

1. Delaying every external mask result until the following edge moved task20
   to tick `398520000`.  It made the previously exact sequence 57 `VSGT` slow
   by one cycle and made sequence 66 two cycles slow, because the delayed
   first-row grant also delays mask clear/refill and the next row's ALU beat.
2. Moving all result reservations to gem5's early operand-data take, admitting
   from queue-Q only, and removing same-edge retry made sequence 1 slow by five
   cycles and task20 complete at tick `413864000` or `408104000` depending on
   pipe placement.  The early data take is not the RTL arithmetic handshake.
3. A separate pending-capacity reservation prevented FIFO overbooking but
   reproduced the same sequence-1 `+5` regression.  Capacity reservation and
   RTL result-queue D/Q state cannot be represented by one occupancy value.
4. Independent tagged A/B staging was timing-neutral by itself: all 8,837
   monitor entries matched the accepted baseline.  Combining it with external
   result-Q visibility still reproduced the sequence-57 `+1` regression.
5. Allowing an old staged masked operand to calculate in the current edge
   exposed a `result_count 0 -> -1` transition until reservation was moved to
   that handshake.  After repairing the invariant, its observable timing was
   still identical to the rejected external-Q candidate.

All rejected source changes were removed.  None used task, sequence, PC,
address, or data-value conditions.

## Next required model boundary

The next implementation must be a single clocked BitALU wrapper transition,
not another latency adjustment.  It must update, from one pre-edge snapshot:

1. tagged A/B data-valid FIFO Q;
2. independent tagged mask-latch Q/D;
3. arithmetic operand valid/ready handshake;
4. result-queue enqueue/dequeue and count Q/D;
5. result requester Q presented to mask/data-bank arbitration;
6. mask-row clear/refill and lane done/retirement.

The transition must cross-check sequence 1, sequence 57, and sequence 66 in
the same build.  Only after those three oracles agree should task17, LSU 68,
and full CCH/SCH regression run.

