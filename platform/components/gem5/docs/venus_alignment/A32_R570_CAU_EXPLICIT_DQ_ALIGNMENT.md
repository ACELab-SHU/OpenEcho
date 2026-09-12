# A32 R570 CAU explicit D/Q alignment

Date: 2026-08-13

Status: accepted as the next alignment baseline. Functional VINS remain exact, but
micro-timing is not complete and the total DAG error is not an acceptance metric.

## Scope

This iteration models the CAU path as registered, tagged transactions instead of
folding admission, calculation and writer visibility into one timing shortcut:

- ordinary CAU A--D VRF responses carry the owning instruction/generation tag;
- at most two CAU tagged reads may be outstanding;
- operand completion starts an explicit arithmetic pipeline D stage;
- completed values enter a separate depth-2 result queue and become visible at Q;
- wakeup/requester visibility follows the registered Q boundary;
- `locallane_cau_calc_cnt` advances before `getCAUResult()`, matching the RTL
  calculation index. This ordering is required for `VRANGE`; computing first
  produced a wrong first element (`65475`) and was rejected.

No LSU/VFU fixed latency, task ID, PC, DAG, address or data special case was
introduced.

Configuration:

```text
VENUS_GEM5_EXPERIMENTAL_VRF_RR=1
VENUS_GEM5_EXPERIMENTAL_REQUESTER_Q_VISIBILITY=1
VENUS_GEM5_EXPERIMENTAL_SHUFFLE_LIVE_INTENT=1
VENUS_GEM5_ENABLE_OPERAND_HAZARDS=1
```

Rebuilt binary SHA256:

```text
3275a609895bdf82e4f2977ca734efabdad68cac60c7ceb52c0d843392222275
```

## Directed results

### CCH task17

- 687/687 VINS are exact against the accepted RTL-exact baseline.
- Task execution is `+5 ns / +0.0151%`.
- First duration/recycle divergence is sequence 180 `VBRDCST`, `-1 cycle`.
- First fire divergence is sequence 194 `VLOAD`, `+1 cycle`.

### CCH task20

- 8,837/8,837 VINS are exact against the accepted RTL-exact baseline.
- Sequences 25--49 now have exact fire, duration and recycle timing. This includes
  the previous sequence 33/34/36/37 CAU/requester divergence window.
- The first divergence moved to sequence 50 `VBRDCST`:

| Dimension | gem5 | RTL | Delta |
|---|---:|---:|---:|
| fire cycle | 4487 | 4564 | -77 |
| duration | 135 | 57 | +78 |
| recycle cycle | 4622 | 4621 | +1 |

The opposite fire and duration deltas mostly cancel at retirement. They must not
be interpreted as accurate modeling.

## Full-chain results

The execution boundary is `tile_start -> task_epilogue` in gem5 and
`start execute -> execute complete` in RTL. Return DMA and tile release are
excluded.

### nrPDCCH / CCH

The functional DAG span is `1,039,156 ns`, versus RTL `1,043,224 ns`:
`-4,068 ns / -0.389945%`. All 24 tasks completed and all 10,253 VINS are exact
against the accepted RTL-exact baseline.

| Task | Delta (ns) | Error |
|---:|---:|---:|
| 0 | -11 | -2.011% |
| 1 | -11 | -0.103% |
| 2 | -11 | -2.011% |
| 3 | -239 | -0.343% |
| 4 | -9 | -0.224% |
| 5 | -9 | -1.006% |
| 6 | -9 | -1.695% |
| 7 | -9 | -1.006% |
| 8 | -9 | -1.695% |
| 9 | -17 | -0.011% |
| 10 | -7 | -2.373% |
| 11 | -9 | -2.400% |
| 12 | -9 | -2.400% |
| 13 | +49 | +8.703% |
| 14 | +215 | +0.106% |
| 15 | -79 | -1.226% |
| 16 | -7 | -0.521% |
| 17 | +5 | +0.015% |
| 18 | -1 | -0.001% |
| 19 | -27 | -1.107% |
| 20 | -3,471 | -0.871% |
| 21 | +3 | +0.032% |
| 22 | -3 | -0.003% |
| 23 | -7 | -0.021% |

Task20 is the dominant independent CCH residual. Task13 has the largest relative
percentage, but only `49 ns` absolute error; tasks 0/2/10/11/12 similarly expose
a systematic short-task boundary error of roughly 7--11 ns rather than a major
compute-unit deficit.

Instruction-level probes:

- task3 first duration/recycle divergence: sequence 0 `VLOAD`, `+8 cycles`;
  first fire divergence: sequence 5 `VLOAD`, `+9 cycles`;
- task14 first duration/recycle divergence: sequence 0 `VLOAD`, `+1 cycle`;
  first fire divergence: sequence 1, `+1 cycle`.

### PDSCHDag2 / SCH

The functional DAG span is `1,539,892 ns`, versus RTL `1,535,100 ns`:
`+4,792 ns / +0.312162%`. All 9 tasks completed and all 8,103 retained VINS are
exact against the accepted RTL-exact baseline.

| Task | Delta (ns) | Error |
|---:|---:|---:|
| 0 | +13 | +0.002% |
| 1 | +9 | +0.011% |
| 2 | -297 | -0.538% |
| 3 | +5,485 | +1.163% |
| 4 | +843 | +2.794% |
| 5 | -95 | -0.859% |
| 6 | -9 | -0.237% |
| 7 | -3 | -0.283% |
| 8 | -1,091 | -0.330% |

The total includes cancellation between task3 and task8 and is not an alignment
pass. Independent priorities are task3, task8, task4 and task2 in that order by
absolute time.

Instruction-level probes:

- task2: first duration/recycle divergence is sequence 45 `VRANGE`, `+3 cycles`;
  first fire divergence is sequence 49 `VXOR`, `+8 cycles`;
- task3: sequence 0 `VLOAD` is only `-1 cycle` in duration/recycle and sequence 1
  is only `+1 cycle` at fire, so its `+5.485 us` residual accumulates later;
- task4: sequence 0 `VLOAD` is `-1 cycle`; first fire divergence is sequence 8
  `VDIV`, `-17 cycles`, pointing at SerDiv admission/result handling;
- task8: first fire/recycle divergence is sequence 2 `SCATTER/VSHUFFLE`,
  `-2 cycles`; sequence 3 `VSTORE` duration is `-1 cycle`, pointing at repeated
  shuffle/LSU arbitration accumulation.

## Rejected experiment

A per-lane completion board plus an RTL-like final lane-desynchronization stall was
implemented and run on task20. It produced 8,837 exact VINS but did not change a
single timing result or the exit tick, so it was completely reverted.

The sequence-50 trace explains why: when sequence 50 was allocated, gem5 had
`vfu_queue_counter[VFU_BitALU] = 3`, assigned running ID 7, and never asserted the
new desynchronization predicate. RTL waits and later reuses freed ID 1. Therefore
the missing cause precedes the final stall gate: it is in the per-lane ready/
completion vector, lane fall-through handshake, or PE-ready return path.

## Remaining model gaps and next probes

1. **CCH task20 sequence 43--51**: compare per-lane operand-ready/completion bits,
   lane fall-through valid/ready, `pe_req_ready_bus_d`, active running-ID vector,
   stable tagged requester vector and per-bank RR state. Do not add another global
   stall or fixed latency.
2. **SCH task4 sequence 8 VDIV**: split SerDiv operand admission, pipeline D,
   depth-2 result queue D/Q, writer request, per-bank grants and retirement.
3. **SCH task3**: locate phase boundaries and the first sustained slope change;
   its entry timing is already nearly exact, so sequence 0 is not the cause.
4. **SCH task8**: inspect persistent Shuffle/LSU contention, tagged requester
   stability, LSU high priority and per-bank RR across repeated operations.
5. **Short CCH tasks**: after the large residuals, isolate the common 7--11 ns
   CPU/task-boundary discrepancy with directed scalar-only cases.

An updated RTL probe for task20 exists, but a fresh RTL capture is currently
dependent on simulator/VIP license availability. Existing captured RTL timing is
still sufficient to continue the gem5-side phase decomposition.

## Evidence

```text
evidence/a32_r570_cau_explicit_dq_20260813/
  task17/
  task20/
  cch_full/
  sch_full/
  lsu_suite/
  diagnostics/
```

The LSU directed suite remains 68/68 passing with 382 VINS exact. This result and
the full-chain VINS comparisons guard against hiding timing improvements behind
functional changes.
