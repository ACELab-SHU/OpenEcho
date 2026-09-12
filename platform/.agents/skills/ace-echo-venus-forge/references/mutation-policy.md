# Mutation and ownership policy

This policy separates application development from platform maintenance. A
successful task or DAG must deploy through the currently approved ACE-ECHO
toolchain and immutable RTL; changing the evaluator to accept the workload is
not application completion.

## Default workload allowlist

For one application run, edits are limited to the selected Echo Hub workload:

- target task sources and task-local headers;
- target BAS/DAG sources and generated workload metadata;
- deterministic cases, reference adapters, approved golden payloads, and
  workload-local comparison code;
- a new workload-local self-checking launcher required by this application;
- immutable run artifacts, candidate metadata, reports, and proposed lessons.

An existing shared launcher is protected during application validation. A new
launcher is mutable only until its ABI and marker contract are frozen; after
that, a changed launcher creates a new attempt and invalidates prior evidence.

## Protected platform surfaces

The following are read-only during a workload run:

- Scheduler core, DAG parser/generator, runtime, DMA, interrupt and fence code;
- DSL parser, compiler frontend/backend, linker, image combiner, and shared
  code-generation scripts;
- Gem5 instruction semantics, timing model, hydration rules, and shared
  execution adapters;
- backend manifest semantics and shared ACE-ECHO orchestration gates;
- existing cross-workload tests and launchers;
- every RTL/SystemVerilog/Verilog source and evaluator input.

Build products may be written only to isolated attempt directories and the
backend-declared generated-output allowlist. RTL source is never mutable, even
in a separately approved infrastructure task.

## Failure ownership

First localize the failure to one of these owners:

```text
WORKLOAD_SOURCE_OR_ABI
WORKLOAD_DAG_OR_CASE
SHARED_DSL_OR_COMPILER
SHARED_SCHEDULER
SHARED_GEM5
RTL_DEPLOYMENT_BEHAVIOR
EXTERNAL_TOOL_OR_LICENSE
```

If a workload-only representation exists, implement it in the workload. Do
not change shared code merely to shorten that representation or accept invalid
metadata. In particular, do not change Scheduler descriptor widths, logical
type mapping, transfer lengths, completion behavior, output-port behavior, or
interrupt handling to make one DAG pass.

When the first necessary fix belongs to a protected surface:

1. preserve the failing workload, exact build/firmware/case digests, and first
   divergent event;
2. reduce it to the smallest positive/negative reproducer;
3. write a run-local or `docs/knowledge/proposed` patch proposal with scope,
   rollback, tests, affected backends, and counterexamples;
4. mark the application attempt
   `BLOCKED_INFRASTRUCTURE_CHANGE_REQUIRED`;
5. wait for explicit human approval of a separate infrastructure change.

An approved infrastructure patch is developed and reviewed independently. It
must be reversible and pass focused positive/negative tests plus affected
historical workloads. Prior workload evidence is stale after the patch: start
a clean application attempt and repeat compile, Gem5, integration, and RTL
gates. Never use results from a locally modified protected component as final
application evidence.

## Dirty-worktree discipline

Freeze status and diffs for the main repository and every submodule before
editing. Preserve unrelated user changes. Record each authorized path in the
run manifest. If ownership of an existing modification is uncertain, do not
overwrite or revert it; isolate the attempt or ask the user.

Use local reversible commits only after the relevant gates pass. Push, merge,
tag, release, live-knowledge promotion, Gem5 semantic changes, Scheduler/DSL
changes, and backend-contract changes require separate explicit approval.
