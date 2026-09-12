# A32 R52: CAU/SerDiv result-edge alignment

Date: 2026-07-30

## Scope and frozen baseline

R52 continues from the promoted R51 binary:

`/tmp/a32_r51_vfu_result_queue_20260730/attempt-010/promoted/gem5.debug`

R51 SHA256:

`653f7e1b77016c288a364995abdc0cdbb08a6135bb70937adebe6c660786be42`

The A47 dispatcher/barrier fixes and the R48--R51 structured LSU, DMT,
tagged operand/VRF response, hazard-class retirement, result-generation
ownership, and fixed-depth VFU result queues are retained. No global DAG,
LSU, or VFU fixed latency was changed.

## Edge observability

Gem5 `LaneVFU` now records CAU/SerDiv:

- instruction admission;
- tagged result enqueue and dequeue;
- every VRF grant attempt, including instruction/running ID, passage,
  offset, size, acceptance, and rejection reason;
- tagged retirement.

`master_busy` means that the single CAU or SerDiv result master already
used its grant on that lane edge. `bank_backpressure` means that the request
reached the physical-bank path and was rejected. These are deliberately
separate because only the latter is evidence of a competing bank requester.

The normalized parser is:

`tools/parse_vfu_edge_trace.py`

RTL VCD extraction and normalization are implemented by:

`tools/parse_rtl_vfu_vcd.py`

The RTL parser samples pre-edge capture and post-edge combinational state,
decodes the four-bank by twelve-master requester vectors, and reports:

- enqueue handshake;
- post-edge grant or blocked request;
- pre-edge grant capture;
- queue occupancy;
- completion;
- selected bank, LSU high-priority state, RR requesters and RR winner.

## RTL collection

The debug-access RTL executable is:

`/home/shenyihao/Project/Venus_3/venus_soc/sim/build_gc0802_overall_nrPDCCH_tv9_r52debug/simv`

Task 17:

`/tmp/a32_r52_vfu_edge_alignment_20260730/attempt-003/rtl_task17_cau_edges.vcd`

Task 20:

`/tmp/a32_r52_vfu_edge_alignment_20260730/attempt-003/rtl_task20_cau_edges_correct.vcd`

The fresh full RTL run regenerated 10,078 VINS dumps byte-exactly against
the frozen nrPDCCH oracle before these short waveform windows were used.

Task 17 contains 65 CAU instructions and no SerDiv instruction. Task 20
contains CAU instructions and no SerDiv instruction. Therefore these tasks
can validate CAU edge timing but cannot be cited as SerDiv task coverage;
SerDiv capacity remains covered by the independent backpressure microbench.

## First-divergence results

### Task 17 first CAU

RTL issue is at 568,120.999 ns. The four result grants are at approximately
568,164.994, 568,170.994, 568,172.996, and 568,174.997 ns. Every enqueue
becomes a post-edge grant at the same VCD timestamp. Wrapper completion is
at the final grant and global release is at 568,179 ns.

Gem5 global fire is at 424,416 ns and Lane admission is at 424,426 ns.
Enqueue/dequeue/grant pairs occur at 424,472, 424,478, 424,480, and
424,484 ns; retirement is at 424,486 ns. The result edge shape is the same.
However, Lane admission is not equivalent to RTL global issue. Relative to
the architectural fire/issue boundary, gem5 is already twelve nanoseconds
slow at the first result:

| Boundary | RTL issue-relative ns | gem5 fire-relative ns | Divergence |
| --- | ---: | ---: | ---: |
| first enqueue/grant | 44 | 56 | +12 |
| last grant / wrapper done | 54 | 68 | +14 |
| global release / lane retirement | 58 | 70 | +12 |

The Lane-local admit-relative values are 46, 58, and 60 ns. They remain
useful for isolating the Lane implementation, but must not be reported as
the end-to-end architectural interval.

The three-microsecond RTL window contains 248 enqueues, 248 grants, zero
blocked CAU grant edges, 62 completions, and maximum occupancy one.

### Task 20 first CAU

RTL issue is at 703,035 ns. The first enqueue is at 703,043 ns, but CAU is
blocked on that edge by the CAU-B operand requester. The first grant is at
703,045 ns, the last grant and wrapper completion are at 703,087 ns, and
global release is at 703,091 ns.

That instruction has seven blocked result edges:

1. CAU-B operand read;
2. LSU high priority;
3. CAU-A operand read;
4. BitALU result;
5. LSU high priority;
6. BitALU result;
7. LSU high priority.

Gem5 global fire is at 560,064 ns and Lane admission is at 560,074 ns.
First enqueue/grant is at 560,078 ns, last grant at 560,108 ns, and
retirement at 560,110 ns. It observes no real bank rejection for this
instruction:

| Boundary | RTL issue-relative ns | gem5 fire-relative ns | Divergence |
| --- | ---: | ---: | ---: |
| first enqueue | 8 | 14 | +6 |
| first grant | 10 | 14 | +4 |
| last grant / wrapper done | 52 | 44 | -8 |
| global release / lane retirement | 56 | 46 | -10 |

The previously quoted 20 ns difference was Lane-admit-relative, not an
architectural issue/fire comparison. The correct end-to-end divergence is
10 ns fast. Seven missing two-nanosecond arbitration stalls remain directly
visible, but front-end fire-to-Lane-admit phase and first-result timing also
differ; these boundaries must be modeled separately rather than summed into
a single CAU latency.

Across the four-microsecond task-20 RTL window, CAU records 256 enqueues,
256 grants, 37 blocked edges, 30 completions, and maximum occupancy two.
Blocked winners are:

- 6 LSU high-priority edges;
- 11 CAU-A and 10 CAU-B operand-read edges;
- 2 BitALU-A and 4 BitALU-B operand-read edges;
- 4 BitALU-result edges.

This identifies the next first divergence: RTL has persistent per-bank
request vectors, LSU-over-low-level priority, and an independent RR pointer
for each bank. Gem5 still lets synchronous `NoncoherentXBar` call order
choose the first requester and does not preserve the RTL RR state.

The frozen call-order trace is:

`/tmp/a32_r52_vfu_edge_alignment_20260730/attempt-012/gem5_task20_bank_calls/m5out/bank_calls.trace`

At task 20's first result edge, lane 0 presents only the CAU-result write
(port 10); the concurrent CAU-B operand intent visible in RTL is absent.
Therefore the first missing structure is persistent tagged requester intent
and operand/result overlap, not merely a different RR winner.

## Rejected deferred-RR prototype

A deferred per-bank XBar arbiter was tested but not promoted.

- Attempt 015 used a single same-tick arbitration event. It completed
  nrPDCCH at tick 1,222,984,000, but task 20 output changed, first observed
  in `VADD_1522.txt`. Requests registered after the event incorrectly waited
  another edge.
- Attempt 016 corrected that event-order issue with per-bank
  `lastGrantTick`. It completed at tick 1,163,280,000, with task 17 at
  57,132 ns and task 20 at 497,000 ns, but the same task-20 functional
  mismatch remained.

The prototype arbitrated transient scalar-latch calls without first
representing stable tagged request ownership. Retry could therefore
reconstruct or reorder an operand request rather than preserve the original
intent. Its code is retained only behind
`VENUS_GEM5_EXPERIMENTAL_VRF_RR=1`; the default path remains R52.

## Structural implementation

CAU and SerDiv still use independent depth-two tagged result queues. R52
changes their queue output phase:

- the arithmetic/iterative output is captured into the registered queue on
  the lane edge;
- the newly registered head immediately participates in the post-edge
  combinational VRF grant phase at the same gem5 tick;
- each CAU or SerDiv data-result master can consume at most one grant per
  lane edge;
- no task, PC, opcode sequence, address, or payload-value condition is used.

This matches `venus_cau_wrapper.sv`, `venus_serdiv_wrapper.sv`, and
`venus_operand_requester.sv`: `result_queue_q` is updated in `always_ff`,
while request and bank grant are combinational functions of the new queue
state. The old R51 SerDiv `enqueueTick` prohibition incorrectly inserted a
whole extra lane cycle and was removed.

## Directed validation

The result-queue backpressure sweep is:

`/tmp/a32_r52_vfu_edge_alignment_20260730/attempt-006/post_edge_capacity/capacity_results.json`

All 1/4/16/32/64/128 ns request-gap runs exit normally and preserve output
hashes. Enqueue and dequeue counts close for every VFU. Depth-two/full is
observed for BitALU from 16 ns, CAU from 4 ns, and SerDiv from 64 ns. At the
64 ns point SerDiv records occupancy two 64 times and full 1,440 times.

The structured LSU suite is:

`/tmp/a32_r52_vfu_edge_alignment_20260730/attempt-010/lsu_suite_post_edge`

- 68/68 processes pass;
- 382/382 VINS files are byte-identical to R51;
- RAW-chain, independent-stream throughput, and load/store capacity-cliff
  cases remain covered.

## Full-DAG validation

### nrPDCCH full

`/tmp/a32_r52_vfu_edge_alignment_20260730/attempt-008/gem5_task20_post_edge`

- natural completion at tick `1,046,184,000`;
- 24/24 tasks release;
- 53/53 return payloads are byte-identical to R51;
- 10,253 VINS contents are byte-identical to R51;
- the known concurrent task-3/task-9 allocation swaps global IDs 422/423,
  while the cross-mapped contents remain byte-identical.

R51 completed at tick 1,050,444,000, so post-edge service removes 4,260 ns.
Per-task timing remains explicitly unclosed:

| Task | RTL ns | R52 gem5 ns | Delta ns | Error |
| ---: | ---: | ---: | ---: | ---: |
| 17 | 33,167 | 49,440 | +16,273 | +49.06% |
| 20 | 398,523 | 389,712 | -8,811 | -2.21% |

### PDSCHDag2 A47 held-out

`/tmp/a32_r52_vfu_edge_alignment_20260730/attempt-009/pdsch_post_edge_heldout`

- natural completion at tick `1,526,032,000`;
- 9/9 tasks release;
- 22/22 return payloads are byte-identical to R51;
- 8,103/8,103 VINS files are byte-identical to R51.

R51 completed at tick 1,526,188,000; R52 removes 156 ns without changing
functional results.

## Promotion boundary

Current binary:

`/home/shenyihao/Project/Venus_3/gem5-freertos/build/RISCV/gem5.debug`

Immutable promoted copy:

`/tmp/a32_r52_vfu_edge_alignment_20260730/attempt-011/promoted/gem5.debug`

SHA256:

`f20550a342007504984a6b61166d48b6bf29038f783c0d9ffac40b9dbc1b62f3`

R52 closes the registered-capture versus post-edge-combinational result
phase for CAU and SerDiv. It does not establish complete microtiming. The
next implementation must first represent each operand/result requester as
a stable tagged intent with explicit ownership and simultaneous visibility.
Only then can a generic four-bank arbiter add LSU high priority and
persistent per-bank RR state without changing payload ownership. It must be
validated against the task-20 blocked-winner sequence above, not by fitting
task or whole-DAG latency.
