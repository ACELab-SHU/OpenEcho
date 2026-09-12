#include "venus_extension_pkg.hh"

#include "sim/sim_object.hh"

namespace gem5
{

int NrLanes = 64;
int NrLines = 512;
int NrBankPerLane = 4;
int NrBytesPerBank = 2;

std::string vew_to_str(VEW enum_input){
    std::string vew_to_str;
    switch(enum_input) {
        case EW8: vew_to_str = "EW8";break;
        case EW16: vew_to_str = "EW16";break;
        default: panic("Unknown EW type: %d",enum_input);break;
    }
    return vew_to_str;
}

std::string vfu_to_str(VFU enum_input){
    std::string vfu_to_str;
    switch(enum_input) {
        case VFU_BitALU: vfu_to_str = "VFU_BitALU";break;
        case VFU_CAU: vfu_to_str = "VFU_CAU";break;
        case VFU_SerDiv: vfu_to_str = "VFU_SerDiv";break;
        case VFU_ShuffleUnit: vfu_to_str = "VFU_ShuffleUnit";break;
        case VFU_Mask: vfu_to_str = "VFU_Mask";break;
        case VFU_NONE: vfu_to_str = "VFU_NONE";break;
        default: panic("Unknown VFU type: %d",enum_input);break;
    }
    return vfu_to_str;
}

std::string ot_to_str(FUNC3 enum_input){
    std::string ot_to_str;
    switch(enum_input) {
      case IVV: ot_to_str = "IVV";break;
      case IVX: ot_to_str = "IVX";break;
      case MVV: ot_to_str = "MVV";break;
      case MVX: ot_to_str = "MVX";break;
      case OPMISC: ot_to_str = "OPMISC";break;
      default: panic("Unknown OT type: %d",enum_input);break;
    }
    return ot_to_str;
}

std::string op_to_str(VenusOp enum_input){
    std::string op_to_str;
    switch(enum_input) {
        case VAND: op_to_str = "VAND";break;
        case VOR: op_to_str = "VOR";break;
        case VXOR: op_to_str = "VXOR";break;
        case VBRDCST: op_to_str = "VBRDCST";break;
        case VSHUFFLE_CLBMV: op_to_str = "VSHUFFLE_CLBMV";break;
        case VSLL: op_to_str = "VSLL";break;
        case VSRL: op_to_str = "VSRL";break;
        case VSRA: op_to_str = "VSRA";break;
        case VMNOT: op_to_str = "VMNOT";break;
        case VSEQ: op_to_str = "VSEQ";break;
        case VSNE: op_to_str = "VSNE";break;
        case VSLTU: op_to_str = "VSLTU";break;
        case VSLT: op_to_str = "VSLT";break;
        case VSLEU: op_to_str = "VSLEU";break;
        case VSLE: op_to_str = "VSLE";break;
        case VSGTU: op_to_str = "VSGTU";break;
        case VSGT: op_to_str = "VSGT";break;
        case VADD: op_to_str = "VADD";break;
        case VSADD: op_to_str = "VSADD";break;
        case VSADDU: op_to_str = "VSADDU";break;
        case VRANGE: op_to_str = "VRANGE";break;
        case VRSUB: op_to_str = "VRSUB";break;
        case VSSUB: op_to_str = "VSSUB";break;
        case VSSUBU: op_to_str = "VSSUBU";break;
        case VSUB: op_to_str = "VSUB";break;
        case VMUL: op_to_str = "VMUL";break;
        case VMULH: op_to_str = "VMULH";break;
        case VMULHU: op_to_str = "VMULHU";break;
        case VMULHSU: op_to_str = "VMULHSU";break;
        case VMULADD: op_to_str = "VMULADD";break;
        case VMULSUB: op_to_str = "VMULSUB";break;
        case VADDMUL: op_to_str = "VADDMUL";break;
        case VSUBMUL: op_to_str = "VSUBMUL";break;
        case VCMXMUL: op_to_str = "VCMXMUL";break;
        case VDIV: op_to_str = "VDIV";break;
        case VREM: op_to_str = "VREM";break;
        case VDIVU: op_to_str = "VDIVU";break;
        case VREMU: op_to_str = "VREMU";break;
        case VMIN: op_to_str = "VMIN";break;
        case VMAX: op_to_str = "VMAX";break;
        case VABS: op_to_str = "VSIGNSET";break;
        case VSHUFFLE: op_to_str = "VSHUFFLE";break;
        case VSTORE: op_to_str = "VSTORE";break;
        case VLOAD: op_to_str = "VLOAD";break;
        case VREDAND: op_to_str = "VREDAND";break;
        case VREDOR: op_to_str = "VREDOR";break;
        case VREDXOR: op_to_str = "VREDXOR";break;
        case VREDMAX: op_to_str = "VREDMAX";break;
        case VREDMAXU: op_to_str = "VREDMAXU";break;
        case VREDMIN: op_to_str = "VREDMIN";break;
        case VREDMINU: op_to_str = "VREDMINU";break;
        case VREDSUM: op_to_str = "VREDSUM";break;
        case YIELD: op_to_str = "YIELD";break;
        case VPRINTF: op_to_str = "VPRINTF";break;
        default: panic("Unknown OP: %d",enum_input);break;
    }
    return op_to_str;
}

std::string instr_stat_to_str(INSTR_STAT enum_input){
    std::string instr_stat_to_str;
    switch(enum_input) {
      case INSTR_GENERATED: instr_stat_to_str = "INSTR_GENERATED";break;
      case INSTR_QUEUED: instr_stat_to_str = "INSTR_QUEUED";break;
      case INSTR_FIRED: instr_stat_to_str = "INSTR_FIRED";break;
      case INSTR_DONE: instr_stat_to_str = "INSTR_DONE";break;
      default: panic("Unknown STAT type: %d",enum_input);break;
    }
    return instr_stat_to_str;
}

std::string int2Hex(int val) {
	std::stringstream ss;
	// 整数转换为大写的十六进制字符串，且每个字节占用两个字符的宽度
	ss << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << val;
	return ss.str();
}

}
