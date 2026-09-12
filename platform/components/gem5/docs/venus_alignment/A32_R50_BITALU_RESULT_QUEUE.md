# A32 R50 explicit BitALU tagged result queue

## Scope and preserved baseline

R50 continues from the promoted R49 binary:

`/tmp/a32_r49_task17_20260729/attempt-031/promoted/gem5.debug`

SHA-256:

`452e662e759a4585159ed7d7d89c901ceba42127805725d78e335ab0171273b6`

The A47 dispatcher/barrier fixes and the R48/R49 structured-LSU,
DMT-return, tagged operand/VRF response, hazard-class retirement, and
result-generation ownership fixes remain enabled. No DAG-wide, VFU, or LSU
fixed latency was changed.

The promoted R50 candidate is:

`/tmp/a32_r50_hazard_class_20260730/attempt-011/promoted/gem5.debug`

SHA-256:

`5aab3d1accd8dc388b7fece1c0a713f47539aade920deaf72578d9751128f547`

## RTL contract and structural implementation

`venus_bitalu_wrapper.sv` has a four-entry vector-instruction queue and a
separate two-entry result queue. Each result entry carries its instruction
ID, payload, address, byte enable, and mask state. The queue is registered:
an entry produced on one edge is visible to the VRF request output from the
following edge. A result present at the read pointer is held until its VRF
or mask grant arrives.

R49 preserved ownership by preventing `datapipe::setPipeLength()` from
switching to a younger generation while an older result was still present.
R50 replaces the direct BitALU pipeline-to-writeback boundary with an
explicit depth-two FIFO:

- each entry owns copied instruction/running tags, data, mask, and separate
  data/mask handshake state;
- the arithmetic pipe may enqueue while the previous queue head dequeues;
- a full queue holds the tagged arithmetic output instead of overwriting or
  dropping it;
- the active pipe generation may change only after its tagged output moved
  into the independent queue;
- duplicate next-cycle service requests coalesce onto the already scheduled
  queue event.

There is no task ID, DAG name, PC, address, payload value, or global latency
branch.

## First divergence and directed queue trace

The first task-20 timing change is instruction 1415, the first `VBRDCST`
after its initial load:

| Boundary, normalized to task start | RTL | gem5 R49 | gem5 R50 |
| --- | ---: | ---: | ---: |
| `VBRDCST` fire | 458 ns | 460 ns | 460 ns |
| `VBRDCST` recycle | 500 ns | 498 ns | 500 ns |

The registered result-queue boundary therefore corrects the first recycle
edge by one 500 MHz tile cycle. This is the first measured divergence from
R49 and matches the RTL edge; it is not a fitted task delay.

Trace:

`/tmp/a32_r50_hazard_class_20260730/attempt-010/nrpdcch_task20_result_queue_trace`

In the observed task-20 window:

- 164 tagged entries enqueue and 164 dequeue;
- every entry preserves instruction 1415/running-ID 1 ownership;
- after filling, dequeue and enqueue occur on the same 2 ns edges, proving
  one-result-per-cycle steady-state throughput;
- observed occupancy is at most one, so this trace does not claim to have
  exercised the depth-two full cliff.

The initial implementation also exposed two duplicate event-scheduling
panics. They were not timing failures: the queue-service event and the
operand-consumer event requested the same next edge. Folding those duplicate
requests preserves the existing edge and state transition.

## Validation

### nrPDCCH full

Run:

`/tmp/a32_r50_hazard_class_20260730/attempt-007/nrpdcch_bitalu_result_queue_v2`

Results:

- natural completion at tick `1,045,668,000`;
- 24/24 tasks released;
- 53/53 RTL-captured return ports pass;
- 10,253/10,253 VINS files are byte-identical to R49;
- LSU accept/request/response/complete are each `797`;
- all 797 LSU lifecycles close.

The allocation-to-last-release interval is `1,045,616 ns`, versus RTL
`1,043,224 ns`: delta `+2,392 ns` (`+0.229%`). The R49 whole-DAG delta was
`-112 ns`; the sign change is retained because per-task evidence, rather
than cancellation in the full-DAG total, is the promotion criterion.

The main per-task changes from R49 are:

| Task | RTL duration | R49 | R50 | R50 minus RTL |
| ---: | ---: | ---: | ---: | ---: |
| 17 | 33,167 ns | 49,176 ns | 49,412 ns | +16,245 ns |
| 20 | 398,523 ns | 386,632 ns | 389,136 ns | -9,387 ns |

Task 20 moves `2,504 ns` toward RTL. Task 17 moves `236 ns` away and remains
the largest positive local error. These errors are not compensated with a
global latency.

### Structured LSU matrix

Run:

`/tmp/a32_r50_hazard_class_20260730/attempt-008/lsu_suite_bitalu_result_queue`

Results:

- 68/68 processes pass;
- 382/382 VINS outputs are byte-identical to R49;
- LSU accept/request/response/complete are each `244`;
- all 244 lifecycles close;
- RAW-chain, independent-stream throughput, and EW8/EW16 load/store
  capacity-cliff cases remain covered.

### PDSCHDag2 A47 held-out

Run:

`/tmp/a32_r50_hazard_class_20260730/attempt-009/pdsch_bitalu_result_queue_heldout`

Results:

- natural completion at tick `1,525,436,000`, 24 ns earlier than R49;
- 9/9 tasks released;
- 22/22 RTL-captured return ports pass;
- 8,103/8,103 VINS outputs are byte-identical to R49;
- LSU accept/request/response/complete are each `1,312`;
- all 1,312 lifecycles close.

PDSCH task 2 grows by 580 ns, task 3 shrinks by 32 ns, and task 6 grows by
8 ns. The critical-path completion moves only 24 ns because the task
dependencies overlap.

## Promotion boundary and next work

R50 is promoted as a more faithful BitALU result-boundary model. It must not
be described as complete microtiming:

1. the observed full DAG did not reach BitALU result occupancy two, so a
   directed VFU grant-backpressure capacity test is still required;
2. CAU and SerDiv still need explicit fixed-depth tagged instruction/result
   queues rather than their current hold representation;
3. nrPDCCH task 17 remains `+16,245 ns`, and task 20 remains `-9,387 ns`;
4. future work must compare queue enqueue/dequeue/full, VRF grant, and
   instruction retirement edges before changing any timing constant.

