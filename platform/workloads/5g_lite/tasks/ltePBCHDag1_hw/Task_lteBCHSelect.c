/*
 * @Author: shenyihao shenyihao@shu.edu.cn
 * @Date: 2024-12-04 13:30:56
 * @LastEditors: Yihao Shen shenyihao@shu.edu.cn
 * @LastEditTime: 2025-05-17 23:51:13
 * @FilePath: /5G-Lite-0530/LTE_ECHO/5g_lite/Tasks/ltePBCH/Task_lteBCHselect.c
 * @Description:
 */

#include "riscv_printf.h"
#include "venus.h"

typedef struct {
  short data;
} __attribute__((aligned(64))) short_struct;

typedef char __v8192i8 __attribute__((ext_vector_type(8192)));
typedef short __v4096i16 __attribute__((ext_vector_type(4096)));
typedef char __v4096i8 __attribute__((ext_vector_type(4096)));
typedef short __v2048i16 __attribute__((ext_vector_type(2048)));

int symbolLength = 2192;
int symbolLength_07 = 2208;

// 取10ms后的sss和pbch
int Task_lteBCHSelect(__v8192i8 dfe_input_1, __v8192i8 dfe_input_2, __v8192i8 dfe_input_3, __v8192i8 dfe_input_4, __v8192i8 dfe_input_5, __v8192i8 dfe_input_6, __v8192i8 dfe_input_7, __v8192i8 dfe_input_8, __v8192i8 dfe_input_9, __v8192i8 dfe_input_10, short_struct subFrameNum) 
{
  short subframe_num = subFrameNum.data;
  __v8192i8 dfe_pbch_real0;
  __v8192i8 dfe_pbch_real1;
  __v8192i8 dfe_pbch_real2;
  __v8192i8 dfe_pbch_real3;
  __v8192i8 dfe_pbch_real4;

  __v8192i8 dfe_pbch_imag0;
  __v8192i8 dfe_pbch_imag1;
  __v8192i8 dfe_pbch_imag2;
  __v8192i8 dfe_pbch_imag3;
  __v8192i8 dfe_pbch_imag4;

  vclaim(dfe_pbch_real0);
  vclaim(dfe_pbch_real1);
  vclaim(dfe_pbch_real2);
  vclaim(dfe_pbch_real3);
  vclaim(dfe_pbch_real4);

  vclaim(dfe_pbch_imag0);
  vclaim(dfe_pbch_imag1);
  vclaim(dfe_pbch_imag2);
  vclaim(dfe_pbch_imag3);
  vclaim(dfe_pbch_imag4);


  __v4096i16 shuffle_index;
  vclaim(shuffle_index);
  vrange(shuffle_index, symbolLength_07);
  shuffle_index = vmul(shuffle_index, 2, MASKREAD_OFF, symbolLength_07);
  shuffle_index = vadd(shuffle_index, 4, MASKREAD_OFF, symbolLength_07);


  if (subframe_num == 5) {
    vshuffle(dfe_pbch_real0, shuffle_index, dfe_input_1, SHUFFLE_GATHER, symbolLength_07);
    vshuffle(dfe_pbch_real1, shuffle_index, dfe_input_2, SHUFFLE_GATHER, symbolLength);
    vshuffle(dfe_pbch_real2, shuffle_index, dfe_input_3, SHUFFLE_GATHER, symbolLength);
    vshuffle(dfe_pbch_real3, shuffle_index, dfe_input_4, SHUFFLE_GATHER, symbolLength);
    vshuffle(dfe_pbch_real4, shuffle_index, dfe_input_5, SHUFFLE_GATHER, symbolLength);
    shuffle_index = vsadd(shuffle_index, 1, MASKREAD_OFF, symbolLength);
    vshuffle(dfe_pbch_imag0, shuffle_index, dfe_input_1, SHUFFLE_GATHER, symbolLength_07);
    vshuffle(dfe_pbch_imag1, shuffle_index, dfe_input_2, SHUFFLE_GATHER, symbolLength);
    vshuffle(dfe_pbch_imag2, shuffle_index, dfe_input_3, SHUFFLE_GATHER, symbolLength);
    vshuffle(dfe_pbch_imag3, shuffle_index, dfe_input_4, SHUFFLE_GATHER, symbolLength);
    vshuffle(dfe_pbch_imag4, shuffle_index, dfe_input_5, SHUFFLE_GATHER, symbolLength);
  } else {
    vshuffle(dfe_pbch_real0, shuffle_index, dfe_input_6, SHUFFLE_GATHER, symbolLength_07);
    vshuffle(dfe_pbch_real1, shuffle_index, dfe_input_7, SHUFFLE_GATHER, symbolLength);
    vshuffle(dfe_pbch_real2, shuffle_index, dfe_input_8, SHUFFLE_GATHER, symbolLength);
    vshuffle(dfe_pbch_real3, shuffle_index, dfe_input_9, SHUFFLE_GATHER, symbolLength);
    vshuffle(dfe_pbch_real4, shuffle_index, dfe_input_10, SHUFFLE_GATHER, symbolLength);
    shuffle_index = vsadd(shuffle_index, 1, MASKREAD_OFF, symbolLength);
    vshuffle(dfe_pbch_imag0, shuffle_index, dfe_input_6, SHUFFLE_GATHER, symbolLength_07);
    vshuffle(dfe_pbch_imag1, shuffle_index, dfe_input_7, SHUFFLE_GATHER, symbolLength);
    vshuffle(dfe_pbch_imag2, shuffle_index, dfe_input_8, SHUFFLE_GATHER, symbolLength);
    vshuffle(dfe_pbch_imag3, shuffle_index, dfe_input_9, SHUFFLE_GATHER, symbolLength);
    vshuffle(dfe_pbch_imag4, shuffle_index, dfe_input_10, SHUFFLE_GATHER, symbolLength);
  }


  // __v8192i8 desampled_real_0;
  // __v8192i8 desampled_real_1;
  // __v8192i8 desampled_real_2;
  // __v8192i8 desampled_real_3;
  // __v8192i8 desampled_real_4;


  // __v8192i8 desampled_imag_0;
  // __v8192i8 desampled_imag_1;
  // __v8192i8 desampled_imag_2;
  // __v8192i8 desampled_imag_3;
  // __v8192i8 desampled_imag_4;


  // vclaim(desampled_real_0);
  // vclaim(desampled_real_1);
  // vclaim(desampled_real_2);
  // vclaim(desampled_real_3);
  // vclaim(desampled_real_4);


  // vclaim(desampled_imag_0);
  // vclaim(desampled_imag_1);
  // vclaim(desampled_imag_2);
  // vclaim(desampled_imag_3);
  // vclaim(desampled_imag_4);


  // __v4096i16 desample_index;
  // vclaim(desample_index);
  // vrange(desample_index, 138);
  // desample_index = vsll(desample_index, 4, MASKREAD_OFF, 138);
  // // desample_index = vsadd(desample_index, 8, MASKREAD_OFF, 138);

  // vshuffle(desampled_real_0, desample_index, dfe_pbch_real0, SHUFFLE_GATHER, 138);
  // vshuffle(desampled_real_1, desample_index, dfe_pbch_real1, SHUFFLE_GATHER, 137);
  // vshuffle(desampled_real_2, desample_index, dfe_pbch_real2, SHUFFLE_GATHER, 137);
  // vshuffle(desampled_real_3, desample_index, dfe_pbch_real3, SHUFFLE_GATHER, 137);
  // vshuffle(desampled_real_4, desample_index, dfe_pbch_real4, SHUFFLE_GATHER, 137);

  // vshuffle(desampled_imag_0, desample_index, dfe_pbch_imag0, SHUFFLE_GATHER, 138);
  // vshuffle(desampled_imag_1, desample_index, dfe_pbch_imag1, SHUFFLE_GATHER, 137);
  // vshuffle(desampled_imag_2, desample_index, dfe_pbch_imag2, SHUFFLE_GATHER, 137);
  // vshuffle(desampled_imag_3, desample_index, dfe_pbch_imag3, SHUFFLE_GATHER, 137);
  // vshuffle(desampled_imag_4, desample_index, dfe_pbch_imag4, SHUFFLE_GATHER, 137);


  vreturn(dfe_pbch_real0, symbolLength_07, dfe_pbch_real1, symbolLength, dfe_pbch_real2, symbolLength, dfe_pbch_real3, symbolLength, dfe_pbch_real4, symbolLength, dfe_pbch_imag0, symbolLength_07, dfe_pbch_imag1, symbolLength, dfe_pbch_imag2, symbolLength, dfe_pbch_imag3, symbolLength, dfe_pbch_imag4, symbolLength);
}
