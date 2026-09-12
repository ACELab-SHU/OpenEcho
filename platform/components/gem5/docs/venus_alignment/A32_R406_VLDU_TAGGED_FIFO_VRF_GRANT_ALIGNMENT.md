# A32 R406 VLDu tagged FIFO and VRF-grant alignment

## Scope and conclusion

This checkpoint continues the structural alignment of the vector requester,
per-bank VRF arbitration, and vector LSU.  It keeps the accepted stable tagged
request-vector, registered requester visibility, live-intent arbitration, LSU
priority, and persistent per-bank RR model.  No task, PC, DAG, address, or
payload special case was added, and no LSU/VFU fixed latency was changed.

The main result is local rather than DAG-total fitting:

- nrPDCCH task17 sequence 107 VLOAD changes from 53 cycles to the RTL 35
  cycles;
- PDSCHDag2 task3 sequence 6 VLOAD is 134/134 cycles and sequence 7 VSHUFFLE
  is 237/237 cycles;
- the current full-DAG totals still contain independent positive and negative
  task errors, so this is not a claim that the complete model is aligned.

Candidate binary:

```text
build/RISCV/gem5.debug
SHA256 7dcd3f1d31f088ce18366c23337f7c2822ac07d679fede686d42059cb6bd61b7
```

The four required experimental controls were enabled in all timing runs:

```text
VENUS_GEM5_EXPERIMENTAL_VRF_RR=1
VENUS_GEM5_EXPERIMENTAL_REQUESTER_Q_VISIBILITY=1
VENUS_GEM5_EXPERIMENTAL_SHUFFLE_LIVE_INTENT=1
VENUS_GEM5_ENABLE_OPERAND_HAZARDS=1
```

Persistent evidence root:

```text
evidence/a32_r406_vldu_tagged_fifo_20260812
```

## RTL first-divergence evidence

The nrPDCCH task17 VLDu waveform shows that result eligibility is selected by
the registered `vinsn_queue_qq.issue_pnt`; it is not a fresh test of the
result row's own running-ID hazard.  The relevant capture is
`rtl_oracles/task17_vldu_signals.txt`.

The PDSCH task3 capture provides the capacity cliff which the previous model
did not represent.  For sequence 6, RTL:

1. accepts a VLDu slot with destination hazard mask `0x3`;
2. destructively updates that mask `0x3 -> 0x1 -> 0` as the two tagged
   producers receive their final bank grants;
3. fills the two-entry VLDu result FIFO and backpressures the AXI R stream;
4. grants two rows, drains them, then accepts/grants the remaining rows;
5. advances the issue pointer through the registered Q -> QQ boundary.

The complete per-edge capture is
`rtl_oracles/pdsch_task3_vldu_signals.txt`.  This evidence rules out fitting a
single larger or smaller VLOAD latency.

## Structural implementation

### Four tagged VLDu issue slots

`VenusSequencer` now models the four RTL VLDu issue slots and separate
accept/D/Q/QQ issue-pointer state.  Every slot captures:

- the consumer running ID;
- the complete destination-hazard bit mask;
- the producer instruction generation behind each running-ID bit.

The mask is intersected destructively with completion state on each tile
edge.  The generation tag prevents a recycled five-entry running ID from
resurrecting an already-cleared dependency.

### Two-entry result FIFO and beat backpressure

The vector-load response path now consumes individual 64-byte AXI R beats,
assembles 128-byte VRF rows, and enqueues rows into a depth-two result FIFO.
When the FIFO is full, every R beat is held.  A beat which straddles a row
boundary retains its remaining bytes for the following tile edge.  The load
does not complete until the final row has passed result enqueue, delayed
issue-pointer hazard gating, per-bank grant, and the existing final response
boundary.

### Live per-bank priority and requester-private completion

LSU priority is reserved only on the edge on which a live VLDu result row is
actually granted; the previous AR-time forecast is gone.  Shuffle reports its
tagged producer-complete boundary when every PE's final destination-bank
grant has occurred.  This early observation updates only the VLDu requester's
private generation scoreboard.  Normal lane/shuffle response, VINS dump,
global hazard broadcast, retirement, and running-ID release remain on their
existing later edges.

This separation is necessary: publishing the shuffle grant into the shared
retirement table changed unrelated completion/dump order in CCH.  The private
scoreboard preserves the RTL VLDu dependency edge without changing global
architectural retirement.

Relevant implementation points:

- `src/venus/VenusSequencer.hh`: `PendingLsuInstr`, `LduIssueSlot`,
  `LduResultRow`, Q/QQ pointers, and requester-private generations;
- `src/venus/VenusSequencer.cc`: `advanceLduRegisters`, beat-wise response,
  result visibility/grant, and tagged shuffle completion;
- `src/venus/VenusShufflePipline.cc`: final per-bank producer-grant
  detection;
- `src/venus/venus_shuffle_producer_completion.hh`: typed completion
  sideband.

## Focused instruction results

One tile cycle is 2 ns.  Durations are fire-to-recycle boundaries.

| Workload/task | Sequence | Operation | RTL cycles | gem5 cycles | Delta |
| --- | ---: | --- | ---: | ---: | ---: |
| nrPDCCH task17 | 107 | VLOAD | 35 | 35 | 0 |
| PDSCH task3 | 3 | VLOAD | 47 | 46 | -1 |
| PDSCH task3 | 4 | VSHUFFLE | 140 | 140 | 0 |
| PDSCH task3 | 6 | VLOAD | 134 | 134 | 0 |
| PDSCH task3 | 7 | VSHUFFLE | 237 | 237 | 0 |
| PDSCH task3 | 8 | VLOAD | 40 | 41 | +1 |
| PDSCH task3 | 9 | VLOAD | 42 | 43 | +1 |

The machine-readable summary is
`focused/instruction_duration_vs_rtl.json`; the corresponding sequencer
monitors are stored beside it.  The task17 sequence-107 control was 53 cycles,
so the 18-cycle correction comes from dependency/grant structure, not a
fixed-latency change.

## Directed and functional regression

- LSU RAW/throughput/capacity matrix: 68/68 cases pass.
- LSU suite VINS: 382/382 files are byte-identical to the accepted control.
- nrPDCCH: 10,253 instructions match the accepted control by task-local
  opcode/suffix order and every emitted element.
- PDSCHDag2: 8,103 instructions match by the same strict task-local rule.
- `git diff --check` is clean.

CCH has 14 globally numbered dump filenames on each side whose IDs differ,
because independent vector instructions complete in a different global
order.  This is not a data mismatch: the strict task-local comparator reports
10,253 instructions and zero mismatches.  The report deliberately does not
claim filename/global-running-ID identity.

Evidence:

- `lsu_suite/suite_results.json`;
- `cch_full/vins_task_ordinals_vs_control.json`;
- `sch_full/vins_task_ordinals_vs_control.json`.

## Current CCH per-task timing

The comparison boundary is tile start to task epilogue versus RTL start
execute to execute complete; return DMA/tile release is excluded.

| Task | RTL ns | gem5 ns | Delta ns | Error |
| ---: | ---: | ---: | ---: | ---: |
| 0 | 547 | 540 | -7 | -1.280% |
| 1 | 10,691 | 10,928 | +237 | +2.217% |
| 2 | 547 | 540 | -7 | -1.280% |
| 3 | 69,631 | 69,724 | +93 | +0.134% |
| 4 | 4,019 | 4,014 | -5 | -0.124% |
| 5 | 895 | 886 | -9 | -1.006% |
| 6 | 531 | 526 | -5 | -0.942% |
| 7 | 895 | 886 | -9 | -1.006% |
| 8 | 531 | 526 | -5 | -0.942% |
| 9 | 158,799 | 158,946 | +147 | +0.093% |
| 10 | 295 | 288 | -7 | -2.373% |
| 11 | 375 | 370 | -5 | -1.333% |
| 12 | 375 | 370 | -5 | -1.333% |
| 13 | 563 | 612 | +49 | +8.703% |
| 14 | 202,779 | 203,810 | +1,031 | +0.508% |
| 15 | 6,443 | 6,408 | -35 | -0.543% |
| 16 | 1,343 | 1,362 | +19 | +1.415% |
| 17 | 33,167 | 33,182 | +15 | +0.045% |
| 18 | 123,235 | 126,700 | +3,465 | +2.812% |
| 19 | 2,439 | 2,466 | +27 | +1.107% |
| 20 | 398,523 | 388,550 | -9,973 | -2.502% |
| 21 | 9,303 | 9,434 | +131 | +1.408% |
| 22 | 100,055 | 100,058 | +3 | +0.003% |
| 23 | 33,951 | 33,944 | -7 | -0.021% |

Task17 improves from +131 ns to +15 ns.  Task3 improves from +191 ns to
+93 ns.  Task20 remains 9,973 ns too fast and task18 remains 3,465 ns too
slow; those are now higher-value absolute gaps than the 49 ns task13 error,
despite task13 having the largest percentage.

The complete data is `cch_full/task_execution_timing_vs_rtl.json`.

## Current SCH per-task timing

| Task | RTL ns | gem5 ns | Delta ns | Error |
| ---: | ---: | ---: | ---: | ---: |
| 0 | 676,287 | 676,304 | +17 | +0.003% |
| 1 | 79,131 | 79,144 | +13 | +0.016% |
| 2 | 55,227 | 57,922 | +2,695 | +4.880% |
| 3 | 471,571 | 478,492 | +6,921 | +1.468% |
| 4 | 30,175 | 31,538 | +1,363 | +4.517% |
| 5 | 11,059 | 11,184 | +125 | +1.130% |
| 6 | 3,795 | 3,784 | -11 | -0.290% |
| 7 | 1,059 | 1,056 | -3 | -0.283% |
| 8 | 330,955 | 330,228 | -727 | -0.220% |

The exact sequence-6/7 correction does not materially change task3's total:
later instructions still contribute +6,921 ns.  The remaining SCH priorities
are therefore task3 first-divergence after sequence 7, then task2 and task4;
task8 is a smaller but opposite-sign gap.  Complete data is
`sch_full/task_execution_timing_vs_rtl.json`.

## DAG totals are diagnostic only

- CCH functional DAG span: 1,037,460 ns versus RTL 1,043,224 ns, or
  -5,764 ns (-0.552%).
- SCH functional DAG span: 1,542,408 ns versus RTL 1,535,100 ns, or
  +7,308 ns (+0.476%).

These numbers are not pass criteria.  CCH alone still contains task20
-9,973 ns and task18 +3,465 ns, while SCH contains task3 +6,921 ns and task8
-727 ns.  The signs prove that task/DAG cancellation remains.

## Next first-divergence work

1. CCH task20: compare the first still-divergent live requester vector,
   per-bank winner, LSU-priority mask, persistent RR state, grant capture, and
   response visibility.  The existing stable-vector structure must be
   corrected at its first edge rather than retuned globally.
2. CCH task18: split its +3,465 ns into scalar-to-vector admission, operand
   grant, VFU result enqueue, and retirement before changing any latency.
3. SCH task3: retain the now-exact sequence 6/7 prefix and find the first
   later sequence whose fire or recycle edge differs.  Then handle task2 and
   task4 as independent held-out gaps.

The simulator is materially closer at the observed VLDu/requester boundary,
but vector requester, bank arbitration, and vector LSU are not globally or
formally complete.
