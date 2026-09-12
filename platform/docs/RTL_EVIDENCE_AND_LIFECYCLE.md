# RTL evidence identity and managed execution

This infrastructure change leaves RTL/SystemVerilog sources, memory geometry,
the workload, Scheduler ABI and Gem5 semantics unchanged. An infrastructure
regression PASS is not independent algorithm-golden or application qualification.

## Daily validation

Compile the selected task/DAG, run Gem5 fast and build the matching Scheduler
firmware through the existing platform commands. Then run:

```sh
./ace-echo rtl run --case example=/absolute/path/to/l1.bin \
  --dag-json /absolute/path/to/compiled/dag1.json \
  --gem5-output-dir /absolute/path/to/fast/artifacts/gem5 \
  --output-layouts /absolute/path/to/approved-output-layouts.json \
  --run-dir runs/example-fresh-rtl --keep-build
```

`--output-layouts` is optional: without it, every requested bit is compared.
Do not infer valid-bit masks from observed X values or mismatches. Establish
layouts from the producer ABI and all consumers before comparison. Preserve
padding unknown/nonzero/difference warnings separately from valid-bit results.
All compared input files are added to the run's artifact inventory. A completion
GPIO alone does not prove either CRC correctness or output equality.

`--dag-json` and `--gem5-output-dir` must be supplied together. Currently their
automatic reconciliation supports one compiled DAG, one invocation and one
RTL case per command. Repeated invocations, multiple DAG IDs, or an unsupported
recorder protocol fail closed instead of collapsing identities. For a new
backend, qualify its recorder contract before enabling this option.

Without comparison inputs, the command is still only a smoke test, and the
report explicitly says `all_output_comparison_status: NOT_RUN`.

## Return identity

`engines.rtl.flow.return_identity` selects the versioned
`venus-serial-ack-v1` protocol and supplies physical tile address geometry.
This is an RTL-recorder contract, not a rule tied to a workload/task name.

The legacy native recorder samples the *current* arbitration winner at DMA
start; the actual DMA uses a previously latched request. During arbitration
changes the payload/address can be correct while the printed task/tile is
wrong. The platform associates each DMA start with a unique acknowledged
request before the next request acceptance. It checks every request/start,
the physical source tile, output slot capacity and relative destination layout,
as well as complete single-invocation task/port coverage. It never chooses an
identity by payload, assumes a fixed latency, silently drops duplicates or
hardcodes a task ID correction.

Compiled `all_output.length` is slot capacity. Run-time `vreturn` may be shorter;
its exact length is checked against the independently produced Gem5 output
file. The common temporary base inferred from all destination/offset pairs is
reported as a consistency check, not promoted to an architectural memory map.

Raw logs are immutable. `output-comparison.json` retains their SHA-256 digests,
the original labels, acknowledgment/DMA line numbers, corrected labels and the
contract used. Missing, conflicting, out-of-capacity or non-unique evidence
blocks qualification. Valid-bit X/mismatch or output-length mismatch fails
comparison even when identity recovery succeeds.

Existing logs can be reviewed independently without rerunning or editing RTL:

```sh
./ace-echo compare-dag-outputs \
  --expected-dma /absolute/path/to/dma_read_data_file_L2.txt \
  --rtl-log /absolute/path/to/sim.log \
  --dag-json /absolute/path/to/compiled/dag1.json \
  --identity-backend /absolute/path/to/selected/backend.json \
  --actual-dir /absolute/path/to/fast/artifacts/gem5 \
  --output-layouts /absolute/path/to/approved-output-layouts.json \
  --output runs/review/output-comparison.json
```

Historical review does not retroactively turn an interrupted run into a clean
execution PASS. Revalidate a fresh attempt after infrastructure changes.

## Startup diagnostics

`flow.startup_diagnostics` uses exact source-file SHA-256, relative path,
instance, message, zero simulation time and an occurrence bound. A changed
source, nonzero time, excess occurrences, different instance/message or absent
source evidence does not qualify. Raw errors and counts remain in the report.

The selected Venus2 source has an unconditional simulation-only `$error` on
both edges of `regen_mbist_enable_done2`, an address-decode signal rather than
a memory-test failure result. The observed error at 0 fs is classified as a
source-scoped startup warning. This is not an MBIST correctness certificate;
real MBIST failures, later decode errors, other simulator errors and firmware
failure GPIOs remain failures. AXI X warnings remain separately visible and do
not waive unknown valid output bits.

## Build, simulation and cancellation

The selected Venus2 flow declares ordered `fresh_build_steps` and a
`fresh_simulator_path`. The adapter executes the existing source build targets
sequentially, shares a build-stage wall-clock budget across them, verifies the
new simulator exists and hashes it, then invokes the existing no-compile
runtime target. It does not modify a native Makefile or reuse an old snapshot.

`flow.timeouts.build_seconds` and `simulation_seconds` default to 1800 each in
Venus2. Per-command `--build-timeout` and `--simulation-timeout` override them.
Debug/legacy combined flows are reported as `COMBINED_LEGACY`, not falsely
represented as split stages. These limits measure host wall time; UCLI horizon
and hardware cycles are different quantities.

Managed execution is on the POSIX/Linux tool host. Each command starts a new
session/process group. Timeout/cancellation sends TERM to that group, allows a
bounded grace period, escalates to KILL and reaps the direct child. A successful
parent leaving live group members is an `ORPHANED_CHILDREN` failure, not PASS.
Command receipts distinguish EXITED, TIMEOUT (124), CANCELLED (130),
ORPHANED_CHILDREN (125) and LAUNCH_ERROR (127), and record cleanup disposition.
Unrelated process groups are not targeted. Commands must not daemonize or
escape the owned process group; this mechanism is not a security container.
Native Windows execution fails before launch because tree containment there
is not implemented; configuration/inspection/dry-run remain available.

Failed/interrupted attempts retain their RTL snapshots even without
`--keep-build`. Successful runs honor the existing retention option. Timeout
does not permit interpreting a later orphan's output as the original command's
success.

## Regression coverage

Tests cover interleaved tile returns with stale labels, variable runtime lengths,
wrong source/destination/length, missing and duplicate events, timestamp units,
binary UART text, unsupported multi-DAG scope, strict fallback, valid-bit
errors, exact startup scopes and negative cases, independent build/runtime
budgets, failed-snapshot retention, missing executables, descendant cleanup and
SIGTERM-resistant grandchildren. A fresh selected-backend RTL run is still
required; unit tests do not establish new hardware semantics.
