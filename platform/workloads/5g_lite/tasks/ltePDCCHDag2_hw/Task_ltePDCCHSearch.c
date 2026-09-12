/**
 * ****************************************
 * @file        Task_lteDmrsIndices.c
 * @brief       LTE Dmrs Generation
 * @author      chenxiaoxiao
 * @date        2025.3.20
 * @copyright   ACE-Lab(Shanghai University)
 * ****************************************
 */

 #include "data_type.h"
 #include "riscv_printf.h"
 #include "venus.h"
 
 typedef short __v2048i16 __attribute__((ext_vector_type(2048)));
 typedef char __v4096i8 __attribute__((ext_vector_type(4096)));
 
 int Task_ltePDCCHSearch(__v4096i8 pdcchllr,short_struct Candidate)
 {
  short Candidate_num = Candidate.data;
  
  __v4096i8 dciMessageBits;
  vclaim(dciMessageBits);

  __v2048i16 index_Candidate0;
  __v2048i16 index_Candidate1;
  __v2048i16 index_Candidate2;
  __v2048i16 index_Candidate3;
  __v2048i16 index_Candidate4;
  __v2048i16 index_Candidate5;
  vclaim(index_Candidate0);
  vclaim(index_Candidate1);
  vclaim(index_Candidate2);
  vclaim(index_Candidate3);
  vclaim(index_Candidate4);
  vclaim(index_Candidate5);
  
  vrange(index_Candidate0, 576);
  vrange(index_Candidate2, 288);

  short_struct dcilength;
  if(Candidate_num == 0){//AL = 8
    vshuffle(dciMessageBits, index_Candidate0, pdcchllr, SHUFFLE_SCATTER, 576);
    dcilength.data = 576;
  }else if(Candidate_num == 1){//AL = 8
    index_Candidate1 = vsadd(index_Candidate0, 576, MASKREAD_OFF, 576);
    vshuffle(dciMessageBits, index_Candidate1, pdcchllr, SHUFFLE_SCATTER, 576);
    dcilength.data = 576;
  }else if(Candidate_num == 2){//AL = 4
    vshuffle(dciMessageBits, index_Candidate2, pdcchllr, SHUFFLE_SCATTER, 288);
    dcilength.data = 288;
  }else if(Candidate_num == 3){//AL = 4  
    index_Candidate3 = vsadd(index_Candidate2, 288, MASKREAD_OFF, 288);
    vshuffle(dciMessageBits, index_Candidate3, pdcchllr, SHUFFLE_SCATTER, 288);
    dcilength.data = 288;
  }else if(Candidate_num == 4){//AL = 4
    index_Candidate4 = vsadd(index_Candidate2, 2*288, MASKREAD_OFF, 288);
    vshuffle(dciMessageBits, index_Candidate4, pdcchllr, SHUFFLE_SCATTER, 288);
    dcilength.data = 288;
  }else if(Candidate_num == 5){//AL = 4
    index_Candidate5 = vsadd(index_Candidate2, 3*288, MASKREAD_OFF, 288);
    vshuffle(dciMessageBits, index_Candidate1, pdcchllr, SHUFFLE_SCATTER, 288);
    dcilength.data = 288;
  }

   vreturn(dciMessageBits, sizeof(dciMessageBits),&dcilength,sizeof(short_struct));
 }
 