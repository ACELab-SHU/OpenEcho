# A32 R906 W-CDC FIFO and direction boundary alignment

Date: 2026-08-15

## Scope

This round did not tune a full-DAG total. It used the PDSCH task4 RTL W-channel
waveform and nrPDCCH task17's alternating contained/crossing store train to
replace the requester-local fixed VSTU restart with explicit state:

- an eight-entry source W CDC FIFO with beat-level occupancy and a two-tile
  read-pointer return synchronizer;
- local and external WLAST tracked separately;
- the four-tile requester keeps its own source FIFO state;
- the long STU-to-LDU release applies only when the source FIFO actually
  backpressures;
- a VSTORE completion and an opposite VLOAD request on the same tile edge
  cross one registered direction-select boundary;
- the 2:1 tile/AXI CDC phase, rather than another fixed latency, produces the
  contained-versus-address-crossing response difference.

No task ID, PC, instruction ordinal, address value, or payload special case was
added. LSU/VFU fixed arithmetic latency was not changed.

## RTL oracle

The task4 fine-grained waveform contains 361 samples. Sequences 73--75 are
VSTORE and sequence 76 is VLOAD. Local W is produced at 500 MHz while external
W is consumed at 250 MHz. The third store reaches the eight-entry source FIFO
capacity and stalls until its read pointer returns; the waveform disproves the
old fixed 12-AXI-cycle restart.

Task17 supplies 26 matched store-store-load controls:

- 13 contained 31-byte stores: the following VLOAD is 104 cycles in RTL;
- 13 stores crossing a 64-byte LSU beat: the following VLOAD is 106 cycles;
- the final one-tile direction boundary plus the existing CDC phase reproduces
  both populations without a burst-length latency table.

The accepted model is also distinguished from two rejected candidates:

- adding two cycles to every ordinary STU-to-LDU switch damaged SCH task3 by
  5.772 us and was rejected;
- splitting the contained case into one admission cycle plus another explicit
  response cycle double-counted the CDC phase, made task17 VLOAD 105 versus
  RTL 104 cycles, and was rejected.

## Final task timing

The compared boundary is RTL `start execute -> execute complete` versus gem5
`tile_start -> task_epilogue`. Return DMA and tile release are excluded.

### SCH / PDSCHDag2

| task | RTL ns | gem5 ns | delta ns | error |
|---:|---:|---:|---:|---:|
| 0 | 676287 | 676292 | +5 | +0.000739% |
| 1 | 79131 | 79134 | +3 | +0.003791% |
| 2 | 55227 | 54896 | -331 | -0.599345% |
| 3 | 471571 | 471552 | -19 | -0.004029% |
| 4 | 30175 | 30154 | -21 | -0.069594% |
| 5 | 11059 | 11098 | +39 | +0.352654% |
| 6 | 3795 | 3782 | -13 | -0.342556% |
| 7 | 1059 | 1050 | -9 | -0.849858% |
| 8 | 330955 | 330968 | +13 | +0.003928% |

The important changes relative to R905 are task3 `+221 -> -19 ns` and task4
`+247 -> -21 ns`. Task2 remains the largest absolute SCH residual at `-331 ns`.
The execution-envelope comparison is RTL 1,534,187 ns versus gem5 1,533,844 ns,
or `-343 ns / -0.022357%`; it is reported only as a diagnostic and is not used
to accept the model.

### CCH / nrPDCCH

| task | RTL ns | gem5 ns | delta ns | error |
|---:|---:|---:|---:|---:|
| 0 | 547 | 532 | -15 | -2.742230% |
| 1 | 10691 | 10676 | -15 | -0.140305% |
| 2 | 547 | 532 | -15 | -2.742230% |
| 3 | 69631 | 69374 | -257 | -0.369088% |
| 4 | 4019 | 4006 | -13 | -0.323464% |
| 5 | 895 | 882 | -13 | -1.452514% |
| 6 | 531 | 518 | -13 | -2.448211% |
| 7 | 895 | 882 | -13 | -1.452514% |
| 8 | 531 | 518 | -13 | -2.448211% |
| 9 | 158799 | 158658 | -141 | -0.088791% |
| 10 | 295 | 284 | -11 | -3.728814% |
| 11 | 375 | 362 | -13 | -3.466667% |
| 12 | 375 | 362 | -13 | -3.466667% |
| 13 | 563 | 552 | -11 | -1.953819% |
| 14 | 202779 | 201776 | -1003 | -0.494627% |
| 15 | 6443 | 6422 | -21 | -0.325935% |
| 16 | 1343 | 1330 | -13 | -0.967982% |
| 17 | 33167 | 33154 | -13 | -0.039196% |
| 18 | 123235 | 123228 | -7 | -0.005680% |
| 19 | 2439 | 2430 | -9 | -0.369004% |
| 20 | 398523 | 398510 | -13 | -0.003262% |
| 21 | 9303 | 9298 | -5 | -0.053746% |
| 22 | 100055 | 100048 | -7 | -0.006996% |
| 23 | 33951 | 33938 | -13 | -0.038290% |

Task17 is exact through sequence 681. Its first remaining lifecycle difference
is sequence 682 VLOAD: fire `-1` cycle and duration `+1` cycle, so recycle is
still aligned. Task17 total returned from the intermediate `-65 ns` regression
to `-13 ns`. Task20 remains unchanged at `-13 ns`.
The CCH execution envelope is RTL 1,042,151 ns versus gem5 1,040,260 ns, or
`-1,891 ns / -0.181452%`; the task table shows that task14 dominates this
residual, so it must not be hidden by the envelope.

## Correctness and regressions

- SCH: 22/22 output files and 8,103/8,103 task-local VINS are byte-exact to the
  accepted baseline.
- CCH: 53/53 output files and 10,253/10,253 task-local VINS are byte-exact to
  the accepted baseline.
- LSU RAW/throughput/capacity suite: 68/68 pass.
- Final gem5 binary SHA256:
  `1fa5dc760c47ace7c178dc11c9e75c523e7b2ea9e737085494317e98056949a9`.

Permanent evidence:

- `evidence/a32_r906_wcdc_direction_20260815/final_cch_sch_summary.json`
- `evidence/a32_r906_wcdc_direction_20260815/task17_vins_timing_vs_rtl.json`
- `evidence/a32_r906_wcdc_direction_20260815/lsu_suite_results.json`
- `evidence/a32_r906_wcdc_direction_20260815/sch_venus_dag_trace.jsonl`
- `evidence/a32_r906_wcdc_direction_20260815/cch_venus_dag_trace.jsonl`

## Next breakpoints

Do not optimize the aggregate DAG. The next independent targets are:

1. SCH task2 (`-331 ns`): locate the first scalar/vector lifecycle divergence;
2. CCH task14 (`-1003 ns`): continue per-bank LSU/VRF arbitration rather than
   introducing another VSTORE/VLOAD latency;
3. CCH task3 (`-257 ns`) and task9 (`-141 ns`): isolate their first divergent
   request/response event;
4. the common `-11..-15 ns` short-task offset: audit scalar task-entry and
   epilogue boundaries; percentage alone is not a valid priority signal.
