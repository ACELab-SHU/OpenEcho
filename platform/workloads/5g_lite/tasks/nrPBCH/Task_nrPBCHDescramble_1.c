/**
 * ****************************************
 * @file        Task nrPBCHDescramble.c
 * @brief       PBCH Descramble
 * @author      yuanfeng
 * @date        2024.5.11
 * @copyright   ACE-Lab(Shanghai University)
 * ****************************************
 */

#include "riscv_printf.h"
#include "venus.h"

typedef short __v2048i16 __attribute__((ext_vector_type(2048)));
typedef char  __v2048i8 __attribute__((ext_vector_type(2048)));

int tabLength      = 31;
int sequenceLength = 864;
int seqSetLength   = 128;

typedef struct {
  short data;
} __attribute__((aligned(64))) short_struct;

int Task_nrPBCHDescramble_1(__v2048i8 seq1_vec, __v2048i16 seq2_init_table_0_vec, __v2048i16 seq2_init_table_1_vec,
                            __v2048i16 seq2_init_table_2_vec, __v2048i16 seq2_init_table_3_vec,
                            __v2048i16 seq2_init_table_4_vec, __v2048i16 seq2_init_table_5_vec,
                            __v2048i16 seq2_init_table_6_vec, __v2048i16 seq2_init_table_7_vec,
                            __v2048i16 seq2_init_table_8_vec, __v2048i16 seq2_init_table_9_vec,
                            __v2048i16 seq2_init_table_10_vec, __v2048i16 seq2_init_table_11_vec,
                            __v2048i16 seq2_init_table_12_vec, __v2048i16 seq2_init_table_13_vec,
                            __v2048i16 seq2_init_table_14_vec, __v2048i16 seq2_init_table_15_vec,
                            __v2048i16 seq2_init_table_16_vec, __v2048i16 seq2_init_table_17_vec,
                            __v2048i16 seq2_init_table_18_vec, __v2048i16 seq2_init_table_19_vec,
                            __v2048i16 seq2_init_table_20_vec, __v2048i16 seq2_init_table_21_vec,
                            __v2048i16 seq2_init_table_22_vec, __v2048i16 seq2_init_table_23_vec,
                            short_struct input_L_max, short_struct input_iBar_SSB, short_struct input_ncellid) {
  int v        = 0;
  int L_max    = input_L_max.data;
  int iBar_SSB = input_iBar_SSB.data;
  int ncellid  = input_ncellid.data;
  if (L_max == 4) {
    v = iBar_SSB % 4;
  } else {
    v = iBar_SSB;
  }

  __v2048i8 init2_vec;
  vclaim(init2_vec);
  vbrdcst(init2_vec, 0, MASKREAD_OFF, 32);
  vbrdcst(init2_vec, 0, MASKREAD_OFF, 32);
  vbrdcst(init2_vec, 0, MASKREAD_OFF, 32);
  vbarrier();
  VSPM_OPEN();
  int init2_vec_addr = vaddr(init2_vec);
  for (int i = 0; i < 31; ++i) {
    // *(volatile short *)(init2_vec_addr + (i << 1)) = ncellid & 0b1;
    *(volatile char *)(init2_vec_addr + i) = ncellid & 0b1;
    ncellid                                = ncellid >> 1;
  }
  VSPM_CLOSE();

  __v2048i8 seq2_init_table_0_tmp;
  vclaim(seq2_init_table_0_tmp);
  vshuffle(seq2_init_table_0_tmp, seq2_init_table_0_vec, init2_vec, SHUFFLE_GATHER, tabLength);
  __v2048i8 seq2_init_table_1_tmp;
  vclaim(seq2_init_table_1_tmp);
  vshuffle(seq2_init_table_1_tmp, seq2_init_table_1_vec, init2_vec, SHUFFLE_GATHER, tabLength);
  __v2048i8 seq2_init_table_2_tmp;
  vclaim(seq2_init_table_2_tmp);
  vshuffle(seq2_init_table_2_tmp, seq2_init_table_2_vec, init2_vec, SHUFFLE_GATHER, tabLength);
  __v2048i8 seq2_init_table_3_tmp;
  vclaim(seq2_init_table_3_tmp);
  vshuffle(seq2_init_table_3_tmp, seq2_init_table_3_vec, init2_vec, SHUFFLE_GATHER, tabLength);
  __v2048i8 seq2_init_table_4_tmp;
  vclaim(seq2_init_table_4_tmp);
  vshuffle(seq2_init_table_4_tmp, seq2_init_table_4_vec, init2_vec, SHUFFLE_GATHER, tabLength);
  __v2048i8 seq2_init_table_5_tmp;
  vclaim(seq2_init_table_5_tmp);
  vshuffle(seq2_init_table_5_tmp, seq2_init_table_5_vec, init2_vec, SHUFFLE_GATHER, tabLength);
  __v2048i8 seq2_init_table_6_tmp;
  vclaim(seq2_init_table_6_tmp);
  vshuffle(seq2_init_table_6_tmp, seq2_init_table_6_vec, init2_vec, SHUFFLE_GATHER, tabLength);
  __v2048i8 seq2_init_table_7_tmp;
  vclaim(seq2_init_table_7_tmp);
  vshuffle(seq2_init_table_7_tmp, seq2_init_table_7_vec, init2_vec, SHUFFLE_GATHER, tabLength);
  __v2048i8 seq2_init_table_8_tmp;
  vclaim(seq2_init_table_8_tmp);
  vshuffle(seq2_init_table_8_tmp, seq2_init_table_8_vec, init2_vec, SHUFFLE_GATHER, tabLength);
  __v2048i8 seq2_init_table_9_tmp;
  vclaim(seq2_init_table_9_tmp);
  vshuffle(seq2_init_table_9_tmp, seq2_init_table_9_vec, init2_vec, SHUFFLE_GATHER, tabLength);
  __v2048i8 seq2_init_table_10_tmp;
  vclaim(seq2_init_table_10_tmp);
  vshuffle(seq2_init_table_10_tmp, seq2_init_table_10_vec, init2_vec, SHUFFLE_GATHER, tabLength);
  __v2048i8 seq2_init_table_11_tmp;
  vclaim(seq2_init_table_11_tmp);
  vshuffle(seq2_init_table_11_tmp, seq2_init_table_11_vec, init2_vec, SHUFFLE_GATHER, tabLength);
  __v2048i8 seq2_init_table_12_tmp;
  vclaim(seq2_init_table_12_tmp);
  vshuffle(seq2_init_table_12_tmp, seq2_init_table_12_vec, init2_vec, SHUFFLE_GATHER, tabLength);
  __v2048i8 seq2_init_table_13_tmp;
  vclaim(seq2_init_table_13_tmp);
  vshuffle(seq2_init_table_13_tmp, seq2_init_table_13_vec, init2_vec, SHUFFLE_GATHER, tabLength);
  __v2048i8 seq2_init_table_14_tmp;
  vclaim(seq2_init_table_14_tmp);
  vshuffle(seq2_init_table_14_tmp, seq2_init_table_14_vec, init2_vec, SHUFFLE_GATHER, tabLength);
  __v2048i8 seq2_init_table_15_tmp;
  vclaim(seq2_init_table_15_tmp);
  vshuffle(seq2_init_table_15_tmp, seq2_init_table_15_vec, init2_vec, SHUFFLE_GATHER, tabLength);
  __v2048i8 seq2_init_table_16_tmp;
  vclaim(seq2_init_table_16_tmp);
  vshuffle(seq2_init_table_16_tmp, seq2_init_table_16_vec, init2_vec, SHUFFLE_GATHER, tabLength);
  __v2048i8 seq2_init_table_17_tmp;
  vclaim(seq2_init_table_17_tmp);
  vshuffle(seq2_init_table_17_tmp, seq2_init_table_17_vec, init2_vec, SHUFFLE_GATHER, tabLength);
  __v2048i8 seq2_init_table_18_tmp;
  vclaim(seq2_init_table_18_tmp);
  vshuffle(seq2_init_table_18_tmp, seq2_init_table_18_vec, init2_vec, SHUFFLE_GATHER, tabLength);
  __v2048i8 seq2_init_table_19_tmp;
  vclaim(seq2_init_table_19_tmp);
  vshuffle(seq2_init_table_19_tmp, seq2_init_table_19_vec, init2_vec, SHUFFLE_GATHER, tabLength);
  __v2048i8 seq2_init_table_20_tmp;
  vclaim(seq2_init_table_20_tmp);
  vshuffle(seq2_init_table_20_tmp, seq2_init_table_20_vec, init2_vec, SHUFFLE_GATHER, tabLength);
  __v2048i8 seq2_init_table_21_tmp;
  vclaim(seq2_init_table_21_tmp);
  vshuffle(seq2_init_table_21_tmp, seq2_init_table_21_vec, init2_vec, SHUFFLE_GATHER, tabLength);
  __v2048i8 seq2_init_table_22_tmp;
  vclaim(seq2_init_table_22_tmp);
  vshuffle(seq2_init_table_22_tmp, seq2_init_table_22_vec, init2_vec, SHUFFLE_GATHER, tabLength);
  __v2048i8 seq2_init_table_23_tmp;
  vclaim(seq2_init_table_23_tmp);
  vshuffle(seq2_init_table_23_tmp, seq2_init_table_23_vec, init2_vec, SHUFFLE_GATHER, tabLength);

  __v2048i16 seq1_shuffle_index;
  vclaim(seq1_shuffle_index);
  vrange(seq1_shuffle_index, sequenceLength);
  seq1_shuffle_index = vsadd(seq1_shuffle_index, v * sequenceLength, MASKREAD_OFF, sequenceLength);
  __v2048i8 seq1_vec1;
  vclaim(seq1_vec1);
  vshuffle(seq1_vec1, seq1_shuffle_index, seq1_vec, SHUFFLE_GATHER, sequenceLength);

  __v2048i16 seq2_init_table_shuffle_index;
  vclaim(seq2_init_table_shuffle_index);
  vrange(seq2_init_table_shuffle_index, tabLength);
  seq2_init_table_shuffle_index = vsadd(seq2_init_table_shuffle_index, v * tabLength, MASKREAD_OFF, tabLength);
  __v2048i16 seq2_init_table_0_vec1;
  __v2048i16 seq2_init_table_1_vec1;
  __v2048i16 seq2_init_table_2_vec1;
  __v2048i16 seq2_init_table_3_vec1;
  __v2048i16 seq2_init_table_4_vec1;
  __v2048i16 seq2_init_table_5_vec1;
  __v2048i16 seq2_init_table_6_vec1;
  __v2048i16 seq2_init_table_7_vec1;
  __v2048i16 seq2_init_table_8_vec1;
  __v2048i16 seq2_init_table_9_vec1;
  __v2048i16 seq2_init_table_10_vec1;
  __v2048i16 seq2_init_table_11_vec1;
  __v2048i16 seq2_init_table_12_vec1;
  __v2048i16 seq2_init_table_13_vec1;
  __v2048i16 seq2_init_table_14_vec1;
  __v2048i16 seq2_init_table_15_vec1;
  __v2048i16 seq2_init_table_16_vec1;
  __v2048i16 seq2_init_table_17_vec1;
  __v2048i16 seq2_init_table_18_vec1;
  __v2048i16 seq2_init_table_19_vec1;
  __v2048i16 seq2_init_table_20_vec1;
  __v2048i16 seq2_init_table_21_vec1;
  __v2048i16 seq2_init_table_22_vec1;
  __v2048i16 seq2_init_table_23_vec1;
  vclaim(seq2_init_table_0_vec1);
  vclaim(seq2_init_table_1_vec1);
  vclaim(seq2_init_table_2_vec1);
  vclaim(seq2_init_table_3_vec1);
  vclaim(seq2_init_table_4_vec1);
  vclaim(seq2_init_table_5_vec1);
  vclaim(seq2_init_table_6_vec1);
  vclaim(seq2_init_table_7_vec1);
  vclaim(seq2_init_table_8_vec1);
  vclaim(seq2_init_table_9_vec1);
  vclaim(seq2_init_table_10_vec1);
  vclaim(seq2_init_table_11_vec1);
  vclaim(seq2_init_table_12_vec1);
  vclaim(seq2_init_table_13_vec1);
  vclaim(seq2_init_table_14_vec1);
  vclaim(seq2_init_table_15_vec1);
  vclaim(seq2_init_table_16_vec1);
  vclaim(seq2_init_table_17_vec1);
  vclaim(seq2_init_table_18_vec1);
  vclaim(seq2_init_table_19_vec1);
  vclaim(seq2_init_table_20_vec1);
  vclaim(seq2_init_table_21_vec1);
  vclaim(seq2_init_table_22_vec1);
  vclaim(seq2_init_table_23_vec1);
  vshuffle(seq2_init_table_0_vec1, seq2_init_table_shuffle_index, seq2_init_table_0_vec, SHUFFLE_GATHER, tabLength);
  vshuffle(seq2_init_table_1_vec1, seq2_init_table_shuffle_index, seq2_init_table_1_vec, SHUFFLE_GATHER, tabLength);
  vshuffle(seq2_init_table_2_vec1, seq2_init_table_shuffle_index, seq2_init_table_2_vec, SHUFFLE_GATHER, tabLength);
  vshuffle(seq2_init_table_3_vec1, seq2_init_table_shuffle_index, seq2_init_table_3_vec, SHUFFLE_GATHER, tabLength);
  vshuffle(seq2_init_table_4_vec1, seq2_init_table_shuffle_index, seq2_init_table_4_vec, SHUFFLE_GATHER, tabLength);
  vshuffle(seq2_init_table_5_vec1, seq2_init_table_shuffle_index, seq2_init_table_5_vec, SHUFFLE_GATHER, tabLength);
  vshuffle(seq2_init_table_6_vec1, seq2_init_table_shuffle_index, seq2_init_table_6_vec, SHUFFLE_GATHER, tabLength);
  vshuffle(seq2_init_table_7_vec1, seq2_init_table_shuffle_index, seq2_init_table_7_vec, SHUFFLE_GATHER, tabLength);
  vshuffle(seq2_init_table_8_vec1, seq2_init_table_shuffle_index, seq2_init_table_8_vec, SHUFFLE_GATHER, tabLength);
  vshuffle(seq2_init_table_9_vec1, seq2_init_table_shuffle_index, seq2_init_table_9_vec, SHUFFLE_GATHER, tabLength);
  vshuffle(seq2_init_table_10_vec1, seq2_init_table_shuffle_index, seq2_init_table_10_vec, SHUFFLE_GATHER, tabLength);
  vshuffle(seq2_init_table_11_vec1, seq2_init_table_shuffle_index, seq2_init_table_11_vec, SHUFFLE_GATHER, tabLength);
  vshuffle(seq2_init_table_12_vec1, seq2_init_table_shuffle_index, seq2_init_table_12_vec, SHUFFLE_GATHER, tabLength);
  vshuffle(seq2_init_table_13_vec1, seq2_init_table_shuffle_index, seq2_init_table_13_vec, SHUFFLE_GATHER, tabLength);
  vshuffle(seq2_init_table_14_vec1, seq2_init_table_shuffle_index, seq2_init_table_14_vec, SHUFFLE_GATHER, tabLength);
  vshuffle(seq2_init_table_15_vec1, seq2_init_table_shuffle_index, seq2_init_table_15_vec, SHUFFLE_GATHER, tabLength);
  vshuffle(seq2_init_table_16_vec1, seq2_init_table_shuffle_index, seq2_init_table_16_vec, SHUFFLE_GATHER, tabLength);
  vshuffle(seq2_init_table_17_vec1, seq2_init_table_shuffle_index, seq2_init_table_17_vec, SHUFFLE_GATHER, tabLength);
  vshuffle(seq2_init_table_18_vec1, seq2_init_table_shuffle_index, seq2_init_table_18_vec, SHUFFLE_GATHER, tabLength);
  vshuffle(seq2_init_table_19_vec1, seq2_init_table_shuffle_index, seq2_init_table_19_vec, SHUFFLE_GATHER, tabLength);
  vshuffle(seq2_init_table_20_vec1, seq2_init_table_shuffle_index, seq2_init_table_20_vec, SHUFFLE_GATHER, tabLength);
  vshuffle(seq2_init_table_21_vec1, seq2_init_table_shuffle_index, seq2_init_table_21_vec, SHUFFLE_GATHER, tabLength);
  vshuffle(seq2_init_table_22_vec1, seq2_init_table_shuffle_index, seq2_init_table_22_vec, SHUFFLE_GATHER, tabLength);
  vshuffle(seq2_init_table_23_vec1, seq2_init_table_shuffle_index, seq2_init_table_23_vec, SHUFFLE_GATHER, tabLength);

  /*--------------------Descramble--------------------*/
  vshuffle(seq2_init_table_0_tmp, seq2_init_table_0_vec1, init2_vec, SHUFFLE_GATHER, tabLength);
  vshuffle(seq2_init_table_1_tmp, seq2_init_table_1_vec1, init2_vec, SHUFFLE_GATHER, tabLength);
  vshuffle(seq2_init_table_2_tmp, seq2_init_table_2_vec1, init2_vec, SHUFFLE_GATHER, tabLength);
  vshuffle(seq2_init_table_3_tmp, seq2_init_table_3_vec1, init2_vec, SHUFFLE_GATHER, tabLength);
  vshuffle(seq2_init_table_4_tmp, seq2_init_table_4_vec1, init2_vec, SHUFFLE_GATHER, tabLength);
  vshuffle(seq2_init_table_5_tmp, seq2_init_table_5_vec1, init2_vec, SHUFFLE_GATHER, tabLength);
  vshuffle(seq2_init_table_6_tmp, seq2_init_table_6_vec1, init2_vec, SHUFFLE_GATHER, tabLength);
  vshuffle(seq2_init_table_7_tmp, seq2_init_table_7_vec1, init2_vec, SHUFFLE_GATHER, tabLength);
  vshuffle(seq2_init_table_8_tmp, seq2_init_table_8_vec1, init2_vec, SHUFFLE_GATHER, tabLength);
  vshuffle(seq2_init_table_9_tmp, seq2_init_table_9_vec1, init2_vec, SHUFFLE_GATHER, tabLength);
  vshuffle(seq2_init_table_10_tmp, seq2_init_table_10_vec1, init2_vec, SHUFFLE_GATHER, tabLength);
  vshuffle(seq2_init_table_11_tmp, seq2_init_table_11_vec1, init2_vec, SHUFFLE_GATHER, tabLength);
  vshuffle(seq2_init_table_12_tmp, seq2_init_table_12_vec1, init2_vec, SHUFFLE_GATHER, tabLength);
  vshuffle(seq2_init_table_13_tmp, seq2_init_table_13_vec1, init2_vec, SHUFFLE_GATHER, tabLength);
  vshuffle(seq2_init_table_14_tmp, seq2_init_table_14_vec1, init2_vec, SHUFFLE_GATHER, tabLength);
  vshuffle(seq2_init_table_15_tmp, seq2_init_table_15_vec1, init2_vec, SHUFFLE_GATHER, tabLength);
  vshuffle(seq2_init_table_16_tmp, seq2_init_table_16_vec1, init2_vec, SHUFFLE_GATHER, tabLength);
  vshuffle(seq2_init_table_17_tmp, seq2_init_table_17_vec1, init2_vec, SHUFFLE_GATHER, tabLength);
  vshuffle(seq2_init_table_18_tmp, seq2_init_table_18_vec1, init2_vec, SHUFFLE_GATHER, tabLength);
  vshuffle(seq2_init_table_19_tmp, seq2_init_table_19_vec1, init2_vec, SHUFFLE_GATHER, tabLength);
  vshuffle(seq2_init_table_20_tmp, seq2_init_table_20_vec1, init2_vec, SHUFFLE_GATHER, tabLength);
  vshuffle(seq2_init_table_21_tmp, seq2_init_table_21_vec1, init2_vec, SHUFFLE_GATHER, tabLength);
  vshuffle(seq2_init_table_22_tmp, seq2_init_table_22_vec1, init2_vec, SHUFFLE_GATHER, tabLength);
  vshuffle(seq2_init_table_23_tmp, seq2_init_table_23_vec1, init2_vec, SHUFFLE_GATHER, tabLength);

  __v2048i8 seq2_tmp;
  vclaim(seq2_tmp);
  vbrdcst(seq2_tmp, 0, MASKREAD_OFF, tabLength + 1);
  seq2_tmp = vxor(seq2_init_table_0_tmp, seq2_init_table_1_tmp, MASKREAD_OFF, tabLength);
  seq2_tmp = vxor(seq2_tmp, seq2_init_table_2_tmp, MASKREAD_OFF, tabLength);
  seq2_tmp = vxor(seq2_tmp, seq2_init_table_3_tmp, MASKREAD_OFF, tabLength);
  seq2_tmp = vxor(seq2_tmp, seq2_init_table_4_tmp, MASKREAD_OFF, tabLength);
  seq2_tmp = vxor(seq2_tmp, seq2_init_table_5_tmp, MASKREAD_OFF, tabLength);
  seq2_tmp = vxor(seq2_tmp, seq2_init_table_6_tmp, MASKREAD_OFF, tabLength);
  seq2_tmp = vxor(seq2_tmp, seq2_init_table_7_tmp, MASKREAD_OFF, tabLength);
  seq2_tmp = vxor(seq2_tmp, seq2_init_table_8_tmp, MASKREAD_OFF, tabLength);
  seq2_tmp = vxor(seq2_tmp, seq2_init_table_9_tmp, MASKREAD_OFF, tabLength);
  seq2_tmp = vxor(seq2_tmp, seq2_init_table_10_tmp, MASKREAD_OFF, tabLength);
  seq2_tmp = vxor(seq2_tmp, seq2_init_table_11_tmp, MASKREAD_OFF, tabLength);
  seq2_tmp = vxor(seq2_tmp, seq2_init_table_12_tmp, MASKREAD_OFF, tabLength);
  seq2_tmp = vxor(seq2_tmp, seq2_init_table_13_tmp, MASKREAD_OFF, tabLength);
  seq2_tmp = vxor(seq2_tmp, seq2_init_table_14_tmp, MASKREAD_OFF, tabLength);
  seq2_tmp = vxor(seq2_tmp, seq2_init_table_15_tmp, MASKREAD_OFF, tabLength);
  seq2_tmp = vxor(seq2_tmp, seq2_init_table_16_tmp, MASKREAD_OFF, tabLength);
  seq2_tmp = vxor(seq2_tmp, seq2_init_table_17_tmp, MASKREAD_OFF, tabLength);
  seq2_tmp = vxor(seq2_tmp, seq2_init_table_18_tmp, MASKREAD_OFF, tabLength);
  seq2_tmp = vxor(seq2_tmp, seq2_init_table_19_tmp, MASKREAD_OFF, tabLength);
  seq2_tmp = vxor(seq2_tmp, seq2_init_table_20_tmp, MASKREAD_OFF, tabLength);
  seq2_tmp = vxor(seq2_tmp, seq2_init_table_21_tmp, MASKREAD_OFF, tabLength);
  seq2_tmp = vxor(seq2_tmp, seq2_init_table_22_tmp, MASKREAD_OFF, tabLength);
  seq2_tmp = vxor(seq2_tmp, seq2_init_table_23_tmp, MASKREAD_OFF, tabLength);

  vreturn(seq1_vec1, sizeof(seq1_vec1), seq2_tmp, sizeof(seq2_tmp));
}
