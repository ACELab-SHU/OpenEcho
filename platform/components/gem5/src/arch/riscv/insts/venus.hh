#ifndef __ARCH_RISCV_INSTS_VENUS_HH__
#define __ARCH_RISCV_INSTS_VENUS_HH__

#include "arch/riscv/insts/static_inst.hh"
#include "arch/riscv/isa.hh"
#include "cpu/exec_context.hh"
#include "arch/generic/pcstate.hh"
#include "venus/venus_instr_pkt.hh"
#include "arch/riscv/regs/int.hh"


namespace gem5 {
namespace RiscvISA {

typedef enum {
    VSETCSR,
    VSETCSRIMM,
    VBARRIER,
    VENUSEXT
} VenusCPUOp;

class RiscvVenusDecoder;

class VenusStaticInst : public RiscvStaticInst {
private:
    RegId srcRegIdxArr[2];
    RegId destRegIdxArr[0];
public:
    VenusStaticInst(const char *mnem, uint64_t _machInst, VenusCPUOp Op)
        : RiscvStaticInst(mnem, _machInst, No_OpClass),
          venusInstBits(_machInst),
          venusOp(Op)
    {
        flags[IsNonSpeculative] = true;
        flags[IsInteger] = true;

        setRegIdxArrays(
            reinterpret_cast<RegIdArrayPtr>(
                &std::remove_pointer_t<decltype(this)>::srcRegIdxArr),
                nullptr
        );

        if (Op == VENUSEXT) {
            uint32_t msb = venusInstBits & 0xffffffff;
            FUNC3 func3 = static_cast<FUNC3>((msb >> 12) & 0x7);
            uint8_t avl_id = (venusInstBits >> 7) & 0x1F;
            uint8_t vs1_id = (venusInstBits >> 32) & 0x1F;
            if (func3 == IVV) {
                setSrcRegIdx(_numSrcRegs++, intRegClass[avl_id]);
            } else if (func3 == IVX) {
                setSrcRegIdx(_numSrcRegs++, intRegClass[avl_id]);
                setSrcRegIdx(_numSrcRegs++, intRegClass[vs1_id]);
            }
        }
        if (Op == VSETCSR) {
            uint32_t vs_id = (venusInstBits >> 7) & 0x1F;
            setSrcRegIdx(_numSrcRegs++, intRegClass[vs_id]);
        }

        const char* opName;
        switch(Op) {
            case VSETCSR:      opName = "VSETCSR"; break;
            case VSETCSRIMM:   opName = "VSETCSRIMM"; break;
            case VBARRIER:     opName = "VBARRIER"; break;
            case VENUSEXT:     opName = "VENUSEXT"; break;
            default:           opName = "UNKNOWN"; break;
        }
        // std::cout << "[VenusStaticInst] Creating " << opName
        //           << " inst=0x" << std::hex << std::setfill('0') << std::setw(16)
        //           << _machInst << " size=" << std::dec << (int)_size << std::endl;
    }

    void
    advancePC(PCStateBase &pc) const override
    {
        auto &venus_pc = pc.as<PCState>();
        // 204->208
        // 20c->210
        // 11532000: system.cpu.fetch2: PC before advancing: (0xffffffff80000204=>0xffffffff80000208).(0=>1)
        // 11532000: system.cpu.fetch2: PC after advancing: (0xffffffff80000210=>0xffffffff8000020c).(0=>1)

        // 11532000: system.cpu.fetch2: PC before advancing: (0xffffffff80000204=>0xffffffff80000208).(0=>1)
        // 11532000: system.cpu.fetch2: PC after advancing: (0xffffffff8000020c=>0xffffffff80000210).(0=>1)
        if (venusOp == VENUSEXT) {
            venus_pc.pc(venus_pc.npc() + 4);
            venus_pc.npc(venus_pc.npc() + 8);
        } else {
            venus_pc.pc(venus_pc.npc());
            venus_pc.npc(venus_pc.npc() + 4);
        }
    }

    Fault execute(ExecContext *xc, trace::InstRecord *traceData) const override;
    std::string generateDisassembly(Addr pc, const loader::SymbolTable *symtab) const override;

    uint64_t venusInstBits;
    VenusCPUOp venusOp;
};

} // namespace RiscvISA
} // namespace gem5

#endif // __ARCH_RISCV_INSTS_VENUS_HH__
