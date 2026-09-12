#include "VenusSequencer.hh"
#include "venus/VenusSharedL2.hh"
#include "venus_instr_pkt.hh"
#include "sim/sim_exit.hh"
#include "mem/se_translating_port_proxy.hh"
#include "mem/noncoherent_xbar.hh"

#include <iostream>
#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

namespace gem5
{

namespace
{
constexpr uint32_t VenusSharedMemBase = 0x70000;
constexpr uint32_t VenusSharedMemLimit = 0x88000;
constexpr size_t VenusSharedMemHalfWords =
    (VenusSharedMemLimit - VenusSharedMemBase) / sizeof(uint16_t);
constexpr int VenusLsuQueueDepth = 4;
/*
 * The LSU/sequencer path can reopen after eight tile clocks.  A nine-cycle
 * spacing is common in scalar streams, but that is the maximum of this
 * structural ready interval and the next scalar request's arrival, not an
 * LSU initiation interval.  Keeping nine here incorrectly hides the
 * dispatcher phase in independent streams and shifts the STU capacity
 * cliff by one cycle per admitted follower.
 */
constexpr int VenusLduAcceptIntervalCycles = 8;
constexpr int VenusStuAcceptIntervalCycles = 8;
/*
 * From the sequencer's PE-valid edge, addrgen captures the request and then
 * presents the tile-side AR channel two registered tile clocks later.  STU
 * asserts its operand-bank request one tile clock after the PE-valid edge.
 * Once every bank grants that request, vstu.sv drives axi_w_valid_o from the
 * registered operand ping-pong row on the following tile edge.  The task17
 * sequence-96 oracle exposes the same boundary (stu_req at 569573 ns, local
 * W handshake at 569575 ns), so operand-ready to local-W is one tile clock.
 */
constexpr int VenusLduAddrgenInternalArCycles = 2;
/*
 * When VLDu's four-entry instruction queue is full, addrgen can retain the
 * fifth request while the oldest load finishes.  The retained request does
 * not present a new tile-side AR on the old load's modeled recycle edge:
 * commit_cnt_q first exposes the free slot, the waiting load is captured,
 * and AR becomes visible on the following registered tile edge.  The
 * task17 sequence-187--194 queue-full oracle observes old pe_resp/recycle,
 * new queue admission, and the new internal AR on successive boundaries.
 */
constexpr int VenusLduHeldRequestAfterRecycleCycles = 1;
constexpr int VenusLduAddrgenAckCycles = 1;
constexpr int VenusStuAddrgenAckCycles = 2;
constexpr int VenusStuOperandRequestCycles = 1;
constexpr int VenusStuOperandToLocalWriteCycles = 1;
/*
 * An STU completion and an opposite-direction addrgen request on
 * the same tile edge cross a registered WLAST/direction-select boundary.
 * RTL presents the new internal AR/AW one tile clock later. A
 * same-direction descriptor or a request arriving after the release edge
 * does not pay this boundary.
 */
constexpr int VenusLsuDirectionSwitchCycles = 1;
/*
 * Once a full W CDC publishes the destination read pointer back to the tile,
 * VSTU still clears WLAST/d1 and hands direction ownership through six
 * registered tile-side boundaries.  A descriptor which was already queued
 * behind a same-direction addrgen head crosses the FIFO pop/head D/Q path as
 * well.  Keep those two stages explicit instead of replacing the pointer and
 * queue state with one task-derived opposite-direction constant.
 */
constexpr int VenusLsuPostSourceReleaseTileCycles = 6;
constexpr int VenusLsuQueuedAddrgenPopTileCycles = 2;
/*
 * vstu keeps issue_cnt_bytes_q live until AXI W consumes the buffered row.
 * Meanwhile its alternate ping-pong row is empty, so stu_operand_req_o
 * advances to seq_word_wr_offset_q + 1 for one speculative VRF row.  The
 * requester therefore owns the banks for one row beyond the architectural
 * payload, even for a sub-row store.
 */
constexpr int VenusStuOperandLookaheadRows = 1;

uint64_t
venusLsuRequesterTag(const VenusInstrPkt *pkt, unsigned row, bool load)
{
    return (uint64_t(pkt->vns_instr_id) << 16) |
        (uint64_t(load) << 15) | uint64_t(row & 0x7fff);
}

uint64_t
venusFullVrfDataBankMask()
{
    const unsigned banks = NrLanes * NrBankPerLane;
    panic_if(banks > 64, "Venus LSU requester vector exceeds 64 banks");
    return banks == 64 ? ~uint64_t(0) : (uint64_t(1) << banks) - 1;
}
/*
 * WLAST clears VSTU's two operand rows for two tile clocks. The registered
 * issue-pointer and requester/operand return boundaries make the next local
 * W possible four tile clocks after WLAST. W-CDC capacity and read-pointer
 * backpressure are modeled beat by beat by VenusSharedL2.
 */
constexpr int VenusStuPostWlastRestartTileCycles = 4;
constexpr int VenusLsuFirstResponseCycles = 4;
constexpr int VenusLsuBeatCycles = 2;
/*
 * VLDu crosses a 2:1 tile-to-AXI clock boundary on both sides of the shared
 * memory.  The sequence-97 RTL waveform exposes the following boundaries:
 *
 *   internal AR -> source wptr  1 tile clock
 *   source wptr -> CDC AR       next AXI sample + 2 AXI clocks
 *   CDC AR      -> external AR  1 AXI clock (axi_cut)
 *   external R  -> cut R        1 AXI clock (axi_cut)
 *   cut R       -> source wptr  1 AXI clock
 *   source wptr -> internal R   next tile sample + 2 tile clocks
 *
 * The shared fabric/RAM stages between external AR and external R belong to
 * VenusSharedL2.  Keeping these boundaries explicit preserves the real clock
 * phase dependence instead of folding the path into a fitted fixed latency.
 */
constexpr int VenusLduOutboundCdcAxiStagesAfterNextEdge = 2;
constexpr int VenusLduOutboundCutAxiStages = 1;
constexpr int VenusLduReturnCutAxiStages = 1;
constexpr int VenusLduReturnCdcTileStagesAfterNextEdge = 2;
/*
 * VSTU B is registered once by axi_cut, then the B CDC source advances its
 * Gray write pointer on the following AXI edge.  A coincident tile edge has
 * already sampled the old pointer, so the destination needs three later tile
 * edges for sync stage 1, sync stage 2, and the spill register.  Keeping
 * these boundaries outside VenusSharedL2 lets the shared object stop at the
 * external slave B response, just as the LDU path stops at external R.
 */
constexpr int VenusStuReturnCutAxiStages = 1;
constexpr int VenusStuReturnSourcePointerAxiStages = 1;
constexpr int VenusStuReturnCdcTileStagesAfterSourceEdge = 3;
/*
 * R data is written into result_queue_d on the handshake edge.  The
 * registered result_queue_q drives ldu_result_req_o one tile clock later.
 * This is also the first edge on which the LDU owns every requested VRF bank.
 */
constexpr int VenusLduVrfWordVisibilityCycles = 1;
constexpr int VenusLduResultQueueVisibilityCycles = 1;
constexpr int VenusLduFinalGrantCycles = 1;
constexpr int VenusLduPeResponseCycles = 1;
constexpr int VenusLduSequencerRecycleCycles = 1;
/* BVALID -> registered pe_resp -> sequencer recycle visibility. */
constexpr int VenusStuCommitCycles = 2;

std::filesystem::path
venusDebugRoot()
{
    const char *root = std::getenv("VENUS_GEM5_DEBUG_DIR");
    return (root != nullptr && root[0] != '\0') ? root : "Debug";
}

bool
venusVinsResultDumpEnabled()
{
    static const bool enabled =
        std::getenv("VENUS_GEM5_DISABLE_VINS_RESULT_DUMP") == nullptr;
    return enabled;
}

bool
venusVinsMonitorEnabled()
{
    static const bool enabled =
        std::getenv("VENUS_GEM5_DISABLE_VINS_MONITOR") == nullptr;
    return enabled;
}

std::ofstream &
venusLsuTrace()
{
    // The four RTL tile sequencers execute on gem5's single event queue and
    // must share one stream.  Independent truncating streams corrupt JSONL
    // and destroy the global request/response/completion order.
    static std::ofstream trace;
    return trace;
}

void
initializeVenusLsuTrace(Tick tickPeriod)
{
    const char *path = std::getenv("VENUS_GEM5_LSU_TRACE");
    if (path == nullptr || path[0] == '\0' || venusLsuTrace().is_open())
        return;

    venusLsuTrace().open(path, std::ios::out | std::ios::trunc);
    fatal_if(!venusLsuTrace(), "cannot open Venus LSU trace %s", path);
    venusLsuTrace() << "{\"event\":\"metadata\","
                    << "\"schema\":\"venus-lsu-events-v1\","
                    << "\"source\":\"gem5\","
                    << "\"tick_period\":" << tickPeriod << "}\n";
    venusLsuTrace().flush();
}

void
writeSharedMemoryByte(std::vector<uint16_t> &sharedMemory, size_t byteIdx,
                      uint8_t value)
{
    const size_t halfWordIdx = byteIdx / 2;
    if (halfWordIdx >= sharedMemory.size())
        return;

    const uint16_t oldValue = sharedMemory[halfWordIdx];
    if (byteIdx & 0x1)
        sharedMemory[halfWordIdx] =
            (oldValue & 0x00ff) | (static_cast<uint16_t>(value) << 8);
    else
        sharedMemory[halfWordIdx] = (oldValue & 0xff00) | value;
}

size_t
loadTextByteArray(const std::filesystem::path &path,
                  std::vector<uint16_t> &sharedMemory)
{
    std::ifstream input(path);
    if (!input.is_open())
        return 0;

    std::string text((std::istreambuf_iterator<char>(input)),
                     std::istreambuf_iterator<char>());
    const size_t openBrace = text.find('{');
    if (openBrace == std::string::npos)
        return 0;

    size_t byteIdx = 0;
    for (size_t pos = openBrace + 1; pos < text.size();) {
        while (pos < text.size() &&
               !(text[pos] == '-' || std::isdigit(static_cast<unsigned char>(text[pos])))) {
            if (text[pos] == '}')
                return byteIdx;
            ++pos;
        }
        if (pos >= text.size() || text[pos] == '}')
            break;

        char *endPtr = nullptr;
        const long parsed = std::strtol(text.c_str() + pos, &endPtr, 10);
        if (endPtr == text.c_str() + pos)
            break;
        writeSharedMemoryByte(sharedMemory, byteIdx,
                              static_cast<uint8_t>(parsed));
        ++byteIdx;
        pos = static_cast<size_t>(endPtr - text.c_str());
    }

    return byteIdx;
}

size_t
loadBinaryBytes(const std::filesystem::path &path,
                std::vector<uint16_t> &sharedMemory)
{
    std::ifstream input(path, std::ios::binary);
    if (!input.is_open())
        return 0;

    size_t byteIdx = 0;
    char byte = 0;
    while (input.get(byte)) {
        writeSharedMemoryByte(sharedMemory, byteIdx,
                              static_cast<uint8_t>(byte));
        ++byteIdx;
        if (byteIdx >= sharedMemory.size() * sizeof(uint16_t))
            break;
    }
    return byteIdx;
}

}

VenusOp
decodeOpcodeCustom2(uint8_t funct5, FUNC3 func3, VEW vew)
{
    // custom-2: 0x5B
    switch (funct5) {
        case 0x00:
            if (func3 == OPMISC) return VMNOT;
            return VAND;
        case 0x01:
            if (func3 == OPMISC && vew == EW16) return VRANGE;
            return VOR;
        case 0x02:
            if (func3 == OPMISC) return VSHUFFLE_CLBMV;
            return VBRDCST;
        case 0x03: return VSLL;
        case 0x04: return VSRL;
        case 0x05: return VSRA;
        case 0x06: return VXOR;
        case 0x07: return VABS;
        case 0x08: return VSHUFFLE;
        case 0x0E: return VLOAD;
        case 0x0F: return VSTORE;
        case 0x10: return VSEQ;
        case 0x11: return VSNE;
        case 0x12: return VSLTU;
        case 0x13: return VSLT;
        case 0x14: return VSLEU;
        case 0x15: return VSLE;
        case 0x16: return VSGTU;
        case 0x17: return VSGT;
        default: return VAND;
    }
}

VenusOp
decodeOpcodeCustom1(uint8_t funct5, FUNC3 func3)
{
    // custom-1: 0x2B
    switch (funct5) {
        case 0x00: return VADD;
        case 0x01: return VRSUB;
        case 0x02: return VSUB;
        case 0x03: return VMUL;
        case 0x04: return VMULH;
        case 0x05: return VMULHU;
        case 0x06: return VMULHSU;
        case 0x07: return VMULADD;
        case 0x08: return VMULSUB;
        case 0x09: return VADDMUL;
        case 0x0A: return VSUBMUL;
        case 0x0B: return VCMXMUL;
        case 0x0C: return VDIV;
        case 0x0D: return VREM;
        case 0x0E: return VDIVU;
        case 0x0F: return VREMU;
        case 0x10: return VSADD;
        case 0x11: return VSADDU;
        case 0x12: return VSSUB;
        case 0x13: return VSSUBU;
        case 0x14: return VMIN;
        case 0x15: return VMAX;
        case 0x18: return VREDAND;
        case 0x19: return VREDOR;
        case 0x1A: return VREDXOR;
        case 0x1B: return VREDMAX;
        case 0x1C: return VREDMAXU;
        case 0x1D: return VREDMIN;
        case 0x1E: return VREDMINU;
        case 0x1F: return VREDSUM;
        default: return VADD;
    }
}

Port & VenusSequencer::getPort(const std::string &if_name, PortID idx)
{
    // 检查请求的端口名称是否与 Python 文件中定义的 "port_venussequencer_receivefrom_venuspacketgen" 匹配
    if (if_name == "port_venussequencer_receivefrom_venuspacketgen") {
        return port_venussequencer_receivefrom_venuspacketgen; // 返回对应的端口对象
    }
    if (if_name == "port_venussequencer_sendto_venuslane") {
        return port_venussequencer_sendto_venuslane[idx]; // 返回对应的端口对象
    }
    if (if_name == "port_venussequencer_sendto_venusshuffle") {
        return port_venussequencer_sendto_venusshuffle; // 返回对应的端口对象
    }
    if (if_name == "port_venussequencer_hazardtable_boardcast") {
        return port_venussequencer_hazardtable_boardcast[idx]; // 返回对应的端口对象
    }
    if (if_name == "port_venussequencer_hazardtable_boardcast_to_shuffle") {
        return port_venussequencer_hazardtable_boardcast_to_shuffle; // 返回对应的端口对象
    }
    // 如果名称不匹配，则调用基类方法（可能会报错，但符合框架规范）
    return ClockedObject::getPort(if_name, idx);
}

VenusInstrPkt*
VenusSequencer::decodeVenusInstruction(uint64_t inst_bits, uint32_t scalar_op, uint32_t avl, uint32_t mulshamt, uint32_t msbhead, uint32_t mulsaturate, uint32_t lsu_addr_msb)
{
    // VenusInstrPkt* pkt = new VenusInstrPkt();
    uint32_t msb = inst_bits & 0xFFFFFFFF;
    uint8_t funct5 = (msb >> 27) & 0x1F; // [31:27] funct5
    VEW vew = static_cast<VEW>((msb >> 26) & 0x1); // [26] vew
    bool vm_r = (msb >> 25) & 0x1;  // [25] vmask_read
    // uint8_t avl_high = (msb >> 20) & 0x1F;  // [24:20]
    // uint8_t avl_low = (msb >> 7) & 0x1F;    // [11:7]
    // int vl = (avl_high << 5) | avl_low;    // [24:20] + [11:7] = avl
    // [19:18] reserved
    // uint8_t vd_head_msb = (msb >> 17) & 0x1;    // [17] vd_head[10]
    // uint8_t vs2_head_msb = (msb >> 16) & 0x1;   // [16] vs2_head[10]
    // uint8_t vs1_head_msb = (msb >> 15) & 0x1;   // [15] vs1_head[10]
    FUNC3 func3 = static_cast<FUNC3>((msb >> 12) & 0x7); // [14:12] funct3
    uint8_t opcode_base = msb & 0x7F;   // [6:0] opcode
    // venus_pkg.sv defines vlen_t as 15 bits and the RTL dispatcher casts
    // scalar_req.avl_op to vlen_t before it enters the sequencer.
    avl &= 0x7fff;
    VenusOp op;
    if (opcode_base == 0x5B) {          // 7'b1011011 (custom-2)
        op = decodeOpcodeCustom2(funct5, func3, vew);
    } else if (opcode_base == 0x2B) {  // 7'b0101011 (custom-1)
        op = decodeOpcodeCustom1(funct5, func3);
    } else {
        fatal("VenusSequencer::decodeVenusInstruction: Invalid opcode_base 0x%x in instruction 0x%016x\n"
          "  funct5=0x%x, funct3=%d, vew=%d, vl=%d\n"
          "  Expected opcode_base: 0x5B (custom-2) or 0x2B (custom-1)",
          opcode_base, inst_bits,
          funct5, (int)func3, (int)vew, avl);
    }
    uint32_t lsb = (inst_bits >> 32) & 0xFFFFFFFF;
    int vs1_head, vs2_head, vd1_head, vd2_head;
    uint8_t vs1_msb = msbhead & 0x3;
    uint8_t vs2_msb = (msbhead >> 2) & 0x3;
    uint8_t vd1_msb = (msbhead >> 4) & 0x3;
    uint8_t vd2_msb = (msbhead >> 6) & 0x3;
    if ((op >= VMULADD && op <= VCMXMUL) && func3 == IVV) {
        // 4oprand：[31:24] vd2, [23:16] vd1, [15:8] vs2, [7:0] vs1
        vd2_head = (lsb >> 24) & 0xFF;
        vd1_head = (lsb >> 16) & 0xFF;
        vs2_head = (lsb >> 8) & 0xFF;
        vs1_head = lsb & 0xFF;
        vs1_head = (vs1_msb << 8) | vs1_head;   // [9:0] = msb + lsb
        vs2_head = (vs2_msb << 8) | vs2_head;
        vd1_head = (vd1_msb << 8) | vd1_head;
        vd2_head = (vd2_msb << 8) | vd2_head;
    } else {
        // 3oprand：[29:20] vd1, [19:10] vs2, [9:0] vs1
        vd1_head = ((lsb >> 20) & 0x3FF);
        vs2_head = ((lsb >> 10) & 0x3FF);
        vs1_head = (lsb & 0x3FF);
        vd2_head = 0;
    }
    // RTL stores every decoded row index in vrow_t, whose width is
    // $clog2(NrLines).  Assigning the 10-bit instruction field therefore
    // truncates high bits on a 128-row tile (for example row 136 becomes 8).
    // Mirror that structural behavior before constructing the packet.
    const int row_mask = NrLines - 1;
    vs1_head &= row_mask;
    vs2_head &= row_mask;
    vd1_head &= row_mask;
    vd2_head &= row_mask;
    unsigned char vfu_shamt = mulshamt & 0xff;
    unsigned char saturate_pre_adder = mulsaturate & 0x1;
    unsigned char saturate_multiplier = (mulsaturate >> 1) & 0x1;
    unsigned char saturate_post_adder = (mulsaturate >> 2) & 0x1;

    if (!(func3 == IVX) || (func3 == MVX)) {
        scalar_op = 0;
    } else if (opcode_base == 0x5B && func3 == IVX && (funct5 == 0x0E || funct5 == 0x0F)) {
        scalar_op = ((lsu_addr_msb & 0xfff) << 16) | (scalar_op & 0xffff);
    }
    VenusInstrPkt* pkt = new VenusInstrPkt(
        op,
        vew,
        func3,
        avl,
        vs1_head,
        vs2_head,
        vd1_head,
        vd2_head,
        vm_r,
        scalar_op,
        vfu_shamt,
        saturate_pre_adder,
        saturate_multiplier,
        saturate_post_adder
    );
    // std::cout << "at tick = " << curTick() << ", Decoded Venus instruction:\n"
    //       << "  bit=0x" << std::hex << std::setw(16) << std::setfill('0') << inst_bits << std::dec << "\n"
    //       << "  op=" << (int)op << ", func3=" << (int)func3 << ", vew=" << (int)vew << "\n"
    //       << "  vl=" << avl << ", vs1=" << vs1_head << ", vs2=" << vs2_head << "\n"
    //       << "  vd1=" << vd1_head << ", vd2=" << vd2_head << ", vm_r=" << vm_r << "\n"
    //       << "  scalar_op=" << scalar_op << "\n"
    //       << "  vfu_shamt=" << (int)vfu_shamt << "\n"
    //       << "  saturate_pre_adder=" << (int)saturate_pre_adder << "\n"
    //       << "  saturate_multiplier=" << (int)saturate_multiplier << "\n"
    //       << "  saturate_post_adder=" << (int)saturate_post_adder
    //       << std::endl;

    return pkt;
}

bool
VenusSequencer::VenusSequencerRVSidePort::recvTimingReq(PacketPtr pkt)
{
    if (owner->scalarDispatchQueue.size() >=
        owner->scalarDispatchDepth) {
        return false;
    }

    // Try to see if this is already a VenusInstrPkt (sent by VenusPacketGen)
    VenusInstrPkt* venus_pkt_already = dynamic_cast<VenusInstrPkt*>(pkt);
    if (venus_pkt_already) {
        owner->scalarDispatchQueue.push_back(venus_pkt_already);
        owner->scalarDispatchReadyTicks.push_back(
            owner->clockEdge(Cycles(1)));
        if (!owner->nextDispatchScalarReqEvent.scheduled()) {
            owner->schedule(owner->nextDispatchScalarReqEvent,
                            owner->scalarDispatchReadyTicks.front());
        }
        return true;
    }

    const uint64_t* data = pkt->getConstPtr<uint64_t>();
    uint64_t venus_inst_bits = data[0];
    uint32_t vs1 = data[1] & 0xFFFFFFFF;
    uint32_t avl = (data[1] >> 32) & 0xFFFFFFFF;
    uint32_t mulshamt = (data[2] >> 32) & 0xFFFFFFFF;
    uint32_t msbhead = data[2] & 0xFFFFFFFF;
    uint32_t mulsaturate = data[3] & 0xFFFFFFFF;
    uint32_t lsu_addr_msb = (data[3] >> 32) & 0xFFFFFFFF;
    DPRINTF(VenusSequencerFull,"Received Venus packet: inst=0x%016lx, vs1=0x%x, avl=0x%x, mulshamt=0x%x, msbhead=0x%x, mulsaturate=0x%x, lsu_addr_msb=0x%x",
          venus_inst_bits, vs1, avl, mulshamt, msbhead, mulsaturate, lsu_addr_msb);
    VenusInstrPkt* venus_pkt = owner->decodeVenusInstruction(venus_inst_bits, vs1, avl, mulshamt, msbhead, mulsaturate, lsu_addr_msb);
    owner->scalarDispatchQueue.push_back(venus_pkt);
    /*
     * scalar600 presents the packed request only after collecting the two
     * 32-bit instruction words.  Venus1 then crosses two registered
     * boundaries before venus_sequencer observes issue_valid: the scalar
     * request output register and the sequencer input transfer register.
     * The same-key RTL dispatch observer therefore measures three Venus
     * clocks from the first valid packed request to issue acceptance.  Keep
     * this as a structural CPU-to-sequencer property; direct packet sources
     * still use the one-cycle path above.
     */
    owner->scalarDispatchReadyTicks.push_back(owner->clockEdge(Cycles(
        owner->rtlScalarSecondWordBoundary ? 3 : 1)));
    DPRINTF(VenusScalarDispatch,
            "CPU scalar-dispatch enqueue op=%s tick=%llu ready=%llu "
            "depth=%llu second_word_boundary=%d\n",
            op_to_str(venus_pkt->op),
            static_cast<unsigned long long>(curTick()),
            static_cast<unsigned long long>(
                owner->scalarDispatchReadyTicks.back()),
            static_cast<unsigned long long>(
                owner->scalarDispatchQueue.size()),
            owner->rtlScalarSecondWordBoundary);
    delete pkt;
    if (!owner->nextDispatchScalarReqEvent.scheduled()) {
        owner->schedule(owner->nextDispatchScalarReqEvent,
                        owner->scalarDispatchReadyTicks.front());
    }
    return true;
}
// 端口发送
void VenusSequencer::VenusSequencerVenusLaneSidePort::sendPacket(PacketPtr pkt) {
    panic_if(blockedPacket != nullptr, "Should never try to send if blocked!");
    if (!sendTimingReq(pkt)) {
        // std::cout << "at tick = " << curTick() << ", VenusSequencerVenusLaneSidePort's sendTimingReq function get an nack." << std::endl;
        blockedPacket = pkt;
        isBlocked = true;
        return;
    }
    isBlocked = false;
}
// 端口重发
void VenusSequencer::VenusSequencerVenusLaneSidePort::recvReqRetry()
{
    assert(blockedPacket != nullptr);
    PacketPtr pkt = blockedPacket;
    blockedPacket = nullptr;
    sendPacket(pkt);
}

void
VenusSequencer::VenusSequencerVenusLaneSidePort::noteVenusLaneProducerGrant(
    int laneId, int runningId, int instructionId)
{
    owner->noteLaneProducerGrant(laneId, runningId, instructionId);
}

bool VenusSequencer::VenusSequencerVenusLaneSidePort::recvTimingResp(PacketPtr pkt)
{
    auto *instr = static_cast<VenusInstrPkt *>(pkt);
    DPRINTF(VenusSequencerFull, "VenusSequencerVenusLaneSidePort,received a response, instr %d with RunningVID = %d is done.\n", instr->vns_instr_id, instr->running_id);
    owner->noteRtlLaneVfuDone(laneId, instr);
    /* bitalu_vinsn_done_o for a reduction is generated by the final
     * RED_COMMIT bank grant and is consumed directly by the sequencer's
     * combinational pe_resp path.  Ordinary lane results still cross the
     * registered response boundary modelled by queueLaneDoneReport(). */
    if (owner->experimentalRequesterQVisibility &&
        !owner->rtlRegisteredPeResponse &&
        instr->op >= VREDAND && instr->op <= VREDSUM) {
        owner->noteRtlLaneDone(laneId, instr);
        return owner->handleVinsnDoneReport(instr);
    }
    if (owner->experimentalRequesterQVisibility) {
        return owner->queueLaneDoneReport(instr, laneId);
    }
    owner->noteRtlLaneDone(laneId, instr);
    return owner->handleVinsnDoneReport(instr);
}

void VenusSequencer::VenusSequencerVenusShuffleSidePort::sendPacket(PacketPtr pkt) {
    panic_if(blockedPacket != nullptr, "Should never try to send if blocked!");
    if (!sendTimingReq(pkt)) {
        // std::cout << "at tick = " << curTick() << ", VenusSequencerVenusShuffleSidePort's sendTimingReq function get an nack." << std::endl;
        blockedPacket = pkt;
        isBlocked = true;
        return;
    }
    isBlocked = false;
}
void VenusSequencer::VenusSequencerVenusShuffleSidePort::recvReqRetry()
{
    assert(blockedPacket != nullptr);
    PacketPtr pkt = blockedPacket;
    blockedPacket = nullptr;
    sendPacket(pkt);
}
void
VenusSequencer::VenusSequencerVenusShuffleSidePort::
noteVenusShuffleProducerGrant(int runningId, int instructionId)
{
    owner->noteShuffleProducerGrant(runningId, instructionId);
}
bool VenusSequencer::VenusSequencerVenusShuffleSidePort::recvTimingResp(PacketPtr pkt)
{
    DPRINTF(VenusSequencerFull, "VenusSequencerVenusShuffleSidePort,received a response, instr %d with RunningVID = %d is done.\n", ((VenusInstrPkt*)pkt)->vns_instr_id, ((VenusInstrPkt*)pkt)->running_id);
    /*
     * pe_resp clears the producer from global_hazard_table_d in this
     * response cycle.  Send that tagged combinational observation to the
     * lane requesters now; releasing the VID remains a separate registered
     * boundary below.
     */
    auto *instr = static_cast<VenusInstrPkt *>(pkt);
    if (owner->rtlRegisteredPeResponse) {
        owner->queueProducerCompletionToLanes(
            instr->running_id, instr->vns_instr_id);
    } else {
        owner->forwardProducerCompletionToLanes(instr);
    }
    /*
     * venus_sequencer.sv removes pe_resp from vinsn_running_d in the
     * response cycle.  Its release detector samples that falling value on
     * the following tile edge before publishing the released VID.  Keep
     * shuffle completion on the same registered done-report boundary; a
     * direct callback released the ID one cycle too early even though the
     * shuffle engine's final grant/complete sequence was already exact.
     */
    return owner->queueLaneDoneReport(instr);
}



void VenusSequencer::VenusSequencerHazardTableBroadcastPort::sendPacket(PacketPtr pkt)
{
    sendTimingReq(pkt);
}


void VenusSequencer::init()
{
    initializeVenusLsuTrace(clockPeriod());
    for(int i=0;i<NrIDs;i++)
    {
        vinsn_running_pkt[i] = nullptr;
        laneProducerCompletionQueuedGeneration[i] = -1;
        rtlPeRunningQ[i] = false;
        rtlRunningQ[i] = false;
        rtlIssuePending[i] = false;
        rtlCompletionPending[i] = false;
        std::fill(std::begin(rtlVfuQueueReleased[i]),
                  std::end(rtlVfuQueueReleased[i]), false);
    }
    venus_hazard_table = new VenusHazardTable();
    for(int i=0;i<NrIDs;i++)
    {
        for(int j=0;j<NrIDs;j++)
        {
            venus_hazard_table->global_hazard_table[i][j] = false;
            venus_hazard_table->vs1_hazard_table[i][j] = false;
            venus_hazard_table->vs2_hazard_table[i][j] = false;
            venus_hazard_table->vd1_hazard_table[i][j] = false;
            venus_hazard_table->vd2_hazard_table[i][j] = false;
            venus_hazard_table->vm_hazard_table[i][j] = false;
            venus_hazard_table->raw_hazard_table[i][j] = false;
            venus_hazard_table->war_hazard_table[i][j] = false;
            venus_hazard_table->waw_hazard_table[i][j] = false;
        }
    }
    for(int i=0;i<NrVFUs;i++)
    {
        vfu_queue_counter[i] = 0;
    }
    if (venusVinsResultDumpEnabled()) {
        const auto debugRoot = venusDebugRoot();
        const auto resultDir = debugRoot / "venusgem5_vins_result";
        std::filesystem::create_directories(debugRoot);
        if (std::filesystem::exists(resultDir))
            std::filesystem::remove_all(resultDir);
        std::filesystem::create_directories(resultDir);
    } else if (venusVinsMonitorEnabled()) {
        std::filesystem::create_directories(venusDebugRoot());
    }

    VenusInstrPkt::createVinsInfo();
    initSharedMemory();
}
// DrainState VenusSequencer::drain()
// {
//     VenusInstrPkt::saveVinsInfo();
//     return DrainState::Drained;
// }

bool VenusSequencer::handleRequest(VenusInstrPkt* pkt)
{
    if(isBusy) {
        return false;
    }
    if (sequencerIssueState != SequencerIssueState::UpstreamReceive)
        return false;
    if(checkVinsnQueueisFull()) {
        DPRINTF(VenusSequencerFull, "vinsn_running_pkt in VenusSequencer isFull\n");
        return false;
    }

    /*
     * The register-range scoreboard identifies true RAW/WAR/WAW
     * dependencies independently of task, PC and opcode sequence.  Legacy
     * profiles hold all such dependencies here.  RTL profiles defer
     * cross-VFU dependencies to the per-operand requester model, while
     * same-VFU dependencies remain here until the gem5 operand data queues
     * carry RTL-equivalent instruction tags.
     */
    struct RegRange {
        int head;
        int tail;
    };
    auto overlaps = [](const RegRange &a, const RegRange &b) {
        return a.head <= b.tail && b.head <= a.tail;
    };
    auto ranges = [](const VenusInstrPkt *insn, bool writes) {
        std::vector<RegRange> result;
        if (writes) {
            if (insn->use_vd1 || insn->use_vd1_op)
                result.push_back({insn->vd1_head, insn->vd1_tail});
            if (insn->use_vd2 || insn->use_vd2_op)
                result.push_back({insn->vd2_head, insn->vd2_tail});
        } else {
            if (insn->use_vs1)
                result.push_back({insn->vs1_head, insn->vs1_tail});
            if (insn->use_vs2)
                result.push_back({insn->vs2_head, insn->vs2_tail});
            // Accumulate-style destinations are also read operands.
            if (insn->use_vd1_op)
                result.push_back({insn->vd1_head, insn->vd1_tail});
            if (insn->use_vd2_op)
                result.push_back({insn->vd2_head, insn->vd2_tail});
        }
        return result;
    };
    const auto newReads = ranges(pkt, false);
    const auto newWrites = ranges(pkt, true);
    auto [vfutmp1, vfutmp2, vfu_mtmp] = findVinsnTargetVFU(pkt);
    for (auto *older : vinsn_running_pkt) {
        if (older == nullptr)
            continue;
        const auto oldReads = ranges(older, false);
        const auto oldWrites = ranges(older, true);
        bool raw = pkt->vm_r && older->vm_w;
        bool war = pkt->vm_w && older->vm_r;
        bool waw = pkt->vm_w && older->vm_w;
        for (const auto &read : newReads)
            for (const auto &write : oldWrites)
                raw |= overlaps(read, write);
        for (const auto &write : newWrites) {
            for (const auto &read : oldReads)
                war |= overlaps(write, read);
            for (const auto &oldWrite : oldWrites)
                waw |= overlaps(write, oldWrite);
        }
        const bool dependency = raw || war || waw;
        /*
         * RTL allocates the running ID before resolving these dependencies
         * and carries the hazard bits into the independent operand
         * requesters.  Keep the conservative admission gate only for the
         * legacy all-or-nothing lane model.
         */
        const bool operandSpecificHazards = true;
        if (dependency) {
            if (!operandSpecificHazards)
                return false;
            auto [olderVfu1, olderVfu2, olderMaskVfu] =
                findVinsnTargetVFU(older);
            const bool laneVfuPair =
                (vfutmp1 == VFU_BitALU || vfutmp1 == VFU_CAU ||
                 vfutmp1 == VFU_SerDiv) &&
                (olderVfu1 == VFU_BitALU || olderVfu1 == VFU_CAU ||
                 olderVfu1 == VFU_SerDiv);
            const bool newHasTaggedLaneRequesters =
                vfutmp1 == VFU_BitALU || vfutmp1 == VFU_CAU ||
                vfutmp1 == VFU_SerDiv;
            const bool newIsLsu =
                pkt->op == VLOAD || pkt->op == VSTORE;
            const bool olderIsLsu =
                older->op == VLOAD || older->op == VSTORE;
            /*
             * Lane operand requesters capture the producer generation for
             * RAW, while their result/writeback queue independently captures
             * the victim generation for WAR and WAW.  Therefore a command
             * behind shuffle remains queueable for every hazard class:
             * operand reads wait for shuffle retirement (shuffle deliberately
             * has no row-progress credit), and destination retirement waits
             * for the tagged victim's progress or completion tombstone.
             * Restricting this boundary to pure RAW serializes mixed
             * RAW+WAR/WAW commands at the sequencer even though RTL has
             * already accepted them into the tagged lane requester.
             *
             * The reciprocal direction is also queueable, but at a different
             * structural boundary.  venus_shuffle_engine.sv captures the
             * complete pe_req (including its hazard vectors) in its depth-2
             * vinsn_queue, then waits on those tags at the queue head.  The
             * gem5 shuffle pipeline has the same depth-2 tagged instruction
             * queue and rechecks the generation-owned hazard table before
             * starting a command.  Holding lane-to-shuffle dependencies here
             * leaves that queue permanently under-filled and serializes
             * independent admission behind the active shuffle.
             */
            const bool shuffleToLaneTaggedDependency =
                vfutmp1 == VFU_BitALU || vfutmp1 == VFU_CAU ||
                vfutmp1 == VFU_SerDiv;
            const bool olderIsShuffle =
                olderVfu1 == VFU_ShuffleUnit ||
                olderVfu2 == VFU_ShuffleUnit;
            const bool newHasTaggedShuffleQueue =
                vfutmp1 == VFU_ShuffleUnit ||
                vfutmp2 == VFU_ShuffleUnit;
            /*
             * LDU/STU accept an instruction before its register dependency
             * is clear.  The structured LSU keeps the request tagged and
             * rechecks the live hazard table at StoreOperands/Response,
             * before executeLsuInstr can read or write architectural VRF
             * state.  Holding every younger LSU here collapses that request
             * phase onto the response phase and loses the RTL overlap
             * (notably a younger LDU WAR behind an active shuffle reader).
             *
             * Arithmetic lane requesters also retain an older LSU
             * generation.  They may accept the command before the LDU/STU
             * retires, while their tagged operand request remains blocked
             * by the live hazard table.  Shuffle has no corresponding
             * lane-local tagged operand context and stays conservative.
             */
            const bool taggedLaneCanHoldDependency =
                laneVfuPair ||
                (newHasTaggedLaneRequesters && olderIsLsu) ||
                (shuffleToLaneTaggedDependency && olderIsShuffle);
            const bool taggedQueueCanHoldDependency =
                taggedLaneCanHoldDependency ||
                newHasTaggedShuffleQueue;
            if (!newIsLsu && !taggedQueueCanHoldDependency)
                return false;
        }
    }

    if(vfutmp1 != VFU_NONE && sequencerQueueAccountsVfu(pkt, vfutmp1)) {
        DPRINTF(VenusSequencerFull, "sequencer check if spare in queue, vfu_queue_counter[%s]:%d\n",vfu_to_str(vfutmp1),vfu_queue_counter[vfutmp1]);
        if(checkVFUQueueisFull(vfutmp1))
        {
            DPRINTF(VenusSequencerFull, "VinsnTargetVFU in VenusSequencer isFull\n");
            return false;
        }
    }
    if(vfutmp2 != VFU_NONE && sequencerQueueAccountsVfu(pkt, vfutmp2)) {
        DPRINTF(VenusSequencerFull, "sequencer check if spare in queue, vfu_queue_counter[%s]:%d\n",vfu_to_str(vfutmp2),vfu_queue_counter[vfutmp2]);
        if(checkVFUQueueisFull(vfutmp2))
        {
            DPRINTF(VenusSequencerFull, "VinsnTargetVFU in VenusSequencer isFull\n");
            return false;
        }
    }
    if(vfu_mtmp != VFU_NONE && sequencerQueueAccountsVfu(pkt, vfu_mtmp)) {
        DPRINTF(VenusSequencerFull, "sequencer check if spare in queue, vfu_queue_counter[%s]:%d\n",vfu_to_str(vfu_mtmp),vfu_queue_counter[vfu_mtmp]);
        if(checkVFUQueueisFull(vfu_mtmp))
        {
            DPRINTF(VenusSequencerFull, "VinsnTargetVFU in VenusSequencer isFull\n");
            return false;
        }
    }

    if (pkt->op == VLOAD && curTick() < lduAcceptAvailableTick)
        return false;
    if (pkt->op == VSTORE && curTick() < stuAcceptAvailableTick)
        return false;

    /*
     * STU has four operand/result slots and backpressures the sequencer at
     * the capacity edge.  LDU additionally has the independently advancing
     * held addrgen request modeled by scheduleLsuDone(), so only stores use
     * this admission guard.
     */
    if (pkt->op == VSTORE && !lsuQueueHasSpace(pkt))
        return false;

    sequencerWaitedForVfuReady = false;
    sequencerWaitedForPeReady = false;
    isBusy = true;
    pkt->vns_instr_id = VenusInstrPkt::vns_instr_gencounter++;
    this->recved_venus_instr_pkt = new VenusInstrPkt(pkt);

    if (rtlDispatcherInclusiveTail &&
        this->recved_venus_instr_pkt->op != VSHUFFLE &&
        this->recved_venus_instr_pkt->op != VSHUFFLE_CLBMV) {
        /*
         * Venus1 venus_dispatcher.sv computes vector_head2tail as the
         * truncated quotient and then assigns the inclusive tail directly:
         *
         *   tail = head + (vl >> (log2(lanes*banks) + EW16 - vew))
         *
         * Venus2 changed this to a row count with a remainder term and
         * subtracts one at the tail assignment.  Keeping the generations
         * separate is observable whenever VL is an exact physical-row
         * multiple: for example V1 EW8/VL4096 occupies rows 8..16, not
         * 8..15.  The additional scoreboard row can create a real
         * RAR/RAW/WAR/WAW dependency even though no extra data element is
         * executed.  This is a backend dispatcher contract, not an opcode,
         * PC or DAG timing exception.
         */
        unsigned laneBanks = NrLanes * NrBankPerLane;
        unsigned laneBanksLog2 = 0;
        panic_if(laneBanks == 0 ||
                     (laneBanks & (laneBanks - 1)) != 0,
                 "Venus1 dispatcher requires power-of-two lanes*banks");
        for (unsigned value = laneBanks; value > 1; value >>= 1)
            ++laneBanksLog2;
        const unsigned quotientShift = laneBanksLog2 +
            static_cast<unsigned>(EW16) -
            static_cast<unsigned>(this->recved_venus_instr_pkt->vew);
        const int headToTail =
            static_cast<unsigned>(this->recved_venus_instr_pkt->vl) >>
            quotientShift;
        const int rowMask = NrLines - 1;
        auto inclusiveTail = [headToTail, rowMask](int head) {
            return (head + headToTail) & rowMask;
        };
        auto *accepted = this->recved_venus_instr_pkt;
        if (accepted->use_vs1)
            accepted->vs1_tail = inclusiveTail(accepted->vs1_head);
        if (accepted->use_vs2)
            accepted->vs2_tail = inclusiveTail(accepted->vs2_head);
        if (accepted->use_vd1 || accepted->use_vd1_op)
            accepted->vd1_tail = inclusiveTail(accepted->vd1_head);
        if (accepted->use_vd2 || accepted->use_vd2_op)
            accepted->vd2_tail = inclusiveTail(accepted->vd2_head);
        accepted->line_usage = headToTail + 1;
    }

    assignTargetVFU(this->recved_venus_instr_pkt, vfutmp1, vfutmp2, vfu_mtmp);
    std::fill(std::begin(sequencerIssueTargetVfus),
              std::end(sequencerIssueTargetVfus), false);
    if (vfutmp1 != VFU_NONE &&
        sequencerQueueAccountsVfu(recved_venus_instr_pkt, vfutmp1))
        sequencerIssueTargetVfus[vfutmp1] = true;
    if (vfutmp2 != VFU_NONE &&
        sequencerQueueAccountsVfu(recved_venus_instr_pkt, vfutmp2))
        sequencerIssueTargetVfus[vfutmp2] = true;
    if (vfu_mtmp != VFU_NONE &&
        sequencerQueueAccountsVfu(recved_venus_instr_pkt, vfu_mtmp))
        sequencerIssueTargetVfus[vfu_mtmp] = true;
    const int runningId = findRunningID();
    assignRunningIDandLaneandQueue(this->recved_venus_instr_pkt, runningId);
    if (this->recved_venus_instr_pkt->op == VLOAD ||
        this->recved_venus_instr_pkt->op == VSTORE) {
        Tick &acceptAvailableTick =
            this->recved_venus_instr_pkt->op == VLOAD
                ? lduAcceptAvailableTick : stuAcceptAvailableTick;
        const int acceptInterval =
            this->recved_venus_instr_pkt->op == VLOAD
                ? VenusLduAcceptIntervalCycles
                : VenusStuAcceptIntervalCycles;
        acceptAvailableTick = afterCycles(Cycles(acceptInterval));
        traceLsuEvent("lsu_accept", this->recved_venus_instr_pkt);
    }
    /*
     * RTL does not expose an accepted request directly to scalar600.  It
     * first registers the PE running table, then OR-reduces it into
     * vinsn_running_q on the following edge.  Preserve that boundary rather
     * than treating packet allocation as scalar-visible activity.
     */
    /*
     * venus_sequencer.sv updates pe_vinsn_running_d for every operation,
     * including VSHUFFLE, on the upstream issue_valid edge.  The later
     * PIPE0/1/2 and downstream PE handshake do not gate this running-table
     * allocation.  The scalar barrier synchronizer must therefore see a
     * shuffle at the same registered boundary as lane and LSU requests.
     */
    rtlIssuePending[runningId] = true;
    sequencerIssueState = SequencerIssueState::Pipe0;
    panic_if(nextSequencerIssueStateEvent.scheduled(),
             "Venus sequencer issue-state event already scheduled");
    schedule(nextSequencerIssueStateEvent, afterCycles(Cycles(1)));
    updateHazardTable();
    if (recved_venus_instr_pkt->op == VSHUFFLE ||
        (recved_venus_instr_pkt->op >= VREDAND &&
         recved_venus_instr_pkt->op <= VREDSUM)) {
        /*
         * pe_req_o carries these hazards into the RTL Shuffle queue.  They
         * are command-local state, not a live lookup performed when the
         * engine eventually leaves IDLE.  Capture the same operand classes
         * plus the explicit Shuffle/reduction cross hazard and tag every
         * bit with the producer generation that created it.
         */
        recved_venus_instr_pkt->shuffle_hazard_snapshot_valid = true;
        for (int producer = 0; producer < NrIDs; ++producer) {
            const VenusInstrPkt *producerPkt =
                vinsn_running_pkt[producer];
            const bool producerIsShuffle =
                producerPkt != nullptr && producerPkt->op == VSHUFFLE;
            const bool producerIsReduction =
                producerPkt != nullptr &&
                producerPkt->op >= VREDAND &&
                producerPkt->op <= VREDSUM;
            const bool crossHazard =
                (recved_venus_instr_pkt->op == VSHUFFLE &&
                 producerIsReduction) ||
                (recved_venus_instr_pkt->op >= VREDAND &&
                 recved_venus_instr_pkt->op <= VREDSUM &&
                 producerIsShuffle);
            const int consumer = recved_venus_instr_pkt->running_id;
            const bool hazard =
                venus_hazard_table->vs1_hazard_table[consumer][producer] ||
                venus_hazard_table->vs2_hazard_table[consumer][producer] ||
                venus_hazard_table->vd1_hazard_table[consumer][producer] ||
                crossHazard;
            recved_venus_instr_pkt->shuffle_hazard_snapshot[producer] =
                hazard;
            recved_venus_instr_pkt->shuffle_hazard_generation[producer] =
                hazard ?
                    venus_hazard_table->running_id_to_vns_instr_id[
                        producer] : -1;
        }
    }
    /*
     * Carry the authoritative, allocation-time retirement dependencies with
     * the instruction itself.  The three-stage hazard-table broadcast and
     * the downstream command can arrive at a lane on the same edge; event
     * ordering must not let the command observe the preceding table and
     * lose an older LSU reader.  The instruction tag is the stable RTL
     * boundary for this snapshot, while later lane-local tables may only
     * merge additional still-live dependencies.
     */
    for (const auto *victim : vinsn_running_pkt) {
        if (victim == nullptr ||
            victim->vns_instr_id >= recved_venus_instr_pkt->vns_instr_id)
            continue;
        const int victimId = victim->running_id;
        const auto victimReads = ranges(victim, false);
        const auto victimWrites = ranges(victim, true);
        bool raw =
            recved_venus_instr_pkt->vm_r && victim->vm_w;
        bool rar =
            recved_venus_instr_pkt->vm_r && victim->vm_r;
        bool war =
            recved_venus_instr_pkt->vm_w && victim->vm_r;
        bool waw =
            recved_venus_instr_pkt->vm_w && victim->vm_w;
        for (const auto &read : newReads) {
            for (const auto &victimRead : victimReads)
                rar |= overlaps(read, victimRead);
            for (const auto &victimWrite : victimWrites)
                raw |= overlaps(read, victimWrite);
        }
        for (const auto &write : newWrites) {
            for (const auto &read : victimReads)
                war |= overlaps(write, read);
            for (const auto &victimWrite : victimWrites)
                waw |= overlaps(write, victimWrite);
        }
        /*
         * RTL's source hazard vectors include RAR as well as true RAW.  RAR
         * does not create a data-chain credit, but it still makes an
         * operand requester wait for the older command generation to
         * retire.  Preserve the producer tag/class for that case too.
         * Otherwise an LSU command (which never enters a lane input
         * register) can inherit stale VFU metadata from an earlier user of
         * the same running ID and miss the tagged LSU completion boundary.
         */
        /*
         * The software range helper above is useful for carrying explicit
         * RAW/WAR/WAW classes, but it is not the final RTL admission
         * oracle.  venus_dispatcher.sv rounds register ranges to physical
         * rows (including its exact-multiple EW16 behavior), and the
         * hazard table produced by updateHazardTable() is the resulting
         * hardware-visible dependency bitmap.  A dependency which exists
         * only after that rounding still needs the producer generation and
         * producer class captured with the requester command.  Otherwise a
         * Shuffle/LSU producer, neither of which enters the ordinary lane
         * input register, can be reconstructed from stale lane-local state
         * left by an older user of the same running ID.  In Venus1 that can
         * turn a Shuffle dependency into a same-CAU dependency and wait for
         * a lane-command retirement the producer can never emit.
         *
         * Use the freshly computed RTL-shaped global bitmap as the superset
         * capture condition.  The operand-specific tables below still
         * decide whether an individual requester actually waits, so this
         * does not add dependencies or workload-specific latency.
         */
        const bool requesterDependency =
            rar || raw || war || waw ||
            venus_hazard_table->global_hazard_table
                [recved_venus_instr_pkt->running_id][victimId];
        if (requesterDependency) {
            /*
             * venus_lane_sequencer places source and destination hazards in
             * the operand requester's command-local bitmap.  Snapshot the
             * producer generation and class together here.  Shuffle and LSU
             * commands do not pass through the ordinary lane input register,
             * so reconstructing their class later from a lane-local running
             * ID slot can pair the new generation with stale VFU metadata.
             */
            recved_venus_instr_pkt->chain_raw_producer_instr[victimId] =
                victim->vns_instr_id;
            recved_venus_instr_pkt->chain_raw_producer_is_lsu[victimId] =
                victim->op == VLOAD || victim->op == VSTORE;
            /* Reductions enter the lane BitALU first, but their tagged
             * command completion is owned by the Shuffle/reduction PE.
             * The allocation-time snapshot is the authoritative producer
             * class later consumed by every lane requester, so recording
             * vfu_lst.front() here would incorrectly route a dependent
             * command onto the ordinary lane-arithmetic retirement path.
             * Classify the compound opcode by its RTL completion owner. */
            const bool victimIsReduction =
                victim->op >= VREDAND && victim->op <= VREDSUM;
            recved_venus_instr_pkt->chain_raw_producer_vfu[victimId] =
                victimIsReduction ? VFU_ShuffleUnit :
                (victim->vfu_lst.empty() ? VFU_NONE :
                    victim->vfu_lst.front());
        }
        if (raw) {
            DPRINTF(VenusSequencerFull,
                    "Tagged RAW producer snapshot consumer instr %d/rid %d "
                    "producer instr %d/rid %d lsu=%d vfu=%d\n",
                    recved_venus_instr_pkt->vns_instr_id, runningId,
                    victim->vns_instr_id, victimId,
                    recved_venus_instr_pkt->
                        chain_raw_producer_is_lsu[victimId],
                    static_cast<int>(recved_venus_instr_pkt->
                        chain_raw_producer_vfu[victimId]));
        }
        if (!war && !waw)
            continue;
        recved_venus_instr_pkt->chain_war_hazard[victimId] = war;
        recved_venus_instr_pkt->chain_waw_hazard[victimId] = waw;
        recved_venus_instr_pkt->chain_retire_victim_instr[victimId] =
            victim->vns_instr_id;
        recved_venus_instr_pkt->chain_retire_victim_is_lsu[victimId] =
            victim->op == VLOAD || victim->op == VSTORE;
        DPRINTF(VenusSequencerFull,
                "Tagged retirement snapshot writer instr %d/rid %d "
                "victim instr %d/rid %d WAR=%d WAW=%d\n",
                recved_venus_instr_pkt->vns_instr_id, runningId,
                victim->vns_instr_id, victimId, war, waw);
    }
    if(!nextBroadcastHazardTablePipe1Event.scheduled())
        schedule(nextBroadcastHazardTablePipe1Event, afterCycles(Cycles(1)));
    // this->recved_venus_instr_pkt->display();
    if(!nextRecvNewInstrEvent.scheduled())
        schedule(nextRecvNewInstrEvent, afterCycles(Cycles(3)));
    else
        panic("nextRecvNewInstrEvent already scheduled during dispatch\n");
    return true;
}

void
VenusSequencer::initSharedMemory()
{
    sharedMemory.assign(VenusSharedMemHalfWords, 0);

    if (sharedL2Object == nullptr) {
        const char *sharedL2Path = std::getenv("VENUS_GEM5_SHARED_L2_FILE");
        if (sharedL2Path != nullptr && sharedL2Path[0] != '\0') {
            std::ifstream input(sharedL2Path, std::ios::binary);
            fatal_if(!input, "VenusSequencer cannot open shared L2 image %s",
                     sharedL2Path);
            input.read(reinterpret_cast<char *>(sharedL2.data()), sharedL2.size());
            const size_t loaded = static_cast<size_t>(input.gcount());
            fatal_if(!input.eof() && input.fail(),
                     "VenusSequencer failed while reading shared L2 image %s",
                     sharedL2Path);
            inform("VenusSequencer loaded %llu shared-L2 bytes from %s\n",
                   static_cast<unsigned long long>(loaded), sharedL2Path);
        }
    }

    const char *envPath = std::getenv("VENUS_GEM5_SHARED_MEM_FILE");
    if (envPath == nullptr || envPath[0] == '\0') {
        inform("Venus shared memory initialized to zero; set "
               "VENUS_GEM5_SHARED_MEM_FILE to supply testbench input\n");
        return;
    }
    const std::filesystem::path sharedMemPath(envPath);

    size_t loadedBytes = loadTextByteArray(sharedMemPath, sharedMemory);
    if (loadedBytes == 0)
        loadedBytes = loadBinaryBytes(sharedMemPath, sharedMemory);

    if (loadedBytes == 0) {
        warn("VenusSequencer could not load shared memory file %s; VLOAD shared memory stays zeroed\n",
             sharedMemPath.string());
        return;
    }

    inform("VenusSequencer loaded %llu shared-memory bytes from %s\n",
           static_cast<unsigned long long>(loadedBytes), sharedMemPath.string());
}

void
VenusSequencer::captureLsuLoad(PendingLsuInstr &pending)
{
    VenusInstrPkt *pkt = pending.pkt;
    panic_if(pkt->op != VLOAD,
             "captureLsuLoad called for non-load LSU op");

    pending.loadValues.assign(pkt->vl, 0);
    pending.loadValid.assign(pkt->vl, 0);
    for (int i = 0; i < pkt->vl; ++i) {
        if (pkt->vm_r == READ_MASKED &&
            !m_venus_vrf->backdoor_Read_Mask_Element(i, pkt->vew)) {
            continue;
        }

        const bool isByte = pkt->vew == EW8;
        const uint32_t targetAddr =
            pkt->scalar_op + (isByte ? i : 2 * i);
        panic_if(!isByte && (targetAddr & 0x1),
                 "EW16 VLOAD target address 0x%x is odd\n", targetAddr);

        uint16_t loadVal = 0;
        if (activeThreadContext != nullptr) {
            constexpr uint32_t TileBlockPrefixMask = 0x0f000000;
            constexpr uint32_t TileBlockPrefix = 0x02000000;
            constexpr uint32_t TileLocalMask = 0x001fffff;
            constexpr uint32_t DspmBase = 0x00020000;
            constexpr uint32_t DspmEnd = 0x00024000;
            const bool isTileBlock =
                (targetAddr & TileBlockPrefixMask) == TileBlockPrefix;
            const uint32_t tileLocalAddr = targetAddr & TileLocalMask;
            if (isTileBlock) {
                panic_if(tileLocalAddr < DspmBase ||
                         tileLocalAddr >= DspmEnd,
                         "VLOAD tile-block address 0x%x is outside the "
                         "modeled DSPM window\n", targetAddr);
                const Addr physAddr = 0x80000000ULL | tileLocalAddr;
                SETranslatingPortProxy(activeThreadContext).readBlob(
                    physAddr, &loadVal, isByte ? 1 : 2);
            } else {
                readSharedL2(targetAddr, isByte ? 1 : 2,
                             reinterpret_cast<uint8_t *>(&loadVal));
            }
        } else {
            panic_if(targetAddr < VenusSharedMemBase ||
                     targetAddr >= VenusSharedMemLimit,
                     "VLOAD target address 0x%x is outside Venus shared "
                     "memory [0x%x, 0x%x)\n",
                     targetAddr, VenusSharedMemBase, VenusSharedMemLimit);
            const size_t memIdx =
                (targetAddr - VenusSharedMemBase) / 2;
            loadVal = sharedMemory[memIdx];
            if (isByte) {
                loadVal = (targetAddr & 0x1) ? (loadVal >> 8) :
                    (loadVal & 0xff);
            }
        }
        pending.loadValues[i] = loadVal;
        pending.loadValid[i] = 1;
    }
}

void
VenusSequencer::commitLsuLoad(const PendingLsuInstr &pending)
{
    VenusInstrPkt *pkt = pending.pkt;
    panic_if(pkt->op != VLOAD,
             "commitLsuLoad called for non-load LSU op");
    panic_if(pending.loadValues.size() != static_cast<size_t>(pkt->vl) ||
             pending.loadValid.size() != static_cast<size_t>(pkt->vl),
             "VLOAD result payload does not match VL");

    for (int i = 0; i < pkt->vl; ++i) {
        if (!pending.loadValid[i])
            continue;
        m_venus_vrf->backdoor_Write_Ldu_Element(
            pkt->vd1_head, i, pkt->vew, pending.loadValues[i]);
    }
}

void
VenusSequencer::executeLsuInstr(VenusInstrPkt* pkt)
{
    panic_if(pkt->op != VLOAD && pkt->op != VSTORE,
             "executeLsuInstr called for non-LSU op");

    for (int i = 0; i < pkt->vl; ++i) {
        if (pkt->vm_r == READ_MASKED &&
            !m_venus_vrf->backdoor_Read_Mask_Element(i, pkt->vew)) {
            continue;
        }

        const bool isByte = pkt->vew == EW8;
        const uint32_t targetAddr = pkt->scalar_op + (isByte ? i : 2 * i);
        panic_if(!isByte && (targetAddr & 0x1),
                 "EW16 %s target address 0x%x is odd\n",
                 op_to_str(pkt->op), targetAddr);
        if (activeThreadContext != nullptr) {
            // targetAddr retains the low 28 bits of the RTL AXI address:
            //   0x800..... is the cluster-L2 perspective (0x00...... here),
            //   0x820..... is the tile-block perspective (0x02...... here).
            // Do not select DSPM from the low offset alone.  A global L2
            // burst can cross 0x20000 (for example 0x8001ffc0), and treating
            // its tail as DSPM silently corrupts the vector load.
            constexpr uint32_t TileBlockPrefixMask = 0x0f000000;
            constexpr uint32_t TileBlockPrefix = 0x02000000;
            constexpr uint32_t TileLocalMask = 0x001fffff;
            constexpr uint32_t DspmBase = 0x00020000;
            constexpr uint32_t DspmEnd = 0x00024000;
            const bool isTileBlock =
                (targetAddr & TileBlockPrefixMask) == TileBlockPrefix;
            const uint32_t tileLocalAddr = targetAddr & TileLocalMask;
            if (isTileBlock) {
                panic_if(tileLocalAddr < DspmBase || tileLocalAddr >= DspmEnd,
                         "%s tile-block address 0x%x is outside the modeled DSPM window\n",
                         op_to_str(pkt->op), targetAddr);
                const Addr physAddr = 0x80000000ULL | tileLocalAddr;
                if (pkt->op == VLOAD) {
                    uint16_t loadVal = 0;
                    SETranslatingPortProxy(activeThreadContext).readBlob(
                        physAddr, &loadVal, isByte ? 1 : 2);
                    m_venus_vrf->backdoor_Write_Ldu_Element(
                        pkt->vd1_head, i, pkt->vew, loadVal);
                } else {
                    const uint16_t storeVal =
                        m_venus_vrf->backdoor_Read_Ldu_Element(
                            pkt->vs2_head, i, pkt->vew);
                    SETranslatingPortProxy(activeThreadContext).writeBlob(
                        physAddr, &storeVal, isByte ? 1 : 2);
                }
                continue;
            }
            // RTL addrgen places LDU/STU on the 0x80... global-memory
            // perspective.  sharedL2 is indexed by the low 28-bit offset;
            // the distinct 0x82... L1 tile-local perspective is handled by
            // VenusDagScheduler and must not be aliased here.
            if (pkt->op == VLOAD) {
                uint16_t loadVal = 0;
                readSharedL2(targetAddr, isByte ? 1 : 2,
                             reinterpret_cast<uint8_t *>(&loadVal));
                m_venus_vrf->backdoor_Write_Ldu_Element(
                    pkt->vd1_head, i, pkt->vew, loadVal);
            } else {
                const uint16_t storeVal =
                    m_venus_vrf->backdoor_Read_Ldu_Element(
                        pkt->vs2_head, i, pkt->vew);
                writeSharedL2(targetAddr, isByte ? 1 : 2,
                              reinterpret_cast<const uint8_t *>(&storeVal));
            }
            continue;
        }

        // Legacy single-task replay keeps its explicit shared-memory input
        // buffer.  DAG mode instead uses the active task's address space,
        // matching the RTL LDU/STU AXI path.
        panic_if(targetAddr < VenusSharedMemBase || targetAddr >= VenusSharedMemLimit,
                 "%s target address 0x%x is outside Venus shared memory [0x%x, 0x%x)\n",
                 op_to_str(pkt->op), targetAddr, VenusSharedMemBase, VenusSharedMemLimit);
        const size_t memIdx = (targetAddr - VenusSharedMemBase) / 2;
        if (pkt->op == VLOAD) {
            uint16_t loadVal = sharedMemory[memIdx];
            if (isByte)
                loadVal = (targetAddr & 0x1) ? (loadVal >> 8) :
                    (loadVal & 0xff);
            m_venus_vrf->backdoor_Write_Ldu_Element(pkt->vd1_head, i, pkt->vew, loadVal);
        } else {
            const uint16_t storeVal =
                m_venus_vrf->backdoor_Read_Ldu_Element(pkt->vs2_head, i, pkt->vew);
            if (isByte) {
                const uint16_t oldVal = sharedMemory[memIdx];
                if (targetAddr & 0x1)
                    sharedMemory[memIdx] = (oldVal & 0x00ff) | ((storeVal & 0xff) << 8);
                else
                    sharedMemory[memIdx] = (oldVal & 0xff00) | (storeVal & 0xff);
            } else {
                sharedMemory[memIdx] = storeVal;
            }
        }
    }
}

void
VenusSequencer::traceLsuEvent(const char *event, VenusInstrPkt *pkt,
                              int beats, Addr addr)
{
    auto &trace = venusLsuTrace();
    if (!trace.is_open())
        return;

    trace << "{\"event\":\"" << event << "\""
          << ",\"source\":\"gem5\""
          << ",\"sequencer\":\"" << name() << "\""
          << ",\"tick\":" << curTick()
          << ",\"cycle\":" << (curTick() / clockPeriod())
          << ",\"task_id\":" << activeTaskId
          << ",\"instr\":" << pkt->vns_instr_id
          << ",\"id\":" << pkt->running_id
          << ",\"op\":\"" << op_to_str(pkt->op) << "\""
          << ",\"vl\":" << pkt->vl
          << ",\"vew\":" << static_cast<int>(pkt->vew);
    if (beats >= 0)
        trace << ",\"beats\":" << beats;
    if (addr != 0)
        trace << ",\"addr\":" << addr;
    trace << "}\n";
    trace.flush();
}

void
VenusSequencer::readSharedL2(Addr addr, size_t size, uint8_t *data) const
{
    if (sharedL2Object != nullptr) {
        sharedL2Object->read(addr, size, data);
        return;
    }
    fatal_if(addr > sharedL2.size() || size > sharedL2.size() - addr,
             "shared L2 read [0x%x, +%d] exceeds modeled capacity 0x%x",
             addr, size, sharedL2.size());
    std::copy_n(sharedL2.data() + addr, size, data);
}

void
VenusSequencer::writeSharedL2(Addr addr, size_t size, const uint8_t *data)
{
    if (sharedL2Object != nullptr) {
        sharedL2Object->write(addr, size, data);
        return;
    }
    fatal_if(addr > sharedL2.size() || size > sharedL2.size() - addr,
             "shared L2 write [0x%x, +%d] exceeds modeled capacity 0x%x",
             addr, size, sharedL2.size());
    std::copy_n(data, size, sharedL2.data() + addr);
}

bool
VenusSequencer::lsuQueueHasSpace(VenusInstrPkt *pkt)
{
    panic_if(pkt->op != VLOAD && pkt->op != VSTORE,
             "lsuQueueHasSpace called for non-LSU op");

    while (!lsuRetiringSlots.empty() &&
           lsuRetiringSlots.front().releaseTick <= curTick()) {
        lsuRetiringSlots.pop_front();
    }
    if (pkt->op == VLOAD)
        return lduActiveLoads < VenusLsuQueueDepth;

    const auto sameUnit = [pkt](const PendingLsuInstr &pending) {
        return pending.pkt->op == pkt->op;
    };
    const auto retiringSameUnit =
        [pkt](const LsuRetiringSlot &retiring) {
            return retiring.op == pkt->op;
        };
    const int pendingCount = std::count_if(
        pendingLsuInstrs.begin(), pendingLsuInstrs.end(), sameUnit);
    const int retiringCount = std::count_if(
        lsuRetiringSlots.begin(), lsuRetiringSlots.end(),
        retiringSameUnit);
    return pendingCount + retiringCount < VenusLsuQueueDepth;
}

void
VenusSequencer::queuePendingLsu(PendingLsuInstr pending)
{
    const char *phase = pending.phase == LsuPhase::Request ? "request" :
        pending.phase == LsuPhase::StoreOperands ? "store_operands" :
        pending.phase == LsuPhase::StoreDataCommit ?
            "store_data_commit" :
        pending.phase == LsuPhase::MemoryResponse ? "memory_response" :
        pending.phase == LsuPhase::ResultQueueVisible ?
            "result_queue_visible" :
        pending.phase == LsuPhase::ResultFinalGrant ?
            "result_final_grant" :
        pending.phase == LsuPhase::PeResponse ? "pe_response" :
        "complete";
    auto it = pendingLsuInstrs.begin();
    while (it != pendingLsuInstrs.end()) {
        if (it->eventTick < pending.eventTick) {
            ++it;
            continue;
        }
        if (it->eventTick > pending.eventTick)
            break;

        /*
         * A response/complete edge may release the shared channel or an LSU
         * slot for a request on that same edge.  Preserve that ordering.
         * Requests which have all been pushed to the same channel-available
         * edge are arbitrated oldest-first, matching the RTL request queue.
         * Stable insertion alone lets a younger request which happened to be
         * retried first bypass older blocked work and can create a circular
         * destination-hazard wait.
         */
        const bool existingRequest = it->phase == LsuPhase::Request;
        const bool pendingRequest = pending.phase == LsuPhase::Request;
        if (existingRequest != pendingRequest) {
            if (!existingRequest) {
                ++it;
                continue;
            }
            break;
        }
        if (pendingRequest &&
            it->pkt->vns_instr_id > pending.pkt->vns_instr_id) {
            break;
        }
        ++it;
    }
    pendingLsuInstrs.insert(it, pending);
    DPRINTF(VenusSequencerFull,
            "LSU queue instr %d/rid %d op=%s phase=%s tick=%llu depth=%llu\n",
            pending.pkt->vns_instr_id, pending.pkt->running_id,
            op_to_str(pending.pkt->op), phase,
            static_cast<unsigned long long>(pending.eventTick),
            static_cast<unsigned long long>(pendingLsuInstrs.size()));
}

void
VenusSequencer::scheduleNextLsuEvent()
{
    if (pendingLsuInstrs.empty())
        return;

    const Tick nextTick = pendingLsuInstrs.front().eventTick;
    if (!nextLsuDoneEvent.scheduled()) {
        schedule(nextLsuDoneEvent, nextTick);
    } else if (nextTick < nextLsuDoneEvent.when()) {
        reschedule(nextLsuDoneEvent, nextTick);
    }
}

void
VenusSequencer::advanceLduRegisters()
{
    /*
     * vldu.sv keeps both vinsn_queue_q and vinsn_queue_qq.  Every tile edge
     * first exposes the previous Q state to QQ, then intersects each Q slot's
     * captured destination hazard with the current global hazard row.  The
     * intersection is intentionally destructive: a producer ID which has
     * cleared once must not reappear when that ID is reused by a younger
     * instruction.
     */
    lduIssueSlotsQQ = lduIssueSlotsQ;
    lduIssuePntQQ = lduIssuePntQ;
    lduIssuePntQ = lduIssuePntD;
    if (venus_hazard_table != nullptr) {
        for (auto &slot : lduIssueSlotsQ) {
            uint8_t live = 0;
            if (slot.runningId >= 0 && slot.runningId < NrIDs) {
                for (int producer = 0; producer < NrIDs; ++producer) {
                    const int generation =
                        slot.hazardGeneration[producer];
                    const bool capturedGenerationRetired =
                        generation >= 0 &&
                        (venus_hazard_table->
                             retired_vns_instr_id[producer] >= generation ||
                         lduRequesterRetiredGeneration[producer] >=
                             generation);
                    if (venus_hazard_table->global_hazard_table[
                            slot.runningId][producer] &&
                        !capturedGenerationRetired) {
                        live |= uint8_t(1U << producer);
                    }
                }
            }
            slot.hazardVd1 &= live;
        }
    }

    if (lduActiveLoads != 0) {
        schedule(nextLduRegisterEvent, afterCycles(Cycles(1)));
    }
}

void
VenusSequencer::queueLduResultRow(LduResultRow row)
{
    auto it = lduRowsWaitingVisibility.begin();
    while (it != lduRowsWaitingVisibility.end() &&
           it->visibleTick <= row.visibleTick) {
        ++it;
    }
    lduRowsWaitingVisibility.insert(it, std::move(row));
    scheduleNextLduResultVisible();
}

void
VenusSequencer::scheduleNextLduResultVisible()
{
    if (lduRowsWaitingVisibility.empty())
        return;
    const Tick tick = lduRowsWaitingVisibility.front().visibleTick;
    if (!nextLduResultVisibleEvent.scheduled()) {
        schedule(nextLduResultVisibleEvent, tick);
    } else if (tick < nextLduResultVisibleEvent.when()) {
        reschedule(nextLduResultVisibleEvent, tick);
    }
}

void
VenusSequencer::executeLduResultVisible()
{
    while (!lduRowsWaitingVisibility.empty() &&
           lduRowsWaitingVisibility.front().visibleTick <= curTick()) {
        fatal_if(lduResultQueue.size() >= 2,
                 "VLDu result queue overflow at tick %llu",
                 static_cast<unsigned long long>(curTick()));
        LduResultRow row = std::move(lduRowsWaitingVisibility.front());
        lduRowsWaitingVisibility.pop_front();
        DPRINTF(VenusSequencerFull,
                "LDU result row visible instr %d/rid %d row %d/%d "
                "occupancy %llu/2\n",
                row.pkt->vns_instr_id, row.pkt->running_id,
                row.row + 1, row.rows,
                static_cast<unsigned long long>(lduResultQueue.size() + 1));
        lduResultQueue.push_back(std::move(row));
    }

    if (!lduResultQueue.empty() && !nextLduResultGrantEvent.scheduled())
        schedule(nextLduResultGrantEvent, curTick());
    scheduleNextLduResultVisible();
}

void
VenusSequencer::executeLduResultGrant()
{
    if (lduResultQueue.empty())
        return;

    const auto &gate = lduIssueSlotsQQ[lduIssuePntQQ];
    if (gate.hazardVd1 != 0) {
        DPRINTF(VenusSequencerFull,
                "LDU result FIFO qq hold instr %d/rid %d: issue_pnt %u "
                "slot_rid %d hazard_vd1 %#x occupancy %llu/2\n",
                lduResultQueue.front().pkt->vns_instr_id,
                lduResultQueue.front().pkt->running_id,
                lduIssuePntQQ, gate.runningId, gate.hazardVd1,
                static_cast<unsigned long long>(lduResultQueue.size()));
        schedule(nextLduResultGrantEvent, afterCycles(Cycles(1)));
        return;
    }

    LduResultRow row = std::move(lduResultQueue.front());
    lduResultQueue.pop_front();
    if (venusVrfXbar != nullptr) {
        venusVrfXbar->reserveVenusVrfLsuPriority(
            venusLsuRequesterTag(row.pkt, row.row, true),
            row.vrfBankMask, curTick(), 1,
            cyclesToTicks(Cycles(1)));
    }
    DPRINTF(VenusSequencerFull,
            "LDU result row grant instr %d/rid %d row %d/%d "
            "issue_pnt_qq %u remaining %llu\n",
            row.pkt->vns_instr_id, row.pkt->running_id,
            row.row + 1, row.rows, lduIssuePntQQ,
            static_cast<unsigned long long>(lduResultQueue.size()));

    if (row.final) {
        PendingLsuInstr pending{
            afterCycles(Cycles(VenusLduFinalGrantCycles)),
            row.pkt,
            LsuPhase::ResultFinalGrant,
            row.beats,
            row.rows,
            0,
            false,
            false,
        };
        pending.loadValues = std::move(row.loadValues);
        pending.loadValid = std::move(row.loadValid);
        queuePendingLsu(std::move(pending));
        scheduleNextLsuEvent();
    }

    if (!lduResultQueue.empty()) {
        schedule(nextLduResultGrantEvent, afterCycles(Cycles(1)));
    }
}

Tick
VenusSequencer::alignLsuAxiEdge(Tick candidate)
{
    const Tick period = cyclesToTicks(Cycles(2));
    panic_if(period == 0, "Venus LSU AXI period cannot be zero");

    if (!lsuAxiPhaseValid) {
        /*
         * The RTL AXI clock has a reset-defined phase; it is not phase-
         * locked to whichever LSU descriptor happens to arrive first.
         * Seeding this value from the first request made later STU CDC
         * timing depend on workload order.  gem5's corresponding clock
         * domain starts at tick zero, so retain that fixed phase here.
         */
        lsuAxiPhaseTick = 0;
        lsuAxiPhaseValid = true;
    }

    const Tick phase = candidate % period;
    const Tick advance =
        (lsuAxiPhaseTick + period - phase) % period;
    return candidate + advance;
}

Tick
VenusSequencer::alignLduOutboundAxiEdge(Tick candidate)
{
    const Tick period = cyclesToTicks(Cycles(2));
    const Tick tilePeriod = cyclesToTicks(Cycles(1));
    panic_if(period == 0 || tilePeriod == 0 || tilePeriod / 2 >= period,
             "Venus LDU clock phases are invalid");

    /*
     * Seed the independent, already-validated STU phase exactly as the old
     * shared helper did when the first LSU operation was a load.  The value
     * returned below models only the AR Gray-pointer CDC.
     */
    (void)alignLsuAxiEdge(candidate);
    /*
     * candidate is the combinational internal-AR-valid boundary.  The
     * cdc_fifo_gray source does not publish that request directly: its Gray
     * write pointer changes on the following tile edge.  Only an AXI edge
     * after that registered pointer update may be the first destination
     * synchronizer sample.
     *
     * In the RTL clock topology the AXI rising edge is one tile half-period
     * after gem5's two-cycle origin.  The retained full-DAG oracles exercise
     * both sides of this rule: task17's pointer changes just before an AXI
     * edge and is captured there, while task20's changes after the preceding
     * edge and waits for the next one.  No request age, task identity, address
     * or payload participates in the decision.
     */
    const Tick sourcePointerTick = candidate + tilePeriod;
    const Tick axiPhase = tilePeriod / 2;
    const Tick sourcePhase = sourcePointerTick % period;
    Tick advance = (axiPhase + period - sourcePhase) % period;
    if (advance == 0)
        advance += period;
    return sourcePointerTick + advance;
}

Tick
VenusSequencer::alignLsuTileEdge(Tick candidate) const
{
    const Tick period = cyclesToTicks(Cycles(1));
    panic_if(period == 0, "Venus LSU tile period cannot be zero");
    const Tick phase = candidate % period;
    return candidate + (period - phase) % period;
}

bool
VenusSequencer::scheduleLsuDone(VenusInstrPkt* pkt)
{
    bool heldLduAck = false;
    Tick heldLduReleaseTick = 0;
    if (!lsuQueueHasSpace(pkt)) {
        /*
         * The RTL addrgen accepts and advances the single held LDU request
         * independently of the four-entry LDU instruction queue.  Once the
         * oldest entry reaches its registered Complete phase, addrgen ACK is
         * visible two tile clocks before that queue entry is retired.  This
         * lets the sequencer return upstream while the held request still
         * waits for the slot; STU has no corresponding path.
         */
        lsuAddrgenBlockedInstrs.insert(pkt->vns_instr_id);
        if (pkt->op == VLOAD) {
            auto complete = std::find_if(
                pendingLsuInstrs.begin(), pendingLsuInstrs.end(),
                [pkt](const PendingLsuInstr &pending) {
                    return pending.pkt->op == pkt->op &&
                           pending.phase == LsuPhase::Complete;
                });
            if (complete != pendingLsuInstrs.end()) {
                heldLduReleaseTick = complete->eventTick;
                const Tick ackTick = heldLduReleaseTick -
                    cyclesToTicks(Cycles(2));
                heldLduAck = curTick() >= ackTick;
            }
        }
        if (!heldLduAck)
            return false;
    }

    const int bytes = pkt->vl << static_cast<int>(pkt->vew);
    /*
     * AXI len counts every 64-byte bus word touched by the transfer, not
     * just ceil(payload_bytes / 64).  An unaligned vector whose tail crosses
     * a bus-word boundary therefore has one more beat.  The RTL addrgen uses
     * the low address bits when constructing len; omitting them made, for
     * example, a 31-byte store at byte offset 62 finish one AXI edge early
     * and also released the following store one edge early.
     */
    constexpr uint64_t AxiBeatBytes = 64;
    const uint64_t firstBeatOffset =
        static_cast<uint64_t>(pkt->scalar_op) & (AxiBeatBytes - 1);
    const int beats = std::max<uint64_t>(
        1, (firstBeatOffset + static_cast<uint64_t>(bytes) +
            AxiBeatBytes - 1) / AxiBeatBytes);
    const int rows = std::max(1, (bytes + 127) / 128);
    const bool addrgenWasBlocked =
        lsuAddrgenBlockedInstrs.erase(pkt->vns_instr_id) != 0;
    const int requestCycles = pkt->op == VLOAD
        ? (addrgenWasBlocked ? VenusLduHeldRequestAfterRecycleCycles
                             : VenusLduAddrgenInternalArCycles)
        : VenusStuOperandRequestCycles;
    pkt->vns_instr_log_starttick = curTick();

    VenusInstrPkt *pendingPkt = new VenusInstrPkt(pkt);
    pendingPkt->vns_instr_stat = INSTR_FIRED;
    if (pkt->op == VLOAD) {
        uint8_t hazardVd1 = 0;
        LduIssueSlot issueSlot;
        issueSlot.runningId = static_cast<int>(pkt->running_id);
        if (venus_hazard_table != nullptr) {
            for (int producer = 0; producer < NrIDs; ++producer) {
                if (venus_hazard_table->vd1_hazard_table[
                        pkt->running_id][producer]) {
                    hazardVd1 |= uint8_t(1U << producer);
                    int generation = venus_hazard_table->
                        running_id_to_vns_instr_id[producer];
                    if (generation < 0 &&
                        vinsn_running_pkt[producer] != nullptr) {
                        generation = vinsn_running_pkt[producer]->
                            vns_instr_id;
                    }
                    issueSlot.hazardGeneration[producer] = generation;
                }
            }
        }
        issueSlot.hazardVd1 = hazardVd1;
        lduIssueSlotsQ[lduAcceptPnt] = issueSlot;
        DPRINTF(VenusSequencerFull,
                "LDU issue slot accept instr %d/rid %d slot %u "
                "hazard_vd1 %#x\n",
                pkt->vns_instr_id, pkt->running_id,
                lduAcceptPnt, hazardVd1);
        lduAcceptPnt = (lduAcceptPnt + 1) % VenusLsuQueueDepth;
        ++lduActiveLoads;
        if (!nextLduRegisterEvent.scheduled())
            schedule(nextLduRegisterEvent, afterCycles(Cycles(1)));
    }
    const Tick rawRequestTick = heldLduAck
        ? heldLduReleaseTick + cyclesToTicks(Cycles(requestCycles))
        : afterCycles(Cycles(requestCycles));
    /*
     * This event is the tile-side PE-valid boundary for both LDU and STU.
     * Neither address descriptor may be delayed to an AXI edge here; their
     * independent outbound CDC phases are applied only after the registered
     * tile-side request state has accepted the descriptor.
     */
    const Tick requestTick = rawRequestTick;
    const bool hazardAtAdmission =
        !venus_hazard_table->checkifreadytofire(pkt->running_id);
    PendingLsuInstr pending{
        requestTick,
        pendingPkt,
        LsuPhase::Request,
        beats,
        rows,
        0,
        hazardAtAdmission,
        false,
    };
    if (pkt->op == VSTORE && venus_hazard_table != nullptr) {
        for (int producer = 0; producer < NrIDs; ++producer) {
            if (!venus_hazard_table->vs2_hazard_table[
                    pkt->running_id][producer]) {
                continue;
            }
            pending.storeHazardVs2 |= uint8_t(1U << producer);
            int generation = venus_hazard_table->
                running_id_to_vns_instr_id[producer];
            if (generation < 0 && vinsn_running_pkt[producer] != nullptr) {
                generation = vinsn_running_pkt[producer]->vns_instr_id;
            }
            pending.storeHazardVs2Generation[producer] = generation;
        }
        DPRINTF(VenusSequencerFull,
                "VSTU queue accept instr %d/rid %d captured hazard_vs2 "
                "%#x\n",
                pkt->vns_instr_id, pkt->running_id,
                pending.storeHazardVs2);
    }
    queuePendingLsu(std::move(pending));
    scheduleNextLsuEvent();
    return true;
}

void
VenusSequencer::executeLsuDone()
{
    bool singleBeatStoreCompletedThisEdge = false;
    bool multiBeatStoreCompletedThisEdge = false;
    while (!pendingLsuInstrs.empty() &&
           pendingLsuInstrs.front().eventTick <= curTick()) {
        PendingLsuInstr pending = pendingLsuInstrs.front();
        pendingLsuInstrs.pop_front();
        const char *phase =
            pending.phase == LsuPhase::Request ? "request" :
            pending.phase == LsuPhase::StoreOperands ? "store_operands" :
            pending.phase == LsuPhase::StoreDataCommit ?
                "store_data_commit" :
            pending.phase == LsuPhase::MemoryResponse ?
                "memory_response" :
            pending.phase == LsuPhase::ResultQueueVisible ?
                "result_queue_visible" :
            pending.phase == LsuPhase::ResultFinalGrant ?
                "result_final_grant" :
            pending.phase == LsuPhase::PeResponse ? "pe_response" :
            "complete";
        DPRINTF(VenusSequencerFull,
                "LSU pop instr %d/rid %d op=%s phase=%s tick=%llu "
                "remaining=%llu\n",
                pending.pkt->vns_instr_id, pending.pkt->running_id,
                op_to_str(pending.pkt->op), phase,
                static_cast<unsigned long long>(curTick()),
                static_cast<unsigned long long>(pendingLsuInstrs.size()));

        if (pending.phase == LsuPhase::Request) {
            if (lsuAddrgenAdmittedHeadInstr >= 0 &&
                pending.pkt->vns_instr_id !=
                    lsuAddrgenAdmittedHeadInstr) {
                /*
                 * A descriptor pushed by the WLAST pop/push edge is now the
                 * addrgen FIFO head even while the old-direction data path
                 * is still draining.  A younger request must not bypass it
                 * merely because it matches that old data-path direction;
                 * doing so can create a circular RAW wait between the
                 * younger store and the admitted load.
                 */
                const auto admitted = std::find_if(
                    pendingLsuInstrs.begin(), pendingLsuInstrs.end(),
                    [this](const PendingLsuInstr &other) {
                        return other.phase == LsuPhase::Request &&
                               other.pkt->vns_instr_id ==
                                   lsuAddrgenAdmittedHeadInstr;
                    });
                panic_if(admitted == pendingLsuInstrs.end(),
                         "addrgen admitted FIFO head %d is absent",
                         lsuAddrgenAdmittedHeadInstr);
                pending.eventTick = std::max(
                    curTick(), admitted->eventTick);
                DPRINTF(VenusSequencerFull,
                        "LSU addrgen FIFO order holds instr %d/rid %d "
                        "behind admitted head %d until %llu\n",
                        pending.pkt->vns_instr_id,
                        pending.pkt->running_id,
                        lsuAddrgenAdmittedHeadInstr,
                        static_cast<unsigned long long>(
                            pending.eventTick));
                queuePendingLsu(pending);
                continue;
            }
            Tick requiredChannelTick = lsuChannelAvailableTick;
            const bool oppositeDirection =
                lsuChannelOwnerValid &&
                pending.pkt->op != lsuChannelOwnerOp;
            if (oppositeDirection) {
                requiredChannelTick = std::max(
                    requiredChannelTick,
                    lsuOppositeDirectionAvailableTick);
            }
            if (curTick() < requiredChannelTick) {
                /*
                 * addrgen's descriptor FIFO admits another request while
                 * its head has the same direction.  The LDU/STU data path
                 * may still be busy until lsuChannelAvailableTick, so leave
                 * this PendingLsuInstr in Request while acknowledging only
                 * the same-direction tagged PE transaction.  An opposite
                 * direction must wait for the descriptor head to drain.
                 */
                if (pending.pkt->vns_instr_id == lsuAddrgenAckInstr &&
                    lsuChannelOwnerValid &&
                    pending.pkt->op == lsuChannelOwnerOp) {
                    pending.addrgenQueuedBehindSameDirection = true;
                    const int ackCycles = pending.pkt->op == VSTORE
                        ? VenusStuAddrgenAckCycles
                        : VenusLduAddrgenAckCycles;
                    lsuAddrgenAckVisibleTick =
                        afterCycles(Cycles(ackCycles));
                    lsuAddrgenAckInstr = -1;
                    DPRINTF(VenusSequencerFull,
                            "LSU same-direction addrgen enqueue instr %d/"
                            "rid %d behind %s; registered ack visible at "
                            "%llu\n",
                            pending.pkt->vns_instr_id,
                            pending.pkt->running_id,
                            op_to_str(lsuChannelOwnerOp),
                            static_cast<unsigned long long>(
                                lsuAddrgenAckVisibleTick));
                }
                /*
                 * addrgen's direction head is popped by VSTU's local WLAST.
                 * Its four-entry FIFO permits the held opposite descriptor
                 * to be pushed on that same edge.  The downstream direction
                 * and W-CDC state can keep the descriptor from executing for
                 * several more cycles, so acknowledging it must not publish
                 * lsu_request or alter the fixed response latency here.
                 *
                 * A same-direction Request still resident in pendingLsuInstrs
                 * is an older descriptor ahead of the opposite request.  In
                 * that case this WLAST only advances the old head; wait for
                 * the final queued descriptor's WLAST instead.
                 */
                const bool ownerDescriptorStillQueued =
                    oppositeDirection && std::any_of(
                        pendingLsuInstrs.begin(), pendingLsuInstrs.end(),
                        [this](const PendingLsuInstr &other) {
                            return other.phase == LsuPhase::Request &&
                                   other.pkt->op == lsuChannelOwnerOp;
                        });
                if (pending.pkt->vns_instr_id == lsuAddrgenAckInstr &&
                    oppositeDirection &&
                    lsuAddrgenDirectionPopValid &&
                    !ownerDescriptorStillQueued) {
                    if (curTick() >= lsuAddrgenDirectionPopTick) {
                        lsuAddrgenAckVisibleTick = curTick();
                        lsuAddrgenAckInstr = -1;
                        lsuAddrgenAdmittedHeadInstr =
                            pending.pkt->vns_instr_id;
                        /*
                         * One WLAST pop creates exactly one descriptor FIFO
                         * slot.  Do not reuse that credit for younger LSU
                         * requests while the admitted opposite descriptor is
                         * still waiting for its data-path direction grant.
                         */
                        lsuAddrgenDirectionPopValid = false;
                        DPRINTF(VenusSequencerFull,
                                "LSU opposite-direction addrgen pop/push "
                                "instr %d/rid %d at %llu; data path held "
                                "until %llu\n",
                                pending.pkt->vns_instr_id,
                                pending.pkt->running_id,
                                static_cast<unsigned long long>(curTick()),
                                static_cast<unsigned long long>(
                                    requiredChannelTick));
                    } else {
                        requiredChannelTick = std::min(
                            requiredChannelTick,
                            lsuAddrgenDirectionPopTick);
                    }
                }
                DPRINTF(VenusSequencerFull,
                        "LSU channel hold instr %d/rid %d until %llu\n",
                        pending.pkt->vns_instr_id,
                        pending.pkt->running_id,
                        static_cast<unsigned long long>(
                            requiredChannelTick));
                pending.eventTick = requiredChannelTick;
                queuePendingLsu(pending);
                continue;
            }
            if ((singleBeatStoreCompletedThisEdge ||
                 multiBeatStoreCompletedThisEdge) &&
                lsuChannelOwnerValid &&
                pending.pkt->op != lsuChannelOwnerOp &&
                curTick() == lsuChannelAvailableTick) {
                /*
                 * Do not collapse WLAST and addrgen's
                 * direction-select D/Q boundary.  Task17's repeating
                 * store-store-load train provides both controls: when the
                 * completing 31-byte store stays within one AXI beat the
                 * following VLOAD is 104 cycles, while a beat-crossing store
                 * makes the otherwise identical VLOAD 106 cycles. Both
                 * admit internal AR one edge later. The 2:1 tile/AXI CDC
                 * phase naturally preserves their different response times;
                 * adding a second explicit response delay double-counts it.
                 */
                pending.eventTick = afterCycles(
                    Cycles(VenusLsuDirectionSwitchCycles));
                DPRINTF(VenusSequencerFull,
                        "LSU registered direction switch instr %d/rid %d "
                        "%s->%s until %llu\n",
                        pending.pkt->vns_instr_id,
                        pending.pkt->running_id,
                        op_to_str(lsuChannelOwnerOp),
                        op_to_str(pending.pkt->op),
                        static_cast<unsigned long long>(
                            pending.eventTick));
                queuePendingLsu(pending);
                continue;
            }
            /*
             * addrgen_ack_o is produced only when the tagged request really
             * leaves addrgen and enters the LDU/STU address queue.  Merely
             * inserting it into gem5's pending list is not an acknowledgement:
             * an opposite-direction descriptor at the queue head can retain
             * the shared addrgen channel for many cycles.  The RTL pulse is
             * captured by ack_done_q on the following tile edge.
             */
            if (pending.pkt->vns_instr_id == lsuAddrgenAckInstr) {
                const int ackCycles = pending.pkt->op == VSTORE
                    ? VenusStuAddrgenAckCycles
                    : VenusLduAddrgenAckCycles;
                lsuAddrgenAckVisibleTick =
                    afterCycles(Cycles(ackCycles));
                lsuAddrgenAckInstr = -1;
                DPRINTF(VenusSequencerFull,
                        "LSU addrgen channel grant instr %d/rid %d; "
                        "registered ack visible at %llu\n",
                        pending.pkt->vns_instr_id,
                        pending.pkt->running_id,
                        static_cast<unsigned long long>(
                            lsuAddrgenAckVisibleTick));
            }
            if (pending.pkt->vns_instr_id ==
                    lsuAddrgenAdmittedHeadInstr) {
                lsuAddrgenAdmittedHeadInstr = -1;
            }
            traceLsuEvent("lsu_request", pending.pkt, pending.beats,
                          pending.pkt->scalar_op);
            const bool operandsReady =
                venus_hazard_table->checkifreadytofire(
                    pending.pkt->running_id);
            constexpr uint32_t TileBlockPrefixMask = 0x0f000000;
            constexpr uint32_t TileBlockPrefix = 0x02000000;
            const bool isClusterL2 =
                (pending.pkt->scalar_op & TileBlockPrefixMask) !=
                TileBlockPrefix;
            if (pending.pkt->op == VSTORE && isClusterL2 &&
                sharedL2Object != nullptr) {
                /*
                 * Preserve the AW phase independently from operand readiness.
                 * A short store can present W before DW_axi's registered
                 * active-ID route is visible; a hazard-delayed/long store
                 * presents W after that route is already primed.
                 */
                pending.addressAxiSampleTick = alignLsuAxiEdge(curTick());
            }
            Tick firstResponseTick = 0;
            Tick lastResponseTick = 0;
            if (pending.pkt->op == VLOAD) {
                if (isClusterL2 && sharedL2Object != nullptr) {
                    const Tick tilePeriod = cyclesToTicks(Cycles(1));
                    const Tick axiPeriod = cyclesToTicks(
                        Cycles(VenusLsuBeatCycles));
                    /*
                     * alignLduOutboundAxiEdge() advances through the
                     * tile-side source-pointer register and returns the first
                     * AXI edge allowed to sample it.  The two destination
                     * pointer/signal intervals below are the synchronizer and
                     * spill boundaries; axi_cut consumes the following edge.
                     */
                    const Tick firstAxiSampleEdge =
                        alignLduOutboundAxiEdge(curTick());
                    const Tick cdcArTick = firstAxiSampleEdge +
                        VenusLduOutboundCdcAxiStagesAfterNextEdge *
                            axiPeriod;
                    const Tick externalArTick = cdcArTick +
                        VenusLduOutboundCutAxiStages * axiPeriod;
                    const auto burst = sharedL2Object->reserveLsuReadBurst(
                        externalArTick, pending.beats, axiPeriod);
                    const auto returnTick = [&](Tick externalRTick) {
                        const Tick cutRTick = externalRTick +
                            VenusLduReturnCutAxiStages * axiPeriod;
                        /*
                         * axi_cut makes R valid after its AXI edge.  The
                         * source side of the return cdc_fifo_gray therefore
                         * advances its Gray write pointer on the following
                         * AXI edge; the tile synchronizer cannot sample the
                         * data-valid state before that registered boundary.
                         */
                        const Tick sourcePointerTick =
                            cutRTick + axiPeriod;
                        return alignLsuTileEdge(sourcePointerTick) +
                            VenusLduReturnCdcTileStagesAfterNextEdge *
                                tilePeriod;
                    };
                    firstResponseTick = returnTick(burst.firstResponseTick);
                    lastResponseTick = returnTick(burst.lastResponseTick);
                } else {
                    firstResponseTick =
                        afterCycles(Cycles(VenusLsuFirstResponseCycles));
                    lastResponseTick = firstResponseTick +
                        cyclesToTicks(Cycles(VenusLsuBeatCycles)) *
                        (pending.beats - 1);
                }
            }
            if (venusVrfXbar != nullptr) {
                if (pending.pkt->op == VLOAD) {
                    /*
                     * VLDu priority is not a forecast made at AR time.
                     * ldu_result_req_o is asserted only after a complete
                     * result row is registered and the delayed issue-slot
                     * hazard is clear. executeLduResultGrant() publishes that
                     * live ownership on the actual request/grant edge.
                     */
                }
            }
            if (pending.pkt->op == VLOAD) {
                pending.minimumResponseTick = lastResponseTick;
                pending.responseBeat = 0;
                pending.responseBytes = 0;
                pending.resultRowBytes = 0;
                pending.responseBeatStride =
                    cyclesToTicks(Cycles(VenusLsuBeatCycles));
                pending.eventTick = firstResponseTick;
                lduResponseOrder.push_back(
                    pending.pkt->vns_instr_id);

                /*
                 * addrgen.sv retains every emitted descriptor in
                 * i_addrgen_req_queue until the selected consumer asserts
                 * axi_addrgen_req_ready_o.  For a load, vldu.sv asserts
                 * that ready only while consuming the final R beat.  The
                 * address generator may pipeline more loads behind a load
                 * head, but it must not change direction to a store while
                 * any older load descriptor still occupies the FIFO.
                 *
                 * Keep the ordinary same-direction AR cadence in
                 * lsuChannelAvailableTick.  Independently retain the last
                 * load-response boundary as the earliest opposite-direction
                 * admission edge.  The extra registered direction edge is
                 * the FIFO pop becoming visible to addrgen's combinational
                 * same-direction test; it is a hardware queue rule and is
                 * independent of task, PC, address, or payload.
                 */
                const Tick tilePeriod = cyclesToTicks(Cycles(1));
                lsuOppositeDirectionAvailableTick = std::max(
                    lsuOppositeDirectionAvailableTick,
                    lastResponseTick + tilePeriod);
            } else {
                if (!(isClusterL2 && sharedL2Object != nullptr)) {
                    int responseCycles = VenusLsuFirstResponseCycles +
                        VenusLsuBeatCycles * (pending.beats - 1);
                    if (pending.hazardAtAdmission && operandsReady) {
                        responseCycles +=
                            4 * std::max(0, pending.rows - 1);
                    }
                    pending.minimumResponseTick =
                        afterCycles(Cycles(responseCycles));
                }
            }
            if (pending.pkt->op == VSTORE) {
                /*
                 * vstu does not consume operands on the PE-valid edge.
                 * Its registered request state becomes visible one tile
                 * clock later; only that state may compete for VRF banks.
                 * Keep this boundary even when the operands were already
                 * ready at admission so requester priority and W timing do
                 * not depend on a zero-time shortcut in the model.
                 */
                pending.eventTick = afterCycles(
                    Cycles(VenusStuOperandRequestCycles));
                pending.phase = LsuPhase::StoreOperands;
            } else {
                pending.phase = LsuPhase::MemoryResponse;
            }
            /*
             * AR is independent of the R burst. The old model held the
             * request channel until response and therefore could not express
             * RTL's multiple outstanding reads. AW/W retains its existing
             * serialized operand/data lifetime.
             */
            if (pending.pkt->op == VLOAD) {
                lsuChannelAvailableTick =
                    afterCycles(Cycles(VenusLsuBeatCycles));
                lsuChannelOwnerOp = VLOAD;
                lsuChannelOwnerValid = true;
                lsuAddrgenDirectionPopValid = false;
            } else {
                lsuChannelAvailableTick = pending.eventTick;
                lsuChannelOwnerOp = VSTORE;
                lsuChannelOwnerValid = true;
                /* Store WLAST is not known until operands are admitted. */
                lsuAddrgenDirectionPopValid = false;
            }
            queuePendingLsu(pending);
            continue;
        }

        if (pending.phase == LsuPhase::StoreOperands) {
            /*
             * RTL vstu.sv gates stu_operand_req_o with the hazard_vs2 value
             * captured in its instruction queue, not with a fresh all-class
             * hazard lookup.  Each cycle the queue entry performs
             *
             *   captured_vs2 &= global_hazard_table[consumer_id]
             *
             * Model the same destructive clear with generation-tagged B/PE
             * completion tombstones.  This is physical VSTU queue behavior;
             * it is independent of instruction PC, address, data, or DAG.
             */
            for (int producer = 0; producer < NrIDs; ++producer) {
                const uint8_t bit = uint8_t(1U << producer);
                if (!(pending.storeHazardVs2 & bit))
                    continue;
                const int generation =
                    pending.storeHazardVs2Generation[producer];
                const bool generationRetired = generation >= 0 &&
                    venus_hazard_table->retired_vns_instr_id[producer] >=
                        generation;
                const bool liveVs2 =
                    venus_hazard_table->vs2_hazard_table[
                        pending.pkt->running_id][producer];
                if (generationRetired || !liveVs2)
                    pending.storeHazardVs2 &= ~bit;
            }
            if (pending.storeHazardVs2 != 0) {
                pending.eventTick = afterCycles(Cycles(1));
                lsuChannelAvailableTick = std::max(
                    lsuChannelAvailableTick, pending.eventTick);
                lsuChannelOwnerOp = VSTORE;
                lsuChannelOwnerValid = true;
                queuePendingLsu(pending);
                continue;
            }
            if (venusVrfXbar != nullptr) {
                venusVrfXbar->reserveVenusVrfLsuPriority(
                    venusLsuRequesterTag(pending.pkt, 0, false),
                    venusFullVrfDataBankMask(), curTick(),
                    pending.rows + VenusStuOperandLookaheadRows,
                    cyclesToTicks(Cycles(1)));
            }
            constexpr uint32_t TileBlockPrefixMask = 0x0f000000;
            constexpr uint32_t TileBlockPrefix = 0x02000000;
            const bool isClusterL2 =
                (pending.pkt->scalar_op & TileBlockPrefixMask) !=
                TileBlockPrefix;
            if (isClusterL2 && sharedL2Object != nullptr) {
                const Tick tilePeriod = cyclesToTicks(Cycles(1));
                const Tick axiPeriod = cyclesToTicks(
                    Cycles(VenusLsuBeatCycles));
                Tick firstLocalWriteTick = afterCycles(Cycles(
                    VenusStuOperandToLocalWriteCycles));
                if (stuWriteDataTailValid) {
                    firstLocalWriteTick = std::max(
                        firstLocalWriteTick,
                        stuWriteDataTailTick +
                            VenusStuPostWlastRestartTileCycles * tilePeriod);
                }
                const auto burst = sharedL2Object->reserveLsuWriteBurst(
                    reinterpret_cast<uintptr_t>(this),
                    firstLocalWriteTick, pending.addressAxiSampleTick,
                    pending.beats, tilePeriod, axiPeriod);
                const Tick cutBTick = burst.externalResponseTick +
                    VenusStuReturnCutAxiStages * axiPeriod;
                const Tick sourcePointerTick = cutBTick +
                    VenusStuReturnSourcePointerAxiStages * axiPeriod;
                pending.minimumResponseTick =
                    alignLsuTileEdge(sourcePointerTick) +
                    VenusStuReturnCdcTileStagesAfterSourceEdge *
                        tilePeriod;
                pending.eventTick = burst.lastExternalWriteTick;
                const Tick lastLocalWriteTick = burst.lastLocalWriteTick;
                DPRINTF(VenusSequencerFull,
                        "LSU W CDC schedule instr %d/rid %d beats=%u "
                        "local=[%llu,%llu] external=[%llu,%llu] "
                        "source_backpressured=%d full_stalls=%u "
                        "outstanding=%u next_release=%llu queued=%d\n",
                        pending.pkt->vns_instr_id,
                        pending.pkt->running_id, pending.beats,
                        static_cast<unsigned long long>(
                            burst.firstLocalWriteTick),
                        static_cast<unsigned long long>(
                            burst.lastLocalWriteTick),
                        static_cast<unsigned long long>(
                            burst.firstExternalWriteTick),
                        static_cast<unsigned long long>(
                            burst.lastExternalWriteTick),
                        burst.sourceBackpressured,
                        burst.sourceFullStallCount,
                        burst.sourceOutstandingAtEnd,
                        static_cast<unsigned long long>(
                            burst.sourceNextReleaseTick),
                        pending.addrgenQueuedBehindSameDirection);
                stuWriteDataTailTick = lastLocalWriteTick;
                stuWriteDataTailValid = true;
                lsuChannelAvailableTick = lastLocalWriteTick + tilePeriod;
                lsuAddrgenDirectionPopTick = lastLocalWriteTick;
                /*
                 * The simultaneous pop/push observation is specific to a
                 * WLAST held by a full W-CDC source FIFO.  An unstalled
                 * WLAST advances through addrgen's ordinary registered
                 * direction boundary; acknowledging it here makes short
                 * store/load loops accumulate an impossible early cycle.
                 */
                lsuAddrgenDirectionPopValid =
                    burst.sourceBackpressured;
                lsuOppositeDirectionAvailableTick =
                    burst.sourceBackpressured
                    ? burst.sourceNextReleaseTick +
                        (VenusLsuPostSourceReleaseTileCycles +
                         (pending.addrgenQueuedBehindSameDirection
                              ? VenusLsuQueuedAddrgenPopTileCycles : 0)) *
                            tilePeriod
                    : lastLocalWriteTick + tilePeriod;
                lsuChannelOwnerOp = VSTORE;
                lsuChannelOwnerValid = true;
            } else {
                const int operandToResponseCycles =
                    5 + 2 * pending.rows +
                    VenusLsuBeatCycles * (pending.beats - 1);
                pending.eventTick = std::max(
                    pending.minimumResponseTick,
                    afterCycles(Cycles(operandToResponseCycles)));
            }
            pending.phase = isClusterL2 && sharedL2Object != nullptr
                ? LsuPhase::StoreDataCommit : LsuPhase::MemoryResponse;
            if (!(isClusterL2 && sharedL2Object != nullptr))
                lsuChannelAvailableTick = pending.eventTick;
            if (!(isClusterL2 && sharedL2Object != nullptr)) {
                lsuChannelOwnerOp = VSTORE;
                lsuChannelOwnerValid = true;
            }
            queuePendingLsu(pending);
            continue;
        }

        if (pending.phase == LsuPhase::StoreDataCommit) {
            /*
             * The slave byte array changes when the final W beat is
             * accepted.  B returns later through the response cut and CDC;
             * delaying the write until B makes a younger load see stale
             * data once request admission and response retirement differ.
             */
            executeLsuInstr(pending.pkt);
            pending.storeDataCommitted = true;
            traceLsuEvent("lsu_store_data_commit", pending.pkt,
                          pending.beats, pending.pkt->scalar_op);
            pending.eventTick = pending.minimumResponseTick;
            pending.phase = LsuPhase::MemoryResponse;
            queuePendingLsu(pending);
            continue;
        }

        if (pending.phase == LsuPhase::MemoryResponse) {
            if (pending.pkt->op == VLOAD) {
                /*
                 * Consume the registered R stream one beat at a time.  A
                 * full two-entry result queue backpressures every R beat,
                 * including the first half of the next 128-byte row.  This
                 * is the boundary exposed by PDSCH task3 sequence 6: two
                 * rows remain resident while hazard_vd1 is live, then the
                 * remaining beats resume as the rows drain.
                 */
                const bool responseHead = !lduResponseOrder.empty() &&
                    lduResponseOrder.front() ==
                        pending.pkt->vns_instr_id;
                if (!responseHead || lduResultQueue.size() >= 2) {
                    pending.responseBackpressured = true;
                    pending.eventTick = afterCycles(Cycles(1));
                    DPRINTF(VenusSequencerFull,
                            "LDU R beat hold instr %d/rid %d beat %d/%d "
                            "head=%d result_occupancy=%llu/2\n",
                            pending.pkt->vns_instr_id,
                            pending.pkt->running_id,
                            pending.responseBeat + 1, pending.beats,
                            responseHead,
                            static_cast<unsigned long long>(
                                lduResultQueue.size()));
                    queuePendingLsu(std::move(pending));
                    continue;
                }

                const int totalBytes =
                    pending.pkt->vl << static_cast<int>(pending.pkt->vew);
                const int firstOffset =
                    static_cast<int>(pending.pkt->scalar_op & 63);
                const int beatCapacity = pending.responseBeat == 0
                    ? 64 - firstOffset : 64;
                const int beatRemaining =
                    beatCapacity - pending.responseBeatBytes;
                const int rowRemaining = 128 - pending.resultRowBytes;
                const int payloadRemaining =
                    totalBytes - pending.responseBytes;
                const int validBytes = std::min(
                    beatRemaining, std::min(rowRemaining, payloadRemaining));
                fatal_if(validBytes <= 0,
                         "VLDu instr %d consumed an empty R beat %d/%d",
                         pending.pkt->vns_instr_id,
                         pending.responseBeat + 1, pending.beats);
                pending.responseBeatBytes += validBytes;
                pending.responseBytes += validBytes;
                pending.resultRowBytes += validBytes;

                const bool finalPayload =
                    pending.responseBytes == totalBytes;
                const bool beatComplete =
                    pending.responseBeatBytes == beatCapacity || finalPayload;
                if (beatComplete) {
                    ++pending.responseBeat;
                    pending.responseBeatBytes = 0;
                }
                const bool rowComplete =
                    pending.resultRowBytes == 128 || finalPayload;

                if (rowComplete) {
                    LduResultRow row;
                    row.visibleTick = afterCycles(Cycles(
                        VenusLduResultQueueVisibilityCycles));
                    row.pkt = pending.pkt;
                    row.row = (pending.responseBytes - 1) / 128;
                    row.rows = pending.rows;
                    row.beats = pending.beats;
                    row.final = finalPayload;
                    /*
                     * The active RTL deliberately sets every lane-valid bit
                     * when a VLDu result row is formed; its older be-derived
                     * lane mask is commented out.  Consequently even a
                     * partial tail row asserts ldu_result_req_i in all lanes
                     * and owns all four banks per lane.
                     */
                    row.vrfBankMask = venusFullVrfDataBankMask();
                    pending.resultRowBytes = 0;

                    if (finalPayload) {
                        fatal_if(pending.responseBeat != pending.beats,
                                 "VLDu instr %d payload ended at beat %d/%d",
                                 pending.pkt->vns_instr_id,
                                 pending.responseBeat, pending.beats);
                        captureLsuLoad(pending);
                        row.loadValues = std::move(pending.loadValues);
                        row.loadValid = std::move(pending.loadValid);
                        traceLsuEvent("lsu_response", pending.pkt,
                                      pending.beats,
                                      pending.pkt->scalar_op);
                        fatal_if(lduResponseOrder.empty() ||
                                 lduResponseOrder.front() !=
                                    pending.pkt->vns_instr_id,
                                 "VLDu response order lost instr %d",
                                 pending.pkt->vns_instr_id);
                        lduResponseOrder.pop_front();
                        lduIssuePntD =
                            (lduIssuePntD + 1) % VenusLsuQueueDepth;
                    }
                    queueLduResultRow(std::move(row));
                }

                if (!finalPayload) {
                    /*
                     * When a beat straddles a result-row boundary, RTL
                     * retains the unconsumed bytes on R and forms the next
                     * row on the following tile edge.  Likewise, a stream
                     * that was backpressured can present already-buffered
                     * beats on consecutive tile edges while catching up.
                     */
                    const Cycles spacing =
                        (!beatComplete || pending.responseBackpressured)
                        ? Cycles(1) : Cycles(VenusLsuBeatCycles);
                    pending.eventTick = afterCycles(spacing);
                    queuePendingLsu(std::move(pending));
                }
                continue;
            }
            const int runningId = pending.pkt->running_id;
            if (!pending.storeDataCommitted &&
                !venus_hazard_table->checkifreadytofire(runningId)) {
                DPRINTF(VenusSequencerFull,
                        "LSU response hazard hold instr %d/rid %d: %s\n",
                        pending.pkt->vns_instr_id, runningId,
                        venus_hazard_table->reportHazardStat(
                            runningId).c_str());
                pending.eventTick = afterCycles(Cycles(1));
                lsuChannelAvailableTick = pending.eventTick;
                lsuChannelOwnerOp = VSTORE;
                lsuChannelOwnerValid = true;
                queuePendingLsu(pending);
                continue;
            }
            if (!pending.storeDataCommitted)
                executeLsuInstr(pending.pkt);
            traceLsuEvent("lsu_response", pending.pkt, pending.beats,
                          pending.pkt->scalar_op);
            if (pending.pkt->op == VSTORE) {
                /*
                 * vstu asserts pe_resp_d on the internal B handshake.  In
                 * that same combinational cycle venus_sequencer masks the
                 * completing generation out of global_hazard_table_d via
                 * vinsn_running_d.  A queued store therefore observes the
                 * dependency clear on the following tile edge, before the
                 * older instruction reaches the later monitored recycle
                 * boundary.  Keep retirement, VID release, VINS dumping,
                 * and the broadcast Q state where they are; this tombstone
                 * is only the generation-tagged D-state observation used by
                 * sequencer-local LSU admission.
                 */
                const int runningId = pending.pkt->running_id;
                venus_hazard_table->retired_vns_instr_id[runningId] =
                    std::max(
                        venus_hazard_table->retired_vns_instr_id[runningId],
                        static_cast<int>(pending.pkt->vns_instr_id));
                DPRINTF(VenusSequencerFull,
                        "VSTU internal-B tombstone instr %d/rid %d at %llu\n",
                        pending.pkt->vns_instr_id, runningId,
                        static_cast<unsigned long long>(curTick()));
            }
            pending.eventTick =
                afterCycles(Cycles(VenusStuCommitCycles));
            pending.phase = LsuPhase::Complete;
            pending.pkt->vns_instr_log_endtick = pending.eventTick;
            queuePendingLsu(pending);
            continue;
        }

        if (pending.phase == LsuPhase::ResultQueueVisible) {
            const int runningId = pending.pkt->running_id;
            if (!venus_hazard_table->checkifreadytofire(runningId)) {
                DPRINTF(VenusSequencerFull,
                        "LDU result FIFO hold instr %d/rid %d: %s\n",
                        pending.pkt->vns_instr_id,
                        runningId,
                        venus_hazard_table->reportHazardStat(
                            runningId).c_str());
                pending.eventTick = afterCycles(Cycles(1));
                queuePendingLsu(pending);
                continue;
            }
            /*
             * ldu_result_req_o and the normal requester grant handshake on
             * this edge.  The operand requester returns its registered final
             * grant one tile clock later.
             */
            pending.eventTick = afterCycles(
                Cycles(VenusLduFinalGrantCycles));
            pending.phase = LsuPhase::ResultFinalGrant;
            queuePendingLsu(pending);
            continue;
        }

        if (pending.phase == LsuPhase::ResultFinalGrant) {
            commitLsuLoad(pending);
            traceLsuEvent("lsu_result_commit", pending.pkt, pending.rows,
                          pending.pkt->scalar_op);
            /*
             * result_final_gnt_d makes load_complete_o and commit_cnt_d
             * visible here.  pe_resp_o is a separate registered boundary.
             */
            pending.eventTick = afterCycles(
                Cycles(VenusLduPeResponseCycles));
            pending.phase = LsuPhase::PeResponse;
            queuePendingLsu(pending);
            continue;
        }

        if (pending.phase == LsuPhase::PeResponse) {
            /*
             * pe_resp_o.vinsn_done is sampled by the sequencer on this edge;
             * its lifecycle/recycle monitor observes the response one
             * registered tile edge later.
             */
            forwardLsuCompletionToShuffle(pending.pkt);
            pending.eventTick = afterCycles(
                Cycles(VenusLduSequencerRecycleCycles));
            pending.phase = LsuPhase::Complete;
            pending.pkt->vns_instr_log_endtick = pending.eventTick;
            queuePendingLsu(pending);
            continue;
        }

        traceLsuEvent("lsu_complete", pending.pkt);
        /*
         * STU commit_cnt_d is decremented in the completion cycle, while
         * pe_req_ready_o is derived from commit_cnt_q.  Backpressure then
         * traverses the sequencer before a new upstream ID can fire.  Keep
         * the completed entry as a retirement token for those two registered
         * visibility edges.  LDU's independently held addrgen request has a
         * separate lifetime and must not inherit the STU rule.
         */
        if (pending.pkt->op == VSTORE) {
            singleBeatStoreCompletedThisEdge |= pending.beats == 1;
            multiBeatStoreCompletedThisEdge |= pending.beats > 1;
            lsuRetiringSlots.push_back({
                pending.pkt->op, afterCycles(Cycles(2))
            });
        } else {
            fatal_if(lduActiveLoads == 0,
                     "VLDu completion underflow for instr %d",
                     pending.pkt->vns_instr_id);
            --lduActiveLoads;
        }
        pending.pkt->makeResponse();
        handleVinsnDoneReport(pending.pkt);
        delete pending.pkt;
    }

    scheduleNextLsuEvent();
}

bool
VenusSequencer::queueLaneDoneReport(VenusInstrPkt *pkt, int laneId)
{
    panic_if(pkt == nullptr, "cannot queue a null lane-done report");
    /*
     * Venus1 first captures the VFU done pulse in lane_sequencer.vinsn_done_q,
     * then captures pe_resp_i in the main sequencer's pe_resp_i_q.  The
     * combinational pe_vinsn_running_q_comb consumes that second register and
     * the following running-table edge is the observable retirement: two
     * edges from the lane completion callback.  The operand requester has an
     * additional requester_q hazard-clear edge, modelled independently by
     * the tagged command-retirement path in VenusLane.  Do not fold that
     * requester edge into the VINS lifecycle itself.  Venus2 comments out the
     * main pe_resp_i_q and retains its established one-edge boundary.
     */
    const Tick visible = afterCycles(Cycles(
        rtlRegisteredPeResponse ? 2 : 1));
    pendingLaneDoneReports.push_back(
        {new VenusInstrPkt(pkt), visible, laneId});
    DPRINTF(VenusSequencer,
            "Queued registered lane done report instr %d/rid %d visible %llu\n",
            pkt->vns_instr_id, pkt->running_id, visible);
    if (!nextLaneDoneReportEvent.scheduled()) {
        schedule(nextLaneDoneReportEvent, visible);
    } else if (visible < nextLaneDoneReportEvent.when()) {
        deschedule(nextLaneDoneReportEvent);
        schedule(nextLaneDoneReportEvent, visible);
    }
    return true;
}

void
VenusSequencer::noteRtlLaneDone(int laneId, const VenusInstrPkt *pkt)
{
    if (!rtlLaneDesyncStallEnabled || laneId < 0 || laneId >= NrLanes ||
        pkt == nullptr || pkt->running_id < 0 ||
        pkt->running_id >= NrIDs) {
        return;
    }
    const int runningId = pkt->running_id;
    const VenusInstrPkt *active = vinsn_running_pkt[runningId];
    if (active == nullptr || active->vns_instr_id != pkt->vns_instr_id)
        return;
    rtlLaneDone[runningId][laneId] = true;
}

void
VenusSequencer::noteRtlLaneVfuDone(
    int laneId, const VenusInstrPkt *pkt)
{
    if (!rtlRegisteredPeResponse || laneId != 0 || pkt == nullptr ||
        pkt->running_id < 0 || pkt->running_id >= NrIDs) {
        return;
    }

    const VenusInstrPkt *active = vinsn_running_pkt[pkt->running_id];
    if (active == nullptr || active->vns_instr_id != pkt->vns_instr_id)
        return;

    auto [vfu1, vfu2, vfuMask] = findVinsnTargetVFU(
        const_cast<VenusInstrPkt *>(active));
    auto laneVfu = [](VFU vfu) {
        return vfu == VFU_BitALU || vfu == VFU_CAU ||
               vfu == VFU_SerDiv || vfu == VFU_Mask;
    };

    /*
     * venus_sequencer.sv decrements the arithmetic/mask queue counters from
     * bitalu/cau/serdiv_vinsn_done_lanes_ms[0].  It does not wait for every
     * lane's pe_resp retirement.  Shuffle owns a separate global done wire
     * and is deliberately left to its ordinary completion path here.
     */
    if (laneVfu(vfu1))
        releaseVfuQueueOnce(active, vfu1);
    if (laneVfu(vfu2))
        releaseVfuQueueOnce(active, vfu2);
    if (laneVfu(vfuMask))
        releaseVfuQueueOnce(active, vfuMask);
}

bool
VenusSequencer::rtlLaneDesyncStall() const
{
    if (!rtlLaneDesyncStallEnabled)
        return false;
    for (int runningId = 0; runningId < NrIDs; ++runningId) {
        if (vinsn_running_pkt[runningId] == nullptr ||
            !rtlLaneDone[runningId][0]) {
            continue;
        }
        for (int lane = 1; lane < NrLanes; ++lane) {
            if (vinsn_running_pkt_laneuseboard[runningId][lane] &&
                !rtlLaneDone[runningId][lane]) {
                return true;
            }
        }
    }
    return false;
}

void
VenusSequencer::noteLaneProducerGrant(
    int laneId, int runningId, int instructionId)
{
    panic_if(laneId < 0 || laneId >= NrLanes,
             "invalid producer-grant lane %d", laneId);
    panic_if(runningId < 0 || runningId >= NrIDs,
             "invalid producer-grant running ID %d", runningId);
    panic_if(instructionId < 0,
             "invalid producer-grant instruction %d", instructionId);

    VenusInstrPkt *active = nullptr;
    for (int i = 0; i < NrIDs; ++i) {
        if (vinsn_running_pkt[i] != nullptr &&
            vinsn_running_pkt[i]->running_id == runningId &&
            vinsn_running_pkt[i]->vns_instr_id == instructionId) {
            active = vinsn_running_pkt[i];
            break;
        }
    }
    if (active == nullptr) {
        DPRINTF(VenusSequencerFull,
                "Ignoring stale lane producer grant instr %d/rid %d lane %d\n",
                instructionId, runningId, laneId);
        return;
    }
    panic_if(active->op == VLOAD || active->op == VSTORE,
             "LSU instruction %d used lane producer-grant sideband",
             instructionId);

    if (laneProducerGrantGeneration[runningId] != instructionId ||
        laneProducerGrantBoard[runningId].size() !=
            static_cast<size_t>(NrLanes)) {
        laneProducerGrantGeneration[runningId] = instructionId;
        laneProducerCompletionQueuedGeneration[runningId] = -1;
        laneProducerGrantBoard[runningId].assign(NrLanes, false);
    }
    laneProducerGrantBoard[runningId][laneId] = true;
    for (int lane = 0; lane < NrLanes; ++lane) {
        /* A zero-local-VL tail lane participates in ready/done but never
         * produces a bank grant. */
        if (vinsn_running_pkt_laneactiveboard[runningId][lane] &&
            !laneProducerGrantBoard[runningId][lane]) {
            return;
        }
    }

    if (laneProducerCompletionQueuedGeneration[runningId] != instructionId) {
        /*
         * The all-lane final-grant reduction first becomes the lane's
         * registered vinsn_done.  Venus1 then adds pe_resp_i_q before the
         * main sequencer may publish the dependency clear; Venus2 has no
         * such register.  This is a backend pipeline boundary, independent
         * of the instruction, task or DAG.
         */
        const Tick visible = afterCycles(Cycles(
            rtlRegisteredPeResponse ? 2 : 1));
        pendingLaneGrantCompletions.push_back(
            {runningId, instructionId, visible});
        laneProducerCompletionQueuedGeneration[runningId] = instructionId;
        if (!nextLaneGrantCompletionEvent.scheduled()) {
            schedule(nextLaneGrantCompletionEvent, visible);
        } else if (visible < nextLaneGrantCompletionEvent.when()) {
            reschedule(nextLaneGrantCompletionEvent, visible);
        }
        DPRINTF(VenusSequencerFull,
                "All-lane final-grant D captured for instr %d/rid %d at "
                "%llu; command completion Q visible %llu\n",
                instructionId, runningId,
                static_cast<unsigned long long>(curTick()),
                static_cast<unsigned long long>(visible));
    }

    /*
     * This tombstone affects sequencer-local dependency tests only.  The
     * lane PE responses, registered done-report collection, running-ID
     * release, VINS dump, and hazard-table broadcast remain at their
     * existing later boundaries.
     */
    if (!rtlRegisteredPeResponse) {
        venus_hazard_table->retired_vns_instr_id[runningId] = std::max(
            venus_hazard_table->retired_vns_instr_id[runningId],
            instructionId);
    }

    /*
     * global_hazard_table_o is driven from global_hazard_table_d in RTL.
     * When every participating lane presents pe_resp for this generation,
     * venus_shuffle_engine destructively masks the matching captured hazard
     * before the later registered VID-release observation.  The banked
     * write path gives gem5 the equivalent tagged boundary when every
     * lane's final result grant has completed.  Publish only that local
     * combinational observation to Shuffle: ordinary lane done collection,
     * VINS retirement, hazard-table broadcast, and ID release remain on
     * their existing registered boundaries.
     */
    if (!rtlRegisteredPeResponse) {
        VenusHazardTable completion;
        completion.producer_completion_valid = true;
        completion.producer_completion_running_id = runningId;
        completion.producer_completion_vns_instr_id = instructionId;
        port_venussequencer_hazardtable_boardcast_to_shuffle.sendPacket(
            static_cast<PacketPtr>(&completion));
    }
    DPRINTF(VenusSequencerFull,
            "All lane final grants visible for instr %d/rid %d at %llu; "
            "tagged completion %s for Shuffle\n",
            instructionId, runningId,
            static_cast<unsigned long long>(curTick()),
            rtlRegisteredPeResponse ? "deferred through pe_resp_i_q" :
                                      "forwarded");
}

void
VenusSequencer::executeLaneGrantCompletions()
{
    Tick nextVisible = MaxTick;
    std::deque<PendingLaneGrantCompletion> deferred;
    while (!pendingLaneGrantCompletions.empty()) {
        const PendingLaneGrantCompletion completion =
            pendingLaneGrantCompletions.front();
        pendingLaneGrantCompletions.pop_front();
        if (completion.visibleTick > curTick()) {
            nextVisible = std::min(nextVisible, completion.visibleTick);
            deferred.push_back(completion);
            continue;
        }

        const int runningId = completion.runningId;
        const int instructionId = completion.instructionId;
        const VenusInstrPkt *active =
            runningId >= 0 && runningId < NrIDs ?
                vinsn_running_pkt[runningId] : nullptr;
        if (active == nullptr || active->vns_instr_id != instructionId) {
            DPRINTF(VenusSequencerFull,
                    "Ignoring stale all-lane command completion instr "
                    "%d/rid %d\n", instructionId, runningId);
            continue;
        }

        if (rtlRegisteredPeResponse) {
            venus_hazard_table->retired_vns_instr_id[runningId] = std::max(
                venus_hazard_table->retired_vns_instr_id[runningId],
                instructionId);
            VenusHazardTable producer;
            producer.producer_completion_valid = true;
            producer.producer_completion_running_id = runningId;
            producer.producer_completion_vns_instr_id = instructionId;
            port_venussequencer_hazardtable_boardcast_to_shuffle.sendPacket(
                static_cast<PacketPtr>(&producer));
        }

        VenusHazardTable tagged;
        tagged.lane_command_completion_valid = true;
        tagged.lane_command_completion_running_id = runningId;
        tagged.lane_command_completion_vns_instr_id = instructionId;
        for (int lane = 0; lane < NrLanes; ++lane) {
            port_venussequencer_hazardtable_boardcast[lane].sendPacket(
                static_cast<PacketPtr>(&tagged));
        }
        DPRINTF(VenusSequencerFull,
                "All-lane command completion Q broadcast instr %d/rid %d "
                "at %llu\n", instructionId, runningId,
                static_cast<unsigned long long>(curTick()));
    }
    pendingLaneGrantCompletions.swap(deferred);
    if (!pendingLaneGrantCompletions.empty())
        schedule(nextLaneGrantCompletionEvent, nextVisible);
}

void
VenusSequencer::queueProducerCompletionToLanes(
    int runningId, int instructionId)
{
    const Tick visible = afterCycles(Cycles(1));
    pendingProducerCompletions.push_back(
        {runningId, instructionId, visible});
    if (!nextProducerCompletionEvent.scheduled()) {
        schedule(nextProducerCompletionEvent, visible);
    } else if (visible < nextProducerCompletionEvent.when()) {
        reschedule(nextProducerCompletionEvent, visible);
    }
}

void
VenusSequencer::executeProducerCompletions()
{
    Tick nextVisible = MaxTick;
    std::deque<PendingProducerCompletion> deferred;
    while (!pendingProducerCompletions.empty()) {
        const PendingProducerCompletion completion =
            pendingProducerCompletions.front();
        pendingProducerCompletions.pop_front();
        if (completion.visibleTick > curTick()) {
            nextVisible = std::min(nextVisible, completion.visibleTick);
            deferred.push_back(completion);
            continue;
        }
        const VenusInstrPkt *active =
            completion.runningId >= 0 && completion.runningId < NrIDs ?
                vinsn_running_pkt[completion.runningId] : nullptr;
        if (active == nullptr ||
            active->vns_instr_id != completion.instructionId) {
            DPRINTF(VenusSequencerFull,
                    "Ignoring stale registered producer completion instr "
                    "%d/rid %d\n", completion.instructionId,
                    completion.runningId);
            continue;
        }
        forwardProducerCompletionToLanes(active);
    }
    pendingProducerCompletions.swap(deferred);
    if (!pendingProducerCompletions.empty())
        schedule(nextProducerCompletionEvent, nextVisible);
}

void
VenusSequencer::noteShuffleProducerGrant(int runningId, int instructionId)
{
    panic_if(runningId < 0 || runningId >= NrIDs || instructionId < 0,
             "invalid shuffle producer grant instr %d/rid %d",
             instructionId, runningId);
    const VenusInstrPkt *active = vinsn_running_pkt[runningId];
    if (active == nullptr || active->vns_instr_id != instructionId) {
        DPRINTF(VenusSequencerFull,
                "Ignoring stale shuffle producer grant instr %d/rid %d\n",
                instructionId, runningId);
        return;
    }

    /*
     * The last shuffle_result grant first raises
     * parallel_shuffle_complete_d.  Only the following registered edge
     * clears global_hazard_table_d and makes an operand request eligible.
     * Queue that D/Q boundary instead of publishing the tombstone directly
     * from the final bank-grant callback.
     */
    const Tick visible = afterCycles(Cycles(1));
    pendingShuffleGrantCompletions.push_back(
        {runningId, instructionId, visible});
    if (!nextShuffleGrantCompletionEvent.scheduled()) {
        schedule(nextShuffleGrantCompletionEvent, visible);
    } else if (visible < nextShuffleGrantCompletionEvent.when()) {
        reschedule(nextShuffleGrantCompletionEvent, visible);
    }
    DPRINTF(VenusSequencerFull,
            "All shuffle final bank grants captured for instr %d/rid %d "
            "at %llu; registered completion visible %llu\n",
            instructionId, runningId,
            static_cast<unsigned long long>(curTick()),
            static_cast<unsigned long long>(visible));
}

void
VenusSequencer::executeShuffleGrantCompletions()
{
    Tick nextVisible = MaxTick;
    std::deque<PendingShuffleGrantCompletion> deferred;
    while (!pendingShuffleGrantCompletions.empty()) {
        const PendingShuffleGrantCompletion completion =
            pendingShuffleGrantCompletions.front();
        pendingShuffleGrantCompletions.pop_front();
        if (completion.visibleTick > curTick()) {
            nextVisible = std::min(nextVisible, completion.visibleTick);
            deferred.push_back(completion);
            continue;
        }

        const int runningId = completion.runningId;
        const int instructionId = completion.instructionId;
        const VenusInstrPkt *active =
            runningId >= 0 && runningId < NrIDs ?
                vinsn_running_pkt[runningId] : nullptr;
        if (active == nullptr || active->vns_instr_id != instructionId) {
            DPRINTF(VenusSequencerFull,
                    "Ignoring stale registered shuffle completion instr "
                    "%d/rid %d\n",
                    instructionId, runningId);
            continue;
        }

        venus_hazard_table->retired_vns_instr_id[runningId] = std::max(
            venus_hazard_table->retired_vns_instr_id[runningId],
            instructionId);
        lduRequesterRetiredGeneration[runningId] = std::max(
            lduRequesterRetiredGeneration[runningId], instructionId);
        DPRINTF(VenusSequencerFull,
                "Registered shuffle completion visible for instr %d/rid "
                "%d at %llu\n",
                instructionId, runningId,
                static_cast<unsigned long long>(curTick()));
    }
    pendingShuffleGrantCompletions.swap(deferred);
    if (!pendingShuffleGrantCompletions.empty())
        schedule(nextShuffleGrantCompletionEvent, nextVisible);
}

void
VenusSequencer::executeLaneDoneReports()
{
    std::deque<PendingLaneDoneReport> deferred;
    Tick nextVisible = MaxTick;
    while (!pendingLaneDoneReports.empty()) {
        PendingLaneDoneReport report = pendingLaneDoneReports.front();
        pendingLaneDoneReports.pop_front();
        panic_if(report.pkt == nullptr,
                 "registered lane-done queue contains a null report");
        if (report.visibleTick <= curTick()) {
            noteRtlLaneDone(report.laneId, report.pkt);
            handleVinsnDoneReport(report.pkt);
            delete report.pkt;
        } else {
            nextVisible = std::min(nextVisible, report.visibleTick);
            deferred.push_back(report);
        }
    }
    pendingLaneDoneReports.swap(deferred);
    if (!pendingLaneDoneReports.empty())
        schedule(nextLaneDoneReportEvent, nextVisible);
}

bool VenusSequencer::handleVinsnDoneReport(VenusInstrPkt* pkt)
{
    int matched_idx = -1;
    for(int i=0;i<NrIDs;i++) {
        if(this->vinsn_running_pkt[i] != nullptr)
        {
            if(this->vinsn_running_pkt[i]->running_id == pkt->running_id)
            {
                matched_idx = i;
                this->vinsn_running_pkt[i]->mergeResultDataFrom(pkt);
                this->vinsn_running_pkt_lanedonecounter[i]++;
                if(this->vinsn_running_pkt_lanedonecounter[i] < NrLanes)
                {
                    DPRINTF(VenusSequencer, "VenusSequencer received lane done report for instr %d whose RunningVID %d, but still waiting for other lanes, now got %d/%d lanes done.\n", pkt->vns_instr_id, pkt->running_id, this->vinsn_running_pkt_lanedonecounter[i], NrLanes);
                    return true;
                }
                else
                {
                    DPRINTF(VenusSequencer, "VenusSequencer received lane done report for instr %d whose RunningVID %d, all lanes done.\n", pkt->vns_instr_id, pkt->running_id);
                    break;
                }
            }
        }
    }

    this->recved_finished_instr_pkt = new VenusInstrPkt(
        matched_idx >= 0 ? this->vinsn_running_pkt[matched_idx] : pkt);
    this->recved_finished_instr_pkt->vns_instr_stat = INSTR_RECYCLE;
    this->recved_finished_instr_pkt->vns_instr_log_recycletick = curTick();

    DPRINTF(VenusSequencer, "VenusSequencer understands that instr %d whose RunningVID %d is done.\n", pkt->vns_instr_id, pkt->running_id);
    auto [vfutmp1, vfutmp2, vfu_mtmp] = findVinsnTargetVFU(this->recved_finished_instr_pkt);
    releaseTargetVFU(this->recved_finished_instr_pkt,
                     vfutmp1, vfutmp2, vfu_mtmp);

    if (this->recved_finished_instr_pkt->op == VLOAD ||
        this->recved_finished_instr_pkt->op == VSTORE) {
        forwardLsuCompletionToLanes(this->recved_finished_instr_pkt);
    }

    /*
     * VBRDCST is already computed by each BitALU lane and committed through
     * the banked result/grant path.  A second full-vector backdoor write at
     * global retirement is not an RTL edge and can violate WAW ordering:
     * younger banked writes may have started before the final lane reports
     * the older broadcast done.  Keep the timing writeback as the sole VRF
     * owner, exactly like every other BitALU operation.
     */

    if (venusVinsResultDumpEnabled()) {
        std::string dumpDir =
            (venusDebugRoot() / "venusgem5_vins_result").string();
        if (activeTaskId >= 0)
            dumpDir += "/task_" + std::to_string(activeTaskId);
        this->recved_finished_instr_pkt->dumpVinsResult(
            m_venus_vrf,
            this->recved_finished_instr_pkt->vns_instr_id,
            dumpDir);
    }
    DPRINTF(VenusSequencerFull,
            "VINS result task=%d dump_idx=%d issue_id=%d op=%s "
            "scalar_op=0x%x vl=%d\n",
            activeTaskId, released_vins_dump_idx,
            this->recved_finished_instr_pkt->vns_instr_id,
            op_to_str(this->recved_finished_instr_pkt->op),
            this->recved_finished_instr_pkt->scalar_op,
            this->recved_finished_instr_pkt->vl);
    ++released_vins_dump_idx;
    this->recved_finished_instr_pkt->dumpVinsInfo();
    // VenusInstrPkt::saveVinsInfo();

    if (matched_idx >= 0) {
        if (rtlScalarBarrierLastAdvanceTick == curTick()) {
            /*
             * scalar600 has already sampled the old vinsn_running_q on this
             * tile edge.  In RTL the combinational pe_resp nevertheless
             * clears both sequencer registers through their non-blocking
             * assignments on that same edge.  Event ordering must not defer
             * that Q update to the next physical clock.
             */
            rtlRunningQ[matched_idx] = false;
            rtlPeRunningQ[matched_idx] = false;
            rtlIssuePending[matched_idx] = false;
            rtlCompletionPending[matched_idx] = false;
        } else {
            rtlCompletionPending[matched_idx] = true;
        }
    }
    venus_hazard_table->retired_vns_instr_id
        [this->recved_finished_instr_pkt->running_id] =
        this->recved_finished_instr_pkt->vns_instr_id;
    releaseRunningIDandremoveinstr(this->recved_finished_instr_pkt);
    updateHazardTable();
    if(!nextBroadcastHazardTablePipe1Event.scheduled())
        schedule(nextBroadcastHazardTablePipe1Event, afterCycles(Cycles(1)));
    maybeFinishTask();
    return true;
}

bool
VenusSequencer::rtlScalarBarrierBusy() const
{
    /*
     * venus_block_wrapper connects scalar600.venusidle_i exclusively to
     * venus_sequencer.venus_idle_o, which is the combinational OR reduction
     * of vinsn_running_q.  Dispatcher residency, issue-pending state and the
     * PE-running register are intentionally not visible on that wire.  Keep
     * this accessor at that exact registered boundary for every operation;
     * widening it to admission state makes the scalar barrier observe work
     * before RTL can and changes the result of its six-edge guard window.
     */
    for (int i = 0; i < NrIDs; ++i) {
        if (rtlRunningQ[i]) {
            DPRINTF(VenusSequencerFull,
                    "scalar barrier busy rid %d pe_q %d issue_pending %d "
                    "completion_pending %d active_instr %d\n",
                    i, rtlPeRunningQ[i], rtlIssuePending[i],
                    rtlCompletionPending[i],
                    vinsn_running_pkt[i] != nullptr ?
                        vinsn_running_pkt[i]->vns_instr_id : -1);
            return true;
        }
    }
    return false;
}

void
VenusSequencer::advanceRtlScalarBarrierState()
{
    /*
     * Mirror the relevant non-blocking updates in venus_sequencer.sv:
     *
     *   vinsn_running_q    <= OR(pe_vinsn_running_q_comb);
     *   pe_vinsn_running_q <= pe_vinsn_running_d;
     *
     * A pending issue updates the PE table on this edge and therefore cannot
     * become visible in vinsn_running_q until the next edge.  A final done
     * report is removed from both next-state expressions, while scalar600
     * has already sampled the prior Q state for this edge.
     */
    rtlScalarBarrierLastAdvanceTick = curTick();
    bool nextRunningQ[NrIDs];
    bool nextPeRunningQ[NrIDs];
    bool completed[NrIDs];
    for (int i = 0; i < NrIDs; ++i) {
        completed[i] = rtlCompletionPending[i];
        nextRunningQ[i] = rtlPeRunningQ[i] && !completed[i];
        nextPeRunningQ[i] =
            (rtlPeRunningQ[i] || rtlIssuePending[i]) && !completed[i];
    }
    for (int i = 0; i < NrIDs; ++i) {
        if (!rtlRunningQ[i] && nextRunningQ[i] &&
            vinsn_running_pkt[i] != nullptr &&
            vinsn_running_pkt[i]->vns_instr_log_firetick == 0) {
            /*
             * The RTL lifecycle monitor observes the rising edge of
             * pe_vinsn_running_o, before the later PE request handshake.
             */
            vinsn_running_pkt[i]->vns_instr_log_firetick = curTick();
        }
        rtlRunningQ[i] = nextRunningQ[i];
        rtlPeRunningQ[i] = nextPeRunningQ[i];
        rtlIssuePending[i] = false;
        rtlCompletionPending[i] = false;
    }
}

void
VenusSequencer::resetRtlScalarBarrierState()
{
    rtlScalarBarrierLastAdvanceTick = MaxTick;
    for (int i = 0; i < NrIDs; ++i) {
        rtlPeRunningQ[i] = false;
        rtlRunningQ[i] = false;
        rtlIssuePending[i] = false;
        rtlCompletionPending[i] = false;
    }
    lduAcceptAvailableTick = 0;
    stuAcceptAvailableTick = 0;
    lsuAddrgenAckVisibleTick = 0;
    lsuAddrgenAckInstr = -1;
    lsuAddrgenDirectionPopTick = 0;
    lsuAddrgenDirectionPopValid = false;
    lsuAddrgenAdmittedHeadInstr = -1;
    lsuChannelOwnerValid = false;
    pendingShuffleGrantCompletions.clear();
    if (nextShuffleGrantCompletionEvent.scheduled())
        deschedule(nextShuffleGrantCompletionEvent);
    pendingLaneGrantCompletions.clear();
    if (nextLaneGrantCompletionEvent.scheduled())
        deschedule(nextLaneGrantCompletionEvent);
    pendingProducerCompletions.clear();
    if (nextProducerCompletionEvent.scheduled())
        deschedule(nextProducerCompletionEvent);
    sequencerIssueState = SequencerIssueState::UpstreamReceive;
    sequencerWaitedForVfuReady = false;
    std::fill(std::begin(sequencerIssueTargetVfus),
              std::end(sequencerIssueTargetVfus), false);
    if (nextSequencerIssueStateEvent.scheduled())
        deschedule(nextSequencerIssueStateEvent);
}

void
VenusSequencer::resetRtlTaskState()
{
    if (!rtlTaskSoftResetEnabled)
        return;

    fatal_if(!isIdle(),
             "%s received tile soft reset with live Venus state: %s",
             name(), drainStatus());
    resetRtlScalarBarrierState();
    for (auto &laneDone : rtlLaneDone)
        laneDone.fill(false);
    if (venusShuffle != nullptr)
        venusShuffle->resetRtlTaskState();
    if (venusVrfXbar != nullptr)
        venusVrfXbar->resetVenusVrfArbitrationState();
}

bool
VenusSequencer::isIdle() const
{
    if (!scalarDispatchQueue.empty() || isBusy ||
        !pendingLsuInstrs.empty() || lduActiveLoads != 0 ||
        !lduRowsWaitingVisibility.empty() || !lduResultQueue.empty() ||
        !pendingLaneGrantCompletions.empty() ||
        !pendingProducerCompletions.empty() ||
        !pendingShuffleGrantCompletions.empty())
        return false;

    for (int i = 0; i < NrIDs; ++i) {
        if (vinsn_running_pkt[i] != nullptr)
            return false;
    }
    for (int i = 0; i < NrVFUs; ++i) {
        if (vfu_queue_counter[i] != 0)
            return false;
    }
    return true;
}

void
VenusSequencer::setActiveThreadContext(ThreadContext *tc, int task_id)
{
    fatal_if(tc != nullptr && task_id < 0,
             "active Venus DAG context requires a non-negative task id");
    activeThreadContext = tc;
    activeTaskId = tc ? task_id : -1;
    released_vins_dump_idx = 0;
}

std::string
VenusSequencer::drainStatus() const
{
    unsigned running = 0;
    std::ostringstream os;
    os << "\"sequencer_busy\":" << (isBusy ? "true" : "false")
       << ",\"pending_lsu\":" << pendingLsuInstrs.size();
    for (int i = 0; i < NrIDs; ++i) {
        if (vinsn_running_pkt[i] != nullptr)
            ++running;
    }
    os << ",\"running_vinsn\":" << running << ",\"vfu_queues\":[";
    for (int i = 0; i < NrVFUs; ++i) {
        if (i)
            os << ',';
        os << vfu_queue_counter[i];
    }
    os << "],\"running_detail\":[";
    bool first = true;
    for (int i = 0; i < NrIDs; ++i) {
        const auto *pkt = vinsn_running_pkt[i];
        if (pkt == nullptr)
            continue;
        if (!first)
            os << ',';
        first = false;
        const auto lanesIssued = std::count(
            vinsn_running_pkt_lanefiresuccessboard[i].begin(),
            vinsn_running_pkt_lanefiresuccessboard[i].end(), true);
        const auto lanesUsed = std::count(
            vinsn_running_pkt_laneuseboard[i].begin(),
            vinsn_running_pkt_laneuseboard[i].end(), true);
        const auto lanesActive = std::count(
            vinsn_running_pkt_laneactiveboard[i].begin(),
            vinsn_running_pkt_laneactiveboard[i].end(), true);
        os << "{\"id\":" << i
           << ",\"vinsn\":" << pkt->vns_instr_id
           << ",\"op\":\"" << op_to_str(pkt->op) << "\""
           << ",\"vl\":" << pkt->vl
           << ",\"lanes_participating\":" << lanesUsed
           << ",\"lanes_active\":" << lanesActive
           << ",\"lanes_issued\":" << lanesIssued
           << ",\"lanes_done\":"
           << vinsn_running_pkt_lanedonecounter[i] << '}';
    }
    os << ']';
    return os.str();
}

void
VenusSequencer::requestTaskExit()
{
    taskExitRequested = true;
    maybeFinishTask();
}

void
VenusSequencer::maybeFinishTask()
{
    if (taskExitRequested && isIdle())
        exitSimLoop("Venus task completed and vector pipeline drained");
}
void VenusSequencer::hazardTablePipe1() {
    if(venus_hazard_table_pipe1 != nullptr) delete venus_hazard_table_pipe1;
    venus_hazard_table_pipe1 = new VenusHazardTable(venus_hazard_table);
    if(!nextBroadcastHazardTablePipe2Event.scheduled())
        schedule(nextBroadcastHazardTablePipe2Event, afterCycles(Cycles(1)));
}
void VenusSequencer::hazardTablePipe2() {
    if(venus_hazard_table_pipe2 != nullptr) delete venus_hazard_table_pipe2;
    venus_hazard_table_pipe2 = new VenusHazardTable(venus_hazard_table_pipe1);
    if(!nextBroadcastHazardTablePipe3Event.scheduled())
        schedule(nextBroadcastHazardTablePipe3Event, afterCycles(Cycles(1)));
}
void VenusSequencer::hazardTablePipe3() {
    if(venus_hazard_table_pipe3 != nullptr) delete venus_hazard_table_pipe3;
    venus_hazard_table_pipe3 = new VenusHazardTable(venus_hazard_table_pipe2);
    broadcastHazardTable(venus_hazard_table_pipe3);
}

int VenusSequencer::findRunningID()
{
    for(int i = 0;i < NrIDs;i++)
        if(!rtlRunningIdReserved(i))
            return i;
    panic("VenusSequencer findRunningID called with no RTL-visible free ID");
}

bool
VenusSequencer::rtlRunningIdReserved(int id) const
{
    return vinsn_running_pkt[id] != nullptr || rtlPeRunningQ[id] ||
           rtlRunningQ[id] || rtlIssuePending[id] ||
           rtlCompletionPending[id];
}
void VenusSequencer::assignRunningIDandLaneandQueue(VenusInstrPkt* pkt, int ID)
{
    this->vinsn_running_pkt[ID]=pkt;
    this->vinsn_running_pkt[ID]->running_id=ID;
    rtlLaneDone[ID].fill(false);
    std::fill(std::begin(rtlVfuQueueReleased[ID]),
              std::end(rtlVfuQueueReleased[ID]), false);

    this->vinsn_running_pkt_laneuseboard[ID].clear();
    this->vinsn_running_pkt_laneuseboard[ID].resize(NrLanes,false);
    this->vinsn_running_pkt_laneactiveboard[ID].clear();
    this->vinsn_running_pkt_laneactiveboard[ID].resize(NrLanes,false);
    this->vinsn_running_pkt_lanefiresuccessboard[ID].clear();
    this->vinsn_running_pkt_lanefiresuccessboard[ID].resize(NrLanes,false);
    this->vinsn_running_pkt_shufflefiresuccessboard[ID] = false;

    if(this->vinsn_running_pkt[ID]->op == VSHUFFLE ||
       this->vinsn_running_pkt[ID]->op == VLOAD ||
       this->vinsn_running_pkt[ID]->op == VSTORE) {
        this->vinsn_running_pkt_lanedonecounter[ID] = NrLanes - 1;
    } else {
        const unsigned elementsPerLane =
            NrBankPerLane * (NrBitsPerBank / 8) /
            (this->vinsn_running_pkt[ID]->vew + 1);
        const unsigned activeLanes = std::min<unsigned>(
            NrLanes,
            std::ceil(static_cast<double>(
                this->vinsn_running_pkt[ID]->vl) / elementsPerLane));
        for (unsigned lane_id = 0; lane_id < activeLanes; ++lane_id)
            this->vinsn_running_pkt_laneactiveboard[ID][lane_id] = true;

        const bool reduction =
            this->vinsn_running_pkt[ID]->op >= VREDAND &&
            this->vinsn_running_pkt[ID]->op <= VREDSUM;
        if (rtlAllLaneIssueHandshake &&
            this->vinsn_running_pkt[ID]->vl != 0 && !reduction) {
            std::fill(
                this->vinsn_running_pkt_laneuseboard[ID].begin(),
                this->vinsn_running_pkt_laneuseboard[ID].end(), true);
            this->vinsn_running_pkt_lanedonecounter[ID] = 0;
        } else {
            for (unsigned lane_id = 0; lane_id < activeLanes; ++lane_id)
                this->vinsn_running_pkt_laneuseboard[ID][lane_id] = true;
            this->vinsn_running_pkt_lanedonecounter[ID] =
                NrLanes - activeLanes;
        }
    }
    if(this->vinsn_running_pkt[ID]->op>=VREDAND && this->vinsn_running_pkt[ID]->op<=VREDSUM) {
        this->vinsn_running_pkt_lanedonecounter[ID] = NrLanes - 1;
    }
    DPRINTF(VenusSequencer, "VenusSequencer Assigned RunningID %d to instr %d, vinsn_running_pkt_lanedonecounter:%d, lane use board:", ID, pkt->vns_instr_id, this->vinsn_running_pkt_lanedonecounter[ID]);
    if(::gem5::debug::VenusSequencer) {
        for(int lane_id = 0; lane_id < NrLanes; lane_id++)
        {
            printf("%d,", static_cast<int>(this->vinsn_running_pkt_laneuseboard[ID][lane_id]));
        }
        printf("\n");
    }
}
void VenusSequencer::releaseRunningIDandremoveinstr(VenusInstrPkt* pkt)
{
    // std::cout<<"at Tick = "<< curTick() << ", releasingRunningID and removeinstr, instr content:" << std::endl;
    // pkt->display();
    for(int i=0;i<NrIDs;i++) {
        if(this->vinsn_running_pkt[i] != nullptr)
        {
            if(this->vinsn_running_pkt[i]->running_id == pkt->running_id)
            {
                delete this->vinsn_running_pkt[i];
                this->vinsn_running_pkt[i]=nullptr;
                delete pkt;
                pkt=nullptr;
                return;
            }
        }
    }
}
std::tuple<VFU, VFU, VFU> VenusSequencer::findVinsnTargetVFU(VenusInstrPkt* pkt)
{
    VFU vfu1;
    VFU vfu2;
    VFU vfu_m;
    if((pkt->op >= VAND) && (pkt->op <= VABS)) {
        vfu1 = VFU_BitALU;
        vfu2 = VFU_NONE;
        vfu_m = (pkt->vm_r|pkt->vm_w)?VFU_Mask:VFU_NONE;
    }
    else if((pkt->op >= VADD) && (pkt->op <= VMAX)) {
        vfu1 = VFU_CAU;
        vfu2 = VFU_NONE;
        vfu_m = (pkt->vm_r|pkt->vm_w)?VFU_Mask:VFU_NONE;
    }
    else if((pkt->op >= VDIV) && (pkt->op <= VREMU)) {
        vfu1 = VFU_SerDiv;
        vfu2 = VFU_NONE;
        vfu_m = (pkt->vm_r|pkt->vm_w)?VFU_Mask:VFU_NONE;
    }
    else if((pkt->op >= VSHUFFLE) && (pkt->op <= VSHUFFLE)) {
        vfu1 = VFU_ShuffleUnit;
        vfu2 = VFU_NONE;
        vfu_m = VFU_NONE;
    }
    else if(pkt->op == VLOAD || pkt->op == VSTORE) {
        vfu1 = VFU_NONE;
        vfu2 = VFU_NONE;
        vfu_m = (pkt->vm_r|pkt->vm_w)?VFU_Mask:VFU_NONE;
    }
    else if((pkt->op >= VREDAND) && (pkt->op <= VREDSUM)) {
        vfu1 = VFU_BitALU;
        vfu2 = VFU_ShuffleUnit;
        vfu_m = (pkt->vm_r|pkt->vm_w)?VFU_Mask:VFU_NONE;
    }
    else {
        std::cout<<"Unknown OP:" << std::endl;
        pkt->display();
        panic("Unknown OP");
    }
    return std::make_tuple(vfu1, vfu2, vfu_m);
}
void VenusSequencer::assignTargetVFU(VenusInstrPkt* pkt, VFU vfu1, VFU vfu2, VFU vfu_m)
{
    if(vfu1 != VFU_NONE) {
        pkt->vfu_lst.push_back(vfu1);
        if (sequencerQueueAccountsVfu(pkt, vfu1))
            vfu_queue_counter[vfu1]++;
    }
    if(vfu2 != VFU_NONE) {
        pkt->vfu_lst.push_back(vfu2);
        if (sequencerQueueAccountsVfu(pkt, vfu2))
            vfu_queue_counter[vfu2]++;
    }
    if(vfu_m != VFU_NONE) {
        pkt->vfu_lst.push_back(vfu_m);
        if (sequencerQueueAccountsVfu(pkt, vfu_m))
            vfu_queue_counter[vfu_m]++;
    }
}
void
VenusSequencer::releaseVfuQueueOnce(const VenusInstrPkt *pkt, VFU vfu)
{
    if (pkt == nullptr || vfu == VFU_NONE ||
        !sequencerQueueAccountsVfu(pkt, vfu)) {
        return;
    }
    panic_if(pkt->running_id < 0 || pkt->running_id >= NrIDs,
             "invalid VFU release running ID %d", pkt->running_id);
    if (rtlVfuQueueReleased[pkt->running_id][vfu])
        return;
    panic_if(vfu_queue_counter[vfu] <= 0,
             "VFU queue underflow releasing %s for instr %d/rid %d",
             vfu_to_str(vfu), pkt->vns_instr_id, pkt->running_id);
    rtlVfuQueueReleased[pkt->running_id][vfu] = true;
    --vfu_queue_counter[vfu];
    DPRINTF(VenusSequencerFull,
            "releasing VFU queue at physical done: %s instr=%d rid=%d\n",
            vfu_to_str(vfu), pkt->vns_instr_id, pkt->running_id);
}

void VenusSequencer::releaseTargetVFU(const VenusInstrPkt *pkt,
                                      VFU vfu1, VFU vfu2, VFU vfu_m)
{
    releaseVfuQueueOnce(pkt, vfu1);
    releaseVfuQueueOnce(pkt, vfu2);
    releaseVfuQueueOnce(pkt, vfu_m);
}

bool
VenusSequencer::sequencerQueueAccountsVfu(
    const VenusInstrPkt *pkt, VFU vfu) const
{
    if (pkt == nullptr || vfu == VFU_NONE)
        return false;

    /*
     * Venus1 venus_sequencer.sv::target_vfus() routes a masked arithmetic
     * instruction to the global VFU_Mask counter instead of charging both
     * the arithmetic wrapper and mask counters.  The lane still receives
     * the real arithmetic target through pkt->vfu_lst; this helper governs
     * only the sequencer's admission/retirement accounting.  Venus2 keeps
     * its previously qualified accounting path.
     */
    if (!rtlRegisteredPeResponse)
        return true;
    const bool masked = pkt->vm_r || pkt->vm_w;
    if (vfu == VFU_Mask)
        return masked;
    if (vfu == VFU_ShuffleUnit)
        return true;
    return !masked;
}
bool VenusSequencer::checkVFUQueueisFull(VFU vfutmp)
{
    if (vfutmp == VFU_BitALU)
        return vfu_queue_counter[VFU_BitALU] >= BitaluInsnQueueDepth;
    if (vfutmp == VFU_CAU)
        return vfu_queue_counter[VFU_CAU] >= CauInsnQueueDepth;
    if (vfutmp == VFU_SerDiv)
        return vfu_queue_counter[VFU_SerDiv] >= SerdivInsnQueueDepth;
    if (vfutmp == VFU_ShuffleUnit)
        return vfu_queue_counter[VFU_ShuffleUnit] >= ShuffleInsnQueueDepth;
    if (vfutmp == VFU_Mask)
        return vfu_queue_counter[VFU_Mask] >=
               SequencerVmaskInsnQueueDepth;
}

bool VenusSequencer::checkVinsnQueueisFull()
{
    bool isFullTemp = true;
    for(int i = 0;i<NrIDs;i++) {
        isFullTemp &= rtlRunningIdReserved(i);
    }
    this->isFull = isFullTemp;
    return isFullTemp;
}

void
VenusSequencer::executeDispatchScalarReq()
{
    panic_if(scalarDispatchQueue.empty(),
             "scalar-dispatch event fired with an empty FIFO");
    panic_if(scalarDispatchReadyTicks.size() != scalarDispatchQueue.size(),
             "scalar-dispatch packet/readiness queues are out of sync");

    if (curTick() < scalarDispatchReadyTicks.front()) {
        schedule(nextDispatchScalarReqEvent,
                 scalarDispatchReadyTicks.front());
        return;
    }

    VenusInstrPkt *pkt = scalarDispatchQueue.front();
    if (!handleRequest(pkt)) {
        DPRINTF(VenusScalarDispatch,
                "scalar-dispatch retry op=%s tick=%llu ready=%llu "
                "depth=%llu issue_state=%d busy=%d\n",
                op_to_str(pkt->op),
                static_cast<unsigned long long>(curTick()),
                static_cast<unsigned long long>(
                    scalarDispatchReadyTicks.front()),
                static_cast<unsigned long long>(
                    scalarDispatchQueue.size()),
                static_cast<int>(sequencerIssueState), isBusy);
        schedule(nextDispatchScalarReqEvent, afterCycles(Cycles(1)));
        return;
    }

    DPRINTF(VenusScalarDispatch,
            "scalar-dispatch accepted op=%s instr=%d tick=%llu ready=%llu "
            "depth=%llu\n",
            op_to_str(pkt->op), recved_venus_instr_pkt->vns_instr_id,
            static_cast<unsigned long long>(curTick()),
                static_cast<unsigned long long>(
                    scalarDispatchReadyTicks.front()),
                static_cast<unsigned long long>(scalarDispatchQueue.size()));

    scalarDispatchQueue.pop_front();
    scalarDispatchReadyTicks.pop_front();
    delete pkt;
    if (!scalarDispatchQueue.empty()) {
        schedule(nextDispatchScalarReqEvent,
                 std::max(afterCycles(Cycles(1)),
                          scalarDispatchReadyTicks.front()));
    }
}

void VenusSequencer::executeRecvNewInstr()
{
    if(this->isBusy == true)
    {
        DPRINTF(VenusSequencer, "VenusSequencer has received an vns instr successfully, assignedID is %d. The content is:\n", this->recved_venus_instr_pkt->running_id);
        if(::gem5::debug::VenusSequencer) recved_venus_instr_pkt->display();
        recved_venus_instr_pkt->vns_instr_stat = INSTR_QUEUED;
        if (recved_venus_instr_pkt->vns_instr_log_firetick == 0)
            recved_venus_instr_pkt->vns_instr_log_firetick = curTick();

        if(!nextTryIssueInstrEvent.scheduled())
            schedule(nextTryIssueInstrEvent, curTick()); // send it immediately
        else
            panic("schedule(nextTryIssueInstrEvent, curTick() + 0*1000);");
    }
}

void
VenusSequencer::advanceSequencerIssueState()
{
    switch (sequencerIssueState) {
      case SequencerIssueState::Pipe0:
        sequencerIssueState = SequencerIssueState::Pipe1;
        break;
      case SequencerIssueState::Pipe1:
        sequencerIssueState = SequencerIssueState::Pipe2;
        break;
      case SequencerIssueState::Pipe2:
        sequencerIssueState = SequencerIssueState::DownstreamReceive;
        break;
      case SequencerIssueState::DownstreamReceive:
        /*
         * The RTL bus_done/ack_done registers advance only after every
         * selected PE has accepted the request.  Once they are both set,
         * venus_sequencer.sv still withholds upstream ready while any VFU
         * targeted by this just-issued instruction is full.  The stable
         * target is the registered current request, not the next scalar
         * request waiting at the dispatcher.
         */
        if (isBusy &&
            recved_venus_instr_pkt->op != VLOAD &&
            recved_venus_instr_pkt->op != VSTORE) {
            sequencerWaitedForPeReady = true;
        }
        if (!isBusy && curTick() >= lsuAddrgenAckVisibleTick) {
            /*
             * Both Venus generations withhold upstream ready while a VFU
             * selected by the current request reports its instruction queue
             * full.  Venus1 returns directly to UPSTREAM_RECEIVE once that
             * condition clears; Venus2 traverses the additional registered
             * DownstreamAck state.  ``rtlDirectDownstreamReturn`` selects
             * only that state transition, not whether target-queue ready is
             * observed.  Skipping the capacity check let younger commands
             * behind a depth-2 Shuffle queue enter unrelated VFUs about one
             * shuffle service interval early.
             */
            const bool returnReady = currentIssueVfuQueuesReady();
            if (returnReady) {
                if (sequencerWaitedForPeReady) {
                    /*
                     * pe_req_valid_o is held while the lane fall-through
                     * register is full.  On the edge where its downstream
                     * command moves and the held PE request is captured,
                     * RTL captures bus_done_q.  Keep the following registered
                     * upstream-return boundary explicit; consuming the next
                     * dispatcher head directly here makes an instruction
                     * whose lane input register is still occupied visible
                     * two clocks too early.
                     */
                    sequencerIssueState =
                        SequencerIssueState::ReturnUpstream;
                    break;
                }
                /*
                 * venus_sequencer.sv leaves WAITING_FOR_READY directly for
                 * UPSTREAM_RECEIVE once the selected VFU queues deassert
                 * full.  The ordinary DOWNSTREAM_RECEIVE path still needs
                 * the registered bus_done/ack_done boundary represented by
                 * DownstreamAck.  Do not make a transaction which really
                 * waited for a VFU-full condition traverse that ordinary
                 * registered stage as well.
                 */
                sequencerIssueState =
                    (sequencerWaitedForVfuReady ||
                     rtlDirectDownstreamReturn)
                    ? SequencerIssueState::ReturnUpstream
                    : SequencerIssueState::DownstreamAck;
            } else {
                sequencerWaitedForVfuReady = true;
            }
        }
        break;
      case SequencerIssueState::DownstreamAck:
        sequencerIssueState = SequencerIssueState::ReturnUpstream;
        break;
      case SequencerIssueState::ReturnUpstream:
        sequencerIssueState = SequencerIssueState::UpstreamReceive;
        /*
         * WAITING_FOR_READY asserts venus_req_ready_o combinationally on
         * the edge that returns upstream.  The scalar FIFO head is stable,
         * but its ordinary retry event may already have run earlier on this
         * gem5 tick and scheduled the next full cycle.  Retry it after this
         * state update so the handshake is neither a cycle early nor late.
         */
        if ((sequencerWaitedForVfuReady || sequencerWaitedForPeReady) &&
            !scalarDispatchQueue.empty() &&
            nextDispatchScalarReqEvent.scheduled() &&
            nextDispatchScalarReqEvent.when() > curTick()) {
            deschedule(nextDispatchScalarReqEvent);
            schedule(nextDispatchScalarReqEvent, curTick());
        }
        return;
      case SequencerIssueState::UpstreamReceive:
        panic("unexpected Venus sequencer issue-state event while ready");
    }
    schedule(nextSequencerIssueStateEvent, afterCycles(Cycles(1)));
}

bool
VenusSequencer::currentIssueVfuQueuesReady() const
{
    if (rtlLaneDesyncStall())
        return false;
    for (int vfu = 0; vfu < NrVFUs; ++vfu) {
        if (!sequencerIssueTargetVfus[vfu])
            continue;

        int depth = 0;
        switch (vfu) {
          case VFU_BitALU:
            depth = BitaluInsnQueueDepth;
            break;
          case VFU_CAU:
            depth = CauInsnQueueDepth;
            break;
          case VFU_SerDiv:
            depth = SerdivInsnQueueDepth;
            break;
          case VFU_ShuffleUnit:
            depth = ShuffleInsnQueueDepth;
            break;
          case VFU_Mask:
            depth = SequencerVmaskInsnQueueDepth;
            break;
          default:
            /* LDU/STU use the structured LSU admission queues. */
            continue;
        }
        if (vfu_queue_counter[vfu] >= depth)
            return false;
    }
    return true;
}

void VenusSequencer::executeTryIssueInstr()
{
    this->isBusy = false;

    // RTL accepts VL=0 as an empty vector operation and lets it retire
    // without waiting for lane work.  The old gem5 path selected zero lanes
    // but left the instruction resident forever because no lane could send a
    // completion response.
    const int runningId = this->recved_venus_instr_pkt->running_id;
    const bool anyLaneUsed = std::any_of(
        this->vinsn_running_pkt_laneactiveboard[runningId].begin(),
        this->vinsn_running_pkt_laneactiveboard[runningId].end(),
        [](bool used) { return used; });
    const bool laneOperation =
        this->recved_venus_instr_pkt->op != VSHUFFLE &&
        this->recved_venus_instr_pkt->op != VLOAD &&
        this->recved_venus_instr_pkt->op != VSTORE;
    if (this->recved_venus_instr_pkt->vl == 0 ||
        (laneOperation && !anyLaneUsed)) {
        /*
         * issue_valid is the edge that writes pe_vinsn_running_d in RTL.
         * Keep it distinct from the earlier dispatcher/sequencer admission.
         */
        auto *done = new VenusInstrPkt(this->recved_venus_instr_pkt);
        done->makeResponse();
        handleVinsnDoneReport(done);
        return;
    }

    if(this->recved_venus_instr_pkt->op == VLOAD ||
       this->recved_venus_instr_pkt->op == VSTORE) {
        if (!scheduleLsuDone(this->recved_venus_instr_pkt)) {
            if(!nextTryIssueInstrEvent.scheduled())
                schedule(nextTryIssueInstrEvent, afterCycles(Cycles(1)));
            this->isBusy = true;
            return;
        }
        /*
         * addrgen.sv first captures pe_req in IDLE, then asserts
         * addrgen_ack_o from ADDRGEN when the LDU/STU address request is
         * accepted.  venus_sequencer.sv captures that pulse in ack_done_q
         * before it may return upstream ready.  Keep all three registered
         * tile-clock boundaries here; the independently modeled AXI
         * request/response phases remain unchanged.
         */
        /*
         * Do not manufacture addrgen_ack from PE admission.  executeLsuDone
         * publishes it when this exact tagged request obtains the shared
         * address channel.  MaxTick keeps DOWNSTREAM_RECEIVE asserted while
         * an older opposite-direction LSU descriptor owns that channel.
         */
        lsuAddrgenAckInstr =
            this->recved_venus_instr_pkt->vns_instr_id;
        lsuAddrgenAckVisibleTick = MaxTick;
        DPRINTF(VenusSequencerFull,
                "LSU addrgen ack for instr %d/rid %d waits for tagged "
                "channel grant\n",
                recved_venus_instr_pkt->vns_instr_id,
                recved_venus_instr_pkt->running_id);
        return;
    }

    if(this->recved_venus_instr_pkt->op == VSHUFFLE) {
        if(this->vinsn_running_pkt_shufflefiresuccessboard[this->recved_venus_instr_pkt->running_id] == false)
        {
            if(port_venussequencer_sendto_venusshuffle.isBlocked == true) {
                DPRINTF(VenusSequencerFull, "VenusSequencer is trying to re-issue an instr to ShuffleUnit. The content is:\n");
                port_venussequencer_sendto_venusshuffle.recvReqRetry();
            } else {
                DPRINTF(VenusSequencerFull, "VenusSequencer is trying to issue an instr to ShuffleUnit. The content is:\n");
                port_venussequencer_sendto_venusshuffle.sendPacket(this->recved_venus_instr_pkt);
            }
            if(port_venussequencer_sendto_venusshuffle.isBlocked == true) {
                if(!nextTryIssueInstrEvent.scheduled())
                    schedule(nextTryIssueInstrEvent, afterCycles(Cycles(1))); // retry interval is 1
                DPRINTF(VenusSequencerFull, "VenusSequencer issue instr to ShuffleUnit complete. Get an nack.\n");
                this->isBusy = true;
            } else {
                this->vinsn_running_pkt_shufflefiresuccessboard[this->recved_venus_instr_pkt->running_id] = true;
            }
        }
    }
    else if(this->recved_venus_instr_pkt->op >= VREDAND && this->recved_venus_instr_pkt->op <= VREDSUM) {
        // if(this->vinsn_running_pkt_shufflefiresuccessboard[this->recved_venus_instr_pkt->running_id] == false)
        // {
        //     if(port_venussequencer_sendto_venusshuffle.isBlocked == true) {
        //         DPRINTF(VenusSequencerFull, "VenusSequencer is trying to re-issue an instr to ShuffleUnit. The content is:\n");
        //         port_venussequencer_sendto_venusshuffle.recvReqRetry();
        //     } else {
        //         DPRINTF(VenusSequencerFull, "VenusSequencer is trying to issue an instr to ShuffleUnit. The content is:\n");
        //         port_venussequencer_sendto_venusshuffle.sendPacket(this->recved_venus_instr_pkt);
        //     }
        //     if(port_venussequencer_sendto_venusshuffle.isBlocked == true) {
        //         if(!nextTryIssueInstrEvent.scheduled())
        //             schedule(nextTryIssueInstrEvent, curTick() + 1*1000); // retry interval is 1
        //         DPRINTF(VenusSequencerFull, "VenusSequencer issue instr to ShuffleUnit complete. Get an nack.\n");
        //         this->isBusy = true;
        //     } else {
        //         this->vinsn_running_pkt_shufflefiresuccessboard[this->recved_venus_instr_pkt->running_id] = true;
        //     }
        // }
        for(int lane_id = 0; lane_id < NrLanes; lane_id++)
        {
            if(this->vinsn_running_pkt_laneuseboard[this->recved_venus_instr_pkt->running_id][lane_id] == true)
            {
                /*
                 * RTL bus_done is an AND of the lanes' current ready signals,
                 * not a sticky reduction of earlier handshakes.  Broadcast
                 * to every participating lane on each retry cycle; each lane
                 * de-handshakes an already accepted generation internally.
                 */
                if(port_venussequencer_sendto_venuslane[lane_id].isBlocked == true) {
                    DPRINTF(VenusSequencerFull, "VenusSequencer is trying to re-issue an instr to Lane %d. The content is:\n", lane_id);
                    port_venussequencer_sendto_venuslane[lane_id].recvReqRetry();
                } else {
                    DPRINTF(VenusSequencerFull, "VenusSequencer is trying to issue an instr to Lane %d. The content is:\n", lane_id);
                    port_venussequencer_sendto_venuslane[lane_id].sendPacket(this->recved_venus_instr_pkt);
                }
                if(port_venussequencer_sendto_venuslane[lane_id].isBlocked == true) {
                    if(!nextTryIssueInstrEvent.scheduled())
                        schedule(nextTryIssueInstrEvent, afterCycles(Cycles(1))); // retry interval is 1
                    DPRINTF(VenusSequencerFull, "VenusSequencer issue instr to Lane %d complete. Get an nack.\n", lane_id);
                    this->isBusy = true;
                } else {
                    this->vinsn_running_pkt_lanefiresuccessboard[this->recved_venus_instr_pkt->running_id][lane_id] = true;
                }
            }
        }
    }
    else {
        for(int lane_id = 0; lane_id < NrLanes; lane_id++)
        {
            if(this->vinsn_running_pkt_laneuseboard[this->recved_venus_instr_pkt->running_id][lane_id] == true)
            {
                if(port_venussequencer_sendto_venuslane[lane_id].isBlocked == true) {
                    DPRINTF(VenusSequencerFull, "VenusSequencer is trying to re-issue an instr to Lane %d. The content is:\n", lane_id);
                    port_venussequencer_sendto_venuslane[lane_id].recvReqRetry();
                } else {
                    DPRINTF(VenusSequencerFull, "VenusSequencer is trying to issue an instr to Lane %d. The content is:\n", lane_id);
                    port_venussequencer_sendto_venuslane[lane_id].sendPacket(this->recved_venus_instr_pkt);
                }
                if(port_venussequencer_sendto_venuslane[lane_id].isBlocked == true) {
                    if(!nextTryIssueInstrEvent.scheduled())
                        schedule(nextTryIssueInstrEvent, afterCycles(Cycles(1))); // retry interval is 1
                    DPRINTF(VenusSequencerFull, "VenusSequencer issue instr to Lane %d complete. Get an nack.\n", lane_id);
                    this->isBusy = true;
                } else {
                    this->vinsn_running_pkt_lanefiresuccessboard[this->recved_venus_instr_pkt->running_id][lane_id] = true;
                }
            }
        }
    }

}

void VenusSequencer::updateHazardTable()
{
    //sort
    VenusInstrPkt* sorted_vinsn_running_pkt[NrIDs];
    int nrinstrrunning = 0;
    for(int i = 0; i < NrIDs; i++) {
        if(this->vinsn_running_pkt[i] != nullptr)
        {
            sorted_vinsn_running_pkt[nrinstrrunning++] = this->vinsn_running_pkt[i];
        }
    }
    std::sort(sorted_vinsn_running_pkt,sorted_vinsn_running_pkt+nrinstrrunning,VenusInstrPkt::cmp_vns_instr_id);
    // std::cout << "at tick = " << curTick() << ", VenusSequencer prints sorted_vinsn_running_pkt: ." << std::endl;
    // for(int i = 0; i < nrinstrrunning; i++) {
    //     sorted_vinsn_running_pkt[i]->display();
    // }
    DPRINTF(VenusSequencer, "at tick = %d, VenusSequencer Global Instructions (Active: %d):\n", curTick(), nrinstrrunning);
    for(int i = 0; i < nrinstrrunning; i++) {
        DPRINTF(VenusSequencer, "LHB 显示当前指令  [%d] instr_id: %d, running_id: %d, op: %s\n",
                i, sorted_vinsn_running_pkt[i]->vns_instr_id,
                sorted_vinsn_running_pkt[i]->running_id,
                op_to_str(sorted_vinsn_running_pkt[i]->op));
    }
    //init
    int lineused_write[NrLines], lineused_write_next[NrLines];
    int lineused_read[NrLines], lineused_read_next[NrLines];
    int maskused_write, maskused_write_next;
    uint16_t maskused_read_mask, maskused_read_mask_next;
    for(int i = 0; i < NrLines; i++) {
        lineused_write[i] = -1; lineused_write_next[i] = -1;
        lineused_read[i] = -1; lineused_read_next[i] = -1;
    }
    maskused_write = -1; maskused_write_next = -1;
    maskused_read_mask = 0;  maskused_read_mask_next = 0;
    for(int i = 0; i < NrIDs;i++)
    {
        venus_hazard_table->running_id_to_vns_instr_id[i] = -1;
        venus_hazard_table->is_shuffle[i] = false;
        for(int j = 0;j < NrIDs;j++)
        {
            venus_hazard_table->global_hazard_table[i][j] = false;
            venus_hazard_table->vs1_hazard_table[i][j] = false;
            venus_hazard_table->vs2_hazard_table[i][j] = false;
            venus_hazard_table->vd1_hazard_table[i][j] = false;
            venus_hazard_table->vd2_hazard_table[i][j] = false;
            venus_hazard_table->vm_hazard_table[i][j] = false;
            venus_hazard_table->raw_hazard_table[i][j] = false;
            venus_hazard_table->war_hazard_table[i][j] = false;
            venus_hazard_table->waw_hazard_table[i][j] = false;
        }
    }

    //calc
    for(int i = 0; i < nrinstrrunning; i++) {
        venus_hazard_table->running_id_to_vns_instr_id[sorted_vinsn_running_pkt[i]->running_id] = sorted_vinsn_running_pkt[i]->vns_instr_id;
        bool is_shuffle_unit_instr = (sorted_vinsn_running_pkt[i]->op == VSHUFFLE) ||
                                     (sorted_vinsn_running_pkt[i]->op >= VREDAND && sorted_vinsn_running_pkt[i]->op <= VREDSUM);
        venus_hazard_table->is_shuffle[sorted_vinsn_running_pkt[i]->running_id] = is_shuffle_unit_instr;
        // sorted_vinsn_running_pkt[i]->display();
        if(sorted_vinsn_running_pkt[i]->vm_r == true)
        {
            for (int m = 0; m < NrIDs; m++) {
                if(maskused_read_mask & (1 << m)) {//RAR
                    venus_hazard_table->global_hazard_table[sorted_vinsn_running_pkt[i]->running_id][m] = true;
                    venus_hazard_table->vm_hazard_table[sorted_vinsn_running_pkt[i]->running_id][m] = true;
                }
            }
            if(maskused_write != -1) {// RAW
                venus_hazard_table->global_hazard_table[sorted_vinsn_running_pkt[i]->running_id][maskused_write] = true;
                venus_hazard_table->vm_hazard_table[sorted_vinsn_running_pkt[i]->running_id][maskused_write] = true;
                venus_hazard_table->raw_hazard_table[sorted_vinsn_running_pkt[i]->running_id][maskused_write] = true;
            }
            maskused_read_mask_next |= (1 << sorted_vinsn_running_pkt[i]->running_id);
        }
        if(sorted_vinsn_running_pkt[i]->use_vs1 == true)
        {
            for(int j = sorted_vinsn_running_pkt[i]->vs1_head; j <= sorted_vinsn_running_pkt[i]->vs1_tail; j++)
            {
                if(lineused_read[j] != -1) {// RAR
                    venus_hazard_table->global_hazard_table[sorted_vinsn_running_pkt[i]->running_id][lineused_read[j]] = true;
                    venus_hazard_table->vs1_hazard_table[sorted_vinsn_running_pkt[i]->running_id][lineused_read[j]] = true;
                }
                if(lineused_write[j] != -1) {// RAW
                    venus_hazard_table->global_hazard_table[sorted_vinsn_running_pkt[i]->running_id][lineused_write[j]] = true;
                    venus_hazard_table->vs1_hazard_table[sorted_vinsn_running_pkt[i]->running_id][lineused_write[j]] = true;
                    venus_hazard_table->raw_hazard_table[sorted_vinsn_running_pkt[i]->running_id][lineused_write[j]] = true;
                }
                lineused_read_next[j] = sorted_vinsn_running_pkt[i]->running_id;
            }
        }
        if(sorted_vinsn_running_pkt[i]->use_vs2 == true)
        {
            for(int j = sorted_vinsn_running_pkt[i]->vs2_head; j <= sorted_vinsn_running_pkt[i]->vs2_tail; j++)
            {
                if(lineused_read[j] != -1) {// RAR
                    venus_hazard_table->global_hazard_table[sorted_vinsn_running_pkt[i]->running_id][lineused_read[j]] = true;
                    venus_hazard_table->vs2_hazard_table[sorted_vinsn_running_pkt[i]->running_id][lineused_read[j]] = true;
                }
                if(lineused_write[j] != -1) {// RAW
                    venus_hazard_table->global_hazard_table[sorted_vinsn_running_pkt[i]->running_id][lineused_write[j]] = true;
                    venus_hazard_table->vs2_hazard_table[sorted_vinsn_running_pkt[i]->running_id][lineused_write[j]] = true;
                    venus_hazard_table->raw_hazard_table[sorted_vinsn_running_pkt[i]->running_id][lineused_write[j]] = true;
                }
                lineused_read_next[j] = sorted_vinsn_running_pkt[i]->running_id;
            }
        }
        if(sorted_vinsn_running_pkt[i]->vm_w == true)
        {
            for (int m = 0; m < NrIDs; m++) {
                if(maskused_read_mask & (1 << m)) {//WAR
                    venus_hazard_table->global_hazard_table[sorted_vinsn_running_pkt[i]->running_id][m] = true;
                    venus_hazard_table->vd1_hazard_table[sorted_vinsn_running_pkt[i]->running_id][m] = true;
                    venus_hazard_table->war_hazard_table[sorted_vinsn_running_pkt[i]->running_id][m] = true;
                }
            }
            if(maskused_write != -1) {//WAW
                venus_hazard_table->global_hazard_table[sorted_vinsn_running_pkt[i]->running_id][maskused_write] = true;
                venus_hazard_table->vd1_hazard_table[sorted_vinsn_running_pkt[i]->running_id][maskused_write] = true;
                venus_hazard_table->waw_hazard_table[sorted_vinsn_running_pkt[i]->running_id][maskused_write] = true;
            }
            maskused_write_next = sorted_vinsn_running_pkt[i]->running_id;
            maskused_read_mask_next = 0;
        }
        if(sorted_vinsn_running_pkt[i]->vm_w == true)
        {
            for (int m = 0; m < NrIDs; m++) {
                if(maskused_read_mask & (1 << m)) {//WAR
                    venus_hazard_table->global_hazard_table[sorted_vinsn_running_pkt[i]->running_id][m] = true;
                    venus_hazard_table->vm_hazard_table[sorted_vinsn_running_pkt[i]->running_id][m] = true;
                    venus_hazard_table->war_hazard_table[sorted_vinsn_running_pkt[i]->running_id][m] = true;
                }
            }
            if(maskused_write != -1) {//WAW
                venus_hazard_table->global_hazard_table[sorted_vinsn_running_pkt[i]->running_id][maskused_write] = true;
                venus_hazard_table->vm_hazard_table[sorted_vinsn_running_pkt[i]->running_id][maskused_write] = true;
                venus_hazard_table->waw_hazard_table[sorted_vinsn_running_pkt[i]->running_id][maskused_write] = true;
            }
            maskused_write_next = sorted_vinsn_running_pkt[i]->running_id;
            maskused_read_mask_next = 0;
        }
        if(sorted_vinsn_running_pkt[i]->use_vd1 == true||sorted_vinsn_running_pkt[i]->use_vd1_op == true)
        {
            for(int j = sorted_vinsn_running_pkt[i]->vd1_head; j <= sorted_vinsn_running_pkt[i]->vd1_tail; j++)
            {
                if(lineused_read[j] != -1) {// WAR
                    venus_hazard_table->global_hazard_table[sorted_vinsn_running_pkt[i]->running_id][lineused_read[j]] = true;
                    venus_hazard_table->vd1_hazard_table[sorted_vinsn_running_pkt[i]->running_id][lineused_read[j]] = true;
                    venus_hazard_table->war_hazard_table[sorted_vinsn_running_pkt[i]->running_id][lineused_read[j]] = true;
                }
                if(lineused_write[j] != -1) {//WAW
                    venus_hazard_table->global_hazard_table[sorted_vinsn_running_pkt[i]->running_id][lineused_write[j]] = true;
                    venus_hazard_table->vd1_hazard_table[sorted_vinsn_running_pkt[i]->running_id][lineused_write[j]] = true;
                    venus_hazard_table->waw_hazard_table[sorted_vinsn_running_pkt[i]->running_id][lineused_write[j]] = true;
                }
                lineused_write_next[j] = sorted_vinsn_running_pkt[i]->running_id;
            }
        }
        if(sorted_vinsn_running_pkt[i]->op>=VCMXMUL && sorted_vinsn_running_pkt[i]->op<=VCMXMUL && (sorted_vinsn_running_pkt[i]->use_vd2 == true||sorted_vinsn_running_pkt[i]->use_vd2_op == true))
        {
            for(int j = sorted_vinsn_running_pkt[i]->vd2_head; j <= sorted_vinsn_running_pkt[i]->vd2_tail; j++)
            {
                if(lineused_read[j] != -1) {// WAR
                    venus_hazard_table->global_hazard_table[sorted_vinsn_running_pkt[i]->running_id][lineused_read[j]] = true;
                    venus_hazard_table->vd2_hazard_table[sorted_vinsn_running_pkt[i]->running_id][lineused_read[j]] = true;
                    venus_hazard_table->war_hazard_table[sorted_vinsn_running_pkt[i]->running_id][lineused_read[j]] = true;
                }
                if(lineused_write[j] != -1) {//WAW
                    venus_hazard_table->global_hazard_table[sorted_vinsn_running_pkt[i]->running_id][lineused_write[j]] = true;
                    venus_hazard_table->vd2_hazard_table[sorted_vinsn_running_pkt[i]->running_id][lineused_write[j]] = true;
                    venus_hazard_table->waw_hazard_table[sorted_vinsn_running_pkt[i]->running_id][lineused_write[j]] = true;
                }
                lineused_write_next[j] = sorted_vinsn_running_pkt[i]->running_id;
            }
        }


        for(int j = 0; j < NrLines; j++)
        {
            lineused_read[j] = lineused_read_next[j];
            lineused_write[j] = lineused_write_next[j];
        }
        maskused_read_mask = maskused_read_mask_next;
        maskused_write = maskused_write_next;


        // std::cout << std::endl;
        // std::cout << "lineused_read::::::::==================" << std::endl;
        // for(int i=0;i<NrLines;i++)
        // {
        //     if(i%16==0)
        //     {
        //         std::cout << std::endl << i << "\t\t:\t";
        //     }
        //     std::cout<<"\t"<<lineused_read[i]<<",";
        // }
        // std::cout << std::endl;
        // std::cout << "lineused_write::::::::==================" << std::endl;
        // for(int i=0;i<NrLines;i++)
        // {
        //     if(i%16==0)
        //     {
        //         std::cout << std::endl << i << "\t\t:\t";
        //     }
        //     std::cout<<"\t"<<lineused_write[i]<<",";
        // }
        // std::cout << std::endl;
        // std::cout << "global_hazard_table_o::::::::==================" << std::endl;
        // for(int i=0;i<NrIDs;i++)
        // {
        //     for(int j=0;j<NrIDs;j++)
        //     {
        //         std::cout << "\t" << venus_hazard_table->global_hazard_table[i][j] << ",";
        //     }
        //     std::cout << std::endl;
        // }

    }

    // if(nextBroadcastHazardTableEvent.scheduled())
    //     deschedule(nextBroadcastHazardTableEvent);
    // schedule(nextBroadcastHazardTableEvent, curTick() + 2*1000);

}
void VenusSequencer::broadcastHazardTable(VenusHazardTable* venus_hazard_table)
{
    for(int i=0;i<NrLanes;i++)
    {
        port_venussequencer_hazardtable_boardcast[i].sendPacket((PacketPtr)venus_hazard_table);
    }
    port_venussequencer_hazardtable_boardcast_to_shuffle.sendPacket((PacketPtr)venus_hazard_table);
    DPRINTF(VenusSequencer,"VenusSequencer is boardcasting a new HazardTable. The content is:\n");
    if (::gem5::debug::VenusSequencer) {
        venus_hazard_table->display();
    }
}

void
VenusSequencer::forwardLsuCompletionToLanes(const VenusInstrPkt *pkt)
{
    panic_if(pkt == nullptr || (pkt->op != VLOAD && pkt->op != VSTORE),
             "Only an LSU instruction can use the LSU completion sideband");

    VenusHazardTable completion;
    completion.lsu_completion_valid = true;
    completion.lsu_completion_running_id = pkt->running_id;
    completion.lsu_completion_vns_instr_id = pkt->vns_instr_id;

    /* Lane requesters sample this sideband at the established sequencer
     * recycle boundary.  Do not pull that already-aligned boundary forward
     * just because the shuffle requester consumes LDU pe_resp directly. */
    for (int lane = 0; lane < NrLanes; ++lane) {
        port_venussequencer_hazardtable_boardcast[lane].sendPacket(
            static_cast<PacketPtr>(&completion));
    }
}

void
VenusSequencer::forwardLsuCompletionToShuffle(const VenusInstrPkt *pkt)
{
    panic_if(pkt == nullptr || pkt->op != VLOAD,
             "Only an LDU instruction can use the early shuffle completion sideband");

    VenusHazardTable completion;
    completion.lsu_completion_valid = true;
    completion.lsu_completion_running_id = pkt->running_id;
    completion.lsu_completion_vns_instr_id = pkt->vns_instr_id;

    /* venus_shuffle_engine consumes global_hazard_table_i directly.  The
     * sequencer masks the completing LDU generation on pe_resp, before the
     * later lifecycle/recycle boundary observed by lane requesters. */
    port_venussequencer_hazardtable_boardcast_to_shuffle.sendPacket(
        static_cast<PacketPtr>(&completion));
}

void
VenusSequencer::forwardProducerCompletionToLanes(const VenusInstrPkt *pkt)
{
    panic_if(pkt == nullptr || pkt->op != VSHUFFLE,
             "Only a non-lane shuffle producer can use this completion sideband");

    VenusHazardTable completion;
    completion.producer_completion_valid = true;
    completion.producer_completion_running_id = pkt->running_id;
    completion.producer_completion_vns_instr_id = pkt->vns_instr_id;

    /* global_hazard_table_d in RTL is masked by vinsn_running_d in the
     * producer response cycle.  Keep the registered table contents intact,
     * but tag this exact generation as no longer contributing to sequencer-
     * local consumers such as the LDU result requester. */
    venus_hazard_table->retired_vns_instr_id[pkt->running_id] =
        std::max(venus_hazard_table->retired_vns_instr_id[pkt->running_id],
                 static_cast<int>(pkt->vns_instr_id));

    for (int lane = 0; lane < NrLanes; ++lane) {
        port_venussequencer_hazardtable_boardcast[lane].sendPacket(
            static_cast<PacketPtr>(&completion));
    }
    port_venussequencer_hazardtable_boardcast_to_shuffle.sendPacket(
        static_cast<PacketPtr>(&completion));
}

}
