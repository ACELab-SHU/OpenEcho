# One-file hardware selection, independent personal paths

This entrypoint is opt-in. Existing commands with explicit --config continue to
work. No hardware model, compiler semantics, Scheduler core or RTL is changed.

## Personal setup (Linux)

After recursive clone and the GETTING_STARTED.md compiler/JSON/Gem5 setup:

    mkdir -p .ace-echo
    cp configs/project.example.toml .ace-echo/project.toml
    cp configs/host-paths.example.json .ace-echo/host-paths.json

Edit host-paths.json once. Each named toolchain contains venus_llvm_bin,
riscv_gcc_root and optionally scheduler_cross_prefix. Use absolute paths to
approved compatible installations. The optional rtl object maps hardware names
to your own immutable RTL checkouts, for example:

    "rtl": {
      "venus1.0": "/your/rtl-v1/venus_soc",
      "venus2.0": "/your/rtl-v2/venus_soc"
    }

These are local files, ignored by Git. Missing RTL permits software-only setup,
but will fail RTL preflight; there is no fallback to another user's RTL. A path
does not prove a hardware version: RTL repository identity, source state, IP,
licenses and compatibility checks remain mandatory.

## Daily use

Only edit .ace-echo/project.toml:

    hardware = "venus2.0"
    toolchain = "shared"

Change hardware to venus1.0 for the V1 300 MHz profile, or venus1.0-150mhz for
the distinct legacy profile. Change toolchain independently to another entry
in host-paths.json. Supported names are declared in configs/hardware-profiles.json.
That tracked catalog binds names to the complete existing backend manifests;
personal paths cannot override rows, lanes, ABI or correctness policies.

    ./ace-echo hardware show
    ./ace-echo doctor --scope fast
    ./ace-echo compile dag --target forge_vector_smoke
    python3 scripts/smoke.py --keep-heavy

No --backend or --config is needed. The same selection applies to run task/dag/
application, scheduler build, rtl-preflight and rtl run. Supply their usual input
arguments. Generated configurations are immutable and cached under
.ace-echo/resolved/HARDWARE/CONFIGURATION_DIGEST/. On first use the selector
stages the pinned DSL and hashes compiler binaries; this can take minutes.
No downloads, system installations or simulator builds are triggered by selection.
Run bootstrap.py fetch-json/build-gem5 separately if dependencies are missing.

Every command reports the selected version on stderr. hardware show produces
JSON with the resolved backend, host paths, toolchain, existing Forge skill and
backend-specific knowledge paths. The same agent is used, with different hardware
context, not a new LLM or automatically launched AI process.

## Forge requests

Use "backend_manifest": "@project" in a request to follow the active selection.
The CLI passes the resolved backend to request validation/run creation, and saves
selection evidence. It never silently overwrites a request pinned to a different
backend, edits an existing run, changes a reference or approves its golden.
The same example is available as examples/forge-project-smoke.example.json.

    ./ace-echo forge examples/forge-project-smoke.example.json --validate-only

## Reproducibility and overrides

Default run roots are runs/HARDWARE/. Explicit --run-dir is still accepted, but
must be fresh. Compile/Scheduler handoffs carry per-file hashes and a hardware/
compiler identity. Selected-mode consumers reject handoffs from another hardware
or compiler and reject missing/modified receipts; recompile after switching.
Historical unlabelled artifacts require explicit legacy --config handling and
independent requalification, not an automatic trust promotion.

Explicit --config bypasses automatic project selection. If it names a generated
selection config, its original hardware binding and artifact checks remain in
force. Explicit --backend cannot conflict with that binding. Workload-local
input-root relocation is allowed, as used by the frozen smoke.

The selector hashes compiler files at first setup and invalidates its cache on
file size/mtime/ctime changes. This is an accidental-change/reproducibility guard,
not adversarial attestation. Never manually edit resolved cache files. Failed
partial setup is reported rather than reused; inspect it before choosing a fresh
configuration. No cleanup, RTL execution or remote push is performed by switching.

Fast execution PASS alone is only software smoke evidence. Switching to V2
does not transfer a V1 PASS. Complete integrated software/golden comparisons
and all-output RTL qualification remain required; the shipped normal route
uses fast, with verification/debug reserved for diagnosis.
