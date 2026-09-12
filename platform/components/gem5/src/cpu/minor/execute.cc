/*
 * Copyright (c) 2013-2014,2018-2020 ARM Limited
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

#include "cpu/minor/execute.hh"

#include <algorithm>
#include <cstdlib>
#include <functional>

#include "arch/riscv/regs/int.hh"
#include "cpu/minor/cpu.hh"
#include "cpu/minor/exec_context.hh"
#include "cpu/minor/fetch1.hh"
#include "cpu/minor/lsq.hh"
#include "cpu/op_class.hh"
#include "debug/Activity.hh"
#include "debug/Branch.hh"
#include "debug/Drain.hh"
#include "debug/ExecFaulting.hh"
#include "debug/MinorExecute.hh"
#include "debug/MinorInterrupt.hh"
#include "debug/MinorMem.hh"
#include "debug/MinorTrace.hh"
#include "debug/PCEvent.hh"
#include "arch/riscv/insts/venus.hh"
#include "venus/venus_extension_pkg.hh"

namespace gem5
{

namespace minor
{

namespace
{
enum class Scalar600DecodeFallback
{
    None,
    NoWriteNop,
    WriteZero,
    ZeroWord,
    AluOverride,
    CsrOverride,
    WfiOverride,
};

Scalar600DecodeFallback
scalar600DecodeFallback(uint32_t bits)
{
    const uint32_t opcode = bits & 0x7fU;
    const uint32_t funct3 = (bits >> 12) & 0x7U;

    if (bits == 0)
        return Scalar600DecodeFallback::ZeroWord;

    /* WFI is selected before the SYSTEM funct3 decoder from opcode and the
     * 12-bit immediate alone. */
    if (opcode == 0x73U && (bits >> 20) == 0x105U) {
        return bits == 0x10500073U ? Scalar600DecodeFallback::None :
            Scalar600DecodeFallback::WfiOverride;
    }

    switch (opcode) {
      case 0x0bU: /* PicoRV32 CUSTOM-0 Scheduler IRQ/Q instructions. */
        return Scalar600DecodeFallback::None;
      case 0x03U: /* LOAD: invalid widths retain decode_we with RES_NOP. */
        return funct3 == 0 || funct3 == 1 || funct3 == 2 ||
               funct3 == 4 || funct3 == 5 ?
            Scalar600DecodeFallback::None :
            Scalar600DecodeFallback::WriteZero;
      case 0x23U: /* STORE */
        return funct3 <= 2 ? Scalar600DecodeFallback::None :
            Scalar600DecodeFallback::NoWriteNop;
      case 0x63U: /* BRANCH */
        return funct3 == 2 || funct3 == 3 ?
            Scalar600DecodeFallback::NoWriteNop :
            Scalar600DecodeFallback::None;
      case 0x73U: { /* SYSTEM */
        if (bits == 0x10500073U) /* WFI is decoded before SYSTEM. */
            return Scalar600DecodeFallback::None;
        if (bits == 0x00100073U) {
            /* External task completion uses the scheduler EBREAK protocol. */
            return Scalar600DecodeFallback::None;
        }
        if (bits == 0x00000073U &&
            std::getenv("VENUS_GEM5_TASK_EBREAK_EXIT")) {
            /* Legacy directed cases end with Linux exit(2), even though the
             * runner selects the task-exit test hook by its historical
             * EBREAK name.  Let that hook observe ECALL; scalar600 still
             * treats ECALL as a NOP in every non-test execution. */
            return Scalar600DecodeFallback::None;
        }
        if (funct3 == 0)
            return Scalar600DecodeFallback::NoWriteNop;
        if (funct3 != 2)
            return Scalar600DecodeFallback::NoWriteNop;

        /* RTL compares CSR[11:1], so each implemented counter aliases the
         * adjacent odd CSR address exactly as scalar600_id_stage does. */
        const uint32_t csr = bits >> 20;
        const uint32_t csr_even = csr & 0xffeU;
        const bool implemented =
            csr_even == 0xc00U || csr_even == 0xc80U ||
            csr_even == 0xc02U || csr_even == 0xc82U ||
            csr_even == 0xf14U;
        if (!implemented)
            return Scalar600DecodeFallback::WriteZero;

        /* CSRRS is a pure read in scalar600: rs1 is ignored, and the RTL
         * only compares CSR[11:1].  Use gem5's native instruction only for
         * the legal even-address, rs1=x0 form; otherwise reproduce that
         * same counter read without invoking generic CSR access checks. */
        const uint32_t rs1 = (bits >> 15) & 0x1fU;
        return ((csr & 1U) != 0 || rs1 != 0) ?
            Scalar600DecodeFallback::CsrOverride :
            Scalar600DecodeFallback::None;
      }
      case 0x2fU: /* LR.W/SC.W only */
        if (funct3 == 2 &&
            (((bits >> 27) & 0x1fU) == 0x02U ||
             ((bits >> 27) & 0x1fU) == 0x03U)) {
            return Scalar600DecodeFallback::None;
        }
        return Scalar600DecodeFallback::NoWriteNop;
      case 0x0fU: /* FENCE/FENCE.I are not decoded by scalar600. */
        return Scalar600DecodeFallback::NoWriteNop;
      case 0x17U: /* AUIPC */
        return Scalar600DecodeFallback::None;
      case 0x13U: { /* OP-IMM */
        const uint32_t funct7 = (bits >> 25) & 0x7fU;
        if ((funct3 == 1 && funct7 != 0) ||
            (funct3 == 5 && funct7 != 0 && funct7 != 0x20U)) {
            return Scalar600DecodeFallback::AluOverride;
        }
        return Scalar600DecodeFallback::None;
      }
      case 0x2bU: /* Venus extension */
      case 0x37U: /* LUI */
      case 0x5bU: /* Venus extension */
      case 0x6fU: /* JAL */
        return Scalar600DecodeFallback::None;
      case 0x67U: /* JALR: scalar600 ignores funct3. */
        return funct3 == 0 ? Scalar600DecodeFallback::None :
            Scalar600DecodeFallback::AluOverride;
      case 0x33U: { /* OP/M */
        const uint32_t funct7 = (bits >> 25) & 0x7fU;
        if (funct7 == 1)
            return Scalar600DecodeFallback::None;
        const bool native = funct3 == 0 || funct3 == 5 ?
            (funct7 == 0 || funct7 == 0x20U) : funct7 == 0;
        return native ? Scalar600DecodeFallback::None :
            Scalar600DecodeFallback::AluOverride;
      }
      default:
        return Scalar600DecodeFallback::NoWriteNop;
    }
}

bool
scalar600ReadsIntegerRegister(uint32_t bits, RegIndex reg)
{
    const uint32_t opcode = bits & 0x7fU;
    const uint32_t funct3 = (bits >> 12) & 0x7U;
    const RegIndex rs1 = (bits >> 15) & 0x1fU;
    const RegIndex rs2 = (bits >> 20) & 0x1fU;

    /* scalar600 checks the load-use register equality before excluding x0,
     * so preserve a decoded x0 dependency as well. */
    switch (opcode) {
      case 0x37U: /* LUI: decoder keeps rdata1_en asserted. */
      case 0x17U: /* AUIPC: decoder keeps rdata1_en asserted. */
      case 0x67U: /* JALR */
      case 0x03U: /* LOAD, including invalid funct3 defaults */
      case 0x13U: /* OP-IMM */
        return reg == rs1;
      case 0x63U: /* BRANCH, including invalid funct3 defaults */
      case 0x23U: /* STORE, including invalid funct3 defaults */
      case 0x33U: /* OP/M */
        return reg == rs1 || reg == rs2;
      case 0x2fU: { /* Only decoded LR.W/SC.W forms read operands. */
        if (funct3 != 2)
            return false;
        const uint32_t funct5 = (bits >> 27) & 0x1fU;
        if (funct5 == 0x02U)
            return reg == rs1;
        if (funct5 == 0x03U)
            return reg == rs1 || reg == rs2;
        return false;
      }
      default:
        return false;
    }
}

bool
isVenusBarrierInst(const MinorDynInstPtr &inst)
{
    if (!inst || !inst->isInst())
        return false;
    const auto *venusInst =
        dynamic_cast<const RiscvISA::VenusStaticInst *>(inst->staticInst.get());
    return venusInst && venusInst->venusOp == RiscvISA::VBARRIER;
}

int32_t
signExtend(uint32_t value, unsigned int bits)
{
    const uint32_t sign = 1U << (bits - 1);
    return static_cast<int32_t>((value ^ sign) - sign);
}

bool
intDestination(const MinorDynInstPtr &inst, ThreadContext *thread,
    RegIndex &rd)
{
    if (!inst || !inst->isInst())
        return false;

    const auto *isa = thread->getIsaPtr();
    for (unsigned int dest = 0; dest < inst->staticInst->numDestRegs();
         ++dest) {
        const RegId reg =
            inst->staticInst->destRegIdx(dest).flatten(*isa);
        if (reg.is(IntRegClass)) {
            rd = reg.index();
            return true;
        }
    }

    return false;
}

bool
scalar600IntDestination(const MinorDynInstPtr &inst, ThreadContext *thread,
    RegIndex &rd)
{
    if (!inst || !inst->isInst())
        return false;
    const uint32_t bits =
        static_cast<uint32_t>(inst->staticInst->getEMI());
    const Scalar600DecodeFallback fallback =
        scalar600DecodeFallback(bits);
    if (fallback == Scalar600DecodeFallback::NoWriteNop ||
        fallback == Scalar600DecodeFallback::ZeroWord ||
        fallback == Scalar600DecodeFallback::WfiOverride) {
        return false;
    }
    if (fallback == Scalar600DecodeFallback::WriteZero ||
        fallback == Scalar600DecodeFallback::AluOverride ||
        fallback == Scalar600DecodeFallback::CsrOverride) {
        rd = (bits >> 7) & 0x1fU;
        return rd != 0;
    }
    return intDestination(inst, thread, rd);
}

struct RtlExValue
{
    bool loadLike = false;
    bool known = false;
    uint32_t value = 0;
};

/* scalar600_id_stage forwards ex_stage's final_res.  Calculate only pure
 * RV32 integer results here; evaluating a StaticInst early would mutate the
 * architectural ThreadContext and is therefore not a forwarding model. */
RtlExValue
scalar600ExValue(const MinorDynInstPtr &inst, uint32_t rs1,
    bool rs1_known, uint32_t rs2, bool rs2_known)
{
    RtlExValue result;
    const uint32_t bits =
        static_cast<uint32_t>(inst->staticInst->getEMI());
    if (scalar600DecodeFallback(bits) ==
        Scalar600DecodeFallback::WriteZero) {
        result.known = true;
        result.value = 0;
        return result;
    }
    const uint32_t opcode = bits & 0x7fU;
    const uint32_t funct3 = (bits >> 12) & 0x7U;
    const uint32_t funct7 = (bits >> 25) & 0x7fU;
    switch (opcode) {
      case 0x03U: /* LOAD */
      case 0x2fU: /* LR/SC/AMO: scalar600 treats LR/SC as load-like */
        result.loadLike = true;
        return result;
      case 0x37U: /* LUI */
        result.known = true;
        result.value = bits & 0xfffff000U;
        return result;
      case 0x17U: /* AUIPC */
        result.known = true;
        result.value = static_cast<uint32_t>(
            inst->pc->instAddr() + (bits & 0xfffff000U));
        return result;
      case 0x6fU: /* JAL link */
      case 0x67U: /* JALR link */
        result.known = true;
        result.value = static_cast<uint32_t>(inst->pc->instAddr() + 4);
        return result;
      case 0x73U: /* scalar600 read-only CSR subset */
        if (funct3 == 0x2U &&
            (((bits >> 20) & 0xffeU) == 0xf14U)) {
            /* Each scalar600 instance is one hart inside its tile. */
            result.known = true;
            result.value = 0;
        }
        return result;
      case 0x13U: { /* OP-IMM */
        if (!rs1_known)
            return result;
        const int32_t immediate = signExtend(bits >> 20, 12);
        switch (funct3) {
          case 0x0U:
            result.value = rs1 + static_cast<uint32_t>(immediate);
            break;
          case 0x2U:
            result.value = static_cast<int32_t>(rs1) < immediate;
            break;
          case 0x3U:
            result.value = rs1 < static_cast<uint32_t>(immediate);
            break;
          case 0x4U:
            result.value = rs1 ^ static_cast<uint32_t>(immediate);
            break;
          case 0x6U:
            result.value = rs1 | static_cast<uint32_t>(immediate);
            break;
          case 0x7U:
            result.value = rs1 & static_cast<uint32_t>(immediate);
            break;
          case 0x1U:
            result.value = rs1 << ((bits >> 20) & 0x1fU);
            break;
          case 0x5U:
            if (funct7 == 0x00U) {
                result.value = rs1 >> ((bits >> 20) & 0x1fU);
            } else {
                result.value = static_cast<uint32_t>(
                    static_cast<int32_t>(rs1) >>
                    ((bits >> 20) & 0x1fU));
            }
            break;
          default:
            return result;
        }
        result.known = true;
        return result;
      }
      case 0x33U: /* OP */
        if (funct7 == 0x01U)
            return result; /* MUL/DIV final_res is not combinational here. */
        if (!rs1_known || !rs2_known)
            return result;
        switch (funct3) {
          case 0x0U:
            if (funct7 == 0x00U)
                result.value = rs1 + rs2;
            else
                result.value = rs1 - rs2;
            break;
          case 0x1U:
            result.value = rs1 << (rs2 & 0x1fU);
            break;
          case 0x2U:
            result.value = static_cast<int32_t>(rs1) <
                static_cast<int32_t>(rs2);
            break;
          case 0x3U:
            result.value = rs1 < rs2;
            break;
          case 0x4U:
            result.value = rs1 ^ rs2;
            break;
          case 0x5U:
            if (funct7 == 0x00U) {
                result.value = rs1 >> (rs2 & 0x1fU);
            } else {
                result.value = static_cast<uint32_t>(
                    static_cast<int32_t>(rs1) >> (rs2 & 0x1fU));
            }
            break;
          case 0x6U:
            result.value = rs1 | rs2;
            break;
          case 0x7U:
            result.value = rs1 & rs2;
            break;
          default:
            return result;
        }
        result.known = true;
        return result;
      default:
        return result;
    }
}
}

Execute::Execute(const std::string &name_,
    MinorCPU &cpu_,
    const BaseMinorCPUParams &params,
    Latch<ForwardInstData>::Output inp_,
    Latch<BranchData>::Input out_) :
    Named(name_),
    inp(inp_),
    out(out_),
    cpu(cpu_),
    issueLimit(params.executeIssueLimit),
    memoryIssueLimit(params.executeMemoryIssueLimit),
    commitLimit(params.executeCommitLimit),
    memoryCommitLimit(params.executeMemoryCommitLimit),
    processMoreThanOneInput(params.executeCycleInput),
    fuDescriptions(*params.executeFuncUnits),
    numFuncUnits(fuDescriptions.funcUnits.size()),
    setTraceTimeOnCommit(params.executeSetTraceTimeOnCommit),
    setTraceTimeOnIssue(params.executeSetTraceTimeOnIssue),
    allowEarlyMemIssue(params.executeAllowEarlyMemoryIssue),
    noCostFUIndex(fuDescriptions.funcUnits.size() + 1),
    lsq(name_ + ".lsq", name_ + ".dcache_port",
        cpu_, *this,
        params.executeMaxAccessesInMemory,
        params.executeMemoryWidth,
        params.executeLSQRequestsQueueSize,
        params.executeLSQTransfersQueueSize,
        params.executeLSQStoreBufferSize,
        params.executeLSQMaxStoreBufferStoresPerCycle),
    executeInfo(params.numThreads,
            ExecuteThreadInfo(params.executeCommitLimit)),
    rtlIdWriters(params.numThreads),
    venusDirectBarrierRequestWindow(params.numThreads, false),
    venusDirectBarrierRequestEpoch(params.numThreads, false),
    venusRequestFollowerWindow(params.numThreads, false),
    venusRequestAcceptedCycle(params.numThreads, Cycles(0)),
    venusRequestFollowerLsuSeq(params.numThreads, 0),
    venusRequestLsuResponseReadyCycle(params.numThreads, Cycles(0)),
    venusRequestLsuReleaseCycle(params.numThreads, Cycles(0)),
    venusRtlScalarTiming(params.venusRtlScalarTiming),
    venusBarrierInId(nullptr),
    venusBarrierThreadId(InvalidThreadID),
    venusBarrierFirstStepCycle(0),
    venusBarrierIssueBlocked(false),
    venusBarrierReleasePending(false),
    venusBarrierDirectIdCapture(false),
    interruptPriority(0),
    issuePriority(0),
    commitPriority(0)
{
    if (commitLimit < 1) {
        fatal("%s: executeCommitLimit must be >= 1 (%d)\n", name_,
            commitLimit);
    }

    if (issueLimit < 1) {
        fatal("%s: executeCommitLimit must be >= 1 (%d)\n", name_,
            issueLimit);
    }

    if (memoryIssueLimit < 1) {
        fatal("%s: executeMemoryIssueLimit must be >= 1 (%d)\n", name_,
            memoryIssueLimit);
    }

    if (memoryCommitLimit > commitLimit) {
        fatal("%s: executeMemoryCommitLimit (%d) must be <="
            " executeCommitLimit (%d)\n",
            name_, memoryCommitLimit, commitLimit);
    }

    if (params.executeInputBufferSize < 1) {
        fatal("%s: executeInputBufferSize must be >= 1 (%d)\n", name_,
        params.executeInputBufferSize);
    }

    if (params.executeInputBufferSize < 1) {
        fatal("%s: executeInputBufferSize must be >= 1 (%d)\n", name_,
        params.executeInputBufferSize);
    }

    /* This should be large enough to count all the in-FU instructions
     *  which need to be accounted for in the inFlightInsts
     *  queue */
    unsigned int total_slots = 0;

    /* Make FUPipelines for each MinorFU */
    for (unsigned int i = 0; i < numFuncUnits; i++) {
        std::ostringstream fu_name;
        MinorFU *fu_description = fuDescriptions.funcUnits[i];

        /* Note the total number of instruction slots (for sizing
         *  the inFlightInst queue) and the maximum latency of any FU
         *  (for sizing the activity recorder) */
        total_slots += fu_description->opLat;

        fu_name << name_ << ".fu." << i;

        FUPipeline *fu = new FUPipeline(fu_name.str(), *fu_description, cpu);

        funcUnits.push_back(fu);
    }

    /** Check that there is a functional unit for all operation classes */
    for (int op_class = No_OpClass + 1; op_class < Num_OpClasses; op_class++) {
        bool found_fu = false;
        unsigned int fu_index = 0;

        while (fu_index < numFuncUnits && !found_fu)
        {
            if (funcUnits[fu_index]->provides(
                static_cast<OpClass>(op_class)))
            {
                found_fu = true;
            }
            fu_index++;
        }

        if (!found_fu) {

            warn("No functional unit for OpClass %s\n",
                enums::OpClassStrings[op_class]);
        }
    }

    /* Per-thread structures */
    for (ThreadID tid = 0; tid < params.numThreads; tid++) {
        std::string tid_str = std::to_string(tid);

        /* Input Buffers */
        inputBuffer.push_back(
            InputBuffer<ForwardInstData>(
                name_ + ".inputBuffer" + tid_str, "insts",
                params.executeInputBufferSize));

        const auto &regClasses = cpu.threads[tid]->getIsaPtr()->regClasses();

        /* Scoreboards */
        scoreboard.emplace_back(name_ + ".scoreboard" + tid_str, regClasses);

        /* In-flight instruction records */
        executeInfo[tid].inFlightInsts =  new Queue<QueuedInst,
            ReportTraitsAdaptor<QueuedInst> >(
            name_ + ".inFlightInsts" + tid_str, "insts", total_slots);

        executeInfo[tid].inFUMemInsts = new Queue<QueuedInst,
            ReportTraitsAdaptor<QueuedInst> >(
            name_ + ".inFUMemInsts" + tid_str, "insts", total_slots);
    }
}

const ForwardInstData *
Execute::getInput(ThreadID tid)
{
    /* Get a line from the inputBuffer to work with */
    if (!inputBuffer[tid].empty()) {
        const ForwardInstData &head = inputBuffer[tid].front();

        return (head.isBubble() ? NULL : &(inputBuffer[tid].front()));
    } else {
        return NULL;
    }
}

void
Execute::popInput(ThreadID tid)
{
    if (!inputBuffer[tid].empty())
        inputBuffer[tid].pop();

    executeInfo[tid].inputIndex = 0;
}

bool
Execute::rtlIdSourcesCommitted(MinorDynInstPtr inst)
{
    const ThreadID tid = inst->id.threadId;
    return scoreboard[tid].sourcesCommitted(inst, cpu.getContext(tid));
}

bool
Execute::rtlDividerBlocksDecode(ThreadID tid) const
{
    if (!venusRtlScalarTiming)
        return false;

    const RtlDividerState &divider = executeInfo[tid].rtlDivider;
    if (!divider.issueBlocked)
        return false;

    /*
     * scalar600 releases ID on the edge before the divider result retires.
     * That lets the next instruction occupy EX on the following edge; a
     * commit-time-only release inserts an artificial bubble between two
     * one-cycle divide-by-zero completions.
     */
    return !divider.pending ||
        cpu.curCycle() + Cycles(1) < divider.pendingReadyCycle;
}

bool
Execute::rtlPreviousLoadFeedsInst(
    ThreadID tid, const MinorDynInstPtr &inst) const
{
    const ExecuteThreadInfo &info = executeInfo[tid];
    if (!inst || !inst->isInst() || !info.previousIssuedLoad)
        return false;

    const auto &load_static = info.previousIssuedLoad->staticInst;
    const uint32_t load_bits = static_cast<uint32_t>(
        load_static->getEMI());
    const RegIndex rtl_load_rd = (load_bits >> 7) & 0x1fU;
    const uint32_t consumer_bits = static_cast<uint32_t>(
        inst->staticInst->getEMI());

    if (scalar600ReadsIntegerRegister(consumer_bits, rtl_load_rd))
        return true;

    /* Retain the architectural description for Venus-extension encodings
     * whose two-word operand protocol is outside the scalar decoder table. */
    for (unsigned dest = 0; dest < load_static->numDestRegs(); ++dest) {
        for (unsigned src = 0; src < inst->staticInst->numSrcRegs(); ++src) {
            if (load_static->destRegIdx(dest) ==
                inst->staticInst->srcRegIdx(src)) {
                return true;
            }
        }
    }
    return false;
}

bool
Execute::rtlScalarPipelineBlocksIssue(ThreadID tid)
{
    if (!venusRtlScalarTiming)
        return false;

    if (rtlDividerBlocksDecode(tid))
        return true;
    return rtlScalarNonDividerPipelineBlocksIssue(tid);
}

bool
Execute::rtlScalarNonDividerPipelineBlocksIssue(ThreadID tid)
{
    if (!venusRtlScalarTiming)
        return false;

    const ExecuteThreadInfo &info = executeInfo[tid];

    /*
     * scalar600 keeps the second word of a Venus request resident in ID
     * while scalar_req_valid_o && !scalar_req_ready_i.  Minor's decoupled
     * Execute input can otherwise contain a younger instruction even though
     * Decode has already observed the blocked CPU port.  Hold both sides of
     * the modeled ID boundary until the same generic ready/valid handshake
     * succeeds.
     */
    if (cpu.isVenusPortBlocked())
        return true;

    if (venusRequestLsuReleaseCycle[tid] == cpu.curCycle())
        return true;

    if (info.inFlightInsts->empty())
        return false;

    const MinorDynInstPtr &head = info.inFlightInsts->front().inst;
    if (!head || !head->isInst())
        return false;

    /*
     * scalar600 has one physical EX/WB pipeline.  MUL holds EX until its
     * registered completion and an LSU operation asserts stallreq_from_wb
     * through its response/commit edge.  The vector request is different:
     * RTL can already expose the following scalar instruction in ID while
     * its two-word request is waiting for the extension, so do not serialize
     * that follower on the fused gem5 Venus instruction.
     */
    if (head->staticInst->opClass() == enums::IntMult)
        return true;

    if (!head->staticInst->isMemRef())
        return false;

    /*
     * scalar600_mul is not fed from the registered EX/WB bus.  Its request
     * and operands come directly from the instruction held in ID through
     * mul_div_bus, so an independent MUL immediately behind a WB-stalled
     * load continues through the multiplier even though the architectural
     * pipeline cannot advance.  Admit exactly that one held-ID occupant.
     * A dependent MUL remains blocked by the load-use/scoreboard checks in
     * issue(), and occupiedSpace() prevents a second younger instruction
     * from entering while the load is still at the commit head.  Stores are
     * intentionally excluded: their shorter LSU transaction does not expose
     * the same completed-behind-WB retirement boundary in the RTL oracle.
     */
    if (head->staticInst->isLoad() &&
        info.inFlightInsts->occupiedSpace() == 1) {
        const ForwardInstData *pending = getInput(tid);
        if (pending) {
            const MinorDynInstPtr &held_id =
                pending->insts[info.inputIndex];
            if (held_id && !held_id->isBubble() && held_id->isInst() &&
                held_id->staticInst->opClass() == enums::IntMult &&
                !rtlPreviousLoadFeedsInst(tid, held_id)) {
                return false;
            }
        }
    }

    return true;
}

void
Execute::rtlIdOnIssue(MinorDynInstPtr inst)
{
    if (!venusRtlScalarTiming || !inst || !inst->isInst())
        return;

    const ThreadID tid = inst->id.threadId;
    if (inst->staticInst->opClass() == enums::IntDiv) {
        executeInfo[tid].rtlDivider.issueBlocked = true;
        /* stallreq_from_ex is formed as soon as DIV/REM enters ID/EX. */
        rtlScalarIdStopRequest = true;
        /*
         * The RTL divider request is driven from ID/EX, not from WB.  Plan
         * its registered completion as soon as Minor issues the operation so
         * Decode can observe the release edge even for a one-cycle follow-up
         * divide-by-zero transaction.
         */
        rtlDividerPlan(inst, executeInfo[tid]);
    }
    ThreadContext *thread = cpu.getContext(tid);
    RegIndex rd;
    if (!scalar600IntDestination(inst, thread, rd))
        return;

    const uint32_t bits =
        static_cast<uint32_t>(inst->staticInst->getEMI());
    if (scalar600DecodeFallback(bits) ==
            Scalar600DecodeFallback::NoWriteNop ||
        scalar600DecodeFallback(bits) ==
            Scalar600DecodeFallback::ZeroWord) {
        return;
    }
    uint32_t rs1 = 0;
    uint32_t rs2 = 0;
    const bool rs1_known = rtlIdReadExReg(
        tid, (bits >> 15) & 0x1fU, rs1);
    const bool rs2_known = rtlIdReadExReg(
        tid, (bits >> 20) & 0x1fU, rs2);
    const RtlExValue ex_value = scalar600ExValue(
        inst, rs1, rs1_known, rs2, rs2_known);
    rtlIdWriters[tid].push_back({
        inst, rd, ex_value.loadLike, ex_value.known, ex_value.value
    });

    DPRINTF(MinorExecute,
        "scalar600 ID writer issue: inst=%s rd=x%d load_like=%d known=%d"
        " value=0x%08x\n", *inst, rd, ex_value.loadLike,
        ex_value.known, ex_value.value);
}

bool
Execute::rtlIdReadExReg(ThreadID tid, RegIndex src, uint32_t &value)
{
    auto &writers = rtlIdWriters[tid];
    std::function<bool(RegIndex, uint32_t &, size_t)> read_before;
    read_before = [&](RegIndex wanted, uint32_t &result, size_t limit) {
        if (wanted == 0) {
            result = 0;
            return true;
        }
        for (size_t pos = limit; pos > 0; --pos) {
            RtlIdWriter &writer = writers[pos - 1];
            if (writer.rd != wanted)
                continue;
            if (writer.loadLike)
                return false;
            if (!writer.valueKnown) {
                const uint32_t bits = static_cast<uint32_t>(
                    writer.inst->staticInst->getEMI());
                uint32_t rs1 = 0;
                uint32_t rs2 = 0;
                const bool rs1_known = read_before(
                    (bits >> 15) & 0x1fU, rs1, pos - 1);
                const bool rs2_known = read_before(
                    (bits >> 20) & 0x1fU, rs2, pos - 1);
                const RtlExValue refreshed = scalar600ExValue(
                    writer.inst, rs1, rs1_known, rs2, rs2_known);
                if (!refreshed.known || refreshed.loadLike)
                    return false;
                writer.valueKnown = true;
                writer.value = refreshed.value;
            }
            result = writer.value;
            return true;
        }
        result = static_cast<uint32_t>(
            cpu.getContext(tid)->getReg(RiscvISA::intRegClass[wanted]));
        return true;
    };
    return read_before(src, value, writers.size());
}

bool
Execute::rtlDividerOperandsReady(MinorDynInstPtr inst)
{
    const uint32_t bits =
        static_cast<uint32_t>(inst->staticInst->getEMI());
    uint32_t ignored = 0;
    return rtlIdReadExReg(
               inst->id.threadId, (bits >> 15) & 0x1fU, ignored) &&
        rtlIdReadExReg(
               inst->id.threadId, (bits >> 20) & 0x1fU, ignored);
}

uint32_t
Execute::rtlDividerResultWire(const RtlDividerState &state,
    bool current_signed) const
{
    constexpr uint64_t mask33 = (1ULL << 33) - 1;
    const bool real_complete =
        state.count == 0xff || state.count == 0xf0;
    const bool real_signed =
        real_complete ? state.signedBuffer : current_signed;
    const bool op1_sign =
        real_complete ? state.op1SignBuffer : (state.op1Reg >> 31);
    const bool op2_sign =
        real_complete ? state.op2SignBuffer : (state.op2Reg >> 31);
    uint64_t quotient = state.unsignedDivResult & mask33;
    uint64_t remainder = state.unsignedRemResult & mask33;
    if (real_signed && op1_sign != op2_sign)
        quotient = (~(quotient - 1)) & mask33;
    if (real_signed && op1_sign)
        remainder = (~(remainder - 1)) & mask33;
    return static_cast<uint32_t>(
        state.divOrRemReg ? remainder : quotient);
}

std::pair<bool, uint32_t>
Execute::rtlDividerStep(RtlDividerState &state, bool en, bool is_signed,
    bool is_rem, uint32_t op1, uint32_t op2)
{
    constexpr uint64_t mask32 = (1ULL << 32) - 1;
    constexpr uint64_t mask33 = (1ULL << 33) - 1;
    const bool old_en = state.enReg;
    const uint32_t old_op1 = state.op1Reg;
    const uint32_t old_op2 = state.op2Reg;
    const uint8_t old_count = state.count;
    const bool old_complete = old_count == 0xff;
    const bool old_complete_delay = old_count == 0xf0;
    const bool old_real_complete = old_complete || old_complete_delay;
    const uint32_t old_result_wire =
        rtlDividerResultWire(state, is_signed);
    const bool real_signed =
        old_real_complete ? state.signedBuffer : is_signed;
    const uint32_t magnitude1 =
        real_signed && (old_op1 >> 31) ? (~old_op1 + 1) : old_op1;
    const uint32_t magnitude2 =
        real_signed && (old_op2 >> 31) ? (~old_op2 + 1) : old_op2;

    uint8_t new_count = old_count;
    uint64_t new_temp_rem = state.tempRem;
    uint64_t new_div = state.unsignedDivResult;
    uint64_t new_rem = state.unsignedRemResult;
    if (!old_en || old_complete_delay) {
        new_count = 32;
        new_temp_rem = 0;
    } else if (old_op2 == 0) {
        new_div = real_signed && (old_op1 >> 31) ? 1 : mask32;
        new_rem = magnitude1;
        new_count = 0xff;
    } else if (!(old_count & 0x80)) {
        /* unsigned_op1 is 33 bits in RTL, with bit 32 tied to zero.  A
         * 32-bit C++ shift by 32 is undefined (and x86 commonly aliases it
         * to bit 0), which corrupts the first restoring-division iteration. */
        const uint64_t bit = old_count < 32 ?
            ((magnitude1 >> old_count) & 1) : 0;
        const uint64_t temp_result =
            ((state.tempRem & mask32) << 1) | bit;
        const uint64_t temp_div =
            (temp_result - magnitude2) & mask33;
        if (temp_div & (1ULL << 32)) {
            new_div = ((state.unsignedDivResult & mask32) << 1) & mask33;
            new_temp_rem = temp_result;
        } else {
            new_div =
                (((state.unsignedDivResult & mask32) << 1) | 1) & mask33;
            new_temp_rem = temp_div;
        }
        new_count = old_count - 1;
    } else {
        new_rem = state.tempRem;
        new_count = 0xf0;
    }

    if (old_en) {
        state.signedBuffer = is_signed;
        state.op1SignBuffer = old_op1 >> 31;
        state.op2SignBuffer = old_op2 >> 31;
    }
    state.enReg = en;
    state.op1Reg = op1;
    state.op2Reg = op2;
    state.unsignedDivResult = new_div;
    state.unsignedRemResult = new_rem;
    state.tempRem = new_temp_rem;
    state.count = new_count;
    if (old_real_complete)
        state.resultReg = old_result_wire;
    state.divOrRemReg = is_rem;

    const bool complete = state.count == 0xff;
    const uint32_t result = state.count == 0xf0 ?
        rtlDividerResultWire(state, is_signed) : state.resultReg;
    return {complete, result};
}

void
Execute::rtlDividerAdvanceIdle(RtlDividerState &state, Cycles target)
{
    while (state.modelCycle < target) {
        rtlDividerStep(state, false, false, false, 0, 0);
        state.modelCycle += Cycles(1);
    }
}

void
Execute::rtlDividerPlan(MinorDynInstPtr inst, ExecuteThreadInfo &info)
{
    RtlDividerState &state = info.rtlDivider;
    if (state.pending && state.pendingSeqNum == inst->id.execSeqNum)
        return;

    const uint32_t bits =
        static_cast<uint32_t>(inst->staticInst->getEMI());
    const RegIndex rs1 = (bits >> 15) & 0x1f;
    const RegIndex rs2 = (bits >> 20) & 0x1f;
    uint32_t op1 = 0;
    uint32_t op2 = 0;
    const bool op1_known = rtlIdReadExReg(inst->id.threadId, rs1, op1);
    const bool op2_known = rtlIdReadExReg(inst->id.threadId, rs2, op2);
    if (!op1_known || !op2_known) {
        panic("scalar600 divider issued without ID/EX operands: %s "
              "rs1=x%d known=%d rs2=x%d known=%d",
              *inst, rs1, op1_known, rs2, op2_known);
    }
    const uint32_t funct3 = (bits >> 12) & 0x7;
    const bool is_signed = funct3 == 4 || funct3 == 6;
    const bool is_rem = funct3 == 6 || funct3 == 7;
    /*
     * scalar600 gives stallreq_from_ex priority over the load/store
     * pipeline.  When an independent DIV/REM enters ID/EX on the edge that
     * the immediately preceding load completes, the held load is
     * re-presented every six cycles during the 35-cycle iterative divide.
     * The divider completion is consequently delayed by another 170
     * cycles, producing the RTL's 205-cycle load-to-DIV retirement gap.
     * This delay belongs to the divider pending-completion edge; expressing
     * it as a generic FU commit delay would run in parallel with the divider
     * FSM and incorrectly take max(170, 35).
     */
    const bool independent_load_replay =
        info.previousIssuedWasLoad && info.previousIssuedLoad &&
        (!info.previousIssuedLoadCommitted ||
         cpu.curCycle() <= info.previousIssuedLoadCommitCycle) &&
        !rtlPreviousLoadFeedsInst(inst->id.threadId, inst);

    uint32_t result = 0;
    Cycles request_cycles(0);
    do {
        const bool complete_before_edge = state.count == 0xff;
        result = rtlDividerStep(
            state, true, is_signed, is_rem, op1, op2).second;
        state.modelCycle += Cycles(1);
        request_cycles += Cycles(1);
        if (complete_before_edge)
            break;
    } while (true);

    state.pending = true;
    state.pendingSeqNum = inst->id.execSeqNum;
    state.pendingReadyCycle = cpu.curCycle() + request_cycles +
        (independent_load_replay ? Cycles(170) : Cycles(0));
    state.pendingResult = result;
    DPRINTF(MinorExecute,
        "scalar600 divider plan inst=%s ready=%llu result=0x%08x\n",
        *inst,
        static_cast<unsigned long long>(state.pendingReadyCycle), result);
}

void
Execute::rtlIdOnComplete(MinorDynInstPtr inst)
{
    if (!venusRtlScalarTiming || !inst)
        return;

    const ThreadID tid = inst->id.threadId;
    auto &writers = rtlIdWriters[tid];
    writers.erase(std::remove_if(writers.begin(), writers.end(),
        [&](const RtlIdWriter &writer) { return writer.inst == inst; }),
        writers.end());
}

bool
Execute::rtlIdHasQueuedWriter(ThreadID tid, RegIndex src) const
{
    if (!venusRtlScalarTiming)
        return false;

    ThreadContext *thread = cpu.getContext(tid);
    bool found = false;
    inputBuffer[tid].visit([&](const ForwardInstData &packet) {
        for (unsigned int index = 0; !found && index < packet.width();
             ++index) {
            const MinorDynInstPtr candidate = packet.insts[index];
            RegIndex rd;
            if (candidate &&
                scalar600IntDestination(candidate, thread, rd) &&
                rd == src) {
                found = true;
            }
        }
    });

    return found;
}

bool
Execute::rtlIdReadIntReg(MinorDynInstPtr inst, RegIndex src,
    uint32_t &value)
{
    const ThreadID tid = inst->id.threadId;
    ThreadContext *thread = cpu.getContext(tid);
    /*
     * An instruction still in Minor's Execute input queue has not entered
     * scalar600 EX, so its result cannot participate in the RTL ID bypass.
     * Hold a control instruction until the youngest matching producer has
     * actually issued; rtlIdWriters below then evaluates the real ordered
     * EX-forward chain.  Computing through several queued producers here
     * creates a non-existent multi-level combinational bypass and redirects
     * branches too early.
     */
    if (rtlIdHasQueuedWriter(tid, src))
        return false;

    auto &writers = rtlIdWriters[tid];
    for (auto writer = writers.rbegin(); writer != writers.rend(); ++writer) {
        if (writer->rd != src)
            continue;

        /* RTL gives a matching EX load/LR/SC priority over both the RF and
         * an x0 read.  Non-load x0 writes still read as architectural zero. */
        if (writer->loadLike)
            return false;

        if (!writer->valueKnown) {
            const RtlDividerState &divider = executeInfo[tid].rtlDivider;
            if (writer->inst->staticInst->opClass() == enums::IntDiv &&
                divider.pending &&
                divider.pendingSeqNum == writer->inst->id.execSeqNum &&
                cpu.curCycle() + Cycles(1) >=
                    divider.pendingReadyCycle) {
                /* scalar600 exposes divider final_res to ID on the completion
                 * edge.  Decode runs one modeled edge ahead of retirement,
                 * so make that registered EX value visible here instead of
                 * waiting for architectural RF writeback. */
                writer->valueKnown = true;
                writer->value = divider.pendingResult;
            }
        }

        if (!writer->valueKnown) {
            /* A dependent ALU may already occupy EX while an older load is
             * completing.  Once that load has crossed WB/RF, scalar600's
             * combinational final_res becomes visible to the branch in ID
             * without waiting for the ALU's own WB edge. */
            const uint32_t bits = static_cast<uint32_t>(
                writer->inst->staticInst->getEMI());
            uint32_t rs1 = 0;
            uint32_t rs2 = 0;
            const bool rs1_known = rtlIdReadExReg(
                tid, (bits >> 15) & 0x1fU, rs1);
            const bool rs2_known = rtlIdReadExReg(
                tid, (bits >> 20) & 0x1fU, rs2);
            const RtlExValue refreshed = scalar600ExValue(
                writer->inst, rs1, rs1_known, rs2, rs2_known);
            if (!refreshed.known || refreshed.loadLike)
                return false;
            writer->valueKnown = true;
            writer->value = refreshed.value;
        }

        value = (src == 0) ? 0 : writer->value;
        return true;
    }

    if (src == 0) {
        value = 0;
        return true;
    }

    value = static_cast<uint32_t>(
        thread->getReg(RiscvISA::intRegClass[src]));
    return true;
}

Execute::RtlIdControlOperands
Execute::rtlIdReadControlOperands(MinorDynInstPtr inst, bool need_rs1,
    bool need_rs2)
{
    RtlIdControlOperands operands;
    if (!inst || inst->isFault())
        return operands;

    const uint32_t bits =
        static_cast<uint32_t>(inst->staticInst->getEMI());
    const RegIndex rs1 = (bits >> 15) & 0x1fU;
    const RegIndex rs2 = (bits >> 20) & 0x1fU;

    if (need_rs1 && !rtlIdReadIntReg(inst, rs1, operands.rs1))
        return operands;
    if (need_rs2 && !rtlIdReadIntReg(inst, rs2, operands.rs2))
        return operands;

    operands.ready = true;
    return operands;
}

void
Execute::tryToBranch(MinorDynInstPtr inst, Fault fault, BranchData &branch)
{
    ThreadContext *thread = cpu.getContext(inst->id.threadId);
    const std::unique_ptr<PCStateBase> pc_before(inst->pc->clone());
    std::unique_ptr<PCStateBase> target(thread->pcState().clone());

    /* Force a branch for SerializeAfter/SquashAfter instructions
     * at the end of micro-op sequence when we're not suspended */
    const uint32_t inst_bits = inst->isInst() ?
        static_cast<uint32_t>(inst->staticInst->getEMI()) : 0;
    const bool rtl_read_only_csr = venusRtlScalarTiming &&
        (inst_bits & 0x7fU) == 0x73U && ((inst_bits >> 12) & 0x7U) == 0x2U;
    const bool rtl_fence_nop = venusRtlScalarTiming &&
        (inst_bits & 0x7fU) == 0x0fU;
    const bool rtl_ecall_nop = venusRtlScalarTiming &&
        inst_bits == 0x00000073U;
    const bool rtl_decode_nop = venusRtlScalarTiming &&
        scalar600DecodeFallback(inst_bits) !=
            Scalar600DecodeFallback::None;
    bool force_branch = thread->status() != ThreadContext::Suspended &&
        !inst->isFault() &&
        inst->isLastOpInInst() &&
        !rtl_read_only_csr &&
        !rtl_fence_nop &&
        !rtl_ecall_nop &&
        !rtl_decode_nop &&
        (inst->staticInst->isSerializeAfter() ||
         inst->staticInst->isSquashAfter());

    DPRINTF(Branch, "tryToBranch before: %s after: %s%s\n",
        *pc_before, *target, (force_branch ? " (forcing)" : ""));

    /* Will we change the PC to something other than the next instruction? */
    bool must_branch = *pc_before != *target ||
        fault != NoFault ||
        force_branch;

    /* The reason for the branch data we're about to generate, set below */
    BranchData::Reason reason = BranchData::NoBranch;

    if (fault == NoFault) {
        inst->staticInst->advancePC(*target);
        thread->pcState(*target);

        if (venusRtlScalarTiming && inst->isInst() &&
            (inst_bits & 0x7fU) == 0x67U && inst->predictedTaken) {
            /*
             * scalar600_id_stage forms JALR target_address as rs1 + imm
             * without the architectural bit-zero clear.  Decode has already
             * captured that exact forwarded target in predictedTarget.  The
             * generic RISC-V execute semantic clears bit zero and would emit
             * a second, later correction redirect for an odd target; retain
             * scalar600's ID result for the RTL timing profile instead.
             */
            set(*target, *inst->predictedTarget);
            thread->pcState(*target);
            /* The generic unknown-instruction semantic used by a lax JALR
             * override never marked the architectural PC as changed before
             * must_branch was sampled above.  The ID redirect is nevertheless
             * a real, correctly predicted scalar600 branch. */
            must_branch = true;
        }

        DPRINTF(Branch, "Advancing current PC from: %s to: %s\n",
            *pc_before, *target);
    }

    if (inst->predictedTaken && !force_branch) {
        /* Predicted to branch */
        if (!must_branch) {
            /* No branch was taken, change stream to get us back to the
             *  intended PC value */
            DPRINTF(Branch, "Predicted a branch from 0x%x to 0x%x but"
                " none happened inst: %s\n",
                inst->pc->instAddr(), inst->predictedTarget->instAddr(),
                *inst);

            reason = BranchData::BadlyPredictedBranch;
        } else if (*inst->predictedTarget == *target) {
            /* Branch prediction got the right target, kill the branch and
             *  carry on.
             *  Note that this information to the branch predictor might get
             *  overwritten by a "real" branch during this cycle */
            DPRINTF(Branch, "Predicted a branch from 0x%x to 0x%x correctly"
                " inst: %s\n",
                inst->pc->instAddr(), inst->predictedTarget->instAddr(),
                *inst);

            reason = BranchData::CorrectlyPredictedBranch;
        } else {
            /* Branch prediction got the wrong target */
            DPRINTF(Branch, "Predicted a branch from 0x%x to 0x%x"
                    " but got the wrong target (actual: 0x%x) inst: %s\n",
                    inst->pc->instAddr(), inst->predictedTarget->instAddr(),
                    target->instAddr(), *inst);

            reason = BranchData::BadlyPredictedBranchTarget;
        }
    } else if (must_branch) {
        /* Unpredicted branch */
        DPRINTF(Branch, "Unpredicted branch from 0x%x to 0x%x inst: %s\n",
            inst->pc->instAddr(), target->instAddr(), *inst);

        reason = BranchData::UnpredictedBranch;
    } else {
        /* No branch at all */
        reason = BranchData::NoBranch;
    }

    updateBranchData(inst->id.threadId, reason, inst, *target, branch);
}

void
Execute::updateBranchData(
    ThreadID tid,
    BranchData::Reason reason,
    MinorDynInstPtr inst, const PCStateBase &target,
    BranchData &branch)
{
    if (reason != BranchData::NoBranch) {
        /* Bump up the stream sequence number on a real branch*/
        if (BranchData::isStreamChange(reason))
            executeInfo[tid].streamSeqNum++;

        /* Branches (even mis-predictions) don't change the predictionSeqNum,
         *  just the streamSeqNum */
        branch = BranchData(reason, tid,
            executeInfo[tid].streamSeqNum,
            /* Maintaining predictionSeqNum if there's no inst is just a
             * courtesy and looks better on minorview */
            (inst->isBubble() ? executeInfo[tid].lastPredictionSeqNum
                : inst->id.predictionSeqNum),
            target, inst);

        DPRINTF(Branch, "Branch data signalled: %s\n", branch);
    }
}

void
Execute::handleMemResponse(MinorDynInstPtr inst,
    LSQ::LSQRequestPtr response, BranchData &branch, Fault &fault)
{
    ThreadID thread_id = inst->id.threadId;
    ThreadContext *thread = cpu.getContext(thread_id);

    ExecContext context(cpu, *cpu.threads[thread_id], *this, inst);

    PacketPtr packet = response->packet;

    bool is_load = inst->staticInst->isLoad();
    bool is_store = inst->staticInst->isStore();
    bool is_atomic = inst->staticInst->isAtomic();
    bool is_prefetch = inst->staticInst->isDataPrefetch();

    /* If true, the trace's predicate value will be taken from the exec
     *  context predicate, otherwise, it will be set to false */
    bool use_context_predicate = true;

    if (inst->translationFault != NoFault) {
        /* Invoke memory faults. */
        DPRINTF(MinorMem, "Completing fault from DTLB access: %s\n",
            inst->translationFault->name());

        if (inst->staticInst->isPrefetch()) {
            DPRINTF(MinorMem, "Not taking fault on prefetch: %s\n",
                inst->translationFault->name());

            /* Don't assign to fault */
        } else {
            /* Take the fault raised during the TLB/memory access */
            fault = inst->translationFault;

            fault->invoke(thread, inst->staticInst);
        }
    } else if (!packet) {
        DPRINTF(MinorMem, "Completing failed request inst: %s\n",
            *inst);
        use_context_predicate = false;
        if (!context.readMemAccPredicate())
            inst->staticInst->completeAcc(nullptr, &context, inst->traceData);
    } else if (packet->isError()) {
        DPRINTF(MinorMem, "Trying to commit error response: %s\n",
            *inst);

        fatal("Received error response packet for inst: %s\n", *inst);
    } else if (is_store || is_load || is_prefetch || is_atomic) {
        assert(packet);

        DPRINTF(MinorMem, "Memory response inst: %s addr: 0x%x size: %d\n",
            *inst, packet->getAddr(), packet->getSize());

        if (is_load && packet->getSize() > 0) {
            DPRINTF(MinorMem, "Memory data[0]: 0x%x\n",
                static_cast<unsigned int>(packet->getConstPtr<uint8_t>()[0]));
        }

        /* Complete the memory access instruction */
        fault = inst->staticInst->completeAcc(packet, &context,
            inst->traceData);

        if (fault != NoFault) {
            /* Invoke fault created by instruction completion */
            DPRINTF(MinorMem, "Fault in memory completeAcc: %s\n",
                fault->name());
            fault->invoke(thread, inst->staticInst);
        } else {
            if (is_store && packet->getSize() > 0) {
                /*
                 * The tile-manager control page observes a scalar store
                 * only when the LSU transaction completes.  In particular,
                 * TASK_DONE clears venustile_softresetreg on this edge; doing
                 * it from ExecContext::writeMem() instead fires the MMIO
                 * side effect at request launch, before scalar600's real
                 * response/commit boundary.  Keep the ordinary memory write
                 * functional path unchanged and notify the control-plane
                 * model here, using the completed packet and its architectural
                 * byte enables.  This applies to every store to the register,
                 * independent of task, PC, address provenance, or workload.
                 */
                cpu.commitVenusTaskDone(
                    thread, packet->req->getVaddr(), packet->getSize(),
                    packet->getConstPtr<uint8_t>(),
                    packet->req->getByteEnable());
            }
            /* Stores need to be pushed into the store buffer to finish
             *  them off */
            if (response->needsToBeSentToStoreBuffer())
                lsq.sendStoreToStoreBuffer(response);
        }
    } else {
        fatal("There should only ever be reads, "
            "writes or faults at this point\n");
    }

    lsq.popResponse(response);

    if (inst->traceData) {
        inst->traceData->setPredicate((use_context_predicate ?
            context.readPredicate() : false));
    }

    doInstCommitAccounting(inst);

    if (is_load &&
        executeInfo[thread_id].previousIssuedLoad == inst) {
        executeInfo[thread_id].previousIssuedLoadCommitted = true;
        executeInfo[thread_id].previousIssuedLoadCommitCycle =
            cpu.curCycle();
    }

    /* Generate output to account for branches */
    tryToBranch(inst, fault, branch);
}

bool
Execute::isInterrupted(ThreadID thread_id) const
{
    BaseInterrupts *controller = cpu.getInterruptController(thread_id);
    return controller && controller->checkInterrupts();
}

bool
Execute::takeInterrupt(ThreadID thread_id, BranchData &branch)
{
    DPRINTF(MinorInterrupt, "Considering interrupt status from PC: %s\n",
        cpu.getContext(thread_id)->pcState());

    Fault interrupt = cpu.getInterruptController(thread_id)->getInterrupt();

    if (interrupt != NoFault) {
        /* The interrupt *must* set pcState */
        cpu.getInterruptController(thread_id)->updateIntrInfo();
        interrupt->invoke(cpu.getContext(thread_id));

        assert(!lsq.accessesInFlight());

        DPRINTF(MinorInterrupt, "Invoking interrupt: %s to PC: %s\n",
            interrupt->name(), cpu.getContext(thread_id)->pcState());

        /* Assume that an interrupt *must* cause a branch.  Assert this? */

        updateBranchData(thread_id, BranchData::Interrupt,
            MinorDynInst::bubble(), cpu.getContext(thread_id)->pcState(),
            branch);
    }

    return interrupt != NoFault;
}

bool
Execute::executeMemRefInst(MinorDynInstPtr inst, BranchData &branch,
    bool &passed_predicate, Fault &fault)
{
    bool issued = false;

    /* Set to true if the mem op. is issued and sent to the mem system */
    passed_predicate = false;

    if (!lsq.canRequest()) {
        /* Not acting on instruction yet as the memory
         * queues are full */
        issued = false;
    } else {
        ThreadContext *thread = cpu.getContext(inst->id.threadId);
        std::unique_ptr<PCStateBase> old_pc(thread->pcState().clone());

        ExecContext context(cpu, *cpu.threads[inst->id.threadId], *this, inst);

        DPRINTF(MinorExecute, "Initiating memRef inst: %s\n", *inst);

        Fault init_fault = inst->staticInst->initiateAcc(&context,
            inst->traceData);

        if (inst->inLSQ) {
            if (init_fault != NoFault) {
                assert(inst->translationFault != NoFault);
                // Translation faults are dealt with in handleMemResponse()
                init_fault = NoFault;
            } else {
                // If we have a translation fault then it got suppressed  by
                // initateAcc()
                inst->translationFault = NoFault;
            }
        }

        if (init_fault != NoFault) {
            DPRINTF(MinorExecute, "Fault on memory inst: %s"
                " initiateAcc: %s\n", *inst, init_fault->name());
            fault = init_fault;
        } else {
            /* Only set this if the instruction passed its
             * predicate */
            if (!context.readMemAccPredicate()) {
                DPRINTF(MinorMem, "No memory access for inst: %s\n", *inst);
                assert(context.readPredicate());
            }
            passed_predicate = context.readPredicate();

            /* Set predicate in tracing */
            if (inst->traceData)
                inst->traceData->setPredicate(passed_predicate);

            /* If the instruction didn't pass its predicate
             * or it is a predicated vector instruction and the
             * associated predicate register is all-false (and so will not
             * progress from here)  Try to branch to correct and branch
             * mis-prediction. */
            if (!inst->inLSQ) {
                /* Leave it up to commit to handle the fault */
                lsq.pushFailedRequest(inst);
                inst->inLSQ = true;
            }
        }

        /* Restore thread PC */
        thread->pcState(*old_pc);
        issued = true;
    }

    return issued;
}

/** Increment a cyclic buffer index for indices [0, cycle_size-1] */
inline unsigned int
cyclicIndexInc(unsigned int index, unsigned int cycle_size)
{
    unsigned int ret = index + 1;

    if (ret == cycle_size)
        ret = 0;

    return ret;
}

/** Decrement a cyclic buffer index for indices [0, cycle_size-1] */
inline unsigned int
cyclicIndexDec(unsigned int index, unsigned int cycle_size)
{
    int ret = index - 1;

    if (ret < 0)
        ret = cycle_size - 1;

    return ret;
}

void
Execute::stepVenusBarrierInId()
{
    panic_if(!venusBarrierInId || venusBarrierThreadId == InvalidThreadID,
        "stepping scalar600 VBARRIER without an ID owner");
    panic_if(!venusBarrierIssueBlocked,
        "scalar600 VBARRIER owner exists without its issue gate");

    bool pass_through = false;
    if (!cpu.stepVenusBarrierFsm(pass_through)) {
        DPRINTF(MinorExecute,
            "VBARRIER ID hold: cycle=%llu delayed_busy=%d owner=%s\n",
            static_cast<unsigned long long>(cpu.curCycle()),
            cpu.rtlScalarBarrierBusyDelayed(), *venusBarrierInId);
        return;
    }

    MinorDynInstPtr released = venusBarrierInId;
    const ThreadID released_thread = venusBarrierThreadId;
    const bool direct_id_capture = venusBarrierDirectIdCapture;
    released->venusBarrierArmed = true;
    /*
     * The persistent zero-counter pass-through is already at RTL's ID
     * acceptance edge.  Applying the ordinary release-to-retire boundary
     * again would double-count three scalar clocks.  An armed barrier still
     * needs the registered scalar600 release pipeline.
     */
    released->minimumCommitCycle = pass_through ? cpu.curCycle() :
        cpu.curCycle() + Cycles(cpu.rtlBarrierIdleReleaseToCommit());
    released->venusBarrierLateBusyRecheck =
        !pass_through && cpu.rtlBarrierLateBusyRecheck();
    venusBarrierInId = nullptr;
    venusBarrierThreadId = InvalidThreadID;
    venusBarrierFirstStepCycle = Cycles(0);
    venusBarrierDirectIdCapture = false;
    if (direct_id_capture && !pass_through) {
        venusDirectBarrierRequestWindow[released_thread] = true;
        venusDirectBarrierRequestEpoch[released_thread] = true;
    }
    if (pass_through) {
        /*
         * With counter==0 and barrier_valid==0, RTL vector_wait_req is
         * combinationally low before this edge.  ID may therefore accept the
         * following instruction on the same edge as the barrier advances.
         * Keeping the issue gate through the current Evaluate adds one false
         * scalar clock.  Armed releases remain registered below.
         */
        venusBarrierIssueBlocked = false;
        venusBarrierReleasePending = false;
    } else {
        venusBarrierReleasePending = true;
    }
    DPRINTF(MinorExecute,
        "VBARRIER ID release: cycle=%llu retire_cycle=%llu pass_through=%d "
        "owner=%s\n",
        static_cast<unsigned long long>(cpu.curCycle()),
        static_cast<unsigned long long>(released->minimumCommitCycle),
        pass_through,
        *released);
}

void
Execute::tryPresentRtlVenusRequestOnIssue(MinorDynInstPtr request)
{
    if (!venusRtlScalarTiming || !request || !request->isInst() ||
        request->venusPktSent) {
        return;
    }

    const auto *venus = dynamic_cast<const RiscvISA::VenusStaticInst *>(
        request->staticInst.get());
    if (!venus || venus->venusOp != RiscvISA::VENUSEXT)
        return;

    const ThreadID tid = request->id.threadId;
    ExecuteThreadInfo &info = executeInfo[tid];
    if (request->id.streamSeqNum != info.streamSeqNum)
        return;

    bool older_extension_state_pending = false;
    info.inFlightInsts->visit([&](const QueuedInst &queued) {
        const MinorDynInstPtr &older = queued.inst;
        if (older_extension_state_pending || !older || !older->isInst() ||
            older->id.execSeqNum >= request->id.execSeqNum) {
            return;
        }
        const auto *older_venus = dynamic_cast<
            const RiscvISA::VenusStaticInst *>(older->staticInst.get());
        if (!older_venus)
            return;

        /*
         * VSETCSR/VSETCSRIMM update the same scalar600 ID-stage registers
         * carried in the outgoing request, while an unsent older VENUSEXT
         * still owns the one-entry scalar request channel.  Until those
         * physical predecessors have been published, using architectural
         * WB state here would be speculative.  Ordinary scalar ALU writers
         * are handled by rtlIdReadExReg's real EX bypass below.
         */
        if (older_venus->venusOp == RiscvISA::VSETCSR ||
            older_venus->venusOp == RiscvISA::VSETCSRIMM ||
            (older_venus->venusOp == RiscvISA::VENUSEXT &&
             !older->venusPktSent)) {
            older_extension_state_pending = true;
        }
    });
    if (older_extension_state_pending || cpu.isVenusPortBlocked())
        return;

    const uint64_t bits = venus->venusInstBits;
    const RegIndex vs1_id = (bits >> 32) & 0x1fU;
    const RegIndex avl_id = (bits >> 7) & 0x1fU;
    uint32_t vs1 = 0;
    uint32_t avl = 0;
    if (!rtlIdReadExReg(tid, vs1_id, vs1) ||
        !rtlIdReadExReg(tid, avl_id, avl)) {
        return;
    }

    /*
     * Minor decodes the two physical 32-bit Venus words into one dynamic
     * instruction.  This issue edge is the scalar600 ID capture boundary;
     * VenusSequencer's rtl_scalar_second_word_boundary then supplies the
     * registered two-word presentation interval before the request can be
     * granted.  Sending here therefore models pipeline overlap, not an
     * opcode latency shortcut.
     */
    if (!cpu.sendVenusInstrPkt(request, vs1, avl)) {
        request->venusRequestBackpressured = true;
        rtlScalarIdStopRequest = true;
        rtlScalarNonDividerIdStopRequest = true;
        DPRINTF(MinorExecute,
            "scalar600 VENUSEXT physical ID presentation backpressured: "
            "%s\n", *request);
        return;
    }

    request->venusPktSent = true;
    venusRequestFollowerWindow[tid] = true;
    venusRequestAcceptedCycle[tid] = cpu.curCycle();
    DPRINTF(MinorExecute,
        "scalar600 VENUSEXT request presented at physical ID issue: %s\n",
        *request);
}

void
Execute::tryPresentRtlVenusRequest(ThreadID tid)
{
    if (!venusRtlScalarTiming)
        return;

    ExecuteThreadInfo &info = executeInfo[tid];
    if (info.inFlightInsts->empty())
        return;

    const MinorDynInstPtr &request = info.inFlightInsts->front().inst;
    if (!request || !request->isInst() || request->venusPktSent ||
        !request->venusRequestMayPresentAheadOfCommit ||
        request->id.streamSeqNum != info.streamSeqNum) {
        return;
    }

    const auto *venus = dynamic_cast<const RiscvISA::VenusStaticInst *>(
        request->staticInst.get());
    panic_if(!venus || venus->venusOp != RiscvISA::VENUSEXT,
        "scalar600 ID request marker attached to non-VENUSEXT %s",
        *request);

    /*
     * scalar600 drives req_v from the two-word instruction in ID while the
     * immediately older instruction retires in WB.  Keep architectural
     * retirement at one instruction per edge, but present the already-head
     * request after the predecessor's side effects (notably VSETCSR) have
     * become visible on this edge.
     */
    if (cpu.isVenusPortBlocked()) {
        DPRINTF(MinorExecute,
            "scalar600 VENUSEXT ID presentation waits for blocked "
            "port: %s\n", *request);
        return;
    }

    ThreadContext *thread = cpu.getContext(tid);
    const auto &reg_classes = thread->getIsaPtr()->regClasses();
    const uint64_t bits = venus->venusInstBits;
    const uint8_t vs1_id = (bits >> 32) & 0x1f;
    const uint8_t avl_id = (bits >> 7) & 0x1f;
    uint32_t vs1 = 0;
    uint32_t avl = 0;
    if (vs1_id != 0) {
        const RegId reg = (*reg_classes.at(IntRegClass))[vs1_id];
        vs1 = thread->getReg(reg);
    }
    if (avl_id != 0) {
        const RegId reg = (*reg_classes.at(IntRegClass))[avl_id];
        avl = thread->getReg(reg);
    }

    if (!cpu.sendVenusInstrPkt(request, vs1, avl)) {
        request->venusRequestBackpressured = true;
        DPRINTF(MinorExecute,
            "scalar600 VENUSEXT ID presentation backpressured: %s\n",
            *request);
        return;
    }

    request->venusPktSent = true;
    venusRequestFollowerWindow[tid] = true;
    venusRequestAcceptedCycle[tid] = cpu.curCycle();
    DPRINTF(MinorExecute,
        "scalar600 VENUSEXT request presented from ID ahead of retirement: "
        "%s\n", *request);
}

unsigned int
Execute::issue(ThreadID thread_id)
{
    const ForwardInstData *insts_in = getInput(thread_id);
    ExecuteThreadInfo &thread = executeInfo[thread_id];

    /* Early termination if we have no instructions */
    if (!insts_in)
        return 0;

    if (venusBarrierIssueBlocked) {
        DPRINTF(MinorExecute,
            "VBARRIER blocks scalar issue for thread %d\n", thread_id);
        return 0;
    }

    /* Start from the first FU */
    unsigned int fu_index = 0;

    /* Remains true while instructions are still being issued.  If any
     *  instruction fails to issue, this is set to false and we exit issue.
     *  This strictly enforces in-order issue.  For other issue behaviours,
     *  a more complicated test in the outer while loop below is needed. */
    bool issued = true;

    /* Number of insts issues this cycle to check for issueLimit */
    unsigned num_insts_issued = 0;

    /* Number of memory ops issues this cycle to check for memoryIssueLimit */
    unsigned num_mem_insts_issued = 0;

    do {
        MinorDynInstPtr inst = insts_in->insts[thread.inputIndex];
        Fault fault = inst->fault;
        bool discarded = false;
        bool issued_mem_ref = false;

        // Venus instructions may hold the commit head while the external
        // sequencer applies backpressure.  The upstream Minor model sizes
        // this queue from FU latency and assumes commit cannot remain
        // blocked indefinitely, so without an explicit capacity check it
        // overflows and admits wrong-path instructions beyond an EBREAK.
        // Apply ordinary in-order backpressure instead of silently pushing
        // beyond the queue capacity.
        if (!inst->isBubble() && !thread.inFlightInsts->canReserve()) {
            DPRINTF(MinorExecute, "Can't issue inst: %s; in-flight queue "
                    "is full\n", *inst);
            issued = false;
        } else if (inst->isBubble()) {
            /* Skip */
            issued = true;
        } else if (cpu.getContext(thread_id)->status() ==
            ThreadContext::Suspended)
        {
            DPRINTF(MinorExecute, "Discarding inst: %s from suspended"
                " thread\n", *inst);

            issued = true;
            discarded = true;
        } else if (inst->id.streamSeqNum != thread.streamSeqNum) {
            DPRINTF(MinorExecute, "Discarding inst: %s as its stream"
                " state was unexpected, expected: %d\n",
                *inst, thread.streamSeqNum);
            issued = true;
            discarded = true;
        } else if (venusRtlScalarTiming &&
            venusRequestFollowerWindow[thread_id] &&
            venusRequestAcceptedCycle[thread_id] == cpu.curCycle() &&
            inst->isInst() && inst->staticInst->isMemRef()) {
            /*
             * A memory instruction immediately following an accepted
             * two-word Venus request cannot enter scalar600 EX on the
             * request handshake edge.  Leave it as the unconsumed follower;
             * it will issue after the registered release next cycle.
             */
            DPRINTF(MinorExecute,
                "scalar600 Venus request holds immediate LSU follower in "
                "ID: %s\n", *inst);
            issued = false;
        } else {
            /* Try and issue an instruction into an FU, assume we didn't and
             * fix that in the loop */
            issued = false;

            /* Try FU from 0 each instruction */
            fu_index = 0;

            /* Try and issue a single instruction stepping through the
             *  available FUs */
            do {
                FUPipeline *fu = funcUnits[fu_index];

                DPRINTF(MinorExecute, "Trying to issue inst: %s to FU: %d\n",
                    *inst, fu_index);

                /* Does the examined fu have the OpClass-related capability
                 *  needed to execute this instruction?  Faults can always
                 *  issue to any FU but probably should just 'live' in the
                 *  inFlightInsts queue rather than having an FU. */
                bool fu_is_capable = (!inst->isFault() ?
                    fu->provides(inst->staticInst->opClass()) : true);

                if (inst->isNoCostInst() && venusRtlScalarTiming &&
                    thread.previousIssuedWasLoad &&
                    thread.previousIssuedLoad &&
                    rtlPreviousLoadFeedsInst(thread_id, inst) &&
                    (!thread.previousIssuedLoadCommitted ||
                     cpu.curCycle() <=
                         thread.previousIssuedLoadCommitCycle)) {
                    /* Unknown encodings are no-cost instructions in gem5,
                     * but scalar600's invalid LOAD/STORE/BRANCH decoder
                     * defaults can retain operand read enables.  Their
                     * load-use interlock is evaluated before the datapath
                     * later treats the operation as a NOP/write-zero. */
                    DPRINTF(MinorExecute,
                        "Can't issue no-cost scalar600 load-use consumer "
                        "%s before registered WB/RF visibility\n", *inst);
                } else if (inst->isNoCostInst()) {
                    /* Issue free insts. to a fake numbered FU */
                    fu_index = noCostFUIndex;

                    /* And start the countdown on activity to allow
                     *  this instruction to get to the end of its FU */
                    cpu.activityRecorder->activity();

                    /* Mark the destinations for this instruction as
                     *  busy */
                    scoreboard[thread_id].markupInstDests(inst, cpu.curCycle() +
                        Cycles(0), cpu.getContext(thread_id), false);

                    DPRINTF(MinorExecute, "Issuing %s to %d\n", inst->id, noCostFUIndex);
                    inst->fuIndex = noCostFUIndex;
                    inst->extraCommitDelay = Cycles(0);
                    inst->extraCommitDelayExpr = NULL;

                    /* Push the instruction onto the inFlight queue so
                     *  it can be committed in order */
                    QueuedInst fu_inst(inst);
                    thread.inFlightInsts->push(fu_inst);

                    issued = true;

                } else if (!fu_is_capable || fu->alreadyPushed()) {
                    /* Skip */
                    if (!fu_is_capable) {
                        DPRINTF(MinorExecute, "Can't issue as FU: %d isn't"
                            " capable\n", fu_index);
                    } else {
                        DPRINTF(MinorExecute, "Can't issue as FU: %d is"
                            " already busy\n", fu_index);
                    }
                } else if (fu->stalled) {
                    DPRINTF(MinorExecute, "Can't issue inst: %s into FU: %d,"
                        " it's stalled\n",
                        *inst, fu_index);
                } else if (!fu->canInsert()) {
                    DPRINTF(MinorExecute, "Can't issue inst: %s to busy FU"
                        " for another: %d cycles\n",
                        *inst, fu->cyclesBeforeInsert());
                } else {
                    MinorFUTiming *timing = (!inst->isFault() ?
                        fu->findTiming(inst->staticInst) : NULL);

                    const std::vector<Cycles> *src_latencies =
                        (timing ? &(timing->srcRegsRelativeLats)
                            : NULL);

                    const std::vector<bool> *cant_forward_from_fu_indices =
                        &(fu->cantForwardFromFUIndices);

                    const bool previous_load_feeds_inst =
                        rtlPreviousLoadFeedsInst(thread_id, inst);

                    if (timing && timing->suppress) {
                        DPRINTF(MinorExecute, "Can't issue inst: %s as extra"
                            " decoding is suppressing it\n",
                            *inst);
                    } else if (venusRtlScalarTiming && !inst->isFault() &&
                        inst->staticInst->opClass() == enums::IntDiv &&
                        !rtlDividerOperandsReady(inst)) {
                        /*
                         * scalar600 starts DIV/REM from the values visible
                         * at its ID/EX boundary.  Minor may consider a chain
                         * forwardable before its oldest load or multi-cycle
                         * producer has actually supplied a value.  Hold the
                         * divider request until the complete bypass chain is
                         * known; planning it from stale architectural RF
                         * corrupts both zero-divisor timing and writeback.
                         */
                        DPRINTF(MinorExecute, "Can't issue DIV/REM %s before"
                            " ID/EX operands are available\n", *inst);
                    } else if (venusRtlScalarTiming &&
                        thread.previousIssuedWasLoad &&
                        thread.previousIssuedLoad &&
                        previous_load_feeds_inst &&
                        (!thread.previousIssuedLoadCommitted ||
                         cpu.curCycle() <=
                             thread.previousIssuedLoadCommitCycle))
                    {
                        /*
                         * scalar600's load result becomes visible to a RAW
                         * consumer through the registered WB/RF boundary on
                         * the edge after LSU completion.  Minor commits before
                         * issue and would otherwise let the dependent
                         * instruction consume the response on that same edge.
                         * This applies to every consumer opcode; DIV/REM is
                         * merely the longest observable instance of the same
                         * load-use boundary.
                         */
                        DPRINTF(MinorExecute, "Can't issue load-use consumer "
                            "%s before registered WB/RF visibility\n", *inst);
                    } else if (!scoreboard[thread_id].canInstIssue(inst,
                        src_latencies, cant_forward_from_fu_indices,
                        cpu.curCycle(), cpu.getContext(thread_id)))
                    {
                        DPRINTF(MinorExecute, "Can't issue inst: %s yet\n",
                            *inst);
                    } else {
                        /* Can insert the instruction into this FU */
                        DPRINTF(MinorExecute, "Issuing inst: %s"
                            " into FU %d\n", *inst,
                            fu_index);

                        Cycles extra_dest_retire_lat = Cycles(0);
                        TimingExpr *extra_dest_retire_lat_expr = NULL;
                        Cycles extra_assumed_lat = Cycles(0);

                        /* Add the extraCommitDelay and extraAssumeLat to
                         *  the FU pipeline timings */
                        if (timing) {
                            extra_dest_retire_lat =
                                timing->extraCommitLat;
                            extra_dest_retire_lat_expr =
                                timing->extraCommitLatExpr;
                            extra_assumed_lat =
                                timing->extraAssumedLat;
                        }

                        /*
                         * A scalar600 store holds WB for one cycle before the
                         * LSU's all-ones completion count releases it.  Minor
                         * otherwise retires a destination-less store in the
                         * issue cycle when it hands the request to its store
                         * buffer.
                         */
                        if (venusRtlScalarTiming && !inst->isFault() &&
                            inst->staticInst->isStore()) {
                            extra_dest_retire_lat += Cycles(1);
                        }

                        if (venusRtlScalarTiming && !inst->isFault() &&
                            inst->staticInst->isStoreConditional()) {
                                /* scalar600 routes SC.W through the same
                                 * five-state registered LSU response path as
                                 * a load, rather than the short store path. */
                                extra_dest_retire_lat += Cycles(3);
                        }

                        issued_mem_ref = inst->isMemRef();

                        QueuedInst fu_inst(inst);

                        /* Decorate the inst with FU details */
                        inst->fuIndex = fu_index;
                        inst->extraCommitDelay = extra_dest_retire_lat;
                        inst->extraCommitDelayExpr =
                            extra_dest_retire_lat_expr;

                        if (issued_mem_ref) {
                            /* Remember which instruction this memory op
                             *  depends on so that initiateAcc can be called
                             *  early */
                            if (allowEarlyMemIssue) {
                                inst->instToWaitFor =
                                    scoreboard[thread_id].execSeqNumToWaitFor(inst,
                                        cpu.getContext(thread_id));

                                if (lsq.getLastMemBarrier(thread_id) >
                                    inst->instToWaitFor)
                                {
                                    DPRINTF(MinorExecute, "A barrier will"
                                        " cause a delay in mem ref issue of"
                                        " inst: %s until after inst"
                                        " %d(exec)\n", *inst,
                                        lsq.getLastMemBarrier(thread_id));

                                    inst->instToWaitFor =
                                        lsq.getLastMemBarrier(thread_id);
                                } else {
                                    DPRINTF(MinorExecute, "Memory ref inst:"
                                        " %s must wait for inst %d(exec)"
                                        " before issuing\n",
                                        *inst, inst->instToWaitFor);
                                }

                                inst->canEarlyIssue = true;
                            }
                            /* Also queue this instruction in the memory ref
                             *  queue to ensure in-order issue to the LSQ */
                            DPRINTF(MinorExecute, "Pushing mem inst: %s\n",
                                *inst);
                            thread.inFUMemInsts->push(fu_inst);
                        }

                        /* Issue to FU */
                        fu->push(fu_inst);
                        /* And start the countdown on activity to allow
                         *  this instruction to get to the end of its FU */
                        cpu.activityRecorder->activity();

                        /* Mark the destinations for this instruction as
                         *  busy */
                        scoreboard[thread_id].markupInstDests(inst, cpu.curCycle() +
                            fu->description.opLat +
                            extra_dest_retire_lat +
                            extra_assumed_lat,
                            cpu.getContext(thread_id),
                            issued_mem_ref && extra_assumed_lat == Cycles(0));

                        /* Push the instruction onto the inFlight queue so
                         *  it can be committed in order */
                        thread.inFlightInsts->push(fu_inst);

                        issued = true;
                    }
                }

                fu_index++;
            } while (fu_index != numFuncUnits && !issued);

            if (!issued)
                DPRINTF(MinorExecute, "Didn't issue inst: %s\n", *inst);
        }

        if (issued) {
            /* Record the value path at the same boundary that makes an
             * instruction visible to scalar600's EX-stage bypass. */
            if (!discarded && !inst->isBubble()) {
                rtlIdOnIssue(inst);
                tryPresentRtlVenusRequestOnIssue(inst);
            }

            if (!discarded && !inst->isBubble() &&
                venusRequestFollowerWindow[thread_id]) {
                if (inst->isInst() && inst->staticInst->isMemRef()) {
                    venusRequestFollowerLsuSeq[thread_id] =
                        inst->id.execSeqNum;
                    DPRINTF(MinorExecute,
                        "scalar600 marks Venus-request follower LSU "
                        "registered release: %s\n", *inst);
                }
                venusRequestFollowerWindow[thread_id] = false;
            }

            const bool issued_venus_barrier =
                !discarded && isVenusBarrierInst(inst);
            const auto *issued_venus =
                (!discarded && inst->isInst()) ?
                dynamic_cast<const RiscvISA::VenusStaticInst *>(
                    inst->staticInst.get()) : nullptr;
            if (issued_venus_barrier) {
                /* A newer barrier supersedes any unused request window. */
                venusDirectBarrierRequestWindow[thread_id] = false;
                venusDirectBarrierRequestEpoch[thread_id] = false;
                venusRequestFollowerWindow[thread_id] = false;
            } else if (venusRtlScalarTiming && issued_venus &&
                issued_venus->venusOp == RiscvISA::VENUSEXT) {
                /*
                 * scalar600 sources every Venus request from its physical
                 * ID stage after assembling the two 32-bit words.  The
                 * request is therefore allowed to handshake when it becomes
                 * the in-order head; it does not wait for the artificial
                 * retirement point of gem5's fused 64-bit instruction.
                 *
                 * The direct-barrier window remains useful for the separate
                 * barrier/LSU follower state below, but it is not a
                 * qualification predicate for ordinary ID presentation.
                 * This is a scalar-core pipeline rule for all VENUSEXT
                 * requests, independent of opcode, PC, task, or DAG.
                 */
                inst->venusRequestMayPresentAheadOfCommit = true;
                if (venusDirectBarrierRequestWindow[thread_id])
                    venusDirectBarrierRequestWindow[thread_id] = false;
                DPRINTF(MinorExecute,
                    "scalar600 marks VENUSEXT for physical ID "
                    "presentation: %s\n", *inst);
            }
            thread.previousIssuedWasLoad =
                !discarded && inst->isInst() &&
                inst->staticInst->isLoad();
            if (thread.previousIssuedWasLoad) {
                thread.previousIssuedLoad = inst;
                thread.previousIssuedLoadCommitted = false;
                thread.previousIssuedLoadCommitCycle = Cycles(0);
            }

            /* Generate MinorTrace's MinorInst lines.  Do this at commit
             *  to allow better instruction annotation? */
            if (debug::MinorTrace && !inst->isBubble()) {
                inst->minorTraceInst(*this);
            }

            /* Mark up barriers in the LSQ */
            if (!discarded && inst->isInst() &&
                inst->staticInst->isFullMemBarrier())
            {
                DPRINTF(MinorMem, "Issuing memory barrier inst: %s\n", *inst);
                lsq.issuedMemBarrierInst(inst);
            }

            if (inst->traceData && setTraceTimeOnIssue) {
                inst->traceData->setWhen(curTick());
            }

            if (issued_mem_ref)
                num_mem_insts_issued++;

            if (!discarded && !inst->isBubble()) {
                num_insts_issued++;

                if (num_insts_issued == issueLimit)
                    DPRINTF(MinorExecute, "Reached inst issue limit\n");
            }

            thread.inputIndex++;
            DPRINTF(MinorExecute, "Stepping to next inst inputIndex: %d\n",
                thread.inputIndex);

            if (issued_venus_barrier) {
                unsigned int older_venus_requests = 0;
                unsigned int older_unsent_venus_requests = 0;
                InstSeqNum youngest_older_venus_seq = 0;
                thread.inFlightInsts->visit(
                    [&](const QueuedInst &queued) {
                        const MinorDynInstPtr &older = queued.inst;
                        if (!older || !older->isInst() ||
                            older->id.execSeqNum >= inst->id.execSeqNum) {
                            return;
                        }
                        const auto *older_venus = dynamic_cast<
                            const RiscvISA::VenusStaticInst *>(
                                older->staticInst.get());
                        if (!older_venus ||
                            older_venus->venusOp != RiscvISA::VENUSEXT) {
                            return;
                        }
                        ++older_venus_requests;
                        youngest_older_venus_seq = std::max(
                            youngest_older_venus_seq,
                            older->id.execSeqNum);
                        if (!older->venusPktSent)
                            ++older_unsent_venus_requests;
                    });
                /*
                 * Bind the physical scalar600 ID-stage owner at the actual
                 * issue edge.  The persistent counter advances here and at
                 * the start of every later Execute edge, independently of
                 * Minor's in-order commit head.
                 */
                panic_if(venusBarrierInId || venusBarrierIssueBlocked,
                    "multiple scalar600 VBARRIER owners in ID");
                venusBarrierInId = inst;
                venusBarrierThreadId = thread_id;
                venusBarrierIssueBlocked = true;
                /*
                 * Minor may bind a younger barrier while an older fused
                 * VENUSEXT still owns scalar600's EX/WB request path.  In
                 * that case the historical extra edge maps the early Minor
                 * bind to the later physical ID capture.  When Venus is
                 * already idle and no older request is waiting to be sent,
                 * this instruction is the real ID owner now: the RTL
                 * counter's first decrement is visible on the immediately
                 * following scalar edge.  Use only pipeline ownership and
                 * synchronizer state; never task, PC, DAG, address or data.
                 */
                const bool direct_barrier_id_capture =
                    !cpu.rtlScalarBarrierBusyDelayed() &&
                    older_unsent_venus_requests == 0;
                venusBarrierDirectIdCapture =
                    direct_barrier_id_capture;
                venusBarrierFirstStepCycle =
                    cpu.curCycle() + Cycles(
                        direct_barrier_id_capture ? 1 : 2);
                /*
                 * Stop the current issue burst too: RTL ID cannot inject a
                 * younger instruction in the cycle in which it captures the
                 * barrier, even for wider non-RTL Minor configurations.
                 */
                DPRINTF(MinorExecute,
                    "VBARRIER issued: block younger thread %d instructions; "
                    "delayed_busy=%d older_venus=%u "
                    "older_unsent_venus=%u "
                    "youngest_older_venus_seq=%llu direct_capture=%d\n",
                    thread_id, cpu.rtlScalarBarrierBusyDelayed(),
                    older_venus_requests,
                    older_unsent_venus_requests,
                    static_cast<unsigned long long>(
                        youngest_older_venus_seq),
                    direct_barrier_id_capture);
                if (direct_barrier_id_capture) {
                    /*
                     * The Minor issue event for a direct capture is the
                     * scalar600 edge on which ID has already sampled the
                     * barrier.  At that edge vec_barrier_cnt executes its
                     * first non-blocking decrement.  Waiting until the next
                     * Execute::evaluate() shifts the whole six-edge window
                     * by one clock and can incorrectly observe a newly
                     * asserted venusidle_i_qqq at counter zero.
                     *
                     * Advance the persistent FSM on this edge.  The normal
                     * top-of-evaluate path resumes on the following edge;
                     * this is driven solely by physical ID ownership and is
                     * independent of task, PC, address, data, or opcode
                     * neighbourhood.
                     */
                    stepVenusBarrierInId();
                }
                /* Preserve the normal input-line pop below, then stop the
                 * surrounding in-order issue loop. */
                issued = false;
            }

        }

        /* Got to the end of a line */
        if (thread.inputIndex == insts_in->width()) {
            popInput(thread_id);
            /* Set insts_in to null to force us to leave the surrounding
             *  loop */
            insts_in = NULL;

            if (processMoreThanOneInput) {
                DPRINTF(MinorExecute, "Wrapping\n");
                insts_in = getInput(thread_id);
            }
        }
    } while (insts_in && thread.inputIndex < insts_in->width() &&
        /* We still have instructions */
        fu_index != numFuncUnits && /* Not visited all FUs */
        issued && /* We've not yet failed to issue an instruction */
        num_insts_issued != issueLimit && /* Still allowed to issue */
        num_mem_insts_issued != memoryIssueLimit);

    return num_insts_issued;
}

bool
Execute::tryPCEvents(ThreadID thread_id)
{
    ThreadContext *thread = cpu.getContext(thread_id);
    unsigned int num_pc_event_checks = 0;

    /* Handle PC events on instructions */
    Addr oldPC;
    do {
        oldPC = thread->pcState().instAddr();
        cpu.threads[thread_id]->pcEventQueue.service(oldPC, thread);
        num_pc_event_checks++;
    } while (oldPC != thread->pcState().instAddr());

    if (num_pc_event_checks > 1) {
        DPRINTF(PCEvent, "Acting on PC Event to PC: %s\n",
            thread->pcState());
    }

    return num_pc_event_checks > 1;
}

void
Execute::doInstCommitAccounting(MinorDynInstPtr inst)
{
    assert(!inst->isFault());

    MinorThread *thread = cpu.threads[inst->id.threadId];

    /* Increment the many and various inst and op counts in the
     *  thread and system */
    if (!inst->staticInst->isMicroop() || inst->staticInst->isLastMicroop())
    {
        thread->numInst++;
        thread->threadStats.numInsts++;
        cpu.commitStats[inst->id.threadId]->numInsts++;
        cpu.baseStats.numInsts++;

        /* Act on events related to instruction counts */
        thread->comInstEventQueue.serviceEvents(thread->numInst);
    }
    thread->numOp++;
    thread->threadStats.numOps++;
    cpu.commitStats[inst->id.threadId]->numOps++;
    cpu.commitStats[inst->id.threadId]
        ->committedInstType[inst->staticInst->opClass()]++;

    /* Set the CP SeqNum to the numOps commit number */
    if (inst->traceData)
        inst->traceData->setCPSeq(thread->numOp);

    cpu.probeInstCommit(inst->staticInst, inst->pc->instAddr());
}

bool
Execute::commitInst(MinorDynInstPtr inst, bool early_memory_issue,
    BranchData &branch, Fault &fault, bool &committed,
    bool &completed_mem_issue)
{
    ThreadID thread_id = inst->id.threadId;
    ThreadContext *thread = cpu.getContext(thread_id);

    bool completed_inst = true;
    fault = NoFault;

    /* Is the thread for this instruction suspended?  In that case, just
     *  stall as long as there are no pending interrupts */
    if (thread->status() == ThreadContext::Suspended &&
        !isInterrupted(thread_id))
    {
        panic("We should never hit the case where we try to commit from a "
              "suspended thread as the streamSeqNum should not match");
    } else if (inst->isFault()) {
        ExecContext context(cpu, *cpu.threads[thread_id], *this, inst);

        DPRINTF(MinorExecute, "Fault inst reached Execute: %s\n",
            inst->fault->name());

        fault = inst->fault;
        inst->fault->invoke(thread, NULL);

        tryToBranch(inst, fault, branch);
    } else if (inst->staticInst->isMemRef()) {
        /* Memory accesses are executed in two parts:
         *  executeMemRefInst -- calculates the EA and issues the access
         *      to memory.  This is done here.
         *  handleMemResponse -- handles the response packet, done by
         *      Execute::commit
         *
         *  While the memory access is in its FU, the EA is being
         *  calculated.  At the end of the FU, when it is ready to
         *  'commit' (in this function), the access is presented to the
         *  memory queues.  When a response comes back from memory,
         *  Execute::commit will commit it.
         */
        bool predicate_passed = false;
        bool completed_mem_inst = executeMemRefInst(inst, branch,
            predicate_passed, fault);

        if (completed_mem_inst && fault != NoFault) {
            if (early_memory_issue) {
                DPRINTF(MinorExecute, "Fault in early executing inst: %s\n",
                    fault->name());
                /* Don't execute the fault, just stall the instruction
                 *  until it gets to the head of inFlightInsts */
                inst->canEarlyIssue = false;
                /* Not completed as we'll come here again to pick up
                 * the fault when we get to the end of the FU */
                completed_inst = false;
            } else {
                DPRINTF(MinorExecute, "Fault in execute: %s\n",
                    fault->name());
                fault->invoke(thread, NULL);

                tryToBranch(inst, fault, branch);
                completed_inst = true;
            }
        } else {
            completed_inst = completed_mem_inst;
        }
        completed_mem_issue = completed_inst;
    } else if (inst->isInst() && inst->staticInst->isFullMemBarrier() &&
        !(venusRtlScalarTiming &&
          scalar600DecodeFallback(static_cast<uint32_t>(
              inst->staticInst->getEMI())) !=
              Scalar600DecodeFallback::None) &&
        !lsq.canPushIntoStoreBuffer())
    {
        DPRINTF(MinorExecute, "Can't commit data barrier inst: %s yet as"
            " there isn't space in the store buffer\n", *inst);

        completed_inst = false;
    } else if (inst->isInst() && inst->staticInst->isQuiesce()
            && !branch.isBubble()){
        /* This instruction can suspend, need to be able to communicate
         * backwards, so no other branches may evaluate this cycle*/
        completed_inst = false;
    } else {
        ExecContext context(cpu, *cpu.threads[thread_id], *this, inst);

        DPRINTF(MinorExecute, "Committing inst: %s\n", *inst);

        // fault = inst->staticInst->execute(&context,
        //     inst->traceData);

        const RiscvISA::VenusStaticInst* venus_inst =
            dynamic_cast<const RiscvISA::VenusStaticInst*>(inst->staticInst.get());
        const uint32_t committed_inst_bits = inst->isInst() ?
            static_cast<uint32_t>(inst->staticInst->getEMI()) : 0;
        const Scalar600DecodeFallback rtl_decode_fallback =
            venusRtlScalarTiming ?
                scalar600DecodeFallback(committed_inst_bits) :
                Scalar600DecodeFallback::None;

        // DPRINTF(MinorExecute, "Preparing sending venus packet...");

        if (venus_inst != nullptr) {
            bool send_success = true;
            DPRINTF(MinorExecute, "Detected Venus instruction: %s\n", *inst);
            ThreadContext *thread = cpu.getContext(thread_id);
            const auto &regClasses = thread->getIsaPtr()->regClasses();
            uint64_t venus_inst_bits = venus_inst->venusInstBits;
            uint8_t vs1_id = (venus_inst_bits >> 32) & 0x1f;
            uint8_t avl_id = (venus_inst_bits >> 7)  & 0x1F;
            uint32_t vs1 = 0;
            uint32_t avl = 0;
            if (vs1_id != 0) {
                RegId vs1_reg = (*regClasses.at(IntRegClass))[vs1_id];
                vs1 = thread->getReg(vs1_reg);
            }
            if (avl_id != 0) {
                RegId avl_reg = (*regClasses.at(IntRegClass))[avl_id];
                avl = thread->getReg(avl_reg);
            }
            // vsetcsr
            auto op = venus_inst->venusOp;
            if (op == gem5::RiscvISA::VSETCSR) {
                uint32_t csr_address = (venus_inst_bits >> 15) & 0xfff;
                if (csr_address >= gem5::MinorCPU::VenusCsrNum)
                    panic("VSETCSR: Invalid vsetcsr id! id=%d", csr_address);
                cpu.setVenusCsr(csr_address, avl);
                if (csr_address == 0) {
                    cpu.setMulshamt(avl);
                    DPRINTF(MinorExecute, "VSETCSR: CPU REG[mulshamt] set 0x%x", avl);
                } else if (csr_address == 1) {
                    cpu.setMsbhead(avl);
                    DPRINTF(MinorExecute, "VSETCSR: CPU REG[msbhead] set 0x%x", avl);
                } else if (csr_address == 2) {
                    cpu.setMulsaturate(avl);
                    DPRINTF(MinorExecute, "VSETCSR: CPU REG[mulsaturate] set 0x%x", avl);
                } else if (csr_address == 3) {
                    cpu.setLsuAddrMsb(avl);
                    DPRINTF(MinorExecute, "VSETCSR: CPU REG[lsu_addr_msb] set 0x%x", avl);
                }
            } else if (op == gem5::RiscvISA::VSETCSRIMM) {
                uint32_t csr_address     = (venus_inst_bits >> 7) & 0x1F;
                uint32_t csr_address_imm = (venus_inst_bits >> 15) & gem5::MinorCPU::VenusCsrMask;
                if (csr_address >= gem5::MinorCPU::VenusCsrNum)
                    panic("VSETCSRIMM: Invalid vsetcsr id! id=%d", csr_address);
                cpu.setVenusCsr(csr_address, csr_address_imm);
                if (csr_address == 0) {
                    cpu.setMulshamt(csr_address_imm);
                    DPRINTF(MinorExecute, "VSETCSRIMM: CPU REG[mulshamt] set 0x%x", avl);
                } else if (csr_address == 1) {
                    cpu.setMsbhead(csr_address_imm);
                    DPRINTF(MinorExecute, "VSETCSRIMM: CPU REG[msbhead] set 0x%x", avl);
                } else if (csr_address == 2) {
                    cpu.setMulsaturate(csr_address_imm);
                    DPRINTF(MinorExecute, "VSETCSRIMM: CPU REG[mulsaturate] set 0x%x", avl);
                } else if (csr_address == 3) {
                    cpu.setLsuAddrMsb(csr_address_imm);
                    DPRINTF(MinorExecute, "VSETCSRIMM: CPU REG[lsu_addr_msb] set 0x%x", csr_address_imm);
                }
            } else if (op == gem5::RiscvISA::VBARRIER) {
                /*
                 * vec_barrier_cnt and barrier_valid are persistent scalar600
                 * ID-stage registers, not fields of an individual dynamic
                 * instruction.  In particular, an early release can leave
                 * counter==0/valid==0 while Venus becomes busy, causing later
                 * barriers to pass until an idle edge rearms the counter.
                 */
                if (!inst->venusBarrierArmed ||
                    cpu.curCycle() < inst->minimumCommitCycle) {
                    completed_inst = false;
                    return false;
                }

                /*
                 * scalar600 samples venusidle_i through q/qq/qqq while the
                 * barrier advances through ID/EX/WB.  For an idle release,
                 * the ordinary WB boundary is two registered edges.  If a
                 * just-issued Venus request reaches qqq only at that WB
                 * edge, the still-live request retains the third edge.  The
                 * decision uses the RTL synchronizer state only; it is not
                 * keyed by task, PC, opcode neighbourhood, address, or data.
                 */
                if (inst->venusBarrierLateBusyRecheck &&
                    cpu.curCycle() == inst->minimumCommitCycle &&
                    cpu.rtlScalarBarrierBusyDelayed()) {
                    completed_inst = false;
                    return false;
                }

                DPRINTF(MinorExecute,
                    "VBARRIER persistent FSM retire: cycle=%llu "
                    "delayed_busy=%d\\n",
                    static_cast<unsigned long long>(cpu.curCycle()),
                    cpu.rtlScalarBarrierBusyDelayed());
            } else if (op == gem5::RiscvISA::VENUSEXT) {
                if (inst->venusPktSent) {
                    DPRINTF(MinorExecute, "Venus instruction already sent, skipping: 0x%016lx\n", venus_inst_bits);
                    completed_inst = true;
                    return true;
                }
                if (cpu.isVenusPortBlocked()) {
                    MinorDynInstPtr blocked_inst = cpu.getBlockedVenusInst();
                    if (blocked_inst && blocked_inst == inst) {
                        send_success = cpu.sendVenusInstrPkt(inst, vs1, avl);
                        if (!send_success) {
                            inst->venusRequestBackpressured = true;
                            DPRINTF(MinorExecute, "Venus packet retry failed, continuing stall: 0x%016lx\n", venus_inst_bits);
                            completed_inst = false;
                            return false;
                        } else {
                            if (inst->fuIndex != noCostFUIndex) {
                                funcUnits[inst->fuIndex]->stalled = false;
                            }
                            cpu.clearVenusBlocked();
                            inst->venusPktSent = true;
                            completed_inst = true;
                            DPRINTF(MinorExecute, "Venus packet sent successfully (recreated) bit=0x%016lx, vs1=x[%d]=%d, avl=x[%d]=%d\n [INST]:%s",
                                venus_inst_bits, vs1_id, vs1, avl_id, avl, *inst);
                        }
                        // cpu.clearVenusBlocked();
                    } else {
                        panic("Venus port blocked by another inst, stalling inst: %s\n", *inst);
                    }
                } else {
                    send_success = cpu.sendVenusInstrPkt(inst, vs1, avl);
                    if (!send_success) {
                        inst->venusRequestBackpressured = true;
                        DPRINTF(MinorExecute, "Venus blocked: inst=%s, streamSeqNum=%d, predSeqNum=%d\n", *inst, executeInfo[thread_id].streamSeqNum, inst->id.predictionSeqNum);
                        DPRINTF(MinorExecute, "Venus packet send blocked, stalling...0x%016lx\n", venus_inst_bits);
                        completed_inst = false;
                        if (inst->fuIndex != noCostFUIndex)
                            funcUnits[inst->fuIndex]->stalled = true;
                        return false;
                    } else {
                        inst->venusPktSent = true;
                        DPRINTF(MinorExecute, "Venus packet sent successfully bit=0x%016lx, vs1=x[%d]=%d, avl=x[%d]=%d, mulshamt=%d, getmsbhead=%d, mulsaturate=%d\n [INST]:%s",
                          venus_inst_bits, vs1_id, vs1, avl_id, avl, cpu.getMulshamt(), cpu.getMsbhead(), cpu.getMulsaturate(), *inst);
                    }
                }
                if (send_success && inst->venusPktSent &&
                    venusDirectBarrierRequestEpoch[thread_id]) {
                    /*
                     * After a direct physical barrier capture, accepted
                     * scalar600 Venus requests retain the two-word ID phase
                     * established by that release until the next barrier.
                     * Record each immediate follower so an LSU follower can
                     * retain its tagged completion/enqueue boundary too.
                     */
                    venusRequestFollowerWindow[thread_id] = true;
                    venusRequestAcceptedCycle[thread_id] = cpu.curCycle();
                }
            } else {
                panic("Wrong VenusCPUOp!!!!");
            }
            if (send_success)
                fault = inst->staticInst->execute(&context, inst->traceData);
        } else if (rtl_decode_fallback !=
                   Scalar600DecodeFallback::None) {
            /*
             * scalar600's inst_ecall/inst_illegal signals are not connected
             * to a trap path.  Unmatched decoder entries retain either the
             * no-write NOP defaults, write-zero results, or scalar600's
             * deliberately lax ALU/CSR decode.  Preserve that decoder
             * behavior instead of invoking gem5 SE-mode faults.
             */
            DPRINTF(MinorExecute,
                "scalar600 decoder fallback=%d retirement: inst=%s\n",
                static_cast<int>(rtl_decode_fallback), *inst);
            if (rtl_decode_fallback ==
                    Scalar600DecodeFallback::WriteZero) {
                const RegIndex rd = (committed_inst_bits >> 7) & 0x1fU;
                thread->setReg(
                    RiscvISA::intRegClass[rd], static_cast<RegVal>(0));
                if (inst->traceData)
                    inst->traceData->setData(0);
            }
            if (rtl_decode_fallback ==
                    Scalar600DecodeFallback::AluOverride) {
                const RegIndex rs1 =
                    (committed_inst_bits >> 15) & 0x1fU;
                const RegIndex rs2 =
                    (committed_inst_bits >> 20) & 0x1fU;
                const uint32_t rs1_value = static_cast<uint32_t>(
                    thread->getReg(RiscvISA::intRegClass[rs1]));
                const uint32_t rs2_value = static_cast<uint32_t>(
                    thread->getReg(RiscvISA::intRegClass[rs2]));
                const RtlExValue ex_value = scalar600ExValue(
                    inst, rs1_value, true, rs2_value, true);
                if (!ex_value.known || ex_value.loadLike) {
                    panic("scalar600 ALU override has no result: %s", *inst);
                }
                const RegIndex rd =
                    (committed_inst_bits >> 7) & 0x1fU;
                const RegVal write_value = static_cast<RegVal>(
                    static_cast<int64_t>(
                        static_cast<int32_t>(ex_value.value)));
                thread->setReg(RiscvISA::intRegClass[rd], write_value);
                if (inst->traceData)
                    inst->traceData->setData(write_value);
            }
            if (rtl_decode_fallback ==
                    Scalar600DecodeFallback::CsrOverride) {
                const uint32_t csr =
                    (committed_inst_bits >> 20) & 0xffeU;
                uint64_t counter = 0;
                if (csr == 0xc00U || csr == 0xc80U) {
                    counter = cpu.architecturalCycleCounter(thread_id);
                } else if (csr == 0xc02U || csr == 0xc82U) {
                    counter = cpu.architecturalInstRetCounter(thread_id);
                } else if (csr != 0xf14U) {
                    panic("scalar600 unsupported CSR override: 0x%x", csr);
                }
                const uint32_t value = (csr == 0xc80U || csr == 0xc82U) ?
                    static_cast<uint32_t>(counter >> 32) :
                    static_cast<uint32_t>(counter);
                const RegIndex rd =
                    (committed_inst_bits >> 7) & 0x1fU;
                const RegVal write_value = static_cast<RegVal>(
                    static_cast<int64_t>(static_cast<int32_t>(value)));
                thread->setReg(RiscvISA::intRegClass[rd], write_value);
                if (inst->traceData)
                    inst->traceData->setData(write_value);
            }
            if (rtl_decode_fallback ==
                    Scalar600DecodeFallback::WfiOverride) {
                cpu.markRtlWfiSuspend(thread_id);
                cpu.suspendContext(thread_id);
            }
            if (rtl_decode_fallback ==
                    Scalar600DecodeFallback::ZeroWord &&
                inst->traceData) {
                /* scalar600_core deliberately omits wb_instr==0 from its
                 * retire/debug stream even though the bubble consumes its
                 * normal pipeline edge. */
                delete inst->traceData;
                inst->traceData = nullptr;
            }
            fault = NoFault;
        } else {
            if (venusRtlScalarTiming && inst->staticInst->isQuiesce())
                cpu.markRtlWfiSuspend(thread_id);
            fault = inst->staticInst->execute(&context, inst->traceData);
        }

        if (venusRtlScalarTiming && inst->isInst() &&
            inst->staticInst->opClass() == enums::IntDiv) {
            RtlDividerState &divider =
                executeInfo[thread_id].rtlDivider;
            if (!divider.pending ||
                divider.pendingSeqNum != inst->id.execSeqNum) {
                panic("scalar600 divider retired without matching plan: %s",
                    *inst);
            }
            const RegVal write_value = static_cast<RegVal>(
                static_cast<int64_t>(
                    static_cast<int32_t>(divider.pendingResult)));
            if (inst->staticInst->numDestRegs() > 0) {
                thread->setReg(inst->staticInst->destRegIdx(0), write_value);
                if (inst->traceData)
                    inst->traceData->setData(
                        static_cast<uint64_t>(write_value));
            }
            DPRINTF(MinorExecute,
                "scalar600 divider writeback inst=%s result=0x%08x\n",
                *inst, divider.pendingResult);
        }

        /* Set the predicate for tracing and dump */
        if (inst->traceData)
            inst->traceData->setPredicate(context.readPredicate());

        committed = true;

        if (fault != NoFault) {
            if (inst->traceData) {
                if (debug::ExecFaulting) {
                    inst->traceData->setFaulting(true);
                } else {
                    delete inst->traceData;
                    inst->traceData = NULL;
                }
            }

            DPRINTF(MinorExecute, "Fault in execute of inst: %s fault: %s\n",
                *inst, fault->name());
            fault->invoke(thread, inst->staticInst);
        }

        if (rtl_decode_fallback != Scalar600DecodeFallback::ZeroWord)
            doInstCommitAccounting(inst);
        // tryToBranch(inst, fault, branch);
        const RiscvISA::VenusStaticInst* venus_check = dynamic_cast<const RiscvISA::VenusStaticInst*>(inst->staticInst.get());
if (venus_check != nullptr && venus_check->venusOp == gem5::RiscvISA::VENUSEXT) {
    // Venus指令的PC已经在Fetch阶段正确处理
    DPRINTF(MinorExecute, "Skipping tryToBranch for Venus instruction\n");
} else {
    tryToBranch(inst, fault, branch);
}
        // if (venus_check != nullptr) {
        //     // Venus 指令不调用 tryToBranch
        //     // 因为 PC 的计算由 Fetch 阶段负责
        //     std::cout << "[Execute] Skipping tryToBranch for Venus instruction" << std::endl;
        // } else {
        //     tryToBranch(inst, fault, branch);
        // }
    }

    // const RiscvISA::VenusStaticInst* venus_inst =
    //         dynamic_cast<const RiscvISA::VenusStaticInst*>(inst->staticInst.get());

    if (completed_inst) {
        // if (venus_inst && venus_inst->venusOp == gem5::RiscvISA::VENUSEXT) {
        //     MinorDynInstPtr blocked_inst = cpu.getBlockedVenusInst();
        //     if (blocked_inst == inst) {
        //         cpu.clearVenusBlocked();
        //     }
        // }
        /* Keep a copy of this instruction's predictionSeqNum just in case
         * we need to issue a branch without an instruction (such as an
         * interrupt) */
        executeInfo[thread_id].lastPredictionSeqNum = inst->id.predictionSeqNum;

        /* Check to see if this instruction suspended the current thread. */
        if (!inst->isFault() &&
            thread->status() == ThreadContext::Suspended &&
            branch.isBubble() && /* It didn't branch too */
            !isInterrupted(thread_id)) /* Don't suspend if we have
                interrupts */
        {
            auto &resume_pc = cpu.getContext(thread_id)->pcState();

            assert(resume_pc.microPC() == 0);

            DPRINTF(MinorInterrupt, "Suspending thread: %d from Execute"
                " inst: %s\n", thread_id, *inst);

            cpu.fetchStats[thread_id]->numFetchSuspends++;

            updateBranchData(thread_id, BranchData::SuspendThread, inst,
                resume_pc, branch);
        }
    }

    return completed_inst;
}

bool
Execute::tryScalar600SameEdgeLsuLaunch(
    ThreadID thread_id, BranchData &branch, bool allow_newly_issued)
{
    ExecuteThreadInfo &ex_info = executeInfo[thread_id];
    if (!venusRtlScalarTiming ||
        branch.isStreamChange() ||
        ex_info.inFlightInsts->empty() ||
        ex_info.inFUMemInsts->empty() || !lsq.canRequest())
        return false;

    MinorDynInstPtr next = ex_info.inFlightInsts->front().inst;
    if (next != ex_info.inFUMemInsts->front().inst || !next->isInst() ||
        !next->staticInst->isMemRef() || next->inLSQ)
        return false;

    /*
     * scalar600 keeps the following scalar instruction in ID while a
     * two-word Venus request is backpressured.  The ready handshake releases
     * ID through registered boundaries; the follower cannot enter the LSU on
     * that same edge.  The ordinary, non-backpressured fused-instruction path
     * retains its existing same-edge launch behavior.
     */
    if (ex_info.hasCommittedInst &&
        ex_info.lastCommittedCycle == cpu.curCycle() &&
        ex_info.lastCommittedHadVenusExtension &&
        ex_info.lastCommittedVenusWasBackpressured) {
        DPRINTF(MinorExecute,
            "scalar600 registered Venus request release blocks same-edge "
            "LSU launch: %s\n", *next);
        return false;
    }

    const unsigned int memory_fu_index = next->fuIndex;
    FUPipeline *memory_fu = funcUnits[memory_fu_index];
    const bool at_fu_head =
        !memory_fu->front().inst->isBubble() &&
        memory_fu->front().inst == next;
    const bool entered_fu_this_edge =
        allow_newly_issued && memory_fu->front().inst->isBubble() &&
        memory_fu->alreadyPushed();
    if (!at_fu_head && !entered_fu_this_edge)
        return false;

    bool launched = false;
    bool completed_memory_issue = false;
    Fault launch_fault = NoFault;
    const bool completed = commitInst(next, true, branch, launch_fault,
        launched, completed_memory_issue);
    if (!completed || !completed_memory_issue || launch_fault != NoFault)
        return false;

    if (entered_fu_this_edge)
        memory_fu->consumePushed();
    memory_fu->stalled = false;
    next->fuIndex = 0;
    next->inLSQ = true;
    ex_info.inFUMemInsts->pop();
    DPRINTF(MinorExecute,
        "scalar600 same-edge in-order LSU launch (%s): %s\n",
        entered_fu_this_edge ? "post-issue" : "FU-head", *next);
    return true;
}

void
Execute::commit(ThreadID thread_id, bool only_commit_microops, bool discard,
    BranchData &branch)
{
    Fault fault = NoFault;
    Cycles now = cpu.curCycle();
    ExecuteThreadInfo &ex_info = executeInfo[thread_id];

    /**
     * Try and execute as many instructions from the end of FU pipelines as
     *  possible.  This *doesn't* include actually advancing the pipelines.
     *
     * We do this by looping on the front of the inFlightInsts queue for as
     *  long as we can find the desired instruction at the end of the
     *  functional unit it was issued to without seeing a branch or a fault.
     *  In this function, these terms are used:
     *      complete -- The instruction has finished its passage through
     *          its functional unit and its fate has been decided
     *          (committed, discarded, issued to the memory system)
     *      commit -- The instruction is complete(d), not discarded and has
     *          its effects applied to the CPU state
     *      discard(ed) -- The instruction is complete but not committed
     *          as its streamSeqNum disagrees with the current
     *          Execute::streamSeqNum
     *
     *  Commits are also possible from two other places:
     *
     *  1) Responses returning from the LSQ
     *  2) Mem ops issued to the LSQ ('committed' from the FUs) earlier
     *      than their position in the inFlightInsts queue, but after all
     *      their dependencies are resolved.
     */

    /* Has an instruction been completed?  Once this becomes false, we stop
     *  trying to complete instructions. */
    bool completed_inst = true;

    /* Number of insts committed this cycle to check against commitLimit */
    unsigned int num_insts_committed = 0;

    /* Number of memory access instructions committed to check against
     *  memCommitLimit */
    unsigned int num_mem_refs_committed = 0;

    if (only_commit_microops && !ex_info.inFlightInsts->empty()) {
        DPRINTF(MinorInterrupt, "Only commit microops %s %d\n",
            *(ex_info.inFlightInsts->front().inst),
            ex_info.lastCommitWasEndOfMacroop);
    }

    while (!ex_info.inFlightInsts->empty() && /* Some more instructions to process */
        !branch.isStreamChange() && /* No real branch */
        fault == NoFault && /* No faults */
        completed_inst && /* Still finding instructions to execute */
        num_insts_committed != commitLimit /* Not reached commit limit */
        )
    {
        if (only_commit_microops) {
            DPRINTF(MinorInterrupt, "Committing tail of insts before"
                " interrupt: %s\n",
                *(ex_info.inFlightInsts->front().inst));
        }

        QueuedInst *head_inflight_inst = &(ex_info.inFlightInsts->front());

        InstSeqNum head_exec_seq_num =
            head_inflight_inst->inst->id.execSeqNum;

        /* The instruction we actually process if completed_inst
         *  remains true to the end of the loop body.
         *  Start by considering the the head of the in flight insts queue */
        MinorDynInstPtr inst = head_inflight_inst->inst;

        bool committed_inst = false;
        bool discard_inst = false;
        bool completed_mem_ref = false;
        bool issued_mem_ref = false;
        bool early_memory_issue = false;

        /* Must set this again to go around the loop */
        completed_inst = false;

        /* If we're just completing a macroop before an interrupt or drain,
         *  can we stil commit another microop (rather than a memory response)
         *  without crosing into the next full instruction? */
        bool can_commit_insts = !ex_info.inFlightInsts->empty() &&
            !(only_commit_microops && ex_info.lastCommitWasEndOfMacroop);

        /* Can we find a mem response for this inst */
        LSQ::LSQRequestPtr mem_response =
            (inst->inLSQ ? lsq.findResponse(inst) : NULL);

        bool rtl_last_load_feeds_inst = false;
        if (venusRtlScalarTiming && ex_info.lastCommittedWasLoad &&
            ex_info.lastCommittedLoad && inst->isInst()) {
            const auto &load_static =
                ex_info.lastCommittedLoad->staticInst;
            for (unsigned int dest = 0;
                 !rtl_last_load_feeds_inst &&
                 dest < load_static->numDestRegs(); ++dest) {
                for (unsigned int src = 0;
                     src < inst->staticInst->numSrcRegs(); ++src) {
                    if (load_static->destRegIdx(dest) ==
                        inst->staticInst->srcRegIdx(src)) {
                        rtl_last_load_feeds_inst = true;
                        break;
                    }
                }
            }
        }

        bool rtl_memory_commit_ready = true;
        if (venusRtlScalarTiming && mem_response && inst->isInst() &&
            ex_info.hasCommittedInst) {
            /*
             * scalar600's LSU freezes the complete pipeline until its
             * registered completion.  Preserve the observed WB distances
             * without changing the memory-system response latency: an
             * ordinary load completes six cycles after the preceding WB
             * edge, or eight cycles after taken control.  When a held Venus
             * request releases a following load, gem5's architectural memory
             * completion represents the RTL WB-visible edge rather than the
             * LSU's preceding complete pulse, so that path also needs eight
             * cycles from the request handshake.  A store needs four cycles
             * after taken control; after a load its WB distance is selected
             * by the scalar decoder's true address/data dependency.
             */
            Cycles minimum_gap(0);
            if (inst->staticInst->isStoreConditional()) {
                minimum_gap = Cycles(6);
            } else if (inst->staticInst->isLoad()) {
                minimum_gap =
                    ex_info.lastCommittedWasControl &&
                    ex_info.lastCommittedControlTaken ? Cycles(8) :
                    (ex_info.lastCommittedHadVenusExtension &&
                     ex_info.lastCommittedVenusWasBackpressured ?
                        Cycles(8) : Cycles(6));
            } else if (inst->staticInst->isStore()) {
                bool last_load_was_lr = false;
                bool store_reads_last_load = false;
                if (ex_info.lastCommittedWasLoad &&
                    ex_info.lastCommittedLoad) {
                    const uint32_t last_bits = static_cast<uint32_t>(
                        ex_info.lastCommittedLoad->staticInst->getEMI());
                    last_load_was_lr =
                        (last_bits & 0x7fU) == 0x2fU &&
                        ((last_bits >> 27) & 0x1fU) == 0x02U;
                    const RegIndex load_rd = (last_bits >> 7) & 0x1fU;
                    const uint32_t store_bits = static_cast<uint32_t>(
                        inst->staticInst->getEMI());
                    store_reads_last_load = load_rd != 0 &&
                        scalar600ReadsIntegerRegister(store_bits, load_rd);
                }
                /*
                 * scalar600 can place an independent store in EX on the
                 * edge after a load reaches WB, giving a two-cycle retire
                 * separation.  Only a true address/data RAW consumes the
                 * registered load-result forwarding edge and expands the
                 * separation to three cycles.  RTL scalar-retire oracles
                 * show this exact split for every observed adjacent pair;
                 * model the dependency, not an opcode-wide delay.
                 */
                minimum_gap =
                    ex_info.lastCommittedWasControl &&
                    ex_info.lastCommittedControlTaken ? Cycles(4) :
                    (ex_info.lastCommittedWasLoad && !last_load_was_lr ?
                        (store_reads_last_load ? Cycles(3) : Cycles(2)) :
                        Cycles(2));
            }
            rtl_memory_commit_ready =
                now >= ex_info.lastCommittedCycle + minimum_gap;
        }

        if (venusRtlScalarTiming && mem_response && inst->isInst() &&
            venusRequestFollowerLsuSeq[thread_id] ==
                inst->id.execSeqNum) {
            /*
             * This response belongs to the LSU operation that immediately
             * followed an accepted Venus request.  scalar600 first registers the
             * LSU completion/result enqueue and only exposes it to WB on the
             * following edge.  Anchor the delay to the observed response,
             * rather than changing the LSU's fixed execution latency.
             */
            Cycles &ready =
                venusRequestLsuResponseReadyCycle[thread_id];
            if (ready == Cycles(0) && rtl_memory_commit_ready) {
                ready = now + Cycles(1);
                DPRINTF(MinorExecute,
                    "scalar600 Venus-request follower LSU response "
                    "registered for cycle %llu: %s\n",
                    static_cast<unsigned long long>(ready), *inst);
            }
            if (ready == Cycles(0) || now < ready)
                rtl_memory_commit_ready = false;
        }

        DPRINTF(MinorExecute, "Trying to commit canCommitInsts: %d\n",
            can_commit_insts);

        /* Test for PC events after every instruction */
        if (isInbetweenInsts(thread_id) && tryPCEvents(thread_id)) {
            ThreadContext *thread = cpu.getContext(thread_id);

            /* Branch as there was a change in PC */
            updateBranchData(thread_id, BranchData::UnpredictedBranch,
                MinorDynInst::bubble(), thread->pcState(), branch);
        } else if (mem_response && rtl_memory_commit_ready &&
            num_mem_refs_committed < memoryCommitLimit)
        {
            /* Try to commit from the memory responses next */
            discard_inst = inst->id.streamSeqNum !=
                           ex_info.streamSeqNum || discard;

            DPRINTF(MinorExecute, "Trying to commit mem response: %s\n",
                *inst);

            /* Complete or discard the response */
            if (discard_inst) {
                DPRINTF(MinorExecute, "Discarding mem inst: %s as its"
                    " stream state was unexpected, expected: %d\n",
                    *inst, ex_info.streamSeqNum);

                lsq.popResponse(mem_response);
            } else {
                handleMemResponse(inst, mem_response, branch, fault);
                committed_inst = true;
            }

            completed_mem_ref = true;
            completed_inst = true;
        } else if (can_commit_insts) {
            /* If true, this instruction will, subject to timing tweaks,
             *  be considered for completion.  try_to_commit flattens
             *  the `if' tree a bit and allows other tests for inst
             *  commit to be inserted here. */
            bool try_to_commit = false;

            /* Try and issue memory ops early if they:
             *  - Can push a request into the LSQ
             *  - Have reached the end of their FUs
             *  - Have had all their dependencies satisfied
             *  - Are from the right stream
             *
             *  For any other case, leave it to the normal instruction
             *  issue below to handle them.
             */
            if (!ex_info.inFUMemInsts->empty() && lsq.canRequest()) {
                DPRINTF(MinorExecute, "Trying to commit from mem FUs\n");

                const MinorDynInstPtr head_mem_ref_inst =
                    ex_info.inFUMemInsts->front().inst;
                FUPipeline *fu = funcUnits[head_mem_ref_inst->fuIndex];
                const MinorDynInstPtr &fu_inst = fu->front().inst;

                /* Use this, possibly out of order, inst as the one
                 *  to 'commit'/send to the LSQ */
                if (!fu_inst->isBubble() &&
                    !fu_inst->inLSQ &&
                    fu_inst->canEarlyIssue &&
                    ex_info.streamSeqNum == fu_inst->id.streamSeqNum &&
                    head_exec_seq_num > fu_inst->instToWaitFor)
                {
                    DPRINTF(MinorExecute, "Issuing mem ref early"
                        " inst: %s instToWaitFor: %d\n",
                        *(fu_inst), fu_inst->instToWaitFor);

                    inst = fu_inst;
                    try_to_commit = true;
                    early_memory_issue = true;
                    completed_inst = true;
                }
            }

            /* Try and commit FU-less insts */
            if (!completed_inst && inst->isNoCostInst()) {
                DPRINTF(MinorExecute, "Committing no cost inst: %s", *inst);

                try_to_commit = true;
                completed_inst = true;
            }

            /* Try to issue from the ends of FUs and the inFlightInsts
             *  queue */
            if (!completed_inst && !inst->inLSQ) {
                DPRINTF(MinorExecute, "Trying to commit from FUs\n");

                /* Try to commit from a functional unit */
                /* Is the head inst of the expected inst's FU actually the
                 *  expected inst? */
                QueuedInst &fu_inst =
                    funcUnits[inst->fuIndex]->front();
                InstSeqNum fu_inst_seq_num = fu_inst.inst->id.execSeqNum;

                if (fu_inst.inst->isBubble()) {
                    /* No instruction ready */
                    completed_inst = false;
                } else if (fu_inst_seq_num != head_exec_seq_num) {
                    /* Past instruction: we must have already executed it
                     * in the same cycle and so the head inst isn't
                     * actually at the end of its pipeline
                     * Future instruction: handled above and only for
                     * mem refs on their way to the LSQ */
                } else if (fu_inst.inst->id == inst->id)  {
                    /* All instructions can be committed if they have the
                     *  right execSeqNum and there are no in-flight
                     *  mem insts before us */
                    try_to_commit = true;
                    completed_inst = true;
                }
            }

            if (try_to_commit) {
                discard_inst = inst->id.streamSeqNum !=
                    ex_info.streamSeqNum || discard;

                /* Is this instruction discardable as its streamSeqNum
                 *  doesn't match? */
                if (!discard_inst) {
                    /* Try to commit or discard a non-memory instruction.
                     *  Memory ops are actually 'committed' from this FUs
                     *  and 'issued' into the memory system so we need to
                     *  account for them later (commit_was_mem_issue gets
                     *  set) */
                    if (inst->extraCommitDelayExpr) {
                        DPRINTF(MinorExecute, "Evaluating expression for"
                            " extra commit delay inst: %s\n", *inst);

                        ThreadContext *thread = cpu.getContext(thread_id);

                        TimingExprEvalContext context(inst->staticInst,
                            thread, NULL);

                        uint64_t extra_delay = inst->extraCommitDelayExpr->
                            eval(context);

                        DPRINTF(MinorExecute, "Extra commit delay expr"
                            " result: %d\n", extra_delay);

                        if (extra_delay < 128) {
                            inst->extraCommitDelay += Cycles(extra_delay);
                        } else {
                            DPRINTF(MinorExecute, "Extra commit delay was"
                                " very long: %d\n", extra_delay);
                        }
                        inst->extraCommitDelayExpr = NULL;
                    }

                    /* Move the extraCommitDelay from the instruction
                     *  into the minimumCommitCycle */
                    if (inst->extraCommitDelay != Cycles(0)) {
                        inst->minimumCommitCycle = cpu.curCycle() +
                            inst->extraCommitDelay;
                        inst->extraCommitDelay = Cycles(0);
                    }

                    if (venusRtlScalarTiming && inst->isInst() &&
                        inst->staticInst->opClass() == enums::IntDiv) {
                        rtlDividerPlan(inst, ex_info);
                    }

                    /* @todo Think about making lastMemBarrier be
                     *  MAX_UINT_64 to avoid using 0 as a marker value */
                    if (!inst->isFault() && inst->isMemRef() &&
                        lsq.getLastMemBarrier(thread_id) <
                            inst->id.execSeqNum &&
                        lsq.getLastMemBarrier(thread_id) != 0)
                    {
                        DPRINTF(MinorExecute, "Not committing inst: %s yet"
                            " as there are incomplete barriers in flight\n",
                            *inst);
                        completed_inst = false;
                    } else if (venusRtlScalarTiming &&
                        ex_info.hasCommittedInst &&
                        ex_info.lastCommittedHadVenusExtension &&
                        now < ex_info.lastCommittedCycle +
                            (inst->isInst() && inst->staticInst->isMemRef() &&
                             ex_info.lastCommittedVenusWasBackpressured ?
                             Cycles(2) : Cycles(3))) {
                        /*
                         * scalar600 retires the second 32-bit word of a
                         * 64-bit Venus instruction on the following edge.
                         * gem5 represents both words as one StaticInst, so
                         * retain that ID/retire occupancy before admitting
                         * the next architectural instruction.
                         */
                        completed_inst = false;
                    } else if (venusRtlScalarTiming &&
                        ex_info.hasCommittedInst &&
                        ex_info.lastCommittedWasControl &&
                        ex_info.lastCommittedControlTaken && inst->isInst() &&
                        !inst->staticInst->isMemRef() &&
                        now < ex_info.lastCommittedCycle + Cycles(3)) {
                        /*
                         * A taken scalar600 control transfer redirects IF
                         * through its registered SRAM boundary.  The first
                         * target instruction therefore cannot retire until
                         * the third edge after the control instruction.  A
                         * not-taken branch remains on the sequential stream
                         * and does not pay this redirect boundary.
                         */
                        completed_inst = false;
                    } else if (venusRtlScalarTiming &&
                        ex_info.hasCommittedInst &&
                        ex_info.lastCommittedWasLoad &&
                        rtl_last_load_feeds_inst &&
                        !inst->staticInst->isMemRef() &&
                        now < ex_info.lastCommittedCycle + Cycles(2)) {
                        /*
                         * The scalar600 load result crosses the registered
                         * WB/RF boundary before a non-memory consumer can
                         * retire.  Minor's predicted scoreboard otherwise
                         * lets an ALU consumer retire on the response edge.
                         */
                        completed_inst = false;
                    } else if (venusRtlScalarTiming &&
                        ex_info.hasCommittedInst && inst->isInst() &&
                        inst->staticInst->opClass() == enums::IntMult &&
                        now < ex_info.lastCommittedCycle +
                            (ex_info.lastCommittedWasControl &&
                             ex_info.lastCommittedControlTaken ?
                             Cycles(5) :
                             (ex_info.lastCommittedWasLoad &&
                             rtl_last_load_feeds_inst ?
                             Cycles(4) :
                             (ex_info.lastCommittedWasLoad ?
                              Cycles(1) :
                              (ex_info.lastCommittedWasStore ?
                               Cycles(5) : Cycles(3)))))) {
                        /*
                         * scalar600_mul is driven directly by the ID-stage
                         * mul_div_bus.  An independent MUL held behind a long
                         * WB load therefore keeps advancing its registered
                         * multiplier while the pipeline is frozen, and can
                         * retire on the edge after that load.  A true load RAW
                         * must first cross WB/RF and retains the four-cycle
                         * distance.  A short store does not hide the complete
                         * multiplier pipeline and leaves a five-cycle observed
                         * store-to-MUL distance.  Ordinary ALU/MUL predecessors
                         * retain the three-cycle distance.  These are physical
                         * ID/EX/WB overlap rules from scalar600_core and
                         * scalar600_mul, not workload or PC classifications.
                         */
                        completed_inst = false;
                    } else if (venusRtlScalarTiming &&
                        inst->isInst() &&
                        inst->staticInst->opClass() == enums::IntDiv &&
                        ex_info.rtlDivider.pending &&
                        now < ex_info.rtlDivider.pendingReadyCycle) {
                        /*
                         * scalar600_core holds ID/EX while the divider's
                         * registered complete signal is low.  The shadow
                         * state above supplies the exact dynamic edge,
                         * including divide-by-zero and post-zero transitions.
                         */
                        completed_inst = false;
                    } else if (inst->minimumCommitCycle > now) {
                        DPRINTF(MinorExecute, "Not committing inst: %s yet"
                            " as it wants to be stalled for %d more cycles\n",
                            *inst, inst->minimumCommitCycle - now);
                        completed_inst = false;
                    } else {
                        completed_inst = commitInst(inst,
                            early_memory_issue, branch, fault,
                            committed_inst, issued_mem_ref);
                    }
                } else {
                    /* Discard instruction */
                    completed_inst = true;
                }

                if (completed_inst) {
                    /* Allow the pipeline to advance.  If the FU head
                     *  instruction wasn't the inFlightInsts head
                     *  but had already been committed, it would have
                     *  unstalled the pipeline before here */
                    if (inst->fuIndex != noCostFUIndex) {
                        DPRINTF(MinorExecute, "Unstalling %d for inst %s\n", inst->fuIndex, inst->id);
                        funcUnits[inst->fuIndex]->stalled = false;
                    }
                }
            }
        } else {
            DPRINTF(MinorExecute, "No instructions to commit\n");
            completed_inst = false;
        }

        /* All discardable instructions must also be 'completed' by now */
        assert(!(discard_inst && !completed_inst));

        /* Instruction committed but was discarded due to streamSeqNum
         *  mismatch */
        if (discard_inst) {
            DPRINTF(MinorExecute, "Discarding inst: %s as its stream"
                " state was unexpected, expected: %d\n",
                *inst, ex_info.streamSeqNum);

            if (fault == NoFault) {
                cpu.executeStats[thread_id]->numDiscardedOps++;
            }
        }

        /* Mark the mem inst as being in the LSQ */
        if (issued_mem_ref) {
            inst->fuIndex = 0;
            inst->inLSQ = true;
        }

        /* Pop issued (to LSQ) and discarded mem refs from the inFUMemInsts
         *  as they've *definitely* exited the FUs */
        if (completed_inst && inst->isMemRef()) {
            /* The MemRef could have been discarded from the FU or the memory
             *  queue, so just check an FU instruction */
            if (!ex_info.inFUMemInsts->empty() &&
                ex_info.inFUMemInsts->front().inst == inst)
            {
                ex_info.inFUMemInsts->pop();
            }
        }

        if (completed_inst && !(issued_mem_ref && fault == NoFault)) {
            /* Note that this includes discarded insts */
            DPRINTF(MinorExecute, "Completed inst: %s\n", *inst);

            /* Got to the end of a full instruction? */
            ex_info.lastCommitWasEndOfMacroop = inst->isFault() ||
                inst->isLastOpInInst();

            /* lastPredictionSeqNum is kept as a convenience to prevent its
             *  value from changing too much on the minorview display */
            ex_info.lastPredictionSeqNum = inst->id.predictionSeqNum;

            /* Finished with the inst, remove it from the inst queue and
             *  clear its dependencies */
            rtlIdOnComplete(inst);
            ex_info.inFlightInsts->pop();

            /* Complete barriers in the LSQ/move to store buffer */
            if (inst->isInst() && inst->staticInst->isFullMemBarrier()) {
                DPRINTF(MinorMem, "Completing memory barrier"
                    " inst: %s committed: %d\n", *inst, committed_inst);
                lsq.completeMemBarrierInst(inst, committed_inst);
            }

            scoreboard[thread_id].clearInstDests(inst, inst->isMemRef());

        }

        /* Handle per-cycle instruction counting */
        if (committed_inst) {
            if (inst->isInst() && inst->staticInst->isMemRef() &&
                venusRequestFollowerLsuSeq[thread_id] ==
                    inst->id.execSeqNum) {
                venusRequestFollowerLsuSeq[thread_id] = 0;
                venusRequestLsuResponseReadyCycle[thread_id] =
                    Cycles(0);
                venusRequestLsuReleaseCycle[thread_id] = now;
                DPRINTF(MinorExecute,
                    "scalar600 Venus-request follower LSU registered "
                    "release edge: %s\n", *inst);
            }
            if (venusRtlScalarTiming) {
                RtlDividerState &divider = ex_info.rtlDivider;
                if (inst->isInst() &&
                    inst->staticInst->opClass() == enums::IntDiv) {
                    divider.pending = false;
                    divider.issueBlocked = false;
                } else {
                    rtlDividerStep(divider, false, false, false, 0, 0);
                    divider.modelCycle += Cycles(1);
                }
            }
            ex_info.hasCommittedInst = true;
            ex_info.lastCommittedCycle = now;
            ex_info.lastCommittedWasControl =
                inst->isInst() && inst->staticInst->isControl();
            ex_info.lastCommittedControlTaken =
                ex_info.lastCommittedWasControl && inst->predictedTaken;
            const auto *venus_static = inst->isInst() ?
                dynamic_cast<const RiscvISA::VenusStaticInst *>(
                    inst->staticInst.get()) : nullptr;
            ex_info.lastCommittedHadVenusExtension =
                venus_static &&
                venus_static->venusOp == RiscvISA::VENUSEXT;
            ex_info.lastCommittedVenusWasBackpressured =
                ex_info.lastCommittedHadVenusExtension &&
                inst->venusRequestBackpressured;
            ex_info.lastCommittedWasMemRef =
                inst->isInst() && inst->staticInst->isMemRef();
            ex_info.lastCommittedWasLoad =
                inst->isInst() && inst->staticInst->isLoad();
            ex_info.lastCommittedWasStore =
                inst->isInst() && inst->staticInst->isStore();
            ex_info.lastCommittedLoad = ex_info.lastCommittedWasLoad ?
                inst : nullptr;

            const bool is_no_cost_inst = inst->isNoCostInst();
            /*
             * CSR, WFI and Venus scalar instructions can be encoded as
             * No_OpClass so they do not require an ordinary Minor functional
             * unit.  They are still real scalar600 instructions: the RTL WB
             * stage retires at most one instruction per cycle.
             */
            const bool consumes_commit_slot =
                !is_no_cost_inst ||
                (venusRtlScalarTiming && inst->isInst());

            /* Don't show no cost instructions as having taken a commit
             *  slot */
            if (debug::MinorTrace && consumes_commit_slot)
                ex_info.instsBeingCommitted.insts[num_insts_committed] = inst;

            if (consumes_commit_slot)
                num_insts_committed++;

            if (num_insts_committed == commitLimit)
                DPRINTF(MinorExecute, "Reached inst commit limit\n");

            /* Re-set the time of the instruction if that's required for
             * tracing */
            if (inst->traceData) {
                if (setTraceTimeOnCommit)
                    inst->traceData->setWhen(curTick());
                inst->traceData->dump();
            }

            if (completed_mem_ref)
                num_mem_refs_committed++;

            if (num_mem_refs_committed == memoryCommitLimit)
                DPRINTF(MinorExecute, "Reached mem ref commit limit\n");

            /*
             * Commit runs before issue on each Minor edge.  Once an older
             * instruction retires, scalar600 can present an already-ready,
             * now-head memory operation to the LSU on that same edge.  This
             * is strictly in-order admission, unlike Minor's speculative
             * executeAllowEarlyMemoryIssue path: the candidate must be the
             * new in-flight head and the oldest memory-FU entry.
             */
            if (!inst->staticInst->isMemRef())
                tryScalar600SameEdgeLsuLaunch(thread_id, branch, false);

            /*
             * The scalar600 Venus request wires are sourced from ID, not
             * from WB retirement.  A request marked by the direct-barrier
             * path may therefore become visible on this edge after its
             * immediately older instruction has retired and updated the
             * architectural register/CSR state.  Keep the younger fused
             * instruction in the in-flight queue so retirement remains
             * strictly one instruction per edge.
             */
            tryPresentRtlVenusRequest(thread_id);
        }
    }
}

bool
Execute::isInbetweenInsts(ThreadID thread_id) const
{
    return executeInfo[thread_id].lastCommitWasEndOfMacroop &&
        !lsq.accessesInFlight();
}

void
Execute::evaluate()
{
    rtlScalarIdStopRequest = false;
    rtlScalarNonDividerIdStopRequest = false;
    if (!inp.outputWire->isBubble())
        inputBuffer[inp.outputWire->threadId].setTail(*inp.outputWire);

    /* vector_support is an ID-stage register block, so it clocks even when
     * an older dynamic instruction prevents this barrier reaching commit. */
    if (venusBarrierInId &&
        cpu.curCycle() >= venusBarrierFirstStepCycle)
        stepVenusBarrierInId();

    BranchData &branch = *out.inputWire;

    unsigned int num_issued = 0;

    /* Do all the cycle-wise activities for dcachePort here to potentially
     *  free up input spaces in the LSQ's requests queue */
    lsq.step();

    /* Check interrupts first.  Will halt commit if interrupt found */
    bool interrupted = false;
    ThreadID interrupt_tid = checkInterrupts(branch, interrupted);
    ThreadID task_suspend_tid = InvalidThreadID;
    if (interrupt_tid == InvalidThreadID)
        task_suspend_tid = cpu.takeVenusTaskSuspend();

    if (interrupt_tid != InvalidThreadID) {
        /* Signalling an interrupt this cycle, not issuing/committing from
         * any other threads */
    } else if (task_suspend_tid != InvalidThreadID) {
        /*
         * tile-manager soft reset is external to scalar600, but its falling
         * edge is sampled by the core clock.  Generate Minor's normal
         * SuspendThread redirect on that CPU edge so Fetch and Execute adopt
         * the same new stream before the context becomes inactive.  Directly
         * suspending from the scheduler's earlier-priority event leaves a
         * matching-stream instruction at the next commit head.
         */
        ThreadContext *thread = cpu.getContext(task_suspend_tid);
        const auto &resume_pc = thread->pcState();
        cpu.suspendContext(task_suspend_tid);
        cpu.fetchStats[task_suspend_tid]->numFetchSuspends++;
        updateBranchData(task_suspend_tid, BranchData::SuspendThread,
            MinorDynInst::bubble(), resume_pc, branch);
    } else if (!branch.isBubble()) {
        /* It's important that this is here to carry Fetch1 wakeups to Fetch1
         *  without overwriting them */
        DPRINTF(MinorInterrupt, "Execute skipping a cycle to allow old"
            " branch to complete\n");
    } else {
        ThreadID commit_tid = getCommittingThread();

        if (commit_tid != InvalidThreadID) {
            ExecuteThreadInfo& commit_info = executeInfo[commit_tid];

            DPRINTF(MinorExecute, "Attempting to commit [tid:%d]\n",
                    commit_tid);
            /* commit can set stalled flags observable to issue and so *must* be
             *  called first */
            if (commit_info.drainState != NotDraining) {
                if (commit_info.drainState == DrainCurrentInst) {
                    /* Commit only micro-ops, don't kill anything else */
                    commit(commit_tid, true, false, branch);

                    if (isInbetweenInsts(commit_tid))
                        setDrainState(commit_tid, DrainHaltFetch);

                    /* Discard any generated branch */
                    branch = BranchData::bubble();
                } else if (commit_info.drainState == DrainAllInsts) {
                    /* Kill all instructions */
                    while (getInput(commit_tid))
                        popInput(commit_tid);
                    commit(commit_tid, false, true, branch);
                }
            } else {
                /* Commit micro-ops only if interrupted.  Otherwise, commit
                 *  anything you like */
                DPRINTF(MinorExecute, "Committing micro-ops for interrupt[tid:%d]\n",
                        commit_tid);
                bool only_commit_microops = interrupted &&
                                            hasInterrupt(commit_tid);
                commit(commit_tid, only_commit_microops, false, branch);
            }

            /* Halt fetch, but don't do it until we have the current instruction in
             *  the bag */
            if (commit_info.drainState == DrainHaltFetch) {
                updateBranchData(commit_tid, BranchData::HaltFetch,
                        MinorDynInst::bubble(),
                        cpu.getContext(commit_tid)->pcState(), branch);

                cpu.wakeupOnEvent(Pipeline::ExecuteStageId);
                setDrainState(commit_tid, DrainAllInsts);
            }
        }
        /*
         * scalar600's id_stop is a property of the physical EX/WB state,
         * not of the presence of a younger instruction at the Execute input.
         * In particular, Decode stops presenting input as soon as DIV enters
         * EX.  A RoundRobin Minor policy then returns InvalidThreadID here;
         * deriving id_stop from issue_tid would incorrectly deassert it for
         * the entire divide interval.  Sample every active hardware context
         * before looking for a new instruction to issue.
         */
        rtlScalarIdStopRequest =
            rtlScalarIdStopRequest || venusBarrierIssueBlocked;
        rtlScalarNonDividerIdStopRequest =
            rtlScalarNonDividerIdStopRequest || venusBarrierIssueBlocked;
        if (venusRtlScalarTiming) {
            for (ThreadID tid = 0; tid < cpu.numThreads; ++tid) {
                if (cpu.getContext(tid)->status() ==
                        ThreadContext::Suspended)
                    continue;
                if (rtlScalarPipelineBlocksIssue(tid))
                    rtlScalarIdStopRequest = true;
                if (rtlScalarNonDividerPipelineBlocksIssue(tid))
                    rtlScalarNonDividerIdStopRequest = true;
            }
        }

        ThreadID issue_tid = getIssuingThread();
        /* This will issue merrily even when interrupted in the sure and
         *  certain knowledge that the interrupt with change the stream */
        if (issue_tid != InvalidThreadID && !venusBarrierIssueBlocked &&
            !rtlScalarIdStopRequest) {
            DPRINTF(MinorExecute, "Attempting to issue [tid:%d]\n",
                    issue_tid);
            num_issued = issue(issue_tid);
            if (num_issued != 0) {
                /*
                 * A scalar600 memory operation enters its LSU directly
                 * from ID/EX.  That path is independent of whether an older
                 * non-memory instruction happened to retire on this edge.
                 * In particular, the first load at a taken-control target
                 * arrives after the redirect bubbles, when there is no
                 * same-edge predecessor commit.  Restricting this launch to
                 * lastCommittedCycle therefore inserted one artificial
                 * cycle on every taken loop backedge whose target is a
                 * load.  The helper still requires the newly issued memory
                 * operation to be both the in-order and memory-FU head, and
                 * rejects an active stream change or unavailable LSQ.
                 */
                tryScalar600SameEdgeLsuLaunch(issue_tid, branch, true);
            }
        } else if (issue_tid != InvalidThreadID) {
            DPRINTF(MinorExecute,
                "scalar600 pipeline suppresses issue [tid:%d]"
                " barrier=%d id_stop=%d\n", issue_tid,
                venusBarrierIssueBlocked, rtlScalarIdStopRequest);
        }

        /*
         * A successful barrier release becomes visible to ID only after the
         * current edge.  Clear the gate after issue so the next Evaluate can
         * admit the first younger instruction.
         */
        if (venusBarrierReleasePending) {
            venusBarrierIssueBlocked = false;
            venusBarrierReleasePending = false;
        }
    }

    /* Run logic to step functional units + decide if we are active on the next
     * clock cycle */
    std::vector<MinorDynInstPtr> next_issuable_insts;
    bool can_issue_next = false;

    for (ThreadID tid = 0; tid < cpu.numThreads; tid++) {
        /* Find the next issuable instruction for each thread and see if it can
           be issued */
        if (getInput(tid)) {
            unsigned int input_index = executeInfo[tid].inputIndex;
            MinorDynInstPtr inst = getInput(tid)->insts[input_index];
            if (inst->isFault()) {
                can_issue_next = true;
            } else if (!inst->isBubble()) {
                next_issuable_insts.push_back(inst);
            }
        }
    }

    bool becoming_stalled = true;

    /* Advance the pipelines and note whether they still need to be
     * advanced */
    for (unsigned int i = 0; i < numFuncUnits; i++) {
        FUPipeline *fu = funcUnits[i];
        fu->advance();

        /* If we need to tick again, the pipeline will have been left or set
         * to be unstalled */
        if (fu->occupancy !=0 && !fu->stalled)
            becoming_stalled = false;

        /* Could we possibly issue the next instruction from any thread?
         * This is quite an expensive test and is only used to determine
         * if the CPU should remain active, only run it if we aren't sure
         * we are active next cycle yet */
        for (auto inst : next_issuable_insts) {
            if (!fu->stalled && fu->provides(inst->staticInst->opClass()) &&
                scoreboard[inst->id.threadId].canInstIssue(inst,
                    NULL, NULL, cpu.curCycle() + Cycles(1),
                    cpu.getContext(inst->id.threadId))) {
                can_issue_next = true;
                break;
            }
        }
    }

    bool head_inst_might_commit = false;

    /* Could the head in flight insts be committed */
    for (auto const &info : executeInfo) {
        if (!info.inFlightInsts->empty()) {
            const QueuedInst &head_inst = info.inFlightInsts->front();

            if (head_inst.inst->isNoCostInst()) {
                head_inst_might_commit = true;
            } else {
                FUPipeline *fu = funcUnits[head_inst.inst->fuIndex];
                if ((fu->stalled &&
                     fu->front().inst->id == head_inst.inst->id) ||
                     lsq.findResponse(head_inst.inst))
                {
                    head_inst_might_commit = true;
                    break;
                }
            }
        }
    }

    DPRINTF(Activity, "Need to tick num issued insts: %s%s%s%s%s%s\n",
       (num_issued != 0 ? " (issued some insts)" : ""),
       (becoming_stalled ? "(becoming stalled)" : "(not becoming stalled)"),
       (can_issue_next ? " (can issued next inst)" : ""),
       (head_inst_might_commit ? "(head inst might commit)" : ""),
       (lsq.needsToTick() ? " (LSQ needs to tick)" : ""),
       (interrupted ? " (interrupted)" : ""));

    bool need_to_tick =
       num_issued != 0 || /* Issued some insts this cycle */
       !becoming_stalled || /* Some FU pipelines can still move */
       can_issue_next || /* Can still issue a new inst */
       head_inst_might_commit || /* Could possible commit the next inst */
       lsq.needsToTick() || /* Must step the dcache port */
       interrupted; /* There are pending interrupts */

    if (!need_to_tick) {
        DPRINTF(Activity, "The next cycle might be skippable as there are no"
            " advanceable FUs\n");
    }

    /* Wake up if we need to tick again */
    if (need_to_tick)
        cpu.wakeupOnEvent(Pipeline::ExecuteStageId);

    /* Note activity of following buffer */
    if (!branch.isBubble())
        cpu.activityRecorder->activity();

    /* Make sure the input (if any left) is pushed */
    if (!inp.outputWire->isBubble())
        inputBuffer[inp.outputWire->threadId].pushTail();
}

ThreadID
Execute::checkInterrupts(BranchData& branch, bool& interrupted)
{
    ThreadID tid = interruptPriority;
    /* Evaluate interrupts in round-robin based upon service */
    do {
        /* Has an interrupt been signalled?  This may not be acted on
         *  straighaway so this is different from took_interrupt */
        bool thread_interrupted = false;

        if (cpu.getInterruptController(tid)) {
            /* This is here because it seems that after drainResume the
             * interrupt controller isn't always set */
            thread_interrupted = executeInfo[tid].drainState == NotDraining &&
                isInterrupted(tid);
            interrupted = interrupted || thread_interrupted;
        } else {
            DPRINTF(MinorInterrupt, "No interrupt controller\n");
        }
        DPRINTF(MinorInterrupt, "[tid:%d] thread_interrupted?=%d isInbetweenInsts?=%d\n",
                tid, thread_interrupted, isInbetweenInsts(tid));
        /* Act on interrupts */
        if (thread_interrupted && isInbetweenInsts(tid)) {
            if (takeInterrupt(tid, branch)) {
                interruptPriority = tid;
                return tid;
            }
        } else {
            tid = (tid + 1) % cpu.numThreads;
        }
    } while (tid != interruptPriority);

    return InvalidThreadID;
}

bool
Execute::hasInterrupt(ThreadID thread_id)
{
    if (FullSystem && cpu.getInterruptController(thread_id)) {
        return executeInfo[thread_id].drainState == NotDraining &&
               isInterrupted(thread_id);
    }

    return false;
}

void
Execute::minorTrace() const
{
    std::ostringstream insts;
    std::ostringstream stalled;

    executeInfo[0].instsBeingCommitted.reportData(insts);
    lsq.minorTrace();
    inputBuffer[0].minorTrace();
    scoreboard[0].minorTrace();

    /* Report functional unit stalling in one string */
    unsigned int i = 0;
    while (i < numFuncUnits)
    {
        stalled << (funcUnits[i]->stalled ? '1' : 'E');
        i++;
        if (i != numFuncUnits)
            stalled << ',';
    }

    minor::minorTrace("insts=%s inputIndex=%d streamSeqNum=%d"
        " stalled=%s drainState=%d isInbetweenInsts=%d\n",
        insts.str(), executeInfo[0].inputIndex, executeInfo[0].streamSeqNum,
        stalled.str(), executeInfo[0].drainState, isInbetweenInsts(0));

    std::for_each(funcUnits.begin(), funcUnits.end(),
        std::mem_fn(&FUPipeline::minorTrace));

    executeInfo[0].inFlightInsts->minorTrace();
    executeInfo[0].inFUMemInsts->minorTrace();
}

inline ThreadID
Execute::getCommittingThread()
{
    std::vector<ThreadID> priority_list;

    switch (cpu.threadPolicy) {
      case enums::SingleThreaded:
          return 0;
      case enums::RoundRobin:
          priority_list = cpu.roundRobinPriority(commitPriority);
          break;
      case enums::Random:
          priority_list = cpu.randomPriority();
          break;
      default:
          panic("Invalid thread policy");
    }

    for (auto tid : priority_list) {
        ExecuteThreadInfo &ex_info = executeInfo[tid];
        bool can_commit_insts = !ex_info.inFlightInsts->empty();
        if (can_commit_insts) {
            QueuedInst *head_inflight_inst = &(ex_info.inFlightInsts->front());
            MinorDynInstPtr inst = head_inflight_inst->inst;

            can_commit_insts = can_commit_insts &&
                (!inst->inLSQ || (lsq.findResponse(inst) != NULL));

            if (!inst->inLSQ) {
                bool can_transfer_mem_inst = false;
                if (!ex_info.inFUMemInsts->empty() && lsq.canRequest()) {
                    const MinorDynInstPtr head_mem_ref_inst =
                        ex_info.inFUMemInsts->front().inst;
                    FUPipeline *fu = funcUnits[head_mem_ref_inst->fuIndex];
                    const MinorDynInstPtr &fu_inst = fu->front().inst;
                    can_transfer_mem_inst =
                        !fu_inst->isBubble() &&
                         fu_inst->id.threadId == tid &&
                         !fu_inst->inLSQ &&
                         fu_inst->canEarlyIssue &&
                         inst->id.execSeqNum > fu_inst->instToWaitFor;
                }

                bool can_execute_fu_inst = inst->fuIndex == noCostFUIndex;
                if (can_commit_insts && !can_transfer_mem_inst &&
                        inst->fuIndex != noCostFUIndex)
                {
                    QueuedInst& fu_inst = funcUnits[inst->fuIndex]->front();
                    can_execute_fu_inst = !fu_inst.inst->isBubble() &&
                        fu_inst.inst->id == inst->id;
                }

                can_commit_insts = can_commit_insts &&
                    (can_transfer_mem_inst || can_execute_fu_inst);
            }
        }


        if (can_commit_insts) {
            commitPriority = tid;
            return tid;
        }
    }

    return InvalidThreadID;
}

inline ThreadID
Execute::getIssuingThread()
{
    std::vector<ThreadID> priority_list;

    switch (cpu.threadPolicy) {
      case enums::SingleThreaded:
          return 0;
      case enums::RoundRobin:
          priority_list = cpu.roundRobinPriority(issuePriority);
          break;
      case enums::Random:
          priority_list = cpu.randomPriority();
          break;
      default:
          panic("Invalid thread scheduling policy.");
    }

    for (auto tid : priority_list) {
        if (getInput(tid)) {
            issuePriority = tid;
            return tid;
        }
    }

    return InvalidThreadID;
}

void
Execute::drainResume()
{
    DPRINTF(Drain, "MinorExecute drainResume\n");

    for (ThreadID tid = 0; tid < cpu.numThreads; tid++) {
        setDrainState(tid, NotDraining);
    }

    cpu.wakeupOnEvent(Pipeline::ExecuteStageId);
}

std::ostream &operator <<(std::ostream &os, Execute::DrainState state)
{
    switch (state)
    {
        case Execute::NotDraining:
          os << "NotDraining";
          break;
        case Execute::DrainCurrentInst:
          os << "DrainCurrentInst";
          break;
        case Execute::DrainHaltFetch:
          os << "DrainHaltFetch";
          break;
        case Execute::DrainAllInsts:
          os << "DrainAllInsts";
          break;
        default:
          os << "Drain-" << static_cast<int>(state);
          break;
    }

    return os;
}

void
Execute::setDrainState(ThreadID thread_id, DrainState state)
{
    DPRINTF(Drain, "setDrainState[%d]: %s\n", thread_id, state);
    executeInfo[thread_id].drainState = state;
}

unsigned int
Execute::drain()
{
    DPRINTF(Drain, "MinorExecute drain\n");

    for (ThreadID tid = 0; tid < cpu.numThreads; tid++) {
        if (executeInfo[tid].drainState == NotDraining) {
            cpu.wakeupOnEvent(Pipeline::ExecuteStageId);

            /* Go to DrainCurrentInst if we're between microops
             * or waiting on an unbufferable memory operation.
             * Otherwise we can go straight to DrainHaltFetch
             */
            if (isInbetweenInsts(tid))
                setDrainState(tid, DrainHaltFetch);
            else
                setDrainState(tid, DrainCurrentInst);
        }
    }
    return (isDrained() ? 0 : 1);
}

bool
Execute::isDrained()
{
    if (!lsq.isDrained())
        return false;

    for (ThreadID tid = 0; tid < cpu.numThreads; tid++) {
        if (!inputBuffer[tid].empty() ||
            !executeInfo[tid].inFlightInsts->empty()) {

            return false;
        }
    }

    return true;
}

Execute::~Execute()
{
    for (unsigned int i = 0; i < numFuncUnits; i++)
        delete funcUnits[i];

    for (ThreadID tid = 0; tid < cpu.numThreads; tid++)
        delete executeInfo[tid].inFlightInsts;
}

bool
Execute::instIsRightStream(MinorDynInstPtr inst)
{
    return inst->id.streamSeqNum == executeInfo[inst->id.threadId].streamSeqNum;
}

bool
Execute::instIsHeadInst(MinorDynInstPtr inst)
{
    bool ret = false;

    if (!executeInfo[inst->id.threadId].inFlightInsts->empty())
        ret = executeInfo[inst->id.threadId].inFlightInsts->front().inst->id == inst->id;

    return ret;
}

MinorCPU::MinorCPUPort &
Execute::getDcachePort()
{
    return lsq.getDcachePort();
}

} // namespace minor
} // namespace gem5
