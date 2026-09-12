---
name: ace-echo-venus-forge
description: Port an algorithm, executable implementation, task, DAG, or complete application to a configured Venus backend through ACE-ECHO; establish and approve an independent golden model, generate and AI-optimize Venus C/BAS, run independent compilation and full-matrix Gem5 fast validation, compare every visible output, qualify the integrated winner on immutable RTL, and use debug for divergence diagnosis. Use for “ACE-ECHO Forge”, Venus ports, reference migrations, high-performance Venus development, Gem5-versus-RTL consistency, or maintenance of backend manifests and optimization knowledge. VEMU is always forbidden.
---

# ACE-ECHO Forge

Turn user intent or a runnable foreign-language implementation into a
functionally equivalent, measured, RTL-qualified Venus application. Keep
hardware generations in backend manifests; never encode Venus2.0 dimensions
as universal rules.

## Load the contract

1. Locate the ACE-ECHO root, selected request, backend manifest, Echo Hub
   submodule, DSL submodule, and immutable RTL root.
2. Read [control-protocol.md](references/control-protocol.md) completely.
3. Read [mutation-policy.md](references/mutation-policy.md) completely before
   editing or building anything. A workload failure never authorizes a shared
   Scheduler, DSL/compiler, Gem5-semantics, or RTL change.
4. Read [venus-operator-development.md](references/venus-operator-development.md)
   completely before planning or writing Venus C/BAS.
5. Read [golden-and-correctness.md](references/golden-and-correctness.md) when
   creating or judging a reference.
6. Read [performance-search.md](references/performance-search.md) before
   generating or ranking optimization candidates.
7. Read [knowledge-governance.md](references/knowledge-governance.md) before
   retrieving prompts, writing a lesson, or proposing a Wiki/linter change.
8. Read [gem5-and-rtl.md](references/gem5-and-rtl.md) before compiling, running
   Gem5, building Scheduler firmware, or qualifying RTL.
9. Read the selected backend manifest and every foundation document it names.
   Treat missing bit-, ABI-, shape-, or hardware-affecting fields as
   `BLOCKED`.

Validate before acting:

```bash
python3 .agents/skills/ace-echo-venus-forge/scripts/validate_manifest.py \
  request request.json
./ace-echo forge request.json --validate-only
```

## Enforce the execution boundary

- Never invoke, import, build, repair, or use VEMU. Reject a request or backend
  that exposes it as an engine.
- Keep application work inside the workload-owned mutation allowlist. If a
  failure points to protected infrastructure, preserve a minimal reproducer
  and stop with `BLOCKED_INFRASTRUCTURE_CHANGE_REQUIRED`; do not fold an
  infrastructure patch into the workload result.
- Use independent compilation, Gem5 fast, complete golden comparison,
  integrated revalidation, timing extraction, and fresh RTL in that order.
  The selected backend's full_software_engine chooses the full-matrix executor;
  shipped profiles use gem5.fast. Use verification/debug for failure or
  divergence diagnosis, not as a mandatory extra run. See gem5-and-rtl.md.
- Treat high-performance Venus payload kernels as vector-first. Scalar code is
  limited to references, small control/setup, or a documented ISA gap; a large
  scalar loop hidden behind `vaddr()` is not an optimized candidate.
- Correctness-first does not mean scalar-task-first. Keep scalar numerical
  models on the host whenever possible. If a scalar Venus task is temporarily
  required to prove compiler, ABI, address, or fixed-point behavior, label it
  `DIAGNOSTIC_ORACLE`, run only the minimum representative case needed to
  resolve that uncertainty, and exclude it from deployable-candidate ranking.
- Freeze a per-hot-task performance budget and its unit before an expensive
  Gem5 run. After the first correct representative measurement, stop matrix
  expansion when a hot task exceeds the budget or its emitted/dynamic payload
  path remains substantially scalar. Transition to vector candidate search;
  do not spend the full correctness matrix, Gem5 verification, integration, or
  RTL budget on a performance-ineligible oracle.
- Report raw simulator ticks and backend clock-normalized accelerator cycles
  together. Derive ticks-per-cycle from the selected backend and active clock
  domains; never present Gem5 ticks as hardware cycles or compare unlike units.
- Use `vbarrier()` by default only for the canonical
  `vbarrier -> VSPM_OPEN -> vaddr/scalar access -> VSPM_CLOSE` visibility
  boundary, and treat that form as a probe protocol rather than a portable
  coherence guarantee. For an RTL-bound product candidate, reject scalar
  dereference of any vector ABI input, temporary, or return value unless a
  focused paired Gem5/fresh-RTL probe qualifies the exact backend and RTL
  digest. Prefer eliminating the scalar view. Reject unexplained barriers
  between pure vector operations; a different use requires a focused backend
  probe and adjacent justification.
- Resolve Gem5, Scheduler, and RTL locations from the selected backend
  manifest. Never embed a checkout-specific RTL path in this skill or a
  workload. Allow a run-local RTL-root override only when it is recorded in
  preflight and evidence.
- Before Scheduler/RTL handoff, require the backend to declare separate
  `max_task_outputs` and `max_dag_outputs` limits. Count only the top-level
  `return_output` entries for the DAG limit; internal task-to-task ports remain
  governed by the task limit. Reject or pack an excessive DAG before building
  firmware. Do not treat either count limit as proof that the Scheduler can
  allocate the returned payloads. After linking the final L1 image, calculate
  the residual Scheduler heap and the peak allocation of every simultaneously
  live DAG return, including allocator headers and alignment. Reject the
  firmware unless the peak fits with a recorded positive margin. Pop and free
  every dynamically allocated return after its last consumer use, including
  outputs retained only for evidence or intentionally discarded.
- Keep RTL source and evaluator inputs immutable. Write only to the backend's
  narrow generated-output allowlist. Freeze RTL status and digests before and
  after each run.
- Run RTL only for a fully correct integrated winner. If Gem5 passes and RTL
  fails, preserve firmware/input identities, reduce the first divergence to a
  probe, and propose reversible Gem5/linter/docs changes. Never patch RTL.
- For the current Venus2.0 RTL, keep each multi-beat return as one semantic
  port but align its transport length to 64 bytes and zero the padding. Keep
  generated return descriptors in canonical port order with no speculative
  response-lag shift. Re-probe these implementation-specific rules before
  applying them to Venus3.0 or later.
- Create local feature branches and reversible commits. Do not push, merge,
  tag, publish, or promote knowledge without explicit human approval.

## Execute the forge

1. Normalize the intent into `ace-echo-forge-request/v1` and start the run:

   ```bash
   ./ace-echo forge request.json
   ```

2. Establish one reference authority. Run provided source first and add
   deterministic tests. For intent/spec-only input, implement an independent
   Python, MATLAB, or C reference. Mark agent-created references
   `PROVISIONAL_GOLDEN` until a human approves their digest.
3. Freeze fixed-point and ABI semantics before Venus code: widths, Q formats,
   complex layout, rounding, saturation/wrap, shifts, shapes, aliases, state,
   and every visible output. A floating model measures algorithm accuracy but
   is not an integer golden.
4. Generate the cheapest correctness evidence that resolves implementation
   uncertainty. Prefer a host oracle plus a vector-first Venus candidate. A
   scalar Venus diagnostic may run one representative case, but it must stop
   after correctness and timing classification; it is not permission to run
   the full matrix.
5. Establish a vectorized hot-path candidate that passes the representative
   golden gate and the frozen performance budget. Audit emitted and dynamic
   instruction mix: vector loads/stores around scalar arithmetic do not count
   as a vectorized payload kernel.
6. Run the AI-first performance loop. Change exactly one falsifiable
   hypothesis per child candidate. Default to 20 candidates and stop after 5
   consecutive non-improvements. Rank only same-key, correct candidates.
7. Expand to the full correctness matrix only for the best correct,
   resource-valid, performance-eligible candidate and necessary structurally
   distinct fallbacks. Rebuild and rerun the winner with the backend's
   full_software_engine over the full declared correctness matrix. Integrate
   it, then repeat the required software scope and all-output comparisons.
   Fast completion alone is not a correctness PASS. Diagnose failures with
   verification/debug and revalidate any changed candidate in a new attempt.
8. Perform a fresh RTL build and compare all visible outputs for the declared
   hardware qualification matrix by following `gem5-and-rtl.md`. The read-only
   RTL preflight, including VCS/VIP/license reachability, is a hard gate before
   creating a snapshot or compiling. Label unrun configurations software-only.
9. Report baseline, winner, top alternatives, failures, per-case performance,
   coverage, specialization, repository identities, and every unclosed gap.
10. Generate run-local and proposed lessons. Stop at `REVIEW_REQUIRED`; only a
   human-reviewed promotion may edit live knowledge.
11. After the entire application is `PASS`, retain baseline, winner, every
    RTL-qualified candidate, and the top three correct candidates. Remove only
    allowlisted heavy data from other candidates while preserving compact
    metadata:

    ```bash
    ./ace-echo forge --finalize-run runs/<run-id>
    ```

## Preserve claims

- Keep `PASS`, `FAIL`, `BLOCKED`, and `REVIEW_REQUIRED` distinct.
- Keep `INELIGIBLE`, `UNRANKED`, and `RANKED` fitness distinct.
- Compare every externally visible output; add task checkpoints whenever the
  reference exposes them. Report the first mismatching checkpoint.
- Freeze normal, boundary, extreme, aligned/unaligned, fixed-seed random, real
  capture, successful decode, and expected CRC-failure cases as applicable.
- Compare integer interfaces bit-for-bit. Do not replace an intermediate
  mismatch with a final-decoder-only claim.
- Bind performance to backend/hardware, contract, case, build, measurement
  scope, environment, and candidate digests. Never rank wall-clock time.
- Keep software verification and RTL qualification separate.
- Preserve the winning source, build flags, memory layout, firmware, and all
  component revisions as one indivisible handoff.
