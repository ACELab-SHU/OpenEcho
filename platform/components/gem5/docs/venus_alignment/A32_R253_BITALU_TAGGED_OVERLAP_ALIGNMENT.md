# A32 R253 BitALU tagged overlap / requester-bank alignment

Date: 2026-08-11

## Scope and conclusion

This round did not use total-DAG cancellation as an acceptance criterion.  It
started from the first task20 lane-0 BitAlu_B operand-queue mismatch and
compared every 2 ns lane edge against a fresh RTL VCD.  The accepted model:

1. accounts BitALU result occupancy with RTL-style registered `q/d` state, so
   an enqueue and grant on the same edge are one net transition;
2. re-evaluates a result-full-stalled operand admission after a same-edge VRF
   result grant, matching the post-edge combinational ready path;
3. retains one registered result visibility boundary;
4. allows a younger BitALU instruction with the same arithmetic pipe length to
   overlap older tagged results, while a real pipe-length transition still
   drains first; and
5. keeps the existing stable tagged requester owner, independent LSU priority,
   and persistent per-bank RTL-tree RR model.

No task ID, instruction PC, DAG name, address, or payload special case was
introduced.  LSU fixed latency was not changed.

## RTL oracle

The RTL source shows that ordinary BitALU operand acceptance is gated by the
registered `result_queue_cnt_q == 2` full flag.  A successful operand handshake
sets `bitalu_operand_ready_o` and increments `result_queue_cnt_d`; a result
grant decrements the same `result_queue_cnt_d`.  The instruction issue pointer
and result commit pointer are independent.

Relevant RTL source:

- `venus_soc/hardware/venus_extension/venus_bitalu_wrapper.sv`, especially
  lines 388--426 and 696--729;
- `venus_soc/hardware/venus_extension/venus_bitalu.sv`, whose ordinary BitALU
  datapath is combinational.

Fresh waveform evidence is `/tmp/a32_task20_bitalu_queue.vcd`.  The extended
`tools/parse_rtl_vfu_vcd.py --vfu BitALU` parser extracts operand ready,
result queue q/d/valid, result request/grant, and issue count from that VCD.

## Rejected intermediate hypotheses

- R243/R245 same-edge pop and requester-intent cancellation changed the wrong
  boundary and were removed.
- R248 counted a hidden arithmetic-pipeline reservation as an RTL result queue
  slot.  It moved the first operand event mismatch from relative cycle 18 to
  17 and the first bank mismatch from 24 to 21, so it was removed.
- R249 incremented a shadow count at the registered pop boundary but waited a
  full extra edge after a result grant.  RTL required a pop at relative cycle
  17; R249 moved it to 18, so it was removed.
- R251 removed the result visibility boundary entirely.  It made BitALUResult
  request a bank at relative cycle 14 when RTL had no such request and also
  exposed the missing same-edge q/d accounting.  The zero-pipe change was
  removed.
- R252 combined zero-pipe with q/d accounting.  Operand events improved, but
  the early BitALUResult bank request remained; it was not accepted.

## Accepted local result

The final task20 anchor is gem5 tick `556220000`, aligned to RTL queue cycle
68.  Against `/tmp/a32_task20_operand_queue.vcd`:

- BitAlu_B relative cycles 0--60: zero event mismatches;
- BitAlu_B relative cycles 0--60: zero registered-usage mismatches;
- the first per-bank request/grant mismatch moved from R247 cycle 24 and R250
  cycle 30 to cycle 89;
- the new cycle-89 mismatch is a Shuffle request, not BitALU, CAU, or LSU.

This is a component-level improvement even though task20's complete duration
moves farther below RTL.  The next task20 breakpoint is therefore Shuffle
request admission/bank selection from relative cycle 89, not another BitALU
latency adjustment.

## Functional and directed regression

Final binary SHA-256:

`d40d117e03104232d43361cc1ff897cf3bee990cbd934543a867c0ca94b82971`

- LSU RAW/throughput/capacity:
  `/tmp/a32_r253_final_lsu_suite/suite_results.json`, 68/68 pass; all 382
  VINS files are byte-exact to the R241 accepted baseline.
- VFU result queues:
  `/tmp/a32_r253_bitalu_tagged_overlap_capacity_logged/capacity_results.json`;
  1/4/16/32/64/128 ns functional outputs are exact and BitALU/CAU/SerDiv all
  reach depth 2 and an observed full condition.
- CCH: `/tmp/a32_r253_final_cch`, 10,253 VINS exact to R237.
- SCH: `/tmp/a32_r253_final_sch`, 8,103 VINS exact to R237.

## Per-task execution timing

The comparison boundary remains RTL `start execute -> execute complete` versus
gem5 `tile_start -> task_epilogue`; return DMA/tile release is excluded.

### CCH

| task | delta ns | error |
|---:|---:|---:|
| 0 | -11 | -2.011% |
| 1 | +199 | +1.861% |
| 2 | -11 | -2.011% |
| 3 | +153 | +0.220% |
| 4 | -9 | -0.224% |
| 5 | -9 | -1.006% |
| 6 | -9 | -1.695% |
| 7 | -9 | -1.006% |
| 8 | -9 | -1.695% |
| 9 | +121 | +0.076% |
| 10 | -7 | -2.373% |
| 11 | -9 | -2.400% |
| 12 | -9 | -2.400% |
| 13 | +49 | +8.703% |
| 14 | +1,137 | +0.561% |
| 15 | +31 | +0.481% |
| 16 | +15 | +1.117% |
| 17 | +491 | +1.480% |
| 18 | +3,465 | +2.812% |
| 19 | +49 | +2.009% |
| 20 | -6,847 | -1.718% |
| 21 | +131 | +1.408% |
| 22 | +5 | +0.005% |
| 23 | -7 | -0.021% |

Full JSON: `/tmp/a32_r253_final_cch/task_execution_timing_vs_rtl.json`.

### SCH

| task | delta ns | error |
|---:|---:|---:|
| 0 | +17 | +0.003% |
| 1 | +13 | +0.016% |
| 2 | +1,717 | +3.109% |
| 3 | +6,933 | +1.470% |
| 4 | +1,023 | +3.390% |
| 5 | +125 | +1.130% |
| 6 | -11 | -0.290% |
| 7 | -3 | -0.283% |
| 8 | +985 | +0.298% |

Full JSON: `/tmp/a32_r253_final_sch/task_execution_timing_vs_rtl.json`.
SCH timing is unchanged from R237, which is expected because the exercised
BitALU overlap window is CCH-specific.

## Remaining breakpoints

- task20: first bank-vector divergence is now Shuffle at relative cycle 89;
  the task duration remains `-6.847 us`, so task-local completion is not
  aligned.
- task17: `+0.491 us`; the earlier sequence-6 VSEQ duration breakpoint still
  requires its own refreshed RTL/gem5 admission/enqueue/retire trace.
- task18: `+3.465 us`; unchanged and not explained by this BitALU window.
- SCH tasks 2/3/4 remain the largest held-out execution gaps.

These must be handled independently.  The CCH total or task17/task20
cancellation is not an acceptance criterion.
