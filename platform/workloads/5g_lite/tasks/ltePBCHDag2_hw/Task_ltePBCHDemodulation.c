/**
  ******************************************************************************
  * @file           : Task_ltePBCHDemodulation.c
  * @author         : YihaoShen
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

// short         rxSignalLength           = 240;
// short         softBitlLength           = 480;
// char input_inSignal_real[240] = {-10,32,-7,-20,19,-12,8,11,28,27,10,-24,11,-14,-20,39,4,-25,8,-3,6,-13,16,3,-36,-12,23,-6,35,-30,51,-11,10,10,66,1,2,-41,-19,-5,-24,52,16,-17,12,-7,-18,10,8,-5,2,13,-22,16,-22,20,41,-4,4,-10,19,-5,-26,10,-3,-11,0,-23,11,-20,-13,-47,8,-12,-18,3,-13,-10,-7,27,30,-4,40,-11,-23,26,-2,-27,42,15,-25,-16,-7,0,-32,10,-6,-9,14,3,-31,-17,-15,5,16,41,17,26,-13,18,-13,18,15,-5,-11,-5,-46,-27,12,6,25,-26,3,37,12,-42,0,-7,-7,-17,-18,-3,-15,46,-14,17,10,-6,5,-48,-16,-55,1,-34,-17,-17,16,38,-20,12,12,6,26,-23,2,1,29,6,7,-24,-32,-26,5,10,-26,-29,18,-17,15,-34,-8,8,7,-15,-8,-5,1,14,28,2,27,-15,1,16,46,44,-11,-17,-30,33,1,-20,0,-7,-18,14,-15,-21,10,-24,21,-31,-3,-16,13,-12,-11,15,5,44,6,10,3,-24,-10,13,-40,-3,-1,6,15,10,-7,-32,-16,23,59,2,9,-57,37,-51,-27,-2,-52,22,-40,30,-20,5};
// char input_inSignal_imag[432] = {-6,-11,-18,8,11,-35,-32,11,3,-50,9,4,8,58,9,5,6,2,-20,-11,1,4,30,14,-9,-18,-32,-1,-11,21,19,22,-10,-29,18,-5,5,35,-12,42,-4,23,-28,-1,15,-30,19,-2,10,7,19,9,2,4,-24,15,11,-12,20,-3,33,-12,8,14,-13,-6,-12,17,6,2,-1,-1,6,23,-12,-19,-16,17,-9,-3,2,26,-1,20,24,38,30,23,-1,5,-12,-44,42,-8,2,20,5,-17,-3,-4,-24,-18,6,30,29,-15,-1,-22,21,-3,-1,30,-13,62,-6,1,9,-21,-7,4,8,23,9,-1,26,-9,41,14,13,-23,-4,36,28,24,17,23,19,13,20,-12,-29,-6,-6,7,-7,-4,44,-1,0,38,-4,45,17,20,-7,-28,17,-25,-17,-36,22,33,17,-18,6,10,16,-29,20,5,-3,15,29,-7,14,-10,-5,-1,4,41,-14,29,-44,-24,28,30,-19,-16,21,24,6,-8,8,16,-28,20,25,-16,-11,-15,21,13,-8,18,-15,-29,32,3,-19,-8,-23,27,8,-1,-9,11,4,-4,-4,27,14,20,16,-9,-14,-21,34,-4,-29,22,-24,-23,-38,19,-21,43,-12,-15,-20,34};

int Task_ltePBCHDemodulation(__v4096i8 inSignal_real, __v4096i8 inSignal_imag, short_struct input_sequence_length, short_struct demod_length) {
  /*--------------------QPSK Demodulate--------------------*/
  short rxSignalLength = input_sequence_length.data;
  short softBitlLength = demod_length.data;
  
  __v4096i8  softbit;
  __v2048i16 softbit_shuffle_index_tmp;
  vclaim(softbit_shuffle_index_tmp);
  vclaim(softbit);
  vrange(softbit_shuffle_index_tmp, rxSignalLength);
  softbit_shuffle_index_tmp = vmul(softbit_shuffle_index_tmp, 2, MASKREAD_OFF, rxSignalLength);
  // vshuffle(softbit, softbit_shuffle_index_tmp, softbit1, SHUFFLE_SCATTER, rxSignalLength);
  vshuffle(softbit, softbit_shuffle_index_tmp, inSignal_real, SHUFFLE_SCATTER, rxSignalLength);
  softbit_shuffle_index_tmp = vadd(softbit_shuffle_index_tmp, 1, MASKREAD_OFF, rxSignalLength);
  // vshuffle(softbit, softbit_shuffle_index_tmp, softbit2, SHUFFLE_SCATTER, rxSignalLength);
  vshuffle(softbit, softbit_shuffle_index_tmp, inSignal_imag, SHUFFLE_SCATTER, rxSignalLength);
  softbit = vmul(softbit, -1, MASKREAD_OFF, softBitlLength);
  //VENUS_PRINTVEC_CHAR(softbit, softBitlLength);

  vreturn(softbit,softBitlLength);
}