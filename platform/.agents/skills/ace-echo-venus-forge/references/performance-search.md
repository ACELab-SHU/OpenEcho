# AI-first performance search

## Search contract

Use AI to propose transformations from the numerical contract, current source,
emitted assembly, backend user guide, Gem5 timing, and read-only RTL analysis.
Static rules protect correctness and deployment; they do not limit creative
algorithm, layout, fusion, tiling, scheduling, or precomputation hypotheses.

Start with a correct measured baseline. Default to at most 20 candidates and
stop after 5 consecutive correct-or-failed candidates produce no improvement.
Allow request overrides. Close every node before selection and change exactly
one falsifiable hypothesis per child.

Before implementation, classify every executable as one of:

- `HOST_ORACLE`: scalar semantic authority or differential model; never ranked
  as accelerator performance.
- `DIAGNOSTIC_ORACLE`: temporary accelerator code used only to resolve a
  compiler, ABI, address, ISA, or fixed-point uncertainty.
- `DEPLOYABLE_CANDIDATE`: workload implementation intended to satisfy both the
  correctness contract and the declared performance budget.

Do not silently promote a diagnostic oracle into a deployable candidate. A
promotion requires a new immutable child whose hot path is designed for the
backend and whose role is explicit in its evidence.

Treat a payload-sized scalar loop as a baseline/fallback, not as a completed
high-performance Venus candidate. Drive the numerical hot path with Venus
vector operations unless a documented ISA/compiler limitation makes that
stage genuinely non-vectorizable. Small control logic, address setup and
irregular tails may remain scalar. Record substantial scalar payload work as a
comparative performance reference, not a ranked deployable candidate. It may
become rankable only when active evidence proves no viable vector formulation
for that work and it still satisfies the frozen performance budget.

## Performance budget and escalation gate

Freeze before the first expensive run:

```text
hot task or stage
representative case and shape
primary metric and unit
budget or target
ticks-per-cycle derivation and clock domain
maximum predicted run cost
vectorization evidence required for matrix expansion
```

Use a user-specified budget when present. Otherwise define a workload- and
backend-specific target from the application contract or a qualified peer; do
not invent one universal cycle limit.

After the first correct representative run:

1. Record raw ticks, normalized accelerator cycles, and the conversion source.
2. Audit emitted assembly and dynamic instruction mix for the hot path.
3. Set `fitness: INELIGIBLE` with reason code
   `PERFORMANCE_BUDGET_EXCEEDED` when it exceeds the frozen budget, or
   `VECTOR_PATH_REQUIRED` when regular payload work remains substantially
   scalar without a documented ISA/compiler blocker.
4. Stop running additional correctness-matrix cases for an ineligible
   candidate. Preserve the representative result as compact oracle or
   counterexample evidence and start a vector/decomposition/layout child.
5. Permit full-matrix expansion only after one correct candidate passes static
   resource gates, the representative performance budget, and the required
   vectorization evidence.

An additional diagnostic case is allowed only when it resolves a new
shape-specific uncertainty that the representative case cannot answer. State
that uncertainty and stopping condition before running it. It does not reopen
general matrix expansion for the oracle.

Estimate long-run cost before execution from representative task cycles,
task multiplicity, case count, and known simulator throughput. If the estimate
exceeds the declared run budget, reduce to the smallest falsifying case or
redesign the candidate first.

## Candidate loop

1. Diagnose the dominant measured bottleneck and retrieve bounded foundation,
   empirical, and prompt evidence.
2. State one hypothesis, predicted mechanism, applicability boundary, and
   falsification observation.
3. Materialize a child without changing the frozen golden or benchmark corpus.
4. Run syntax/ABI/address/row/workspace gates and independent compilation.
   Classify every `vbarrier()`: retain the canonical
   `vbarrier -> VSPM_OPEN -> vaddr/scalar access -> VSPM_CLOSE` boundary, and
   reject an unexplained barrier between pure vector operations.
5. Run Gem5 fast and compare every required output.
6. Profile only a correct candidate; bind metrics to the comparability key,
   record raw ticks plus normalized cycles, and record dynamic scalar/venus
   instruction mix for each hot task.
7. Apply the performance/vector-path escalation gate before adding cases or
   integrating the child.
8. Accept it as a new optimization parent only when measured evidence supports
   the hypothesis. Preserve compact evidence for failures and rejected nodes.

## Creative prompt dimensions

- Reformulate the fixed-point algorithm around supported fused/complex/mask
  operations rather than scalar control flow.
- Shorten live ranges, delay claims, reuse registers, isolate stage boundaries,
  and inspect compiler-created temporaries before shrinking vectors.
- Transform static layout for continuous transfers and register shuffles.
- Fuse adjacent stages only with explicit rounding/saturation/checkpoint proof.
- Precompute configuration-derived indices, tables, and layouts outside runtime
  tasks with address non-overlap evidence.
- Optimize the full DAG critical path, data movement, and parallel schedule,
  not just a locally expensive task.
- Analyze instruction syntax and RTL datapaths to create minimal compatibility
  probes and new candidate ideas.
- Increase useful parallel work per vector instruction through batching,
  layout transformation, masks, shuffles, reductions, and multi-antenna or
  multi-symbol processing before accepting scalar traversal of a regular
  tensor.
- Remove superstition barriers from pure-vector regions one justified boundary
  at a time. Measure the integrated candidate after the complete correctness
  gate; fewer barriers alone is structural evidence, not a speedup claim.

## Ranking

Build the comparability key from backend ID, hardware digest, workload
contract, frozen case/benchmark digest, build config, measurement scope, metric
definition, and environment. Exclude candidate identity from the key.

Use backend ticks/cycles, never host wall-clock time. Separate correctness and
performance corpora. Measure small, typical, and maximum configurations; use
manifest weights or equal weights by default. Report every case and the Pareto
front so an aggregate cannot hide a severe regression.

Run the full correctness matrix with the backend's full_software_engine
(normally fast) for the best integrated candidate and necessary fallbacks.
Use verification/debug to diagnose failures or divergence. Run fresh RTL only
for an integrated winner with complete passing output comparisons.

## Required handoff summary

For every hot task, report in one table: candidate role, correctness scope,
raw ticks, normalized cycles, budget, budget status, scalar/vector dynamic
instruction mix, matrix cases run, and next allowed stage. A wall-clock-long
run or a large raw tick count must trigger unit normalization and budget
classification before any further execution.

Candidate evidence must include `candidate_role`, standard `fitness`
(`INELIGIBLE`, `UNRANKED`, or `RANKED`), `reason_code`, and
`next_allowed_stage`; do not encode reason-specific values in `fitness`.
