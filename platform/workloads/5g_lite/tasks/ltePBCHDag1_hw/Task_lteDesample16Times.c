/*
 * @author: shenyihao shenyihao@shu.edu.cn
 * @Date: 2024-11-28 16:43:43
 * @LastEditors: Yihao Shen shenyihao@shu.edu.cn
 * @LastEditTime: 2025-06-05 00:25:06
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

// int Task_lteDesample16Times(__v4096i8 dfe_output_real_0, __v4096i8 dfe_output_real_1, __v4096i8 dfe_output_real_2,
//                             __v4096i8 dfe_output_real_3, __v4096i8 dfe_output_real_4, __v4096i8 dfe_output_real_5,
//                             __v4096i8 dfe_output_real_6, __v4096i8 dfe_output_real_7, __v4096i8 dfe_output_real_8,
//                             __v4096i8 dfe_output_imag_0, __v4096i8 dfe_output_imag_1, __v4096i8 dfe_output_imag_2,
//                             __v4096i8 dfe_output_imag_3, __v4096i8 dfe_output_imag_4, __v4096i8 dfe_output_imag_5,
//                             __v4096i8 dfe_output_imag_6, __v4096i8 dfe_output_imag_7, __v4096i8 dfe_output_imag_8) {

//   __v4096i8 desampled_real_0;
//   __v4096i8 desampled_real_1;
//   __v4096i8 desampled_real_2;
//   __v4096i8 desampled_real_3;
//   __v4096i8 desampled_real_4;
//   __v4096i8 desampled_real_5;
//   __v4096i8 desampled_real_6;
//   __v4096i8 desampled_real_7;
//   __v4096i8 desampled_real_8;

//   __v4096i8 desampled_imag_0;
//   __v4096i8 desampled_imag_1;
//   __v4096i8 desampled_imag_2;
//   __v4096i8 desampled_imag_3;
//   __v4096i8 desampled_imag_4;
//   __v4096i8 desampled_imag_5;
//   __v4096i8 desampled_imag_6;
//   __v4096i8 desampled_imag_7;
//   __v4096i8 desampled_imag_8;

//   vclaim(desampled_real_0);
//   vclaim(desampled_real_1);
//   vclaim(desampled_real_2);
//   vclaim(desampled_real_3);
//   vclaim(desampled_real_4);
//   vclaim(desampled_real_5);
//   vclaim(desampled_real_6);
//   vclaim(desampled_real_7);
//   vclaim(desampled_real_8);

//   vclaim(desampled_imag_0);
//   vclaim(desampled_imag_1);
//   vclaim(desampled_imag_2);
//   vclaim(desampled_imag_3);
//   vclaim(desampled_imag_4);
//   vclaim(desampled_imag_5);
//   vclaim(desampled_imag_6);
//   vclaim(desampled_imag_7);
//   vclaim(desampled_imag_8);

//   __v2048i16 desample_index;
//   vclaim(desample_index);
//   vrange(desample_index, 138);
//   desample_index = vsll(desample_index, 4, MASKREAD_OFF, 138);

//   vshuffle(desampled_real_0, desample_index, dfe_output_real_0, SHUFFLE_GATHER, 137);
//   vshuffle(desampled_real_1, desample_index, dfe_output_real_1, SHUFFLE_GATHER, 138);
//   vshuffle(desampled_real_2, desample_index, dfe_output_real_2, SHUFFLE_GATHER, 137);
//   vshuffle(desampled_real_3, desample_index, dfe_output_real_3, SHUFFLE_GATHER, 137);
//   vshuffle(desampled_real_4, desample_index, dfe_output_real_4, SHUFFLE_GATHER, 137);
//   vshuffle(desampled_real_5, desample_index, dfe_output_real_5, SHUFFLE_GATHER, 138);
//   vshuffle(desampled_real_6, desample_index, dfe_output_real_6, SHUFFLE_GATHER, 137);
//   vshuffle(desampled_real_7, desample_index, dfe_output_real_7, SHUFFLE_GATHER, 137);
//   vshuffle(desampled_real_8, desample_index, dfe_output_real_8, SHUFFLE_GATHER, 137);

//   vshuffle(desampled_imag_0, desample_index, dfe_output_imag_0, SHUFFLE_GATHER, 137);
//   vshuffle(desampled_imag_1, desample_index, dfe_output_imag_1, SHUFFLE_GATHER, 138);
//   vshuffle(desampled_imag_2, desample_index, dfe_output_imag_2, SHUFFLE_GATHER, 137);
//   vshuffle(desampled_imag_3, desample_index, dfe_output_imag_3, SHUFFLE_GATHER, 137);
//   vshuffle(desampled_imag_4, desample_index, dfe_output_imag_4, SHUFFLE_GATHER, 137);
//   vshuffle(desampled_imag_5, desample_index, dfe_output_imag_5, SHUFFLE_GATHER, 138);
//   vshuffle(desampled_imag_6, desample_index, dfe_output_imag_6, SHUFFLE_GATHER, 137);
//   vshuffle(desampled_imag_7, desample_index, dfe_output_imag_7, SHUFFLE_GATHER, 137);
//   vshuffle(desampled_imag_8, desample_index, dfe_output_imag_8, SHUFFLE_GATHER, 137);

//   vreturn(desampled_real_0, 137, desampled_real_1, 138, desampled_real_2, 137, desampled_real_3, 137,
//   desampled_real_4,
//           137, desampled_real_5, 138, desampled_real_6, 137, desampled_real_7, 137, desampled_real_8, 137,
//           desampled_imag_0, 137, desampled_imag_1, 138, desampled_imag_2, 137, desampled_imag_3, 137,
//           desampled_imag_4, 137, desampled_imag_5, 138, desampled_imag_6, 137, desampled_imag_7, 137,
//           desampled_imag_8, 137);
// }

int Task_lteDesample16Times(__v4096i8 dfe_output_real_0, __v4096i8 dfe_output_imag_0) {

  __v4096i8 desampled_real_0;
  __v4096i8 desampled_imag_0;

  vclaim(desampled_real_0);
  vclaim(desampled_imag_0);

  __v2048i16 desample_index;
  vclaim(desample_index);
  vrange(desample_index, 138);
  desample_index = vsll(desample_index, 4, MASKREAD_OFF, 138);

  __v4096i8 A_r0;
  __v4096i8 A_i0;
  vclaim(A_r0);
  vclaim(A_i0);
  vshuffle(A_r0, desample_index, dfe_output_real_0, SHUFFLE_GATHER, 137);
  vshuffle(A_i0, desample_index, dfe_output_imag_0, SHUFFLE_GATHER, 137);
  
  __v4096i8 B_r0;
  __v4096i8 B_i0;
  vclaim(B_r0);
  vclaim(B_i0);
  desample_index = vsadd(desample_index, 1, MASKREAD_OFF, 138);
  vshuffle(B_r0, desample_index, dfe_output_real_0, SHUFFLE_GATHER, 137);
  vshuffle(B_i0, desample_index, dfe_output_imag_0, SHUFFLE_GATHER, 137);

  __v4096i8 C_r0;
  __v4096i8 C_i0;
  vclaim(C_r0);
  vclaim(C_i0);
  desample_index = vsadd(desample_index, 1, MASKREAD_OFF, 138);
  vshuffle(C_r0, desample_index, dfe_output_real_0, SHUFFLE_GATHER, 137);
  vshuffle(C_i0, desample_index, dfe_output_imag_0, SHUFFLE_GATHER, 137);

  __v4096i8 D_r0;
  __v4096i8 D_i0;
  vclaim(D_r0);
  vclaim(D_i0);
  desample_index = vsadd(desample_index, 1, MASKREAD_OFF, 138);
  vshuffle(D_r0, desample_index, dfe_output_real_0, SHUFFLE_GATHER, 137);
  vshuffle(D_i0, desample_index, dfe_output_imag_0, SHUFFLE_GATHER, 137);

  A_r0 = vsra(A_r0, 1, MASKREAD_OFF, 137);
  A_i0 = vsra(A_i0, 1, MASKREAD_OFF, 137);
  B_r0 = vsra(B_r0, 1, MASKREAD_OFF, 137);
  B_i0 = vsra(B_i0, 1, MASKREAD_OFF, 137);
  C_r0 = vsra(C_r0, 1, MASKREAD_OFF, 137);
  C_i0 = vsra(C_i0, 1, MASKREAD_OFF, 137);
  D_r0 = vsra(D_r0, 1, MASKREAD_OFF, 137);
  D_i0 = vsra(D_i0, 1, MASKREAD_OFF, 137);

  A_r0 = vsadd(A_r0, B_r0, MASKREAD_OFF, 137);
  A_i0 = vsadd(A_i0, B_i0, MASKREAD_OFF, 137);
  C_r0 = vsadd(C_r0, D_r0, MASKREAD_OFF, 137);
  C_i0 = vsadd(C_i0, D_i0, MASKREAD_OFF, 137);

  A_r0 = vsra(A_r0, 1, MASKREAD_OFF, 137);
  A_i0 = vsra(A_i0, 1, MASKREAD_OFF, 137);
  C_r0 = vsra(C_r0, 1, MASKREAD_OFF, 137);
  C_i0 = vsra(C_i0, 1, MASKREAD_OFF, 137);

  A_r0 = vsadd(A_r0, C_r0, MASKREAD_OFF, 137);
  A_i0 = vsadd(A_i0, C_i0, MASKREAD_OFF, 137);

  __v4096i8 E_r0;
  __v4096i8 E_i0;
  vclaim(E_r0);
  vclaim(E_i0);

  desample_index = vsadd(desample_index, 1, MASKREAD_OFF, 138);
  vshuffle(B_r0, desample_index, dfe_output_real_0, SHUFFLE_GATHER, 137);
  vshuffle(B_i0, desample_index, dfe_output_imag_0, SHUFFLE_GATHER, 137);
  desample_index = vsadd(desample_index, 1, MASKREAD_OFF, 138);
  vshuffle(C_r0, desample_index, dfe_output_real_0, SHUFFLE_GATHER, 137);
  vshuffle(C_i0, desample_index, dfe_output_imag_0, SHUFFLE_GATHER, 137);
  desample_index = vsadd(desample_index, 1, MASKREAD_OFF, 138);
  vshuffle(D_r0, desample_index, dfe_output_real_0, SHUFFLE_GATHER, 137);
  vshuffle(D_i0, desample_index, dfe_output_imag_0, SHUFFLE_GATHER, 137);
  desample_index = vsadd(desample_index, 1, MASKREAD_OFF, 138);
  vshuffle(E_r0, desample_index, dfe_output_real_0, SHUFFLE_GATHER, 137);
  vshuffle(E_i0, desample_index, dfe_output_imag_0, SHUFFLE_GATHER, 137);

  B_r0 = vsra(B_r0, 1, MASKREAD_OFF, 137);
  B_i0 = vsra(B_i0, 1, MASKREAD_OFF, 137);
  C_r0 = vsra(C_r0, 1, MASKREAD_OFF, 137);
  C_i0 = vsra(C_i0, 1, MASKREAD_OFF, 137);
  D_r0 = vsra(D_r0, 1, MASKREAD_OFF, 137);
  D_i0 = vsra(D_i0, 1, MASKREAD_OFF, 137);
  E_r0 = vsra(E_r0, 1, MASKREAD_OFF, 137);
  E_i0 = vsra(E_i0, 1, MASKREAD_OFF, 137);

  B_r0 = vsadd(B_r0, C_r0, MASKREAD_OFF, 137);
  B_i0 = vsadd(B_i0, C_i0, MASKREAD_OFF, 137);
  D_r0 = vsadd(D_r0, E_r0, MASKREAD_OFF, 137);
  D_i0 = vsadd(D_i0, E_i0, MASKREAD_OFF, 137);

  B_r0 = vsra(B_r0, 1, MASKREAD_OFF, 137);
  B_i0 = vsra(B_i0, 1, MASKREAD_OFF, 137);
  D_r0 = vsra(D_r0, 1, MASKREAD_OFF, 137);
  D_i0 = vsra(D_i0, 1, MASKREAD_OFF, 137);
  B_r0 = vsadd(B_r0, D_r0, MASKREAD_OFF, 137);
  B_i0 = vsadd(B_i0, D_i0, MASKREAD_OFF, 137);


  __v4096i8 F_r0;
  __v4096i8 F_i0;
  vclaim(F_r0);
  vclaim(F_i0);
  desample_index = vsadd(desample_index, 1, MASKREAD_OFF, 138);
  vshuffle(C_r0, desample_index, dfe_output_real_0, SHUFFLE_GATHER, 137);
  vshuffle(C_i0, desample_index, dfe_output_imag_0, SHUFFLE_GATHER, 137);
  desample_index = vsadd(desample_index, 1, MASKREAD_OFF, 138);
  vshuffle(D_r0, desample_index, dfe_output_real_0, SHUFFLE_GATHER, 137);
  vshuffle(D_i0, desample_index, dfe_output_imag_0, SHUFFLE_GATHER, 137);
  desample_index = vsadd(desample_index, 1, MASKREAD_OFF, 138);
  vshuffle(E_r0, desample_index, dfe_output_real_0, SHUFFLE_GATHER, 137);
  vshuffle(E_i0, desample_index, dfe_output_imag_0, SHUFFLE_GATHER, 137);
  desample_index = vsadd(desample_index, 1, MASKREAD_OFF, 138);
  vshuffle(F_r0, desample_index, dfe_output_real_0, SHUFFLE_GATHER, 137);
  vshuffle(F_i0, desample_index, dfe_output_imag_0, SHUFFLE_GATHER, 137);

  C_r0 = vsra(C_r0, 1, MASKREAD_OFF, 137);
  C_i0 = vsra(C_i0, 1, MASKREAD_OFF, 137);
  D_r0 = vsra(D_r0, 1, MASKREAD_OFF, 137);
  D_i0 = vsra(D_i0, 1, MASKREAD_OFF, 137);
  E_r0 = vsra(E_r0, 1, MASKREAD_OFF, 137);
  E_i0 = vsra(E_i0, 1, MASKREAD_OFF, 137);
  F_r0 = vsra(F_r0, 1, MASKREAD_OFF, 137);
  F_i0 = vsra(F_i0, 1, MASKREAD_OFF, 137);

  C_r0 = vsadd(C_r0, D_r0, MASKREAD_OFF, 137);
  C_i0 = vsadd(C_i0, D_i0, MASKREAD_OFF, 137);
  E_r0 = vsadd(E_r0, F_r0, MASKREAD_OFF, 137);
  E_i0 = vsadd(E_i0, F_i0, MASKREAD_OFF, 137);
  C_r0 = vsra(C_r0, 1, MASKREAD_OFF, 137);
  C_i0 = vsra(C_i0, 1, MASKREAD_OFF, 137);
  E_r0 = vsra(E_r0, 1, MASKREAD_OFF, 137);
  E_i0 = vsra(E_i0, 1, MASKREAD_OFF, 137);
  C_r0 = vsadd(C_r0, E_r0, MASKREAD_OFF, 137);
  C_i0 = vsadd(C_i0, E_i0, MASKREAD_OFF, 137);

  desample_index = vsadd(desample_index, 1, MASKREAD_OFF, 138);
  vshuffle(D_r0, desample_index, dfe_output_real_0, SHUFFLE_GATHER, 137);
  vshuffle(D_i0, desample_index, dfe_output_imag_0, SHUFFLE_GATHER, 137);
  desample_index = vsadd(desample_index, 1, MASKREAD_OFF, 138);
  vshuffle(E_r0, desample_index, dfe_output_real_0, SHUFFLE_GATHER, 137);
  vshuffle(E_i0, desample_index, dfe_output_imag_0, SHUFFLE_GATHER, 137);
  desample_index = vsadd(desample_index, 1, MASKREAD_OFF, 138);
  vshuffle(F_r0, desample_index, dfe_output_real_0, SHUFFLE_GATHER, 137);
  vshuffle(F_i0, desample_index, dfe_output_imag_0, SHUFFLE_GATHER, 137);
  D_r0 = vsra(D_r0, 1, MASKREAD_OFF, 137);
  D_i0 = vsra(D_i0, 1, MASKREAD_OFF, 137);
  E_r0 = vsra(E_r0, 1, MASKREAD_OFF, 137);
  E_i0 = vsra(E_i0, 1, MASKREAD_OFF, 137);
  F_r0 = vsra(F_r0, 1, MASKREAD_OFF, 137);
  F_i0 = vsra(F_i0, 1, MASKREAD_OFF, 137);
  D_r0 = vsadd(D_r0, E_r0, MASKREAD_OFF, 137);
  D_i0 = vsadd(D_i0, E_i0, MASKREAD_OFF, 137);

  desample_index = vsra(desample_index, 1, MASKREAD_OFF, 138);
  vshuffle(E_r0, desample_index, dfe_output_real_0, SHUFFLE_GATHER, 137);
  vshuffle(E_i0, desample_index, dfe_output_imag_0, SHUFFLE_GATHER, 137);
  E_r0 = vsra(E_r0, 1, MASKREAD_OFF, 137);
  E_i0 = vsra(E_i0, 1, MASKREAD_OFF, 137);
  E_r0 = vsadd(E_r0, F_r0, MASKREAD_OFF, 137);
  E_i0 = vsadd(E_i0, F_i0, MASKREAD_OFF, 137);

  D_r0 = vsra(D_r0, 1, MASKREAD_OFF, 137);
  D_i0 = vsra(D_i0, 1, MASKREAD_OFF, 137);
  E_r0 = vsra(E_r0, 1, MASKREAD_OFF, 137);
  E_i0 = vsra(E_i0, 1, MASKREAD_OFF, 137);
  D_r0 = vsadd(D_r0, E_r0, MASKREAD_OFF, 137);
  D_i0 = vsadd(D_i0, E_i0, MASKREAD_OFF, 137);

  A_r0 = vsra(A_r0, 1, MASKREAD_OFF, 137);
  A_i0 = vsra(A_i0, 1, MASKREAD_OFF, 137);
  B_r0 = vsra(B_r0, 1, MASKREAD_OFF, 137);
  B_i0 = vsra(B_i0, 1, MASKREAD_OFF, 137);
  C_r0 = vsra(C_r0, 1, MASKREAD_OFF, 137);
  C_i0 = vsra(C_i0, 1, MASKREAD_OFF, 137);
  D_r0 = vsra(D_r0, 1, MASKREAD_OFF, 137);
  D_i0 = vsra(D_i0, 1, MASKREAD_OFF, 137);

  A_r0 = vsadd(A_r0, B_r0, MASKREAD_OFF, 137);
  A_i0 = vsadd(A_i0, B_i0, MASKREAD_OFF, 137);
  C_r0 = vsadd(C_r0, D_r0, MASKREAD_OFF, 137);
  C_i0 = vsadd(C_i0, D_i0, MASKREAD_OFF, 137);
  A_r0 = vsra(A_r0, 1, MASKREAD_OFF, 137);
  A_i0 = vsra(A_i0, 1, MASKREAD_OFF, 137);
  C_r0 = vsra(C_r0, 1, MASKREAD_OFF, 137);
  C_i0 = vsra(C_i0, 1, MASKREAD_OFF, 137);
  desampled_real_0 = vsadd(A_r0, C_r0, MASKREAD_OFF, 137);
  desampled_imag_0 = vsadd(A_i0, C_i0, MASKREAD_OFF, 137);






  // vshuffle(desampled_real_0, desample_index, dfe_output_real_0, SHUFFLE_GATHER, 137);
  // vshuffle(desampled_imag_0, desample_index, dfe_output_imag_0, SHUFFLE_GATHER, 137);



  vreturn(desampled_real_0, 137, desampled_imag_0, 137);
}
