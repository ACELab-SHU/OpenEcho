/**
 * ****************************************
 * @file        Task_nrDemodulate.c
 * @brief       Demodulate
 * @author      yuanfeng
 * @date        2024.5.28
 * @copyright   ACE-Lab(Shanghai University)
 * ****************************************
 */

#include "data_type.h"
#include "riscv_printf.h"
#include "venus.h"
#include "vmath.h"


// typedef short __v2048i16 __attribute__((ext_vector_type(2048)));
// typedef char  __v4096i8 __attribute__((ext_vector_type(4096)));
typedef short __v2048i16 __attribute__((ext_vector_type(4000)));
typedef char  __v4096i8 __attribute__((ext_vector_type(8192)));


// short input_inSignal_real[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
// short input_inSignal_imag[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};

#define recpSqrt2   0.70710678                // 1/根号2
#define recpSqrt10  0.316227766016838         // 1/根号10
#define recpSqrt42  0.154303349962092         // 1/根号42
#define recpSqrt170 0.076696498884737         // 1/根号170

// TODO: 暂时将噪声方差定义为short类型，后续需要修改为double类型
// int Task_nrDemodulate(__v4096i8 inSignal_real, __v4096i8 inSignal_imag, short_struct input_nModulationSymb,
//                       double_struct input_nVar, short_struct input_rxSignalLength) {
int Task_lteDemodulate(__v4096i8 inSignal_real, __v4096i8 inSignal_imag, 
  short_struct input_nModulationSymb, short_struct input_rxSignalLength) {
  // int nModulationSymb = 2;
  int fractionLength = 0;
  short nModulation = input_nModulationSymb.data;
  short nVar = 1;
  short rxSignalLength = input_rxSignalLength.data;

  printf("rxSignalLength:%hd\n", &rxSignalLength);

  // /*--------------------input Signal--------------------*/
  // __v4096i16 inSignal_real;
  // __v4096i16 inSignal_imag;
  // vclaim(inSignal_real);
  // vbarrier();
  // VSPM_OPEN();
  // int inSignal_real_addr = vaddr(inSignal_real);
  // for (int i = 0; i < rxSignalLength; ++i) {
  //   *(volatile short *)(inSignal_real_addr + (i << 1)) = input_inSignal_real[i];
  // }
  // VSPM_CLOSE();
  // vclaim(inSignal_imag);
  // vbarrier();
  // VSPM_OPEN();
  // int inSignal_imag_addr = vaddr(inSignal_imag);
  // for (int i = 0; i < rxSignalLength; ++i) {
  //   *(volatile short *)(inSignal_imag_addr + (i << 1)) = input_inSignal_imag[i];
  // }
  // VSPM_CLOSE();


  short nModulationSymb = 0;
  if (nModulation == 0){          //BPSK
    nModulationSymb = 1;
  }else if(nModulation == 1){      //QPSK
    nModulationSymb = 2;
  }else if(nModulation == 2){      //16QAM
    nModulationSymb = 4;
  }else if(nModulation == 3){      //64QAM
    nModulationSymb = 6;
  }else if(nModulation == 4){      //256QAM
    nModulationSymb = 8;
  }else if(nModulation == 5){      //1024QAM
    nModulationSymb = 10;
  }



  // __v4096i8 tempi8;
  // vclaim(tempi8);


  // __v4096i8 vecfactor;
  // vclaim(vecfactor);
  // __v4096i8 vecfactorneg;
  // vclaim(vecfactorneg);

  // __v4096i8 vecfactor2;
  // vclaim(vecfactor2);
  // __v4096i8 vecfactor2neg;
  // vclaim(vecfactor2neg);

  // __v4096i8 vecfactor3;
  // vclaim(vecfactor3);
  // __v4096i8 vecfactor3neg;
  // vclaim(vecfactor3neg);

  // __v4096i8 vecfactor4;
  // vclaim(vecfactor4);
  // __v4096i8 vecfactor4neg;
  // vclaim(vecfactor4neg);

  // __v4096i8 vecfactor5;
  // vclaim(vecfactor5);
  // __v4096i8 vecfactor5neg;
  // vclaim(vecfactor5neg);

  // __v4096i8 vecfactor6;
  // vclaim(vecfactor6);
  // __v4096i8 vecfactor6neg;
  // vclaim(vecfactor6neg);
  
  // __v4096i8 vecfactor7;
  // vclaim(vecfactor7);
  // __v4096i8 vecfactor7neg;
  // vclaim(vecfactor7neg);

  // __v4096i8 vecfactor8;
  // vclaim(vecfactor8);
  // __v4096i8 vecfactor8neg;
  // vclaim(vecfactor8neg);


  // __v4096i8 vecd;
  // vclaim(vecd);
  // __v4096i8 vecdneg;
  // vclaim(vecdneg);

  // __v4096i8 vecd2;
  // vclaim(vecd2);
  // __v4096i8 vecd2neg;
  // vclaim(vecd2neg);

  // __v4096i8 vecd3;
  // vclaim(vecd3);
  // __v4096i8 vecd3neg;
  // vclaim(vecd3neg);

  // __v4096i8 vecd4;
  // vclaim(vecd4);
  // __v4096i8 vecd4neg;
  // vclaim(vecd4neg);

  // __v4096i8 vecd5;
  // vclaim(vecd5);
  // __v4096i8 vecd5neg;
  // vclaim(vecd5neg);

  // __v4096i8 vecd6;
  // vclaim(vecd6);
  // __v4096i8 vecd6neg;
  // vclaim(vecd6neg);

  // __v4096i8 vecd7;
  // vclaim(vecd7);
  // __v4096i8 vecd7neg;
  // vclaim(vecd7neg);

  // __v4096i8 vecd8;
  // vclaim(vecd8);
  // __v4096i8 vecd8neg;
  // vclaim(vecd8neg);

  // __v4096i8 vecd9;
  // vclaim(vecd9);
  // __v4096i8 vecd9neg;
  // vclaim(vecd9neg);

  // __v4096i8 vecd10;
  // vclaim(vecd10);
  // __v4096i8 vecd10neg;
  // vclaim(vecd10neg);

  // __v4096i8 vecd11;
  // vclaim(vecd11);
  // __v4096i8 vecd11neg;
  // vclaim(vecd11neg);

  // __v4096i8 vecd12;
  // vclaim(vecd12);
  // __v4096i8 vecd12neg;
  // vclaim(vecd12neg);

  // __v4096i8 vecd13;
  // vclaim(vecd13);
  // __v4096i8 vecd13neg;
  // vclaim(vecd13neg);

  // __v4096i8 vecd14;
  // vclaim(vecd14);
  // __v4096i8 vecd14neg;
  // vclaim(vecd14neg);




  __v4096i8 softbit;
  vclaim(softbit);
  switch (nModulationSymb) {
  case 2: {
    /*--------------------QPSK Demodulate--------------------*/
    short     d = recpSqrt2 * (1 << fractionLength);
    __v4096i8 softbit1;
    __v4096i8 softbit2;
    // short     factor = d * nVar;
    short factor = 1;
    // short     factor = 4 * d * nVar;
    vsetshamt(fractionLength);
    softbit1 = vmul(inSignal_real, factor, MASKREAD_OFF, rxSignalLength);
    softbit2 = vmul(inSignal_imag, factor, MASKREAD_OFF, rxSignalLength);
    vsetshamt(0);
    __v2048i16 softbit_shuffle_index_tmp;
    vclaim(softbit_shuffle_index_tmp);
    vrange(softbit_shuffle_index_tmp, rxSignalLength);
    softbit_shuffle_index_tmp = vmul(softbit_shuffle_index_tmp, 2, MASKREAD_OFF, rxSignalLength);
    vshuffle(softbit, softbit_shuffle_index_tmp, softbit1, SHUFFLE_SCATTER, rxSignalLength);
    softbit_shuffle_index_tmp = vadd(softbit_shuffle_index_tmp, 1, MASKREAD_OFF, rxSignalLength);
    vshuffle(softbit, softbit_shuffle_index_tmp, softbit2, SHUFFLE_SCATTER, rxSignalLength);
  } break;


  
  // case 4: {
  //   /*--------------------16QAM Demodulate--------------------*/
  //   short     d = recpSqrt10 * (1 << fractionLength);

  //   vbrdcst(vecd, d, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecdneg, -d, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecd2, 2 * d, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecd2neg, -2 * d, MASKREAD_OFF, rxSignalLength);

  //   __v4096i8 softbit1;
  //   __v4096i8 softbit2;
  //   __v4096i8 softbit3;
  //   __v4096i8 softbit4;

  //   /* softbit1 */
  //   short     factor = 4 * d * nVar;
  //   __v4096i8 const_value;
  //   vclaim(const_value);
  //   vbrdcst(const_value, 2 * d, MASKREAD_OFF, rxSignalLength);

  //   vbrdcst(vecfactor, factor, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecfactorneg, -factor, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecfactor2, 2 * factor, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecfactor2neg, -2 * factor, MASKREAD_OFF, rxSignalLength);

  //   // vsge(inSignal_real, 2 * d, MASKREAD_OFF, MASKWRITE_ON, rxSignalLength);
  //   vsle(const_value, inSignal_real, MASKREAD_OFF, MASKWRITE_ON, rxSignalLength);
  //   vsle( vecd2neg,inSignal_real, MASKREAD_ON, MASKWRITE_ON, rxSignalLength);
  //   vsetshamt(fractionLength);
  //   softbit1 = vmul(vecfactor, inSignal_real, MASKREAD_ON, rxSignalLength);
  //   vsetshamt(0);
  //   vslt(inSignal_real, 2 * d, MASKREAD_OFF, MASKWRITE_ON, rxSignalLength);
  //   vsetshamt(fractionLength);
  //   softbit1 = vadd( vecdneg,inSignal_real, MASKREAD_ON, rxSignalLength);
  //   softbit1 = vmul( vecfactor2,inSignal_real, MASKREAD_ON, rxSignalLength);
  //   vsetshamt(0);
  //   vsgt(inSignal_real, -2 * d, MASKREAD_OFF, MASKWRITE_ON, rxSignalLength);
  //   vsetshamt(fractionLength);
  //   softbit1 = vadd(vecd, inSignal_real,  MASKREAD_ON, rxSignalLength);
  //   softbit1 = vmul( vecfactor2,inSignal_real, MASKREAD_ON, rxSignalLength);
  //   vsetshamt(0);

  //   /* softbit2 */
  //   factor = -4 * d * nVar;
  //   vbrdcst(const_value, 2 * d, MASKREAD_OFF, rxSignalLength);

  //   vbrdcst(vecfactor, factor, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecfactorneg, -factor, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecfactor2, 2 * factor, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecfactor2neg, -2 * factor, MASKREAD_OFF, rxSignalLength);

  //   // vsge(inSignal_imag, 2 * d, MASKREAD_OFF, MASKWRITE_ON, rxSignalLength);
  //   vsle(const_value, inSignal_imag, MASKREAD_OFF, MASKWRITE_ON, rxSignalLength);
  //   vsle( vecd2neg ,inSignal_imag, MASKREAD_ON, MASKWRITE_ON, rxSignalLength);
  //   vsetshamt(fractionLength);
  //   softbit2 = vmul(vecfactor, inSignal_imag,  MASKREAD_ON, rxSignalLength);
  //   vsetshamt(0);
  //   vslt(inSignal_imag, 2 * d, MASKREAD_OFF, MASKWRITE_ON, rxSignalLength);
  //   vsetshamt(fractionLength);
  //   softbit2 = vadd( vecdneg,inSignal_imag, MASKREAD_ON, rxSignalLength);
  //   softbit2 = vmul( vecfactor2 ,softbit2, MASKREAD_ON, rxSignalLength);
  //   vsetshamt(0);
  //   vsgt(inSignal_imag, -2 * d, MASKREAD_OFF, MASKWRITE_ON, rxSignalLength);
  //   vsetshamt(fractionLength);
  //   softbit2 = vadd( vecd,inSignal_imag, MASKREAD_ON, rxSignalLength);
  //   softbit2 = vmul( vecfactor2,softbit2, MASKREAD_ON, rxSignalLength);
  //   vsetshamt(0);

  //   /* softbit3 */
  //   vsgt(inSignal_real, 0, MASKREAD_OFF, MASKWRITE_ON, rxSignalLength);
  //   vbrdcst(tempi8, 0xFFFF, MASKREAD_OFF, rxSignalLength);
  //   softbit3 = vxor( tempi8,inSignal_real, MASKREAD_ON, rxSignalLength);
  //   vbrdcst(tempi8, 1, MASKREAD_OFF, rxSignalLength);
  //   softbit3 = vsadd( tempi8,softbit3, MASKREAD_ON, rxSignalLength);
  //   softbit3 = vadd( vecd2neg,softbit3, MASKREAD_ON, rxSignalLength);
  //   vsetshamt(fractionLength);
  //   softbit3 = vmul(vecfactor,softbit3 , MASKREAD_ON, rxSignalLength);
  //   vsetshamt(0);

  //   /* softbit4 */
  //   vsgt(inSignal_imag, 0, MASKREAD_OFF, MASKWRITE_ON, rxSignalLength);
  //   vbrdcst(tempi8, 0xFFFF, MASKREAD_OFF, rxSignalLength);
  //   softbit4 = vxor( tempi8,inSignal_imag, MASKREAD_ON, rxSignalLength);
  //   vbrdcst(tempi8, 1, MASKREAD_OFF, rxSignalLength);
  //   softbit4 = vsadd( tempi8,softbit4, MASKREAD_ON, rxSignalLength);
  //   softbit4 = vadd( vecd2neg,softbit4, MASKREAD_ON, rxSignalLength);
  //   vsetshamt(fractionLength);
  //   softbit4 = vmul(vecfactor, softbit4, MASKREAD_ON, rxSignalLength);
  //   vsetshamt(0);

  //   /* data merge */
  //   __v2048i16 softbit_shuffle_index_tmp;
  //   vclaim(softbit_shuffle_index_tmp);
  //   vrange(softbit_shuffle_index_tmp, rxSignalLength);
  //   softbit_shuffle_index_tmp = vmul(softbit_shuffle_index_tmp, 4, MASKREAD_OFF, rxSignalLength);
  //   vshuffle(softbit, softbit_shuffle_index_tmp, softbit1, SHUFFLE_SCATTER, rxSignalLength);
  //   softbit_shuffle_index_tmp = vadd(softbit_shuffle_index_tmp, 1, MASKREAD_OFF, rxSignalLength);
  //   vshuffle(softbit, softbit_shuffle_index_tmp, softbit2, SHUFFLE_SCATTER, rxSignalLength);
  //   softbit_shuffle_index_tmp = vadd(softbit_shuffle_index_tmp, 1, MASKREAD_OFF, rxSignalLength);
  //   vshuffle(softbit, softbit_shuffle_index_tmp, softbit3, SHUFFLE_SCATTER, rxSignalLength);
  //   softbit_shuffle_index_tmp = vadd(softbit_shuffle_index_tmp, 1, MASKREAD_OFF, rxSignalLength);
  //   vshuffle(softbit, softbit_shuffle_index_tmp, softbit4, SHUFFLE_SCATTER, rxSignalLength);
  // } break;

  // case 6: {
  //   /*--------------------64QAM Demodulate--------------------*/
  //   short     d = recpSqrt42 * (1 << fractionLength);

  //   vbrdcst(vecd, d, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecdneg, -d, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecd2, 2 * d, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecd2neg, -2 * d, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecd3, 3 * d, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecd3neg, -3 * d, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecd4, 4 * d, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecd4neg, -4 * d, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecd5, 5 * d, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecd5neg, -5 * d, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecd6, 6 * d, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecd6neg, -6 * d, MASKREAD_OFF, rxSignalLength);

  //   __v4096i8 softbit1;
  //   __v4096i8 softbit2;
  //   __v4096i8 softbit3;
  //   __v4096i8 softbit4;
  //   __v4096i8 softbit5;
  //   __v4096i8 softbit6;
  //   /* softbit1 */
  //   short factor = 4 * d * nVar;

  //   vbrdcst(vecfactor, factor, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecfactorneg, -factor, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecfactor2, 2 * factor, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecfactor2neg, -2 * factor, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecfactor3, 3 * factor, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecfactor3neg, -3 * factor, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecfactor4, 4 * factor, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecfactor4neg, -4 * factor, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecfactor5, 5 * factor, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecfactor5neg, -5 * factor, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecfactor6, 6 * factor, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecfactor6neg, -6 * factor, MASKREAD_OFF, rxSignalLength);


  //   vsle(inSignal_real, 6 * d, MASKREAD_OFF, MASKWRITE_ON, rxSignalLength);
  //   vsetshamt(fractionLength);
  //   softbit1 = vadd( vecd3neg,inSignal_real, MASKREAD_ON, rxSignalLength);
  //   softbit1 = vmul( vecfactor4,softbit1, MASKREAD_ON, rxSignalLength);
  //   vsetshamt(0);
  //   vsgt(inSignal_real, 6 * d, MASKREAD_OFF, MASKWRITE_ON, rxSignalLength);
  //   vsle( vecd4,inSignal_real, MASKREAD_ON, MASKWRITE_ON, rxSignalLength);
  //   vsetshamt(fractionLength);
  //   softbit1 = vadd( vecd2neg,inSignal_real, MASKREAD_ON, rxSignalLength);
  //   softbit1 = vmul( vecfactor3,softbit1, MASKREAD_ON, rxSignalLength);
  //   vsetshamt(0);
  //   vsgt(inSignal_real, 4 * d, MASKREAD_OFF, MASKWRITE_ON, rxSignalLength);
  //   vsle( vecd2,inSignal_real, MASKREAD_ON, MASKWRITE_ON, rxSignalLength);
  //   vsetshamt(fractionLength);
  //   softbit1 = vadd( vecdneg,inSignal_real, MASKREAD_ON, rxSignalLength);
  //   softbit1 = vmul( vecfactor2,softbit1, MASKREAD_ON, rxSignalLength);
  //   vsetshamt(0);
  //   vsgt(inSignal_real, 2 * d, MASKREAD_OFF, MASKWRITE_ON, rxSignalLength);
  //   vsle( vecfactor2neg,inSignal_real, MASKREAD_ON, MASKWRITE_ON, rxSignalLength);
  //   vsetshamt(fractionLength);
  //   softbit1 = vmul( vecfactor,inSignal_real, MASKREAD_ON, rxSignalLength);
  //   vsetshamt(0);
  //   vsgt(inSignal_real, -2 * d, MASKREAD_OFF, MASKWRITE_ON, rxSignalLength);
  //   vsle( vecd4neg,inSignal_real, MASKREAD_ON, MASKWRITE_ON, rxSignalLength);
  //   vsetshamt(fractionLength);
  //   softbit1 = vadd( vecd,inSignal_real, MASKREAD_ON, rxSignalLength);
  //   softbit1 = vmul( vecfactor2,softbit1, MASKREAD_ON, rxSignalLength);
  //   vsetshamt(0);
  //   vsgt(inSignal_real, -4 * d, MASKREAD_OFF, MASKWRITE_ON, rxSignalLength);
  //   vsle( vecd6neg,inSignal_real, MASKREAD_ON, MASKWRITE_ON, rxSignalLength);
  //   vsetshamt(fractionLength);
  //   softbit1 = vadd( vecd2,inSignal_real, MASKREAD_ON, rxSignalLength);
  //   softbit1 = vmul( vecfactor3,softbit1, MASKREAD_ON, rxSignalLength);
  //   vsetshamt(0);
  //   vsgt(inSignal_real, -6 * d, MASKREAD_OFF, MASKWRITE_ON, rxSignalLength);
  //   vsetshamt(fractionLength);
  //   softbit1 = vadd(  vecd3,inSignal_real, MASKREAD_ON, rxSignalLength);
  //   softbit1 = vmul( vecfactor4,softbit1, MASKREAD_ON, rxSignalLength);
  //   vsetshamt(0);

  //   /* softbit2 */
  //   factor = -4 * d * nVar;

  //   vbrdcst(vecfactor, factor, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecfactorneg, -factor, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecfactor2, 2 * factor, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecfactor2neg, -2 * factor, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecfactor3, 3 * factor, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecfactor3neg, -3 * factor, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecfactor4, 4 * factor, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecfactor4neg, -4 * factor, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecfactor5, 5 * factor, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecfactor5neg, -5 * factor, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecfactor6, 6 * factor, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecfactor6neg, -6 * factor, MASKREAD_OFF, rxSignalLength);

  //   vsle(inSignal_imag, 6 * d, MASKREAD_OFF, MASKWRITE_ON, rxSignalLength);
  //   vsetshamt(fractionLength);
  //   softbit2 = vadd( vecd3neg, inSignal_imag, MASKREAD_ON, rxSignalLength);
  //   softbit2 = vmul( vecfactor4 ,softbit2, MASKREAD_ON, rxSignalLength);
  //   vsetshamt(0);
  //   vsgt(inSignal_imag, 6 * d, MASKREAD_OFF, MASKWRITE_ON, rxSignalLength);
  //   vsle( vecd4 ,inSignal_imag, MASKREAD_ON, MASKWRITE_ON, rxSignalLength);
  //   vsetshamt(fractionLength);
  //   softbit2 = vadd( vecd2neg ,inSignal_imag, MASKREAD_ON, rxSignalLength);
  //   softbit2 = vmul( vecfactor3,softbit2, MASKREAD_ON, rxSignalLength);
  //   vsetshamt(0);
  //   vsgt(inSignal_imag, 4 * d, MASKREAD_OFF, MASKWRITE_ON, rxSignalLength);
  //   vsle( vecd2,inSignal_imag, MASKREAD_ON, MASKWRITE_ON, rxSignalLength);
  //   vsetshamt(fractionLength);
  //   softbit2 = vadd( vecdneg ,inSignal_imag, MASKREAD_ON, rxSignalLength);
  //   softbit2 = vmul( vecfactor2 ,softbit2, MASKREAD_ON, rxSignalLength);
  //   vsetshamt(0);
  //   vsgt(inSignal_imag, 2 * d, MASKREAD_OFF, MASKWRITE_ON, rxSignalLength);
  //   vsle( vecd2neg ,inSignal_imag, MASKREAD_ON, MASKWRITE_ON, rxSignalLength);
  //   vsetshamt(fractionLength);
  //   softbit2 = vmul( vecfactor,inSignal_imag, MASKREAD_ON, rxSignalLength);
  //   vsetshamt(0);
  //   vsgt(inSignal_imag, -2 * d, MASKREAD_OFF, MASKWRITE_ON, rxSignalLength);
  //   vsle( vecd4neg,inSignal_imag, MASKREAD_ON, MASKWRITE_ON, rxSignalLength);
  //   vsetshamt(fractionLength);
  //   softbit2 = vadd( vecd,inSignal_imag, MASKREAD_ON, rxSignalLength);
  //   softbit2 = vmul( vecfactor2,softbit2, MASKREAD_ON, rxSignalLength);
  //   vsetshamt(0);
  //   vsgt(inSignal_imag, -4 * d, MASKREAD_OFF, MASKWRITE_ON, rxSignalLength);
  //   vsle( vecd6neg ,inSignal_imag, MASKREAD_ON, MASKWRITE_ON, rxSignalLength);
  //   vsetshamt(fractionLength);
  //   softbit2 = vadd( vecd2 ,inSignal_imag, MASKREAD_ON, rxSignalLength);
  //   softbit2 = vmul( vecfactor3 ,softbit2, MASKREAD_ON, rxSignalLength);
  //   vsetshamt(0);
  //   vsgt(inSignal_imag, -6 * d, MASKREAD_OFF, MASKWRITE_ON, rxSignalLength);
  //   vsetshamt(fractionLength);
  //   softbit2 = vadd( vecd3,inSignal_imag, MASKREAD_ON, rxSignalLength);
  //   softbit2 = vmul( vecfactor4,softbit2, MASKREAD_ON, rxSignalLength);
  //   vsetshamt(0);

  //   /* softbit3 */
  //   factor = 4 * d * nVar;

  //   vbrdcst(vecfactor, factor, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecfactorneg, -factor, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecfactor2, 2 * factor, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecfactor2neg, -2 * factor, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecfactor3, 3 * factor, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecfactor3neg, -3 * factor, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecfactor4, 4 * factor, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecfactor4neg, -4 * factor, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecfactor5, 5 * factor, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecfactor5neg, -5 * factor, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecfactor6, 6 * factor, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecfactor6neg, -6 * factor, MASKREAD_OFF, rxSignalLength);

  //   vsle(inSignal_real, vecd6, MASKREAD_OFF, MASKWRITE_ON, rxSignalLength);
  //   vsetshamt(fractionLength);
  //   softbit3 = vadd( vecd5neg,inSignal_real, MASKREAD_ON, rxSignalLength);
  //   softbit3 = vmul( vecfactor2neg,softbit3, MASKREAD_ON, rxSignalLength);
  //   vsetshamt(0);
  //   vsgt(inSignal_real, vecd6, MASKREAD_OFF, MASKWRITE_ON, rxSignalLength);
  //   vsle( vecd2,inSignal_real, MASKREAD_ON, MASKWRITE_ON, rxSignalLength);
  //   vsetshamt(fractionLength);
  //   softbit3 = vadd( vecd4neg,inSignal_real, MASKREAD_ON, rxSignalLength);
  //   softbit3 = vmul( vecfactorneg,softbit3, MASKREAD_ON, rxSignalLength);
  //   vsetshamt(0);
  //   vsgt(inSignal_real, vecd2, MASKREAD_OFF, MASKWRITE_ON, rxSignalLength);
  //   vbrdcst(tempi8, 0, MASKREAD_OFF, rxSignalLength);
  //   vsle( tempi8,inSignal_real, MASKREAD_ON, MASKWRITE_ON, rxSignalLength);
  //   vsetshamt(fractionLength);
  //   softbit3 = vadd( vecd3neg,inSignal_real, MASKREAD_ON, rxSignalLength);
  //   softbit3 = vmul( vecfactor2neg,softbit3, MASKREAD_ON, rxSignalLength);
  //   vsetshamt(0);
  //   vsgt(inSignal_real, 0, MASKREAD_OFF, MASKWRITE_ON, rxSignalLength);
  //   vsle( vecd2neg,inSignal_real, MASKREAD_ON, MASKWRITE_ON, rxSignalLength);
  //   vsetshamt(fractionLength);
  //   softbit3 = vadd( vecd3,inSignal_real, MASKREAD_ON, rxSignalLength);
  //   softbit3 = vmul( vecfactor2,softbit3, MASKREAD_ON, rxSignalLength);
  //   vsetshamt(0);
  //   vsgt(inSignal_real, vecd2neg, MASKREAD_OFF, MASKWRITE_ON, rxSignalLength);
  //   vsle( vecd6neg,inSignal_real, MASKREAD_ON, MASKWRITE_ON, rxSignalLength);
  //   vsetshamt(fractionLength);
  //   softbit3 = vadd( vecd4,inSignal_real, MASKREAD_ON, rxSignalLength);
  //   softbit3 = vmul( vecfactor,softbit3, MASKREAD_ON, rxSignalLength);
  //   vsetshamt(0);
  //   vsgt(inSignal_real, vecd6neg, MASKREAD_OFF, MASKWRITE_ON, rxSignalLength);
  //   vsetshamt(fractionLength);
  //   softbit3 = vadd( vecd5,inSignal_real, MASKREAD_ON, rxSignalLength);
  //   softbit3 = vmul( vecfactor2,softbit3, MASKREAD_ON, rxSignalLength);
  //   vsetshamt(0);

  //   /* softbit4 */
  //   factor = 4 * d * nVar;

  //   vbrdcst(vecfactor, factor, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecfactorneg, -factor, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecfactor2, 2 * factor, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecfactor2neg, -2 * factor, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecfactor3, 3 * factor, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecfactor3neg, -3 * factor, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecfactor4, 4 * factor, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecfactor4neg, -4 * factor, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecfactor5, 5 * factor, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecfactor5neg, -5 * factor, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecfactor6, 6 * factor, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecfactor6neg, -6 * factor, MASKREAD_OFF, rxSignalLength);

  //   vsle(inSignal_imag, 6 * d, MASKREAD_OFF, MASKWRITE_ON, rxSignalLength);
  //   vsetshamt(fractionLength);
  //   softbit4 = vadd( vecd5neg,inSignal_imag, MASKREAD_ON, rxSignalLength);
  //   softbit4 = vmul( vecfactor2neg,softbit4, MASKREAD_ON, rxSignalLength);
  //   vsetshamt(0);
  //   vsgt(inSignal_imag, 6 * d, MASKREAD_OFF, MASKWRITE_ON, rxSignalLength);
  //   vsle( vecd2,inSignal_imag, MASKREAD_ON, MASKWRITE_ON, rxSignalLength);
  //   vsetshamt(fractionLength);
  //   softbit4 = vadd( vecd4neg,inSignal_imag, MASKREAD_ON, rxSignalLength);
  //   softbit4 = vmul( vecfactorneg,softbit4, MASKREAD_ON, rxSignalLength);
  //   vsetshamt(0);
  //   vsgt(inSignal_imag, vecd2, MASKREAD_OFF, MASKWRITE_ON, rxSignalLength);
  //   vbrdcst(tempi8, 0, MASKREAD_OFF, rxSignalLength);
  //   vsle( tempi8,inSignal_imag, MASKREAD_ON, MASKWRITE_ON, rxSignalLength);
  //   vsetshamt(fractionLength);
  //   softbit4 = vadd( vecd3neg,inSignal_imag, MASKREAD_ON, rxSignalLength);
  //   softbit4 = vmul( vecfactor2neg,softbit4, MASKREAD_ON, rxSignalLength);
  //   vsetshamt(0);
  //   vsgt(inSignal_imag, 0, MASKREAD_OFF, MASKWRITE_ON, rxSignalLength);
  //   vsle( vecd2neg,inSignal_imag, MASKREAD_ON, MASKWRITE_ON, rxSignalLength);
  //   vsetshamt(fractionLength);
  //   softbit4 = vadd( vecd3,inSignal_imag, MASKREAD_ON, rxSignalLength);
  //   softbit4 = vmul( vecfactor2,softbit4, MASKREAD_ON, rxSignalLength);
  //   vsetshamt(0);
  //   vsgt(inSignal_imag, vecd2neg, MASKREAD_OFF, MASKWRITE_ON, rxSignalLength);
  //   vsle( vecd6neg,inSignal_imag, MASKREAD_ON, MASKWRITE_ON, rxSignalLength);
  //   vsetshamt(fractionLength);
  //   softbit4 = vadd( vecd4,inSignal_imag, MASKREAD_ON, rxSignalLength);
  //   softbit4 = vmul( vecfactor,softbit4, MASKREAD_ON, rxSignalLength);
  //   vsetshamt(0);
  //   vsgt(inSignal_imag, vecd6neg, MASKREAD_OFF, MASKWRITE_ON, rxSignalLength);
  //   vsetshamt(fractionLength);
  //   softbit4 = vadd( vecd5,inSignal_imag, MASKREAD_ON, rxSignalLength);
  //   softbit4 = vmul( vecfactor2,softbit4, MASKREAD_ON, rxSignalLength);
  //   vsetshamt(0);

  //   /* softbit5 */
  //   factor = 4 * d * nVar;

  //   vbrdcst(vecfactor, factor, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecfactorneg, -factor, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecfactor2, 2 * factor, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecfactor2neg, -2 * factor, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecfactor3, 3 * factor, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecfactor3neg, -3 * factor, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecfactor4, 4 * factor, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecfactor4neg, -4 * factor, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecfactor5, 5 * factor, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecfactor5neg, -5 * factor, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecfactor6, 6 * factor, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecfactor6neg, -6 * factor, MASKREAD_OFF, rxSignalLength);

  //   vsle(inSignal_real, 4 * d, MASKREAD_OFF, MASKWRITE_ON, rxSignalLength);
  //   vsetshamt(fractionLength);
  //   softbit5 = vadd( vecd6neg,inSignal_real, MASKREAD_ON, rxSignalLength);
  //   softbit5 = vmul( vecfactorneg,softbit5, MASKREAD_ON, rxSignalLength);
  //   vsetshamt(0);
  //   vsgt(inSignal_real, 4 * d, MASKREAD_OFF, MASKWRITE_ON, rxSignalLength);
  //   vbrdcst(tempi8, 0, MASKREAD_OFF, rxSignalLength);
  //   vsle( tempi8,inSignal_real, MASKREAD_ON, MASKWRITE_ON, rxSignalLength);
  //   vsetshamt(fractionLength);
  //   softbit5 = vadd( vecd2neg,inSignal_real, MASKREAD_ON, rxSignalLength);
  //   softbit5 = vmul(vecfactor,softbit5 , MASKREAD_ON, rxSignalLength);
  //   vsetshamt(0);
  //   vsgt(inSignal_real, 0, MASKREAD_OFF, MASKWRITE_ON, rxSignalLength);
  //   vsle( vecd4neg,inSignal_real, MASKREAD_ON, MASKWRITE_ON, rxSignalLength);
  //   vsetshamt(fractionLength);
  //   softbit5 = vadd( vecd2,inSignal_real, MASKREAD_ON, rxSignalLength);
  //   softbit5 = vmul( vecfactorneg,softbit5, MASKREAD_ON, rxSignalLength);
  //   vsetshamt(0);
  //   vsgt(inSignal_real, vecd4neg, MASKREAD_OFF, MASKWRITE_ON, rxSignalLength);
  //   vsetshamt(fractionLength);
  //   softbit5 = vadd( vecd6,inSignal_real, MASKREAD_ON, rxSignalLength);
  //   softbit5 = vmul( vecfactor,softbit5, MASKREAD_ON, rxSignalLength);
  //   vsetshamt(0);

  //   /* softbit6 */
  //   factor = 4 * d * nVar;

  //   vbrdcst(vecfactor, factor, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecfactorneg, -factor, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecfactor2, 2 * factor, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecfactor2neg, -2 * factor, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecfactor3, 3 * factor, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecfactor3neg, -3 * factor, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecfactor4, 4 * factor, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecfactor4neg, -4 * factor, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecfactor5, 5 * factor, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecfactor5neg, -5 * factor, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecfactor6, 6 * factor, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecfactor6neg, -6 * factor, MASKREAD_OFF, rxSignalLength);

  //   vsle(inSignal_imag, 4 * d, MASKREAD_OFF, MASKWRITE_ON, rxSignalLength);
  //   vsetshamt(fractionLength);
  //   softbit6 = vadd( vecd6neg,inSignal_imag, MASKREAD_ON, rxSignalLength);
  //   softbit6 = vmul( vecfactorneg,softbit6, MASKREAD_ON, rxSignalLength);
  //   vsetshamt(0);
  //   vsgt(inSignal_imag, 4 * d, MASKREAD_OFF, MASKWRITE_ON, rxSignalLength);
  //   vbrdcst(tempi8, 0, MASKREAD_OFF, rxSignalLength);
  //   vsle( tempi8,inSignal_imag, MASKREAD_ON, MASKWRITE_ON, rxSignalLength);
  //   vsetshamt(fractionLength);
  //   softbit6 = vadd( vecd2neg,inSignal_imag, MASKREAD_ON, rxSignalLength);
  //   softbit6 = vmul( vecfactor,softbit6, MASKREAD_ON, rxSignalLength);
  //   vsetshamt(0);
  //   vsgt(inSignal_imag, 0, MASKREAD_OFF, MASKWRITE_ON, rxSignalLength);
  //   vsle( vecd4neg,inSignal_imag, MASKREAD_ON, MASKWRITE_ON, rxSignalLength);
  //   vsetshamt(fractionLength);
  //   softbit6 = vadd( vecd2,inSignal_imag, MASKREAD_ON, rxSignalLength);
  //   softbit6 = vmul( vecfactorneg,softbit6, MASKREAD_ON, rxSignalLength);
  //   vsetshamt(0);
  //   vsgt(inSignal_imag, vecd4neg, MASKREAD_OFF, MASKWRITE_ON, rxSignalLength);
  //   vsetshamt(fractionLength);
  //   softbit6 = vadd( vecd6,inSignal_imag, MASKREAD_ON, rxSignalLength);
  //   softbit6 = vmul( vecfactor,softbit6, MASKREAD_ON, rxSignalLength);
  //   vsetshamt(0);

  //   /* data merge */
  //   __v2048i16 softbit_shuffle_index_tmp;
  //   vclaim(softbit_shuffle_index_tmp);
  //   vrange(softbit_shuffle_index_tmp, rxSignalLength);
  //   softbit_shuffle_index_tmp = vmul(softbit_shuffle_index_tmp, 6, MASKREAD_OFF, rxSignalLength);
  //   vshuffle(softbit, softbit_shuffle_index_tmp, softbit1, SHUFFLE_SCATTER, rxSignalLength);
  //   softbit_shuffle_index_tmp = vadd(softbit_shuffle_index_tmp, 1, MASKREAD_OFF, rxSignalLength);
  //   vshuffle(softbit, softbit_shuffle_index_tmp, softbit2, SHUFFLE_SCATTER, rxSignalLength);
  //   softbit_shuffle_index_tmp = vadd(softbit_shuffle_index_tmp, 1, MASKREAD_OFF, rxSignalLength);
  //   vshuffle(softbit, softbit_shuffle_index_tmp, softbit3, SHUFFLE_SCATTER, rxSignalLength);
  //   softbit_shuffle_index_tmp = vadd(softbit_shuffle_index_tmp, 1, MASKREAD_OFF, rxSignalLength);
  //   vshuffle(softbit, softbit_shuffle_index_tmp, softbit4, SHUFFLE_SCATTER, rxSignalLength);
  //   softbit_shuffle_index_tmp = vadd(softbit_shuffle_index_tmp, 1, MASKREAD_OFF, rxSignalLength);
  //   vshuffle(softbit, softbit_shuffle_index_tmp, softbit5, SHUFFLE_SCATTER, rxSignalLength);
  //   softbit_shuffle_index_tmp = vadd(softbit_shuffle_index_tmp, 1, MASKREAD_OFF, rxSignalLength);
  //   vshuffle(softbit, softbit_shuffle_index_tmp, softbit6, SHUFFLE_SCATTER, rxSignalLength);
  // } break;
  
  
  
  // case 8: {
  //   /*--------------------256QAM Demodulate--------------------*/
  //   short     d = recpSqrt170 * (1 << fractionLength);

  //   vbrdcst(vecd, d, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecdneg, -d, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecd2, 2 * d, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecd2neg, -2 * d, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecd3, 3 * d, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecd3neg, -3 * d, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecd4, 4 * d, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecd4neg, -4 * d, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecd5, 5 * d, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecd5neg, -5 * d, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecd6, 6 * d, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecd6neg, -6 * d, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecd7, 7 * d, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecd7neg, -7 * d, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecd8, 8 * d, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecd8neg, -8 * d, MASKREAD_OFF, rxSignalLength);    
  //   vbrdcst(vecd9, 9 * d, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecd9neg, -9 * d, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecd10, 10 * d, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecd10neg, -10 * d, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecd11, 11 * d, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecd11neg, -11 * d, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecd12, 12 * d, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecd12neg, -12 * d, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecd13, 13 * d, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecd13neg, -13 * d, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecd14, 14 * d, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecd14neg, -14 * d, MASKREAD_OFF, rxSignalLength);


  //   __v4096i8 softbit1;
  //   __v4096i8 softbit2;
  //   __v4096i8 softbit3;
  //   __v4096i8 softbit4;
  //   __v4096i8 softbit5;
  //   __v4096i8 softbit6;
  //   __v4096i8 softbit7;
  //   __v4096i8 softbit8;


  //   /* softbit1 */
  //   short factor = 4 * d * nVar;

  //   vbrdcst(vecfactor, factor, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecfactorneg, -factor, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecfactor2, 2 * factor, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecfactor2neg, -2 * factor, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecfactor3, 3 * factor, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecfactor3neg, -3 * factor, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecfactor4, 4 * factor, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecfactor4neg, -4 * factor, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecfactor5, 5 * factor, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecfactor5neg, -5 * factor, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecfactor6, 6 * factor, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecfactor6neg, -6 * factor, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecfactor7, 7 * factor, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecfactor7neg, -7 * factor, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecfactor8, 8 * factor, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecfactor8neg, -8 * factor, MASKREAD_OFF, rxSignalLength);


  //   vsle(inSignal_real, vecd14, MASKREAD_OFF, MASKWRITE_ON, rxSignalLength);
  //   vsetshamt(fractionLength);
  //   softbit1 = vadd( vecd7neg,inSignal_real, MASKREAD_ON, rxSignalLength);
  //   softbit1 = vmul( vecfactor8,softbit1, MASKREAD_ON, rxSignalLength);
  //   vsetshamt(0);
  //   vsgt(inSignal_real, vecd14, MASKREAD_OFF, MASKWRITE_ON, rxSignalLength);
  //   vsle( vecd12,inSignal_real, MASKREAD_ON, MASKWRITE_ON, rxSignalLength);
  //   vsetshamt(fractionLength);
  //   softbit1 = vadd( vecd6neg,inSignal_real, MASKREAD_ON, rxSignalLength);
  //   softbit1 = vmul( vecfactor7,softbit1, MASKREAD_ON, rxSignalLength);
  //   vsetshamt(0);
  //   vsgt(inSignal_real, vecd12, MASKREAD_OFF, MASKWRITE_ON, rxSignalLength);
  //   vsle( vecd10,inSignal_real, MASKREAD_ON, MASKWRITE_ON, rxSignalLength);
  //   vsetshamt(fractionLength);
  //   softbit1 = vadd( vecd5neg,inSignal_real, MASKREAD_ON, rxSignalLength);
  //   softbit1 = vmul( vecfactor6,softbit1, MASKREAD_ON, rxSignalLength);
  //   vsetshamt(0);
  //   vsgt(inSignal_real, vecd10, MASKREAD_OFF, MASKWRITE_ON, rxSignalLength);
  //   vsle( vecd8,inSignal_real, MASKREAD_ON, MASKWRITE_ON, rxSignalLength);
  //   vsetshamt(fractionLength);
  //   softbit1 = vadd( vecd4neg,inSignal_real, MASKREAD_ON, rxSignalLength);
  //   softbit1 = vmul( vecfactor5,softbit1, MASKREAD_ON, rxSignalLength);
  //   vsetshamt(0);
  //   vsgt(inSignal_real, vecd8, MASKREAD_OFF, MASKWRITE_ON, rxSignalLength);
  //   vsle( vecd6,inSignal_real, MASKREAD_ON, MASKWRITE_ON, rxSignalLength);
  //   vsetshamt(fractionLength);
  //   softbit1 = vadd( vecd3neg,inSignal_real, MASKREAD_ON, rxSignalLength);
  //   softbit1 = vmul( vecfactor4,softbit1, MASKREAD_ON, rxSignalLength);
  //   vsetshamt(0);
  //   vsgt(inSignal_real, vecd6, MASKREAD_OFF, MASKWRITE_ON, rxSignalLength);
  //   vsle( vecd4,inSignal_real, MASKREAD_ON, MASKWRITE_ON, rxSignalLength);
  //   vsetshamt(fractionLength);
  //   softbit1 = vadd( vecd2neg,inSignal_real, MASKREAD_ON, rxSignalLength);
  //   softbit1 = vmul( vecfactor3,softbit1, MASKREAD_ON, rxSignalLength);
  //   vsetshamt(0);
  //   vsgt(inSignal_real, vecd4, MASKREAD_OFF, MASKWRITE_ON, rxSignalLength);
  //   vsle( vecd2,inSignal_real, MASKREAD_ON, MASKWRITE_ON, rxSignalLength);
  //   vsetshamt(fractionLength);
  //   softbit1 = vadd( vecdneg,inSignal_real, MASKREAD_ON, rxSignalLength);
  //   softbit1 = vmul( vecfactor2,softbit1, MASKREAD_ON, rxSignalLength);
  //   vsetshamt(0);
  //   vsgt(inSignal_real, vecd2, MASKREAD_OFF, MASKWRITE_ON, rxSignalLength);
  //   vsle( vecd2neg,inSignal_real, MASKREAD_ON, MASKWRITE_ON, rxSignalLength);
  //   vsetshamt(fractionLength);
  //   softbit1 = vmul( vecfactor,softbit1, MASKREAD_ON, rxSignalLength);
  //   vsetshamt(0);
  //   vsgt(inSignal_real, vecd2neg, MASKREAD_OFF, MASKWRITE_ON, rxSignalLength);
  //   vsle( vecd4neg,inSignal_real, MASKREAD_ON, MASKWRITE_ON, rxSignalLength);
  //   vsetshamt(fractionLength);
  //   softbit1 = vadd( vecd,inSignal_real, MASKREAD_ON, rxSignalLength);
  //   softbit1 = vmul( vecfactor2,softbit1, MASKREAD_ON, rxSignalLength);
  //   vsetshamt(0);
  //   vsgt(inSignal_real, vecd4neg, MASKREAD_OFF, MASKWRITE_ON, rxSignalLength);
  //   vsle( vecd6neg,inSignal_real, MASKREAD_ON, MASKWRITE_ON, rxSignalLength);
  //   vsetshamt(fractionLength);
  //   softbit1 = vadd( vecd2,inSignal_real, MASKREAD_ON, rxSignalLength);
  //   softbit1 = vmul( vecfactor3,softbit1, MASKREAD_ON, rxSignalLength);
  //   vsetshamt(0);
  //   vsgt(inSignal_real, vecd6neg, MASKREAD_OFF, MASKWRITE_ON, rxSignalLength);
  //   vsle( vecd8neg,inSignal_real, MASKREAD_ON, MASKWRITE_ON, rxSignalLength);
  //   vsetshamt(fractionLength);
  //   softbit1 = vadd( vecd3,inSignal_real, MASKREAD_ON, rxSignalLength);
  //   softbit1 = vmul( vecfactor4,softbit1, MASKREAD_ON, rxSignalLength);
  //   vsetshamt(0);
  //   vsgt(inSignal_real, vecd8neg, MASKREAD_OFF, MASKWRITE_ON, rxSignalLength);
  //   vsle( vecd10neg,inSignal_real, MASKREAD_ON, MASKWRITE_ON, rxSignalLength);
  //   vsetshamt(fractionLength);
  //   softbit1 = vadd( vecd4,inSignal_real, MASKREAD_ON, rxSignalLength);
  //   softbit1 = vmul( vecfactor5,softbit1, MASKREAD_ON, rxSignalLength);
  //   vsetshamt(0);
  //   vsgt(inSignal_real, vecd10neg, MASKREAD_OFF, MASKWRITE_ON, rxSignalLength);
  //   vsle( vecd12neg,inSignal_real, MASKREAD_ON, MASKWRITE_ON, rxSignalLength);
  //   vsetshamt(fractionLength);
  //   softbit1 = vadd( vecd5,inSignal_real, MASKREAD_ON, rxSignalLength);
  //   softbit1 = vmul( vecfactor6,softbit1, MASKREAD_ON, rxSignalLength);
  //   vsetshamt(0);
  //   vsgt(inSignal_real, vecd12neg, MASKREAD_OFF, MASKWRITE_ON, rxSignalLength);
  //   vsle( vecd14neg,inSignal_real, MASKREAD_ON, MASKWRITE_ON, rxSignalLength);
  //   vsetshamt(fractionLength);
  //   softbit1 = vadd( vecd6,inSignal_real, MASKREAD_ON, rxSignalLength);
  //   softbit1 = vmul( vecfactor7,softbit1, MASKREAD_ON, rxSignalLength);
  //   vsetshamt(0);
  //   vsgt(inSignal_real, vecd14neg, MASKREAD_OFF, MASKWRITE_ON, rxSignalLength);
  //   vsetshamt(fractionLength);
  //   softbit1 = vadd( vecd7,inSignal_real, MASKREAD_ON, rxSignalLength);
  //   softbit1 = vmul( vecfactor8,softbit1, MASKREAD_ON, rxSignalLength);
  //   vsetshamt(0);

  //   /* softbit2 */
  //   factor = -4 * d * nVar;

  //   vbrdcst(vecfactor, factor, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecfactorneg, -factor, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecfactor2, 2 * factor, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecfactor2neg, -2 * factor, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecfactor3, 3 * factor, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecfactor3neg, -3 * factor, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecfactor4, 4 * factor, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecfactor4neg, -4 * factor, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecfactor5, 5 * factor, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecfactor5neg, -5 * factor, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecfactor6, 6 * factor, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecfactor6neg, -6 * factor, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecfactor7, 7 * factor, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecfactor7neg, -7 * factor, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecfactor8, 8 * factor, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecfactor8neg, -8 * factor, MASKREAD_OFF, rxSignalLength);

  //   vsle(inSignal_imag, vecd14, MASKREAD_OFF, MASKWRITE_ON, rxSignalLength);
  //   vsetshamt(fractionLength);
  //   softbit2 = vadd(vecd7neg,inSignal_imag, MASKREAD_ON, rxSignalLength);
  //   softbit2 = vmul( vecfactor8,softbit2, MASKREAD_ON, rxSignalLength);
  //   vsetshamt(0);
  //   vsgt(inSignal_imag, vecd14, MASKREAD_OFF, MASKWRITE_ON, rxSignalLength);
  //   vsle( vecd12,inSignal_imag, MASKREAD_ON, MASKWRITE_ON, rxSignalLength);
  //   vsetshamt(fractionLength);
  //   softbit2 = vadd( vecd6neg,inSignal_imag, MASKREAD_ON, rxSignalLength);
  //   softbit2 = vmul( vecfactor7,softbit2, MASKREAD_ON, rxSignalLength);
  //   vsetshamt(0);
  //   vsgt(inSignal_imag, vecd12, MASKREAD_OFF, MASKWRITE_ON, rxSignalLength);
  //   vsle( vecd10,inSignal_imag, MASKREAD_ON, MASKWRITE_ON, rxSignalLength);
  //   vsetshamt(fractionLength);
  //   softbit2 = vadd( vecd5neg,inSignal_imag, MASKREAD_ON, rxSignalLength);
  //   softbit2 = vmul( vecfactor6,softbit2, MASKREAD_ON, rxSignalLength);
  //   vsetshamt(0);
  //   vsgt(inSignal_imag, vecd10, MASKREAD_OFF, MASKWRITE_ON, rxSignalLength);
  //   vsle( vecd8,inSignal_imag, MASKREAD_ON, MASKWRITE_ON, rxSignalLength);
  //   vsetshamt(fractionLength);
  //   softbit2 = vadd( vecd4neg,inSignal_imag, MASKREAD_ON, rxSignalLength);
  //   softbit2 = vmul( vecfactor5,softbit2, MASKREAD_ON, rxSignalLength);
  //   vsetshamt(0);
  //   vsgt(inSignal_imag, vecd8, MASKREAD_OFF, MASKWRITE_ON, rxSignalLength);
  //   vsle( vecd6,inSignal_imag, MASKREAD_ON, MASKWRITE_ON, rxSignalLength);
  //   vsetshamt(fractionLength);
  //   softbit2 = vadd( vecd3neg,inSignal_imag, MASKREAD_ON, rxSignalLength);
  //   softbit2 = vmul( vecfactor4,softbit2, MASKREAD_ON, rxSignalLength);
  //   vsetshamt(0);
  //   vsgt(inSignal_imag, vecd6, MASKREAD_OFF, MASKWRITE_ON, rxSignalLength);
  //   vsle( vecd4,inSignal_imag, MASKREAD_ON, MASKWRITE_ON, rxSignalLength);
  //   vsetshamt(fractionLength);
  //   softbit2 = vadd( vecd2neg,inSignal_imag, MASKREAD_ON, rxSignalLength);
  //   softbit2 = vmul( vecfactor3,softbit2, MASKREAD_ON, rxSignalLength);
  //   vsetshamt(0);
  //   vsgt(inSignal_imag, vecd4, MASKREAD_OFF, MASKWRITE_ON, rxSignalLength);
  //   vsle( vecd2,inSignal_imag, MASKREAD_ON, MASKWRITE_ON, rxSignalLength);
  //   vsetshamt(fractionLength);
  //   softbit2 = vadd( vecdneg,inSignal_imag, MASKREAD_ON, rxSignalLength);
  //   softbit2 = vmul( vecfactor2,softbit2, MASKREAD_ON, rxSignalLength);
  //   vsetshamt(0);
  //   vsgt(inSignal_imag, vecd2, MASKREAD_OFF, MASKWRITE_ON, rxSignalLength);
  //   vsle( vecd2neg,inSignal_imag, MASKREAD_ON, MASKWRITE_ON, rxSignalLength);
  //   vsetshamt(fractionLength);
  //   softbit2 = vmul(vecfactor,softbit2, MASKREAD_ON, rxSignalLength);
  //   vsetshamt(0);
  //   vsgt(inSignal_imag, vecd2neg, MASKREAD_OFF, MASKWRITE_ON, rxSignalLength);
  //   vsle( vecd4neg,inSignal_imag, MASKREAD_ON, MASKWRITE_ON, rxSignalLength);
  //   vsetshamt(fractionLength);
  //   softbit2 = vadd( vecd6,inSignal_imag, MASKREAD_ON, rxSignalLength);
  //   softbit2 = vmul( vecfactor2,softbit2, MASKREAD_ON, rxSignalLength);
  //   vsetshamt(0);
  //   vsgt(inSignal_imag, vecd4neg, MASKREAD_OFF, MASKWRITE_ON, rxSignalLength);
  //   vsle( vecd6neg,inSignal_imag, MASKREAD_ON, MASKWRITE_ON, rxSignalLength);
  //   vsetshamt(fractionLength);
  //   softbit2 = vadd( vecd2,inSignal_imag, MASKREAD_ON, rxSignalLength);
  //   softbit2 = vmul( vecfactor3,softbit2, MASKREAD_ON, rxSignalLength);
  //   vsetshamt(0);
  //   vsgt(inSignal_imag, vecd6neg, MASKREAD_OFF, MASKWRITE_ON, rxSignalLength);
  //   vsle( vecd8neg,inSignal_imag, MASKREAD_ON, MASKWRITE_ON, rxSignalLength);
  //   vsetshamt(fractionLength);
  //   softbit2 = vadd( vecd3,inSignal_imag, MASKREAD_ON, rxSignalLength);
  //   softbit2 = vmul( vecfactor4,softbit2, MASKREAD_ON, rxSignalLength);
  //   vsetshamt(0);
  //   vsgt(inSignal_imag, vecd8neg, MASKREAD_OFF, MASKWRITE_ON, rxSignalLength);
  //   vsle( vecd10neg,inSignal_imag, MASKREAD_ON, MASKWRITE_ON, rxSignalLength);
  //   vsetshamt(fractionLength);
  //   softbit2 = vadd( vecd4,inSignal_imag, MASKREAD_ON, rxSignalLength);
  //   softbit2 = vmul( vecfactor5,softbit2, MASKREAD_ON, rxSignalLength);
  //   vsetshamt(0);
  //   vsgt(inSignal_imag, vecd10neg, MASKREAD_OFF, MASKWRITE_ON, rxSignalLength);
  //   vsle( vecd12neg,inSignal_imag, MASKREAD_ON, MASKWRITE_ON, rxSignalLength);
  //   vsetshamt(fractionLength);
  //   softbit2 = vadd( vecd5,inSignal_imag, MASKREAD_ON, rxSignalLength);
  //   softbit2 = vmul( vecfactor6,softbit2, MASKREAD_ON, rxSignalLength);
  //   vsetshamt(0);
  //   vsgt(inSignal_imag, vecd12neg, MASKREAD_OFF, MASKWRITE_ON, rxSignalLength);
  //   vsle( vecd14neg,inSignal_imag, MASKREAD_ON, MASKWRITE_ON, rxSignalLength);
  //   vsetshamt(fractionLength);
  //   softbit2 = vadd( vecd6,inSignal_imag, MASKREAD_ON, rxSignalLength);
  //   softbit2 = vmul( vecfactor7,softbit2, MASKREAD_ON, rxSignalLength);
  //   vsetshamt(0);
  //   vsgt(inSignal_imag, vecd14neg, MASKREAD_OFF, MASKWRITE_ON, rxSignalLength);
  //   vsetshamt(fractionLength);
  //   softbit2 = vadd( vecd7,inSignal_imag, MASKREAD_ON, rxSignalLength);
  //   softbit2 = vmul( vecfactor8,softbit2, MASKREAD_ON, rxSignalLength);
  //   vsetshamt(0);

  //   /* softbit3 */
  //   factor = 4 * d * nVar;

  //   vbrdcst(vecfactor, factor, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecfactorneg, -factor, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecfactor2, 2 * factor, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecfactor2neg, -2 * factor, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecfactor3, 3 * factor, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecfactor3neg, -3 * factor, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecfactor4, 4 * factor, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecfactor4neg, -4 * factor, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecfactor5, 5 * factor, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecfactor5neg, -5 * factor, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecfactor6, 6 * factor, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecfactor6neg, -6 * factor, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecfactor7, 7 * factor, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecfactor7neg, -7 * factor, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecfactor8, 8 * factor, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecfactor8neg, -8 * factor, MASKREAD_OFF, rxSignalLength);

  //   vsle(inSignal_real, vecd14, MASKREAD_OFF, MASKWRITE_ON, rxSignalLength);
  //   vsetshamt(fractionLength);
  //   softbit3 = vadd( vecd11neg,inSignal_real, MASKREAD_ON, rxSignalLength);
  //   softbit3 = vmul( vecfactor4neg,softbit3, MASKREAD_ON, rxSignalLength);
  //   vsetshamt(0);
  //   vsgt(inSignal_real, vecd14, MASKREAD_OFF, MASKWRITE_ON, rxSignalLength);
  //   vsle( vecd12,inSignal_real, MASKREAD_ON, MASKWRITE_ON, rxSignalLength);
  //   vsetshamt(fractionLength);
  //   softbit3 = vadd( vecd10neg,inSignal_real, MASKREAD_ON, rxSignalLength);
  //   softbit3 = vmul( vecfactor3neg,softbit3, MASKREAD_ON, rxSignalLength);
  //   vsetshamt(0);
  //   vsgt(inSignal_real, vecd12, MASKREAD_OFF, MASKWRITE_ON, rxSignalLength);
  //   vsle( vecd10,inSignal_real, MASKREAD_ON, MASKWRITE_ON, rxSignalLength);
  //   vsetshamt(fractionLength);
  //   softbit3 = vadd( vecd9neg,inSignal_real, MASKREAD_ON, rxSignalLength);
  //   softbit3 = vmul( vecfactor2neg,softbit3, MASKREAD_ON, rxSignalLength);
  //   vsetshamt(0);
  //   vsgt(inSignal_real, vecd10, MASKREAD_OFF, MASKWRITE_ON, rxSignalLength);
  //   vsle( vecd6,inSignal_real, MASKREAD_ON, MASKWRITE_ON, rxSignalLength);
  //   vsetshamt(fractionLength);
  //   softbit3 = vadd( vecd8neg,inSignal_real, MASKREAD_ON, rxSignalLength);
  //   softbit3 = vmul( vecfactor2,softbit3, MASKREAD_ON, rxSignalLength);
  //   vsetshamt(0);
  //   vsgt(inSignal_real, vecd6, MASKREAD_OFF, MASKWRITE_ON, rxSignalLength);
  //   vsle( vecd4,inSignal_real, MASKREAD_ON, MASKWRITE_ON, rxSignalLength);
  //   vsetshamt(fractionLength);
  //   softbit3 = vadd( vecd7neg,inSignal_real, MASKREAD_ON, rxSignalLength);
  //   softbit3 = vmul( vecfactor2neg,softbit3, MASKREAD_ON, rxSignalLength);
  //   vsetshamt(0);
  //   vsgt(inSignal_real, vecd4, MASKREAD_OFF, MASKWRITE_ON, rxSignalLength);
  //   vsle( vecd2,inSignal_real, MASKREAD_ON, MASKWRITE_ON, rxSignalLength);
  //   vsetshamt(fractionLength);
  //   softbit3 = vadd( vecd6neg,inSignal_real, MASKREAD_ON, rxSignalLength);
  //   softbit3 = vmul( vecfactor3neg,softbit3, MASKREAD_ON, rxSignalLength);
  //   vsetshamt(0);
  //   vsgt(inSignal_real, vecd2, MASKREAD_OFF, MASKWRITE_ON, rxSignalLength);
  //   vbrdcst(tempi8, 0, MASKREAD_OFF, rxSignalLength);
  //   vsle( tempi8,inSignal_real, MASKREAD_ON, MASKWRITE_ON, rxSignalLength);
  //   vsetshamt(fractionLength);
  //   softbit3 = vadd( vecd5neg,inSignal_real, MASKREAD_ON, rxSignalLength);
  //   softbit3 = vmul( vecfactor4neg,softbit3, MASKREAD_ON, rxSignalLength);
  //   vsetshamt(0);
  //   vsgt(inSignal_real, 0, MASKREAD_OFF, MASKWRITE_ON, rxSignalLength);
  //   vsle( vecd2neg,inSignal_real, MASKREAD_ON, MASKWRITE_ON, rxSignalLength);
  //   vsetshamt(fractionLength);
  //   softbit3 = vadd( vecd5,inSignal_real, MASKREAD_ON, rxSignalLength);
  //   softbit3 = vmul( vecfactor4,softbit3, MASKREAD_ON, rxSignalLength);
  //   vsetshamt(0);
  //   vsgt(inSignal_real, vecd2neg, MASKREAD_OFF, MASKWRITE_ON, rxSignalLength);
  //   vsle( vecd4neg,inSignal_real, MASKREAD_ON, MASKWRITE_ON, rxSignalLength);
  //   vsetshamt(fractionLength);
  //   softbit3 = vadd( vecd6,inSignal_real, MASKREAD_ON, rxSignalLength);
  //   softbit3 = vmul( vecfactor3,softbit3, MASKREAD_ON, rxSignalLength);
  //   vsetshamt(0);
  //   vsgt(inSignal_real, vecd4neg, MASKREAD_OFF, MASKWRITE_ON, rxSignalLength);
  //   vsle( vecd6neg,inSignal_real, MASKREAD_ON, MASKWRITE_ON, rxSignalLength);
  //   vsetshamt(fractionLength);
  //   softbit3 = vadd( vecd7,inSignal_real, MASKREAD_ON, rxSignalLength);
  //   softbit3 = vmul( vecfactor2,softbit3, MASKREAD_ON, rxSignalLength);
  //   vsetshamt(0);
  //   vsgt(inSignal_real, vecd6neg, MASKREAD_OFF, MASKWRITE_ON, rxSignalLength);
  //   vsle( vecd10neg,inSignal_real, MASKREAD_ON, MASKWRITE_ON, rxSignalLength);
  //   vsetshamt(fractionLength);
  //   softbit3 = vadd( vecd8,inSignal_real, MASKREAD_ON, rxSignalLength);
  //   softbit3 = vmul( vecfactor,softbit3, MASKREAD_ON, rxSignalLength);
  //   vsetshamt(0);
  //   vsgt(inSignal_real, vecd10neg, MASKREAD_OFF, MASKWRITE_ON, rxSignalLength);
  //   vsle( vecd12neg,inSignal_real, MASKREAD_ON, MASKWRITE_ON, rxSignalLength);
  //   vsetshamt(fractionLength);
  //   softbit3 = vadd( vecd9,inSignal_real, MASKREAD_ON, rxSignalLength);
  //   softbit3 = vmul( vecfactor2,softbit3, MASKREAD_ON, rxSignalLength);
  //   vsetshamt(0);
  //   vsgt(inSignal_real, vecd12neg, MASKREAD_OFF, MASKWRITE_ON, rxSignalLength);
  //   vsle( vecd14neg,inSignal_real, MASKREAD_ON, MASKWRITE_ON, rxSignalLength);
  //   vsetshamt(fractionLength);
  //   softbit3 = vadd( vecd10,inSignal_real, MASKREAD_ON, rxSignalLength);
  //   softbit3 = vmul( vecfactor3,softbit3, MASKREAD_ON, rxSignalLength);
  //   vsetshamt(0);
  //   vsgt(inSignal_real, vecd14neg, MASKREAD_OFF, MASKWRITE_ON, rxSignalLength);
  //   vsetshamt(fractionLength);
  //   softbit3 = vadd( vecd11,inSignal_real, MASKREAD_ON, rxSignalLength);
  //   softbit3 = vmul( vecfactor4,softbit3, MASKREAD_ON, rxSignalLength);
  //   vsetshamt(0);

  //   /* softbit4 */
  //   factor = 4 * d * nVar;

  //   vbrdcst(vecfactor, factor, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecfactorneg, -factor, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecfactor2, 2 * factor, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecfactor2neg, -2 * factor, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecfactor3, 3 * factor, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecfactor3neg, -3 * factor, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecfactor4, 4 * factor, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecfactor4neg, -4 * factor, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecfactor5, 5 * factor, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecfactor5neg, -5 * factor, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecfactor6, 6 * factor, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecfactor6neg, -6 * factor, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecfactor7, 7 * factor, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecfactor7neg, -7 * factor, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecfactor8, 8 * factor, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecfactor8neg, -8 * factor, MASKREAD_OFF, rxSignalLength);

  //   vsle(inSignal_imag, vecd14, MASKREAD_OFF, MASKWRITE_ON, rxSignalLength);
  //   vsetshamt(fractionLength);
  //   softbit4 = vadd( vecd11neg,inSignal_imag, MASKREAD_ON, rxSignalLength);
  //   softbit4 = vmul( vecfactor4neg, softbit4, MASKREAD_ON, rxSignalLength);
  //   vsetshamt(0);
  //   vsgt(inSignal_imag, vecd14, MASKREAD_OFF, MASKWRITE_ON, rxSignalLength);
  //   vsle( vecd12,inSignal_imag, MASKREAD_ON, MASKWRITE_ON, rxSignalLength);
  //   vsetshamt(fractionLength);
  //   softbit4 = vadd( vecd10neg,inSignal_imag, MASKREAD_ON, rxSignalLength);
  //   softbit4 = vmul( vecfactor3neg,softbit4, MASKREAD_ON, rxSignalLength);
  //   vsetshamt(0);
  //   vsgt(inSignal_imag, vecd12, MASKREAD_OFF, MASKWRITE_ON, rxSignalLength);
  //   vsle( vecd10,inSignal_imag, MASKREAD_ON, MASKWRITE_ON, rxSignalLength);
  //   vsetshamt(fractionLength);
  //   softbit4 = vadd( vecd9neg,inSignal_imag, MASKREAD_ON, rxSignalLength);
  //   softbit4 = vmul( vecfactor2neg,softbit4, MASKREAD_ON, rxSignalLength);
  //   vsetshamt(0);
  //   vsgt(inSignal_imag, vecd10, MASKREAD_OFF, MASKWRITE_ON, rxSignalLength);
  //   vsle( vecd6,inSignal_imag, MASKREAD_ON, MASKWRITE_ON, rxSignalLength);
  //   vsetshamt(fractionLength);
  //   softbit4 = vadd( vecd8neg,inSignal_imag, MASKREAD_ON, rxSignalLength);
  //   softbit4 = vmul( vecfactor2,softbit4, MASKREAD_ON, rxSignalLength);
  //   vsetshamt(0);
  //   vsgt(inSignal_imag, vecd6, MASKREAD_OFF, MASKWRITE_ON, rxSignalLength);
  //   vsle( vecd4,inSignal_imag, MASKREAD_ON, MASKWRITE_ON, rxSignalLength);
  //   vsetshamt(fractionLength);
  //   softbit4 = vadd( vecd7neg,inSignal_imag, MASKREAD_ON, rxSignalLength);
  //   softbit4 = vmul( vecfactor2neg,softbit4, MASKREAD_ON, rxSignalLength);
  //   vsetshamt(0);
  //   vsgt(inSignal_imag, vecd4, MASKREAD_OFF, MASKWRITE_ON, rxSignalLength);
  //   vsle( vecd2,inSignal_imag, MASKREAD_ON, MASKWRITE_ON, rxSignalLength);
  //   vsetshamt(fractionLength);
  //   softbit4 = vadd( vecd6neg,inSignal_imag, MASKREAD_ON, rxSignalLength);
  //   softbit4 = vmul( vecfactor3neg,softbit4, MASKREAD_ON, rxSignalLength);
  //   vsetshamt(0);
  //   vsgt(inSignal_imag, vecd2, MASKREAD_OFF, MASKWRITE_ON, rxSignalLength);
  //   vbrdcst(tempi8, 0, MASKREAD_OFF, rxSignalLength);
  //   vsle( tempi8,inSignal_imag, MASKREAD_ON, MASKWRITE_ON, rxSignalLength);
  //   vsetshamt(fractionLength);
  //   softbit4 = vadd( vecd5neg,inSignal_imag, MASKREAD_ON, rxSignalLength);
  //   softbit4 = vmul( vecfactor4neg,softbit4, MASKREAD_ON, rxSignalLength);
  //   vsetshamt(0);
  //   vsgt(inSignal_imag, 0, MASKREAD_OFF, MASKWRITE_ON, rxSignalLength);
  //   vsle( vecd2neg,inSignal_imag, MASKREAD_ON, MASKWRITE_ON, rxSignalLength);
  //   vsetshamt(fractionLength);
  //   softbit4 = vadd( vecd5,inSignal_imag, MASKREAD_ON, rxSignalLength);
  //   softbit4 = vmul( vecfactor4,softbit4, MASKREAD_ON, rxSignalLength);
  //   vsetshamt(0);
  //   vsgt(inSignal_imag, vecd2neg, MASKREAD_OFF, MASKWRITE_ON, rxSignalLength);
  //   vsle( vecd4neg,inSignal_imag, MASKREAD_ON, MASKWRITE_ON, rxSignalLength);
  //   vsetshamt(fractionLength);
  //   softbit4 = vadd( vecd6,inSignal_imag, MASKREAD_ON, rxSignalLength);
  //   softbit4 = vmul( vecfactor3,softbit4, MASKREAD_ON, rxSignalLength);
  //   vsetshamt(0);
  //   vsgt(inSignal_imag, vecd4neg, MASKREAD_OFF, MASKWRITE_ON, rxSignalLength);
  //   vsle( vecd6neg,inSignal_imag, MASKREAD_ON, MASKWRITE_ON, rxSignalLength);
  //   vsetshamt(fractionLength);
  //   softbit4 = vadd( vecd7,inSignal_imag, MASKREAD_ON, rxSignalLength);
  //   softbit4 = vmul( vecfactor2,softbit4, MASKREAD_ON, rxSignalLength);
  //   vsetshamt(0);
  //   vsgt(inSignal_imag, vecd6neg, MASKREAD_OFF, MASKWRITE_ON, rxSignalLength);
  //   vsle( vecd10neg,inSignal_imag, MASKREAD_ON, MASKWRITE_ON, rxSignalLength);
  //   vsetshamt(fractionLength);
  //   softbit4 = vadd( vecd8,inSignal_imag, MASKREAD_ON, rxSignalLength);
  //   softbit4 = vmul( vecfactor,softbit4, MASKREAD_ON, rxSignalLength);
  //   vsetshamt(0);
  //   vsgt(inSignal_imag, vecd10neg, MASKREAD_OFF, MASKWRITE_ON, rxSignalLength);
  //   vsle( vecd12neg,inSignal_imag, MASKREAD_ON, MASKWRITE_ON, rxSignalLength);
  //   vsetshamt(fractionLength);
  //   softbit4 = vadd( vecd9,inSignal_imag, MASKREAD_ON, rxSignalLength);
  //   softbit4 = vmul( vecfactor2,softbit4, MASKREAD_ON, rxSignalLength);
  //   vsetshamt(0);
  //   vsgt(inSignal_imag, vecd12neg, MASKREAD_OFF, MASKWRITE_ON, rxSignalLength);
  //   vsle( vecd14neg,inSignal_imag, MASKREAD_ON, MASKWRITE_ON, rxSignalLength);
  //   vsetshamt(fractionLength);
  //   softbit4 = vadd( vecd10,inSignal_imag, MASKREAD_ON, rxSignalLength);
  //   softbit4 = vmul( vecfactor3,softbit4, MASKREAD_ON, rxSignalLength);
  //   vsetshamt(0);
  //   vsgt(inSignal_imag, vecd14neg, MASKREAD_OFF, MASKWRITE_ON, rxSignalLength);
  //   vsetshamt(fractionLength);
  //   softbit4 = vadd( vecd11,inSignal_imag, MASKREAD_ON, rxSignalLength);
  //   softbit4 = vmul( vecfactor4,softbit4, MASKREAD_ON, rxSignalLength);
  //   vsetshamt(0);

  //   /* softbit5 */
  //   factor = 4 * d * nVar;

  //   vbrdcst(vecfactor, factor, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecfactorneg, -factor, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecfactor2, 2 * factor, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecfactor2neg, -2 * factor, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecfactor3, 3 * factor, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecfactor3neg, -3 * factor, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecfactor4, 4 * factor, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecfactor4neg, -4 * factor, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecfactor5, 5 * factor, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecfactor5neg, -5 * factor, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecfactor6, 6 * factor, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecfactor6neg, -6 * factor, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecfactor7, 7 * factor, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecfactor7neg, -7 * factor, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecfactor8, 8 * factor, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecfactor8neg, -8 * factor, MASKREAD_OFF, rxSignalLength);

  //   vsle(inSignal_real, vecd14, MASKREAD_OFF, MASKWRITE_ON, rxSignalLength);
  //   vsetshamt(fractionLength);
  //   softbit5 = vadd( vecd13neg,inSignal_real, MASKREAD_ON, rxSignalLength);
  //   softbit5 = vmul( vecfactor2neg,softbit5, MASKREAD_ON, rxSignalLength);
  //   vsetshamt(0);
  //   vsgt(inSignal_real, vecd14, MASKREAD_OFF, MASKWRITE_ON, rxSignalLength);
  //   vsle( vecd10,inSignal_real, MASKREAD_ON, MASKWRITE_ON, rxSignalLength);
  //   vsetshamt(fractionLength);
  //   softbit5 = vadd( vecd12neg,inSignal_real, MASKREAD_ON, rxSignalLength);
  //   softbit5 = vmul( vecfactor2,softbit5, MASKREAD_ON, rxSignalLength);
  //   vsetshamt(0);
  //   vsgt(inSignal_real, vecd10, MASKREAD_OFF, MASKWRITE_ON, rxSignalLength);
  //   vsle( vecd8,inSignal_real, MASKREAD_ON, MASKWRITE_ON, rxSignalLength);
  //   vsetshamt(fractionLength);
  //   softbit5 = vadd( vecd11neg,inSignal_real, MASKREAD_ON, rxSignalLength);
  //   softbit5 = vmul( vecfactor2neg,softbit5, MASKREAD_ON, rxSignalLength);
  //   vsetshamt(0);
  //   vsgt(inSignal_real, vecd8, MASKREAD_OFF, MASKWRITE_ON, rxSignalLength);
  //   vsle( vecd6,inSignal_real, MASKREAD_ON, MASKWRITE_ON, rxSignalLength);
  //   vsetshamt(fractionLength);
  //   softbit5 = vadd(vecd5neg,inSignal_real, MASKREAD_ON, rxSignalLength);
  //   softbit5 = vmul( vecfactor2,softbit5, MASKREAD_ON, rxSignalLength);
  //   vsetshamt(0);
  //   vsgt(inSignal_real, vecd6, MASKREAD_OFF, MASKWRITE_ON, rxSignalLength);
  //   vsle( vecd2,inSignal_real, MASKREAD_ON, MASKWRITE_ON, rxSignalLength);
  //   vsetshamt(fractionLength);
  //   softbit5 = vadd( vecd4neg,inSignal_real, MASKREAD_ON, rxSignalLength);
  //   softbit5 = vmul(vecfactor,softbit5,  MASKREAD_ON, rxSignalLength);
  //   vsetshamt(0);
  //   vsgt(inSignal_real, vecd2, MASKREAD_OFF, MASKWRITE_ON, rxSignalLength);
  //   vbrdcst(tempi8, 0, MASKREAD_OFF, rxSignalLength);
  //   vsle( tempi8,inSignal_real, MASKREAD_ON, MASKWRITE_ON, rxSignalLength);
  //   vsetshamt(fractionLength);
  //   softbit5 = vadd( vecd3neg, inSignal_real,MASKREAD_ON, rxSignalLength);
  //   softbit5 = vmul( vecfactor2,softbit5, MASKREAD_ON, rxSignalLength);
  //   vsetshamt(0);
  //   vsgt(inSignal_real, 0, MASKREAD_OFF, MASKWRITE_ON, rxSignalLength);
  //   vsle( vecd2neg,inSignal_real, MASKREAD_ON, MASKWRITE_ON, rxSignalLength);
  //   vsetshamt(fractionLength);
  //   softbit5 = vadd( vecd3,inSignal_real, MASKREAD_ON, rxSignalLength);
  //   softbit5 = vmul( vecfactor2neg,softbit5, MASKREAD_ON, rxSignalLength);
  //   vsetshamt(0);
  //   vsgt(inSignal_real, vecd2neg, MASKREAD_OFF, MASKWRITE_ON, rxSignalLength);
  //   vsle( vecd6neg,inSignal_real, MASKREAD_ON, MASKWRITE_ON, rxSignalLength);
  //   vsetshamt(fractionLength);
  //   softbit5 = vadd( vecd4,inSignal_real, MASKREAD_ON, rxSignalLength);
  //   softbit5 = vmul( vecfactor2,softbit5, MASKREAD_ON, rxSignalLength);
  //   vsetshamt(0);
  //   vsgt(inSignal_real, vecd6neg, MASKREAD_OFF, MASKWRITE_ON, rxSignalLength);
  //   vsle( vecd8neg,inSignal_real, MASKREAD_ON, MASKWRITE_ON, rxSignalLength);
  //   vsetshamt(fractionLength);
  //   softbit5 = vadd( vecd5,inSignal_real, MASKREAD_ON, rxSignalLength);
  //   softbit5 = vmul( vecfactor2neg,softbit5, MASKREAD_ON, rxSignalLength);
  //   vsetshamt(0);
  //   vsgt(inSignal_real, vecd8neg, MASKREAD_OFF, MASKWRITE_ON, rxSignalLength);
  //   vsle( vecd10neg,inSignal_real, MASKREAD_ON, MASKWRITE_ON, rxSignalLength);
  //   vsetshamt(fractionLength);
  //   softbit5 = vadd( vecd11,inSignal_real, MASKREAD_ON, rxSignalLength);
  //   softbit5 = vmul( vecfactor2,softbit5, MASKREAD_ON, rxSignalLength);
  //   vsetshamt(0);
  //   vsgt(inSignal_real, vecd10neg, MASKREAD_OFF, MASKWRITE_ON, rxSignalLength);
  //   vsle( vecd14neg,inSignal_real, MASKREAD_ON, MASKWRITE_ON, rxSignalLength);
  //   vsetshamt(fractionLength);
  //   softbit5 = vadd( vecd12,inSignal_real, MASKREAD_ON, rxSignalLength);
  //   softbit5 = vmul( vecfactor,softbit5, MASKREAD_ON, rxSignalLength);
  //   vsetshamt(0);
  //   vsgt(inSignal_real, vecd14neg, MASKREAD_OFF, MASKWRITE_ON, rxSignalLength);
  //   vsetshamt(fractionLength);
  //   softbit5 = vadd( vecd13,inSignal_real, MASKREAD_ON, rxSignalLength);
  //   softbit5 = vmul( vecfactor2,softbit5, MASKREAD_ON, rxSignalLength);
  //   vsetshamt(0);

  //   /* softbit6 */
  //   factor = 4 * d * nVar;

  //   vbrdcst(vecfactor, factor, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecfactorneg, -factor, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecfactor2, 2 * factor, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecfactor2neg, -2 * factor, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecfactor3, 3 * factor, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecfactor3neg, -3 * factor, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecfactor4, 4 * factor, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecfactor4neg, -4 * factor, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecfactor5, 5 * factor, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecfactor5neg, -5 * factor, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecfactor6, 6 * factor, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecfactor6neg, -6 * factor, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecfactor7, 7 * factor, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecfactor7neg, -7 * factor, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecfactor8, 8 * factor, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecfactor8neg, -8 * factor, MASKREAD_OFF, rxSignalLength);

  //   vsle(inSignal_imag, vecd14, MASKREAD_OFF, MASKWRITE_ON, rxSignalLength);
  //   vsetshamt(fractionLength);
  //   softbit6 = vadd( vecd13neg,inSignal_imag, MASKREAD_ON, rxSignalLength);
  //   softbit6 = vmul( vecfactor2neg,softbit6, MASKREAD_ON, rxSignalLength);
  //   vsetshamt(0);
  //   vsgt(inSignal_imag, vecd14, MASKREAD_OFF, MASKWRITE_ON, rxSignalLength);
  //   vsle( vecd10,inSignal_imag, MASKREAD_ON, MASKWRITE_ON, rxSignalLength);
  //   vsetshamt(fractionLength);
  //   softbit6 = vadd( vecd12neg, inSignal_imag,MASKREAD_ON, rxSignalLength);
  //   softbit6 = vmul( vecfactor2,softbit6, MASKREAD_ON, rxSignalLength);
  //   vsetshamt(0);
  //   vsgt(inSignal_imag, vecd10, MASKREAD_OFF, MASKWRITE_ON, rxSignalLength);
  //   vsle( vecd8, inSignal_imag, MASKREAD_ON, MASKWRITE_ON, rxSignalLength);
  //   vsetshamt(fractionLength);
  //   softbit6 = vadd( vecd11neg, inSignal_imag, MASKREAD_ON, rxSignalLength);
  //   softbit6 = vmul( vecfactor2neg, softbit6, MASKREAD_ON, rxSignalLength);
  //   vsetshamt(0);
  //   vsgt(inSignal_imag, vecd8, MASKREAD_OFF, MASKWRITE_ON, rxSignalLength);
  //   vsle( vecd6,inSignal_imag, MASKREAD_ON, MASKWRITE_ON, rxSignalLength);
  //   vsetshamt(fractionLength);
  //   softbit6 = vadd( vecd5neg, inSignal_imag, MASKREAD_ON, rxSignalLength);
  //   softbit6 = vmul( vecfactor2,softbit6, MASKREAD_ON, rxSignalLength);
  //   vsetshamt(0);
  //   vsgt(inSignal_imag, vecd6, MASKREAD_OFF, MASKWRITE_ON, rxSignalLength);
  //   vsle( vecd2,inSignal_imag, MASKREAD_ON, MASKWRITE_ON, rxSignalLength);
  //   vsetshamt(fractionLength);
  //   softbit6 = vadd( vecd4neg,inSignal_imag, MASKREAD_ON, rxSignalLength);
  //   softbit6 = vmul( vecfactor,softbit6, MASKREAD_ON, rxSignalLength);
  //   vsetshamt(0);
  //   vsgt(inSignal_imag, vecd2, MASKREAD_OFF, MASKWRITE_ON, rxSignalLength);
  //   vbrdcst(tempi8, 0, MASKREAD_OFF, rxSignalLength);
  //   vsle( tempi8,inSignal_imag, MASKREAD_ON, MASKWRITE_ON, rxSignalLength);
  //   vsetshamt(fractionLength);
  //   softbit6 = vadd( vecd3neg,inSignal_imag, MASKREAD_ON, rxSignalLength);
  //   softbit6 = vmul( vecfactor2,softbit6, MASKREAD_ON, rxSignalLength);
  //   vsetshamt(0);
  //   vsgt(inSignal_imag, 0, MASKREAD_OFF, MASKWRITE_ON, rxSignalLength);
  //   vsle( vecd2neg,inSignal_imag, MASKREAD_ON, MASKWRITE_ON, rxSignalLength);
  //   vsetshamt(fractionLength);
  //   softbit6 = vadd( vecd3,inSignal_imag, MASKREAD_ON, rxSignalLength);
  //   softbit6 = vmul( vecfactor2neg,softbit6, MASKREAD_ON, rxSignalLength);
  //   vsetshamt(0);
  //   vsgt(inSignal_imag, vecd2neg, MASKREAD_OFF, MASKWRITE_ON, rxSignalLength);
  //   vsle( vecd6neg,inSignal_imag, MASKREAD_ON, MASKWRITE_ON, rxSignalLength);
  //   vsetshamt(fractionLength);
  //   softbit6 = vadd( vecd4,inSignal_imag, MASKREAD_ON, rxSignalLength);
  //   softbit6 = vmul( vecfactor2,softbit6, MASKREAD_ON, rxSignalLength);
  //   vsetshamt(0);
  //   vsgt(inSignal_imag, vecd6neg, MASKREAD_OFF, MASKWRITE_ON, rxSignalLength);
  //   vsle( vecd8neg,inSignal_imag, MASKREAD_ON, MASKWRITE_ON, rxSignalLength);
  //   vsetshamt(fractionLength);
  //   softbit6 = vadd( vecd5,inSignal_imag, MASKREAD_ON, rxSignalLength);
  //   softbit6 = vmul( vecfactor2neg,softbit6, MASKREAD_ON, rxSignalLength);
  //   vsetshamt(0);
  //   vsgt(inSignal_imag, vecd8neg, MASKREAD_OFF, MASKWRITE_ON, rxSignalLength);
  //   vsle( vecd10neg,inSignal_imag, MASKREAD_ON, MASKWRITE_ON, rxSignalLength);
  //   vsetshamt(fractionLength);
  //   softbit6 = vadd( vecd11,inSignal_imag, MASKREAD_ON, rxSignalLength);
  //   softbit6 = vmul( vecfactor2, softbit6,MASKREAD_ON, rxSignalLength);
  //   vsetshamt(0);
  //   vsgt(inSignal_imag, vecd10neg, MASKREAD_OFF, MASKWRITE_ON, rxSignalLength);
  //   vsle( vecd14neg,inSignal_imag, MASKREAD_ON, MASKWRITE_ON, rxSignalLength);
  //   vsetshamt(fractionLength);
  //   softbit6 = vadd( vecd12, inSignal_imag, MASKREAD_ON, rxSignalLength);
  //   softbit6 = vmul( vecfactor,softbit6, MASKREAD_ON, rxSignalLength);
  //   vsetshamt(0);
  //   vsgt(inSignal_imag, vecd14neg, MASKREAD_OFF, MASKWRITE_ON, rxSignalLength);
  //   vsetshamt(fractionLength);
  //   softbit6 = vadd( vecd13,inSignal_imag, MASKREAD_ON, rxSignalLength);
  //   softbit6 = vmul( vecfactor2,softbit6, MASKREAD_ON, rxSignalLength);
  //   vsetshamt(0);

  //   /* softbit7 */
  //   factor = 4 * d * nVar;

  //   vbrdcst(vecfactor, factor, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecfactorneg, -factor, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecfactor2, 2 * factor, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecfactor2neg, -2 * factor, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecfactor3, 3 * factor, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecfactor3neg, -3 * factor, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecfactor4, 4 * factor, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecfactor4neg, -4 * factor, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecfactor5, 5 * factor, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecfactor5neg, -5 * factor, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecfactor6, 6 * factor, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecfactor6neg, -6 * factor, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecfactor7, 7 * factor, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecfactor7neg, -7 * factor, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecfactor8, 8 * factor, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecfactor8neg, -8 * factor, MASKREAD_OFF, rxSignalLength);

  //   vsle(inSignal_real, vecd12, MASKREAD_OFF, MASKWRITE_ON, rxSignalLength);
  //   vsetshamt(fractionLength);
  //   softbit7 = vadd( vecd14neg,inSignal_real, MASKREAD_ON, rxSignalLength);
  //   softbit7 = vmul( vecfactor2,softbit7, MASKREAD_ON, rxSignalLength);
  //   vsetshamt(0);
  //   vsgt(inSignal_real, vecd12, MASKREAD_OFF, MASKWRITE_ON, rxSignalLength);
  //   vsle( vecd8,inSignal_real, MASKREAD_ON, MASKWRITE_ON, rxSignalLength);
  //   vsetshamt(fractionLength);
  //   softbit7 = vadd( vecd10neg, inSignal_real, MASKREAD_ON, rxSignalLength);
  //   softbit7 = vmul( vecfactor,softbit7, MASKREAD_ON, rxSignalLength);
  //   vsetshamt(0);
  //   vsgt(inSignal_real, vecd8, MASKREAD_OFF, MASKWRITE_ON, rxSignalLength);
  //   vsle( vecd4,inSignal_real, MASKREAD_ON, MASKWRITE_ON, rxSignalLength);
  //   vsetshamt(fractionLength);
  //   softbit7 = vadd( vecd6neg, inSignal_real, MASKREAD_ON, rxSignalLength);
  //   softbit7 = vmul( vecfactor2,softbit7, MASKREAD_ON, rxSignalLength);
  //   vsetshamt(0);
  //   vsgt(inSignal_real, vecd4, MASKREAD_OFF, MASKWRITE_ON, rxSignalLength);
  //   vbrdcst(tempi8, 0, MASKREAD_OFF, rxSignalLength);
  //   vsle( tempi8,inSignal_real, MASKREAD_ON, MASKWRITE_ON, rxSignalLength);
  //   vsetshamt(fractionLength);
  //   softbit7 = vadd( vecd2neg,inSignal_real, MASKREAD_ON, rxSignalLength);
  //   softbit7 = vmul( vecfactor,softbit7, MASKREAD_ON, rxSignalLength);
  //   vsetshamt(0);
  //   vsgt(inSignal_real, 0, MASKREAD_OFF, MASKWRITE_ON, rxSignalLength);
  //   vsle( vecd4neg,inSignal_real, MASKREAD_ON, MASKWRITE_ON, rxSignalLength);
  //   vsetshamt(fractionLength);
  //   softbit7 = vadd( vecd2,inSignal_real, MASKREAD_ON, rxSignalLength);
  //   softbit7 = vmul( vecfactor2,softbit7, MASKREAD_ON, rxSignalLength);
  //   vsetshamt(0);
  //   vsgt(inSignal_real, vecd4neg, MASKREAD_OFF, MASKWRITE_ON, rxSignalLength);
  //   vsle( vecd8neg, inSignal_real, MASKREAD_ON, MASKWRITE_ON, rxSignalLength);
  //   vsetshamt(fractionLength);
  //   softbit7 = vadd( vecd6, inSignal_real, MASKREAD_ON, rxSignalLength);
  //   softbit7 = vmul( vecfactor, softbit7, MASKREAD_ON, rxSignalLength);
  //   vsetshamt(0);
  //   vsgt(inSignal_real, vecd8neg, MASKREAD_OFF, MASKWRITE_ON, rxSignalLength);
  //   vsle( vecd12neg,inSignal_real, MASKREAD_ON, MASKWRITE_ON, rxSignalLength);
  //   vsetshamt(fractionLength);
  //   softbit7 = vadd( vecd10,inSignal_real, MASKREAD_ON, rxSignalLength);
  //   softbit7 = vmul( vecfactor2, softbit7 ,MASKREAD_ON, rxSignalLength);
  //   vsetshamt(0);
  //   vsgt(inSignal_real, vecd14neg, MASKREAD_OFF, MASKWRITE_ON, rxSignalLength);
  //   vsetshamt(fractionLength);
  //   softbit7 = vadd( vecd14,inSignal_real,  MASKREAD_ON, rxSignalLength);
  //   softbit7 = vmul( vecfactor,softbit7, MASKREAD_ON, rxSignalLength);
  //   vsetshamt(0);

  //   /* softbit8 */
  //   factor = 4 * d * nVar;

  //   vbrdcst(vecfactor, factor, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecfactorneg, -factor, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecfactor2, 2 * factor, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecfactor2neg, -2 * factor, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecfactor3, 3 * factor, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecfactor3neg, -3 * factor, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecfactor4, 4 * factor, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecfactor4neg, -4 * factor, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecfactor5, 5 * factor, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecfactor5neg, -5 * factor, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecfactor6, 6 * factor, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecfactor6neg, -6 * factor, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecfactor7, 7 * factor, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecfactor7neg, -7 * factor, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecfactor8, 8 * factor, MASKREAD_OFF, rxSignalLength);
  //   vbrdcst(vecfactor8neg, -8 * factor, MASKREAD_OFF, rxSignalLength);

  //   vsle(inSignal_imag, vecd12, MASKREAD_OFF, MASKWRITE_ON, rxSignalLength);
  //   vsetshamt(fractionLength);
  //   softbit8 = vadd( vecd14neg, inSignal_imag , MASKREAD_ON, rxSignalLength);
  //   softbit8 = vmul( vecfactor2,softbit8, MASKREAD_ON, rxSignalLength);
  //   vsetshamt(0);

  //   vsgt(inSignal_imag, vecd12, MASKREAD_OFF, MASKWRITE_ON, rxSignalLength);
  //   vsle( vecd8,inSignal_imag, MASKREAD_ON, MASKWRITE_ON, rxSignalLength);
  //   vsetshamt(fractionLength);
  //   softbit8 = vadd( vecd10neg,inSignal_imag,  MASKREAD_ON, rxSignalLength);
  //   softbit8 = vmul( vecfactor,softbit8, MASKREAD_ON, rxSignalLength);
  //   vsetshamt(0);

  //   vsgt(inSignal_imag, vecd8, MASKREAD_OFF, MASKWRITE_ON, rxSignalLength);
  //   vsle( vecd4,inSignal_imag, MASKREAD_ON, MASKWRITE_ON, rxSignalLength);
  //   vsetshamt(fractionLength);
  //   softbit8 = vadd( vecd6neg,inSignal_imag, MASKREAD_ON, rxSignalLength);
  //   softbit8 = vmul( vecfactor2,softbit8, MASKREAD_ON, rxSignalLength);
  //   vsetshamt(0);

  //   vsgt(inSignal_imag, vecd4, MASKREAD_OFF, MASKWRITE_ON, rxSignalLength);
  //   vbrdcst(tempi8, 0, MASKREAD_OFF, rxSignalLength);
  //   vsle( tempi8,inSignal_imag, MASKREAD_ON, MASKWRITE_ON, rxSignalLength);
  //   vsetshamt(fractionLength);
  //   softbit8 = vadd( vecd2neg,inSignal_imag, MASKREAD_ON, rxSignalLength);
  //   softbit8 = vmul( vecfactor,softbit8, MASKREAD_ON, rxSignalLength);
  //   vsetshamt(0);

  //   vsgt(inSignal_imag, 0, MASKREAD_OFF, MASKWRITE_ON, rxSignalLength);
  //   vsle( vecd4neg,inSignal_imag, MASKREAD_ON, MASKWRITE_ON, rxSignalLength);
  //   vsetshamt(fractionLength);
  //   softbit8 = vadd( vecd2,inSignal_imag, MASKREAD_ON, rxSignalLength);
  //   softbit8 = vmul( vecfactor2,softbit8, MASKREAD_ON, rxSignalLength);
  //   vsetshamt(0);

  //   vsgt(inSignal_imag, vecd4neg, MASKREAD_OFF, MASKWRITE_ON, rxSignalLength);
  //   vsle( vecd8neg,inSignal_imag, MASKREAD_ON, MASKWRITE_ON, rxSignalLength);
  //   vsetshamt(fractionLength);
  //   softbit8 = vadd( vecd6,inSignal_imag, MASKREAD_ON, rxSignalLength);
  //   softbit8 = vmul( vecfactor,softbit8, MASKREAD_ON, rxSignalLength);
  //   vsetshamt(0);

  //   vsgt(inSignal_imag, vecd8neg, MASKREAD_OFF, MASKWRITE_ON, rxSignalLength);
  //   vsle( vecd12neg,inSignal_imag, MASKREAD_ON, MASKWRITE_ON, rxSignalLength);
  //   vsetshamt(fractionLength);
  //   softbit8 = vadd( vecd10,inSignal_imag, MASKREAD_ON, rxSignalLength);
  //   softbit8 = vmul( vecfactor2,softbit8, MASKREAD_ON, rxSignalLength);
  //   vsetshamt(0);

  //   vsgt(inSignal_imag, vecd14neg, MASKREAD_OFF, MASKWRITE_ON, rxSignalLength);
  //   vsetshamt(fractionLength);
  //   softbit8 = vadd( vecd14,inSignal_imag, MASKREAD_ON, rxSignalLength);
  //   softbit8 = vmul( vecfactor,softbit8, MASKREAD_ON, rxSignalLength);
  //   vsetshamt(0);

  //   /* data merge */
  //   __v2048i16 softbit_shuffle_index_tmp;
  //   vclaim(softbit_shuffle_index_tmp);
  //   vrange(softbit_shuffle_index_tmp, rxSignalLength);
  //   softbit_shuffle_index_tmp = vmul(softbit_shuffle_index_tmp, 6, MASKREAD_OFF, rxSignalLength);
  //   vshuffle(softbit, softbit_shuffle_index_tmp, softbit1, SHUFFLE_SCATTER, rxSignalLength);
  //   softbit_shuffle_index_tmp = vadd(softbit_shuffle_index_tmp, 1, MASKREAD_OFF, rxSignalLength);
  //   vshuffle(softbit, softbit_shuffle_index_tmp, softbit2, SHUFFLE_SCATTER, rxSignalLength);
  //   softbit_shuffle_index_tmp = vadd(softbit_shuffle_index_tmp, 1, MASKREAD_OFF, rxSignalLength);
  //   vshuffle(softbit, softbit_shuffle_index_tmp, softbit3, SHUFFLE_SCATTER, rxSignalLength);
  //   softbit_shuffle_index_tmp = vadd(softbit_shuffle_index_tmp, 1, MASKREAD_OFF, rxSignalLength);
  //   vshuffle(softbit, softbit_shuffle_index_tmp, softbit4, SHUFFLE_SCATTER, rxSignalLength);
  //   softbit_shuffle_index_tmp = vadd(softbit_shuffle_index_tmp, 1, MASKREAD_OFF, rxSignalLength);
  //   vshuffle(softbit, softbit_shuffle_index_tmp, softbit5, SHUFFLE_SCATTER, rxSignalLength);
  //   softbit_shuffle_index_tmp = vadd(softbit_shuffle_index_tmp, 1, MASKREAD_OFF, rxSignalLength);
  //   vshuffle(softbit, softbit_shuffle_index_tmp, softbit6, SHUFFLE_SCATTER, rxSignalLength);
  //   softbit_shuffle_index_tmp = vadd(softbit_shuffle_index_tmp, 1, MASKREAD_OFF, rxSignalLength);
  //   vshuffle(softbit, softbit_shuffle_index_tmp, softbit7, SHUFFLE_SCATTER, rxSignalLength);
  //   softbit_shuffle_index_tmp = vadd(softbit_shuffle_index_tmp, 1, MASKREAD_OFF, rxSignalLength);
  //   vshuffle(softbit, softbit_shuffle_index_tmp, softbit6, SHUFFLE_SCATTER, rxSignalLength);
  // } break;
  default:
    break;
  }

  

  short length = rxSignalLength * nModulationSymb;
  short_struct demod_length;
  demod_length.data = length;

  
  softbit = vmul(softbit, -1, MASKREAD_OFF,length);
  softbit = vsadd(softbit, 0, MASKREAD_OFF,length);

  printf("demod_length = %hd\n",&length);
  // char word[20] = "symdemod finished";
  // printf("----------- %s -----------\n",&word);

  // vreturn(softbit, sizeof(softbit), &demod_length, sizeof(demod_length));
  vreturn(softbit, length, &demod_length, sizeof(short));
}