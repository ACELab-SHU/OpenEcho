---
title: Get started with ACE-Echo 1.0
layout: echo-doc
summary: "Build and run Venus 1.0 workloads with the Gem5 backend."
sidebar: mydoc_sidebar
permalink: venus1_get_started.html
folder: mydoc
---

ACE-Echo 1.0 brings the Venus 1.0 development flow to Gem5. The public distribution lives in `platform/`; the repository's older root-level tools belong to the legacy release.

## Prepare your tools

Use a Linux host with Python 3.8 or later, Git, Make, a C/C++ compiler and the dependencies listed in `platform/docs/GETTING_STARTED.md`. You also need the **Venus-custom LLVM toolchain** and a compatible RISC-V GCC installation. Ordinary upstream LLVM does not implement the Venus instruction extensions. The compiler, RTL source and commercial EDA tools are separate prerequisites and are not bundled in this release.

## Clone the source

```bash
git clone https://github.com/ACELab-SHU/ACE-Echo.git
cd ACE-Echo/platform
```

The public source bundle includes pinned DSL sources, the eight Venus1 regression DAGs and a small onboarding smoke example. It does not require access to an internal submodule server. See `PUBLIC_SOURCE_MANIFEST.json` for the exact source revisions.

## Select Venus 1.0

Create the host configuration and edit the external tool paths in `.ace-echo/host-tools.json`:

```bash
mkdir -p .ace-echo
cp configs/host.example.json .ace-echo/host-tools.json
```

Then resolve the 64-lane, 512-row Venus1 profile, with a nominal 300 MHz Tile clock and 150 MHz AXI clock:

```bash
python3 scripts/bootstrap.py fetch-json
python3 scripts/bootstrap.py configure \
  --tools .ace-echo/host-tools.json \
  --backend configs/backends/venus1p0-64x512-300mhz.json
python3 scripts/bootstrap.py build-gem5 --jobs 4 --mode opt
./ace-echo --config .ace-echo/host/local.toml doctor \
  --scope fast --backend .ace-echo/host/backend.json
```

## Check the installation

```bash
make test
make smoke
```

The smoke compiles a fresh two-task DAG, runs Gem5 fast mode and compares both task outputs against its software reference. Its report includes the active clock and timing boundaries. This is an installation check; it does not qualify the eight radio algorithms or replace an RTL regression.

Continue with the commands and backend contracts in `platform/docs/GETTING_STARTED.md`, `platform/docs/HARDWARE_SELECTION.md` and the CLI `--help` output. Read the [Venus1 validation report](venus1_validation.html) before interpreting timing or correctness results.

## Existing users

Keep your host-specific paths in `.ace-echo/`. Use the release's paired DSL and workload sources together. The [legacy setup guide](mydoc_get_started.html) is retained for older experiments; its simulator instructions do not describe ACE-Echo 1.0.
