# Gem5 and RTL execution

## Resolve configuration

Treat the selected `configs/backends/*.json` file as the architecture and RTL
authority. Resolve relative entries under `paths` against the manifest
directory. Use `configs/local.toml` only for host tool installations and the
ACE-ECHO CLI defaults. Record resolved paths, repository commits/status,
backend digest, tool identities, and command argv in the attempt.

`paths.rtl_root` is required and may be absolute or manifest-relative. Do not
hardcode an RTL checkout in the skill, workload, generated main, or script. A
single run may override only the preflight root with `--rtl-root`; record the
override and use that same resolved tree for the complete qualification.

Before execution, validate configuration:

```bash
python3 .agents/skills/ace-echo-venus-forge/scripts/validate_manifest.py \
  backend configs/backends/<backend>.json --digest
./ace-echo --config configs/local.toml doctor --json
```

Stop as `BLOCKED` if a path/tool required by the selected stage is absent, the backend exposes a
forbidden engine, or a bit/ABI-affecting backend field is missing.
Debug is optional on the shipped normal fast-to-RTL route. Use
`doctor --scope diagnostic` before explicit verification/debug diagnosis.

## Compile and run Gem5

Compile the BAS DAG independently into an immutable attempt:

```bash
./ace-echo --config configs/local.toml compile dag \
  --target <workload-dag-directory-name> \
  --run-dir runs/<run>-compile-a001
```

`compile dag --target` selects a DAG directory below the configured workload
root; it is not a `.bas` path. Use the emitted paths from the compile attempt
(for the current adapter, `artifacts/toolchain/dag1.json`,
`artifacts/toolchain/dag1.bin`, and `artifacts/toolchain/tasks`) in all later
commands. Do not reconstruct legacy `final_output` paths by convention.

Materialize each deterministic case outside task source. Run fast mode during
correctness-first candidate search:

```bash
./ace-echo --config configs/local.toml run dag --mode fast \
  --dag-json runs/<compile>/artifacts/compile/final_output/<dag>.json \
  --combined-bin runs/<compile>/artifacts/compile/final_output/<dag>_combined.bin \
  --case-dir workloads/<path>/cases/<case> \
  --run-dir runs/<run>-<case>-fast-a001
```

Use `run task` with the same arguments plus `--task <id>` only for a declared
task-level probe. A task replay is not a full-DAG claim. Use `run application`
with `--l1-elf` only for the Scheduler contract/application scope.

Compare every visible integer output bit-for-bit and store the comparison:

```bash
./ace-echo compare --dtype raw \
  --output runs/<run>/artifacts/compare/<name>.json \
  <golden.bin> <actual.bin>
```

Rank only correct fast-mode candidates sharing one comparability key. Rebuild
the integrated winner, then run the full matrix with
`execution_policy.full_software_engine` (shipped backends: `gem5.fast`):

```bash
./ace-echo --config configs/local.toml run dag --mode fast \
  --dag-json <integrated-dag.json> --combined-bin <integrated-combined.bin> \
  --case-dir <case-dir> --run-dir runs/<run>-integrated-fast-a001
```

Normal route: compile -> fast -> complete golden/output comparison and
integration validation -> fresh RTL -> software/RTL output comparison.
Do not require a second debug run on this route. A custom backend may still
select gem5.verification; respect that explicit policy.

On a failure, assertion, hang or output divergence, preserve the first failing
case and identities; use `run ... --mode verification` in a new attempt for
debug diagnosis. Never alias opt to debug or silently fall back. A debug PASS
does not erase a fast/RTL failure. After a fix, start fresh validation of the
affected matrix and RTL scope. Missing debug blocks diagnosis, not an otherwise
correct fast-to-RTL handoff.

Before RTL, bind the complete case matrix, approved golden and required-output
comparisons to exact backend, build, inputs, integration firmware and software
engine/binary identities. Preserve all ABI/resource gates, RTL preflight,
fresh-build/source-immutability checks, self-checks, output coverage and bus
assertions. Fast exit code alone cannot authorize RTL qualification. Report
debug as NOT_RUN when unused, never as PASS. Existing run-local scripts may
still hard-code debug; do not silently relabel their historical evidence.

Preserve Gem5 stdout/stderr, `m5out`, generated/hydrated manifests, output
dumps, tick/cycle metrics, input hashes, and complete comparisons. A zero exit
status is execution evidence, never correctness by itself.

Before Scheduler handoff, audit scalar/vector memory coherence using the
selected backend rather than a simulator assumption. Compute one physical VRF
row as `lanes * banks_per_lane * bank_width_bits / 8`. A pointer returned by
`vaddr()` is not a portable scalar/vector coherence guarantee, even when the
access remains within one physical row. Reject scalar dereference of a vector
ABI input, vector temporary, or return value for an RTL-bound product unless a
focused paired Gem5/fresh-RTL probe qualifies that exact access on the selected
backend and RTL digest. If such an access is qualified, keep it within one
physical row; a logical vector spanning multiple rows must be split into
separately claimed row-sized scalar views, and every cross-row move must use
type-compatible vector instructions. Prefer generated scalar constants,
explicit scalar ABI fields, or pure vector dataflow over `vaddr()`. Reject
literal `vload`/`vstore` scratch addresses unless that address range is
explicitly declared by the backend. Also verify that the scheduler's parsed
`vreturn` byte count equals both the producer ABI and consumer input size;
prefer an unambiguous literal when the legacy metadata parser cannot resolve
`sizeof(local_vector)` correctly.

Also audit the task-call ABI and every multi-output boundary before accepting
Gem5 evidence. Keep the generated C function parameter count within
`abi.task_call.register_argument_count` whenever
`stack_arguments_supported` is false. Confirm this in the task disassembly:
loads of arguments beyond `a0`-`a7` from the incoming stack are a rejection,
even when Gem5 completes. Audit output count and each `vreturn` byte length as
separate fields. On the current Venus2.0 RTL, each return entry carries an
independent 32-bit requested length; 64 bytes is the AXI/DMA beat and alignment
granularity, not a per-output payload limit. Paired probes on this implementation
showed that requested lengths 96, 288 and 864 bytes transported only 64, 256
and 832 bytes. Treat a narrow final return beat as unsupported until the exact
backend passes a positive probe: round the transport allocation and `vreturn`
length up to 64 bytes and zero every padding byte. Do not split a logical output
into 64-byte ports solely because DMA works in beats. Preserve one logical
multi-beat output when possible and verify its generated length, aligned
address, downstream descriptor width and complete Gem5/RTL contents. Use BAS
`concat(...)` only when the algorithm or a focused backend probe establishes a
real need.

Audit output descriptor order separately from output lengths. Use the canonical
producer port order emitted by the staged DAG and require distinct destination
addresses. Do not rotate or lag descriptors based on an assumed response delay;
the Venus2.0 CCH probe demonstrated that a one-port shift duplicated the final
destination and aliased two MRC outputs, while zero shift passed 61/61 ports.

## Build the Scheduler handoff

Only an integrated winner passing full software correctness may produce RTL firmware. Build
Scheduler in the CLI's isolated copy and stage explicit DAG artifacts:

```bash
./ace-echo --config configs/local.toml scheduler build \
  --backend configs/backends/<backend>.json \
  --target <backend-scheduler-target> \
  --main-src <self-checking-main.c> \
  --dag-name <dag-name> --dag-json <integrated-dag.json> \
  --dag-bin <integrated-combined.bin> \
  --run-dir runs/<run>-scheduler-a001
```

Before invoking the build, read both output capacities from
`abi.scheduler_output`. `max_task_outputs` limits each task's `all_output` /
`vreturn` port count; `max_dag_outputs` limits the final DAG-level
`return_output` list exported through the L2 Scheduler to L1. They are not
interchangeable, and internal task-to-task edges do not consume DAG return
slots. The CLI must reject a staged DAG whose final return list exceeds the
backend limit. Packing is allowed only when output byte layout, alignment,
padding, producer ABI and the self-checking consumer are all updated and
reverified; never truncate the list.

The output-count check is necessary but not a memory-capacity check. Audit the
actual linked Scheduler image before RTL handoff: determine the heap interval,
then compute the peak bytes occupied by all returns from one outstanding DAG
fire using the Scheduler allocator's real header and alignment rules. Record
the heap start/end, available bytes, per-DAG peak and positive margin in the
build receipt, and reject a candidate that does not fit. On the current
Scheduler allocator the model is `align_up(return_length + 8, 64)` per live
return, but this is an adapter qualification item rather than a cross-backend
constant. A popped return owns dynamically allocated storage; the generated
self-checking main must free it immediately after its final comparison or
discard. In particular, eight legal descriptors can still deadlock when eight
large buffers exceed the small heap left after embedding all DAG payloads.

Require `l1.elf`, `l1.bin`, `l1.map`, and the staged JSON/BIN payload. Audit
all memory regions against backend limits. Verify the Scheduler's physical
task-input descriptor width and logical-to-physical mapping against
`abi.task_input_descriptor`; never truncate logical JSON type values directly.
The Scheduler build must also reject every type listed in
`rtl_unsupported_logical_types`. A mapping that fits the physical bit width is
not sufficient when it loses behavior. Do not infer descriptor width or pointer
support from the generation name alone. The `venus2p0-16x128` manifest targets
RTL `bf82d804`, whose three-bit descriptors use physical types 4/5/5 for
logical types 4/5/6. The physical type-6 construction branch does not fetch
its global parameter in the preceding state; logical type 6 therefore uses
the qualified type-5 pointer path, retaining L1 dynamic-input ownership.
This is pointer-to-pointer compatibility, not a pointer-to-value conversion.
Other RTL revisions must
be checked against their selected manifest; never map pointers to value types
merely to pass a width check. See `docs/VENUS2_POINTER_ABI.md` for evidence and
the distinction between pointer support and complete RTL qualification.

Make the handoff self-checking: compare every returned buffer and emit distinct
GPIO/status markers for boot, main entry, DAG completion, compare pass, and
compare failure. Hash the exact `l1.bin`, case inputs, expected outputs, map,
and generated main before RTL staging.

## Run immutable RTL

Read the RTL flow, firmware destinations, make variables, required environment
names, completion markers, and evidence paths from `engines.rtl.flow`. Do not
guess a flow for a new backend.

1. Resolve `paths.rtl_root`; freeze its commit, status, diff/untracked list,
   and protected-source digests. Run read-only preflight **before creating an
   RTL snapshot or starting compilation**:

   ```bash
   ./ace-echo rtl-preflight \
     --backend configs/backends/<backend>.json \
     --output runs/<run>-rtl-preflight/artifacts/rtl-preflight.json
   ```

   To use an explicitly selected checkout, add `--rtl-root <path>` and retain
   that resolved path in all subsequent evidence.

   This is a hard infrastructure gate. It must check the backend-declared
   `vlogan`/`vcs` search path, `VCS_HOME`, `DESIGNWARE_HOME`, executable
   `SLI_USER_SLISERV_PATH`, and every license source named by
   `LM_LICENSE_FILE` and `SNPSLMD_LICENSE_FILE`. For a `port@host` source,
   require a successful TCP connection; for a license-file source, require a
   non-empty regular file. Record `BLOCKED` and stop before the expensive
   build when no configured source is usable. Do not mistake a configured
   environment string for a reachable license service.

   A sandbox or container may not share the host loopback namespace. When the
   manifest names `port@localhost`, run both preflight and the authorized VCS
   flow in the same host network context as the license daemon. If
   `CODEX_SANDBOX_NETWORK_DISABLED` is set and sandbox-local loopback cannot
   see the endpoint, record `BLOCKED` with reason code
   `REQUIRES_HOST_RECHECK`, never `BLOCKED_LICENSE` or “no service
   listening.” Repeat `./ace-echo
   rtl-preflight` through the host-authorized execution context and retain that
   report; use `ss` plus the installed FlexNet diagnostic (`lmutil lmdiag`)
   when available to distinguish a listening daemon from an actual checkout.
   Only the host-context result may authorize or block VCS, and the fresh VCS
   build/simulation must run in that same network context.

   License recovery is configuration- and authorization-driven. Never guess a
   daemon command or silently start a shared service. A backend may document a
   host-local recovery command only after its executable, referenced license
   file, and path mapping have been validated; running it still requires the
   authority applicable to that host. After recovery, rerun `rtl-preflight`
   and proceed only on `PASS`. In particular, do not use a launcher merely
   because it is named `licsrv-start`: reject it when it points to missing
   installations or empty license files.

2. Create a run-local isolated snapshot excluding `.git` and prior build
   directories. Never compile or simulate in the protected source tree.
   Recheck the source-tree status/digests after the run.

   Before compilation, copy and relocate every source dependency named by the
   checked-in build flow, including ignored generated caches that are inputs to
   compilation. The current Venus RTL flow requires fabric/IP `.lst` files,
   `hardware/mbist`, `hardware/generated`, and
   `sim/model/venus_extension/venus_full_dag_perf_monitor.svh`. Missing one is a
   snapshot-construction failure, not a workload or RTL functional failure.

3. Copy the exact Scheduler `l1.bin` to every backend-declared firmware
   destination inside the snapshot. Verify byte identity with SHA-256 and
   `cmp` before simulation. Never fall back to an existing repository binary.

4. Validate checked-in file lists. If they contain a stale absolute checkout
   root, stop the source-tree flow. A backend-declared relocation may rewrite
   paths only in the isolated snapshot; record the changed file list and
   before/after digests. Never change RTL/SystemVerilog/Verilog source.

5. Export only the backend-declared tool environment, prepend every declared
   `path_prepend` entry to `PATH`, and verify that the simulator executable is
   discoverable before starting the build. Materialize the launch environment
   directly from the manifest and compare every exported required-environment
   value byte-for-byte with the manifest immediately before `make`; never
   manually derive `DESIGNWARE_HOME`, truncate `SLI_USER_SLISERV_PATH` to its
   containing directory, or substitute a nearby tool installation. Record this
   comparison in the preflight/launch receipt. `VCS_HOME` alone does not make
   `vlogan`, `vcs`, or `vcselab` executable. Run the configured fresh-build
   target. For the current overall VCS flow the manifest expands to this
   command shape:

   ```bash
   make -C <rtl-snapshot> sim_compileall \
     SIM_NAME=<sim-name> TESTBENCH_NAME=<testbench> \
     SIM_TYPE=<sim-type> COPY_ID=<unique-copy-id> \
     TARGET_BIN_FILE_NAME=<firmware-case-name>
   ```

   Require a newly generated simulator and zero compile/elaboration errors.
   If only the runtime launch environment was incomplete, the exact fresh
   simulator may be rerun with the configured `sim_nocompile` target only
   after source/build hashes prove identity.

   Treat simulator and VIP license availability as an external prerequisite.
   A time-zero `Unable to locate SLI server` is `BLOCKED` launch/license
   evidence, not a workload or RTL failure. Before blaming license capacity,
   verify that the backend-declared `DESIGNWARE_HOME` and optional
   `SLI_USER_SLISERV_PATH` resolve to the installed `sliserv`; do not replace
   them with `VCS_HOME`. Preserve the failed launch, prove the configured
   license endpoint is reachable, and retry the same freshly hashed simulator
   without rebuilding or changing sources. Reuse `sim_nocompile` only when a
   fresh simulator was actually produced and its source/build/firmware hashes
   still match. If elaboration never produced that simulator, recover the
   license first and repeat the required fresh build; never relabel an old
   simulator as fresh.

6. Monitor progress without treating a long simulation as success or failure.
   Use backend markers and traces. For the current overall flow:

   - boot marker absent: boot/firmware staging failure;
   - boot present, main-entry absent: L1 startup failure;
   - main-entry present, no L2 task allocation: inspect Scheduler descriptor
     width/type packing before waiting longer;
   - task allocation present, no completion: localize DMA, dependency, LSU, or
     task execution;
   - completion present: extract all visible outputs and compare bit-for-bit.

   A deterministic no-progress state is `BLOCKED` or `FAIL`, not “still
   running”. Preserve the first missing marker and the smallest causal trace.

7. Copy the configured logs, GPIO/status trace, L1/L2 DMA traces, L2 allocation
   log, output dumps, compile report, simulator hash, and immutable-source
   postcheck into the ACE-ECHO attempt. Report `rtl_all_visible_outputs_exact`
   only after the self-check and independent comparison both pass.

Never invoke, import, build, or repair VEMU anywhere in this flow. Never patch
RTL to make a workload pass. A Gem5/RTL difference creates a focused probe and
a reviewable Scheduler, Gem5, linter, or documentation proposal.
