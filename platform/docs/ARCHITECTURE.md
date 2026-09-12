# ACE-ECHO architecture

## Boundary

ACE-ECHO owns orchestration and pins all source inputs:

```text
ACE-owned DSL/BAS frontend + external compiler binaries
                   |
                   v
       task ELF / DAG JSON+BIN
                   |
        +----------+-----------+
        |                      |
        v                      v
ACE-ECHO Scheduler      ACE-ECHO Gem5
   l1.elf/l1.bin     CPU + DMA + L2/L1 + Venus
        |                      |
        +----------+-----------+
                   v
       outputs, traces, timing, run.json
```

No command resolves Gem5, Scheduler, or RTL through an old project path.
Historical RTL paths in imported alignment documents are evidence citations,
not executable dependencies.

## Three development scopes

### Task

The task adapter consumes post-synthesis DAG JSON, combined DAG BIN, and task
flat images. It materializes a one-task manifest and runs that task through
the same Gem5 Venus tile implementation used by larger scopes.

### DAG

The DAG adapter runs all tasks in one Gem5 process. Dependencies, task
admission, DMA, physical tile selection, returns, and task timing are emitted
as structured artifacts. It does not use VEMU output as an execution oracle.

### Application

The currently enabled backend decodes the runtime DAG contract embedded in
an ACE-ECHO Scheduler `l1.elf`, constructs the same L1/DMT/DMA contract as the
RTL-shaped Scheduler model, then runs it in Gem5.

Full firmware mode additionally requires Gem5 to execute the Scheduler CPU
firmware and model its interrupt/peripheral boundary. The CLI reserves this
backend but refuses to claim it before implementation and regression.

## Independence

- `components/toolchain/dsl` is a pinned standard Git submodule.
- `workloads` is the pinned Echo Hub standard Git submodule.
- `components/gem5` is a working-tree snapshot, not a symlink.
- `components/scheduler` is a working-tree snapshot, not a symlink.
- Gem5 opt/debug executables are included for immediate local use.
- Scheduler builds happen in a run-local copy because its upstream Makefile
  cleans and regenerates files.
- RTL is optional evidence. ACE-ECHO can compare imported RTL logs without
  having an RTL repository present.
- Compiler executable installations and the immutable RTL repository are
  backend-manifest paths. DSL and workload source revisions are fixed by the
  ACE-ECHO superproject.

## Forge planes

```text
Control:   request, backend, artifacts, gates, budgets, retention
Execution: reference, compile, Gem5 fast/full, compare, integrate, RTL
Knowledge: foundations, run-local memory, proposals, human-reviewed live rules
```

Forge prohibits VEMU at request and backend validation. The AI may freely
propose performance hypotheses, but deterministic gates decide eligibility and
only same-key measured candidates are ranked. RTL source is immutable; a
Gem5/RTL mismatch becomes a focused probe and a reversible simulator/linter/
documentation proposal.

## Modes

`verification` preserves detailed VINS output and sequencer monitoring.
`fast` disables those expensive observations but preserves simulated events,
task/DAG timing, final outputs, and Gem5 statistics. Results always record the
selected mode.

The CLI defaults to fast. Shipped backend `full_software_engine` selects
`gem5.fast` for the complete correctness matrix and integrated RTL handoff;
this changes the executor, not output coverage. `verification` remains an
explicit diagnostic mode with its own binary and no silent fallback.
`doctor --scope diagnostic` checks debug availability; normal software/full
doctor scopes follow the selected backend policy. Legacy/custom manifests
without this policy field retain the verification requirement.
