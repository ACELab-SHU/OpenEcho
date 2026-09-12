/**
 * ****************************************
 * @file        Task_nrEqualize.c
 * @brief       equalization using ZF
 * @author      yuanfeng
 * @date        2024.7.25
 * @copyright   ACE-Lab(Shanghai University)
 * ****************************************
 */

#include "data_type.h"
#include "riscv_printf.h"
#include "venus.h"

typedef short __v6144i16 __attribute__((ext_vector_type(6144)));
typedef char  __v12288i8 __attribute__((ext_vector_type(12288)));

// typedef struct {
//   short data;
// } __attribute__((aligned(64))) short_struct;

int Task_nrEqualize(__v12288i8 rxData_real0, __v12288i8 rxData_imag0, __v12288i8 H_real, __v12288i8 H_imag,
                    __v6144i16 pdcch_index, __v6144i16 rxData_shuffle_index, short_struct input_sequence_length,
                    short_struct input_nrbLength, short_struct input_data_symbol_length) __attribute__((aligned(64))) {

  int   sequence_length    = input_sequence_length.data;
  short subcarrierLength   = input_nrbLength.data * 12;
  short data_symbol_length = input_data_symbol_length.data;

  int fractionLength = 6;
  int nVar           = 0;

  /*--------------------ExtractResources--------------------*/
  __v12288i8 rxData_real;
  __v12288i8 rxData_imag;
  vclaim(rxData_real);
  vclaim(rxData_imag);
  vshuffle(rxData_real, rxData_shuffle_index, rxData_real0, SHUFFLE_GATHER, subcarrierLength * data_symbol_length);
  vshuffle(rxData_imag, rxData_shuffle_index, rxData_imag0, SHUFFLE_GATHER, subcarrierLength * data_symbol_length);

  __v12288i8 rxSym_real;
  __v12288i8 rxSym_imag;

  vclaim(rxSym_real);
  vclaim(rxSym_imag);
  vshuffle(rxSym_real, pdcch_index, rxData_real, SHUFFLE_GATHER, sequence_length);
  vshuffle(rxSym_imag, pdcch_index, rxData_imag, SHUFFLE_GATHER, sequence_length);

  __v12288i8 Hest_real;
  __v12288i8 Hest_imag;

  vclaim(Hest_real);
  vclaim(Hest_imag);
  vshuffle(Hest_real, pdcch_index, H_real, SHUFFLE_GATHER, sequence_length);
  vshuffle(Hest_imag, pdcch_index, H_imag, SHUFFLE_GATHER, sequence_length);

  /*--------------------Equalization--------------------*/
  __v12288i8 csi;
  __v12288i8 csi_tmp1;
  __v12288i8 csi_tmp2;

  vsetshamt(fractionLength);
  csi_tmp1 = vmul(Hest_real, Hest_real, MASKREAD_OFF, sequence_length);
  csi_tmp2 = vmul(Hest_imag, Hest_imag, MASKREAD_OFF, sequence_length);
  vsetshamt(0);

  csi = vsadd(csi_tmp1, csi_tmp2, MASKREAD_OFF, sequence_length);
  csi = vsadd(csi, nVar, MASKREAD_OFF, sequence_length);

  __v12288i8 pdcchEq_real;
  __v12288i8 pdcchEq_imag;
  __v12288i8 pdcchEq_tmp1;
  __v12288i8 pdcchEq_tmp2;
  __v12288i8 pdcchEq_tmp3;
  __v12288i8 pdcchEq_tmp4;

  vsetshamt(fractionLength);
  Hest_real = vdiv(csi, Hest_real, MASKREAD_OFF, sequence_length);
  Hest_imag = vdiv(csi, Hest_imag, MASKREAD_OFF, sequence_length);
  vsetshamt(0);

  vsetshamt(fractionLength);
  pdcchEq_tmp1 = vmul(Hest_real, rxSym_real, MASKREAD_OFF, sequence_length);
  pdcchEq_tmp2 = vmul(Hest_imag, rxSym_imag, MASKREAD_OFF, sequence_length);
  vsetshamt(0);
  pdcchEq_real = vsadd(pdcchEq_tmp1, pdcchEq_tmp2, MASKREAD_OFF, sequence_length);

  vsetshamt(fractionLength);
  pdcchEq_tmp3 = vmul(Hest_real, rxSym_imag, MASKREAD_OFF, sequence_length);
  pdcchEq_tmp4 = vmul(Hest_imag, rxSym_real, MASKREAD_OFF, sequence_length);
  vsetshamt(0);
  pdcchEq_imag = vssub(pdcchEq_tmp4, pdcchEq_tmp3, MASKREAD_OFF, sequence_length);

  vreturn(pdcchEq_real, sizeof(pdcchEq_real), pdcchEq_imag, sizeof(pdcchEq_imag), csi, sizeof(csi));
}