/*
 * @author: shenyihao shenyihao@shu.edu.cn
 * @Date: 2024-11-28 16:43:43
 * @LastEditors: shenyihao shenyihao@shu.edu.cn
 * @LastEditTime: 2025-03-12 16:53:43
 * @FilePath: /5G-Lite-0530/LTE_ECHO/5g_lite/Tasks/ltePBCH/Task_inputSplit.c
 * @Description:
 *
 * Copyright (c) 2024 by ACE_Lab（Shanghai University）, All Rights Reserved.
 */

#include "riscv_printf.h"
#include "venus.h"

typedef char __v8192i8 __attribute__((ext_vector_type(8192)));
typedef short __v4096i16 __attribute__((ext_vector_type(4096)));
typedef char __v4096i8 __attribute__((ext_vector_type(4096)));
typedef short __v2048i16 __attribute__((ext_vector_type(2048)));

int symbolLength = 2192;
int symbolLength_07 = 2208;

// int Task_ltePBCHinputSplit(__v8192i8 dfe_input_0, __v8192i8 dfe_input_1, __v8192i8 dfe_input_2, __v8192i8 dfe_input_3,
//                            __v8192i8 dfe_input_4, __v8192i8 dfe_input_5, __v8192i8 dfe_input_6, __v8192i8 dfe_input_7,
//                            __v8192i8 dfe_input_8) {

int Task_ltePBCHinputSplit(__v8192i8 dfe_input_0) {

  // generate shuffle index
  __v4096i16 shuffle_index;
  vclaim(shuffle_index);
  vrange(shuffle_index, symbolLength_07);
  shuffle_index = vmul(shuffle_index, 2, MASKREAD_OFF, symbolLength_07);
  shuffle_index = vadd(shuffle_index, 4, MASKREAD_OFF, symbolLength_07);

  __v8192i8 dfe_output_real_0;
//   __v8192i8 dfe_output_real_1;
//   __v8192i8 dfe_output_real_2;
//   __v8192i8 dfe_output_real_3;
//   __v8192i8 dfe_output_real_4;
//   __v8192i8 dfe_output_real_5;
//   __v8192i8 dfe_output_real_6;
//   __v8192i8 dfe_output_real_7;
//   __v8192i8 dfe_output_real_8;

  __v8192i8 dfe_output_imag_0;
//   __v8192i8 dfe_output_imag_1;
//   __v8192i8 dfe_output_imag_2;
//   __v8192i8 dfe_output_imag_3;
//   __v8192i8 dfe_output_imag_4;
//   __v8192i8 dfe_output_imag_5;
//   __v8192i8 dfe_output_imag_6;
//   __v8192i8 dfe_output_imag_7;
//   __v8192i8 dfe_output_imag_8;

  vclaim(dfe_output_real_0);
//   vclaim(dfe_output_real_1);
//   vclaim(dfe_output_real_2);
//   vclaim(dfe_output_real_3);
//   vclaim(dfe_output_real_4);
//   vclaim(dfe_output_real_5);
//   vclaim(dfe_output_real_6);
//   vclaim(dfe_output_real_7);
//   vclaim(dfe_output_real_8);

  vclaim(dfe_output_imag_0);
//   vclaim(dfe_output_imag_1);
//   vclaim(dfe_output_imag_2);
//   vclaim(dfe_output_imag_3);
//   vclaim(dfe_output_imag_4);
//   vclaim(dfe_output_imag_5);
//   vclaim(dfe_output_imag_6);
//   vclaim(dfe_output_imag_7);
//   vclaim(dfe_output_imag_8);

  // extract real data
  vshuffle(dfe_output_real_0, shuffle_index, dfe_input_0, SHUFFLE_GATHER, symbolLength);
//   vshuffle(dfe_output_real_1, shuffle_index, dfe_input_1, SHUFFLE_GATHER, symbolLength_07);
//   vshuffle(dfe_output_real_2, shuffle_index, dfe_input_2, SHUFFLE_GATHER, symbolLength);
//   vshuffle(dfe_output_real_3, shuffle_index, dfe_input_3, SHUFFLE_GATHER, symbolLength);
//   vshuffle(dfe_output_real_4, shuffle_index, dfe_input_4, SHUFFLE_GATHER, symbolLength);
//   vshuffle(dfe_output_real_5, shuffle_index, dfe_input_5, SHUFFLE_GATHER, symbolLength_07);
//   vshuffle(dfe_output_real_6, shuffle_index, dfe_input_6, SHUFFLE_GATHER, symbolLength);
//   vshuffle(dfe_output_real_7, shuffle_index, dfe_input_7, SHUFFLE_GATHER, symbolLength);
//   vshuffle(dfe_output_real_8, shuffle_index, dfe_input_8, SHUFFLE_GATHER, symbolLength);

  // extract imag data
  shuffle_index = vsadd(shuffle_index, 1, MASKREAD_OFF, symbolLength);
  vshuffle(dfe_output_imag_0, shuffle_index, dfe_input_0, SHUFFLE_GATHER, symbolLength);
//   vshuffle(dfe_output_imag_1, shuffle_index, dfe_input_1, SHUFFLE_GATHER, symbolLength_07);
//   vshuffle(dfe_output_imag_2, shuffle_index, dfe_input_2, SHUFFLE_GATHER, symbolLength);
//   vshuffle(dfe_output_imag_3, shuffle_index, dfe_input_3, SHUFFLE_GATHER, symbolLength);
//   vshuffle(dfe_output_imag_4, shuffle_index, dfe_input_4, SHUFFLE_GATHER, symbolLength);
//   vshuffle(dfe_output_imag_5, shuffle_index, dfe_input_5, SHUFFLE_GATHER, symbolLength_07);
//   vshuffle(dfe_output_imag_6, shuffle_index, dfe_input_6, SHUFFLE_GATHER, symbolLength);
//   vshuffle(dfe_output_imag_7, shuffle_index, dfe_input_7, SHUFFLE_GATHER, symbolLength);
//   vshuffle(dfe_output_imag_8, shuffle_index, dfe_input_8, SHUFFLE_GATHER, symbolLength);

//   vreturn(dfe_output_real_0, symbolLength, dfe_output_real_1, symbolLength_07, dfe_output_real_2, symbolLength,
//           dfe_output_real_3, symbolLength, dfe_output_real_4, symbolLength, dfe_output_real_5, symbolLength_07,
//           dfe_output_real_6, symbolLength, dfe_output_real_7, symbolLength, dfe_output_real_8, symbolLength,
//           dfe_output_imag_0, symbolLength, dfe_output_imag_1, symbolLength_07, dfe_output_imag_2, symbolLength,
//           dfe_output_imag_3, symbolLength, dfe_output_imag_4, symbolLength, dfe_output_imag_5, symbolLength_07,
//           dfe_output_imag_6, symbolLength, dfe_output_imag_7, symbolLength, dfe_output_imag_8, symbolLength);

  vreturn(dfe_output_real_0, symbolLength, dfe_output_imag_0, symbolLength);
}
