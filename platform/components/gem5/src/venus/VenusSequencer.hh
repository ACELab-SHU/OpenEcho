#ifndef __SIM_VENUS_SEQUENCER_HH__
#define __SIM_VENUS_SEQUENCER_HH__

#include "venus_extension_pkg.hh"

#include "debug/VenusSequencer.hh"
#include "debug/VenusSequencerFull.hh"
#include "debug/VenusScalarDispatch.hh"
#include "params/VenusSequencer.hh" // 包含自动生成的Params类
#include "sim/clocked_object.hh"
#include "base/trace.hh"
#include "mem/noncoherent_xbar.hh"
#include "mem/port.hh"
#include "mem/venus_vrf_mem.hh"
#include "sim/system.hh"
#include "venus/VenusSharedL2.hh"
#include "venus/VenusShufflePipline.hh"
#include "venus/venus_lane_producer_completion.hh"
#include "venus/venus_shuffle_producer_completion.hh"
#include "venus_instr_pkt.hh"

#include <array>
#include <cmath>
#include <cstdlib>
#include <deque>
#include <string>
#include <unordered_set>
#include <vector>

// 基础宏：定义单个端口初始化项，使用 # 将参数 name 字符串化
#define PORT_INIT_ITEM(name, index) {#name "[" #index "]", this}

// 递归展开宏：通过令牌连接 ## 和递归调用生成指定数量的初始化项
#define PORT_INIT_1(name, index) PORT_INIT_ITEM(name, index)
#define PORT_INIT_2(name, index) PORT_INIT_1(name, index), PORT_INIT_1(name, index+1)
#define PORT_INIT_4(name, index) PORT_INIT_2(name, index), PORT_INIT_2(name, index+2)
#define PORT_INIT_8(name, index) PORT_INIT_4(name, index), PORT_INIT_4(name, index+4)
#define PORT_INIT_16(name, index) PORT_INIT_8(name, index), PORT_INIT_8(name, index+8)
#define PORT_INIT_32(name, index) PORT_INIT_16(name, index), PORT_INIT_16(name, index+16)
#define PORT_INIT_64(name, index) PORT_INIT_32(name, index), PORT_INIT_32(name, index+32)
#define PORT_INIT_128(name, index) PORT_INIT_64(name, index), PORT_INIT_64(name, index+64)
#define PORT_INIT_256(name, index) PORT_INIT_128(name, index), PORT_INIT_128(name, index+128)
#define LANE_PORT_INIT_1(name, index) {#name "[" #index "]", this, index}
#define LANE_PORT_INIT_2(name, index) LANE_PORT_INIT_1(name, index), LANE_PORT_INIT_1(name, index+1)
#define LANE_PORT_INIT_4(name, index) LANE_PORT_INIT_2(name, index), LANE_PORT_INIT_2(name, index+2)
#define LANE_PORT_INIT_8(name, index) LANE_PORT_INIT_4(name, index), LANE_PORT_INIT_4(name, index+4)
#define LANE_PORT_INIT_16(name, index) LANE_PORT_INIT_8(name, index), LANE_PORT_INIT_8(name, index+8)
#define LANE_PORT_INIT_32(name, index) LANE_PORT_INIT_16(name, index), LANE_PORT_INIT_16(name, index+16)
#define LANE_PORT_INIT_64(name, index) LANE_PORT_INIT_32(name, index), LANE_PORT_INIT_32(name, index+32)

namespace gem5
{
class ThreadContext;
class VenusSequencer : public ClockedObject
{
  public:
    class VenusSequencerRVSidePort : public ResponsePort
    {
      private:
          VenusSequencer *owner;
      protected:
          Tick recvAtomic(PacketPtr pkt) override { panic("recvAtomic unimpl."); }
          void recvFunctional(PacketPtr pkt) override { panic("recvFunctional unimpl."); }
          bool recvTimingReq(PacketPtr pkt) override;
          void recvRespRetry() override { panic("recvRespRetry unimpl."); }
      public:
          AddrRangeList getAddrRanges() const override { panic("getAddrRanges unimpl."); }

          VenusSequencerRVSidePort(const std::string& name, VenusSequencer *owner) :
              ResponsePort(name, owner), owner(owner)
          { }
    };
    class VenusSequencerVenusLaneSidePort :
        public RequestPort, public VenusLaneProducerCompletionSink
    {
      private:
          VenusSequencer *owner;
          int laneId;
          PacketPtr blockedPacket = nullptr;
      protected:
          bool recvTimingResp(PacketPtr pkt) override;
          void recvRangeChange() override { panic("recvRangeChange unimpl."); }
      public:
          bool isBlocked = false;
          void sendPacket(PacketPtr pkt);
          void recvReqRetry() override;
          void noteVenusLaneProducerGrant(
              int laneId, int runningId, int instructionId) override;
          VenusSequencerVenusLaneSidePort(const std::string& name,
                                          VenusSequencer *owner,
                                          int lane_id) :
              RequestPort(name, owner), owner(owner), laneId(lane_id)
          { }
    };
    class VenusSequencerVenusShuffleSidePort :
        public RequestPort, public VenusShuffleProducerCompletionSink
    {
      private:
          VenusSequencer *owner;
          PacketPtr blockedPacket = nullptr;
      protected:
          bool recvTimingResp(PacketPtr pkt) override;
          void recvRangeChange() override { panic("recvRangeChange unimpl."); }
      public:
          bool isBlocked = false;
          void sendPacket(PacketPtr pkt);
          void recvReqRetry() override;
          void noteVenusShuffleProducerGrant(
              int runningId, int instructionId) override;
          VenusSequencerVenusShuffleSidePort(const std::string& name, VenusSequencer *owner) :
              RequestPort(name, owner), owner(owner)
          { }
    };
    class VenusSequencerHazardTableBroadcastPort : public RequestPort
    {
      private:
          VenusSequencer *owner;
      protected:
          bool recvTimingResp(PacketPtr pkt) override { panic("recvTimingResp unimpl."); }
          void recvRangeChange() override { panic("recvRangeChange unimpl."); }
      public:
          void sendPacket(PacketPtr pkt);
          void recvReqRetry() override { panic("recvReqRetry unimpl."); }
          VenusSequencerHazardTableBroadcastPort(const std::string& name, VenusSequencer *owner) :
              RequestPort(name, owner), owner(owner)
          { }
    };
    Port &getPort(const std::string &if_name, PortID idx=InvalidPortID) override;

  private:
    VenusInstrPkt*decodeVenusInstruction(uint64_t inst_bits, uint32_t scalar_op, uint32_t avl, uint32_t mulshamt, uint32_t msbhead, uint32_t mulsaturate, uint32_t lsu_addr_msb);

    // 关键仿真组件
    EventFunctionWrapper nextDispatchScalarReqEvent;
    EventFunctionWrapper nextRecvNewInstrEvent;
    EventFunctionWrapper nextTryIssueInstrEvent;
    EventFunctionWrapper nextSequencerIssueStateEvent;
    EventFunctionWrapper nextLsuDoneEvent;
    EventFunctionWrapper nextLduRegisterEvent;
    EventFunctionWrapper nextLduResultVisibleEvent;
    EventFunctionWrapper nextLduResultGrantEvent;
    EventFunctionWrapper nextLaneDoneReportEvent;
    EventFunctionWrapper nextLaneGrantCompletionEvent;
    EventFunctionWrapper nextProducerCompletionEvent;
    EventFunctionWrapper nextShuffleGrantCompletionEvent;
    EventFunctionWrapper nextBroadcastHazardTablePipe1Event;
    EventFunctionWrapper nextBroadcastHazardTablePipe2Event;
    EventFunctionWrapper nextBroadcastHazardTablePipe3Event;
    /*
     * Venus timing is expressed in tile-clock cycles.  Convert it through
     * this object's configured clock domain instead of assuming 1 ns/tick.
     * Adding to curTick() intentionally preserves the historical scheduling
     * phase when the callback was not itself delivered on a clock edge.
     */
    Tick afterCycles(Cycles cycles) const
    {
        return curTick() + cyclesToTicks(cycles);
    }
    VenusSequencerRVSidePort                 port_venussequencer_receivefrom_venuspacketgen;
    VenusSequencerVenusLaneSidePort          port_venussequencer_sendto_venuslane[MaxNrLanes];
    VenusSequencerVenusShuffleSidePort       port_venussequencer_sendto_venusshuffle;
    VenusSequencerHazardTableBroadcastPort   port_venussequencer_hazardtable_boardcast[MaxNrLanes];
    VenusSequencerHazardTableBroadcastPort   port_venussequencer_hazardtable_boardcast_to_shuffle;
    // 内部私有变量
    /*
     * venus_dispatcher.sv's non-bypass spill_register has A/B storage.
     * Scalar retirement follows admission here; sequencer admission is
     * retried independently from the FIFO head.
     */
    std::deque<VenusInstrPkt *> scalarDispatchQueue;
    /*
     * A CPU-originated Venus request is reconstructed from two scalar600
     * retire words.  The decoded packet can be built when the first word
     * retires, but it is not visible to venus_dispatcher until the second
     * word crosses the following registered edge.  Packet-generator tests
     * already present a complete VenusInstrPkt and therefore use the
     * ordinary one-cycle dispatcher boundary.
     */
    std::deque<Tick> scalarDispatchReadyTicks;
    struct PendingLaneDoneReport
    {
        VenusInstrPkt *pkt = nullptr;
        Tick visibleTick = MaxTick;
        int laneId = -1;
    };
    /*
     * Every lane pe_resp crosses its own registered visibility boundary.
     * Keep that boundary on the report itself: a younger report arriving
     * while an older batch's event is executing must not hitchhike through
     * the older event one cycle early.
     */
    std::deque<PendingLaneDoneReport> pendingLaneDoneReports;
    struct PendingLaneGrantCompletion
    {
        int runningId = -1;
        int instructionId = -1;
        Tick visibleTick = MaxTick;
    };
    /*
     * The all-lane final-grant reduction is a D value.  Its tagged command
     * completion is broadcast from Q on the next tile edge; each lane's
     * requester then captures that clear into requester_q.
     */
    std::deque<PendingLaneGrantCompletion> pendingLaneGrantCompletions;
    struct PendingProducerCompletion
    {
        int runningId = -1;
        int instructionId = -1;
        Tick visibleTick = MaxTick;
    };
    /*
     * Venus1 routes a PE completion through pe_resp_i_q before the main
     * sequencer can clear global_hazard_table_q.  Keep the tagged producer
     * notification pending across that backend-specific registered edge;
     * Venus2 consumes pe_resp_i directly and never enters this queue.
     */
    std::deque<PendingProducerCompletion> pendingProducerCompletions;
    struct PendingShuffleGrantCompletion
    {
        int runningId = -1;
        int instructionId = -1;
        Tick visibleTick = MaxTick;
    };
    /*
     * venus_shuffle_engine registers parallel_shuffle_complete_d before its
     * global-hazard update is visible to operand requesters.  Preserve the
     * exact producer generation while that D/Q boundary is in flight so a
     * recycled running ID cannot satisfy an older completion token.
     */
    std::deque<PendingShuffleGrantCompletion>
        pendingShuffleGrantCompletions;
    bool experimentalRequesterQVisibility = false;
    bool rtlRegisteredPeResponse = false;
    bool rtlScalarSecondWordBoundary = false;
    /*
     * Input-side capacity is a backend property.  The sequencer's active
     * request already represents dispatcher.venus_req_o; this depth counts
     * only requests still resident in dispatcher.i_spill_register.
     */
    unsigned scalarDispatchDepth = 2;
    bool rtlDirectDownstreamReturn = false;
    bool rtlLaneDesyncStallEnabled = false;
    bool rtlAllLaneIssueHandshake = false;
    bool rtlDispatcherInclusiveTail = false;
    std::array<std::array<bool, MaxNrLanes>, NrIDs> rtlLaneDone{};
    VenusInstrPkt* recved_venus_instr_pkt = nullptr;
    VenusInstrPkt* recved_finished_instr_pkt = nullptr;
    VenusInstrPkt* vinsn_running_pkt[NrIDs];
    int  vinsn_running_pkt_lanedonecounter[NrIDs];
    int laneProducerGrantGeneration[NrIDs] = {};
    int laneProducerCompletionQueuedGeneration[NrIDs] = {};
    std::vector<bool> laneProducerGrantBoard[NrIDs];
    /*
     * Physical PE participation is not the same as useful data work.  The
     * V1 sequencer broadcasts ordinary requests to every lane; tail lanes
     * compute local VL zero only after that handshake and never produce a
     * data/writeback grant.
     */
    std::vector<bool> vinsn_running_pkt_laneactiveboard[NrIDs];
    std::vector<bool> vinsn_running_pkt_laneuseboard[NrIDs];
    bool vinsn_running_pkt_shufflefiresuccessboard[NrIDs];
    std::vector<bool> vinsn_running_pkt_lanefiresuccessboard[NrIDs];
    bool vinsn_is_running[NrIDs];
    bool vinsn_can_fire_now[NrIDs];
    /*
     * Registered state corresponding to venus_sequencer.sv's PE running-ID
     * table and its OR-reduced vinsn_running_q output.  The packet-pointer
     * table above is an implementation/admission structure and changes at
     * asynchronous gem5 callback boundaries; it is not the scalar-visible
     * RTL signal.
     */
    bool rtlPeRunningQ[NrIDs] = {};
    bool rtlRunningQ[NrIDs] = {};
    bool rtlIssuePending[NrIDs] = {};
    bool rtlCompletionPending[NrIDs] = {};
    /*
     * The scalar CPU and Venus sequencer are clocked on the same tile edge.
     * gem5 may dispatch a combinational PE done callback after the CPU event
     * at that tick.  Remember whether scalar600 has already sampled the old
     * running-Q state so the late callback can still apply the sequencer's
     * non-blocking Q update for that same physical edge.
     */
    Tick rtlScalarBarrierLastAdvanceTick = MaxTick;
    enum class SequencerIssueState
    {
        UpstreamReceive,
        Pipe0,
        Pipe1,
        Pipe2,
        DownstreamReceive,
        DownstreamAck,
        ReturnUpstream
    };
    SequencerIssueState sequencerIssueState =
        SequencerIssueState::UpstreamReceive;
    bool sequencerWaitedForVfuReady = false;
    /*
     * A non-LSU PE transaction may remain in DOWNSTREAM_RECEIVE while a
     * lane's one-entry fall-through register is full.  Remember that this
     * was a real bus-ready wait so the registered return edge is not charged
     * again after the final lane captures the held request.
     */
    bool sequencerWaitedForPeReady = false;
    /* Stable target vector for the current RTL downstream transaction. */
    bool sequencerIssueTargetVfus[NrVFUs] = {};
    int vfu_queue_counter[NrVFUs];
    /* Venus1's queue-capacity counters consume lane-0 VFU-done pulses,
     * independently of the later all-lane running-ID retirement. */
    bool rtlVfuQueueReleased[NrIDs][NrVFUs] = {};
    std::vector<uint16_t> sharedMemory;
    std::vector<uint8_t> sharedL2;
    VenusSharedL2 *sharedL2Object = nullptr;
    enum class LsuPhase
    {
        Request,
        StoreOperands,
        StoreDataCommit,
        MemoryResponse,
        ResultQueueVisible,
        ResultFinalGrant,
        PeResponse,
        Complete
    };
    struct PendingLsuInstr
    {
        Tick eventTick;
        VenusInstrPkt *pkt;
        LsuPhase phase;
        int beats;
        int rows;
        Tick minimumResponseTick;
        bool hazardAtAdmission;
        bool storeDataCommitted;
        bool addrgenQueuedBehindSameDirection = false;
        std::vector<uint16_t> loadValues;
        std::vector<uint8_t> loadValid;
        int responseBeat = 0;
        int responseBeatBytes = 0;
        int responseBytes = 0;
        int resultRowBytes = 0;
        Tick responseBeatStride = 0;
        bool responseBackpressured = false;
        /*
         * Physical AXI edge on which the STU address descriptor first
         * enters the outbound CDC.  W may reach the fabric before the
         * registered AW-to-W route is visible, in which case DW_axi buffers
         * and masks the first beat for one AXI clock.
         */
        Tick addressAxiSampleTick = 0;
        /*
         * vstu stores hazard_vs2 in its four-entry instruction queue at
         * acceptance, then destructively masks that captured value with the
         * live global hazard row until every producer clears.  Rechecking
         * only the current hazard table loses this queue state when an older
         * producer retires between store acceptance and operand issue.
         *
         * Keep the producer generation beside each physical ID.  gem5 can
         * recycle an ID before all timing callbacks for the old generation
         * have drained; the tag prevents a younger reuse from satisfying or
         * extending the queued store's original dependency.
         */
        uint8_t storeHazardVs2 = 0;
        std::array<int, NrIDs> storeHazardVs2Generation{{
            -1, -1, -1, -1, -1, -1, -1, -1}};
    };
    std::deque<PendingLsuInstr> pendingLsuInstrs;
    struct LduIssueSlot
    {
        int runningId = 0;
        uint8_t hazardVd1 = 0;
        /*
         * A raw running-ID bit is not a stable dependency tag: the five-ID
         * pool may recycle an ID while an older VLDu slot is still resident.
         * Retain the producer generation captured with each hazard bit so a
         * completion tombstone can clear exactly that producer without
         * aliasing a younger reuse of the same ID.
         */
        std::array<int, NrIDs> hazardGeneration{{-1, -1, -1, -1, -1}};
    };
    struct LduResultRow
    {
        Tick visibleTick = 0;
        VenusInstrPkt *pkt = nullptr;
        int row = 0;
        int rows = 0;
        int beats = 0;
        bool final = false;
        uint64_t vrfBankMask = 0;
        std::vector<uint16_t> loadValues;
        std::vector<uint8_t> loadValid;
    };
    std::array<LduIssueSlot, 4> lduIssueSlotsQ{};
    std::array<LduIssueSlot, 4> lduIssueSlotsQQ{};
    unsigned lduAcceptPnt = 0;
    unsigned lduIssuePntD = 0;
    unsigned lduIssuePntQ = 0;
    unsigned lduIssuePntQQ = 0;
    unsigned lduActiveLoads = 0;
    /* Requester-private completion generations; never broadcast to lanes. */
    std::array<int, NrIDs> lduRequesterRetiredGeneration{
        {-1, -1, -1, -1, -1}};
    std::deque<int> lduResponseOrder;
    std::deque<LduResultRow> lduRowsWaitingVisibility;
    std::deque<LduResultRow> lduResultQueue;
    struct LsuRetiringSlot
    {
        VenusOp op;
        Tick releaseTick;
    };
    std::deque<LsuRetiringSlot> lsuRetiringSlots;
    Tick lsuChannelAvailableTick = 0;
    // Opposite-direction addrgen selection has a longer registered release
    // than same-direction descriptor/data continuation.
    Tick lsuOppositeDirectionAvailableTick = 0;
    /*
     * When W-CDC backpressure holds WLAST until a source slot returns, that
     * completing edge may pop addrgen's direction FIFO and accept the
     * opposite descriptor even though the LDU data path is not yet usable.
     * Keep that simultaneous pop/push boundary separate from the ordinary
     * registered direction release above.
     */
    Tick lsuAddrgenDirectionPopTick = 0;
    bool lsuAddrgenDirectionPopValid = false;
    /* Descriptor admitted by the simultaneous WLAST pop/push boundary. */
    int lsuAddrgenAdmittedHeadInstr = -1;
    VenusOp lsuChannelOwnerOp = VLOAD;
    bool lsuChannelOwnerValid = false;
    Tick stuWriteDataTailTick = 0;
    bool stuWriteDataTailValid = false;
    Tick lduAcceptAvailableTick = 0;
    Tick stuAcceptAvailableTick = 0;
    Tick lsuAxiPhaseTick = 0;
    bool lsuAxiPhaseValid = false;
    /*
     * venus_sequencer keeps an LSU request in DOWNSTREAM_RECEIVE until the
     * separately registered addrgen state machine acknowledges it.  This is
     * an issue-handshake boundary, not part of the later AXI/response
     * latency.
     */
    Tick lsuAddrgenAckVisibleTick = 0;
    /*
     * The main sequencer has at most one downstream LSU transaction waiting
     * for addrgen_ack, while the LSU itself may retain several older tagged
     * requests.  Keep the generation here so an older request reaching the
     * shared addrgen channel cannot acknowledge the current PE request.
     */
    int lsuAddrgenAckInstr = -1;
    std::unordered_set<int> lsuAddrgenBlockedInstrs;
    unsigned int released_vins_dump_idx = 0;
    int activeTaskId = -1;
    bool taskExitRequested = false;

    // 内部私有状态
    bool isBusy = false; //sequencer忙状态信息
    bool isFull = false; //sequencer满状态信息
    SimObject *m_venus_vrf_simobject;
    memory::venus_vrf_mem *m_venus_vrf;
    NoncoherentXBar *venusVrfXbar = nullptr;
    VenusShufflePipline *venusShuffle = nullptr;
    bool rtlTaskSoftResetEnabled = false;
    System *system;
    ThreadContext *activeThreadContext = nullptr;
  public:
    // 构造函数，初始化成员变量
    VenusSequencer(const VenusSequencerParams &params) : ClockedObject(params),
        port_venussequencer_receivefrom_venuspacketgen(params.name + ".port_venussequencer_receivefrom_venuspacketgen", this),
        port_venussequencer_sendto_venusshuffle(params.name + ".port_venussequencer_sendto_venusshuffle", this),
        port_venussequencer_hazardtable_boardcast_to_shuffle(params.name + ".port_venussequencer_hazardtable_boardcast_to_shuffle", this),
        port_venussequencer_sendto_venuslane{ LANE_PORT_INIT_64(port_venussequencer_sendto_venuslane, 0) },
        port_venussequencer_hazardtable_boardcast{ PORT_INIT_64(port_venussequencer_hazardtable_boardcast, 0)},
        nextDispatchScalarReqEvent([this]{executeDispatchScalarReq();},name()),
        nextRecvNewInstrEvent([this]{executeRecvNewInstr();},name()),
        nextTryIssueInstrEvent([this]{executeTryIssueInstr();},name()),
        nextSequencerIssueStateEvent(
            [this]{advanceSequencerIssueState();}, name(), false,
            /*
             * The registered state is visible to the combinational upstream
             * ready path before ordinary request callbacks on the new tile
             * edge.  Give this event an explicit phase so equal-tick
             * insertion order cannot change the modeled handshake cadence.
             */
            Event::Default_Pri - 1),
        nextLsuDoneEvent([this]{executeLsuDone();},name()),
        nextLduRegisterEvent(
            [this]{advanceLduRegisters();}, name(), false,
            Event::Default_Pri - 2),
        nextLduResultVisibleEvent(
            [this]{executeLduResultVisible();}, name(), false,
            Event::Default_Pri - 1),
        nextLduResultGrantEvent(
            [this]{executeLduResultGrant();}, name(), false,
            Event::Default_Pri + 1),
        nextLaneDoneReportEvent(
            [this]{executeLaneDoneReports();}, name()),
        nextLaneGrantCompletionEvent(
            [this]{executeLaneGrantCompletions();}, name(), false,
            Event::Default_Pri + 1),
        nextProducerCompletionEvent(
            [this]{executeProducerCompletions();}, name(), false,
            Event::Default_Pri + 1),
        nextShuffleGrantCompletionEvent(
            [this]{executeShuffleGrantCompletions();}, name(), false,
            /*
             * LSU/LDU requester callbacks on this edge sample the old Q
             * state.  Expose the newly registered Shuffle completion only
             * after those callbacks, for use on the following tile edge.
             */
            Event::Default_Pri + 1),
        nextBroadcastHazardTablePipe1Event([this]{hazardTablePipe1();},name()),
        nextBroadcastHazardTablePipe2Event([this]{hazardTablePipe2();},name()),
        nextBroadcastHazardTablePipe3Event([this]{hazardTablePipe3();},name()),
        sharedL2(32 * 1024 * 1024, 0),
        sharedL2Object(dynamic_cast<VenusSharedL2 *>(params.shared_l2_object)),
        experimentalRequesterQVisibility(
            params.experimental_requester_q_visibility),
        rtlRegisteredPeResponse(params.rtl_registered_pe_response),
        rtlScalarSecondWordBoundary(
            params.rtl_scalar_second_word_boundary),
        scalarDispatchDepth(params.rtl_scalar_dispatch_capacity),
        rtlDirectDownstreamReturn(
            params.rtl_direct_downstream_return),
        rtlLaneDesyncStallEnabled(params.rtl_lane_desync_stall),
        rtlAllLaneIssueHandshake(params.rtl_all_lane_issue_handshake),
        rtlDispatcherInclusiveTail(
            params.rtl_dispatcher_inclusive_tail),
        m_venus_vrf_simobject(params.vrf_object),
        venusVrfXbar(dynamic_cast<NoncoherentXBar *>(
            params.vrf_xbar_object)),
        venusShuffle(dynamic_cast<VenusShufflePipline *>(
            params.shuffle_object)),
        rtlTaskSoftResetEnabled(params.rtl_task_soft_reset),
        system(params.system)

    {   
        registerExitCallback([this]() { VenusInstrPkt::saveVinsInfo(); });
        m_venus_vrf = dynamic_cast<memory::venus_vrf_mem*>(m_venus_vrf_simobject);
        // m_venus_vrf->backdoor_Read();
    }


    // 全局障碍表输出
    VenusHazardTable* venus_hazard_table       = nullptr;
    VenusHazardTable* venus_hazard_table_pipe1 = nullptr;
    VenusHazardTable* venus_hazard_table_pipe2 = nullptr;
    VenusHazardTable* venus_hazard_table_pipe3 = nullptr;

    bool handleRequest(VenusInstrPkt* pkt);
    bool handleVinsnDoneReport(VenusInstrPkt* pkt);
    bool queueLaneDoneReport(VenusInstrPkt* pkt, int laneId = -1);
    void noteRtlLaneDone(int laneId, const VenusInstrPkt *pkt);
    void noteRtlLaneVfuDone(int laneId, const VenusInstrPkt *pkt);
    bool rtlLaneDesyncStall() const;
    void noteLaneProducerGrant(
        int laneId, int runningId, int instructionId);
    void executeLaneGrantCompletions();
    void queueProducerCompletionToLanes(
        int runningId, int instructionId);
    void executeProducerCompletions();
    void noteShuffleProducerGrant(int runningId, int instructionId);
    void executeLaneDoneReports();
    void executeShuffleGrantCompletions();
    int findRunningID();
    bool rtlRunningIdReserved(int id) const;
    void assignRunningIDandLaneandQueue(VenusInstrPkt* pkt, int ID);
    std::tuple<VFU, VFU, VFU> findVinsnTargetVFU(VenusInstrPkt* pkt);
    void initSharedMemory();
    void executeLsuInstr(VenusInstrPkt* pkt);
    void captureLsuLoad(PendingLsuInstr &pending);
    void commitLsuLoad(const PendingLsuInstr &pending);
    void traceLsuEvent(const char *event, VenusInstrPkt *pkt,
                       int beats = -1, Addr addr = 0);
    bool lsuQueueHasSpace(VenusInstrPkt *pkt);
    Tick alignLsuAxiEdge(Tick candidate);
    Tick alignLduOutboundAxiEdge(Tick candidate);
    Tick alignLsuTileEdge(Tick candidate) const;
    bool scheduleLsuDone(VenusInstrPkt* pkt);
    void queuePendingLsu(PendingLsuInstr pending);
    void scheduleNextLsuEvent();
    void executeLsuDone();
    void advanceLduRegisters();
    void queueLduResultRow(LduResultRow row);
    void scheduleNextLduResultVisible();
    void executeLduResultVisible();
    void executeLduResultGrant();
    void assignTargetVFU(VenusInstrPkt* pkt, VFU vfu1, VFU vfu2, VFU vfu_m);
    void releaseTargetVFU(const VenusInstrPkt *pkt,
                          VFU vfu1, VFU vfu2, VFU vfu_m);
    void releaseVfuQueueOnce(const VenusInstrPkt *pkt, VFU vfu);
    bool sequencerQueueAccountsVfu(const VenusInstrPkt *pkt, VFU vfu) const;
    bool checkVFUQueueisFull(VFU vfutmp);
    bool checkVinsnQueueisFull();
    void executeDispatchScalarReq();
    void executeRecvNewInstr();
    void executeTryIssueInstr();
    void advanceSequencerIssueState();
    bool currentIssueVfuQueuesReady() const;
    void releaseRunningIDandremoveinstr(VenusInstrPkt* pkt);
    void updateHazardTable();
    void hazardTablePipe1();
    void hazardTablePipe2();
    void hazardTablePipe3();
    void broadcastHazardTable(VenusHazardTable* venus_hazard_table);
    void forwardLsuCompletionToLanes(const VenusInstrPkt *pkt);
    void forwardLsuCompletionToShuffle(const VenusInstrPkt *pkt);
    void forwardProducerCompletionToLanes(const VenusInstrPkt *pkt);
    /**
     * Exact source of scalar600's venusidle_i input.  Despite the signal
     * name, high means that at least one sequencer issue ID is still live.
     */
    bool rtlScalarBarrierBusy() const;
    /**
     * Advance the sequencer-side registered running-ID model once per scalar
     * clock edge, after scalar600 has sampled the previous Q state.
     */
    void advanceRtlScalarBarrierState();
    /** Model venus_extension reset while the tile soft-reset is asserted. */
    void resetRtlScalarBarrierState();
    /**
     * Model the full task-local Venus reset asserted by tile_soft_reset_n.
     * SRAM contents are intentionally outside this reset domain.
     */
    void resetRtlTaskState();
    bool isIdle() const;
    std::string drainStatus() const;
    void requestTaskExit();
    void maybeFinishTask();
    void setActiveThreadContext(ThreadContext *tc, int task_id = -1);
    void readSharedL2(Addr addr, size_t size, uint8_t *data) const;
    void writeSharedL2(Addr addr, size_t size, const uint8_t *data);

    void init() override;
    // DrainState drain() override;



};
}
#endif // __SIM_VENUS_SEQUENCER_HH__
