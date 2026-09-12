# Hardware selector validation, server17, 2026-09-08

This is software/configuration validation in shenyihao's isolated
ACE-ECHO-onboarding-a001 worktree, not a new RTL qualification or clean-user
deployment claim. Original research checkout and RTL were not modified.

- 230 platform tests and 4 Hub tests passed.
- Venus1 300 MHz and Venus2 16x128: the same three frozen two-task cases were
  freshly compiled and executed on Gem5 fast after changing only the project
  hardware field. All six outputs (384 bytes) matched the provisional reference
  for each hardware.
- V1 DAG allocation-to-completion: 560028 ticks / 3333 ticks per tile cycle.
- V2 DAG allocation-to-completion: 328000 ticks / 2000 ticks per tile cycle.
- V2 active selection rejected V1 JSON/BIN handoffs before starting a simulator.
- Selected V2 Scheduler produced l1.elf/l1.bin; RTL command planning was tested
  without starting RTL. The selected V2 read-only RTL preflight passed.
- Forge @project resolved to V2 and initialized READY with REVIEW_REQUIRED.
  No provisional golden was promoted.
- Both hardware smoke runs used the same shared Venus LLVM installation.
  Independent alternate compiler selection and changed compiler invalidation
  are covered by host-side tests, not an alternate-compiler numerical claim.

Evidence under the isolated worktree:

- runs/venus1.0/hardware-switch-smoke-a001/report.json
- runs/venus2.0/hardware-switch-smoke-a001/report.json
- runs/venus2.0/hardware-switch-scheduler-a001/run.json
- runs/venus2.0/hardware-switch-rtl-plan-a001/run.json (PLANNED only)
- runs/venus2.0/hardware-switch-forge-a001/forge-run.json
- .ace-echo/hardware-selection-tests-a002.log
- .ace-echo/wrong-hardware-rejected.log
- .ace-echo/rtl-preflight-selected-v2.json

No Gem5/DSL/Scheduler core/RTL semantic changes, remote push, main merge,
automatic LLM calls or artifact cleanup were part of this change.
