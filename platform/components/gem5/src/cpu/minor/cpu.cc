/*
 * Copyright (c) 2012-2014, 2017 ARM Limited
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

#include "cpu/minor/cpu.hh"

#include <algorithm>

#include "cpu/minor/dyn_inst.hh"
#include "cpu/minor/fetch1.hh"
#include "cpu/minor/pipeline.hh"
#include "debug/Drain.hh"
#include "debug/MinorCPU.hh"
#include "debug/Quiesce.hh"
#include "arch/riscv/interrupts.hh"
#include "arch/riscv/insts/venus.hh"
#include "mem/se_translating_port_proxy.hh"
#include "mem/venus_vrf_mem.hh"
#include "venus/VenusSequencer.hh"
#include "venus/VenusDagScheduler.hh"

namespace gem5
{

MinorCPU::MinorCPU(const BaseMinorCPUParams &params) :
    BaseCPU(params),
    threadPolicy(params.threadPolicy),
    stats(this),
    port_venusminorcpu_sendto_venussequencer(
        params.name + ".port_venusminorcpu_sendto_venussequencer", this),
    venusVrf(dynamic_cast<memory::venus_vrf_mem *>(params.venus_vrf_object)),
    venusSequencer(dynamic_cast<VenusSequencer *>(params.venus_sequencer)),
    venusRtlScalarTiming(params.venusRtlScalarTiming),
    venusRtlBarrierIdleReleaseToCommit(
        params.venusRtlBarrierIdleReleaseToCommit),
    venusRtlBarrierLateBusyRecheck(params.venusRtlBarrierLateBusyRecheck)
{
    fatal_if(venusRtlBarrierIdleReleaseToCommit > 3,
             "scalar600 VBARRIER release-to-commit cannot exceed its "
             "three registered ID/EX/WB edges");
    /* This is only written for one thread at the moment */
    minor::MinorThread *thread;

    for (ThreadID i = 0; i < numThreads; i++) {
        if (FullSystem) {
            thread = new minor::MinorThread(this, i, params.system,
                    params.mmu, params.isa[i], params.decoder[i]);
            thread->setStatus(ThreadContext::Halted);
        } else {
            thread = new minor::MinorThread(this, i, params.system,
                    params.workload[i], params.mmu,
                    params.isa[i], params.decoder[i]);
        }

        threads.push_back(thread);
        ThreadContext *tc = thread->getTC();
        threadContexts.push_back(tc);
    }


    if (params.checker) {
        fatal("The Minor model doesn't support checking (yet)\n");
    }

    pipeline = new minor::Pipeline(*this, params);
    activityRecorder = pipeline->getActivityRecorder();

    fetchEventWrapper = NULL;
    rtlWfiSuspended.resize(numThreads, false);
}

void
MinorCPU::commitVenusVspmWrite(Addr addr, size_t size, const uint8_t *data)
{
    if (!data || size == 0)
        return;

    // RV32 SE binaries may reach this window through either a zero-extended
    // or sign-extended pointer.  mem2lanes consumes the low address bits.
    const Addr local = static_cast<uint32_t>(addr);
    constexpr Addr VspmBase = 0x80100000;
    // RTL venus_block_wrapper decodes VSPM only up to
    // BLOCK_CTRLREGS_OFFSET.  The final 4 KiB of the block window is the
    // tile-manager/control-register page and must never alias banked VRF
    // storage.
    constexpr Addr TileManagerBase = 0x801ff000;
    if (local < VspmBase || local >= TileManagerBase ||
        size > TileManagerBase - local)
        return;

    fatal_if(!venusVrf,
             "scalar VSPM access requires BaseMinorCPU.venus_vrf_object");

    venusVrf->backdoor_WriteVspm(local, size, data);
}

void
MinorCPU::commitVenusSharedL2Write(Addr addr, size_t size,
                                   const uint8_t *data)
{
    if (!venusSequencer || !data || size == 0)
        return;

    // venus_soc_pkg exposes cluster 0's L2 through the SoC-global
    // [0x20000000, 0x22000000) aperture.  Venus LSU instructions carry the
    // corresponding cluster-local offset through its own LSU path.  Scalar
    // stores must use the global aperture and are translated here.
    const Addr local = static_cast<uint32_t>(addr);
    constexpr Addr GlobalCluster0L2Base = 0x20000000;
    constexpr Addr SharedL2Size = 32 * 1024 * 1024;
    if (local < GlobalCluster0L2Base ||
        local >= GlobalCluster0L2Base + SharedL2Size)
        return;
    const Addr offset = local - GlobalCluster0L2Base;
    if (size > SharedL2Size - offset)
        return;
    venusSequencer->writeSharedL2(offset, size, data);
}

void
MinorCPU::commitVenusTaskDone(
    ThreadContext *tc, Addr addr, size_t size, const uint8_t *data,
    const std::vector<bool> &byteEnable)
{
    if (!venusDagScheduler || !tc || !data || size < sizeof(uint32_t) ||
        byteEnable.size() < sizeof(uint32_t))
        return;

    /*
     * scalar600's block-control page is distinct from VSPM.  The runtime
     * first publishes the return descriptor at +0x28..+0x34, then stores
     * zero at TASK_DONE (+0x0).  Use the architectural address seen by the
     * core, accepting RV32's zero- and sign-extended pointer forms.
     */
    constexpr Addr TaskDone = 0x801ff000;
    if (static_cast<uint32_t>(addr) != TaskDone)
        return;
    for (unsigned i = 0; i < sizeof(uint32_t); ++i) {
        if (!byteEnable[i] || data[i] != 0)
            return;
    }
    venusDagScheduler->requestTaskExit(tc);
}

void
MinorCPU::mirrorVenusDspmWrite(ThreadContext *tc, Addr addr, size_t size,
                               const uint8_t *data)
{
    if (!tc || !data || size == 0)
        return;

    // This gc0802 configuration exposes its 16 KiB DSPM through tile-local
    // [0x20000, 0x24000) and block [0x80020000, 0x80024000) perspectives.
    // SE gives those ranges
    // distinct pages, so mirror scalar stores until that alias is represented
    // by a dedicated memory object.
    constexpr Addr DspmBase = 0x00020000;
    constexpr Addr DspmEnd = 0x00024000;
    constexpr Addr BlockBase = 0x80000000;
    const Addr local = static_cast<uint32_t>(addr);
    Addr alias = 0;
    if (local >= DspmBase && local + size <= DspmEnd)
        alias = BlockBase + local;
    else if (local >= BlockBase + DspmBase &&
             local + size <= BlockBase + DspmEnd)
        alias = local - BlockBase;
    else
        return;

    SETranslatingPortProxy(tc).writeBlob(alias, data, size);
}

void
MinorCPU::setVenusDagScheduler(VenusDagScheduler *scheduler)
{
    fatal_if(venusDagScheduler && venusDagScheduler != scheduler,
             "MinorCPU cannot attach two Venus DAG schedulers");
    venusDagScheduler = scheduler;
}

MinorCPU::~MinorCPU()
{
    delete pipeline;

    if (fetchEventWrapper != NULL)
        delete fetchEventWrapper;
    if (rtlWfiWakeEventWrapper != nullptr)
        delete rtlWfiWakeEventWrapper;

    for (ThreadID thread_id = 0; thread_id < threads.size(); thread_id++) {
        delete threads[thread_id];
    }
}

bool
MinorCPU::isTaskMemoryDrained()
{
    return pipeline->isLsqDrained();
}

void
MinorCPU::prepareVenusTaskContext(ThreadID thread_id)
{
    fatal_if(thread_id >= threads.size(),
             "Venus task reset has invalid thread %d", thread_id);
    pipeline->prepareVenusTaskContext(thread_id);
}

void
MinorCPU::setVenusSequencer(VenusSequencer *sequencer)
{
    fatal_if(!sequencer,
             "VBARRIER requires BaseMinorCPU.venus_sequencer");
    fatal_if(venusSequencer && venusSequencer != sequencer,
             "conflicting Venus sequencer connections for %s", name());
    venusSequencer = sequencer;
}

void
MinorCPU::advanceVenusBarrierBusySynchronizer()
{
    const bool busy = venusSequencer && venusSequencer->rtlScalarBarrierBusy();
    /*
     * This is the direct non-blocking-assignment equivalent of
     * scalar600_id_stage.sv's venusidle_i_q/qq/qqq register chain.
     */
    /* scalar600's barrier FSM consumes the old qqq on this edge. */
    venusBarrierBusyQqqPreEdge = venusBarrierBusyQqq;
    venusBarrierBusyQqq = venusBarrierBusyQq;
    venusBarrierBusyQq = venusBarrierBusyQ;
    venusBarrierBusyQ = busy;

    /*
     * scalar600 keeps vec_barrier_cnt at zero after an early barrier release
     * while delayed Venus busy is high.  Once the synchronized busy signal
     * is low on a later edge and barrier_valid is already clear, the
     * persistent counter rearms for the next barrier.
     */
    if (!venusBarrierValid &&
        venusBarrierCounter != VenusBarrierCounterReset &&
        !venusBarrierBusyQqqPreEdge) {
        venusBarrierCounter = VenusBarrierCounterReset;
    }

    /*
     * Both scalar600 and venus_sequencer use non-blocking assignments at
     * this edge.  Shift the scalar synchronizer from the old sequencer Q
     * first, then publish the sequencer Q state for the next scalar edge.
     */
    if (venusSequencer)
        venusSequencer->advanceRtlScalarBarrierState();
}

bool
MinorCPU::rtlScalarBarrierBusyDelayed() const
{
    fatal_if(!venusSequencer,
             "VBARRIER requires BaseMinorCPU.venus_sequencer");
    return venusBarrierBusyQqqPreEdge;
}

bool
MinorCPU::stepVenusBarrierFsm(bool &passThrough)
{
    /*
     * scalar600_id_stage drives vector_wait_req directly from the old
     * vec_barrier_cnt/barrier_valid state.  A second barrier arriving while
     * both are already clear is therefore an ID pass-through, even if the
     * synchronized Venus-busy signal is still high.  Keep that boundary
     * distinct from an armed barrier which has just finished waiting.
     */
    passThrough = venusBarrierCounter == 0 && !venusBarrierValid;

    if (venusBarrierCounter == VenusBarrierCounterReset) {
        --venusBarrierCounter;
        venusBarrierValid = true;
        return false;
    }

    if (venusBarrierValid && venusBarrierCounter != 0) {
        --venusBarrierCounter;
        return false;
    }

    if (!venusBarrierBusyQqqPreEdge) {
        /*
         * With old barrier_valid=1, RTL clears valid but deliberately keeps
         * counter zero for this edge.  The continuously clocked update above
         * rearms it on a later idle edge.  With valid already zero this is a
         * pass-through barrier in the same zero-counter window.
         */
        if (!venusBarrierValid)
            venusBarrierCounter = VenusBarrierCounterReset;
        venusBarrierValid = false;
        return true;
    }

    /* counter==0, busy==1: an armed barrier waits; a cleared one is bypassed. */
    return !venusBarrierValid;
}

void
MinorCPU::resetVenusBarrierBusySynchronizer()
{
    venusBarrierBusyQ = false;
    venusBarrierBusyQq = false;
    venusBarrierBusyQqq = false;
    venusBarrierBusyQqqPreEdge = false;
    venusBarrierCounter = VenusBarrierCounterReset;
    venusBarrierValid = false;
    rtlScalarCycleCounter = 0;
    rtlScalarInstCounter = 0;
    rtlScalarIdStopRemaining = 0;
    rtlScalarFlushRemaining = 0;
    rtlScalarSkipInitialInstEdge = true;
}

void
MinorCPU::advanceRtlScalarCounters(bool idStopRequest, bool flushRequest)
{
    if (!venusRtlScalarTiming)
        return;

    ++rtlScalarCycleCounter;

    /*
     * count_instr samples registered id_stop/flush state.  Requests formed
     * on this edge therefore hold following edges, not the current edge.
     * Task activation exposes count_cycle one replay edge before the ID
     * counter, represented by the initial held ID edge.
     */
    const bool held = rtlScalarSkipInitialInstEdge ||
        rtlScalarIdStopRemaining != 0 || rtlScalarFlushRemaining != 0;
    DPRINTF(MinorCPU,
        "scalar600 counter edge cycle=%llu inst=%llu held=%d"
        " id_stop_q=%u flush_q=%u id_stop_d=%d flush_d=%d\n",
        static_cast<unsigned long long>(rtlScalarCycleCounter),
        static_cast<unsigned long long>(rtlScalarInstCounter), held,
        rtlScalarIdStopRemaining, rtlScalarFlushRemaining,
        idStopRequest, flushRequest);
    if (!held)
        ++rtlScalarInstCounter;
    rtlScalarSkipInitialInstEdge = false;

    if (rtlScalarIdStopRemaining != 0)
        --rtlScalarIdStopRemaining;
    if (rtlScalarFlushRemaining != 0)
        --rtlScalarFlushRemaining;
    if (idStopRequest)
        rtlScalarIdStopRemaining =
            std::max(rtlScalarIdStopRemaining, 1U);
    if (flushRequest)
        rtlScalarFlushRemaining =
            std::max(rtlScalarFlushRemaining, 3U);
}

Counter
MinorCPU::architecturalCycleCounter(ThreadID tid) const
{
    return venusRtlScalarTiming ? rtlScalarCycleCounter : curCycle();
}

Counter
MinorCPU::architecturalInstRetCounter(ThreadID tid) const
{
    return venusRtlScalarTiming ? rtlScalarInstCounter : totalInsts();
}

void
MinorCPU::init()
{
    BaseCPU::init();

    if (!params().switched_out && system->getMemoryMode() != enums::timing) {
        fatal("The Minor CPU requires the memory system to be in "
            "'timing' mode.\n");
    }
}

/** Stats interface from SimObject (by way of BaseCPU) */
void
MinorCPU::regStats()
{
    BaseCPU::regStats();
    pipeline->regStats();
}

void
MinorCPU::serializeThread(CheckpointOut &cp, ThreadID thread_id) const
{
    threads[thread_id]->serialize(cp);
}

void
MinorCPU::unserializeThread(CheckpointIn &cp, ThreadID thread_id)
{
    threads[thread_id]->unserialize(cp);
}

void
MinorCPU::serialize(CheckpointOut &cp) const
{
    pipeline->serialize(cp);
    BaseCPU::serialize(cp);
}

void
MinorCPU::unserialize(CheckpointIn &cp)
{
    pipeline->unserialize(cp);
    BaseCPU::unserialize(cp);
}

void
MinorCPU::wakeup(ThreadID tid)
{
    DPRINTF(Drain, "[tid:%d] MinorCPU wakeup\n", tid);
    assert(tid < numThreads);

    if (threads[tid]->status() == ThreadContext::Suspended) {
        threads[tid]->activate();
    }
}

void
MinorCPU::startup()
{
    DPRINTF(MinorCPU, "MinorCPU startup\n");

    BaseCPU::startup();

    for (ThreadID tid = 0; tid < numThreads; tid++)
        pipeline->wakeupFetch(tid);
}

DrainState
MinorCPU::drain()
{
    // Deschedule any power gating event (if any)
    deschedulePowerGatingEvent();

    if (switchedOut()) {
        DPRINTF(Drain, "Minor CPU switched out, draining not needed.\n");
        return DrainState::Drained;
    }

    DPRINTF(Drain, "MinorCPU drain\n");

    /* Need to suspend all threads and wait for Execute to idle.
     * Tell Fetch1 not to fetch */
    if (pipeline->drain()) {
        DPRINTF(Drain, "MinorCPU drained\n");
        return DrainState::Drained;
    } else {
        DPRINTF(Drain, "MinorCPU not finished draining\n");
        return DrainState::Draining;
    }
}

void
MinorCPU::signalDrainDone()
{
    DPRINTF(Drain, "MinorCPU drain done\n");
    Drainable::signalDrainDone();
}

void
MinorCPU::drainResume()
{
    /* When taking over from another cpu make sure lastStopped
     * is reset since it might have not been defined previously
     * and might lead to a stats corruption */
    pipeline->resetLastStopped();

    if (switchedOut()) {
        DPRINTF(Drain, "drainResume while switched out.  Ignoring\n");
        return;
    }

    DPRINTF(Drain, "MinorCPU drainResume\n");

    if (!system->isTimingMode()) {
        fatal("The Minor CPU requires the memory system to be in "
            "'timing' mode.\n");
    }

    for (ThreadID tid = 0; tid < numThreads; tid++){
        wakeup(tid);
    }

    pipeline->drainResume();

    // Reschedule any power gating event (if any)
    schedulePowerGatingEvent();
}

void
MinorCPU::memWriteback()
{
    DPRINTF(Drain, "MinorCPU memWriteback\n");
}

void
MinorCPU::switchOut()
{
    DPRINTF(MinorCPU, "MinorCPU switchOut\n");

    assert(!switchedOut());
    BaseCPU::switchOut();

    /* Check that the CPU is drained? */
    activityRecorder->reset();
}

void
MinorCPU::takeOverFrom(BaseCPU *old_cpu)
{
    DPRINTF(MinorCPU, "MinorCPU takeOverFrom\n");

    BaseCPU::takeOverFrom(old_cpu);
}

void
MinorCPU::activateContext(ThreadID thread_id)
{
    DPRINTF(MinorCPU, "ActivateContext thread: %d\n", thread_id);

    /* Do some cycle accounting.  lastStopped is reset to stop the
     *  wakeup call on the pipeline from adding the quiesce period
     *  to BaseCPU::numCycles */
    stats.quiesceCycles += pipeline->cyclesSinceLastStopped();
    pipeline->resetLastStopped();

    /* Wake up the thread, wakeup the pipeline tick */
    threads[thread_id]->activate();
    wakeupOnEvent(minor::Pipeline::CPUStageId);

    if (!threads[thread_id]->getUseForClone())//the thread is not cloned
    {
        pipeline->wakeupFetch(thread_id);
    } else { //the thread from clone
        if (fetchEventWrapper != NULL)
            delete fetchEventWrapper;
        fetchEventWrapper = new EventFunctionWrapper([this, thread_id]
                  { pipeline->wakeupFetch(thread_id); }, "wakeupFetch");
        schedule(*fetchEventWrapper, clockEdge(Cycles(0)));
    }

    BaseCPU::activateContext(thread_id);
}

void
MinorCPU::suspendContext(ThreadID thread_id)
{
    DPRINTF(MinorCPU, "SuspendContext %d\n", thread_id);

    threads[thread_id]->suspend();

    BaseCPU::suspendContext(thread_id);
}

void
MinorCPU::requestVenusTaskSuspend(ThreadID thread_id)
{
    fatal_if(thread_id >= threads.size(),
             "Venus task suspend has invalid thread %d", thread_id);
    fatal_if(pendingVenusTaskSuspend != InvalidThreadID &&
                 pendingVenusTaskSuspend != thread_id,
             "Venus task suspend for thread %d overlaps thread %d",
             thread_id, pendingVenusTaskSuspend);
    pendingVenusTaskSuspend = thread_id;
    wakeupOnEvent(minor::Pipeline::ExecuteStageId);
}

ThreadID
MinorCPU::takeVenusTaskSuspend()
{
    const ThreadID thread_id = pendingVenusTaskSuspend;
    pendingVenusTaskSuspend = InvalidThreadID;
    return thread_id;
}

void
MinorCPU::markRtlWfiSuspend(ThreadID thread_id)
{
    if (venusRtlScalarTiming)
        rtlWfiSuspended.at(thread_id) = true;
}

void
MinorCPU::requestRtlWfiWake(ThreadID thread_id)
{
    fatal_if(!venusRtlScalarTiming,
             "requestRtlWfiWake requires scalar600 RTL timing mode");
    fatal_if(thread_id >= threads.size(),
             "requestRtlWfiWake invalid thread %d", thread_id);
    fatal_if(!rtlWfiSuspended.at(thread_id) ||
                 threads[thread_id]->status() != ThreadContext::Suspended,
             "requestRtlWfiWake thread %d is not suspended by WFI", thread_id);

    if (rtlWfiWakeEventWrapper && rtlWfiWakeEventWrapper->scheduled())
        return;
    if (rtlWfiWakeEventWrapper)
        delete rtlWfiWakeEventWrapper;

    rtlWfiWakeThread = thread_id;
    rtlWfiWakeEventWrapper = new EventFunctionWrapper(
        [this] {
            const ThreadID tid = rtlWfiWakeThread;
            rtlWfiWakeThread = InvalidThreadID;
            rtlWfiSuspended.at(tid) = false;
            threads[tid]->activate();
        },
        name() + ".rtlWfiWake");

    /*
     * scalar600_controller samples wake_up_sync_i in HALTED, registers
     * wfi_flush_stop_r, and only then exposes wfi_flush_stop to IF.  Relative
     * to Minor's internal activation point this is a two-tile-clock boundary.
     */
    schedule(*rtlWfiWakeEventWrapper, clockEdge(Cycles(2)));
    DPRINTF(MinorCPU,
            "scalar600 external WFI wake thread %d: activation @ %llu\n",
            thread_id,
            static_cast<unsigned long long>(rtlWfiWakeEventWrapper->when()));
}

void
MinorCPU::postVenusPicoIrq(ThreadID thread_id, uint32_t cause)
{
    fatal_if(thread_id >= threads.size(),
             "postVenusPicoIrq invalid thread %d", thread_id);
    fatal_if(cause == 0,
             "postVenusPicoIrq requires a non-zero hardware cause mask");
    postInterrupt(
        thread_id,
        RiscvISA::Interrupts::VenusPicoInterruptNumber,
        static_cast<int>(cause));
    // BaseCPU::postInterrupt activates a suspended SE context.  Ensure the
    // stopped Minor pipeline also receives an edge on which it can take the
    // newly pending architectural interrupt.
    wakeupOnEvent(minor::Pipeline::ExecuteStageId);
}

void
MinorCPU::wakeupOnEvent(unsigned int stage_id)
{
    DPRINTF(Quiesce, "Event wakeup from stage %d\n", stage_id);

    /* Mark that some activity has taken place and start the pipeline */
    activityRecorder->activateStage(stage_id);
    pipeline->start();
}

Port &
MinorCPU::getInstPort()
{
    return pipeline->getInstPort();
}

Port &
MinorCPU::getDataPort()
{
    return pipeline->getDataPort();
}

Counter
MinorCPU::totalInsts() const
{
    Counter ret = 0;

    for (auto i = threads.begin(); i != threads.end(); i ++)
        ret += (*i)->numInst;

    return ret;
}

Counter
MinorCPU::totalOps() const
{
    Counter ret = 0;

    for (auto i = threads.begin(); i != threads.end(); i ++)
        ret += (*i)->numOp;

    return ret;
}

Port&
MinorCPU::getPort(const std::string &if_name, PortID idx)
{
    if (if_name == "port_venusminorcpu_sendto_venussequencer") {
        return port_venusminorcpu_sendto_venussequencer;
    }
    return BaseCPU::getPort(if_name, idx);
}

void
MinorCPU::VenusMinorCPUSequencerSidePort::sendPacket(PacketPtr pkt)
{
    panic_if(blockedPacket != nullptr, "Should never try to send if blocked!");
    if (!sendTimingReq(pkt)) {
        blockedPacket = pkt;
        isBlocked = true;
        return;
    }
    isBlocked = false;
}

void
MinorCPU::VenusMinorCPUSequencerSidePort::recvReqRetry()
{
    assert(blockedPacket != nullptr);
    assert(isBlocked);

    PacketPtr pkt = blockedPacket;
    blockedPacket = nullptr;

    if (sendTimingReq(pkt)) {
        isBlocked = false;
        owner->clearVenusBlocked();
        delete (VenusInstrPkt*)pkt;
    } else {
        blockedPacket = pkt;
    }
}

bool
MinorCPU::sendVenusInstrPkt(minor::MinorDynInstPtr inst, uint32_t vs1, uint32_t avl)
{
    // VenusInstrPkt* venus_pkt = new VenusInstrPkt();
    const RiscvISA::VenusStaticInst* venus_inst =
            dynamic_cast<const RiscvISA::VenusStaticInst*>(inst->staticInst.get());

    if (!venus_inst) {
        panic("MinorCPU:sendVenusInstrPkt: Instruction at PC is not a VenusStaticInst! "
              "Instruction type: %s",
              inst->staticInst->getName());
    }

    uint64_t venus_inst_bits = venus_inst->venusInstBits;

    RequestPtr req = std::make_shared<Request>(
        inst->pc->instAddr(),
        40,
        0,
        inst->id.execSeqNum
    );
    // sequencer::recvTimingReq will delete this pkt
    PacketPtr pkt = new Packet(req, MemCmd::ReadReq);
    pkt->allocate();
    uint64_t* data = pkt->getPtr<uint64_t>();
    data[0] = venus_inst_bits;
    data[1] = ((uint64_t)avl << 32) | vs1;
    data[2] = ((uint64_t)this->mulshamt << 32) | (uint64_t)this->msbhead;
    data[3] = ((uint64_t)this->lsuAddrMsb << 32) | (uint64_t)this->mulsaturate;
    data[4] = ((uint64_t)this->getVenusCsr(5) << 32) | (uint64_t)this->getVenusCsr(4);

    if (!port_venusminorcpu_sendto_venussequencer.sendTimingReq(pkt)) {
        /*
         * This call site retries by rebuilding the request from the
         * MinorDynInst, rather than retaining the rejected timing packet.
         * A receiver must not consume a rejected packet, so release it here.
         */
        delete pkt;
        setVenusBlocked(inst);
        return false;
    }
    return true;
}


} // namespace gem5
