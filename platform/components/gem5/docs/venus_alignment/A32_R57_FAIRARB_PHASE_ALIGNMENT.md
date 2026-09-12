# A32 R57 FairArb and cross-instruction phase alignment

## Scope and disposition

R57 supersedes the R56 timing conclusion after checking the actual RTL
`rr_arb_tree.sv` FairArb implementation.  It retains the stable tagged
requester, independent LSU-priority intent, per-bank selected owner, and
registered task17 dependency/retirement boundaries.  It does not change any
fixed LSU or VFU latency.

The candidate uses:

    VENUS_GEM5_EXPERIMENTAL_VRF_RR=1
    VENUS_GEM5_EXPERIMENTAL_REQUESTER_Q_VISIBILITY=1
    --venus-config=venus-rtl-16x128

Candidate binary:

    build/RISCV/gem5.debug
    SHA256 4dd151555e14307972ed18d4514bdadf5ba82226b33333c349684dc59857067e

Evidence root:

    evidence/a32_r57_fairarb_phase_20260731

This candidate is not a complete timing model.  Task17 sequence 6 has an
exact 34-cycle endpoint but a two-nanosecond operand-B admission mismatch.
Task20's first VRANGE has exact blocked count and endpoint, but one winning
requester differs and the next instruction is the first cross-instruction
divergence.

## RTL-source correction to persistent RR

The R56 arbiter treated `rr_q` as binary-tree preference bits.  The actual
FairArb path in
`venus_soc/hardware/venus_extension/common_cells/rr_arb_tree.sv:203-232`
does something different:

1. select the lowest asserted requester whose index is strictly greater than
   `rr_q`;
2. wrap to the lowest asserted requester at or below `rr_q` if necessary;
3. update `rr_q` to the requester that was actually granted.

The gem5 Xbar now implements that exact ordering in
`src/mem/noncoherent_xbar.cc:153-180` and updates the persistent per-bank
pointer only after a successful bank crossing at
`src/mem/noncoherent_xbar.cc:381-410`.  A downstream retry retains the exact
selected source and tagged packet.  LSU priority remains outside the twelve
ordinary VFU masters.

This is a source-structure correction, not a latency fit.  R56 remains a
historical experiment but its stage-exact and RR-witness claims are not the
current result.

## task17 sequence-6 VSEQ

The first seven task-local instruction durations remain exact:

| Sequence | Operation | RTL cycles | gem5 cycles | Delta |
| ---: | --- | ---: | ---: | ---: |
| 0 | VBRDCST | 12 | 12 | 0 |
| 1 | VLOAD | 34 | 34 | 0 |
| 2 | VBRDCST | 12 | 12 | 0 |
| 3 | VSEQ | 29 | 29 | 0 |
| 4 | VMUL | 32 | 32 | 0 |
| 5 | VSADD | 34 | 34 | 0 |
| 6 | VSEQ | 34 | 34 | 0 |

For sequence 6, offsets are relative to gem5 fire tick 426,144,000:

| Stage | RTL ns | gem5 ns | Status |
| --- | --- | --- | --- |
| operand admission A/B | 8 / 14 | 8 / 12 | B is 2 ns early |
| operand VRF grants | 50, 54, 56, 58 | 50, 54, 56, 58 | exact |
| result enqueue | 56, 60, 62, 64 | 56, 60, 62, 64 | exact |
| result VRF grants | 56, 60, 62, 64 | 56, 60, 62, 64 | exact |
| lane retirement | 66 | 66 | exact |
| sequencer recycle | 68 | 68 | exact |

Therefore the endpoint is exact, but stage timing is not.  The earlier
operand-B admission catches up before the first VRF grant; this is precisely
why duration-only comparison would miss the divergence.  No artificial
operand-B delay was added because the current evidence points to requester
availability inherited from surrounding instruction state, not a universal
fixed boundary.

The causal edge is now explicit.  Sequence 6 reuses the BitALU-B requester
that is still serving sequence 3, global instruction 715.  Relative to 715's
fire, the current FairArb run grants its four B rows at 40, 42, 44, and 46 ns;
the requester consumes the last response, completes, and performs its
fall-through handoff at +48 ns.  Since instruction 718 fires 36 ns after 715,
that handoff produces the observed +12 ns admission.  In superseded R56 the
last 715 grant was +48 ns and completion was +50 ns, producing the RTL-like
718 admission at +14 ns.  The missing edge is therefore the final-bank
request vector/RR winner for predecessor 715, not a generic delay on 718.

Machine-readable evidence:

- `attempt-001_task17_stage/task17_sequence6_vs_rtl.json`
- `attempt-001_task17_stage/stage.log`
- `attempt-001_task17_stage/venusgem5_sequencer_monitor.json`

The complete run measures task17 at 33,780 ns versus RTL 33,167 ns:
+613 ns, +1.848%.  Thus sequence-6 endpoint equality does not close task17.

## task20 stable requester, LSU priority, and FairArb

### First VRANGE

Global instruction 1447 fires at 561,804,000 and recycles at 561,866,000:
31 tile cycles, equal to RTL.  The local result boundaries also match:

| Boundary from fire | RTL ns | gem5 ns |
| --- | ---: | ---: |
| first result enqueue | 14 | 14 |
| first result grant | 16 | 16 |
| final result grant | 58 | 58 |
| lane retirement | 60 | 60 |
| sequencer recycle | 62 | 62 |

Both models have seven normalized blocked result edges, but the winner
sequence is not byte-for-byte equivalent as an arbitration trace:

| Edge | RTL winner | gem5 winner |
| ---: | --- | --- |
| 0 | CAU-B | CAU-B |
| 1 | LSU | LSU |
| 2 | CAU-A | CAU-B |
| 3 | BitALU result | BitALU result |
| 4 | LSU | LSU |
| 5 | BitALU result | BitALU result |
| 6 | LSU | LSU |

Count and endpoint equality therefore do not constitute exact requester-vector
alignment.

### Aligned first-30-CAU window

The comparison aligns the same 30 retired CAU instructions, beginning with
1447, instead of comparing unrelated wall-clock endpoints.  Duplicate gem5
rejections are collapsed by tick/instruction/running-ID/offset; a rejection
is removed when the same tagged request is accepted on that tick.  The bank
identity is carried across each stable tagged retry before assigning the
winning requester.

| Metric | RTL | gem5 |
| --- | ---: | ---: |
| result enqueue | 256 | 256 |
| accepted grant | 256 | 256 |
| retirement | 30 | 30 |
| observed result-queue occupancy | 2 | 2 |
| normalized blocked | 37 | 46 |

Blocked winners are:

| Winner | RTL | gem5 | Delta |
| --- | ---: | ---: | ---: |
| LSU | 6 | 7 | +1 |
| CAU-A | 11 | 6 | -5 |
| CAU-B | 10 | 14 | +4 |
| BitALU-A | 2 | 1 | -1 |
| BitALU-B | 4 | 6 | +2 |
| BitALU result | 4 | 12 | +8 |

The first instruction is locally count/endpoint exact.  The first subsequent
divergence is instruction 1448: RTL has three normalized blocked edges and
gem5 has seven, while gem5's first result is 1.999 ns early.  Later
instructions contain both positive and negative blocked deltas, and the first
result phase moves from about +10 ns late at 1458 to 24--50 ns early around
1481--1502.  This is cross-instruction issue/admission/hazard phase drift, not
a remaining scalar offset in the RR selector.

Machine-readable comparison and raw captures:

- `attempt-002_task20_fairarb/task20_first30_vs_rtl.json`
- `attempt-002_task20_fairarb/lane0_cau_events.jsonl`
- `attempt-002_task20_fairarb/lane0_cau_summary.jsonl`
- `attempt-002_task20_fairarb/task20_4us.log`
- `attempt-002_task20_fairarb/rtl_task20_lsu_window.vcd`
- `attempt-002_task20_fairarb/rtl_task20_lsu_window.log`

## Directed regression gates

The current binary passes all 68 LSU RAW/throughput/capacity cases.  The 382
accepted VINS outputs are unchanged from the R55 exact baseline.

The experimental RR capacity path passes all six 1/4/16/32/64/128 ns cases
with exact functional hashes.  BitALU, CAU, and SerDiv all actually reach
depth 2/full; the former R56 SerDiv coverage exception is gone.

Evidence:

- `attempt-003_directed/lsu_suite_results.json`
- `attempt-003_directed/vfu_capacity_results.json`

## nrPDCCH full regression

Run: `attempt-004_nrpdcch_full`.

- natural completion at tick 1,051,364,000;
- 24/24 tasks and 53/53 returns complete;
- all return files are exact to the preceding accepted candidate;
- 10,253 VINS are task-local ordinal exact to R55;
- allocation-to-last-release is 1,051,312 ns versus RTL 1,043,224 ns:
  +8,088 ns, +0.775%.

Selected task timing:

| Task | RTL ns | gem5 ns | Delta ns | Error |
| ---: | ---: | ---: | ---: | ---: |
| 0 | 547 | 1,128 | +581 | +106.216% |
| 3 | 69,631 | 70,824 | +1,193 | +1.713% |
| 9 | 158,799 | 170,472 | +11,673 | +7.351% |
| 14 | 202,779 | 204,036 | +1,257 | +0.620% |
| 17 | 33,167 | 33,780 | +613 | +1.848% |
| 18 | 123,235 | 123,552 | +317 | +0.257% |
| 20 | 398,523 | 393,272 | -5,251 | -1.318% |
| 22 | 100,055 | 104,892 | +4,837 | +4.834% |
| 23 | 33,951 | 38,576 | +4,625 | +13.623% |

Task9 and task20 still cancel materially.  The full-DAG percentage is not an
acceptance criterion, and the short tasks still expose 30--111% relative
errors.

## PDSCHDag2 held-out regression

Run: `attempt-005_pdsch_heldout`.

- natural completion at tick 1,542,336,000;
- 9/9 tasks and 22/22 returns complete and exact to the preceding candidate;
- all 8,103 VINS are task-local ordinal exact to R55;
- all 7,256 retained RTL VINS compare element-by-element exact;
- 847 VINS from tasks 1 and 2 still have no RTL dump and are not presented as
  a direct RTL comparison;
- allocation-to-last-release is 1,542,284 ns versus RTL 1,535,100 ns:
  +7,184 ns, +0.468%.

| Task | RTL ns | gem5 ns | Delta ns | Error |
| ---: | ---: | ---: | ---: | ---: |
| 0 | 676,287 | 678,728 | +2,441 | +0.361% |
| 1 | 79,131 | 78,860 | -271 | -0.342% |
| 2 | 55,227 | 59,792 | +4,565 | +8.266% |
| 3 | 471,571 | 476,364 | +4,793 | +1.016% |
| 4 | 30,175 | 31,444 | +1,269 | +4.205% |
| 5 | 11,059 | 11,132 | +73 | +0.660% |
| 6 | 3,795 | 4,024 | +229 | +6.034% |
| 7 | 1,059 | 1,272 | +213 | +20.113% |
| 8 | 330,955 | 333,328 | +2,373 | +0.717% |

## Conclusion and next breakpoint

R57 corrects the per-bank RR semantics against RTL source and keeps the
requested stable tagged ownership and LSU priority active.  It also prevents
an endpoint match from hiding task17's operand-B admission mismatch and
task20's requester-winner mismatch.

The next local breakpoints are:

1. task17 sequence-6 operand-B command/requester availability: explain the
   final instruction-715 B-row grant that makes admission +12 ns versus RTL
   +14 ns, without adding a universal delay;
2. task20 instruction 1447 to 1448: align the surviving requester/hazard state
   and issue phase that creates seven blocked edges instead of three;
3. propagate that fix through the 30-instruction window and require both the
   37-edge total and per-instruction/winner sequence to match;
4. continue treating short-task scheduler/DMT boundaries independently.

Whole-DAG percentage, task cancellation, duration-only equality, and blocked
count-only equality remain non-acceptance criteria.
