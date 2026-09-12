/**
 * ****************************************
 * @file        Task_nrPBCHBitProcess.c
 * @brief       PBCH bit process
 * @author      yuanfeng
 * @date        2024.6.26
 * @copyright   ACE-Lab(Shanghai University)
 * ****************************************
 */

#include "riscv_printf.h"
#include "venus.h"

typedef short __v2048i16 __attribute__((ext_vector_type(2048)));
typedef char  __v4096i8 __attribute__((ext_vector_type(4096)));

#define INF 0x7F
// input data for test

//---------------------------CRC------------------------------------//
enum crcType { CRC24A, CRC24B, CRC24C, CRC16, CRC11, CRC6 };

// uint8_t POLY_CRC24A[25] = {1, 1, 0, 0, 0, 0, 1, 1, 0, 0, 1, 0, 0,
//                            1, 1, 0, 0, 1, 1, 1, 1, 1, 0, 1, 1};
// uint8_t POLY_CRC24B[25] = {1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
//                            0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 1, 1};
// uint8_t POLY_CRC24C[25] = {1, 1, 0, 1, 1, 0, 0, 1, 0, 1, 0, 1, 1,
//                            0, 0, 0, 1, 0, 0, 0, 1, 0, 1, 1, 1};
// uint8_t POLY_CRC16[17] = {1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
// uint8_t POLY_CRC11[12] = {1, 1, 1, 0, 0, 0, 1, 0, 0, 0, 0, 1};
// uint8_t POLY_CRC6[7] = {1, 1, 0, 0, 0, 0, 1};
//---------------------------------------------------------------//

// 输入定点化的点数，实现定点乘法的输出
VENUS_INLINE __v4096i8 MUL4096_8_FIXED(__v4096i8 a, __v4096i8 b, int fix_point, int length) {
  __v4096i8 result;
  vsetshamt(fix_point);
  result = vmul(a, b, MASKREAD_OFF, length);
  vsetshamt(0);
  return result;
  // __v4096i8 high8;
  // __v4096i8 low8;
  // __v4096i8 result;
  // short     high_shift = 8 - fix_point;

  // low8   = vmul(a, b, MASKREAD_OFF, length);
  // high8  = vmulh(a, b, MASKREAD_OFF, length);
  // low8   = vsrl(low8, fix_point, MASKREAD_OFF, length);
  // high8  = vsll(high8, high_shift, MASKREAD_OFF, length);
  // result = vor(low8, high8, MASKREAD_OFF, length);
  // return result;
};

VENUS_INLINE __v4096i8 raterecover_polar_downlink(__v4096i8 vin, uint16_t E, uint16_t K, uint16_t N, uint16_t IBIL,
                                                  __v2048i16 pi) {
  __v4096i8  vout;
  __v2048i16 index;
  // __v2048i16 pi;
  __v2048i16 jn;
  __v2048i16 ii;
  __v2048i16 n;

  vclaim(vout);
  vclaim(index);
  // vclaim(pi);
  vclaim(jn);
  vclaim(ii);
  vclaim(n);

  uint16_t N_32    = (N >> 5);
  uint16_t logN_32 = 0;

  for (int i = N_32; i > 1; i >>= 1)
    logN_32++;

  // int pi_addr = vaddr(pi);
  // vbarrier();
  // VSPM_OPEN();
  // for (size_t i = 0; i < 32; i++) {
  //   unsigned int addr = pi_addr + (i << 1);
  //   *(volatile unsigned short *)(addr) = pi_[i];
  // }
  // VSPM_CLOSE();

  if (E >= N) {
    vout = vsadd(vin, 0, MASKREAD_OFF, N);
  } else {
    if ((K * 64 / E) <= 28) {
      vrange(index, E);
      index = vsadd(index, N - E, MASKREAD_OFF, E);
      vshuffle(vout, index, vin, SHUFFLE_SCATTER, E);
    } else {
      vbrdcst(vout, INF, MASKREAD_OFF, N);
      vout = vsadd(vin, 0, MASKREAD_OFF, E);
    }
  }

  vbrdcst(jn, 0, MASKREAD_OFF, N);

  vrange(ii, N);
  ii = vsll(ii, 5, MASKREAD_OFF, N);
  ii = vsrl(ii, 9, MASKREAD_OFF, N);

  vshuffle(jn, ii, pi, SHUFFLE_GATHER, N);
  jn = vmul(jn, N_32, MASKREAD_OFF, N);

  vrange(ii, N);
  ii = vsrl(ii, logN_32, MASKREAD_OFF, N);
  ii = vsll(ii, logN_32, MASKREAD_OFF, N);
  vrange(n, N);
  n  = vrsub(n, ii, MASKREAD_OFF, N);
  jn = vsadd(jn, n, MASKREAD_OFF, N);

  __v4096i8 vout1;
  vclaim(vout1);
  vshuffle(vout1, jn, vout, SHUFFLE_SCATTER, N);

  return vout1;
}

VENUS_INLINE __v4096i8 polar_512bits(__v4096i8 vin, uint16_t K, uint16_t E, uint16_t RNTI, uint16_t iternumber,
                                     __v2048i16 Xor_index_up, __v2048i16 Xor_index_down, __v2048i16 Equal_index_up,
                                     __v2048i16 Equal_index_down, __v4096i8 Mask_1010, __v4096i8 Mask_0101,
                                     __v2048i16 index_front_odd, __v2048i16 index_back_odd, __v2048i16 index_front_even,
                                     __v2048i16 index_back_even, __v4096i8 R_Matrix_0_up, __v4096i8 R_Matrix_0_down,
                                     __v2048i16 PBCH_bit_Index, __v2048i16 iIL_index) {
  short Polar_Length = 512;
  // __v2048i16 Xor_index_up;
  // __v2048i16 Xor_index_down;
  // __v2048i16 Equal_index_up;
  // __v2048i16 Equal_index_down;
  // vclaim(Xor_index_up);
  // vclaim(Xor_index_down);
  // vclaim(Equal_index_up);
  // vclaim(Equal_index_down);

  // vbarrier();
  // VSPM_OPEN();
  // int Xor_index_up_addr = vaddr(Xor_index_up);
  // for (int i = 0; i < 256; i++) {
  //   *(volatile unsigned short *)(Xor_index_up_addr + (i << 1)) =
  //       Xor_up_index[i];
  // }
  // int Xor_index_down_addr = vaddr(Xor_index_down);
  // for (int i = 0; i < 256; i++) {
  //   *(volatile unsigned short *)(Xor_index_down_addr + (i << 1)) =
  //       Xor_down_index[i];
  // }
  // int Equal_index_up_addr = vaddr(Equal_index_up);
  // for (int i = 0; i < 256; i++) {
  //   *(volatile unsigned short *)(Equal_index_up_addr + (i << 1)) =
  //       Equal_up_index[i];
  // }
  // int Equal_index_down_addr = vaddr(Equal_index_down);
  // for (int i = 0; i < 256; i++) {
  //   *(volatile unsigned short *)(Equal_index_down_addr + (i << 1)) =
  //       Equal_down_index[i];
  // }
  // VSPM_CLOSE();

  // __v4096i8 Mask_1010;
  // __v4096i8 Mask_0101;
  // vclaim(Mask_1010);
  // vclaim(Mask_0101);

  // vbarrier();
  // VSPM_OPEN();
  // int Mask_1010_addr = vaddr(Mask_1010);
  // for (int i = 0; i < 256; i++) {
  //   *(volatile unsigned char *)(Mask_1010_addr + i) = Mask_1010_table[i];
  // }
  // int Mask_0101_addr = vaddr(Mask_0101);
  // for (int i = 0; i < 256; i++) {
  //   *(volatile unsigned char *)(Mask_0101_addr + i) = Mask_0101_table[i];
  // }
  // VSPM_CLOSE();

  // __v2048i16 index_front_odd;
  // __v2048i16 index_back_odd;
  // __v2048i16 index_front_even;
  // __v2048i16 index_back_even;
  // vclaim(index_front_odd);
  // vclaim(index_back_odd);
  // vclaim(index_front_even);
  // vclaim(index_back_even);

  // vbarrier();
  // VSPM_OPEN();
  // int index_front_odd_addr = vaddr(index_front_odd);
  // for (int i = 0; i < 256; i++) {
  //   *(volatile unsigned short *)(index_front_odd_addr + (i << 1)) =
  //       index_front_135[i];
  // }
  // int index_back_odd_addr = vaddr(index_back_odd);
  // for (int i = 0; i < 256; i++) {
  //   *(volatile unsigned short *)(index_back_odd_addr + (i << 1)) =
  //       index_back_135[i];
  // }
  // int index_front_even_addr = vaddr(index_front_even);
  // for (int i = 0; i < 256; i++) {
  //   *(volatile unsigned short *)(index_front_even_addr + (i << 1)) =
  //       index_front_024[i];
  // }
  // int index_back_even_addr = vaddr(index_back_even);
  // for (int i = 0; i < 256; i++) {
  //   *(volatile unsigned short *)(index_back_even_addr + (i << 1)) =
  //       index_back_024[i];
  // }
  // VSPM_CLOSE();

  __v4096i8 Mask_000111;
  __v4096i8 Mask_111000;
  vclaim(Mask_000111);
  vclaim(Mask_111000);
  vbrdcst(Mask_000111, 1, MASKREAD_OFF, 256);
  vbrdcst(Mask_000111, 0, MASKREAD_OFF, 128);
  Mask_000111 = vsadd(Mask_000111, 0, MASKREAD_OFF, 256); // 冗余
  vbrdcst(Mask_111000, 0, MASKREAD_OFF, 256);
  vbrdcst(Mask_111000, 1, MASKREAD_OFF, 128);
  Mask_111000 = vsadd(Mask_111000, 0, MASKREAD_OFF, 256); // 冗余

  __v4096i8 L_Matrix_N_up;
  __v4096i8 L_Matrix_N_down;
  vclaim(L_Matrix_N_up);
  vclaim(L_Matrix_N_down);

  // VSPM_OPEN();
  // vbarrier();
  // int L_Matrix_N_up_addr = vaddr(L_Matrix_N_up);
  // for (int i = 0; i < 256; i++)
  // {
  //     *(volatile unsigned char *)(L_Matrix_N_up_addr + i) = input_llr[i];
  // }
  // int L_Matrix_N_down_addr = vaddr(L_Matrix_N_down);
  // for (int i = 0; i < 256; i++)
  // {
  //     *(volatile unsigned char *)(L_Matrix_N_down_addr + i) = input_llr[256 +
  //     i];
  // }
  // VSPM_CLOSE();
  __v2048i16 index_input;
  vrange(index_input, 256);
  vshuffle(L_Matrix_N_up, index_input, vin, SHUFFLE_GATHER, 256);
  index_input = vsadd(index_input, 256, MASKREAD_OFF, 256);
  vshuffle(L_Matrix_N_down, index_input, vin, SHUFFLE_GATHER, 256);

  // __v4096i8 R_Matrix_0_up;
  // __v4096i8 R_Matrix_0_down;
  // vclaim(R_Matrix_0_up);
  // vclaim(R_Matrix_0_down);

  // vbarrier();
  // VSPM_OPEN();
  // int R_Matrix_0_up_addr = vaddr(R_Matrix_0_up);
  // for (int i = 0; i < 256; i++) {
  //   *(volatile unsigned char *)(R_Matrix_0_up_addr + i) = R_Matrix_0[i];
  // }
  // int R_Matrix_0_down_addr = vaddr(R_Matrix_0_down);
  // for (int i = 0; i < 256; i++) {
  //   *(volatile unsigned char *)(R_Matrix_0_down_addr + i) = R_Matrix_0[256 +
  //   i];
  // }
  // VSPM_CLOSE();

  __v4096i8 alpha;
  vclaim(alpha);
  vbrdcst(alpha, 60, MASKREAD_OFF, 256);
  alpha = vsadd(alpha, 0, MASKREAD_OFF, 256); // 冗余

  // 生成R矩阵
  __v4096i8 R_Matrix_1_up;
  __v4096i8 R_Matrix_1_down;
  vclaim(R_Matrix_1_up);
  vclaim(R_Matrix_1_down);
  __v4096i8 R_Matrix_2_up;
  __v4096i8 R_Matrix_2_down;
  vclaim(R_Matrix_2_up);
  vclaim(R_Matrix_2_down);
  __v4096i8 R_Matrix_3_up;
  __v4096i8 R_Matrix_3_down;
  vclaim(R_Matrix_3_up);
  vclaim(R_Matrix_3_down);
  __v4096i8 R_Matrix_4_up;
  __v4096i8 R_Matrix_4_down;
  vclaim(R_Matrix_4_up);
  vclaim(R_Matrix_4_down);
  __v4096i8 R_Matrix_5_up;
  __v4096i8 R_Matrix_5_down;
  vclaim(R_Matrix_5_up);
  vclaim(R_Matrix_5_down);
  __v4096i8 R_Matrix_6_up;
  __v4096i8 R_Matrix_6_down;
  vclaim(R_Matrix_6_up);
  vclaim(R_Matrix_6_down);
  __v4096i8 R_Matrix_7_up;
  __v4096i8 R_Matrix_7_down;
  vclaim(R_Matrix_7_up);
  vclaim(R_Matrix_7_down);
  __v4096i8 R_Matrix_8_up;
  __v4096i8 R_Matrix_8_down;
  vclaim(R_Matrix_8_up);
  vclaim(R_Matrix_8_down);
  __v4096i8 R_Matrix_9_up;
  __v4096i8 R_Matrix_9_down;
  vclaim(R_Matrix_9_up);
  vclaim(R_Matrix_9_down);

  //  冗余
  R_Matrix_9_up   = vmul(alpha, 0, MASKREAD_OFF, 256);
  R_Matrix_9_down = vmul(alpha, 0, MASKREAD_OFF, 256);
  R_Matrix_8_up   = vmul(alpha, 0, MASKREAD_OFF, 256);
  R_Matrix_8_down = vmul(alpha, 0, MASKREAD_OFF, 256);
  R_Matrix_7_up   = vmul(alpha, 0, MASKREAD_OFF, 256);
  R_Matrix_7_down = vmul(alpha, 0, MASKREAD_OFF, 256);
  R_Matrix_6_up   = vmul(alpha, 0, MASKREAD_OFF, 256);
  R_Matrix_6_down = vmul(alpha, 0, MASKREAD_OFF, 256);
  R_Matrix_5_up   = vmul(alpha, 0, MASKREAD_OFF, 256);
  R_Matrix_5_down = vmul(alpha, 0, MASKREAD_OFF, 256);
  R_Matrix_4_up   = vmul(alpha, 0, MASKREAD_OFF, 256);
  R_Matrix_4_down = vmul(alpha, 0, MASKREAD_OFF, 256);
  R_Matrix_3_up   = vmul(alpha, 0, MASKREAD_OFF, 256);
  R_Matrix_3_down = vmul(alpha, 0, MASKREAD_OFF, 256);
  R_Matrix_2_up   = vmul(alpha, 0, MASKREAD_OFF, 256);
  R_Matrix_2_down = vmul(alpha, 0, MASKREAD_OFF, 256);
  R_Matrix_1_up   = vmul(alpha, 0, MASKREAD_OFF, 256);
  R_Matrix_1_down = vmul(alpha, 0, MASKREAD_OFF, 256);

  // 生成L矩阵
  __v4096i8 L_Matrix_0_up;
  __v4096i8 L_Matrix_0_down;
  vclaim(L_Matrix_0_up);
  vclaim(L_Matrix_0_down);
  __v4096i8 L_Matrix_1_up;
  __v4096i8 L_Matrix_1_down;
  vclaim(L_Matrix_1_up);
  vclaim(L_Matrix_1_down);
  __v4096i8 L_Matrix_2_up;
  __v4096i8 L_Matrix_2_down;
  vclaim(L_Matrix_2_up);
  vclaim(L_Matrix_2_down);
  __v4096i8 L_Matrix_3_up;
  __v4096i8 L_Matrix_3_down;
  vclaim(L_Matrix_3_up);
  vclaim(L_Matrix_3_down);
  __v4096i8 L_Matrix_4_up;
  __v4096i8 L_Matrix_4_down;
  vclaim(L_Matrix_4_up);
  vclaim(L_Matrix_4_down);
  __v4096i8 L_Matrix_5_up;
  __v4096i8 L_Matrix_5_down;
  vclaim(L_Matrix_5_up);
  vclaim(L_Matrix_5_down);
  __v4096i8 L_Matrix_6_up;
  __v4096i8 L_Matrix_6_down;
  vclaim(L_Matrix_6_up);
  vclaim(L_Matrix_6_down);
  __v4096i8 L_Matrix_7_up;
  __v4096i8 L_Matrix_7_down;
  vclaim(L_Matrix_7_up);
  vclaim(L_Matrix_7_down);
  __v4096i8 L_Matrix_8_up;
  __v4096i8 L_Matrix_8_down;
  vclaim(L_Matrix_8_up);
  vclaim(L_Matrix_8_down);

  //  冗余
  L_Matrix_8_up   = vmul(alpha, 0, MASKREAD_OFF, 256);
  L_Matrix_8_down = vmul(alpha, 0, MASKREAD_OFF, 256);
  L_Matrix_7_up   = vmul(alpha, 0, MASKREAD_OFF, 256);
  L_Matrix_7_down = vmul(alpha, 0, MASKREAD_OFF, 256);
  L_Matrix_6_up   = vmul(alpha, 0, MASKREAD_OFF, 256);
  L_Matrix_6_down = vmul(alpha, 0, MASKREAD_OFF, 256);
  L_Matrix_5_up   = vmul(alpha, 0, MASKREAD_OFF, 256);
  L_Matrix_5_down = vmul(alpha, 0, MASKREAD_OFF, 256);
  L_Matrix_4_up   = vmul(alpha, 0, MASKREAD_OFF, 256);
  L_Matrix_4_down = vmul(alpha, 0, MASKREAD_OFF, 256);
  L_Matrix_3_up   = vmul(alpha, 0, MASKREAD_OFF, 256);
  L_Matrix_3_down = vmul(alpha, 0, MASKREAD_OFF, 256);
  L_Matrix_2_up   = vmul(alpha, 0, MASKREAD_OFF, 256);
  L_Matrix_2_down = vmul(alpha, 0, MASKREAD_OFF, 256);
  L_Matrix_1_up   = vmul(alpha, 0, MASKREAD_OFF, 256);
  L_Matrix_1_down = vmul(alpha, 0, MASKREAD_OFF, 256);
  L_Matrix_0_up   = vmul(alpha, 0, MASKREAD_OFF, 256);
  L_Matrix_0_down = vmul(alpha, 0, MASKREAD_OFF, 256);

  __v2048i16 copy_index;
  vclaim(copy_index);
  vrange(copy_index, 256);

  // __v4096i8 b_add_d;
  // vclaim(b_add_d);

  __v4096i8 temp_a;
  __v4096i8 temp_b;
  __v4096i8 temp_c;
  __v4096i8 temp_d;
  vclaim(temp_a);
  vclaim(temp_b);
  vclaim(temp_c);
  vclaim(temp_d);
  __v4096i8 result_up_temp;
  vclaim(result_up_temp);
  __v4096i8 result_up_temp_a;
  vclaim(result_up_temp_a);
  __v4096i8 min_bd_c;
  vclaim(min_bd_c);
  __v4096i8 temp;
  vclaim(temp);
  __v4096i8 temp1;
  vclaim(temp1);
  //  迭代Polar译码
  for (int iter = 0; iter < iternumber; iter++) {
    //  L Matrix Calculation
    for (int column = 9; column > 0; column--) {
      __v4096i8 indata_up_a;
      __v4096i8 indata_up_b;
      __v4096i8 indata_down_a;
      __v4096i8 indata_down_b;
      indata_up_a   = vmul(alpha, 0, MASKREAD_OFF, 256);
      indata_up_b   = vmul(alpha, 0, MASKREAD_OFF, 256);
      indata_down_a = vmul(alpha, 0, MASKREAD_OFF, 256);
      indata_down_b = vmul(alpha, 0, MASKREAD_OFF, 256);

      if (column == 9) {
        vshuffle(indata_up_a, index_front_even, L_Matrix_N_up, SHUFFLE_GATHER,
                 256); // even = {0,2,4,6,...}
        vshuffle(indata_up_b, index_back_even, L_Matrix_N_down, SHUFFLE_GATHER, 256);
        vshuffle(indata_down_a, index_front_odd, L_Matrix_N_up, SHUFFLE_GATHER, 256);
        vshuffle(indata_down_b, index_back_odd, L_Matrix_N_down, SHUFFLE_GATHER, 256);
        indata_up_a   = vmul(indata_up_a, Mask_111000, MASKREAD_OFF, 256);
        indata_up_b   = vmul(indata_up_b, Mask_000111, MASKREAD_OFF, 256);
        indata_down_a = vmul(indata_down_a, Mask_111000, MASKREAD_OFF, 256);
        indata_down_b = vmul(indata_down_b, Mask_000111, MASKREAD_OFF, 256);

        temp_a = vsadd(indata_up_a, indata_up_b, MASKREAD_OFF, 256);
        temp_b = vsadd(indata_down_a, indata_down_b, MASKREAD_OFF, 256);
        vshuffle(temp_c, copy_index, R_Matrix_8_up, SHUFFLE_GATHER, 256);
        vshuffle(temp_d, copy_index, R_Matrix_8_down, SHUFFLE_GATHER, 256);
      } else if (column == 8) {
        vshuffle(indata_up_a, index_front_even, L_Matrix_8_up, SHUFFLE_GATHER,
                 256); // even = {0,2,4,6,...}
        vshuffle(indata_up_b, index_back_even, L_Matrix_8_down, SHUFFLE_GATHER, 256);
        vshuffle(indata_down_a, index_front_odd, L_Matrix_8_up, SHUFFLE_GATHER, 256);
        vshuffle(indata_down_b, index_back_odd, L_Matrix_8_down, SHUFFLE_GATHER, 256);
        indata_up_a   = vmul(indata_up_a, Mask_111000, MASKREAD_OFF, 256);
        indata_up_b   = vmul(indata_up_b, Mask_000111, MASKREAD_OFF, 256);
        indata_down_a = vmul(indata_down_a, Mask_111000, MASKREAD_OFF, 256);
        indata_down_b = vmul(indata_down_b, Mask_000111, MASKREAD_OFF, 256);

        temp_a = vsadd(indata_up_a, indata_up_b, MASKREAD_OFF, 256);
        temp_b = vsadd(indata_down_a, indata_down_b, MASKREAD_OFF, 256);
        vshuffle(temp_c, copy_index, R_Matrix_7_up, SHUFFLE_GATHER, 256);
        vshuffle(temp_d, copy_index, R_Matrix_7_down, SHUFFLE_GATHER, 256);
      } else if (column == 7) {
        vshuffle(indata_up_a, index_front_even, L_Matrix_7_up, SHUFFLE_GATHER,
                 256); // even = {0,2,4,6,...}
        vshuffle(indata_up_b, index_back_even, L_Matrix_7_down, SHUFFLE_GATHER, 256);
        vshuffle(indata_down_a, index_front_odd, L_Matrix_7_up, SHUFFLE_GATHER, 256);
        vshuffle(indata_down_b, index_back_odd, L_Matrix_7_down, SHUFFLE_GATHER, 256);
        indata_up_a   = vmul(indata_up_a, Mask_111000, MASKREAD_OFF, 256);
        indata_up_b   = vmul(indata_up_b, Mask_000111, MASKREAD_OFF, 256);
        indata_down_a = vmul(indata_down_a, Mask_111000, MASKREAD_OFF, 256);
        indata_down_b = vmul(indata_down_b, Mask_000111, MASKREAD_OFF, 256);

        temp_a = vsadd(indata_up_a, indata_up_b, MASKREAD_OFF, 256);
        temp_b = vsadd(indata_down_a, indata_down_b, MASKREAD_OFF, 256);
        vshuffle(temp_c, copy_index, R_Matrix_6_up, SHUFFLE_GATHER, 256);
        vshuffle(temp_d, copy_index, R_Matrix_6_down, SHUFFLE_GATHER, 256);
      } else if (column == 6) {
        vshuffle(indata_up_a, index_front_even, L_Matrix_6_up, SHUFFLE_GATHER,
                 256); // even = {0,2,4,6,...}
        vshuffle(indata_up_b, index_back_even, L_Matrix_6_down, SHUFFLE_GATHER, 256);
        vshuffle(indata_down_a, index_front_odd, L_Matrix_6_up, SHUFFLE_GATHER, 256);
        vshuffle(indata_down_b, index_back_odd, L_Matrix_6_down, SHUFFLE_GATHER, 256);
        indata_up_a   = vmul(indata_up_a, Mask_111000, MASKREAD_OFF, 256);
        indata_up_b   = vmul(indata_up_b, Mask_000111, MASKREAD_OFF, 256);
        indata_down_a = vmul(indata_down_a, Mask_111000, MASKREAD_OFF, 256);
        indata_down_b = vmul(indata_down_b, Mask_000111, MASKREAD_OFF, 256);

        temp_a = vsadd(indata_up_a, indata_up_b, MASKREAD_OFF, 256);
        temp_b = vsadd(indata_down_a, indata_down_b, MASKREAD_OFF, 256);
        vshuffle(temp_c, copy_index, R_Matrix_5_up, SHUFFLE_GATHER, 256);
        vshuffle(temp_d, copy_index, R_Matrix_5_down, SHUFFLE_GATHER, 256);
      } else if (column == 5) {
        vshuffle(indata_up_a, index_front_even, L_Matrix_5_up, SHUFFLE_GATHER,
                 256); // even = {0,2,4,6,...}
        vshuffle(indata_up_b, index_back_even, L_Matrix_5_down, SHUFFLE_GATHER, 256);
        vshuffle(indata_down_a, index_front_odd, L_Matrix_5_up, SHUFFLE_GATHER, 256);
        vshuffle(indata_down_b, index_back_odd, L_Matrix_5_down, SHUFFLE_GATHER, 256);
        indata_up_a   = vmul(indata_up_a, Mask_111000, MASKREAD_OFF, 256);
        indata_up_b   = vmul(indata_up_b, Mask_000111, MASKREAD_OFF, 256);
        indata_down_a = vmul(indata_down_a, Mask_111000, MASKREAD_OFF, 256);
        indata_down_b = vmul(indata_down_b, Mask_000111, MASKREAD_OFF, 256);

        temp_a = vsadd(indata_up_a, indata_up_b, MASKREAD_OFF, 256);
        temp_b = vsadd(indata_down_a, indata_down_b, MASKREAD_OFF, 256);
        vshuffle(temp_c, copy_index, R_Matrix_4_up, SHUFFLE_GATHER, 256);
        vshuffle(temp_d, copy_index, R_Matrix_4_down, SHUFFLE_GATHER, 256);
      } else if (column == 4) {
        vshuffle(indata_up_a, index_front_even, L_Matrix_4_up, SHUFFLE_GATHER,
                 256); // even = {0,2,4,6,...}
        vshuffle(indata_up_b, index_back_even, L_Matrix_4_down, SHUFFLE_GATHER, 256);
        vshuffle(indata_down_a, index_front_odd, L_Matrix_4_up, SHUFFLE_GATHER, 256);
        vshuffle(indata_down_b, index_back_odd, L_Matrix_4_down, SHUFFLE_GATHER, 256);
        indata_up_a   = vmul(indata_up_a, Mask_111000, MASKREAD_OFF, 256);
        indata_up_b   = vmul(indata_up_b, Mask_000111, MASKREAD_OFF, 256);
        indata_down_a = vmul(indata_down_a, Mask_111000, MASKREAD_OFF, 256);
        indata_down_b = vmul(indata_down_b, Mask_000111, MASKREAD_OFF, 256);

        temp_a = vsadd(indata_up_a, indata_up_b, MASKREAD_OFF, 256);
        temp_b = vsadd(indata_down_a, indata_down_b, MASKREAD_OFF, 256);
        vshuffle(temp_c, copy_index, R_Matrix_3_up, SHUFFLE_GATHER, 256);
        vshuffle(temp_d, copy_index, R_Matrix_3_down, SHUFFLE_GATHER, 256);
      } else if (column == 3) {
        vshuffle(indata_up_a, index_front_even, L_Matrix_3_up, SHUFFLE_GATHER,
                 256); // even = {0,2,4,6,...}
        vshuffle(indata_up_b, index_back_even, L_Matrix_3_down, SHUFFLE_GATHER, 256);
        vshuffle(indata_down_a, index_front_odd, L_Matrix_3_up, SHUFFLE_GATHER, 256);
        vshuffle(indata_down_b, index_back_odd, L_Matrix_3_down, SHUFFLE_GATHER, 256);
        indata_up_a   = vmul(indata_up_a, Mask_111000, MASKREAD_OFF, 256);
        indata_up_b   = vmul(indata_up_b, Mask_000111, MASKREAD_OFF, 256);
        indata_down_a = vmul(indata_down_a, Mask_111000, MASKREAD_OFF, 256);
        indata_down_b = vmul(indata_down_b, Mask_000111, MASKREAD_OFF, 256);

        temp_a = vsadd(indata_up_a, indata_up_b, MASKREAD_OFF, 256);
        temp_b = vsadd(indata_down_a, indata_down_b, MASKREAD_OFF, 256);
        vshuffle(temp_c, copy_index, R_Matrix_2_up, SHUFFLE_GATHER, 256);
        vshuffle(temp_d, copy_index, R_Matrix_2_down, SHUFFLE_GATHER, 256);
      } else if (column == 2) {
        vshuffle(indata_up_a, index_front_even, L_Matrix_2_up, SHUFFLE_GATHER,
                 256); // even = {0,2,4,6,...}
        vshuffle(indata_up_b, index_back_even, L_Matrix_2_down, SHUFFLE_GATHER, 256);
        vshuffle(indata_down_a, index_front_odd, L_Matrix_2_up, SHUFFLE_GATHER, 256);
        vshuffle(indata_down_b, index_back_odd, L_Matrix_2_down, SHUFFLE_GATHER, 256);
        indata_up_a   = vmul(indata_up_a, Mask_111000, MASKREAD_OFF, 256);
        indata_up_b   = vmul(indata_up_b, Mask_000111, MASKREAD_OFF, 256);
        indata_down_a = vmul(indata_down_a, Mask_111000, MASKREAD_OFF, 256);
        indata_down_b = vmul(indata_down_b, Mask_000111, MASKREAD_OFF, 256);

        temp_a = vsadd(indata_up_a, indata_up_b, MASKREAD_OFF, 256);
        temp_b = vsadd(indata_down_a, indata_down_b, MASKREAD_OFF, 256);
        vshuffle(temp_c, copy_index, R_Matrix_1_up, SHUFFLE_GATHER, 256);
        vshuffle(temp_d, copy_index, R_Matrix_1_down, SHUFFLE_GATHER, 256);
      } else if (column == 1) {
        vshuffle(indata_up_a, index_front_even, L_Matrix_1_up, SHUFFLE_GATHER,
                 256); // even = {0,2,4,6,...}
        vshuffle(indata_up_b, index_back_even, L_Matrix_1_down, SHUFFLE_GATHER, 256);
        vshuffle(indata_down_a, index_front_odd, L_Matrix_1_up, SHUFFLE_GATHER, 256);
        vshuffle(indata_down_b, index_back_odd, L_Matrix_1_down, SHUFFLE_GATHER, 256);
        indata_up_a   = vmul(indata_up_a, Mask_111000, MASKREAD_OFF, 256);
        indata_up_b   = vmul(indata_up_b, Mask_000111, MASKREAD_OFF, 256);
        indata_down_a = vmul(indata_down_a, Mask_111000, MASKREAD_OFF, 256);
        indata_down_b = vmul(indata_down_b, Mask_000111, MASKREAD_OFF, 256);

        temp_a = vsadd(indata_up_a, indata_up_b, MASKREAD_OFF, 256);
        temp_b = vsadd(indata_down_a, indata_down_b, MASKREAD_OFF, 256);
        vshuffle(temp_c, copy_index, R_Matrix_0_up, SHUFFLE_GATHER, 256);
        vshuffle(temp_d, copy_index, R_Matrix_0_down, SHUFFLE_GATHER, 256);
      }

      // 冗余
      temp_a = vsadd(temp_a, 0, MASKREAD_OFF, 256);
      temp_b = vsadd(temp_b, 0, MASKREAD_OFF, 256);
      temp_c = vsadd(temp_c, 0, MASKREAD_OFF, 256);
      temp_d = vsadd(temp_d, 0, MASKREAD_OFF, 256);

      __v4096i8 b_add_d;
      b_add_d = vsadd(temp_b, temp_d, MASKREAD_OFF, 256);

      __v4096i8 temp_sign_larger_0;
      __v4096i8 temp_sign_smaller_0;
      temp_sign_smaller_0 = vsgt(b_add_d, 0, MASKREAD_OFF, MASKWRITE_OFF, 256);
      temp_sign_larger_0  = vslt(b_add_d, 0, MASKREAD_OFF, MASKWRITE_OFF, 256);

      __v4096i8 sign_b_add_d;
      sign_b_add_d = vsadd(temp_sign_larger_0, 0, MASKREAD_OFF, 256);
      sign_b_add_d = vssub(temp_sign_smaller_0, sign_b_add_d, MASKREAD_OFF, 256);

      vbrdcst(temp_sign_larger_0, 0, MASKREAD_OFF, 256);
      vbrdcst(temp_sign_smaller_0, 0, MASKREAD_OFF, 256);

      temp_sign_smaller_0 = vsgt(temp_a, 0, MASKREAD_OFF, MASKWRITE_OFF, 256);
      temp_sign_larger_0  = vslt(temp_a, 0, MASKREAD_OFF, MASKWRITE_OFF, 256);

      __v4096i8 sign_a;
      sign_a = vsadd(temp_sign_larger_0, 0, MASKREAD_OFF, 256);
      sign_a = vssub(temp_sign_smaller_0, sign_a, MASKREAD_OFF, 256);

      __v4096i8 result_up_temp;
      __v4096i8 result_up_temp_a;
      result_up_temp_a = vmul(sign_a, sign_b_add_d, MASKREAD_OFF, 256);
      result_up_temp   = vmul(result_up_temp_a, alpha, MASKREAD_OFF, 256);

      __v4096i8 abs_b_add_d;
      __v4096i8 abs_a;
      vsgt(b_add_d, 0, MASKREAD_OFF, MASKWRITE_ON, 256);
      vbrdcst(temp, 0xFF, MASKREAD_OFF, 256);
      vbrdcst(temp1, 0x1, MASKREAD_OFF, 256);
      abs_b_add_d = vxor(b_add_d, temp, MASKREAD_ON, 256);
      abs_b_add_d = vsadd(abs_b_add_d, temp1, MASKREAD_ON, 256);
      // abs_b_add_d = vmul(b_add_d, -1, MASKREAD_ON, MASKWRITE_OFF, 256);
      vsgt(temp_a, 0, MASKREAD_OFF, MASKWRITE_ON, 256);
      abs_a = vxor(temp_a, temp, MASKREAD_ON, 256);
      abs_a = vsadd(abs_a, temp1, MASKREAD_ON, 256);
      // abs_a = vmul(temp_a, -1, MASKREAD_ON, MASKWRITE_OFF, 256);

      __v4096i8 min_bd_a;
      min_bd_a = vmul(abs_a, 0, MASKREAD_OFF, 256); // 冗余
      vsle(abs_b_add_d, abs_a, MASKREAD_OFF, MASKWRITE_ON, 256);
      min_bd_a = vsadd(min_bd_a, abs_a, MASKREAD_ON, 256);
      vsgt(abs_b_add_d, abs_a, MASKREAD_OFF, MASKWRITE_ON, 256);
      min_bd_a = vsadd(min_bd_a, abs_b_add_d, MASKREAD_ON, MASKWRITE_OFF, 256);

      result_up_temp = MUL4096_8_FIXED(result_up_temp, min_bd_a, 6, 256);

      //  down
      temp_sign_smaller_0 = vsgt(temp_c, 0, MASKREAD_OFF, MASKWRITE_OFF, 256);
      temp_sign_larger_0  = vslt(temp_c, 0, MASKREAD_OFF, MASKWRITE_OFF, 256);

      __v4096i8 sign_c;
      sign_c = vsadd(temp_sign_larger_0, 0, MASKREAD_OFF, 256);
      sign_c = vssub(temp_sign_smaller_0, sign_c, MASKREAD_OFF, 256);

      __v4096i8 result_down_temp;
      __v4096i8 result_down_temp_a;
      result_down_temp_a = vmul(sign_a, sign_c, MASKREAD_OFF, 256);
      result_down_temp   = vmul(result_down_temp_a, alpha, MASKREAD_OFF, 256);

      __v4096i8 abs_c;
      vsgt(temp_c, 0, MASKREAD_OFF, MASKWRITE_ON, 256);
      abs_c = vxor(temp_c, temp, MASKREAD_ON, 256);
      abs_c = vsadd(abs_c, temp1, MASKREAD_ON, 256);
      // abs_c = vmul(temp_c, -1, MASKREAD_ON, MASKWRITE_OFF, 256);

      __v4096i8 min_a_c;
      min_a_c = vmul(abs_c, 0, MASKREAD_OFF, 256); //  冗余
      vsle(abs_a, abs_c, MASKREAD_OFF, MASKWRITE_ON, 256);
      min_a_c = vsadd(min_a_c, abs_c, MASKREAD_ON, MASKWRITE_OFF, 256);
      vsgt(abs_a, abs_c, MASKREAD_OFF, MASKWRITE_ON, 256);
      min_a_c = vsadd(min_a_c, abs_a, MASKREAD_ON, MASKWRITE_OFF, 256);

      result_down_temp = MUL4096_8_FIXED(result_down_temp, min_a_c, 6, 256);
      result_down_temp = vsadd(result_down_temp, temp_b, MASKREAD_OFF, 256);

      if (column == 9) {
        L_Matrix_8_up   = vmul(L_Matrix_8_up, 0, MASKREAD_OFF, 256);   //  冗余
        L_Matrix_8_down = vmul(L_Matrix_8_down, 0, MASKREAD_OFF, 256); //  冗余
        vshuffle(L_Matrix_8_up, copy_index, result_up_temp, SHUFFLE_GATHER, 256);
        vshuffle(L_Matrix_8_down, copy_index, result_down_temp, SHUFFLE_GATHER, 256);
      } else if (column == 8) {
        L_Matrix_7_up   = vmul(L_Matrix_7_up, 0, MASKREAD_OFF, 256);   //  冗余
        L_Matrix_7_down = vmul(L_Matrix_7_down, 0, MASKREAD_OFF, 256); //  冗余
        vshuffle(L_Matrix_7_up, copy_index, result_up_temp, SHUFFLE_GATHER, 256);
        vshuffle(L_Matrix_7_down, copy_index, result_down_temp, SHUFFLE_GATHER, 256);
      } else if (column == 7) {
        L_Matrix_6_up   = vmul(L_Matrix_6_up, 0, MASKREAD_OFF, 256);   //  冗余
        L_Matrix_6_down = vmul(L_Matrix_6_down, 0, MASKREAD_OFF, 256); //  冗余
        vshuffle(L_Matrix_6_up, copy_index, result_up_temp, SHUFFLE_GATHER, 256);
        vshuffle(L_Matrix_6_down, copy_index, result_down_temp, SHUFFLE_GATHER, 256);
      } else if (column == 6) {
        L_Matrix_5_up   = vmul(L_Matrix_5_up, 0, MASKREAD_OFF, 256);   //  冗余
        L_Matrix_5_down = vmul(L_Matrix_5_down, 0, MASKREAD_OFF, 256); //  冗余
        vshuffle(L_Matrix_5_up, copy_index, result_up_temp, SHUFFLE_GATHER, 256);
        vshuffle(L_Matrix_5_down, copy_index, result_down_temp, SHUFFLE_GATHER, 256);
      } else if (column == 5) {
        L_Matrix_4_up   = vmul(L_Matrix_4_up, 0, MASKREAD_OFF, 256);   //  冗余
        L_Matrix_4_down = vmul(L_Matrix_4_down, 0, MASKREAD_OFF, 256); //  冗余
        vshuffle(L_Matrix_4_up, copy_index, result_up_temp, SHUFFLE_GATHER, 256);
        vshuffle(L_Matrix_4_down, copy_index, result_down_temp, SHUFFLE_GATHER, 256);
      } else if (column == 4) {
        L_Matrix_3_up   = vmul(L_Matrix_3_up, 0, MASKREAD_OFF, 256);   //  冗余
        L_Matrix_3_down = vmul(L_Matrix_3_down, 0, MASKREAD_OFF, 256); //  冗余
        vshuffle(L_Matrix_3_up, copy_index, result_up_temp, SHUFFLE_GATHER, 256);
        vshuffle(L_Matrix_3_down, copy_index, result_down_temp, SHUFFLE_GATHER, 256);
      } else if (column == 3) {
        L_Matrix_2_up   = vmul(L_Matrix_2_up, 0, MASKREAD_OFF, 256);   //  冗余
        L_Matrix_2_down = vmul(L_Matrix_2_down, 0, MASKREAD_OFF, 256); //  冗余
        vshuffle(L_Matrix_2_up, copy_index, result_up_temp, SHUFFLE_GATHER, 256);
        vshuffle(L_Matrix_2_down, copy_index, result_down_temp, SHUFFLE_GATHER, 256);
      } else if (column == 2) {
        L_Matrix_1_up   = vmul(L_Matrix_1_up, 0, MASKREAD_OFF, 256);   //  冗余
        L_Matrix_1_down = vmul(L_Matrix_1_down, 0, MASKREAD_OFF, 256); //  冗余
        vshuffle(L_Matrix_1_up, copy_index, result_up_temp, SHUFFLE_GATHER, 256);
        vshuffle(L_Matrix_1_down, copy_index, result_down_temp, SHUFFLE_GATHER, 256);
      } else if (column == 1) {
        L_Matrix_0_up   = vmul(L_Matrix_0_up, 0, MASKREAD_OFF, 256);   //  冗余
        L_Matrix_0_down = vmul(L_Matrix_0_down, 0, MASKREAD_OFF, 256); //  冗余
        vshuffle(L_Matrix_0_up, copy_index, result_up_temp, SHUFFLE_GATHER, 256);
        vshuffle(L_Matrix_0_down, copy_index, result_down_temp, SHUFFLE_GATHER, 256);
      }
    }

    //  R Matrix Calculation
    for (int column = 1; column < 10; column++) {
      __v4096i8 indata_up_a;
      __v4096i8 indata_up_b;
      __v4096i8 indata_down_a;
      __v4096i8 indata_down_b;
      indata_up_a   = vmul(alpha, 0, MASKREAD_OFF, 256);
      indata_up_b   = vmul(alpha, 0, MASKREAD_OFF, 256);
      indata_down_a = vmul(alpha, 0, MASKREAD_OFF, 256);
      indata_down_b = vmul(alpha, 0, MASKREAD_OFF, 256);
      if (column == 1) {
        vshuffle(indata_up_a, index_front_even, L_Matrix_1_up, SHUFFLE_GATHER,
                 256); // even = {0,2,4,6,...}
        vshuffle(indata_up_b, index_back_even, L_Matrix_1_down, SHUFFLE_GATHER, 256);
        vshuffle(indata_down_a, index_front_odd, L_Matrix_1_up, SHUFFLE_GATHER, 256);
        vshuffle(indata_down_b, index_back_odd, L_Matrix_1_down, SHUFFLE_GATHER, 256);
        indata_up_a   = vmul(indata_up_a, Mask_111000, MASKREAD_OFF, 256);
        indata_up_b   = vmul(indata_up_b, Mask_000111, MASKREAD_OFF, 256);
        indata_down_a = vmul(indata_down_a, Mask_111000, MASKREAD_OFF, 256);
        indata_down_b = vmul(indata_down_b, Mask_000111, MASKREAD_OFF, 256);

        temp_a = vsadd(indata_up_a, indata_up_b, MASKREAD_OFF, 256);
        temp_b = vsadd(indata_down_a, indata_down_b, MASKREAD_OFF, 256);
        vshuffle(temp_c, copy_index, R_Matrix_0_up, SHUFFLE_GATHER, 256);
        vshuffle(temp_d, copy_index, R_Matrix_0_down, SHUFFLE_GATHER, 256);
      } else if (column == 2) {
        vshuffle(indata_up_a, index_front_even, L_Matrix_2_up, SHUFFLE_GATHER,
                 256); // even = {0,2,4,6,...}
        vshuffle(indata_up_b, index_back_even, L_Matrix_2_down, SHUFFLE_GATHER, 256);
        vshuffle(indata_down_a, index_front_odd, L_Matrix_2_up, SHUFFLE_GATHER, 256);
        vshuffle(indata_down_b, index_back_odd, L_Matrix_2_down, SHUFFLE_GATHER, 256);
        indata_up_a   = vmul(indata_up_a, Mask_111000, MASKREAD_OFF, 256);
        indata_up_b   = vmul(indata_up_b, Mask_000111, MASKREAD_OFF, 256);
        indata_down_a = vmul(indata_down_a, Mask_111000, MASKREAD_OFF, 256);
        indata_down_b = vmul(indata_down_b, Mask_000111, MASKREAD_OFF, 256);

        temp_a = vsadd(indata_up_a, indata_up_b, MASKREAD_OFF, 256);
        temp_b = vsadd(indata_down_a, indata_down_b, MASKREAD_OFF, 256);
        vshuffle(temp_c, copy_index, R_Matrix_1_up, SHUFFLE_GATHER, 256);
        vshuffle(temp_d, copy_index, R_Matrix_1_down, SHUFFLE_GATHER, 256);
      } else if (column == 3) {
        vshuffle(indata_up_a, index_front_even, L_Matrix_3_up, SHUFFLE_GATHER,
                 256); // even = {0,2,4,6,...}
        vshuffle(indata_up_b, index_back_even, L_Matrix_3_down, SHUFFLE_GATHER, 256);
        vshuffle(indata_down_a, index_front_odd, L_Matrix_3_up, SHUFFLE_GATHER, 256);
        vshuffle(indata_down_b, index_back_odd, L_Matrix_3_down, SHUFFLE_GATHER, 256);
        indata_up_a   = vmul(indata_up_a, Mask_111000, MASKREAD_OFF, 256);
        indata_up_b   = vmul(indata_up_b, Mask_000111, MASKREAD_OFF, 256);
        indata_down_a = vmul(indata_down_a, Mask_111000, MASKREAD_OFF, 256);
        indata_down_b = vmul(indata_down_b, Mask_000111, MASKREAD_OFF, 256);

        temp_a = vsadd(indata_up_a, indata_up_b, MASKREAD_OFF, 256);
        temp_b = vsadd(indata_down_a, indata_down_b, MASKREAD_OFF, 256);
        vshuffle(temp_c, copy_index, R_Matrix_2_up, SHUFFLE_GATHER, 256);
        vshuffle(temp_d, copy_index, R_Matrix_2_down, SHUFFLE_GATHER, 256);
      } else if (column == 4) {
        vshuffle(indata_up_a, index_front_even, L_Matrix_4_up, SHUFFLE_GATHER,
                 256); // even = {0,2,4,6,...}
        vshuffle(indata_up_b, index_back_even, L_Matrix_4_down, SHUFFLE_GATHER, 256);
        vshuffle(indata_down_a, index_front_odd, L_Matrix_4_up, SHUFFLE_GATHER, 256);
        vshuffle(indata_down_b, index_back_odd, L_Matrix_4_down, SHUFFLE_GATHER, 256);
        indata_up_a   = vmul(indata_up_a, Mask_111000, MASKREAD_OFF, 256);
        indata_up_b   = vmul(indata_up_b, Mask_000111, MASKREAD_OFF, 256);
        indata_down_a = vmul(indata_down_a, Mask_111000, MASKREAD_OFF, 256);
        indata_down_b = vmul(indata_down_b, Mask_000111, MASKREAD_OFF, 256);

        temp_a = vsadd(indata_up_a, indata_up_b, MASKREAD_OFF, 256);
        temp_b = vsadd(indata_down_a, indata_down_b, MASKREAD_OFF, 256);
        vshuffle(temp_c, copy_index, R_Matrix_3_up, SHUFFLE_GATHER, 256);
        vshuffle(temp_d, copy_index, R_Matrix_3_down, SHUFFLE_GATHER, 256);
      } else if (column == 5) {
        vshuffle(indata_up_a, index_front_even, L_Matrix_5_up, SHUFFLE_GATHER,
                 256); // even = {0,2,4,6,...}
        vshuffle(indata_up_b, index_back_even, L_Matrix_5_down, SHUFFLE_GATHER, 256);
        vshuffle(indata_down_a, index_front_odd, L_Matrix_5_up, SHUFFLE_GATHER, 256);
        vshuffle(indata_down_b, index_back_odd, L_Matrix_5_down, SHUFFLE_GATHER, 256);
        indata_up_a   = vmul(indata_up_a, Mask_111000, MASKREAD_OFF, 256);
        indata_up_b   = vmul(indata_up_b, Mask_000111, MASKREAD_OFF, 256);
        indata_down_a = vmul(indata_down_a, Mask_111000, MASKREAD_OFF, 256);
        indata_down_b = vmul(indata_down_b, Mask_000111, MASKREAD_OFF, 256);

        temp_a = vsadd(indata_up_a, indata_up_b, MASKREAD_OFF, 256);
        temp_b = vsadd(indata_down_a, indata_down_b, MASKREAD_OFF, 256);
        vshuffle(temp_c, copy_index, R_Matrix_4_up, SHUFFLE_GATHER, 256);
        vshuffle(temp_d, copy_index, R_Matrix_4_down, SHUFFLE_GATHER, 256);
      } else if (column == 6) {
        vshuffle(indata_up_a, index_front_even, L_Matrix_6_up, SHUFFLE_GATHER,
                 256); // even = {0,2,4,6,...}
        vshuffle(indata_up_b, index_back_even, L_Matrix_6_down, SHUFFLE_GATHER, 256);
        vshuffle(indata_down_a, index_front_odd, L_Matrix_6_up, SHUFFLE_GATHER, 256);
        vshuffle(indata_down_b, index_back_odd, L_Matrix_6_down, SHUFFLE_GATHER, 256);
        indata_up_a   = vmul(indata_up_a, Mask_111000, MASKREAD_OFF, 256);
        indata_up_b   = vmul(indata_up_b, Mask_000111, MASKREAD_OFF, 256);
        indata_down_a = vmul(indata_down_a, Mask_111000, MASKREAD_OFF, 256);
        indata_down_b = vmul(indata_down_b, Mask_000111, MASKREAD_OFF, 256);

        temp_a = vsadd(indata_up_a, indata_up_b, MASKREAD_OFF, 256);
        temp_b = vsadd(indata_down_a, indata_down_b, MASKREAD_OFF, 256);
        vshuffle(temp_c, copy_index, R_Matrix_5_up, SHUFFLE_GATHER, 256);
        vshuffle(temp_d, copy_index, R_Matrix_5_down, SHUFFLE_GATHER, 256);
      } else if (column == 7) {
        vshuffle(indata_up_a, index_front_even, L_Matrix_7_up, SHUFFLE_GATHER,
                 256); // even = {0,2,4,6,...}
        vshuffle(indata_up_b, index_back_even, L_Matrix_7_down, SHUFFLE_GATHER, 256);
        vshuffle(indata_down_a, index_front_odd, L_Matrix_7_up, SHUFFLE_GATHER, 256);
        vshuffle(indata_down_b, index_back_odd, L_Matrix_7_down, SHUFFLE_GATHER, 256);
        indata_up_a   = vmul(indata_up_a, Mask_111000, MASKREAD_OFF, 256);
        indata_up_b   = vmul(indata_up_b, Mask_000111, MASKREAD_OFF, 256);
        indata_down_a = vmul(indata_down_a, Mask_111000, MASKREAD_OFF, 256);
        indata_down_b = vmul(indata_down_b, Mask_000111, MASKREAD_OFF, 256);

        temp_a = vsadd(indata_up_a, indata_up_b, MASKREAD_OFF, 256);
        temp_b = vsadd(indata_down_a, indata_down_b, MASKREAD_OFF, 256);
        vshuffle(temp_c, copy_index, R_Matrix_6_up, SHUFFLE_GATHER, 256);
        vshuffle(temp_d, copy_index, R_Matrix_6_down, SHUFFLE_GATHER, 256);
      } else if (column == 8) {
        vshuffle(indata_up_a, index_front_even, L_Matrix_8_up, SHUFFLE_GATHER,
                 256); // even = {0,2,4,6,...}
        vshuffle(indata_up_b, index_back_even, L_Matrix_8_down, SHUFFLE_GATHER, 256);
        vshuffle(indata_down_a, index_front_odd, L_Matrix_8_up, SHUFFLE_GATHER, 256);
        vshuffle(indata_down_b, index_back_odd, L_Matrix_8_down, SHUFFLE_GATHER, 256);
        indata_up_a   = vmul(indata_up_a, Mask_111000, MASKREAD_OFF, 256);
        indata_up_b   = vmul(indata_up_b, Mask_000111, MASKREAD_OFF, 256);
        indata_down_a = vmul(indata_down_a, Mask_111000, MASKREAD_OFF, 256);
        indata_down_b = vmul(indata_down_b, Mask_000111, MASKREAD_OFF, 256);

        temp_a = vsadd(indata_up_a, indata_up_b, MASKREAD_OFF, 256);
        temp_b = vsadd(indata_down_a, indata_down_b, MASKREAD_OFF, 256);
        vshuffle(temp_c, copy_index, R_Matrix_7_up, SHUFFLE_GATHER, 256);
        vshuffle(temp_d, copy_index, R_Matrix_7_down, SHUFFLE_GATHER, 256);
      } else if (column == 9) {
        vshuffle(indata_up_a, index_front_even, L_Matrix_N_up, SHUFFLE_GATHER,
                 256); // even = {0,2,4,6,...}
        vshuffle(indata_up_b, index_back_even, L_Matrix_N_down, SHUFFLE_GATHER, 256);
        vshuffle(indata_down_a, index_front_odd, L_Matrix_N_up, SHUFFLE_GATHER, 256);
        vshuffle(indata_down_b, index_back_odd, L_Matrix_N_down, SHUFFLE_GATHER, 256);
        indata_up_a   = vmul(indata_up_a, Mask_111000, MASKREAD_OFF, 256);
        indata_up_b   = vmul(indata_up_b, Mask_000111, MASKREAD_OFF, 256);
        indata_down_a = vmul(indata_down_a, Mask_111000, MASKREAD_OFF, 256);
        indata_down_b = vmul(indata_down_b, Mask_000111, MASKREAD_OFF, 256);

        temp_a = vsadd(indata_up_a, indata_up_b, MASKREAD_OFF, 256);
        temp_b = vsadd(indata_down_a, indata_down_b, MASKREAD_OFF, 256);
        vshuffle(temp_c, copy_index, R_Matrix_8_up, SHUFFLE_GATHER, 256);
        vshuffle(temp_d, copy_index, R_Matrix_8_down, SHUFFLE_GATHER, 256);
      }

      __v4096i8 b_add_d;
      b_add_d = vsadd(temp_b, temp_d, MASKREAD_OFF, 256);

      __v4096i8 temp_sign_larger_0;
      __v4096i8 temp_sign_smaller_0;
      temp_sign_smaller_0 = vsgt(b_add_d, 0, MASKREAD_OFF, MASKWRITE_OFF, 256);
      temp_sign_larger_0  = vslt(b_add_d, 0, MASKREAD_OFF, MASKWRITE_OFF, 256);

      __v4096i8 sign_b_add_d;
      sign_b_add_d = vsadd(temp_sign_larger_0, 0, MASKREAD_OFF, 256);
      sign_b_add_d = vssub(temp_sign_smaller_0, sign_b_add_d, MASKREAD_OFF, 256);

      vbrdcst(temp_sign_larger_0, 0, MASKREAD_OFF, 256);
      vbrdcst(temp_sign_smaller_0, 0, MASKREAD_OFF, 256);

      temp_sign_smaller_0 = vsgt(temp_c, 0, MASKREAD_OFF, MASKWRITE_OFF, 256);
      temp_sign_larger_0  = vslt(temp_c, 0, MASKREAD_OFF, MASKWRITE_OFF, 256);

      __v4096i8 sign_c;
      sign_c = vsadd(temp_sign_larger_0, 0, MASKREAD_OFF, 256);
      sign_c = vssub(temp_sign_smaller_0, sign_c, MASKREAD_OFF, 256);

      // __v4096i8 result_up_temp_a;
      result_up_temp_a = vmul(sign_c, sign_b_add_d, MASKREAD_OFF, 256);
      result_up_temp   = vmul(result_up_temp_a, alpha, MASKREAD_OFF, 256);

      __v4096i8 abs_b_add_d;
      __v4096i8 abs_c;
      vsgt(b_add_d, 0, MASKREAD_OFF, MASKWRITE_ON, 256);
      abs_b_add_d = vxor(b_add_d, temp, MASKREAD_ON, 256);
      abs_b_add_d = vsadd(abs_b_add_d, temp1, MASKREAD_ON, 256);
      // abs_b_add_d = vmul(b_add_d, -1, MASKREAD_ON, MASKWRITE_OFF, 256);
      vsgt(temp_c, 0, MASKREAD_OFF, MASKWRITE_ON, 256);
      abs_c = vxor(temp_c, temp, MASKREAD_ON, 256);
      abs_c = vsadd(abs_c, temp1, MASKREAD_ON, 256);
      // abs_a = vmul(temp_a, -1, MASKREAD_ON, MASKWRITE_OFF, 256);

      // vsgt(b_add_d, 0, MASKREAD_OFF, MASKWRITE_ON, 256);
      // abs_b_add_d = vmul(b_add_d, -1, MASKREAD_ON, MASKWRITE_OFF, 256);
      // vsgt(temp_c, 0, MASKREAD_OFF, MASKWRITE_ON, 256);
      // abs_c = vmul(temp_c, -1, MASKREAD_ON, MASKWRITE_OFF, 256);

      // __v4096i8 min_bd_c;
      min_bd_c = vmul(abs_c, 0, MASKREAD_OFF, 256); // 冗余
      vsle(abs_b_add_d, abs_c, MASKREAD_OFF, MASKWRITE_ON, 256);
      min_bd_c = vsadd(min_bd_c, abs_c, MASKREAD_ON, 256);
      vsgt(abs_b_add_d, abs_c, MASKREAD_OFF, MASKWRITE_ON, 256);
      min_bd_c = vsadd(min_bd_c, abs_b_add_d, MASKREAD_ON, MASKWRITE_OFF, 256);

      result_up_temp = MUL4096_8_FIXED(result_up_temp, min_bd_c, 6, 256);

      //  down
      temp_sign_smaller_0 = vsgt(temp_a, 0, MASKREAD_OFF, MASKWRITE_OFF, 256);
      temp_sign_larger_0  = vslt(temp_a, 0, MASKREAD_OFF, MASKWRITE_OFF, 256);

      __v4096i8 sign_a;
      sign_a = vsadd(temp_sign_larger_0, 0, MASKREAD_OFF, 256);
      sign_a = vssub(temp_sign_smaller_0, sign_a, MASKREAD_OFF, 256);

      __v4096i8 result_down_temp;
      __v4096i8 result_down_temp_a;
      result_down_temp_a = vmul(sign_a, sign_c, MASKREAD_OFF, 256);
      result_down_temp   = vmul(result_down_temp_a, alpha, MASKREAD_OFF, 256);

      __v4096i8 abs_a;
      vsgt(temp_a, 0, MASKREAD_OFF, MASKWRITE_ON, 256);
      abs_a = vxor(temp_a, temp, MASKREAD_ON, 256);
      abs_a = vsadd(abs_a, temp1, MASKREAD_ON, 256);
      // abs_a = vmul(temp_a, -1, MASKREAD_ON, MASKWRITE_OFF, 256);

      __v4096i8 min_a_c;
      min_a_c = vmul(abs_c, 0, MASKREAD_OFF, 256); //  冗余
      vsle(abs_a, abs_c, MASKREAD_OFF, MASKWRITE_ON, 256);
      min_a_c = vsadd(min_a_c, abs_c, MASKREAD_ON, MASKWRITE_OFF, 256);
      vsgt(abs_a, abs_c, MASKREAD_OFF, MASKWRITE_ON, 256);
      min_a_c = vsadd(min_a_c, abs_a, MASKREAD_ON, MASKWRITE_OFF, 256);

      result_down_temp = MUL4096_8_FIXED(result_down_temp, min_a_c, 6, 256);
      result_down_temp = vsadd(result_down_temp, temp_d, MASKREAD_OFF, 256);

      __v4096i8 result_up;
      __v4096i8 result_down;
      __v4096i8 result_up_a;
      __v4096i8 result_up_b;
      __v4096i8 result_down_a;
      __v4096i8 result_down_b;

      //  冗余
      result_up_a   = vmul(result_down_temp, 0, MASKREAD_OFF, 256);
      result_up_b   = vmul(result_down_temp, 0, MASKREAD_OFF, 256);
      result_down_a = vmul(result_down_temp, 0, MASKREAD_OFF, 256);
      result_down_b = vmul(result_down_temp, 0, MASKREAD_OFF, 256);

      vshuffle(result_up_a, Xor_index_up, result_up_temp, SHUFFLE_GATHER, 256);
      vshuffle(result_up_b, Equal_index_up, result_down_temp, SHUFFLE_GATHER, 256);
      vshuffle(result_down_a, Xor_index_down, result_up_temp, SHUFFLE_GATHER, 256);
      vshuffle(result_down_b, Equal_index_down, result_down_temp, SHUFFLE_GATHER, 256);
      result_up_a   = vmul(result_up_a, Mask_1010, MASKREAD_OFF, 256);
      result_up_b   = vmul(result_up_b, Mask_0101, MASKREAD_OFF, 256);
      result_down_a = vmul(result_down_a, Mask_1010, MASKREAD_OFF, 256);
      result_down_b = vmul(result_down_b, Mask_0101, MASKREAD_OFF, 256);
      result_up     = vsadd(result_up_a, result_up_b, MASKREAD_OFF, 256);
      result_down   = vsadd(result_down_a, result_down_b, MASKREAD_OFF, 256);

      if (column == 9) {
        R_Matrix_9_up   = vmul(alpha, 0, MASKREAD_OFF, 256); // 冗余
        R_Matrix_9_down = vmul(alpha, 0, MASKREAD_OFF, 256); // 冗余
        vshuffle(R_Matrix_9_up, copy_index, result_up, SHUFFLE_GATHER, 256);
        vshuffle(R_Matrix_9_down, copy_index, result_down, SHUFFLE_GATHER, 256);
      } else if (column == 8) {
        R_Matrix_8_up   = vmul(alpha, 0, MASKREAD_OFF, 256); // 冗余
        R_Matrix_8_down = vmul(alpha, 0, MASKREAD_OFF, 256); // 冗余
        vshuffle(R_Matrix_8_up, copy_index, result_up, SHUFFLE_GATHER, 256);
        vshuffle(R_Matrix_8_down, copy_index, result_down, SHUFFLE_GATHER, 256);
      } else if (column == 7) {
        R_Matrix_7_up   = vmul(alpha, 0, MASKREAD_OFF, 256); // 冗余
        R_Matrix_7_down = vmul(alpha, 0, MASKREAD_OFF, 256); // 冗余
        vshuffle(R_Matrix_7_up, copy_index, result_up, SHUFFLE_GATHER, 256);
        vshuffle(R_Matrix_7_down, copy_index, result_down, SHUFFLE_GATHER, 256);
      } else if (column == 6) {
        R_Matrix_6_up   = vmul(alpha, 0, MASKREAD_OFF, 256); // 冗余
        R_Matrix_6_down = vmul(alpha, 0, MASKREAD_OFF, 256); // 冗余
        vshuffle(R_Matrix_6_up, copy_index, result_up, SHUFFLE_GATHER, 256);
        vshuffle(R_Matrix_6_down, copy_index, result_down, SHUFFLE_GATHER, 256);
      } else if (column == 5) {
        R_Matrix_5_up   = vmul(alpha, 0, MASKREAD_OFF, 256); // 冗余
        R_Matrix_5_down = vmul(alpha, 0, MASKREAD_OFF, 256); // 冗余
        vshuffle(R_Matrix_5_up, copy_index, result_up, SHUFFLE_GATHER, 256);
        vshuffle(R_Matrix_5_down, copy_index, result_down, SHUFFLE_GATHER, 256);
      } else if (column == 4) {
        R_Matrix_4_up   = vmul(alpha, 0, MASKREAD_OFF, 256); // 冗余
        R_Matrix_4_down = vmul(alpha, 0, MASKREAD_OFF, 256); // 冗余
        vshuffle(R_Matrix_4_up, copy_index, result_up, SHUFFLE_GATHER, 256);
        vshuffle(R_Matrix_4_down, copy_index, result_down, SHUFFLE_GATHER, 256);
      } else if (column == 3) {
        R_Matrix_3_up   = vmul(alpha, 0, MASKREAD_OFF, 256); // 冗余
        R_Matrix_3_down = vmul(alpha, 0, MASKREAD_OFF, 256); // 冗余
        vshuffle(R_Matrix_3_up, copy_index, result_up, SHUFFLE_GATHER, 256);
        vshuffle(R_Matrix_3_down, copy_index, result_down, SHUFFLE_GATHER, 256);
      } else if (column == 2) {
        R_Matrix_2_up   = vmul(alpha, 0, MASKREAD_OFF, 256); // 冗余
        R_Matrix_2_down = vmul(alpha, 0, MASKREAD_OFF, 256); // 冗余
        vshuffle(R_Matrix_2_up, copy_index, result_up, SHUFFLE_GATHER, 256);
        vshuffle(R_Matrix_2_down, copy_index, result_down, SHUFFLE_GATHER, 256);
      } else if (column == 1) {
        R_Matrix_1_up   = vmul(alpha, 0, MASKREAD_OFF, 256); // 冗余
        R_Matrix_1_down = vmul(alpha, 0, MASKREAD_OFF, 256); // 冗余
        vshuffle(R_Matrix_1_up, copy_index, result_up, SHUFFLE_GATHER, 256);
        vshuffle(R_Matrix_1_down, copy_index, result_down, SHUFFLE_GATHER, 256);
      }
    }
  }
  __v4096i8 final_llr_up;
  __v4096i8 final_llr_down;
  final_llr_up   = vsadd(L_Matrix_0_up, R_Matrix_0_up, MASKREAD_OFF, 256);
  final_llr_down = vsadd(L_Matrix_0_down, R_Matrix_0_down, MASKREAD_OFF, 256);

  __v4096i8 final_llr;
  vbrdcst(final_llr, 0, MASKREAD_OFF, 512);
  __v2048i16 move_256to512;
  __v2048i16 mask_0to255;
  vrange(move_256to512, 512);
  move_256to512 = vrsub(move_256to512, 256, MASKREAD_OFF, 512);
  mask_0to255   = vmul(move_256to512, 0, MASKREAD_OFF, 512); // 初始化乘0
  mask_0to255   = vsadd(mask_0to255, 1, MASKREAD_OFF, 512);  // 全部赋值为1
  mask_0to255   = vrsub(mask_0to255, 1, MASKREAD_OFF, 256);  // 前一半为0
  move_256to512 = vmul(move_256to512, mask_0to255, MASKREAD_OFF, 512);

  vshuffle(final_llr, move_256to512, final_llr_down, SHUFFLE_GATHER, 512);
  __v4096i8 mask_0to255_8bit;
  vbrdcst(mask_0to255_8bit, 1, MASKREAD_OFF, 512);
  vbrdcst(mask_0to255_8bit, 0, MASKREAD_OFF, 256);
  final_llr = vmul(final_llr, mask_0to255_8bit, MASKREAD_OFF, 512);
  vbrdcst(mask_0to255_8bit, 0, MASKREAD_OFF, 512);
  vbrdcst(mask_0to255_8bit, 1, MASKREAD_OFF, 256);
  final_llr_up = vmul(final_llr_up, mask_0to255_8bit, MASKREAD_OFF, 512);

  final_llr = vsadd(final_llr, final_llr_up, MASKREAD_OFF, 512);

  __v4096i8 final_bit;
  final_bit = vsgt(final_llr, 0, MASKREAD_OFF, MASKWRITE_OFF, 512);

  // __v2048i16 PBCH_bit_Index;
  // __v2048i16 iIL_index;
  // vclaim(PBCH_bit_Index);
  // vclaim(iIL_index);

  // vbarrier();
  // VSPM_OPEN();
  // int PBCH_bit_Index_addr = vaddr(PBCH_bit_Index);
  // for (int i = 0; i < 56; i++) {
  //   *(volatile unsigned short *)(PBCH_bit_Index_addr + (i << 1)) =
  //       PBCH_Info_index[i];
  // }
  // int iIL_index_addr = vaddr(iIL_index);
  // for (int i = 0; i < 56; i++) {
  //   *(volatile unsigned short *)(iIL_index_addr + (i << 1)) = iIL_56_PBCH[i];
  // }
  // VSPM_CLOSE();

  __v4096i8 PBCH_bits_out_temp;
  __v4096i8 PBCH_bits_out;
  vclaim(PBCH_bits_out_temp);
  vclaim(PBCH_bits_out);
  vshuffle(PBCH_bits_out_temp, PBCH_bit_Index, final_bit, SHUFFLE_GATHER, 56);
  vshuffle(PBCH_bits_out, iIL_index, PBCH_bits_out_temp, SHUFFLE_SCATTER, 56);

  vbrdcst(mask_0to255_8bit, 0, MASKREAD_OFF, 512);
  vbrdcst(mask_0to255_8bit, 1, MASKREAD_OFF, 56);
  PBCH_bits_out = vmul(PBCH_bits_out, mask_0to255_8bit, MASKREAD_OFF, 512);

  // __v4096i8 final_bits_up;
  // __v4096i8 final_bits_down;
  // final_bits_up = vsgt(final_llr_up, 0, MASKREAD_OFF, MASKWRITE_OFF, 256);
  // final_bits_down = vsgt(final_llr_down, 0, MASKREAD_OFF, MASKWRITE_OFF,
  // 256);

  // printf("\n\nPolar_Length:%hd\n\n", &Polar_Length);

  // VENUS_PRINTVEC_CHAR(min_bd_c, 256);
  return PBCH_bits_out;
}

VENUS_INLINE __v4096i8 crc_24C(__v4096i8 vin, int fullLen, __v4096i8 poly) {
  int pariLen = 24 + 1;
  int msgLen  = fullLen - pariLen + 1;
  int tmp;

  __v4096i8 buf;
  __v4096i8 msg;
  // __v4096i8 poly;
  __v2048i16 index;
  vclaim(buf);
  vclaim(msg);
  // vclaim(poly);
  vclaim(index);

  vrange(index, msgLen);
  vbrdcst(msg, 0, MASKREAD_OFF, fullLen);
  vshuffle(msg, index, vin, SHUFFLE_GATHER, msgLen);

  // int poly_addr = vaddr(poly);
  // vbarrier();
  // VSPM_OPEN();
  // for (size_t i = 0; i < pariLen; i++) {
  //   unsigned int addr = poly_addr + i;
  //   *(volatile unsigned char *)(addr) = POLY_CRC24C[i];
  // }
  // VSPM_CLOSE();

  for (int i = 0; i < msgLen; i++) {
    int m_addr = vaddr(msg);
    vbarrier();
    VSPM_OPEN();
    unsigned int addr = m_addr + i;
    tmp               = *(volatile unsigned char *)(addr);
    VSPM_CLOSE();

    if (tmp == 1) {
      vrange(index, pariLen);
      index = vsadd(index, i, MASKREAD_OFF, pariLen);
      vshuffle(buf, index, msg, SHUFFLE_GATHER, pariLen);
      buf = vxor(buf, poly, MASKREAD_OFF, pariLen);
      vshuffle(msg, index, buf, SHUFFLE_SCATTER, pariLen);
    }
  }

  vrange(index, pariLen - 1);
  index = vsadd(index, msgLen, MASKREAD_OFF, pariLen - 1);
  vshuffle(buf, index, msg, SHUFFLE_GATHER, pariLen - 1);

  return buf;
}

static uint8_t seqShuffleIndex[32];
static uint8_t aBarShuffleIndex[32];
static uint8_t G[32] = {16, 23, 18, 17, 8,  30, 10, 6,  24, 7,  0,  5,  3,  2,  1,  4,
                        9,  11, 12, 13, 14, 15, 19, 20, 21, 22, 25, 26, 27, 28, 29, 31};
static uint8_t isScrambled[32];

VENUS_INLINE __v4096i8 DescrambleAndDeInterleaverIndex(__v4096i8 scrBlk, __v4096i8 seqSet, short Lssb) {

  uint8_t jSFN = 7;
  uint8_t jHRF = 10;
  uint8_t jSSB = 11;
  uint8_t jOTH = 14;
  uint8_t M    = (Lssb == 64) ? 26 : 29;
  uint8_t v    = 0;

  int scrBlk_addr = vaddr(scrBlk);
  vbarrier();
  VSPM_OPEN();
  v += (*(volatile char *)(scrBlk_addr + G[jSFN + 1]));
  VSPM_CLOSE();
  scrBlk_addr = vaddr(scrBlk);
  vbarrier();
  VSPM_OPEN();
  v += (*(volatile char *)(scrBlk_addr + G[jSFN])) * 2;
  VSPM_CLOSE();

  __v2048i16 v_shuffle_index;
  vclaim(v_shuffle_index);
  vrange(v_shuffle_index, M);
  __v2048i16 const_value;
  vclaim(const_value);
  vbrdcst(const_value, v * M, MASKREAD_OFF, M);
  v_shuffle_index = vsadd(v_shuffle_index, const_value, MASKREAD_OFF, M);

  __v4096i8 seqSet_tmp;
  vclaim(seqSet_tmp);
  vshuffle(seqSet_tmp, v_shuffle_index, seqSet, SHUFFLE_GATHER, M);

  for (int i = 0; i < 32; ++i) {
    isScrambled[i] = 1;
  }

  jSFN = 0;
  for (int i = 0; i < 32; ++i) {
    if ((i > 0 && i <= 6) || (i >= 24 && i <= 27)) {
      if (i == 25 || i == 26)
        isScrambled[G[jSFN]] = 0;
      aBarShuffleIndex[i] = G[jSFN];
      jSFN++;
    } else if (i == 28) {
      aBarShuffleIndex[i]  = G[jHRF];
      isScrambled[G[jHRF]] = 0;
    } else if (i >= 29 && i <= 31) {
      if (Lssb == 64)
        isScrambled[G[jSSB]] = 0;
      aBarShuffleIndex[i] = G[jSSB];
      jSSB++;
    } else {
      aBarShuffleIndex[i] = G[jOTH];
      jOTH++;
    }
  }

  uint8_t cnt = 0;
  for (int i = 0; i < 32; ++i) {
    if (isScrambled[i] != 0) {
      seqShuffleIndex[cnt] = i;
      cnt++;
    }
  }

  __v2048i16 seqShuffleIndex_vec;
  vclaim(seqShuffleIndex_vec);
  int seqShuffleIndex_vec_addr = vaddr(seqShuffleIndex_vec);
  vbarrier();
  VSPM_OPEN();
  for (size_t i = 0; i < cnt; i++) {
    *(volatile short *)(seqShuffleIndex_vec_addr + (i << 1)) = seqShuffleIndex[i];
  }
  VSPM_CLOSE();

  __v2048i16 aBarShuffleIndex_vec;
  vclaim(aBarShuffleIndex_vec);
  int aBarShuffleIndex_vec_addr = vaddr(aBarShuffleIndex_vec);
  vbarrier();
  VSPM_OPEN();
  for (size_t i = 0; i < 32; i++) {
    *(volatile short *)(aBarShuffleIndex_vec_addr + (i << 1)) = aBarShuffleIndex[i];
  }
  VSPM_CLOSE();

  __v4096i8 seq;
  vclaim(seq);
  vbrdcst(seq, 0, MASKREAD_OFF, 32);
  vshuffle(seq, seqShuffleIndex_vec, seqSet_tmp, SHUFFLE_SCATTER, cnt);

  __v4096i8 trBlk;
  trBlk = vxor(scrBlk, seq, MASKREAD_OFF, 32);
  __v4096i8 trBlk1;
  vclaim(trBlk1);
  vshuffle(trBlk1, aBarShuffleIndex_vec, trBlk, SHUFFLE_GATHER, 32);

  return trBlk1;
}

typedef struct {
  short data;
} __attribute__((aligned(64))) short_struct;
int Task_nrPBCHCRC(__v4096i8 decoded, __v4096i8 poly, __v4096i8 seqSet, short_struct input_Lssb) {
  int E    = 864;
  int K    = 56;
  int N    = 512;
  int RNTI = 0;
  int Lssb = input_Lssb.data;

  __v4096i8 crcout = crc_24C(decoded, K, poly);

  short_struct crcBCH;
  short_struct sfn4lsb;
  short_struct nHalfFrame;
  short_struct msbidxoffset;

  crcBCH.data = 0;
  vbarrier();
  VSPM_OPEN();
  int decoded_addr = vaddr(decoded);
  int crcout_addr  = vaddr(crcout);
  for (int i = 0; i < 24; i++) {
    if (*(volatile char *)(decoded_addr + 32 + i) != *(volatile char *)(crcout_addr + i))
      crcBCH.data = 1;
  }
  VSPM_CLOSE();

  // printf("crcBCH:%hd\n", &crcBCH.data);
  __v4096i8 scrBlk = DescrambleAndDeInterleaverIndex(decoded, seqSet, Lssb);

  int scrBlk_addr = vaddr(scrBlk);
  sfn4lsb.data    = 0;
  vbarrier();
  VSPM_OPEN();
  for (int i = 0; i < 4; ++i)
    sfn4lsb.data += (*(volatile char *)(scrBlk_addr + 24 + i)) << (4 - i - 1);
  VSPM_CLOSE();

  scrBlk_addr = vaddr(scrBlk);
  vbarrier();
  VSPM_OPEN();
  nHalfFrame.data = (*(volatile char *)(scrBlk_addr + 28));
  VSPM_CLOSE();

  scrBlk_addr = vaddr(scrBlk);
  vbarrier();
  VSPM_OPEN();
  msbidxoffset.data = (*(volatile char *)(scrBlk_addr + 29));
  VSPM_CLOSE();

  vreturn(scrBlk, 32, &crcBCH, sizeof(crcBCH), &sfn4lsb, sizeof(sfn4lsb), &nHalfFrame, sizeof(nHalfFrame),
          &msbidxoffset, sizeof(msbidxoffset));
}