# A32 R797 Shuffle completion-Q alignment

## Scope and verdict

This round models the registered boundary between the final per-bank Shuffle
result grant and the producer-completion state observed by dependent LSU/LDU
requesters.  It does not change CPU, LSU, Shuffle, or VFU fixed latency and it
contains no task, sequence, PC, opcode, address, or data special case.

The promoted binary SHA256 is
`db507e49b1cecaa7b51a168bd347b80f5eb54aa386b0d5a439f4927a5c78c740`.
Permanent evidence is under:

```
evidence/a32_r797_shuffle_completion_q_20260814/
```

The structural boundary is now RTL-backed and task20 improves materially, but
complete vector microtiming alignment is still **not achieved**.  In isolated
task20 replay the first lifecycle divergence moves from sequence 647 to 8834;
in the concurrent CCH run task20 still begins with a one-cycle VLOAD phase
difference.  CCH task3/task9/task14 and SCH task3/task4/task8 remain independent
targets and must not be accepted through DAG-level cancellation.

## Two independent RTL oracles

The full-CCH oracle observes task20 sequence646 `SCATTER` feeding sequence647
`VSTORE`.  The final Shuffle bank grants precede the registered complete and
hazard/request boundaries:

- final bank grant: approximately 729,971 ns;
- Shuffle complete-D: approximately 729,973 ns;
- `global_hazard_table_d` clears and VSTU operand request rises: 729,975 ns;
- operand-valid/local W rises: 729,977 ns.

An independent tail18--23 RTL run reproduces the same four-edge sequence:

- final grant/done-D completion: oracle edge 15 at 296,247 ns;
- `parallel_shuffle_complete_d`: edge 17 at 296,249 ns;
- hazard-D clear and VSTU operand request: edge 19 at 296,251 ns;
- hazard-Q clear and operand-valid/W: edge 21 at 296,253 ns.

Before R797, `noteShuffleProducerGrant()` immediately advanced the sequencer-
local completion tombstones on the final bank-grant callback.  The dependent
VSTU therefore observed completion one tile edge too early.  This was hidden
for some stores by the AXI sampling aperture, but sequence647 crossed the
physical AXI edge and recycled four nanoseconds early.

## Generic structural fix

`VenusSequencer` now queues a tagged `PendingShuffleGrantCompletion` containing
the running ID, instruction generation, and registered visibility tick.  The
token becomes visible one tile cycle later, after same-edge LSU/LDU requester
callbacks have sampled the old Q state.  Visibility revalidates the exact
active generation before advancing both the general and LDU-private completion
tombstones; stale ID reuse is ignored.  Task reset clears the queue and the
queue participates in `isIdle()`.

The later ordinary Shuffle response, VINS retirement, broadcast hazard table,
and running-ID recycle boundaries are unchanged.

## Focused task20 lifecycle result

In the isolated task20 replay:

- sequence647 `VSTORE`: fire 17,423, duration 138, recycle 17,561 cycles,
  all exact to RTL;
- sequence648 `VLOAD`: fire 17,582, duration 49, recycle 17,631 cycles,
  all exact to RTL;
- the former sequence29/30 short-store negative control remains exact;
- sequences 0--8,833 have exact fire/duration/recycle lifecycles;
- the first remaining lifecycle divergence is sequence8834 `VBRDCST`, whose
  duration/recycle are 31 versus RTL 35 cycles (`-4 cycles`); no earlier fire
  divergence remains.

All 8,837 instructions complete.  The focused run exits at 398,520 ns.

## Complete CCH timing

The complete-DAG span is RTL 1,042,151 ns versus gem5 1,042,612 ns:
**+461 ns, +0.044%**.  This aggregate is not an acceptance metric.

| Task | RTL ns | gem5 ns | Delta ns | Error |
| ---: | ---: | ---: | ---: | ---: |
| 0 | 547 | 536 | -11 | -2.011% |
| 1 | 10,691 | 10,680 | -11 | -0.103% |
| 2 | 547 | 536 | -11 | -2.011% |
| 3 | 69,631 | 69,386 | -245 | -0.352% |
| 4 | 4,019 | 4,010 | -9 | -0.224% |
| 5 | 895 | 886 | -9 | -1.006% |
| 6 | 531 | 522 | -9 | -1.695% |
| 7 | 895 | 886 | -9 | -1.006% |
| 8 | 531 | 522 | -9 | -1.695% |
| 9 | 158,799 | 158,712 | -87 | -0.055% |
| 10 | 295 | 288 | -7 | -2.373% |
| 11 | 375 | 366 | -9 | -2.400% |
| 12 | 375 | 366 | -9 | -2.400% |
| 13 | 563 | 556 | -7 | -1.243% |
| 14 | 202,779 | 203,014 | +235 | +0.116% |
| 15 | 6,443 | 6,388 | -55 | -0.854% |
| 16 | 1,343 | 1,336 | -7 | -0.521% |
| 17 | 33,167 | 33,170 | +3 | +0.009% |
| 18 | 123,235 | 123,234 | -1 | -0.001% |
| 19 | 2,439 | 2,436 | -3 | -0.123% |
| 20 | 398,523 | 398,512 | -11 | -0.003% |
| 21 | 9,303 | 9,306 | +3 | +0.032% |
| 22 | 100,055 | 100,052 | -3 | -0.003% |
| 23 | 33,951 | 33,944 | -7 | -0.021% |

Relative to R785, task20 improves by 288 ns (`-299 -> -11 ns`).  Other
Shuffle-dependent tasks move independently: task3 `-16 ns`, task9 `-20 ns`,
task14 `+16 ns`, and task15 `+12 ns`.  These movements are retained because
the RTL oracle proves the shared registered boundary; they are not hidden by
the improved aggregate.

All 24 tasks and 53 returns complete.  All 10,253 VINS are task-local exact to
the accepted R770/R767 functional baseline.  In the concurrent context:

- task20 first differs at sequence0 VLOAD duration/recycle `-1 cycle`, with
  the first fire difference at sequence2 VSTORE `-1 cycle`;
- task17 first differs at sequence1 VLOAD duration/recycle `+1 cycle`, with
  sequence9 VSEQ the first fire difference at `+1 cycle`;
- task18 emits no VINS dump and its task boundary is `-1 ns`.

## Complete SCH held-out timing

The complete-DAG span is RTL 1,534,187 ns versus gem5 1,539,936 ns:
**+5,749 ns, +0.375%**.  The additional DAG tail is scheduler/return-path
phase and is not used to judge the local Shuffle fix.

| Task | RTL ns | gem5 ns | Delta ns | Error |
| ---: | ---: | ---: | ---: | ---: |
| 0 | 676,287 | 676,300 | +13 | +0.002% |
| 1 | 79,131 | 79,140 | +9 | +0.011% |
| 2 | 55,227 | 54,930 | -297 | -0.538% |
| 3 | 471,571 | 477,028 | +5,457 | +1.157% |
| 4 | 30,175 | 31,078 | +903 | +2.993% |
| 5 | 11,059 | 10,974 | -85 | -0.769% |
| 6 | 3,795 | 3,786 | -9 | -0.237% |
| 7 | 1,059 | 1,056 | -3 | -0.283% |
| 8 | 330,955 | 329,864 | -1,091 | -0.330% |

All nine tasks and 22 returns complete.  All 8,103 VINS are task-local exact
to the accepted baseline.  The retained RTL capture still directly covers
7,256 VINS; the other 847 are not claimed as a full RTL dump comparison.

The main independent SCH targets remain task3 CAU/SerDiv/LSU overlap, task4
SerDiv D/Q, and task8 Shuffle/LSU arbitration.  Their first lifecycle
differences are recorded under `sch_full/`.

## Regression gates and next boundaries

- build completes; `git diff --check` passes;
- LSU RAW/throughput/capacity: 68/68 processes pass;
- all 382 LSU-suite VINS files are byte-exact to the retained control;
- BitALU, CAU, and SerDiv each reach depth 2/full; 1/4/64 ns outputs are
  functionally exact;
- CCH/SCH functional streams remain 10,253/8,103 exact.

Next CCH work must keep the sequence647/648 closure while separating the
concurrent task20 sequence0 VLOAD phase from the isolated replay, then address
the final sequence8834 BitALU/VBRDCST result boundary.  Task17 remains an
independent LSU/VSEQ admission problem.  SCH continues with task3, task4, and
task8; no DAG-total or cross-task cancellation is accepted.
