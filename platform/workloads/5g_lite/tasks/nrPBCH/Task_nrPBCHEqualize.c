/**
 * ****************************************
 * @file        Task_nrEqualize.c
 * @brief       equalization using ZF
 * @author      yuanfeng
 * @date        2024.6.6
 * @copyright   ACE-Lab(Shanghai University)
 * ****************************************
 */

#include "riscv_printf.h"
#include "venus.h"

#define PBCH_LENGTH       432
#define TOTAL_DATA_LENGTH 720

typedef short __v2048i16 __attribute__((ext_vector_type(2048)));
typedef short __v4096i16 __attribute__((ext_vector_type(4096)));
typedef char  __v4096i8 __attribute__((ext_vector_type(4096)));

typedef struct {
  short data;
} __attribute__((aligned(64))) short_struct;

int Task_nrPBCHEqualize(__v4096i8 rxData_real0, __v4096i8 rxData_imag0, __v4096i8 H_real, __v4096i8 H_imag,
                        __v2048i16 pbch_index, __v2048i16 rxData_shuffle_index) {

  int fractionLength = 6;
  int nVar           = 0;

  /*--------------------ExtractResources--------------------*/
  __v4096i8 rxData_real;
  __v4096i8 rxData_imag;
  vclaim(rxData_real);
  vclaim(rxData_imag);
  vshuffle(rxData_real, rxData_shuffle_index, rxData_real0, SHUFFLE_GATHER, 720);
  vshuffle(rxData_imag, rxData_shuffle_index, rxData_imag0, SHUFFLE_GATHER, 720);

  __v4096i8 rxSym_real;
  __v4096i8 rxSym_imag;

  vclaim(rxSym_real);
  vclaim(rxSym_imag);
  vshuffle(rxSym_real, pbch_index, rxData_real, SHUFFLE_GATHER, PBCH_LENGTH);
  vshuffle(rxSym_imag, pbch_index, rxData_imag, SHUFFLE_GATHER, PBCH_LENGTH);

  __v4096i8 Hest_real;
  __v4096i8 Hest_imag;

  vclaim(Hest_real);
  vclaim(Hest_imag);
  vshuffle(Hest_real, pbch_index, H_real, SHUFFLE_GATHER, PBCH_LENGTH);
  vshuffle(Hest_imag, pbch_index, H_imag, SHUFFLE_GATHER, PBCH_LENGTH);

  /*--------------------Equalization--------------------*/
  __v4096i8 csi;
  __v4096i8 csi_tmp1;
  __v4096i8 csi_tmp2;

  vsetshamt(fractionLength);
  csi_tmp1 = vmul(Hest_real, Hest_real, MASKREAD_OFF, PBCH_LENGTH);
  csi_tmp2 = vmul(Hest_imag, Hest_imag, MASKREAD_OFF, PBCH_LENGTH);
  vsetshamt(0);

  csi = vsadd(csi_tmp1, csi_tmp2, MASKREAD_OFF, PBCH_LENGTH);
  csi = vsadd(csi, nVar, MASKREAD_OFF, PBCH_LENGTH);

  __v4096i8 pbchEq_real;
  __v4096i8 pbchEq_imag;
  __v4096i8 pbchEq_tmp1;
  __v4096i8 pbchEq_tmp2;
  __v4096i8 pbchEq_tmp3;
  __v4096i8 pbchEq_tmp4;

  vsetshamt(fractionLength);
  Hest_real = vdiv(csi, Hest_real, MASKREAD_OFF, PBCH_LENGTH);
  Hest_imag = vdiv(csi, Hest_imag, MASKREAD_OFF, PBCH_LENGTH);
  vsetshamt(0);

  vsetshamt(fractionLength);
  pbchEq_tmp1 = vmul(Hest_real, rxSym_real, MASKREAD_OFF, PBCH_LENGTH);
  pbchEq_tmp2 = vmul(Hest_imag, rxSym_imag, MASKREAD_OFF, PBCH_LENGTH);
  vsetshamt(0);
  pbchEq_real = vsadd(pbchEq_tmp1, pbchEq_tmp2, MASKREAD_OFF, PBCH_LENGTH);

  vsetshamt(fractionLength);
  pbchEq_tmp3 = vmul(Hest_real, rxSym_imag, MASKREAD_OFF, PBCH_LENGTH);
  pbchEq_tmp4 = vmul(Hest_imag, rxSym_real, MASKREAD_OFF, PBCH_LENGTH);
  vsetshamt(0);
  pbchEq_imag = vssub(pbchEq_tmp4, pbchEq_tmp3, MASKREAD_OFF, PBCH_LENGTH);

  vreturn(pbchEq_real, PBCH_LENGTH, pbchEq_imag, PBCH_LENGTH, csi, PBCH_LENGTH);
}