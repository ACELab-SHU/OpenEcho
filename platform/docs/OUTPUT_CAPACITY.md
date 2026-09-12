# Output capacity is a hard safety gate

Three quantities must remain separate: BAS declared bytes, the compiler's
occupied allocation, and the task's `vreturn` transport bytes. A vector's C
`sizeof` is not proof that the DAG reserved that many bytes.

`compile dag` exports `all_output[].allocation_bytes` from the DSL temporary
memory map. It joins variable name, offset and raw size exactly and uses the
allocator's recorded row-exclusive granularity. It preserves `length`, offsets,
the binary and the original memory map. No neighbouring gap or observed X/data
pattern is used to infer capacity. The legacy `allocated_size_bytes` map field
is unrounded; occupied bytes are rounded using `row_bytes`.

The configured compiler ABI (`abi.compiled_return_registers`) identifies return
count/length register stores in emitted `.0.ll`. This checks constant lengths
after macro and `sizeof` expansion, on all emitted paths including rejection
branches. For the shipped Venus2 compiler, `EmitRISCVVenusRetExpr` writes count
at CR+0x2c, lengths at CR+0x34+8*port; this backend uses CR=0x801ff000.
These addresses are profile data, not universal hardware assumptions.

- `OUTPUT_CAPACITY_EXCEEDED`: **ERROR**, nonzero CLI exit. Compilation or
  observed software validation fails; the RTL preflight rejects the handoff
  before snapshot/build. Valid-bit layouts never waive this error.
- `OUTPUT_COUNT_MISMATCH`: **ERROR**; no silent missing/extra return ports.
- `OUTPUT_LENGTH_DIFFERS`: **WARNING** when transport differs from declared
  bytes but fits the proven allocation. This does not establish numerical
  correctness or prove padding was initialized.
- `OUTPUT_EMPTY_RETURN`: **WARNING** for an emitted zero-length error return;
  zero is not an overrun. The requested case's output/coverage gates still apply.
- `RETURN_LENGTH_UNRESOLVED`: **WARNING**, static status `PENDING_RUNTIME`,
  never a proof of safety. Dynamic values, unavailable IR or unconfigured ABI
  require observed validation before hardware handoff.

Gem5 task/DAG runs validate observed output sizes against compiled metadata.
Single-DAG firmware runs use the decoder's digest-bound staged JSON. Firmware
without that allocation metadata explicitly reports pending validation. These
checks cover the observed invocation only, not unexecuted branches or an entire
multi-DAG invocation history. They do not alter Gem5 instruction semantics or
stop an overrun mid-instruction; they prevent it being accepted or sent to RTL.
The RTL adapter rechecks supplied Gem5 outputs before building, and native DMA
reconciliation enforces the same allocation capacity. Without exported capacity,
old metadata remains conservative: capacity equals declared length, with no
automatic rounding. Recompile legacy artifacts to obtain the allocation record.

Example: BAS `char result[128]` with occupied 128 B and `vreturn(result,4096)`
fails, even if only the first 128 bytes matter. An aligned struct with 2 B of
semantic data in a proven exclusive 64 B slot may return 64 B with a warning.
Semantic/padding comparison is independently controlled by explicit
[output validity layouts](OUTPUT_VALIDITY.md). AXI assertions and firmware
self-checks remain separate; this change neither edits RTL nor waives assertions.

Artifacts: `toolchain/output-capacity-report.json`, exported
`toolchain/temp_memory_map.json`, `gem5/output-capacity-report.json`, and
`output-capacity-preflight.json` in the RTL artifact directory.
