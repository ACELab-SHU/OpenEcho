# Venus RTL DAG bring-up

This tree can now execute the scalar CPU plus one Venus tile.  The full RTL
DAG path is being added on top of that verified baseline.

## PDSCHDag2 format

`dag1.json` contains 15 real task descriptors (and one trailing empty entry).
For dependency inputs (`type == 0b00`), `parentTasksPort >> 4` is the parent
task ID and `parentTasksPort & 0xf` is its output port.  PDSCHDag2 starts tasks
0, 1, and 2 in parallel.

RTL task binaries start at PC 0 and use two address views:

- `0x00000000..0x001fffff`: tile-local scalar memory
- `0x80000000..0x801fffff`: the sign-extended/physical alias, including VRF

gem5 SE cannot use an executable entry value of exactly zero. The materializer
therefore starts at `0x4`, skipping only the first `addi x1, x0, 0`; SE already
initializes x1 to zero. The task body stays at its original RTL addresses, so
absolute function pointers and jump tables remain valid.

The JSON data field is a big-endian hexadecimal representation of a complete
DMA payload.  It must be byte-reversed before being written to increasing tile
addresses.

## Inspect and replay a task

```sh
python3 tools/venus_dag.py /path/to/PDSCHDag2_hw/dag1.json inspect

python3 tools/venus_dag.py /path/to/PDSCHDag2_hw/dag1.json materialize \
  --case-dir /path/to/PDSCHDag2_hw --task 0 --output-dir /tmp/pdsch-task0

VENUS_GEM5_TASK_EBREAK_EXIT=1 ./build/RISCV/gem5.debug \
  configs/tutorial/part1/packet_gen.py \
  --binary=/tmp/pdsch-task0/Task_nrPDSCHIndices.replay.elf \
  --venus-config=venus-rtl-16x128
```

`VENUS_GEM5_TASK_EBREAK_EXIT=1` suspends the scalar thread at the RTL task's
terminal `ebreak`. gem5 exits normally only after all Sequencer running IDs,
VFU queues, and pending LSU operations have drained, so stats and instruction
dumps include the complete task.

## Replay the DAG dependency graph

```sh
python3 tools/venus_dag.py /path/to/PDSCHDag2_hw/dag1.json run-dag \
  --case-dir /path/to/PDSCHDag2_hw \
  --run-dir /tmp/venus-pdsch-dag \
  --venus-config venus-rtl-16x512 --jobs 3
```

This first-stage L1 mode dispatches tasks by topological level and uses the
golden parent-input payloads embedded in the descriptor. Use `--tasks 0 1 2`
to bring up only the three initially-ready root tasks. Each task gets an
isolated directory containing its replay ELF, gem5 stats, console log, Venus
instruction dumps, and Sequencer monitor. `dag_replay_summary.json` is the
aggregate pass/fail and performance index.

Golden-input replay remains useful as a task-level diagnostic. It is not the
functional DAG path described below.

## Run one live DAG simulation

```sh
python3 tools/venus_dag.py /path/to/PDSCHDag2_hw/dag1.json \
  run-dag-inprocess --case-dir /path/to/PDSCHDag2_hw \
  --run-dir /tmp/venus-pdsch-live \
  --venus-config venus-rtl-16x128
```

This starts one gem5 process containing all task contexts plus
`VenusDagScheduler`. The scheduler mirrors the RTL L2 scheduler's externally
visible lifecycle:

1. dependency-ready scan in task-ID order;
2. tile allocation and input DMA from the shared DMT;
3. tile start and task execution;
4. drain all outstanding Venus work after the task epilogue;
5. output DMA into the DMT, dependency release, and tile release;
6. DAG completion only after every task has completed.

Dependency inputs are copied from the bytes actually produced by the parent,
not from the descriptor's golden replay payload. The generated
`venus_dag_trace.jsonl` records `task_ready`, `tile_allocated`, `dma_input`,
`tile_started`, `task_epilogue`, `dma_output`, `tile_released`, and
`dag_complete` events. Long vector drains also emit sparse `drain_progress`
records containing the live instruction count, pending LSU count, and per-VFU
queue occupancy; these distinguish slow execution from a persistent stall.

Every output DMA is also preserved byte-for-byte as
`task_<id>_port_<port>.bin` in the run directory. These dumps are generic
observability artifacts: their addresses and lengths come only from the DAG
descriptor, and they can be compared directly with data reconstructed from
the RTL L2 DMA transaction log.

The descriptor-driven prototype has been exercised through all 15 listed
tasks and exits normally at tick 628611000 with the RTL-derived VSPM mapping.
This is only a prototype lifecycle check: the reference RTL run creates nine
runtime task instances, so parsing the real L1 runtime graph is required before
the two executions can be called structurally or functionally equivalent.

The gc0802 RTL configuration is 16 lanes by 128 rows. gem5 therefore applies
the same `vrow_t` narrowing as RTL and the in-place `VSHUFFLE` scratch-row
relocation from `venus_sequencer.sv`. AVL is likewise narrowed to RTL's 15-bit
`vlen_t`, and instructions for which the dispatcher selects no lanes retire as
empty operations instead of occupying a running ID forever. Legacy
operand-overlap checks are only enabled when
`VENUS_GEM5_STRICT_OPERAND_OVERLAP=1` is set.

## Alignment stages

The current first stage deliberately uses one physical tile so flow and data
dependencies can be made functionally identical before concurrency affects
debugging. The next stages are:

1. finish instruction-by-instruction and output-by-output functional bring-up
   of all 15 PDSCHDag2 tasks;
2. add four independent gc0802 tile instances and RTL-compatible task/tile
   capability matching;
3. model L1 registration/interrupt-visible control state and explicit DMA
   timing without changing the proven DMT dataflow;
4. compare RTL and gem5 lifecycle/instruction traces;
5. only then calibrate DMA, memory, issue, and functional-unit latency for
   performance error reduction.

The real `l1.elf` is also retained as a control-plane reference.  It registers
the embedded DAG through the cluster window at `0x21ff0000`, transfers the DAG
image and descriptor tables by DMA, and waits for the L2 scheduler interrupt
to set `MUTEX_dag_done`.

## Runtime DAG from the L1 ELF

The post-synthesis JSON is not the graph executed by L2. Decode the packed
runtime graph (`task_container_content` in `l2_scheduler_pkg.sv`) directly from
the symbols embedded in the L1 ELF:

```sh
python3 tools/venus_l1_dag.py /path/to/l1.elf \
  --output-dir /tmp/venus_l1_runtime
```

An L1 ELF can contain zero-filled parameter slots which L1 initializes at
runtime. Until L1 itself runs inside the gem5 platform, an RTL DMA capture can
be supplied explicitly as a validation fixture:

```sh
python3 tools/venus_l1_dag.py /path/to/l1.elf \
  --rtl-dma-log /path/to/dma_read_data_file_L2.txt \
  --output-dir /tmp/venus_l1_runtime

VENUS_GEM5_DAG=1 build/RISCV/gem5.debug \
  --outdir=/tmp/venus_l1_runtime/m5out \
  configs/tutorial/part1/packet_gen.py \
  --dag-manifest=/tmp/venus_l1_runtime/venus_dag_manifest.json \
  --venus-config=venus2p0-16x128
```

The capture is input data only. It never changes task topology, instruction
behavior, output addresses, or output lengths. Those remain descriptor- and
task-program-driven. A graph containing `ptr_temp` inputs must provide real
L1/DMT allocation metadata; the model fails rather than inventing an address.

## Current RTL-aligned regression

Use the RTL hardware profile for both DAG and single-task work:

```sh
--venus-config=venus-rtl-16x128
```

This profile automatically enables RTL-style requester-local dependency
handling. No task or DAG environment switch is required. The older
instruction-global hazard gate is available only as a diagnostic with
`VENUS_GEM5_LEGACY_GLOBAL_HAZARDS=1`.

Validated results:

- PDSCHDag2: all nine runtime tasks complete; every RTL-known output byte
  passes `tools/compare_dag_dma.py`; modeled DAG duration is 899,974 cycles
  versus 910,115.5 RTL cycles (-1.11%).
- LDPC `test_1`: all 1,527 Venus instruction dumps pass
  `tools/compare_vins_outputs.py`; the vector instruction span is 23,401
  cycles versus 21,606 RTL cycles (+8.31%).

The remaining performance work is therefore cross-workload refinement, not
functional bring-up. In particular, gem5 still holds same-VFU dependent
instructions at the sequencer boundary because its operand data FIFOs do not
yet carry the RTL command tags needed to queue those dependencies safely.
