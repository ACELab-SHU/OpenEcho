# A32 R78 scalar600 CPU pipeline diagnosis

Date: 2026-08-10

Status: **diagnosis complete; timing candidates rejected; accepted R76 CPU
behavior restored**.

## Scope

This pass treated the scalar CPU as an independently validated component.  It
read the protected `scalar600_if_stage`, `scalar600_id_stage`,
`scalar600_ex_stage`, `scalar600_wb_stage`, controller, and LSU sources.  VEMU,
scheduler, and RTL sources were not modified.  No task, PC, DAG, address,
branch direction, or operand-value special case was introduced.

The current model is not an unmodified stock MinorCPU.  The RTL profile already
has single issue/retire, ID-resolved controls with EX bypass, in-order LSU
launch, response-driven load completion, and scalar600-specific WB boundaries.
Its remaining mismatch is that these contracts are layered on Minor's
decoupled fetch/decode/execute queues rather than represented as scalar600's
held IF/ID, ID/EX, and EX/WB registers plus the controller stall vector.

## Retire comparator correction

`tools/compare_scalar_retire.py` now accepts the leading whitespace that gem5
uses to align short tick values.  Before this correction, early task records
were silently omitted and the reported first divergence could point into the
task epilogue.

With the corrected parser, CCH task0 matches the RTL retirement timing for the
first 31 comparable boundaries.  The first real mismatch is the entry JAL
target:

- RTL `0x80 JAL -> 0x88 ADDI`: 3 tile cycles;
- gem5: 4 tile cycles;
- first divergent relative retirement: index 32, RTL cycle 34 versus gem5
  cycle 35.

## Task20 three-boundary experiment

The accepted R76 behavior and RTL reference are:

| boundary | RTL cycles | accepted gem5 cycles |
|---|---:|---:|
| `0x26c branch -> 0x270 load` | 6 | 6 |
| `0x270 load -> 0x274 dependent branch` | 2 | 4 |
| taken `0x274 branch -> 0x264 target` | 3 | 3 |

A dedicated one-EX-register control FU closed all 164 load-to-dependent-branch
edges to 2 cycles, but moved the bubble into the redirect path:

| boundary | candidate cycles | instances |
|---|---:|---:|
| branch -> load | 6 (one cold edge at 7) | 164 |
| load -> dependent branch | 2 | 164 |
| taken branch -> target | 6 | 101 |

The loop became one cycle slower per taken iteration.  The candidate was
rejected because it aligns one visible boundary by breaking another.

Two additional candidates were also rejected:

1. Reducing the generic Minor IntALU pipeline from three slots to two moved
   absolute retirement times but left task0 JAL-to-target at four cycles.
2. Collapsing the general Fetch1-to-Fetch2 latch to a zero-cycle latch stopped
   task0 retirement entirely.  Scalar600's combinational branch bus cannot be
   represented by globally removing a Minor forward latch.

No rejected candidate remains in the executable model.  The restored task0
smoke run reproduces the accepted first divergence.  Protected-tree audit:
`SCOPE_OK changes=0 protected=0`.

## Consequence for CCH and SCH

There is no accepted CPU timing change in R78, so CCH/SCH execution timing is
unchanged from the R77 execution-boundary evidence.  It would be misleading to
publish a new full-DAG number from either rejected candidate.

The next CPU change must model an explicit held-ID control token and a tagged
redirect-target token together.  Acceptance requires the three task20
boundaries above, the task0 JAL target, and the straight-line retire prefix to
match simultaneously before full CCH/SCH and VINS/LSU regression is run.
