#include "VenusLane.hh"
#include "venus_instr_pkt.hh"

#include <iostream>
#include <algorithm>

namespace gem5
{


Port & VenusLane::getPort(const std::string &if_name, PortID idx)
{
    // 检查请求的端口名称是否与 Python 文件中定义的 "port_venuslane_receivefrom_venussequencer" 匹配
    if (if_name == "port_venuslane_receivefrom_venussequencer") {
        return port_venuslane_receivefrom_venussequencer; // 返回对应的端口对象
    }
    if (if_name == "port_venuslane_hazardtable_listen") {
        return port_venuslane_hazardtable_listen; // 返回对应的端口对象
    }
    if (if_name == "port_venuslane_receivefrom_venusshuffle")
    {
        return port_venuslane_receivefrom_venusshuffle; // 返回对应的端口对象
    }
    if (if_name == "port_venuslanetovrf_1")
    {
        return VenusLaneRequestPorts[0]; // 返回对应的端口对象
    }
    if (if_name == "port_venuslanetovrf_2")
    {
        return VenusLaneRequestPorts[1]; // 返回对应的端口对象
    }
    if (if_name == "port_venuslanetovrf_3")
    {
        return VenusLaneRequestPorts[2]; // 返回对应的端口对象
    }
    if (if_name == "port_venuslanetovrf_4")
    {
        return VenusLaneRequestPorts[3]; // 返回对应的端口对象
    }
    if (if_name == "port_venuslanetovrf_5")
    {
        return VenusLaneRequestPorts[4]; // 返回对应的端口对象
    }
    if (if_name == "port_venuslanetovrf_6")
    {
        return VenusLaneRequestPorts[5]; // 返回对应的端口对象
    }
    if (if_name == "port_venuslanetovrf_7")
    {
        return VenusLaneRequestPorts[6]; // 返回对应的端口对象
    }
    if (if_name == "port_venuslanetovrf_8")
    {
        return VenusLaneRequestPorts[7]; // 返回对应的端口对象
    }
    if (if_name == "port_venuslanetovrf_9")
    {
        return VenusLaneRequestPorts[8]; // 返回对应的端口对象
    }
    if (if_name == "port_venuslanetovrf_10")
    {
        return VenusLaneRequestPorts[9]; // 返回对应的端口对象
    }
    if (if_name == "port_venuslanetovrf_11")
    {
        return VenusLaneRequestPorts[10]; // 返回对应的端口对象
    }
    if (if_name == "port_venuslanetovrf_12")
    {
        return VenusLaneRequestPorts[11]; // 返回对应的端口对象
    }
    if (if_name == "port_venuslanetovrf_13")
    {
        return VenusLaneRequestPorts[12]; // 返回对应的端口对象
    }
    if (if_name == "port_venuslanetovrf_14")
    {
        return VenusLaneRequestPorts[13]; // 返回对应的端口对象
    }
    if (if_name == "port_venuslanetovrf_15")
    {
        return VenusLaneRequestPorts[14]; // 返回对应的端口对象
    }
    if (if_name == "port_venuslanetovrf_16")
    {
        return VenusLaneRequestPorts[15]; // 返回对应的端口对象
    }
    // 如果名称不匹配，则调用基类方法（可能会报错，但符合框架规范）
    return ClockedObject::getPort(if_name, idx);
}


bool VenusLane::VenusLaneSequencerSidePort::recvTimingReq(PacketPtr pkt)
{

    // DPRINTF(LaneSequencer,"VenusLaneSequencerSidePort received some info.\n");
    if (!owner->LanehandleNewInstrRequest((VenusInstrPkt*)pkt)) {
        // needRetry = true;
        // DPRINTF(LaneSequencer,"VenusLaneSequencerSidePort reports an nack.\n");
        return false;
    } else {
        // DPRINTF(LaneSequencer,"VenusLaneSequencerSidePort reports an ack.\n");
        return true;
    }
}

void
VenusLane::VenusLaneSequencerSidePort::reportProducerGrantCompletion(
    const VenusInstrPkt *pkt)
{
    panic_if(pkt == nullptr, "cannot report a null producer grant");
    auto *sink = dynamic_cast<VenusLaneProducerCompletionSink *>(&getPeer());
    fatal_if(sink == nullptr,
             "lane sequencer peer has no producer-completion sideband");
    sink->noteVenusLaneProducerGrant(
        owner->lane_id, pkt->running_id, pkt->vns_instr_id);
}
bool VenusLane::VenusLaneShuffleSidePort::recvTimingReq(PacketPtr pkt)
{

    // DPRINTF(ShuffleEngine,"VenusLaneShuffleSidePort received some info.\n");
    if (!owner->transparentShuffleVFURequest(pkt)) {
        // needRetry = true;
        // DPRINTF(ShuffleEngine,"VenusLaneShuffleSidePort reports an nack.\n");
        return false;
    } else {
        // DPRINTF(ShuffleEngine,"VenusLaneShuffleSidePort reports an ack.\n");
        return true;
    }
}
void
VenusLane::VenusLaneShuffleSidePort::publishVenusVrfLiveIntent(PacketPtr pkt)
{
    owner->publishShuffleVrfLiveIntent(pkt);
}

void
VenusLane::VenusLaneShuffleSidePort::withdrawVenusVrfLiveIntent(PacketPtr pkt)
{
    owner->withdrawShuffleVrfLiveIntent();
}

void
VenusLane::VenusLaneShuffleSidePort::grantLiveIntent(PacketPtr pkt)
{
    auto *source = dynamic_cast<VenusVrfLiveIntentSource *>(&getPeer());
    fatal_if(source == nullptr,
             "lane Shuffle peer has no live-intent source");
    source->grantVenusVrfLiveIntent(pkt);
}

void
VenusLane::VenusLaneToVrfRequestPort::publishLiveIntent(PacketPtr pkt)
{
    auto *sink = dynamic_cast<VenusVrfLiveIntentSink *>(&getPeer());
    fatal_if(sink == nullptr,
             "VRF port %d peer has no live-intent sink", portid);
    liveIntentPacket = pkt;
    sink->publishVenusVrfLiveIntent(pkt);
}

void
VenusLane::VenusLaneToVrfRequestPort::withdrawLiveIntent()
{
    if (liveIntentPacket == nullptr)
        return;
    auto *sink = dynamic_cast<VenusVrfLiveIntentSink *>(&getPeer());
    fatal_if(sink == nullptr,
             "VRF port %d peer has no live-intent sink", portid);
    sink->withdrawVenusVrfLiveIntent(liveIntentPacket);
    liveIntentPacket = nullptr;
}

void
VenusLane::VenusLaneToVrfRequestPort::grantVenusVrfLiveIntent(PacketPtr pkt)
{
    fatal_if(liveIntentPacket != pkt,
             "VRF port %d granted stale live intent", portid);
    liveIntentPacket = nullptr;
    owner->handleShuffleVrfLiveGrant(portid, pkt);
}

void
VenusLane::publishShuffleVrfLiveIntent(PacketPtr pkt)
{
    fatal_if(pkt == nullptr || (pkt->id != 9 && pkt->id != 16),
             "invalid Shuffle live intent packet");
    const int selectedPort = pkt->id == 9 ? 8 : 15;
    const int otherPort = pkt->id == 9 ? 15 : 8;
    VenusLaneRequestPorts[otherPort].withdrawLiveIntent();
    VenusLaneRequestPorts[selectedPort].publishLiveIntent(pkt);
}

void
VenusLane::withdrawShuffleVrfLiveIntent()
{
    VenusLaneRequestPorts[8].withdrawLiveIntent();
    VenusLaneRequestPorts[15].withdrawLiveIntent();
}

void
VenusLane::updateVectorWriterBoundary(int running_id)
{
    panic_if(running_id < 0 || running_id >= NrIDs,
             "invalid vector writer running ID %d", running_id);
    if (locallane_writer_boundary_tick[running_id] == MaxTick ||
        curTick() < locallane_writer_boundary_tick[running_id]) {
        return;
    }

    locallane_writer_grant_count_q[running_id] =
        locallane_writer_grant_count_d[running_id];
    locallane_writer_vfu_q[running_id] =
        locallane_writer_vfu_d[running_id];
    locallane_writer_vaddr_q[running_id] =
        locallane_writer_vaddr_d[running_id];
    locallane_writer_valid_q[running_id] =
        locallane_writer_valid_d[running_id];
    locallane_writer_q_tick[running_id] =
        locallane_writer_boundary_tick[running_id];
    locallane_writer_boundary_tick[running_id] = MaxTick;
}

void
VenusLane::noteVectorWriterGrant(
    int running_id, VFU writer_vfu, Addr writer_vaddr)
{
    panic_if(running_id < 0 || running_id >= NrIDs,
             "invalid vector writer grant running ID %d", running_id);

    /*
     * venus_operand_requester registers vinsn_writeback_d together with the
     * writer VFU and address.  Commit an older D value before installing the
     * grant from this edge, then expose this grant to requester_q exactly one
     * lane clock later.  The registers intentionally survive running-ID
     * reuse, as the RTL registers do.
     */
    updateVectorWriterBoundary(running_id);
    ++locallane_writer_grant_count_d[running_id];
    locallane_writer_vfu_d[running_id] = writer_vfu;
    locallane_writer_vaddr_d[running_id] = writer_vaddr;
    locallane_writer_valid_d[running_id] = true;
    locallane_writer_boundary_tick[running_id] =
        curTick() + clockPeriod();
    DPRINTF(LaneOperandRequester,
            "vector writer grant d rid %d vfu %d addr %llu count %llu "
            "visible %llu\n",
            running_id, writer_vfu,
            static_cast<unsigned long long>(writer_vaddr),
            static_cast<unsigned long long>(
                locallane_writer_grant_count_d[running_id]),
            static_cast<unsigned long long>(
                locallane_writer_boundary_tick[running_id]));
}

Addr
VenusLane::shuffleWriterVaddr(PacketPtr pkt) const
{
    panic_if(pkt == nullptr, "Shuffle writer grant has no packet");
    const Addr laneSpan = bank_num * line_num * sizeof(uint16_t);
    const Addr bankSpan = line_num * sizeof(uint16_t);
    const Addr laneBase = vrf_base_addr + lane_id * laneSpan;
    panic_if(pkt->getAddr() < laneBase ||
                 pkt->getAddr() >= laneBase + laneSpan,
             "Shuffle writer address %#llx is outside lane %d VRF window",
             static_cast<unsigned long long>(pkt->getAddr()), lane_id);
    const Addr laneOffset = pkt->getAddr() - laneBase;
    const unsigned physicalBank = laneOffset / bankSpan;
    const unsigned row = (laneOffset % bankSpan) / sizeof(uint16_t);
    const unsigned logicalBank =
        (physicalBank + bank_num - (row % bank_num)) % bank_num;
    return row * bank_num + logicalBank;
}

void
VenusLane::handleShuffleVrfLiveGrant(int portid, PacketPtr pkt)
{
    fatal_if(portid != 8 && portid != 15,
             "Shuffle live grant arrived on VRF port %d", portid);
    /* RTL currently hard-wires shuffle_result_id_o to zero.  Consequently
     * every Shuffle write grant updates writer slot zero even when the
     * executing Shuffle instruction owns another running ID. */
    if (pkt->id == 9 || pkt->id == 16) {
        noteVectorWriterGrant(
            0, VFU_ShuffleUnit, shuffleWriterVaddr(pkt));
    }
    port_venuslane_receivefrom_venusshuffle.grantLiveIntent(pkt);
}
bool VenusLane::transparentShuffleVFURequest(PacketPtr pkt)
{
    if(pkt->id == 9) { // ShuffleRead use passage 9
        const bool granted = VenusLaneRequestPorts[8].sendTimingReq(pkt);
        if (granted)
            noteVectorWriterGrant(
                0, VFU_ShuffleUnit, shuffleWriterVaddr(pkt));
        return granted;
    } else if(pkt->id == 16) { // ShuffleWrite use passage 16
        const bool granted = VenusLaneRequestPorts[15].sendTimingReq(pkt);
        if (granted)
            noteVectorWriterGrant(
                0, VFU_ShuffleUnit, shuffleWriterVaddr(pkt));
        return granted;
    }
    else
        panic("illegal packet id received from ShuffleUnit, ShuffleUnit is trying to occupy operand passage %d", pkt->id);
}
bool VenusLane::transparentVFUResponsetoShuffle(PacketPtr pkt)
{
    // pkt->makeResponse();
    return port_venuslane_receivefrom_venusshuffle.sendTimingResp(pkt);
}

bool
VenusLane::VenusLaneToVrfRequestPort::sendPacket(PacketPtr pkt)
{
    if (!owner->experimentalVrfRr)
        return sendTimingReq(pkt);

    // The RTL requester keeps one stable request payload asserted until the
    // bank grant.  A second transient packet must never replace that owner.
    if (blockedPacket != nullptr)
        return false;

    if (sendTimingReq(pkt))
        return true;

    blockedPacket = pkt;
    return false;
}

bool
VenusLane::VenusLaneToVrfRequestPort::consumeDeferredWriteGrant(
    Addr addr, unsigned size, int running_id, int vns_instr_id)
{
    if (deferredWriteGrant == nullptr)
        return false;

    auto *state = dynamic_cast<VrfWriteCommitState *>(
        deferredWriteGrant->senderState);
    panic_if(state == nullptr,
             "VRF port %d deferred grant is not a tagged write", portid);
    panic_if(deferredWriteGrant->getAddr() != addr ||
                 deferredWriteGrant->getSize() != size ||
                 state->running_id != running_id ||
                 state->vns_instr_id != vns_instr_id,
             "VRF port %d deferred write grant mismatch: packet "
             "instr %d/rid %d addr %#x size %u, requester "
             "instr %d/rid %d addr %#x size %u",
             portid, state->vns_instr_id, state->running_id,
             deferredWriteGrant->getAddr(), deferredWriteGrant->getSize(),
             vns_instr_id, running_id, addr, size);
    deferredWriteGrant = nullptr;
    return true;
}

void
VenusLane::VenusLaneToVrfRequestPort::recvReqRetry()
{
    if (liveIntentPacket != nullptr) {
        PacketPtr pkt = liveIntentPacket;
        if (sendTimingReq(pkt))
            grantVenusVrfLiveIntent(pkt);
        return;
    }
    if (owner->experimentalVrfRr && blockedPacket != nullptr) {
        PacketPtr pkt = blockedPacket;
        if (sendTimingReq(pkt)) {
            blockedPacket = nullptr;
            if (dynamic_cast<VrfWriteCommitState *>(pkt->senderState))
                deferredWriteGrant = pkt;
            owner->handleLaneToVrfGrant(portid, pkt);
        }
        return;
    }

    owner->handleLaneToVrfRetry(portid);
}

void
VenusLane::handleLaneToVrfRetry(int portid)
{
    // R52 only forwards retry for the two shuffle bridge ports.  The
    // all-requester retry path belongs to the deferred per-bank RR prototype
    // and must not alter the promoted default model.
    if (!experimentalVrfRr) {
        if (portid == 8 || portid == 15)
            port_venuslane_receivefrom_venusshuffle.sendRetryReq();
        return;
    }

    const auto retryRead = [this](
        EventFunctionWrapper& event, OPERANDTYPE operand) {
        if (event.scheduled())
            deschedule(event);
        operandRequesterGetVectorData(operand);
    };

    switch (portid) {
      case 0:
        retryRead(nextOperandRequesterGetsBitAlu_A_DataEvent, BitAlu_A);
        break;
      case 1:
        retryRead(nextOperandRequesterGetsBitAlu_B_DataEvent, BitAlu_B);
        break;
      case 2:
        retryRead(nextOperandRequesterGetsCAU_A_DataEvent, CAU_A);
        break;
      case 3:
        retryRead(nextOperandRequesterGetsCAU_B_DataEvent, CAU_B);
        break;
      case 4:
        retryRead(nextOperandRequesterGetsCAU_C_DataEvent, CAU_C);
        break;
      case 5:
        retryRead(nextOperandRequesterGetsCAU_D_DataEvent, CAU_D);
        break;
      case 6:
        retryRead(nextOperandRequesterGetsSerDiv_A_DataEvent, SerDiv_A);
        break;
      case 7:
        retryRead(nextOperandRequesterGetsSerDiv_B_DataEvent, SerDiv_B);
        break;
      case 8:
      case 15:
        port_venuslane_receivefrom_venusshuffle.sendRetryReq();
        break;
      case 9:
        retryRead(nextOperandRequesterGetsMask_DataEvent, Mask);
        break;
      case 10:
      case 11:
        serviceCauResultQueue();
        break;
      case 12:
      case 13:
        serviceBitAluResultQueue();
        break;
      case 14:
        serviceSerDivResultQueue();
        break;
      default:
        panic("Unexpected Venus lane VRF retry port %d", portid);
    }
}

bool
VenusLane::producerCompletionPending(int runningId, int instrId) const
{
    const auto matches = [runningId, instrId](const VenusInstrPkt *pkt) {
        return pkt != nullptr && pkt->running_id == runningId &&
            pkt->vns_instr_id == instrId;
    };
    return matches(bitalu_doneinstr_pkt) || matches(cau_doneinstr_pkt) ||
        matches(serdiv_doneinstr_pkt) || matches(tshuffle_doneinstr_pkt);
}

unsigned
VenusLane::operandQueueDataDepth(OPERANDTYPE operand) const
{
    switch (operand) {
      case BitAlu_A:
      case BitAlu_B:
        return BitaluDataQueueDepth;
      case CAU_A:
      case CAU_B:
      case CAU_C:
      case CAU_D:
        return CauDataQueueDepth;
      case SerDiv_A:
      case SerDiv_B:
        return SerdivDataQueueDepth;
      case Mask:
        return VmaskDataQueueDepth;
      case ShuffleUnit:
        return ShuffleDataQueueDepth;
      default:
        panic("unknown operand queue type %d", operand);
    }
}

void
VenusLane::updateOperandQueueUsageBoundary(OPERANDTYPE operand)
{
    const unsigned index = static_cast<unsigned>(operand);
    panic_if(index >= operandQueueUsageQ.size(),
             "operand queue usage has invalid type %d", operand);
    if (operandQueueUsageTick[index] == curTick())
        return;

    const int next = static_cast<int>(operandQueueUsageQ[index]) +
        operandQueueUsageDelta[index];
    panic_if(next < 0 || next > static_cast<int>(operandQueueDataDepth(operand)),
             "operand queue type %d registered usage transition %u %+d is "
             "outside [0,%u] at tick %llu", operand,
             operandQueueUsageQ[index], operandQueueUsageDelta[index],
             operandQueueDataDepth(operand), curTick());
    operandQueueUsageQ[index] = static_cast<unsigned>(next);
    operandQueueUsageSnapshot[index] = operandQueueUsageQ[index];
    operandQueueUsageDelta[index] = 0;
    operandQueueUsageTick[index] = curTick();
}

bool
VenusLane::operandQueueReadyForIssue(OPERANDTYPE operand)
{
    updateOperandQueueUsageBoundary(operand);
    return operandQueueUsageSnapshot[static_cast<unsigned>(operand)] !=
        operandQueueDataDepth(operand);
}

void
VenusLane::noteOperandQueueIssue(OPERANDTYPE operand)
{
    updateOperandQueueUsageBoundary(operand);
    const unsigned index = static_cast<unsigned>(operand);
    panic_if(operandQueueUsageSnapshot[index] == operandQueueDataDepth(operand),
             "operand queue type %d issued while registered full", operand);
    ++operandQueueUsageDelta[index];
    DPRINTF(LaneOperandRequester,
            "operand queue usage type %d issued: q %u delta %+d depth %u\n",
            operand, operandQueueUsageSnapshot[index],
            operandQueueUsageDelta[index], operandQueueDataDepth(operand));
}

void
VenusLane::noteOperandQueuePop(OPERANDTYPE operand)
{
    updateOperandQueueUsageBoundary(operand);
    const unsigned index = static_cast<unsigned>(operand);
    --operandQueueUsageDelta[index];
    DPRINTF(LaneOperandRequester,
            "operand queue usage type %d popped: q %u delta %+d depth %u\n",
            operand, operandQueueUsageSnapshot[index],
            operandQueueUsageDelta[index], operandQueueDataDepth(operand));
}

void
VenusLane::noteOperandQueueUnpop(OPERANDTYPE operand)
{
    updateOperandQueueUsageBoundary(operand);
    const unsigned index = static_cast<unsigned>(operand);
    ++operandQueueUsageDelta[index];
    DPRINTF(LaneOperandRequester,
            "operand queue usage type %d restored: q %u delta %+d depth %u\n",
            operand, operandQueueUsageSnapshot[index],
            operandQueueUsageDelta[index], operandQueueDataDepth(operand));
}

bool
VenusLane::deferOperandQueuePopToBitAluAdmission(
    OPERANDTYPE operand, const VenusInstrPkt *instr) const
{
    /* BitALU A/B usage is committed by the common arithmetic-handshake
     * transition, after all required tagged FIFO-Q operands are present.
     * Mask keeps its separate latch/result-boundary contract for now. */
    if (operand == BitAlu_A || operand == BitAlu_B)
        return true;
    if (operand != Mask || instr == nullptr)
        return false;
    return std::find(instr->vfu_lst.begin(), instr->vfu_lst.end(),
                     VFU_BitALU) != instr->vfu_lst.end();
}

void
VenusLane::scheduleBitAluOperandPopBoundary(const VenusInstrPkt *instr,
                                            bool maskRowFetched)
{
    if (!experimentalRequesterQVisibility)
        return;
    panic_if(instr == nullptr,
             "BitALU operand pop boundary has no instruction tag");
    /*
     * operandQueuePopdataFIFO now consumes only FIFO-Q-visible entries, so
     * this call is the RTL bitalu_operand_valid/ready handshake itself.  The
     * operand-queue ibuf counters and result_queue_cnt_d are both updated
     * from that same pre-edge snapshot.  Delaying these deltas by another
     * edge makes a full queue appear full after a simultaneous grant/pop and
     * removes the requester's next bank from req_lvl2 for one arbitration.
     */
    const bool popA = instr->use_vs1;
    const bool popB = instr->use_vs2;
    const bool reservesResult =
        !(instr->op >= VREDAND && instr->op <= VREDSUM);
    if (popA)
        noteOperandQueuePop(BitAlu_A);
    if (popB)
        noteOperandQueuePop(BitAlu_B);
    if (maskRowFetched)
        noteOperandQueuePop(Mask);
    /*
     * result_queue_cnt_d is produced by this handshake too, but the
     * operand-ready logic on the same RTL edge still observes
     * result_queue_cnt_q.  gem5 may serialize more than one lane service
     * callback at one tick, so exposing the enqueue delta here would let a
     * later callback observe D as though it were Q.  Commit that one piece
     * at the following registered boundary; operand usage readiness already
     * uses its explicit Q snapshot and can safely retain the pop delta now.
     */
    panic_if(bitAluOperandPopBoundaryEvent.scheduled(),
             "overlapping BitALU result reservation boundary");
    bitAluOperandPopReservesResult = reservesResult;
    bitAluOperandPopA = false;
    bitAluOperandPopB = false;
    bitAluOperandPopMask = false;
    schedule(bitAluOperandPopBoundaryEvent, afterCycles(Cycles(1)));
    DPRINTF(LaneOperandRequester,
            "BitALU operand handshake boundary A %d B %d mask %d "
            "result count %d/%u\n",
            popA, popB, maskRowFetched, bitAluEffectiveResultCount(),
            BitAluResultQueueDepth);
}

void
VenusLane::serviceBitAluOperandPopBoundary()
{
    /*
     * This is the registered BitALU input-handshake/result-reservation
     * transition.  It intentionally runs before the arithmetic/result-Q
     * service event on the same lane edge.  RTL derives result_queue_cnt_d
     * from one pre-edge result_queue_cnt_q snapshot, so a simultaneous
     * enqueue and grant nets to zero; it can never expose the transient
     * 0 -> -1 -> 0 state produced when gem5 serviced the grant first.
     */
    if (bitAluOperandPopReservesResult)
        noteBitAluResultEnqueue();
    if (bitAluOperandPopA)
        noteOperandQueuePop(BitAlu_A);
    if (bitAluOperandPopB)
        noteOperandQueuePop(BitAlu_B);
    if (bitAluOperandPopMask)
        noteOperandQueuePop(Mask);
    DPRINTF(LaneOperandRequester,
            "BitALU registered operand pop boundary A %d B %d mask %d "
            "result count %d/%u\n",
            bitAluOperandPopA, bitAluOperandPopB, bitAluOperandPopMask,
            bitAluEffectiveResultCount(), BitAluResultQueueDepth);
    bitAluOperandPopA = false;
    bitAluOperandPopB = false;
    bitAluOperandPopMask = false;
    bitAluOperandPopReservesResult = false;
}

void
VenusLane::updateBitAluResultCountBoundary()
{
    if (bitAluResultCountTick == curTick())
        return;

    const int next = static_cast<int>(bitAluResultCountQ) +
        bitAluResultCountDelta;
    panic_if(next < 0 || next > static_cast<int>(BitAluResultQueueDepth),
             "BitALU registered result-count transition %u %+d is outside "
             "[0,%u] at tick %llu", bitAluResultCountQ,
             bitAluResultCountDelta, BitAluResultQueueDepth, curTick());
    bitAluResultCountQ = static_cast<unsigned>(next);
    bitAluResultCountDelta = 0;
    bitAluResultCountTick = curTick();
}

int
VenusLane::bitAluEffectiveResultCount() const
{
    return static_cast<int>(bitAluResultCountQ) + bitAluResultCountDelta;
}

void
VenusLane::noteBitAluResultEnqueue()
{
    updateBitAluResultCountBoundary();
    ++bitAluResultCountDelta;
}

void
VenusLane::noteBitAluResultGrant()
{
    updateBitAluResultCountBoundary();
    --bitAluResultCountDelta;
}

bool
VenusLane::bitAluOperandAdmissionReady(const VenusInstrPkt *instr)
{
    panic_if(instr == nullptr,
             "BitALU operand admission has no instruction tag");
    if (!experimentalRequesterQVisibility ||
        (instr->op >= VREDAND && instr->op <= VREDSUM)) {
        return true;
    }
    updateBitAluResultCountBoundary();
    const int occupancy = bitAluEffectiveResultCount();
    const bool ready = occupancy < static_cast<int>(BitAluResultQueueDepth);
    if (!ready) {
        DPRINTF(LaneVFU,
                "BitALU result-queue full; hold operand admission "
                "occupancy %d/%u\n", occupancy,
                BitAluResultQueueDepth);
    }
    return ready;
}

bool
VenusLane::cauOperandAdmissionReady(VenusInstrPkt *instr)
{
    panic_if(instr == nullptr,
             "CAU operand admission has no instruction tag");
    if (!experimentalRequesterQVisibility)
        return true;

    /* venus_pipeline is elastic and contains cau_latency() registers ahead
     * of the two-entry result queue.  Its ready_o falls when every pipeline
     * register and both result slots are occupied.  Treat the tagged
     * pipeline deque and result deque as that exact finite storage; allowing
     * an unbounded pending-result deque lets CAU consume operands while RTL
     * holds cau_operand_ready_o low behind a full result queue. */
    const unsigned capacity = CauResultQueueDepth +
        static_cast<unsigned>(getCauPipeLength(instr));
    const unsigned occupancy = cauPipelineResults.size() +
        cauResultQueue.size();
    const bool ready = occupancy < capacity;
    if (!ready) {
        DPRINTF(LaneVFU,
                "CAU elastic pipeline/result queue full; hold operand "
                "admission occupancy %u/%u\n",
                occupancy, capacity);
    }
    return ready;
}

void
VenusLane::commitOperandReadGrant(
    VenusInstrPkt *pkt, OPERANDTYPE operand,
    bool &dataToRead, bool &dataArrived)
{
    panic_if(pkt == nullptr, "VRF read grant has no requester owner");
    const int requestStep = operand == Mask ? 1 : 2 - pkt->vew;
    panic_if(requestStep <= 0,
             "VRF read grant has invalid VEW %d", pkt->vew);

    for (int producer = 0;
         producer < NrIDs && pkt->chain_overlap_active; ++producer) {
        if (pkt->chain_raw_hazard[producer]) {
            pkt->chain_raw_credit[producer] = false;
            /* In venus_operand_requester.sv, the requester grant clears
             * requester_d.raw_hazard_counter after the registered producer
             * pulse has set it.  Mark the structural path as waiting for the
             * next producer pulse; that pulse enters the counter on one edge
             * and can drive a request only on the following edge. */
            if (experimentalRequesterQVisibility) {
                pkt->chain_raw_credit_pipe[producer] = 2;
                pkt->chain_raw_credit_visible_tick[producer] = MaxTick;
            }
        }
    }

    if (experimentalRequesterQVisibility)
        noteOperandQueueIssue(operand);
    operandRequesterLastGrantTick[static_cast<unsigned>(operand)] = curTick();
    pkt->operand_issue_counter += requestStep;
    int operandLength = 0;
    switch (operand) {
      case BitAlu_A:
      case CAU_A:
      case SerDiv_A:
      case ShuffleUnit:
        operandLength = pkt->locallane_vs1_operand_len;
        break;
      case BitAlu_B:
      case CAU_B:
      case SerDiv_B:
        operandLength = pkt->locallane_vs2_operand_len;
        break;
      case CAU_C:
        operandLength = pkt->locallane_vd1_operand_len;
        break;
      case CAU_D:
        operandLength = pkt->locallane_vd2_operand_len;
        break;
      case Mask:
        operandLength = pkt->locallane_vmask_operand_len;
        break;
      default:
        panic("unknown operand requester type %d", operand);
    }
    if (experimentalRequesterQVisibility &&
        pkt->operand_issue_counter >= operandLength) {
        /*
         * venus_operand_requester changes requester_d to IDLE (or captures
         * a replacement command) from the final VRF grant, not from the
         * corresponding SRAM response.  The replacement requester_q is
         * therefore allowed to present its first stable bank request on the
         * following lane edge.  Remember that edge now; the final tagged
         * response may still have to enter the operand queue before the C++
         * owner pointer can be reused.
         */
        operandRequesterHandoffVisibleTick[
            static_cast<unsigned>(operand)] = afterCycles(Cycles(1));
    }
    /* Tagged CAU reads, like chained-RAW reads, carry their requester owner
     * through the response path.  They must not enter the legacy single-read
     * DATA_TOREADFROM_VRF wait state after a grant: RTL may keep issuing until
     * the registered operand queue reaches its two-entry capacity. */
    const bool taggedRequesterRead = experimentalRequesterQVisibility &&
        ((operand >= BitAlu_A && operand <= CAU_D) || operand == Mask);
    dataToRead = !pkt->chain_raw_pipeline_active && !taggedRequesterRead;
    dataArrived = false;
}

void
VenusLane::handleLaneToVrfGrant(int portid, PacketPtr pkt)
{
    if (dynamic_cast<VrfWriteCommitState *>(pkt->senderState)) {
        /*
         * The original tagged write packet is already accepted by the VRF.
         * Re-enter the queue service once so the ordinary writeback state
         * machine captures exactly that one grant and advances its tagged
         * row counter. sendVFUWriteRequest consumes deferredWriteGrant
         * instead of constructing or sending a replacement packet.
         */
        switch (portid) {
          case 10:
          case 11:
            serviceCauResultQueue();
            break;
          case 12:
          case 13:
            serviceBitAluResultQueue();
            break;
          case 14:
            serviceSerDivResultQueue();
            break;
          default:
            panic("Unexpected tagged VRF write grant port %d", portid);
        }
        panic_if(VenusLaneRequestPorts[portid].hasDeferredWriteGrant(),
                 "VRF write grant on port %d was not captured", portid);
        return;
    }

    auto *state = dynamic_cast<VrfReadResponseState *>(pkt->senderState);
    panic_if(state == nullptr,
             "VRF operand retry grant on port %d lacks a read tag", portid);

    const auto commit = [&](OPERANDTYPE operand, VenusInstrPkt *ownerPkt,
                            bool &dataToRead, bool &dataArrived) {
        panic_if(ownerPkt == nullptr ||
                     ownerPkt->running_id != state->running_id ||
                     ownerPkt->vns_instr_id != state->vns_instr_id,
                 "VRF operand retry grant ownership mismatch on port %d: "
                 "packet instr %d/rid %d", portid,
                 state->vns_instr_id, state->running_id);
        commitOperandReadGrant(ownerPkt, operand, dataToRead, dataArrived);
        DPRINTF(LaneVFU,
                "operand request grant port %d instr %d/rid %d offset %d\n",
                portid, state->vns_instr_id, state->running_id,
                ownerPkt->operand_issue_counter);
    };

    switch (portid) {
      case 0:
        commit(BitAlu_A, queueing_bitaluA_instr_pkt,
               operandrequester_bitaluA_datatoread_from_VRF,
               operandrequester_bitaluA_datafrom_VRF_arrived);
        break;
      case 1:
        commit(BitAlu_B, queueing_bitaluB_instr_pkt,
               operandrequester_bitaluB_datatoread_from_VRF,
               operandrequester_bitaluB_datafrom_VRF_arrived);
        break;
      case 2:
        commit(CAU_A, queueing_cauA_instr_pkt,
               operandrequester_cauA_datatoread_from_VRF,
               operandrequester_cauA_datafrom_VRF_arrived);
        break;
      case 3:
        commit(CAU_B, queueing_cauB_instr_pkt,
               operandrequester_cauB_datatoread_from_VRF,
               operandrequester_cauB_datafrom_VRF_arrived);
        break;
      case 4:
        commit(CAU_C, queueing_cauC_instr_pkt,
               operandrequester_cauC_datatoread_from_VRF,
               operandrequester_cauC_datafrom_VRF_arrived);
        break;
      case 5:
        commit(CAU_D, queueing_cauD_instr_pkt,
               operandrequester_cauD_datatoread_from_VRF,
               operandrequester_cauD_datafrom_VRF_arrived);
        break;
      case 6:
        commit(SerDiv_A, queueing_serdivA_instr_pkt,
               operandrequester_serdivA_datatoread_from_VRF,
               operandrequester_serdivA_datafrom_VRF_arrived);
        break;
      case 7:
        commit(SerDiv_B, queueing_serdivB_instr_pkt,
               operandrequester_serdivB_datatoread_from_VRF,
               operandrequester_serdivB_datafrom_VRF_arrived);
        break;
      default:
        panic("Unexpected tagged VRF operand grant port %d", portid);
    }
}

bool VenusLane::VenusLaneHazardTableListenPort::recvTimingReq(PacketPtr pkt)
{
    return owner->listenHazardTableRequest((VenusHazardTable*)pkt);
}

bool VenusLane::listenHazardTableRequest(VenusHazardTable* pkt)
{
    if (pkt->lane_command_completion_valid) {
        const int runningId = pkt->lane_command_completion_running_id;
        const int instructionId =
            pkt->lane_command_completion_vns_instr_id;
        panic_if(runningId < 0 || runningId >= NrIDs || instructionId < 0,
                 "Invalid lane command completion instr %d/rid %d",
                 instructionId, runningId);
        if (instructionId >=
            locallane_command_retired_vns_instr_id[runningId]) {
            locallane_command_retired_vns_instr_id[runningId] =
                instructionId;
            /*
             * The tagged command-completion sideband is emitted at the
             * sequencer's registered pe_resp boundary.  Venus1 then writes
             * global_hazard_table_o before requester_d can mask its captured
             * hazard and requester_q can expose the clear: two lane edges
             * after this callback.  Venus2's row-chaining path consumes the
             * established one-edge requester boundary and must remain
             * unchanged.  This is a backend pipeline distinction, not an
             * opcode/workload delay.
             */
            locallane_command_retirement_visible_tick[runningId] =
                curTick() + clockPeriod() * (rtlChainingEnabled ? 1 : 2);
        }
        DPRINTF(LaneOperandRequester,
                "all-lane command completion instr %d/rid %d visible "
                "%llu\n", instructionId, runningId,
                static_cast<unsigned long long>(
                    locallane_command_retirement_visible_tick[runningId]));
        return true;
    }

    if (pkt->lsu_completion_valid || pkt->producer_completion_valid) {
        const bool isLsuCompletion = pkt->lsu_completion_valid;
        const int runningId = isLsuCompletion ?
            pkt->lsu_completion_running_id :
            pkt->producer_completion_running_id;
        const int instructionId = isLsuCompletion ?
            pkt->lsu_completion_vns_instr_id :
            pkt->producer_completion_vns_instr_id;
        panic_if(runningId < 0 || runningId >= NrIDs,
                 "Invalid producer completion running ID %d", runningId);
        panic_if(instructionId < 0,
                 "Invalid producer completion generation %d", instructionId);

        /*
         * vldu/vstu register pe_resp_o.vinsn_done.  The RTL sequencer then
         * clears pe_vinsn_running_q_comb combinationally, and
         * venus_operand_requester consumes that clear into requester_q on
         * the following lane edge.  Preserve those two boundaries without
         * clearing any global RAW/WAR/WAW matrix here.  The generation tag
         * prevents a delayed completion from releasing a reused running ID.
         */
        if (instructionId >= locallane_retired_vns_instr_id[runningId]) {
            locallane_retired_vns_instr_id[runningId] = instructionId;
            /* A Shuffle completion clears the combinational hazard on this
             * edge, but venus_operand_requester first captures that clear in
             * requester_q.  Its newly unblocked bank request is therefore
             * visible on the following lane edge.  The task17 sequence-100
             * -> 101 RTL oracle exposes exactly this boundary: req0_hazard
             * clears one edge before req0_gnt.  LDU/STU completion already
             * arrives through the separately registered pe_resp path, so it
             * keeps the established post-edge visibility contract. */
            const bool requesterQClearBoundary =
                !isLsuCompletion || !experimentalRequesterQVisibility;
            locallane_retirement_visible_tick[runningId] =
                curTick() + (requesterQClearBoundary ? clockPeriod() : 0);
        }
        DPRINTF(LaneOperandRequester,
                "%s completion forwarded instr %d/rid %d visible at %llu\n",
                isLsuCompletion ? "LSU" : "producer", instructionId, runningId,
                locallane_retirement_visible_tick[runningId]);
        return true;
    }

    if (venus_hazard_table != nullptr) {
        for (int consumer = 0; consumer < NrIDs; ++consumer) {
            for (int producer = 0; producer < NrIDs; ++producer) {
                const bool wasSet = venus_hazard_table->global_hazard_table
                    [consumer][producer];
                const bool remainsSet = pkt->global_hazard_table
                    [consumer][producer];
                if (remainsSet) {
                    locallane_global_hazard_clear_visible_tick
                        [consumer][producer] = MaxTick;
                } else if (wasSet) {
                    /* requester_d consumes the broadcast combinationally;
                     * requester_q exposes the clear one lane edge later. */
                    locallane_global_hazard_clear_visible_tick
                        [consumer][producer] = curTick() + clockPeriod();
                }
            }
        }
        delete venus_hazard_table;
    }
    venus_hazard_table = new VenusHazardTable(pkt);
    // for(int i=0; i<NrIDs; i++) {
    //     for(int j=0; j<NrIDs; j++) {
    //         venus_hazard_table->global_hazard_table[i][j] = false;
    //     }
    // }
    //暂时不清零全局hazard表
    // DPRINTF(Lane,"VenusLane's listenHazardTableRequest got a new table. The content is:\n");
    // if (::gem5::debug::Lane) {
    //     venus_hazard_table->display();
    // }
    return true;
}


void VenusLane::init()
{
    this->venus_function_unit.localLaneID = this->lane_id;

    schedule(nextVFUBitAluCalcEvent, afterCycles(Cycles(1)));
    schedule(nextVFUCAUCalcEvent, afterCycles(Cycles(1)));
    schedule(nextVFUSerDivCalcEvent, afterCycles(Cycles(1)));
    // schedule(nextVFUShuffleCalcEvent,curTick()+1*1000);
}

bool VenusLane::LanehandleNewInstrRequest(VenusInstrPkt* pkt)
{
    /*
     * venus_lane_sequencer.sv masks a request that already handshook, but
     * still exposes the current ready state of its one-entry fall-through
     * register to the main sequencer.  The latter requires all lane ready
     * signals in the same cycle for bus_done; returning a sticky "accepted"
     * bit here incorrectly hides lane desynchronization.
     */
    if (laneSequencerLastAcceptedInstr == pkt->vns_instr_id)
        return !laneSequencerisBusy;

    if(laneSequencerisBusy) {
        if (laneSequencerLastAcceptedInstr != pkt->vns_instr_id) {
            if (laneSequencerHeldInstr != pkt->vns_instr_id) {
                laneSequencerHeldInstr = pkt->vns_instr_id;
                laneSequencerHeldSince = curTick();
            }
            DPRINTF(LaneSequencer,
                    "lane fall-through holds upstream instr %d/rid %d "
                    "since %llu\n",
                    pkt->vns_instr_id, pkt->running_id,
                    static_cast<unsigned long long>(
                        laneSequencerHeldSince));
        }
        return false;
    }
    // if(isFull) {
    //     panic("vinsn_running_pkt in VenusSequencer isFull");
    //     return false;
    // }
    const bool heldValidBeforeEdge =
        experimentalRequesterQVisibility &&
        laneSequencerHeldInstr == pkt->vns_instr_id &&
        laneSequencerHeldSince < curTick();
    laneSequencerHeldInstr = -1;
    laneSequencerHeldSince = MaxTick;
    laneSequencerisBusy = true;
    laneSequencerLastAcceptedInstr = pkt->vns_instr_id;
    DPRINTF(LaneSequencer,
            "lane input-register capture instr %d/rid %d at %llu\n",
            pkt->vns_instr_id, pkt->running_id,
            static_cast<unsigned long long>(curTick()));

    /*
     * The RTL main sequencer performs an all-lane ready/valid handshake
     * before venus_lane_sequencer derives the local VL.  A non-reduction
     * tail lane with local VL zero raises its registered done token without
     * allocating operand requesters or a VFU.  Preserve that PE/resource
     * boundary instead of dropping the lane before admission or inventing
     * zero-length arithmetic work.
     */
    const bool reduction = pkt->op >= VREDAND && pkt->op <= VREDSUM;
    if (!reduction &&
        getLocalLaneCalcLen(pkt->vl, pkt->vew, pkt->op, false) == 0) {
        laneSequencerisBusy = false;
        laneSequencerHeldInstr = -1;
        laneSequencerHeldSince = MaxTick;
        pendingZeroLocalLaneDone.push_back(new VenusInstrPkt(pkt));
        if (!nextZeroLocalLaneDoneEvent.scheduled())
            schedule(nextZeroLocalLaneDoneEvent, afterCycles(Cycles(1)));
        DPRINTF(LaneSequencer,
                "lane %d local-VL-zero completion queued instr %d/rid %d "
                "for %llu\n",
                lane_id, pkt->vns_instr_id, pkt->running_id,
                static_cast<unsigned long long>(afterCycles(Cycles(1))));
        return true;
    }
    locallane_write_vinsn_progress[pkt->running_id] = 0;
    locallane_read_vinsn_progress[pkt->running_id] = 0;
    locallane_vinsn_has_mask[pkt->running_id] = (pkt->vm_r || pkt->vm_w);
    locallane_vinsn_is_lsu[pkt->running_id] =
        (pkt->op == VLOAD || pkt->op == VSTORE);
    /* A reduction is a compound lane-plus-Shuffle command in RTL.  Its
     * dependency cannot retire on the ordinary lane-arithmetic command
     * sideband merely because BitALU is the first item in vfu_lst: the
     * Shuffle/reduction PE still owns the same running ID.  Classify that
     * producer as Shuffle for requester chaining/retirement purposes so a
     * dependent command consumes the registered global completion clear.
     * This is an opcode-class hardware rule, independent of task or PC. */
    locallane_vinsn_vfu[pkt->running_id] = reduction ?
        VFU_ShuffleUnit :
        (pkt->vfu_lst.empty() ? VFU_NONE : pkt->vfu_lst.front());
    locallane_accepted_vns_instr_id[pkt->running_id] =
        pkt->vns_instr_id;
    //pkt->display();
    if(this->recved_venus_instr_pkt!= nullptr) delete this->recved_venus_instr_pkt;
    this->recved_venus_instr_pkt = new VenusInstrPkt(pkt);
    for(int victim_id = 0; victim_id < NrIDs; ++victim_id) {
        if(!recved_venus_instr_pkt->chain_war_hazard[victim_id] &&
           !recved_venus_instr_pkt->chain_waw_hazard[victim_id])
            continue;
        DPRINTF(Lane,
                "Lane accepted tagged retirement snapshot writer instr %d/rid %d "
                "victim instr %d/rid %d WAR=%d WAW=%d\n",
                recved_venus_instr_pkt->vns_instr_id,
                recved_venus_instr_pkt->running_id,
                recved_venus_instr_pkt->chain_retire_victim_instr[victim_id],
                victim_id,
                recved_venus_instr_pkt->chain_war_hazard[victim_id],
                recved_venus_instr_pkt->chain_waw_hazard[victim_id]);
    }
    /*
     * fall_through_register presents a held valid_i payload directly to its
     * downstream consumer when the resident entry pops.  In that case the
     * retry callback is the sampling edge itself, so operation/operand Q is
     * written now.  A newly presented request crosses the backend's
     * registered PE-to-command path.  Venus1 has distinct sequencer PE
     * request, lane command, and VFU/requester visibility boundaries; the
     * profile value comes from those RTL edges and is independent of opcode,
     * PC, task, and DAG identity.
     */
    if (heldValidBeforeEdge && laneSequencerCanTransfer()) {
        laneSequencerPushFIFONewInstr();
    } else if(!nextLaneSequencerPushFIFONewInstrEvent.scheduled()) {
        schedule(nextLaneSequencerPushFIFONewInstrEvent,
                 afterCycles(Cycles(rtlPeCommandVisibilityCycles)));
    } else {
        panic("schedule(nextLaneSequencerPushFIFONewInstrEvent, curTick() + 1*1000);");
    }
    return true;
}

void
VenusLane::reportZeroLocalLaneDone()
{
    while (!pendingZeroLocalLaneDone.empty()) {
        VenusInstrPkt *pkt = pendingZeroLocalLaneDone.front();
        pendingZeroLocalLaneDone.pop_front();
        pkt->makeResponse();
        port_venuslane_receivefrom_venussequencer.sendTimingResp(pkt);
        delete pkt;
    }
}

bool
VenusLane::laneSequencerCanTransfer() const
{
    if (recved_venus_instr_pkt == nullptr)
        return false;

    const auto targets = [this](VFU vfu) {
        return std::find(recved_venus_instr_pkt->vfu_lst.begin(),
                         recved_venus_instr_pkt->vfu_lst.end(),
                         vfu) != recved_venus_instr_pkt->vfu_lst.end();
    };
    const auto hasCommand = [](const std::list<VenusInstrPkt *> &queue,
                               OPERANDTYPE operand) {
        return std::any_of(queue.begin(), queue.end(),
            [operand](const VenusInstrPkt *pkt) {
                switch (operand) {
                  case BitAlu_A:
                  case CAU_A:
                  case SerDiv_A:
                    return pkt->use_vs1;
                  case BitAlu_B:
                  case CAU_B:
                  case SerDiv_B:
                    return pkt->use_vs2;
                  case CAU_C:
                    return pkt->use_vd1_op;
                  case CAU_D:
                    return pkt->use_vd2_op;
                  case Mask:
                    return pkt->vm_r || pkt->vm_w;
                  default:
                    return true;
                }
            });
    };

    /*
     * operand_req_valid_o is a one-entry command register per operand.
     * CAU admission uses an independent timing state because the functional
     * gem5 requester FIFOs intentionally run lane-synchronously and cannot
     * represent the RTL's 40-row/44-row tail phase difference.
     */
    if (targets(VFU_CAU) && experimentalOneEntryOperandCommands) {
        /*
         * operand_req_valid_o is one physical register per operand in RTL.
         * It remains asserted until that particular requester raises
         * operand_req_ready_i; a later CAU operation must therefore remain
         * in the lane input fall-through register while any CAU command is
         * pending.  The *_instr_FIFO_lanseq lists are the gem5 image of
         * those valid bits.  Do not allow them to grow into an artificial
         * multi-entry command queue.
         */
        return !hasCommand(cauA_instr_FIFO_lanseq, CAU_A) &&
               !hasCommand(cauB_instr_FIFO_lanseq, CAU_B) &&
               !hasCommand(cauC_instr_FIFO_lanseq, CAU_C) &&
               !hasCommand(cauD_instr_FIFO_lanseq, CAU_D) &&
               !hasCommand(mask_instr_FIFO_lanseq, Mask) &&
               curTick() >= operandCommandAckVisibleTick[CAU_A] &&
               curTick() >= operandCommandAckVisibleTick[CAU_B] &&
               curTick() >= operandCommandAckVisibleTick[CAU_C] &&
               curTick() >= operandCommandAckVisibleTick[CAU_D] &&
               curTick() >= operandCommandAckVisibleTick[Mask];
    }

    if (targets(VFU_CAU))
        return curTick() >= operandCommandReleaseTick[CAU_A] &&
               curTick() >= operandCommandReleaseTick[CAU_B] &&
               curTick() >= operandCommandReleaseTick[CAU_C] &&
               curTick() >= operandCommandReleaseTick[CAU_D] &&
               curTick() >= operandCommandReleaseTick[Mask];

    if (targets(VFU_BitALU)) {
        if (hasCommand(bitaluA_instr_FIFO_lanseq, BitAlu_A) ||
            hasCommand(bitaluB_instr_FIFO_lanseq, BitAlu_B) ||
            hasCommand(mask_instr_FIFO_lanseq, Mask))
            return false;
        if (experimentalOneEntryOperandCommands &&
            (curTick() < operandCommandAckVisibleTick[BitAlu_A] ||
             curTick() < operandCommandAckVisibleTick[BitAlu_B] ||
             curTick() < operandCommandAckVisibleTick[Mask]))
            return false;
    }

    if (targets(VFU_SerDiv)) {
        if (hasCommand(serdivA_instr_FIFO_lanseq, SerDiv_A) ||
            hasCommand(serdivB_instr_FIFO_lanseq, SerDiv_B) ||
            hasCommand(mask_instr_FIFO_lanseq, Mask))
            return false;
        if (experimentalOneEntryOperandCommands &&
            (curTick() < operandCommandAckVisibleTick[SerDiv_A] ||
             curTick() < operandCommandAckVisibleTick[SerDiv_B] ||
             curTick() < operandCommandAckVisibleTick[Mask]))
            return false;
    }

    return true;
}

unsigned
VenusLane::maxLocalLaneCalcLen(const VenusInstrPkt *pkt) const
{
    /*
     * This is the maximum of getLocalLaneCalcLen() over all lanes, expressed
     * directly from the RTL stripe geometry.  A partial stripe is assigned
     * in lane-sized chunks, so lane 0 carries the maximum tail.
     */
    const unsigned bytesPerLaneStripe =
        NrBankPerLane * NrBitsPerBank / 8;
    const unsigned bytesPerFullStripe = NrLanes * bytesPerLaneStripe;
    const unsigned vlEw8 = pkt->vl << pkt->vew;
    const unsigned fullStripeBytes =
        (vlEw8 / bytesPerFullStripe) * bytesPerLaneStripe;
    const unsigned tailBytes = vlEw8 % bytesPerFullStripe;
    const unsigned maxTailBytes =
        std::min(tailBytes, bytesPerLaneStripe);
    return (fullStripeBytes + maxTailBytes) >> pkt->vew;
}

void
VenusLane::occupyOperandCommandStages()
{
    if (recved_venus_instr_pkt == nullptr)
        return;

    const auto targets = [this](VFU vfu) {
        return std::find(recved_venus_instr_pkt->vfu_lst.begin(),
                         recved_venus_instr_pkt->vfu_lst.end(),
                         vfu) != recved_venus_instr_pkt->vfu_lst.end();
    };
    if (!targets(VFU_CAU))
        return;

    /*
     * A CAU operand requester can accept the next command in the same cycle
     * that its final row is granted.  The arithmetic pipeline and result
     * queue are downstream of that command handshake; charging their depth
     * again here double-counts backpressure already represented by the VFU
     * result queue and VRF arbiter.  Keep only the registered requester
     * handoff cycle between back-to-back command streams.
     *
     * The lane's operand_req_valid_o register is cleared only on the edge
     * where the requester accepts the tag.  The upstream fall-through
     * register then clears on the following edge, so pe_req_ready_o exposes
     * the freed command slot two cycles after requester acceptance.  Keep
     * that visibility boundary separate from requester service time.
     */
    constexpr unsigned CauSaturatedBoundaryCycles = 1;
    constexpr unsigned OperandCommandRetireVisibilityCycles = 1;

    /*
     * Although the last participating lane can contain one fewer row, RTL's
     * shared per-bank grant and operand-queue backpressure keep the CAU
     * requester handoff aligned across participating lanes.  Using the local
     * tail length here makes the short lane release its input FIFO one cycle
     * early and creates a lane de-synchronization which is absent in RTL.
     */
    const unsigned alignedRows =
        maxLocalLaneCalcLen(recved_venus_instr_pkt);
    const unsigned saturatedRows =
        maxLocalLaneCalcLen(recved_venus_instr_pkt) +
        CauSaturatedBoundaryCycles;

    const auto occupy = [&](OPERANDTYPE operand, bool used) {
        if (!used)
            return;
        const Tick now = curTick();
        const Tick start =
            std::max(now, operandRequesterAvailableTick[operand]);
        const bool queuedBehindActiveRequester = start > now;
        const unsigned serviceCycles =
            queuedBehindActiveRequester ? saturatedRows : alignedRows;
        operandCommandReleaseTick[operand] =
            start + OperandCommandRetireVisibilityCycles * clockPeriod();
        operandRequesterAvailableTick[operand] =
            start + serviceCycles * clockPeriod();
    };

    occupy(CAU_A, recved_venus_instr_pkt->use_vs1);
    occupy(CAU_B, recved_venus_instr_pkt->use_vs2);
    occupy(CAU_C, recved_venus_instr_pkt->use_vd1_op);
    occupy(CAU_D, recved_venus_instr_pkt->use_vd2_op);
    occupy(Mask, recved_venus_instr_pkt->vm_r ||
                   recved_venus_instr_pkt->vm_w);
}

#define __GENreportAndRecycleDoneInstr_Name__(VFU_TYPE, INSTRDONE_PKT) \
{ \
    DPRINTF(LaneSequencer,"reportAndRecycleDoneInstr with VID = %d\n",INSTRDONE_PKT->vns_instr_id); \
    DPRINTF(LaneVFU, \
            "VFU retirement vfu %d instr %d/rid %d\n", \
            VFU_TYPE, INSTRDONE_PKT->vns_instr_id, \
            INSTRDONE_PKT->running_id); \
    /* Preserve the local registered retirement edge.  An arithmetic \
     * requester also requires the sequencer's generation-tagged all-lane \
     * completion before this local observation may clear its hazard. */ \
    locallane_retired_vns_instr_id[INSTRDONE_PKT->running_id] = \
        INSTRDONE_PKT->vns_instr_id; \
    locallane_retirement_visible_tick[INSTRDONE_PKT->running_id] = \
        curTick() + clockPeriod(); \
    INSTRDONE_PKT->makeResponse(); \
    port_venuslane_receivefrom_venussequencer.sendTimingResp((PacketPtr)INSTRDONE_PKT); \
    delete INSTRDONE_PKT;  \
    INSTRDONE_PKT=nullptr; \
    return true; \
}
bool VenusLane::reportAndRecycleDoneInstr(VFU VFUType)
{
    switch (VFUType) {
        case VFU_BitALU     : __GENreportAndRecycleDoneInstr_Name__(VFU_BitALU     , bitalu_doneinstr_pkt  );break;
        case VFU_CAU        : __GENreportAndRecycleDoneInstr_Name__(VFU_CAU        , cau_doneinstr_pkt     );break;
        case VFU_SerDiv     : __GENreportAndRecycleDoneInstr_Name__(VFU_SerDiv     , serdiv_doneinstr_pkt  );break;
        case VFU_ShuffleUnit: __GENreportAndRecycleDoneInstr_Name__(VFU_ShuffleUnit, tshuffle_doneinstr_pkt);break;
        default: panic("unknown VFUType");
    }
}

int VenusLane::getVFUProcessingTime(VenusInstrPkt* pkt)
{
    // if((((running_mask_data_pkt) & 0x1)|(((running_mask_data_pkt) & 0x100)>>8)) )  // mask in the end能提升的点
    //     return 1;
    // else
        if((pkt->op >= VAND && pkt->op <= VSSUBU) || pkt->op == VABS || (pkt->op >= VMIN && pkt->op <= VMAX))
            return 1;
        else if(pkt->op >= VMUL && pkt->op <= VSUBMUL)
            return 1;
        else if(pkt->op == VCMXMUL)
            return 1;
        else if(pkt->op >= VREDAND && pkt->op <= VREDSUM)
            return 1;
        else if(pkt->op >= VDIV && pkt->op <= VREMU) {
            bool uop_i;
            if(pkt->op == VDIVU || pkt->op == VREMU)
                uop_i = false;
            else if(pkt->op == VDIV || pkt->op == VREM)
                uop_i = true;
            VEW vew_i = pkt->vew;
            bool use_scalar_op = pkt->use_scalar_op;
            unsigned int scalar_op = pkt->scalar_op;

            int cycle_consume = 0;

            /*
             * Mirror venus_serdiv_controller's datapath before applying the
             * divider LZCs.  In particular, cau_mul_shamt_i (vfu_shamt in
             * the architectural packet) shifts the sign/zero-extended
             * dividend in the 2*ELEN domain.  Omitting this made shift-zero
             * SCH divisions accurate only by accident and previously led to
             * an unrelated vector-length multiplier to compensate for
             * shifted divisions.
             */
            auto elementCycles = [uop_i, pkt](
                                     uint32_t raw_a, uint32_t raw_b,
                                     unsigned width) {
                const uint32_t element_mask =
                    width == 8 ? 0xffu : 0xffffu;
                const uint32_t sign_mask = 1u << (width - 1);
                raw_a &= element_mask;
                raw_b &= element_mask;

                uint32_t op_a_i = raw_a;
                uint32_t op_b_i = raw_b;
                if (uop_i && (raw_a & sign_mask))
                    op_a_i |= ~element_mask;
                if (uop_i && (raw_b & sign_mask))
                    op_b_i |= ~element_mask;

                op_a_i <<= pkt->vfu_shamt;
                const bool op_a_sign = (op_a_i & 0x80000000u) != 0;
                const bool op_b_sign = (op_b_i & 0x80000000u) != 0;
                const uint32_t lzc_a_input =
                    (uop_i && op_a_sign && op_a_i == 0xffffffffu) ?
                        ((~op_a_i << 1) | 1u) :
                    (uop_i && op_a_sign) ? (~op_a_i << 1) : op_a_i;
                const uint32_t lzc_b_input =
                    (uop_i && op_b_sign) ? ~op_b_i : op_b_i;
                const unsigned shift_a = lzc_a_input == 0 ?
                    32 : __builtin_clz(lzc_a_input);
                const int div_shift = lzc_b_input == 0 ?
                    32 : static_cast<int>(__builtin_clz(lzc_b_input)) -
                             static_cast<int>(shift_a);
                return div_shift < 0 ? 2 : div_shift + 3;
            };

            if(vew_i == EW16) {
                const uint32_t op_a_i =
                    use_scalar_op ? scalar_op : running_serdivA_data_pkt;
                const uint32_t op_b_i = running_serdivB_data_pkt;
                cycle_consume += elementCycles(op_a_i, op_b_i, 16);
                cycle_consume += 3;
            } else {
                const uint32_t op_a_i =
                    use_scalar_op ? scalar_op : running_serdivA_data_pkt;
                const uint32_t op_b_i = running_serdivB_data_pkt;
                cycle_consume += elementCycles(
                    op_a_i & 0xffu, op_b_i & 0xffu, 8);
                cycle_consume += elementCycles(
                    (op_a_i >> 8) & 0xffu,
                    (op_b_i >> 8) & 0xffu, 8);
                cycle_consume += 3;
            }
            // std::cout<<"cycle_consume = "<<cycle_consume<<"\n";
            /*
             * This is the acceptance-to-result interval of one lane word in
             * venus_serdiv_controller plus venus_serdiv.  Operand FIFO depth
             * does not scale the iterative arithmetic latency: the wrapper
             * holds ready_o low while this word is active and accepts the
             * next word when the controller returns to ISSUE_IDLE.  Queue
             * capacity and result back-pressure are modeled independently by
             * the operand and tagged result queues.
             */
            return cycle_consume;
        }
        else
            {std::cout<<"UNSUPPORTED INSTR!"<<"\n"; pkt->display(); panic("UNSUPPORTED INSTR!");}
}
int VenusLane::getVFUPipeLength(VenusInstrPkt* pkt)
{
    /* venus_bitalu_wrapper writes the combinational venus_bitalu result
     * directly into result_queue_d on the operand-handshake edge.  For the
     * masked path, whose independent operand-mask D/Q latch is modelled
     * explicitly, the result queue below is therefore the only output
     * register.  The ordinary A/B-only path still retains its existing
     * effective one-edge ingress until that queue's data-visible boundary is
     * split out in the same way. */
    if (!(pkt->op >= VREDAND && pkt->op <= VREDSUM))
        return 0;
    if(pkt->op >= VREDAND && pkt->op <= VREDSUM)
        return 1;
    else
        {std::cout<<"UNSUPPORTED INSTR!"<<"\n"; pkt->display(); panic("UNSUPPORTED INSTR!");}
}
int VenusLane::getCauPipeLength(VenusInstrPkt* pkt)
{
    /*
     * Match venus_cau_wrapper.sv's cau_latency() exactly.  Operand requester
     * Q visibility and the depth-2 result queue are modeled as their own
     * registered boundaries; folding either boundary into this arithmetic
     * pipe double-counts it.  In particular, task17's VMUL operand grants
     * already coincide with RTL and a 4-cycle pipe makes the first result
     * grant two cycles late.  The RTL parameters are LatAddSub=0, LatMul=2,
     * and LatCmxMul=4.
     */
    if (pkt->op >= VADD && pkt->op <= VSUB)
        return 0;
    if (pkt->op >= VMUL && pkt->op <= VSUBMUL)
        return 2;
    if (pkt->op == VCMXMUL)
        return 4;
    return 0;
}
unsigned int VenusLane::getVFUReduceResult(VenusOp op, VEW vew, int vl)
{
    if(lane_id != 0) {
        panic("getVFUReduceResult called by a non-zero lane_id %d, which is illegal", lane_id);
    }

    int nrUsedLanes = 0;
    if(vew == EW8) {
        nrUsedLanes = std::min((vl + (NrBankPerLane * NrBitsPerBank / 8) - 1) / (NrBankPerLane * NrBitsPerBank / 8) , (NrLanes));
    }
    if(vew == EW16) {
        nrUsedLanes = std::min((vl + (NrBankPerLane * NrBitsPerBank / 16) - 1) / (NrBankPerLane * NrBitsPerBank / 16) , (NrLanes));
    }

    switch (op) {
        case VREDAND: {
            unsigned int result;
            if(vew == EW8) {result = 255;}
            if(vew == EW16) {result = 65535;}
            for(int i = 0; i < nrUsedLanes; i++) {
                if(vew == EW8)
                    result &= (unsigned int)m_venus_lanes[i]->venus_function_unit.and_value;
                if(vew == EW16)
                    result &= (unsigned int)m_venus_lanes[i]->venus_function_unit.and_value_1;
            }
            return result;
            break;
        }
        case VREDOR : {
            unsigned int result;
            if(vew == EW8) {result = 0;}
            if(vew == EW16) {result = 0;}
            for(int i = 0; i < nrUsedLanes; i++) {
                if(vew == EW8)
                    result |= (unsigned int)m_venus_lanes[i]->venus_function_unit.or_value;
                if(vew == EW16)
                    result |= (unsigned int)m_venus_lanes[i]->venus_function_unit.or_value_1;
            }
            return result;
            break;
        }
        case VREDXOR: {
            unsigned int result;
            if(vew == EW8) {result = 0;}
            if(vew == EW16) {result = 0;}
            for(int i = 0; i < nrUsedLanes; i++) {
                if(vew == EW8)
                    result ^= (unsigned int)m_venus_lanes[i]->venus_function_unit.xor_value;
                if(vew == EW16)
                    result ^= (unsigned int)m_venus_lanes[i]->venus_function_unit.xor_value_1;
            }
            return result;
            break;
        }
        case VREDMIN:  {
            int result;
            if(vew == EW8) {result = 127;}
            if(vew == EW16) {result = 32767;}
            for(int i = 0; i < nrUsedLanes; i++) {
                if(vew == EW8) {
                    if(result > (int)m_venus_lanes[i]->venus_function_unit.min_value) result = (int)m_venus_lanes[i]->venus_function_unit.min_value;
                }
                if(vew == EW16) {
                    if(result > (int)m_venus_lanes[i]->venus_function_unit.min_value_1) result = (int)m_venus_lanes[i]->venus_function_unit.min_value_1;
                }
            }
            return (unsigned int)result;
            break;
        }
        case VREDMAX:  {
            int result;
            if(vew == EW8) {result = -128;}
            if(vew == EW16) {result = -32768;}
            for(int i = 0; i < nrUsedLanes; i++) {
                if(vew == EW8) {
                    if(result < (int)m_venus_lanes[i]->venus_function_unit.max_value) result = (int)m_venus_lanes[i]->venus_function_unit.max_value;
                    DPRINTF(LaneVFUFull,"Reducing, comparing Lane %d, champion is %d, challenger is %d\n", i, result, m_venus_lanes[i]->venus_function_unit.max_value);
                }
                if(vew == EW16) {
                    if(result < (int)m_venus_lanes[i]->venus_function_unit.max_value_1) result = (int)m_venus_lanes[i]->venus_function_unit.max_value_1;
                    DPRINTF(LaneVFUFull,"Reducing, comparing Lane %d, champion is %d, challenger is %d\n", i, result, m_venus_lanes[i]->venus_function_unit.max_value_1);
                }
            }
            return (unsigned int)result;
            break;
        }
        case VREDMINU:  {
            unsigned int result;
            if(vew == EW8) {result = 255;}
            if(vew == EW16) {result = 65535;}
            for(int i = 0; i < nrUsedLanes; i++) {
                if(vew == EW8) {
                    if(result > (unsigned int)m_venus_lanes[i]->venus_function_unit.min_uvalue) result = (unsigned int)m_venus_lanes[i]->venus_function_unit.min_uvalue;
                }
                if(vew == EW16) {
                    if(result > (unsigned int)m_venus_lanes[i]->venus_function_unit.min_uvalue_1) result = (unsigned int)m_venus_lanes[i]->venus_function_unit.min_uvalue_1;
                }
            }
            return (unsigned int)result;
            break;
        }
        case VREDMAXU:  {
            unsigned int result;
            if(vew == EW8) {result = 0;}
            if(vew == EW16) {result = 0;}
            for(int i = 0; i < nrUsedLanes; i++) {
                if(vew == EW8) {
                    if(result < (unsigned int)m_venus_lanes[i]->venus_function_unit.max_uvalue) result = (unsigned int)m_venus_lanes[i]->venus_function_unit.max_uvalue;
                }
                if(vew == EW16) {
                    if(result < (unsigned int)m_venus_lanes[i]->venus_function_unit.max_uvalue_1) result = (unsigned int)m_venus_lanes[i]->venus_function_unit.max_uvalue_1;
                }
            }
            return (unsigned int)result;
            break;
        }
        case VREDSUM:  {
            int result = 0;
            for(int i = 0; i < nrUsedLanes; i++) {
                    result += (int)m_venus_lanes[i]->venus_function_unit.sum_result;
            }
            return (unsigned int)result;
            break;
        }
        default:
            panic("getVFUReduceResult called with an illegal op %s", op_to_str(op));
    }
}
unsigned int VenusLane::getLocalLaneCalcLen(int vl, VEW vew, VenusOp op, bool is_write_back = false)
{
    if(is_write_back == true) {
        if(op == VREDAND || op == VREDOR || op == VREDXOR || op == VREDMIN || op == VREDMAX || op == VREDMINU || op == VREDMAXU)
            if(lane_id == 0)
                return 1;
            else
                return 0;
        else if(op == VREDSUM)
            if(lane_id == 0)
                if(vew == EW8)
                    return 4;
                else
                    return 2;
            else
                return 0;
    }

    int vl_ew8 = (vl << (vew));
    int vltail_ew8 = vl_ew8 % (NrLanes*NrBankPerLane*NrBitsPerBank/8);
    vl_ew8 = ((vl_ew8 >> int(log2(NrLanes*NrBankPerLane*NrBitsPerBank/8))) << int(log2(NrBankPerLane*NrBitsPerBank/8)));
    int lane_id_element = (lane_id << int(log2(NrBankPerLane*NrBitsPerBank/8)));
    int sub_vltail_lane = vltail_ew8 - lane_id_element;
    if(sub_vltail_lane > 0) {
        if(sub_vltail_lane < NrBankPerLane*NrBitsPerBank/8)
            vl_ew8 += sub_vltail_lane;
        else
            vl_ew8 += NrBankPerLane*NrBitsPerBank/8;
    }
    return (vl_ew8 >> (vew));
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#define __GENlaneSequencerPushFIFO_Name__(VFU_NAME, OPERAND_USED, OPERAND_LENGTH, OPERAND_TYPE_NAME, FIFO_NAME, BUSY_NAME, EVENT_NAME) \
{ \
    bool found = (std::find(this->recved_venus_instr_pkt->vfu_lst.begin(), this->recved_venus_instr_pkt->vfu_lst.end(), VFU_NAME) != this->recved_venus_instr_pkt->vfu_lst.end()); \
    const bool operand_used = (VFU_NAME == VFU_Mask) ? \
        (this->recved_venus_instr_pkt->vm_r || \
         this->recved_venus_instr_pkt->vm_w) : \
        (this->recved_venus_instr_pkt->OPERAND_USED == 1); \
    if(VFU_NAME == VFU_Mask && !operand_used) \
        found = false; \
    if(found){ \
        if(operand_used) { \
            const unsigned local_elements = getLocalLaneCalcLen(this->recved_venus_instr_pkt->vl, this->recved_venus_instr_pkt->vew, this->recved_venus_instr_pkt->op); \
            this->recved_venus_instr_pkt->OPERAND_LENGTH = \
                (VFU_NAME == VFU_Mask) ? \
                    ((local_elements * (this->recved_venus_instr_pkt->vew + 1) + 7) / 8) : \
                    local_elements; \
            this->recved_venus_instr_pkt->operand_lst.push_back(OPERAND_TYPE_NAME); \
        } else { \
            this->recved_venus_instr_pkt->OPERAND_LENGTH = 0; \
        } \
        FIFO_NAME.push_back(new VenusInstrPkt(this->recved_venus_instr_pkt)); \
        /*DPRINTF(LaneSequencer,"laneSequencer issue a new insn pkt to %s where its size() = %d, %s = %d, target length is: %d\n",#FIFO_NAME, FIFO_NAME.size(), #BUSY_NAME, BUSY_NAME, this->recved_venus_instr_pkt->OPERAND_LENGTH); */\
        if(BUSY_NAME == false && FIFO_NAME.size()==1) \
            if(!EVENT_NAME.scheduled()) \
                /* In RTL operand_req_o is captured by requester_q on the \
                 * next lane edge.  The legacy model used two event cycles; \
                 * retain that default while the registered-boundary \
                 * experiment uses the single physical edge. */ \
                schedule(EVENT_NAME, afterCycles(Cycles( \
                    experimentalRequesterQVisibility ? 1 : 2))); \
    } \
}

void VenusLane::laneSequencerPushFIFONewInstr()
{
    if(laneSequencerisBusy == true)
    {
        if (!laneSequencerCanTransfer()) {
            if (!nextLaneSequencerPushFIFONewInstrEvent.scheduled())
                schedule(nextLaneSequencerPushFIFONewInstrEvent,
                         afterCycles(Cycles(1)));
            return;
        }

        occupyOperandCommandStages();
        laneSequencerisBusy = false;
        DPRINTF(LaneSequencer,
                "lane operation-register transfer instr %d/rid %d at %llu\n",
                recved_venus_instr_pkt->vns_instr_id,
                recved_venus_instr_pkt->running_id,
                static_cast<unsigned long long>(curTick()));
        //DPRINTF(LaneSequencer, "VenusLaneSequencer has received an vns instr successfully. The content is:\n");
        // this->recved_venus_instr_pkt->display();
        // delete this->recved_venus_instr_pkt;
        // this->recved_venus_instr_pkt=nullptr;

        __GENlaneSequencerPushFIFO_Name__(VFU_BitALU     , use_vs1                                , locallane_vs1_operand_len  , BitAlu_A   , bitaluA_instr_FIFO_lanseq, operandrequester_bitaluA_busy, nextOperandRequesterGetsBitAlu_A_DataEvent   );
        __GENlaneSequencerPushFIFO_Name__(VFU_BitALU     , use_vs2                                , locallane_vs2_operand_len  , BitAlu_B   , bitaluB_instr_FIFO_lanseq, operandrequester_bitaluB_busy, nextOperandRequesterGetsBitAlu_B_DataEvent   );
        __GENlaneSequencerPushFIFO_Name__(VFU_CAU        , use_vs1                                , locallane_vs1_operand_len  , CAU_A      , cauA_instr_FIFO_lanseq   , operandrequester_cauA_busy   , nextOperandRequesterGetsCAU_A_DataEvent      );
        __GENlaneSequencerPushFIFO_Name__(VFU_CAU        , use_vs2                                , locallane_vs2_operand_len  , CAU_B      , cauB_instr_FIFO_lanseq   , operandrequester_cauB_busy   , nextOperandRequesterGetsCAU_B_DataEvent      );
        __GENlaneSequencerPushFIFO_Name__(VFU_CAU        , use_vd1_op                             , locallane_vd1_operand_len  , CAU_C      , cauC_instr_FIFO_lanseq   , operandrequester_cauC_busy   , nextOperandRequesterGetsCAU_C_DataEvent      );
        __GENlaneSequencerPushFIFO_Name__(VFU_CAU        , use_vd2_op                             , locallane_vd2_operand_len  , CAU_D      , cauD_instr_FIFO_lanseq   , operandrequester_cauD_busy   , nextOperandRequesterGetsCAU_D_DataEvent      );
        __GENlaneSequencerPushFIFO_Name__(VFU_SerDiv     , use_vs1                                , locallane_vs1_operand_len  , SerDiv_A   , serdivA_instr_FIFO_lanseq, operandrequester_serdivA_busy, nextOperandRequesterGetsSerDiv_A_DataEvent   );
        __GENlaneSequencerPushFIFO_Name__(VFU_SerDiv     , use_vs2                                , locallane_vs2_operand_len  , SerDiv_B   , serdivB_instr_FIFO_lanseq, operandrequester_serdivB_busy, nextOperandRequesterGetsSerDiv_B_DataEvent   );
        __GENlaneSequencerPushFIFO_Name__(VFU_Mask       , vm_r                                   , locallane_vmask_operand_len, Mask       , mask_instr_FIFO_lanseq   , operandrequester_mask_busy   , nextOperandRequesterGetsMask_DataEvent       );
        // __GENlaneSequencerPushFIFO_Name__(VFU_ShuffleUnit, use_vs1                                , locallane_vs1_operand_len  , ShuffleUnit, shuffle_instr_FIFO_lanseq, operandrequester_shuffle_busy, nextOperandRequesterGetsShuffleUnit_DataEvent);
    }
}



#define __GENlaneSequencerPopFIFO_Name__(OPERAND_TYPE_NAME, RUNNING_PKT_NAME, FIFO_NAME) \
if(FIFO_NAME.size() == 0) \
    return false; \
{ \
    int count = 0; \
    for (auto it = FIFO_NAME.begin(); it != FIFO_NAME.end() && count < 8; ++it, ++count) { \
        /*DPRINTF(LaneSequencer, "LHB 显示当前指令 %s [%d]: vns_instr_id %d, running_id %d\n", #FIFO_NAME, count, (*it)->vns_instr_id, (*it)->running_id);*/ \
    } \
} \
int cur_id = FIFO_NAME.front()->running_id; \
/* A requester must not inspect a broadcast table from before its instruction \
 * was allocated.  RTL transports pe_req.id and its hazard vectors together; \
 * this generation check provides the same atomicity across gem5's three-stage \
 * hazard-table broadcast. */ \
const bool hazard_table_is_current = \
    venus_hazard_table->running_id_to_vns_instr_id[cur_id] == \
    FIFO_NAME.front()->vns_instr_id; \
bool ready = hazard_table_is_current && \
    venus_hazard_table->checkifreadytofire(cur_id); \
bool overlap_admission = false; \
const bool operand_used = \
    ((OPERAND_TYPE_NAME == BitAlu_A || OPERAND_TYPE_NAME == CAU_A || \
      OPERAND_TYPE_NAME == SerDiv_A || OPERAND_TYPE_NAME == ShuffleUnit) && \
        FIFO_NAME.front()->use_vs1) || \
    ((OPERAND_TYPE_NAME == BitAlu_B || OPERAND_TYPE_NAME == CAU_B || \
      OPERAND_TYPE_NAME == SerDiv_B) && FIFO_NAME.front()->use_vs2) || \
    (OPERAND_TYPE_NAME == CAU_C && FIFO_NAME.front()->use_vd1_op) || \
    (OPERAND_TYPE_NAME == CAU_D && FIFO_NAME.front()->use_vd2_op) || \
    (OPERAND_TYPE_NAME == Mask && \
        (FIFO_NAME.front()->vm_r || FIFO_NAME.front()->vm_w)); \
/* Each RTL requester accepts its tagged command independently of destination \
 * WAR/WAW retirement.  Source RAW is latched into requester-local state. \
 * R52 permits cross-VFU arithmetic row credits.  The tagged per-bank research \
 * path also permits same-VFU arithmetic credits so a following requester can \
 * overlap the producer result stream; LSU and shuffle still require full \
 * retirement. */ \
if(!ready && hazard_table_is_current) { \
    const bool tagged_operand_requester = \
        OPERAND_TYPE_NAME == BitAlu_A || \
        OPERAND_TYPE_NAME == BitAlu_B || \
        OPERAND_TYPE_NAME == CAU_A || OPERAND_TYPE_NAME == CAU_B || \
        OPERAND_TYPE_NAME == CAU_C || OPERAND_TYPE_NAME == CAU_D || \
        OPERAND_TYPE_NAME == SerDiv_A || \
        OPERAND_TYPE_NAME == SerDiv_B || \
        OPERAND_TYPE_NAME == Mask; \
    ready = !operand_used || tagged_operand_requester; \
    overlap_admission = operand_used && tagged_operand_requester; \
} \
if(ready) \
{ \
    if(RUNNING_PKT_NAME != nullptr) delete RUNNING_PKT_NAME; \
    RUNNING_PKT_NAME = new VenusInstrPkt(FIFO_NAME.front()); \
    RUNNING_PKT_NAME->chain_overlap_active = overlap_admission; \
    RUNNING_PKT_NAME->chain_raw_pipeline_active = false; \
    for (int p_id = 0; p_id < NrIDs; p_id++) { \
        bool operand_raw_hazard = false; \
        if(OPERAND_TYPE_NAME == BitAlu_A || OPERAND_TYPE_NAME == CAU_A || OPERAND_TYPE_NAME == SerDiv_A) \
            operand_raw_hazard = venus_hazard_table->vs1_hazard_table[cur_id][p_id]; \
        else if(OPERAND_TYPE_NAME == BitAlu_B || OPERAND_TYPE_NAME == CAU_B || OPERAND_TYPE_NAME == SerDiv_B) \
            operand_raw_hazard = venus_hazard_table->vs2_hazard_table[cur_id][p_id]; \
        else if(OPERAND_TYPE_NAME == CAU_C) \
            operand_raw_hazard = venus_hazard_table->raw_hazard_table[cur_id][p_id] && \
                venus_hazard_table->vd1_hazard_table[cur_id][p_id]; \
        else if(OPERAND_TYPE_NAME == CAU_D) \
            operand_raw_hazard = venus_hazard_table->raw_hazard_table[cur_id][p_id] && \
                venus_hazard_table->vd2_hazard_table[cur_id][p_id]; \
        else if(OPERAND_TYPE_NAME == Mask) \
            operand_raw_hazard = venus_hazard_table->vm_hazard_table[cur_id][p_id]; \
        operand_raw_hazard = p_id != cur_id && operand_used && \
            operand_raw_hazard; \
        /* venus_lane_sequencer.sv places destination hazards in each \
         * source requester's tagged hazard bitmap as well: BitALU and \
         * SerDiv use hazard_vd1, while all four CAU requesters use \
         * hazard_vd1|hazard_vd2.  This is distinct from result retirement: \
         * it prevents a younger requester's reads from overtaking an older \
         * reader/writer before its command generation is safe. */ \
        bool operand_destination_hazard = false; \
        if(OPERAND_TYPE_NAME == BitAlu_A || \
           OPERAND_TYPE_NAME == BitAlu_B || \
           OPERAND_TYPE_NAME == SerDiv_A || \
           OPERAND_TYPE_NAME == SerDiv_B) \
            operand_destination_hazard = \
                venus_hazard_table->vd1_hazard_table[cur_id][p_id]; \
        else if(OPERAND_TYPE_NAME == CAU_A || \
                OPERAND_TYPE_NAME == CAU_B || \
                OPERAND_TYPE_NAME == CAU_C || \
                OPERAND_TYPE_NAME == CAU_D) \
            operand_destination_hazard = \
                venus_hazard_table->vd1_hazard_table[cur_id][p_id] || \
                venus_hazard_table->vd2_hazard_table[cur_id][p_id]; \
        const bool operand_requester_hazard = \
            p_id != cur_id && operand_used && \
            (operand_raw_hazard || operand_destination_hazard); \
        RUNNING_PKT_NAME->chain_raw_hazard[p_id] = \
            operand_requester_hazard; \
        RUNNING_PKT_NAME->chain_raw_source_dependency[p_id] = \
            operand_raw_hazard; \
        RUNNING_PKT_NAME->chain_raw_credit[p_id] = false; \
        RUNNING_PKT_NAME->chain_raw_credit_pipe[p_id] = 0; \
        RUNNING_PKT_NAME->chain_raw_credit_visible_tick[p_id] = MaxTick; \
        const int producer_instr = \
            venus_hazard_table->running_id_to_vns_instr_id[p_id]; \
        const bool tagged_producer_matches = \
            RUNNING_PKT_NAME->chain_raw_producer_instr[p_id] == \
                producer_instr; \
        if(!tagged_producer_matches) { \
            RUNNING_PKT_NAME->chain_raw_producer_is_lsu[p_id] = \
                locallane_vinsn_is_lsu[p_id]; \
            RUNNING_PKT_NAME->chain_raw_producer_vfu[p_id] = \
                locallane_vinsn_vfu[p_id]; \
        } \
        /* venus_lane_sequencer.sv tags every ordinary arithmetic operand \
         * requester with VFU_NONE when the instruction reads or writes the \
         * mask.  Such a requester still latches its complete hazard bitmap, \
         * but no producer writeback may turn that bitmap into a chaining \
         * credit.  The mask requester itself retains its separately tagged \
         * target VFU. */ \
        const bool requester_chaining_disabled = \
            (OPERAND_TYPE_NAME != Mask) && \
            (RUNNING_PKT_NAME->vm_r || RUNNING_PKT_NAME->vm_w); \
        const VFU consumer_vfu = requester_chaining_disabled ? VFU_NONE : \
            (RUNNING_PKT_NAME->vfu_lst.empty() ? \
                VFU_NONE : RUNNING_PKT_NAME->vfu_lst.front()); \
        const VFU producer_vfu = \
            RUNNING_PKT_NAME->chain_raw_producer_vfu[p_id]; \
        /* venus_operand_requester.sv carries source and destination \
         * dependencies in the same requester_q.hazard bitmap.  Its \
         * raw_hazard_counter is fed by every tagged producer writeback, \
         * including a destination-only WAR/WAW predecessor.  Restricting \
         * credit eligibility to a source RAW dependency serializes operand \
         * reads behind destination retirement and loses the row-by-row \
         * cross-VFU chain. */ \
        /* venus_mask_operand_requester captures a command and its hazard in \
         * IDLE exactly like the ordinary requesters, but unlike \
         * venus_operand_requester it has no raw_hazard_counter/writeback \
         * chaining path.  It must wait for the registered global/local \
         * retirement clear after admission. */ \
        RUNNING_PKT_NAME->chain_raw_credit_eligible[p_id] = \
            rtlChainingEnabled && \
            operand_requester_hazard && \
            OPERAND_TYPE_NAME != Mask && \
            !locallane_vinsn_is_lsu[cur_id] && \
            !RUNNING_PKT_NAME->chain_raw_producer_is_lsu[p_id] && \
            consumer_vfu != VFU_NONE && \
            producer_vfu != VFU_NONE && \
            consumer_vfu != producer_vfu && \
            consumer_vfu != VFU_ShuffleUnit && \
            producer_vfu != VFU_ShuffleUnit; \
        RUNNING_PKT_NAME->chain_raw_pipeline_active = \
            RUNNING_PKT_NAME->chain_raw_pipeline_active || \
            RUNNING_PKT_NAME->chain_raw_credit_eligible[p_id]; \
        updateVectorWriterBoundary(p_id); \
        /* requester_q is loaded on the same edge as vinsn_writeback_q.  If \
         * a producer pulse became Q-visible on this exact edge, the new \
         * requester observes it in REQUESTING combinational logic and \
         * captures raw_hazard_counter on the following edge.  Treating the \
         * current Q count as already seen drops that pulse and waits for an \
         * unrelated later writeback. */ \
        const uint64_t writer_q_count = \
            locallane_writer_grant_count_q[p_id]; \
        const bool writer_pulse_on_capture_edge = \
            writer_q_count != 0 && \
            locallane_writer_q_tick[p_id] == curTick(); \
        RUNNING_PKT_NAME->chain_raw_seen_progress[p_id] = \
            writer_q_count - (writer_pulse_on_capture_edge ? 1 : 0); \
        RUNNING_PKT_NAME->chain_raw_producer_instr[p_id] = \
            producer_instr; \
        DPRINTF(LaneOperandRequester, \
                "requester RAW snapshot lane %d operand %s instr %d/rid %d " \
                "producer instr %d/rid %d consumer-vfu %d producer-vfu %d " \
                "raw=%d destination=%d tagged=%d source=%d credit=%d\n", \
                lane_id, #OPERAND_TYPE_NAME, \
                RUNNING_PKT_NAME->vns_instr_id, \
                RUNNING_PKT_NAME->running_id, producer_instr, p_id, \
                static_cast<int>(consumer_vfu), \
                static_cast<int>(producer_vfu), operand_raw_hazard, \
                operand_destination_hazard, operand_requester_hazard, \
                RUNNING_PKT_NAME->chain_raw_source_dependency[p_id], \
                RUNNING_PKT_NAME->chain_raw_credit_eligible[p_id]); \
        const bool requester_war_hazard = \
            venus_hazard_table->war_hazard_table[cur_id][p_id]; \
        const bool requester_waw_hazard = \
            venus_hazard_table->waw_hazard_table[cur_id][p_id]; \
        RUNNING_PKT_NAME->chain_war_hazard[p_id] |= \
            requester_war_hazard; \
        RUNNING_PKT_NAME->chain_waw_hazard[p_id] |= \
            requester_waw_hazard; \
        if(requester_war_hazard || requester_waw_hazard) \
            RUNNING_PKT_NAME->chain_retire_victim_instr[p_id] = \
                venus_hazard_table->running_id_to_vns_instr_id[p_id]; \
    } \
    delete FIFO_NAME.front(); \
    FIFO_NAME.pop_front(); \
    if (experimentalOneEntryOperandCommands && operand_used && \
        (OPERAND_TYPE_NAME == BitAlu_A || \
         OPERAND_TYPE_NAME == BitAlu_B || \
         OPERAND_TYPE_NAME == CAU_A || OPERAND_TYPE_NAME == CAU_B || \
         OPERAND_TYPE_NAME == CAU_C || OPERAND_TYPE_NAME == CAU_D || \
         OPERAND_TYPE_NAME == SerDiv_A || \
         OPERAND_TYPE_NAME == SerDiv_B || \
         OPERAND_TYPE_NAME == Mask)) { \
        operandCommandAckVisibleTick[OPERAND_TYPE_NAME] = \
            afterCycles(Cycles(1)); \
    } \
    DPRINTF(LaneVFU, \
            "operand requester admit type %d instr %d/rid %d\n", \
            OPERAND_TYPE_NAME, RUNNING_PKT_NAME->vns_instr_id, \
            RUNNING_PKT_NAME->running_id); \
    return true; \
} else {  \
    /*DPRINTF(LaneSequencer, "%s insn does not start, instr %d with runningID %d, hazard stat is:%s\n",#OPERAND_TYPE_NAME, FIFO_NAME.front()->vns_instr_id, FIFO_NAME.front()->running_id, venus_hazard_table->reportHazardStat(FIFO_NAME.front()->running_id)); */\
    return false; \
}


bool VenusLane::laneSequencerPopFIFOforOperandRequester(OPERANDTYPE OperandType)
{
    switch (OperandType) {
        case BitAlu_A:    { __GENlaneSequencerPopFIFO_Name__(BitAlu_A   , queueing_bitaluA_instr_pkt, bitaluA_instr_FIFO_lanseq ); break; }
        case BitAlu_B:    { __GENlaneSequencerPopFIFO_Name__(BitAlu_B   , queueing_bitaluB_instr_pkt, bitaluB_instr_FIFO_lanseq ); break; }
        case CAU_A:       { __GENlaneSequencerPopFIFO_Name__(CAU_A      , queueing_cauA_instr_pkt   , cauA_instr_FIFO_lanseq    ); break; }
        case CAU_B:       { __GENlaneSequencerPopFIFO_Name__(CAU_B      , queueing_cauB_instr_pkt   , cauB_instr_FIFO_lanseq    ); break; }
        case CAU_C:       { __GENlaneSequencerPopFIFO_Name__(CAU_C      , queueing_cauC_instr_pkt   , cauC_instr_FIFO_lanseq    ); break; }
        case CAU_D:       { __GENlaneSequencerPopFIFO_Name__(CAU_D      , queueing_cauD_instr_pkt   , cauD_instr_FIFO_lanseq    ); break; }
        case SerDiv_A:    { __GENlaneSequencerPopFIFO_Name__(SerDiv_A   , queueing_serdivA_instr_pkt, serdivA_instr_FIFO_lanseq ); break; }
        case SerDiv_B:    { __GENlaneSequencerPopFIFO_Name__(SerDiv_B   , queueing_serdivB_instr_pkt, serdivB_instr_FIFO_lanseq ); break; }
        case Mask:        { __GENlaneSequencerPopFIFO_Name__(Mask       , queueing_mask_instr_pkt   , mask_instr_FIFO_lanseq    ); break; }
        case ShuffleUnit: { __GENlaneSequencerPopFIFO_Name__(ShuffleUnit, queueing_shuffle_instr_pkt, shuffle_instr_FIFO_lanseq ); break; }
        default: panic("unknown OperandType");
    }
}

#define CONCAT(text1,text2) text1##text2

#define __GENoperandRequesterGetVectorData_Name__(OPERAND_TYPE_NAME, OPERAND_HEAD, OPERAND_USED, OPERAND_LENGTH, FIFO_NAME, RUNNING_PKT_NAME, BUSY_NAME, OPEREQ_EVENT_NAME, INSTR_TOPUSH, DATA_TOPUSH, DATA_READY_COUNTER, DATA_TOREADFROM_VRF, DATA_FROM_VRF_ARRIVED) \
if(experimentalRequesterQVisibility && \
   OPERAND_TYPE_NAME >= BitAlu_A && OPERAND_TYPE_NAME <= CAU_D) { \
    serviceDrainingOperandResponses(OPERAND_TYPE_NAME); \
    if(BUSY_NAME && RUNNING_PKT_NAME != nullptr && \
       RUNNING_PKT_NAME->operand_issue_counter >= \
           RUNNING_PKT_NAME->OPERAND_LENGTH && \
       curTick() >= operandRequesterHandoffVisibleTick \
           [OPERAND_TYPE_NAME]) { \
        panic_if(operandRequesterDrainingPkt[OPERAND_TYPE_NAME] != nullptr, \
                 "operand requester %d completed a second command while " \
                 "the prior tagged response still drains", \
                 OPERAND_TYPE_NAME); \
        DPRINTF(LaneVFU, \
                "operand requester grant-driven release type %d instr " \
                "%d/rid %d issued %d/%d\n", \
                OPERAND_TYPE_NAME, RUNNING_PKT_NAME->vns_instr_id, \
                RUNNING_PKT_NAME->running_id, \
                RUNNING_PKT_NAME->operand_issue_counter, \
                RUNNING_PKT_NAME->OPERAND_LENGTH); \
        operandRequesterDrainingPkt[OPERAND_TYPE_NAME] = RUNNING_PKT_NAME; \
        operandRequesterDrainingReadyCount[OPERAND_TYPE_NAME] = \
            RUNNING_PKT_NAME->DATA_READY_COUNTER; \
        RUNNING_PKT_NAME = nullptr; \
        BUSY_NAME = false; \
        DATA_TOPUSH = false; \
        DATA_TOREADFROM_VRF = false; \
        DATA_FROM_VRF_ARRIVED = false; \
        serviceDrainingOperandResponses(OPERAND_TYPE_NAME); \
    } \
} \
if(BUSY_NAME == false) \
/*没有正在进行的任务，获取新任务*/ \
{ \
    if(laneSequencerPopFIFOforOperandRequester(OPERAND_TYPE_NAME) == false) { \
        /*获取新任务失败*/ \
        BUSY_NAME = false; \
        DATA_TOPUSH = false; \
        INSTR_TOPUSH = false; \
        /*如果FIFO中没有已经处理完的指令，就不再继续主动POP FIFO，而是等待SEQUENCER PUSH FIFO后调度*/ \
        if(FIFO_NAME.size() > 0) \
            schedule(OPEREQ_EVENT_NAME, afterCycles(Cycles(1))); \
        return; \
    } else { \
        /*获取新任务成功*/ \
        DPRINTF(LaneOperandRequester, "operandRequester got a new instruction: instruction %d with runningID %d local_vl: %d\n",RUNNING_PKT_NAME->vns_instr_id, RUNNING_PKT_NAME->running_id, RUNNING_PKT_NAME->OPERAND_LENGTH); \
        RUNNING_PKT_NAME->DATA_READY_COUNTER = 0; \
        RUNNING_PKT_NAME->operand_issue_counter = 0; \
        /* This callback is requester_q's capture edge in the experimental \
         * model.  REQUESTING is combinational from the newly captured Q \
         * state, so its first stable bank request is visible after the same \
         * edge; adding another cycle double-counts requester_q. */ \
        RUNNING_PKT_NAME->operand_request_visible_tick = \
            std::max(curTick(), \
                     operandRequesterHandoffVisibleTick \
                         [OPERAND_TYPE_NAME]); \
        operandRequesterHandoffVisibleTick[OPERAND_TYPE_NAME] = 0; \
        /*获取新任务成功，计数归0*/ \
        BUSY_NAME = true; \
        DPRINTF(LaneOperandRequester, "operandRequester%s, turns into busy.\n", #OPERAND_TYPE_NAME); \
        DATA_TOPUSH = false; \
        DATA_TOREADFROM_VRF = false; \
        DATA_FROM_VRF_ARRIVED = false; \
        INSTR_TOPUSH = true; \
        /*schedule(OPEREQ_EVENT_NAME,curTick()+1*1000);*/ \
        /*return;*/ \
    } \
} \
/* requester_q and requester_d advance on every lane edge, independently of \
 * operand-queue response/backpressure.  Mature the prior D token and sample \
 * the registered writer pulse before any data-path branch can return early. \
 * A later same-edge read grant still clears both states in \
 * commitOperandReadGrant, matching the RTL clear-wins assignment order. */ \
if(experimentalRequesterQVisibility && \
   OPERAND_TYPE_NAME >= BitAlu_A && \
   OPERAND_TYPE_NAME <= SerDiv_B && \
   RUNNING_PKT_NAME->chain_raw_pipeline_active) { \
    for(int p_id = 0; p_id < NrIDs; p_id++) { \
        if(!RUNNING_PKT_NAME->chain_raw_hazard[p_id]) \
            continue; \
        if(RUNNING_PKT_NAME->chain_raw_credit_pipe[p_id] == 1 && \
           curTick() >= \
               RUNNING_PKT_NAME->chain_raw_credit_visible_tick[p_id]) { \
            RUNNING_PKT_NAME->chain_raw_credit[p_id] = true; \
            RUNNING_PKT_NAME->chain_raw_credit_pipe[p_id] = 0; \
            RUNNING_PKT_NAME->chain_raw_credit_visible_tick[p_id] = \
                MaxTick; \
        } \
        updateVectorWriterBoundary(p_id); \
        const uint64_t producer_progress = \
            locallane_writer_grant_count_q[p_id]; \
        if(producer_progress > \
           RUNNING_PKT_NAME->chain_raw_seen_progress[p_id]) { \
            RUNNING_PKT_NAME->chain_raw_seen_progress[p_id] = \
                producer_progress; \
            const Addr requester_base = \
                RUNNING_PKT_NAME->OPERAND_HEAD * NrBankPerLane; \
            if(requester_base < locallane_writer_vaddr_q[p_id]) { \
                const Tick credit_visible = \
                    locallane_writer_q_tick[p_id] + clockPeriod(); \
                if(curTick() >= credit_visible) { \
                    RUNNING_PKT_NAME->chain_raw_credit[p_id] = true; \
                    RUNNING_PKT_NAME->chain_raw_credit_pipe[p_id] = 0; \
                    RUNNING_PKT_NAME->chain_raw_credit_visible_tick[p_id] = \
                        MaxTick; \
                } else { \
                    RUNNING_PKT_NAME->chain_raw_credit_pipe[p_id] = 1; \
                    RUNNING_PKT_NAME->chain_raw_credit_visible_tick[p_id] = \
                        credit_visible; \
                } \
            } \
        } \
        DPRINTF(LaneOperandRequester, \
                "requester token sample lane %d operand %s instr %d/rid %d " \
                "producer instr %d/rid %d hazard %d eligible %d credit %d " \
                "pipe %d seen %llu writer_valid %d writer_vfu %d " \
                "writer_vaddr %llu writer_count %llu writer_tick %llu " \
                "requester_base %llu\n", \
                lane_id, #OPERAND_TYPE_NAME, \
                RUNNING_PKT_NAME->vns_instr_id, \
                RUNNING_PKT_NAME->running_id, \
                RUNNING_PKT_NAME->chain_raw_producer_instr[p_id], p_id, \
                RUNNING_PKT_NAME->chain_raw_hazard[p_id], \
                RUNNING_PKT_NAME->chain_raw_credit_eligible[p_id], \
                RUNNING_PKT_NAME->chain_raw_credit[p_id], \
                RUNNING_PKT_NAME->chain_raw_credit_pipe[p_id], \
                static_cast<unsigned long long>( \
                    RUNNING_PKT_NAME->chain_raw_seen_progress[p_id]), \
                locallane_writer_valid_q[p_id], \
                locallane_writer_vfu_q[p_id], \
                static_cast<unsigned long long>( \
                    locallane_writer_vaddr_q[p_id]), \
                static_cast<unsigned long long>(producer_progress), \
                static_cast<unsigned long long>( \
                    locallane_writer_q_tick[p_id]), \
                static_cast<unsigned long long>( \
                    RUNNING_PKT_NAME->OPERAND_HEAD * NrBankPerLane)); \
    } \
} \
if(INSTR_TOPUSH == true) { \
    if(operandQueuePushinstrFIFO(OPERAND_TYPE_NAME, RUNNING_PKT_NAME) == false) \
    /*push cmd*/ \
    { \
        /*push fail*/ \
        DPRINTF(LaneOperandRequester, "operandRequester%s , is trying to push instruction %d into operandQueue. However, it got an nack.\n", #OPERAND_TYPE_NAME, RUNNING_PKT_NAME->vns_instr_id); \
        INSTR_TOPUSH = true; \
        if(!OPEREQ_EVENT_NAME.scheduled()) \
            schedule(OPEREQ_EVENT_NAME, afterCycles(Cycles(1))); \
        else \
            panic("schedule(OPEREQ_EVENT_NAME,curTick()+1*1000);"); \
        return; \
    } else { \
        /*push success*/ \
        DPRINTF(LaneOperandRequester, "operandRequester%s, successfully pushed instruction %d into operandQueue.\n", #OPERAND_TYPE_NAME, RUNNING_PKT_NAME->vns_instr_id); \
        INSTR_TOPUSH = false; \
    } \
} \
if(DATA_TOPUSH != true && \
   (RUNNING_PKT_NAME->chain_raw_pipeline_active || \
    (experimentalRequesterQVisibility && \
     ((OPERAND_TYPE_NAME >= BitAlu_A && OPERAND_TYPE_NAME <= CAU_D) || \
      OPERAND_TYPE_NAME == Mask)))) { \
    const int response_id = \
        OPERAND_TYPE_NAME == Mask ? 10 : \
        (OPERAND_TYPE_NAME == ShuffleUnit ? 9 : \
         static_cast<int>(OPERAND_TYPE_NAME) + 1); \
    unsigned int tagged_response_data = 0; \
    const int response_offset = (OPERAND_TYPE_NAME == Mask) ? \
        RUNNING_PKT_NAME->DATA_READY_COUNTER * 8 : \
        RUNNING_PKT_NAME->DATA_READY_COUNTER * \
            (RUNNING_PKT_NAME->vew + 1); \
    if(takeVrfReadResponse(response_id, RUNNING_PKT_NAME->running_id, \
                           RUNNING_PKT_NAME->vns_instr_id, \
                           response_offset, \
                           tagged_response_data)) { \
        CONCAT(readdata_fromVRF_buf_,OPERAND_TYPE_NAME) = \
            tagged_response_data; \
        /* Tagged response replaces the legacy wait latch. */ \
        DATA_TOREADFROM_VRF = false; \
        DATA_FROM_VRF_ARRIVED = false; \
        DATA_TOPUSH = true; \
    } \
} \
if(DATA_TOREADFROM_VRF == true) \
{ \
    if(DATA_FROM_VRF_ARRIVED == true) \
    { \
        DPRINTF(LaneOperandRequester, "operandRequester%s, VRF已经取回数据, DATA_TOREADFROM_VRF = %d, DATA_FROM_VRF_ARRIVED = %d, DATA_TOPUSH = %d\n",#OPERAND_TYPE_NAME , DATA_TOREADFROM_VRF, DATA_FROM_VRF_ARRIVED, DATA_TOPUSH); \
        /*VRF已经取回数据*/ \
        DATA_TOREADFROM_VRF = false; \
        DATA_FROM_VRF_ARRIVED = false; \
        DATA_TOPUSH = true; \
    } else { \
        /*VRF没有取回数据*/ \
        DPRINTF(LaneOperandRequester, "operandRequester%s, VRF没有取回数据, DATA_TOREADFROM_VRF = %d, DATA_FROM_VRF_ARRIVED = %d, DATA_TOPUSH = %d\n",#OPERAND_TYPE_NAME , DATA_TOREADFROM_VRF, DATA_FROM_VRF_ARRIVED, DATA_TOPUSH); \
        DATA_TOREADFROM_VRF = true; \
        DATA_FROM_VRF_ARRIVED = false; \
        DATA_TOPUSH = false; \
        if(!OPEREQ_EVENT_NAME.scheduled()) \
            schedule(OPEREQ_EVENT_NAME, afterCycles(Cycles(1))); \
        else \
            panic("schedule(OPEREQ_EVENT_NAME,curTick()+1*1000);"); \
        /*等待VRF取回数据*/ \
        return; \
    } \
} \
if(DATA_TOPUSH == true) { \
    /*如果有待写的数据，就去写入FIFO*/ \
    if (operandQueuePushdataFIFO(OPERAND_TYPE_NAME, CONCAT(readdata_fromVRF_buf_,OPERAND_TYPE_NAME), RUNNING_PKT_NAME) == false) \
    /*push data, suppose the data is 2025 TRICK*/ \
    { \
        /*写入失败*/ \
        DPRINTF(LaneOperandRequester, "operandRequester%s, tries to push the %dth/%d data of instr %d received from VRF into operandQueue. However, it got an nack.\n", #OPERAND_TYPE_NAME, RUNNING_PKT_NAME->DATA_READY_COUNTER, RUNNING_PKT_NAME->OPERAND_LENGTH, RUNNING_PKT_NAME->vns_instr_id); \
        DATA_TOPUSH = true; \
        if(!OPEREQ_EVENT_NAME.scheduled()) \
            schedule(OPEREQ_EVENT_NAME, afterCycles(Cycles(1))); \
        else \
            panic("schedule(OPEREQ_EVENT_NAME,curTick()+1*1000);"); \
        /*下一轮再尝试读取*/ \
        return; \
    } else { \
        /*写入成功*/ \
        DPRINTF(Lane, "operandRequester%s, successfully pushed the %dth/%d data of instr %d received from VRF into operandQueue. data is 0x%x\n", #OPERAND_TYPE_NAME, RUNNING_PKT_NAME->DATA_READY_COUNTER, RUNNING_PKT_NAME->OPERAND_LENGTH, RUNNING_PKT_NAME->vns_instr_id, CONCAT(readdata_fromVRF_buf_,OPERAND_TYPE_NAME)); \
        DATA_TOPUSH = false; \
        RUNNING_PKT_NAME->DATA_READY_COUNTER += \
            (OPERAND_TYPE_NAME == Mask) ? 1 : \
                (2 - RUNNING_PKT_NAME->vew); \
        /*ew8:+1 ew16:+2*/ \
        \
    } \
} \
if(DATA_TOPUSH != true) { \
    /*如果没有待写的数据，就去请求读取数据*/ \
    const int request_step = (OPERAND_TYPE_NAME == Mask) ? 1 : \
        (2 - RUNNING_PKT_NAME->vew); \
    const int outstanding_reads = \
        (RUNNING_PKT_NAME->operand_issue_counter - \
         RUNNING_PKT_NAME->DATA_READY_COUNTER) / request_step; \
    const bool requester_operand_used = (OPERAND_TYPE_NAME == Mask) ? \
        (RUNNING_PKT_NAME->vm_r || RUNNING_PKT_NAME->vm_w) : \
        (RUNNING_PKT_NAME->OPERAND_USED == 1); \
    if(requester_operand_used && \
       RUNNING_PKT_NAME->operand_issue_counter < \
           RUNNING_PKT_NAME->OPERAND_LENGTH && \
       outstanding_reads < \
           ((RUNNING_PKT_NAME->chain_raw_pipeline_active || \
             (experimentalRequesterQVisibility && \
              OPERAND_TYPE_NAME >= CAU_A && \
              OPERAND_TYPE_NAME <= CAU_D) || \
             (experimentalRequesterQVisibility && \
              OPERAND_TYPE_NAME == Mask)) ? 2 : 1)) { \
        if(curTick() < RUNNING_PKT_NAME->operand_request_visible_tick) { \
            if(!OPEREQ_EVENT_NAME.scheduled()) \
                schedule(OPEREQ_EVENT_NAME, \
                         RUNNING_PKT_NAME->operand_request_visible_tick); \
            return; \
        } \
        /* Check the requester-local, one-bit, non-accumulating RAW credit. */ \
        int blocked_producer_id = -1; \
        bool p_ready = true; \
        const int requested_row = \
            RUNNING_PKT_NAME->operand_issue_counter / request_step; \
        for(int p_id = 0; p_id < NrIDs && \
            RUNNING_PKT_NAME->chain_overlap_active; p_id++) { \
            if(!RUNNING_PKT_NAME->chain_raw_hazard[p_id]) \
                continue; \
            const Tick requesterRetirementVisible = \
                locallane_retirement_visible_tick[p_id]; \
            const VFU requester_vfu = \
                RUNNING_PKT_NAME->vfu_lst.empty() ? VFU_NONE : \
                    RUNNING_PKT_NAME->vfu_lst.front(); \
            const VFU producer_vfu = \
                RUNNING_PKT_NAME->chain_raw_producer_vfu[p_id]; \
            const bool command_wide_same_vfu_raw = \
                experimentalRequesterQVisibility && \
                RUNNING_PKT_NAME->chain_raw_source_dependency[p_id] && \
                !RUNNING_PKT_NAME->chain_raw_producer_is_lsu[p_id] && \
                requester_vfu != VFU_NONE && \
                producer_vfu != VFU_NONE && \
                producer_vfu != VFU_ShuffleUnit && \
                requester_vfu == producer_vfu; \
            /* A short producer is dispatched only to its active lane \
             * prefix.  A wider dependent command is nevertheless admitted \
             * by every one of its lanes with the global RAW snapshot.  RTL \
             * lets a lane which never accepted the tagged producer consume \
             * the registered all-lane command completion; waiting for that \
             * lane's local generation change adds three tile cycles.  Keep \
             * active producer lanes on their local cross-VFU chaining path, \
             * and exclude LSU/Shuffle from this arithmetic sideband. */ \
            const bool command_wide_inactive_lane_raw = \
                experimentalRequesterQVisibility && \
                RUNNING_PKT_NAME->chain_raw_source_dependency[p_id] && \
                !RUNNING_PKT_NAME->chain_raw_producer_is_lsu[p_id] && \
                requester_vfu != VFU_NONE && \
                producer_vfu != VFU_NONE && \
                requester_vfu != VFU_ShuffleUnit && \
                producer_vfu != VFU_ShuffleUnit && \
                locallane_accepted_vns_instr_id[p_id] != \
                    RUNNING_PKT_NAME->chain_raw_producer_instr[p_id]; \
            /* Venus1 has no row-chaining path.  For every ordinary lane \
             * arithmetic producer, pe_resp_i_q masks vinsn_running_d and \
             * therefore clears all of that producer's global hazard bits \
             * at one tagged all-lane boundary.  This completion clear does \
             * not traverse the three-stage new-instruction hazard-compute \
             * pipeline.  Shuffle and LSU have distinct completion FSMs and \
             * intentionally remain on their dedicated paths.  Venus2 keeps \
             * its row-credit behavior unchanged. */ \
            const bool v1_lane_arithmetic_retirement = \
                experimentalRequesterQVisibility && \
                !rtlChainingEnabled && \
                !RUNNING_PKT_NAME->chain_raw_producer_is_lsu[p_id] && \
                producer_vfu != VFU_NONE && \
                producer_vfu != VFU_ShuffleUnit; \
            /* Shuffle is a global, non-lane producer.  In Venus1 its \
             * pe_resp clears global_hazard_table_d in the physical done \
             * cycle, and the lane requester captures that clear in \
             * requester_q on the following edge.  The generation-tagged \
             * producer-completion sideband models exactly those two \
             * boundaries.  Waiting for the separately pipelined full-table \
             * broadcast adds two spurious edges to every Shuffle-to-lane \
             * RAW handoff. */ \
            const bool v1_shuffle_retirement = \
                experimentalRequesterQVisibility && \
                !rtlChainingEnabled && \
                !RUNNING_PKT_NAME->chain_raw_producer_is_lsu[p_id] && \
                producer_vfu == VFU_ShuffleUnit; \
            const bool command_wide_retirement = \
                command_wide_same_vfu_raw || \
                command_wide_inactive_lane_raw || \
                v1_lane_arithmetic_retirement; \
            const Tick retirement_visible = command_wide_retirement ? \
                locallane_command_retirement_visible_tick[p_id] : \
                requesterRetirementVisible; \
            const int retired_generation = command_wide_retirement ? \
                locallane_command_retired_vns_instr_id[p_id] : \
                locallane_retired_vns_instr_id[p_id]; \
            /* Instruction generations are monotonic while running IDs are
             * reused.  A requester can remain resident after the producer's
             * ID has completed one or more younger generations.  Seeing a
             * retired generation newer than the captured producer proves
             * the captured generation retired as well; requiring equality
             * turns legal ID reuse into a permanent RAW dependency. */ \
            const bool tagged_retirement_visible = \
                RUNNING_PKT_NAME->chain_raw_producer_instr[p_id] >= 0 && \
                retired_generation >= \
                    RUNNING_PKT_NAME->chain_raw_producer_instr[p_id] && \
                curTick() >= retirement_visible; \
            /* Venus1 may observe only the all-lane sequencer retirement; \
             * Venus2 may additionally use its qualified local/chaining \
             * retirement paths selected above. */ \
            if((rtlChainingEnabled || command_wide_retirement || \
                v1_shuffle_retirement) && \
               tagged_retirement_visible) { \
                DPRINTF(LaneOperandRequester, \
                        "requester RAW clear lane %d operand %s instr " \
                        "%d/rid %d producer instr %d/rid %d reason " \
                        "tagged-retirement visible %llu\n", \
                        lane_id, #OPERAND_TYPE_NAME, \
                        RUNNING_PKT_NAME->vns_instr_id, \
                        RUNNING_PKT_NAME->running_id, \
                        RUNNING_PKT_NAME->chain_raw_producer_instr[p_id], \
                        p_id, static_cast<unsigned long long>( \
                            retirement_visible)); \
                RUNNING_PKT_NAME->chain_raw_hazard[p_id] = false; \
                RUNNING_PKT_NAME->chain_raw_credit[p_id] = false; \
                RUNNING_PKT_NAME->chain_raw_credit_pipe[p_id] = 0; \
                RUNNING_PKT_NAME->chain_raw_credit_visible_tick[p_id] = \
                    MaxTick; \
                continue; \
            } \
            if(rtlChainingEnabled && \
               venus_hazard_table->running_id_to_vns_instr_id[p_id] != \
               RUNNING_PKT_NAME->chain_raw_producer_instr[p_id]) { \
                DPRINTF(LaneOperandRequester, \
                        "requester RAW clear lane %d operand %s instr " \
                        "%d/rid %d producer instr %d/rid %d reason " \
                        "generation-change observed %d\n", \
                        lane_id, #OPERAND_TYPE_NAME, \
                        RUNNING_PKT_NAME->vns_instr_id, \
                        RUNNING_PKT_NAME->running_id, \
                        RUNNING_PKT_NAME->chain_raw_producer_instr[p_id], \
                        p_id, venus_hazard_table-> \
                            running_id_to_vns_instr_id[p_id]); \
                RUNNING_PKT_NAME->chain_raw_hazard[p_id] = false; \
                RUNNING_PKT_NAME->chain_raw_credit[p_id] = false; \
                RUNNING_PKT_NAME->chain_raw_credit_pipe[p_id] = 0; \
                RUNNING_PKT_NAME->chain_raw_credit_visible_tick[p_id] = \
                    MaxTick; \
                continue; \
            } \
            const bool global_hazard_clear = \
                !venus_hazard_table->global_hazard_table \
                    [RUNNING_PKT_NAME->running_id][p_id]; \
            /* Venus1 requester_d instances consume the same global-table \
             * broadcast in parallel.  Their requester_q clear is anchored \
             * to that shared broadcast edge in listenHazardTableRequest(), \
             * rather than to whichever C++ requester callback happens to \
             * inspect the bit first.  Venus2 keeps its qualified \
             * chaining/fallback path bit-for-bit unchanged. */ \
            const bool v1_registered_source_clear = \
                !rtlChainingEnabled && \
                RUNNING_PKT_NAME->chain_raw_source_dependency[p_id]; \
            if(v1_registered_source_clear && !command_wide_retirement) { \
                const Tick clearVisible = \
                    locallane_global_hazard_clear_visible_tick \
                        [RUNNING_PKT_NAME->running_id][p_id]; \
                if(global_hazard_clear && clearVisible != MaxTick && \
                   curTick() >= clearVisible) { \
                    RUNNING_PKT_NAME->chain_raw_hazard[p_id] = false; \
                    continue; \
                } \
            } \
            if((rtlChainingEnabled || \
                (!v1_registered_source_clear && \
                 !command_wide_retirement)) && \
               global_hazard_clear) { \
                DPRINTF(LaneOperandRequester, \
                        "requester RAW clear lane %d operand %s instr " \
                        "%d/rid %d producer instr %d/rid %d reason " \
                        "global-hazard-clear\n", \
                        lane_id, #OPERAND_TYPE_NAME, \
                        RUNNING_PKT_NAME->vns_instr_id, \
                        RUNNING_PKT_NAME->running_id, \
                        RUNNING_PKT_NAME->chain_raw_producer_instr[p_id], \
                        p_id); \
                RUNNING_PKT_NAME->chain_raw_hazard[p_id] = false; \
                RUNNING_PKT_NAME->chain_raw_credit[p_id] = false; \
                RUNNING_PKT_NAME->chain_raw_credit_pipe[p_id] = 0; \
                RUNNING_PKT_NAME->chain_raw_credit_visible_tick[p_id] = \
                    MaxTick; \
                continue; \
            } \
            /* A producer done pulse may remove the global hazard only after \
             * this requester has consumed a credit and is waiting for the \
             * following registered producer pulse (pipe=2).  Gating the \
             * completion boundary this way preserves the earlier CAU chain \
             * while matching requester_q clear-wins at the final row. */ \
            if(rtlChainingEnabled && \
               experimentalRequesterQVisibility && \
               (OPERAND_TYPE_NAME == BitAlu_A || \
                OPERAND_TYPE_NAME == BitAlu_B) && \
               RUNNING_PKT_NAME->chain_raw_credit_pipe[p_id] == 2 && \
               producerCompletionPending( \
                   p_id, \
                   RUNNING_PKT_NAME->chain_raw_producer_instr[p_id])) { \
                DPRINTF(LaneOperandRequester, \
                        "requester RAW clear lane %d operand %s instr " \
                        "%d/rid %d producer instr %d/rid %d reason " \
                        "producer-completion-pending\n", \
                        lane_id, #OPERAND_TYPE_NAME, \
                        RUNNING_PKT_NAME->vns_instr_id, \
                        RUNNING_PKT_NAME->running_id, \
                        RUNNING_PKT_NAME->chain_raw_producer_instr[p_id], \
                        p_id); \
                RUNNING_PKT_NAME->chain_raw_hazard[p_id] = false; \
                RUNNING_PKT_NAME->chain_raw_credit[p_id] = false; \
                RUNNING_PKT_NAME->chain_raw_credit_pipe[p_id] = 0; \
                RUNNING_PKT_NAME->chain_raw_credit_visible_tick[p_id] = \
                    MaxTick; \
                continue; \
            } \
            if(!RUNNING_PKT_NAME->chain_raw_credit_eligible[p_id]) { \
                p_ready = false; \
                blocked_producer_id = p_id; \
                break; \
            } \
            updateVectorWriterBoundary(p_id); \
            const uint64_t producer_progress = \
                locallane_writer_grant_count_q[p_id]; \
            const bool producer_grant = producer_progress > \
                RUNNING_PKT_NAME->chain_raw_seen_progress[p_id]; \
            if(!(experimentalRequesterQVisibility && \
                 OPERAND_TYPE_NAME >= BitAlu_A && \
                 OPERAND_TYPE_NAME <= SerDiv_B) && \
               producer_progress > \
                   RUNNING_PKT_NAME->chain_raw_seen_progress[p_id]) { \
                RUNNING_PKT_NAME->chain_raw_seen_progress[p_id] = \
                    producer_progress; \
            } \
            if(experimentalRequesterQVisibility) { \
                /* requester_d.raw_hazard_counter is set by every writeback \
                 * pulse for the hazardous running ID, independent of the \
                 * writer VFU.  The separately registered writer metadata \
                 * decides whether that retained one-bit token is eligible. \
                 * This matters across running-ID reuse: RTL hard-wires \
                 * Shuffle result IDs to zero, so a late Shuffle write can \
                 * seed the token without enabling a chain until a later \
                 * CAU write changes vinsn_writer_vfu_q. */ \
                if(!(OPERAND_TYPE_NAME >= BitAlu_A && \
                     OPERAND_TYPE_NAME <= SerDiv_B)) { \
                    const Addr requester_base = \
                        RUNNING_PKT_NAME->OPERAND_HEAD * NrBankPerLane; \
                    if(producer_grant && \
                       requester_base < locallane_writer_vaddr_q[p_id]) \
                        RUNNING_PKT_NAME->chain_raw_credit[p_id] = true; \
                } \
                const bool requester_chaining_disabled = \
                    (OPERAND_TYPE_NAME != Mask) && \
                    (RUNNING_PKT_NAME->vm_r || RUNNING_PKT_NAME->vm_w); \
                const VFU consumer_vfu = requester_chaining_disabled ? \
                    VFU_NONE : (RUNNING_PKT_NAME->vfu_lst.empty() ? \
                        VFU_NONE : RUNNING_PKT_NAME->vfu_lst.front()); \
                const VFU writer_vfu = \
                    locallane_writer_vfu_q[p_id]; \
                const bool writer_eligible = \
                    locallane_writer_valid_q[p_id] && \
                    consumer_vfu != VFU_NONE && \
                    writer_vfu != VFU_NONE && \
                    consumer_vfu != writer_vfu && \
                    consumer_vfu != VFU_ShuffleUnit && \
                    writer_vfu != VFU_ShuffleUnit; \
                if(!writer_eligible) { \
                    DPRINTF(LaneOperandRequester, \
                            "requester token block lane %d operand %s " \
                            "instr %d/rid %d producer instr %d/rid %d " \
                            "row %d reason writer-ineligible credit %d " \
                            "pipe %d writer_valid %d writer_vfu %d\n", \
                            lane_id, #OPERAND_TYPE_NAME, \
                            RUNNING_PKT_NAME->vns_instr_id, \
                            RUNNING_PKT_NAME->running_id, \
                            RUNNING_PKT_NAME->chain_raw_producer_instr[p_id], \
                            p_id, requested_row, \
                            RUNNING_PKT_NAME->chain_raw_credit[p_id], \
                            RUNNING_PKT_NAME->chain_raw_credit_pipe[p_id], \
                            locallane_writer_valid_q[p_id], writer_vfu); \
                    p_ready = false; \
                    blocked_producer_id = p_id; \
                    break; \
                } \
            } else { \
                const bool requester_grant = \
                    (RUNNING_PKT_NAME->chain_raw_credit_pipe[p_id] & 0x1) != 0; \
                RUNNING_PKT_NAME->chain_raw_credit_pipe[p_id] = \
                    ((producer_grant && producer_progress > requested_row) ? \
                        0x1 : 0x0); \
                if(!RUNNING_PKT_NAME->chain_raw_credit[p_id] && \
                   requester_grant) { \
                    RUNNING_PKT_NAME->chain_raw_credit[p_id] = true; \
                } \
            } \
            if(!RUNNING_PKT_NAME->chain_raw_credit[p_id]) { \
                DPRINTF(LaneOperandRequester, \
                        "requester token block lane %d operand %s instr " \
                        "%d/rid %d producer instr %d/rid %d row %d " \
                        "reason no-credit eligible %d pipe %d seen %llu " \
                        "writer_count %llu writer_vaddr %llu\n", \
                        lane_id, #OPERAND_TYPE_NAME, \
                        RUNNING_PKT_NAME->vns_instr_id, \
                        RUNNING_PKT_NAME->running_id, \
                        RUNNING_PKT_NAME->chain_raw_producer_instr[p_id], \
                        p_id, requested_row, \
                        RUNNING_PKT_NAME->chain_raw_credit_eligible[p_id], \
                        RUNNING_PKT_NAME->chain_raw_credit_pipe[p_id], \
                        static_cast<unsigned long long>( \
                            RUNNING_PKT_NAME->chain_raw_seen_progress[p_id]), \
                        static_cast<unsigned long long>(producer_progress), \
                        static_cast<unsigned long long>( \
                            locallane_writer_vaddr_q[p_id])); \
                p_ready = false; \
                blocked_producer_id = p_id; \
                break; \
            } \
        } \
        if(!p_ready) { \
                DPRINTF(Lane, "Hazard-class operand issue: requester%s instr %d/rid %d stalls for producer instr %d/rid %d (row %d, progress %d, credit eligible %d)\n", #OPERAND_TYPE_NAME, RUNNING_PKT_NAME->vns_instr_id, RUNNING_PKT_NAME->running_id, RUNNING_PKT_NAME->chain_raw_producer_instr[blocked_producer_id], blocked_producer_id, requested_row, locallane_write_vinsn_progress[blocked_producer_id], RUNNING_PKT_NAME->chain_raw_credit_eligible[blocked_producer_id]); \
             if(!OPEREQ_EVENT_NAME.scheduled()) \
                 schedule(OPEREQ_EVENT_NAME, afterCycles(Cycles(1))); \
             return; \
        } \
        /* operand_queue_ready_o is driven by registered issued occupancy, \
         * not by the returned-data deque.  A full queue therefore remains \
         * unavailable throughout an edge even if its VFU pops on that same \
         * edge. */ \
        if(experimentalRequesterQVisibility && \
           operandRequesterLastGrantTick[OPERAND_TYPE_NAME] == curTick()) { \
            DPRINTF(LaneOperandRequester, \
                    "operandRequester%s instr %d/rid %d stalls after " \
                    "same-edge tagged grant\n", \
                    #OPERAND_TYPE_NAME, RUNNING_PKT_NAME->vns_instr_id, \
                    RUNNING_PKT_NAME->running_id); \
            if(!OPEREQ_EVENT_NAME.scheduled()) \
                schedule(OPEREQ_EVENT_NAME, afterCycles(Cycles(1))); \
            return; \
        } \
        if(experimentalRequesterQVisibility && \
           !operandQueueReadyForIssue(OPERAND_TYPE_NAME)) { \
            DPRINTF(LaneOperandRequester, \
                    "operandRequester%s instr %d/rid %d stalls for " \
                    "registered operand queue full\n", \
                    #OPERAND_TYPE_NAME, RUNNING_PKT_NAME->vns_instr_id, \
                    RUNNING_PKT_NAME->running_id); \
            if(!OPEREQ_EVENT_NAME.scheduled()) \
                schedule(OPEREQ_EVENT_NAME, afterCycles(Cycles(1))); \
            return; \
        } \
        /*如果当前操作数被用到且是向量*/ \
        const int operand_offset = (OPERAND_TYPE_NAME == Mask) ? \
            RUNNING_PKT_NAME->operand_issue_counter * 8 : \
            RUNNING_PKT_NAME->operand_issue_counter * \
                (RUNNING_PKT_NAME->vew + 1); \
        if( requestArbiter(RUNNING_PKT_NAME->OPERAND_HEAD, operand_offset, \
                           OPERAND_TYPE_NAME, INT_MIN, \
                           (OPERAND_TYPE_NAME == Mask) ? 8 : 2, \
                           RUNNING_PKT_NAME->running_id, \
                           RUNNING_PKT_NAME->vns_instr_id, \
                           RUNNING_PKT_NAME->chain_raw_pipeline_active || \
                               (experimentalRequesterQVisibility && \
                                (OPERAND_TYPE_NAME == BitAlu_A || \
                                 OPERAND_TYPE_NAME == BitAlu_B || \
                                 OPERAND_TYPE_NAME == Mask))) == false) \
        /*提供起始行号+当前计数，请求读取数据*/ \
        { \
            /*读取失败*/ \
            DPRINTF(Lane, "operandRequester%s, is trying to issue the %dth/%d data of instruction %d to VRF. However, it got an nack.\n", #OPERAND_TYPE_NAME, RUNNING_PKT_NAME->operand_issue_counter, RUNNING_PKT_NAME->OPERAND_LENGTH, RUNNING_PKT_NAME->vns_instr_id); \
            DATA_TOPUSH = false; \
            /*下一轮再尝试读取*/ \
            if(!OPEREQ_EVENT_NAME.scheduled()) \
                schedule(OPEREQ_EVENT_NAME, afterCycles(Cycles(1))); \
            else \
                panic("schedule(OPEREQ_EVENT_NAME,curTick()+1*1000);"); \
            return; \
        } else { \
            /*读取成功*/ \
            DPRINTF(Lane, "operandRequester%s, successfully issued the %dth/%d tagged read request of instruction %d to VRF.\n",#OPERAND_TYPE_NAME, RUNNING_PKT_NAME->operand_issue_counter, RUNNING_PKT_NAME->OPERAND_LENGTH, RUNNING_PKT_NAME->vns_instr_id); \
            commitOperandReadGrant(RUNNING_PKT_NAME, OPERAND_TYPE_NAME, \
                                   DATA_TOREADFROM_VRF, \
                                   DATA_FROM_VRF_ARRIVED); \
        } \
    } \
} \
if( RUNNING_PKT_NAME->DATA_READY_COUNTER >= RUNNING_PKT_NAME->OPERAND_LENGTH) \
/*检查当前指令有没有读完*/ \
{ \
    /*读完*/ \
    DPRINTF(LaneOperandRequester, "operandRequester%s, has sent all of the %d data request of instruction %d to vrf\n", #OPERAND_TYPE_NAME, RUNNING_PKT_NAME->OPERAND_LENGTH, RUNNING_PKT_NAME->vns_instr_id); \
    DPRINTF(LaneVFU, \
            "operand requester complete type %d instr %d/rid %d rows %d\n", \
            OPERAND_TYPE_NAME, RUNNING_PKT_NAME->vns_instr_id, \
            RUNNING_PKT_NAME->running_id, RUNNING_PKT_NAME->OPERAND_LENGTH); \
    BUSY_NAME = false; \
    /* The requester handoff edge was fixed by the final VRF grant.  The C++ \
     * owner pointer remains on the old tagged command only long enough to \
     * place its final response in the operand queue.  Do not move the \
     * already-recorded requester_q boundary forward to this response edge. */ \
    if(experimentalRequesterQVisibility && FIFO_NAME.size() > 0) { \
        DPRINTF(LaneVFU, \
                "operand requester fall-through handoff type %d after instr " \
                "%d/rid %d\n", \
                OPERAND_TYPE_NAME, RUNNING_PKT_NAME->vns_instr_id, \
                RUNNING_PKT_NAME->running_id); \
        if(operandRequesterHandoffVisibleTick[OPERAND_TYPE_NAME] == 0) \
            operandRequesterHandoffVisibleTick[OPERAND_TYPE_NAME] = \
                afterCycles(Cycles(1)); \
        operandRequesterGetVectorData(OPERAND_TYPE_NAME); \
        return; \
    } \
    if(!OPEREQ_EVENT_NAME.scheduled()) \
        schedule(OPEREQ_EVENT_NAME, afterCycles(Cycles(1))); \
    else \
        panic("schedule(OPEREQ_EVENT_NAME,curTick()+1*1000);"); \
} else { \
    /*没有读完*/ \
    if(!OPEREQ_EVENT_NAME.scheduled()) \
        schedule(OPEREQ_EVENT_NAME, afterCycles(Cycles(1))); \
    else \
        panic("schedule(OPEREQ_EVENT_NAME,curTick()+1*1000);"); \
}



bool VenusLane::VenusLaneToVrfRequestPort::recvTimingResp(PacketPtr pkt)
{
    DPRINTF(LaneVSPM, "Received packet with ID: %d\n", pkt->id);

    auto dataPtr = pkt->getPtr<uint8_t>();

    if (dataPtr && ((pkt->id >= 1 && pkt->id <= 8) || pkt->id == 10))
    {
        /*
         * venus_dspm registers the SRAM output/tag and venus_operand_queue
         * registers that tagged beat before a VFU can consume it.  Keep the
         * response tagged across both stages; a recycled running ID must not
         * publish an old response into the next requester's scalar latch.
         */
        auto *state = dynamic_cast<VrfReadResponseState *>(pkt->senderState);
        panic_if(state == nullptr,
                 "VRF read response %d has no generation tag", pkt->id);
        DPRINTF(LaneVFU,
                "operand VRF response type %d instr %d/rid %d offset %d "
                "pipelined %d\n",
                pkt->id - 1, state->vns_instr_id, state->running_id,
                state->operand_offset, state->pipelined);
        const bool registeredRequesterResponse =
            owner->experimentalRequesterQVisibility &&
            ((pkt->id >= 1 && pkt->id <= 6) || pkt->id == 10);
        if (!state->pipelined && !registeredRequesterResponse) {
            /* Preserve the accepted legacy boundary for units whose
             * SRAM-output oracle has not yet been split from VFU admission.
             * CAU uses the explicit registered response path below for both
             * ordinary VRF reads and cross-VFU RAW overlap. */
            pkt->senderState = nullptr;
            delete state;
        } else {
        unsigned int responseData = 0;
        if (pkt->id == 10) {
            panic_if(pkt->getSize() != 8,
                     "logical mask row response has size %u, expected 8",
                     pkt->getSize());
            for (unsigned bit = 0; bit < 8; ++bit)
                responseData |= (dataPtr[bit] & 0x1) << bit;
        } else {
            responseData = (dataPtr[1] << 8) | dataPtr[0];
        }
        PendingVrfReadResponse response{
            /*
             * The memory response represents the DSPM SRAM output.  RTL
             * captures it into the operand-queue data FIFO on the next lane
             * edge.  Publishing at clockEdge(1) keeps that registered
             * boundary distinct from the later VFU input handshake.
             */
            (owner->experimentalRequesterQVisibility &&
             state->pipelined && (pkt->id == 1 || pkt->id == 2)) ?
                curTick() : owner->clockEdge(Cycles(1)),
            pkt->id,
            state->running_id,
            state->vns_instr_id,
            state->operand_offset,
            responseData
        };
        owner->pendingVrfReadResponses.push_back(response);
        if (!owner->nextVrfReadResponseStageEvent.scheduled()) {
            owner->schedule(owner->nextVrfReadResponseStageEvent,
                            response.visible_tick);
        } else if (response.visible_tick <
                   owner->nextVrfReadResponseStageEvent.when()) {
            owner->reschedule(owner->nextVrfReadResponseStageEvent,
                              response.visible_tick);
        }
        pkt->senderState = nullptr;
        delete state;
        delete pkt;
        return true;
        }
    }

    if (dataPtr)
    {
        // 根据 packet ID 来决定不同的操作
        if (pkt->id == 1) // 如果 id 是 1
        {
            DPRINTF(LaneVSPM, "sram response to bitalu A: %d\n", static_cast<int>((dataPtr[1] << 8) | dataPtr[0]));
            owner->readdata_fromVRF_buf_BitAlu_A = (dataPtr[1] << 8) | dataPtr[0];
            owner->operandrequester_bitaluA_datafrom_VRF_arrived = true;
            delete pkt;
        }
        else if (pkt->id == 2) // 如果 id 是 2
        {
            DPRINTF(LaneVSPM, "sram response to bitalu B: %d\n", static_cast<int>((dataPtr[1] << 8) | dataPtr[0]));
            owner->readdata_fromVRF_buf_BitAlu_B = (dataPtr[1] << 8) | dataPtr[0];
            owner->operandrequester_bitaluB_datafrom_VRF_arrived = true;
            delete pkt;
        }
        else if (pkt->id == 3) // 如果 id 是 2
        {
            DPRINTF(LaneVSPM, "sram response to cau A: %d\n", static_cast<int>((dataPtr[1] << 8) | dataPtr[0]));
            owner->readdata_fromVRF_buf_CAU_A = (dataPtr[1] << 8) | dataPtr[0];
            owner->operandrequester_cauA_datafrom_VRF_arrived = true;
            delete pkt;
        }
        else if (pkt->id == 4) // 如果 id 是 2
        {
            DPRINTF(LaneVSPM, "sram response to cau B: %d\n", static_cast<int>((dataPtr[1] << 8) | dataPtr[0]));
            owner->readdata_fromVRF_buf_CAU_B = (dataPtr[1] << 8) | dataPtr[0];
            owner->operandrequester_cauB_datafrom_VRF_arrived = true;
            delete pkt;
        }
        else if (pkt->id == 5) // 如果 id 是 2
        {
            DPRINTF(LaneVSPM, "sram response to cau C: %d\n", static_cast<int>((dataPtr[1] << 8) | dataPtr[0]));
            owner->readdata_fromVRF_buf_CAU_C = (dataPtr[1] << 8) | dataPtr[0];
            owner->operandrequester_cauC_datafrom_VRF_arrived = true;
            delete pkt;
        }
       else if (pkt->id == 6) // 如果 id 是 2
        {
            DPRINTF(LaneVSPM, "sram response to cau D: %d\n", static_cast<int>((dataPtr[1] << 8) | dataPtr[0]));
            owner->readdata_fromVRF_buf_CAU_D = (dataPtr[1] << 8) | dataPtr[0];
            owner->operandrequester_cauD_datafrom_VRF_arrived = true;
            delete pkt;
        }
       else if (pkt->id == 7) // 如果 id 是 7, SerDiv_A
        {
            DPRINTF(LaneVSPM, "sram response to SerDiv_A: %d\n", static_cast<int>((dataPtr[1] << 8) | dataPtr[0]));
            owner->readdata_fromVRF_buf_SerDiv_A = (dataPtr[1] << 8) | dataPtr[0];
            owner->operandrequester_serdivA_datafrom_VRF_arrived = true;
            delete pkt;
        }
       else if (pkt->id == 8) // 如果 id 是 8, SerDiv_B
        {
            DPRINTF(LaneVSPM, "sram response to SerDiv_B: %d\n", static_cast<int>((dataPtr[1] << 8) | dataPtr[0]));
            owner->readdata_fromVRF_buf_SerDiv_B = (dataPtr[1] << 8) | dataPtr[0];
            owner->operandrequester_serdivB_datafrom_VRF_arrived = true;
            delete pkt;
        }
       else if (pkt->id == 9) // 如果 id 是 9, ShuffleUnit
        {
            DPRINTF(LaneVSPM, "sram response to ShuffleUnit: %d\n", static_cast<int>((dataPtr[1] << 8) | dataPtr[0]));
            owner->transparentVFUResponsetoShuffle(pkt);
        }
       else if (pkt->id == 10) // 如果 id 是 10, Mask
        {
            DPRINTF(LaneVSPM, "sram response to Mask: %d\n", static_cast<int>((dataPtr[1] << 8) | dataPtr[0]));
            owner->readdata_fromVRF_buf_Mask = (dataPtr[1] << 8) | dataPtr[0];
            owner->operandrequester_mask_datafrom_VRF_arrived = true;
            delete pkt;
        }
       else if (pkt->id == 16) // 如果 id 是 16, ShuffleUnitWriteback
        {
            owner->transparentVFUResponsetoShuffle(pkt);
        }
        else if (pkt->id == 11 || pkt->id == 12 ||
                 pkt->id == 13 || pkt->id == 15)
        {
            /*
             * A timing-request ACK only means that the VRF accepted the
             * write.  Publish RAW-chain progress at the tagged write
             * response, when the row is committed and a same-cycle reader
             * can no longer observe the previous value.
             */
            auto *state = dynamic_cast<VrfWriteCommitState *>(
                pkt->senderState);
            panic_if(state == nullptr,
                     "VRF write response %d has no commit tag", pkt->id);
            DPRINTF(LaneVSPM,
                    "VRF committed tagged write: instr %d runningID %d\n",
                    state->vns_instr_id, state->running_id);
            int passage = 0;
            if (pkt->id == 11)
                passage = OperandPassage_CAUA;
            else if (pkt->id == 12)
                passage = OperandPassage_CAUB;
            else if (pkt->id == 13)
                passage = OperandPassage_BitALU;
            else if (pkt->id == 15)
                passage = OperandPassage_SerDiv;
            DPRINTF(LaneVFU,
                    "VFU VRF write response passage %d instr %d/rid %d\n",
                    passage, state->vns_instr_id, state->running_id);
            pkt->senderState = nullptr;
            delete state;
            delete pkt;
        }
        else
        {
            DPRINTF(LaneVSPM, "Unknown packet ID as %d\n", pkt->id);
            delete pkt;
        }
    }
    else
    {
        DPRINTF(LaneVSPM, "Data pointer is null!\n");
    }

    return true;
}

bool
VenusLane::vrfReadGenerationMatches(
    const PendingVrfReadResponse &resp) const
{
    const VenusInstrPkt *running = nullptr;
    switch (resp.response_id) {
      /*
       * A read response is owned by the requester-local command generation,
       * not by the operand-queue/VFU head.  The tagged requester may start
       * generation N+1 while generation N still occupies the queue head.
       * Comparing against running_* drops valid N+1 responses as "stale" and
       * leaves an unrecoverable hole in that operand stream.
       */
      case 1: running = queueing_bitaluA_instr_pkt; break;
      case 2: running = queueing_bitaluB_instr_pkt; break;
      case 3: running = queueing_cauA_instr_pkt; break;
      case 4: running = queueing_cauB_instr_pkt; break;
      case 5: running = queueing_cauC_instr_pkt; break;
      case 6: running = queueing_cauD_instr_pkt; break;
      case 7: running = queueing_serdivA_instr_pkt; break;
      case 8: running = queueing_serdivB_instr_pkt; break;
      case 10: running = queueing_mask_instr_pkt; break;
      default: return false;
    }
    const auto matches = [&resp](const VenusInstrPkt *pkt) {
        return pkt != nullptr && pkt->running_id == resp.running_id &&
            pkt->vns_instr_id == resp.vns_instr_id;
    };
    const int operand = resp.response_id - 1;
    return matches(running) ||
        (operand >= 0 && operand < 10 &&
         matches(operandRequesterDrainingPkt[operand]));
}

void
VenusLane::publishVrfReadResponses()
{
    const auto wakeRequester = [this](int responseId) {
        auto wake = [this](auto &event) {
            if (!event.scheduled())
                schedule(event, curTick());
            else if (event.when() > curTick())
                reschedule(event, curTick());
        };
        switch (responseId) {
          /*
       * Ordinary tagged requester reads become visible on a lane-clock
       * boundary and
           * need an explicit wakeup after the registered response stage.
           * Do not wake the other requesters here: pipelined/RAW responses
           * can return on the intervening SRAM tick, and pulling their event
           * to curTick() would permit two grants in one 500 MHz lane cycle.
           */
          case 3: wake(nextOperandRequesterGetsCAU_A_DataEvent); break;
          case 4: wake(nextOperandRequesterGetsCAU_B_DataEvent); break;
          case 5: wake(nextOperandRequesterGetsCAU_C_DataEvent); break;
          case 6: wake(nextOperandRequesterGetsCAU_D_DataEvent); break;
          default: break;
        }
    };
    for (auto it = pendingVrfReadResponses.begin();
         it != pendingVrfReadResponses.end();) {
        if (it->visible_tick > curTick()) {
            ++it;
            continue;
        }

        if (vrfReadGenerationMatches(*it)) {
            panic_if(it->response_id < 0 || it->response_id > 10,
                     "invalid tagged VRF response passage %d",
                     it->response_id);
            readyVrfReadResponses[it->response_id].push_back(*it);
            wakeRequester(it->response_id);
            DPRINTF(LaneVSPM,
                    "Queued tagged VRF response: passage %d instr %d "
                    "runningID %d offset %d\n",
                    it->response_id, it->vns_instr_id, it->running_id,
                    it->operand_offset);
        } else {
            DPRINTF(LaneVSPM,
                    "Dropped stale tagged VRF response: passage %d instr %d "
                    "runningID %d\n",
                    it->response_id, it->vns_instr_id, it->running_id);
        }
        it = pendingVrfReadResponses.erase(it);
    }

    Tick next_tick = MaxTick;
    for (const auto &response : pendingVrfReadResponses)
        next_tick = std::min(next_tick, response.visible_tick);
    if (next_tick != MaxTick)
        schedule(nextVrfReadResponseStageEvent, next_tick);
}

bool
VenusLane::takeVrfReadResponse(
    int response_id, int running_id, int vns_instr_id, int operand_offset,
    unsigned int &data)
{
    if (response_id < 0 || response_id > 10)
        return false;

    auto &responses = readyVrfReadResponses[response_id];
    for (auto it = responses.begin(); it != responses.end();) {
        if (it->running_id == running_id &&
            it->vns_instr_id == vns_instr_id &&
            it->operand_offset == operand_offset) {
            data = it->data;
            DPRINTF(LaneVFU,
                    "operand response consume type %d instr %d/rid %d "
                    "offset %d\n",
                    response_id - 1, vns_instr_id, running_id,
                    operand_offset);
            responses.erase(it);
            return true;
        }

        if (it->running_id != running_id ||
            it->vns_instr_id != vns_instr_id) {
            DPRINTF(LaneVSPM,
                    "Discarded stale ready VRF response: passage %d "
                    "instr %d runningID %d offset %d while requester is "
                    "instr %d runningID %d offset %d\n",
                    response_id, it->vns_instr_id, it->running_id,
                    it->operand_offset, vns_instr_id, running_id,
                    operand_offset);
            it = responses.erase(it);
        } else {
            // Same tagged command, different row: retain it until that
            // precise row reaches the head of the operand stream.
            ++it;
        }
    }
    return false;
}

void
VenusLane::serviceDrainingOperandResponses(OPERANDTYPE operand)
{
    const unsigned index = static_cast<unsigned>(operand);
    panic_if(index >= operandRequesterDrainingPkt.size(),
             "invalid draining operand requester %u", index);
    VenusInstrPkt *pkt = operandRequesterDrainingPkt[index];
    if (pkt == nullptr)
        return;

    unsigned int &data = operandRequesterDrainingData[index];
    bool &dataValid = operandRequesterDrainingDataValid[index];
    int &readyCount = operandRequesterDrainingReadyCount[index];
    const int responseId = static_cast<int>(operand) + 1;
    if (!dataValid) {
        const int offset = readyCount * (pkt->vew + 1);
        dataValid = takeVrfReadResponse(
            responseId, pkt->running_id, pkt->vns_instr_id, offset, data);
    }
    if (!dataValid)
        return;

    if (!operandQueuePushdataFIFO(operand, data, pkt))
        return;

    dataValid = false;
    readyCount += 2 - pkt->vew;
    DPRINTF(LaneVFU,
            "draining tagged operand response type %d instr %d/rid %d "
            "rows %d/%d\n",
            operand, pkt->vns_instr_id, pkt->running_id,
            readyCount,
            operand == CAU_A ? pkt->locallane_vs1_operand_len :
            operand == CAU_B ? pkt->locallane_vs2_operand_len :
            operand == CAU_C ? pkt->locallane_vd1_operand_len :
                               pkt->locallane_vd2_operand_len);

    int operandLength = 0;
    switch (operand) {
      case BitAlu_A:
        operandLength = pkt->locallane_vs1_operand_len;
        break;
      case BitAlu_B:
        operandLength = pkt->locallane_vs2_operand_len;
        break;
      case CAU_A: operandLength = pkt->locallane_vs1_operand_len; break;
      case CAU_B: operandLength = pkt->locallane_vs2_operand_len; break;
      case CAU_C: operandLength = pkt->locallane_vd1_operand_len; break;
      case CAU_D: operandLength = pkt->locallane_vd2_operand_len; break;
      default:
        panic("unsupported draining operand requester %d", operand);
    }
    if (readyCount >= operandLength) {
        DPRINTF(LaneVFU,
                "draining operand requester complete type %d instr %d/rid "
                "%d rows %d\n",
                operand, pkt->vns_instr_id, pkt->running_id, operandLength);
        delete pkt;
        operandRequesterDrainingPkt[index] = nullptr;
        readyCount = 0;
    }
}


// Function to calculate the physical address
Addr VenusLane::calculatePhysicalAddress(int headline, int offset)
{

    // std::cout << "offset: " << offset << std::endl;
    // Calculate the byte within the bank
    int byte_within_bank = offset % (2 * bank_num);
    int row_num = offset / (2 * bank_num);
    // std::cout << "byte_within_bank: " << byte_within_bank << std::endl;
    // Calculate the bank index
    int bank_index = ((byte_within_bank / (2)) + row_num + headline) % bank_num;
    // std::cout << curTick() <<": bank_index: " << bank_index << std::endl;
    // Print the value of bank_num and line_num
    // std::cout << "bank_num: " << bank_num << std::endl;
    // std::cout << "line_num: " << line_num << std::endl;
    // Calculate the physical address using the formula
    // RTL carries a vrow_t through the VRF request path, so stepping beyond
    // the final row wraps to row zero instead of escaping into the next bank.
    const int effective_row = (headline + row_num) & (line_num - 1);
    Addr physical_address = vrf_base_addr + effective_row * 2 +
        bank_index * 2 * line_num +
        lane_id * 2 * NrBankPerLane * line_num;
    // Print the log in a single line with the "at tick" prefix
    // std::cout << "at tick = " << curTick()
    //           << " offset: " << offset
    //           << " byte_within_bank: " << byte_within_bank
    //           << " bank_index: " << bank_index
    //           << " bank_num: " << bank_num
    //           << " line_num: " << line_num << std::endl;
    DPRINTF(LaneVSPM,
        "offset=%d byte_within_bank=%d bank_index=%d bank_num=%d line_num=%d\n",
         offset, byte_within_bank, bank_index, bank_num, line_num);

    return physical_address;
}


bool VenusLane::sendVFUReadRequest(
    int headline, int offset, const std::string& vfuName, int _responseid,
    int requestport, int running_id, int vns_instr_id, bool pipelined)
{
    DPRINTF(LaneVSPM,"into %s\n", vfuName);

    // 检查 VenusLaneRequestPorts 是否为空
    if (!VenusLaneRequestPorts.empty()) {
        Addr paddr = calculatePhysicalAddress(headline, offset);  // 计算物理地址
        DPRINTF(LaneVSPM,"%s Headline: %d, Offset: %d, Physical Address: %d\n", vfuName, headline, offset, paddr);
        DPRINTF(LaneVSPM,"%s Physical Address (Hex): %x\n", vfuName, paddr);

        unsigned int size = 2;
        RequestorID req_id = 2; // 请求者ID（标识哪个核心发出的请求）
        Request::Flags req_flags = Request::PHYSICAL;

        // 创建请求
        auto* vrfPktreq = new Request(paddr, size, req_flags, req_id);
        int id = _responseid;  // 包ID
        Packet* vrfPkt = new Packet(RequestPtr(vrfPktreq), MemCmd::ReadReq, size, id);
        if (_responseid != 9)
            vrfPkt->senderState =
                new VrfReadResponseState(
                    running_id, vns_instr_id, offset, pipelined);

        // 动态分配数据并传入
        uint8_t* dynamicData = new uint8_t[size]{};
        vrfPkt->dataDynamic(dynamicData);

        // 发送timing request
        DPRINTF(LaneVSPM,"%s send timing req\n", vfuName);
        bool result = VenusLaneRequestPorts[requestport].sendPacket(vrfPkt);

        if (result) {
            auto dataPtr = vrfPkt->getPtr<uint8_t>();
            if (dataPtr) {
                DPRINTF(LaneVSPM,"First byte as decimal: %d\n", static_cast<int>(dataPtr[0]));
            } else {
                DPRINTF(LaneVSPM,"Data pointer is null!\n");
            }
            return true;  // 成功发送请求
        } else {
            DPRINTF(LaneVSPM,"Failed to send request for %s\n", vfuName);
            // In the experimental tagged-intent path the port owns the
            // original packet until grant.  Otherwise preserve the promoted
            // R52 allocation/deletion contract.
            if (!VenusLaneRequestPorts[requestport].
                    ownsBlockedPacket(vrfPkt)) {
                delete vrfPkt->senderState;
                vrfPkt->senderState = nullptr;
                delete vrfPkt;
            }
            return false; // 请求发送失败
        }
    }
    DPRINTF(LaneVSPM,"%s failed, VenusLaneRequestPorts is empty!\n", vfuName);
    return false;  // 如果端口为空，返回失败
}

bool VenusLane::sendVFUWriteRequest(int headline, int offset, const std::string& vfuName, int _responseid, int requestport, unsigned int writebackdata, unsigned int size, int running_id, int vns_instr_id) {
    DPRINTF(LaneVSPM,"into %s\n", vfuName);

    // 检查 VenusLaneRequestPorts 是否为空
    if (!VenusLaneRequestPorts.empty()) {
        Addr paddr = calculatePhysicalAddress(headline, offset);  // 计算物理地址
        if (size == 1)
            paddr += offset & 0x1;
        DPRINTF(LaneVSPM,"%s Headline: %d, Offset: %d, Physical Address: %d\n", vfuName, headline, offset, paddr);
        DPRINTF(LaneVSPM,"%s Physical Address (Hex): %x\n", vfuName, paddr);

        if (experimentalVrfRr &&
            VenusLaneRequestPorts[requestport].consumeDeferredWriteGrant(
                paddr, size, running_id, vns_instr_id)) {
            DPRINTF(LaneVFU,
                    "captured deferred VFU VRF write grant port %d "
                    "instr %d/rid %d offset %d size %u\n",
                    requestport, vns_instr_id, running_id, offset, size);
            return true;
        }

        RequestorID req_id = 2; // 请求者ID（标识哪个核心发出的请求）
        Request::Flags req_flags = Request::PHYSICAL;

        // 创建请求
        auto* vrfPktreq = new Request(paddr, size, req_flags, req_id);
        int id = _responseid;  // 包ID
        Packet* vrfPkt = new Packet(RequestPtr(vrfPktreq), MemCmd::WriteReq, size, id);
        if (_responseid == 11 || _responseid == 12 ||
            _responseid == 13 || _responseid == 15)
            vrfPkt->senderState =
                new VrfWriteCommitState(running_id, vns_instr_id);

        // 动态分配数据并传入
        uint8_t *dynamicData = new uint8_t[size];
        uint16_t wb16 = static_cast<uint16_t>(writebackdata);
        if (size == 1) {
            dynamicData[0] = static_cast<uint8_t>((offset & 0x1) ? (wb16 >> 8) : wb16);
        } else {
            dynamicData[0] = static_cast<uint8_t>(wb16 & 0xFF);        // 低字节
            dynamicData[1] = static_cast<uint8_t>((wb16 >> 8) & 0xFF); // 高字节
        }
        vrfPkt->dataDynamic(dynamicData);

        // 发送timing request
        DPRINTF(LaneVSPM,"%s send timing req\n", vfuName);
        bool result = VenusLaneRequestPorts[requestport].sendPacket(vrfPkt);

        if (result) {
            auto dataPtr = vrfPkt->getPtr<uint8_t>();
            if (dataPtr) {
                DPRINTF(LaneVSPM,"First byte as decimal: %d\n", static_cast<int>(dataPtr[0]));
            } else {
                DPRINTF(LaneVSPM,"Data pointer is null!\n");
            }
            return true;  // 成功发送请求
        } else {
            DPRINTF(LaneVSPM,"Failed to send request for %s\n", vfuName);
            if (!VenusLaneRequestPorts[requestport].
                    ownsBlockedPacket(vrfPkt)) {
                delete vrfPkt->senderState;
                vrfPkt->senderState = nullptr;
                delete vrfPkt;
            }
            return false; // 请求发送失败
        }
    }
    DPRINTF(LaneVSPM,"%s failed, VenusLaneRequestPorts is empty!\n", vfuName);
    return false;  // 如果端口为空，返回失败
}


// Function to calculate the physical address
Addr VenusLane::calculateMaskPhysicalAddress(int offset)
{

    // std::cout << "offset: " << offset << std::endl;
    // Calculate the byte within the bank
    int byte_within_row = offset % (NrBytesPerBank * NrBankPerLane);
    int row_num = offset / (NrBytesPerBank * NrBankPerLane);
    // std::cout << "byte_within_bank: " << byte_within_bank << std::endl;
    // Calculate the bank index
    // std::cout << "bank_index: " << bank_index << std::endl;
    // Print the value of bank_num and line_num
    // std::cout << "bank_num: " << bank_num << std::endl;
    // std::cout << "line_num: " << line_num << std::endl;
    // Calculate the physical address using the formula
    //                                      之前的行数                                                                                                   262144
    Addr physical_address = vrf_base_addr + (row_num) * (NrBytesPerBank * NrBankPerLane) + byte_within_row + lane_id * NrLines * NrBytesPerBank * NrBankPerLane + (NrLanes * NrLines * NrBytesPerBank * NrBankPerLane);
    // Print the log in a single line with the "at tick" prefix
    // std::cout << "at tick = " << curTick()
    //           << " offset: " << offset
    //           << " byte_within_bank: " << byte_within_bank
    //           << " bank_index: " << bank_index
    //           << " bank_num: " << bank_num
    //           << " line_num: " << line_num << std::endl;
    DPRINTF(LaneVSPM,
        "MASK offset=%d byte_within_row=%d row_num=%d physical_address=%d\n",
         offset, byte_within_row, row_num, physical_address);

    return physical_address;
}

bool VenusLane::sendMaskReadRequest(
    int offset, const std::string& vfuName, int _responseid, int requestport,
    int running_id, int vns_instr_id, bool pipelined)
{
    DPRINTF(LaneVSPM,"into %s\n", vfuName);

    // 检查 VenusLaneRequestPorts 是否为空
    if (!VenusLaneRequestPorts.empty()) {
        Addr paddr = calculateMaskPhysicalAddress(offset);  // 计算物理地址
        DPRINTF(LaneVSPM,"%s Offset: %d, MaskPhysical Address: %d\n", vfuName, offset, paddr);
        DPRINTF(LaneVSPM,"%s MaskPhysical Address (Hex): %x\n", vfuName, paddr);

        /* RTL mask SRAM is one 8-bit logical row wide.  gem5 expands each
         * bit to a byte so VINS/LSU code can address mask elements directly;
         * one row transaction therefore spans eight backing bytes. */
        unsigned int size = 8;
        RequestorID req_id = 2; // 请求者ID（标识哪个核心发出的请求）
        Request::Flags req_flags = Request::PHYSICAL;

        // 创建请求
        auto* vrfPktreq = new Request(paddr, size, req_flags, req_id);
        int id = _responseid;  // 包ID
        Packet* vrfPkt = new Packet(RequestPtr(vrfPktreq), MemCmd::ReadReq, size, id);
        vrfPkt->senderState =
            new VrfReadResponseState(
                running_id, vns_instr_id, offset, pipelined);

        // 动态分配数据并传入
        uint8_t* dynamicData = new uint8_t[size]{};
        vrfPkt->dataDynamic(dynamicData);

        // 发送timing request
        DPRINTF(LaneVSPM,"%s send timing req\n", vfuName);
        bool result = VenusLaneRequestPorts[requestport].sendTimingReq(vrfPkt);  // 假设pkt是传递给端口的包

        if (result) {
            auto dataPtr = vrfPkt->getPtr<uint8_t>();
            if (dataPtr) {
                DPRINTF(LaneVSPM,"First byte as decimal: %d\n", static_cast<int>(dataPtr[0]));
            } else {
                DPRINTF(LaneVSPM,"Data pointer is null!\n");
            }
            return true;  // 成功发送请求
        } else {
            DPRINTF(LaneVSPM,"Failed to send request for %s\n", vfuName);
            delete vrfPkt->senderState;
            vrfPkt->senderState = nullptr;
            delete vrfPkt;
            return false; // 请求发送失败
        }
    }
    DPRINTF(LaneVSPM,"%s failed, VenusLaneRequestPorts is empty!\n", vfuName);
    return false;  // 如果端口为空，返回失败
}

bool VenusLane::sendMaskWriteRequest(int offset, const std::string& vfuName, int _responseid, int requestport, unsigned int writebackdata) {
    DPRINTF(LaneVSPM,"into %s\n", vfuName);

    // 检查 VenusLaneRequestPorts 是否为空
    if (!VenusLaneRequestPorts.empty()) {
        Addr paddr = calculateMaskPhysicalAddress(offset);  // 计算物理地址
        DPRINTF(LaneVSPM,"%s Offset: %d, Physical Address: %d\n", vfuName, offset, paddr);
        DPRINTF(LaneVSPM,"%s Physical Address (Hex): %x\n", vfuName, paddr);

        unsigned int size = 2;  // 数据大小（假设是64字节）
        RequestorID req_id = 2; // 请求者ID（标识哪个核心发出的请求）
        Request::Flags req_flags = Request::PHYSICAL;

        // 创建请求
        auto* vrfPktreq = new Request(paddr, size, req_flags, req_id);
        int id = _responseid;  // 包ID
        Packet* vrfPkt = new Packet(RequestPtr(vrfPktreq), MemCmd::WriteReq, size, id);

        // 动态分配数据并传入
        uint8_t *dynamicData = new uint8_t[size];
        uint16_t wb16 = static_cast<uint16_t>(writebackdata);
        dynamicData[0] = static_cast<uint8_t>(wb16 & 0xFF);        // 低字节
        dynamicData[1] = static_cast<uint8_t>((wb16 >> 8) & 0xFF); // 高字节
        vrfPkt->dataDynamic(dynamicData);

        // 发送timing request
        DPRINTF(LaneVSPM,"%s send timing req\n", vfuName);
        bool result = VenusLaneRequestPorts[requestport].sendTimingReq(vrfPkt);  // 假设pkt是传递给端口的包

        if (result) {
            auto dataPtr = vrfPkt->getPtr<uint8_t>();
            if (dataPtr) {
                DPRINTF(LaneVSPM,"First byte as decimal: %d\n", static_cast<int>(dataPtr[0]));
            } else {
                DPRINTF(LaneVSPM,"Data pointer is null!\n");
            }
            return true;  // 成功发送请求
        } else {
            DPRINTF(LaneVSPM,"Failed to send request for %s\n", vfuName);
            delete vrfPkt;
            return false; // 请求发送失败
        }
    }
    DPRINTF(LaneVSPM,"%s failed, VenusLaneRequestPorts is empty!\n", vfuName);
    return false;  // 如果端口为空，返回失败
}

bool
VenusLane::sendMaskRowWriteRequest(int maskAddr, uint8_t rowBits,
                                   int runningId, int instrId)
{
    panic_if(VenusLaneRequestPorts.size() <= 13,
             "mask write requester port is missing");
    const unsigned row = static_cast<unsigned>(maskAddr) / NrBankPerLane;
    const unsigned size = 8;
    const Addr paddr = calculateMaskPhysicalAddress(row * size);
    if (experimentalVrfRr &&
        VenusLaneRequestPorts[13].consumeDeferredWriteGrant(
            paddr, size, runningId, instrId)) {
        DPRINTF(LaneVFU,
                "captured deferred BitALU mask-row grant instr %d/rid %d "
                "row %u\n",
                instrId, runningId, row);
        return true;
    }
    auto *request = new Request(paddr, size, Request::PHYSICAL, 2);
    auto *pkt = new Packet(RequestPtr(request), MemCmd::WriteReq, size, 14);
    if (experimentalVrfRr)
        pkt->senderState = new VrfWriteCommitState(runningId, instrId);
    auto *data = new uint8_t[size];
    for (unsigned bit = 0; bit < size; ++bit)
        data[bit] = (rowBits >> bit) & 0x1;
    pkt->dataDynamic(data);

    const bool accepted = VenusLaneRequestPorts[13].sendPacket(pkt);
    DPRINTF(LaneVFU,
            "BitALU mask-row request instr-address %d row %u bits 0x%02x "
            "accepted %d\n",
            maskAddr, row, rowBits, accepted);
    if (!accepted && !VenusLaneRequestPorts[13].ownsBlockedPacket(pkt)) {
        delete pkt->senderState;
        pkt->senderState = nullptr;
        delete pkt;
    }
    return accepted;
}

bool VenusLane::requestArbiter(
    int headline, int offset, int ot, unsigned int writebackdata,
    unsigned int write_size, int request_running_id,
    int request_vns_instr_id, bool pipelined_read)
{
    // +------------------------------------+
    // |         READ  BitAlu_A = 0         |
    // |         READ  BitAlu_B = 1         |
    // |         READ  CAU_A = 2            |
    // |         READ  CAU_B = 3            |
    // |         READ  CAU_C = 4            |
    // |         READ  CAU_D = 5            |
    // |         READ  SerDiv_A = 6         |
    // |         READ  SerDiv_B = 7         |
    // |         READ  Mask = 8             |
    // |         READ  ShuffleUnit = 9      |
    // |                                    |
    // |         WRITE BitALU = -1          |OperandPassage_BitALU
    // |         WRITE BitALUMask = -2      |OperandPassage_BitALUMask
    // |         WRITE CAUA = -3            |OperandPassage_CAUA
    // |         WRITE CAUB = -4            |OperandPassage_CAUB
    // |         WRITE SerDiv = -5          |OperandPassage_SerDiv
    // |         WRITE ShuffleUnit = -6     |OperandPassage_ShuffleUnit
    // +------------------------------------+


    // std::cout<<"requestArbiter:headline = "<<headline<<", offset = "<<offset<<", ot = "<<ot<<std::endl;
    DPRINTF(LaneVSPM,"requestArbiter called, operating vfu is %d\n", ot);
    bool result = true;
    /*
     * CAU and SerDiv each expose one data-result master to the RTL's
     * bank-wise arbiter.  Their result FIFOs may contain paired or newly
     * captured entries, but one master cannot consume two grants in the
     * same lane clock edge.
     */
    const bool masterGrantUsed =
        (ot == OperandPassage_BitALU &&
         bitaluLastVrfGrantTick == curTick()) ||
        ((ot == OperandPassage_CAUA || ot == OperandPassage_CAUB) &&
         cauLastVrfGrantTick == curTick()) ||
        (ot == OperandPassage_SerDiv &&
         serdivLastVrfGrantTick == curTick());
    if (masterGrantUsed) {
        result = false;
    }
    if(result == true) {
        switch(ot){
            case BitAlu_A:
                result = sendVFUReadRequest(
                    headline, offset, "BitAlu_A", 1, 0,
                    request_running_id, request_vns_instr_id,
                    pipelined_read);
                break;
            case BitAlu_B:
                result = sendVFUReadRequest(
                    headline, offset, "BitAlu_B", 2, 1,
                    request_running_id, request_vns_instr_id,
                    pipelined_read);
                break;
            case CAU_A:
                result = sendVFUReadRequest(
                    headline, offset, "CAU_A", 3, 2,
                    request_running_id, request_vns_instr_id,
                    pipelined_read);
                break;
            case CAU_B:
                result = sendVFUReadRequest(
                    headline, offset, "CAU_B", 4, 3,
                    request_running_id, request_vns_instr_id,
                    pipelined_read);
                break;
            case CAU_C:
                result = sendVFUReadRequest(
                    headline, offset, "CAU_C", 5, 4,
                    request_running_id, request_vns_instr_id,
                    pipelined_read);
                break;
            case CAU_D:
                result = sendVFUReadRequest(
                    headline, offset, "CAU_D", 6, 5,
                    request_running_id, request_vns_instr_id,
                    pipelined_read);
                break;
            case SerDiv_A:
                result = sendVFUReadRequest(
                    headline, offset, "SerDiv_A", 7, 6,
                    request_running_id, request_vns_instr_id,
                    pipelined_read);
                break;
            case SerDiv_B:
                result = sendVFUReadRequest(
                    headline, offset, "SerDiv_B", 8, 7,
                    request_running_id, request_vns_instr_id,
                    pipelined_read);
                break;
            case ShuffleUnit:
                result = sendVFUReadRequest(
                    headline, offset, "ShuffleUnit", 9, 8,
                    request_running_id, request_vns_instr_id,
                    pipelined_read);
                break;
            case Mask:
                result = sendMaskReadRequest(
                    offset, "Mask", 10, 9,
                    request_running_id, request_vns_instr_id,
                    pipelined_read);
                break;
            case OperandPassage_CAUA:
                result = sendVFUWriteRequest(headline, offset, "WB_CAU_A", 11, 10, writebackdata, write_size, request_running_id, request_vns_instr_id);
                break;
            case OperandPassage_CAUB:
                result = sendVFUWriteRequest(headline, offset, "WB_CAU_B", 12, 11, writebackdata, write_size, request_running_id, request_vns_instr_id);
                break;
            case OperandPassage_BitALU:
                result = sendVFUWriteRequest(headline, offset, "WB_BITALU", 13, 12, writebackdata, write_size, request_running_id, request_vns_instr_id);
                break;
            case OperandPassage_BitALUMask:
                result = sendMaskWriteRequest(offset, "WB_BITALUMask", 14, 13, writebackdata);
                break;
            case OperandPassage_SerDiv:
                result = sendVFUWriteRequest(headline, offset, "WB_SerDiv", 15, 14, writebackdata, write_size, request_running_id, request_vns_instr_id);
                break;
            case OperandPassage_ShuffleUnit:
                result = sendVFUWriteRequest(headline, offset, "WB_ShuffleUnit", 16, 15, writebackdata, write_size, request_running_id, request_vns_instr_id);
                break;
            default: break;
        }
    }
    if (result && ot == OperandPassage_BitALU) {
        bitaluLastVrfGrantTick = curTick();
    }
    if (result &&
        (ot == OperandPassage_CAUA || ot == OperandPassage_CAUB)) {
        cauLastVrfGrantTick = curTick();
    }
    if (result && ot == OperandPassage_SerDiv) {
        serdivLastVrfGrantTick = curTick();
    }
    if (ot >= BitAlu_A && ot <= Mask) {
        DPRINTF(LaneVFU,
                "operand VRF request type %d instr %d/rid %d offset %d "
                "size %u accepted %d\n",
                ot, request_vns_instr_id, request_running_id, offset,
                write_size, result);
    }
    if (ot == OperandPassage_BitALU ||
        ot == OperandPassage_CAUA ||
        ot == OperandPassage_CAUB ||
        ot == OperandPassage_SerDiv) {
        DPRINTF(LaneVFU,
                "VFU VRF grant passage %d instr %d/rid %d offset %d "
                "size %u accepted %d reason %s\n",
                ot, request_vns_instr_id, request_running_id, offset,
                write_size, result,
                result ? "granted" :
                    (masterGrantUsed ? "master_busy" : "bank_backpressure"));
    }
    return result;//trick
}
void VenusLane::generateVRFReadData(OPERANDTYPE ot)//trick
{
    /* TRICK */ \
    if(ot == BitAlu_A   ) {readdata_fromVRF_buf_BitAlu_A    = getRandomInt(0,65535); /*readdata_fromVRF_buf_BitAlu_A    = readdata_fromVRF_buf_BitAlu_A    + 1;*/ operandrequester_bitaluA_datafrom_VRF_arrived = true;/*std::cout<<"at tick = "<<curTick()<<"VRFReadData of BitAlu_A    generated, data = "<< readdata_fromVRF_buf_BitAlu_A    <<std::endl;*/}
    if(ot == BitAlu_B   ) {readdata_fromVRF_buf_BitAlu_B    = getRandomInt(0,65535); /*readdata_fromVRF_buf_BitAlu_B    = readdata_fromVRF_buf_BitAlu_B    + 1;*/ operandrequester_bitaluB_datafrom_VRF_arrived = true;/*std::cout<<"at tick = "<<curTick()<<"VRFReadData of BitAlu_B    generated, data = "<< readdata_fromVRF_buf_BitAlu_B    <<std::endl;*/}
    if(ot == CAU_A      ) {readdata_fromVRF_buf_CAU_A       = getRandomInt(0,20); /*readdata_fromVRF_buf_CAU_A       = readdata_fromVRF_buf_CAU_A       + 1;*/ operandrequester_cauA_datafrom_VRF_arrived    = true;/*std::cout<<"at tick = "<<curTick()<<"VRFReadData of CAU_A       generated, data = "<< readdata_fromVRF_buf_CAU_A       <<std::endl;*/}
    if(ot == CAU_B      ) {readdata_fromVRF_buf_CAU_B       = getRandomInt(0,10); /*readdata_fromVRF_buf_CAU_B       = readdata_fromVRF_buf_CAU_B       + 1;*/ operandrequester_cauB_datafrom_VRF_arrived    = true;/*std::cout<<"at tick = "<<curTick()<<"VRFReadData of CAU_B       generated, data = "<< readdata_fromVRF_buf_CAU_B       <<std::endl;*/}
    if(ot == CAU_C      ) {readdata_fromVRF_buf_CAU_C       = getRandomInt(0,65535); /*readdata_fromVRF_buf_CAU_C       = readdata_fromVRF_buf_CAU_C       + 1;*/ operandrequester_cauC_datafrom_VRF_arrived    = true;/*std::cout<<"at tick = "<<curTick()<<"VRFReadData of CAU_C       generated, data = "<< readdata_fromVRF_buf_CAU_C       <<std::endl;*/}
    if(ot == CAU_D      ) {readdata_fromVRF_buf_CAU_D       = getRandomInt(0,65535); /*readdata_fromVRF_buf_CAU_D       = readdata_fromVRF_buf_CAU_D       + 1;*/ operandrequester_cauD_datafrom_VRF_arrived    = true;/*std::cout<<"at tick = "<<curTick()<<"VRFReadData of CAU_D       generated, data = "<< readdata_fromVRF_buf_CAU_D       <<std::endl;*/}
    if(ot == SerDiv_A   ) {readdata_fromVRF_buf_SerDiv_A    = getRandomInt(0,65535); /*readdata_fromVRF_buf_SerDiv_A    = readdata_fromVRF_buf_SerDiv_A    + 1;*/ operandrequester_serdivA_datafrom_VRF_arrived = true;/*std::cout<<"at tick = "<<curTick()<<"VRFReadData of SerDiv_A    generated, data = "<< readdata_fromVRF_buf_SerDiv_A    <<std::endl;*/}
    if(ot == SerDiv_B   ) {readdata_fromVRF_buf_SerDiv_B    = getRandomInt(0,65535); /*readdata_fromVRF_buf_SerDiv_B    = readdata_fromVRF_buf_SerDiv_B    + 1;*/ operandrequester_serdivB_datafrom_VRF_arrived = true;/*std::cout<<"at tick = "<<curTick()<<"VRFReadData of SerDiv_B    generated, data = "<< readdata_fromVRF_buf_SerDiv_B    <<std::endl;*/}
    if(ot == Mask       ) {readdata_fromVRF_buf_Mask        = getRandomChoice(4,0,1,256,257); /*readdata_fromVRF_buf_Mask        = readdata_fromVRF_buf_Mask        + 1;*/ operandrequester_mask_datafrom_VRF_arrived    = true;/*std::cout<<"at tick = "<<curTick()<<"VRFReadData of Mask        generated, data = "<< readdata_fromVRF_buf_Mask        <<std::endl;*/}
    // if(ot == ShuffleUnit) {readdata_fromVRF_buf_ShuffleUnit = getRandomInt(0,65535); /*readdata_fromVRF_buf_ShuffleUnit = readdata_fromVRF_buf_ShuffleUnit + 1;*/ operandrequester_shuffle_datafrom_VRF_arrived = true;/*std::cout<<"at tick = "<<curTick()<<"VRFReadData of ShuffleUnit generated, data = "<< readdata_fromVRF_buf_ShuffleUnit <<std::endl;*/}
}

void VenusLane::operandRequesterGetVectorData(OPERANDTYPE OperandType)
{
    switch (OperandType) {
        case BitAlu_A   : __GENoperandRequesterGetVectorData_Name__(BitAlu_A   , vs1_head, use_vs1   , locallane_vs1_operand_len  , bitaluA_instr_FIFO_lanseq, queueing_bitaluA_instr_pkt , operandrequester_bitaluA_busy, nextOperandRequesterGetsBitAlu_A_DataEvent   , operandrequester_bitaluA_push_instr_topush, operandrequester_bitaluA_push_data_topush, locallane_vs1_dataready_cnt  , operandrequester_bitaluA_datatoread_from_VRF, operandrequester_bitaluA_datafrom_VRF_arrived);break;
        case BitAlu_B   : __GENoperandRequesterGetVectorData_Name__(BitAlu_B   , vs2_head, use_vs2   , locallane_vs2_operand_len  , bitaluB_instr_FIFO_lanseq, queueing_bitaluB_instr_pkt , operandrequester_bitaluB_busy, nextOperandRequesterGetsBitAlu_B_DataEvent   , operandrequester_bitaluB_push_instr_topush, operandrequester_bitaluB_push_data_topush, locallane_vs2_dataready_cnt  , operandrequester_bitaluB_datatoread_from_VRF, operandrequester_bitaluB_datafrom_VRF_arrived);break;
        case CAU_A      : __GENoperandRequesterGetVectorData_Name__(CAU_A      , vs1_head, use_vs1   , locallane_vs1_operand_len  , cauA_instr_FIFO_lanseq   , queueing_cauA_instr_pkt    , operandrequester_cauA_busy   , nextOperandRequesterGetsCAU_A_DataEvent      , operandrequester_cauA_push_instr_topush   , operandrequester_cauA_push_data_topush   , locallane_vs1_dataready_cnt  , operandrequester_cauA_datatoread_from_VRF   , operandrequester_cauA_datafrom_VRF_arrived   );break;
        case CAU_B      : __GENoperandRequesterGetVectorData_Name__(CAU_B      , vs2_head, use_vs2   , locallane_vs2_operand_len  , cauB_instr_FIFO_lanseq   , queueing_cauB_instr_pkt    , operandrequester_cauB_busy   , nextOperandRequesterGetsCAU_B_DataEvent      , operandrequester_cauB_push_instr_topush   , operandrequester_cauB_push_data_topush   , locallane_vs2_dataready_cnt  , operandrequester_cauB_datatoread_from_VRF   , operandrequester_cauB_datafrom_VRF_arrived   );break;
        case CAU_C      : __GENoperandRequesterGetVectorData_Name__(CAU_C      , vd1_head, use_vd1_op, locallane_vd1_operand_len  , cauC_instr_FIFO_lanseq   , queueing_cauC_instr_pkt    , operandrequester_cauC_busy   , nextOperandRequesterGetsCAU_C_DataEvent      , operandrequester_cauC_push_instr_topush   , operandrequester_cauC_push_data_topush   , locallane_vd1_dataready_cnt  , operandrequester_cauC_datatoread_from_VRF   , operandrequester_cauC_datafrom_VRF_arrived   );break;
        case CAU_D      : __GENoperandRequesterGetVectorData_Name__(CAU_D      , vd2_head, use_vd2_op, locallane_vd2_operand_len  , cauD_instr_FIFO_lanseq   , queueing_cauD_instr_pkt    , operandrequester_cauD_busy   , nextOperandRequesterGetsCAU_D_DataEvent      , operandrequester_cauD_push_instr_topush   , operandrequester_cauD_push_data_topush   , locallane_vd2_dataready_cnt  , operandrequester_cauD_datatoread_from_VRF   , operandrequester_cauD_datafrom_VRF_arrived   );break;
        case SerDiv_A   : __GENoperandRequesterGetVectorData_Name__(SerDiv_A   , vs1_head, use_vs1   , locallane_vs1_operand_len  , serdivA_instr_FIFO_lanseq, queueing_serdivA_instr_pkt , operandrequester_serdivA_busy, nextOperandRequesterGetsSerDiv_A_DataEvent   , operandrequester_serdivA_push_instr_topush, operandrequester_serdivA_push_data_topush, locallane_vs1_dataready_cnt  , operandrequester_serdivA_datatoread_from_VRF, operandrequester_serdivA_datafrom_VRF_arrived);break;
        case SerDiv_B   : __GENoperandRequesterGetVectorData_Name__(SerDiv_B   , vs2_head, use_vs2   , locallane_vs2_operand_len  , serdivB_instr_FIFO_lanseq, queueing_serdivB_instr_pkt , operandrequester_serdivB_busy, nextOperandRequesterGetsSerDiv_B_DataEvent   , operandrequester_serdivB_push_instr_topush, operandrequester_serdivB_push_data_topush, locallane_vs2_dataready_cnt  , operandrequester_serdivB_datatoread_from_VRF, operandrequester_serdivB_datafrom_VRF_arrived);break;
        case Mask       : __GENoperandRequesterGetVectorData_Name__(Mask       , vm_r    , vm_r      , locallane_vmask_operand_len, mask_instr_FIFO_lanseq   , queueing_mask_instr_pkt    , operandrequester_mask_busy   , nextOperandRequesterGetsMask_DataEvent       , operandrequester_mask_push_instr_topush   , operandrequester_mask_push_data_topush   , locallane_vmask_dataready_cnt, operandrequester_mask_datatoread_from_VRF   , operandrequester_mask_datafrom_VRF_arrived   );break;
        // case ShuffleUnit: __GENoperandRequesterGetVectorData_Name__(ShuffleUnit, vs1_head, use_vs1   , locallane_vs1_operand_len  , shuffle_instr_FIFO_lanseq, queueing_shuffle_instr_pkt , operandrequester_shuffle_busy, nextOperandRequesterGetsShuffleUnit_DataEvent, operandrequester_shuffle_push_instr_topush, operandrequester_shuffle_push_data_topush, locallane_vs1_dataready_cnt  , operandrequester_shuffle_datatoread_from_VRF, operandrequester_shuffle_datafrom_VRF_arrived);break;
        default: panic("unknown OperandType");
    }
}


#define __GENoperandQueuePushInstrFIFO_Name__(OPERAND_TYPE_NAME, FIFO_NAME, DATA, MAX_DEPTH) \
if(FIFO_NAME.size() < MAX_DEPTH) { \
    FIFO_NAME.push_back(new VenusInstrPkt(DATA)); \
    DPRINTF(LaneVFU, \
            "operand instruction enqueue type %d instr %d/rid %d depth %d/%d\n", \
            OPERAND_TYPE_NAME, DATA->vns_instr_id, DATA->running_id, \
            FIFO_NAME.size(), MAX_DEPTH); \
    return true; \
} else { \
    return false; \
}
#define __GENoperandQueuePushDataFIFO_Name__(OPERAND_TYPE_NAME, FIFO_NAME, DATA, INSTR, MAX_DEPTH) \
if(FIFO_NAME.size() < MAX_DEPTH) { \
    FIFO_NAME.push_back(TaggedOperandData{DATA, INSTR->running_id, \
        INSTR->vns_instr_id, \
        (experimentalRequesterQVisibility && \
         (OPERAND_TYPE_NAME == BitAlu_A || \
          OPERAND_TYPE_NAME == BitAlu_B)) ? \
            afterCycles(Cycles(1)) : curTick()}); \
    DPRINTF(LaneOperandRequester, "operandQueue%s push tagged data: instr %d runningID %d depth %d\n", \
            #OPERAND_TYPE_NAME, INSTR->vns_instr_id, INSTR->running_id, \
            FIFO_NAME.size()); \
    return true; \
} else { \
    return false; \
}


bool VenusLane::operandQueuePushinstrFIFO(OPERANDTYPE OperandType, VenusInstrPkt* opinstr)
{
    switch (OperandType) {
        case BitAlu_A   : __GENoperandQueuePushInstrFIFO_Name__(BitAlu_A   , bitaluA_instr_FIFO_opqueue, opinstr, BitaluInsnQueueDepth);break;
        case BitAlu_B   : __GENoperandQueuePushInstrFIFO_Name__(BitAlu_B   , bitaluB_instr_FIFO_opqueue, opinstr, BitaluInsnQueueDepth);break;
        case CAU_A      : __GENoperandQueuePushInstrFIFO_Name__(CAU_A      , cauA_instr_FIFO_opqueue   , opinstr, CauInsnQueueDepth);break;
        case CAU_B      : __GENoperandQueuePushInstrFIFO_Name__(CAU_B      , cauB_instr_FIFO_opqueue   , opinstr, CauInsnQueueDepth);break;
        case CAU_C      : __GENoperandQueuePushInstrFIFO_Name__(CAU_C      , cauC_instr_FIFO_opqueue   , opinstr, CauInsnQueueDepth);break;
        case CAU_D      : __GENoperandQueuePushInstrFIFO_Name__(CAU_D      , cauD_instr_FIFO_opqueue   , opinstr, CauInsnQueueDepth);break;
        case SerDiv_A   : __GENoperandQueuePushInstrFIFO_Name__(SerDiv_A   , serdivA_instr_FIFO_opqueue, opinstr, SerdivInsnQueueDepth);break;
        case SerDiv_B   : __GENoperandQueuePushInstrFIFO_Name__(SerDiv_B   , serdivB_instr_FIFO_opqueue, opinstr, SerdivInsnQueueDepth);break;
        case Mask       : __GENoperandQueuePushInstrFIFO_Name__(Mask       , mask_instr_FIFO_opqueue   , opinstr, VmaskInsnQueueDepth);break;
        // case ShuffleUnit: __GENoperandQueuePushInstrFIFO_Name__(ShuffleUnit, shuffle_instr_FIFO_opqueue, opinstr, ShuffleInsnQueueDepth);break;
        default: panic("unknown OperandType");
    }
}
bool
VenusLane::operandQueuePushdataFIFO(OPERANDTYPE OperandType,
                                    unsigned int opdata,
                                    const VenusInstrPkt* instr)
{
    if (instr == nullptr)
        panic("operandQueuePushdataFIFO requires an instruction tag");
    switch (OperandType) {
        case BitAlu_A   : __GENoperandQueuePushDataFIFO_Name__(BitAlu_A   , bitaluA_data_FIFO_opqueue, opdata, instr, BitaluDataQueueDepth);break;
        case BitAlu_B   : __GENoperandQueuePushDataFIFO_Name__(BitAlu_B   , bitaluB_data_FIFO_opqueue, opdata, instr, BitaluDataQueueDepth);break;
        case CAU_A      : __GENoperandQueuePushDataFIFO_Name__(CAU_A      , cauA_data_FIFO_opqueue   , opdata, instr, CauDataQueueDepth);break;
        case CAU_B      : __GENoperandQueuePushDataFIFO_Name__(CAU_B      , cauB_data_FIFO_opqueue   , opdata, instr, CauDataQueueDepth);break;
        case CAU_C      : __GENoperandQueuePushDataFIFO_Name__(CAU_C      , cauC_data_FIFO_opqueue   , opdata, instr, CauDataQueueDepth);break;
        case CAU_D      : __GENoperandQueuePushDataFIFO_Name__(CAU_D      , cauD_data_FIFO_opqueue   , opdata, instr, CauDataQueueDepth);break;
        case SerDiv_A   : __GENoperandQueuePushDataFIFO_Name__(SerDiv_A   , serdivA_data_FIFO_opqueue, opdata, instr, SerdivDataQueueDepth);break;
        case SerDiv_B   : __GENoperandQueuePushDataFIFO_Name__(SerDiv_B   , serdivB_data_FIFO_opqueue, opdata, instr, SerdivDataQueueDepth);break;
        case Mask       : __GENoperandQueuePushDataFIFO_Name__(Mask       , mask_data_FIFO_opqueue   , opdata, instr, VmaskDataQueueDepth);break;
        // case ShuffleUnit: __GENoperandQueuePushDataFIFO_Name__(ShuffleUnit, shuffle_data_FIFO_opqueue, opdata, ShuffleDataQueueDepth);break;
        default: panic("unknown OperandType");
    }
}


#define __GENoperandQueuePopInstrFIFO_Name__(OPERAND_TYPE_NAME, RUNNING_DATA_NAME, FIFO_NAME) \
if(FIFO_NAME.size() == 0) \
    return false; \
if(RUNNING_DATA_NAME != nullptr) delete RUNNING_DATA_NAME; \
RUNNING_DATA_NAME = new VenusInstrPkt(FIFO_NAME.front()); \
delete FIFO_NAME.front(); \
FIFO_NAME.pop_front(); \
return true;

#define __GENoperandQueuePopDataFIFO_Name__(OPERAND_TYPE_NAME, RUNNING_DATA_NAME, FIFO_NAME, EXPECTED) \
if(FIFO_NAME.size() == 0) \
    return false; \
if(FIFO_NAME.front().visibleTick > curTick()) { \
    DPRINTF(LaneOperandRequester, \
            "operandQueue%s tagged data waits for FIFO Q: instr %d runningID %d visible %llu\n", \
            #OPERAND_TYPE_NAME, FIFO_NAME.front().vns_instr_id, \
            FIFO_NAME.front().running_id, \
            static_cast<unsigned long long>(FIFO_NAME.front().visibleTick)); \
    return false; \
} \
if(FIFO_NAME.front().running_id != EXPECTED->running_id || \
   FIFO_NAME.front().vns_instr_id != EXPECTED->vns_instr_id) { \
    DPRINTF(LaneOperandRequester, \
            "operandQueue%s tag mismatch: expected instr %d runningID %d, head instr %d runningID %d\n", \
            #OPERAND_TYPE_NAME, EXPECTED->vns_instr_id, EXPECTED->running_id, \
            FIFO_NAME.front().vns_instr_id, FIFO_NAME.front().running_id); \
    return false; \
} \
RUNNING_DATA_NAME = FIFO_NAME.front().data; \
FIFO_NAME.pop_front(); \
if(experimentalRequesterQVisibility && \
   !deferOperandQueuePopToBitAluAdmission(OPERAND_TYPE_NAME, EXPECTED)) \
    noteOperandQueuePop(OPERAND_TYPE_NAME); \
return true;

#define __GENoperandQueueUNPopInstrFIFO_Name__(OPERAND_TYPE_NAME, FIFO_NAME, DATA) \
FIFO_NAME.push_front(DATA); \
return true;

#define __GENoperandQueueUNPopDataFIFO_Name__(OPERAND_TYPE_NAME, FIFO_NAME, DATA, INSTR) \
FIFO_NAME.push_front(TaggedOperandData{DATA, INSTR->running_id, \
    INSTR->vns_instr_id, curTick()}); \
if(experimentalRequesterQVisibility && \
   !deferOperandQueuePopToBitAluAdmission(OPERAND_TYPE_NAME, INSTR)) \
    noteOperandQueueUnpop(OPERAND_TYPE_NAME); \
return true;

bool VenusLane::operandQueuePopinstrFIFO(OPERANDTYPE OperandType)
{
    switch (OperandType) {
        case BitAlu_A   : __GENoperandQueuePopInstrFIFO_Name__(BitAlu_A   , running_bitaluA_instr_pkt    , bitaluA_instr_FIFO_opqueue);break;
        case BitAlu_B   : __GENoperandQueuePopInstrFIFO_Name__(BitAlu_B   , running_bitaluB_instr_pkt    , bitaluB_instr_FIFO_opqueue);break;
        case CAU_A      : __GENoperandQueuePopInstrFIFO_Name__(CAU_A      , running_cauA_instr_pkt       , cauA_instr_FIFO_opqueue   );break;
        case CAU_B      : __GENoperandQueuePopInstrFIFO_Name__(CAU_B      , running_cauB_instr_pkt       , cauB_instr_FIFO_opqueue   );break;
        case CAU_C      : __GENoperandQueuePopInstrFIFO_Name__(CAU_C      , running_cauC_instr_pkt       , cauC_instr_FIFO_opqueue   );break;
        case CAU_D      : __GENoperandQueuePopInstrFIFO_Name__(CAU_D      , running_cauD_instr_pkt       , cauD_instr_FIFO_opqueue   );break;
        case SerDiv_A   : __GENoperandQueuePopInstrFIFO_Name__(SerDiv_A   , running_serdivA_instr_pkt    , serdivA_instr_FIFO_opqueue);break;
        case SerDiv_B   : __GENoperandQueuePopInstrFIFO_Name__(SerDiv_B   , running_serdivB_instr_pkt    , serdivB_instr_FIFO_opqueue);break;
        case Mask       : __GENoperandQueuePopInstrFIFO_Name__(Mask       , running_mask_instr_pkt       , mask_instr_FIFO_opqueue   );break;
        // case ShuffleUnit: __GENoperandQueuePopInstrFIFO_Name__(ShuffleUnit, running_shuffle_instr_pkt    , shuffle_instr_FIFO_opqueue);break;
        default: panic("unknown OperandType");
    }
}
bool VenusLane::operandQueueUNPopinstrFIFO(OPERANDTYPE OperandType, VenusInstrPkt* instr)
{
    switch (OperandType) {
        case BitAlu_A   : __GENoperandQueueUNPopInstrFIFO_Name__(BitAlu_A   , bitaluA_instr_FIFO_opqueue, instr);break;
        case BitAlu_B   : __GENoperandQueueUNPopInstrFIFO_Name__(BitAlu_B   , bitaluB_instr_FIFO_opqueue, instr);break;
        case CAU_A      : __GENoperandQueueUNPopInstrFIFO_Name__(CAU_A      , cauA_instr_FIFO_opqueue   , instr);break;
        case CAU_B      : __GENoperandQueueUNPopInstrFIFO_Name__(CAU_B      , cauB_instr_FIFO_opqueue   , instr);break;
        case CAU_C      : __GENoperandQueueUNPopInstrFIFO_Name__(CAU_C      , cauC_instr_FIFO_opqueue   , instr);break;
        case CAU_D      : __GENoperandQueueUNPopInstrFIFO_Name__(CAU_D      , cauD_instr_FIFO_opqueue   , instr);break;
        case SerDiv_A   : __GENoperandQueueUNPopInstrFIFO_Name__(SerDiv_A   , serdivA_instr_FIFO_opqueue, instr);break;
        case SerDiv_B   : __GENoperandQueueUNPopInstrFIFO_Name__(SerDiv_B   , serdivB_instr_FIFO_opqueue, instr);break;
        case Mask       : __GENoperandQueueUNPopInstrFIFO_Name__(Mask       , mask_instr_FIFO_opqueue   , instr);break;
        // case ShuffleUnit: __GENoperandQueueUNPopFIFO_Name__(ShuffleUnit, shuffle_instr_FIFO_opqueue, instr);break;
        default: panic("unknown OperandType");
    }
}
bool
VenusLane::operandQueuePopdataFIFO(OPERANDTYPE OperandType,
                                   const VenusInstrPkt* expected)
{
    if (expected == nullptr)
        panic("operandQueuePopdataFIFO requires an instruction tag");
    switch (OperandType) {
        case BitAlu_A   : __GENoperandQueuePopDataFIFO_Name__(BitAlu_A   , running_bitaluA_data_pkt , bitaluA_data_FIFO_opqueue, expected);break;
        case BitAlu_B   : __GENoperandQueuePopDataFIFO_Name__(BitAlu_B   , running_bitaluB_data_pkt , bitaluB_data_FIFO_opqueue, expected);break;
        case CAU_A      : __GENoperandQueuePopDataFIFO_Name__(CAU_A      , running_cauA_data_pkt    , cauA_data_FIFO_opqueue   , expected);break;
        case CAU_B      : __GENoperandQueuePopDataFIFO_Name__(CAU_B      , running_cauB_data_pkt    , cauB_data_FIFO_opqueue   , expected);break;
        case CAU_C      : __GENoperandQueuePopDataFIFO_Name__(CAU_C      , running_cauC_data_pkt    , cauC_data_FIFO_opqueue   , expected);break;
        case CAU_D      : __GENoperandQueuePopDataFIFO_Name__(CAU_D      , running_cauD_data_pkt    , cauD_data_FIFO_opqueue   , expected);break;
        case SerDiv_A   : __GENoperandQueuePopDataFIFO_Name__(SerDiv_A   , running_serdivA_data_pkt , serdivA_data_FIFO_opqueue, expected);break;
        case SerDiv_B   : __GENoperandQueuePopDataFIFO_Name__(SerDiv_B   , running_serdivB_data_pkt , serdivB_data_FIFO_opqueue, expected);break;
        case Mask       : panic("mask operands require a VFU-local latch");
        // case ShuffleUnit: __GENoperandQueuePopDataFIFO_Name__(ShuffleUnit, running_shuffle_data_pkt , shuffle_data_FIFO_opqueue);break;
        default: panic("unknown OperandType");
    }
}

bool
VenusLane::operandQueuePopMaskData(unsigned int &opdata,
                                   const VenusInstrPkt* expected)
{
    if (expected == nullptr)
        panic("operandQueuePopMaskData requires an instruction tag");
    if (mask_data_FIFO_opqueue.empty())
        return false;
    if (mask_data_FIFO_opqueue.front().running_id != expected->running_id ||
        mask_data_FIFO_opqueue.front().vns_instr_id != expected->vns_instr_id) {
        DPRINTF(LaneOperandRequester,
                "operandQueueMask tag mismatch: expected instr %d runningID %d, head instr %d runningID %d\n",
                expected->vns_instr_id, expected->running_id,
                mask_data_FIFO_opqueue.front().vns_instr_id,
                mask_data_FIFO_opqueue.front().running_id);
        return false;
    }
    opdata = mask_data_FIFO_opqueue.front().data;
    mask_data_FIFO_opqueue.pop_front();
    if (experimentalRequesterQVisibility &&
        !deferOperandQueuePopToBitAluAdmission(Mask, expected))
        noteOperandQueuePop(Mask);
    return true;
}

bool
VenusLane::prepareMaskOperand(unsigned int &slice,
                              unsigned int &rowBits, bool &rowValid,
                              bool &rowFetched,
                              const VenusInstrPkt *instr,
                              int calcCount)
{
    panic_if(instr == nullptr, "mask operand has no instruction tag");
    rowFetched = false;
    if (!(instr->vm_r || instr->vm_w)) {
        slice = 0;
        return true;
    }

    const int step = 2 - instr->vew;
    panic_if(step <= 0, "mask operand has invalid VEW %d", instr->vew);
    const unsigned ordinal = calcCount / step;
    const unsigned slot = ordinal % NrBankPerLane;

    if (slot == 0) {
        panic_if(rowValid,
                 "mask operand overwrote an unconsumed row for instr %d/rid %d",
                 instr->vns_instr_id, instr->running_id);
        if (!operandQueuePopMaskData(rowBits, instr))
            return false;
        rowBits &= 0xff;
        rowValid = true;
        rowFetched = true;
    } else if (!rowValid) {
        return false;
    }

    /* gem5 expands each logical mask bit to one byte in its backing store.
     * The VFU consumes two such byte-bits per 16-bit micro-operation. */
    slice = ((rowBits >> (2 * slot)) & 0x1) |
        (((rowBits >> (2 * slot + 1)) & 0x1) << 8);
    return true;
}

void
VenusLane::updateBitAluMaskLatchBoundary()
{
    if (bitalu_mask_row_boundary_tick == MaxTick ||
        curTick() < bitalu_mask_row_boundary_tick) {
        return;
    }

    running_bitalu_mask_row_pkt = bitalu_mask_row_d;
    running_bitalu_mask_row_valid = bitalu_mask_row_valid_d;
    running_bitalu_mask_row_running_id =
        bitalu_mask_row_running_id_d;
    running_bitalu_mask_row_instr_id = bitalu_mask_row_instr_id_d;
    bitalu_mask_row_boundary_tick = MaxTick;

    DPRINTF(LaneVFU,
            "BitALU mask latch Q valid %d instr %d/rid %d row %#x\n",
            running_bitalu_mask_row_valid,
            running_bitalu_mask_row_instr_id,
            running_bitalu_mask_row_running_id,
            running_bitalu_mask_row_pkt);
}

bool
VenusLane::prepareBitAluMaskOperand(unsigned int &slice,
                                    bool &rowHandshake,
                                    const VenusInstrPkt *instr,
                                    int calcCount)
{
    panic_if(instr == nullptr, "BitALU mask latch has no instruction tag");
    updateBitAluMaskLatchBoundary();
    rowHandshake = false;

    if (!(instr->vm_r || instr->vm_w)) {
        panic_if(running_bitalu_mask_row_valid ||
                     bitalu_mask_row_boundary_tick != MaxTick,
                 "unmasked BitALU instr %d/rid %d inherited a mask row",
                 instr->vns_instr_id, instr->running_id);
        slice = 0;
        return true;
    }

    if (running_bitalu_mask_row_valid) {
        if (running_bitalu_mask_row_running_id != instr->running_id ||
            running_bitalu_mask_row_instr_id != instr->vns_instr_id) {
            DPRINTF(LaneVFU,
                    "BitALU mask latch tag mismatch: active instr %d/rid %d, "
                    "row instr %d/rid %d\n",
                    instr->vns_instr_id, instr->running_id,
                    running_bitalu_mask_row_instr_id,
                    running_bitalu_mask_row_running_id);
            return false;
        }

        const int step = 2 - instr->vew;
        panic_if(step <= 0, "BitALU mask has invalid VEW %d", instr->vew);
        const unsigned ordinal = calcCount / step;
        const unsigned slot = ordinal % NrBankPerLane;
        slice = ((running_bitalu_mask_row_pkt >> (2 * slot)) & 0x1) |
            (((running_bitalu_mask_row_pkt >> (2 * slot + 1)) & 0x1) << 8);
        return true;
    }

    /* The RTL wrapper handshakes mask input into operand_mask_valid_d while
     * arithmetic continues to observe operand_mask_valid_q.  Therefore a
     * newly accepted row cannot be consumed on this edge.  This handshake is
     * independent of A/B readiness and can prefetch the row while either
     * ordinary operand waits. */
    if (bitalu_mask_row_boundary_tick == MaxTick) {
        unsigned int row = 0;
        if (operandQueuePopMaskData(row, instr)) {
            bitalu_mask_row_d = row & 0xff;
            bitalu_mask_row_valid_d = true;
            bitalu_mask_row_running_id_d = instr->running_id;
            bitalu_mask_row_instr_id_d = instr->vns_instr_id;
            bitalu_mask_row_boundary_tick = afterCycles(Cycles(1));
            rowHandshake = true;
            if (experimentalRequesterQVisibility)
                noteOperandQueuePop(Mask);
            DPRINTF(LaneVFU,
                    "BitALU mask latch handshake D instr %d/rid %d row %#x "
                    "visible %llu\n",
                    instr->vns_instr_id, instr->running_id,
                    bitalu_mask_row_d,
                    static_cast<unsigned long long>(
                        bitalu_mask_row_boundary_tick));
        }
    }
    return false;
}

void
VenusLane::commitBitAluMaskOperand(const VenusInstrPkt *instr,
                                   int calcCount)
{
    if (!(instr->vm_r || instr->vm_w))
        return;
    panic_if(!running_bitalu_mask_row_valid ||
                 running_bitalu_mask_row_running_id != instr->running_id ||
                 running_bitalu_mask_row_instr_id != instr->vns_instr_id,
             "BitALU consumed an untagged mask row for instr %d/rid %d",
             instr->vns_instr_id, instr->running_id);

    const int step = 2 - instr->vew;
    const unsigned ordinal = calcCount / step;
    const unsigned slot = ordinal % NrBankPerLane;
    const bool final = calcCount + step >=
        instr->locallane_bitalu_calctime_len;
    if (slot != NrBankPerLane - 1 && !final)
        return;

    /* A clear also crosses D->Q.  Since the RTL refill condition examines
     * the old Q value, the next row cannot handshake until the following
     * edge; this creates the real mask-row boundary bubble. */
    panic_if(bitalu_mask_row_boundary_tick != MaxTick,
             "BitALU mask latch already has a pending D transition");
    bitalu_mask_row_d = running_bitalu_mask_row_pkt;
    bitalu_mask_row_valid_d = false;
    bitalu_mask_row_running_id_d = instr->running_id;
    bitalu_mask_row_instr_id_d = instr->vns_instr_id;
    bitalu_mask_row_boundary_tick = afterCycles(Cycles(1));
}

void
VenusLane::updateCauMaskLatchBoundary()
{
    if (cau_mask_row_boundary_tick == MaxTick ||
        curTick() < cau_mask_row_boundary_tick) {
        return;
    }

    running_cau_mask_row_pkt = cau_mask_row_d;
    running_cau_mask_row_valid = cau_mask_row_valid_d;
    running_cau_mask_row_running_id = cau_mask_row_running_id_d;
    running_cau_mask_row_instr_id = cau_mask_row_instr_id_d;
    cau_mask_row_boundary_tick = MaxTick;

    DPRINTF(LaneVFU,
            "CAU mask latch Q valid %d instr %d/rid %d row %#x\n",
            running_cau_mask_row_valid,
            running_cau_mask_row_instr_id,
            running_cau_mask_row_running_id,
            running_cau_mask_row_pkt);
}

bool
VenusLane::prepareCauMaskOperand(unsigned int &slice,
                                 bool &rowHandshake,
                                 const VenusInstrPkt *instr,
                                 int calcCount)
{
    panic_if(instr == nullptr, "CAU mask latch has no instruction tag");
    updateCauMaskLatchBoundary();
    rowHandshake = false;

    if (!(instr->vm_r || instr->vm_w)) {
        panic_if(running_cau_mask_row_valid ||
                     cau_mask_row_boundary_tick != MaxTick,
                 "unmasked CAU instr %d/rid %d inherited a mask row",
                 instr->vns_instr_id, instr->running_id);
        slice = 0;
        return true;
    }

    if (running_cau_mask_row_valid) {
        if (running_cau_mask_row_running_id != instr->running_id ||
            running_cau_mask_row_instr_id != instr->vns_instr_id) {
            DPRINTF(LaneVFU,
                    "CAU mask latch tag mismatch: active instr %d/rid %d, "
                    "row instr %d/rid %d\n",
                    instr->vns_instr_id, instr->running_id,
                    running_cau_mask_row_instr_id,
                    running_cau_mask_row_running_id);
            return false;
        }

        const int step = 2 - instr->vew;
        panic_if(step <= 0, "CAU mask has invalid VEW %d", instr->vew);
        const unsigned ordinal = calcCount / step;
        const unsigned slot = ordinal % NrBankPerLane;
        slice = ((running_cau_mask_row_pkt >> (2 * slot)) & 0x1) |
            (((running_cau_mask_row_pkt >> (2 * slot + 1)) & 0x1) << 8);
        return true;
    }

    /* venus_cau_wrapper captures operand_mask_valid_d independently of the
     * ordinary operands.  Arithmetic continues to observe the old Q value,
     * so a newly handshaked row is first consumable on the next lane edge. */
    if (cau_mask_row_boundary_tick == MaxTick) {
        unsigned int row = 0;
        if (operandQueuePopMaskData(row, instr)) {
            cau_mask_row_d = row & 0xff;
            cau_mask_row_valid_d = true;
            cau_mask_row_running_id_d = instr->running_id;
            cau_mask_row_instr_id_d = instr->vns_instr_id;
            cau_mask_row_boundary_tick = afterCycles(Cycles(1));
            rowHandshake = true;
            DPRINTF(LaneVFU,
                    "CAU mask latch handshake D instr %d/rid %d row %#x "
                    "visible %llu\n",
                    instr->vns_instr_id, instr->running_id,
                    cau_mask_row_d,
                    static_cast<unsigned long long>(
                        cau_mask_row_boundary_tick));
        }
    }
    return false;
}

void
VenusLane::commitCauMaskOperand(const VenusInstrPkt *instr,
                                int calcCount)
{
    if (!(instr->vm_r || instr->vm_w))
        return;
    panic_if(!running_cau_mask_row_valid ||
                 running_cau_mask_row_running_id != instr->running_id ||
                 running_cau_mask_row_instr_id != instr->vns_instr_id,
             "CAU consumed an untagged mask row for instr %d/rid %d",
             instr->vns_instr_id, instr->running_id);

    const int step = 2 - instr->vew;
    const unsigned ordinal = calcCount / step;
    const unsigned slot = ordinal % NrBankPerLane;
    const bool final = calcCount + step >=
        instr->locallane_cau_calctime_len;
    if (slot != NrBankPerLane - 1 && !final)
        return;

    /* The wrapper clears operand_mask_valid_d from the final arithmetic
     * handshake.  The old Q remains visible for that beat; refill can only
     * enter D after Q clears on the next edge, producing the real row bubble. */
    panic_if(cau_mask_row_boundary_tick != MaxTick,
             "CAU mask latch already has a pending D transition");
    cau_mask_row_d = running_cau_mask_row_pkt;
    cau_mask_row_valid_d = false;
    cau_mask_row_running_id_d = instr->running_id;
    cau_mask_row_instr_id_d = instr->vns_instr_id;
    cau_mask_row_boundary_tick = afterCycles(Cycles(1));
}

void
VenusLane::rollbackMaskOperand(unsigned int rowBits, bool &rowValid,
                               bool rowFetched,
                               const VenusInstrPkt *instr)
{
    if (!rowFetched)
        return;
    panic_if(!rowValid, "rolling back an invalid mask row");
    operandQueueUNPopdataFIFO(Mask, rowBits, instr);
    rowValid = false;
}

void
VenusLane::commitMaskOperand(bool &rowValid,
                             const VenusInstrPkt *instr,
                             int calcCount)
{
    if (!(instr->vm_r || instr->vm_w))
        return;
    const int step = 2 - instr->vew;
    const unsigned ordinal = calcCount / step;
    const unsigned slot = ordinal % NrBankPerLane;
    const bool final = calcCount + step >=
        ((std::find(instr->vfu_lst.begin(), instr->vfu_lst.end(),
                    VFU_BitALU) != instr->vfu_lst.end()) ?
             instr->locallane_bitalu_calctime_len :
         (std::find(instr->vfu_lst.begin(), instr->vfu_lst.end(),
                    VFU_CAU) != instr->vfu_lst.end()) ?
             instr->locallane_cau_calctime_len :
             instr->locallane_serdiv_calctime_len);
    if (slot == NrBankPerLane - 1 || final)
        rowValid = false;
}

bool
VenusLane::operandQueueUNPopdataFIFO(OPERANDTYPE OperandType,
                                     unsigned int opdata,
                                     const VenusInstrPkt* instr)
{
    if (instr == nullptr)
        panic("operandQueueUNPopdataFIFO requires an instruction tag");
    switch (OperandType) {
        case BitAlu_A   : __GENoperandQueueUNPopDataFIFO_Name__(BitAlu_A   , bitaluA_data_FIFO_opqueue, opdata, instr);break;
        case BitAlu_B   : __GENoperandQueueUNPopDataFIFO_Name__(BitAlu_B   , bitaluB_data_FIFO_opqueue, opdata, instr);break;
        case CAU_A      : __GENoperandQueueUNPopDataFIFO_Name__(CAU_A      , cauA_data_FIFO_opqueue   , opdata, instr);break;
        case CAU_B      : __GENoperandQueueUNPopDataFIFO_Name__(CAU_B      , cauB_data_FIFO_opqueue   , opdata, instr);break;
        case CAU_C      : __GENoperandQueueUNPopDataFIFO_Name__(CAU_C      , cauC_data_FIFO_opqueue   , opdata, instr);break;
        case CAU_D      : __GENoperandQueueUNPopDataFIFO_Name__(CAU_D      , cauD_data_FIFO_opqueue   , opdata, instr);break;
        case SerDiv_A   : __GENoperandQueueUNPopDataFIFO_Name__(SerDiv_A   , serdivA_data_FIFO_opqueue, opdata, instr);break;
        case SerDiv_B   : __GENoperandQueueUNPopDataFIFO_Name__(SerDiv_B   , serdivB_data_FIFO_opqueue, opdata, instr);break;
        case Mask       : __GENoperandQueueUNPopDataFIFO_Name__(Mask       , mask_data_FIFO_opqueue   , opdata, instr);break;
        // case ShuffleUnit: __GENoperandQueueUNPopFIFO_Name__(ShuffleUnit, shuffle_data_FIFO_opqueue, opdata);break;
        default: panic("unknown OperandType");
    }
}

bool
VenusLane::vfuOperandInstructionsReady(VFU vfu) const
{
    auto matches = [](const std::list<VenusInstrPkt*>& fifo,
                      const VenusInstrPkt* expected) {
        return !fifo.empty() &&
               fifo.front()->running_id == expected->running_id &&
               fifo.front()->vns_instr_id == expected->vns_instr_id;
    };

    const VenusInstrPkt* pkt = nullptr;
    if (vfu == VFU_BitALU) {
        if (bitaluB_instr_FIFO_opqueue.empty())
            return false;
        pkt = bitaluB_instr_FIFO_opqueue.front();
        /*
         * gem5 fans the VFU instruction into every operand requester,
         * including zero-length operands.  Require that whole tagged command
         * set before popping any member; otherwise an independently delayed
         * requester leaves a stale command that can later pair with a reused
         * running ID.
         */
        if (!matches(bitaluA_instr_FIFO_opqueue, pkt))
            return false;
    } else if (vfu == VFU_CAU) {
        if (cauB_instr_FIFO_opqueue.empty())
            return false;
        pkt = cauB_instr_FIFO_opqueue.front();
        if (!matches(cauA_instr_FIFO_opqueue, pkt))
            return false;
        if (!matches(cauC_instr_FIFO_opqueue, pkt))
            return false;
        if (!matches(cauD_instr_FIFO_opqueue, pkt))
            return false;
    } else if (vfu == VFU_SerDiv) {
        if (serdivB_instr_FIFO_opqueue.empty())
            return false;
        pkt = serdivB_instr_FIFO_opqueue.front();
        if (!matches(serdivA_instr_FIFO_opqueue, pkt))
            return false;
    } else {
        return true;
    }

    if ((pkt->vm_r || pkt->vm_w) &&
        !matches(mask_instr_FIFO_opqueue, pkt))
        return false;
    return true;
}

bool
VenusLane::advanceCAUResultHold()
{
    /*
     * RTL keeps the arithmetic pipeline and the operand consumer
     * independently elastic.  Accepting a younger CAU command therefore
     * cannot stop older tagged results from walking towards the result
     * queue.
     *
     * A real input advances datapipe in VFUCAUCalculating().  When the
     * active command is waiting for an operand, inject one invalid beat
     * instead.  The normal output handshake continues to own and hold each
     * valid result together with its instruction tag.
     */
    if (vfu_cau_busy_calculating ||
        (cau_vd1_result_buf_pipeline.size() == 0 &&
         cau_vd2_result_buf_pipeline.size() == 0)) {
        return false;
    }

    vfu_cau_busy_calculating = true;
    cau_vd1_result_calc_done = true;

    if (cau_vd1_result_buf_pipeline.size() > 0) {
        cau_vd1_result_buf_pipeline.push_pipe(INT_MIN, INT_MIN, nullptr);
        cau_vd1_result_writeback_done = false;
    } else {
        cau_vd1_result_writeback_done = true;
    }
    if (cau_vd2_result_buf_pipeline.size() > 0) {
        cau_vd2_result_buf_pipeline.push_pipe(INT_MIN, INT_MIN, nullptr);
        cau_vd2_result_writeback_done = false;
    } else {
        cau_vd2_result_writeback_done = true;
    }

    DPRINTF(Lane,
            "CAU tagged result hold advances without a new operand "
            "(vd1 occupancy %d, vd2 occupancy %d)\n",
            cau_vd1_result_buf_pipeline.size(),
            cau_vd2_result_buf_pipeline.size());
    if (!nextVFUCAUStartCalcEvent.scheduled()) {
        schedule(nextVFUCAUStartCalcEvent, afterCycles(Cycles(1)));
    }
    return true;
}


#define __GENVFUCalc_Name__(VFU_TYPE_NAME, RUNNING_PKT_NAME, BUSY_NAME, VFU_EVENT_NAME) \
if(BUSY_NAME == false) \
/*没有正在进行的任务，获取新任务*/ \
{ \
    if(VFU_TYPE_NAME == VFU_BitALU) \
    { \
        /*
         * RTL keeps every lane in the reduction transaction until lane 0
         * finishes the inter-lane/SIMD reduction.  A short tail lane may
         * consume its local operands earlier, but it must not accept the
         * next VRED* and reset its accumulator while lane 0 is still
         * collecting partial results.
         */ \
        if (RUNNING_PKT_NAME != nullptr && \
            RUNNING_PKT_NAME->op >= VREDAND && \
            RUNNING_PKT_NAME->op <= VREDSUM && \
            venus_hazard_table != nullptr && \
            venus_hazard_table->running_id_to_vns_instr_id \
                [RUNNING_PKT_NAME->running_id] == \
                RUNNING_PKT_NAME->vns_instr_id) { \
            if(!VFU_EVENT_NAME.scheduled()) \
                schedule(VFU_EVENT_NAME, afterCycles(Cycles(1))); \
            return; \
        } \
        if(!vfuOperandInstructionsReady(VFU_BitALU)) { /*没有完整的新任务*/ \
            /*如果pipe里有未写回的数据，就写回*/ \
            if(bitalu_vd1_result_buf_pipeline.size() > 0 || bitalu_vmask_result_buf_pipeline.size() > 0 || vfu_bitalu_busy_calculating == true) { \
                if(vfu_bitalu_busy_calculating == true) { \
                    /*上一组数据部分没写回，直接等待全部写回*/ \
                    /*std::cout<<"VFU_BitALU 上一组数据部分没写回，直接等待全部写回"<<std::endl;*/ \
                } else { \
                    /* The arithmetic/result pipeline owns its own clocked \
                     * advance.  Keep the operand consumer alive, but do not \
                     * shift the tagged pipe from this lower-priority event: \
                     * doing so can advance twice on one RTL edge. */ \
                    bitalu_vd1_result_calc_done = true; \
                    if(!nextVFUBitAluStartCalcEvent.scheduled()) \
                        schedule(nextVFUBitAluStartCalcEvent,afterCycles(Cycles(1))); \
                } \
                if(!VFU_EVENT_NAME.scheduled()) \
                    schedule(VFU_EVENT_NAME,afterCycles(Cycles(1))); \
                else \
                    panic("schedule(VFU_EVENT_NAME,curTick()+1*1000);"); \
                return; \
            } \
        } else { /*有新任务*/ \
            /* The RTL instruction queue advances independently from tagged \
             * results of the preceding instruction.  A same-latency younger \
             * instruction can therefore enter while those tagged beats are \
             * still moving through the arithmetic pipe.  setPipeLength() \
             * preserves every slot when the length is unchanged; only a \
             * true latency-domain transition must drain first. */ \
            const int nextBitaluPipeLength = getVFUPipeLength( \
                bitaluB_instr_FIFO_opqueue.front()); \
            const bool bitaluPipeLengthTransition = \
                bitalu_vd1_result_buf_pipeline.pipeLength() != \
                    nextBitaluPipeLength; \
            if(vfu_bitalu_busy_calculating == true || \
               (bitaluPipeLengthTransition && \
                ((bitalu_vd1_result_buf_pipeline.size() > 0) || \
                 (bitalu_vmask_result_buf_pipeline.size() > 0)))) \
            { \
                if(vfu_bitalu_busy_calculating == true) { \
                    /*上一组数据部分没写回，直接等待全部写回*/ \
                    /*std::cout<<"VFU_BitALU 上一组数据部分没写回，新任务不能开始"<<std::endl;*/ \
                } else { \
                    bitalu_vd1_result_calc_done = true; \
                    if(!nextVFUBitAluStartCalcEvent.scheduled()) \
                        schedule(nextVFUBitAluStartCalcEvent,afterCycles(Cycles(1))); \
                } \
                if(!VFU_EVENT_NAME.scheduled()) \
                    schedule(VFU_EVENT_NAME,afterCycles(Cycles(1))); \
                else \
                    panic("schedule(VFU_EVENT_NAME,curTick()+1*1000);"); \
                return; \
            } \
        } \
        if(vfuOperandInstructionsReady(VFU_BitALU) && operandQueuePopinstrFIFO(BitAlu_B) == true) \
        { \
            /*获取新任务成功*/ \
            operandQueuePopinstrFIFO(BitAlu_A); \
            BUSY_NAME = true; \
            if(RUNNING_PKT_NAME != nullptr) delete RUNNING_PKT_NAME; \
            RUNNING_PKT_NAME = new VenusInstrPkt(running_bitaluB_instr_pkt); \
            RUNNING_PKT_NAME->locallane_bitalu_calctime_len = getLocalLaneCalcLen(RUNNING_PKT_NAME->vl, RUNNING_PKT_NAME->vew, RUNNING_PKT_NAME->op); \
            RUNNING_PKT_NAME->locallane_bitalu_calc_cnt = 0; \
            RUNNING_PKT_NAME->vns_instr_stat = INSTR_FIRED; \
            RUNNING_PKT_NAME->vns_instr_log_starttick = curTick(); \
            bitalu_vd1_result_buf_pipeline.setPipeLength(getVFUPipeLength(RUNNING_PKT_NAME)); \
            bitalu_vmask_result_buf_pipeline.setPipeLength(getVFUPipeLength(RUNNING_PKT_NAME)); \
            venus_function_unit.resetReduceStat(); \
            DPRINTF(LaneVFU, "VFU_BitALU Unit, has received a new instr : instr %d with runningID %d\n", RUNNING_PKT_NAME->vns_instr_id, RUNNING_PKT_NAME->running_id); \
        } else { \
            /*没有新任务可获取*/ \
            if(!VFU_EVENT_NAME.scheduled()) \
                schedule(VFU_EVENT_NAME,afterCycles(Cycles(1))); \
            else \
                panic("schedule(VFU_EVENT_NAME,curTick()+1*1000);"); \
            return; \
        } \
    } \
    if(VFU_TYPE_NAME == VFU_CAU) \
    { \
        if(!vfuOperandInstructionsReady(VFU_CAU)) { /*没有完整的新任务*/ \
            /*如果pipe里有未写回的数据，就写回*/ \
            if(cau_vd1_result_buf_pipeline.size() > 0 || cau_vd2_result_buf_pipeline.size() > 0 || vfu_cau_busy_calculating == true) { \
                if(vfu_cau_busy_calculating == true) { \
                    /*上一组数据部分没写回，直接等待全部写回*/ \
                    /*std::cout<<"VFU_CAU 上一组数据部分没写回，直接等待全部写回"<<std::endl;*/ \
                } else { \
                    /*std::cout<<"VFU_CAU 上一组数据全都写回了，写回pipe里有未写回的数据"<<std::endl;*/ \
                    vfu_cau_busy_calculating = true; cau_vd1_result_calc_done = true; \
                    if(cau_vd1_result_buf_pipeline.size() > 0) {cau_vd1_result_buf_pipeline.push_pipe(INT_MIN, INT_MIN, nullptr); cau_vd1_result_writeback_done = false;} \
                    if(cau_vd2_result_buf_pipeline.size() > 0) {cau_vd2_result_buf_pipeline.push_pipe(INT_MIN, INT_MIN, nullptr); cau_vd2_result_writeback_done = false;} \
                    if(!nextVFUCAUStartCalcEvent.scheduled()) \
                        schedule(nextVFUCAUStartCalcEvent,afterCycles(Cycles(1))); \
                } \
                if(!VFU_EVENT_NAME.scheduled()) \
                    schedule(VFU_EVENT_NAME,afterCycles(Cycles(1))); \
                else \
                    panic("schedule(VFU_EVENT_NAME,curTick()+1*1000);"); \
                return; \
            } \
        } else { /*有新任务*/ \
            const int next_cau_pipe_length = \
                getCauPipeLength(cauB_instr_FIFO_opqueue.front()); \
            const bool pending_cau_generation = \
                cau_vd1_result_buf_pipeline.size() > 0 || \
                cau_vd2_result_buf_pipeline.size() > 0; \
            const bool cau_pipe_shape_change = pending_cau_generation && \
                (cau_vd1_result_buf_pipeline.pipeLength() != \
                     next_cau_pipe_length || \
                 cau_vd2_result_buf_pipeline.pipeLength() != \
                     next_cau_pipe_length); \
            /* The pipe shape belongs to the tagged results already in \
             * flight.  A younger command may be admitted into its operand \
             * queues, but it cannot resize the shared arithmetic/result \
             * pipe until the older generation has drained. */ \
            if(cau_pipe_shape_change || \
               (cau_vd1_result_buf_pipeline.size() > next_cau_pipe_length) || \
               (cau_vd2_result_buf_pipeline.size() > next_cau_pipe_length) || \
               (vfu_cau_busy_calculating == true) ) \
            { \
                if(vfu_cau_busy_calculating == true) { \
                    /*上一组数据部分没写回，直接等待全部写回*/ \
                    /*std::cout<<"VFU_CAU 上一组数据部分没写回，新任务不能开始"<<std::endl;*/ \
                } else { \
                    /*上一组数据全都写回了，写回pipe里有未写回的数据*/ \
                    /*std::cout<<"VFU_CAU 上一组数据全都写回了，写回pipe里有未写回的数据, 新任务不能开始"<<std::endl;*/ \
                    vfu_cau_busy_calculating = true; cau_vd1_result_calc_done = true; \
                    if(cau_vd1_result_buf_pipeline.size() > 0) {cau_vd1_result_buf_pipeline.push_pipe(INT_MIN, INT_MIN, nullptr); cau_vd1_result_writeback_done = false;} \
                    if(cau_vd2_result_buf_pipeline.size() > 0) {cau_vd2_result_buf_pipeline.push_pipe(INT_MIN, INT_MIN, nullptr); cau_vd2_result_writeback_done = false;} \
                    if(!nextVFUCAUStartCalcEvent.scheduled()) \
                        schedule(nextVFUCAUStartCalcEvent,afterCycles(Cycles(1))); \
                } \
                if(!VFU_EVENT_NAME.scheduled()) \
                    schedule(VFU_EVENT_NAME,afterCycles(Cycles(1))); \
                else \
                    panic("schedule(VFU_EVENT_NAME,curTick()+1*1000);"); \
                return; \
            } \
        } \
        if(vfuOperandInstructionsReady(VFU_CAU) && operandQueuePopinstrFIFO(CAU_B) == true) \
        { \
            /*获取新任务成功*/ \
            operandQueuePopinstrFIFO(CAU_A); \
            operandQueuePopinstrFIFO(CAU_C); \
            operandQueuePopinstrFIFO(CAU_D); \
            BUSY_NAME = true; \
            if(RUNNING_PKT_NAME != nullptr) delete RUNNING_PKT_NAME; \
            RUNNING_PKT_NAME = new VenusInstrPkt(running_cauB_instr_pkt); \
            RUNNING_PKT_NAME->locallane_cau_calctime_len = getLocalLaneCalcLen(RUNNING_PKT_NAME->vl, RUNNING_PKT_NAME->vew, RUNNING_PKT_NAME->op); \
            RUNNING_PKT_NAME->locallane_cau_calc_cnt = 0; \
            RUNNING_PKT_NAME->vns_instr_stat = INSTR_FIRED; \
            RUNNING_PKT_NAME->vns_instr_log_starttick = curTick(); \
            /* The explicit depth-2 tagged result queue below is the \
             * registered CAU result-hold boundary.  Adding a second latency \
             * slot only for RAW-overlap commands double-counts that boundary \
             * and makes an otherwise identical arithmetic operation one \
             * lane cycle slower.  Keep the fixed arithmetic pipe shape \
             * independent of requester overlap. */ \
            const int cau_result_pipe_length = \
                getCauPipeLength(RUNNING_PKT_NAME); \
            cau_vd1_result_buf_pipeline.setPipeLength(cau_result_pipe_length); \
            cau_vd2_result_buf_pipeline.setPipeLength(cau_result_pipe_length); \
            DPRINTF(LaneVFU, "VFU_CAU Unit, has received a new instr : instr %d with runningID %d\n", RUNNING_PKT_NAME->vns_instr_id, RUNNING_PKT_NAME->running_id); \
        } else { \
            /*没有新任务可获取*/ \
            if(!VFU_EVENT_NAME.scheduled()) \
                schedule(VFU_EVENT_NAME,afterCycles(Cycles(1))); \
            else \
                panic("schedule(VFU_EVENT_NAME,curTick()+1*1000);"); \
            return; \
        } \
    } \
    if(VFU_TYPE_NAME == VFU_SerDiv) \
    { \
        if(vfu_serdiv_busy_calculating == true) \
        { \
            /*上个任务还没全部写回，新任务不能开始*/ \
            /*std::cout<<"VFU_SerDiv 上个任务还没全部写回，新任务不能开始"<<std::endl;*/ \
            if(!VFU_EVENT_NAME.scheduled()) \
                schedule(VFU_EVENT_NAME,afterCycles(Cycles(1))); \
            else \
                panic("schedule(VFU_EVENT_NAME,curTick()+1*1000);"); \
            return; \
        } \
        if(vfuOperandInstructionsReady(VFU_SerDiv) && operandQueuePopinstrFIFO(SerDiv_B) == true) \
        { \
            /*获取新任务成功*/ \
            operandQueuePopinstrFIFO(SerDiv_A); \
            BUSY_NAME = true; \
            if(RUNNING_PKT_NAME != nullptr) delete RUNNING_PKT_NAME; \
            RUNNING_PKT_NAME = new VenusInstrPkt(running_serdivB_instr_pkt); \
            RUNNING_PKT_NAME->locallane_serdiv_calctime_len = getLocalLaneCalcLen(RUNNING_PKT_NAME->vl, RUNNING_PKT_NAME->vew, RUNNING_PKT_NAME->op); \
            RUNNING_PKT_NAME->locallane_serdiv_calc_cnt = 0; \
            RUNNING_PKT_NAME->vns_instr_stat = INSTR_FIRED; \
            RUNNING_PKT_NAME->vns_instr_log_starttick = curTick(); \
            DPRINTF(LaneVFU, "VFU_SerDiv Unit, has received a new instr: instr %d with runningID %d\n", RUNNING_PKT_NAME->vns_instr_id, RUNNING_PKT_NAME->running_id); \
        } else { \
            /*没有新任务可获取*/ \
            if(!VFU_EVENT_NAME.scheduled()) \
                schedule(VFU_EVENT_NAME,afterCycles(Cycles(1))); \
            else \
                panic("schedule(VFU_EVENT_NAME,curTick()+1*1000);"); \
            return; \
        } \
    } \
    if(VFU_TYPE_NAME == VFU_ShuffleUnit) \
    { \
        if(operandQueuePopinstrFIFO(ShuffleUnit) == true) \
        { \
            /*获取新任务成功*/ \
            BUSY_NAME = true; \
            if(RUNNING_PKT_NAME != nullptr) delete RUNNING_PKT_NAME; \
            RUNNING_PKT_NAME = new VenusInstrPkt(running_shuffle_instr_pkt); \
            RUNNING_PKT_NAME->locallane_shuffle_calctime_len = getLocalLaneCalcLen(RUNNING_PKT_NAME->vl, RUNNING_PKT_NAME->vew, RUNNING_PKT_NAME->op); \
            RUNNING_PKT_NAME->locallane_shuffle_calc_cnt = 0; \
            RUNNING_PKT_NAME->vns_instr_stat = INSTR_FIRED; \
            RUNNING_PKT_NAME->vns_instr_log_starttick = curTick(); \
            DPRINTF(LaneVFU, "VFU_ShuffleUnit Unit, has received a new instr: instr %d with runningID %d\n", RUNNING_PKT_NAME->vns_instr_id, RUNNING_PKT_NAME->running_id); \
        } else { \
            /*没有新任务可获取*/ \
            if(!VFU_EVENT_NAME.scheduled()) \
                schedule(VFU_EVENT_NAME,afterCycles(Cycles(1))); \
            else \
                panic("schedule(VFU_EVENT_NAME,curTick()+1*1000);"); \
            return; \
        } \
    } \
    if ((RUNNING_PKT_NAME->vm_r || RUNNING_PKT_NAME->vm_w) == true) \
    { \
        if(operandQueuePopinstrFIFO(Mask) == true) {/*std::cout<<"at tick = "<<curTick()<<", operandQueuePopinstrFIFO(Mask) success! \n";*/} \
        else {panic("for a masked instruction, operandQueuePopinstrFIFO(Mask) failed!");} \
        /*std::cout<<"at tick = "<<curTick()<<", VFU pop instr from mask operandqueue: instr "<<RUNNING_PKT_NAME->vns_instr_id <<" with runningID "<< RUNNING_PKT_NAME->running_id <<std::endl;*/ \
    } \
    if(VFU_TYPE_NAME == VFU_BitALU) { \
        updateBitAluMaskLatchBoundary(); \
        panic_if(running_bitalu_mask_row_valid || \
                     bitalu_mask_row_boundary_tick != MaxTick, \
                 "new BitALU instr %d/rid %d entered with a live mask latch", \
                 RUNNING_PKT_NAME->vns_instr_id, \
                 RUNNING_PKT_NAME->running_id); \
    } \
    if(VFU_TYPE_NAME == VFU_CAU) { \
        updateCauMaskLatchBoundary(); \
        panic_if(running_cau_mask_row_valid || \
                     cau_mask_row_boundary_tick != MaxTick, \
                 "new CAU instr %d/rid %d entered with a live mask latch", \
                 RUNNING_PKT_NAME->vns_instr_id, \
                 RUNNING_PKT_NAME->running_id); \
    } \
    if(VFU_TYPE_NAME == VFU_SerDiv) running_serdiv_mask_row_valid = false; \
} \
if(VFU_TYPE_NAME == VFU_BitALU && (RUNNING_PKT_NAME->locallane_bitalu_calc_cnt < RUNNING_PKT_NAME->locallane_bitalu_calctime_len)) \
{ \
    bool vs1_ready = true; \
    bool vs2_ready = true; \
    bool vs1_popped = false; \
    bool vs2_popped = false; \
    bool mask_ready = true; \
    bool mask_row_handshake = false; \
    bool result_ready = bitAluOperandAdmissionReady(RUNNING_PKT_NAME); \
    unsigned int last_bitaluA_data_pkt = running_bitaluA_data_pkt; \
    unsigned int last_bitaluB_data_pkt = running_bitaluB_data_pkt; \
    if(RUNNING_PKT_NAME->vm_r || RUNNING_PKT_NAME->vm_w) { \
        /* In venus_bitalu_wrapper the operand-mask D/Q handshake is inside \
         * the same !result_queue_full guard as ordinary operand admission. \
         * Do not prefetch a mask row while result-count Q is full: that \
         * would make operand_mask_valid_q, and hence the first arithmetic \
         * result, visible one edge before RTL. */ \
        if(result_ready) \
            mask_ready = prepareBitAluMaskOperand( \
                running_bitalu_mask_data_pkt, mask_row_handshake, \
                RUNNING_PKT_NAME, \
                RUNNING_PKT_NAME->locallane_bitalu_calc_cnt); \
        else \
            mask_ready = false; \
    } else { running_bitalu_mask_data_pkt = 0; } \
    if(mask_ready && RUNNING_PKT_NAME->use_vs1) { \
        vs1_ready = operandQueuePopdataFIFO(BitAlu_A, RUNNING_PKT_NAME); \
        vs1_popped = vs1_ready; \
    } else if(!RUNNING_PKT_NAME->use_vs1) { running_bitaluA_data_pkt = 0; } \
    if(mask_ready && RUNNING_PKT_NAME->use_vs2) { \
        vs2_ready = operandQueuePopdataFIFO(BitAlu_B, RUNNING_PKT_NAME); \
        vs2_popped = vs2_ready; \
    } else if(!RUNNING_PKT_NAME->use_vs2) { running_bitaluB_data_pkt = 0; } \
    if(vs1_ready == false || vs2_ready == false || mask_ready == false || vfu_bitalu_busy_calculating == true || result_ready == false) { \
        bitAluAdmissionBlockedOnResultFull = vs1_ready && vs2_ready && \
            mask_ready && !vfu_bitalu_busy_calculating && !result_ready; \
        if(vs1_popped) operandQueueUNPopdataFIFO(BitAlu_A, running_bitaluA_data_pkt, RUNNING_PKT_NAME); \
        if(vs2_popped) operandQueueUNPopdataFIFO(BitAlu_B, running_bitaluB_data_pkt, RUNNING_PKT_NAME); \
        running_bitaluA_data_pkt = last_bitaluA_data_pkt; \
        running_bitaluB_data_pkt = last_bitaluB_data_pkt; \
        DPRINTF(LaneVSPM, "VFU_BitALU Unit, fails to pops new data from operandQueue where vs1_ready = %d , vs2_ready = %d, mask_ready = %d, vfu_bitalu_busy_calculating = %d, result_ready = %d\n", vs1_ready, vs2_ready , mask_ready, vfu_bitalu_busy_calculating, result_ready); \
        if(!VFU_EVENT_NAME.scheduled()) \
            schedule(VFU_EVENT_NAME,afterCycles(Cycles(1))); \
        else \
            panic("schedule(VFU_EVENT_NAME,curTick()+1*1000);"); \
        return; \
    } else { \
        bitAluAdmissionBlockedOnResultFull = false; \
        DPRINTF(LaneVSPM, "VFU_BitALU Unit, pops the %dth/%d data of instruction %d  from operandQueue successfully. ", RUNNING_PKT_NAME->locallane_bitalu_calc_cnt, RUNNING_PKT_NAME->locallane_bitalu_calctime_len, RUNNING_PKT_NAME->vns_instr_id); \
        scheduleBitAluOperandPopBoundary(RUNNING_PKT_NAME, false); \
        commitBitAluMaskOperand( \
            RUNNING_PKT_NAME, RUNNING_PKT_NAME->locallane_bitalu_calc_cnt); \
        locallane_read_vinsn_progress[RUNNING_PKT_NAME->running_id]++; \
        DPRINTFR(LaneVSPM, "%s%s",(RUNNING_PKT_NAME->use_vs1?"  data_vs1 = 0x":""), (RUNNING_PKT_NAME->use_vs1?int2Hex(running_bitaluA_data_pkt):"")); \
        DPRINTFR(LaneVSPM, "%s%s",(RUNNING_PKT_NAME->use_vs2?"  data_vs2 = 0x":""), (RUNNING_PKT_NAME->use_vs2?int2Hex(running_bitaluB_data_pkt):"")); \
        DPRINTFR(LaneVSPM, "%s%s\n",(RUNNING_PKT_NAME->vm_r   ?" data_mask = 0x":""), (RUNNING_PKT_NAME->vm_r   ?int2Hex(running_bitalu_mask_data_pkt):"")); \
        vfu_bitalu_busy_calculating = true; bitalu_vd1_result_writeback_done = false; bitalu_mask_result_writeback_done = false; bitalu_vd1_result_calc_done = false; \
        if(!VFU_EVENT_NAME.scheduled()) \
            schedule(VFU_EVENT_NAME,afterCycles(Cycles(1))); \
        else \
            panic("schedule(VFU_EVENT_NAME,curTick()+1*1000);"); \
        RUNNING_PKT_NAME->locallane_bitalu_calc_cnt = \
            RUNNING_PKT_NAME->locallane_bitalu_calc_cnt - \
            RUNNING_PKT_NAME->vew + 2; \
        if(!(RUNNING_PKT_NAME->op >= VREDAND && \
             RUNNING_PKT_NAME->op <= VREDSUM)) { \
            /* venus_bitalu is combinational.  The accepted A/B/mask Q \
             * payload writes result_queue_d on this edge; result_queue_q \
             * visibility is modelled by TaggedBitAluResult::visibleTick. */ \
            VFUBitAluCalculating(); \
        } else if(!nextVFUBitAluStartCalcEvent.scheduled()) { \
            schedule(nextVFUBitAluStartCalcEvent,afterCycles(Cycles(1))); \
        } \
        /* A queue-service edge may already be pending.  It observes the \
         * explicit result-Q visibility and coalesces with this cause. */ \
    } \
} \
if(VFU_TYPE_NAME == VFU_CAU && (RUNNING_PKT_NAME->locallane_cau_calc_cnt < RUNNING_PKT_NAME->locallane_cau_calctime_len)) \
{ \
    bool vs1_ready = true; \
    bool vs2_ready = true; \
    bool vd1_ready = true; \
    bool vd2_ready = true; \
    bool mask_ready = true; \
    bool mask_row_fetched = false; \
    bool result_ready = cauOperandAdmissionReady(RUNNING_PKT_NAME); \
    unsigned int last_cauA_data_pkt = running_cauA_data_pkt; \
    unsigned int last_cauB_data_pkt = running_cauB_data_pkt; \
    unsigned int last_cauC_data_pkt = running_cauC_data_pkt; \
    unsigned int last_cauD_data_pkt = running_cauD_data_pkt; \
    unsigned int last_mask_data_pkt = running_cau_mask_data_pkt; \
    if(RUNNING_PKT_NAME->use_vs1) { \
        vs1_ready = operandQueuePopdataFIFO(CAU_A, RUNNING_PKT_NAME); \
    } else { running_cauA_data_pkt = 0; } \
    if(RUNNING_PKT_NAME->use_vs2) { \
        vs2_ready = operandQueuePopdataFIFO(CAU_B, RUNNING_PKT_NAME); \
    } else { running_cauB_data_pkt = 0; } \
    if(RUNNING_PKT_NAME->use_vd1_op) { \
        vd1_ready = operandQueuePopdataFIFO(CAU_C, RUNNING_PKT_NAME); \
    } else { running_cauC_data_pkt = 0; } \
    if(RUNNING_PKT_NAME->use_vd2_op) { \
        vd2_ready = operandQueuePopdataFIFO(CAU_D, RUNNING_PKT_NAME); \
    } else { running_cauD_data_pkt = 0; } \
    if(RUNNING_PKT_NAME->vm_r || RUNNING_PKT_NAME->vm_w) { \
        mask_ready = prepareCauMaskOperand( \
            running_cau_mask_data_pkt, mask_row_fetched, \
            RUNNING_PKT_NAME, RUNNING_PKT_NAME->locallane_cau_calc_cnt); \
    } else { running_cau_mask_data_pkt = 0; } \
    if(vs1_ready == false || vs2_ready == false || vd1_ready == false || vd2_ready == false || mask_ready == false || vfu_cau_busy_calculating == true || result_ready == false) { \
        if(RUNNING_PKT_NAME->use_vs1    && vs1_ready)  operandQueueUNPopdataFIFO(CAU_A, running_cauA_data_pkt, RUNNING_PKT_NAME); \
        if(RUNNING_PKT_NAME->use_vs2    && vs2_ready)  operandQueueUNPopdataFIFO(CAU_B, running_cauB_data_pkt, RUNNING_PKT_NAME); \
        if(RUNNING_PKT_NAME->use_vd1_op && vd1_ready)  operandQueueUNPopdataFIFO(CAU_C, running_cauC_data_pkt, RUNNING_PKT_NAME); \
        if(RUNNING_PKT_NAME->use_vd2_op && vd2_ready)  operandQueueUNPopdataFIFO(CAU_D, running_cauD_data_pkt, RUNNING_PKT_NAME); \
        running_cauA_data_pkt = last_cauA_data_pkt; \
        running_cauB_data_pkt = last_cauB_data_pkt; \
        running_cauC_data_pkt = last_cauC_data_pkt; \
        running_cauD_data_pkt = last_cauD_data_pkt; \
        running_cau_mask_data_pkt = last_mask_data_pkt; \
        DPRINTF(LaneVSPM, "VFU_CAU Unit, fails to pops new data from operandQueue where vs1_ready = %d , vs2_ready = %d , vd1_ready = %d , vd2_ready = %d , mask_ready = %d , vfu_cau_busy_calculating = %d, result_ready = %d\n", vs1_ready, vs2_ready, vd1_ready, vd2_ready, mask_ready, vfu_cau_busy_calculating, result_ready); \
        advanceCAUResultHold(); \
        if(!VFU_EVENT_NAME.scheduled()) \
            schedule(VFU_EVENT_NAME,afterCycles(Cycles(1))); \
        else \
            panic("schedule(VFU_EVENT_NAME,curTick()+1*1000);"); \
        return; \
    } else { \
        DPRINTF(Lane, "VFU_CAU Unit, pops the %dth/%d data of instruction %d from operandQueue successfully.", RUNNING_PKT_NAME->locallane_cau_calc_cnt, RUNNING_PKT_NAME->locallane_cau_calctime_len, RUNNING_PKT_NAME->vns_instr_id); \
        commitCauMaskOperand( \
            RUNNING_PKT_NAME, RUNNING_PKT_NAME->locallane_cau_calc_cnt); \
        locallane_read_vinsn_progress[RUNNING_PKT_NAME->running_id]++; \
        DPRINTFR(Lane, "%s%s", (RUNNING_PKT_NAME->use_vs1   ?"  data_vs1 = 0x":""), (RUNNING_PKT_NAME->use_vs1   ?int2Hex(running_cauA_data_pkt):"")); \
        DPRINTFR(Lane, "%s%s", (RUNNING_PKT_NAME->use_vs2   ?"  data_vs2 = 0x":""), (RUNNING_PKT_NAME->use_vs2   ?int2Hex(running_cauB_data_pkt):"")); \
        DPRINTFR(Lane, "%s%s", (RUNNING_PKT_NAME->use_vd1_op?"  data_vd1 = 0x":""), (RUNNING_PKT_NAME->use_vd1_op?int2Hex(running_cauC_data_pkt):"")); \
        DPRINTFR(Lane, "%s%s", (RUNNING_PKT_NAME->use_vd2_op?"  data_vd2 = 0x":""), (RUNNING_PKT_NAME->use_vd2_op?int2Hex(running_cauD_data_pkt):"")); \
        DPRINTFR(Lane, "%s%s\n", (RUNNING_PKT_NAME->vm_r      ?" data_mask = 0x":""), (RUNNING_PKT_NAME->vm_r      ?int2Hex(running_cau_mask_data_pkt):"")); \
        vfu_cau_busy_calculating = true; cau_vd1_result_writeback_done = false; cau_vd2_result_writeback_done = false; cau_vd1_result_calc_done = false; \
        if(experimentalRequesterQVisibility) { \
            /* The legacy start-calc event runs one edge after operand \
             * admission, after locallane_cau_calc_cnt has advanced below. \
             * VRANGE derives its element index from that registered count. \
             * The explicit pipeline computes on the input-handshake edge, \
             * so commit the same counter D->Q transition before taking the \
             * arithmetic snapshot; otherwise the first index underflows. */ \
            RUNNING_PKT_NAME->locallane_cau_calc_cnt = \
                RUNNING_PKT_NAME->locallane_cau_calc_cnt - \
                RUNNING_PKT_NAME->vew + 2; \
            cau_vd1_result_calc_done = getCAUResult(); \
            cauPipelineResults.push_back({ \
                cau_vd1_result_buf, cau_vd2_result_buf, \
                running_cau_mask_data_pkt, \
                std::make_shared<VenusInstrPkt>(RUNNING_PKT_NAME), \
                afterCycles(Cycles(getCauPipeLength(RUNNING_PKT_NAME))) \
            }); \
            vfu_cau_busy_calculating = false; \
            VFUCAUCalculating(); \
        } else if(!nextVFUCAUStartCalcEvent.scheduled()) { \
            schedule(nextVFUCAUStartCalcEvent,afterCycles(Cycles(1))); \
        } \
        if(!VFU_EVENT_NAME.scheduled()) \
            schedule(VFU_EVENT_NAME,afterCycles(Cycles(1))); \
        else \
            panic("schedule(VFU_EVENT_NAME,curTick()+1*1000);"); \
        if(!experimentalRequesterQVisibility) \
            RUNNING_PKT_NAME->locallane_cau_calc_cnt = RUNNING_PKT_NAME->locallane_cau_calc_cnt - RUNNING_PKT_NAME->vew + 2; \
    } \
} \
if(VFU_TYPE_NAME == VFU_SerDiv && (RUNNING_PKT_NAME->locallane_serdiv_calc_cnt < RUNNING_PKT_NAME->locallane_serdiv_calctime_len)) \
{ \
    bool vs1_ready = true; \
    bool vs2_ready = true; \
    bool mask_ready = true; \
    bool mask_row_fetched = false; \
    unsigned int last_serdivA_data_pkt = running_serdivA_data_pkt; \
    unsigned int last_serdivB_data_pkt = running_serdivB_data_pkt; \
    unsigned int last_mask_data_pkt    = running_serdiv_mask_data_pkt; \
    if(RUNNING_PKT_NAME->use_vs1) { \
        vs1_ready = operandQueuePopdataFIFO(SerDiv_A, RUNNING_PKT_NAME); \
    } else { running_serdivA_data_pkt = 0; } \
    if(RUNNING_PKT_NAME->use_vs2) { \
        vs2_ready = operandQueuePopdataFIFO(SerDiv_B, RUNNING_PKT_NAME); \
    } else { running_serdivB_data_pkt = 0; } \
    if(RUNNING_PKT_NAME->vm_r || RUNNING_PKT_NAME->vm_w) { \
        mask_ready = prepareMaskOperand( \
            running_serdiv_mask_data_pkt, running_serdiv_mask_row_pkt, \
            running_serdiv_mask_row_valid, mask_row_fetched, \
            RUNNING_PKT_NAME, \
            RUNNING_PKT_NAME->locallane_serdiv_calc_cnt); \
    } else { running_serdiv_mask_data_pkt = 0; } \
    if(vs1_ready == false || vs2_ready == false || mask_ready == false || vfu_serdiv_busy_calculating == true) { \
        if(RUNNING_PKT_NAME->use_vs1    && vs1_ready)  operandQueueUNPopdataFIFO(SerDiv_A, running_serdivA_data_pkt, RUNNING_PKT_NAME); \
        if(RUNNING_PKT_NAME->use_vs2    && vs2_ready)  operandQueueUNPopdataFIFO(SerDiv_B, running_serdivB_data_pkt, RUNNING_PKT_NAME); \
        rollbackMaskOperand(running_serdiv_mask_row_pkt, \
                            running_serdiv_mask_row_valid, \
                            mask_row_fetched, RUNNING_PKT_NAME); \
        running_serdivA_data_pkt = last_serdivA_data_pkt; \
        running_serdivB_data_pkt = last_serdivB_data_pkt; \
        running_serdiv_mask_data_pkt = last_mask_data_pkt; \
        DPRINTF(LaneVSPM, "VFU_SerDiv Unit, fails to pops new data from operandQueue where vs1_ready = %d , vs2_ready = %d, mask_ready = %d, vfu_serdiv_busy_calculating = %d\n", vs1_ready, vs2_ready, mask_ready, vfu_serdiv_busy_calculating); \
        if(!VFU_EVENT_NAME.scheduled()) \
            schedule(VFU_EVENT_NAME,afterCycles(Cycles(1))); \
        else \
            panic("schedule(VFU_EVENT_NAME,curTick()+1*1000);"); \
        return; \
    } else { \
        DPRINTF(LaneVSPM, "VFU_SerDiv Unit, pops the %dth/%d data of instruction %d from operandQueue successfully.", RUNNING_PKT_NAME->locallane_serdiv_calc_cnt, RUNNING_PKT_NAME->locallane_serdiv_calctime_len, RUNNING_PKT_NAME->vns_instr_id); \
        commitMaskOperand(running_serdiv_mask_row_valid, RUNNING_PKT_NAME, \
                          RUNNING_PKT_NAME->locallane_serdiv_calc_cnt); \
        locallane_read_vinsn_progress[RUNNING_PKT_NAME->running_id]++; \
        DPRINTFR(LaneVSPM, "%s%s", (RUNNING_PKT_NAME->use_vs1?"  data_vs1 = 0x":""), (RUNNING_PKT_NAME->use_vs1?int2Hex(running_serdivA_data_pkt):"")); \
        DPRINTFR(LaneVSPM, "%s%s", (RUNNING_PKT_NAME->use_vs2?"  data_vs2 = 0x":""), (RUNNING_PKT_NAME->use_vs2?int2Hex(running_serdivB_data_pkt):"")); \
        DPRINTFR(LaneVSPM, "%s%s\n", (RUNNING_PKT_NAME->vm_r ?" data_mask = 0x":""), (RUNNING_PKT_NAME->vm_r   ?int2Hex(running_serdiv_mask_data_pkt):"")); \
        vfu_serdiv_busy_calculating = true; serdiv_vd1_result_writeback_done = false; serdiv_vd1_result_calc_done = false; \
        if(!nextVFUSerdivStartCalcEvent.scheduled()) \
            schedule(nextVFUSerdivStartCalcEvent,afterCycles(Cycles(getVFUProcessingTime(RUNNING_PKT_NAME)))); \
        else \
            panic("schedule(nextVFUSerdivStartCalcEvent,curTick()+1*1000);"); \
        if(!VFU_EVENT_NAME.scheduled()) \
            schedule(VFU_EVENT_NAME,afterCycles(Cycles(getVFUProcessingTime(RUNNING_PKT_NAME)))); \
        else \
            panic("schedule(VFU_EVENT_NAME,curTick()+getVFUProcessingTime(RUNNING_PKT_NAME)*1000);"); \
        RUNNING_PKT_NAME->locallane_serdiv_calc_cnt = RUNNING_PKT_NAME->locallane_serdiv_calc_cnt - RUNNING_PKT_NAME->vew + 2; \
    } \
} \
if(VFU_TYPE_NAME == VFU_ShuffleUnit && (RUNNING_PKT_NAME->locallane_shuffle_calc_cnt < RUNNING_PKT_NAME->locallane_shuffle_calctime_len)) \
{ \
    bool vs1_ready = true; \
    bool mask_ready = true; \
    unsigned int last_shuffle_data_pkt = running_shuffle_data_pkt; \
    unsigned int last_mask_data_pkt    = running_shuffle_mask_data_pkt; \
    if(RUNNING_PKT_NAME->use_vs1) { \
        vs1_ready = operandQueuePopdataFIFO(ShuffleUnit, RUNNING_PKT_NAME); \
    } else { running_shuffle_data_pkt = 0; } \
    if(RUNNING_PKT_NAME->vm_r) { \
        mask_ready = operandQueuePopMaskData(running_shuffle_mask_data_pkt, RUNNING_PKT_NAME); \
    } else { running_shuffle_mask_data_pkt = 0; } \
    if(vs1_ready == false || mask_ready == false || vfu_shuffle_busy_calculating == true) { \
        if(RUNNING_PKT_NAME->use_vs1    && vs1_ready)  operandQueueUNPopdataFIFO(ShuffleUnit, running_shuffle_data_pkt, RUNNING_PKT_NAME); \
        if(RUNNING_PKT_NAME->vm_r       && mask_ready) operandQueueUNPopdataFIFO(Mask       , running_shuffle_mask_data_pkt, RUNNING_PKT_NAME); \
        running_shuffle_data_pkt = last_shuffle_data_pkt; \
        running_shuffle_mask_data_pkt = last_mask_data_pkt; \
        if(!VFU_EVENT_NAME.scheduled()) \
            schedule(VFU_EVENT_NAME,afterCycles(Cycles(1))); \
        else \
            panic("schedule(VFU_EVENT_NAME,curTick()+1*1000);"); \
        return; \
    } else { \
        DPRINTF(LaneVSPM, "VFU_ShuffleUnit Unit, pops new data successfully: instr %d with runningID %d\n", RUNNING_PKT_NAME->vns_instr_id, RUNNING_PKT_NAME->running_id); \
        vfu_shuffle_busy_calculating = true; shuffle_vd1_result_writeback_done = false; shuffle_vd1_result_calc_done = false; \
        if(!nextVFUShuffleUnitStartCalcEvent.scheduled()) \
            schedule(nextVFUShuffleUnitStartCalcEvent,afterCycles(Cycles(getVFUProcessingTime(RUNNING_PKT_NAME)))); \
        else \
            panic("schedule(nextVFUShuffleUnitStartCalcEvent,curTick()+1*1000);"); \
        if(!VFU_EVENT_NAME.scheduled()) \
            schedule(VFU_EVENT_NAME,afterCycles(Cycles(getVFUProcessingTime(RUNNING_PKT_NAME)))); \
        else \
            panic("schedule(VFU_EVENT_NAME,curTick()+getVFUProcessingTime(RUNNING_PKT_NAME)*1000);"); \
        RUNNING_PKT_NAME->locallane_shuffle_calc_cnt = RUNNING_PKT_NAME->locallane_shuffle_calc_cnt - RUNNING_PKT_NAME->vew + 2; \
    } \
} \
if(VFU_TYPE_NAME == VFU_BitALU && (RUNNING_PKT_NAME->locallane_bitalu_calc_cnt >= RUNNING_PKT_NAME->locallane_bitalu_calctime_len)) \
{ \
    /*已经全部送去计算*/ \
    if(RUNNING_PKT_NAME->op >= VREDAND && RUNNING_PKT_NAME->op <= VREDSUM) { \
        if(reduce_delay_count < (((int)log2(NrLanes)+1)*5)) { \
            if(lane_id == 0) { \
                m_venus_shuffle_pipeline->shuffle_halt = true; \
            } \
            reduce_delay_count++; \
            if(lane_id == 0) { \
                DPRINTF(LaneVFUFull, "VFU_BitALU Unit is reducing, operation is %s, instr %d with runningID %d, reduce_delay_count = %d.\n", op_to_str(RUNNING_PKT_NAME->op), RUNNING_PKT_NAME->vns_instr_id, RUNNING_PKT_NAME->running_id, reduce_delay_count); \
            } \
            if(!VFU_EVENT_NAME.scheduled()) \
                schedule(VFU_EVENT_NAME,afterCycles(Cycles(1))); \
            return; \
        } else { \
            if(lane_id == 0) { \
                if(RUNNING_PKT_NAME->op == VREDSUM) { \
                    if(reduce_delay_count == (((int)log2(NrLanes)+1)*5)) \
                    { \
                        if(vfu_bitalu_busy_calculating == false) { \
                            /*写回前两个字节*/ \
                            unsigned int wb_data = getVFUReduceResult(RUNNING_PKT_NAME->op, RUNNING_PKT_NAME->vew, RUNNING_PKT_NAME->vl); \
                            bitalu_vd1_result_buf_pipeline.push_pipe((uint16_t)(wb_data&0xffff), 0x1, cloneBitAluResultTag()); \
                            bitalu_vmask_result_buf_pipeline.push_pipe(0, 0x1, cloneBitAluResultTag()); \
                            bitalu_vd1_result_writeback_done = false; vfu_bitalu_busy_calculating = true; \
                            if(!nextVFUBitAluStartCalcEvent.scheduled()) \
                                schedule(nextVFUBitAluStartCalcEvent,afterCycles(Cycles(1))); \
                            reduce_delay_count++; \
                            DPRINTF(LaneVFUFull, "写回前两个字节VFU_BitALU Unit reduce complete, operation is %s, instr %d with runningID %d, wb_data = %x, reduce_delay_count = %d.\n", op_to_str(RUNNING_PKT_NAME->op), RUNNING_PKT_NAME->vns_instr_id, RUNNING_PKT_NAME->running_id, wb_data, reduce_delay_count); \
                            if(!VFU_EVENT_NAME.scheduled()) \
                                schedule(VFU_EVENT_NAME,afterCycles(Cycles(1))); \
                            return; \
                        } else { \
                            if(!VFU_EVENT_NAME.scheduled()) \
                                schedule(VFU_EVENT_NAME,afterCycles(Cycles(1))); \
                            return; \
                        } \
                    } \
                    else \
                    { \
                        /*写回后两个字节*/ \
                        if(vfu_bitalu_busy_calculating == false) { \
                            unsigned int wb_data = getVFUReduceResult(RUNNING_PKT_NAME->op, RUNNING_PKT_NAME->vew, RUNNING_PKT_NAME->vl); \
                            bitalu_vd1_result_buf_pipeline.push_pipe((uint16_t)((wb_data&0xffff0000)>>16), 0x1, cloneBitAluResultTag()); \
                            bitalu_vmask_result_buf_pipeline.push_pipe(0, 0x1, cloneBitAluResultTag()); \
                            bitalu_vd1_result_writeback_done = false; vfu_bitalu_busy_calculating = true; \
                            if(!nextVFUBitAluStartCalcEvent.scheduled()) \
                                schedule(nextVFUBitAluStartCalcEvent,afterCycles(Cycles(1))); \
                            reduce_delay_count = 0; \
                            m_venus_shuffle_pipeline->shuffle_halt = false; \
                            DPRINTF(LaneVFUFull, "写回后两个字节VFU_BitALU Unit reduce complete, operation is %s, instr %d with runningID %d, wb_data = %x, reduce_delay_count = %d.\n", op_to_str(RUNNING_PKT_NAME->op), RUNNING_PKT_NAME->vns_instr_id, RUNNING_PKT_NAME->running_id, wb_data, reduce_delay_count); \
                        } else { \
                            if(!VFU_EVENT_NAME.scheduled()) \
                                schedule(VFU_EVENT_NAME,afterCycles(Cycles(1))); \
                            return; \
                        } \
                    } \
                } else { \
                    if(vfu_bitalu_busy_calculating == false) { \
                        unsigned int wb_data = getVFUReduceResult(RUNNING_PKT_NAME->op, RUNNING_PKT_NAME->vew, RUNNING_PKT_NAME->vl); \
                        bitalu_vd1_result_buf_pipeline.push_pipe((uint16_t)(wb_data&0xffff), 0x1, cloneBitAluResultTag()); \
                        bitalu_vmask_result_buf_pipeline.push_pipe(0, 0x1, cloneBitAluResultTag()); \
                        bitalu_vd1_result_writeback_done = false; vfu_bitalu_busy_calculating = true; \
                        if(!nextVFUBitAluStartCalcEvent.scheduled()) \
                            schedule(nextVFUBitAluStartCalcEvent,afterCycles(Cycles(1))); \
                        reduce_delay_count = 0; \
                        m_venus_shuffle_pipeline->shuffle_halt = false; \
                        DPRINTF(LaneVFUFull, "VFU_BitALU Unit reduce complete, operation is %s, instr %d with runningID %d, wb_data = %x, reduce_delay_count = %d.\n", op_to_str(RUNNING_PKT_NAME->op), RUNNING_PKT_NAME->vns_instr_id, RUNNING_PKT_NAME->running_id, wb_data, reduce_delay_count); \
                    } else { \
                        if(!VFU_EVENT_NAME.scheduled()) \
                            schedule(VFU_EVENT_NAME,afterCycles(Cycles(1))); \
                        return; \
                    } \
                } \
            } \
            BUSY_NAME = false; \
            DPRINTF(LaneVSPM, "VFU_BitALU Unit has pop all the data of: instr %d with runningID %d, VFU_BitALU Unit Sleeps.\n", RUNNING_PKT_NAME->vns_instr_id, RUNNING_PKT_NAME->running_id); \
            if(!VFU_EVENT_NAME.scheduled()) \
                schedule(VFU_EVENT_NAME,afterCycles(Cycles(1))); \
            return; \
        } \
    } else { \
        BUSY_NAME = false; \
        DPRINTF(LaneVSPM, "VFU_BitALU Unit has pop all the data of: instr %d with runningID %d, VFU_BitALU Unit Sleeps.\n", RUNNING_PKT_NAME->vns_instr_id, RUNNING_PKT_NAME->running_id); \
        return; \
    } \
} \
if(VFU_TYPE_NAME == VFU_CAU && (RUNNING_PKT_NAME->locallane_cau_calc_cnt >= RUNNING_PKT_NAME->locallane_cau_calctime_len)) \
{ \
    /*已经全部送去计算*/ \
    BUSY_NAME = false; \
    DPRINTF(LaneVSPM, "VFU_CAU Unit has pop all the data of: instr %d with runningID %d, VFU_CAU Unit Sleeps.\n", RUNNING_PKT_NAME->vns_instr_id, RUNNING_PKT_NAME->running_id); \
    /* The operand consumer going idle does not make the independently \
     * elastic CAU result pipeline idle.  Re-arbitrate on the next edge so \
     * the final tagged result can be advanced out of the hold even when a \
     * younger same-VFU command is waiting on RAW retirement.  Without this \
     * transition a short lane can strand its last result permanently and \
     * fill all four CAU admission slots. */ \
    if(!VFU_EVENT_NAME.scheduled()) \
        schedule(VFU_EVENT_NAME,afterCycles(Cycles(1))); \
    return; \
} \
if(VFU_TYPE_NAME == VFU_SerDiv && (RUNNING_PKT_NAME->locallane_serdiv_calc_cnt >= RUNNING_PKT_NAME->locallane_serdiv_calctime_len)) \
{ \
    /*已经全部送去计算*/ \
    BUSY_NAME = false; \
    DPRINTF(LaneVSPM, "VFU_SerDiv Unit has pop all the data of: instr %d with runningID %d, VFU_SerDiv Unit Sleeps.\n", RUNNING_PKT_NAME->vns_instr_id, RUNNING_PKT_NAME->running_id); \
    return; \
} \
if(VFU_TYPE_NAME == VFU_ShuffleUnit && (RUNNING_PKT_NAME->locallane_shuffle_calc_cnt >= RUNNING_PKT_NAME->locallane_shuffle_calctime_len)) \
{ \
    /*已经全部送去计算*/ \
    BUSY_NAME = false; \
    DPRINTF(LaneVSPM, "VFU_ShuffleUnit Unit has pop all the data of: instr %d with runningID %d, VFU_ShuffleUnit Unit Sleeps.\n", RUNNING_PKT_NAME->vns_instr_id, RUNNING_PKT_NAME->running_id); \
    return; \
} \


VenusInstrPkt *
VenusLane::cloneBitAluResultTag() const
{
    auto *tag = new VenusInstrPkt(running_bitalu_instr_pkt);
    tag->locallane_bitalu_calc_cnt =
        running_bitalu_instr_pkt->locallane_bitalu_calc_cnt;
    tag->locallane_bitalu_calctime_len =
        running_bitalu_instr_pkt->locallane_bitalu_calctime_len;
    return tag;
}

bool
VenusLane::serviceBitAluMaskResult(TaggedBitAluResult &result)
{
    if (!result.instr->vm_w)
        return true;
    panic_if(writtingback_bitalu_instr_pkt == nullptr ||
                 writtingback_bitalu_instr_pkt->vns_instr_id !=
                     result.instr->vns_instr_id ||
                 writtingback_bitalu_instr_pkt->running_id !=
                     result.instr->running_id,
             "BitALU mask result has no matching writeback owner");

    auto *owner = writtingback_bitalu_instr_pkt;
    if (owner->locallane_vmask_writeback_len == INT_MAX) {
        owner->locallane_vmask_writeback_len = getLocalLaneCalcLen(
            owner->vl, owner->vew, owner->op, true);
        owner->locallane_vmask_writeback_cnt = 0;
        operandrequester_writeback_bitalumask_busy =
            owner->locallane_vmask_writeback_len > 0;
    }

    if (result.maskExternalValid &&
        !sendMaskRowWriteRequest(
            result.maskAddr, result.maskRowBits,
            result.instr->running_id, result.instr->vns_instr_id)) {
        DPRINTF(LaneVFU,
                "BitALU mask-row request stalls instr %d/rid %d addr %u\n",
                result.instr->vns_instr_id, result.instr->running_id,
                result.maskAddr);
        return false;
    }

    const int step = 2 - owner->vew;
    owner->locallane_vmask_writeback_cnt = std::min(
        owner->locallane_vmask_writeback_len,
        owner->locallane_vmask_writeback_cnt + step);
    DPRINTF(LaneVFU,
            "BitALU mask result commit instr %d/rid %d addr %u kind %s "
            "progress %d/%d row 0x%02x\n",
            owner->vns_instr_id, owner->running_id, result.maskAddr,
            result.maskExternalValid ? "write" : "jump",
            owner->locallane_vmask_writeback_cnt,
            owner->locallane_vmask_writeback_len, result.maskRowBits);

    if (owner->locallane_vmask_writeback_cnt <
        owner->locallane_vmask_writeback_len)
        return true;

    operandrequester_writeback_bitalumask_busy = false;
    operandrequester_writeback_bitalumask_data_topush = false;
    const bool dataDone = !owner->use_vd1 ||
        owner->locallane_vd1_writeback_cnt >=
            owner->locallane_vd1_writeback_len;
    if (!dataDone)
        return true;

    if (bitalu_doneinstr_pkt != nullptr)
        delete bitalu_doneinstr_pkt;
    bitalu_doneinstr_pkt = new VenusInstrPkt(owner);
    bitalu_doneinstr_pkt->vns_instr_stat = INSTR_DONE;
    bitalu_doneinstr_pkt->vns_instr_log_endtick = curTick();
    port_venuslane_receivefrom_venussequencer.
        reportProducerGrantCompletion(owner);
    panic_if(nextVFUBitAluReportandRecycleInstrEvent.scheduled(),
             "BitALU scheduled duplicate mask completion");
    /* The real mask-row grant is consumed by bitalu_wrapper on this edge.
     * bitalu_vinsn_done_o is observed by the lane sequencer on the following
     * edge; no second arithmetic-pipeline boundary exists in RTL. */
    schedule(nextVFUBitAluReportandRecycleInstrEvent,
             afterCycles(Cycles(1)));
    return true;
}

bool
VenusLane::serviceBitAluResultQueue()
{
    if (bitaluResultQueue.empty())
        return false;

    auto &result = bitaluResultQueue.front();
    if (experimentalRequesterQVisibility &&
        result.visibleTick > curTick())
        return false;
    bitalu_vd1_result_buf_pipeout = result.vd1;
    bitalu_vmask_result_buf_pipeout = result.vmask;
    bitalu_vd1_mask_buf_pipeout = result.mask;
    bitalu_vmask_mask_buf_pipeout = result.mask;
    bitalu_vd1_instr_buf_pipeout = result.instr.get();
    bitalu_vmask_instr_buf_pipeout = result.instr.get();

    if (!result.vd1Done)
        result.vd1Done =
            operandRequesterSetVectorData(OperandPassage_BitALU);
    if (!result.maskDone)
        result.maskDone = serviceBitAluMaskResult(result);

    if (!result.vd1Done || !result.maskDone)
        return false;

    DPRINTF(LaneVFU,
            "BitALU result-queue dequeue instr %d/rid %d occupancy %d/%d\n",
            result.instr->vns_instr_id, result.instr->running_id,
            bitaluResultQueue.size() - 1, BitAluResultQueueDepth);
    if (experimentalRequesterQVisibility &&
        !(result.instr->op >= VREDAND && result.instr->op <= VREDSUM)) {
        noteBitAluResultGrant();
        DPRINTF(LaneVFU,
                "BitALU RTL result-count grant instr %d/rid %d count %d/%u\n",
                result.instr->vns_instr_id, result.instr->running_id,
                bitAluEffectiveResultCount(), BitAluResultQueueDepth);
    }
    bitaluResultQueue.pop_front();
    bitalu_vd1_instr_buf_pipeout = nullptr;
    bitalu_vmask_instr_buf_pipeout = nullptr;
    /* A grant updates result_queue_cnt_d.  The BitALU operand-ready path on
     * this edge still observes result_queue_cnt_q, so a full-queue stall is
     * retried by the already pending next-edge VFU event, never in this
     * gem5 tick. */
    return true;
}

void VenusLane::VFUBitAluCalculating()
{
    /*
     * RTL separates the arithmetic latency from a two-entry tagged result
     * queue.  Service the registered queue head before publishing a newly
     * ready pipeline beat: an enqueue on this edge is not a combinational
     * bypass to the VRF.
     */
    serviceBitAluResultQueue();

    if(bitalu_vd1_result_calc_done == false) {
        DPRINTF(Lane, "VFUBitAluCalc Unit, is calculating......dataA = 0x%x, dataB = 0x%x, mask = 0x%x\n", running_bitaluA_data_pkt, running_bitaluB_data_pkt, running_bitalu_mask_data_pkt);
        bitalu_vd1_result_calc_done = getBitAluResult();
        const bool isReduction =
            running_bitalu_instr_pkt->op >= VREDAND &&
            running_bitalu_instr_pkt->op <= VREDSUM;
        if (isReduction) {
            /*
             * Reduction input beats update the lane-local accumulator only.
             * RTL exposes no per-element vd result: lane 0 publishes the
             * single cross-lane result after reduce_delay_count expires.
             * Enqueuing these intermediate values makes a one-element
             * reduction destination retire repeatedly and can leave the
             * following scalar barrier permanently busy.
             */
            bitalu_vd1_result_writeback_done = true;
            bitalu_mask_result_writeback_done = true;
            vfu_bitalu_busy_calculating = false;
            return;
        }
        /* VenusInstrPkt's copy constructor deliberately resets the mutable
         * lane execution counters.  A tagged arithmetic result nevertheless
         * needs the accepted beat's position after it has crossed the input
         * handshake; otherwise every delayed result looks like beat zero and
         * mask-row jump/write boundaries can never be reconstructed. */
        auto *vd1ResultTag = new VenusInstrPkt(running_bitalu_instr_pkt);
        auto *maskResultTag = new VenusInstrPkt(running_bitalu_instr_pkt);
        vd1ResultTag->locallane_bitalu_calc_cnt =
            running_bitalu_instr_pkt->locallane_bitalu_calc_cnt;
        maskResultTag->locallane_bitalu_calc_cnt =
            running_bitalu_instr_pkt->locallane_bitalu_calc_cnt;
        vd1ResultTag->locallane_bitalu_calctime_len =
            running_bitalu_instr_pkt->locallane_bitalu_calctime_len;
        maskResultTag->locallane_bitalu_calctime_len =
            running_bitalu_instr_pkt->locallane_bitalu_calctime_len;
        bitalu_vd1_result_buf_pipeline.push_pipe(
            bitalu_vd1_result_buf, running_bitalu_mask_row_pkt,
            vd1ResultTag);
        bitalu_vmask_result_buf_pipeline.push_pipe(
            bitalu_vmask_result_buf, running_bitalu_mask_row_pkt,
            maskResultTag);
        if(running_bitalu_instr_pkt->use_vd1) DPRINTF(Lane, "VFUBitAluCalc Unit, calculate done. resultvd1 = 0x%x\n", bitalu_vd1_result_buf);
        if(running_bitalu_instr_pkt->vm_w) DPRINTF(Lane, "VFUBitAluCalc Unit, calculate done. resultvmask = 0x%x\n", bitalu_vmask_result_buf);
    } else {
        const bool vd1Interior =
            !bitalu_vd1_result_buf_pipeline.front_valid() &&
            bitalu_vd1_result_buf_pipeline.size() > 0;
        const bool maskInterior =
            !bitalu_vmask_result_buf_pipeline.front_valid() &&
            bitalu_vmask_result_buf_pipeline.size() > 0;
        panic_if(vd1Interior != maskInterior,
                 "BitALU paired result pipelines lost interior alignment");
        if (vd1Interior) {
            /* A scheduled arithmetic-pipeline edge advances even when the
             * operand consumer has no new beat.  This is the registered
             * bubble shift that exposes an older result to result_queue_q;
             * the lower-priority VFUCalc event must not perform a second
             * shift on the same edge. */
            bitalu_vd1_result_buf_pipeline.push_pipe(
                INT_MIN, INT_MIN, nullptr);
            bitalu_vmask_result_buf_pipeline.push_pipe(
                INT_MIN, INT_MIN, nullptr);
            bitalu_vd1_result_writeback_done = false;
            bitalu_mask_result_writeback_done = false;
            DPRINTF(LaneVFU,
                    "BitALU tagged result pipeline advances a bubble "
                    "without a new operand\n");
        }
    }

    const bool vd1Ready = bitalu_vd1_result_buf_pipeline.front_valid();
    const bool maskReady = bitalu_vmask_result_buf_pipeline.front_valid();
    panic_if(vd1Ready != maskReady,
             "BitALU paired result pipelines lost tag alignment");

    if (vd1Ready && bitaluResultQueue.size() < BitAluResultQueueDepth) {
        VenusInstrPkt *vd1Instr =
            bitalu_vd1_result_buf_pipeline.front_instr();
        VenusInstrPkt *maskInstr =
            bitalu_vmask_result_buf_pipeline.front_instr();
        panic_if(vd1Instr == nullptr || maskInstr == nullptr ||
                 vd1Instr->vns_instr_id != maskInstr->vns_instr_id ||
                 vd1Instr->running_id != maskInstr->running_id,
                 "BitALU paired result pipelines have different tags");
        const int resultStep = 2 - vd1Instr->vew;
        panic_if(resultStep <= 0,
                 "BitALU result has invalid VEW %d", vd1Instr->vew);
        const unsigned completed =
            std::max(0, vd1Instr->locallane_bitalu_calc_cnt);
        const unsigned ordinal = completed >= resultStep ?
            completed / resultStep - 1 : 0;
        const unsigned slot = ordinal % NrBankPerLane;
        const unsigned row = ordinal / NrBankPerLane;
        const unsigned rawMaskRow =
            bitalu_vd1_result_buf_pipeline.front_mask() & 0xff;
        const unsigned maskSlice =
            ((rawMaskRow >> (2 * slot)) & 0x1) |
            (((rawMaskRow >> (2 * slot + 1)) & 0x1) << 8);

        bool maskExternalValid = false;
        uint8_t accumulatedMaskRow = rawMaskRow;
        if (vd1Instr->vm_w) {
            if (slot == 0 ||
                bitaluMaskAccumulatorInstr !=
                    static_cast<int>(vd1Instr->vns_instr_id) ||
                bitaluMaskAccumulatorRunningId !=
                    static_cast<int>(vd1Instr->running_id) ||
                bitaluMaskAccumulatorRow != row) {
                bitaluMaskAccumulatorInstr = vd1Instr->vns_instr_id;
                bitaluMaskAccumulatorRunningId = vd1Instr->running_id;
                bitaluMaskAccumulatorRow = row;
                bitaluMaskAccumulatorBits = rawMaskRow;
            }
            const unsigned firstElement = ordinal * resultStep;
            const unsigned remaining =
                vd1Instr->locallane_bitalu_calctime_len > firstElement ?
                    vd1Instr->locallane_bitalu_calctime_len -
                        firstElement : 0;
            const unsigned validElements =
                std::min<unsigned>(resultStep, remaining);
            for (unsigned element = 0; element < validElements; ++element) {
                const bool value =
                    (bitalu_vmask_result_buf_pipeline.front() >>
                     (8 * element)) & 0x1;
                if (vd1Instr->vew == EW16) {
                    /*
                     * venus_bitalu_wrapper writes mask_q through the same
                     * byte enable used by the EW16 arithmetic result.  One
                     * logical EW16 element therefore owns both adjacent
                     * strb bits (be == 2'b11), and the comparison result is
                     * duplicated in wdata[0] and wdata[8].  The expanded
                     * gem5 mask SRAM stores those two bits as two bytes, so
                     * retain both of them in the accumulated physical row.
                     * Treating resultStep == 1 as one *byte* lost the high
                     * mask bit and made a later EW16 masked instruction see
                     * 0x0001 instead of RTL's 0x0101.
                     */
                    const unsigned lowBit = 2 * slot;
                    const unsigned pairMask = 0x3u << lowBit;
                    const unsigned pairValue =
                        (value ? 0x3u : 0u) << lowBit;
                    bitaluMaskAccumulatorBits =
                        (bitaluMaskAccumulatorBits & ~pairMask) | pairValue;
                } else {
                    const unsigned bit = 2 * slot + element;
                    bitaluMaskAccumulatorBits =
                        (bitaluMaskAccumulatorBits & ~(1u << bit)) |
                        (static_cast<unsigned>(value) << bit);
                }
            }
            accumulatedMaskRow = bitaluMaskAccumulatorBits;
            maskExternalValid = slot == NrBankPerLane - 1 ||
                completed >= static_cast<unsigned>(
                    vd1Instr->locallane_bitalu_calctime_len);
        }
        const bool reductionResult =
            vd1Instr->op >= VREDAND && vd1Instr->op <= VREDSUM;
        bitaluResultQueue.push_back({
            bitalu_vd1_result_buf_pipeline.front(),
            bitalu_vmask_result_buf_pipeline.front(),
            maskSlice,
            accumulatedMaskRow,
            ordinal,
            maskExternalValid,
            experimentalRequesterQVisibility && !reductionResult ?
                afterCycles(Cycles(1)) : curTick(),
            std::make_shared<VenusInstrPkt>(vd1Instr),
            false,
            false
        });
        /* A/B ibuf occupancy retires at the earlier VFU input handshake.
         * The mask operand still uses the separately modelled latch boundary
         * and is retired here until its own RTL queue oracle is available. */
        DPRINTF(LaneVFU,
                "BitALU result-queue enqueue instr %d/rid %d ordinal %u "
                "mask-row 0x%02x external %d occupancy %d/%d\n",
                vd1Instr->vns_instr_id, vd1Instr->running_id,
                ordinal, accumulatedMaskRow, maskExternalValid,
                bitaluResultQueue.size(), BitAluResultQueueDepth);
        bitalu_vd1_result_buf_pipeline.pop_pipe();
        bitalu_vmask_result_buf_pipeline.pop_pipe();
        bitalu_vd1_result_writeback_done = true;
        bitalu_mask_result_writeback_done = true;
        vfu_bitalu_busy_calculating = false;
        /* The registered result becomes visible to the combinational
         * requester on its explicit Q edge, never through a D bypass. */
        serviceBitAluResultQueue();
    } else if (!vd1Ready) {
        /*
         * Invalid latency bubbles are paired as well.  Retire the boundary
         * stage and let the next operand advance the arithmetic pipeline.
         */
        bitalu_vd1_result_buf_pipeline.pop_pipe();
        bitalu_vmask_result_buf_pipeline.pop_pipe();
        bitalu_vd1_result_writeback_done = true;
        bitalu_mask_result_writeback_done = true;
        vfu_bitalu_busy_calculating = false;
    } else {
        DPRINTF(LaneVFU,
                "BitALU result-queue full; hold tagged pipeline output "
                "occupancy %d/%d\n",
                bitaluResultQueue.size(), BitAluResultQueueDepth);
        vfu_bitalu_busy_calculating = true;
    }

    /* The arithmetic pipe remains live after its current front beat enters
     * the tagged result queue.  RTL advances those older beats even when no
     * new operand arrives on the next edge; sleeping here strands an
     * interior beat until a later operand happens to wake the VFU. */
    if(vfu_bitalu_busy_calculating || !bitaluResultQueue.empty() ||
       bitalu_vd1_result_buf_pipeline.size() > 0 ||
       bitalu_vmask_result_buf_pipeline.size() > 0) {
        if(!nextVFUBitAluStartCalcEvent.scheduled()) {
            schedule(nextVFUBitAluStartCalcEvent, afterCycles(Cycles(1)));
        }
    }
}

bool
VenusLane::serviceCauResultQueue()
{
    if (cauResultQueue.empty())
        return false;

    auto &result = cauResultQueue.front();
    if (experimentalRequesterQVisibility && result.visibleTick > curTick())
        return false;
    cau_vd1_result_buf_pipeout = result.vd1;
    cau_vd2_result_buf_pipeout = result.vd2;
    cau_vd1_mask_buf_pipeout = result.mask;
    cau_vd2_mask_buf_pipeout = result.mask;
    cau_vd1_instr_buf_pipeout = result.instr.get();
    cau_vd2_instr_buf_pipeout = result.instr.get();

    if (!result.vd1Done)
        result.vd1Done =
            operandRequesterSetVectorData(OperandPassage_CAUA);
    if (!result.vd2Done)
        result.vd2Done =
            operandRequesterSetVectorData(OperandPassage_CAUB);

    if (!result.vd1Done || !result.vd2Done)
        return false;

    DPRINTF(LaneVFU,
            "CAU result-queue dequeue instr %d/rid %d occupancy %d/%d\n",
            result.instr->vns_instr_id, result.instr->running_id,
            cauResultQueue.size() - 1, CauResultQueueDepth);
    cauResultQueue.pop_front();
    cau_vd1_instr_buf_pipeout = nullptr;
    cau_vd2_instr_buf_pipeout = nullptr;
    return true;
}

void VenusLane::VFUCAUCalculating()
{
    if (experimentalRequesterQVisibility) {
        /* One explicit RTL edge: result_queue_q is serviced first, then a
         * ready arithmetic result is captured into result_queue_d.  The new
         * entry carries a following-edge visibility timestamp, so event
         * ordering can never create a combinational D-to-VRF bypass. */
        serviceCauResultQueue();

        if (!cauPipelineResults.empty() &&
            cauPipelineResults.front().readyTick <= curTick() &&
            cauResultQueue.size() < CauResultQueueDepth) {
            auto result = std::move(cauPipelineResults.front());
            cauPipelineResults.pop_front();
            cauResultQueue.push_back({
                result.vd1,
                result.vd2,
                result.mask,
                std::move(result.instr),
                afterCycles(Cycles(1)),
                false,
                false
            });
            DPRINTF(LaneVFU,
                    "CAU explicit pipeline D enqueue instr %d/rid %d "
                    "ready %llu visible %llu occupancy %d/%d\n",
                    cauResultQueue.back().instr->vns_instr_id,
                    cauResultQueue.back().instr->running_id,
                    curTick(), cauResultQueue.back().visibleTick,
                    cauResultQueue.size(), CauResultQueueDepth);
        }

        if (!cauPipelineResults.empty() || !cauResultQueue.empty()) {
            if (!nextVFUCAUStartCalcEvent.scheduled())
                schedule(nextVFUCAUStartCalcEvent,
                         afterCycles(Cycles(1)));
        }
        return;
    }

    /*
     * venus_cau_wrapper has a registered two-entry result FIFO downstream
     * of the arithmetic pipeline.  Drain only a head captured on an earlier
     * edge; an entry born below is result_queue_d until the following edge.
     */
    serviceCauResultQueue();

    if(cau_vd1_result_calc_done == false) {
        DPRINTF(Lane, "VFUCAUCalc Unit, is calculating......dataA = 0x%x, dataB = 0x%x, dataC = 0x%x, dataD = 0x%x, mask = 0x%x\n", running_cauA_data_pkt, running_cauB_data_pkt, running_cauC_data_pkt, running_cauD_data_pkt, running_cau_mask_data_pkt);
        cau_vd1_result_calc_done = getCAUResult();
        cau_vd1_result_buf_pipeline.push_pipe(cau_vd1_result_buf, running_cau_mask_data_pkt, new VenusInstrPkt(running_cau_instr_pkt));
        cau_vd2_result_buf_pipeline.push_pipe(cau_vd2_result_buf, running_cau_mask_data_pkt, new VenusInstrPkt(running_cau_instr_pkt));
        if(running_cau_instr_pkt->use_vd1) DPRINTF(Lane, "VFUCAUCalc Unit, calculate done. resultvd1 = 0x%x\n", cau_vd1_result_buf);
        if(running_cau_instr_pkt->use_vd2) DPRINTF(Lane, "VFUCAUCalc Unit, calculate done. resultvd2 = 0x%x\n", cau_vd2_result_buf);
    }
    const bool vd1Ready = cau_vd1_result_buf_pipeline.front_valid();
    const bool vd2Ready = cau_vd2_result_buf_pipeline.front_valid();
    panic_if(vd1Ready != vd2Ready,
             "CAU paired result pipelines lost tag alignment");

    if (vd1Ready && cauResultQueue.size() < CauResultQueueDepth) {
        VenusInstrPkt *vd1Instr =
            cau_vd1_result_buf_pipeline.front_instr();
        VenusInstrPkt *vd2Instr =
            cau_vd2_result_buf_pipeline.front_instr();
        panic_if(vd1Instr == nullptr || vd2Instr == nullptr ||
                 vd1Instr->vns_instr_id != vd2Instr->vns_instr_id ||
                 vd1Instr->running_id != vd2Instr->running_id,
                 "CAU paired result pipelines have different tags");
        cauResultQueue.push_back({
            cau_vd1_result_buf_pipeline.front(),
            cau_vd2_result_buf_pipeline.front(),
            cau_vd1_result_buf_pipeline.front_mask(),
            std::make_shared<VenusInstrPkt>(vd1Instr),
            0,
            false,
            false
        });
        DPRINTF(LaneVFU,
                "CAU result-queue enqueue instr %d/rid %d occupancy "
                "%d/%d\n",
                vd1Instr->vns_instr_id, vd1Instr->running_id,
                cauResultQueue.size(), CauResultQueueDepth);
        cau_vd1_result_buf_pipeline.pop_pipe();
        cau_vd2_result_buf_pipeline.pop_pipe();
        cau_vd1_result_writeback_done = true;
        cau_vd2_result_writeback_done = true;
        vfu_cau_busy_calculating = false;
        serviceCauResultQueue();
    } else if (!vd1Ready) {
        cau_vd1_result_buf_pipeline.pop_pipe();
        cau_vd2_result_buf_pipeline.pop_pipe();
        cau_vd1_result_writeback_done = true;
        cau_vd2_result_writeback_done = true;
        vfu_cau_busy_calculating = false;
    } else {
        DPRINTF(LaneVFU,
                "CAU result-queue full; hold tagged pipeline output "
                "occupancy %d/%d\n",
                cauResultQueue.size(), CauResultQueueDepth);
        vfu_cau_busy_calculating = true;
    }

    if(vfu_cau_busy_calculating || !cauResultQueue.empty() ||
       cau_vd1_result_buf_pipeline.size() > 0 ||
       cau_vd2_result_buf_pipeline.size() > 0) {
        if(!nextVFUCAUStartCalcEvent.scheduled())
            schedule(nextVFUCAUStartCalcEvent, afterCycles(Cycles(1)));
    }
}

bool
VenusLane::serviceSerDivResultQueue()
{
    if (serdivResultQueue.empty())
        return false;

    auto &result = serdivResultQueue.front();
    serdiv_vd1_result_buf_pipeout = result.vd1;
    serdiv_vd1_mask_buf_pipeout = result.mask;
    serdiv_vd1_instr_buf_pipeout = result.instr.get();

    if (!result.vd1Done)
        result.vd1Done =
            operandRequesterSetVectorData(OperandPassage_SerDiv);
    if (!result.vd1Done)
        return false;

    DPRINTF(LaneVFU,
            "SerDiv result-queue dequeue instr %d/rid %d occupancy %d/%d\n",
            result.instr->vns_instr_id, result.instr->running_id,
            serdivResultQueue.size() - 1, SerDivResultQueueDepth);
    serdivResultQueue.pop_front();
    serdiv_vd1_instr_buf_pipeout = nullptr;
    return true;
}

void
VenusLane::serviceSerDivResultQueueEvent()
{
    serviceSerDivResultQueue();
    if (!serdivResultQueue.empty() &&
        !nextVFUSerdivResultQueueEvent.scheduled()) {
        schedule(nextVFUSerdivResultQueueEvent, afterCycles(Cycles(1)));
    }
}

void VenusLane::VFUSerdivCalculating()
{
    /*
     * venus_serdiv_wrapper decouples iterative completion from the VRF
     * grant with a registered two-entry tagged result FIFO.
     */
    if (vfu_serdiv_busy_calculating &&
        serdiv_vd1_result_calc_done == false) {
        DPRINTF(Lane, "VFUSerdivCalc Unit, is calculating......dataA = 0x%x, dataB = 0x%x, mask = 0x%x\n", running_serdivA_data_pkt, running_serdivB_data_pkt, running_serdiv_mask_data_pkt);
        serdiv_vd1_result_calc_done = getSerdivResult();
    }

    if (vfu_serdiv_busy_calculating &&
        serdivResultQueue.size() < SerDivResultQueueDepth) {
        if(running_serdiv_instr_pkt->use_vd1) DPRINTF(Lane, "VFUSerdivCalc Unit, calculate done. resultvd1 = 0x%x\n", serdiv_vd1_result_buf);
        serdivResultQueue.push_back({
            serdiv_vd1_result_buf,
            running_serdiv_mask_data_pkt,
            std::make_shared<VenusInstrPkt>(running_serdiv_instr_pkt),
            false
        });
        DPRINTF(LaneVFU,
                "SerDiv result-queue enqueue instr %d/rid %d occupancy "
                "%d/%d\n",
                running_serdiv_instr_pkt->vns_instr_id,
                running_serdiv_instr_pkt->running_id,
                serdivResultQueue.size(), SerDivResultQueueDepth);
        serdiv_vd1_result_writeback_done = true;
        vfu_serdiv_busy_calculating = false;
        /*
         * The iterative output is captured by result_queue_q on this edge.
         * RTL immediately re-evaluates its combinational request output, so
         * allow the new registered head to seek a grant in this timestamp.
         */
        serviceSerDivResultQueue();
        if (!serdivResultQueue.empty() &&
            !nextVFUSerdivResultQueueEvent.scheduled()) {
            schedule(nextVFUSerdivResultQueueEvent,
                     afterCycles(Cycles(1)));
        }
    } else if (vfu_serdiv_busy_calculating) {
        DPRINTF(LaneVFU,
                "SerDiv result-queue full; hold tagged iterative output "
                "occupancy %d/%d\n",
                serdivResultQueue.size(), SerDivResultQueueDepth);
    }

    if(vfu_serdiv_busy_calculating) {
        if(!nextVFUSerdivStartCalcEvent.scheduled()) {
            schedule(nextVFUSerdivStartCalcEvent, afterCycles(Cycles(1)));
        }
    }
}

void VenusLane::VFUShuffleUnitCalculating()
{
    if(shuffle_vd1_result_calc_done == false)
        shuffle_vd1_result_calc_done = getShuffleUnitResult();
    if(shuffle_vd1_result_writeback_done == false)
        shuffle_vd1_result_writeback_done = operandRequesterSetVectorData(OperandPassage_ShuffleUnit);
    if(shuffle_vd1_result_writeback_done == true)
        vfu_shuffle_busy_calculating = false;
    if(vfu_shuffle_busy_calculating == true)
        schedule(nextVFUShuffleUnitStartCalcEvent, afterCycles(Cycles(1)));
}

bool VenusLane::getBitAluResult()
{
                                  // instr_pkt,              vd1_w,                   vd2_w,   vd1_r,   vd2_r,   vmask_w,                 vs1,                      vs2,                      vmask_r);
    venus_function_unit.doVenusVFU(VFU_BitALU, running_bitalu_instr_pkt, &bitalu_vd1_result_buf, nullptr, nullptr, nullptr, &bitalu_vmask_result_buf, &running_bitaluA_data_pkt, &running_bitaluB_data_pkt, &running_bitalu_mask_data_pkt);
    return true;
}
bool VenusLane::getCAUResult()
{
                                // instr_pkt,              vd1_w,               vd2_w,              vd1_r,                  vd2_r,                 vmask_w,  vs1,                    vs2,                    vmask_r);
    venus_function_unit.doVenusVFU(VFU_CAU, running_cau_instr_pkt, &cau_vd1_result_buf, &cau_vd2_result_buf, &running_cauC_data_pkt, &running_cauD_data_pkt, nullptr, &running_cauA_data_pkt, &running_cauB_data_pkt, &running_cau_mask_data_pkt);
    return true;
}
bool VenusLane::getSerdivResult()
{
                                // instr_pkt,                vd1_w,                  vd2_w,     vd1_r,   vd2_r, vmask_w,  vs1,                      vs2,                        vmask_r);
    venus_function_unit.doVenusVFU(VFU_SerDiv, running_serdiv_instr_pkt, &serdiv_vd1_result_buf, nullptr, nullptr, nullptr, nullptr, &running_serdivA_data_pkt, &running_serdivB_data_pkt, &running_serdiv_mask_data_pkt);
    return true;
}
bool VenusLane::getShuffleUnitResult() //running_tshuffle_instr_pkt, running_shuffle_data_pkt, running_mask_data_pkt
{
    panic("getShuffleUnitResult() is not implemented yet!");
    return true;
}
    // running_bitaluA_data_pkt
    // running_bitaluB_data_pkt
    // running_cauA_data_pkt
    // running_cauB_data_pkt
    // running_cauC_data_pkt
    // running_cauD_data_pkt
    // running_serdivA_data_pkt
    // running_serdivB_data_pkt
    // running_mask_data_pkt
    // running_shuffle_data_pkt
// use_vs1                                , BitAlu_A
// use_vs2                                , BitAlu_B
// use_vs1                                , CAU_A
// use_vs2                                , CAU_B
// use_vd1_op                             , CAU_C
// use_vd2_op                             , CAU_D
// use_vs1                                , SerDiv_A
// use_vs2                                , SerDiv_B
// vm_r|this->recved_venus_instr_pkt->vm_w, Mask
// use_vs1                                , ShuffleUnit

void VenusLane::VFUCalc(VFU VFUType)
{
    switch (VFUType) {
        case VFU_BitALU     : __GENVFUCalc_Name__(VFU_BitALU     , running_bitalu_instr_pkt  , vfu_bitalu_busy, nextVFUBitAluCalcEvent );break;
        case VFU_CAU        : __GENVFUCalc_Name__(VFU_CAU        , running_cau_instr_pkt     , vfu_cau_busy   , nextVFUCAUCalcEvent    );break;
        case VFU_SerDiv     : __GENVFUCalc_Name__(VFU_SerDiv     , running_serdiv_instr_pkt  , vfu_serdiv_busy, nextVFUSerDivCalcEvent );break;
        // case VFU_ShuffleUnit: __GENVFUCalc_Name__(VFU_ShuffleUnit, running_tshuffle_instr_pkt, vfu_shuffle_busy, nextVFUShuffleCalcEvent);break;
        default: panic("unknown VFUType");
    }
}



#define __GENoperandRequesterSetVectorData_Name__(OPERANDPASSAGE_TYPE_NAME, BUSY_NAME, WRITEBACK_INSTR_PKT, RUNNING_INSTR_PKT, DATA_TOPUSH, DATA_BUFFER, MASK_BUFFER, OPERAND_USED, OPERAND_HEAD, WRITE_BACK_COUNTER, WRITE_BACK_LENGTH, DONEINSTR_PKT, DONEINSTR_EVENT) \
if(BUSY_NAME == false) \
/*如果是第一次写回，需要登记这个指令，并重置计数器*/ \
{ \
    /*BITALU和CAU有多个返回值，只需要登记第一次*/ \
    if(OPERANDPASSAGE_TYPE_NAME==OperandPassage_BitALUMask || OPERANDPASSAGE_TYPE_NAME==OperandPassage_BitALU) \
    { \
        if(writtingback_bitalu_instr_pkt == nullptr) { \
            writtingback_bitalu_instr_pkt = new VenusInstrPkt(RUNNING_INSTR_PKT); \
        } else if(writtingback_bitalu_instr_pkt->vns_instr_id != RUNNING_INSTR_PKT->vns_instr_id) { \
            delete writtingback_bitalu_instr_pkt; \
            writtingback_bitalu_instr_pkt = new VenusInstrPkt(RUNNING_INSTR_PKT); \
        } \
    } \
    else if(OPERANDPASSAGE_TYPE_NAME==OperandPassage_CAUA || OPERANDPASSAGE_TYPE_NAME==OperandPassage_CAUB) \
    { \
        if(writtingback_cau_instr_pkt == nullptr) { \
            writtingback_cau_instr_pkt = new VenusInstrPkt(RUNNING_INSTR_PKT); \
        } else if(writtingback_cau_instr_pkt->vns_instr_id != RUNNING_INSTR_PKT->vns_instr_id) { \
            delete writtingback_cau_instr_pkt; \
            writtingback_cau_instr_pkt = new VenusInstrPkt(RUNNING_INSTR_PKT); \
        } \
    } \
    else \
    { \
        if(WRITEBACK_INSTR_PKT != nullptr) delete WRITEBACK_INSTR_PKT; \
        WRITEBACK_INSTR_PKT = new VenusInstrPkt(RUNNING_INSTR_PKT); \
    } \
    for(int tagged_victim_id = 0; tagged_victim_id < NrIDs; \
        ++tagged_victim_id) { \
        if(!WRITEBACK_INSTR_PKT->chain_war_hazard[tagged_victim_id] && \
           !WRITEBACK_INSTR_PKT->chain_waw_hazard[tagged_victim_id]) \
            continue; \
        DPRINTF(Lane, \
                "Writeback registered tagged retirement writer instr %d/rid %d " \
                "victim instr %d/rid %d WAR=%d WAW=%d\n", \
                WRITEBACK_INSTR_PKT->vns_instr_id, \
                WRITEBACK_INSTR_PKT->running_id, \
                WRITEBACK_INSTR_PKT->chain_retire_victim_instr \
                    [tagged_victim_id], \
                tagged_victim_id, \
                WRITEBACK_INSTR_PKT->chain_war_hazard[tagged_victim_id], \
                WRITEBACK_INSTR_PKT->chain_waw_hazard[tagged_victim_id]); \
    } \
    /* Operand requesters snapshot hazards independently, while an LSU \
     * victim never traverses a lane requester of its own.  A writer whose \
     * command reaches some lanes before the newest hazard broadcast can \
     * therefore miss that older LSU read in its requester-local WAR bitmap. \
     * Revalidate retirement classes when the tagged writeback generation is \
     * registered.  updateHazardTable() is age sorted, so this can only merge \
     * dependencies on older live generations; it cannot bind the writer to \
     * a younger instruction. */ \
    if(venus_hazard_table->running_id_to_vns_instr_id \
           [WRITEBACK_INSTR_PKT->running_id] == \
       WRITEBACK_INSTR_PKT->vns_instr_id) { \
        for(int victim_id = 0; victim_id < NrIDs; victim_id++) { \
            const int victim_instr = \
                venus_hazard_table->running_id_to_vns_instr_id[victim_id]; \
            if(victim_instr < 0 || \
               victim_instr >= WRITEBACK_INSTR_PKT->vns_instr_id) \
                continue; \
            const bool live_war = venus_hazard_table->war_hazard_table \
                [WRITEBACK_INSTR_PKT->running_id][victim_id]; \
            const bool live_waw = venus_hazard_table->waw_hazard_table \
                [WRITEBACK_INSTR_PKT->running_id][victim_id]; \
            if(live_war || live_waw) { \
                WRITEBACK_INSTR_PKT->chain_war_hazard[victim_id] |= live_war; \
                WRITEBACK_INSTR_PKT->chain_waw_hazard[victim_id] |= live_waw; \
                WRITEBACK_INSTR_PKT->chain_retire_victim_instr[victim_id] = \
                    victim_instr; \
            } \
        } \
    } \
    /*WRITEBACK_INSTR_PKT = RUNNING_INSTR_PKT;*/ \
    if(WRITEBACK_INSTR_PKT->OPERAND_USED == 1) { \
        /*如果当前操作数被用到且是向量*/ \
        WRITEBACK_INSTR_PKT->WRITE_BACK_LENGTH = getLocalLaneCalcLen(WRITEBACK_INSTR_PKT->vl, WRITEBACK_INSTR_PKT->vew, WRITEBACK_INSTR_PKT->op, true); \
    } \
    else { \
        WRITEBACK_INSTR_PKT->WRITE_BACK_LENGTH = 0; \
    } \
    if(WRITEBACK_INSTR_PKT->WRITE_BACK_LENGTH > 0) { \
        WRITEBACK_INSTR_PKT->WRITE_BACK_COUNTER = 0; \
        DATA_TOPUSH = false; \
        BUSY_NAME = true; \
        DPRINTF(LaneOperandRequester, "operandRequesterWRITEBACK(%s): registered a new instr for write back: instr %d with runningID %d, target length is:%d\n", #OPERANDPASSAGE_TYPE_NAME, WRITEBACK_INSTR_PKT->vns_instr_id, WRITEBACK_INSTR_PKT->running_id, WRITEBACK_INSTR_PKT->WRITE_BACK_LENGTH); \
    } else { \
        WRITEBACK_INSTR_PKT->WRITE_BACK_COUNTER = 0; \
        DATA_TOPUSH = false; \
        BUSY_NAME = false; \
        return true; \
    } \
} \
if(WRITEBACK_INSTR_PKT->WRITE_BACK_COUNTER >= WRITEBACK_INSTR_PKT->WRITE_BACK_LENGTH) \
{ \
    /*已经全部写回*/ \
    DATA_TOPUSH = false; \
    BUSY_NAME = false; \
    DPRINTF(Lane, "operandRequesterWRITEBACK(%s) has written back all the %d data.\n", #OPERANDPASSAGE_TYPE_NAME, WRITEBACK_INSTR_PKT->WRITE_BACK_LENGTH); \
    if(((!WRITEBACK_INSTR_PKT->use_vd1)|(WRITEBACK_INSTR_PKT->locallane_vd1_writeback_cnt >= WRITEBACK_INSTR_PKT->locallane_vd1_writeback_len)) &&  \
       ((!WRITEBACK_INSTR_PKT->use_vd2)|(WRITEBACK_INSTR_PKT->locallane_vd2_writeback_cnt >= WRITEBACK_INSTR_PKT->locallane_vd2_writeback_len)) &&  \
       ((!WRITEBACK_INSTR_PKT->vm_w)|(WRITEBACK_INSTR_PKT->locallane_vmask_writeback_cnt >= WRITEBACK_INSTR_PKT->locallane_vmask_writeback_len))) { \
        DPRINTF(LaneOperandRequester, "all passage done, start recycle instr.\n"); \
        if(DONEINSTR_PKT == nullptr) delete DONEINSTR_PKT; \
        DONEINSTR_PKT = new VenusInstrPkt(WRITEBACK_INSTR_PKT); \
        /* A deferred bank grant can re-enter this requester through the \
         * completion-at-entry branch, so publish the final grant from both \
         * completion branches.  The sequencer sideband de-duplicates by \
         * lane and tagged instruction generation. */ \
        port_venuslane_receivefrom_venussequencer. \
            reportProducerGrantCompletion(WRITEBACK_INSTR_PKT); \
        /* A reduction owns its result-queue slot from the first partial \
         * accumulation.  In RTL, the final high-word bank grant drives \
         * bitalu_vinsn_done_o combinationally through RED_COMMIT; it does \
         * not traverse the ordinary newly-enqueued arithmetic-result Q or \
         * a separate lane retirement register.  Retire from that grant \
         * boundary while preserving the ordinary path for every \
         * non-reduction BitALU operation. */ \
        if(experimentalRequesterQVisibility && \
           OPERANDPASSAGE_TYPE_NAME==OperandPassage_BitALU && \
           WRITEBACK_INSTR_PKT->op >= VREDAND && \
           WRITEBACK_INSTR_PKT->op <= VREDSUM) \
            return reportAndRecycleDoneInstr(VFU_BitALU); \
        if(!DONEINSTR_EVENT.scheduled()) \
            if(experimentalRequesterQVisibility && \
               (OPERANDPASSAGE_TYPE_NAME==OperandPassage_CAUA || \
                OPERANDPASSAGE_TYPE_NAME==OperandPassage_CAUB)) \
                schedule(DONEINSTR_EVENT,afterCycles(Cycles(1))); \
            else if(experimentalRequesterQVisibility && \
                    (OPERANDPASSAGE_TYPE_NAME==OperandPassage_BitALU || \
                     OPERANDPASSAGE_TYPE_NAME==OperandPassage_BitALUMask)) \
                schedule(DONEINSTR_EVENT,afterCycles(Cycles(1))); \
            else if(OPERANDPASSAGE_TYPE_NAME==OperandPassage_SerDiv) \
                schedule(DONEINSTR_EVENT,afterCycles(Cycles(2))); \
            else \
                schedule(DONEINSTR_EVENT,afterCycles(Cycles(1))); \
        else \
            panic("schedule(DONEINSTR_EVENT,curTick()+1*1000);"); \
        /*reportAndRecycleDoneInstr(WRITEBACK_INSTR_PKT);*/ \
    } \
} else { \
    DATA_TOPUSH = true; \
} \
if(DATA_TOPUSH == true) { \
    /*如果有待存的数据，就去请求存储数据*/ \
    if(WRITEBACK_INSTR_PKT->OPERAND_USED == 1) { \
        /* Destination retirement has its own hazard-class clocks: WAR waits \
         * for the victim's read progress, while WAW waits for its write \
         * progress.  Never release either class from a folded max(). */ \
        bool war_ready = true; \
        int blocked_war_id = -1; \
        int blocked_war_progress = 0; \
        const int writeback_row = WRITEBACK_INSTR_PKT->WRITE_BACK_COUNTER / \
            (2 - WRITEBACK_INSTR_PKT->vew); \
        for(int war_id = 0; war_id < NrIDs; war_id++) { \
            const bool war_hazard = \
                WRITEBACK_INSTR_PKT->chain_war_hazard[war_id]; \
            const bool waw_hazard = \
                WRITEBACK_INSTR_PKT->chain_waw_hazard[war_id]; \
            if(!war_hazard && !waw_hazard) \
                continue; \
            const int observed_victim_instr = \
                venus_hazard_table->running_id_to_vns_instr_id[war_id]; \
            if(observed_victim_instr != \
               WRITEBACK_INSTR_PKT->chain_retire_victim_instr[war_id]) { \
                /* A delayed or skipped active-generation snapshot is not \
                 * proof of retirement.  Only the sequencer's tagged \
                 * completion tombstone can release this dependency. */ \
                if(venus_hazard_table->retired_vns_instr_id[war_id] < \
                   WRITEBACK_INSTR_PKT-> \
                       chain_retire_victim_instr[war_id]) { \
                    war_ready = false; \
                    blocked_war_id = war_id; \
                    blocked_war_progress = -1; \
                    continue; \
                } \
                WRITEBACK_INSTR_PKT->chain_war_hazard[war_id] = false; \
                WRITEBACK_INSTR_PKT->chain_waw_hazard[war_id] = false; \
                continue; \
            } \
            if(WRITEBACK_INSTR_PKT->chain_retire_victim_is_lsu[war_id]) { \
                war_ready = false; \
                blocked_war_id = war_id; \
                blocked_war_progress = -2; \
                continue; \
            } \
            const int victim_read_progress = \
                locallane_read_vinsn_progress[war_id]; \
            const int victim_write_progress = \
                locallane_write_vinsn_progress[war_id]; \
            if((war_hazard && victim_read_progress <= writeback_row) || \
               (waw_hazard && victim_write_progress <= writeback_row)) { \
                war_ready = false; \
                blocked_war_id = war_id; \
                blocked_war_progress = war_hazard ? \
                    victim_read_progress : victim_write_progress; \
            } \
        } \
        if(!war_ready) { \
             DPRINTF(Lane, "Hazard-class retirement: Writeback(%s) instr %d/rid %d stalls for WAR/WAW victim instr %d/rid %d progress (current wb idx: %d, victim progress: %d)\n", #OPERANDPASSAGE_TYPE_NAME, WRITEBACK_INSTR_PKT->vns_instr_id, WRITEBACK_INSTR_PKT->running_id, WRITEBACK_INSTR_PKT->chain_retire_victim_instr[blocked_war_id], blocked_war_id, writeback_row, blocked_war_progress); \
             return false; \
        } \
        /*如果当前操作数被用到且是向量*/ \
        int write_offset_bytes = WRITEBACK_INSTR_PKT->WRITE_BACK_COUNTER*(WRITEBACK_INSTR_PKT->vew+1); \
        unsigned int write_size = WRITEBACK_INSTR_PKT->vew + 1; \
        if(WRITEBACK_INSTR_PKT->vew == EW8 && \
           WRITEBACK_INSTR_PKT->WRITE_BACK_COUNTER + 1 < WRITEBACK_INSTR_PKT->WRITE_BACK_LENGTH) \
            write_size = 2; \
        if( requestArbiter(WRITEBACK_INSTR_PKT->OPERAND_HEAD, write_offset_bytes, OPERANDPASSAGE_TYPE_NAME , DATA_BUFFER, write_size, WRITEBACK_INSTR_PKT->running_id, WRITEBACK_INSTR_PKT->vns_instr_id) == false) \
            /*提供起始行号+当前计数，请求读取数据*/ \
            { \
                /*存储失败*/ \
                DATA_TOPUSH = false; \
                /*下一轮再尝试存储*/ \
                DPRINTF(Lane, "operandRequesterWRITEBACK(%s) is trying to write back the %dth/%d data of instruction %d , However, it gets an nack.\n", #OPERANDPASSAGE_TYPE_NAME, WRITEBACK_INSTR_PKT->WRITE_BACK_COUNTER, WRITEBACK_INSTR_PKT->WRITE_BACK_LENGTH, WRITEBACK_INSTR_PKT->vns_instr_id); \
                return false; \
            } else { \
                /*存储成功*/ \
                DATA_TOPUSH = true; \
                WRITEBACK_INSTR_PKT->recordResultWrite(lane_id, OPERANDPASSAGE_TYPE_NAME == OperandPassage_CAUB, write_offset_bytes, DATA_BUFFER, write_size); \
                const VFU writer_vfu = \
                    (OPERANDPASSAGE_TYPE_NAME == OperandPassage_CAUA || \
                     OPERANDPASSAGE_TYPE_NAME == OperandPassage_CAUB) ? \
                        VFU_CAU : \
                    (OPERANDPASSAGE_TYPE_NAME == OperandPassage_SerDiv) ? \
                        VFU_SerDiv : VFU_BitALU; \
                const Addr writer_vaddr = \
                    WRITEBACK_INSTR_PKT->OPERAND_HEAD * NrBankPerLane + \
                    write_offset_bytes / sizeof(uint16_t); \
                noteVectorWriterGrant( \
                    WRITEBACK_INSTR_PKT->running_id, writer_vfu, \
                    writer_vaddr); \
                WRITEBACK_INSTR_PKT->WRITE_BACK_COUNTER = WRITEBACK_INSTR_PKT->WRITE_BACK_COUNTER - WRITEBACK_INSTR_PKT->vew + 2; \
                locallane_write_vinsn_progress[WRITEBACK_INSTR_PKT->running_id]++; \
                DPRINTF(Lane, "operandRequesterWRITEBACK(%s) has written back the %dth/%d data of instruction %d, write back data = 0x%x.\n", #OPERANDPASSAGE_TYPE_NAME, WRITEBACK_INSTR_PKT->WRITE_BACK_COUNTER, WRITEBACK_INSTR_PKT->WRITE_BACK_LENGTH, WRITEBACK_INSTR_PKT->vns_instr_id, DATA_BUFFER); \
                /*ew8:+1 ew16:+2*/ \
            } \
        /*}*/ \
    } \
} \
if(WRITEBACK_INSTR_PKT->WRITE_BACK_COUNTER >= WRITEBACK_INSTR_PKT->WRITE_BACK_LENGTH) \
{ \
    /*已经全部写回*/ \
    DATA_TOPUSH = false; \
    BUSY_NAME = false; \
    DPRINTF(Lane, "operandRequesterWRITEBACK(%s) has written back all the %d data of instruction %d.\n", #OPERANDPASSAGE_TYPE_NAME, WRITEBACK_INSTR_PKT->WRITE_BACK_LENGTH, WRITEBACK_INSTR_PKT->vns_instr_id); \
    if(((!WRITEBACK_INSTR_PKT->use_vd1)|(WRITEBACK_INSTR_PKT->locallane_vd1_writeback_cnt >= WRITEBACK_INSTR_PKT->locallane_vd1_writeback_len)) &&  \
       ((!WRITEBACK_INSTR_PKT->use_vd2)|(WRITEBACK_INSTR_PKT->locallane_vd2_writeback_cnt >= WRITEBACK_INSTR_PKT->locallane_vd2_writeback_len)) &&  \
       ((!WRITEBACK_INSTR_PKT->vm_w)|(WRITEBACK_INSTR_PKT->locallane_vmask_writeback_cnt >= WRITEBACK_INSTR_PKT->locallane_vmask_writeback_len))) { \
        DPRINTF(LaneOperandRequester, "all passage done, start recycle instr.\n"); \
        if(DONEINSTR_PKT == nullptr) delete DONEINSTR_PKT; \
        DONEINSTR_PKT = new VenusInstrPkt(WRITEBACK_INSTR_PKT); \
        DONEINSTR_PKT->vns_instr_stat = INSTR_DONE; \
        DONEINSTR_PKT->vns_instr_log_endtick = curTick(); \
        port_venuslane_receivefrom_venussequencer. \
            reportProducerGrantCompletion(WRITEBACK_INSTR_PKT); \
        /* The normal final-grant path reaches this post-request completion \
         * branch in the same edge as RED_COMMIT.  Reduction done therefore \
         * leaves from this branch as well as the completion-at-entry branch \
         * above; waiting for DONEINSTR_EVENT would add a register which does \
         * not exist between the RTL bank grant and bitalu_vinsn_done_o. */ \
        if(experimentalRequesterQVisibility && \
           OPERANDPASSAGE_TYPE_NAME==OperandPassage_BitALU && \
           WRITEBACK_INSTR_PKT->op >= VREDAND && \
           WRITEBACK_INSTR_PKT->op <= VREDSUM) \
            return reportAndRecycleDoneInstr(VFU_BitALU); \
        if(!DONEINSTR_EVENT.scheduled()) \
            if(experimentalRequesterQVisibility && \
               (OPERANDPASSAGE_TYPE_NAME==OperandPassage_CAUA || \
                OPERANDPASSAGE_TYPE_NAME==OperandPassage_CAUB)) \
                schedule(DONEINSTR_EVENT,afterCycles(Cycles(1))); \
            else if(experimentalRequesterQVisibility && \
                    (OPERANDPASSAGE_TYPE_NAME==OperandPassage_BitALU || \
                     OPERANDPASSAGE_TYPE_NAME==OperandPassage_BitALUMask)) \
                schedule(DONEINSTR_EVENT,afterCycles(Cycles(1))); \
            else if(OPERANDPASSAGE_TYPE_NAME==OperandPassage_SerDiv) \
                schedule(DONEINSTR_EVENT,afterCycles(Cycles(2))); \
            else \
                schedule(DONEINSTR_EVENT,afterCycles(Cycles(1))); \
        else \
            panic("schedule(DONEINSTR_EVENT,curTick()+1*1000);"); \
        /*reportAndRecycleDoneInstr(WRITEBACK_INSTR_PKT);*/ \
    } \
} \
return true;


bool VenusLane::operandRequesterSetVectorData(OPERANDPASSAGE OperandPassageType)
{
    switch (OperandPassageType) {
        case OperandPassage_BitALU     : __GENoperandRequesterSetVectorData_Name__(OperandPassage_BitALU     , operandrequester_writeback_bitalu_busy     , writtingback_bitalu_instr_pkt  , bitalu_vd1_instr_buf_pipeout  , operandrequester_writeback_bitalu_data_topush     , bitalu_vd1_result_buf_pipeout  , bitalu_vd1_mask_buf_pipeout  , use_vd1, vd1_head, locallane_vd1_writeback_cnt  , locallane_vd1_writeback_len  , bitalu_doneinstr_pkt  , nextVFUBitAluReportandRecycleInstrEvent );break;
        case OperandPassage_BitALUMask : __GENoperandRequesterSetVectorData_Name__(OperandPassage_BitALUMask , operandrequester_writeback_bitalumask_busy , writtingback_bitalu_instr_pkt  , bitalu_vmask_instr_buf_pipeout, operandrequester_writeback_bitalumask_data_topush , bitalu_vmask_result_buf_pipeout, bitalu_vmask_mask_buf_pipeout, vm_w   , vd1_head, locallane_vmask_writeback_cnt, locallane_vmask_writeback_len, bitalu_doneinstr_pkt  , nextVFUBitAluReportandRecycleInstrEvent );break; // parameter:vd1_head needs to be changed!
        case OperandPassage_CAUA       : __GENoperandRequesterSetVectorData_Name__(OperandPassage_CAUA       , operandrequester_writeback_caua_busy       , writtingback_cau_instr_pkt     , cau_vd1_instr_buf_pipeout     , operandrequester_writeback_cauA_data_topush       , cau_vd1_result_buf_pipeout     , cau_vd1_mask_buf_pipeout     , use_vd1, vd1_head, locallane_vd1_writeback_cnt  , locallane_vd1_writeback_len  , cau_doneinstr_pkt     , nextVFUCAUReportandRecycleInstrEvent    );break;
        case OperandPassage_CAUB       : __GENoperandRequesterSetVectorData_Name__(OperandPassage_CAUB       , operandrequester_writeback_caub_busy       , writtingback_cau_instr_pkt     , cau_vd2_instr_buf_pipeout     , operandrequester_writeback_cauB_data_topush       , cau_vd2_result_buf_pipeout     , cau_vd2_mask_buf_pipeout     , use_vd2, vd2_head, locallane_vd2_writeback_cnt  , locallane_vd2_writeback_len  , cau_doneinstr_pkt     , nextVFUCAUReportandRecycleInstrEvent    );break;
        case OperandPassage_SerDiv     : __GENoperandRequesterSetVectorData_Name__(OperandPassage_SerDiv     , operandrequester_writeback_serdiv_busy     , writtingback_serdiv_instr_pkt  , serdiv_vd1_instr_buf_pipeout  , operandrequester_writeback_serdiv_data_topush     , serdiv_vd1_result_buf_pipeout  , serdiv_vd1_mask_buf_pipeout  , use_vd1, vd1_head, locallane_vd1_writeback_cnt  , locallane_vd1_writeback_len  , serdiv_doneinstr_pkt  , nextVFUSerdivReportandRecycleInstrEvent );break;
        // case OperandPassage_ShuffleUnit: __GENoperandRequesterSetVectorData_Name__(OperandPassage_ShuffleUnit, operandrequester_writeback_shuffle_busy    , writtingback_tshuffle_instr_pkt, running_tshuffle_instr_pkt    , operandrequester_writeback_shuffle_data_topush    , shuffle_vd1_result_buf         , running_mask_data_pkt        , use_vd1, vd1_head, locallane_vd1_writeback_cnt  , locallane_vd1_writeback_len  , tshuffle_doneinstr_pkt, nextVFUShuffleReportandRecycleInstrEvent);break;
        default: panic("unknown OperandType");
    }
}

}
