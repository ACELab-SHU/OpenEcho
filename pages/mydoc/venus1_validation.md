---
title: Venus 1.0 validation
summary: "Measured Gem5–RTL agreement, with explicit timing and correctness boundaries."
sidebar: mydoc_sidebar
permalink: venus1_validation.html
folder: mydoc
---

The 13 September 2026 regression completed eight DAGs, covering **202 tasks and 557 returns**, using Gem5 fast mode with the L1 contract timing engine and fresh full-SoC RTL runs. Nominal clocks were 300 MHz for the Tile and 150 MHz for AXI.

## Measured timing

| DAG | Tasks | DAG interval error | Task execution sum error |
|---|---:|---:|---:|
| nrPBCH | 19 | −0.0355% | −0.0579% |
| nrPDCCH | 24 | −0.1604% | −0.1007% |
| ltePBCHDag1_hw | 27 | +0.0021% | −0.0520% |
| ltePBCHDag2_hw | 58 | +0.0584% | +0.0207% |
| ltePCFICH | 7 | −0.4026% | +0.0006% |
| ltePDCCHDag1_hw | 9 | +0.4633% | +0.5316% |
| ltePDCCHDag2_hw | 6 | −0.1422% | −0.1125% |
| ltePDSCH | 52 | −0.0347% | −0.0782% |

Error is `(Gem5 − RTL) / RTL`. The DAG interval starts at the first task's execution and ends at the last task's completion. It includes inter-task DMA and scheduling, but excludes boot, initial loading and final return DMA. The largest absolute DAG interval error was **0.4633%**; the largest individual task error was **2.2834%**. These measurements apply to this input matrix, rather than every possible workload.

## Output agreement

No known output bits differed. However, the RTL return buffers contained **607,984 unknown bits**. These remain unverified and were not silently classified as padding. A full bit-exact correctness PASS is therefore not asserted.

This comparison does not establish independent algorithm correctness against MATLAB or an approved golden output. In particular, the PDSCH sample CRC question remains separate from simulator agreement.

## Transfer and model boundaries

All 3,128 RTL transfers matched in ordered direction, Tile-local address and length after accounting for 82 extra Gem5 code loads. RTL can skip a code load after a code-CRC hit; the current Gem5 path retains it. RTL runtime DMT allocation also differs from Gem5's static slot reuse. These results do not claim AXI handshake equivalence.

The L1 contract engine interprets a firmware execution plan and models transfers. It does not execute the full L1 scalar Scheduler CPU. ACE-Echo provides cycle-level modelling with measured agreement within the stated scope; this report is not a claim of universal cycle-for-cycle equivalence.

## Source identities

- Regression platform: `be19d6a782d80360dfd89d37524a8277f6c5c339`
- DSL: `91f4579a187890e58f2ee225bb35b29762967632`
- Eight DAG workloads: `2c0feaf8b5e773553711cf4431ada59e9b434329`
- RTL reference: `a34a99aeb9245e01178d7a357c226e87a4249aef`

The release adds packaging, documentation and an onboarding sample without changing these eight DAG implementations. Public distribution provenance is recorded in `platform/PUBLIC_SOURCE_MANIFEST.json`.

[Get started with Venus 1.0](venus1_get_started.html) or [return to the platform overview](index.html).
