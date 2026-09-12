#include "venus/VenusShufflePipline.hh"
#include <algorithm>
#include <cstdlib>
#include "mem/packet.hh"
#include "mem/packet_access.hh" 
#include "mem/request.hh"
#include "base/trace.hh"

namespace gem5
{

namespace
{

int
selectShuffleRrTree(const std::vector<bool> &requests, unsigned rr)
{
    fatal_if(requests.empty(), "shuffle RR tree has no inputs");
    fatal_if(rr >= requests.size(),
             "shuffle RR pointer %u is outside %u inputs", rr,
             requests.size());

    unsigned levels = 0;
    unsigned paddedInputs = 1;
    while (paddedInputs < requests.size()) {
        paddedInputs <<= 1;
        ++levels;
    }

    unsigned first = 0;
    unsigned width = paddedInputs;
    for (int bit = static_cast<int>(levels) - 1; bit >= 0; --bit) {
        const unsigned half = width / 2;
        bool left = false;
        bool right = false;
        for (unsigned input = first;
             input < first + half && input < requests.size(); ++input) {
            left |= requests[input];
        }
        for (unsigned input = first + half;
             input < first + width && input < requests.size(); ++input) {
            right |= requests[input];
        }
        fatal_if(!left && !right,
                 "shuffle RR tree descended into an empty subtree");
        const bool selectRight =
            !left || (right && ((rr >> bit) & 1U));
        if (selectRight)
            first += half;
        width = half;
    }

    fatal_if(first >= requests.size() || !requests[first],
             "shuffle RR tree selected inactive input %u", first);
    return static_cast<int>(first);
}

unsigned
nextShuffleFairRr(const std::vector<bool> &requests, unsigned rr)
{
    fatal_if(requests.empty(), "shuffle FairArb has no inputs");
    fatal_if(rr >= requests.size(),
             "shuffle RR pointer %u is outside %u inputs", rr,
             requests.size());

    for (unsigned input = rr + 1; input < requests.size(); ++input) {
        if (requests[input])
            return input;
    }
    for (unsigned input = 0; input <= rr; ++input) {
        if (requests[input])
            return input;
    }
    fatal("shuffle FairArb cannot advance an empty request vector");
}

uint64_t
shuffleRequestMask(const std::vector<bool> &requests)
{
    fatal_if(requests.size() > 64,
             "shuffle request mask only supports up to 64 inputs");
    uint64_t mask = 0;
    for (unsigned input = 0; input < requests.size(); ++input) {
        if (requests[input])
            mask |= uint64_t(1) << input;
    }
    return mask;
}

} // anonymous namespace

    VenusShufflePipline::VenusShufflePipline(const VenusShufflePiplineParams &p)
        : ClockedObject(p),
        //   nextElementToAssign(0),
          commandActive(false),
          engineState(EngineState::Idle),
          numPes(p.num_pes),
          banksPerLane(p.num_banks_per_lane),
          linesPerBank(p.line_num),
          vrfBaseAddr(p.vrf_base_addr),
          registeredRequestVisibility(p.registered_request_visibility),
          liveRequesterIntent(p.live_requester_intent),
          legacyLockstep(p.legacy_lockstep),
          numBanks(p.num_pes * p.num_banks_per_lane),
          port_venusshuffle_hazardtable_listen(p.name + ".port_venusshuffle_hazardtable_listen", this),
          port_venusshuffle_receivefrom_venussequencer(p.name + ".port_venusshuffle_receivefrom_venussequencer", this),
          tickEvent([this]{ processFSM(); }, name())
    {

        for (int i = 0; i < numPes; ++i) {
            bankPorts.push_back(new BankPort(csprintf("%s.lane_port_%d", name(), i), *this, i));
        }

        peTaskQueues.resize(numPes);

        unsigned numLanes = numBanks / banksPerLane;
        laneBlocked.resize(numLanes, false);
        laneRetryReady.resize(numLanes, false);
        laneRoundRobin.resize(numLanes, 0);
        laneBlockedRequests.resize(
            numLanes, std::vector<bool>(numPes, false));
        pendingResponses.resize(numLanes);

        pes.resize(numPes);
        for(int i=0; i<numPes; ++i) {
            pes[i].id = i;
        }

        fatal_if(linesPerBank == 0, "VRF bank must contain at least one row");
        const Addr bankSpan = linesPerBank * sizeof(uint16_t);
        for (int i = 0; i < numBanks; ++i) {
            bankBaseAddrs.push_back(vrfBaseAddr + (i * bankSpan));
        }

    }

    Port &VenusShufflePipline::getPort(const std::string &if_name, PortID idx)
    {
        if (if_name == "lane_ports") {
            if (idx >= 0 && idx < bankPorts.size())
                return *bankPorts[idx];
        }
        if (if_name == "port_venusshuffle_receivefrom_venussequencer") {
            return port_venusshuffle_receivefrom_venussequencer; // 返回对应的端口对象
        }
        if (if_name == "port_venusshuffle_hazardtable_listen") {
            return port_venusshuffle_hazardtable_listen; // 返回对应的端口对象
        }
        return ClockedObject::getPort(if_name, idx);
    }

    void
    VenusShufflePipline::resetRtlTaskState()
    {
        fatal_if(commandActive || !cmdQueue.empty() || !instrpktQueue.empty(),
                 "%s received tile soft reset with an active command",
                 name());
        for (unsigned lane = 0; lane < laneRoundRobin.size(); ++lane) {
            fatal_if(laneBlocked[lane] || !pendingResponses[lane].empty(),
                     "%s received tile soft reset with lane %u in flight",
                     name(), lane);
            laneRoundRobin[lane] = 0;
            laneRetryReady[lane] = false;
            std::fill(laneBlockedRequests[lane].begin(),
                      laneBlockedRequests[lane].end(), false);
        }
        for (auto &pe : pes) {
            fatal_if(!pe.slots.empty() || pe.outstandingReads != 0,
                     "%s received tile soft reset with PE %d in flight",
                     name(), pe.id);
            pe.nextSeq = 0;
            pe.phase = PE_PHASE_INDEX;
            pe.totalElements = 0;
            pe.requestedIndex = 0;
            pe.receivedIndex = 0;
            pe.requestedData = 0;
            pe.receivedData = 0;
            pe.completedWrite = 0;
            pe.indexGrantThisCycle = false;
            pe.dataGrantThisCycle = false;
        }
        for (const auto &queue : peTaskQueues)
            fatal_if(!queue.empty(),
                     "%s received tile soft reset with queued PE work",
                     name());
        producerGrantCompletionReported = false;
        engineState = EngineState::Idle;
        legacyPhaseColdFilled.fill(false);
        legacyPhaseColdFillPending = false;
        legacyPhaseColdFillPhase = PE_PHASE_INDEX;
        legacyPhaseColdFillVisibleCycle = Cycles(0);
        shuffle_halt = false;
        if (tickEvent.scheduled())
            deschedule(tickEvent);
    }
    bool VenusShufflePipline::VenusShuffleHazardTableListenPort::recvTimingReq(PacketPtr pkt)
    {
        return owner->listenHazardTableRequest((VenusHazardTable*)pkt);
    }
    bool VenusShufflePipline::listenHazardTableRequest(VenusHazardTable* pkt)
    {
        if (pkt->lsu_completion_valid || pkt->producer_completion_valid) {
            const bool isLsuCompletion = pkt->lsu_completion_valid;
            const int runningId = isLsuCompletion ?
                pkt->lsu_completion_running_id :
                pkt->producer_completion_running_id;
            const int instructionId = isLsuCompletion ?
                pkt->lsu_completion_vns_instr_id :
                pkt->producer_completion_vns_instr_id;
            panic_if(venus_hazard_table == nullptr,
                     "shuffle completion arrived before its hazard table");
            panic_if(runningId < 0 || runningId >= NrIDs ||
                         instructionId < 0,
                     "invalid shuffle requester completion instr %d/rid %d",
                     instructionId, runningId);
            venus_hazard_table->retired_vns_instr_id[runningId] =
                std::max(venus_hazard_table->retired_vns_instr_id[runningId],
                         instructionId);
            bool clearedQueuedHazard = false;
            for (auto &queued : instrpktQueue) {
                if (!queued.shuffle_hazard_snapshot_valid ||
                    !queued.shuffle_hazard_snapshot[runningId])
                    continue;
                if (queued.shuffle_hazard_generation[runningId] <=
                    instructionId) {
                    queued.shuffle_hazard_snapshot[runningId] = false;
                    clearedQueuedHazard = true;
                }
            }
            /*
             * LDU pe_resp removes its generation from the sequencer's
             * combinational global_hazard_table_d.  Shuffle masks queued
             * hazard_d and evaluates IDLE from that d value on the same
             * tile edge.  The dedicated early LSU sideband can arrive after
             * this object's ordinary tick event, so re-evaluate that case at
             * the current tick.  Lane/Shuffle producer completions already
             * represent their registered requester-visible boundary and
             * deliberately remain on the following ordinary tick.
             */
            if (isLsuCompletion && clearedQueuedHazard &&
                !commandActive && !cmdQueue.empty()) {
                if (tickEvent.scheduled() && tickEvent.when() > curTick())
                    deschedule(tickEvent);
                if (!tickEvent.scheduled())
                    schedule(tickEvent, curTick());
            }
            DPRINTF(ShuffleEngine,
                    "%s completion forwarded to shuffle requester "
                    "instr %d/rid %d\n",
                    isLsuCompletion ? "LSU" : "producer",
                    instructionId, runningId);
            return true;
        }
        VenusHazardTable *replacement = new VenusHazardTable(pkt);
        if (venus_hazard_table != nullptr) {
            delete venus_hazard_table;
        }
        venus_hazard_table = replacement;
        return true;
    }
    bool VenusShufflePipline::VenusShuffleSequencerSidePort::recvTimingReq(PacketPtr pkt)
    {
        
        DPRINTF(ShuffleEngine,"VenusShuffleSequencerSidePort received some info.\n");
        if (!owner->ShufflehandleNewInstrRequest((VenusInstrPkt*)pkt)) {
            // needRetry = true;
            DPRINTF(ShuffleEngine,"VenusShuffleSequencerSidePort reports an nack.\n");
            return false;
        } else {
            DPRINTF(ShuffleEngine,"VenusShuffleSequencerSidePort reports an ack.\n");
            return true;
        }
    }
    bool VenusShufflePipline::ShufflehandleNewInstrRequest(VenusInstrPkt* pkt)
    {
        if(instrpktQueue.size() > 1)
        {
            return false; // 队列已满，拒绝新指令
        }



        unsigned full_vl = pkt->vl;
        unsigned processed_vl = 0;

        while (processed_vl < full_vl) {
            unsigned current_chunk_vl = full_vl - processed_vl;

            ShuffleCommand cmd;
            cmd.vs1_row = pkt->vs1_head;
            cmd.vs2_row = pkt->vs2_head;
            cmd.vd_row  = pkt->vd1_head;
            cmd.vl      = current_chunk_vl;
            cmd.start_idx = processed_vl;
            cmd.op      = (pkt->vm_r == 1) ? SHUFFLE_GATHER : SHUFFLE_SCATTER;
            cmd.width = (unsigned int)pkt->vew + 1;

            cmdQueue.push_back(cmd);
            processed_vl += current_chunk_vl;
        }
        instrpktQueue.push_back(*pkt);

        if (!tickEvent.scheduled()) {
            schedule(tickEvent, nextCycle());
        }
        return true;
    }

    void VenusShufflePipline::init()
    {
        ClockedObject::init();
        std::cout<<"Note: ASYNC ShuffleEngine Equipped!"<<std::endl;
    }

    VenusShufflePipline::BankLoc VenusShufflePipline::getBankLoc(unsigned rowOffset, unsigned idx, unsigned width)
    {
        constexpr unsigned bytesPerBankRow = sizeof(uint16_t);
        fatal_if(width != 1 && width != 2,
                 "Shuffle element width %u is not EW8/EW16", width);

        const unsigned elementsPerBank = bytesPerBankRow / width;
        const unsigned elementsPerRow = numBanks * elementsPerBank;
        const unsigned rowNumber = idx / elementsPerRow;
        const unsigned indexWithinRow = idx % elementsPerRow;
        const unsigned logicalGlobalBank = indexWithinRow / elementsPerBank;
        const unsigned laneIdx = logicalGlobalBank / banksPerLane;
        const unsigned logicalBank = logicalGlobalBank % banksPerLane;

        // venus_operand_requester.sv barber-poles a logical bank by the low
        // bank bits of the encoded vector row address.
        const unsigned encodedRow = (rowOffset + rowNumber) % linesPerBank;
        const unsigned physicalBank =
            (logicalBank + encodedRow) % banksPerLane;
        const unsigned physicalGlobalBank =
            laneIdx * banksPerLane + physicalBank;
        const unsigned byteInBank =
            (indexWithinRow % elementsPerBank) * width;
        const Addr addr = bankBaseAddrs[physicalGlobalBank] +
            encodedRow * bytesPerBankRow + byteInBank;

        return {static_cast<int>(laneIdx), static_cast<int>(physicalBank),
                static_cast<int>(physicalGlobalBank), addr};
    }

    void VenusShufflePipline::updateQueuedShuffleHazards()
    {
        if (venus_hazard_table == nullptr)
            return;

        for (auto &queued : instrpktQueue) {
            const int consumer = queued.running_id;
            const bool consumerCurrent =
                consumer >= 0 && consumer < NrIDs &&
                venus_hazard_table->running_id_to_vns_instr_id[consumer] ==
                    static_cast<int>(queued.vns_instr_id);

            /* Compatibility for direct unit injections which predate the
             * pe_req snapshot.  Normal Sequencer traffic always arrives
             * with a valid, allocation-time snapshot. */
            if (!queued.shuffle_hazard_snapshot_valid) {
                if (!consumerCurrent)
                    continue;
                queued.shuffle_hazard_snapshot_valid = true;
                for (int producer = 0; producer < NrIDs; ++producer) {
                    const bool hazard =
                        venus_hazard_table->global_hazard_table
                            [consumer][producer];
                    queued.shuffle_hazard_snapshot[producer] = hazard;
                    queued.shuffle_hazard_generation[producer] =
                        hazard ?
                            venus_hazard_table->
                                running_id_to_vns_instr_id[producer] : -1;
                }
            }

            for (int producer = 0; producer < NrIDs; ++producer) {
                if (!queued.shuffle_hazard_snapshot[producer])
                    continue;
                const int generation =
                    queued.shuffle_hazard_generation[producer];
                if (generation < 0 ||
                    venus_hazard_table->retired_vns_instr_id[producer] >=
                        generation) {
                    queued.shuffle_hazard_snapshot[producer] = false;
                    continue;
                }

                /*
                 * RTL performs queued_hazard &= global_hazard[consumer].
                 * Wait until this consumer generation is the broadcast row,
                 * then apply the same destructive mask.  The generation
                 * comparison additionally prevents a reused VID from
                 * reviving the old command-local dependency.
                 */
                if (consumerCurrent &&
                    (venus_hazard_table->running_id_to_vns_instr_id
                         [producer] != generation ||
                     !venus_hazard_table->global_hazard_table
                         [consumer][producer])) {
                    queued.shuffle_hazard_snapshot[producer] = false;
                }
            }
        }
    }

    bool VenusShufflePipline::frontShuffleHazardsReady() const
    {
        if (instrpktQueue.empty() ||
            !instrpktQueue.front().shuffle_hazard_snapshot_valid)
            return false;
        for (int producer = 0; producer < NrIDs; ++producer) {
            if (instrpktQueue.front().shuffle_hazard_snapshot[producer])
                return false;
        }
        return true;
    }

    std::string VenusShufflePipline::reportFrontShuffleHazards() const
    {
        std::string result;
        if (instrpktQueue.empty())
            return result;
        for (int producer = 0; producer < NrIDs; ++producer) {
            if (!instrpktQueue.front().shuffle_hazard_snapshot[producer])
                continue;
            if (!result.empty())
                result += ",";
            result += std::to_string(producer) + "@" +
                std::to_string(instrpktQueue.front().
                    shuffle_hazard_generation[producer]);
        }
        return result;
    }

    void VenusShufflePipline::processFSM()
    {
        if(shuffle_halt == true) {
            DPRINTF(ShuffleEngine,"VenusShufflePipline halt!.\n");
            schedule(tickEvent, nextCycle());
            return;
        }

        bool anyPeBusy = false;

        unsigned numLanes = numBanks / banksPerLane;

        /* recvTimingResp is asynchronous; only processFSM may make a grant
         * visible to the PE.  Reset these tile-edge markers before selecting
         * the registered req_q requests below. */
        for (auto &pe : pes) {
            pe.indexGrantThisCycle = false;
            pe.dataGrantThisCycle = false;
        }

        updateQueuedShuffleHazards();

        if (legacyLockstep && commandActive)
            noteLegacyPhaseCycle();

        /*
         * The RTL does not enter its pipelined PE datapath directly from
         * IDLE.  WAIT selects the shuffle path, STAGE3 initializes the PE
         * counters, and only the following EXECUTE edge may generate req_d.
         */
        if (commandActive && engineState == EngineState::Wait) {
            engineState = EngineState::Stage3;
            DPRINTF(ShuffleEngine, "[%llu] SH_FSM WAIT->STAGE3\n",
                    static_cast<unsigned long long>(curTick()));
            schedule(tickEvent, nextCycle());
            return;
        }
        if (commandActive && engineState == EngineState::Stage3) {
            engineState = EngineState::Execute;
            DPRINTF(ShuffleEngine, "[%llu] SH_FSM STAGE3->EXECUTE\n",
                    static_cast<unsigned long long>(curTick()));
            schedule(tickEvent, nextCycle());
            return;
        }
        if (commandActive && engineState == EngineState::Stage1) {
            DPRINTF(ShuffleEngine, "[%llu] SH_FSM STAGE1->IDLE done\n",
                    static_cast<unsigned long long>(curTick()));
            DPRINTF(ShuffleEngine, "[%d] Command Completed.\n", curTick());
            DPRINTF(LaneSequencer,
                    "reportAndRecycleDoneInstr with VID = %d\n",
                    instrpktQueue.front().vns_instr_id);

            reportLegacyPhaseSummary();

            VenusInstrPkt returninstr = instrpktQueue.front();
            returninstr.vns_instr_stat = INSTR_DONE;
            returninstr.vns_instr_log_endtick = curTick();
            returninstr.makeResponse();
            port_venusshuffle_receivefrom_venussequencer.sendTimingResp(
                (PacketPtr)&returninstr);
            instrpktQueue.pop_front();
            commandActive = false;
            engineState = EngineState::Idle;

            if (!cmdQueue.empty())
                schedule(tickEvent, nextCycle());
            return;
        }

        if (!commandActive && !cmdQueue.empty()) {
            /*
             * RTL transports the queue entry's ID and hazard vectors in one
             * pe_req handshake.  gem5 broadcasts the live table through
             * three explicit register stages, so do not let an ID-reuse or
             * pre-allocation table image make a newly queued command appear
             * ready.  Once this generation is visible, the live table clears
             * its captured dependencies as their exact generations retire.
             */
            const int runningId = instrpktQueue.front().running_id;
            const bool hazardTableIsCurrent =
                venus_hazard_table != nullptr &&
                venus_hazard_table->running_id_to_vns_instr_id[runningId] ==
                instrpktQueue.front().vns_instr_id;
            if(hazardTableIsCurrent &&
               frontShuffleHazardsReady()) {
                DPRINTF(LaneSequencer, "VenusShuffle insn send from laneSequencerFIFO to operandRequester, instr %d with runningID %d\n", instrpktQueue.front().vns_instr_id, instrpktQueue.front().running_id);
            } else {
                DPRINTF(LaneSequencer,
                        "VenusShuffle insn does not start, instr %d with "
                        "runningID %d, generation current=%d, captured "
                        "hazards:%s\n",
                        instrpktQueue.front().vns_instr_id, runningId,
                        hazardTableIsCurrent,
                        reportFrontShuffleHazards());
                schedule(tickEvent, nextCycle());
                return;
            }

            instrpktQueue.front().vns_instr_stat = INSTR_FIRED;
            instrpktQueue.front().vns_instr_log_starttick = curTick();

            currentCmd = cmdQueue.front();
            cmdQueue.pop_front();
            // nextElementToAssign = 0;
            commandActive = true;
            producerGrantCompletionReported = false;
            engineState = EngineState::Wait;

            // scheduledIndices.clear();
            for(auto& q : peTaskQueues) q.clear();
            for (auto &pe : pes) {
                pe.slots.clear();
                pe.nextSeq = 0;
                pe.phase = PE_PHASE_INDEX;
                pe.totalElements = 0;
                pe.requestedIndex = 0;
                pe.receivedIndex = 0;
                pe.requestedData = 0;
                pe.receivedData = 0;
                pe.completedWrite = 0;
                pe.outstandingReads = 0;
                pe.indexWaveRequests = 0;
                pe.dataWaveRequests = 0;
            }
            legacyPhaseCycles.fill(0);
            legacyPhaseGrants.fill(0);
            legacyPhaseColdFilled.fill(false);
            legacyPhaseColdFillPending = false;
            legacyPhaseColdFillPhase = PE_PHASE_INDEX;
            legacyPhaseColdFillVisibleCycle = Cycles(0);

            /*
             * Match venus_shuffle_engine.sv's shuffle_pe_vl and temp
             * traversal exactly.  A PE owns NrBankPerLane * ELEN/element
             * bits from every vector row (8 EW8 elements or 4 EW16
             * elements for the current RTL).  The RTL counts temp down
             * from shuffle_pe_vl - 1, so a PE visits its elements in
             * reverse order, including across vector rows.
             *
             * The old model always used numBanks (64) as an element row
             * and visited each group forwards.  That happened to describe
             * EW16's row size, but was wrong for EW8 and did not reproduce
             * the RTL request/return ordering for either width.
             */
            fatal_if(numPes != numBanks / banksPerLane,
                     "RTL requires one shuffle PE per lane");
            const unsigned elementsPerPe =
                banksPerLane * sizeof(uint16_t) / currentCmd.width;
            const unsigned elementsPerRow = numPes * elementsPerPe;
            const unsigned fullRows = currentCmd.vl / elementsPerRow;
            const unsigned tail = currentCmd.vl % elementsPerRow;
            unsigned maxPeWaves = 0;

            for (unsigned p = 0; p < numPes; ++p) {
                const int tailForPe = std::max(
                    0, std::min(static_cast<int>(elementsPerPe),
                                static_cast<int>(tail) -
                                static_cast<int>(p * elementsPerPe)));
                const unsigned peVl =
                    fullRows * elementsPerPe + tailForPe;
                maxPeWaves = std::max(maxPeWaves, peVl);

                for (int temp = static_cast<int>(peVl) - 1;
                     temp >= 0; --temp) {
                    const unsigned logicalIdx =
                        (temp / elementsPerPe) * elementsPerRow +
                        p * elementsPerPe + (temp % elementsPerPe);
                    peTaskQueues[p].push_back(logicalIdx);
                }
                pes[p].totalElements = peTaskQueues[p].size();
            }

            if (legacyLockstep &&
                std::getenv("VENUS_GEM5_SHUFFLE_SUMMARY") != nullptr) {
                std::cout << "SH_V1_SUMMARY op="
                          << (currentCmd.op == SHUFFLE_GATHER ? "gather" :
                              "scatter")
                          << " tick=" << curTick()
                          << " width=" << currentCmd.width
                          << " vl=" << currentCmd.vl
                          << " waves=" << maxPeWaves << std::endl;
            }

            DPRINTF(ShuffleEngine, "\n========================================\n");
            DPRINTF(ShuffleEngine, "[%d] NEW COMMAND STARTED (VL=%d) Total Sched Size=%d\n", curTick(), currentCmd.vl, currentCmd.vl);
            for(int i=0; i<numPes; ++i) {
                DPRINTF(ShuffleEngine, "  PE%d Tasks: %d\n", i, peTaskQueues[i].size());
            }
            DPRINTF(ShuffleEngine, "========================================\n");
            schedule(tickEvent, nextCycle());
            return;
        }

        if (!commandActive && cmdQueue.empty()) {
            for(const auto& pe : pes) if(!pe.slots.empty()) anyPeBusy = true;
            if(!anyPeBusy) return; 
        }

        /*
         * RTL separates the selection of a shuffle request from the cycle in
         * which the lane accepts it.  The PE first writes req_d/payload_d;
         * the registered req_q reaches the lane arbiter on the following tile
         * edge.  Treating sendTimingReq() as that first selection collapsed
         * req_d -> req_q -> grant into one GEM5 event and made every IDX,
         * DATA and WRITE request one cycle too early.
         */
        for (auto &pe : pes) {
            while (!pe.slots.empty() && pe.slots.front().state == PE_DONE)
                pe.slots.pop_front();
        }

        /*
         * A gem5 timing-port request already crosses an event boundary before
         * the SRAM can return ready/data.  Materialize req_q before lane
         * arbitration in this tile event so that the RTL request register is
         * not counted a second time on top of that timing-port boundary.
         */
        for (unsigned i = 0; i < numPes; ++i) {
            PEInfo &pe = pes[i];
            if (!pe.slots.empty())
                anyPeBusy = true;

            bool hasRegisteredEgress = false;
            for (const auto &candidate : pe.slots)
                hasRegisteredEgress = hasRegisteredEgress ||
                    candidate.egressRegistered;
            if (hasRegisteredEgress)
                continue;

            PESlot *slot = nullptr;
            switch (pe.phase) {
              case PE_PHASE_INDEX:
                if (pe.requestedIndex < pe.totalElements &&
                    (!legacyLockstep ||
                     pe.requestedIndex == pe.completedWrite) &&
                    pe.outstandingReads < MaxInFlight) {
                    for (auto &candidate : pe.slots) {
                        if (candidate.seq == pe.requestedIndex &&
                            candidate.state == PE_REQ_INDEX) {
                            slot = &candidate;
                            break;
                        }
                    }
                    if (!slot && !peTaskQueues[i].empty()) {
                        const int logicalIdx = peTaskQueues[i].front();
                        peTaskQueues[i].pop_front();
                        pe.slots.emplace_back(
                            pe.nextSeq++,
                            currentCmd.start_idx + logicalIdx);
                        slot = &pe.slots.back();
                    }
                }
                break;
              case PE_PHASE_DATA:
                if (pe.requestedData < pe.receivedIndex &&
                    (!legacyLockstep ||
                     pe.requestedData == pe.completedWrite) &&
                    pe.outstandingReads < MaxInFlight) {
                    for (auto &candidate : pe.slots) {
                        if (candidate.seq == pe.requestedData &&
                            candidate.state == PE_REQ_DATA) {
                            slot = &candidate;
                            break;
                        }
                    }
                }
                break;
              case PE_PHASE_WRITE:
                if (pe.completedWrite < pe.receivedData) {
                    for (auto &candidate : pe.slots) {
                        if (candidate.seq == pe.completedWrite &&
                            candidate.state == PE_REQ_WRITE) {
                            slot = &candidate;
                            break;
                        }
                    }
                }
                break;
            }

            if (slot == nullptr)
                continue;

            unsigned targetIdx = 0;
            unsigned targetRow = 0;
            if (slot->state == PE_REQ_INDEX) {
                targetIdx = slot->elementIdx;
                targetRow = currentCmd.vs2_row;
            } else if (slot->state == PE_REQ_DATA) {
                targetIdx = currentCmd.op == SHUFFLE_GATHER
                    ? slot->fetchedIndex : slot->elementIdx;
                targetRow = currentCmd.vs1_row;
            } else if (slot->state == PE_REQ_WRITE) {
                targetIdx = currentCmd.op == SHUFFLE_GATHER
                    ? slot->elementIdx : slot->fetchedIndex;
                targetRow = currentCmd.vd_row;
            } else {
                fatal("shuffle PE%d selected non-request state %d", pe.id,
                      slot->state);
            }

            const BankLoc loc = getBankLoc(
                targetRow, targetIdx,
                slot->state == PE_REQ_INDEX ? 2 : currentCmd.width);
            slot->egressRegistered = true;
            slot->egressLaneId = loc.laneIdx;
            slot->egressVisibleCycle = curCycle() +
                Cycles(registeredRequestVisibility ? 1 : 0);
            DPRINTF(ShuffleEngine,
                "[%llu] SH_EGRESS_REG PE%d seq=%u state=%d lane=%d "
                "row=%u idx=%u addr=%#llx\n",
                static_cast<unsigned long long>(curTick()), pe.id, slot->seq,
                slot->state, loc.laneIdx, targetRow, targetIdx,
                static_cast<unsigned long long>(loc.addr));
        }

        /*
         * First service the already-registered req_q requests.  Match
         * rr_arb_tree's FairArb path exactly: choose the lowest active input
         * whose index is strictly greater than rr_q, or wrap to the lowest
         * active input.  On a successful grant rr_q becomes the winning
         * index.  This is not equivalent to interpreting rr_q as one
         * priority bit per binary-tree level.
         */
        for (unsigned lane = 0; lane < numLanes; ++lane) {
            std::vector<PESlot *> laneRequests(numPes, nullptr);
            std::vector<bool> arbitrationRequests(numPes, false);
            PEInfo *selectedPe = nullptr;
            PESlot *selectedSlot = nullptr;

            for (unsigned peId = 0; peId < numPes; ++peId) {
                for (auto &candidate : pes[peId].slots) {
                    if (candidate.egressRegistered &&
                        candidate.egressVisibleCycle <= curCycle() &&
                        candidate.egressLaneId == static_cast<int>(lane)) {
                        fatal_if(laneRequests[peId] != nullptr,
                                 "shuffle PE%u has multiple registered "
                                 "egress requests on lane %u", peId, lane);
                        laneRequests[peId] = &candidate;
                    }
                }
            }

            if (laneBlocked[lane]) {
                if (!laneRetryReady[lane])
                    continue;
                arbitrationRequests = laneBlockedRequests[lane];
                for (unsigned peId = 0; peId < numPes; ++peId) {
                    PESlot *candidate = laneRequests[peId];
                    if (candidate != nullptr && candidate->blockedPkt != nullptr &&
                        candidate->blockedLaneId == static_cast<int>(lane)) {
                        selectedPe = &pes[peId];
                        selectedSlot = candidate;
                        break;
                    }
                }
                fatal_if(selectedSlot == nullptr,
                         "shuffle lane %u retried without its blocked request",
                         lane);
            } else {
                bool anyRequest = false;
                for (unsigned peId = 0; peId < numPes; ++peId) {
                    arbitrationRequests[peId] =
                        laneRequests[peId] != nullptr;
                    anyRequest |= arbitrationRequests[peId];
                }
                if (!anyRequest) {
                    if (liveRequesterIntent)
                        bankPorts[lane]->withdrawLiveIntent(nullptr);
                    continue;
                }

                const unsigned rr = laneRoundRobin[lane] % numPes;
                const int winner =
                    selectShuffleRrTree(arbitrationRequests, rr);

                fatal_if(winner < 0,
                         "shuffle lane %u RR selected no active request",
                         lane);
                selectedPe = &pes[static_cast<unsigned>(winner)];
                selectedSlot = laneRequests[static_cast<unsigned>(winner)];
                fatal_if(selectedSlot == nullptr,
                         "shuffle lane %u RR selected inactive PE%d", lane,
                         winner);
                DPRINTF(ShuffleEngine,
                    "[%llu] SH_LANE_SELECT lane=%u rr=%u req=%#llx "
                    "winner=%d seq=%u state=%d\n",
                    static_cast<unsigned long long>(curTick()), lane, rr,
                    static_cast<unsigned long long>(
                        shuffleRequestMask(arbitrationRequests)),
                    winner, selectedSlot->seq, selectedSlot->state);
            }

            const SlotState requestState = selectedSlot->state;
            fatal_if(requestState != PE_REQ_INDEX &&
                     requestState != PE_REQ_DATA &&
                     requestState != PE_REQ_WRITE,
                     "shuffle lane %u registered non-request state %d", lane,
                     requestState);

            unsigned targetIdx = 0;
            unsigned targetRow = 0;
            unsigned size = 0;
            MemCmd command = MemCmd::ReadReq;
            int passage = 9;
            SlotState waitState = requestState;
            if (requestState == PE_REQ_INDEX) {
                targetIdx = selectedSlot->elementIdx;
                targetRow = currentCmd.vs2_row;
                size = 2;
                waitState = PE_WAIT_INDEX;
            } else if (requestState == PE_REQ_DATA) {
                targetIdx = currentCmd.op == SHUFFLE_GATHER
                    ? selectedSlot->fetchedIndex : selectedSlot->elementIdx;
                targetRow = currentCmd.vs1_row;
                size = currentCmd.width;
                waitState = PE_WAIT_DATA;
            } else {
                targetIdx = currentCmd.op == SHUFFLE_GATHER
                    ? selectedSlot->elementIdx : selectedSlot->fetchedIndex;
                targetRow = currentCmd.vd_row;
                size = currentCmd.width;
                command = MemCmd::WriteReq;
                passage = 16;
                waitState = PE_WAIT_WRITE;
            }

            const BankLoc loc = getBankLoc(
                targetRow, targetIdx,
                requestState == PE_REQ_INDEX ? 2 : currentCmd.width);
            fatal_if(loc.laneIdx != static_cast<int>(lane),
                     "shuffle registered lane %u changed to lane %d", lane,
                     loc.laneIdx);

            PacketPtr pkt = selectedSlot->blockedPkt;
            if (pkt == nullptr) {
                RequestPtr req = std::make_shared<Request>(
                    loc.addr, size, 0, bankPorts[lane]->getId());
                pkt = new Packet(req, command, req->getSize(), passage);
                pkt->allocate();
                if (requestState == PE_REQ_WRITE) {
                    uint8_t data[2] = {
                        static_cast<uint8_t>(selectedSlot->fetchedData),
                        static_cast<uint8_t>(selectedSlot->fetchedData >> 8)
                    };
                    pkt->setData(data);
                }
                pkt->pushSenderState(
                    new ShuffleSenderState(selectedPe->id, selectedSlot->seq));
            }

            if (liveRequesterIntent) {
                fatal_if(laneBlocked[lane],
                         "shuffle live-intent lane %u entered timing retry",
                         lane);
                selectedSlot->blockedPkt = pkt;
                selectedSlot->blockedLaneId = static_cast<int>(lane);
                bankPorts[lane]->publishLiveIntent(pkt);
                DPRINTF(ShuffleEngine,
                    "[%llu] SH_LIVE_INTENT lane=%u rr=%u req=%#llx "
                    "winner=PE%d seq=%u state=%d bank=%d\n",
                    static_cast<unsigned long long>(curTick()), lane,
                    laneRoundRobin[lane],
                    static_cast<unsigned long long>(
                        shuffleRequestMask(arbitrationRequests)),
                    selectedPe->id, selectedSlot->seq, requestState,
                    loc.globalBankIdx);
                continue;
            }

            if (bankPorts[lane]->sendTimingReq(pkt)) {
                selectedSlot->blockedPkt = nullptr;
                selectedSlot->blockedLaneId = -1;
                selectedSlot->egressRegistered = false;
                selectedSlot->egressLaneId = -1;
                selectedSlot->state = waitState;
                if (requestState == PE_REQ_INDEX)
                    selectedPe->indexGrantThisCycle = true;
                else if (requestState == PE_REQ_DATA)
                    selectedPe->dataGrantThisCycle = true;
                requestAccepted(*selectedPe, *selectedSlot, requestState);
                laneBlocked[lane] = false;
                laneRetryReady[lane] = false;

                const unsigned oldRr = laneRoundRobin[lane];
                laneRoundRobin[lane] = nextShuffleFairRr(
                    arbitrationRequests, oldRr);
                DPRINTF(ShuffleEngine,
                    "[%llu] SH_LANE_RR lane=%u old=%u req=%#llx new=%u "
                    "grant=PE%d\n",
                    static_cast<unsigned long long>(curTick()), lane,
                    oldRr, static_cast<unsigned long long>(
                        shuffleRequestMask(arbitrationRequests)),
                    laneRoundRobin[lane], selectedPe->id);
                std::fill(laneBlockedRequests[lane].begin(),
                          laneBlockedRequests[lane].end(), false);
            } else {
                selectedSlot->blockedPkt = pkt;
                selectedSlot->blockedLaneId = static_cast<int>(lane);
                laneBlocked[lane] = true;
                laneRetryReady[lane] = false;
                laneBlockedRequests[lane] = arbitrationRequests;
            }
        }

        if (commandActive || anyPeBusy || !cmdQueue.empty()) {
            schedule(tickEvent, nextCycle());
        }

        bool allIdle = true;
        for(const auto& pe : pes) if(!pe.slots.empty()) allIdle = false;

        bool allQueuesEmpty = true;
        for(const auto& q : peTaskQueues) if(!q.empty()) allQueuesEmpty = false;

        if (commandActive && allQueuesEmpty && allIdle) {
            if (legacyLockstep &&
                !legacyPhaseColdFillReady(PE_PHASE_WRITE)) {
                if (!tickEvent.scheduled())
                    schedule(tickEvent, nextCycle());
                return;
            }
            /*
             * parallel_shuffle_complete_d and state_d are registered here.
             * The response is emitted from SHUFFLE_STAGE1 on the following
             * edge rather than directly from the last write grant.
             */
            engineState = EngineState::Stage1;
            DPRINTF(ShuffleEngine, "[%llu] SH_FSM EXECUTE->STAGE1\n",
                    static_cast<unsigned long long>(curTick()));
            if (!tickEvent.scheduled())
                schedule(tickEvent, nextCycle());
        }

        /*
         * The memory response event already represents the registered DSPM
         * output.  Retire one response per lane only after request selection,
         * preserving the RTL non-bypass behavior and arbitration order.  Its
         * new PE state can first be selected on the following tile edge.
         */
        const Cycles current_cycle = curCycle();
        for (unsigned lane = 0; lane < pendingResponses.size(); ++lane) {
            auto &responses = pendingResponses[lane];
            if (!responses.empty() &&
                responses.front().visibleCycle <= current_cycle) {
                PacketPtr pkt = responses.front().pkt;
                responses.pop_front();
                makeResponseVisible(pkt, lane);
            }
        }

        /*
         * Venus1 implements VSHUFFLE as one global three-stage loop:
         * every active PE must receive its index before any PE may request
         * data, every active PE must receive data before any PE may write,
         * and all writes must be granted before the next element begins.
         * Venus2 replaced that structure with independent pipelined PE
         * counters.  Keep both RTL generations in this shared component and
         * select the structure from the backend profile, never from a DAG,
         * instruction address or vector length.
         */
        advanceLegacyLockstepPhase();

    }

    unsigned
    VenusShufflePipline::legacyRtlPhase() const
    {
        switch (engineState) {
          case EngineState::Idle:
            return 0;
          case EngineState::Stage1:
            return 1;
          case EngineState::Wait:
            return 7;
          case EngineState::Stage3:
            return 6;
          case EngineState::Execute:
            break;
        }

        PePhase phase = PE_PHASE_INDEX;
        bool found = false;
        for (const auto &pe : pes) {
            if (pe.totalElements == 0)
                continue;
            if (!found) {
                phase = pe.phase;
                found = true;
            }
        }
        if (!found)
            return 1;
        const unsigned base =
            currentCmd.op == SHUFFLE_GATHER ? 8 : 3;
        return base + static_cast<unsigned>(phase);
    }

    void
    VenusShufflePipline::noteLegacyPhaseCycle()
    {
        ++legacyPhaseCycles.at(legacyRtlPhase());
    }

    void
    VenusShufflePipline::reportLegacyPhaseSummary() const
    {
        if (!legacyLockstep ||
            std::getenv("VENUS_GEM5_SHUFFLE_PHASE_SUMMARY") == nullptr)
            return;
        std::cout << "ACE_ECHO_GEM5_SHUFFLE_PHASE id "
                  << instrpktQueue.front().running_id
                  << " vm_r "
                  << (currentCmd.op == SHUFFLE_GATHER ? 1 : 0)
                  << " vew " << (currentCmd.width - 1)
                  << " vl " << currentCmd.vl
                  << " start "
                  << instrpktQueue.front().vns_instr_log_starttick
                  << " complete " << curTick() << " cycles ";
        for (unsigned state = 0; state < legacyPhaseCycles.size(); ++state) {
            if (state != 0)
                std::cout << ',';
            std::cout << legacyPhaseCycles[state];
        }
        std::cout << " grants ";
        for (unsigned state = 0; state < legacyPhaseGrants.size(); ++state) {
            if (state != 0)
                std::cout << ',';
            std::cout << legacyPhaseGrants[state];
        }
        std::cout << std::endl;
    }

    bool
    VenusShufflePipline::legacyPhaseColdFillReady(PePhase phase)
    {
        const unsigned phaseIndex = static_cast<unsigned>(phase);
        fatal_if(phaseIndex >= legacyPhaseColdFilled.size(),
                 "invalid legacy Shuffle phase %u", phaseIndex);
        if (legacyPhaseColdFilled[phaseIndex])
            return true;

        if (!legacyPhaseColdFillPending) {
            legacyPhaseColdFillPending = true;
            legacyPhaseColdFillPhase = phase;
            /* gnt_received_reg_q and operand_rec_tag_q are two separate RTL
             * registers.  Their cold-fill is paid once when a command first
             * traverses this global phase; later PE waves are pipelined. */
            legacyPhaseColdFillVisibleCycle = curCycle() + Cycles(2);
            return false;
        }
        fatal_if(legacyPhaseColdFillPhase != phase,
                 "legacy Shuffle cold-fill changed phase %d -> %d",
                 legacyPhaseColdFillPhase, phase);
        if (curCycle() < legacyPhaseColdFillVisibleCycle)
            return false;

        legacyPhaseColdFillPending = false;
        legacyPhaseColdFilled[phaseIndex] = true;
        return true;
    }

    void
    VenusShufflePipline::advanceLegacyLockstepPhase()
    {
        if (!legacyLockstep || !commandActive ||
            engineState != EngineState::Execute) {
            return;
        }

        PePhase phase = PE_PHASE_INDEX;
        bool foundNonEmptyPe = false;
        for (const auto &pe : pes) {
            if (pe.totalElements == 0)
                continue;
            if (!foundNonEmptyPe) {
                phase = pe.phase;
                foundNonEmptyPe = true;
            } else {
                fatal_if(pe.phase != phase,
                         "legacy shuffle PEs left lockstep: PE%d phase=%d "
                         "expected=%d", pe.id, pe.phase, phase);
            }
        }
        if (!foundNonEmptyPe)
            return;

        bool anyIncomplete = false;
        bool barrierReady = true;
        for (const auto &pe : pes) {
            if (pe.completedWrite >= pe.totalElements)
                continue;
            anyIncomplete = true;
            if (phase == PE_PHASE_INDEX) {
                barrierReady = barrierReady &&
                    pe.requestedIndex == pe.completedWrite + 1 &&
                    pe.receivedIndex == pe.requestedIndex;
            } else if (phase == PE_PHASE_DATA) {
                barrierReady = barrierReady &&
                    pe.requestedData == pe.completedWrite + 1 &&
                    pe.receivedData == pe.requestedData;
            } else {
                barrierReady = barrierReady &&
                    pe.completedWrite == pe.receivedData;
            }
        }

        /* A PE which completed its last element remains part of the write
         * barrier for that element.  Its counters are already equal, while
         * any PE still waiting for a write grant has done_wrt < rcv_dat. */
        if (phase == PE_PHASE_WRITE) {
            barrierReady = true;
            for (const auto &pe : pes) {
                if (pe.totalElements != 0 &&
                    pe.completedWrite != pe.receivedData) {
                    barrierReady = false;
                    break;
                }
            }
        }

        if (!barrierReady || (!anyIncomplete && phase != PE_PHASE_WRITE)) {
            return;
        }

        if (!legacyPhaseColdFillReady(phase))
            return;

        /*
         * The timing-port callback already represents the registered
         * requester grant/operand_rec_tag_q edge.  Do not add another wait at
         * later loop boundaries: STAGE3/4/5 and STAGE7/8/9 repeat for every
         * PE wave after their phase pipeline is filled.
         */

        const PePhase nextPhase = phase == PE_PHASE_INDEX ? PE_PHASE_DATA :
            (phase == PE_PHASE_DATA ? PE_PHASE_WRITE : PE_PHASE_INDEX);
        for (auto &pe : pes)
            pe.phase = nextPhase;
        DPRINTF(ShuffleEngine,
                "[%llu] SH_V1_BARRIER %s->%s\n",
                static_cast<unsigned long long>(curTick()),
                phase == PE_PHASE_INDEX ? "IDX" :
                    (phase == PE_PHASE_DATA ? "DATA" : "WRITE"),
                nextPhase == PE_PHASE_INDEX ? "IDX" :
                    (nextPhase == PE_PHASE_DATA ? "DATA" : "WRITE"));
    }

    void VenusShufflePipline::requestAccepted(PEInfo &pe, PESlot &slot,
        SlotState requestState)
    {
        if (legacyLockstep) {
            unsigned state =
                currentCmd.op == SHUFFLE_GATHER ? 8 : 3;
            if (requestState == PE_REQ_DATA)
                ++state;
            else if (requestState == PE_REQ_WRITE)
                state += 2;
            legacyPhaseGrants.at(state)++;
        }
        switch (requestState) {
          case PE_REQ_INDEX:
            fatal_if(pe.phase != PE_PHASE_INDEX ||
                     slot.seq != pe.requestedIndex,
                     "shuffle PE%d accepted unexpected index seq %u "
                     "(phase=%d expected=%u)", pe.id, slot.seq, pe.phase,
                     pe.requestedIndex);
            ++pe.requestedIndex;
            ++pe.outstandingReads;
            /* RTL checks idex_number_d before incrementing it, so a read
             * response can release the outstanding credit and permit the
             * final (fifth) request before this wave closes. */
            if (!legacyLockstep &&
                (pe.indexWaveRequests == MaxInFlight ||
                pe.requestedIndex == pe.totalElements ||
                pe.receivedIndex > pe.requestedData)) {
                pe.phase = PE_PHASE_DATA;
                pe.indexWaveRequests = 0;
                DPRINTF(ShuffleEngine,
                    "[%llu] SH_PHASE PE%d IDX->DATA req_idx=%u rcv_idx=%u\n",
                    static_cast<unsigned long long>(curTick()), pe.id,
                    pe.requestedIndex, pe.receivedIndex);
            } else {
                ++pe.indexWaveRequests;
            }
            break;

          case PE_REQ_DATA:
            fatal_if(pe.phase != PE_PHASE_DATA ||
                     slot.seq != pe.requestedData,
                     "shuffle PE%d accepted unexpected data seq %u "
                     "(phase=%d expected=%u)", pe.id, slot.seq, pe.phase,
                     pe.requestedData);
            ++pe.requestedData;
            ++pe.outstandingReads;
            if (!legacyLockstep &&
                (pe.dataWaveRequests == MaxInFlight ||
                pe.requestedData == pe.totalElements ||
                pe.requestedData == pe.requestedIndex)) {
                pe.phase = PE_PHASE_WRITE;
                pe.dataWaveRequests = 0;
                DPRINTF(ShuffleEngine,
                    "[%llu] SH_PHASE PE%d DATA->WRITE req_dat=%u req_idx=%u\n",
                    static_cast<unsigned long long>(curTick()), pe.id,
                    pe.requestedData, pe.requestedIndex);
            } else {
                ++pe.dataWaveRequests;
            }
            break;

          case PE_REQ_WRITE:
            fatal_if(pe.phase != PE_PHASE_WRITE ||
                     slot.seq != pe.completedWrite,
                     "shuffle PE%d accepted unexpected write seq %u "
                     "(phase=%d expected=%u)", pe.id, slot.seq, pe.phase,
                     pe.completedWrite);
            /* scalar RTL counts a shuffle write complete at the lane grant,
             * not at an eventual SRAM write response. */
            slot.state = PE_DONE;
            ++pe.completedWrite;
            if (!legacyLockstep &&
                pe.completedWrite == pe.requestedData) {
                /*
                 * An index response may become visible while this PE is in
                 * WRITE.  In that case requestedIndex is already ahead of
                 * requestedData and returning unconditionally to INDEX
                 * leaves no index request to issue: the PE has credit only
                 * for DATA and deadlocks at the end of a wave.  Select from
                 * the registered counters, matching the RTL's next-state
                 * test after operand_rec_tag_q is folded into the command
                 * state.  This is a generic shuffle credit rule; it does
                 * not depend on an instruction address or workload shape.
                 */
                const bool bufferedIndex =
                    pe.requestedData < pe.receivedIndex;
                pe.phase = bufferedIndex ? PE_PHASE_DATA : PE_PHASE_INDEX;
                DPRINTF(ShuffleEngine,
                    "[%llu] SH_PHASE PE%d WRITE->%s done=%u req_dat=%u "
                    "rcv_idx=%u\n",
                    static_cast<unsigned long long>(curTick()), pe.id,
                    bufferedIndex ? "DATA" : "IDX", pe.completedWrite,
                    pe.requestedData, pe.receivedIndex);
            }
            maybeReportProducerGrantCompletion();
            break;

          default:
            fatal("shuffle PE%d accepted non-request state %d", pe.id,
                  requestState);
        }
    }

    void
    VenusShufflePipline::maybeReportProducerGrantCompletion()
    {
        if (!commandActive || producerGrantCompletionReported ||
            instrpktQueue.empty()) {
            return;
        }
        for (const auto &tasks : peTaskQueues) {
            if (!tasks.empty())
                return;
        }
        for (const auto &pe : pes) {
            if (pe.completedWrite != pe.totalElements ||
                pe.outstandingReads != 0) {
                return;
            }
            for (const auto &slot : pe.slots) {
                if (slot.state != PE_DONE)
                    return;
            }
        }

        producerGrantCompletionReported = true;
        port_venusshuffle_receivefrom_venussequencer.
            reportProducerGrantCompletion(&instrpktQueue.front());
        DPRINTF(ShuffleEngine,
                "[%llu] SH_FINAL_BANK_GRANTS instr=%d rid=%d\n",
                static_cast<unsigned long long>(curTick()),
                instrpktQueue.front().vns_instr_id,
                instrpktQueue.front().running_id);
    }

    bool VenusShufflePipline::handleResponse(PacketPtr pkt, int portId)
    {
        fatal_if(portId < 0 ||
                 static_cast<unsigned>(portId) >= pendingResponses.size(),
                 "shuffle response on invalid lane %d", portId);

        /*
         * Only VRF reads cross venus_extension.sv's non-bypass operand
         * spill register.  A shuffle destination write completes from the
         * shuffle_result_req/gnt handshake itself; it does not return on the
         * operand path.  The timing memory still produces a write response,
         * so consume that acknowledgement here without placing it in the
         * read-response spill.  Otherwise a 3 ns read response and the next
         * cycle's 1 ns write response can arrive together and create a
        * response burst which cannot exist on the RTL operand path.
         */
        if (pkt->isWrite()) {
            ShuffleSenderState *senderState =
                dynamic_cast<ShuffleSenderState *>(pkt->popSenderState());
            fatal_if(senderState == nullptr,
                     "shuffle write response on lane %d has no sender state",
                     portId);
            delete senderState;
            delete pkt;
            return true;
        }

        const unsigned responsePipelineDepth = legacyLockstep ?
            LegacyLaneResponsePipelineDepth : MaxLaneResponsePipelineDepth;
        fatal_if(pendingResponses[portId].size() >= responsePipelineDepth,
                 "shuffle lane %d response spill overflow", portId);

        /*
         * The common response path represents the synchronous DSPM return at
         * the timing-memory boundary.  Venus1 then crosses the registered
         * requester/operand-return path before the global Shuffle FSM may
         * change phase.  The per-grant oracle shows three response-visible
         * edges; the remaining two-cycle discrepancy belongs to the
         * command-level FSM boundary, not to every PE response.  Venus2 uses
         * independent pipelined PE counters and does not traverse this
         * legacy path.
         */
        const Cycles visibleCycle = curCycle() +
            Cycles(legacyLockstep ? 3 : 1);
        pendingResponses[portId].emplace_back(pkt, visibleCycle);

        DPRINTF(ShuffleEngine,
            "[%llu] Shuffle response arrived on lane %d; visible at cycle %llu\n",
            static_cast<unsigned long long>(curTick()), portId,
            static_cast<unsigned long long>(visibleCycle));

        if (!tickEvent.scheduled())
            schedule(tickEvent, nextCycle());
        return true;
    }

    void VenusShufflePipline::makeResponseVisible(PacketPtr pkt, int portId)
    {
        ShuffleSenderState* senderState = dynamic_cast<ShuffleSenderState*>(pkt->popSenderState());
        fatal_if(senderState == nullptr,
                 "shuffle response on lane %d has no sender state", portId);
        int peId = senderState->peId;
        unsigned seq = senderState->seq;
        delete senderState;

        PEInfo& pe = pes[peId];
        auto slotIt = std::find_if(
            pe.slots.begin(), pe.slots.end(),
            [seq](const PESlot &slot) { return slot.seq == seq; });
        fatal_if(slotIt == pe.slots.end(),
                 "shuffle response for retired PE%d ROB seq %d", peId, seq);
        PESlot &slot = *slotIt;

        if (slot.state == PE_WAIT_INDEX && pkt->isRead()) {
            fatal_if(pe.outstandingReads == 0,
                     "shuffle PE%d index response without an outstanding read",
                     pe.id);
            --pe.outstandingReads;
            ++pe.receivedIndex;
            slot.fetchedIndex = (uint16_t)pkt->getLE<uint16_t>();
            slot.state = PE_REQ_DATA;
            DPRINTF(ShuffleEngine,
                "[%llu] SH_INDEX_RESP PE%d seq=%u element=%u index=%u\n",
                static_cast<unsigned long long>(curTick()), pe.id, seq,
                slot.elementIdx, slot.fetchedIndex);
            /*
             * In SHUFFLE_EXECUTE the RTL first folds an operand return into
             * cnt_rcv_idx_d, then the index-request branch tests that D
             * value to hand the PE to DATA.  Our memory callback is made
             * visible after this cycle's request selection, so reproduce
             * the same registered handoff here for the following cycle.
             * Without it, four accepted requests can fill the ROB just
             * before the first registered response becomes visible and
             * leave the PE stuck in INDEX with no free request slot.
             */
            const bool indexRequestPending = std::any_of(
                pe.slots.begin(), pe.slots.end(),
                [](const PESlot &candidate) {
                    return candidate.state == PE_REQ_INDEX;
                });
            if (!legacyLockstep && pe.phase == PE_PHASE_INDEX &&
                !indexRequestPending &&
                pe.receivedIndex > pe.requestedData) {
                pe.phase = PE_PHASE_DATA;
                pe.indexWaveRequests = 0;
                DPRINTF(ShuffleEngine,
                    "[%llu] SH_PHASE PE%d IDX->DATA on response "
                    "rcv_idx=%u req_dat=%u\n",
                    static_cast<unsigned long long>(curTick()), pe.id,
                    pe.receivedIndex, pe.requestedData);
            }
        }
        else if (slot.state == PE_WAIT_DATA && pkt->isRead()) {
            fatal_if(pe.outstandingReads == 0,
                     "shuffle PE%d data response without an outstanding read",
                     pe.id);
            --pe.outstandingReads;
            ++pe.receivedData;
            slot.fetchedData = 0;
            unsigned bytesToCopy = (pkt->getSize() > 2) ? 2 : pkt->getSize();
            if (bytesToCopy == 1)
                slot.fetchedData = pkt->getLE<uint8_t>();
            else if (bytesToCopy == 2)
                slot.fetchedData = pkt->getLE<uint16_t>();
            slot.state = PE_REQ_WRITE;
            DPRINTF(ShuffleEngine,
                "[%llu] SH_DATA_RESP PE%d seq=%u element=%u index=%u "
                "data=%#x\n",
                static_cast<unsigned long long>(curTick()), pe.id, seq,
                slot.elementIdx, slot.fetchedIndex, slot.fetchedData);
            /* Same D-state handoff as cnt_rcv_dat_d > cnt_done_wrt_d
             * in venus_shuffle_engine.sv. */
            const bool dataRequestPending = std::any_of(
                pe.slots.begin(), pe.slots.end(),
                [](const PESlot &candidate) {
                    return candidate.state == PE_REQ_DATA;
                });
            if (!legacyLockstep && pe.phase == PE_PHASE_DATA &&
                !dataRequestPending &&
                pe.receivedData > pe.completedWrite) {
                pe.phase = PE_PHASE_WRITE;
                pe.dataWaveRequests = 0;
                DPRINTF(ShuffleEngine,
                    "[%llu] SH_PHASE PE%d DATA->WRITE on response "
                    "rcv_dat=%u done=%u\n",
                    static_cast<unsigned long long>(curTick()), pe.id,
                    pe.receivedData, pe.completedWrite);
            }
        } else {
            fatal("shuffle response does not match PE%d ROB seq %d state %d",
                  peId, seq, slot.state);
        }
        delete pkt;
    }


    void VenusShufflePipline::handleRetry(int portId)
    {
        fatal_if(portId < 0 ||
                 static_cast<unsigned>(portId) >= laneBlocked.size(),
                 "shuffle retry on invalid lane %d", portId);

        /*
         * The deferred VRF crossbar uses recvReqRetry as the combinational
         * grant for a request which remains registered in req_q.  Capture
         * only that tagged request here.  Re-entering the whole shuffle FSM
         * would also create and arbitrate new req_d values in the same tick,
         * allowing a PE to cross several RTL register states on one edge.
         */
        laneRetryReady[portId] = true;
        DPRINTF(ShuffleEngine,
                "[%llu] Shuffle lane %d registered request granted\n",
                static_cast<unsigned long long>(curTick()), portId);

        PEInfo *selectedPe = nullptr;
        PESlot *selectedSlot = nullptr;
        for (auto &pe : pes) {
            for (auto &candidate : pe.slots) {
                if (candidate.egressRegistered &&
                    candidate.blockedPkt != nullptr &&
                    candidate.blockedLaneId == portId) {
                    fatal_if(selectedSlot != nullptr,
                             "shuffle lane %d has multiple blocked tagged "
                             "requests", portId);
                    selectedPe = &pe;
                    selectedSlot = &candidate;
                }
            }
        }
        fatal_if(selectedSlot == nullptr,
                 "shuffle lane %d grant has no blocked tagged request",
                 portId);

        const SlotState requestState = selectedSlot->state;
        SlotState waitState = requestState;
        if (requestState == PE_REQ_INDEX)
            waitState = PE_WAIT_INDEX;
        else if (requestState == PE_REQ_DATA)
            waitState = PE_WAIT_DATA;
        else if (requestState == PE_REQ_WRITE)
            waitState = PE_WAIT_WRITE;
        else
            fatal("shuffle lane %d granted non-request state %d", portId,
                  requestState);

        PacketPtr pkt = selectedSlot->blockedPkt;
        if (bankPorts[portId]->sendTimingReq(pkt)) {
            selectedSlot->blockedPkt = nullptr;
            selectedSlot->blockedLaneId = -1;
            selectedSlot->egressRegistered = false;
            selectedSlot->egressLaneId = -1;
            selectedSlot->state = waitState;
            if (requestState == PE_REQ_INDEX)
                selectedPe->indexGrantThisCycle = true;
            else if (requestState == PE_REQ_DATA)
                selectedPe->dataGrantThisCycle = true;
            requestAccepted(*selectedPe, *selectedSlot, requestState);
            laneBlocked[portId] = false;
            laneRetryReady[portId] = false;
            const std::vector<bool> &requests =
                laneBlockedRequests[portId];
            fatal_if(!requests[static_cast<unsigned>(selectedPe->id)],
                     "shuffle lane %d blocked tag PE%d is absent from its "
                     "stable arbitration vector", portId, selectedPe->id);
            const unsigned oldRr = laneRoundRobin[portId];
            laneRoundRobin[portId] = nextShuffleFairRr(requests, oldRr);
            DPRINTF(ShuffleEngine,
                "[%llu] SH_LANE_RR lane=%d old=%u req=%#llx new=%u "
                "grant=PE%d retry=1\n",
                static_cast<unsigned long long>(curTick()), portId, oldRr,
                static_cast<unsigned long long>(
                    shuffleRequestMask(requests)),
                laneRoundRobin[portId], selectedPe->id);
            std::fill(laneBlockedRequests[portId].begin(),
                      laneBlockedRequests[portId].end(), false);
        } else {
            /*
             * A true downstream retry leaves the same packet and tag
             * registered until its next callback.
             */
            laneRetryReady[portId] = false;
        }

        if (!tickEvent.scheduled())
            schedule(tickEvent, nextCycle());
    }

    void
    VenusShufflePipline::handleLiveIntentGrant(int portId, PacketPtr pkt)
    {
        fatal_if(!liveRequesterIntent,
                 "shuffle lane %d received an unexpected live grant",
                 portId);

        PEInfo *selectedPe = nullptr;
        PESlot *selectedSlot = nullptr;
        std::vector<bool> requests(numPes, false);
        for (unsigned peId = 0; peId < numPes; ++peId) {
            for (auto &candidate : pes[peId].slots) {
                if (candidate.egressRegistered &&
                    candidate.egressVisibleCycle <= curCycle() &&
                    candidate.egressLaneId == portId) {
                    requests[peId] = true;
                }
                if (candidate.blockedPkt == pkt) {
                    fatal_if(selectedSlot != nullptr,
                             "shuffle live packet has multiple PE owners");
                    selectedPe = &pes[peId];
                    selectedSlot = &candidate;
                }
            }
        }
        fatal_if(selectedSlot == nullptr,
                 "shuffle lane %d live grant has no PE owner", portId);

        const int winner = selectShuffleRrTree(
            requests, laneRoundRobin[portId] % numPes);
        fatal_if(winner != selectedPe->id,
                 "shuffle lane %d granted stale PE%d, live winner is PE%d",
                 portId, selectedPe->id, winner);

        const SlotState requestState = selectedSlot->state;
        SlotState waitState;
        if (requestState == PE_REQ_INDEX)
            waitState = PE_WAIT_INDEX;
        else if (requestState == PE_REQ_DATA)
            waitState = PE_WAIT_DATA;
        else if (requestState == PE_REQ_WRITE)
            waitState = PE_WAIT_WRITE;
        else
            fatal("shuffle lane %d live grant has state %d", portId,
                  requestState);

        selectedSlot->blockedPkt = nullptr;
        selectedSlot->blockedLaneId = -1;
        selectedSlot->egressRegistered = false;
        selectedSlot->egressLaneId = -1;
        selectedSlot->state = waitState;
        if (requestState == PE_REQ_INDEX)
            selectedPe->indexGrantThisCycle = true;
        else if (requestState == PE_REQ_DATA)
            selectedPe->dataGrantThisCycle = true;
        requestAccepted(*selectedPe, *selectedSlot, requestState);

        const unsigned oldRr = laneRoundRobin[portId];
        laneRoundRobin[portId] = nextShuffleFairRr(requests, oldRr);
        DPRINTF(ShuffleEngine,
            "[%llu] SH_LIVE_GRANT lane=%d old_rr=%u req=%#llx "
            "new_rr=%u grant=PE%d seq=%u state=%d\n",
            static_cast<unsigned long long>(curTick()), portId, oldRr,
            static_cast<unsigned long long>(shuffleRequestMask(requests)),
            laneRoundRobin[portId], selectedPe->id, selectedSlot->seq,
            requestState);

        if (!tickEvent.scheduled())
            schedule(tickEvent, nextCycle());
    }
}
