/**
 * ****************************************
 * @file        Task_inputSpilt.c
 * @brief       split input data(real & imag)
 * @author      yuanfeng
 * @date        2024.6.19
 * @copyright   ACE-Lab(Shanghai University)
 * ****************************************
 */

#include "data_type.h"
#include "riscv_printf.h"
#include "venus.h"

typedef char  __v8192i8 __attribute__((ext_vector_type(8192)));
typedef short __v4096i16 __attribute__((ext_vector_type(4096)));

int Task_inputSplit(__v8192i8 dfe_input, short_struct subCarrierSpace, short_struct symbolNum) {

  short scs        = subCarrierSpace.data;
  short symbol_num = symbolNum.data;

  int header_length = 4;
  int N_FFT         = 0;
  int cp_length     = 0;
  int symbolLength  = 0;

  if (scs == 15) {
    N_FFT = 2048;
    if (symbol_num == 0 || symbol_num == 7) {
      cp_length = 160;
    } else {
      cp_length = 144;
    }
  } else if (scs == 30) {
    N_FFT = 1024;
    if (symbol_num == 0) {
      cp_length = 88;
    } else {
      cp_length = 72;
    }
  }
  symbolLength = N_FFT + cp_length;

  if ((symbol_num <= 13) && (symbol_num >= 0)) {
    // generate shuffle index
    __v4096i16 shuffle_index;
    vclaim(shuffle_index);
    vrange(shuffle_index, symbolLength);
    shuffle_index = vmul(shuffle_index, 2, MASKREAD_OFF, symbolLength);
    shuffle_index = vsadd(shuffle_index, header_length, MASKREAD_OFF, symbolLength);

    __v8192i8 dfe_output_real;
    __v8192i8 dfe_output_imag;
    vclaim(dfe_output_real);
    vclaim(dfe_output_imag);

    // extract real data
    vshuffle(dfe_output_real, shuffle_index, dfe_input, SHUFFLE_GATHER, symbolLength);

    // extract imag data
    shuffle_index = vsadd(shuffle_index, 1, MASKREAD_OFF, symbolLength);
    vshuffle(dfe_output_imag, shuffle_index, dfe_input, SHUFFLE_GATHER, symbolLength);

    vreturn(dfe_output_real, symbolLength, dfe_output_imag, symbolLength);
    // vreturn(dfe_output_real, sizeof(dfe_output_real), dfe_output_imag, sizeof(dfe_output_imag));
  } else {

    __v8192i8 dfe_output_real;
    __v8192i8 dfe_output_imag;
    vclaim(dfe_output_real);
    vclaim(dfe_output_imag);
    vbrdcst(dfe_output_real, 0, MASKREAD_OFF, 1);
    vbrdcst(dfe_output_imag, 0, MASKREAD_OFF, 1);
    vreturn(dfe_output_real, symbolLength, dfe_output_imag, symbolLength);
    // vreturn(dfe_output_real, sizeof(dfe_output_real), dfe_output_imag, sizeof(dfe_output_imag));
  }
}
