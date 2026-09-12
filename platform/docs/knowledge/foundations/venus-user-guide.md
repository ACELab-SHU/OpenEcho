# Venus source and user-guide routing

Use evidence labels on every rule: `HEADER`, `COMPILER`, `RTL`, `PROBE`,
`INFERENCE`, or `UNVERIFIED`. Never turn an inference into a semantic fact.

## Active sources

- `HEADER`: `components/toolchain/dsl/venus_test/venus.h`,
  `venusbuiltin.h`, and generated `venustype.h`.
- `COMPILER`: the compiler configured by the selected backend manifest and the
  emitted assembly/map files for the current candidate.
- `PROBE`: focused cases under `workloads/5g_lite/tasks/`, especially pointer,
  asymmetric-vector, broadcast, shuffle, LSU, and scalar ABI probes.
- `RTL`: the immutable RTL repository and commit recorded by the run. RTL may
  explain deployment behavior but does not by itself expose a supported C API.
- `PROBE`: Gem5/RTL paired regressions with identical firmware and case digest.

## Core programming rules

- Treat a declared extended-vector length as capacity, not physical lane
  count. Compute row use from the active backend manifest.
- Pass active element length to each intrinsic and keep i8/i16 byte lengths
  distinct at `vreturn`.
- Use continuous `vload`/`vstore`; register shuffle is not a gather/scatter
  memory operation.
- Use i16 shuffle indices. Keep vector-vector operands at the same declared
  vector type even when their active lengths differ.
- Track mask and CSR state explicitly. Initialize destinations before masked
  updates and restore fixed-point CSR defaults before return.
- Record BAS/source, ELF VMA, combined-bin offset, runtime LSU address,
  descriptor address, and temporary offset as different address domains.
- Require an interval audit before using initialized task-local static/global
  data.
- Align workspace regions to the backend declaration and enforce the complete
  DAG workspace limit.

## Address domains, transfer units, and memory maps

- `COMPILER`/`PROBE`: Keep BAS ordinal, ELF symbol VMA, combined-image file
  offset, runtime LDU/STU address, scalar virtual address, descriptor-storage
  address, temporary-pool offset, and VSPM/VRF address as separate domains.
  Record an observed conversion for each pointer class; never infer a fixed
  bias from one payload or from similar numeric values in different domains.
- `HEADER`: The final `vload`/`vstore` length is an element count for the
  vector element type, while addresses and `vreturn` lengths are bytes. For
  example, a 16 KiB `i16` payload is 8192 load/store elements and 16384 return
  bytes. Passing 16384 as the `i16` load length requests 32 KiB.
- `RTL`/`PROBE`: A backend's physical VRF capacity is independent of shared
  SRAM capacity. If an oversized transfer wraps a physical row index and a
  later part overwrites an earlier part, classify the candidate as an ABI or
  length failure. Do not reinterpret the resulting displaced data as an LSU
  address translation.
- `COMPILER`/`PROBE`: A descriptor payload offset accepted by Venus LDU/STU is
  not automatically a scalar C pointer. Scalar access to external shared L2
  requires an explicit backend/adapter mapping and a focused positive and
  negative probe. Until then, keep payload-sized work on vector LDU/STU paths.
- `CONTRACT`: Absolute bases, capacities, aliases and requester-specific
  translations are backend- and hardware-digest-scoped facts. Put them in the
  backend manifest or a scoped empirical profile; never promote them as
  generation-independent Venus syntax.

### Scoped Venus2.0 example at RTL `bf82d804`

The following values are evidence for backend `venus2p0-16x128` with RTL
commit `bf82d804968672e04db31d105876092d78969b0e` and active configuration
SHA-256 `1eaadb9cbe1720108245dcbcad2b1e77e82651e288747ef3e32dd11ff5d18d97`.
They must be revalidated after a backend, configuration, adapter or RTL digest
change.

| Address domain | Range or size | Meaning |
| --- | --- | --- |
| cluster-local program SRAM | `0x80000000..0x80007fff`, 32 KiB | scalar program storage |
| cluster-local shared SRAM | `0x80008000..0x80087fff`, 512 KiB | configured hardware shared storage |
| Gem5 SharedL2 aperture | `0x80000000..0x81ffffff`, 32 MiB | modeled logical aperture/backing, not hardware SRAM capacity |
| tile0 ISPM | `0x82000000..0x82007fff`, 32 KiB | task instructions |
| tile0 DSPM | `0x82020000..0x82023fff`, 16 KiB | descriptors and scalar-local data |
| tile0 VSPM/VRF data | base `0x82100000`, 16 KiB | vector data storage/return source |

For the PBCH FEP replay that exposed this distinction, the combined image was
`0x10080` bytes: task text `0x0000..0x247f`, task data
`0x2480..0x24bf`, input IQ `0x8000..0xbfff`, configuration
`0xc000..0xc03f`, and twiddles `0xc040..0x1003f`, followed by alignment
padding. This is a workload instance, not a fixed DAG layout.

The failed diagnostic passed 16384 to an `i16` `vload` into an 8192-element
vector. The 32 KiB request wrapped the 16 KiB physical VRF, so its second half
overwrote the first. That explains why a load at combined offset `0x8000`
appeared to return the `0xc000` slice, and why a biased load at `0x4000`
appeared to return the original `0x8000` input. There is no evidence for a
general `+0x4000` LDU translation; the `-0x4000` workaround is rejected.

The active memory wrapper and AXI fabric place shared SRAM at `0x80008000`,
while older SoC package constants still name `0x80020000`. The backend
manifest must resolve this authority conflict before RTL qualification. Do not
select either value by convention or silently change an approved workload's
backend digest.

## `vbarrier`, `vaddr`, and scalar VSPM access

- `HEADER`/`RTL`: `vbarrier()` emits ten scalar NOPs followed by the Venus
  barrier instruction. It stalls the scalar pipeline until earlier Venus work
  has drained and the vector extension is observed idle. It is a
  vector-to-scalar visibility boundary, not a general-purpose separator for
  ordinary vector data dependencies.
- `PROBE`: The standard production use is immediately before scalar access to
  a vector through `vaddr()`, with the scalar access bracketed by
  `VSPM_OPEN()` and `VSPM_CLOSE()`:

  ```c
  vbarrier();
  VSPM_OPEN();
  volatile short *p = (volatile short *)(intptr_t)vaddr(value);
  scalar = p[0];
  VSPM_CLOSE();
  ```

  Place the barrier before `VSPM_OPEN()`. Keep the open/close region minimal
  and use volatile scalar accesses.
- `COMPILER`/`RTL`: Do not insert `vbarrier()` between pure vector operations
  merely to protect an ordinary RAW/WAR/WAW dependency; the Venus dependency
  machinery owns those hazards. Every barrier must name a scalar/VSPM
  consumer or a focused backend workaround. Any other use requires a positive
  Gem5/RTL probe and a documented applicability boundary.
- `PROBE`: A barrier drains useful overlap and includes fixed scalar/RTL guard
  overhead. Barrier count and placement are therefore part of both the static
  correctness audit and the measured performance review.

## Task outputs and 64-byte transfers

- `RTL`: Venus2 exposes up to 16 task return entries. Each entry contains an
  independent return address and 32-bit byte length; that length is propagated
  to the Scheduler DMA request.
- `RTL`: The current DMA datapath uses 64-byte AXI beats and supports
  multi-beat requests. Therefore 64 bytes is a transport granularity, not the
  capacity of one task output port.
- `PROBE`: The current Venus2.0 return path did not transport a final partial
  beat in paired Gem5/RTL probes: requested lengths 96, 288 and 864 bytes were
  observed as 64, 256 and 832 bytes. Until a newer backend has its own positive
  narrow-tail probe, make every RTL-bound `vreturn` length a multiple of 64
  bytes and initialize every padding byte deterministically.
- `COMPILER`/`RTL`: Treat output count, exact `vreturn` byte length, temporary
  allocation/alignment, downstream input-length width, and DMA beat size as
  different contracts. Never derive one from another.
- `COMPILER`: Keep a semantic tensor in one output when its length is accepted
  by the active DSL/backend. Split it and use BAS `concat(...)` only for a
  semantic layout boundary, a focused proven backend restriction, or a
  measured schedule benefit—not merely because 64-byte DMA beats are visible.
- `RTL` evidence: `hardware/L2_scheduler/l2_scheduler_pkg.sv` defines 16
  returns and address/size fields; `venus_tile_manager.sv` forwards each
  return-length register; `dma_streamer.sv` chunks large requests. Existing
  generated DAG metadata and paired probes contain individual outputs larger
  than one beat, including 128, 320, 512 and 896 bytes.

## Multi-row values and descriptor order

- `PROBE`: On Venus2.0, `vaddr()` exposes a scalar view of one physical VRF
  row. Do not assume that `vaddr(vector) + offset` reaches later rows of an
  extended vector. Keep multi-row movement in vector operations, or cross a
  task boundary as explicit row-sized ports whose order and length are audited.
- `PROBE`: Scheduler return descriptors use the canonical generated port
  order. Do not shift a descriptor to compensate for an assumed one-port
  response lag. Such a shift duplicated the last destination and aliased two
  outputs in the CCH probe; the unshifted mapping produced 61/61 exact RTL
  outputs.

## Deployment truth

Run independent compilation, full-matrix Gem5 validation (normally fast),
golden comparison, integrated revalidation, and fresh RTL. Use debug for
failure/divergence diagnosis. VEMU is forbidden. When Gem5 and RTL disagree,
preserve the firmware and inputs, reduce to a syntax/ABI probe, and treat the
RTL result as the target deployment fact. Propose reversible Gem5, linter, and
documentation changes; never edit RTL.

## Performance guidance

Venus high-performance tasks are vector-first. A scalar implementation may be
used to establish a golden, a correctness baseline, small control/setup code,
or an unavoidable irregular tail, but payload-sized arithmetic loops over
`vaddr()` pointers are not a completed high-performance implementation while a
plausible vector mapping remains unexplored. Prefer long useful vectors,
continuous transfers, masks/shuffles/reductions, cross-antenna or cross-symbol
batching, short live ranges, delayed claims, stage-local CSR state, reusable
temporaries, static precomputation, and bit-exact fusion.

Record the emitted and dynamic scalar/vector instruction mix for hot tasks. A
kernel remains scalar even if vector instructions only allocate, zero, copy or
return its buffers. Performance transformations are `INFERENCE` hypotheses
until bit-exact same-key measurements show an improvement. Analyze complete
schedules and emitted instructions; C line count is not performance evidence.
