#ifndef __VENUS_EXTENSION_PKG_HH__
#define __VENUS_EXTENSION_PKG_HH__

#include <string>
#include <cstdint>
#include <sstream>
#include <iomanip>
#include <climits>
#include "sim/sim_object.hh"

#define NrIDs 8
#define MaxNrLanes 64
// #define NrLines 512
// #define NrBankPerLane 4
#define NrBitsPerBank 16
#define NrVFUs 5

#define BitaluInsnQueueDepth 4
#define BitaluDataQueueDepth 4
#define CauInsnQueueDepth 4
#define CauDataQueueDepth 4
#define SerdivInsnQueueDepth 4
#define SerdivDataQueueDepth 4
#define VmaskInsnQueueDepth 5
#define VmaskDataQueueDepth 5
/*
 * The Venus1 RTL compiled by the venus1p0 backend has NrVFUs == 5 and vfu_e
 * ordered BitALU, CAU, SerDiv, Shuffle, Mask.  venus_sequencer.sv initializes
 * InsnQueueDepth in that same order, so the sequencer-visible Mask capacity
 * is the real VmaskInsnQueueDepth (5), matching venus_mask.sv's command/data
 * buffers.  Keep the alias explicit because other Venus generations have a
 * different VFU enumeration and are selected through separate profiles.
 */
#define SequencerVmaskInsnQueueDepth VmaskInsnQueueDepth
#define ShuffleInsnQueueDepth 2
#define ShuffleDataQueueDepth 2

// typedef unsigned char uint8_t;
// typedef unsigned short uint16_t;
// typedef char int8_t;
// typedef short int16_t;

namespace gem5
{
extern int NrLanes;
extern int NrLines;
extern int NrBankPerLane;
extern int NrBytesPerBank;

// 枚举类型 VenusOp
typedef enum  {
    VAND, VOR, VXOR,
    VBRDCST,
    VSLL, VSRL, VSRA,
    VSEQ, VSNE, VSLTU, VSLT, VSLEU, VSLE, VSGTU, VSGT,
    VABS,
    VADD, VSADD, VSADDU, VRANGE, VRSUB, VSUB, VSSUB, VSSUBU,
    VMUL, VMULH, VMULHU, VMULHSU,
    VMULADD, VMULSUB, VADDMUL, VSUBMUL,
    VCMXMUL,
    VMIN, VMAX,
    VDIV, VREM, VDIVU, VREMU,
    VSHUFFLE,
    VSTORE, VLOAD,
    VREDAND, VREDOR, VREDXOR, VREDMAX, VREDMAXU, VREDMIN, VREDMINU, VREDSUM,
    YIELD, VSHUFFLE_CLBMV, VMNOT,
    VPRINTF
} VenusOp;


typedef enum   { EW8, EW16 }VEW;
typedef enum   { NORMAL_READ, READ_MASKED }VMASK_READ;
typedef enum   { NORMAL_WRITE, WRITE_MASKED }VMASK_WRITE;
typedef enum   { IVV, IVX, IVI, MVV, MVX, MVI, nop, OPMISC }FUNC3;
typedef enum   { VFU_BitALU, VFU_CAU, VFU_SerDiv, VFU_ShuffleUnit, VFU_Mask, VFU_NONE }VFU;

typedef enum   { INSTR_GENERATED, INSTR_QUEUED, INSTR_FIRED, INSTR_DONE, INSTR_RECYCLE }INSTR_STAT;

typedef enum   { BitAlu_A = 0, BitAlu_B = 1, CAU_A = 2, CAU_B = 3, CAU_C = 4, CAU_D = 5, SerDiv_A = 6, SerDiv_B = 7, Mask = 8, ShuffleUnit = 9 } OPERANDTYPE;

typedef enum   { OperandPassage_BitALU = -1, OperandPassage_BitALUMask = -2, OperandPassage_CAUA = -3, OperandPassage_CAUB = -4, OperandPassage_SerDiv = -5, OperandPassage_ShuffleUnit = -6 }OPERANDPASSAGE;

std::string vew_to_str(VEW enum_input);

std::string vfu_to_str(VFU enum_input);

std::string ot_to_str(FUNC3 enum_input);

std::string op_to_str(VenusOp enum_input);

std::string instr_stat_to_str(INSTR_STAT enum_input);

std::string int2Hex(int val);

}
#endif // __VENUS_EXTENSION_PKG_HH__
