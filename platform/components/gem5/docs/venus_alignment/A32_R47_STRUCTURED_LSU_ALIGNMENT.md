# A32 R47 structured LSU alignment

## Scope

This work continues from the accepted A32 R47 dispatcher/barrier baseline.
It does not replace or relax the R47 two-entry dispatcher, pending-state
barrier coverage, or one-cycle non-bypass Shuffle response.

The LSU calibration set consists only of standalone directed cases:

- load/store RAW chains;
- independent load/store streams;
- a 4096-byte blocker with follower counts 2, 3, 4, 5, and 6 around the
  RTL-derived four-entry unit capacity.

`PDSCHDag2 A32 R2` remained held out until the directed implementation and
validation were complete.

## First divergence and falsifiable hypothesis

The old gem5 LSU performed the whole data move at issue and reported a fixed
three-cycle completion. In the first traced EW8/VL16 load:

- RTL: request +14, response +18, completion +27 cycles;
- old gem5: request +3, response +3, completion +6 cycles.

The first divergence was therefore the response collapsing onto the request,
not a full-DAG total-latency error.

Long-burst and cliff traces then separated the mechanisms:

- shared 64-byte AXI beats have a two-tile-cycle cadence;
- load and store commit tails are distinct;
- LDU and STU each have four unit slots;
- LDU can hold one independently advancing addrgen request when those slots
  are full;
- STU backpressures sequencer admission at four outstanding entries;
- independent LDU/STU sequencer initiation intervals are nine cycles;
- store response waits for source operands when producer data is not ready.

## Implementation

`src/venus/VenusSequencer.cc` and `.hh` now model LSU work as ordered phases:

1. `Request`
2. optional `StoreOperands`
3. `Response`
4. `Complete`

The implementation includes:

- separate per-operation four-entry capacity checks;
- independent LDU/STU admission-ready timestamps;
- a shared request/response channel availability timestamp;
- byte-count-derived 64-byte beat and 128-byte VRF-row counts;
- a per-instruction record of addrgen progress while an LDU request is held;
- delayed functional load/store movement at response rather than issue;
- load/store-specific commit tails;
- task-independent JSONL accept/request/response/completion tracing.

No task ID, PC, instruction sequence, payload value, DAG name, or fixed
full-DAG latency is used. The rejected
`VENUS_GEM5_VLOAD_LAT`/`VENUS_GEM5_VSTORE_LAT` fitting path is absent.

RTL observation is implemented in
`venus_soc/sim/model/venus_extension/venus_full_dag_perf_monitor.svh` with
`+venus_trace_lsu_task=N`. It records addrgen ack, AXI AR/AW, R/B, store
beats, VRF grants, and final completion without changing ready/valid.

Directed tooling:

- `tools/venus_lsu_microbench.py`
- `tools/venus_lsu_suite.py`
- `tools/run_venus_lsu_suite.py`
- `tools/compare_lsu_events.py`

Store throughput/capacity cases include a pre-measurement scalar drain so
their source rows are ready. This keeps producer latency in the RAW suite
instead of allowing follower-count-dependent broadcast work to contaminate
the capacity experiment.

## Directed validation

Final gem5 binary SHA-256:

`ee1be0da0e1b5eef3721552bb7b36deec2d14d16b05d6f2ac0c71350cee033bc`

The final 68-case smoke matrix completed 68/68 processes without panic or
fatal:

`/tmp/a32_lsu_structured_20260728/attempt-048/smoke_gem5`

Representative event comparisons:

| Case | Event result |
| --- | --- |
| EW8/VL16 load to barrier | exact |
| EW8/VL16 store to barrier | exact |
| EW16/VL256 load to consumer | exact |
| EW8/VL16, six independent loads | pass, uniform 0/1-cycle observer phase |
| EW8/VL16, six independent stores | pass, uniform 0/1-cycle observer phase |
| load capacity, N-2 and N+2 | pass within three-cycle cross-clock phase |
| store capacity, N-2 and N+2 | pass within three-cycle cross-clock phase |

Primary comparison evidence:

- `/tmp/a32_lsu_structured_20260728/attempt-043`
- `/tmp/a32_lsu_structured_20260728/attempt-046`

The remaining producer-to-store case is deliberately not hidden:

- RTL store response: +70 cycles;
- gem5 store response: +86 cycles.

The same no-dependency EW16/VL256 store has exact LSU phases. In the failing
case, RTL VADD becomes ready 43 cycles after store accept while gem5 becomes
ready after 59 cycles. The first remaining divergence is therefore the
upstream VADD/requester readiness boundary (+16), not STU memory timing. LSU
latency was not shortened to compensate for it.

An earlier N-2 store attempt without operand prewarm is superseded. Its first
4096-byte store began before the broadcast had populated all source rows, so
its +250-cycle response measured RAW readiness rather than capacity.

## Held-out PDSCHDag2 A47 regression

Run evidence:

`/tmp/a32_lsu_structured_20260728/attempt-049/pdsch_heldout`

L0:

- 22/22 RTL return payloads are byte-exact;
- 7,256 available RTL instruction dumps match element-by-element;
- all 8,103 gem5 instruction IDs 0..8102 occur exactly once.

Per-task VINS counts remain:

| Task | VINS |
| ---: | ---: |
| 0 | 1 |
| 1 | 6 |
| 2 | 841 |
| 3 | 289 |
| 4 | 193 |
| 5 | 40 |
| 6 | 44 |
| 7 | 0 |
| 8 | 6689 |

L1 LSU counts are exact against RTL:

| Task | RTL/gem5 loads | RTL/gem5 stores | RTL/gem5 max load outstanding | RTL/gem5 max store outstanding |
| ---: | ---: | ---: | ---: | ---: |
| 3 | 62 | 44 | 4 | 2 |
| 4 | 43 | 30 | 4 | 3 |
| 5 | 10 | 5 | 2 | 1 |
| 8 | 559 | 559 | 2 | 2 |
| Total | 674 | 638 | — | — |

Every accepted LSU operation has exactly one request, response, and
completion; final outstanding count is zero.

Task execution timing:

| Task | RTL ns | structured LSU gem5 ns | Delta ns | Error |
| ---: | ---: | ---: | ---: | ---: |
| 0 | 676287 | 677824 | +1537 | +0.227% |
| 1 | 79131 | 78468 | -663 | -0.838% |
| 2 | 55227 | 56524 | +1297 | +2.348% |
| 3 | 471571 | 476598 | +5027 | +1.066% |
| 4 | 30175 | 31658 | +1483 | +4.915% |
| 5 | 11059 | 11394 | +335 | +3.029% |
| 6 | 3795 | 3858 | +63 | +1.660% |
| 7 | 1059 | 1222 | +163 | +15.392% |
| 8 | 330955 | 326936 | -4019 | -1.214% |

Full allocation-to-release span:

- RTL: 1,535,100 ns;
- structured LSU gem5: 1,539,108 ns;
- delta: +4,008 ns = +2,004 tile cycles = +0.2611%.

This is still a sub-1% held-out L3 result, but it is slower than R47's
+0.0581%. That regression is retained rather than hidden with a fixed LSU
latency. The next isolated timing target is the upstream VADD/requester
readiness boundary exposed by producer-to-store RAW.

## Requester/chaining continuation (attempts 053-085)

RTL observation-only tracing was extended with `+venus_trace_vfu_task=N`.
For the EW16/VL256 producer-to-store case, lane 0 shows:

- VADD fire at cycle 2765;
- CAU A and B requester command acceptance at cycle 2769;
- the two BitALU producer writeback spans at 2761..2779 and 2780..2799;
- CAU operand/writeback activity at 2786..2813;
- store accept at 2771, request +14, response +70, completion +77.

This localized the old +16 error above the STU: gem5 held the complete CAU
command until producer progress, whereas RTL accepts the command into the
requester and gates individual rows.

An initial gem5 requester-decoupling implementation made the directed store
event exact and passed all 68 directed processes. It was rejected by held-out
PDSCH:

- attempts 062 and 064 completed but changed four return payloads;
- the first differing instruction dump was task 3 `VDIV_1054`;
- attempts 066/069/071/073/075 modeled a requester-local hazard snapshot and
  credit, but exposed a deadlock at the VSSUB/VDIV boundary.

RTL source inspection established that its requester keeps a one-bit,
non-accumulating credit per producer. The gem5 lane currently lacks the
corresponding end-to-end queue/result handshake: under row overlap a CAU
producer can stop at 19/20 writeback rows while the paired SerDiv operand
queues wait on each other. Treating LSU operations as BitALU producers was
also incorrect; RTL explicitly excludes LSU and shuffle dependencies from
chaining.

No failed chaining branch was promoted. The default lane overlap path is
disabled, while the observation trace and experimental state representation
remain available for the next structural implementation. No dispatcher,
barrier, or LSU fixed-latency constant was reverted or retuned.

Final default binary:

`188de0052f5849f9d5330788f94abbc023eb43b3732bd5d3f34df71d77e4efb9`

Final default validation:

- `/tmp/a32_lsu_structured_20260728/attempt-084/pdsch_final_default`:
  22/22 returns and 8,103/8,103 VINS are byte-identical to accepted
  attempt 049; natural completion at tick 1,539,132,000.
- `/tmp/a32_lsu_structured_20260728/attempt-085/final_default_suite`:
  68/68 processes passed and all 382 VINS dumps are byte-identical to
  attempt 048.

The model is therefore functionally restored but not complete. The next
required implementation boundary is VFU result valid/ready holding, tagged
operand command/data pairing, and requester-local credit lifetime. Only after
that passes the directed matrix may PDSCH and an independent LDPC DAG be used
as held-out promotion gates.

## Tagged retirement and LSU-victim continuation (attempts 053-073)

The result-hold, tagged operand/data queues and hazard-class split were kept
enabled. No dispatcher/barrier fix or fixed LSU/VFU latency was changed.

The first task-3 deadlock was a generation-ownership bug. CAU instruction 1058
issued valid VRF reads, but passage responses were compared with the current
VFU/operand-queue head (1057) and discarded as stale. Passage responses now
compare with the requester-local `queueing_*_instr_pkt`. A younger CAU command
also cannot resize a shared result pipe while older tagged results remain in
flight.

The next held-out run completed but exposed a functional first divergence at
`VSTORE_1061`: output byte 49, logical EW8 element 48. The producer
`VSADD_1059` had byte-identical operands, results and lane writebacks. The
younger `VSADD_1066`, which writes the same `vd16..20` range, committed its
first beat on lanes 6..15 before the store response. This identified a
destination WAR-retirement lifetime error, not an LSU latency error.

The retirement implementation now has these explicit boundaries:

- the sequencer snapshots WAR/WAW victims from the active instruction ranges
  after running-ID allocation and carries the snapshot with the writer tag;
- lane-local requester state merges but never overwrites that snapshot;
- hazard broadcasts carry monotonically advancing per-ID retirement
  tombstones, so a skipped active generation and rapid ID reuse are safe;
- arithmetic victims use lane read/write progress;
- LSU victims never use lane-local progress (an LSU never traverses the lane
  requester); they release only after the LSU completion tombstone.

The promoted binary is:

`/tmp/a32_microtiming_20260729/attempt-069/promoted_candidate/gem5.debug`

SHA-256:

`4a61edd03246e52681ce450bfadd41051b1252b541a7c86de4657b2590222305`

Directed promotion gates:

- attempt 070: Task1 strict 6/6, no first divergence;
- attempt 071: 68/68 LSU RAW, independent throughput and capacity-cliff cases;
- attempt 072: six back-to-back loads and stores, zero-cycle-tolerance event
  comparison, both exact.

Final held-out:

`/tmp/a32_microtiming_20260729/attempt-073/pdsch_a47_heldout`

- natural completion at tick 1,524,632,000;
- 22/22 return payload files byte-identical to accepted A47;
- 8,103/8,103 VINS files byte-identical, IDs 0..8102 exactly once;
- 674 loads and 638 stores;
- exactly 1,312 accept, request, response and completion events;
- maximum outstanding: total 6, load 4, store 3; final outstanding zero.

This closes the structured LSU/VFU retirement interaction for the current A47
promotion gates. It is not yet proof that every workload and arbitration
corner is micro-timing complete; an independent LDPC/second-DAG held-out and
additional simultaneous queue-full/arbitration traces remain required for
that stronger claim.
