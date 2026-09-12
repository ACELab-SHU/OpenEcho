/**
 * ****************************************
 * @file        Task_ltecheckFlag.c
 * @brief       MIB decode
 * @author      yuanfeng
 * @date        2024.7.15
 * @copyright   ACE-Lab(Shanghai University)
 * ****************************************
 */

 #include "riscv_printf.h"
 #include "venus.h"
 //#include "data_type.h"
 typedef struct {
  short data;
} __attribute__((aligned(64))) short_struct;

 typedef short __v2048i16 __attribute__((ext_vector_type(2048)));
 typedef char  __v4096i8 __attribute__((ext_vector_type(4096)));
 
 int Task_ltecheckFlag(__v4096i8 Crcout_43,__v4096i8 Crcout_42, __v4096i8 Crcout_41, __v4096i8 Crcout_40, __v4096i8 Crcout_23, __v4096i8 Crcout_22, __v4096i8 Crcout_21, __v4096i8 Crcout_20,   __v4096i8 Crcout_13, __v4096i8 Crcout_12, __v4096i8 Crcout_11, __v4096i8 Crcout_10, short_struct CrcFlag_43,  short_struct CrcFlag_42, short_struct CrcFlag_41, short_struct CrcFlag_40,  short_struct CrcFlag_23, short_struct CrcFlag_22, short_struct CrcFlag_21, short_struct CrcFlag_20, short_struct CrcFlag_13, short_struct CrcFlag_12, short_struct CrcFlag_11, short_struct CrcFlag_10) {

  int crcflag43 = CrcFlag_43.data;
  int crcflag42 = CrcFlag_42.data;
  int crcflag41 = CrcFlag_41.data;
  int crcflag40 = CrcFlag_40.data;

  int crcflag23 = CrcFlag_23.data;
  int crcflag22 = CrcFlag_22.data;
  int crcflag21 = CrcFlag_21.data;
  int crcflag20 = CrcFlag_20.data;

  int crcflag13 = CrcFlag_13.data;
  int crcflag12 = CrcFlag_12.data;
  int crcflag11 = CrcFlag_11.data;
  int crcflag10 = CrcFlag_10.data;

  // printf("crcflag43:%d\n",&crcflag43);
  // printf("crcflag42:%d\n",&crcflag42);
  // printf("crcflag41:%d\n",&crcflag41);
  // printf("crcflag40:%d\n",&crcflag40);
  // printf("crcflag23:%d\n",&crcflag23);
  // printf("crcflag22:%d\n",&crcflag22);
  // printf("crcflag21:%d\n",&crcflag21);
  // printf("crcflag20:%d\n",&crcflag20);
  // printf("crcflag13:%d\n",&crcflag13);
  // printf("crcflag12:%d\n",&crcflag12);  
  // printf("crcflag11:%d\n",&crcflag11);
  // printf("crcflag10:%d\n",&crcflag10);

  // __v4096i8 crcflag_43;
  // vclaim(crcflag_43);
  // vbrdcst(crcflag_43, crcflag43, MASKREAD_OFF, 1);

  short crcError = 0;

  short_struct cellrefnum;
  short_struct nfmod4;
  __v4096i8 Crcout;
  vclaim(Crcout);

  __v2048i16 index24;
  vclaim(index24);
  vrange(index24, 24);

  if(crcflag43 == 1){
    vshuffle(Crcout, index24, Crcout_43, SHUFFLE_GATHER, 24);
    cellrefnum.data = 4;
    nfmod4.data = 3;
  }
  else if(crcflag42 == 1){
    vshuffle(Crcout, index24, Crcout_42, SHUFFLE_GATHER, 24);
    cellrefnum.data = 4;
    nfmod4.data = 2;
  }
  else if(crcflag41 == 1){
    vshuffle(Crcout, index24, Crcout_41, SHUFFLE_GATHER, 24);
    cellrefnum.data = 4;
    nfmod4.data = 1;
  }
  else if(crcflag40 == 1){
    vshuffle(Crcout, index24, Crcout_40, SHUFFLE_GATHER, 24);
    cellrefnum.data = 4;
    nfmod4.data = 0;
  }
  else if(crcflag23 == 1){
    vshuffle(Crcout, index24, Crcout_23, SHUFFLE_GATHER, 24);
    cellrefnum.data = 2;
    nfmod4.data = 3;
    // printf("cellrefnum:%hd\n",&(cellrefnum.data));
    // printf("nfmod4:%hd\n",&(nfmod4.data));
  }
  else if(crcflag22 == 1){
    vshuffle(Crcout, index24, Crcout_22, SHUFFLE_GATHER, 24);
    cellrefnum.data = 2;
    nfmod4.data = 2;
    // printf("cellrefnum:%hd\n",&(cellrefnum.data));
    // printf("nfmod4:%hd\n",&(nfmod4.data));
  }
  else if(crcflag21 == 1){
    vshuffle(Crcout, index24, Crcout_21, SHUFFLE_GATHER, 24);
    cellrefnum.data = 2;
    nfmod4.data = 1;
    // printf("cellrefnum:%hd\n",&(cellrefnum.data));
    // printf("nfmod4:%hd\n",&(nfmod4.data));
  }
  else if(crcflag20 == 1){
    vshuffle(Crcout, index24, Crcout_20, SHUFFLE_GATHER, 24);
    cellrefnum.data = 2;
    nfmod4.data = 0;
    // printf("cellrefnum:%hd\n",&(cellrefnum.data));
    // printf("nfmod4:%hd\n",&(nfmod4.data));
  }
  else if(crcflag13 == 1){
    vshuffle(Crcout, index24, Crcout_13, SHUFFLE_GATHER, 24);
    cellrefnum.data = 1;
    nfmod4.data = 3;
  }
  else if(crcflag12 == 1){
    vshuffle(Crcout, index24, Crcout_12, SHUFFLE_GATHER, 24);
    cellrefnum.data = 1;
    nfmod4.data = 2;
  }
  else if(crcflag11 == 1){
    vshuffle(Crcout, index24, Crcout_11, SHUFFLE_GATHER, 24);
    cellrefnum.data = 1;
    nfmod4.data = 1;
  }
  else if(crcflag10 == 1){
    vshuffle(Crcout, index24, Crcout_10, SHUFFLE_GATHER, 24);
    cellrefnum.data = 1;
    nfmod4.data = 0;
  }
  else
  {
    crcError = 1;
  }
  // printf("cellrefnum:%hd\n",&(cellrefnum.data));
  // printf("nfmod4:%hd\n",&(nfmod4.data));
  short_struct CrcError_out;
  CrcError_out.data = crcError;
  // printf("CrcError_out:%hd\n",&(CrcError_out.data));
  vreturn(Crcout, 24, &cellrefnum, sizeof(cellrefnum), &nfmod4, sizeof(nfmod4), &CrcError_out, sizeof(CrcError_out));
 }