# Venus task and DAG development contract

This is the ACE-ECHO adaptation of the reviewed Venus operator-porting
knowledge. It preserves the language, ABI, planning, correctness, and
performance gates while replacing every software execution step with
independent compilation and Gem5. Architecture quantities and supported
features always come from the selected backend manifest and active sources.

## Evidence before generation

Read the active generated `venus.h`, `venusbuiltin.h`, vector type headers,
compiler configuration, emitted assembly/map format, selected backend
manifest, and its foundation document. Use evidence labels `HEADER`,
`COMPILER`, `RTL`, `PROBE`, `INFERENCE`, and `UNVERIFIED`.

Evidence priority is:

```text
active headers/compiler and backend contract
focused positive/negative probes
bit-exact workload examples for the same backend and ABI
read-only RTL explanation of deployment behavior
uncategorized workload corpus
```

Existing tasks are syntax and scheduling hypotheses, not semantic truth. Never
infer signedness, overflow/saturation, rounding, shift behavior, operand order,
pointer encoding, output order, or output length by majority vote.

## Freeze the operator contract

Before Venus source exists, record:

- input/output shapes, strides, element widths, signedness, complex layout,
  aliasing and in-place behavior;
- intermediate and accumulator widths, integer promotions, Q formats, shifts,
  rounding/tie rules, saturation or wrap behavior;
- branch, tail, early-return, error and CRC-failure behavior;
- C task argument order and representation, BAS call order, output order, and
  every `vreturn` byte length;
- generic shape domain or explicit case-specialization predicates and
  fallback/reject behavior;
- the one authoritative golden and deterministic case digests.

If any bit-affecting field is unresolved, return `BLOCKED`; do not let code
generation invent it.

## Public programming model

Extended-vector length is logical capacity, not physical lane count. The final
intrinsic argument is normally active elements; inspect the active macro before
using an uncommon operation. Common forms are:

```c
vclaim(dst, active_elements);
vrange(index_i16, active_elements);
vbrdcst(dst, scalar, MASKREAD_OFF, active_elements);
vload(dst, byte_address, MASKREAD_OFF, active_elements);
vstore(byte_address, src, MASKREAD_OFF, active_elements);

dst = vadd(a, b, mask_read, active_elements);
vseq(a, b, mask_read, MASKWRITE_ON, active_elements);
vshuffle(dst, index_i16, src, SHUFFLE_GATHER, active_elements);

vreturn(out0, byte_length0, out1, byte_length1);
```

Rules:

- vector-vector operands use the same declared vector type;
- shuffle is an in-register rearrangement, uses an i16 index, and is not a
  non-contiguous memory operation;
- public `vload`/`vstore` are continuous byte-addressed transfers unless the
  active backend explicitly exposes and validates another API;
- i16 active length counts elements, while address increments, allocation and
  `vreturn` lengths count bytes;
- comparison with `MASKWRITE_ON` changes implicit state; initialize the full
  destination before a masked update and keep mask production adjacent to its
  consumer;
- scope `vsetshamt`, saturation and other CSR changes to a documented stage and
  restore the backend default on every exit path;
- reduction valid lanes, fused-operation operand roles, non-commutative
  operand order, multiply/shift semantics and multi-output mappings require a
  focused golden probe unless active authoritative evidence already fixes them;
- avoid hard-coded DSPM/VSPM/scratch addresses, initialized task-global tables,
  and barrier workarounds without an explicit backend contract and regression.

For RTL-bound candidates, a successful Gem5 access to task-local DSPM is not
deployment evidence. Reject a payload-sized vector round trip through an
absolute tile-local address unless a focused paired Gem5/fresh-RTL probe for
the selected backend and RTL digest proves the exact load/store path. Prefer
keeping intermediates in vector registers. When register pressure requires a
spill, use a manifest-qualified shared workspace with aligned, disjoint ranges
for concurrently runnable tasks, explicit producer/consumer lifetimes, and
full-output Gem5 validation (normally fast) before the integrated fresh-RTL run. Never
generalize a working shared-workspace address to another backend.

## `vbarrier` placement contract

Use `vbarrier()` by default only at a vector-to-scalar VSPM visibility
boundary. The following form is a probe protocol, not proof of portable
scalar/vector coherence:

```c
vbarrier();
VSPM_OPEN();
volatile T *p = (volatile T *)(intptr_t)vaddr(vector_value);
/* bounded scalar access */
VSPM_CLOSE();
```

The barrier must precede `VSPM_OPEN()`, and the open/close interval must contain
only the bounded scalar access that requires it. Do not insert a barrier
between pure Venus vector operations for normal register dependencies. The
vector scheduler/hazard machinery owns those dependencies; forcing the unit
idle destroys overlap and adds the header NOPs plus RTL guard latency.

For an RTL-bound product, reject this scalar view for vector ABI inputs,
temporaries, and return values unless a focused paired Gem5/fresh-RTL probe
qualifies the exact backend and RTL digest. Gem5 success is insufficient: an
observed Venus2.0 case returned unknown scalar data for a vector ABI input even
though the vector descriptor DMA completed. Prefer pure vector flow or
generated scalar constants.

Statically classify every barrier as `vaddr_scalar_access` or
`backend_workaround`. A workaround requires a focused positive/negative
Gem5/RTL probe, an applicability boundary, and an adjacent source comment. An
unclassified barrier rejects a high-performance candidate until it is removed
or justified.

## ABI and address domains

The C signature, BAS positional call, generated descriptors and `vreturn`
value/length pairs are one ABI. `vreturn` lengths are bytes. Enforce the
backend's register-argument count and reject stack arguments when unsupported;
confirm in disassembly, not only source.

Keep these domains separate for every pointer class:

```text
BAS/source ordinal
ELF symbol VMA
combined-image file offset
runtime payload/LSU address
descriptor storage address
temporary-pool offset
```

The same BAS `&name` spelling does not prove one descriptor encoding. Validate
parameter, external input, temporary output, shared workspace and return
pointers independently with generated metadata and an observed load/store or
return address. A BAS initializer is not automatically the runtime task input;
use the consumer-entry payload or a verified upstream output.

Use only backend-supported logical input types for RTL-bound DAGs. Do not
truncate a logical type to fit a physical descriptor. A `vreturn` pair carries
one output address and its byte length; do not confuse the DMA beat/alignment
size with a per-port payload cap. Derive the output-count limit, return-length
width, alignment and any real payload limit independently from the active
backend. Keep one semantic tensor in one output when it fits those contracts;
use multiple ports or BAS `concat(...)` only for a semantic partition, an
independently verified backend restriction, or a measured DAG-level benefit.
Do not change Scheduler packing to make a workload pass.

For the current Venus2.0 backend, paired Gem5/RTL probes show two additional
deployment constraints. First, the return path drops a final partial 64-byte
beat, so every `vreturn` transport length must be rounded up to a whole beat and
the padding must be initialized before return. This is not a 64-byte per-port
limit: multi-beat outputs are supported and should remain one semantic output
when practical. Second, the generated Scheduler descriptor order is canonical;
do not apply a speculative response-lag rotation. Always prove descriptor
destination uniqueness and producer-port order from the staged payload before
RTL.

## Vector-first implementation rule

A scalar reference is useful for freezing semantics, producing a golden and
bringing up the first correctness probe, but it is not a high-performance
Venus implementation. In a performance candidate, map payload-sized arithmetic
and regular memory work to Venus vector instructions by default. In particular,
do not implement channel estimation, complex multiply, filtering, combining,
equalization, clipping, de/scrambling, rate matching, or decoder inner loops as
large scalar loops over pointers returned by `vaddr()` when the active ISA can
express the work in vectors.

Scalar code is acceptable for small configuration/control decisions, address
setup, bounded irregular tails, or an operation for which active ISA/compiler
evidence shows no viable vector formulation. Document that exception and
measure it. If substantial payload work remains scalar, label the candidate a
correctness baseline or fallback rather than a high-performance result, and
continue searching vector layouts, batching, masks, shuffles, reductions,
fusion, or stage decomposition.

Audit emitted code and dynamic execution, not just C spelling. Record useful
active vector lengths, dynamic scalar and Venus instruction counts, transfers,
and cycles for the hot stage. A few setup/zero/copy vector instructions around
a scalar kernel do not make the kernel vectorized.

## Row and workspace planning

Derive physical row bytes from the manifest:

```text
row_bytes = lanes * banks_per_lane * bank_width_bits / 8
rows(vector_bits) = ceil(vector_bits / (8 * row_bytes))
```

For every stage record inputs, outputs, temporaries, declared capacities,
active lengths, peak simultaneously-live rows (including function inputs,
retained outputs and compiler temporaries), mask/CSR in/out, and continuous
memory transfers. Peak live rows must not exceed the backend row count.

`vaddr()` is not a promise that all rows of an extended vector are physically
contiguous in scalar address space. Scalar dereference is restricted to the
first physical row unless an active backend probe establishes more. For a
multi-row task boundary, prefer explicit row-sized inputs/outputs or
type-compatible vector shuffles; never recover later rows with pointer
arithmetic alone.

Delay claims and definitions until first use, shorten live ranges, reuse dead
temporaries, and spill only at auditable stage boundaries before reducing
useful vector length. Workspace regions use manifest alignment, include
producer/consumer/lifetime and relative plus absolute intervals, do not
overlap, and stay within the whole-DAG limit. Initialized static/global data
requires task-map, variable-map and combined-image non-overlap evidence.

## Candidate gates

Generate candidates in immutable run attempts before integration. Each
candidate has a source-to-plan map, ABI manifest, workspace layout, estimated
stage live-row peak, specialization guards, and unresolved-warning list.

Apply gates in order:

```text
contract and golden frozen
static syntax/type/ABI/address/row/workspace/mask/CSR audit
independent compile and emitted assembly/map audit
Gem5 task or DAG execution
all required outputs bit-exact
same-key measured performance
integrated full-matrix Gem5 validation and output comparison (normally fast)
fresh immutable RTL qualification
```

A zero exit code, successful compilation, final CRC alone, or reduced C/VINS
count is not correctness. Normalize integer values by their declared bit width
and require exact length and every bit. Report the first mismatch and strongest
proven scope: task output, full-DAG task boundary, downstream completion, final
DAG output, or RTL all-visible-output exact.

## Performance loop

Start from a correct measured baseline, then establish a vectorized hot-path
candidate before claiming high-performance completion. Prefer long useful
active lengths and continuous transfers when row pressure permits; physical
lane count is not the default tile length. Inspect emitted instructions, stage
liveness, scalar-versus-vector dynamic instruction mix, LSU traffic,
queue/functional-unit behavior and Gem5 timing. Change one falsifiable
hypothesis per child candidate, then repeat every static/build/correctness gate.

Structural evidence (active length, dynamic instruction families, transfer
counts, workspace, live rows) may explain a hypothesis but cannot claim a
cycle speedup. Rank only bit-exact candidates with the same backend/hardware,
case digest, build, execution scope, metric definition and clock domain. Replay
performance does not prove integrated/full-DAG performance; remeasure at the
claimed scope.

Keep failed candidates as compact counterexamples while the run is active.
After an application-complete PASS, retain the baseline, winner, RTL-qualified
candidates and top correct alternatives, and prune only allowlisted heavy
artifacts.
