# A32 R55 LSU completion and addrgen-ack alignment

## Scope

This continuation preserves the A47 dispatcher/barrier repair and all later
structured LSU, DMT, tagged operand/VRF response, hazard-class retirement,
and fixed-depth BitALU/CAU/SerDiv result-queue work.  It does not change a
DAG-wide, VFU, LSU memory, or commit fixed latency, and it contains no branch
on task ID, PC, DAG name, address, or payload.

The candidate binary is:

```
build/RISCV/gem5.debug
SHA256 bca92022835b06d4f9b273a234bc0de4c442e9afff11dcf8cd1c1d55c88bf058
```

Evidence root:

```
evidence/a32_r55_lsu_completion_20260731
```

## RTL-backed structural change

RTL observation around the first task17 VLOAD showed three distinct edges:

- LDU response/writeback becomes valid;
- the requester active hazard clears two nanoseconds later;
- the requester pending/stall and PE running state clear another two
  nanoseconds later.

RTL source inspection also showed that the sequencer remains in downstream
receive until `addrgen_ack_i`.  The addrgen captures a PE request in IDLE and
asserts the acknowledgement from its registered ADDRGEN state.  The old gem5
sequencer instead returned upstream as soon as it scheduled the LSU work.

R55 therefore adds two orthogonal structures:

1. LSU completion is forwarded to lanes through a tagged sideband carrying
   `running_id` and `vns_instr_id`.  A lane updates only its local retirement
   visibility after validating the tag.  It does not clear global hazard
   state a second time.
2. Successful LSU schedule records an addrgen-ack-visible edge two tile
   cycles later.  The sequencer remains in `DownstreamReceive` until that
   edge.  This models the registered request/ack handshake without modifying
   LSU memory response or completion latency.

Relevant implementation:

- `src/venus/venus_instr_pkt.hh:215-217`
- `src/venus/VenusLane.cc:329-349`
- `src/venus/VenusSequencer.cc:1969-1975`
- `src/venus/VenusSequencer.cc:2028-2040`
- `src/venus/VenusSequencer.cc:2415-2430`

## Task17 first divergence

Tagged completion alone did not fix the first issue boundary.  An issue trace
showed VLOAD scheduling at cycle 280 followed by the next instruction handle
at cycle 286, proving that the missing structure was upstream addrgen ack,
not a completion-delay constant.

After adding the ack boundary, the first seven instruction durations are:

| Sequence | Operation | RTL cycles | gem5 cycles | Delta |
| ---: | --- | ---: | ---: | ---: |
| 0 | VBRDCST | 12 | 12 | 0 |
| 1 | VLOAD | 34 | 34 | 0 |
| 2 | VBRDCST | 12 | 12 | 0 |
| 3 | VSEQ | 29 | 29 | 0 |
| 4 | VMUL | 32 | 32 | 0 |
| 5 | VSADD | 34 | 34 | 0 |
| 6 | VSEQ | 34 | 37 | +3 |

The first duration divergence moved from sequence 3 to sequence 6.  The
remaining first fire/recycle divergence is still the VLOAD at sequence 1:
gem5 cycle 7/41 versus RTL cycle 8/42.  This is a one-cycle cold
dispatcher/scalar-arrival phase and must remain separate from the VSEQ
requester/result/grant divergence.

Task17 total time is now 33,380 ns versus RTL 33,167 ns: +213 ns, +0.642%.
The machine-readable comparison is
`attempt-003_lsu_addrgen_ack_task17/task17_timing.json`.

## Directed regressions

### LSU matrix

`attempt-004_lsu_suite/suite_results.json` reports 68/68 passing cases.  The
382 VINS outputs are byte-identical to the pre-change structured-LSU control.
The matrix covers RAW chains, independent streams, and load/store outstanding
capacity around the cliff for EW8/EW16 and VL16/VL256.

### Result-queue capacity under VRF backpressure

`attempt-007_vfu_capacity/capacity_results.json` passes all 1/4/16/32/64/128
ns grant-gap points with output hashes identical to the 1 ns control.  Every
queue closes enqueue/dequeue counts and reaches the intended depth-2/full
condition:

- BitALU and CAU become full from 4 ns onward;
- SerDiv becomes full at 64 ns and 128 ns.

This gate proves that the result queues hold tags and payloads under physical
VRF grant backpressure; it is not a replacement for real RTL arbitration
edge comparison.

## nrPDCCH full regression

Run: `attempt-005_nrpdcch_full`.

- natural completion at tick 1,045,072,000;
- 24/24 tasks and 53/53 returns complete;
- 10,253 VINS outputs are byte-exact to the accepted pre-change control;
- full allocation-to-last-release span is 1,045,020 ns versus RTL
  1,043,224 ns: +1,796 ns, +0.172%.

The small total error is not a per-task pass.  Task9 and task20 alone differ
by +11,661 ns and -11,747 ns, so their cancellation must not be used as an
alignment claim.

| Task | RTL ns | gem5 ns | Delta ns | Error |
| ---: | ---: | ---: | ---: | ---: |
| 0 | 547 | 1,040 | +493 | +90.128% |
| 1 | 10,691 | 11,420 | +729 | +6.819% |
| 2 | 547 | 1,032 | +485 | +88.665% |
| 3 | 69,631 | 68,780 | -851 | -1.222% |
| 4 | 4,019 | 5,328 | +1,309 | +32.570% |
| 5 | 895 | 1,392 | +497 | +55.531% |
| 6 | 531 | 1,052 | +521 | +98.117% |
| 7 | 895 | 1,172 | +277 | +30.950% |
| 8 | 531 | 1,016 | +485 | +91.337% |
| 9 | 158,799 | 170,460 | +11,661 | +7.343% |
| 10 | 295 | 484 | +189 | +64.068% |
| 11 | 375 | 792 | +417 | +111.200% |
| 12 | 375 | 660 | +285 | +76.000% |
| 13 | 563 | 952 | +389 | +69.094% |
| 14 | 202,779 | 203,956 | +1,177 | +0.580% |
| 15 | 6,443 | 6,808 | +365 | +5.665% |
| 16 | 1,343 | 1,800 | +457 | +34.028% |
| 17 | 33,167 | 33,380 | +213 | +0.642% |
| 18 | 123,235 | 123,552 | +317 | +0.257% |
| 19 | 2,439 | 2,928 | +489 | +20.049% |
| 20 | 398,523 | 386,776 | -11,747 | -2.948% |
| 21 | 9,303 | 9,196 | -107 | -1.150% |
| 22 | 100,055 | 104,892 | +4,837 | +4.834% |
| 23 | 33,951 | 38,576 | +4,625 | +13.623% |

The source values and exact boundary definitions are frozen in
`attempt-005_nrpdcch_full/task_timing_vs_rtl.json`.

## PDSCHDag2 A47 held-out regression

Run: `attempt-009_pdsch_heldout`.

- natural completion at tick 1,541,244,000;
- 9/9 tasks release and gem5 emits 8,103 VINS outputs;
- the retained RTL capture contains 7,256 outputs for six tile0 tasks; all
  7,256 compare element-by-element after normalizing RTL GATHER/SCATTER and
  gem5 VSHUFFLE monitor names;
- 847 outputs from tasks 1 and 2 have no retained RTL VINS capture, so this
  run does not claim a new direct 8,103/8,103 RTL comparison;
- full allocation-to-last-release span is 1,541,192 ns versus RTL
  1,535,100 ns: +6,092 ns, +0.397%.

| Task | RTL ns | gem5 ns | Delta ns | Error |
| ---: | ---: | ---: | ---: | ---: |
| 0 | 676,287 | 678,596 | +2,309 | +0.341% |
| 1 | 79,131 | 78,752 | -379 | -0.479% |
| 2 | 55,227 | 59,712 | +4,485 | +8.121% |
| 3 | 471,571 | 476,768 | +5,197 | +1.102% |
| 4 | 30,175 | 32,556 | +2,381 | +7.891% |
| 5 | 11,059 | 10,596 | -463 | -4.187% |
| 6 | 3,795 | 3,992 | +197 | +5.191% |
| 7 | 1,059 | 1,272 | +213 | +20.113% |
| 8 | 330,955 | 331,420 | +465 | +0.141% |

Direct VINS evidence is
`attempt-009_pdsch_heldout/vins_vs_rtl_available.json`; timing values are in
`attempt-009_pdsch_heldout/task_timing_vs_rtl.json`.  The comparator's
`--normalize-shuffle` option changes only opcode-name equivalence; all values,
suffixes, instruction counts, and ordering remain strict.

## Current conclusion and next boundary

The LSU addrgen request/ack and tagged completion visibility are now explicit,
and task17 is locally close, but the model is not microtiming-complete.

The next first-divergence work is structural:

1. task17 sequence-6 VSEQ: split operand requester admission, result enqueue,
   per-bank VRF grant, and hazard-class retirement edges;
2. task20: replace transient generic XBar retries with a stable tagged
   per-bank request vector, independent LSU high priority, persistent RR, and
   one-shot grant capture;
3. short tasks: compare task start/complete and DMT release edges separately
   so the recurring 0.2--0.5 microsecond overhead is assigned to its true
   scheduler/DMT owner.

No global latency fitting is justified by the current evidence.
