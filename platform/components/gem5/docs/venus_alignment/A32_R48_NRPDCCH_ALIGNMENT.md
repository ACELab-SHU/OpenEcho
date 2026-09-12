# A32 R48 nrPDCCH generic alignment

## Scope and frozen baseline

This continuation starts from the accepted A32 R47 binary:

`/tmp/a32_nrpdcch_alignment_20260729/attempt-001/baseline/gem5.debug`

SHA-256:

`4a61edd03246e52681ce450bfadd41051b1252b541a7c86de4657b2590222305`

The A47 dispatcher/barrier, structured LSU, VFU result hold, tagged operand
queues, tagged VRF responses, and RAW/WAR/WAW admission/retirement fixes are
preserved. No DAG-wide, VFU, or LSU fixed latency was changed.

The new workload is `nrPDCCH_tv9_a28_full`, 24 tasks on four runtime tiles.
The RTL reference is:

`/home/shenyihao/Project/Venus_3/venus_soc/sim/build_gc0802_overall_nrPDCCH_tv9_nrPDCCH_full_structzero_a28_20260727`

## First divergences

### Immutable shared-L2 inputs

The old A47 run stopped at task 14 and task 1 already had a functional
divergence: its GATHER index was correct but the data read returned zero.
The first bad physical byte was not a VFU computation. The generated
`shared_l2.bin` ended at `0x4a080`, while valid immutable globals referenced
by the manifest lived at and beyond that boundary.

After recovering the type-1/type-2 inputs, task 1 became byte-exact. The next
functional first divergence moved to task 3 local instruction 367:

- instructions 0..366: RTL/gem5 byte-exact;
- instruction 367: EW16/VL576 VLOAD;
- RTL first elements: `48, 49, 50, ...`;
- gem5 first element: zero;
- architectural source: declared type-5 immutable global at `0x48c40`.

The RTL LSU capture has a conflict-free 1,152-byte observation at that range,
starting `48,0,49,0,...`; the old dense image contained zeroes there.

Input materialization is now manifest/range driven:

- `tools/venus_l1_dag.py` materializes declared type-1/type-2 globals from
  byte-complete RTL DMA evidence and retains declared immutable-global RTL
  LSU bytes, including sparse bytes beyond the canonical `*_bin`;
- `tools/relocate_venus_dag_manifest.py` reconstructs a run-local dense image
  from every manifest `initial_inputs` entry whose source space is
  `shared_l2`;
- conflicting aliases, canonical-byte overwrites, missing evidence, and
  addresses outside the 32 MiB model fail explicitly.

This is validation input construction. It does not use task IDs, PCs,
payload values, or addresses to change gem5 execution behavior.

### LSU same-edge arbitration

With correct inputs, task 14 still reproduced the old stop. Event tracing
showed instructions 552, 553, and 554 repeatedly pushed to one
`lsuChannelAvailableTick`. Stable reinsertion allowed younger 554 to bypass
552/553. Instruction 554 then waited for the older destination-hazard
victims, while its retries kept advancing the shared channel and prevented
those victims from issuing: a circular wait.

`VenusSequencer::queuePendingLsu()` now applies structural same-edge rules:

1. response/complete edges precede requests on the same tick;
2. requests on the same tick arbitrate by oldest `vns_instr_id`;
3. all other insertion remains stable.

There is no task, PC, DAG-name, address, or payload branch. After the fix:

```
552 request 244510, response 244578, complete 244596
553 request 244578, response 244698, complete 244712
554 request 244698, response 244766, complete 244784
```

### Dynamic DMT return length

The first full run after LSU repair reached task 20 and exposed a separate
runtime-DAG contract bug. The producer returned 4,096 bytes while the only
consumer copied a fixed 128-byte prefix. The timing path incorrectly treated
`consumerBytes` as producer capacity and aborted.

`VenusDagScheduler` now uses the same contract as its non-timing
`publishOutput()` path:

- `consumerBytes` is the fixed consumer prefix encoded in type-0/type-4
  transfers;
- the producer's dynamic return DMA may be larger;
- the DMT snapshot preserves `max(consumerBytes, producerBytes)`.

## Directed validation

Final binary:

`/home/shenyihao/Project/Venus_3/gem5-freertos/build/RISCV/gem5.debug`

SHA-256:

`1b55e6a1d890f41ae3154a68098e80873e3170a2f4fb78c730ce6797d583afad`

The final 68-case LSU matrix is:

`/tmp/a32_nrpdcch_alignment_20260729/attempt-020/lsu_suite`

Results:

- 68/68 process and event comparisons pass;
- all JSONL traces parse;
- every accept has exactly one request, response, and completion;
- RAW-chain, independent-stream throughput, and EW8/EW16 load/store
  capacity N-2..N+2 remain covered;
- LDU accept-to-complete outstanding plateaus at 5 in this sequencer+held
  addrgen model;
- STU accept-to-complete outstanding plateaus at 4;
- request-to-response channel outstanding is 1;
- every final outstanding count is zero.

The shared four-tile logger is now a single process-wide stream instead of
four independently truncating streams. Events carry a sequencer name and
remain globally tick ordered. A task0..3 probe recorded 85 complete
lifecycles with 0 malformed JSON records:

`/tmp/a32_nrpdcch_alignment_20260729/attempt-021/task3_logger`

## nrPDCCH full regression

Final run:

`/tmp/a32_nrpdcch_alignment_20260729/attempt-023/nrpdcch_final`

Results:

- natural completion at tick `1,055,312,000`;
- 24/24 tasks complete;
- 53/53 captured RTL return ports pass the known-byte comparator;
- task 3 has 370/370 byte-exact VINS in the directed first-divergence run;
- 797 loads/stores each have one accept, request, response, and completion;
- LSU JSONL is valid and globally tick monotonic;
- peak accept-to-complete outstanding is total/load/store `8/5/4` across
  four tiles;
- final total/load/store outstanding is `0/0/0`.

Task 3 VINS evidence:

`/tmp/a32_nrpdcch_alignment_20260729/attempt-018/task3_vins`

Return comparison:

`/tmp/a32_nrpdcch_alignment_20260729/attempt-023/nrpdcch_final/dma_compare.txt`

Lifecycle comparison:

`/tmp/a32_nrpdcch_alignment_20260729/attempt-023/nrpdcch_final/lifecycle_compare.txt`

Functional alignment is complete for the captured bytes, but microtiming is
not yet exact. Normalized allocation-to-last-release is:

- RTL: `1,043,224 ns`;
- gem5: `1,055,260 ns`;
- delta: `+12,036 ns` (`+1.154%`).

Selected task execution spans show where future timing work belongs:

| Task | RTL ns | gem5 ns | Delta | Error |
| ---: | ---: | ---: | ---: | ---: |
| 1 | 10,691 | 10,796 | +105 | +0.982% |
| 3 | 69,631 | 72,604 | +2,973 | +4.270% |
| 9 | 158,799 | 168,186 | +9,387 | +5.911% |
| 14 | 202,779 | 202,576 | -203 | -0.100% |
| 17 | 33,167 | 49,390 | +16,223 | +48.913% |
| 20 | 398,523 | 398,304 | -219 | -0.055% |
| 22 | 100,055 | 104,468 | +4,413 | +4.411% |
| 23 | 33,951 | 38,362 | +4,411 | +12.992% |

The lifecycle comparator also reports the first normalized event difference
at task 0 start (`RTL +601 ns`, gem5 `+584 ns`) and a task 6/7 tile-choice
swap. These remain explicit evidence; they were not hidden by latency tuning.

## PDSCHDag2 A47 held-out

Final held-out run:

`/tmp/a32_nrpdcch_alignment_20260729/attempt-022/pdsch_heldout`

Results:

- natural completion at tick `1,526,008,000`;
- 22/22 return payloads byte-exact against the accepted A47 RTL-exact
  reference;
- 8,103/8,103 VINS files byte-exact;
- LSU accept/request/response/complete each `1,312`;
- 1,312/1,312 strict per-instruction lifecycles;
- peak total/load/store outstanding `7/4/3`;
- final outstanding zero.

The natural completion is `+1,376,000` ticks (`+1,376 ns`, about `+0.090%`)
relative to the accepted A47 binary. This timing movement follows the generic
oldest-first LSU arbitration and is retained rather than compensated with a
fixed latency.

## Promotion boundary

The generic fixes are promoted for this checkpoint because they pass the
directed LSU matrix, the new nrPDCCH full regression, and the previously held
out PDSCHDag2 regression without workload-specific execution branches.

This does not justify claiming complete RTL microtiming. The next
first-divergence target is nrPDCCH lifecycle/VINS timing around task 17, then
the task 6/7 allocator tie and task 22/23 tail. The correct continuation is
requester/result/retirement and allocator event comparison, not global
latency fitting.
