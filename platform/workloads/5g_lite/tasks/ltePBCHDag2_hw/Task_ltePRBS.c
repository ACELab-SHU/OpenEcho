
/**
 * ****************************************
 * @file        Task nrPDSCHDescramble.c
 * @brief       PDSCH Descramble
 * @author      yuanfeng
 * @date        2024.7.28
 * @copyright   ACE-Lab(Shanghai University)
 * ****************************************
 */

 #include "data_type.h"
 #include "riscv_printf.h"
 #include "venus.h"
 
 typedef short __v2048i16 __attribute__((ext_vector_type(2048)));
 typedef short __v4096i16 __attribute__((ext_vector_type(4096)));
 typedef char __v4096i8 __attribute__((ext_vector_type(4096)));
 
 int tabLength = 31;
 
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
        __v2048i16 seq2_trans_table_6_vec, int sequenceLength)
 {
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
 
   for (int i = 1; i < ((sequenceLength + tabLength - 1) / tabLength); i++)
   {
     vshuffle(seq2_trans_0_tmp, seq2_trans_table_0_vec, seq2_tmp, SHUFFLE_GATHER, tabLength);
     vshuffle(seq2_trans_1_tmp, seq2_trans_table_1_vec, seq2_tmp, SHUFFLE_GATHER, tabLength);
     vshuffle(seq2_trans_2_tmp, seq2_trans_table_2_vec, seq2_tmp, SHUFFLE_GATHER, tabLength);
     vshuffle(seq2_trans_3_tmp, seq2_trans_table_3_vec, seq2_tmp, SHUFFLE_GATHER, tabLength);
     vshuffle(seq2_trans_4_tmp, seq2_trans_table_4_vec, seq2_tmp, SHUFFLE_GATHER, tabLength);
     vshuffle(seq2_trans_5_tmp, seq2_trans_table_5_vec, seq2_tmp, SHUFFLE_GATHER, tabLength);
     vshuffle(seq2_trans_6_tmp, seq2_trans_table_6_vec, seq2_tmp, SHUFFLE_GATHER, tabLength);
     vbrdcst(seq2_tmp, 0, MASKREAD_OFF, tabLength);
     seq2_tmp = vxor(seq2_trans_0_tmp, seq2_trans_1_tmp, MASKREAD_OFF, tabLength);
     seq2_tmp = vxor(seq2_tmp, seq2_trans_2_tmp, MASKREAD_OFF, tabLength);
     seq2_tmp = vxor(seq2_tmp, seq2_trans_3_tmp, MASKREAD_OFF, tabLength);
     seq2_tmp = vxor(seq2_tmp, seq2_trans_4_tmp, MASKREAD_OFF, tabLength);
     seq2_tmp = vxor(seq2_tmp, seq2_trans_5_tmp, MASKREAD_OFF, tabLength);
     seq2_tmp = vxor(seq2_tmp, seq2_trans_6_tmp, MASKREAD_OFF, tabLength);
     seq2_shuffle_index = vsadd(seq2_shuffle_index, tabLength, MASKREAD_OFF, tabLength);
     vshuffle(seq2_vec, seq2_shuffle_index, seq2_tmp, SHUFFLE_SCATTER, tabLength);
   }
 
   // generate dmrs
   __v4096i8 seq;
   seq = vxor(seq1_vec, seq2_vec, MASKREAD_OFF, sequenceLength);
   seq = vmul(seq, -2, MASKREAD_OFF, sequenceLength);
   seq = vsadd(seq, 1, MASKREAD_OFF, sequenceLength);
   // seq = vmul(seq, -1, MASKREAD_OFF, sequenceLength);
   // seq = vaddmul(seq, -2, 1, MASKREAD_OFF, sequenceLength);
   return seq;
 }
 
 int Task_ltePRBS(
     __v4096i8 seq1_vec, __v2048i16 seq2_init_table_0_vec, __v2048i16 seq2_init_table_1_vec,
     __v2048i16 seq2_init_table_2_vec, __v2048i16 seq2_init_table_3_vec, __v2048i16 seq2_init_table_4_vec,
     __v2048i16 seq2_init_table_5_vec, __v2048i16 seq2_init_table_6_vec, __v2048i16 seq2_init_table_7_vec,
     __v2048i16 seq2_init_table_8_vec, __v2048i16 seq2_init_table_9_vec, __v2048i16 seq2_init_table_10_vec,
     __v2048i16 seq2_init_table_11_vec, __v2048i16 seq2_init_table_12_vec, __v2048i16 seq2_init_table_13_vec,
     __v2048i16 seq2_init_table_14_vec, __v2048i16 seq2_init_table_15_vec, __v2048i16 seq2_init_table_16_vec,
     __v2048i16 seq2_init_table_17_vec, __v2048i16 seq2_init_table_18_vec, __v2048i16 seq2_init_table_19_vec,
     __v2048i16 seq2_trans_table_0_vec, __v2048i16 seq2_trans_table_1_vec, __v2048i16 seq2_trans_table_2_vec,
     __v2048i16 seq2_trans_table_3_vec, __v2048i16 seq2_trans_table_4_vec, __v2048i16 seq2_trans_table_5_vec,
     __v2048i16 seq2_trans_table_6_vec, short_struct nCellID, short_struct input_sequence_length)
 {
 
   uint16_t NID = nCellID.data;
   int sequenceLength = input_sequence_length.data;
   int init2 = NID % (1 << 31);
 
   __v4096i8 init2_vec;
   vclaim(init2_vec);
   vbrdcst(init2_vec, 0, MASKREAD_OFF, 32);
   vbrdcst(init2_vec, 0, MASKREAD_OFF, 32);
   vbrdcst(init2_vec, 0, MASKREAD_OFF, 32);
   vbrdcst(init2_vec, 0, MASKREAD_OFF, 32);
   vbarrier();
   VSPM_OPEN();
   int init2_vec_addr = vaddr(init2_vec);
   for (int i = 0; i < 31; ++i)
   {
     *(volatile char *)(init2_vec_addr + i) = init2 & 0b1;
     init2 = init2 >> 1;
   }
   VSPM_CLOSE();
 
   __v4096i8 seq;
   seq = nrPRBS(seq1_vec, init2_vec, seq2_init_table_0_vec, seq2_init_table_1_vec, seq2_init_table_2_vec,
                seq2_init_table_3_vec, seq2_init_table_4_vec, seq2_init_table_5_vec, seq2_init_table_6_vec,
                seq2_init_table_7_vec, seq2_init_table_8_vec, seq2_init_table_9_vec, seq2_init_table_10_vec,
                seq2_init_table_11_vec, seq2_init_table_12_vec, seq2_init_table_13_vec, seq2_init_table_14_vec,
                seq2_init_table_15_vec, seq2_init_table_16_vec, seq2_init_table_17_vec, seq2_init_table_18_vec,
                seq2_init_table_19_vec, seq2_trans_table_0_vec, seq2_trans_table_1_vec, seq2_trans_table_2_vec,
                seq2_trans_table_3_vec, seq2_trans_table_4_vec, seq2_trans_table_5_vec, seq2_trans_table_6_vec,
                sequenceLength);

 
   vreturn(seq, sequenceLength);
 }
 