#!/usr/bin/env python3
"""Isolated Venus1 DMT publication/decode regression; requires licensed VCS.

Uses original RTL without edits. Forced FSM inputs do not qualify whole-SoC
handshakes, firmware, numerical DAG correctness, or cycle performance.
"""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess

SOURCES = ['hardware/soc_hierachy/venus_soc_pkg.sv', 'hardware/L2_scheduler/l2_scheduler_pkg.sv', 'hardware/L2_scheduler/dma_pkg.sv', 'hardware/venus_extension/venus_pkg.sv', 'hardware/generated/cluster0_tile0_pkg.sv', 'hardware/generated/cluster0_tile1_pkg.sv', 'hardware/generated/cluster0_tile2_pkg.sv', 'hardware/generated/cluster0_tile3_pkg.sv', 'hardware/generated/cluster0_tile4_pkg.sv', 'hardware/generated/cluster0_tile5_pkg.sv', 'hardware/generated/cluster0_tile6_pkg.sv', 'hardware/generated/cluster0_tile7_pkg.sv', 'hardware/generated/cluster0_tile8_pkg.sv', 'hardware/generated/cluster0_tile9_pkg.sv', 'hardware/generated/cluster0_tile10_pkg.sv', 'hardware/generated/cluster0_tile11_pkg.sv', 'hardware/generated/cluster0_pkg.sv', 'hardware/soc_hierachy/venus_gc0802_pkg.sv', 'sim/model/memory_model/ram_model.sv', 'hardware/venus_extension/common_cells/lzc.sv', 'hardware/L2_scheduler/task_manager.sv']

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--rtl-root', type=Path, required=True)
    parser.add_argument('--out', type=Path, required=True)
    parser.add_argument('--vcs', default='vcs')
    args = parser.parse_args()
    rtl, out = args.rtl_root.resolve(), args.out.resolve()
    sources = [rtl / name for name in SOURCES]
    sources.append(Path(__file__).with_name('dmt_length_tb.sv').resolve())
    for source in sources:
        if not source.is_file():
            parser.error('missing source: ' + str(source))
    out.mkdir(parents=True, exist_ok=False)
    command = [args.vcs, '-full64', '-sverilog', '-debug_access+all',
               '-timescale=1ns/1ps', '-LDFLAGS', '-Wl,--no-as-needed',
               '+define+SNPS_SYN_NODDRPHY', '+incdir+' + str(rtl / 'hardware/generated'),
               '-top', 'dmt_length_tb', '-o', str(out / 'simv')]
    command += [str(source) for source in sources]
    receipt = {'argv': command, 'sources': [
        {'path': str(source), 'sha256': hashlib.sha256(source.read_bytes()).hexdigest()}
        for source in sources]}
    with (out / 'build.log').open('w') as log:
        build = subprocess.run(command, cwd=out, stdout=log, stderr=subprocess.STDOUT)
    receipt['build_exit'] = build.returncode
    passed = False
    if build.returncode == 0:
        with (out / 'sim.log').open('w') as log:
            sim = subprocess.run([str(out / 'simv')], cwd=out, stdout=log,
                                 stderr=subprocess.STDOUT, timeout=60)
        receipt['run_exit'] = sim.returncode
        passed = sim.returncode == 0 and 'DMT_RTL_UNIT_PASS' in (out / 'sim.log').read_text()
    receipt['passed'] = passed
    (out / 'receipt.json').write_text(json.dumps(receipt, indent=2) + '\n')
    return 0 if passed else 1

if __name__ == '__main__':
    raise SystemExit(main())
