# A32 R905: SCH scalar redirect LSU and SerDiv shamt alignment

Date: 2026-08-15

## Scope and acceptance rule

This round intentionally prioritizes PDSCHDag2/SCH.  CCH was not used to tune
either change and was not rerun as an aggregate acceptance signal.  The
acceptance evidence is structural RTL correspondence, directed lifecycle
timing, byte-exact functional regression, and independent CPU/LSU/VFU gates.
The small full-DAG residual is reported but is not treated as proof of complete
modeling.

## Accepted structural changes

### Taken-control target LSU launch

The task3 scalar oracle aligned 3,643 consecutive non-memory retire events
from the first repeated REM loop through the next Venus boundary.  It showed
that the first loop was already exact, while the second loop accumulated one
cycle at every taken `BLT -> LHU` edge.  Gem5 issued the target load but waited
an extra Minor-FU service cycle before launching it into the scalar LSU.

`execute.cc` now invokes the existing same-edge scalar-LSU launch check after
any successful issue, including a taken-control target.  The helper still
requires an in-order memory head, a ready LSQ, the correct memory-FU boundary,
and no stream change.  There is no task, PC, opcode-value, address, or data
special case.

This moves SCH task3 from `+5.527 us` to `+4.141 us` before the SerDiv change.
Sequences 0--2 then have exact normalized fire/recycle timing.

### SerDiv input shift and data-dependent latency

The task3 sequence25/26 oracle first proved that operand admission was already
aligned: all lanes accepted the VDIV at fire plus four tile cycles, exactly as
RTL.  The old Gem5 model nevertheless completed long lanes 71--86 cycles late.
The cause was an unverified rule that multiplied per-word iterative latency by
two when the lane vector exceeded the four-word operand FIFO.

That rule is not present in RTL.  `venus_serdiv_wrapper` uses the four-entry
operand FIFO for admission and a separate two-entry result FIFO for output
backpressure; neither multiplies `venus_serdiv` arithmetic latency.  Removing
the multiplier makes task3 sequence25/26 VDIV durations `111/193` cycles versus
RTL `109/192`, instead of `191/355`.

The shift-zero control alone exposed the opposite task4 result: its VDIVs
became far too fast.  A direct packet oracle then proved the structural
difference:

- task3 sequence25/26: `vfu_shamt=0`;
- task4 sequence8/9: `vfu_shamt=6`.

RTL sign- or zero-extends each EW8/EW16 dividend into the 32-bit `serdiv_t`
domain, shifts it left by `cau_mul_shamt_i`, and only then performs the LZCs
that determine iterative duration.  Gem5 functional execution already used
`vfu_shamt`, but its timing calculation omitted it.  The timing path now
mirrors the RTL transformation before computing each element's divider cycles.
Operand/result queue capacity remains modeled independently.

Task4 sequence8 is now exactly `556/556` cycles and sequence9 is
`1130/1131` Gem5/RTL cycles.  The former unrelated vector-length multiplier is
gone.

## Current SCH timing

The per-task boundary is RTL `start execute -> execute complete` versus Gem5
`tile_start -> task_epilogue`; return DMA and tile release are excluded.

| task | RTL (us) | Gem5 (us) | delta (us) | error |
|---:|---:|---:|---:|---:|
| 0 | 676.287 | 676.292 | +0.005 | +0.000739% |
| 1 | 79.131 | 79.134 | +0.003 | +0.003791% |
| 2 | 55.227 | 54.896 | -0.331 | -0.599345% |
| 3 | 471.571 | 471.792 | +0.221 | +0.046865% |
| 4 | 30.175 | 30.422 | +0.247 | +0.818558% |
| 5 | 11.059 | 10.984 | -0.075 | -0.678181% |
| 6 | 3.795 | 3.782 | -0.013 | -0.342556% |
| 7 | 1.059 | 1.050 | -0.009 | -0.849858% |
| 8 | 330.955 | 330.968 | +0.013 | +0.003928% |

The like-for-like first-start to last-epilogue envelope is RTL
`1534.187 us`, Gem5 `1534.240 us`, delta `+0.053 us` (`+0.003455%`).  The
legacy first-allocation to last-release Gem5 boundary is `1535.108 us`; when
compared with the historical RTL execution envelope it gives `+0.921 us`
(`+0.060032%`).  These boundaries must not be mixed when claiming accuracy.

Relative to the post-scalar-fix candidate, task3 improves by `3.920 us`, task4
by `0.696 us`, and the execution envelope by `4.616 us`.  This is not task
cancellation: both positive SerDiv-dominated task residuals shrink
independently.

## Functional and directed gates

- SCH: 9/9 tasks and 22/22 returned ports byte-exact to the accepted run.
- SCH Gem5 VINS: 8,103/8,103 byte-exact to the accepted run.  Direct RTL VINS
  coverage remains 7,256; the other 847 instructions are not presented as RTL
  comparisons.
- scalar600 runner: 52/52.  The valid RTL oracle has 1,258 retire and 1,258
  writeback events exact.  The suite-level 51/52 count retains the historical
  `ecall_nop` task-exit-hook exception.
- vector LSU: 68/68 and 382/382 VINS exact.
- BitALU, CAU, and SerDiv result queues all reach depth two and a real full
  condition; 1/4/64 ns grant-gap outputs are exact to control.
- Tested binary SHA-256:
  `3eda8e317384984bcd7305a2cb4b8c9908fab78c6b114428f439f698a0aebe7f`.

## Remaining independent SCH gaps

The model is still not complete despite the `+0.003455%` execution envelope:

1. Task3 ends with vector recycle `+116` cycles.  Recurring VSTORE durations
   remain `+10/+14/+15` cycles, followed by smaller dependent CAU/SerDiv/VRF
   waves.  Its first fire difference remains sequence3 VLOAD `-2` cycles and
   first duration difference sequence6 VLOAD `+1` cycle.
2. Task4 ends with vector recycle `+130` cycles.  Its remaining repeated
   VSTORE gaps are `+21/+27/+39/+41` cycles and VLOAD gaps `+31/+33` cycles.
   The first lifecycle difference is sequence7 VSADD recycle `-1` cycle;
   sequence12 VMUL fire is also `-1`.
3. Task2 remains `-331 ns`, and task5 remains `-75 ns`.  These are independent
   negative residuals and must not be used to cancel task3/task4.
4. Short task6/task7 percentage errors come from `-13/-9 ns` absolute gaps;
   they do not justify changing a vector fixed latency.

The next SCH target is therefore the repeated task4/task3 VSTORE/VLOAD
multi-beat lifecycle: separate operand admission, addrgen capture, AXI beat and
WLAST completion, per-bank VRF grant, and final retirement.  SerDiv fixed or
data-dependent latency should remain unchanged unless a new direct oracle
contradicts the current sequence8/9 and sequence25/26 controls.

Machine-readable evidence is in
`evidence/a32_r905_sch_serdiv_shamt_20260815/`.
