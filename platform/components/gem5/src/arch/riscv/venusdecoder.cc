#include "arch/riscv/venusdecoder.hh"
#include "arch/riscv/insts/venus.hh"
#include "base/bitfield.hh"
#include "debug/Decode.hh"
#include "venus/venus_extension_pkg.hh"
#include <unordered_map>

namespace gem5
{

namespace RiscvISA
{

RiscvVenusDecoder::RiscvVenusDecoder(const RiscvVenusDecoderParams &p)
    : Decoder(p), venusInstBuf(0), waitingForSecondPart(false),
      venusInstReady(false)
{
}

void RiscvVenusDecoder::reset()
{
    Decoder::reset();
    venusInstBuf = 0;
    waitingForSecondPart = false;
    venusInstReady = false;
}

bool RiscvVenusDecoder::isVenusInst(uint32_t inst_bits)
{
    uint8_t opcode = inst_bits & 0x7F;
    // custom-1: 0x2B, custom-2: 0x5B
    if (opcode != 0x2B && opcode != 0x5B)
        return false;
    FUNC3 func3 = static_cast<FUNC3>((inst_bits >> 12) & 0x7);
    uint8_t func5 = (inst_bits >> 27) & 0x1F;
    uint32_t bits_26_15 = (inst_bits >> 15) & 0xFFF;
    bool isMSBOnly = (func3 == OPMISC) &&
                     ((func5 == 0b00100) ||
                      (func5 == 0b00101) ||
                      (func5 == 0b00001 && bits_26_15 == 0));
    return !isMSBOnly;
}

bool RiscvVenusDecoder::isCPUExtInst(uint32_t inst_bits)
{
    uint8_t opcode = inst_bits & 0x7F;
    FUNC3 func3 = static_cast<FUNC3>((inst_bits >> 12) & 0x7);
    uint8_t func5 = (inst_bits >> 27) & 0x1F;
    uint32_t bits_26_15 = (inst_bits >> 15) & 0xFFF;
    bool isMSBOnly = (func3 == OPMISC) &&
                     ((func5 == 0b00100) ||
                      (func5 == 0b00101) ||
                      (func5 == 0b00001 && bits_26_15 == 0));
    return (opcode == 0x5B && isMSBOnly);
}

void RiscvVenusDecoder::moreBytes(const PCStateBase &pc, Addr fetchPC)
{
        // std::cout << "[Fetch] DECODE: emi.instBits=0x"
        //   << std::hex << std::setfill('0') << std::setw(8) << emi.instBits  // 8位十六进制，不足补0
        //   << ", fetchPC=0x"
        //   << std::setw(8) << fetchPC  // 同上，保持位数一致
        //   << std::dec << std::setfill(' ')  // 恢复默认的十进制和空格补位
        //   << std::endl;

    if (waitingForSecondPart) {
        // warn("  -> Branch: waitingForSecondPart, fetchPC=0x%x", fetchPC);
        // std::cout << "waitingForSecondPart isvenusinst" << std::endl;
        Decoder::moreBytes(pc, fetchPC);
        /* 原设计里面支持16bit压缩指令
         * 因此若刚好venus的高32bit指令的低2位是压缩指令的opcode
         * 则outOfBytes会返回false, 导致needMoreBytes返回false
         * cpu会将这部分指令误认为压缩指令fetch传入后续流水线
         * 因此此处将outOfBytes统一设置为true, 推进流水线 */
        outOfBytes = true;

        if (instDone) {
            uint32_t secondPart = emi.instBits;
            venusInstBuf = venusInstBuf | (uint64_t(secondPart) << 32);
            emi.instBits = venusInstBuf;

            // std::cout << "[DECODE] PC=0x" << std::hex << std::setfill('0') << std::setw(8) << fetchPC
            //           << " inst=0x" << std::setw(8) << secondPart
            //           << " [Venus HIGH 32-bit received]" << std::dec << std::endl;
            // std::cout << "[DECODE] *** Venus 64-bit complete: 0x" << std::hex << std::setfill('0')
            //           << std::setw(16) << venusInstBuf << std::dec
            //           << " (LOW=0x" << std::hex << std::setw(8) << (uint32_t)venusInstBuf
            //           << " HIGH=0x" << std::setw(8) << secondPart << std::dec << ")" << std::endl;

            waitingForSecondPart = false;
            venusInstReady = true;
        }
        return;
    }

    Decoder::moreBytes(pc, fetchPC);

    if (instDone) {
        uint32_t currentInst = emi.instBits;
        // DPRINTF(Fetch, "DECODE: emi.instBits=0x%x, fetchPC=0x%x", currentInst, fetchPC);
        if (isCPUExtInst(currentInst)) {
            // warn("morebytes iscpuextinst= 0x%x", currentInst);
            // std::cout << "morebytes iscpuextinst" << std::endl;
            // std::cout << "[DECODE] PC=0x" << std::hex << std::setfill('0') << std::setw(8) << fetchPC
            //           << " inst=0x" << std::setw(8) << currentInst
            //           << " [CPU Extension - Single 32-bit]" << std::dec << std::endl;
            venusInstBuf = currentInst;
            waitingForSecondPart = false;
            venusInstReady = true;
            // instDone = false;
            // outOfBytes = true;
        } else if (isVenusInst(currentInst)) {
            // warn("morebytes isvenusinst = 0x%x", currentInst);
            // std::cout << "morebytes isvenusinst" << std::endl;
            // std::cout << "[DECODE] PC=0x" << std::hex << std::setfill('0') << std::setw(8) << fetchPC
            //           << " inst=0x" << std::setw(8) << currentInst
            //           << " [Venus LOW 32-bit detected]" << std::dec << std::endl;

            venusInstBuf = currentInst;
            waitingForSecondPart = true;
            venusInstReady = false;
            instDone = false;
            outOfBytes = true;
        }
    }
}

StaticInstPtr RiscvVenusDecoder::decode(PCStateBase &nextPC)
{
    if (!instDone) {
        warn("  -> Returning nullptr (instDone=false)");
        return nullptr;
    }

    if (venusInstReady) {
        instDone = false;
        venusInstReady = false;
        uint64_t venus_inst;
        venus_inst = venusInstBuf;
        venusInstBuf = 0;
        bool isCPUExt = isCPUExtInst(static_cast<uint32_t>(venus_inst));
        uint8_t func5 = (venus_inst >> 27) & 0x1F;
        VenusCPUOp Op;
        if ((func5 == 0b00100) && isCPUExt) {
            Op = VSETCSR;
        } else if ((func5 == 0b00101) && isCPUExt) {
            Op = VSETCSRIMM;
        } else if ((func5 == 0b00001) && isCPUExt) {
            Op = VBARRIER;
        } else {
            Op = VENUSEXT;
        }
        StaticInstPtr si = decodeVenusInst(venus_inst, Op);
        return si;
    }

    StaticInstPtr si = Decoder::decode(nextPC);
    // if (si) {
    //     warn("  Decoded: %s at PC=0x%lx",
    //          si->getName(), nextPC.instAddr());
    // }
    return si;
}

StaticInstPtr
RiscvVenusDecoder::decodeVenusInst(uint64_t mach_venus_inst, VenusCPUOp Op)
{
    using CacheKey = std::pair<uint64_t, VenusCPUOp>;
    static std::map<CacheKey, StaticInstPtr> instCache;
    CacheKey key = {mach_venus_inst, Op};
    auto it = instCache.find(key);
    if (it != instCache.end()) {
        // 存在则直接返回，不new
        return it->second;
    } else {
        // 不存在则new，并存入缓存
        StaticInstPtr inst = new VenusStaticInst("venus", mach_venus_inst, Op);
        instCache[key] = inst;
        return inst;
    }
    // return new VenusStaticInst("venus", mach_venus_inst, Op);
}

} // namespace RiscvISA
} // namespace gem5
