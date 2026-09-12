/**
 * ****************************************
 * @file        SSS_Search.c
 * @brief       SSS search
 * @author      yuanfeng
 * @date        2024.4.24
 * @copyright   ACE-Lab(Shanghai University)
 * ****************************************
 */
#include <stdint.h>
#include <stdlib.h>

#include "riscv_printf.h"
#include "venus.h"

#define SSS_LENGTH 127

typedef short __v2048i16 __attribute__((ext_vector_type(2048)));
typedef short __v4096i16 __attribute__((ext_vector_type(4096)));
typedef char  __v4096i8 __attribute__((ext_vector_type(4096)));

typedef struct {
  short data;
} __attribute__((aligned(64))) short_struct;

int64_t sssEst[336] = {0};

int Task_SSS_Search(__v4096i8 d_SSS0_vec, __v4096i8 d_SSS1_vec, __v4096i8 rxData_real0, __v4096i8 rxData_imag0,
                    __v2048i16 SSS_Indices, __v2048i16 rxData_shuffle_index, short_struct in_NID2)
    __attribute__((aligned(64))) {
  int fractionLength = 2;
  // input
  int NID2        = in_NID2.data;
  int maxSSS      = 0;
  int maxSSSIndex = 0;

  /************ ExtractResource ************ */
  __v4096i8 rxData_real;
  __v4096i8 rxData_imag;
  vclaim(rxData_real);
  vclaim(rxData_imag);
  vshuffle(rxData_real, rxData_shuffle_index, rxData_real0, SHUFFLE_GATHER, 720);
  vshuffle(rxData_imag, rxData_shuffle_index, rxData_imag0, SHUFFLE_GATHER, 720);

  __v4096i8 sssRx_real_vec;
  __v4096i8 sssRx_imag_vec;
  vclaim(sssRx_real_vec);
  vclaim(sssRx_imag_vec);
  vshuffle(sssRx_real_vec, SSS_Indices, rxData_real, SHUFFLE_GATHER, SSS_LENGTH);
  vshuffle(sssRx_imag_vec, SSS_Indices, rxData_imag, SHUFFLE_GATHER, SSS_LENGTH);

  __v2048i16 scalar_value;
  vclaim(scalar_value);
  vbrdcst(scalar_value, SSS_LENGTH, MASKREAD_OFF, SSS_LENGTH);
  for (int NID1 = 0; NID1 <= 335; ++NID1) {
    // generate sss sequence
    int        m0 = 15 * (NID1 / 112) + 5 * NID2;
    int        m1 = NID1 % 112;
    __v2048i16 sssSeqIndex0;
    vclaim(sssSeqIndex0);
    vrange(sssSeqIndex0, SSS_LENGTH);
    sssSeqIndex0 = vsadd(sssSeqIndex0, m0, MASKREAD_OFF, SSS_LENGTH);
    sssSeqIndex0 = vrem(scalar_value, sssSeqIndex0, MASKREAD_OFF, SSS_LENGTH);
    __v4096i8 sssSeq0;
    vclaim(sssSeq0);
    vshuffle(sssSeq0, sssSeqIndex0, d_SSS0_vec, SHUFFLE_GATHER, SSS_LENGTH);
    __v2048i16 sssSeqIndex1;
    vclaim(sssSeqIndex1);
    vrange(sssSeqIndex1, SSS_LENGTH);
    sssSeqIndex1 = vsadd(sssSeqIndex1, m1, MASKREAD_OFF, SSS_LENGTH);
    sssSeqIndex1 = vrem(scalar_value, sssSeqIndex1, MASKREAD_OFF, SSS_LENGTH);
    __v4096i8 sssSeq1;
    vclaim(sssSeq1);
    vshuffle(sssSeq1, sssSeqIndex1, d_SSS1_vec, SHUFFLE_GATHER, SSS_LENGTH);
    __v4096i8 sssSeq;
    sssSeq = vmul(sssSeq0, sssSeq1, MASKREAD_OFF, SSS_LENGTH);
    // VENUS_PRINTVEC_SHORT(sssSeq, SSS_LENGTH);

    /* sss互相关 */
    __v4096i8 sssEst_real_vec;
    __v4096i8 sssEst_imag_vec;

    sssEst_real_vec = vmul(sssRx_real_vec, sssSeq, MASKREAD_OFF, SSS_LENGTH);
    sssEst_imag_vec = vmul(sssRx_imag_vec, sssSeq, MASKREAD_OFF, SSS_LENGTH);
    __v4096i8 sssEst_mean_real;
    __v4096i8 sssEst_mean_imag;
    vclaim(sssEst_mean_real);
    vclaim(sssEst_mean_imag);
    vbrdcst(sssEst_mean_real, 0, MASKREAD_OFF, SSS_LENGTH);
    vbrdcst(sssEst_mean_imag, 0, MASKREAD_OFF, SSS_LENGTH);
    sssEst_mean_real              = vredsum(sssEst_real_vec, MASKREAD_OFF, SSS_LENGTH);
    sssEst_mean_imag              = vredsum(sssEst_imag_vec, MASKREAD_OFF, SSS_LENGTH);
    int32_t sssEst_tmp            = 0;
    int     sssEst_mean_real_addr = vaddr(sssEst_mean_real);
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
      sssEst[NID1] = INT32_MAX;
    } else {
      sssEst[NID1] = sssEst_tmp > 0 ? sssEst_tmp : -sssEst_tmp;
    }
    // printf("sssEst:%hd\n", &sssEst_tmp);
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
      sssEst[NID1] += INT32_MAX;
    } else {
      sssEst[NID1] += sssEst_tmp > 0 ? sssEst_tmp : -sssEst_tmp;
    }
    maxSSSIndex = sssEst[NID1] > maxSSS ? NID1 : maxSSSIndex;
    maxSSS      = sssEst[NID1] > maxSSS ? sssEst[NID1] : maxSSS;
    // printf("sssEst:%d\n", &sssEst[NID1]);
  }

  short NID1 = maxSSSIndex;

  short_struct ncellid;
  ncellid.data = 3 * NID1 + NID2;
  vreturn(&ncellid, sizeof(ncellid));
}