#!/usr/bin/env python3
"""Extract CAU/SerDiv result-path clock edges from a small RTL VCD window."""

from __future__ import annotations

import argparse
import json
import re
from pathlib import Path


VAR_RE = re.compile(
    r"^\$var\s+\S+\s+\d+\s+(?P<code>\S+)\s+(?P<name>\S+)"
)


def bit_value(text):
    if text is None or any(char in text.lower() for char in "xz"):
        return None
    return int(text, 2)


def selected_codes(names):
    result = {}
    for code, paths in names.items():
        for path in paths:
            if path.endswith(".clk_i"):
                result["clk"] = code
            if ".has_cau.u_cau_wrap." in path:
                leaf = path.rsplit(".", 1)[-1]
                result["cau." + leaf] = code
            if ".has_serdiv.u_serdiv_wrap." in path:
                leaf = path.rsplit(".", 1)[-1]
                result["serdiv." + leaf] = code
            if ".has_alu.u_bitalu_wrap." in path:
                leaf = path.rsplit(".", 1)[-1]
                if leaf in {
                    "result_queue_cnt_q", "result_queue_cnt_d",
                    "result_queue_valid_q", "bitalu_operand_ready_o",
                    "bitalu_result_req_o", "bitalu_result_gnt_i",
                    "issue_cnt_q",
                }:
                    result["bitalu." + leaf] = code
            if path.endswith(".cau_result_id_vfu_oq"):
                result["cau.result_id"] = code
            if path.endswith(".cau_result_addr_vfu_oq"):
                result["cau.result_addr"] = code
            if path.endswith(".cau_result_req_vfu_oq"):
                result["lane.cau_result_req"] = code
            if path.endswith(".cau_result_gnt_oq_vfu"):
                result["lane.cau_result_gnt"] = code
            if path.endswith(".serdiv_result_id_vfu_oq"):
                result["serdiv.result_id"] = code
            if path.endswith(".serdiv_result_addr_vfu_oq"):
                result["serdiv.result_addr"] = code
            if path.endswith(".vfu_operation_valid_ls_vfu"):
                result["lane.vfu_operation_valid"] = code
            if path.endswith(".vfu_operation_ls_vfu"):
                result["lane.vfu_operation"] = code
            if ".u_operand_requester." in path:
                leaf = path.rsplit(".", 1)[-1]
                if leaf in {
                    "req_ls", "req_lvl2", "gnt_lvl2",
                    "operand_req", "operand_gnt",
                    "operand_queue_ready_i", "operand_req_valid_i",
                    "operand_req_ready_o", "operand_issued_o",
                }:
                    result["requester." + leaf] = code
                if ".gen_operand_requester[" in path and leaf in {
                    "state_q", "requester_q", "stall", "request_bank",
                }:
                    requester = path.rsplit(
                        ".gen_operand_requester[", 1)[1].split("]", 1)[0]
                    result[
                        "requester.%s.%s" % (requester, leaf)
                    ] = code
            if ".u_opqueue_bitalu_b." in path:
                leaf = path.rsplit(".", 1)[-1]
                if leaf in {
                    "ibuf_usage_q", "ibuf_pop", "operand_valid_i",
                    "operand_valid_o", "operand_ready_i",
                }:
                    result["queue.BitAluB." + leaf] = code
            if path.endswith(".u_dspm.rdata_valid_q"):
                result["lane.rdata_valid_q"] = code
            if path.endswith(".vrf_operand_valid_dspm_oq"):
                result["lane.vrf_operand_valid"] = code
    return result


def parse_change(line):
    if not line:
        return None
    if line[0] in "01xXzZ":
        return line[1:], line[0].lower()
    if line[0] in "bB":
        value, code = line[1:].split(None, 1)
        return code, value.lower()
    return None


def asserted(state, code):
    return code is not None and bit_value(state.get(code)) not in (None, 0)


def number(state, code):
    return bit_value(state.get(code)) if code is not None else None


MASTER_NAMES = {
    0: "BitAluA",
    1: "BitAluB",
    2: "CAUA",
    3: "CAUB",
    4: "CAUC",
    5: "CAUD",
    6: "SerDivA",
    7: "SerDivB",
    8: "BitALUResult",
    9: "CAUResult",
    10: "SerDivResult",
    11: "Shuffle",
}


def decode_vfu_operation(value):
    """Decode the 73-bit gc0802 ``vfu_operation_t`` packed struct."""
    if value is None:
        return None
    fields = (
        ("rid", 3),
        ("op", 6),
        ("use_vs1", 1),
        ("use_vs2", 1),
        ("use_vd1_op", 1),
        ("use_vd2_op", 1),
        ("vfu_id", 3),
        ("vd1_head", 7),
        ("vd2_head", 7),
        ("vl", 15),
        ("vew", 2),
        ("vm_r", 1),
        ("vm_w", 1),
        ("use_scalar_op", 1),
        ("scalar_op", 16),
        ("cau_mul_shamt", 4),
        ("saturate_en", 3),
    )
    remaining = 73
    decoded = {}
    for name, width in fields:
        remaining -= width
        decoded[name] = (value >> remaining) & ((1 << width) - 1)
    assert remaining == 0
    return decoded


def set_masters(state, code, bank):
    """Decode logic [3:0][11:0] as four contiguous 12-master banks."""
    value = number(state, code)
    if value is None:
        return None
    bank_value = (value >> (bank * 12)) & 0xfff
    return [
        MASTER_NAMES.get(master, str(master))
        for master in range(12)
        if bank_value & (1 << master)
    ]


def arbitration_snapshot(state, codes, addr):
    if addr is None:
        return {}
    row = addr >> 2
    bank = ((addr & 0x3) + (row & 0x3)) & 0x3
    req_ls = number(state, codes.get("requester.req_ls"))
    return {
        "bank": bank,
        "lsu_priority": (
            None if req_ls is None else bool(req_ls & (1 << bank))
        ),
        "req_masters": set_masters(
            state, codes.get("requester.req_lvl2"), bank
        ),
        "rr_gnt_masters": set_masters(
            state, codes.get("requester.gnt_lvl2"), bank
        ),
        "effective_gnt_masters": set_masters(
            state, codes.get("requester.operand_gnt"), bank
        ),
    }


def edge_events(time_fs, cycle, pre, post, codes):
    events = []
    bitalu_cnt_q = codes.get("bitalu.result_queue_cnt_q")
    if bitalu_cnt_q is not None:
        events.append({
            "time_fs": time_fs,
            "time_ns": time_fs / 1_000_000.0,
            "cycle": cycle,
            "event": "result_queue_state",
            "vfu": "BitALU",
            "queue_count_pre": number(pre, bitalu_cnt_q),
            "queue_count_post": number(post, bitalu_cnt_q),
            "queue_count_d_pre": number(
                pre, codes.get("bitalu.result_queue_cnt_d")),
            "queue_count_d_post": number(
                post, codes.get("bitalu.result_queue_cnt_d")),
            "queue_valid_pre": number(
                pre, codes.get("bitalu.result_queue_valid_q")),
            "queue_valid_post": number(
                post, codes.get("bitalu.result_queue_valid_q")),
            "operand_ready_pre": number(
                pre, codes.get("bitalu.bitalu_operand_ready_o")),
            "operand_ready_post": number(
                post, codes.get("bitalu.bitalu_operand_ready_o")),
            "result_req_pre": number(
                pre, codes.get("bitalu.bitalu_result_req_o")),
            "result_req_post": number(
                post, codes.get("bitalu.bitalu_result_req_o")),
            "result_gnt_pre": number(
                pre, codes.get("bitalu.bitalu_result_gnt_i")),
            "result_gnt_post": number(
                post, codes.get("bitalu.bitalu_result_gnt_i")),
            "issue_count_pre": number(
                pre, codes.get("bitalu.issue_cnt_q")),
            "issue_count_post": number(
                post, codes.get("bitalu.issue_cnt_q")),
        })
    queue_keys = sorted(
        key for key in codes if key.startswith("queue.BitAluB."))
    if queue_keys:
        events.append({
            "time_fs": time_fs,
            "time_ns": time_fs / 1_000_000.0,
            "cycle": cycle,
            "event": "operand_queue_state",
            "vfu": "BitAluB",
            "pre": {
                key.rsplit(".", 1)[-1]: number(pre, codes[key])
                for key in queue_keys
            },
            "post": {
                key.rsplit(".", 1)[-1]: number(post, codes[key])
                for key in queue_keys
            },
            "rdata_valid_pre": number(
                pre, codes.get("lane.rdata_valid_q")),
            "rdata_valid_post": number(
                post, codes.get("lane.rdata_valid_q")),
            "operand_valid_pre": number(
                pre, codes.get("lane.vrf_operand_valid")),
            "operand_valid_post": number(
                post, codes.get("lane.vrf_operand_valid")),
        })
    queue_ready = number(
        post, codes.get("requester.operand_queue_ready_i"))
    issued = number(post, codes.get("requester.operand_issued_o"))
    req_valid = number(
        post, codes.get("requester.operand_req_valid_i"))
    req_ready = number(
        post, codes.get("requester.operand_req_ready_o"))
    requester_state_codes = [
        key for key in codes if key.startswith("requester.") and
        key.rsplit(".", 1)[-1] in {
            "state_q", "requester_q", "stall", "request_bank",
        }
    ]
    if any(value is not None for value in (
            queue_ready, issued, req_valid, req_ready)) or \
            requester_state_codes:
        requester_states = {}
        for key in requester_state_codes:
            _, requester, leaf = key.split(".")
            requester_states.setdefault(requester, {})[leaf] = number(
                post, codes[key])
        events.append({
            "time_fs": time_fs,
            "time_ns": time_fs / 1_000_000.0,
            "cycle": cycle,
            "event": "requester_state",
            "vfu": "VRF",
            "operand_queue_ready": queue_ready,
            "operand_issued": issued,
            "operand_req_valid": req_valid,
            "operand_req_ready": req_ready,
            "requesters": requester_states,
        })
    operation_valid = codes.get("lane.vfu_operation_valid")
    operation = decode_vfu_operation(
        number(post, codes.get("lane.vfu_operation")))
    if asserted(post, operation_valid) and operation is not None:
        vfu_names = {
            0: "BitALU",
            1: "CAU",
            2: "SerDiv",
            3: "Shuffle",
            4: "LDU",
            5: "STU",
            6: "Mask",
            7: "None",
        }
        events.append({
            "time_fs": time_fs,
            "time_ns": time_fs / 1_000_000.0,
            "cycle": cycle,
            "event": "admit",
            "vfu": vfu_names.get(
                operation["vfu_id"], str(operation["vfu_id"])),
            **operation,
        })
    req_ls = number(post, codes.get("requester.req_ls"))
    if req_ls:
        events.append({
            "time_fs": time_fs,
            "time_ns": time_fs / 1_000_000.0,
            "cycle": cycle,
            "event": "lsu_priority",
            "vfu": "LSU",
            "banks": [bank for bank in range(4) if req_ls & (1 << bank)],
        })
    for bank in range(4):
        req_masters = set_masters(
            post, codes.get("requester.req_lvl2"), bank)
        if not req_masters:
            continue
        events.append({
            "time_fs": time_fs,
            "time_ns": time_fs / 1_000_000.0,
            "cycle": cycle,
            "event": "vrf_arbitration",
            "vfu": "VRF",
            "bank": bank,
            "lsu_priority": (
                None if req_ls is None else bool(req_ls & (1 << bank))
            ),
            "req_masters": req_masters,
            "rr_gnt_masters": set_masters(
                post, codes.get("requester.gnt_lvl2"), bank),
            "effective_gnt_masters": set_masters(
                post, codes.get("requester.operand_gnt"), bank),
        })
    # Some focused VCDs expose the CAU result requester only at the lane
    # boundary, without result address/ID signals inside u_cau_wrap.  Recover
    # its physical bank from master 9 in req_lvl2 so these captures still form
    # a complete RTL requester/grant oracle.
    if asserted(post, codes.get("lane.cau_result_req")):
        request_banks = []
        for bank in range(4):
            if "CAUResult" in (set_masters(
                    post, codes.get("requester.req_lvl2"), bank) or []):
                request_banks.append(bank)
        bank = request_banks[0] if len(request_banks) == 1 else None
        lsu_priority = None
        rr_gnt_masters = None
        if bank is not None:
            lsu_priority = (
                None if req_ls is None else bool(req_ls & (1 << bank)))
            rr_gnt_masters = set_masters(
                post, codes.get("requester.gnt_lvl2"), bank)
        events.append({
            "time_fs": time_fs,
            "time_ns": time_fs / 1_000_000.0,
            "cycle": cycle,
            "event": (
                "lane_cau_result_grant"
                if asserted(post, codes.get("lane.cau_result_gnt"))
                else "lane_cau_result_blocked"
            ),
            "vfu": "CAU",
            "bank": bank,
            "request_banks": request_banks,
            "lsu_priority": lsu_priority,
            "rr_gnt_masters": rr_gnt_masters,
        })
    for vfu in ("cau", "serdiv"):
        prefix = vfu + "."
        req = codes.get(prefix + (
            "cau_result_req_o" if vfu == "cau" else "serdiv_result_req_o"
        ))
        gnt = codes.get(prefix + (
            "cau_result_gnt_i" if vfu == "cau" else "serdiv_result_gnt_i"
        ))
        mask_gnt = codes.get(prefix + "mask_jump_gnt")
        out_valid = codes.get(prefix + (
            "cau_out_valid" if vfu == "cau" else "serdiv_out_valid"
        ))
        in_ready = codes.get(prefix + (
            "cau_in_ready" if vfu == "cau" else "serdiv_in_ready"
        ))
        cnt_q = codes.get(prefix + "result_queue_cnt_q")
        cnt_d = codes.get(prefix + "result_queue_cnt_d")
        commit_q = codes.get(prefix + "commit_cnt_q")
        commit_d = codes.get(prefix + "commit_cnt_d")
        done = codes.get(prefix + (
            "cau_vinsn_done_o" if vfu == "cau" else "serdiv_vinsn_done_o"
        ))
        rid = codes.get(prefix + "result_id")
        addr = codes.get(prefix + "result_addr")

        common = {
            "time_fs": time_fs,
            "time_ns": time_fs / 1_000_000.0,
            "cycle": cycle,
            "vfu": "CAU" if vfu == "cau" else "SerDiv",
        }
        if asserted(pre, out_valid) and asserted(pre, in_ready):
            events.append({
                **common,
                "event": "enqueue_handshake",
                "queue_count": number(pre, cnt_q),
                "queue_count_d": number(pre, cnt_d),
                "commit_count": number(pre, commit_q),
            })
        if asserted(pre, req) and (
            asserted(pre, gnt) or asserted(pre, mask_gnt)
        ):
            events.append({
                **common,
                "event": "vrf_grant_capture",
                "rid": number(pre, rid),
                "addr": number(pre, addr),
                "queue_count": number(pre, cnt_q),
                "commit_count": number(pre, commit_q),
                "commit_count_d": number(pre, commit_d),
            })
        if asserted(post, req):
            post_addr = number(post, addr)
            arb = arbitration_snapshot(post, codes, post_addr)
            if asserted(post, gnt) or asserted(post, mask_gnt):
                events.append({
                    **common,
                    "event": "vrf_grant",
                    "rid": number(post, rid),
                    "addr": post_addr,
                    "queue_count": number(post, cnt_q),
                    "commit_count": number(post, commit_q),
                    "commit_count_d": number(post, commit_d),
                    **arb,
                })
            else:
                events.append({
                    **common,
                    "event": "vrf_grant_blocked",
                    "rid": number(post, rid),
                    "addr": post_addr,
                    "queue_count": number(post, cnt_q),
                    **arb,
                })
        pre_count = number(pre, cnt_q)
        post_count = number(post, cnt_q)
        if pre_count != post_count:
            events.append({
                **common,
                "event": "queue_count",
                "before": pre_count,
                "after": post_count,
            })
        pre_done = number(pre, done)
        post_done = number(post, done)
        if post_done not in (None, 0) and post_done != pre_done:
            events.append({
                **common,
                "event": "done",
                "done_mask": post_done,
                "commit_count_before": number(pre, commit_q),
                "commit_count_d_after": number(post, commit_d),
            })
    return events


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("vcd", type=Path)
    parser.add_argument("--vfu", choices=("BitALU", "CAU", "SerDiv"))
    parser.add_argument(
        "--start-ns", type=float,
        help="Only emit sampled events at or after this RTL time",
    )
    parser.add_argument(
        "--end-ns", type=float,
        help="Only emit sampled events at or before this RTL time",
    )
    args = parser.parse_args()

    scopes = []
    names = {}
    in_header = True
    state = {}
    group_time = None
    group_changes = []
    cycle = -1

    def selected(event):
        if args.vfu is not None and event["vfu"] != args.vfu:
            return False
        if args.start_ns is not None and event["time_ns"] < args.start_ns:
            return False
        if args.end_ns is not None and event["time_ns"] > args.end_ns:
            return False
        return True

    def process_group():
        nonlocal cycle
        if group_time is None:
            return []
        pre = dict(state)
        clock_code = codes.get("clk")
        posedge = False
        for code, value in group_changes:
            if code == clock_code and value == "1" and pre.get(code) != "1":
                posedge = True
            state[code] = value
        if not posedge:
            return []
        cycle += 1
        return edge_events(group_time, cycle, pre, state, codes)

    with args.vcd.open(encoding="utf-8", errors="replace") as stream:
        for raw_line in stream:
            line = raw_line.strip()
            if in_header:
                if line.startswith("$scope"):
                    scopes.append(line.split()[2])
                elif line.startswith("$upscope"):
                    scopes.pop()
                else:
                    match = VAR_RE.match(line)
                    if match:
                        path = ".".join(scopes + [match["name"]])
                        names.setdefault(match["code"], []).append(path)
                if line.startswith("$enddefinitions"):
                    in_header = False
                    codes = selected_codes(names)
                continue

            if line.startswith("#"):
                for event in process_group():
                    if selected(event):
                        try:
                            print(json.dumps(event, sort_keys=True))
                        except BrokenPipeError:
                            return 0
                group_time = int(line[1:])
                group_changes = []
                continue
            change = parse_change(line)
            if change is not None:
                group_changes.append(change)

    for event in process_group():
        if selected(event):
            try:
                print(json.dumps(event, sort_keys=True))
            except BrokenPipeError:
                return 0
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
