# A32 R56 VSEQ requester and per-bank RR alignment

> Superseded by A32 R57.  Inspection of the RTL FairArb source showed that
> R56 interpreted `rr_q` incorrectly.  With source-correct FairArb, task17
> sequence 6 still has an exact endpoint but operand-B admission is +12 ns
> versus RTL +14 ns.  R56 is retained only as historical evidence.

## Scope and disposition

R56 continues from R55 without changing any fixed LSU or VFU latency.  It
adds explicit registered boundaries for the task17 sequence-6 dependency
chain and exercises the experimental stable requester/per-bank arbitration
path required by task20.

The candidate uses:

    VENUS_GEM5_EXPERIMENTAL_VRF_RR=1
    VENUS_GEM5_EXPERIMENTAL_REQUESTER_Q_VISIBILITY=1
    --venus-config=venus-rtl-16x128

Candidate binary:

    build/RISCV/gem5.debug
    SHA256 65cf04f1681f6039eac511bb7f2cc8d57e880f17d9e23fc172db38ae27323d78

Evidence root:

    evidence/a32_r56_vseq_requester_rr_20260731

The result is not promoted as a complete timing model.  Task17 sequence 6 is
now edge-exact, but later task17 instructions diverge.  Task20 arbitration is
structurally active and much closer, but the aligned 30-instruction window
still has 46 blocked result edges versus RTL 37.

## Structural changes

### Registered VSEQ dependency and retirement boundaries

The requester-local RAW counter now treats the first producer writeback as a
registered pulse.  A dependent BitALU requester consumes it on the following
lane edge.  The completion-pending path keeps RTL clear-wins semantics for
the tail; it is not an added instruction latency.

Lane completion is also queued for one tile cycle before the sequencer sees
it.  This separates lane retirement from sequencer recycle instead of
collapsing both into one gem5 callback.

Relevant implementation:

- src/venus/VenusLane.cc:1343-1439
- src/venus/VenusSequencer.py:22-27
- src/venus/VenusSequencer.hh:118-150
- src/venus/VenusSequencer.cc:419-426
- src/venus/VenusSequencer.cc:1517-1540

### Stable tagged requester and per-bank arbitration

Each lane request port holds exactly one blocked packet and retries that same
packet.  A deferred write grant is captured once and validated against its
address, size, instruction ID, and running ID before result-queue dequeue.

The Xbar keeps per-bank pending sources, a selected source, and a persistent
RR pointer.  LSU priority is represented outside the ordinary VFU requester
vector.  The selected source and RR state change only after the packet
actually crosses the Xbar.

Relevant implementation:

- src/venus/VenusLane.cc:135-198
- src/mem/noncoherent_xbar.hh:75-83
- src/mem/noncoherent_xbar.cc:231-315
- src/mem/noncoherent_xbar.cc:338-445

No task ID, PC, DAG, address, or payload special case was added.

## task17 sequence-6 VSEQ

The first seven instruction durations now match RTL:

| Sequence | Operation | RTL cycles | gem5 cycles | Delta |
| ---: | --- | ---: | ---: | ---: |
| 0 | VBRDCST | 12 | 12 | 0 |
| 1 | VLOAD | 34 | 34 | 0 |
| 2 | VBRDCST | 12 | 12 | 0 |
| 3 | VSEQ | 29 | 29 | 0 |
| 4 | VMUL | 32 | 32 | 0 |
| 5 | VSADD | 34 | 34 | 0 |
| 6 | VSEQ | 34 | 34 | 0 |

For sequence 6, offsets are relative to fire at tick 426,304,000:

| Stage | RTL ns | gem5 ns |
| --- | --- | --- |
| operand admission A/B | 8 / 14 | 8 / 14 |
| operand VRF grants | 50, 54, 56, 58 | 50, 54, 56, 58 |
| result enqueue | 56, 60, 62, 64 | 56, 60, 62, 64 |
| result VRF grants | 56, 60, 62, 64 | 56, 60, 62, 64 |
| lane retirement | 66 | 66 |
| sequencer recycle | 68 | 68 |

Machine-readable evidence is
attempt-001_task17_stage/task17_sequence6_vs_rtl.json.  This exact local
match does not make task17 complete: its whole-task duration is now 33,728 ns
versus RTL 33,167 ns, or +561 ns (+1.691%).  The next divergence is after
sequence 6 and must be located independently.

## task20 requester and arbitration

### First VRANGE

Instruction 1447 now takes 31 cycles in both RTL and gem5:

| Boundary from fire | gem5 ns | RTL match |
| --- | ---: | --- |
| first result enqueue | 14 | yes |
| first result grant | 16 | yes |
| final result grant | 58 | yes |
| lane retirement | 60 | yes |
| sequencer recycle | 62 | yes |

The tagged source-1 request for bank 2 is blocked by LSU priority at tick
561,968,000 and the same source is granted at 561,970,000.  A bank-0 witness
shows persistent state across winners:

| Tick | old RR | winner | next RR | contenders |
| ---: | ---: | ---: | ---: | ---: |
| 561,964,000 | 9 | 1 | 1 | 2 |
| 561,966,000 | 1 | 3 | 2 | 2 |
| 561,970,000 | 2 | 2 | 8 | 3 |
| 561,972,000 | 8 | 8 | 9 | 2 |

The first-VRANGE evidence is
attempt-002_task20_arbiter/task20_first_vrange_summary.json.

### Aligned 30-CAU-instruction window

The longer check aligns the first 30 retired CAU instructions, not a
wall-clock endpoint at which the faster model could contain more work.
Duplicate rejection prints are collapsed by
tick/instruction/running-ID/offset, and a rejection is excluded when that
same tagged request is accepted on the same tick.

| Metric | RTL | previous gem5 | R56 gem5 |
| --- | ---: | ---: | ---: |
| result enqueue | 256 | 256 | 256 |
| accepted grant | 256 | 256 | 256 |
| retirement | 30 | 30 | 30 |
| result queue max occupancy | 2 | 2 | 2 |
| normalized blocked | 37 | 99 | 46 |

R56 removes 53 of the former 62 excess blocked edges, but still has nine
more than RTL.  Thus the stable requester vector, independent LSU priority,
and persistent per-bank RR are demonstrably active, while their complete
edge ordering is not yet exact.

Evidence:

- attempt-006_task20_4us/task20_4us_summary.json
- attempt-006_task20_4us/lane0_cau_events.jsonl
- attempt-006_task20_4us/lane0_cau_summary.jsonl

## Directed regression gates

The LSU suite passes 68/68 cases, and all 382 accepted VINS streams are
byte-exact to R55.

On the normal capacity-test path, all six 1/4/16/32/64/128 ns gap cases are
functionally exact and BitALU, CAU, and SerDiv all reach depth 2/full.

With the experimental RR flags enabled, outputs remain exact and BitALU/CAU
reach depth 2/full.  SerDiv reaches occupancy 2, but RR service cadence means
the microbench does not attempt a third enqueue by 128 ns, so its full signal
is not observed.  The automated RR capacity report therefore remains false;
this is a coverage exception and is not reported as a pass.

Evidence is under attempt-003_directed.

## nrPDCCH full regression

Run: attempt-004_nrpdcch_full.

- natural completion at tick 1,047,988,000;
- 24/24 tasks and 53/53 returns complete;
- 10,253 VINS are task-local ordinal exact to the accepted R55 baseline;
- allocation-to-last-release is 1,047,936 ns versus RTL 1,043,224 ns:
  +4,712 ns, +0.452%.

Selected task timing:

| Task | RTL ns | gem5 ns | Delta ns | Error |
| ---: | ---: | ---: | ---: | ---: |
| 3 | 69,631 | 70,676 | +1,045 | +1.501% |
| 9 | 158,799 | 170,456 | +11,657 | +7.341% |
| 14 | 202,779 | 203,912 | +1,133 | +0.559% |
| 17 | 33,167 | 33,728 | +561 | +1.691% |
| 18 | 123,235 | 123,552 | +317 | +0.257% |
| 20 | 398,523 | 389,752 | -8,771 | -2.201% |
| 22 | 100,055 | 104,892 | +4,837 | +4.834% |
| 23 | 33,951 | 38,576 | +4,625 | +13.623% |

The full table is frozen in
attempt-004_nrpdcch_full/task_timing_vs_rtl.json.  Task9 and task20 still
cancel materially, and sub-microsecond tasks still show large relative
errors.  The +0.452% DAG result is not an alignment pass.

## PDSCHDag2 held-out regression

Run: attempt-005_pdsch_heldout.

- natural completion at tick 1,542,300,000;
- 9/9 tasks and 22/22 returns complete;
- all 8,103 task-ordinal VINS are exact to R55;
- all 7,256 retained RTL VINS compare element-by-element exact;
- 847 VINS from tasks 1 and 2 have no retained RTL dump and are not counted
  as a direct RTL comparison;
- allocation-to-last-release is 1,542,248 ns versus RTL 1,535,100 ns:
  +7,148 ns, +0.466%.

| Task | RTL ns | gem5 ns | Delta ns | Error |
| ---: | ---: | ---: | ---: | ---: |
| 0 | 676,287 | 678,596 | +2,309 | +0.341% |
| 1 | 79,131 | 78,752 | -379 | -0.479% |
| 2 | 55,227 | 59,692 | +4,465 | +8.085% |
| 3 | 471,571 | 476,444 | +4,873 | +1.033% |
| 4 | 30,175 | 31,620 | +1,445 | +4.789% |
| 5 | 11,059 | 11,020 | -39 | -0.353% |
| 6 | 3,795 | 4,032 | +237 | +6.245% |
| 7 | 1,059 | 1,272 | +213 | +20.113% |
| 8 | 330,955 | 333,272 | +2,317 | +0.700% |

The direct RTL comparison and timing boundaries are recorded in
attempt-005_pdsch_heldout/vins_vs_rtl_available.json and
attempt-005_pdsch_heldout/task_timing_vs_rtl.json.

## Conclusion and next breakpoint

R56 closes the requested task17 sequence-6 operand-admission, result-enqueue,
per-bank grant, lane-retirement, and sequencer-recycle edges exactly.  It
also replaces task20's generic retry behavior with stable tagged ownership,
independent LSU priority, and persistent per-bank RR, reducing the comparable
blocked count from 99 to 46.

The model is still incomplete:

1. locate task17's first post-sequence-6 duration divergence;
2. classify the remaining task20 46 versus 37 blocked edges by winning
   requester and correct the nine-edge excess without latency fitting;
3. isolate scheduler/DMT fixed boundaries for short tasks;
4. keep the experimental flags off the safe default until those local
   timing checks close.

Whole-DAG percentage and cross-task cancellation remain non-acceptance
criteria.
