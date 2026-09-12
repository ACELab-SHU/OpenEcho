#!/usr/bin/env python3
import json, sys, argparse
import os
from pathlib import Path

_script_dir = os.path.dirname(os.path.abspath(__file__))
if _script_dir not in sys.path:
    sys.path.insert(0, _script_dir)

from type_codes import (
    TYPE_PARAM_GLOBAL,
    TYPE_DAG_DFE_STATIC,
    POINTER_TYPES,
    POINTER_INPUT_STRUCT_BYTES,
)

LIMITS = {
    "single_io_bytes_max": 65536,
    "task_text_bytes_max": 0x8000,
    "task_data_bytes_max": 0x3000,
    "task_scalar_bytes_max": 0x1000,
    "dag_total_bytes_max": 0x88000,
    "task_input_num_min": 1,
    "task_input_num_max": 63,
    "task_output_num_min": 1,
    "task_output_num_max": 15,
}

LSU_STORE_BOUNDARY_OFFSET = 0x8000

def to_int(v):
    if isinstance(v, int):
        return v
    if isinstance(v, str):
        s = v.strip().lower()
        if s.startswith('0x'):
            return int(s, 16)
        try:
            return int(s)
        except ValueError:
            return 0
    return 0

def collect_task_program_sizes(task, seen_tasks):
    task_id = task.get("debug_task_name")
    actual_task_name = task_id.split('*')[0] if task_id and '*' in task_id else task_id

    text_len = (to_int(task.get("text_length", 0)) + 63) & ~63
    data_len = ((to_int(task.get("data_length", 0)) + 63) & ~63) + 64
    total_len = to_int(task.get("total_length", 0))
    if actual_task_name in seen_tasks:
        return text_len, data_len, 0
    seen_tasks.add(actual_task_name)
    program_len = text_len + data_len
    return text_len, data_len, program_len

def collect_task_scalar_size(task):
    for k in ("scalar_size", "input_scalar_size", "params_size", "parameter_bytes"):
        if k in task:
            return to_int(task[k])
    if isinstance(task.get("scalar"), dict):
        return max(to_int(task["scalar"].get("size", 0)), to_int(task["scalar"].get("length", 0)))
    return 0

def sum_static_inputs(task, seen_names):
    """
    统计 all_input 中非指针静态量：type 为 0b001（parameter/global）或 0b010（dag_input/dfedata）。
    seen_names 跨 task 共享，保证同名变量全局只计一次。
    """
    total = 0
    inputs = task.get("all_input") or []
    if not isinstance(inputs, list):
        return 0
    for inp in inputs:
        if inp.get("type") in (TYPE_PARAM_GLOBAL, TYPE_DAG_DFE_STATIC):
            name = inp.get("name")
            if name not in seen_names:
                length = to_int(inp.get("length", 0))
                total += length
                seen_names.add(name)
    return total

def check_single_io_limits(task, issues):
    details = {"inputs": [], "outputs": [], "all_outputs": []}
    max_io = LIMITS["single_io_bytes_max"]

    # 检查 all_input
    inputs = task.get("all_input") or []
    if isinstance(inputs, list):
        for idx, inp in enumerate(inputs):
            size = to_int(inp.get("length", 0))
            name = inp.get("name", "")
            is_static_pointer_buffer = (
                inp.get("type") in (TYPE_PARAM_GLOBAL, TYPE_DAG_DFE_STATIC)
                and (name.startswith("workspace_") or size > LIMITS["task_scalar_bytes_max"])
            )
            if inp.get("type") in POINTER_TYPES or name.startswith("workspace_"):
                size = POINTER_INPUT_STRUCT_BYTES
            ok = size <= max_io
            dest_raw = inp.get("dest_address")
            scalar_range_ok = True
            if size > 0 and dest_raw is not None and not is_static_pointer_buffer:
                if not (isinstance(dest_raw, str) and dest_raw.strip().lower() == "null"):
                    start_addr = to_int(dest_raw)
                    if start_addr < 0x100000:
                        end_addr = start_addr + size - 1
                        if end_addr > 0x23FFF:
                            scalar_range_ok = False
                            issues.append(
                                f"[Task {task.get('debug_task_name') or task.get('current_taskId')} / input#{idx}] "
                                f"scalar input overflow: start={hex(start_addr)}, length={size}, end={hex(end_addr)} > 0x23fff"
                            )
            if not ok:
                issues.append(f"[Task {task.get('debug_task_name') or task.get('current_taskId')} / input#{idx}] length={size} > {max_io}")
            details["inputs"].append({"index": inp.get("index", idx), "length": size, "ok": ok and scalar_range_ok})

    # 检查 return_output
    ro = task.get("return_output") or task.get("outputs") or []
    for idx, outp in enumerate(ro):
        size = to_int(outp.get("length", 0))
        ok = size <= max_io
        if not ok:
            issues.append(f"[Task {task.get('debug_task_name') or task.get('current_taskId')} / return_output#{idx}] length={size} > {max_io}")
        details["outputs"].append({"index": outp.get("index", idx), "length": size, "ok": ok})

    # 检查 all_output（新增）
    all_outputs = task.get("all_output") or []
    if isinstance(all_outputs, list):
        for idx, outp in enumerate(all_outputs):
            size = to_int(outp.get("length", 0))
            ok = size <= max_io
            # 检查字段完整性
            has_name = "name" in outp
            has_temp_offset = "temp_offset" in outp
            has_length = "length" in outp
            has_parent = "parentTasksPort" in outp

            if not ok:
                issues.append(f"[Task {task.get('debug_task_name') or task.get('current_taskId')} / all_output#{idx}] length={size} > {max_io}")
            if not has_name:
                issues.append(f"[Task {task.get('debug_task_name') or task.get('current_taskId')} / all_output#{idx}] missing 'name' field")
            if not has_temp_offset:
                issues.append(f"[Task {task.get('debug_task_name') or task.get('current_taskId')} / all_output#{idx}] missing 'temp_offset' field")
            if not has_length:
                issues.append(f"[Task {task.get('debug_task_name') or task.get('current_taskId')} / all_output#{idx}] missing 'length' field")
            if not has_parent:
                issues.append(f"[Task {task.get('debug_task_name') or task.get('current_taskId')} / all_output#{idx}] missing 'parentTasksPort' field")

            details["all_outputs"].append({
                "name": outp.get("name", ""),
                "length": size,
                "temp_offset": outp.get("temp_offset", ""),
                "ok": ok and has_name and has_temp_offset and has_length and has_parent
            })

    return details

def analyze(tasks):
    issues = []
    per_task = []
    dag_program_sum = 0
    dag_static_sum = 0      # 0b001 + 0b010 静态输入字节合计
    scalar_unknown = []
    seen_static_names = set()   # 跨 task 去重，同名变量只计一次
    seen_tasks = set()
    for task in tasks:
        if "text_length" not in task and "data_length" not in task and "all_input" not in task:
            continue

        # print(task)
        text_len, data_len, program_len = collect_task_program_sizes(task, seen_tasks)
        dag_program_sum += program_len
        scalar_size = collect_task_scalar_size(task)
        if scalar_size == 0 and not any(k in task for k in ("scalar_size","input_scalar_size","params_size","parameter_bytes","scalar")):
            scalar_unknown.append(task.get("debug_task_name") or str(task.get("current_taskId")))

        static_bytes = sum_static_inputs(task, seen_static_names)
        dag_static_sum += static_bytes

        if text_len > LIMITS["task_text_bytes_max"]:
            issues.append(f"[Task {task.get('debug_task_name') or task.get('current_taskId')}] text_length={text_len} > {LIMITS['task_text_bytes_max']}")
        if data_len > LIMITS["task_data_bytes_max"]:
            issues.append(f"[Task {task.get('debug_task_name') or task.get('current_taskId')}] data_length={data_len} > {LIMITS['task_data_bytes_max']}")
        if scalar_size > LIMITS["task_scalar_bytes_max"]:
            issues.append(f"[Task {task.get('debug_task_name') or task.get('current_taskId')}] input_scalar_size={scalar_size} > {LIMITS['task_scalar_bytes_max']}")
        input_num = to_int(task.get("Input_Num", 0))
        output_num = to_int(task.get("Output_Num", 0))
        if input_num < LIMITS["task_input_num_min"]:
            issues.append(f"[Task {task.get('debug_task_name') or task.get('current_taskId')}] Input_Num={input_num} < {LIMITS['task_input_num_min']}")
        if input_num > LIMITS["task_input_num_max"]:
            issues.append(f"[Task {task.get('debug_task_name') or task.get('current_taskId')}] Input_Num={input_num} > {LIMITS['task_input_num_max']}")
        if output_num < LIMITS["task_output_num_min"]:
            issues.append(f"[Task {task.get('debug_task_name') or task.get('current_taskId')}] Output_Num={output_num} < {LIMITS['task_output_num_min']}")
        if output_num > LIMITS["task_output_num_max"]:
            issues.append(f"[Task {task.get('debug_task_name') or task.get('current_taskId')}] Output_Num={output_num} > {LIMITS['task_output_num_max']}")

        io_details = check_single_io_limits(task, issues)

        # 统计 all_output 数量和总字节数（与 return_output 分开）
        all_output_count = 0
        all_output_bytes = 0
        all_outputs = task.get("all_output") or []
        if isinstance(all_outputs, list):
            all_output_count = len(all_outputs)
            for outp in all_outputs:
                all_output_bytes += to_int(outp.get("length", 0))

        per_task.append({
            "task": task.get("debug_task_name") or f"task_{task.get('current_taskId')}",
            "text_length": text_len,
            "data_length": data_len,
            "program_length": program_len,
            "input_scalar_size": scalar_size,
            "static_input_bytes": static_bytes,
            "all_output_count": all_output_count,
            "all_output_bytes": all_output_bytes,
            "is_spmd": task.get("is_spmd", 0),
            "min_core_num": task.get("min_core_num", 1),
            "io_check": io_details
        })

    dag_total = dag_program_sum + dag_static_sum
    if dag_total > LIMITS["dag_total_bytes_max"]:
        issues.append(f"[DAG] program+static_input={dag_total} > {LIMITS['dag_total_bytes_max']}")

    report = {
        "limits": LIMITS,
        "summary": {
            "dag_program_bytes_sum": dag_program_sum,
            "dag_static_input_bytes_sum": dag_static_sum,
            "dag_total_bytes": dag_total,
        },
        "per_task": per_task,
        "notes": {},
        "ok": len(issues) == 0,
        "errors": issues
    }

    if scalar_unknown:
        report["notes"]["scalar_size_missing_for_tasks"] = scalar_unknown

    return report

# ──────────────────────────────────────────────
# 阶段四步骤2：临时变量检查
# ──────────────────────────────────────────────

def check_temp_variables(ij_dir: Path, temp_total_bytes: int, temp_align_bytes: int) -> dict:
    """
    读取 IJ/<dag>/temp_memory_map.json，执行四项检查：
    1. size_bytes 缺失或为 0
    2. 超出临时区总大小
    3. 生命周期重叠变量地址重叠（基于 temp_offset）
    4. temp_offset 未满足对齐要求
    返回 {"ok": bool, "errors": [...], "summary": {...}}
    """
    issues = []
    all_dag_summaries = {}

    map_files = list(ij_dir.glob("*/temp_memory_map.json"))
    if not map_files:
        # 没有临时变量，不报错，只记录
        return {"ok": True, "errors": [], "summary": {}, "note": "no temp_memory_map.json found"}

    for mf in map_files:
        try:
            mem_map = json.loads(mf.read_text(encoding="utf-8"))
        except Exception as e:
            issues.append(f"[{mf}] 读取失败: {e}")
            continue

        dag_name = mem_map.get("dag", mf.parent.name)
        allocs = mem_map.get("allocations", [])
        peak_bytes = to_int(mem_map.get("peak_bytes", 0))
        align_bytes = to_int(mem_map.get("align_bytes", temp_align_bytes))

        dag_issues = []

        # 检查1：size_bytes 缺失或为 0
        for entry in allocs:
            name = entry.get("name", "?")
            sb = entry.get("size_bytes")
            if sb is None or to_int(sb) == 0:
                dag_issues.append(f"[{dag_name}] 临时变量 '{name}' size_bytes 缺失或为 0")

        # 检查2：超出临时区总大小
        effective_total = temp_total_bytes if temp_total_bytes > 0 else to_int(mem_map.get("total_bytes", 0))
        if effective_total > 0 and peak_bytes > effective_total:
            dag_issues.append(
                f"[{dag_name}] 临时区峰值 {peak_bytes} bytes 超出总大小 {effective_total} bytes"
            )

        # 检查3：生命周期重叠变量地址重叠（基于 temp_offset）
        # 生命周期重叠语义与分配器一致：
        #   若 a.last_use_order <= b.birth_order，说明 a 在 b 诞生的同一个 task 内被最后消费，
        #   b 是该 task 的输出，a 和 b 不会并存，视为不重叠（允许复用）。
        # 两个变量生命周期重叠 = NOT (a.last_use_order <= b.birth_order OR b.last_use_order <= a.birth_order)
        # 地址重叠 = NOT (a_end <= b_start OR b_end <= a_start)
        for i, a in enumerate(allocs):
            a_offset = to_int(a.get("temp_offset", 0))
            a_size   = to_int(a.get("allocated_size_bytes", 0))
            a_end    = a_offset + a_size
            a_birth  = to_int(a.get("birth_order", 0))
            a_last   = to_int(a.get("last_use_order", 0))
            for b in allocs[i+1:]:
                b_offset = to_int(b.get("temp_offset", 0))
                b_size   = to_int(b.get("allocated_size_bytes", 0))
                b_end    = b_offset + b_size
                b_birth  = to_int(b.get("birth_order", 0))
                b_last   = to_int(b.get("last_use_order", 0))
                lifetime_overlap = not (a_last <= b_birth or b_last <= a_birth)
                addr_overlap     = not (a_end <= b_offset or b_end <= a_offset)
                if lifetime_overlap and addr_overlap:
                    dag_issues.append(
                        f"[{dag_name}] 生命周期重叠变量地址冲突: "
                        f"'{a.get('name')}' [offset {hex(a_offset)},{hex(a_end)}) order[{a_birth},{a_last}] 与 "
                        f"'{b.get('name')}' [offset {hex(b_offset)},{hex(b_end)}) order[{b_birth},{b_last}]"
                    )

        # 检查4：temp_offset 对齐
        for entry in allocs:
            name = entry.get("name", "?")
            offset = to_int(entry.get("temp_offset", 0))
            if align_bytes > 0 and offset % align_bytes != 0:
                dag_issues.append(
                    f"[{dag_name}] 临时变量 '{name}' temp_offset {hex(offset)} 未对齐到 {align_bytes} bytes"
                )

            size = to_int(entry.get("allocated_size_bytes", 0))
            if offset < LSU_STORE_BOUNDARY_OFFSET < offset + size:
                dag_issues.append(
                    f"[{dag_name}] 临时变量 '{name}' "
                    f"[offset {hex(offset)},{hex(offset + size)}) "
                    f"跨越 LSU {hex(LSU_STORE_BOUNDARY_OFFSET)} 边界"
                )

        issues.extend(dag_issues)
        all_dag_summaries[dag_name] = {
            "peak_bytes": peak_bytes,
            "total_bytes": effective_total,
            "utilization": f"{peak_bytes}/{effective_total}" if effective_total > 0 else f"{peak_bytes}/unknown",
            "num_temp_vars": len(allocs),
            "errors": dag_issues,
        }

    return {
        "ok": len(issues) == 0,
        "errors": issues,
        "summary": all_dag_summaries,
    }

def main():
    parser = argparse.ArgumentParser(description="DSL resource checker")
    parser.add_argument("input", help="final_output 目录或单个 JSON 文件")
    parser.add_argument("summary_out", nargs="?", help="单文件模式下的输出路径")
    parser.add_argument("--temp-base-addr",  default="0x0",  help="临时变量区基地址（来自 config.mk）")
    parser.add_argument("--temp-total-bytes", type=lambda x: int(x, 0), default=0,
                        help="临时变量区总大小（字节）")
    parser.add_argument("--temp-align-bytes", type=lambda x: int(x, 0), default=128,
                        help="临时变量对齐字节数")
    args = parser.parse_args()

    in_path = Path(args.input)

    # IJ 目录与 final_output 同级
    ij_dir = in_path.parent / "IJ" if in_path.is_dir() else in_path.parent.parent / "IJ"

    # ── 临时变量检查（无论单文件还是目录模式都执行）──
    temp_result = check_temp_variables(ij_dir, args.temp_total_bytes, args.temp_align_bytes)
    temp_ok = temp_result["ok"]
    if not temp_ok:
        for err in temp_result["errors"]:
            print(f"\033[91m[TEMP] {err}\033[0m", file=sys.stderr)
    if temp_result["summary"]:
        for dag_name, s in temp_result["summary"].items():
            print(f"[TEMP] {dag_name}: 峰值 {s['peak_bytes']} bytes / 总 {s['total_bytes']} bytes"
                  f"  ({s['num_temp_vars']} 个临时变量)")

    # === 目录模式 ===
    if in_path.is_dir():
        json_files = [f for f in in_path.glob("*.json") if not f.stem.endswith("_debug")]
        if not json_files:
            print(f"\033[91mNo JSON files found in {in_path}\033[0m", file=sys.stderr)
            sys.exit(3)

        failed = []
        for jf in json_files:
            try:
                data = json.loads(jf.read_text(encoding="utf-8"))
                if isinstance(data, dict):
                    for key in ("tasks", "dag", "nodes"):
                        if key in data and isinstance(data[key], list):
                            data = data[key]
                            break
                    if isinstance(data, dict):
                        data = [data]

                if not isinstance(data, list):
                    print(f"\033[91m[{jf}] Input JSON must be a list of task objects.\033[0m", file=sys.stderr)
                    failed.append(jf)
                    continue

                report = analyze(data)
                report["temp_check"] = temp_result
                out_path = jf.with_name(jf.stem + "_debug.json")
                out_path.write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding="utf-8")

                if not report["ok"]:
                    print(f"\033[91m[{jf}] Resource check FAILED\033[0m", file=sys.stderr)
                    failed.append(jf)
                else:
                    print(f"\033[92m[{jf}] Resource check PASSED\033[0m")

            except Exception as e:
                print(f"\033[91m[{jf}] Exception: {e}\033[0m", file=sys.stderr)
                failed.append(jf)

        if not temp_ok:
            failed.append(Path("temp_check"))

        if failed:
            print(f"\033[91m{len(failed)} check(s) failed: {[f.name for f in failed]}\033[0m", file=sys.stderr)
            sys.exit(1)
        else:
            print("\033[92mAll checks passed.\033[0m")
            sys.exit(0)

    # === 单文件模式 ===
    else:
        if not args.summary_out:
            print("\033[91mUsage: dsl_resource_checker.py <input_json> <summary_out.json>\033[0m", file=sys.stderr)
            sys.exit(2)

        out_path = Path(args.summary_out)
        data = json.loads(in_path.read_text(encoding="utf-8"))

        if isinstance(data, dict):
            for key in ("tasks", "dag", "nodes"):
                if key in data and isinstance(data[key], list):
                    data = data[key]
                    break
            if isinstance(data, dict):
                data = [data]

        if not isinstance(data, list):
            print("\033[91mInput JSON must be a list of task objects or contain a 'tasks' list.\033[0m", file=sys.stderr)
            sys.exit(3)

        report = analyze(data)
        report["temp_check"] = temp_result
        out_path.write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding="utf-8")

        overall_ok = report["ok"] and temp_ok
        if not overall_ok:
            print("\033[91mResource check FAILED. See summary for details.\033[0m", file=sys.stderr)
            sys.exit(1)

        print("\033[92mResource check PASSED. Summary written to\033[0m", out_path)

if __name__ == "__main__":
    main()
