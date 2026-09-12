# Roadmap

## Available foundation

- Standalone owned Gem5 and Scheduler snapshots.
- Fast and verification execution profiles.
- Single-task and multi-task DAG runners.
- Scheduler build isolation and L1 runtime-contract runner.
- Immutable run records, command logs, source revisions and input hashes.
- Bit-exact raw/i8/i16 comparison.

## Next implementation milestones

1. Add a canonical deployment bundle schema shared by task, DAG and Scheduler
   builds; stop using ad-hoc case directories.
2. Execute Scheduler firmware in Gem5 instead of decoding its contract on the
   host; model reset, interrupts, WFI, MMIO and L1 DMA programming.
3. Add application manifests that can contain multiple DAG fires and host/RF
   input events.
4. Add golden-set registration and automatic final-output comparison.
5. Add optional RTL evidence import without any RTL runtime dependency.
6. Establish CI tiers: CLI/unit, task smoke, DAG regression, application
   regression, and separately triggered RTL conformance.
