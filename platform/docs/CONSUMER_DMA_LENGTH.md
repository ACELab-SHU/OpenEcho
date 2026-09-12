# Venus1 ordinary dependency DMA lengths

For Venus1 profiles (`venus1p0-64x512` and its legacy-bpll / 300mhz variants),
ordinary type-0 dependency transfers follow the RTL DMT `input_len[15:0]`.
Both the fast functional path and v5 L1 timing path use
`producer_runtime_vreturn_bytes & 0xffff`. They do not copy a producer allocation
bound or a consumer vector capacity, clamp payload to either hint, or zero-fill
the untouched receiver tail. Producer return DMA itself keeps its runtime length.

The RTL task descriptor contains parent, port and destination, but no consumer
length. `task_manager.sv` publishes return size into the 16-bit DMT field and
uses that field for type-0 fire `src_size`. Static/global inputs and pointer
contracts are unchanged. This is a transport semantic repair, not a claim of
complete allocator, scheduler-firmware or cycle equivalence to the whole SoC.

The DSL retains `all_output.length` / `allocation_bytes` as producer allocation
metadata. Signature-derived `consumer_capacity_bytes`, argument index/type and
legacy `length=min(producer bound, consumer capacity)` remain advisory metadata
for Venus1; other hardware profiles retain their previous transfer behavior.
An unresolved or ambiguous signature/address binding produces a warning and
omits the advisory annotation. It does not block an otherwise compilable DAG.

For Venus1, a return exceeding a static producer or consumer estimate is a
warning. The platform preserves the actual data and does not silently clamp it.
A return above 65535 warns about the RTL 16-bit truncation and reports the
actual transfer size. These warnings do not certify memory safety: static
annotations alone cannot prove that adjacent live state is safe to overwrite.
Malformed structural descriptors, absent actual returns and simulator memory
access failures remain errors; lowering their severity would hide failed runs.
Legacy profiles retain their capacity checks.

PDSCH example: producer allocation hint 15600, consumer vector 6148, runtime
return 172 bytes. Both Gem5 paths now transfer 172 bytes. The original RTL unit
probe confirms 172. With an aligned 512-bit bus this is three data beats with
44 valid bytes in the final beat; it does not imply three total clock cycles.

Regression coverage includes Python policy tests, all eight Venus1 DAG fast
runs, a v5 PCFICH timing-engine probe and `tests/rtl/dmt_length_tb.sv` against
unmodified Venus1 task_manager RTL. The RTL probe forces isolated FSM inputs;
it is not a complete DAG/SoC simulation or numerical/cycle qualification.
