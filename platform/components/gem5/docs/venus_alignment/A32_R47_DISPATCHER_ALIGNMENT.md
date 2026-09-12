# A32 R47 dispatcher alignment

## Scope and reference

- Workload: fresh RTL `PDSCHDag2 A32 R2`, 16 lanes x 128 rows.
- RTL run:
  `/home/shenyihao/Project/Venus_3/venus_soc/sim/build_gc0802_overall_PDSCHDag2_A32_R2_overall_PDSCHDag2_A32_R2`
- gem5 run: `/tmp/a32_r47_dispatch_barriercov_full`
- frozen gem5 binary: `/tmp/a32_r47_gem5.debug`
- Reference RTL functional span: 1,535,100 ns (767,550 tile cycles).
- This report separates L0 functional, L1 event, L2 cycle, and L3
  application-level conclusions. It does not claim full L1/L2 conformance.

## Falsifiable hypothesis and first divergence

The A42 two-entry dispatcher experiment admitted a scalar Venus request into
the modeled spill before final sequencer admission. Its scalar barrier only
observed `vinsn_running_q`, leaving a model-only low pulse while the request
was still in the dispatcher or crossing into the PE running table.

The first functional consequence was in Task3's second VLOAD:

- expected first elements: `1152, 1154, 1156, ...`
- A42 elements: `2, 6, 1156, ...`

The stale `2, 6` values came from the preceding three-element VLOAD. Task3
then took a different scalar branch and issued only 2 of its 289 Venus
instructions.

## RTL evidence

- `venus_dispatcher.sv:38-50` instantiates a non-bypass scalar request spill.
- `spill_register_flushable.sv:36-95` implements A/B storage, accepts while B
  is not full, and drains B before A.
- `venus_dispatcher.sv:58-68` adds the registered Venus request boundary.
- `scalar600_id_stage.sv:255-280` applies a three-register busy synchronizer
  and barrier counter.
- `scalar600_id_stage.sv:852-857` stalls on an unaccepted scalar Venus request,
  the barrier state, or an incomplete two-word Venus instruction.

## Minimal model change

R47 models the two-entry dispatcher A/B capacity explicitly. Scalar
retirement follows spill admission; a separate event retries only the FIFO
head against sequencer ready.

Because gem5 represents the RTL register boundaries as separately ordered
events, the scalar barrier busy sample covers the dispatcher FIFO,
`rtlIssuePending`, `rtlPeRunningQ`, and `rtlRunningQ`. This prevents an
event-ordering low pulse without adding a task-, opcode-, PC-, or
payload-specific rule.

Relevant gem5 implementation:

- `src/venus/VenusSequencer.hh:133-139`
- `src/venus/VenusSequencer.cc:322-358`
- `src/venus/VenusSequencer.cc:926-942`
- `src/venus/VenusSequencer.cc:1258-1274`

The previously validated shuffle response spill remains non-bypass at
`src/venus/VenusShufflePipline.cc:675-686`.

## Validation

### L0 functional alignment: PASS

- All 22 DAG output payloads are byte-exact against the fresh RTL evidence.
- Task3 VLOAD849 begins `1152, 1154, 1156, 1158, 1160`.
- No VEMU, RTL, scheduler/launcher, workload output, or task-specific behavior
  was modified.

### L1 dispatch/count alignment: partial PASS

| Task | RTL VINS | gem5 R47 VINS |
| ---: | ---: | ---: |
| 0 | 1 | 1 |
| 1 | 6 | 6 |
| 2 | 841 | 841 |
| 3 | 289 | 289 |
| 4 | 193 | 193 |
| 5 | 40 | 40 |
| 6 | 44 | 44 |
| 7 | 0 | 0 |
| 8 | 6,689 | 6,689 |
| Total | 8,103 | 8,103 |

All gem5 issue counters 0..8102 occur exactly once. Full L1 equivalence is not
yet claimed because the RTL trace distinguishes GATHER/SCATTER while the gem5
monitor currently labels both VSHUFFLE, and LSU event lifetimes are not yet
structurally equivalent.

### L3 application timing: PASS for the current sub-1% target

| Task | RTL ns | gem5 R47 ns | Delta ns | Error |
| ---: | ---: | ---: | ---: | ---: |
| 0 | 676,287 | 678,780 | +2,493 | +0.369% |
| 1 | 79,131 | 78,956 | -175 | -0.221% |
| 2 | 55,227 | 56,776 | +1,549 | +2.805% |
| 3 | 471,571 | 474,688 | +3,117 | +0.661% |
| 4 | 30,175 | 30,504 | +329 | +1.090% |
| 5 | 11,059 | 11,528 | +469 | +4.241% |
| 6 | 3,795 | 3,948 | +153 | +4.032% |
| 7 | 1,059 | 1,304 | +245 | +23.135% |
| 8 | 330,955 | 329,252 | -1,703 | -0.515% |

Full functional span:

- RTL: 1,535,100 ns
- gem5 R47: 1,535,992 ns
- delta: +892 ns = +446 tile cycles = **+0.0581%**

Short tasks retain large percentages from small absolute fixed overheads.
Those errors must be addressed with event-level tests, not compensated by
changing the total DAG latency.

## Rejected experiments

- A42: two-entry spill without barrier coverage. It appeared fast but failed
  Task1/3/4 and corrupted Task3 VLOAD849.
- A44/A45: collapsing shuffle response visibility to an earlier event phase.
  Function stayed exact, but full-DAG error worsened from +1.659% to +1.809%.
- R46 response delay 2/3/4: overflowed the per-lane response spill; only the
  RTL-backed one-cycle delay is legal.
- R48 fixed LSU latency 10/30/50: all remained functional, but a single magic
  constant traded Task8 error against Task3 and full-DAG error. It is not an
  acceptable aligned model.

## Remaining risk and next feature

The next isolated feature is vector LSU completion timing. RTL Task8 VLOAD and
VSTORE lifetimes vary with operation, VL, arbitration, and dependencies;
gem5 still executes data movement immediately and uses a fixed three-cycle
completion event. A load/store RAW chain and independent-stream cliff sweep
is required before changing the LSU model.

R47 is the current accepted functional and application-level baseline. Do not
replace it with a fixed-latency fit.
