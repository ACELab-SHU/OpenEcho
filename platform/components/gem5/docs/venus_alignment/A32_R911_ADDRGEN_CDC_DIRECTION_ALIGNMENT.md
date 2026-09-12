# A32 R911 addrgen queue / W-CDC direction alignment

Date: 2026-08-15

## Verdict

`INCONCLUSIVE` for complete RTL equivalence. This round accepts one generic
LSU timing refinement: an opposite-direction descriptor is released from the
actual W-CDC source-pointer boundary, and a store descriptor which previously
entered addrgen's same-direction FIFO pays its additional pop/head D/Q path.

There is no task ID, DAG name, PC, instruction ordinal, address, payload, or
burst-length condition. VEMU, scheduler/L1 packaging, RTL, LSU fixed latency,
and VFU fixed latency were not changed.

## RTL structure and old-model defect

RTL `i_cdc_w` is an eight-entry Gray-pointer FIFO. Its source `ready` observes
the destination read pointer only after two synchronizer stages. RTL addrgen
has a four-entry descriptor FIFO: it may enqueue a descriptor behind a head of
the same direction, while an opposite-direction descriptor waits until that
head drains.

R909 modeled the W FIFO beat by beat, but collapsed the final direction
boundary to `sourceBackpressured ? 10 : 1` tile cycles. That constant could not
represent both observed states:

- SCH task5 sequence 7 is one 32-beat store. At local WLAST the modeled FIFO
  is 8/8 occupied, with the next returned source slot visible two tile cycles
  later. The following load needs eight cycles from WLAST, not ten.
- SCH task4 sequences 73--75 are three same-direction stores. Sequence 75 was
  already acknowledged into addrgen behind the active store. It has the same
  8/8 W-FIFO occupancy and +2-cycle source-pointer return, but the subsequent
  direction switch needs the two additional FIFO pop/head register stages.

R911 therefore returns `sourceNextReleaseTick`, final occupancy, and full-stall
count from `VenusSharedL2::reserveLsuWriteBurst`. `PendingLsuInstr` records
whether the tagged request was truly acknowledged behind a same-direction
addrgen owner. The opposite release is now:

```
sourceNextReleaseTick
  + 6 tile stages for WLAST/d1 and direction ownership
  + 2 tile stages only for an addrgen-queued descriptor
```

Non-backpressured stores retain the existing one-cycle boundary. The task4
captured replay monitor is byte-identical before and after the change; all 193
VINS have identical normalized fire/recycle timing.

## SCH task timing

Boundary: RTL `start execute -> execute complete`; Gem5 `tile_start ->
task_epilogue`. Return DMA and tile release are excluded.

| task | RTL ns | Gem5 ns | delta ns | error |
|---:|---:|---:|---:|---:|
| 0 | 676287 | 676300 | +13 | +0.001922% |
| 1 | 79131 | 79144 | +13 | +0.016428% |
| 2 | 55227 | 55228 | +1 | +0.001811% |
| 3 | 471571 | 471560 | -11 | -0.002333% |
| 4 | 30175 | 30164 | -11 | -0.036454% |
| 5 | 11059 | 11104 | +45 | +0.406908% |
| 6 | 3795 | 3792 | -3 | -0.079051% |
| 7 | 1059 | 1060 | +1 | +0.094429% |
| 8 | 330955 | 330976 | +21 | +0.006345% |

Only task5 changes, from +49 ns to +45 ns. Sum of absolute per-task deltas
falls from 123 ns to 119 ns. Its sequence 8 VLOAD moves from duration/recycle
`+2/+2` cycles to `0/0`; sequences 0--8 are now lifecycle-exact. The first
divergence advances to sequence 9 VLOAD: fire `+9`, duration `-9`, recycle
exact (previous fire delta `+11`). This proves the accepted boundary but also
shows that the later load admission/overlap is still incomplete.

The SCH execution envelope is 1,533,912 ns versus RTL 1,534,187 ns,
`-275 ns/-0.017925%`. It is slightly farther from zero than R909's -267 ns
because an earlier task5 completion lies on a dependency path. This aggregate
is not used as the acceptance criterion.

## CCH task timing

| task | RTL ns | Gem5 ns | delta ns | error |
|---:|---:|---:|---:|---:|
| 0 | 547 | 540 | -7 | -1.279707% |
| 1 | 10691 | 10684 | -7 | -0.065476% |
| 2 | 547 | 540 | -7 | -1.279707% |
| 3 | 69631 | 69384 | -247 | -0.354727% |
| 4 | 4019 | 4016 | -3 | -0.074645% |
| 5 | 895 | 892 | -3 | -0.335196% |
| 6 | 531 | 528 | -3 | -0.564972% |
| 7 | 895 | 892 | -3 | -0.335196% |
| 8 | 531 | 528 | -3 | -0.564972% |
| 9 | 158799 | 158668 | -131 | -0.082494% |
| 10 | 295 | 292 | -3 | -1.016949% |
| 11 | 375 | 372 | -3 | -0.800000% |
| 12 | 375 | 372 | -3 | -0.800000% |
| 13 | 563 | 560 | -3 | -0.532860% |
| 14 | 202779 | 202760 | -19 | -0.009370% |
| 15 | 6443 | 6432 | -11 | -0.170728% |
| 16 | 1343 | 1340 | -3 | -0.223380% |
| 17 | 33167 | 33164 | -3 | -0.009045% |
| 18 | 123235 | 123236 | +1 | +0.000811% |
| 19 | 2439 | 2440 | +1 | +0.041000% |
| 20 | 398523 | 398520 | -3 | -0.000753% |
| 21 | 9303 | 9308 | +5 | +0.053746% |
| 22 | 100055 | 100056 | +1 | +0.000999% |
| 23 | 33951 | 33948 | -3 | -0.008836% |

Only held-out task14 changes, from +25 ns to -19 ns; the other 23 tasks are
unchanged. Sum of absolute per-task deltas falls from 482 ns to 476 ns. This
cross-workload result supports the state-based model, but the sign crossing
also forbids claiming task14 complete without a direct lifecycle split.

The CCH execution envelope is 1,041,340 ns versus RTL 1,042,151 ns,
`-811 ns/-0.077820%`, compared with R909's -771 ns. As in SCH, the dependency
envelope moves farther from zero even while the relevant internal boundary
and total per-task absolute error improve; task-to-task cancellation remains
an invalid model criterion.

## Regression gates

- SCH: 22/22 output files and 8,103/8,103 VINS dumps byte-exact to R908.
- CCH: 53/53 output files and 10,253/10,253 VINS dumps byte-exact to R908.
- LSU RAW/throughput/capacity: 68/68 cases; 382/382 VINS byte-exact to R909.
- scalar600: 52/52 executions; frozen RTL oracle 51/52 with the existing
  `ecall_nop` fixture exclusion; 1,258 retire and 1,258 writeback exact.
- BitALU, CAU, and SerDiv result queues all reached depth 2/full; outputs are
  exact across 1/4/64 ns grant-gap controls.
- `git diff --check` passes for the four modified LSU/sequencer files.
- final binary SHA256:
  `8bb5493512e3682ea7e3d8e9c46febc97a090e1fd62351398ac4548b36c22de0`.

Permanent evidence is under
`evidence/a32_r911_addrgen_cdc_direction_20260815/`.

## Next breakpoints

1. SCH task5: sequence 9 VLOAD now fires 9 cycles late while recycling exactly;
   split addrgen admission from LDU queue admission and AR publication. Do not
   change fixed load latency because sequence 8 is now exact.
2. CCH task3 -247 ns and task9 -131 ns remain unchanged. Find their first
   scalar retire or VINS admission divergence; neither is this W-CDC boundary.
3. CCH task14 -19 ns must be split at its first changed store/load group before
   any further LSU direction edit; its total task sign alone is not evidence.
4. Preserve task4's queued-store control and task17/task20 at -3 ns while
   continuing stable tagged requester, LSU priority, and persistent per-bank
   RR checks.
