/**
 * ****************************************
 * @file        Task_inputSpilt.c
 * @brief       split input data(real & imag)
 * @author      yuanfeng
 * @date        2024.6.19
 * @copyright   ACE-Lab(Shanghai University)
 * ****************************************
 */

#include "riscv_printf.h"
#include "venus.h"

typedef char  __v8192i8 __attribute__((ext_vector_type(8192)));
typedef short __v4096i16 __attribute__((ext_vector_type(4096)));

int symbolLength = 2192;

int Task_inputSplit(__v8192i8 dfe_input_0, __v8192i8 dfe_input_1, __v8192i8 dfe_input_2, __v8192i8 dfe_input_3) {

  // generate shuffle index
  __v4096i16 shuffle_index;
  vclaim(shuffle_index);
  vrange(shuffle_index, symbolLength);
  shuffle_index = vmul(shuffle_index, 2, MASKREAD_OFF, symbolLength);
  shuffle_index = vadd(shuffle_index, 4, MASKREAD_OFF, symbolLength);

  __v8192i8 dfe_output_real_0;
  __v8192i8 dfe_output_real_1;
  __v8192i8 dfe_output_real_2;
  __v8192i8 dfe_output_real_3;
  __v8192i8 dfe_output_imag_0;
  __v8192i8 dfe_output_imag_1;
  __v8192i8 dfe_output_imag_2;
  __v8192i8 dfe_output_imag_3;
  vclaim(dfe_output_real_0);
  vclaim(dfe_output_real_1);
  vclaim(dfe_output_real_2);
  vclaim(dfe_output_real_3);
  vclaim(dfe_output_imag_0);
  vclaim(dfe_output_imag_1);
  vclaim(dfe_output_imag_2);
  vclaim(dfe_output_imag_3);

  // extract real data
  vshuffle(dfe_output_real_0, shuffle_index, dfe_input_0, SHUFFLE_GATHER, symbolLength);
  vshuffle(dfe_output_real_1, shuffle_index, dfe_input_1, SHUFFLE_GATHER, symbolLength);
  vshuffle(dfe_output_real_2, shuffle_index, dfe_input_2, SHUFFLE_GATHER, symbolLength);
  vshuffle(dfe_output_real_3, shuffle_index, dfe_input_3, SHUFFLE_GATHER, symbolLength);

  // extract imag data
  shuffle_index = vsadd(shuffle_index, 1, MASKREAD_OFF, symbolLength);
  vshuffle(dfe_output_imag_0, shuffle_index, dfe_input_0, SHUFFLE_GATHER, symbolLength);
  vshuffle(dfe_output_imag_1, shuffle_index, dfe_input_1, SHUFFLE_GATHER, symbolLength);
  vshuffle(dfe_output_imag_2, shuffle_index, dfe_input_2, SHUFFLE_GATHER, symbolLength);
  vshuffle(dfe_output_imag_3, shuffle_index, dfe_input_3, SHUFFLE_GATHER, symbolLength);

  vreturn(dfe_output_real_0, symbolLength, dfe_output_real_1, symbolLength, dfe_output_real_2, symbolLength,
          dfe_output_real_3, symbolLength, dfe_output_imag_0, symbolLength, dfe_output_imag_1, symbolLength,
          dfe_output_imag_2, symbolLength, dfe_output_imag_3, symbolLength);
}
