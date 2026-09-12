"""
temporary_variable_allocator.py
临时变量静态地址分配器

输入：
  ./IJ/<dag>/temp_variables.json   （由 basinterp.py 生成）

输出：
  ./variable/map/<dag>_temp_alloc.json
  ./IJ/<dag>/temp_memory_map.json

用法（由 Makefile 调用）：
  python3 ./script/temporary_variable_allocator.py \
      --dag <dag_name> \
      --base-addr <TEMP_VRF_BASE_ADDR> \
      --total-bytes <TEMP_VRF_TOTAL_BYTES> \
      --align-bytes <TEMP_ALLOC_ALIGN_BYTES> \
      --lane-num <VENUSLANE>

实际执行规则（与代码一致）：
    1. 变量处理顺序
        - 按 birth_order 升序处理所有临时变量
    2. 生命周期与释放
        - 在处理当前变量前，释放所有满足：
            last_use_order <= 当前 birth_order 且 birth_order < 当前 birth_order 的块
        - 释放后加入 free_blocks
    3. 行对齐与占用规则
        - 行大小固定为 64 字节（row_bytes = 64）
        - 每个变量的起始地址必须对齐到 64B
        - 实际占用大小为：
            align_up(size_bytes, 64)
        - 分配和释放都以整行粒度进行
        - 同一行不会被多个变量共享
    4. 空闲块管理
        - 每轮分配前：
            - 将 [peak_offset, total_bytes) 作为新空闲块加入
            - 对 free_blocks 做合并（相邻或重叠块）
        - 空闲块始终保持按 offset 合并后的区间集合
    5. 分配策略（first-fit + 行对齐）
        - 按 offset 从小到大遍历 free_blocks
        - 对每个块：
            - 计算 candidate_offset = 向上对齐到 64B
            - 若 candidate_offset + 占用大小 在该块范围内，则分配
        - 命中后立即分配（first-fit），不做 best-fit
        - 分配后从空闲块中精确切割该区间
    6. 峰值更新
        - peak_offset 记录已使用空间上界
        - 若分配后超过当前 peak，则更新
    7. 分配失败
        - 若所有空闲块均无法满足分配需求：
            抛出 MemoryError（超出 total_bytes）
    8. 输出结果
        - temp_offset：相对 base_addr 的偏移
        - allocated_size_bytes：原始 size_bytes（未对齐值）
"""

import argparse
import json
import os


LSU_STORE_BOUNDARY_OFFSET = 0x8000


def align_up(value, align):
    """向上对齐到 align 的倍数。"""
    if align <= 0:
        return value
    return ((value + align - 1) // align) * align


def avoid_lsu_store_boundary_crossing(offset, size):
    """单个临时变量若跨越 0x8000，则整体从 0x8000 开始分配。"""
    if offset < LSU_STORE_BOUNDARY_OFFSET < offset + size:
        return LSU_STORE_BOUNDARY_OFFSET
    return offset


def _remap_lifetimes_with_heft(temp_vars, dag_name):
    with open(f"./heft_new/DAG/{dag_name}.json", "r") as f:
        heft_tasks = json.load(f)
    schedule = {
        t["debug_task_name"]: int(t["current_taskId"])
        for t in heft_tasks
        if "debug_task_name" in t and "current_taskId" in t
    }
    max_order = max(schedule.values()) if schedule else 0
    with open(f"./IJ/{dag_name}/input_tasks.json", "r") as f:
        input_tasks = json.load(f)
    consumers = {}
    for task in input_tasks:
        consumer = task["taskId"]               #错误：consumer = task["taskId"].split("*")[0]，必须保留完整实例名
        # if consumer in ("concat", "slice"):
        if consumer.split("*")[0] in ("concat", "slice"):
            continue
        for parent in task.get("parentTasks", []):
            var = parent.get("outputVar")
            if var:
                consumers.setdefault(var, set()).add(consumer)
    for var in temp_vars:
        # prod = var["producer_task_id"].split("*")[0]
        prod = var["producer_task_id"]
        var["birth_order"] = schedule[prod]
        var["producer_task_order"] = schedule[prod]
        uses = consumers.get(var["name"], [])
        if var.get("is_return_value"):
            var["last_use_order"] = max_order
        else:
            var["last_use_order"] = max(schedule[u] for u in uses) if uses else var["birth_order"]
    return temp_vars

def allocate(temp_vars, base_addr, total_bytes, align_bytes, lane_num):
    """
    对临时变量列表做静态分配（行首对齐，行独占）。

    分配策略：
      A. is_pinned=True 的变量：按 pin_order 升序从 offset 0 bump 分配，
         占用 align_up(size, 64)，彼此紧挨，永不释放、永不被普通 temp 复用。
      B. 其余变量：从 pinned_region_end 起按原 first-fit + 生命周期释放；
         free_blocks / peak 均不得落入 [0, pinned_region_end)。
      C. 输出 result 的顺序必须与入参 temp_vars 一致（不得因 pin 重排），
         以保证下游 task_info 生成的 all_output 端口顺序不变。

    参数/返回：与原接口相同。
    """
    row_bytes = 64  # 一行的字节数，固定为 64B（硬件规格）

    pinned_vars = sorted(
        [v for v in temp_vars if v.get("is_pinned")],
        key=lambda v: int(v.get("pin_order", 0)),
    )
    normal_vars = sorted(
        [v for v in temp_vars if not v.get("is_pinned")],
        key=lambda v: v["birth_order"],
    )

    offset_by_name = {}
    pinned_region_end = 0

    # -------- Phase A: pin arena（从 0 bump）--------
    for var in pinned_vars:
        size_raw = var["size_bytes"]
        occupied_size = align_up(size_raw, row_bytes)
        offset = avoid_lsu_store_boundary_crossing(
            pinned_region_end, occupied_size
        )
        if offset + occupied_size > total_bytes:
            raise MemoryError(
                f"Pinned temp region overflow while allocating '{var['name']}': "
                f"need {occupied_size} bytes at offset {offset}, total_bytes={total_bytes}."
            )
        pinned_region_end = offset + occupied_size
        offset_by_name[var["name"]] = offset

    # -------- Phase B: 普通 temp（原 first-fit，起点=pinned_region_end）--------
    allocated = []
    free_blocks = []
    peak_offset = pinned_region_end

    for var in normal_vars:
        birth = var["birth_order"]
        last_use = var["last_use_order"]
        size_raw = var["size_bytes"]

        still_alive = []
        for blk in allocated:
            if blk["last_use_order"] <= birth and blk["birth_order"] < birth:
                free_blocks.append({"offset": blk["offset"], "size": blk["size"]})
            else:
                still_alive.append(blk)
        allocated = still_alive

        free_blocks.append({"offset": peak_offset, "size": total_bytes - peak_offset})
        free_blocks = _merge_free_blocks(free_blocks)

        occupied_size = align_up(size_raw, row_bytes)
        offset = None

        for fb in sorted(free_blocks, key=lambda b: b["offset"]):
            candidate_offset = align_up(fb["offset"], row_bytes)
            if candidate_offset < pinned_region_end:
                candidate_offset = align_up(pinned_region_end, row_bytes)
            candidate_offset = avoid_lsu_store_boundary_crossing(
                candidate_offset, occupied_size
            )
            candidate_end = candidate_offset + occupied_size

            if candidate_offset < fb["offset"]:
                continue
            if candidate_end > fb["offset"] + fb["size"]:
                continue

            offset = candidate_offset
            free_blocks = _carve_from_free_at_offset(free_blocks, fb, offset, occupied_size)
            break

        if offset is None:
            raise MemoryError(
                f"Temporary VRF region overflow: need {occupied_size} bytes "
                f"but total_bytes={total_bytes}. "
                f"Increase TEMP_VRF_TOTAL_BYTES in config.mk."
            )

        if offset + occupied_size > peak_offset:
            peak_offset = offset + occupied_size

        allocated.append({
            "offset": offset,
            "size": align_up(size_raw, row_bytes),
            "last_use_order": last_use,
            "birth_order": birth,
        })
        offset_by_name[var["name"]] = offset

    # -------- Phase C: 按入参顺序输出（禁止 pin 重排破坏 all_output）--------
    result = []
    for var in temp_vars:
        entry = dict(var)
        entry["temp_offset"] = offset_by_name[var["name"]]
        entry["allocated_size_bytes"] = var["size_bytes"]
        entry.pop("dest_address", None)
        result.append(entry)

    return result, peak_offset


def _merge_free_blocks(free_blocks):
    """合并相邻或重叠的空闲块。"""
    if not free_blocks:
        return []
    sorted_blocks = sorted(free_blocks, key=lambda b: b["offset"])
    merged = [dict(sorted_blocks[0])]
    for blk in sorted_blocks[1:]:
        last = merged[-1]
        if blk["offset"] <= last["offset"] + last["size"]:
            # 相邻或重叠，合并
            end = max(last["offset"] + last["size"], blk["offset"] + blk["size"])
            last["size"] = end - last["offset"]
        else:
            merged.append(dict(blk))
    return merged


def _best_fit(free_blocks, size, peak_offset):
    """
    从空闲块中选 best-fit：
      1. 只考虑 size >= 需求的块
      2. 选最小的（best-fit）
      3. 并列时低地址优先
      4. 再并列时优先不抬高峰值边界（即 offset + size <= peak_offset）
    返回选中的空闲块 dict，或 None。
    """
    candidates = [b for b in free_blocks if b["size"] >= size]
    if not candidates:
        return None

    # 按 (size, offset) 排序：size 最小优先，同 size 低地址优先
    candidates.sort(key=lambda b: (b["size"], b["offset"]))

    # 在最小 size 的候选里，优先选不抬高峰值边界的
    min_size = candidates[0]["size"]
    min_size_candidates = [b for b in candidates if b["size"] == min_size]

    # 不抬高峰值边界：offset + size <= peak_offset
    no_peak_raise = [b for b in min_size_candidates if b["offset"] + size <= peak_offset]
    if no_peak_raise:
        return min(no_peak_raise, key=lambda b: b["offset"])

    return min_size_candidates[0]


def _carve_from_free_at_offset(free_blocks, chosen, offset, size):
    """从空闲块 chosen 的指定 offset 处切割 size 字节，剩余部分放回。"""
    result = []
    for blk in free_blocks:
        if blk["offset"] == chosen["offset"] and blk["size"] == chosen["size"]:
            # 切割前段（offset 之前的部分）
            if offset > blk["offset"]:
                result.append({"offset": blk["offset"], "size": offset - blk["offset"]})
            # 切割后段（offset + size 之后的部分）
            remainder_offset = offset + size
            remainder_end = blk["offset"] + blk["size"]
            if remainder_offset < remainder_end:
                result.append({"offset": remainder_offset, "size": remainder_end - remainder_offset})
        else:
            result.append(blk)
    return result


def _carve_from_free(free_blocks, chosen, size):
    """从空闲列表中切割 chosen 块的 size 字节，剩余部分放回。"""
    result = []
    for blk in free_blocks:
        if blk["offset"] == chosen["offset"] and blk["size"] == chosen["size"]:
            remainder = blk["size"] - size
            if remainder > 0:
                result.append({"offset": blk["offset"] + size, "size": remainder})
            # 已消耗，不放回
        else:
            result.append(blk)
    return result


def main():
    parser = argparse.ArgumentParser(description="Temporary variable static allocator")
    parser.add_argument("--dag", required=True, help="DAG name (e.g. dag1)")
    parser.add_argument("--base-addr", required=True,
                        help="Temp VRF base address (hex or dec, e.g. 0x80200000)")
    parser.add_argument("--total-bytes", required=True, type=int,
                        help="Temp VRF total size in bytes")
    parser.add_argument("--align-bytes", required=True, type=int,
                        help="Allocation alignment in bytes")
    parser.add_argument("--lane-num", required=True, type=int,
                        help="Number of VRF lanes (VENUSLANE)")
    args = parser.parse_args()

    dag_name = args.dag
    base_addr = int(args.base_addr, 0)  # 支持 0x 前缀
    total_bytes = args.total_bytes
    align_bytes = args.align_bytes
    lane_num = args.lane_num

    # 输入文件
    input_path = f"./IJ/{dag_name}/temp_variables.json"
    if not os.path.exists(input_path):
        raise FileNotFoundError(
            f"Input file not found: {input_path}. "
            f"Run basinterp.py first to generate temp_variables.json."
        )

    with open(input_path, "r") as f:
        temp_vars = json.load(f)

    if not temp_vars:
        print(f"[temp_alloc] {dag_name}: no temporary variables, nothing to allocate.")
        # 写空输出
        os.makedirs(f"./IJ/{dag_name}", exist_ok=True)
        os.makedirs("./variable/map", exist_ok=True)
        with open(f"./IJ/{dag_name}/temp_memory_map.json", "w") as f:
            json.dump({"dag": dag_name, "base_addr": hex(base_addr),
                       "total_bytes": total_bytes, "peak_bytes": 0,
                       "allocations": []}, f, indent=4)
        with open(f"./variable/map/{dag_name}_temp_alloc.json", "w") as f:
            json.dump([], f, indent=4)
        return
    
    temp_vars = _remap_lifetimes_with_heft(temp_vars, dag_name)

    with open("./debug.txt", "a", encoding="utf-8") as f:
        f.write(f"[temp_lifetime] {dag_name}: {len(temp_vars)} variable(s) after HEFT remap.\n")
        for entry in sorted(temp_vars, key=lambda v: v["birth_order"]):
            f.write(
                f"  {entry['name']}: birth={entry['birth_order']}, "
                f"last_use={entry['last_use_order']}, "
                f"size_bytes={entry['size_bytes']}, "
                f"is_pinned={entry.get('is_pinned', False)}, "
                f"pin_order={entry.get('pin_order', -1)}\n"
            )

    # 按 birth_order 排序
    temp_vars_sorted = sorted(temp_vars, key=lambda v: v["birth_order"])

    # 执行分配
    result, peak_bytes = allocate(
        temp_vars_sorted, base_addr, total_bytes, align_bytes, lane_num
    )

    # 输出1：./variable/map/<dag>_temp_alloc.json
    os.makedirs("./variable/map", exist_ok=True)
    alloc_path = f"./variable/map/{dag_name}_temp_alloc.json"
    with open(alloc_path, "w") as f:
        json.dump(result, f, indent=4)

    # 输出2：./IJ/<dag>/temp_memory_map.json
    os.makedirs(f"./IJ/{dag_name}", exist_ok=True)
    memory_map = {
        "dag": dag_name,
        "base_addr": hex(base_addr),
        "total_bytes": total_bytes,
        "peak_bytes": peak_bytes,
        "peak_addr": hex(base_addr + peak_bytes),
        "row_bytes": 64,  # 固定为 64B（硬件规格）
        "align_bytes": align_bytes,
        "allocations": [
            {
                "name": r["name"],
                "size_bytes": r["size_bytes"],
                "allocated_size_bytes": r["allocated_size_bytes"],
                "birth_order": r["birth_order"],
                "last_use_order": r["last_use_order"],
                "temp_offset": r["temp_offset"],
            }
            for r in result
        ],
    }
    map_path = f"./IJ/{dag_name}/temp_memory_map.json"
    with open(map_path, "w") as f:
        json.dump(memory_map, f, indent=4)

    print(f"[temp_alloc] {dag_name}: {len(result)} variable(s) allocated, "
          f"peak={peak_bytes} bytes ({hex(peak_bytes)}), "
          f"base={hex(base_addr)}, peak_addr={hex(base_addr + peak_bytes)}")
    for r in result:
        print(f"  {r['name']}: temp_offset={r['temp_offset']} ({hex(r['temp_offset'])}) "
              f"size={r['size_bytes']}B -> allocated={r['allocated_size_bytes']}B "
              f"[{r['birth_order']}, {r['last_use_order']}]")


if __name__ == "__main__":
    main()
