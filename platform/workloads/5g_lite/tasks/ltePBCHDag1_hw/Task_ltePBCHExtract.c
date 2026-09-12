/**
  ******************************************************************************
  * @file           : ltePBCHIndices.c
  * @author         : YihaoShen
  * @brief          : LTE PBCH Indices Generation
  * @attention      : Indices start from 0 of a slot
  * @date           : 2024/11/11
  ******************************************************************************
  */

  #include "data_type.h"
  #include "riscv_printf.h"
  #include "venus.h"
  
  
  typedef short __v2048i16 __attribute__((ext_vector_type(2048)));
  typedef char __v4096i8 __attribute__((ext_vector_type(4096)));
  
  int Task_ltePBCHExtract(__v2048i16 pbchIndices,__v4096i8 rxdata_r,__v4096i8 rxdata_i, __v4096i8 hest0_r,__v4096i8 hest0_i,__v4096i8 hest1_r,__v4096i8 hest1_i,__v4096i8 hest2_r,__v4096i8 hest2_i,__v4096i8 hest3_r,__v4096i8 hest3_i) {



    __v4096i8 rxdata240_r;
    __v4096i8 rxdata240_i;
    vclaim(rxdata240_r);
    vclaim(rxdata240_i);
    vshuffle(rxdata240_r,pbchIndices,rxdata_r,SHUFFLE_GATHER,240);
    vshuffle(rxdata240_i,pbchIndices,rxdata_i,SHUFFLE_GATHER,240);


    __v4096i8 hest_r0;
    __v4096i8 hest_i0;
    vclaim(hest_r0);
    vclaim(hest_i0);
    vshuffle(hest_r0,pbchIndices,hest0_r,SHUFFLE_GATHER,240);
    vshuffle(hest_i0,pbchIndices,hest0_i,SHUFFLE_GATHER,240);

    __v4096i8 hest_r1;
    __v4096i8 hest_i1;
    vclaim(hest_r1);
    vclaim(hest_i1);
    vshuffle(hest_r1,pbchIndices,hest1_r,SHUFFLE_GATHER,240);
    vshuffle(hest_i1,pbchIndices,hest1_i,SHUFFLE_GATHER,240);
    
    __v4096i8 hest_r2;
    __v4096i8 hest_i2;
    vclaim(hest_r2);
    vclaim(hest_i2);
    vshuffle(hest_r2,pbchIndices,hest2_r,SHUFFLE_GATHER,240);
    vshuffle(hest_i2,pbchIndices,hest2_i,SHUFFLE_GATHER,240);

    __v4096i8 hest_r3;
    __v4096i8 hest_i3;
    vclaim(hest_r3);
    vclaim(hest_i3);
    vshuffle(hest_r3,pbchIndices,hest3_r,SHUFFLE_GATHER,240);
    vshuffle(hest_i3,pbchIndices,hest3_i,SHUFFLE_GATHER,240);

    vreturn(rxdata240_r,240,rxdata240_i,240,hest_r0,240,hest_i0,240,hest_r1,240,hest_i1,240,hest_r2,240,hest_i2,240,hest_r3,240,hest_i3,240);
  }
  