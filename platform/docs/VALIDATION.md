# Bootstrap validation

Validated on 2026-08-16 using only files owned by the ACE-ECHO staging tree,
plus the configured external compiler installations.

| Check | Result |
| --- | --- |
| Python unit tests | 7/7 PASS |
| `ace-echo doctor` | PASS |
| Owned DSL frontend compile: `asym_vector_arith_probe` | PASS; JSON, combined BIN, and task image produced |
| Owned Gem5 single-task fast run | PASS; four 64-byte output ports and stats produced |
| Owned Gem5 DAG fast run | PASS; hydrated static inputs, dynamic `vreturn` capture, DAG trace and stats produced |
| Owned Scheduler isolated build | PASS; `l1.elf`, `l1.bin`, map, disassembly and payload directory produced |
| Scheduler-contract application run | PASS; 9 tasks, four runtime-assignable tiles, task outputs, DAG trace and stats produced |

The application run completed at Gem5 tick `1795360000`. This proves the
Scheduler runtime-contract backend, not execution of the Scheduler firmware
instruction stream. The latter remains a separately gated backend.

Run the same sequence with:

```bash
make smoke
```

## RTL evaluator preflight

Before an expensive fresh RTL build, run the read-only checks declared by the
selected backend manifest:

```bash
./ace-echo rtl-preflight \
  --backend configs/backends/venus2p0-16x128.json \
  --output runs/<run-id>/artifacts/rtl-preflight.json
```

A failed preflight blocks RTL qualification; it does not imply a Venus
software/hardware numerical mismatch. Preserve the function declaration and
call locations, source digests, RTL commit, firmware digest, and input-case
digest. Do not edit RTL or its evaluator to turn the failure into a pass.
