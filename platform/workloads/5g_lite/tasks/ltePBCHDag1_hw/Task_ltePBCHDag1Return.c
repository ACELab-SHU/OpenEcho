/**
  ******************************************************************************
  * @file           : ltePBCHDag1Return.c
  * @author         : YihaoShen
  * @brief          : LTE PBCH Dag1 Return (reduce return variables to less than 8) 
  * @attention      : concat the hest0, hest1, hest2, hest3
  * @date           : 2025/05/06
  ******************************************************************************
  */

  #include "data_type.h"
  #include "riscv_printf.h"
  #include "venus.h"
  
  
  typedef short __v2048i16 __attribute__((ext_vector_type(2048)));
  typedef char __v4096i8 __attribute__((ext_vector_type(4096)));
  
  int Task_ltePBCHDag1Return(__v4096i8 rxgrid_r, __v4096i8 rxgrid_i,__v4096i8 hest0_r,__v4096i8 hest0_i,__v4096i8 hest1_r,__v4096i8 hest1_i,__v4096i8 hest2_r,__v4096i8 hest2_i,__v4096i8 hest3_r,__v4096i8 hest3_i, short_struct nCellID) {
    short ncellid = nCellID.data;
    __v2048i16 copy_index;
    vclaim(copy_index);
    vrange(copy_index, 240);

    __v4096i8 rxgrid_return_r;
    __v4096i8 rxgrid_return_i;
    vclaim(rxgrid_return_r);
    vclaim(rxgrid_return_i);
    vshuffle(rxgrid_return_r, copy_index, rxgrid_r, SHUFFLE_GATHER, 240);
    vshuffle(rxgrid_return_i, copy_index, rxgrid_i, SHUFFLE_GATHER, 240);

    __v4096i8 hest_return_r;
    __v4096i8 hest_return_i;
    vclaim(hest_return_r);
    vclaim(hest_return_i);
    vshuffle(hest_return_r, copy_index, hest0_r, SHUFFLE_SCATTER, 240);
    vshuffle(hest_return_i, copy_index, hest0_i, SHUFFLE_SCATTER, 240);
    copy_index = vadd(copy_index, 240, MASKREAD_OFF, 240);
    vshuffle(hest_return_r, copy_index, hest1_r, SHUFFLE_SCATTER, 240);
    vshuffle(hest_return_i, copy_index, hest1_i, SHUFFLE_SCATTER, 240);
    copy_index = vadd(copy_index, 240, MASKREAD_OFF, 240);
    vshuffle(hest_return_r, copy_index, hest2_r, SHUFFLE_SCATTER, 240);
    vshuffle(hest_return_i, copy_index, hest2_i, SHUFFLE_SCATTER, 240);
    copy_index = vadd(copy_index, 240, MASKREAD_OFF, 240);
    vshuffle(hest_return_r, copy_index, hest3_r, SHUFFLE_SCATTER, 240);
    vshuffle(hest_return_i, copy_index, hest3_i, SHUFFLE_SCATTER, 240);

    short_struct cellID;
    cellID.data = ncellid;


    vreturn(rxgrid_return_r, 240, rxgrid_return_i, 240, hest_return_r, 960, hest_return_i, 960, &cellID, sizeof(cellID));
  }
  