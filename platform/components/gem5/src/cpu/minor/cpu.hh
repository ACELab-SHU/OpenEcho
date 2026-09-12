/*
 * Copyright (c) 2012-2014, 2020 ARM Limited
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
 *
 *  Top level definition of the Minor in-order CPU model
 */

#ifndef __CPU_MINOR_CPU_HH__
#define __CPU_MINOR_CPU_HH__

#include "base/compiler.hh"
#include "base/random.hh"
#include "cpu/base.hh"
#include "cpu/minor/activity.hh"
#include "cpu/minor/stats.hh"
#include "cpu/simple_thread.hh"
#include "enums/ThreadPolicy.hh"
#include "params/BaseMinorCPU.hh"
#include "mem/port.hh"
#include "mem/packet.hh"
#include "venus/venus_instr_pkt.hh"

#include <array>
#include <utility>

namespace gem5
{

class VenusSequencer;
class VenusDagScheduler;

namespace memory
{
class venus_vrf_mem;
}

namespace minor
{

/** Forward declared to break the cyclic inclusion dependencies between
 *  pipeline and cpu */
class Pipeline;

class MinorDynInst;
typedef RefCountingPtr<MinorDynInst> MinorDynInstPtr;

/** Minor will use the SimpleThread state for now */
typedef SimpleThread MinorThread;



} // namespace minor
/**
 *  MinorCPU is an in-order CPU model with four fixed pipeline stages:
 *
 *  Fetch1 - fetches lines from memory
 *  Fetch2 - decomposes lines into macro-op instructions
 *  Decode - decomposes macro-ops into micro-ops
 *  Execute - executes those micro-ops
 *
 *  This pipeline is carried in the MinorCPU::pipeline object.
 *  The exec_context interface is not carried by MinorCPU but by
 *      minor::ExecContext objects
 *  created by minor::Execute.
 */
class MinorCPU : public BaseCPU
{
  public:
    class VenusMinorCPUSequencerSidePort : public RequestPort
    {
      private:
        MinorCPU *owner;
      protected:
        bool recvTimingResp(PacketPtr pkt) override { panic("recvTimingResp unimpl."); }
        void recvRangeChange() override { panic("recvRangeChange unimpl."); }
      public:
        bool isBlocked = false;
        PacketPtr blockedPacket = nullptr;
        void sendPacket(PacketPtr pkt);
        void recvReqRetry() override;
        VenusMinorCPUSequencerSidePort(const std::string& name, MinorCPU *owner) :
            RequestPort(name, owner), owner(owner){}
    };
    Port &getPort(const std::string &if_name, PortID idx=InvalidPortID) override;
    bool isVenusPortBlocked() const { return venusInstrBlocked; }
    void setVenusBlocked(minor::MinorDynInstPtr inst) {
      venusInstrBlocked = true;
      blockedVenusInst = inst;
    }
    void clearVenusBlocked() {
      venusInstrBlocked = false;
      blockedVenusInst = nullptr;
    }
    minor::MinorDynInstPtr getBlockedVenusInst() { return blockedVenusInst; }
    bool sendVenusInstrPkt(minor::MinorDynInstPtr inst, uint32_t vs1, uint32_t avl);
    /**
     * A tile task may be handed to another SE context only after scalar
     * loads/stores from the previous context have left Minor's LSQ/store
     * buffer. This is an observation hook; it does not initiate gem5's
     * global checkpoint drain protocol.
     */
    bool isTaskMemoryDrained();
    /** Hold a DAG context in the same reset state as an unfired RTL tile. */
    void prepareVenusTaskContext(ThreadID thread_id);
    static constexpr unsigned VenusCsrNum = 32;
    static constexpr uint32_t VenusCsrMask = 0xfff;
    uint32_t getVenusCsr(unsigned idx) const { return venusCsrs.at(idx); }
    void setVenusCsr(unsigned idx, uint32_t val) { venusCsrs.at(idx) = val & VenusCsrMask; }
    uint32_t getMulshamt() const { return mulshamt; }
    uint32_t getMsbhead() const { return msbhead; }
    uint32_t getMulsaturate() const { return mulsaturate; }
    uint32_t getLsuAddrMsb() const { return lsuAddrMsb; }
    void setMulshamt(uint32_t val) { mulshamt = val & VenusCsrMask; setVenusCsr(0, val); }
    void setMsbhead(uint32_t val) { msbhead = val & VenusCsrMask; setVenusCsr(1, val); }
    void setMulsaturate(uint32_t val) { mulsaturate = val & VenusCsrMask; setVenusCsr(2, val); }
    void setLsuAddrMsb(uint32_t val) { lsuAddrMsb = val & VenusCsrMask; setVenusCsr(3, val); }
  private:
    uint32_t mulshamt    = 0;
    uint32_t msbhead     = 0;
    uint32_t mulsaturate = VenusCsrMask;
    uint32_t lsuAddrMsb  = 0;
    std::array<uint32_t, VenusCsrNum> venusCsrs = [] {
        std::array<uint32_t, VenusCsrNum> csrs = {};
        csrs[2] = VenusCsrMask;
        return csrs;
    }();
    ThreadID pendingVenusTaskSuspend = InvalidThreadID;

  protected:
    /** pipeline is a container for the clockable pipeline stage objects.
     *  Elements of pipeline call TheISA to implement the model. */
    minor::Pipeline *pipeline;

    Random::RandomPtr rng = Random::genRandom();

  public:
    /** Activity recording for pipeline.  This belongs to Pipeline but
     *  stages will access it through the CPU as the MinorCPU object
     *  actually mediates idling behaviour */
    minor::MinorActivityRecorder *activityRecorder;

    /** These are thread state-representing objects for this CPU.  If
     *  you need a ThreadContext for *any* reason, use
     *  threads[threadId]->getTC() */
    std::vector<minor::MinorThread *> threads;

  public:
    /** Provide a non-protected base class for Minor's Ports as derived
     *  classes are created by Fetch1 and Execute */
    class MinorCPUPort : public RequestPort
    {
      public:
        /** The enclosing cpu */
        MinorCPU &cpu;

      public:
        MinorCPUPort(const std::string& name_, MinorCPU &cpu_)
            : RequestPort(name_), cpu(cpu_)
        { }

    };

    /** Thread Scheduling Policy (RoundRobin, Random, etc) */
    enums::ThreadPolicy threadPolicy;
  protected:
     /** Return a reference to the data port. */
    Port &getDataPort() override;

    /** Return a reference to the instruction port. */
    Port &getInstPort() override;

  public:
    MinorCPU(const BaseMinorCPUParams &params);

    ~MinorCPU();

  public:
    /** Starting, waking and initialisation */
    void init() override;
    void startup() override;
    void wakeup(ThreadID tid) override;

    /** Processor-specific statistics */
    minor::MinorStats stats;

    VenusMinorCPUSequencerSidePort port_venusminorcpu_sendto_venussequencer;

    /**
     * Make committed scalar stores through the architectural VSPM window
     * visible in the banked Venus VRF.  RTL performs this conversion in
     * venus_mem2lanes; the ordinary gem5 crossbar does not rewrite packet
     * addresses, so the CPU calls the same conversion on write completion.
     */
    void commitVenusVspmWrite(Addr addr, size_t size, const uint8_t *data);
    void commitVenusSharedL2Write(Addr addr, size_t size,
                                  const uint8_t *data);
    void mirrorVenusDspmWrite(ThreadContext *tc, Addr addr, size_t size,
                              const uint8_t *data);
    /**
     * Observe the scalar600 task-completion control-register write.  RTL
     * retires a task at this architectural store, before the compiler's
     * aligned function epilogue and the SE-only ebreak trampoline.
     */
    void commitVenusTaskDone(ThreadContext *tc, Addr addr, size_t size,
                             const uint8_t *data,
                             const std::vector<bool> &byteEnable);
    /**
     * Attach the sequencer supplying Venus' RTL-visible running-ID state.
     * Both configuration and an optional DAG scheduler may provide this
     * connection, but they must resolve to the same object.
     */
    void setVenusSequencer(VenusSequencer *sequencer);
    /** Attach the live DAG scheduler which consumes task-done stores. */
    void setVenusDagScheduler(VenusDagScheduler *scheduler);
    /**
     * Advance the scalar600 three-flop synchronizer for venusidle_i.  In
     * the RTL signal naming is inverted: a high input means that at least
     * one Venus instruction ID is still running.
     */
    void advanceVenusBarrierBusySynchronizer();
    /** Return the qqq output sampled by the scalar barrier state machine. */
    bool rtlScalarBarrierBusyDelayed() const;
    unsigned rtlBarrierIdleReleaseToCommit() const
    { return venusRtlBarrierIdleReleaseToCommit; }
    bool rtlBarrierLateBusyRecheck() const
    { return venusRtlBarrierLateBusyRecheck; }
    /**
     * Advance scalar600_id_stage's persistent vec_barrier_cnt/barrier_valid
     * registers for a decoded VBARRIER.  Return true when ID may release it.
     * passThrough is set when an already-cleared zero-counter window accepts
     * the barrier combinationally, without starting a new six-cycle wait.
     */
    bool stepVenusBarrierFsm(bool &passThrough);
    /** Model scalar600 reset while a tile is not executing a task. */
    void resetVenusBarrierBusySynchronizer();

    /**
     * Clock scalar600_id_stage's architectural cycle/instruction counters.
     * This is called after the pipeline stages have sampled the current edge,
     * matching the non-blocking updates in the RTL ID stage.
     */
    void advanceRtlScalarCounters(bool idStopRequest, bool flushRequest);

    Counter architecturalCycleCounter(ThreadID tid) const override;
    Counter architecturalInstRetCounter(ThreadID tid) const override;

    bool venusInstrBlocked = false;
    minor::MinorDynInstPtr blockedVenusInst = nullptr;

  private:
    memory::venus_vrf_mem *venusVrf = nullptr;
    VenusSequencer *venusSequencer = nullptr;
    VenusDagScheduler *venusDagScheduler = nullptr;
    const bool venusRtlScalarTiming;
    const unsigned venusRtlBarrierIdleReleaseToCommit;
    const bool venusRtlBarrierLateBusyRecheck;
    Counter rtlScalarCycleCounter = 0;
    Counter rtlScalarInstCounter = 0;
    unsigned rtlScalarIdStopRemaining = 0;
    unsigned rtlScalarFlushRemaining = 0;
    bool rtlScalarSkipInitialInstEdge = true;
    bool venusBarrierBusyQ = false;
    bool venusBarrierBusyQq = false;
    bool venusBarrierBusyQqq = false;
    bool venusBarrierBusyQqqPreEdge = false;
    static constexpr unsigned VenusBarrierCounterReset = 6;
    unsigned venusBarrierCounter = VenusBarrierCounterReset;
    bool venusBarrierValid = false;
    std::vector<bool> rtlWfiSuspended;
    ThreadID rtlWfiWakeThread = InvalidThreadID;
    EventFunctionWrapper *rtlWfiWakeEventWrapper = nullptr;

  public:

    /** Stats interface from SimObject (by way of BaseCPU) */
    void regStats() override;

    /** Simple inst count interface from BaseCPU */
    Counter totalInsts() const override;
    Counter totalOps() const override;

    void serializeThread(CheckpointOut &cp, ThreadID tid) const override;
    void unserializeThread(CheckpointIn &cp, ThreadID tid) override;

    /** Serialize pipeline data */
    void serialize(CheckpointOut &cp) const override;
    void unserialize(CheckpointIn &cp) override;

    /** Drain interface */
    DrainState drain() override;
    void drainResume() override;
    /** Signal from Pipeline that MinorCPU should signal that a drain
     *  is complete and set its drainState */
    void signalDrainDone();
    void memWriteback() override;

    /** Switching interface from BaseCPU */
    void switchOut() override;
    void takeOverFrom(BaseCPU *old_cpu) override;

    /** Thread activation interface from BaseCPU. */
    void activateContext(ThreadID thread_id) override;
    void suspendContext(ThreadID thread_id) override;

    /**
     * Ask Minor's Execute stage to generate a real SuspendThread stream
     * redirect on its next edge.  An external clock-domain event must not
     * change ThreadContext status ahead of the CPU tick, because that leaves
     * matching-stream instructions at the commit head.
     */
    void requestVenusTaskSuspend(ThreadID thread_id);
    ThreadID takeVenusTaskSuspend();

    /** Mark a committed scalar600 WFI as the source of the suspension. */
    void markRtlWfiSuspend(ThreadID thread_id);
    /**
     * Present an external wake edge to scalar600.  RTL registers the wake
     * through its controller before releasing IF, so this is intentionally
     * distinct from Minor's immediate internal activateContext hook.
     */
    void requestRtlWfiWake(ThreadID thread_id);

    /**
     * Present a Scheduler-peripheral interrupt cause to the optional
     * PicoRV32 Q-register IRQ profile.  The cause is a hardware interrupt
     * bit mask (DMA, cluster, and so on), not a gem5/RISC-V interrupt ID.
     */
    void postVenusPicoIrq(ThreadID thread_id, uint32_t cause);

    /** Thread scheduling utility functions */
    std::vector<ThreadID> roundRobinPriority(ThreadID priority)
    {
        std::vector<ThreadID> prio_list;
        for (ThreadID i = 1; i <= numThreads; i++) {
            prio_list.push_back((priority + i) % numThreads);
        }
        return prio_list;
    }

    std::vector<ThreadID> randomPriority()
    {
        std::vector<ThreadID> prio_list;
        for (ThreadID i = 0; i < numThreads; i++) {
            prio_list.push_back(i);
        }

        std::shuffle(prio_list.begin(), prio_list.end(),
                     rng->gen);

        return prio_list;
    }

    /** The tick method in the MinorCPU is simply updating the cycle
     * counters as the ticking of the pipeline stages is already
     * handled by the Pipeline object.
     */
    void tick() { updateCycleCounters(BaseCPU::CPU_STATE_ON); }

    /** Interface for stages to signal that they have become active after
     *  a callback or eventq event where the pipeline itself may have
     *  already been idled.  The stage argument should be from the
     *  enumeration Pipeline::StageId */
    void wakeupOnEvent(unsigned int stage_id);
    EventFunctionWrapper *fetchEventWrapper;
};

} // namespace gem5

#endif /* __CPU_MINOR_CPU_HH__ */
