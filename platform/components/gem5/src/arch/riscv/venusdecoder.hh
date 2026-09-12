#ifndef __ARCH_RISCV_VENUSDECODER_HH__
#define __ARCH_RISCV_VENUSDECODER_HH__

#include "arch/riscv/decoder.hh"
#include "arch/riscv/types.hh"
#include "cpu/static_inst.hh"
#include "params/RiscvVenusDecoder.hh"
#include "arch/riscv/insts/venus.hh"

namespace gem5
{

namespace RiscvISA
{

class RiscvVenusDecoder : public Decoder
{
  protected:
    uint64_t venusInstBuf;
    bool waitingForSecondPart;
    bool venusInstReady;
  public:
    using Params = RiscvVenusDecoderParams;
    explicit RiscvVenusDecoder(const RiscvVenusDecoderParams &p);
    void reset() override;
    void moreBytes(const PCStateBase &pc, Addr fetchPC) override;
    StaticInstPtr decode(PCStateBase &nextPC) override;

  private:
    bool isVenusInst(uint32_t inst_bits);
    bool isCPUExtInst(uint32_t inst_bits);

    StaticInstPtr decodeVenusInst(uint64_t venus_inst, VenusCPUOp Op);
};

} // namespace RiscvISA
} // namespace gem5

#endif // __ARCH_RISCV_VENUSDECODER_HH__
