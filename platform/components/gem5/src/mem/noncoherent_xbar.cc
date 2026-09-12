/*
 * Copyright (c) 2011-2015, 2018-2019 ARM Limited
 * All rights reserved
 *
 * The license below extends only to copyright in the software and shall
 * not be construed as granting a license to any other intellectual
 * property including but not limited to intellectual property relating
 * to a hardware implementation of the functionality of the software
 * licensed hereunder.  You may use the software subject to the license
 * terms below provided that you ensure that this notice is replicated
 * unmodified and in its entirety in all distributions of the software,
 * modified or unmodified, in source code or in binary form.
 *
 * Copyright (c) 2006 The Regents of The University of Michigan
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met: redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer;
 * redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution;
 * neither the name of the copyright holders nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @file
 * Definition of a non-coherent crossbar object.
 */

#include "mem/noncoherent_xbar.hh"

#include "base/logging.hh"
#include "base/trace.hh"
#include "debug/NoncoherentXBar.hh"
#include "debug/XBar.hh"

namespace gem5
{

    NoncoherentXBar::NoncoherentXBar(const NoncoherentXBarParams &p)
        : BaseXBar(p),
          venusVrfDataBanks(p.venus_vrf_data_banks),
          venusVrfMaskBanks(p.venus_vrf_mask_banks),
          venusVrfArbitratedBanks(
              venusVrfDataBanks + venusVrfMaskBanks),
          venusVrfPortsPerLane(p.venus_vrf_ports_per_lane),
          bufferResponses(p.buffer_responses),
          venusVrfPending(venusVrfArbitratedBanks),
          venusVrfSelected(venusVrfArbitratedBanks, InvalidPortID),
          venusVrfSelectedNextRr(venusVrfArbitratedBanks, 0),
          venusVrfRr(venusVrfArbitratedBanks, 0),
          venusVrfDownstreamRetry(venusVrfArbitratedBanks, false),
          venusVrfLastGrantTick(venusVrfArbitratedBanks, MaxTick),
          venusVrfArbitrationEvent(
              [this] { arbitrateVenusVrfRequests(); },
              name() + ".venus_vrf_arbitration", false,
              /*
               * Collect every requester that becomes combinationally
               * visible on this lane edge before choosing a winner.  Some
               * operand requesters are woken by same-tick VRF responses and
               * therefore run after ordinary Default_Pri callbacks; +1 can
               * arbitrate a partial vector.  Maximum_Pri is the gem5
               * end-of-edge phase corresponding to RTL's stable req_lvl2.
               */
              Event::Maximum_Pri)
    {
        panic_if(venusVrfArbitratedBanks >
                     p.port_mem_side_ports_connection_count,
                 "Venus VRF arbitration covers %u banks but %s has only %u "
                 "memory-side ports",
                 venusVrfArbitratedBanks, name(),
                 p.port_mem_side_ports_connection_count);
        panic_if(venusVrfArbitratedBanks && venusVrfPortsPerLane != 16,
                 "Venus VRF master decoding currently requires 16 lane "
                 "ports, got %u", venusVrfPortsPerLane);
        panic_if(venusVrfMaskBanks &&
                     venusVrfDataBanks != venusVrfMaskBanks * 4,
                 "Venus mask arbitration expects four data banks per lane: "
                 "%u data banks, %u mask banks",
                 venusVrfDataBanks, venusVrfMaskBanks);

        // create the ports based on the size of the memory-side port and
        // CPU-side port vector ports, and the presence of the default port,
        // the ports are enumerated starting from zero
        for (int i = 0; i < p.port_mem_side_ports_connection_count; ++i)
        {
            std::string portName = csprintf("%s.mem_side_port[%d]", name(), i);
            RequestPort *bp = new NoncoherentXBarRequestPort(portName, *this, i);
            memSidePorts.push_back(bp);
            reqLayers.push_back(new ReqLayer(*bp, *this,
                                             csprintf("reqLayer%d", i)));
        }

        // see if we have a default CPU-side-port device connected and if so add
        // our corresponding memory-side port
        if (p.port_default_connection_count)
        {
            defaultPortID = memSidePorts.size();
            std::string portName = name() + ".default";
            RequestPort *bp = new NoncoherentXBarRequestPort(portName, *this,
                                                             defaultPortID);
            memSidePorts.push_back(bp);
            reqLayers.push_back(new ReqLayer(*bp, *this, csprintf("reqLayer%d", defaultPortID)));
        }

        // create the CPU-side ports, once again starting at zero
        for (int i = 0; i < p.port_cpu_side_ports_connection_count; ++i)
        {
            std::string portName = csprintf("%s.cpu_side_ports[%d]", name(), i);
            QueuedResponsePort *bp = new NoncoherentXBarResponsePort(portName,
                                                                     *this, i);
            cpuSidePorts.push_back(bp);
            respLayers.push_back(new RespLayer(*bp, *this,
                                               csprintf("respLayer%d", i)));
        }
    }

void
NoncoherentXBar::resetVenusVrfArbitrationState()
{
    if (venusVrfArbitratedBanks == 0)
        return;

    fatal_if(!venusVrfLiveIntents.empty(),
             "%s received tile soft reset with live VRF intents", name());
    for (unsigned bank = 0; bank < venusVrfArbitratedBanks; ++bank) {
        fatal_if(!venusVrfPending[bank].empty() ||
                     venusVrfSelected[bank] != InvalidPortID ||
                     venusVrfDownstreamRetry[bank],
                 "%s received tile soft reset with VRF bank %u in flight",
                 name(), bank);
        venusVrfSelectedNextRr[bank] = 0;
        venusVrfRr[bank] = 0;
        venusVrfLastGrantTick[bank] = MaxTick;
    }
    venusVrfLsuPriorityVectors.clear();
    if (venusVrfArbitrationEvent.scheduled())
        deschedule(venusVrfArbitrationEvent);
}

    bool
    NoncoherentXBar::usesVenusVrfArbitration(
        PortID mem_side_port_id) const
    {
        return mem_side_port_id >= 0 &&
               static_cast<unsigned>(mem_side_port_id) <
                   venusVrfArbitratedBanks;
    }

    int
    NoncoherentXBar::venusVrfMaster(
        PortID mem_side_port_id, PortID cpu_side_port_id) const
    {
        const unsigned port =
            static_cast<unsigned>(cpu_side_port_id) %
            venusVrfPortsPerLane;
        if (static_cast<unsigned>(mem_side_port_id) >= venusVrfDataBanks) {
            const unsigned maskLane =
                static_cast<unsigned>(mem_side_port_id) -
                venusVrfDataBanks;
            const unsigned sourceLane =
                static_cast<unsigned>(cpu_side_port_id) /
                venusVrfPortsPerLane;
            if (sourceLane != maskLane)
                return -1;
            if (port == 9)
                return 0; // vmask_read
            if (port == 13)
                return 1; // vmask_write
            return -1;
        }
        if (port <= 7)
            return port;
        if (port == 12)
            return 8;  // BitALU result
        if (port == 10 || port == 11)
            return 9;  // paired CAU result passages form one RTL master
        if (port == 14)
            return 10; // SerDiv result
        if (port == 8 || port == 15)
            return 11; // shuffle read/write requester
        return -1;      // mask SRAM passages never target a data bank
    }

    int
    NoncoherentXBar::selectVenusVrfMaster(
        const std::vector<bool>& requests, unsigned rr) const
    {
        const unsigned numMasters = requests.size();
        panic_if(numMasters != 2 && numMasters != 12,
                 "Venus VRF arbiter expected 2 or 12 masters, got %u",
                 numMasters);
        panic_if(rr >= numMasters,
                 "Venus VRF arbiter RR pointer %u is out of range", rr);

        /*
         * rr_arb_tree.sv always chooses the current winner in gen_levels.
         * FairArb changes only the next rr_q value; it does not bypass this
         * binary tree.  At every level the right subtree wins when the left
         * subtree is empty, or when both are non-empty and the corresponding
         * rr_q bit is one.  NumIn=12 is embedded in the RTL's 16-leaf tree.
         */
        const unsigned numLevels = numMasters == 2 ? 1 : 4;
        unsigned first = 0;
        unsigned width = 1U << numLevels;
        for (int bit = numLevels - 1; bit >= 0; --bit) {
            const unsigned half = width / 2;
            bool left = false;
            bool right = false;
            for (unsigned master = first;
                 master < first + half && master < numMasters; ++master) {
                left |= requests[master];
            }
            for (unsigned master = first + half;
                 master < first + width && master < numMasters; ++master) {
                right |= requests[master];
            }
            panic_if(!left && !right,
                     "Venus VRF tree reached an empty subtree");
            const bool selectRight = !left || (right && ((rr >> bit) & 1U));
            if (selectRight)
                first += half;
            width = half;
        }
        panic_if(first >= numMasters || !requests[first],
                 "Venus VRF tree selected invalid master %u", first);
        return static_cast<int>(first);
    }

    unsigned
    NoncoherentXBar::nextVenusVrfRr(
        const std::vector<bool>& requests, unsigned rr) const
    {
        const unsigned numMasters = requests.size();
        panic_if(numMasters != 2 && numMasters != 12,
                 "Venus VRF arbiter expected 2 or 12 masters, got %u",
                 numMasters);
        panic_if(rr >= numMasters,
                 "Venus VRF arbiter RR pointer %u is out of range", rr);

        /* FairArb's upper/lower LZCs compute rr_d independently from the
         * tree winner: lowest request strictly above rr_q, wrapping to the
         * lowest request at or below rr_q. */
        for (unsigned master = rr + 1; master < numMasters; ++master) {
            if (requests[master])
                return master;
        }
        for (unsigned master = 0; master <= rr; ++master) {
            if (requests[master])
                return master;
        }
        panic("Venus VRF FairArb cannot advance an empty request vector");
    }

    void
    NoncoherentXBar::scheduleVenusVrfArbitration()
    {
        if (venusVrfArbitrationEvent.scheduled())
            return;
        schedule(venusVrfArbitrationEvent, curTick());
    }

    void
    NoncoherentXBar::publishVenusVrfLiveIntent(
        PortID source, PacketPtr pkt)
    {
        panic_if(pkt == nullptr, "Venus VRF live intent has no packet");
        const PortID bank = findPort(pkt);
        panic_if(!usesVenusVrfArbitration(bank),
                 "Venus VRF live intent targets non-data bank %d", bank);

        auto old = venusVrfLiveIntents.find(source);
        if (old != venusVrfLiveIntents.end()) {
            const PortID oldBank = old->second.first;
            if (venusVrfSelected[oldBank] == source) {
                /* The packet has already entered a downstream timing retry.
                 * It is no longer a combinational intent and must remain
                 * stable until that memory-side transaction is accepted. */
                panic_if(old->second.second != pkt,
                         "Venus VRF source %d replaced selected live intent",
                         source);
                return;
            }
            venusVrfLiveIntents.erase(old);
        }
        venusVrfLiveIntents[source] = {bank, pkt};
        DPRINTF(NoncoherentXBar,
                "Venus VRF live intent src %d bank %d addr %#x\n",
                source, bank, pkt->getAddr());
        scheduleVenusVrfArbitration();
    }

    void
    NoncoherentXBar::withdrawVenusVrfLiveIntent(
        PortID source, PacketPtr pkt)
    {
        auto current = venusVrfLiveIntents.find(source);
        if (current == venusVrfLiveIntents.end())
            return;
        if (pkt != nullptr && current->second.second != pkt)
            return;
        const PortID bank = current->second.first;
        if (venusVrfSelected[bank] == source)
            return;
        venusVrfLiveIntents.erase(current);
        DPRINTF(NoncoherentXBar,
                "Venus VRF live intent withdrawn src %d bank %d\n",
                source, bank);
    }

    void
    NoncoherentXBar::reserveVenusVrfLsuPriority(
        uint64_t requester_tag, uint64_t bank_mask,
        Tick first_tick, unsigned grants, Tick stride)
    {
        if (!venusVrfDataBanks || bank_mask == 0 || grants == 0)
            return;
        panic_if(venusVrfDataBanks > 64,
                 "Venus VRF LSU bank vector supports at most 64 banks");
        panic_if(stride == 0,
                 "Venus VRF LSU priority stride must be non-zero");

        auto old = venusVrfLsuPriorityVectors.begin();
        while (old != venusVrfLsuPriorityVectors.end() &&
               old->first < curTick()) {
            old = venusVrfLsuPriorityVectors.erase(old);
        }
        const uint64_t validMask = venusVrfDataBanks == 64
            ? ~uint64_t(0)
            : (uint64_t(1) << venusVrfDataBanks) - 1;
        bank_mask &= validMask;
        for (unsigned grant = 0; grant < grants; ++grant) {
            auto &tagged = venusVrfLsuPriorityVectors[
                first_tick + grant * stride];
            auto [it, inserted] = tagged.emplace(requester_tag, bank_mask);
            panic_if(!inserted && it->second != bank_mask,
                     "Venus VRF LSU tag %#llx changed bank vector",
                     static_cast<unsigned long long>(requester_tag));
        }

        DPRINTF(NoncoherentXBar,
                "Venus VRF LSU priority tag %#llx banks %#llx first %llu "
                "grants %u stride %llu\n",
                static_cast<unsigned long long>(requester_tag),
                static_cast<unsigned long long>(bank_mask),
                static_cast<unsigned long long>(first_tick), grants,
                static_cast<unsigned long long>(stride));
    }

    void
    NoncoherentXBar::arbitrateVenusVrfRequests()
    {
        bool pending = false;
        const auto lsuVectors = venusVrfLsuPriorityVectors.find(curTick());

        for (unsigned bank = 0; bank < venusVrfArbitratedBanks; ++bank) {
            const bool dataBank = bank < venusVrfDataBanks;
            bool lsuPriority = false;
            if (dataBank && lsuVectors != venusVrfLsuPriorityVectors.end()) {
                for (const auto &tagged : lsuVectors->second)
                    lsuPriority |=
                        (tagged.second & (uint64_t(1) << bank)) != 0;
            }
            bool hasLiveIntent = false;
            for (const auto &intent : venusVrfLiveIntents) {
                hasLiveIntent |= intent.second.first ==
                    static_cast<PortID>(bank);
            }
            if (venusVrfPending[bank].empty() &&
                venusVrfSelected[bank] == InvalidPortID &&
                !hasLiveIntent)
                continue;
            if (dataBank && lsuPriority) {
                pending = true;
                DPRINTF(NoncoherentXBar,
                        "Venus VRF bank %u blocked by LSU priority\n",
                        bank);
                continue;
            }
            if (venusVrfLastGrantTick[bank] == curTick()) {
                pending = true;
                continue;
            }

            PortID winner = venusVrfSelected[bank];
            if (winner == InvalidPortID) {
                std::vector<bool> requests(dataBank ? 12 : 2, false);
                for (const PortID source : venusVrfPending[bank]) {
                    const int master = venusVrfMaster(bank, source);
                    panic_if(master < 0,
                             "Venus VRF data bank %u got undecodable source "
                             "port %d", bank, source);
                    requests[master] = true;
                }
                for (const auto &intent : venusVrfLiveIntents) {
                    if (intent.second.first != static_cast<PortID>(bank))
                        continue;
                    const int master = venusVrfMaster(bank, intent.first);
                    panic_if(master < 0,
                             "Venus VRF live source %d is undecodable",
                             intent.first);
                    requests[master] = true;
                }

                unsigned requestMask = 0;
                for (unsigned master = 0; master < requests.size();
                     ++master) {
                    if (requests[master])
                        requestMask |= 1U << master;
                }
                DPRINTF(NoncoherentXBar,
                        "Venus VRF stable vector bank %u rr %u "
                        "requests %#05x lsu %d\n",
                        bank, venusVrfRr[bank], requestMask, lsuPriority);

                const int winnerMaster =
                    selectVenusVrfMaster(requests, venusVrfRr[bank]);
                panic_if(winnerMaster < 0,
                         "Venus VRF bank %u has pending sources but no "
                         "master", bank);
                for (const PortID source : venusVrfPending[bank]) {
                    if (venusVrfMaster(bank, source) == winnerMaster) {
                        winner = source;
                        break;
                    }
                }
                if (winner == InvalidPortID) {
                    for (const auto &intent : venusVrfLiveIntents) {
                        if (intent.second.first == static_cast<PortID>(bank) &&
                            venusVrfMaster(bank, intent.first) == winnerMaster) {
                            winner = intent.first;
                            break;
                        }
                    }
                }
                panic_if(winner == InvalidPortID,
                         "Venus VRF bank %u failed to resolve master %d",
                         bank, winnerMaster);
                venusVrfSelected[bank] = winner;
                venusVrfSelectedNextRr[bank] =
                    nextVenusVrfRr(requests, venusVrfRr[bank]);
            }

            auto live = venusVrfLiveIntents.find(winner);
            const bool liveWinner = live != venusVrfLiveIntents.end() &&
                live->second.first == static_cast<PortID>(bank);
            if (liveWinner) {
                PacketPtr pkt = live->second.second;
                if (recvTimingReq(pkt, winner)) {
                    auto *source = dynamic_cast<VenusVrfLiveIntentSource *>(
                        &cpuSidePorts[winner]->getPeer());
                    panic_if(source == nullptr,
                             "Venus VRF live winner %d has no source callback",
                             winner);
                    source->grantVenusVrfLiveIntent(pkt);
                }
            } else {
                cpuSidePorts[winner]->sendRetryReq();
            }
            /* recvTimingReq clears selected only after the packet really
             * crosses the bank. A downstream rejection retains the exact
             * tagged owner instead of running RR again on a transient
             * retry. */
            pending |= venusVrfSelected[bank] != InvalidPortID ||
                       !venusVrfPending[bank].empty();
        }

        pending |= !venusVrfLiveIntents.empty();

        if (pending && !venusVrfArbitrationEvent.scheduled())
            schedule(venusVrfArbitrationEvent, clockEdge(Cycles(1)));
    }

    NoncoherentXBar::~NoncoherentXBar()
    {
        for (auto l : reqLayers)
            delete l;
        for (auto l : respLayers)
            delete l;
    }

    bool
    NoncoherentXBar::recvTimingReq(PacketPtr pkt, PortID cpu_side_port_id)
    {
        // determine the source port based on the id
        ResponsePort *src_port = cpuSidePorts[cpu_side_port_id];

        // we should never see express snoops on a non-coherent crossbar
        assert(!pkt->isExpressSnoop());

        // determine the destination port
        PortID mem_side_port_id = findPort(pkt);

        const bool venusVrfBank =
            usesVenusVrfArbitration(mem_side_port_id);
        const bool venusVrfSelectedRequest = venusVrfBank &&
            venusVrfSelected[mem_side_port_id] == cpu_side_port_id;
        const auto liveIntent = venusVrfLiveIntents.find(cpu_side_port_id);
        const bool venusVrfLiveRequest = venusVrfBank &&
            liveIntent != venusVrfLiveIntents.end() &&
            liveIntent->second.first == mem_side_port_id &&
            liveIntent->second.second == pkt;
        if (venusVrfBank) {
            bool lsuPriority = false;
            const auto lsuVectors =
                venusVrfLsuPriorityVectors.find(curTick());
            if (static_cast<unsigned>(mem_side_port_id) <
                    venusVrfDataBanks &&
                lsuVectors != venusVrfLsuPriorityVectors.end()) {
                for (const auto &tagged : lsuVectors->second) {
                    lsuPriority |= (tagged.second &
                        (uint64_t(1) << mem_side_port_id)) != 0;
                }
            }
            if (lsuPriority) {
                venusVrfPending[mem_side_port_id].insert(cpu_side_port_id);
                DPRINTF(NoncoherentXBar,
                        "Venus VRF request pending src %d bank %d "
                        "behind LSU priority\n",
                        cpu_side_port_id, mem_side_port_id);
                scheduleVenusVrfArbitration();
                return false;
            }
            if (venusVrfSelected[mem_side_port_id] != cpu_side_port_id) {
                const int master = venusVrfMaster(
                    mem_side_port_id, cpu_side_port_id);
                panic_if(master < 0,
                         "Venus VRF data bank %d got undecodable source "
                         "port %d", mem_side_port_id, cpu_side_port_id);
                venusVrfPending[mem_side_port_id].insert(cpu_side_port_id);
                DPRINTF(NoncoherentXBar,
                        "Venus VRF request pending src %d bank %d "
                        "master %d\n",
                        cpu_side_port_id, mem_side_port_id, master);
                scheduleVenusVrfArbitration();
                return false;
            }
        }

        // test if the layer should be considered occupied for the current
        // port
        if (!reqLayers[mem_side_port_id]->tryTiming(src_port))
        {
            DPRINTF(NoncoherentXBar, "Collision happens and src port is %d destination bank is %d\n", cpu_side_port_id, mem_side_port_id);
            DPRINTF(NoncoherentXBar, "recvTimingReq: src %s %s 0x%x BUSY\n",
                    src_port->name(), pkt->cmdString(), pkt->getAddr());
            return false;
        }

        DPRINTF(NoncoherentXBar, "recvTimingReq: src %s %s 0x%x\n",
                src_port->name(), pkt->cmdString(), pkt->getAddr());

        // store size and command as they might be modified when
        // forwarding the packet
        unsigned int pkt_size = pkt->hasData() ? pkt->getSize() : 0;
        unsigned int pkt_cmd = pkt->cmdToIndex();

        // store the old header delay so we can restore it if needed
        Tick old_header_delay = pkt->headerDelay;

        // a request sees the frontend and forward latency
        Tick xbar_delay = (frontendLatency + forwardLatency) * clockPeriod();

        // set the packet header and payload delay
        calcPacketTiming(pkt, xbar_delay);

        // determine how long to be crossbar layer is busy
        // Tick packetFinishTime = clockEdge(Cycles(1)) + pkt->payloadDelay;
        Tick packetFinishTime = clockEdge(Cycles(1));
        // before forwarding the packet (and possibly altering it),
        // remember if we are expecting a response
        const bool expect_response = pkt->needsResponse() &&
                                     !pkt->cacheResponding();

        // since it is a normal request, attempt to send the packet
        bool success = memSidePorts[mem_side_port_id]->sendTimingReq(pkt);

        if (!success)
        {
            DPRINTF(NoncoherentXBar, "recvTimingReq: src %s %s 0x%x RETRY\n",
                    src_port->name(), pkt->cmdString(), pkt->getAddr());

            // restore the header delay as it is additive
            pkt->headerDelay = old_header_delay;

            // occupy until the header is sent
            reqLayers[mem_side_port_id]->failedTiming(src_port,
                                                      clockEdge(Cycles(1)));

            return false;
        }

        if (venusVrfSelectedRequest) {
            const unsigned bank = mem_side_port_id;
            const unsigned oldRr = venusVrfRr[bank];
            const int winnerMaster = venusVrfMaster(
                mem_side_port_id, cpu_side_port_id);
            unsigned contenders = 0;
            const bool dataBank = bank < venusVrfDataBanks;
            std::vector<bool> requests(dataBank ? 12 : 2, false);
            for (const PortID source : venusVrfPending[bank]) {
                const int master = venusVrfMaster(bank, source);
                if (master >= 0)
                    requests[master] = true;
            }
            for (const auto &intent : venusVrfLiveIntents) {
                if (intent.second.first != static_cast<PortID>(bank))
                    continue;
                const int master = venusVrfMaster(bank, intent.first);
                if (master >= 0)
                    requests[master] = true;
            }
            for (const bool request : requests)
                contenders += request;

            /* FairArb latches the mask/LZC next_idx selected from the same
             * stable request vector as the tree winner.  It is intentionally
             * not necessarily the master that gen_levels granted. */
            venusVrfRr[bank] = venusVrfSelectedNextRr[bank];
            venusVrfPending[bank].erase(cpu_side_port_id);
            venusVrfSelected[bank] = InvalidPortID;
            if (venusVrfLiveRequest)
                venusVrfLiveIntents.erase(cpu_side_port_id);
            venusVrfLastGrantTick[bank] = curTick();
            DPRINTF(NoncoherentXBar,
                    "Venus VRF RR grant bank %u old_rr %u "
                    "winner_master %d source %d next_rr %u contenders %u\n",
                    bank, oldRr, winnerMaster, cpu_side_port_id,
                    venusVrfRr[bank], contenders);
            if (!venusVrfPending[bank].empty())
                scheduleVenusVrfArbitration();
        }

        // remember where to route the response to
        if (expect_response)
        {
            assert(routeTo.find(pkt->req) == routeTo.end());
            routeTo[pkt->req] = cpu_side_port_id;
        }

        reqLayers[mem_side_port_id]->succeededTiming(packetFinishTime);

        // stats updates
        pktCount[cpu_side_port_id][mem_side_port_id]++;
        pktSize[cpu_side_port_id][mem_side_port_id] += pkt_size;
        transDist[pkt_cmd]++;

        return true;
    }

    bool
    NoncoherentXBar::recvTimingResp(PacketPtr pkt, PortID mem_side_port_id)
    {
        // determine the source port based on the id
        RequestPort *src_port = memSidePorts[mem_side_port_id];

        // determine the destination
        const auto route_lookup = routeTo.find(pkt->req);
        assert(route_lookup != routeTo.end());
        const PortID cpu_side_port_id = route_lookup->second;
        assert(cpu_side_port_id != InvalidPortID);
        assert(cpu_side_port_id < respLayers.size());

        // test if the layer should be considered occupied for the current
        // port
        if (!respLayers[cpu_side_port_id]->tryTiming(src_port))
        {
            DPRINTF(NoncoherentXBar, "recvTimingResp: src %s %s 0x%x BUSY\n",
                    src_port->name(), pkt->cmdString(), pkt->getAddr());
            return false;
        }

        DPRINTF(NoncoherentXBar, "recvTimingResp: src %s %s 0x%x\n",
                src_port->name(), pkt->cmdString(), pkt->getAddr());

        // store size and command as they might be modified when
        // forwarding the packet
        unsigned int pkt_size = pkt->hasData() ? pkt->getSize() : 0;
        unsigned int pkt_cmd = pkt->cmdToIndex();

        // a response sees the response latency
        Tick xbar_delay = responseLatency * clockPeriod();

        // set the packet header and payload delay
        calcPacketTiming(pkt, xbar_delay);

        // determine how long to be crossbar layer is busy
        // Tick packetFinishTime = clockEdge(Cycles(1)) + pkt->payloadDelay;
        Tick packetFinishTime = curTick() + 1;
        pkt->headerDelay = 0;
        if (bufferResponses) {
            // A crossbar feeding another timing crossbar can encounter a
            // busy downstream response layer.  Preserve the response in the
            // port's existing queue until that peer sends a retry, while
            // retaining same-edge visibility when the peer is free.
            static_cast<QueuedResponsePort*>(
                cpuSidePorts[cpu_side_port_id])->sendTimingRespOrQueue(pkt);
        } else {
            // Directly connected RTL datapaths retain their same-edge
            // response visibility.
            cpuSidePorts[cpu_side_port_id]->sendTimingResp(pkt);
        }
        // remove the request from the routing table
        routeTo.erase(route_lookup);

        respLayers[cpu_side_port_id]->succeededTiming(packetFinishTime);

        // stats updates
        pktCount[cpu_side_port_id][mem_side_port_id]++;
        pktSize[cpu_side_port_id][mem_side_port_id] += pkt_size;
        transDist[pkt_cmd]++;

        return true;
    }

    void
    NoncoherentXBar::recvReqRetry(PortID mem_side_port_id)
    {
        // responses never block on forwarding them, so the retry will
        // always be coming from a port to which we tried to forward a
        // request
        if (usesVenusVrfArbitration(mem_side_port_id))
            venusVrfDownstreamRetry[mem_side_port_id] = true;
        reqLayers[mem_side_port_id]->recvRetry();
        if (usesVenusVrfArbitration(mem_side_port_id))
            venusVrfDownstreamRetry[mem_side_port_id] = false;
    }

    Tick
    NoncoherentXBar::recvAtomicBackdoor(PacketPtr pkt, PortID cpu_side_port_id,
                                        MemBackdoorPtr *backdoor)
    {
        DPRINTF(NoncoherentXBar, "recvAtomic: packet src %s addr 0x%x cmd %s\n",
                cpuSidePorts[cpu_side_port_id]->name(), pkt->getAddr(),
                pkt->cmdString());

        unsigned int pkt_size = pkt->hasData() ? pkt->getSize() : 0;
        unsigned int pkt_cmd = pkt->cmdToIndex();

        // determine the destination port
        PortID mem_side_port_id = findPort(pkt);

        // stats updates for the request
        pktCount[cpu_side_port_id][mem_side_port_id]++;
        pktSize[cpu_side_port_id][mem_side_port_id] += pkt_size;
        transDist[pkt_cmd]++;

        // forward the request to the appropriate destination
        auto mem_side_port = memSidePorts[mem_side_port_id];
        Tick response_latency = backdoor ? mem_side_port->sendAtomicBackdoor(pkt, *backdoor) : mem_side_port->sendAtomic(pkt);

        // add the response data
        if (pkt->isResponse())
        {
            pkt_size = pkt->hasData() ? pkt->getSize() : 0;
            pkt_cmd = pkt->cmdToIndex();

            // stats updates
            pktCount[cpu_side_port_id][mem_side_port_id]++;
            pktSize[cpu_side_port_id][mem_side_port_id] += pkt_size;
            transDist[pkt_cmd]++;
        }

        // @todo: Not setting first-word time
        pkt->payloadDelay = response_latency;
        return response_latency;
    }

    void
    NoncoherentXBar::recvMemBackdoorReq(const MemBackdoorReq &req,
                                        MemBackdoorPtr &backdoor)
    {
        PortID dest_id = findPort(req.range());
        memSidePorts[dest_id]->sendMemBackdoorReq(req, backdoor);
    }

    void
    NoncoherentXBar::recvFunctional(PacketPtr pkt, PortID cpu_side_port_id)
    {
        if (!pkt->isPrint())
        {
            // don't do DPRINTFs on PrintReq as it clutters up the output
            DPRINTF(NoncoherentXBar,
                    "recvFunctional: packet src %s addr 0x%x cmd %s\n",
                    cpuSidePorts[cpu_side_port_id]->name(), pkt->getAddr(),
                    pkt->cmdString());
        }

        // since our CPU-side ports are queued ports we need to check them as well
        for (const auto &p : cpuSidePorts)
        {
            // if we find a response that has the data, then the
            // downstream caches/memories may be out of date, so simply stop
            // here
            if (p->trySatisfyFunctional(pkt))
            {
                if (pkt->needsResponse())
                    pkt->makeResponse();
                return;
            }
        }

        // determine the destination port
        PortID dest_id = findPort(pkt);

        // forward the request to the appropriate destination
        memSidePorts[dest_id]->sendFunctional(pkt);
    }

} // namespace gem5
