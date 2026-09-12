# Forge control protocol

## Ownership

The control plane owns lifecycle, typed artifacts, immutable attempts, budgets,
candidate lineage, cleanup, and knowledge review. The selected backend owns
architecture, ABI, build/run entry points, correctness scopes, comparison,
metrics, resource gates, and mutable/immutable paths.

Do not pass critical state only through chat. Store canonical JSON artifacts
with SHA-256 references. A changed artifact creates a new attempt and path.

## Capability graph

```text
F00 NORMALIZE
 -> F01 REFERENCE
 -> F02 NUMERIC_ABI_CONTRACT
 -> F03 EVIDENCE_AND_TEST_CORPUS
 -> F04 BASELINE_PLAN
 -> F05 MATERIALIZE
 -> F06 STATIC_AND_BUILD
 -> F07 GEM5_FAST_EXECUTE_JUDGE_PROFILE
 -> F08 AI_SEARCH (loop to F04, one hypothesis per child)
 -> F09 FULL_MATRIX_SOFTWARE_VALIDATION (backend engine; normally fast)
 -> F10 INTEGRATE_AND_REVERIFY
 -> F11 FRESH_RTL_AND_COMPARE
 -> F12 REPORT_AND_RETENTION
 -> L00 TRIAGE -> L01 PROPOSE -> L02 SHADOW_REPLAY
 -> L03 HUMAN_REVIEW -> L04 PROMOTE
```

`EXECUTE`, `JUDGE`, and `PROFILE` are separate decisions. A zero exit code is
not correctness. Only correct candidates may be profiled and ranked. Only the
integrated all-output-correct software winner may enter RTL. Debug is a
diagnostic branch after failure or divergence, not a mandatory normal-path
stage. Preserve F09/F10 coverage regardless of the selected binary.

## Artifact envelope

Every stage emits:

```text
schema, run_id, stage, attempt, producer, status,
inputs[], outputs[], diagnostics[], metrics, side_effects[], decision
```

Every artifact reference emits:

```text
artifact_id, artifact_type, schema, path, digest,
producer_stage, producer_attempt
```

Use `PASS`, `FAIL`, `BLOCKED`, and `REVIEW_REQUIRED`. Use `INELIGIBLE` for a
failed hard gate, `UNRANKED` for a correct candidate without comparable
metrics, and `RANKED` only for correct comparable evidence.

## Hard gates

Apply in this order:

```text
environment/repositories
reference authority
numeric and ABI contract
static syntax/resources
independent build
Gem5 execution
all-output correctness
resource limits
same-key performance
integrated full-matrix Gem5 execution and all-output comparison
fresh immutable RTL
```

Do not infer missing signedness, rounding, saturation, Q format, pointer ABI,
shape, output order, or state. Stop the affected stage as `BLOCKED`.

## Repository contract

- Keep Echo Hub at `workloads/` and DSL at `components/toolchain/dsl` as pinned
  standard Git submodules.
- Freeze main repository and submodule commits in every run.
- Work on local branches. Commit reversible units. Require approval for push,
  merge, tag, release, Gem5 semantic patches, and live knowledge promotion.
- Treat RTL source as immutable evidence. A dirty pre-existing RTL tree is not
  automatically invalid, but freeze its status/diff digest and prove the run
  did not change it.

## Retention

Keep raw evidence while a run is active, failed, blocked, or under review. Only
an application-complete `PASS` report unlocks cleanup. Preserve baseline,
winner, RTL-qualified candidates, top three correct alternatives, all compact
candidate metadata, failure fingerprints, metrics, final evidence, and minimal
regression reproducers. Remove only allowlisted heavy candidate entries.
