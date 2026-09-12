/**
  ******************************************************************************
  * @file           : Task_ltePDCCHExtract.c
  * @author         : chenxiaoxiao
  * @brief          : LTE PDCCH Indices Generation
  * @attention      : Indices start from 0 of a slot
  * @date           : 2024/11/11
  ******************************************************************************
  */

  #include "data_type.h"
  #include "riscv_printf.h"
  #include "venus.h"
  
  
  typedef short __v4800i16 __attribute__((ext_vector_type(4800)));
  typedef char __v9600i8 __attribute__((ext_vector_type(9600)));
  
  int Task_ltePDCCHExtract(__v4800i16 pdcchIndices, short_struct pdcchlength, __v9600i8 rxdata_r,__v9600i8 rxdata_i, __v9600i8 hest0_r,__v9600i8 hest0_i,__v9600i8 hest1_r,__v9600i8 hest1_i,__v9600i8 hest2_r,__v9600i8 hest2_i,__v9600i8 hest3_r,__v9600i8 hest3_i) {

    int pdcchlength_data = pdcchlength.data;
    __v9600i8 rxdata_real;
    __v9600i8 rxdata_imag;
    vclaim(rxdata_real);
    vclaim(rxdata_imag);
    vshuffle(rxdata_real,pdcchIndices,rxdata_r,SHUFFLE_GATHER,pdcchlength_data);
    vshuffle(rxdata_imag,pdcchIndices,rxdata_i,SHUFFLE_GATHER,pdcchlength_data);
    
    __v9600i8 hest_r0;
    __v9600i8 hest_i0;
    vclaim(hest_r0);
    vclaim(hest_i0);
    vshuffle(hest_r0,pdcchIndices,hest0_r,SHUFFLE_GATHER,pdcchlength_data);
    vshuffle(hest_i0,pdcchIndices,hest0_i,SHUFFLE_GATHER,pdcchlength_data);

    __v9600i8 hest_r1;
    __v9600i8 hest_i1;
    vclaim(hest_r1);
    vclaim(hest_i1);
    vshuffle(hest_r1,pdcchIndices,hest1_r,SHUFFLE_GATHER,pdcchlength_data);
    vshuffle(hest_i1,pdcchIndices,hest1_i,SHUFFLE_GATHER,pdcchlength_data);
    
    __v9600i8 hest_r2;
    __v9600i8 hest_i2;
    vclaim(hest_r2);
    vclaim(hest_i2);
    vshuffle(hest_r2,pdcchIndices,hest2_r,SHUFFLE_GATHER,pdcchlength_data);
    vshuffle(hest_i2,pdcchIndices,hest2_i,SHUFFLE_GATHER,pdcchlength_data);

    __v9600i8 hest_r3;
    __v9600i8 hest_i3;
    vclaim(hest_r3);
    vclaim(hest_i3);
    vshuffle(hest_r3,pdcchIndices,hest3_r,SHUFFLE_GATHER,pdcchlength_data);
    vshuffle(hest_i3,pdcchIndices,hest3_i,SHUFFLE_GATHER,pdcchlength_data);

    vreturn(rxdata_real,pdcchlength_data,rxdata_imag,pdcchlength_data,hest_r0,pdcchlength_data,hest_i0,pdcchlength_data,hest_r1,pdcchlength_data,hest_i1,pdcchlength_data,hest_r2,pdcchlength_data,hest_i2,pdcchlength_data,hest_r3,pdcchlength_data,hest_i3,pdcchlength_data);
  }
  