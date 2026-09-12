# A32 R81 scalar600 CPU conformance matrix

Date: 2026-08-10

Status: **scalar core directed matrix exact; CPU--Venus handshake remains
open, so complete CPU alignment is not claimed**.

## Accepted structural changes

- Route scalar instruction/data accesses through a 500 MHz tile-side memory
  path, with non-local accesses falling back to the 250 MHz system XBar.
- Keep two Fetch1 requests available so an ID redirect can replace the
  registered PC while the discarded sequential SRAM response returns.
- Model the integer EX result as a one-cycle operation and preserve the
  scalar600 single-issue/single-retire contract.
- Forward values through the explicit scalar600 EX writer chain when Decode
  resolves JALR and conditional controls.
- Preserve the registered LSU store completion boundary.
- Require every scalar MUL retirement to be at least three tile cycles after
  the preceding retirement, matching `scalar600_mul.complete` for continuous
  and interposed-ALU request patterns.
- Model the RTL divider's per-thread count/result state, including zero-divisor
  history, stale-result writeback, and DIV-final EX-to-ID forwarding.
- Preserve taken redirect, load-to-MUL, branch-to-store, Venus 64-bit word
  occupancy, and registered barrier-release-to-WB boundaries.
- Keep scalar ISPM/DSPM accesses on the 500 MHz tile-local path.  When the
  scalar and L1-DMA paths collide on a response, retain only the rejected
  response in the existing retry queue; successful responses remain same-edge.

None of these changes matches a task, PC, DAG, address, operand value, or
workload name.

## Exact directed cases

The following cases pass ordered PC/class, architectural writeback, and every
relative scalar retirement cycle against RTL:

1. ALU/control: `alu_independent`, `alu_raw`, `branch_forward`,
   `branch_loop`, `jal_jalr`, `branch_transitions`.
2. MUL/DIV: `mul`, `mul_transitions`, `divrem`,
   `div_zero_transitions`, all seven `div0/rem0` gap/history cases, and the
   dedicated `div_alu_forward` case.
3. LSU/interactions: corrected `lsu_widths`, corrected `load_use_control`,
   `load_mul_forward`, and `branch_store_transitions`.
4. Architectural state: `csr_mhartid`, `lrsc_success`, `lrsc_invalidate`, and
   `mixed_pipeline`.

The effective matrix is 24/24 exact.  The obsolete R79
`load_use_control` fixture is intentionally excluded because its RTL and gem5
instruction streams differ; the corrected replacement is 25 retire events
exact.  Dedicated DIV-to-ALU, load-to-MUL, and branch-to-store cases add
21/22/21 exact retire events respectively.

## Remaining CPU boundary

The divider blocker described by R81 is closed.  Complete CPU alignment is
still not claimed because full-DAG Venus admission exposes a separate
CPU--Sequencer boundary:

- CCH task17 has identical scalar PC/class order for all 3,140 common retire
  events after discarded/squashed Minor completions are excluded.
- Its first timing difference is retire index 351, at the VSEQ encoded at
  PC `0x59c`: RTL relative cycle 1111, gem5 1113.
- The final scalar-retire drift is 10,246 tile cycles.  Venus-to-Venus gaps
  contribute +9,561 cycles; ALU-to-ALU, ALU/load/store, MUL, and ordinary
  control gap classes are exact.  This is Sequencer/VFU/VRF backpressure
  exposed at the scalar interface, not a generic scalar pipeline bubble.
- The comparison oracle now excludes a `Completed inst` record when the same
  tick/thread/PC was first logged as `Discarding inst`; otherwise taken-branch
  squashes were falsely counted as retirement.

Therefore the next alignment target is the stable tagged Venus requester and
its completion/VRF grant timing, not another scalar fixed-latency adjustment.

## Evidence

R120 revalidation after the CAU completion-boundary and sequencer
WAITING_FOR_READY corrections:

- effective scalar matrix: 24/24 exact (five runners report 25/25 before
  excluding the obsolete fixture);
- LSU regression: 68/68;
- CCH/SCH functional regression: 10,253/8,103 task-local VINS exact;
- task17 sequences 0--21 now have exact fire/recycle/duration timing; the
  first remaining fire difference is sequence 22 (+4 cycles) and the first
  remaining recycle difference is sequence 35 (+1 cycle);
- binary SHA-256:
  `27cdb64e4b554dfa8cbe9062e451a44960bc1e2062896408c7cb00155e774014`.

R120 evidence is under `/tmp/a32_r117_waiting_same_tick_cch`,
`/tmp/a32_r120_waiting_same_tick_sch`, the five
`/tmp/a32_cpu_*_r120` directories, and
`/tmp/a32_r120_waiting_same_tick_lsu_suite`.

Post-R120 lane-command boundary audit (2026-08-10):

- changing the modeled command-retire visibility from two cycles to one kept
  task17 sequences 0--21 exact and reduced sequence-22 fire from +4 to +2
  cycles, but exposed recycle +1 at sequence 34 and worsened task20 to
  -16,173 ns; evidence is `/tmp/a32_r121_cmd_release1_cch`;
- zero-cycle visibility made sequences 22--23 exact only by moving the first
  fire divergence to sequence 24 (+9 cycles), and worsened task20 to
  -17,293 ns; evidence is `/tmp/a32_r122_cmd_release0_cch`;
- both candidates preserved all 10,253 task-local VINS byte-exactly, proving
  again that functional equality alone cannot validate the timing boundary;
- both candidates were rejected and the two-cycle R120 source was restored.
  The restored rebuild has SHA-256
  `51146dd1b41b2e5a050e2223cb3bdfc5f3f5daad1dc00ab860fb0a0a9d5ad5b5`
  and re-passes all five scalar runners (25/25 reported, 24/24 effective) in
  `/tmp/a32_cpu_{suite,transition,lsu,div,extended}_restored_r120`.

R123--R125 RTL-waveform continuation (2026-08-10):

- A fresh RTL UCLI sample covers task17 sequence 21--24 at 1 ns resolution:
  `/tmp/a32_task17_seq21_24_edges.log`.  Lane 0--3 have identical input-FIFO,
  command-valid, requester-ready, and issued vectors throughout the target
  window even though lane 3 has seven local rows and lanes 0--2 have eight.
- Replacing command timing with the functional requester FIFOs was rejected:
  it moved the first task17 fire divergence backwards from sequence 22 to
  sequence 10 and worsened task3 from -1,209 ns to -2,979 ns.  Evidence:
  `/tmp/a32_r123_explicit_cau_cmd_cch`.
- Aligning an idle CAU requester's structural service window to the maximum
  participating-lane row count removed the false lane de-synchronization.
  With two-cycle command visibility, sequence 0--21 remained exact and the
  sequence-22 fire/duration errors reduced from +4/-4 to +2/-2 cycles without
  changing any task endpoint.  Evidence: `/tmp/a32_r124_aligned_idle_cau_cch`.
- The RTL requester-ready waveform shows command retirement becoming visible
  on the next tile edge.  Combining one-cycle visibility with aligned service
  pushes the first task17 fire/duration divergence to sequence 25 VMUL
  (+4/-4 cycles); sequence 0--24 is exact and the first recycle divergence is
  sequence 35 (+1 cycle).  This R125 candidate is retained for the next
  micro-timing iteration.
- R125 regressions pass: scalar runners 25/25 reported (24/24 effective), LSU
  68/68, CCH/SCH task-local VINS 10,253/8,103 exact.  SCH task timing is
  unchanged from R120.  Binary SHA-256 is
  `7c08c9327939df4c50d19f9e4009c078d45ce650a036d4ee4753642b4afb9838`.
- R125 evidence: `/tmp/a32_r125_aligned_cau_release1_cch`,
  `/tmp/a32_r125_aligned_cau_release1_sch`,
  `/tmp/a32_r125_aligned_cau_release1_lsu_suite`, and the five
  `/tmp/a32_cpu_*_r125` directories.

R125 does not establish complete CPU-to-Venus alignment.  CCH task17 remains
+20,449 ns, while task20 becomes -16,173 ns because its stable requester
vector, LSU priority, and persistent per-bank RR model is still absent.  The
next task17 boundary is sequence 24 VSEQ to sequence 25 VMUL; task20 must be
fixed independently rather than used as endpoint cancellation.

R126 CAU requester handoff continuation (2026-08-10):

- RTL `venus_operand_requester.sv` can accept the next operand command in the
  same cycle in which the final row grant makes the current requester length
  zero.  The downstream CAU arithmetic/result queues are not part of that
  command service window.  Removing their duplicated four-cycle charge while
  retaining the registered one-cycle handoff pushes task17's exact prefix from
  sequence 0--24 to sequence 0--95 (fire, duration, and recycle all exact).
- The first remaining task17 difference is sequence 96 VSTORE: fire is exact,
  but gem5 recycles in 42 cycles versus RTL's 52 cycles.  The first fire
  difference is consequently sequence 97 VLOAD, 11 cycles early.  This is now
  being split at the STU operand admission, per-bank VRF grant, AW/W/B, and
  retirement boundaries; no fixed ten-cycle LSU adjustment is justified.
- R126 preserves 10,253/8,103 task-local CCH/SCH VINS byte-exactly.  SCH task
  timing is unchanged.  LSU remains 68/68, and all five scalar runners report
  25/25 (24/24 effective matrix).
- CCH task17 improves to +20,329 ns, while task20 moves independently to
  -17,037 ns.  These endpoints are not evidence of alignment and must not be
  offset against one another.
- Binary SHA-256:
  `04421048446c81bd96b6b3da4031052eb474e1cd6a55c84faa44c193b504a6d8`.
  Evidence is under `/tmp/a32_r126_cau_handoff1_{cch,sch,lsu_suite}`,
  `/tmp/a32_cpu_*_r126`, and `/tmp/a32_r126_task17_seq96_trace`.

- Fresh scalar suites: `/tmp/a32_cpu_{suite,transition,lsu,div,extended}_r102`
- Dedicated cases: `/tmp/a32_cpu_post_r102`
- LSU regression: `/tmp/a32_r102_cpu_complete_lsu_suite/suite_results.json`
  (`68/68`).
- CCH timing/VINS: `/tmp/a32_r102_cpu_complete/cch/`
  (`10,253` VINS exact against the accepted baseline).
- SCH timing/VINS: `/tmp/a32_r102_cpu_complete/sch/`
  (`8,103` VINS exact against the accepted baseline).
- Final binary SHA-256:
  `791a56dbae86fd02b572a35ee95657a142728bde494d9e6b06be117e569b3a4d`.
- Protected-tree audit: `checked=218315 changed=0 missing=0` outside the
  explicitly allowed CPU microbench run directories.
