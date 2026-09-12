#ifndef __SIM_VENUS_VFU_HH__
#define __SIM_VENUS_VFU_HH__

#include "debug/LaneVFUFull.hh"
#include "venus_extension_pkg.hh"
#include "venus_instr_pkt.hh"
#include <cstdint>

namespace gem5
{
class venus_vfu {
public:
  struct VENUS_INS_PARAM {
    // uint16_t avl;
    bool vmask_read;
    bool vmask_write;
    VEW vew;
    FUNC3 function3;
    // uint16_t op_code;
    // uint16_t function5;
    // uint16_t vd1_head;
    // uint16_t vd2_head;
    // uint16_t vs1_head;
    // uint16_t vs2_head;
    int16_t scalar_op;
    VenusOp op;
    // uint16_t vd1_msb;
    // uint16_t vs1_msb;
    // uint16_t vs2_msb;
    uint8_t vfu_shamt;
    // uint8_t high_vd2_bits; // Current 3bits
    // uint8_t high_vd1_bits; // Current 2bits
    // uint8_t high_vs2_bits; // Current 2bits
    // uint8_t high_vs1_bits; // Current 2bits
    uint8_t saturate_pre_adder;
    uint8_t saturate_multiplier;
    uint8_t saturate_post_adder;
  } MY_VENUS_INS_PARAM;

  unsigned int localLaneID;


  char *instr_name;
  unsigned int data_vd1;
  unsigned int data_vd2;
  unsigned int data_vs1;
  unsigned int data_vs2;
  unsigned int data_vmask_r;
  unsigned int data_vmask_w;

  unsigned int data_vmask_r_element;

  void doVenusVFU(VFU vfu, VenusInstrPkt* instr_pkt, unsigned int *vd1_w, unsigned int *vd2_w, unsigned int *vd1_r, unsigned int *vd2_r, unsigned int *vmask_w, unsigned int *vs1, unsigned int *vs2, unsigned int *vmask_r);

  void doVEMU(VenusInstrPkt* instr_pkt, uint16_t vd1_mask, uint16_t vd2_mask, uint16_t vs1_mask, uint16_t vs2_mask);

  void resetReduceStat();

  uint8_t and_value = 255;
  uint16_t and_value_1 = 65535;
  uint8_t or_value = 0;
  uint16_t or_value_1 = 0;
  uint8_t xor_value = 0;
  uint16_t xor_value_1 = 0;
  int8_t min_value = 127;
  int16_t min_value_1 = 32767;
  uint8_t min_uvalue = 255;
  uint16_t min_uvalue_1 = 65535;
  int8_t max_value = -128;
  int16_t max_value_1 = -32768;
  uint8_t max_uvalue = 0;
  uint16_t max_uvalue_1 = 0;
  int32_t sum_result = 0;
};
}

#endif //__SIM_VENUS_VFU_HH__
