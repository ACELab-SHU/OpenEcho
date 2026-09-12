# A32 R508 producer-completion alignment

Date: 2026-08-12

Status: **INCONCLUSIVE**. Two real combinational completion boundaries are now
modeled and task20's exact instruction prefix has advanced, but later task20
and SCH microtiming still diverge. Whole-DAG timing is diagnostic only.

## Scope and invariant

This round changed only generation-tagged completion visibility:

1. after every participating lane has received the final destination-bank
   grant, that exact producer generation is made visible to Shuffle's captured
   hazard mask;
2. after Shuffle has received its final destination-bank grant, that exact
   producer generation is made visible to sequencer-local consumers such as
   VSTU operand admission and the LDU result requester.

The registered lane/Shuffle PE responses, VINS dump, public hazard broadcast,
running-ID release, and retirement monitor remain at their later boundaries.
No LSU/VFU fixed latency was changed. There is no task, DAG, PC, address, data,
or opcode special case.

Implementation:

- `src/venus/VenusSequencer.cc::noteLaneProducerGrant` aggregates active lanes
  by `(running_id, instruction_id)` and forwards one tagged completion only to
  Shuffle.
- `src/venus/VenusSequencer.cc::noteShuffleProducerGrant` retires that exact
  generation only from sequencer-local dependency tests.
- The existing registered completion and public lifecycle paths are unchanged.

Binary SHA256:

```text
d335761e3727285b2c5b4ed2bd5e1fde6f3ddb47eab29515f84aa9f5cfa2e50e
```

## RTL oracle: task20 sequence 29

The accelerated `nrPDCCH_tail18_23_a27` task2 is cycle-equivalent to full-DAG
task20. The R499 oracle shows:

- sequence 29 captures hazards against running IDs 2, 0, and 1;
- sequence 28 receives its final lane bank grant at 269055.999 ns;
- Shuffle changes `IDLE -> WAIT` while the registered hazard row still appears
  live, because RTL consumes `global_hazard_table_d`;
- the public sequence-28 VID recycle is later, at 269059 ns.

Before the fix, gem5 waited for the public recycle and started sequence 29 four
tile cycles late. After the tagged lane-producer sideband, sequence 29 is exact:

| boundary | RTL cycles | gem5 cycles |
| --- | ---: | ---: |
| fire | 4116 | 4116 |
| duration | 84 | 84 |
| recycle | 4200 | 4200 |

## RTL oracle: task20 sequence 30

The R506 STU/AXI oracle shows sequence 30's address request is not the gap:
STU accept and registered addrgen acknowledgement already match. RTL begins the
VSTU operand request when the preceding Shuffle's final bank grants clear the
captured RAW generation, before public Shuffle recycle. Gem5 previously waited
for public recycle, missed an AXI phase, and completed two tile cycles late.

After making Shuffle final-bank completion visible to sequencer-local hazards:

| boundary | RTL cycles | gem5 cycles |
| --- | ---: | ---: |
| fire | 4122 | 4122 |
| duration | 116 | 116 |
| recycle | 4238 | 4238 |
| sequence 31 fire / recycle | 4253 / 4302 | 4253 / 4302 |
| sequence 32 fire / recycle | 4261 / 4320 | 4261 / 4320 |

The gem5 LSU event stream correspondingly moves final W data commit from cycle
4224 to 4222, B response from 4238 to 4236, and public completion from 4240 to
4238. This was achieved without changing `VenusStuCommitCycles` or any other
latency constant.

The first remaining task20 lifecycle divergence is now sequence 34/36:

- sequence 34 VADD duration/recycle is one cycle early;
- sequence 36 VBRDCST duration/recycle is one cycle late;
- sequence 37 VBRDCST is four cycles early;
- sequences 40--49 later reconverge at their public recycle boundaries;
- sequence 50 first fires 77 cycles early but recycles on the same RTL cycle,
  indicating admission/resource-ID timing is still not equivalent even where
  a later completion happens to reconverge.

## Post-R508 RTL oracle: sequence 33--37 requester and RR phase

R516 adds lane-0 result IDs/addresses to the RTL oracle and R519 exposes the
four physical `rr_arb_tree` `rr_q` registers.  They establish the following:

- sequence 34 is CAU result ID 3, addresses 96--111; sequence 36 is BitALU
  result ID 5, addresses 88--95; sequence 37 is BitALU result ID 6, addresses
  128--143;
- sequence 37's long tail is real arbitration: it loses bank 3 to requester 1,
  and later loses cycles to LSU high priority.  It is not a fixed BitALU
  latency error;
- all four RTL RR pointers are persistent and enter this window at requester
  11.  Gem5's persistent per-bank RR algorithm and requester-number mapping
  match the RTL structure; the first proved mismatch occurs before the RR
  choice, in the requester vector presented to it;
- RTL `issue_mailbox` samples `pe_req_valid_o`.  Its lane input is a
  fall-through register, so sequence 33 presents its first operand requests
  one tile cycle after mailbox issue.  The accepted gem5 baseline presents
  them four cycles after its otherwise exact fire boundary.

Two general structural experiments were therefore run, with no opcode/task/
address special case:

| experiment | sequence-33 fire shift | sequence-34 recycle vs RTL | task end vs accepted baseline | disposition |
| --- | ---: | ---: | ---: | --- |
| PE broadcast at fire + same-edge lane fall-through | -32 cycles by this window | much earlier | -648 ns | rejected |
| same-edge lane fall-through only | -4 cycles | -8 cycles | +288 ns | rejected |

Both experiments improve the isolated front-end relation but expose an older
compensating delay: removing the lane-side bubble also advances running-ID
release and later admission.  They were fully reverted.  The next fix must
model the pair of RTL boundaries together: same-edge fall-through command
capture and the corresponding registered command-consume/completion/ready
return.  Changing fixed VFU latency or the RR rotation cannot repair this
earlier request-vector phase error.

The restored-source rebuild is SHA256
`4e50fb61a079af4600c66f6853ba8bebc283e4e81dfee79d9b8d31976818bb7f`.
Its task20 exit tick is again `391316000`, and its complete 8,837-entry
sequencer monitor is byte-identical to R517; sequences 31--37 return exactly
to the accepted values above.

Permanent oracle files are under
`evidence/a32_r508_producer_completion_20260812/rtl_oracle/`.  Rejected runs
remain in `/tmp/a32_r520_task20_fallthrough_gem/` and
`/tmp/a32_r521_task20_lane_fallthrough_gem/`.

## Directed regression

- task20: 8,837 VINS values byte-exact; first divergence advanced past
  sequence 30 to sequence 34/36.
- task17: 687 VINS values byte-exact; first divergence remains sequence 180
  VBRDCST at -1 cycle, and first fire divergence remains sequence 194 VLOAD at
  +1 cycle.
- LSU RAW/throughput/capacity: 68/68 passed; 382 result files remain exact.

## Current CCH task execution timing

The boundary is RTL `start execute -> execute complete` versus gem5
`tile_start -> task_epilogue`; return DMA is excluded.

| task | RTL ns | gem5 ns | delta ns | error |
| ---: | ---: | ---: | ---: | ---: |
| 0 | 547 | 536 | -11 | -2.011% |
| 1 | 10,691 | 10,580 | -111 | -1.038% |
| 2 | 547 | 536 | -11 | -2.011% |
| 3 | 69,631 | 69,360 | -271 | -0.389% |
| 4 | 4,019 | 4,010 | -9 | -0.224% |
| 5 | 895 | 886 | -9 | -1.006% |
| 6 | 531 | 522 | -9 | -1.695% |
| 7 | 895 | 886 | -9 | -1.006% |
| 8 | 531 | 522 | -9 | -1.695% |
| 9 | 158,799 | 158,678 | -121 | -0.076% |
| 10 | 295 | 288 | -7 | -2.373% |
| 11 | 375 | 366 | -9 | -2.400% |
| 12 | 375 | 366 | -9 | -2.400% |
| 13 | 563 | 612 | +49 | +8.703% |
| 14 | 202,779 | 202,974 | +195 | +0.096% |
| 15 | 6,443 | 6,344 | -99 | -1.537% |
| 16 | 1,343 | 1,330 | -13 | -0.968% |
| 17 | 33,167 | 33,154 | -13 | -0.039% |
| 18 | 123,235 | 123,234 | -1 | -0.001% |
| 19 | 2,439 | 2,398 | -41 | -1.681% |
| 20 | 398,523 | 391,308 | -7,215 | -1.810% |
| 21 | 9,303 | 9,306 | +3 | +0.032% |
| 22 | 100,055 | 100,052 | -3 | -0.003% |
| 23 | 33,951 | 33,944 | -7 | -0.021% |

CCH has 24/24 completed tasks and 10,253 VINS values exact. The functional DAG
span is 1,035,128 ns versus RTL 1,043,224 ns: -8,096 ns (-0.776%). This got
farther from zero because the newly correct early completion removes delay that
had been compensating later task20 gaps. It is not a reason to revert a locally
proven hardware boundary.

## Current SCH task execution timing

| task | RTL ns | gem5 ns | delta ns | error |
| ---: | ---: | ---: | ---: | ---: |
| 0 | 676,287 | 676,296 | +9 | +0.001% |
| 1 | 79,131 | 79,116 | -15 | -0.019% |
| 2 | 55,227 | 54,854 | -373 | -0.675% |
| 3 | 471,571 | 476,756 | +5,185 | +1.100% |
| 4 | 30,175 | 30,618 | +443 | +1.468% |
| 5 | 11,059 | 11,016 | -43 | -0.389% |
| 6 | 3,795 | 3,786 | -9 | -0.237% |
| 7 | 1,059 | 1,056 | -3 | -0.283% |
| 8 | 330,955 | 329,864 | -1,091 | -0.330% |

SCH has 9/9 completed tasks and 8,103 VINS values exact. The functional DAG span
is 1,539,216 ns versus RTL 1,535,100 ns: +4,116 ns (+0.268%). Task3's +5,185 ns
and task8's -1,091 ns still cancel in the total.

## Next first-divergence work

1. task20 sequence 33--37: replace the inferred FIFO/busy timing with explicit
   lane fall-through valid/ready and per-operand command-register state.  The
   same-edge command capture must be paired with RTL's registered consume and
   completion/ready return before checking the now-observable stable requester
   vector against each bank's persistent RR state.  Sequence 50's early fire
   remains a separate admission/ID-allocation boundary even though recycle
   reconverges.
2. SCH task3: preserve the exact sequence-6/7 prefix and locate its first later
   fire/recycle divergence; then keep task2, task4, and task8 as independent
   held-out gaps.
3. Short scalar-only CCH tasks have a recurring 7--13 ns deficit, while task13
   has +49 ns/+8.703%. These are CPU/tile lifecycle boundaries, not evidence
   that task13 should outrank the multi-microsecond vector gaps.

Evidence is under `evidence/a32_r508_producer_completion_20260812/`.
