/*
 * @Author: YihaoShen
 * @Date: 2024-11-19 15:03:23
 * @LastEditors: Yihao Shen shenyihao@shu.edu.cn
 * user.email & please set dead value or install git & please set dead value or install git
 * @LastEditTime: 2025-06-09 10:48:39
 * @FilePath: /5G-Lite-0530/venus_soc/software/emulator/task_src/lteSssSearch.c
 * @Description:
 *
 * Copyright (c) 2024 by ACE_Lab（Shanghai University）, All Rights Reserved.
 */

#include "riscv_printf.h"
#include "stdint.h"
#include "venus.h"

typedef char __v4096i8 __attribute__((ext_vector_type(4096)));
typedef short __v2048i16 __attribute__((ext_vector_type(2048)));

// short SSS_real[62] = {-53, 67, -5, 24, -9, -23, -5, 17,  13, -25, 25,  42, -1, 31,  24, 11, -23, 13,  -29, -14, 52,
//                       -22, 19, 53, -1, 3,  -8,  2,  52,  13, -21, 14,  8,  56, 29,  28, -8, 10,  -22, -4,  3,   14,
//                       2,   8,  -4, -5, 8,  -15, -7, -16, 28, -22, -23, 0,  -9, -42, 10, 59, 4,   -33, 28,  -6};
// short SSS_imag[62] = {27,  16, 31, -12, 0,  -18, 0,   -8,  -28, -16, -40, -3, -16, -11, 13, 26, 3,  29,  10,  12, 7,
//                       46,  24, 19, -11, 5,  -38, -26, -12, -62, 10,  -13, 11, 29,  41,  0,  14, 18, -35, -13, 2,  30,
//                       -51, -5, 8,  -10, 48, -4,  -5,  2,   -9,  19,  -17, 18, -4,  17,  17, 3,  28, -22, 16,  29};

// short S_base[62] = {1,  1,  1,  1,  -1, 1,  1,  -1, 1, -1, -1, 1,  1, -1, -1, -1, -1, -1, 1, 1,  1,
//                     -1, -1, 1,  -1, -1, -1, 1,  -1, 1, -1, 1,  1,  1, 1,  -1, 1,  1,  -1, 1, -1, -1,
//                     1,  1,  -1, -1, -1, -1, -1, 1,  1, 1,  -1, -1, 1, -1, -1, -1, 1,  -1, 1, -1};

// short C_base[62] = {1,  1,  1,  1,  -1, 1, -1, 1,  -1, -1, -1, 1, -1, -1, 1,  1, 1,  -1, -1, -1, -1,
//                     -1, 1,  1,  -1, -1, 1, -1, 1,  1,  -1, 1,  1, 1,  1,  -1, 1, -1, 1,  -1, -1, -1,
//                     1,  -1, -1, 1,  1,  1, -1, -1, -1, -1, -1, 1, 1,  -1, -1, 1, -1, 1,  1,  -1};

// short Z_base[62] = {1, 1,  1,  1,  -1, -1, -1, 1,  1,  -1, -1, 1,  -1, -1, -1, -1, -1, 1, -1, 1,  1,
//                     1, -1, 1,  1,  -1, 1,  -1, 1,  -1, -1, 1,  1,  1,  1,  -1, -1, -1, 1, 1,  -1, -1,
//                     1, -1, -1, -1, -1, -1, 1,  -1, 1,  1,  1,  -1, 1,  1,  -1, 1,  -1, 1, -1, -1};

int sssEst[31] = {0};
int sssEstOdd[31] = {0};

typedef struct {
  short data;
} __attribute__((aligned(64))) short_struct;

int Task_lteSssSearch(__v4096i8 OFDM_Real, __v4096i8 OFDM_Imag, short_struct NCELLID2, __v4096i8 S_BaseSeq,
                      __v4096i8 C_BaseSeq, __v4096i8 Z_BaseSeq) {
  short NID2 = NCELLID2.data;

  int maxMEven = 0;
  int maxMEvenIndex = 0;
  __v4096i8 SSS_RealVec;
  __v4096i8 SSS_ImagVec;
  vclaim(SSS_RealVec);
  vclaim(SSS_ImagVec);

  __v2048i16 sssIndices;
  vclaim(sssIndices);
  vrange(sssIndices, 62);
  sssIndices = vsadd(sssIndices, 5, MASKREAD_OFF, 62);
  vshuffle(SSS_RealVec, sssIndices, OFDM_Real, SHUFFLE_GATHER, 62);
  vshuffle(SSS_ImagVec, sssIndices, OFDM_Imag, SHUFFLE_GATHER, 62);

  __v2048i16 S_index;
  __v2048i16 C_index;
  __v2048i16 Z_index;
  vclaim(S_index);
  vclaim(C_index);
  vclaim(Z_index);
  vrange(S_index, 31);
  vrange(C_index, 31);
  vrange(Z_index, 31);

  __v2048i16 S_Indices;
  __v2048i16 C_Indices;
  __v2048i16 Z_Indices;
  vclaim(S_Indices);
  vclaim(C_Indices);
  vclaim(Z_Indices);

  __v4096i8 d_even;
  __v4096i8 d_odd;
  __v4096i8 s_temp;
  __v4096i8 c_temp;
  __v4096i8 z_temp;
  vclaim(s_temp);
  vclaim(c_temp);
  vclaim(z_temp);
  __v4096i8 sssEst_real_vec;
  __v4096i8 sssEst_imag_vec;

  __v4096i8 SSS_Real_even;
  __v4096i8 SSS_Imag_even;
  __v4096i8 SSS_Real_odd;
  __v4096i8 SSS_Imag_odd;
  vclaim(SSS_Real_even);
  vclaim(SSS_Imag_even);
  vclaim(SSS_Real_odd);
  vclaim(SSS_Imag_odd);
  __v2048i16 index_temp;
  vclaim(index_temp);
  vrange(index_temp, 31);
  index_temp = vmul(index_temp, 2, MASKREAD_OFF, 31);
  vshuffle(SSS_Real_even, index_temp, SSS_RealVec, SHUFFLE_GATHER, 31);
  vshuffle(SSS_Imag_even, index_temp, SSS_ImagVec, SHUFFLE_GATHER, 31);
  index_temp = vadd(index_temp, 1, MASKREAD_OFF, 31);
  vshuffle(SSS_Real_odd, index_temp, SSS_RealVec, SHUFFLE_GATHER, 31);
  vshuffle(SSS_Imag_odd, index_temp, SSS_ImagVec, SHUFFLE_GATHER, 31);

  __v4096i8 sssEst_mean_real;
  __v4096i8 sssEst_mean_imag;
  vclaim(sssEst_mean_real);
  vclaim(sssEst_mean_imag);

  C_Indices = vadd(C_index, NID2, MASKREAD_OFF, 31);
  vshuffle(c_temp, C_Indices, C_BaseSeq, SHUFFLE_GATHER, 31);
  for (int mEven = 0; mEven < 31; mEven++) {
    /* code */
    S_Indices = vadd(S_index, mEven, MASKREAD_OFF, 31);

    vshuffle(s_temp, S_Indices, S_BaseSeq, SHUFFLE_GATHER, 31);

    d_even = vmul(s_temp, c_temp, MASKREAD_OFF, 31);

    sssEst_real_vec = vmul(SSS_Real_even, d_even, MASKREAD_OFF, 31);
    sssEst_imag_vec = vmul(SSS_Imag_even, d_even, MASKREAD_OFF, 31);

    vbrdcst(sssEst_mean_real, 0, MASKREAD_OFF, 127);
    vbrdcst(sssEst_mean_imag, 0, MASKREAD_OFF, 127);
    sssEst_mean_real = vredsum(sssEst_real_vec, MASKREAD_OFF, 31);
    sssEst_mean_imag = vredsum(sssEst_imag_vec, MASKREAD_OFF, 31);
    int32_t sssEst_tmp = 0;
    int sssEst_mean_real_addr = vaddr(sssEst_mean_real);
    vbarrier();
    VSPM_OPEN();
    sssEst_tmp = ((*(volatile char *)(sssEst_mean_real_addr)) & 0xFF);
    VSPM_CLOSE();
    sssEst_mean_real_addr = vaddr(sssEst_mean_real);
    vbarrier();
    VSPM_OPEN();
    sssEst_tmp += (((*(volatile char *)(sssEst_mean_real_addr + 1)) & 0xFF) << 8);
    VSPM_CLOSE();
    sssEst_mean_real_addr = vaddr(sssEst_mean_real);
    vbarrier();
    VSPM_OPEN();
    sssEst_tmp += (((*(volatile char *)(sssEst_mean_real_addr + 2)) & 0xFF) << 16);
    VSPM_CLOSE();
    sssEst_mean_real_addr = vaddr(sssEst_mean_real);
    vbarrier();
    VSPM_OPEN();
    sssEst_tmp += (((*(volatile char *)(sssEst_mean_real_addr + 3)) & 0xFF) << 24);
    VSPM_CLOSE();

    if (sssEst_tmp == INT32_MIN) {
      sssEst[mEven] = INT32_MAX;
    } else {
      sssEst[mEven] = sssEst_tmp > 0 ? sssEst_tmp : -sssEst_tmp;
    }

    int sssEst_mean_imag_addr = vaddr(sssEst_mean_imag);
    vbarrier();
    VSPM_OPEN();
    sssEst_tmp = ((*(volatile char *)(sssEst_mean_imag_addr)) & 0xFF);
    VSPM_CLOSE();
    sssEst_mean_imag_addr = vaddr(sssEst_mean_imag);
    vbarrier();
    VSPM_OPEN();
    sssEst_tmp += (((*(volatile char *)(sssEst_mean_imag_addr + 1)) & 0xFF) << 8);
    VSPM_CLOSE();
    sssEst_mean_imag_addr = vaddr(sssEst_mean_imag);
    vbarrier();
    VSPM_OPEN();
    sssEst_tmp += (((*(volatile char *)(sssEst_mean_imag_addr + 2)) & 0xFF) << 16);
    VSPM_CLOSE();
    sssEst_mean_imag_addr = vaddr(sssEst_mean_imag);
    vbarrier();
    VSPM_OPEN();
    sssEst_tmp += (((*(volatile char *)(sssEst_mean_imag_addr + 3)) & 0xFF) << 24);
    VSPM_CLOSE();
    // printf("sssEst:%hd\n", &sssEst_tmp);
    if (sssEst_tmp == INT32_MIN) {
      sssEst[mEven] += INT32_MAX;
    } else {
      sssEst[mEven] += sssEst_tmp > 0 ? sssEst_tmp : -sssEst_tmp;
    }
    // printf("sssEst:%d\n", &sssEst[mEven]);

    maxMEvenIndex = sssEst[mEven] > maxMEven ? mEven : maxMEvenIndex;
    maxMEven = sssEst[mEven] > maxMEven ? sssEst[mEven] : maxMEven;
  }

  int temp = 0;

  int maxMOdd = 0;
  int maxMOddIndex = 0;
  temp = maxMEvenIndex % 8;
  C_Indices = vadd(C_index, NID2 + 3, MASKREAD_OFF, 31);
  Z_Indices = vadd(Z_index, temp, MASKREAD_OFF, 31);
  vshuffle(c_temp, C_Indices, C_BaseSeq, SHUFFLE_GATHER, 31);
  vshuffle(z_temp, Z_Indices, Z_BaseSeq, SHUFFLE_GATHER, 31);
  for (short mOdd = maxMEvenIndex + 1; mOdd < maxMEvenIndex + 8; mOdd++) {
    /* code */
    if (mOdd <= 30) {
      S_Indices = vadd(S_index, mOdd, MASKREAD_OFF, 31);
      vshuffle(s_temp, S_Indices, S_BaseSeq, SHUFFLE_GATHER, 31);
      d_odd = vmul(s_temp, c_temp, MASKREAD_OFF, 31);
      d_odd = vmul(d_odd, z_temp, MASKREAD_OFF, 31);

      sssEst_real_vec = vmul(SSS_Real_odd, d_odd, MASKREAD_OFF, 31);
      sssEst_imag_vec = vmul(SSS_Imag_odd, d_odd, MASKREAD_OFF, 31);

      vbrdcst(sssEst_mean_real, 0, MASKREAD_OFF, 127);
      vbrdcst(sssEst_mean_imag, 0, MASKREAD_OFF, 127);
      sssEst_mean_real = vredsum(sssEst_real_vec, MASKREAD_OFF, 31);
      sssEst_mean_imag = vredsum(sssEst_imag_vec, MASKREAD_OFF, 31);
      int32_t sssEst_tmp = 0;
      int sssEst_mean_real_addr = vaddr(sssEst_mean_real);
      vbarrier();
      VSPM_OPEN();
      sssEst_tmp = ((*(volatile char *)(sssEst_mean_real_addr)) & 0xFF);
      VSPM_CLOSE();
      sssEst_mean_real_addr = vaddr(sssEst_mean_real);
      vbarrier();
      VSPM_OPEN();
      sssEst_tmp += (((*(volatile char *)(sssEst_mean_real_addr + 1)) & 0xFF) << 8);
      VSPM_CLOSE();
      sssEst_mean_real_addr = vaddr(sssEst_mean_real);
      vbarrier();
      VSPM_OPEN();
      sssEst_tmp += (((*(volatile char *)(sssEst_mean_real_addr + 2)) & 0xFF) << 16);
      VSPM_CLOSE();
      sssEst_mean_real_addr = vaddr(sssEst_mean_real);
      vbarrier();
      VSPM_OPEN();
      sssEst_tmp += (((*(volatile char *)(sssEst_mean_real_addr + 3)) & 0xFF) << 24);
      VSPM_CLOSE();

      if (sssEst_tmp == INT32_MIN) {
        sssEstOdd[mOdd] = INT32_MAX;
      } else {
        sssEstOdd[mOdd] = sssEst_tmp > 0 ? sssEst_tmp : -sssEst_tmp;
      }

      int sssEst_mean_imag_addr = vaddr(sssEst_mean_imag);
      vbarrier();
      VSPM_OPEN();
      sssEst_tmp = ((*(volatile char *)(sssEst_mean_imag_addr)) & 0xFF);
      VSPM_CLOSE();
      sssEst_mean_imag_addr = vaddr(sssEst_mean_imag);
      vbarrier();
      VSPM_OPEN();
      sssEst_tmp += (((*(volatile char *)(sssEst_mean_imag_addr + 1)) & 0xFF) << 8);
      VSPM_CLOSE();
      sssEst_mean_imag_addr = vaddr(sssEst_mean_imag);
      vbarrier();
      VSPM_OPEN();
      sssEst_tmp += (((*(volatile char *)(sssEst_mean_imag_addr + 2)) & 0xFF) << 16);
      VSPM_CLOSE();
      sssEst_mean_imag_addr = vaddr(sssEst_mean_imag);
      vbarrier();
      VSPM_OPEN();
      sssEst_tmp += (((*(volatile char *)(sssEst_mean_imag_addr + 3)) & 0xFF) << 24);
      VSPM_CLOSE();
      // printf("sssEst:%hd\n", &sssEst_tmp);
      if (sssEst_tmp == INT32_MIN) {
        sssEstOdd[mOdd] += INT32_MAX;
      } else {
        sssEstOdd[mOdd] += sssEst_tmp > 0 ? sssEst_tmp : -sssEst_tmp;
      }
      // printf("sssEst:%d\n", &sssEst[mEven]);

      maxMOddIndex = sssEstOdd[mOdd] > maxMOdd ? mOdd : maxMOddIndex;
      maxMOdd = sssEstOdd[mOdd] > maxMOdd ? sssEstOdd[mOdd] : maxMOdd;
    }
  }

  for (short mOdd = maxMEvenIndex - 7; mOdd < maxMEvenIndex; mOdd++) {
    /* code */
    if (mOdd >= 0) {
      S_Indices = vadd(S_index, mOdd, MASKREAD_OFF, 31);
      vshuffle(s_temp, S_Indices, S_BaseSeq, SHUFFLE_GATHER, 31);
      d_odd = vmul(s_temp, c_temp, MASKREAD_OFF, 31);
      d_odd = vmul(d_odd, z_temp, MASKREAD_OFF, 31);

      sssEst_real_vec = vmul(SSS_Real_odd, d_odd, MASKREAD_OFF, 31);
      sssEst_imag_vec = vmul(SSS_Imag_odd, d_odd, MASKREAD_OFF, 31);

      vbrdcst(sssEst_mean_real, 0, MASKREAD_OFF, 127);
      vbrdcst(sssEst_mean_imag, 0, MASKREAD_OFF, 127);
      sssEst_mean_real = vredsum(sssEst_real_vec, MASKREAD_OFF, 31);
      sssEst_mean_imag = vredsum(sssEst_imag_vec, MASKREAD_OFF, 31);
      int32_t sssEst_tmp = 0;
      int sssEst_mean_real_addr = vaddr(sssEst_mean_real);
      vbarrier();
      VSPM_OPEN();
      sssEst_tmp = ((*(volatile char *)(sssEst_mean_real_addr)) & 0xFF);
      VSPM_CLOSE();
      sssEst_mean_real_addr = vaddr(sssEst_mean_real);
      vbarrier();
      VSPM_OPEN();
      sssEst_tmp += (((*(volatile char *)(sssEst_mean_real_addr + 1)) & 0xFF) << 8);
      VSPM_CLOSE();
      sssEst_mean_real_addr = vaddr(sssEst_mean_real);
      vbarrier();
      VSPM_OPEN();
      sssEst_tmp += (((*(volatile char *)(sssEst_mean_real_addr + 2)) & 0xFF) << 16);
      VSPM_CLOSE();
      sssEst_mean_real_addr = vaddr(sssEst_mean_real);
      vbarrier();
      VSPM_OPEN();
      sssEst_tmp += (((*(volatile char *)(sssEst_mean_real_addr + 3)) & 0xFF) << 24);
      VSPM_CLOSE();

      if (sssEst_tmp == INT32_MIN) {
        sssEstOdd[mOdd] = INT32_MAX;
      } else {
        sssEstOdd[mOdd] = sssEst_tmp > 0 ? sssEst_tmp : -sssEst_tmp;
      }

      int sssEst_mean_imag_addr = vaddr(sssEst_mean_imag);
      vbarrier();
      VSPM_OPEN();
      sssEst_tmp = ((*(volatile char *)(sssEst_mean_imag_addr)) & 0xFF);
      VSPM_CLOSE();
      sssEst_mean_imag_addr = vaddr(sssEst_mean_imag);
      vbarrier();
      VSPM_OPEN();
      sssEst_tmp += (((*(volatile char *)(sssEst_mean_imag_addr + 1)) & 0xFF) << 8);
      VSPM_CLOSE();
      sssEst_mean_imag_addr = vaddr(sssEst_mean_imag);
      vbarrier();
      VSPM_OPEN();
      sssEst_tmp += (((*(volatile char *)(sssEst_mean_imag_addr + 2)) & 0xFF) << 16);
      VSPM_CLOSE();
      sssEst_mean_imag_addr = vaddr(sssEst_mean_imag);
      vbarrier();
      VSPM_OPEN();
      sssEst_tmp += (((*(volatile char *)(sssEst_mean_imag_addr + 3)) & 0xFF) << 24);
      VSPM_CLOSE();
      // printf("sssEst:%hd\n", &sssEst_tmp);
      if (sssEst_tmp == INT32_MIN) {
        sssEstOdd[mOdd] += INT32_MAX;
      } else {
        sssEstOdd[mOdd] += sssEst_tmp > 0 ? sssEst_tmp : -sssEst_tmp;
      }

      maxMOddIndex = sssEstOdd[mOdd] > maxMOdd ? mOdd : maxMOddIndex;
      maxMOdd = sssEstOdd[mOdd] > maxMOdd ? sssEstOdd[mOdd] : maxMOdd;
    }
  }

  int m0;
  int m1;

  int subframe_num = 0;
  if (maxMEvenIndex >= maxMOddIndex) {
    m0 = maxMOddIndex;
    m1 = maxMEvenIndex;
    subframe_num = 0;
  } else {
    m0 = maxMEvenIndex;
    m1 = maxMOddIndex;
    subframe_num = 5;
  }
  int del_m = m1 - m0;
  int NID_1;
  if (del_m == 1)
    NID_1 = m0;
  else if (del_m == 2)
    NID_1 = m0 + 30;
  else if (del_m == 3)
    NID_1 = m0 + 59;
  else if (del_m == 4)
    NID_1 = m0 + 87;
  else if (del_m == 5)
    NID_1 = m0 + 114;
  else if (del_m == 6)
    NID_1 = m0 + 140;
  else if (del_m == 7)
    NID_1 = m0 + 165;
  else
    NID_1 = 444;

  short_struct nCELLID;
  nCELLID.data = 3 * NID_1 + NID2;
  short_struct subFrameNum;
  subFrameNum.data = subframe_num;
  // printf("subFrameNum:%d\n", &subFrameNum.data);
  // printf("NCELLID:%d\n", &nCELLID.data);
  vreturn(&nCELLID, sizeof(nCELLID), &subFrameNum, sizeof(subFrameNum));
}