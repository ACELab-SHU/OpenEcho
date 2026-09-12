
/**
 * ****************************************
 * @file        nrExtractResources.c
 * @brief       extract resources
 * @author      yuanfeng
 * @date        2024.6.6
 * @copyright   ACE-Lab(Shanghai University)
 * ****************************************
 */

#include "riscv_printf.h"
#include "venus.h"

typedef short __v2048i16 __attribute__((ext_vector_type(2048)));
typedef short __v4096i16 __attribute__((ext_vector_type(4096)));
typedef char  __v4096i8 __attribute__((ext_vector_type(4096)));

// short seq1[288] = {0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 1, 0, 1, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 1, 1, 1, 1, 0, 1, 0,
//                    0, 0, 1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 1, 1, 0, 1, 0, 1, 0, 1, 1, 0,
//                    0, 0, 0, 1, 0, 0, 1, 0, 0, 1, 1, 1, 1, 0, 1, 0, 0, 0, 1, 1, 0, 0, 1, 1, 1, 1, 0, 0, 1, 1, 0, 1,
//                    0, 0, 0, 0, 0, 0, 1, 1, 0, 1, 0, 1, 0, 1, 1, 1, 0, 1, 0, 1, 1, 0, 1, 1, 0, 1, 0, 0, 1, 0, 1, 0,
//                    0, 0, 1, 1, 0, 0, 1, 1, 1, 1, 0, 1, 1, 0, 1, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 1, 0, 1, 1, 1,
//                    0, 1, 0, 1, 1, 0, 1, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 1, 0, 0, 1, 1, 1, 0, 0, 0, 1, 1, 0, 1, 1,
//                    0, 0, 0, 1, 0, 1, 0, 0, 0, 1, 1, 0, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 1, 1, 1,
//                    0, 1, 1, 0, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 0, 1, 1, 1, 1, 0, 0, 0,
//                    0, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 0, 1, 1, 1, 1, 0, 0, 1, 0, 1, 1, 1, 0, 0, 1, 1};

// short seq2_init_table_0[31] = {1, 2, 3, 4, 5, 6, 7, 8, 0, 1, 2, 0, 0, 1, 2,
// 0,
//                                1, 2, 3, 0, 1, 2, 3, 0, 1, 2, 3, 4, 0, 0, 0};
// short seq2_init_table_1[31] = {2, 3, 4, 5, 6, 7, 8, 9, 1, 2, 3, 1, 5, 6, 7,
// 1,
//                                2, 3, 4, 1, 2, 3, 4, 1, 2, 3, 4, 5, 1, 4, 2};
// short seq2_init_table_2[31] = {3, 4, 5, 6, 7, 8, 9, 10, 2, 3, 4, 2, 6, 7, 8,
// 2,
//                                3, 4, 5, 2, 3, 4, 5, 2,  3, 4, 5, 6, 2, 6, 3};
// short seq2_init_table_3[31] = {8, 9, 10, 11, 12, 13, 14, 15, 3, 4, 5,
//                                4, 7, 8,  9,  8,  9,  10, 11, 3, 4, 5,
//                                6, 3, 4,  5,  6,  7,  3,  7,  5};
// short seq2_init_table_4[31] = {12, 13, 14, 15, 16, 17, 18, 19, 9, 10, 11,
//                                5,  13, 14, 15, 9,  10, 11, 12, 4, 5,  6,
//                                7,  4,  5,  6,  7,  8,  5,  8,  7};
// short seq2_init_table_5[31] = {16, 17, 18, 19, 20, 21, 22, 23, 10, 11, 12,
//                                6,  14, 15, 16, 10, 11, 12, 13, 5,  6,  7,
//                                8,  5,  6,  7,  8,  9,  6,  9,  8};
// short seq2_init_table_6[31] = {19, 20, 21, 22, 23, 24, 25, 26, 11, 12, 13,
//                                12, 15, 16, 17, 16, 17, 18, 19, 6,  7,  8,
//                                9,  6,  7,  8,  9,  10, 7,  10, 9};
// short seq2_init_table_7[31] = {20, 21, 22, 23, 24, 25, 26, 27, 16, 17, 18,
//                                13, 20, 21, 22, 17, 18, 19, 20, 12, 13, 14,
//                                15, 7,  8,  9,  10, 11, 8,  11, 10};
// short seq2_init_table_8[31] = {23, 24, 25, 26, 27, 28, 29, 30, 20, 21, 22,
//                                14, 24, 25, 26, 18, 19, 20, 21, 13, 14, 15,
//                                16, 8,  9,  10, 11, 12, 9,  12, 11};
// short seq2_init_table_9[31] = {31, 31, 31, 31, 31, 31, 31, 31, 24, 25, 26,
//                                19, 28, 29, 30, 23, 24, 25, 26, 14, 15, 16,
//                                17, 9,  10, 11, 12, 13, 10, 13, 12};
// short seq2_init_table_10[31] = {31, 31, 31, 31, 31, 31, 31, 31, 27, 28, 29,
//                                 23, 31, 31, 31, 27, 28, 29, 30, 20, 21, 22,
//                                 23, 10, 11, 12, 13, 14, 11, 14, 13};
// short seq2_init_table_11[31] = {31, 31, 31, 31, 31, 31, 31, 31, 28, 29, 30,
//                                 27, 31, 31, 31, 31, 31, 31, 31, 21, 22, 23,
//                                 24, 16, 17, 18, 19, 20, 12, 15, 14};
// short seq2_init_table_12[31] = {31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31,
//                                 30, 31, 31, 31, 31, 31, 31, 31, 22, 23, 24,
//                                 25, 17, 18, 19, 20, 21, 13, 16, 15};
// short seq2_init_table_13[31] = {31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31,
//                                 31, 31, 31, 31, 31, 31, 31, 31, 27, 28, 29,
//                                 30, 18, 19, 20, 21, 22, 14, 22, 16};
// short seq2_init_table_14[31] = {31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31,
//                                 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31,
//                                 31, 24, 25, 26, 27, 28, 15, 23, 17};
// short seq2_init_table_15[31] = {31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31,
//                                 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31,
//                                 31, 25, 26, 27, 28, 29, 21, 24, 23};
// short seq2_init_table_16[31] = {31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31,
//                                 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31,
//                                 31, 26, 27, 28, 29, 30, 22, 30, 24};
// short seq2_init_table_17[31] = {31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31,
//                                 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31,
//                                 31, 31, 31, 31, 31, 31, 23, 31, 25};
// short seq2_init_table_18[31] = {31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31,
//                                 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31,
//                                 31, 31, 31, 31, 31, 31, 29, 31, 31};
// short seq2_init_table_19[31] = {31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31,
//                                 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31,
//                                 31, 31, 31, 31, 31, 31, 30, 31, 31};

// short seq2_trans_table_0[31] = {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10,
//                                 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21,
//                                 22, 23, 24, 25, 26, 27, 0,  0,  0};
// short seq2_trans_table_1[31] = {1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11,
//                                 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22,
//                                 23, 24, 25, 26, 27, 28, 1,  4,  2};
// short seq2_trans_table_2[31] = {2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12,
//                                 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23,
//                                 24, 25, 26, 27, 28, 29, 2,  29, 3};
// short seq2_trans_table_3[31] = {3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13,
//                                 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24,
//                                 25, 26, 27, 28, 29, 30, 3,  30, 5};
// short seq2_trans_table_4[31] = {31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31,
//                                 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31,
//                                 31, 31, 31, 31, 31, 31, 28, 31, 30};
// short seq2_trans_table_5[31] = {31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31,
//                                 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31,
//                                 31, 31, 31, 31, 31, 31, 29, 31, 31};
// short seq2_trans_table_6[31] = {31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31,
//                                 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31,
//                                 31, 31, 31, 31, 31, 31, 30, 31, 31};

// short init2[31 + 1] = {0};

int tabLength = 31;

#define recpSqrt2 0.70710678
// short ncellid = 0;
VENUS_INLINE __v4096i8
nrPRBS(__v4096i8 seq1_vec, __v4096i8 init2_vec, __v2048i16 seq2_init_table_0_vec, __v2048i16 seq2_init_table_1_vec,
       __v2048i16 seq2_init_table_2_vec, __v2048i16 seq2_init_table_3_vec, __v2048i16 seq2_init_table_4_vec,
       __v2048i16 seq2_init_table_5_vec, __v2048i16 seq2_init_table_6_vec, __v2048i16 seq2_init_table_7_vec,
       __v2048i16 seq2_init_table_8_vec, __v2048i16 seq2_init_table_9_vec, __v2048i16 seq2_init_table_10_vec,
       __v2048i16 seq2_init_table_11_vec, __v2048i16 seq2_init_table_12_vec, __v2048i16 seq2_init_table_13_vec,
       __v2048i16 seq2_init_table_14_vec, __v2048i16 seq2_init_table_15_vec, __v2048i16 seq2_init_table_16_vec,
       __v2048i16 seq2_init_table_17_vec, __v2048i16 seq2_init_table_18_vec, __v2048i16 seq2_init_table_19_vec,
       __v2048i16 seq2_trans_table_0_vec, __v2048i16 seq2_trans_table_1_vec, __v2048i16 seq2_trans_table_2_vec,
       __v2048i16 seq2_trans_table_3_vec, __v2048i16 seq2_trans_table_4_vec, __v2048i16 seq2_trans_table_5_vec,
       __v2048i16 seq2_trans_table_6_vec, int sequenceLength) {
  /*--------------------DMRS GENERATION--------------------*/
  // gengrate init 31 seq
  __v4096i8 seq2_init_table_0_tmp;
  vclaim(seq2_init_table_0_tmp);
  vshuffle(seq2_init_table_0_tmp, seq2_init_table_0_vec, init2_vec, SHUFFLE_GATHER, tabLength);
  __v4096i8 seq2_init_table_1_tmp;
  vclaim(seq2_init_table_1_tmp);
  vshuffle(seq2_init_table_1_tmp, seq2_init_table_1_vec, init2_vec, SHUFFLE_GATHER, tabLength);
  __v4096i8 seq2_init_table_2_tmp;
  vclaim(seq2_init_table_2_tmp);
  vshuffle(seq2_init_table_2_tmp, seq2_init_table_2_vec, init2_vec, SHUFFLE_GATHER, tabLength);
  __v4096i8 seq2_init_table_3_tmp;
  vclaim(seq2_init_table_3_tmp);
  vshuffle(seq2_init_table_3_tmp, seq2_init_table_3_vec, init2_vec, SHUFFLE_GATHER, tabLength);
  __v4096i8 seq2_init_table_4_tmp;
  vclaim(seq2_init_table_4_tmp);
  vshuffle(seq2_init_table_4_tmp, seq2_init_table_4_vec, init2_vec, SHUFFLE_GATHER, tabLength);
  __v4096i8 seq2_init_table_5_tmp;
  vclaim(seq2_init_table_5_tmp);
  vshuffle(seq2_init_table_5_tmp, seq2_init_table_5_vec, init2_vec, SHUFFLE_GATHER, tabLength);
  __v4096i8 seq2_init_table_6_tmp;
  vclaim(seq2_init_table_6_tmp);
  vshuffle(seq2_init_table_6_tmp, seq2_init_table_6_vec, init2_vec, SHUFFLE_GATHER, tabLength);
  __v4096i8 seq2_init_table_7_tmp;
  vclaim(seq2_init_table_7_tmp);
  vshuffle(seq2_init_table_7_tmp, seq2_init_table_7_vec, init2_vec, SHUFFLE_GATHER, tabLength);
  __v4096i8 seq2_init_table_8_tmp;
  vclaim(seq2_init_table_8_tmp);
  vshuffle(seq2_init_table_8_tmp, seq2_init_table_8_vec, init2_vec, SHUFFLE_GATHER, tabLength);
  __v4096i8 seq2_init_table_9_tmp;
  vclaim(seq2_init_table_9_tmp);
  vshuffle(seq2_init_table_9_tmp, seq2_init_table_9_vec, init2_vec, SHUFFLE_GATHER, tabLength);
  __v4096i8 seq2_init_table_10_tmp;
  vclaim(seq2_init_table_10_tmp);
  vshuffle(seq2_init_table_10_tmp, seq2_init_table_10_vec, init2_vec, SHUFFLE_GATHER, tabLength);
  __v4096i8 seq2_init_table_11_tmp;
  vclaim(seq2_init_table_11_tmp);
  vshuffle(seq2_init_table_11_tmp, seq2_init_table_11_vec, init2_vec, SHUFFLE_GATHER, tabLength);
  __v4096i8 seq2_init_table_12_tmp;
  vclaim(seq2_init_table_12_tmp);
  vshuffle(seq2_init_table_12_tmp, seq2_init_table_12_vec, init2_vec, SHUFFLE_GATHER, tabLength);
  __v4096i8 seq2_init_table_13_tmp;
  vclaim(seq2_init_table_13_tmp);
  vshuffle(seq2_init_table_13_tmp, seq2_init_table_13_vec, init2_vec, SHUFFLE_GATHER, tabLength);
  __v4096i8 seq2_init_table_14_tmp;
  vclaim(seq2_init_table_14_tmp);
  vshuffle(seq2_init_table_14_tmp, seq2_init_table_14_vec, init2_vec, SHUFFLE_GATHER, tabLength);
  __v4096i8 seq2_init_table_15_tmp;
  vclaim(seq2_init_table_15_tmp);
  vshuffle(seq2_init_table_15_tmp, seq2_init_table_15_vec, init2_vec, SHUFFLE_GATHER, tabLength);
  __v4096i8 seq2_init_table_16_tmp;
  vclaim(seq2_init_table_16_tmp);
  vshuffle(seq2_init_table_16_tmp, seq2_init_table_16_vec, init2_vec, SHUFFLE_GATHER, tabLength);
  __v4096i8 seq2_init_table_17_tmp;
  vclaim(seq2_init_table_17_tmp);
  vshuffle(seq2_init_table_17_tmp, seq2_init_table_17_vec, init2_vec, SHUFFLE_GATHER, tabLength);
  __v4096i8 seq2_init_table_18_tmp;
  vclaim(seq2_init_table_18_tmp);
  vshuffle(seq2_init_table_18_tmp, seq2_init_table_18_vec, init2_vec, SHUFFLE_GATHER, tabLength);
  __v4096i8 seq2_init_table_19_tmp;
  vclaim(seq2_init_table_19_tmp);
  vshuffle(seq2_init_table_19_tmp, seq2_init_table_19_vec, init2_vec, SHUFFLE_GATHER, tabLength);
  __v4096i8 seq2_tmp;
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

  __v2048i16 seq2_shuffle_index;
  vclaim(seq2_shuffle_index);
  vrange(seq2_shuffle_index, tabLength);

  __v4096i8 seq2_vec;
  vclaim(seq2_vec);
  vshuffle(seq2_vec, seq2_shuffle_index, seq2_tmp, SHUFFLE_SCATTER, tabLength);

  // gengrate the rest seqs
  __v4096i8 seq2_trans_0_tmp;
  __v4096i8 seq2_trans_1_tmp;
  __v4096i8 seq2_trans_2_tmp;
  __v4096i8 seq2_trans_3_tmp;
  __v4096i8 seq2_trans_4_tmp;
  __v4096i8 seq2_trans_5_tmp;
  __v4096i8 seq2_trans_6_tmp;
  vclaim(seq2_trans_0_tmp);
  vclaim(seq2_trans_1_tmp);
  vclaim(seq2_trans_2_tmp);
  vclaim(seq2_trans_3_tmp);
  vclaim(seq2_trans_4_tmp);
  vclaim(seq2_trans_5_tmp);
  vclaim(seq2_trans_6_tmp);

  for (int i = 1; i < ((sequenceLength + tabLength - 1) / tabLength); i++) {
    vshuffle(seq2_trans_0_tmp, seq2_trans_table_0_vec, seq2_tmp, SHUFFLE_GATHER, tabLength);
    vshuffle(seq2_trans_1_tmp, seq2_trans_table_1_vec, seq2_tmp, SHUFFLE_GATHER, tabLength);
    vshuffle(seq2_trans_2_tmp, seq2_trans_table_2_vec, seq2_tmp, SHUFFLE_GATHER, tabLength);
    vshuffle(seq2_trans_3_tmp, seq2_trans_table_3_vec, seq2_tmp, SHUFFLE_GATHER, tabLength);
    vshuffle(seq2_trans_4_tmp, seq2_trans_table_4_vec, seq2_tmp, SHUFFLE_GATHER, tabLength);
    vshuffle(seq2_trans_5_tmp, seq2_trans_table_5_vec, seq2_tmp, SHUFFLE_GATHER, tabLength);
    vshuffle(seq2_trans_6_tmp, seq2_trans_table_6_vec, seq2_tmp, SHUFFLE_GATHER, tabLength);
    vbrdcst(seq2_tmp, 0, MASKREAD_OFF, tabLength);
    seq2_tmp           = vxor(seq2_trans_0_tmp, seq2_trans_1_tmp, MASKREAD_OFF, tabLength);
    seq2_tmp           = vxor(seq2_tmp, seq2_trans_2_tmp, MASKREAD_OFF, tabLength);
    seq2_tmp           = vxor(seq2_tmp, seq2_trans_3_tmp, MASKREAD_OFF, tabLength);
    seq2_tmp           = vxor(seq2_tmp, seq2_trans_4_tmp, MASKREAD_OFF, tabLength);
    seq2_tmp           = vxor(seq2_tmp, seq2_trans_5_tmp, MASKREAD_OFF, tabLength);
    seq2_tmp           = vxor(seq2_tmp, seq2_trans_6_tmp, MASKREAD_OFF, tabLength);
    seq2_shuffle_index = vsadd(seq2_shuffle_index, tabLength, MASKREAD_OFF, tabLength);
    vshuffle(seq2_vec, seq2_shuffle_index, seq2_tmp, SHUFFLE_SCATTER, tabLength);
  }

  // generate dmrs
  __v4096i8 seq;
  seq = vxor(seq1_vec, seq2_vec, MASKREAD_OFF, sequenceLength);
  seq = vmul(seq, -2, MASKREAD_OFF, sequenceLength);
  seq = vsadd(seq, 1, MASKREAD_OFF, sequenceLength);
  // seq = vaddmul(seq, -2, 1, MASKREAD_OFF, sequenceLength);
  return seq;
}

typedef struct {
  short data;
} __attribute__((aligned(64))) short_struct;

int Task_nrPBCHDMRS(
    __v4096i8 seq1_vec, __v2048i16 seq2_init_table_0_vec, __v2048i16 seq2_init_table_1_vec,
    __v2048i16 seq2_init_table_2_vec, __v2048i16 seq2_init_table_3_vec, __v2048i16 seq2_init_table_4_vec,
    __v2048i16 seq2_init_table_5_vec, __v2048i16 seq2_init_table_6_vec, __v2048i16 seq2_init_table_7_vec,
    __v2048i16 seq2_init_table_8_vec, __v2048i16 seq2_init_table_9_vec, __v2048i16 seq2_init_table_10_vec,
    __v2048i16 seq2_init_table_11_vec, __v2048i16 seq2_init_table_12_vec, __v2048i16 seq2_init_table_13_vec,
    __v2048i16 seq2_init_table_14_vec, __v2048i16 seq2_init_table_15_vec, __v2048i16 seq2_init_table_16_vec,
    __v2048i16 seq2_init_table_17_vec, __v2048i16 seq2_init_table_18_vec, __v2048i16 seq2_init_table_19_vec,
    __v2048i16 seq2_trans_table_0_vec, __v2048i16 seq2_trans_table_1_vec, __v2048i16 seq2_trans_table_2_vec,
    __v2048i16 seq2_trans_table_3_vec, __v2048i16 seq2_trans_table_4_vec, __v2048i16 seq2_trans_table_5_vec,
    __v2048i16 seq2_trans_table_6_vec, short_struct input_ncellid, short_struct input_ibar_SSB)
    __attribute__((aligned(64))) {
  // short ncellid = 133;
  // short ibar_SSB = 5;
  short ncellid  = input_ncellid.data;
  short ibar_SSB = input_ibar_SSB.data;

  unsigned int cinit = (1 << 11) * (ibar_SSB + 1) * ((ncellid / 4) + 1) + (1 << 6) * (ibar_SSB + 1) + (ncellid % 4);
  __v4096i8    init;
  vclaim(init);
  // vbrdcst(init, 0, MASKREAD_OFF, 32);
  vbarrier();
  VSPM_OPEN();
  int init_addr = vaddr(init);
  for (int i = 0; i < 32; ++i) {
    *(volatile char *)(init_addr + i) = cinit & 0b1;
    cinit                             = cinit >> 1;
  }
  VSPM_CLOSE();
  // input parameters: cinit(need ibar_SSB and ncellid)
  int       fractionLength = 7;
  int       sequenceLength = 288;
  __v4096i8 seq;
  seq =
      nrPRBS(seq1_vec, init, seq2_init_table_0_vec, seq2_init_table_1_vec, seq2_init_table_2_vec, seq2_init_table_3_vec,
             seq2_init_table_4_vec, seq2_init_table_5_vec, seq2_init_table_6_vec, seq2_init_table_7_vec,
             seq2_init_table_8_vec, seq2_init_table_9_vec, seq2_init_table_10_vec, seq2_init_table_11_vec,
             seq2_init_table_12_vec, seq2_init_table_13_vec, seq2_init_table_14_vec, seq2_init_table_15_vec,
             seq2_init_table_16_vec, seq2_init_table_17_vec, seq2_init_table_18_vec, seq2_init_table_19_vec,
             seq2_trans_table_0_vec, seq2_trans_table_1_vec, seq2_trans_table_2_vec, seq2_trans_table_3_vec,
             seq2_trans_table_4_vec, seq2_trans_table_5_vec, seq2_trans_table_6_vec, sequenceLength);
  __v2048i16 dmrs_shuffle_index;
  vclaim(dmrs_shuffle_index);
  vrange(dmrs_shuffle_index, 144);
  dmrs_shuffle_index = vmul(dmrs_shuffle_index, 2, MASKREAD_OFF, 144);
  __v4096i8 dmrs_real;
  vclaim(dmrs_real);
  vshuffle(dmrs_real, dmrs_shuffle_index, seq, SHUFFLE_GATHER, 144);
  dmrs_real          = vmul(dmrs_real, (int)(recpSqrt2 * (1 << fractionLength)), MASKREAD_OFF, 144);
  dmrs_shuffle_index = vsadd(dmrs_shuffle_index, 1, MASKREAD_OFF, 144);
  __v4096i8 dmrs_imag;
  vclaim(dmrs_imag);
  vshuffle(dmrs_imag, dmrs_shuffle_index, seq, SHUFFLE_GATHER, 144);
  dmrs_imag = vmul(dmrs_imag, (int)(recpSqrt2 * (1 << fractionLength)), MASKREAD_OFF, 144);

  vreturn(dmrs_real, 144, dmrs_imag, 144);
}