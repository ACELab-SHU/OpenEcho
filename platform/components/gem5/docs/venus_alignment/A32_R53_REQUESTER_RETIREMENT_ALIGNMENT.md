# A32 R53: requester visibility and retirement ownership

Date: 2026-07-30

## Scope and frozen baseline

R53 continues from the promoted R52 binary:

`/tmp/a32_r52_vfu_edge_alignment_20260730/attempt-011/promoted/gem5.debug`

R52 SHA256:

`f20550a342007504984a6b61166d48b6bf29038f783c0d9ffac40b9dbc1b62f3`

The A47 dispatcher/barrier fixes and the R48--R52 structured LSU, DMT,
tagged operand/VRF response, hazard-class retirement, result-generation
ownership, and fixed-depth VFU result queues remain in place. No global DAG,
LSU, or VFU fixed latency was changed.

## Structural changes

### VRANGE operand contract

RTL OPMISC/VRANGE presents `vs1`, `vs2`, and `vd1` as real operand
requesters even though the CAU generates the range value internally. Gem5
previously elided these reads. `VenusInstrPkt` decode/autocomplete now sets
`use_vs1=1`, `use_vs2=1`, `use_vd1=1`, `vm_r=0`, and `vew=EW16`.
This is an opcode contract, not a task or PC special case.

### Tagged requester-q visibility prototype

A one-lane-edge command-to-requester boundary was added to arithmetic
operand requesters. It is independently controlled by:

`VENUS_GEM5_EXPERIMENTAL_REQUESTER_Q_VISIBILITY=1`

The existing deferred per-bank RR prototype is controlled by:

`VENUS_GEM5_EXPERIMENTAL_VRF_RR=1`

Both remain disabled by default. The combination is useful evidence, but the
four-microsecond window below proves that it is not yet an RTL-equivalent
arbiter.

### VBRDCST single-owner retirement

The experimental requester boundary exposed a real WAW violation:

1. VBRDCST 1464 completed its ordinary BitALU banked writes;
2. younger VSGT 1465 began timing writes to the same physical VRF rows;
3. when the last VBRDCST lane reported done, the sequencer performed a
   second full-vector backdoor write;
4. that retirement side effect overwrote already granted younger data;
5. VSUB 1468 diverged because its first three CAU-B words became zero.

The watched physical address was `0x82100204`. Before the fix:

`[VRF watch] tick=571910000 write physical=0x82100204 old=0x1 new=0x0`

After the fix, the same edge is only a read and retains `0x1`.

The sequencer retirement backdoor writer was removed. The BitALU banked
result/grant path is now the sole VRF owner. VBRDCST VINS snapshots use the
instruction's tagged completion payload rather than rereading shared VRF
state after a younger WAW grant. RTL task20 ordinal 49 and 50 are both
256-element zero vectors; the repaired snapshots match them exactly while
VSUB 1468 also remains exact.

`MemoryAccess` debug output in `abstract_mem.cc` and the physical VRF watch
remain observability only; they do not change timing.

## First-edge alignment

For task20's first CAU/VRANGE:

| Boundary | RTL issue-relative | experimental gem5 admit-relative |
| --- | ---: | ---: |
| requester_q operand request | +2 ns | +2 ns |
| first result enqueue | +8 ns | +8 ns |
| first result blocked | +8 ns, CAU-B | +8 ns, bank arbitration |
| first result grant | +10 ns | +10 ns |
| queue reaches occupancy 2 | +10 ns | +12 ns |
| next CAU admission | +12 ns | +12 ns |

The original missing requester/result overlap is represented and the first
blocked/grant edge is aligned. The occupancy-2 edge is still one tile cycle
late.

Gem5 evidence:

`/tmp/a32_r52_vfu_edge_alignment_20260730/attempt-027_requester_q_visibility/focused/m5out/task20_requester_q.trace`

RTL evidence:

`/tmp/a32_r52_vfu_edge_alignment_20260730/attempt-003/rtl_task20_cau_edges_correct.vcd`

## Four-microsecond arbiter divergence

First-edge alignment is not sufficient. A four-microsecond task20 trace was
collected at:

`/tmp/a32_r52_vfu_edge_alignment_20260730/attempt-028_vbrdcst_retirement_owner/task20_4us/m5out/task20_4us.trace`

| Event | RTL | experimental gem5 |
| --- | ---: | ---: |
| CAU admissions | 32 | 27 |
| result enqueue/dequeue | 256/256 | 232/232 |
| maximum result occupancy | 2 | 2 |
| real blocked result edges | 37 | 99 |

Gem5 emits 317 raw `bank_backpressure` observations. After collapsing
duplicate calls for the same tagged intent and removing 140 cases that
receive a grant later in the same tick, 99 genuinely delayed edges remain.
Their observed winner/path classes are:

- 27 CAU-A/CAU-B operand edges, versus RTL 21;
- 22 BitALU-A/BitALU-B operand edges, versus RTL 6;
- 9 BitALU-result edges, versus RTL 4;
- 41 generic downstream/bank-busy edges without an RR winner in the same
  tick, while RTL separately identifies only 6 LSU-priority edges.

This is the next first divergence. The generic XBar retry protocol still
mixes deferred admission, downstream busy, and retry callbacks instead of
sampling one stable vector of tagged requester intents and one explicit
LSU-high-priority intent per bank edge.

## Experimental candidate validation and rejection

The RR/requester-q candidate is functionally general:

- nrPDCCH:
  `/tmp/a32_r52_vfu_edge_alignment_20260730/attempt-028_vbrdcst_retirement_owner/full_v2`
  completes at tick `1,163,372,000` with `10,253/10,253` VINS exact;
- VFU capacity:
  `/tmp/a32_r52_vfu_edge_alignment_20260730/attempt-028_vbrdcst_retirement_owner/capacity`
  passes all 1/4/16/32/64/128 ns gaps, preserves control hashes, and reaches
  occupancy 2/full in BitALU, CAU, and SerDiv;
- structured LSU:
  `/tmp/a32_r52_vfu_edge_alignment_20260730/attempt-028_vbrdcst_retirement_owner/lsu_suite`
  passes `68/68`, with `382/382` VINS exact;
- PDSCHDag2 held-out:
  `/tmp/a32_r52_vfu_edge_alignment_20260730/attempt-028_vbrdcst_retirement_owner/pdsch_heldout`
  completes at tick `1,614,308,000`, releases `9/9` tasks, and preserves
  `8,103/8,103` VINS.

It is not promoted as the default timing model. Task17 becomes `57,492 ns`
against RTL `33,167 ns` (`+73.341%`), and task20 becomes `496,680 ns`
against RTL `398,523 ns` (`+24.630%`). R52 task20 was only `-2.21%` from
RTL. The 99-versus-37 blocked-edge count is direct evidence that the larger
error is structural over-stall, not a missing fixed latency.

## Safe default validation and promotion

The safe default keeps both experimental switches off while retaining the
generic VRANGE decode and VBRDCST single-owner/snapshot fixes.

nrPDCCH:

`/tmp/a32_r53_vrf_requester_retirement_20260730/attempt-002_safe_default/nrpdcch`

- natural completion at tick `1,046,396,000`;
- `10,253/10,253` VINS exact;
- task17 `49,440 ns`;
- task20 `389,712 ns`.

PDSCHDag2 A47 held-out:

`/tmp/a32_r53_vrf_requester_retirement_20260730/attempt-002_safe_default/pdsch`

- natural completion at tick `1,526,048,000`;
- `9/9` tasks release;
- `8,103/8,103` VINS exact.

Promoted binary:

`/tmp/a32_r53_vrf_requester_retirement_20260730/attempt-003_promoted/gem5.debug`

SHA256:

`77ab61f8c5db6c46904f81c228e8e6fcfb9dc781521d3b1434269d4234b89234`

## Next breakpoint

Do not tune task or whole-DAG latency. The next implementation must move
VRF arbitration ownership out of transient `sendTimingReq` retry order:

1. snapshot one stable tagged intent per operand/result requester at each
   lane edge;
2. snapshot LSU bank intent separately and apply explicit LSU-over-low-level
   priority;
3. run one persistent per-bank RR decision over the complete request vector;
4. capture at most one grant per bank and one grant per requester master for
   that edge;
5. distinguish same-edge deferred admission from a real blocked edge;
6. close the task20 `99 -> 37` divergence before default promotion.

SerDiv still has no task17/task20 workload coverage. Its depth/full behavior
remains covered by the independent capacity test and must be rechecked after
the bank-edge intent refactor.
