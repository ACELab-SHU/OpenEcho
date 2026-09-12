#!/usr/bin/env python3
"""Generate deterministic Venus LSU cliff microbenchmarks.

The generated task image is shared by RTL and gem5.  Venus instructions are
emitted as their architectural 32/64-bit encodings so this tool does not
depend on a private compiler build.  Each case also carries a machine-readable
contract describing its dependency and sweep intent.
"""

import argparse
import hashlib
import json
import shutil
import subprocess
from pathlib import Path


VSETCSR_LSU_MSB = 0x2001FFDB
VBARRIER = 0x0800705B
TASK_HASH = 0x4C535530

REG = {
    "zero": 0,
    "ra": 1,
    "sp": 2,
    "t0": 5,
    "t1": 6,
    "t2": 7,
    "s0": 8,
    "s1": 9,
    "a0": 10,
    "a1": 11,
    "a2": 12,
    "a3": 13,
    "a4": 14,
    "a5": 15,
    "a6": 16,
    "a7": 17,
    "t3": 28,
    "t4": 29,
    "t5": 30,
    "t6": 31,
}


def venus_low(funct5, ew, func3, avl_reg, opcode):
    return (
        (funct5 << 27)
        | (ew << 26)
        | (func3 << 12)
        | (avl_reg << 7)
        | opcode
    )


def operand3(vs1, vs2, vd1):
    return (vd1 << 20) | (vs2 << 10) | vs1


def vload(avl_reg, base_reg, vd, ew):
    return venus_low(0x0E, ew, 1, avl_reg, 0x5B), operand3(
        base_reg, vd, vd
    )


def vstore(avl_reg, base_reg, vs, ew):
    return venus_low(0x0F, ew, 1, avl_reg, 0x5B), operand3(
        base_reg, vs, 0
    )


def vbroadcast(avl_reg, scalar_reg, vd, ew):
    return venus_low(0x02, ew, 1, avl_reg, 0x5B), operand3(
        scalar_reg, vd, vd
    )


def vadd(avl_reg, vs1, vs2, vd, ew):
    return venus_low(0x00, ew, 0, avl_reg, 0x2B), operand3(
        vs1, vs2, vd
    )


def emit_venus(words):
    return [
        f"    .4byte 0x{words[0]:08x}",
        f"    .4byte 0x{words[1]:08x}",
    ]


def emit_lsu_base(register, address):
    return [
        f"    li {register}, 0x{address:08x}",
        f"    srli t6, {register}, 16",
        "    slli t6, t6, 19",
        "    srli t6, t6, 19",
        f"    .4byte 0x{VSETCSR_LSU_MSB:08x}",
    ]


def row_stride(ew):
    return 2 if ew else 1


def independent_rows(count, ew, start=0):
    stride = max(4, row_stride(ew))
    rows = [start + index * stride for index in range(count)]
    if rows and rows[-1] + row_stride(ew) > 128:
        raise ValueError("independent stream rows exceed the 128-row VRF")
    return rows


def emit_drain_delay():
    return [
        # Keep tile soft-reset away from the observation window even if the
        # scalar barrier itself retires before every LSU post-response stage.
        # This delay is after the measured sequence and is never used as a
        # latency oracle.
        "    li t0, 512",
        "1:",
        "    addi t0, t0, -1",
        "    bnez t0, 1b",
    ]


def emit_store_prewarm_delay():
    return [
        # The RTL barrier observes sequencer running state, but a short
        # standalone program can still reach the following scalar sequence
        # before a large broadcast has populated every STU source row.
        # Keep operand readiness out of the independent-stream/capacity
        # experiment; producer->store latency is measured by the RAW cases.
        "    li t0, 512",
        "2:",
        "    addi t0, t0, -1",
        "    bnez t0, 2b",
    ]


def emit_return():
    return [
        f"    li a0, 0x{TASK_HASH:08x}",
        "    li a3, 0",
        "    li a2, 0x801ff000",
        "    sw a0, 40(a2)",
        "    sw a3, 44(a2)",
        "    li a0, 0",
        "    sw a0, 48(a2)",
        "    li a1, 0",
        "    sw a1, 52(a2)",
        "    sw zero, 0(a2)",
        # The two reserved words keep the termination block fixed-size for
        # either the scheduler EBREAK/drain protocol or legacy Linux SE exit.
        "    ebreak",
        "    nop",
        "    nop",
    ]


def raw_case(args):
    ew = 0 if args.ew == 8 else 1
    lines = [
        '    .section .text, "ax", @progbits',
        "    .option norvc",
        "    .4byte 0x00000093",
        "    .global _start",
        "_start:",
        "    li sp, 0x24000",
    ]
    contract = {
        "schema": "venus-lsu-microbench/v1",
        "kind": args.kind,
        "operation": args.operation,
        "ew_bits": args.ew,
        "vl": args.vl,
        "streams": args.streams,
        "blocker_vl": args.blocker_vl,
        "base_address": args.base_address,
        "controlled_variables": {
            "lanes": 16,
            "rows": 128,
            "alignment": 64,
            "masked": False,
            "unit_stride": True,
        },
    }

    if args.kind == "raw":
        lines += [f"    li a1, {args.vl}"]
        if args.operation == "load":
            lines += emit_lsu_base("a0", args.base_address)
            lines += emit_venus(vload(REG["a1"], REG["a0"], 0, ew))
            if args.consumer:
                lines += ["    li a3, 1"]
                lines += emit_venus(
                    vbroadcast(REG["a1"], REG["a3"], 4, ew)
                )
                lines += emit_venus(vadd(REG["a1"], 0, 4, 8, ew))
        else:
            lines += ["    li a3, 3"]
            lines += emit_venus(vbroadcast(REG["a1"], REG["a3"], 0, ew))
            if args.producer:
                lines += ["    li a4, 1"]
                lines += emit_venus(
                    vbroadcast(REG["a1"], REG["a4"], 4, ew)
                )
                lines += emit_venus(vadd(REG["a1"], 0, 4, 8, ew))
                source = 8
            else:
                source = 0
            lines += emit_lsu_base("a0", args.base_address)
            lines += emit_venus(
                vstore(REG["a1"], REG["a0"], source, ew)
            )
        lines += [f"    .4byte 0x{VBARRIER:08x}"]
        contract["dependency"] = (
            "load_to_consumer" if args.consumer
            else "producer_to_store" if args.producer
            else f"{args.operation}_to_barrier"
        )

    elif args.kind == "throughput":
        rows = independent_rows(args.streams, ew)
        lines += [f"    li a1, {args.vl}"]
        if args.operation == "store" and not args.back_to_back:
            for index, row in enumerate(rows):
                lines += [f"    li a3, {index + 1}"]
                lines += emit_venus(
                    vbroadcast(REG["a1"], REG["a3"], row, ew)
                )
            lines += [f"    .4byte 0x{VBARRIER:08x}"]
            lines += emit_store_prewarm_delay()
        if args.back_to_back:
            lines += emit_lsu_base("a0", args.base_address)
        for index, row in enumerate(rows):
            address = args.base_address + index * 0x400
            if not args.back_to_back:
                lines += emit_lsu_base("a0", address)
            if args.operation == "load":
                lines += emit_venus(
                    vload(REG["a1"], REG["a0"], row, ew)
                )
            else:
                lines += emit_venus(
                    vstore(REG["a1"], REG["a0"], row, ew)
                )
        lines += [f"    .4byte 0x{VBARRIER:08x}"]
        contract["stream_rows"] = rows
        contract["dependency"] = "independent"
        contract["stream_addressing"] = (
            "shared_preloaded_base" if args.back_to_back
            else "per_stream_scalar_base"
        )
        contract["store_source_setup"] = (
            "reset_vrf_no_hazard"
            if args.operation == "store" and args.back_to_back
            else "broadcast_and_scalar_drain"
            if args.operation == "store"
            else "not_applicable"
        )

    else:
        followers = args.streams
        follower_rows = independent_rows(followers, ew, start=64)
        blocker_rows = (
            args.blocker_vl * (1 << ew) + 127
        ) // 128
        if blocker_rows >= 64:
            raise ValueError("blocker overlaps follower VRF rows")
        if args.operation == "store":
            lines += [f"    li a1, {args.blocker_vl}", "    li a3, 7"]
            lines += emit_venus(
                vbroadcast(REG["a1"], REG["a3"], 0, ew)
            )
            lines += [f"    .4byte 0x{VBARRIER:08x}"]
            lines += [f"    li a1, {args.vl}"]
            for index, row in enumerate(follower_rows):
                lines += [f"    li a3, {index + 1}"]
                lines += emit_venus(
                    vbroadcast(REG["a1"], REG["a3"], row, ew)
                )
            lines += [f"    .4byte 0x{VBARRIER:08x}"]
            lines += emit_store_prewarm_delay()
        lines += [f"    li a1, {args.blocker_vl}"]
        lines += emit_lsu_base("a0", args.base_address)
        if args.operation == "load":
            lines += emit_venus(vload(REG["a1"], REG["a0"], 0, ew))
        else:
            lines += emit_venus(vstore(REG["a1"], REG["a0"], 0, ew))
        lines += [f"    li a1, {args.vl}"]
        for index, row in enumerate(follower_rows):
            address = args.base_address + 0x8000 + index * 0x400
            lines += emit_lsu_base("a0", address)
            if args.operation == "load":
                lines += emit_venus(
                    vload(REG["a1"], REG["a0"], row, ew)
                )
            else:
                lines += emit_venus(
                    vstore(REG["a1"], REG["a0"], row, ew)
                )
        lines += [f"    .4byte 0x{VBARRIER:08x}"]
        contract["dependency"] = "long_response_capacity_blocker"
        contract["follower_rows"] = follower_rows
        contract["candidate_capacity"] = 4

    lines += emit_drain_delay()
    lines += emit_return()
    return "\n".join(lines) + "\n", contract


def require_tool(name):
    path = shutil.which(name)
    if path is None:
        raise RuntimeError(f"required tool is not available: {name}")
    return path


def build_case(args, case_builder=raw_case, stem_prefix="lsu"):
    output_dir = args.output_dir.resolve()
    if output_dir.exists() and any(output_dir.iterdir()):
        raise RuntimeError(
            f"immutable attempt directory is not empty: {output_dir}"
        )
    output_dir.mkdir(parents=True, exist_ok=True)
    assembly, contract = case_builder(args)
    stem = (
        f"{stem_prefix}_{args.kind}_{args.operation}_ew{args.ew}_vl{args.vl}"
        f"_n{args.streams}"
    )
    source = output_dir / f"{stem}.S"
    obj = output_dir / f"{stem}.o"
    linked = output_dir / f"{stem}.linked.elf"
    raw = output_dir / f"Task_{stem}.bin"
    linker_script = output_dir / f"{stem}.ld"
    source.write_text(assembly, encoding="utf-8")
    linker_script.write_text(
        "ENTRY(_start)\n"
        "SECTIONS\n"
        "{\n"
        "  . = 0; .text : { *(.text*) }\n"
        "  /DISCARD/ : { *(.comment) *(.riscv.attributes) }\n"
        "}\n",
        encoding="utf-8",
    )
    clang = require_tool("clang")
    linker = require_tool("ld.lld")
    objcopy = require_tool("llvm-objcopy")
    subprocess.run(
        [
            clang,
            "--target=riscv32",
            "-march=rv32im",
            "-mabi=ilp32",
            "-c",
            str(source),
            "-o",
            str(obj),
        ],
        check=True,
    )
    subprocess.run(
        [
            linker,
            "-m",
            "elf32lriscv",
            "-T",
            str(linker_script),
            str(obj),
            "-o",
            str(linked),
        ],
        check=True,
    )
    subprocess.run(
        [objcopy, "-O", "binary", str(linked), str(raw)],
        check=True,
    )
    image = raw.read_bytes()
    text_image = image.ljust((len(image) + 63) // 64 * 64, b"\0")
    text_bytes = len(text_image)
    scheduler_launch_input = bool(
        getattr(args, "scheduler_launch_input", False)
    )
    # The full gc0802 scheduler launch path requires at least one DAG input.
    # Timing probes do not consume runtime data, so append a generic token in
    # the task data area.  The instruction image and task behavior remain
    # unchanged, and RTL/gem5 still consume the same complete task image.
    padded = (
        text_image + bytes(64) if scheduler_launch_input else text_image
    )
    raw.write_bytes(padded)

    replay_dir = output_dir / "gem5"
    replay_dir.mkdir()
    low_bin = replay_dir / "task.low.bin"
    high_bin = replay_dir / "task.high.bin"
    low_obj = replay_dir / "task.low.o"
    high_obj = replay_dir / "task.high.o"
    replay_elf = replay_dir / f"{stem}.elf"
    replay_ld = replay_dir / "task.ld"
    gem5_image = bytearray(padded)
    rtl_termination = bytes.fromhex(
        "73001000"  # ebreak
        "13000000"  # nop
        "13000000"  # nop
    )
    gem5_termination_mode = getattr(
        args, "gem5_termination", "linux_se_exit"
    )
    if gem5_termination_mode == "scheduler_ebreak":
        gem5_termination = rtl_termination
        gem5_termination_description = "scheduler ebreak"
    elif gem5_termination_mode == "linux_se_exit":
        gem5_termination = bytes.fromhex(
            "13050000"  # addi a0, zero, 0
            "9308d005"  # addi a7, zero, 93
            "73000000"  # ecall
        )
        gem5_termination_description = "Linux SE exit(0)"
    else:
        raise ValueError(
            f"unsupported gem5 termination mode: {gem5_termination_mode}"
        )
    termination_offset = gem5_image.rfind(rtl_termination)
    if termination_offset < 0:
        raise RuntimeError("cannot locate reserved RTL termination sequence")
    gem5_image[
        termination_offset:termination_offset + len(rtl_termination)
    ] = gem5_termination
    local_image = bytes(gem5_image).ljust(0x200000, b"\0")
    low_bin.write_bytes(local_image)
    high_bin.write_bytes(local_image)
    gnu_objcopy = require_tool("riscv64-linux-gnu-objcopy")
    gnu_ld = require_tool("riscv64-linux-gnu-ld")
    for binary, object_file in ((low_bin, low_obj), (high_bin, high_obj)):
        subprocess.run(
            [
                gnu_objcopy,
                "-I",
                "binary",
                "-O",
                "elf32-littleriscv",
                "-B",
                "riscv:rv32",
                str(binary),
                str(object_file),
            ],
            check=True,
        )
    replay_ld.write_text(
        "SECTIONS\n"
        "{\n"
        "  . = 0x0; .local : { task.low.o(.data) }\n"
        "  . = 0x80000000; .venus : { task.high.o(.data) }\n"
        "}\n",
        encoding="utf-8",
    )
    subprocess.run(
        [
            gnu_ld,
            "-m",
            "elf32lriscv",
            "-T",
            replay_ld.name,
            "-e",
            str(contract.get("gem5_entry", "0x4")),
            "-o",
            replay_elf.name,
            low_obj.name,
            high_obj.name,
        ],
        cwd=replay_dir,
        check=True,
    )

    case_dir = output_dir / "rtl_case"
    case_dir.mkdir()
    rtl_task = case_dir / raw.name
    shutil.copy2(raw, rtl_task)
    task_name = raw.stem
    task_inputs = []
    if scheduler_launch_input:
        task_inputs.append(
            {
                "dest_address": "0x22000",
                "name": "launch_token",
                "parentTasksPort": "0b0000000000",
                "slice_data_dest_str": "0x22000",
                "slice_data_type": "0",
                "slice_length": "0",
                "type": "0b010",
                "index": 1,
                "length": 4,
                "datatype": "int",
                "offset": text_bytes,
            }
        )
    descriptor = [
        {
            "Input_Num": len(task_inputs),
            # These are timing probes, not task-output tests.  Declaring an
            # unwritten VSPM result makes the tile testbench compare X data
            # and obscures the LSU event verdict.
            "Output_Num": 0,
            "all_input": task_inputs,
            "all_output": [],
            "current_taskId": 0,
            "data_length": len(padded) - text_bytes,
            "data_offset": text_bytes,
            "debug_task_name": task_name,
            "hardwareinfo": "0b00000101111",
            "hash": f"0x{TASK_HASH:08x}",
            "is_spmd": 0,
            "min_core_num": 1,
            "text_length": text_bytes,
            "text_offset": 0,
            "total_length": len(padded),
        },
        {"return_output": "None"},
    ]
    (case_dir / "dag1.json").write_text(
        json.dumps(descriptor, indent=2) + "\n", encoding="utf-8"
    )
    (case_dir / "dma_expected_file.txt").write_text("", encoding="utf-8")

    contract.update(
        {
            "name": stem,
            "task_hash": f"0x{TASK_HASH:08x}",
            "assembly_sha256": hashlib.sha256(
                assembly.encode("utf-8")
            ).hexdigest(),
            "raw_sha256": hashlib.sha256(padded).hexdigest(),
            "raw_bytes": len(padded),
            "termination": {
                "rtl": "scheduler ebreak",
                "gem5": gem5_termination_description,
                "patch_offset": termination_offset,
            },
            "rtl_case_dir": str(case_dir),
            "gem5_elf": str(replay_elf),
        }
    )
    contract_path = output_dir / "case.json"
    contract_path.write_text(
        json.dumps(contract, indent=2) + "\n", encoding="utf-8"
    )
    print(contract_path)


def parse_args():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--output-dir", type=Path, required=True,
        help="new immutable case directory",
    )
    parser.add_argument(
        "--kind", choices=("raw", "throughput", "capacity"), required=True
    )
    parser.add_argument(
        "--operation", choices=("load", "store"), required=True
    )
    parser.add_argument("--ew", type=int, choices=(8, 16), required=True)
    parser.add_argument("--vl", type=int, required=True)
    parser.add_argument("--streams", type=int, default=1)
    parser.add_argument("--blocker-vl", type=int, default=4320)
    parser.add_argument("--base-address", type=lambda value: int(value, 0),
                        default=0x80070000)
    parser.add_argument("--consumer", action="store_true")
    parser.add_argument("--producer", action="store_true")
    parser.add_argument(
        "--back-to-back", action="store_true",
        help=(
            "for throughput cases, preload one address and emit adjacent "
            "Venus commands so scalar address setup cannot limit LSU "
            "admission"
        ),
    )
    args = parser.parse_args()
    if args.vl <= 0 or args.streams <= 0 or args.blocker_vl <= 0:
        parser.error("VL and stream counts must be positive")
    if args.kind != "raw" and (args.consumer or args.producer):
        parser.error("--consumer/--producer apply only to raw cases")
    if args.back_to_back and args.kind != "throughput":
        parser.error("--back-to-back applies only to throughput cases")
    if args.consumer and args.operation != "load":
        parser.error("--consumer requires --operation=load")
    if args.producer and args.operation != "store":
        parser.error("--producer requires --operation=store")
    # ECALL is part of the scalar600 conformance surface and is a hardware
    # no-op.  Terminate timing microbenches through the same EBREAK/drain
    # protocol as production tasks rather than an artificial SE syscall.
    args.gem5_termination = "scheduler_ebreak"
    return args


if __name__ == "__main__":
    build_case(parse_args())
