/**
  ******************************************************************************
  * @file           : Task_ltePDCCHDemodulation.c
  * @author         : XiaoxiaoChen
  * @brief          : PBCH Demodulation(QPSK) == same as nrPBCHDemodulation
  * @attention      : only QPSK be applied for PBCH(determined in 3GPP 36.211 6.6.2)
  * @date           : 2024/11/11
  ******************************************************************************
  */

  #include "riscv_printf.h"
  #include "venus.h"
  #include "stdint.h"
  #include "data_type.h"
  #include "vmath.h"

typedef short __v2048i16 __attribute__((ext_vector_type(2048)));
typedef char  __v4096i8 __attribute__((ext_vector_type(4096)));

int Task_ltePDCCHDemodulation(__v4096i8 inSignal_real, __v4096i8 inSignal_imag, short_struct input_sequence_length) {
  /*--------------------QPSK Demodulate--------------------*/
  short rxSignalLength = input_sequence_length.data;
  short softBitlLength = rxSignalLength * 2;

  __v4096i8  softbit;
  __v2048i16 softbit_shuffle_index_tmp;
  vclaim(softbit_shuffle_index_tmp);
  vclaim(softbit);
  vrange(softbit_shuffle_index_tmp, rxSignalLength);

  __v4096i8 temp_one;
  __v4096i8 temp_255;
  vclaim(temp_one);
  vclaim(temp_255);
  vbrdcst(temp_one, 1, MASKREAD_OFF, softBitlLength);
  vbrdcst(temp_255, 0xFF, MASKREAD_OFF, softBitlLength);

  softbit_shuffle_index_tmp = vmul(softbit_shuffle_index_tmp, 2, MASKREAD_OFF, rxSignalLength);
  vshuffle(softbit, softbit_shuffle_index_tmp, inSignal_real, SHUFFLE_SCATTER, rxSignalLength);
  softbit_shuffle_index_tmp = vadd(softbit_shuffle_index_tmp, 1, MASKREAD_OFF, rxSignalLength);
  vshuffle(softbit, softbit_shuffle_index_tmp, inSignal_imag, SHUFFLE_SCATTER, rxSignalLength);
  softbit = vxor(softbit,temp_255,MASKREAD_OFF,softBitlLength);
  softbit = vsadd(softbit, temp_one, MASKREAD_OFF,softBitlLength);

  short_struct softBitlLength_out;
  softBitlLength_out.data = softBitlLength;

  vreturn(softbit,softBitlLength,&softBitlLength_out,sizeof(short));
}