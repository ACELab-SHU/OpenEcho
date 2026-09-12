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
  
  int Task_ltePBCHIndices(__v2048i16 pbch_indices0, __v2048i16 pbch_indices1, __v2048i16 pbch_indices2, short_struct NCellID_in) {
      int NCellID = NCellID_in.data;
      int choose = NCellID % 3;
      __v2048i16 pbchIndices;
      vclaim(pbchIndices);
      __v2048i16 copy;
      vrange(copy,240);
      if (choose == 0)
      {
        /* code */
        vshuffle(pbchIndices,copy,pbch_indices0,SHUFFLE_GATHER,240);
      }
      else if(choose == 1){
        vshuffle(pbchIndices,copy,pbch_indices1,SHUFFLE_GATHER,240);
      }
      else{
        vshuffle(pbchIndices,copy,pbch_indices2,SHUFFLE_GATHER,240);
      }

      vreturn(pbchIndices,480);
      
  }
  