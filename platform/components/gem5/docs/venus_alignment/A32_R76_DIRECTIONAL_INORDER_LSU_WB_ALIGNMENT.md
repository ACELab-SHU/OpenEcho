# A32 R76 directional in-order LSU / WB alignment

Date: 2026-08-10

Verdict: `INCONCLUSIVE`

## Scope and model change

This pass compared CCH task18 and task20 at scalar retirement boundaries and
read the protected RTL `scalar600_core`, `scalar600_lsu`, controller, and WB
stage without modifying RTL, VEMU, or scheduler sources.  The gem5 RTL scalar
profile now models four general pipeline properties:

1. an already-ready memory operation may enter the LSU on the same edge that
   its immediately older in-order head retires;
2. this path is not speculative early issue: the operation must be both the
   new in-flight head and the oldest memory-FU entry;
3. load completion remains response-driven, with a six-cycle WB distance
   after fall-through/not-taken control and an eight-cycle distance after a
   taken redirect;
4. scalar stores retain the RTL-observed control-to-store and load-to-store
   WB distances (four and three cycles), and a true load RAW consumer observes
   the registered two-cycle WB/RF boundary.

No task ID, PC, DAG, address, or operand-value test was added.  LSU and VFU
fixed response latencies were not changed.  Global
`executeAllowEarlyMemoryIssue` remains disabled.

## Paired task18/task20 evidence

Before the change, task18's small total error hid thousands of opposing
cycles.  The pre-direction diagnostic showed the following boundaries after
the in-order LSU/store/RAW changes:

- the 2,048-instance task18 `branch -> store` loop changed from gem5 2 cycles
  to 4 cycles, matching RTL 4;
- 1,483 task18 `load -> store` edges changed to 3 cycles, matching RTL 3;
- task18 aggregate `load -> ALU` cycles became exact;
- task20's 164 `branch -> load` edges changed from gem5 8 cycles to 6 cycles,
  matching RTL 6.

The remaining direction discriminator was real: task18's backward taken
`0x638 -> 0x618` load edge is approximately 8 cycles in RTL, whereas
fall-through `0x620 -> 0x624` and task20 `0x26c -> 0x270` are 6.  R76 records
the actual ID-resolved taken state and applies the corresponding registered
boundary.

The scalar part is still incomplete.  Task20's 164 dependent
`load -> branch` edges remain gem5 4 cycles versus RTL 2, a +328-cycle
residual.  Its larger negative timing error is dominated by VSEQ/VFU
transition phase: the pre-direction full scalar alignment still showed large
opposing `venus -> venus`, `venus -> ALU`, and `ALU -> venus` terms.  Those
must be resolved through requester/admission/retirement state, not another
CPU or VFU fixed latency.

## Functional gates

- CCH: 10,253 task-local VINS exact to the accepted R59/R55 gem5 baseline.
- SCH: 8,103 task-local VINS exact to the accepted baseline.  The retained RTL
  coverage remains 7,256 instructions; the other 847 still have no RTL dump.
- LSU RAW/throughput/capacity suite: 68/68 pass.
- Build SHA256:
  `41469f72c13a8b3814f259cb413f0c805527f2cd36282cdf1f4c9788fde1f699`.

## Timing result

CCH allocation-to-last-release is RTL 1,043,224 ns versus gem5 1,045,708 ns:
+2,484 ns (+0.238%).  SCH is RTL 1,535,100 ns versus gem5 1,548,560 ns:
+13,460 ns (+0.877%).  Neither aggregate is an acceptance metric.

Important CCH residuals are task9 +10,185 ns, task18 +3,089 ns, task20
-13,035 ns, task22 +4,013 ns, and task23 +4,481 ns.  Important SCH residuals
are task0 +11,761 ns, task2 +4,529 ns, task3 +2,549 ns, and task4 +1,633 ns.
The complete task tables are stored in the evidence paths below.

## Evidence

- `evidence/a32_r76_directional_inorder_lsu_20260810/cch/task_timing_vs_rtl.json`
- `evidence/a32_r76_directional_inorder_lsu_20260810/sch/task_timing_vs_rtl.json`
- `evidence/a32_r76_directional_inorder_lsu_20260810/cch/vins_vs_r59.json`
- `evidence/a32_r76_directional_inorder_lsu_20260810/sch/vins_vs_r59.json`
- `evidence/a32_r76_directional_inorder_lsu_20260810/directed/lsu_suite_results.json`
- `evidence/a32_r76_directional_inorder_lsu_20260810/cch/pre_direction_task18_scalar_boundary_evidence.txt`
- `evidence/a32_r76_directional_inorder_lsu_20260810/cch/pre_direction_task20_scalar_boundary_evidence.txt`

## Next first divergences

1. Model ID-resolved dependent control admission so task20 `load -> branch`
   reaches the RTL two-cycle boundary without moving the bubble to the branch
   follower.
2. Resume task20's stable tagged requester vector, LSU independent priority,
   and persistent per-bank RR work on the first divergent VSEQ/VFU transition.
3. Analyze task9 and SCH task0 independently; their positive residuals must
   not be used to cancel task20.

