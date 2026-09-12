/**
  ******************************************************************************
  * @file           : ltepcfichIndices.c
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
  //__v2048i16 pcfichIndices,short_struct nDLRB,__v4096i8 rxdata_r,__v4096i8 rxdata_i, __v4096i8 hest0,__v4096i8 hest1,__v4096i8 hest2,__v4096i8 hest3
  // int Task_ltePCFICHExtract(__v2048i16 pcfichIndices,__v4096i8 rxdata_r,__v4096i8 rxdata_i, __v4096i8 hest0_r,__v4096i8 hest0_i,__v4096i8 hest1_r,__v4096i8 hest1_i,__v4096i8 hest2_r,__v4096i8 hest2_i,__v4096i8 hest3_r,__v4096i8 hest3_i) {  // int Task_ltePCFICHExtract(__v2048i16 pcfichIndices,__v4096i8 rxdata_r,__v4096i8 rxdata_i, __v4096i8 hest0_r,__v4096i8 hest0_i,__v4096i8 hest1_r,__v4096i8 hest1_i,__v4096i8 hest2_r,__v4096i8 hest2_i,__v4096i8 hest3_r,__v4096i8 hest3_i) {
  int Task_ltePCFICHExtract(__v2048i16 pcfichIndices,__v4096i8 rxdata_r,__v4096i8 rxdata_i, __v4096i8 hest0_r,__v4096i8 hest0_i,__v4096i8 hest1_r,__v4096i8 hest1_i,__v4096i8 hest2_r,__v4096i8 hest2_i,__v4096i8 hest3_r,__v4096i8 hest3_i) {
    
    // short RB = nDLRB.data;
    // __v4096i8 pcfichIndices_i;
    // vclaim(pcfichIndices_i);
    // pcfichIndices_i = vsadd(pcfichIndices, RB*12*14, MASKREAD_OFF, 16);
  
    __v4096i8 rxdata16_r;
    __v4096i8 rxdata16_i;
    vclaim(rxdata16_r);
    vclaim(rxdata16_i);
    vshuffle(rxdata16_r,pcfichIndices,rxdata_r,SHUFFLE_GATHER,16);
    vshuffle(rxdata16_i,pcfichIndices,rxdata_i,SHUFFLE_GATHER,16);

    __v4096i8 hest_r0;
    __v4096i8 hest_i0;
    vclaim(hest_r0);
    vclaim(hest_i0);
    // vshuffle(hest_r0,pcfichIndices,hest0,SHUFFLE_GATHER,16);
    // vshuffle(hest_i0,pcfichIndices_i,hest0,SHUFFLE_GATHER,16);
    vshuffle(hest_r0,pcfichIndices,hest0_r,SHUFFLE_GATHER,16);
    vshuffle(hest_i0,pcfichIndices,hest0_i,SHUFFLE_GATHER,16);

    __v4096i8 hest_r1;
    __v4096i8 hest_i1;
    vclaim(hest_r1);
    vclaim(hest_i1);
    // vshuffle(hest_r1,pcfichIndices,hest1,SHUFFLE_GATHER,16);
    // vshuffle(hest_i1,pcfichIndices_i,hest1,SHUFFLE_GATHER,16);
    vshuffle(hest_r1,pcfichIndices,hest1_r,SHUFFLE_GATHER,16);
    vshuffle(hest_i1,pcfichIndices,hest1_i,SHUFFLE_GATHER,16);
    
    __v4096i8 hest_r2;
    __v4096i8 hest_i2;
    vclaim(hest_r2);
    vclaim(hest_i2);
    // vshuffle(hest_r2,pcfichIndices,hest2,SHUFFLE_GATHER,16);
    // vshuffle(hest_i2,pcfichIndices_i,hest2,SHUFFLE_GATHER,16);
    vshuffle(hest_r2,pcfichIndices,hest2_r,SHUFFLE_GATHER,16);
    vshuffle(hest_i2,pcfichIndices,hest2_i,SHUFFLE_GATHER,16);

    __v4096i8 hest_r3;
    __v4096i8 hest_i3;
    vclaim(hest_r3);
    vclaim(hest_i3);
    // vshuffle(hest_r3,pcfichIndices,hest3,SHUFFLE_GATHER,16);
    // vshuffle(hest_i3,pcfichIndices_i,hest3,SHUFFLE_GATHER,16);
    vshuffle(hest_r3,pcfichIndices,hest3_r,SHUFFLE_GATHER,16);
    vshuffle(hest_i3,pcfichIndices,hest3_i,SHUFFLE_GATHER,16);

    vreturn(rxdata16_r,16,rxdata16_i,16,hest_r0,16,hest_i0,16,hest_r1,16,hest_i1,16,hest_r2,16,hest_i2,16,hest_r3,16,hest_i3,16);
  }
  