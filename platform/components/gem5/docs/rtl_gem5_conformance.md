# Venus RTL/gem5 conformance contract

The gem5 model must not branch on a task name, task ID, DAG name, or a known
payload value. A functional change is accepted only when it implements a
general RTL-visible rule and can be checked independently of PDSCHDag2.

| Contract | RTL source | gem5 model/check |
| --- | --- | --- |
| Tile-local address is VRF when address bit 20 is one | `hardware/L2_scheduler/venus_task_lv_scheduler_tile_req_responder.sv`, `is_vrf_addr` | `VenusDagScheduler::isVrfAddress` |
| VSPM address fields are byte, bank, lane, row and only `MEM_WIDTH` low bits are consumed | `hardware/venus_extension/venus_mem2lanes.sv`, `MEM_WIDTH`, `bitfield_lane`, `bitfield_row` | `venus_vrf_mem::backdoor_ReadVspm` and `backdoor_WriteVspm` |
| Physical VRF bank uses the row-dependent barber-pole rotation | `hardware/venus_extension/venus_mem2lanes.sv`, `bitfield_bank`/`bank_unbarber` | `vspmLogicalByteAddr` |
| VRF row indices wrap at the width of `vrow_t` | generated tile package, `typedef logic [$clog2(Nrlines)-1:0] vrow_t` | VRF element-address helpers truncate the effective row |
| Vector length is 15 bits | `hardware/venus_extension/venus_pkg.sv`, `vlen_t` | Venus instruction decode truncates AVL to 15 bits |
| DAG readiness is dependency-driven and parent output bytes feed child input DMA | L2 scheduler task manager and tile request responder | `VenusDagScheduler` task states and DMT |
| Task completion waits for the vector pipeline to drain | tile/sequencer completion handshake | `VenusSequencer::isIdle` plus scheduler `Draining` state |
| Shared-memory payloads belong to the testbench/case, not the hardware model | RTL testbench memory initialization | `VENUS_GEM5_SHARED_MEM_FILE`; absent means reset-zero memory |
| Mask data is tagged/latched independently by each consuming VFU | lane mask operand queue and BitALU/CAU/SerDiv mask latches | independent BitALU, CAU, SerDiv and Shuffle mask packets in `VenusLane` |
| Dependencies are resolved by the requesting operand, not by one instruction-global gate | `venus_operand_requester.sv`, requester-local `hazard` | operand-specific VS1/VS2/VD1/VD2/VM checks in `VenusHazardTable` |
| A requester cannot consume a hazard table older than its allocated instruction | lane sequencer `pe_req.id` and hazard vectors are transferred together | running-ID/instruction-ID generation check before requester release |
| Row-progress chain credit is disabled for masked consumers and same-VFU producer/consumer pairs | `venus_operand_requester.sv`, `vfu != VFU_NONE` and writer VFU comparison | cross-VFU admission only; masked and same-VFU dependencies remain ordered |
| A result pipeline drains even when the next instruction has only a partial operand command set | BitALU/CAU result pipeline valid/ready flow | VFU flush uses complete operand-command readiness, not only B-queue occupancy |
| Scalar Venus requests pass through a non-bypass two-entry A/B spill | `venus_dispatcher.sv` scalar spill and `spill_register_flushable.sv` | `VenusSequencer::scalarDispatchQueue`, depth 2, with independent downstream retry |
| A scalar barrier must not observe a model-only idle pulse while an accepted spill request crosses event boundaries | scalar request spill, registered Venus request, sequencer running table, and scalar600 busy synchronizer | barrier busy sampling covers dispatcher, issue-pending, PE-running, and running-Q states |

## L1/L2 runtime contract now modeled

- Runtime task count, output counts, code/data slices, hardware requirements,
  and input descriptors are decoded from L1 ELF symbols and the exact RTL
  packed structures.
- Input types `000/001/010/100/101/110` retain their RTL meanings. Direct
  dependencies use DMT payloads; pointer dependencies use the 512-bit pointer
  record and its referenced L2 payload.
- Scalar tile-local addresses alias the software block perspective at
  `0x80000000`; VSPM addresses use RTL logical lane/bank/row mapping.
- LDU/STU access the active task address space through the general memory path,
  rather than a single-task-only fixed input window.
- Return count/address/length come from tile-manager registers written by the
  task epilogue. Descriptor output counts are assertions, not output data.

The runtime graph now completes all nine tasks and all 22 return transactions.
All RTL-known bytes in every returned port of PDSCHDag2 pass
`tools/compare_dag_dma.py`. The A32 R47 dispatcher remains the accepted
baseline. Its structured-LSU continuation also completes all 22 outputs
byte-exact and issues the same 8,103 Venus instructions.

The LSU model now exposes accept, request, response, and completion phases,
operation-specific queues and admission intervals, beat-derived transfer
time, store-operand waiting, and the LDU held-addrgen capacity boundary. In
the held-out PDSCHDag2 run, 674 loads and 638 stores have exactly one request,
response, and completion; per-task counts and maximum outstanding values
match RTL. Its full span is 769,554 tile cycles versus 767,550 cycles in RTL
(+2,004 cycles, +0.2611%). This remains an L0 pass and sub-1% L3 result. The
remaining producer-to-store first divergence is upstream VADD/requester
readiness, not a fixed LSU completion constant. See
`docs/venus_alignment/A32_R47_DISPATCHER_ALIGNMENT.md` and
`docs/venus_alignment/A32_R47_STRUCTURED_LSU_ALIGNMENT.md`.

The independent LDPC single-task regression compares 1,527/1,527 Venus
instruction dumps exactly. Its Venus fire-to-recycle span is 23,401 cycles
versus 21,606 cycles in RTL (+8.31%). This is the current worst measured
cross-workload timing error; no LDPC, task-ID, opcode-sequence, or PC-specific
rule is used.

The RTL profiles enable requester-local hazard handling by default. Set
`VENUS_GEM5_LEGACY_GLOBAL_HAZARDS=1` only for an A/B comparison with the
earlier conservative sequencer scoreboard.

## Required validation order

1. Exhaustively check that the configured VSPM logical-to-physical mapping is
   a bijection over the implemented data bytes.
2. Check DAG lifecycle events and DMA metadata against RTL.
3. Compare every `task_<id>_port_<port>.bin` with the corresponding RTL DMA
   payload and stop at the first differing producer.
4. Compare instruction and sequencer traces inside that producer.
5. Tune latency and contention only after byte equality passes.
