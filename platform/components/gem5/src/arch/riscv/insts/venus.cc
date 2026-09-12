#include "arch/riscv/insts/venus.hh"
#include "base/trace.hh"

namespace gem5 {
namespace RiscvISA {

Fault
VenusStaticInst::execute(ExecContext *xc, trace::InstRecord *traceData) const
{
    // std::cout << "[VenusStaticInst]====== Venus Execute ======" << std::endl;
    // std::cout << "[VenusStaticInst]  Venus Execute machInst:          0x"
    //           << std::hex << std::setfill('0') << std::setw(16) << venusInstBits
    //           << std::dec << std::setfill(' ')  // 恢复默认的十进制和空格补位
    //           << std::endl;
    return NoFault;
}

std::string
VenusStaticInst::generateDisassembly(
    Addr pc, const loader::SymbolTable *symtab) const
{
    return csprintf("venus 0x%016x", venusInstBits);
}

} // namespace RiscvISA
} // namespace gem5
