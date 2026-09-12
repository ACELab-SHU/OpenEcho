# ACE-ECHO Forge Agent

Use `.agents/skills/ace-echo-venus-forge/SKILL.md` for every request to port,
generate, optimize, integrate, or RTL-qualify a Venus task, DAG, or application.
It is the controlling workflow; this file only fixes repository-wide safety
and ownership boundaries.

## Active hardware and personal paths

When .ace-echo/project.toml exists, begin with ./ace-echo hardware show and read
the resolved backend and its knowledge references. The hardware/toolchain
selection applies to compilation, Gem5, Scheduler, RTL and Forge @project
requests. Use .ace-echo/host-paths.json for personal compiler/RTL locations;
do not put account-specific paths into the shared hardware catalog. Do not
override the active hardware with a stale --backend or reuse another version's
compiled artifacts. See docs/HARDWARE_SELECTION.md. This setup does not change
the golden, correctness or RTL qualification requirements below.

## Scope

Accept user intent, a numerical specification, executable foreign-language
source, existing Venus source, a task, a DAG, or a complete application. Use
the selected `configs/backends/*.json` manifest for every architecture fact.
Do not treat Venus2.0 rows, lanes, banks, memory limits, paths, or commands as
universal constants.

## Absolute rules

- Never invoke, build, import, repair, or depend on VEMU.
- Establish exactly one reference authority. Agent-created references remain
  provisional until a human approves their digest.
- Freeze fixed-point, ABI, shape, state, and complete-output semantics before
  Venus generation. Compare integer outputs bit-for-bit.
- Use independent compile, Gem5 fast, golden comparison over the complete
  declared matrix, integrated revalidation, and fresh RTL in that order.
  Follow the backend's full_software_engine (shipped profiles use gem5.fast).
  Use gem5.verification/debug for failures or divergence diagnosis, not as a
  mandatory extra normal-path run; never waive correctness or RTL gates.
- Treat RTL source as immutable. Do not patch, format, replace, or delete it.
- Rank only correct candidates with identical comparability keys. Host wall
  time is not a rankable accelerator metric.
- Let AI propose creative performance transformations, but change one
  falsifiable hypothesis per candidate and pass every deterministic gate.
- Write run-local/proposed knowledge automatically. Require reproducible
  replay and human approval before changing live knowledge or Gem5 semantics.

## Repository boundaries

- `workloads/` is the pinned Echo Hub source snapshot and owns application, task,
  DAG, golden, and application-test sources.
- `components/toolchain/dsl/` is the pinned DSL source snapshot and owns DSL/compiler
  frontend fixes and probes.
- ACE-ECHO owns orchestration, backend manifests, adapters, static checks,
  evidence schemas, and formal knowledge.
- During an application run, shared Scheduler, DSL/compiler, Gem5 semantics,
  backend-contract semantics, existing shared launchers, and orchestration
  gates are protected. A workload failure does not authorize changing them.
  Preserve a minimal reproducer and require a separate reviewed infrastructure
  change; then rerun the application from a clean attempt.
- Workload-owned task/DAG/case/golden files and a new workload-local launcher
  are the default mutation surface. RTL remains immutable without exception.
- Work on local feature branches and reversible commits. Do not push, merge,
  tag, publish, or promote live knowledge without explicit human approval.
- Preserve unrelated dirty work in all repositories.

## Output verdicts: valid data versus alignment padding

- Keep explicit output ABI declarations in the workload's `output-validity.json`.
  Compilation binds them to the emitted DAG/BIN and RTL comparison loads the
  verified bundle automatically. See `docs/OUTPUT_VALIDITY.md`.
- Known, equal valid bits with differences/unknowns only in declared padding
  are a functional PASS with padding WARNINGs, not a functional FAIL. The
  separate raw `transport_bit_exact_status` must remain truthful and must not
  be mistaken for the functional verdict.
- Unknown/mismatching valid bits, missing returns or bytes, wrong lengths,
  stale/invalid contracts and allocation overflow still fail or block the
  relevant gate. Never infer padding from X locations or output differences.
- Declaration-free legacy outputs remain strict. Rejudging retained evidence
  requires a new report with the contract identity; preserve the original
  report and do not claim that rejudgment was a new hardware simulation.

## Retention

Keep all evidence while an application is active, failed, blocked, or under
review. After an application-complete PASS, retain baseline, winner, all
RTL-qualified candidates, the top three correct alternatives, final evidence,
minimal reproducers, and compact metadata for every attempt. Remove only the
heavy candidate entries allowlisted by Forge cleanup.

Treat storage as a measured run resource rather than an afterthought:

- Record the output root and allocated bytes before and after every compile,
  Gem5, Scheduler, and RTL stage. Keep generated data inside the run-local
  output tree so it can be attributed and cleaned without broad path matches.
- Classify generated artifacts as `handoff`, `canonical_reproducer`,
  `candidate_evidence`, `compact_metadata`, or `reproducible_heavy`. A file is
  unused only when its producer and consumer stages are closed, no retained
  handoff or reproducer references it, and its exact recipe, input digests,
  command, outcome, and failure fingerprint remain available.
- At every safe stage boundary, review storage growth. After an
  application-complete PASS, or an explicit user-authorized cleanup point,
  delete closed non-winning `reproducible_heavy` entries such as Gem5 replay
  workdirs, duplicate dumps, disposable shadow/build trees, and superseded
  intermediate images. Keep the winning build, exact firmware handoff,
  canonical replay, RTL-qualified candidates, top three correct alternatives,
  minimal negative reproducers, and compact summaries.
- Before deletion, freeze an exact-path cleanup manifest with sizes and hashes
  of retained evidence. Never delete outputs of a live process, protected
  source/backend/RTL/Scheduler inputs, or paths selected by unresolved globs.
  Verify retained hashes and report actual reclaimed bytes after cleanup.

### Routine Gem5 fast retention (user default, 2026-09-08)

For routine performance exploration, retain inputs, outputs, cycles,
configuration, and conclusions by default. This explicit user-authorized
cleanup point overrides the requirement above to keep every heavy artifact
until the entire application reaches PASS. Apply it separately to each
closed fast case after its required output comparisons and timing extraction
finish; a successful simulator exit alone is not a correctness PASS.

- Preserve the actual inputs or an immutable retained input corpus, task
  code, source revision/patch, build recipe, case and backend configuration,
  seeds, tool versions, input/output hashes, commands, and exit status.
- Preserve required actual and reference outputs, bit-exact comparison
  results, original stats/timing records, raw ticks, clock/conversion source,
  normalized cycles, metric boundaries, and the candidate conclusion. Rank
  deployment latency without host-side acceptance or golden-comparison work.
- After these records are saved and consumers are closed, allowlist and
  remove the run-local generated replay objects, linked replay ELFs, and
  padded replay images, including both named *.replay.* and runtime_task_*
  forms. Preserve code.bin, data.bin, shared_l2.bin, link recipes, and any
  other unique inputs needed to regenerate them. Do not delete by extension
  alone or remove a whole experiment directory.
- Do not request per-instruction/vector Debug dumps for routine fast runs.
  Keep detailed traces only for a representative diagnostic case or failure
  whose investigation requires them. Retain compact failure conclusions and
  a minimal reproducer; do not auto-prune unresolved failure evidence.
- Protect active runs, exact deployment firmware, RTL-qualified handoffs,
  canonical reproducers, and artifacts still consumed by another stage.
  Keep baseline/winner/comparison results without automatically duplicating
  their entire heavy replay workdirs for every matrix case.
- Freeze an exact-path cleanup manifest and retained-evidence hashes before
  unlinking files, verify those hashes afterward, and record actual freed
  allocated bytes. Each run's handoff must state whether compaction completed
  or which dependency still requires the heavy files.

This is the default agent workflow, not a simulator-exit cleanup hook. It
does not authorize blanket retroactive deletion of historical experiments.
