/**
 * ****************************************
 * @file        Task_nrRateRecoverPolar.c
 * @brief       Rate Recover for Polar Decode
 * @author      yuanfeng
 * @date        2024.7.26
 * @copyright   ACE-Lab(Shanghai University)
 * ****************************************
 */

#include "data_type.h"
#include "riscv_printf.h"
#include "venus.h"

typedef short __v2048i16 __attribute__((ext_vector_type(2048)));
typedef char  __v4096i8 __attribute__((ext_vector_type(4096)));

VENUS_INLINE short ceillog2_Venus(short in) {
  short out  = -1;
  short temp = in;
  while (temp > 0) {
    temp = temp >> 1;
    out  = out + 1;
  }
  if (in > (1 << out)) {
    out = out + 1;
  }
  return out;
}

VENUS_INLINE short polarGetN(short K, short E, uint8_t nMax) {
  // if (K >= E)
  // {
  //     printf("\n\nError!! K must smaller than E:%hd\n\n", &K);
  // }
  short   Ceil_Log2_E = ceillog2_Venus(E);
  uint8_t n1          = 0;
  if ((E <= (9.0 / 8.0) * (1 << (Ceil_Log2_E - 1)) && ((K * 1.0) / E < 9.0 / 16.0))) {
    n1 = Ceil_Log2_E - 1;
  } else {
    n1 = Ceil_Log2_E;
  }

  uint8_t n2    = ceillog2_Venus(8 * K);
  uint8_t nMin  = 5;
  uint8_t n     = 0;
  uint8_t min_n = n1;
  if (n2 < min_n) {
    min_n = n2;
  }
  if (nMax < min_n) {
    min_n = nMax;
  }
  if (min_n >= nMin) {
    n = min_n;
  } else {
    n = nMin;
  }
  uint16_t N_temp = 1 << n;
  return N_temp;
}

int Task_nrRateRecoverPolar(__v4096i8 vin, short_struct input_E, short_struct input_K, short_struct input_N,
                            __v2048i16 pi) {
  uint16_t E = input_E.data;
  uint16_t K = input_K.data;
  uint16_t N = input_N.data;
  // uint16_t N = polarGetN(K, E, 9);

  __v4096i8  vout;
  __v2048i16 index;
  __v2048i16 jn;
  __v2048i16 ii;
  __v2048i16 n;

  vclaim(vout);
  vclaim(index);
  vclaim(jn);
  vclaim(ii);
  vclaim(n);

  uint16_t N_32    = (N >> 5);
  uint16_t logN_32 = 0;

  for (int i = N_32; i > 1; i >>= 1)
    logN_32++;

  if (E >= N) {
    vout = vsadd(vin, 0, MASKREAD_OFF, N);
  } else {
    if ((K * 64 / E) <= 28) {
      vrange(index, E);
      index = vsadd(index, N - E, MASKREAD_OFF, E);
      vbrdcst(vout, 0, MASKREAD_OFF, N);
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

  vreturn(vout1, sizeof(vout));
}