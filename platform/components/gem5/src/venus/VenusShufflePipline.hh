#ifndef __MEM_VENUS_SHUFFLE_HH__
#define __MEM_VENUS_SHUFFLE_HH__

#include "sim/clocked_object.hh"
#include "mem/port.hh"
#include <array>
#include <vector>
#include <deque>
#include <iostream>
#include <iomanip>
#include "params/VenusShufflePipline.hh"
#include "debug/ShuffleEngine.hh"
#include "debug/LaneSequencer.hh"
#include "venus_instr_pkt.hh"
#include "venus/venus_shuffle_producer_completion.hh"
#include "venus/venus_vrf_live_intent.hh"

namespace gem5
{

    enum ShuffleOp {
        SHUFFLE_SCATTER = 0,
        SHUFFLE_GATHER  = 1
    };

    struct ShuffleCommand {
        unsigned vs1_row; 
        unsigned vs2_row; 
        unsigned vd_row;  
        unsigned vl;
        unsigned start_idx;      
        ShuffleOp op;
        unsigned width; //debug
    };

    class VenusShufflePipline : public ClockedObject
    {
    private:
        class VenusShuffleSequencerSidePort : public ResponsePort {
            private:
                VenusShufflePipline *owner;

            protected:
                Tick recvAtomic(PacketPtr pkt) override { panic("recvAtomic unimpl."); }
                void recvFunctional(PacketPtr pkt) override { panic("recvFunctional unimpl."); }
                bool recvTimingReq(PacketPtr pkt) override;
                // bool sendTimingResp(PacketPtr pkt) override;
                void recvRespRetry() override { panic("recvRespRetry unimpl."); }

            public:
            AddrRangeList getAddrRanges() const override { panic("getAddrRanges unimpl."); }

            void reportProducerGrantCompletion(const VenusInstrPkt *pkt)
            {
                auto *sink = dynamic_cast<
                    VenusShuffleProducerCompletionSink *>(&getPeer());
                fatal_if(sink == nullptr,
                         "shuffle sequencer peer has no producer-completion "
                         "sink");
                sink->noteVenusShuffleProducerGrant(
                    pkt->running_id, pkt->vns_instr_id);
            }

            VenusShuffleSequencerSidePort(const std::string &name, VenusShufflePipline *owner) : ResponsePort(name, owner), owner(owner)
            {
            }
        };
        class VenusShuffleHazardTableListenPort : public ResponsePort
        {
            private:
                VenusShufflePipline *owner;

            protected:
                Tick recvAtomic(PacketPtr pkt) override { panic("recvAtomic unimpl."); }
                void recvFunctional(PacketPtr pkt) override { panic("recvFunctional unimpl."); }
                bool recvTimingReq(PacketPtr pkt) override;
                void recvRespRetry() override { panic("recvRespRetry unimpl."); }

            public:
                AddrRangeList getAddrRanges() const override { panic("getAddrRanges unimpl."); }

                VenusShuffleHazardTableListenPort(const std::string &name, VenusShufflePipline *owner) : ResponsePort(name, owner), owner(owner)
                {
                }
        };
        bool ShufflehandleNewInstrRequest(VenusInstrPkt* pkt);
        class BankPort : public RequestPort,
                         public VenusVrfLiveIntentSource {
            private:
                VenusShufflePipline& owner;
                int portId;
            public:
                BankPort(const std::string& _name, VenusShufflePipline& _owner, int _id) :
                    RequestPort(_name, &_owner), owner(_owner), portId(_id) {}
                bool recvTimingResp(PacketPtr pkt) override { return owner.handleResponse(pkt, portId); }
                void recvReqRetry() override { owner.handleRetry(portId); }
                int getId() const { return portId; }
                void publishLiveIntent(PacketPtr pkt)
                {
                    auto *sink = dynamic_cast<VenusVrfLiveIntentSink *>(
                        &getPeer());
                    fatal_if(sink == nullptr,
                             "shuffle lane %d peer has no live-intent sink",
                             portId);
                    sink->publishVenusVrfLiveIntent(pkt);
                }
                void withdrawLiveIntent(PacketPtr pkt)
                {
                    auto *sink = dynamic_cast<VenusVrfLiveIntentSink *>(
                        &getPeer());
                    fatal_if(sink == nullptr,
                             "shuffle lane %d peer has no live-intent sink",
                             portId);
                    sink->withdrawVenusVrfLiveIntent(pkt);
                }
                void grantVenusVrfLiveIntent(PacketPtr pkt) override
                {
                    owner.handleLiveIntentGrant(portId, pkt);
                }
        };
        
        enum SlotState {
            PE_REQ_INDEX,
            PE_WAIT_INDEX,
            PE_REQ_DATA,
            PE_WAIT_DATA,
            PE_REQ_WRITE,
            PE_WAIT_WRITE,
            PE_DONE
        };

        /*
         * `venus_shuffle_engine.sv` does not let one PE refill index work
         * simply because a software slot became free.  It advances a
         * registered IDX -> DATA -> WRITE phase machine and returns to IDX
         * only after the current data wave has received write grants.
         */
        enum PePhase {
            PE_PHASE_INDEX,
            PE_PHASE_DATA,
            PE_PHASE_WRITE
        };

        struct PESlot {
            unsigned seq;
            SlotState state;
            unsigned elementIdx;
            uint16_t fetchedIndex = 0;
            uint16_t fetchedData = 0;
            /*
             * A shuffle PE first registers its result request.  The lane
             * arbiter only observes that request on the following tile edge;
             * it must not be collapsed into the timing-port acceptance that
             * created the request.
             */
            bool egressRegistered = false;
            int egressLaneId = -1;
            Cycles egressVisibleCycle = Cycles(0);
            PacketPtr blockedPkt = nullptr;
            int blockedLaneId = -1;

            PESlot(unsigned _seq, unsigned _elementIdx)
                : seq(_seq), state(PE_REQ_INDEX), elementIdx(_elementIdx)
            {}
        };

        struct PEInfo {
            int id;
            unsigned nextSeq = 0;
            PePhase phase = PE_PHASE_INDEX;
            unsigned totalElements = 0;
            unsigned requestedIndex = 0;
            unsigned receivedIndex = 0;
            unsigned requestedData = 0;
            unsigned receivedData = 0;
            unsigned completedWrite = 0;
            unsigned outstandingReads = 0;
            unsigned indexWaveRequests = 0;
            unsigned dataWaveRequests = 0;
            /*
             * A returned operand contributes to this cycle's D counters, but
             * it changes IDX/DATA phase only through the matching lane-grant
             * branch.  These edge-local markers keep an ungranted req_q from
             * being promoted by a response alone.
             */
            bool indexGrantThisCycle = false;
            bool dataGrantThisCycle = false;
            std::deque<PESlot> slots;
            PEInfo() : id(0) {}
        };

        struct ShuffleSenderState : public Packet::SenderState {
            int peId;
            unsigned seq;
            ShuffleSenderState(int _id, unsigned _seq)
                : peId(_id), seq(_seq) {}
        };

        /*
         * The RTL does not feed a VRF read response directly back into the
         * shuffle engine.  venus_dspm registers the read-valid/tag and the
         * lane then crosses the response through a non-bypass spill register
         * before the shuffle crossbar.  Keep arrival and visibility separate
         * here so a memory-event ordering cannot create a combinational
         * response path in the model.
         */
        struct PendingResponse {
            PacketPtr pkt;
            Cycles visibleCycle;

            PendingResponse(PacketPtr pkt_, Cycles visible_cycle)
                : pkt(pkt_), visibleCycle(visible_cycle) {}
        };

        VenusShuffleSequencerSidePort port_venusshuffle_receivefrom_venussequencer;
        VenusShuffleHazardTableListenPort port_venusshuffle_hazardtable_listen;
        std::vector<BankPort*> bankPorts;
        std::vector<bool> laneBlocked;
        std::vector<bool> laneRetryReady;
        std::vector<unsigned> laneRoundRobin;
        /* Exact req_q vector which selected the blocked tagged request. */
        std::vector<std::vector<bool>> laneBlockedRequests;
        std::vector<std::deque<PendingResponse>> pendingResponses;
        std::vector<PEInfo> pes;

        std::deque<ShuffleCommand> cmdQueue;
        std::deque<VenusInstrPkt> instrpktQueue;
        ShuffleCommand currentCmd;

        /*
         * Registered command-control states around the pipelined PE datapath
         * in venus_shuffle_engine.sv.
         */
        enum class EngineState {
            Idle,
            Wait,
            Stage3,
            Execute,
            Stage1,
        };

        // unsigned nextElementToAssign;
        bool commandActive;
        bool producerGrantCompletionReported = false;
        EngineState engineState;
        
        unsigned numBanks; // Total banks (16)
        unsigned numPes;   // Total PEs (16)
        std::vector<Addr> bankBaseAddrs;

        // unsigned dataWidth;     debug
        // unsigned numLanes;      // 4
        unsigned banksPerLane;  // 4
        unsigned linesPerBank;
        Addr vrfBaseAddr;
        bool registeredRequestVisibility;
        bool liveRequesterIntent;
        bool legacyLockstep;
        std::array<uint64_t, 11> legacyPhaseCycles{};
        std::array<uint64_t, 11> legacyPhaseGrants{};
        std::array<bool, 3> legacyPhaseColdFilled{};
        bool legacyPhaseColdFillPending = false;
        PePhase legacyPhaseColdFillPhase = PE_PHASE_INDEX;
        Cycles legacyPhaseColdFillVisibleCycle = Cycles(0);
        // std::vector<int> scheduledIndices;
        std::vector<std::deque<int>> peTaskQueues;
        static constexpr unsigned MaxInFlight = 4;
        static constexpr unsigned MaxLaneResponsePipelineDepth = 2;
        static constexpr unsigned LegacyLaneResponsePipelineDepth = 4;

        EventFunctionWrapper tickEvent;

        void processFSM();
        void updateQueuedShuffleHazards();
        bool frontShuffleHazardsReady() const;
        std::string reportFrontShuffleHazards() const;
        bool handleResponse(PacketPtr pkt, int portId);
        void makeResponseVisible(PacketPtr pkt, int portId);
        void requestAccepted(PEInfo &pe, PESlot &slot,
            SlotState requestState);
        void advanceLegacyLockstepPhase();
        bool legacyPhaseColdFillReady(PePhase phase);
        unsigned legacyRtlPhase() const;
        void noteLegacyPhaseCycle();
        void reportLegacyPhaseSummary() const;
        void maybeReportProducerGrantCompletion();
        void handleRetry(int portId);
        void handleLiveIntentGrant(int portId, PacketPtr pkt);
        
        struct BankLoc {
            int laneIdx;     // New: Lane ID
            int localBankIdx;// New: Bank ID within Lane
            int globalBankIdx;
            Addr addr;
        };
        BankLoc getBankLoc(unsigned row, unsigned idx, unsigned width);

        VenusHazardTable *venus_hazard_table = nullptr;

        bool listenHazardTableRequest(VenusHazardTable *pkt);
    public:
        VenusShufflePipline(const VenusShufflePiplineParams &p);
        Port &getPort(const std::string &if_name, PortID idx=InvalidPortID) override;
        void init() override;
        /** Reset the control/arbitration state covered by tile_soft_reset_n. */
        void resetRtlTaskState();

        bool shuffle_halt = false;
    };

}

#endif
