# Validation coverage contract

ACE-ECHO reports only the strongest coverage actually executed.

| Coverage value | Meaning |
| --- | --- |
| `compiled_dag_artifacts` | Compiler produced captured JSON/BIN/task images; nothing executed |
| `scheduler_l1_artifacts` | Scheduler `l1.elf/l1.bin` built in isolation |
| `gem5_single_task_execution` | One task executed in Gem5; correctness still requires a compared golden |
| `gem5_dag_scheduler_model` | A DAG executed with the Gem5 Scheduler/DMA model |
| `gem5_scheduler_contract_model` | Scheduler ELF runtime contract decoded and executed; firmware itself did not execute |
| `planned` | Dry run only |
| `unproven` | Failure or missing evidence |

A successful simulation is not automatically bit-exact correctness. A
correctness claim requires an explicit `ace-echo compare` result against a
trusted MATLAB, scalar, frozen hardware, or independently validated golden.

## Gates for full application validation

The future `gem5_scheduler_firmware_execution` level requires all of:

1. Scheduler CPU firmware starts from the real reset vector.
2. Interrupt, WFI/wake, CSR and peripheral behavior is represented.
3. The firmware consumes the same `l1.bin`/DAG/input payload as hardware.
4. DMA and task dispatch originate from firmware-visible state, not a decoded
   schedule injected by the host.
5. Final application outputs are compared bit-exactly.
6. Per-task, full-DAG, and application timing scopes are separately reported.

