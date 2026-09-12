/*
 * @Author: Yihao Shen shenyihao@shu.edu.cn
 * @Date: 2025-04-16 17:34:05
 * @LastEditors: Yihao Shen shenyihao@shu.edu.cn
 * @LastEditTime: 2025-06-08 14:41:09
 * @FilePath: /VEMU/AceEcho/tasks/ltePBCH/Task_lteEqualizeMMSE.c
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
/**
 * ****************************************
 * @file        Task_lteEqualizeMMSE.c
 * @brief       equalization using ZF
 * @author      yuanfeng
 * @date        2024.7.25
 * @copyright   ACE-Lab(Shanghai University)
 * ****************************************
 */

#include "riscv_printf.h"
#include "venus.h"
#include "data_type.h"

typedef short v2048i16 __attribute__((ext_vector_type(2048)));
typedef char  v4096i8 __attribute__((ext_vector_type(4096)));

int Task_lteEqualizeMMSE(v4096i8 rxData_real, v4096i8 rxData_imag, v4096i8 H_real, v4096i8 H_imag,
   short_struct input_sequence_length, short_struct input_bLength, short_struct input_data_symbol_length) {

  int   sequence_length    = input_sequence_length.data;
  short subcarrierLength   = input_bLength.data * 12;
  short data_symbol_length = input_data_symbol_length.data;


  // rxData_real = vsra(rxData_real,2,MASKREAD_OFF,sequence_length);
  // rxData_imag = vsra(rxData_imag,2,MASKREAD_OFF,sequence_length);

  // H_real = vsra(H_real,2,MASKREAD_OFF,sequence_length);
  // H_imag = vsra(H_imag,2,MASKREAD_OFF,sequence_length);

  int fractionLength = 7;
  int nVar           = 0;

  // /*--------------------ExtractResources--------------------*/
  // vshuffle(rxData_real, rxData_shuffle_index, rxData_real, SHUFFLE_GATHER, subcarrierLength * data_symbol_length);
  // vshuffle(rxData_imag, rxData_shuffle_index, rxData_imag, SHUFFLE_GATHER, subcarrierLength * data_symbol_length);

  // v4096i8 rxSym_real;
  // v4096i8 rxSym_imag;

  // vclaim(rxSym_real);
  // vclaim(rxSym_imag);
  // vshuffle(rxSym_real, pbch_index, rxData_real, SHUFFLE_GATHER, sequence_length);
  // vshuffle(rxSym_imag, pbch_index, rxData_imag, SHUFFLE_GATHER, sequence_length);

  // v4096i8 Hest_real;
  // v4096i8 Hest_imag;

  // vclaim(Hest_real);
  // vclaim(Hest_imag);
  // vshuffle(Hest_real, pbch_index, H_real, SHUFFLE_GATHER, sequence_length);
  // vshuffle(Hest_imag, pbch_index, H_imag, SHUFFLE_GATHER, sequence_length);

  /*--------------------Equalization--------------------*/
  v4096i8 csi;
  v4096i8 csi_tmp1;
  v4096i8 csi_tmp2;

  vsetshamt(fractionLength);
  csi_tmp1 = vmul(H_real, H_real, MASKREAD_OFF, sequence_length);
  csi_tmp2 = vmul(H_imag, H_imag, MASKREAD_OFF, sequence_length);
  vsetshamt(0);

  csi = vsadd(csi_tmp1, csi_tmp2, MASKREAD_OFF, sequence_length);
  csi = vsadd(csi, nVar, MASKREAD_OFF, sequence_length);

  v4096i8 pbchEq_real;
  v4096i8 pbchEq_imag;
  v4096i8 pbchEq_tmp1;
  v4096i8 pbchEq_tmp2;
  v4096i8 pbchEq_tmp3;
  v4096i8 pbchEq_tmp4;

  vsetshamt(fractionLength);
  H_real = vdiv(csi, H_real, MASKREAD_OFF, sequence_length);
  H_imag = vdiv(csi, H_imag, MASKREAD_OFF, sequence_length);
  vsetshamt(0);

  vsetshamt(fractionLength);
  pbchEq_tmp1 = vmul(H_real, rxData_real, MASKREAD_OFF, sequence_length);
  pbchEq_tmp2 = vmul(H_imag, rxData_imag, MASKREAD_OFF, sequence_length);
  vsetshamt(0);
  pbchEq_real = vsadd(pbchEq_tmp1, pbchEq_tmp2, MASKREAD_OFF, sequence_length);

  vsetshamt(fractionLength);
  pbchEq_tmp3 = vmul(H_real, rxData_imag, MASKREAD_OFF, sequence_length);
  pbchEq_tmp4 = vmul(H_imag, rxData_real, MASKREAD_OFF, sequence_length);
  vsetshamt(0);
  pbchEq_imag = vssub(pbchEq_tmp4, pbchEq_tmp3, MASKREAD_OFF, sequence_length);

  vreturn(pbchEq_real, sequence_length, pbchEq_imag, sequence_length, csi, sequence_length);
}