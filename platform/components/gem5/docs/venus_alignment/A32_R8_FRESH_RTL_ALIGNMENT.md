# A32 R8 fresh RTL—gem5 evidence

This record separates demonstrated contracts from outstanding timing work.
It does **not** claim that the whole RTL and gem5 implementations are
cycle-exact.

## Fixed inputs and fresh RTL run

- RTL workspace: `/home/shenyihao/Project/Venus_3/venus_soc`.
- Runtime image: `software/gatesim/gc0802_overall/PDSCHDag2_A32/l1.bin`,
  SHA-256 `d80305c2da5c30b37e125fc088b310b4d412d1d9edfe6e62fab832c7238b776f`.
- Fresh compile/run directory:
  `sim/build_gc0802_overall_PDSCHDag2_A32_R2_overall_PDSCHDag2_A32_R2`.
- Freshly elaborated `simv` SHA-256:
  `187755c6b948878af16fa4e8586846cb21510816cb635a0d3838f8bbe0cb8728`.
  This differs from the older reused A28 executable, so R2 is a real VCS
  compilation rather than a payload-only replay.
- VCS completed normally on 2026-07-28.  The overall testbench observed
  DAG completion at 1,678,056 ns, CRC completion at 4,563,416 ns, and emitted
  a VCS Simulation Report.

The fresh R2 `dma_read_data_file_L2.txt`,
`L2_scheduler_task_allocation_info.txt`, and `L2_DMA_trx.log` are byte-for-byte
identical to the prior A32 R1 captures (their respective SHA-256 values are
`a1f9d015…3cc9ec3`, `ad57981c…128647`, and `f6691968…a30feb`).

## VCS invocation note

The earlier apparent VCS-license outage was an execution-environment issue:
the sandbox did not load the user's interactive VCS setup and isolates its
loopback network, whereas the license setting is host-local.  In a host
interactive shell `vcs` and `vlogan` resolved normally and the fresh R2
compile obtained a license.  Future fresh RTL runs should therefore use the
host VCS environment (for example `bash -ic` in this setup), not treat a
sandbox `connect()` failure as evidence of exhausted license capacity.

## R8 gem5 regression

The fresh full-DAG run is `/tmp/a32_gem5_dynamic_r8.wQtkvB` with
`venus-rtl-16x128`; it exited naturally at tick `1610976000` with
`Venus RTL-aligned DAG completed`.

Proven against the fresh R2 RTL capture:

- All nine task epilogues and releases occurred, with 22 returned ports and
  one DAG completion event.
- 22/22 returned payloads are strict RTL↔gem5 matches.
- The same 22 ports are VEMU↔gem5 bit-exact (nine task-level matches).
- L1 transfer routing: 116 fires + 22 returns, 924 checks passed.
- Scheduler admission trace: 138/138 records match in order, IDs, raw
  addresses, and byte lengths.
- Streamer physical plan: 1,619 read and 1,619 write 64-byte beats, including
  bus addresses, byte selection, and WSTRB, match the RTL descriptor rule.

## Task0 DMA timing closure for the observed A32 descriptors

The R8 DMA model now represents the RTL state transition
`dma_go -> streamer descriptor load -> AR` and permits the next logical read
burst on the RLAST boundary.  This is a structural state-machine change, not a
length-specific delay.

For Task0's four RTL descriptors, all 130 read and 130 write physical 64-byte
beats and their data matched.  gem5 admission timestamps were 28, 488, 556,
and 756 ns, giving the fresh RTL intervals exactly:

```
6144 B -> 128 B : 460 ns
 128 B -> 1984 B:  68 ns
1984 B -> 64 B  : 200 ns
```

## Explicit remaining gaps

The lifecycle comparator remains a mismatch and should remain so-labelled.
After normalising each run to Task0 allocation, the full-DAG release span is
1,535,100 ns in RTL and 1,610,924 ns in gem5: gem5 is 75,824 ns (4.94%)
slower overall.

- Task3 is the largest positive error: 570,132 ns in gem5 versus 471,571 ns
  in RTL (+98,561 ns, +20.90%).  The next isolated feature is JAL-only
  Decode/ID redirect; conditional branches and JALR are intentionally not
  accelerated until their bypass and load-use behavior is modeled.
- Task8 is 33,735 ns faster in gem5 (297,220 versus 330,955 ns).  Its shuffle
  transport still bypasses RTL pipeline/arbitration/return-mailbox states.
- The DMA result above covers all descriptors observed in A32, but not an
  independent held-out FIFO-backpressure or destination-4KB-split case.
- gem5 currently expresses each beat as a timing packet and its `rd_ar`,
  `wr_aw`, and `b_hs` records are logical burst markers.  Do not call this a
  pin-level AXI waveform equivalence proof.
