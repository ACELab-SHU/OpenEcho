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

int Task_nrPBCHDescramble_2(__v2048i8 demod, __v2048i8 seq1_vec1, __v2048i8 seq2_tmp, __v2048i16 seq2_trans_table_0_vec,
                            __v2048i16 seq2_trans_table_1_vec, __v2048i16 seq2_trans_table_2_vec,
                            __v2048i16 seq2_trans_table_3_vec, __v2048i16 seq2_trans_table_4_vec,
                            __v2048i16 seq2_trans_table_5_vec, __v2048i16 seq2_trans_table_6_vec) {

  __v2048i16 seq2_shuffle_index;
  vclaim(seq2_shuffle_index);
  vrange(seq2_shuffle_index, tabLength);

  __v2048i8 seq2_vec;
  vclaim(seq2_vec);
  vshuffle(seq2_vec, seq2_shuffle_index, seq2_tmp, SHUFFLE_SCATTER, tabLength);

  __v2048i8 seq2_trans_0_tmp;
  __v2048i8 seq2_trans_1_tmp;
  __v2048i8 seq2_trans_2_tmp;
  __v2048i8 seq2_trans_3_tmp;
  __v2048i8 seq2_trans_4_tmp;
  __v2048i8 seq2_trans_5_tmp;
  __v2048i8 seq2_trans_6_tmp;
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

  __v2048i8 seq;
  seq = vxor(seq1_vec1, seq2_vec, MASKREAD_OFF, sequenceLength);
  seq = vmul(seq, -1, MASKREAD_OFF, sequenceLength);

  __v2048i8 pbchbits;
  pbchbits = vxor(demod, seq, MASKREAD_OFF, sequenceLength);

  vreturn(pbchbits, sizeof(pbchbits));
}