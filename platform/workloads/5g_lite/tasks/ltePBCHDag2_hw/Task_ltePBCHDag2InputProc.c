/**
  ******************************************************************************
  * @file           : Task_ltePBCHDag2InputProc.c
  * @author         : YihaoShen
  * @brief          : Process LTE PBCH Dag1 Return
  * @attention      : to get the hest0, hest1, hest2, hest3
  * @date           : 2025/05/06
  ******************************************************************************
  */

  #include "data_type.h"
  #include "riscv_printf.h"
  #include "venus.h"
  
  
  typedef short __v2048i16 __attribute__((ext_vector_type(2048)));
  typedef char __v4096i8 __attribute__((ext_vector_type(4096)));
  
  int Task_ltePBCHDag2InputProc(__v4096i8 hest_r,__v4096i8 hest_i) {
    __v2048i16 copy_index;
    vclaim(copy_index);
    vrange(copy_index, 240);

    __v4096i8 hest0_r;
    __v4096i8 hest0_i;
    vclaim(hest0_r);
    vclaim(hest0_i);
    vshuffle(hest0_r, copy_index, hest_r, SHUFFLE_GATHER, 240);
    vshuffle(hest0_i, copy_index, hest_i, SHUFFLE_GATHER, 240);
    copy_index = vadd(copy_index, 240, MASKREAD_OFF, 240);
    __v4096i8 hest1_r;
    __v4096i8 hest1_i;
    vclaim(hest1_r);
    vclaim(hest1_i);
    vshuffle(hest1_r, copy_index, hest_r, SHUFFLE_GATHER, 240);
    vshuffle(hest1_i, copy_index, hest_i, SHUFFLE_GATHER, 240);
    copy_index = vadd(copy_index, 240, MASKREAD_OFF, 240);
    __v4096i8 hest2_r;
    __v4096i8 hest2_i;
    vclaim(hest2_r);
    vclaim(hest2_i);
    vshuffle(hest2_r, copy_index, hest_r, SHUFFLE_GATHER, 240);
    vshuffle(hest2_i, copy_index, hest_i, SHUFFLE_GATHER, 240);
    copy_index = vadd(copy_index, 240, MASKREAD_OFF, 240);
    __v4096i8 hest3_r;
    __v4096i8 hest3_i;
    vclaim(hest3_r);
    vclaim(hest3_i);
    vshuffle(hest3_r, copy_index, hest_r, SHUFFLE_GATHER, 240);
    vshuffle(hest3_i, copy_index, hest_i, SHUFFLE_GATHER, 240);


    vreturn(hest0_r, 240, hest0_i, 240, hest1_r, 240, hest1_i, 240, hest2_r, 240, hest2_i, 240, hest3_r, 240, hest3_i, 240);
  }
  