/**
  ******************************************************************************
  * @file           : Task_lteosfbcDecode.c
  * @author         : XiaoxiaoChen
  * @brief          : PBCH
  * @attention      : 
  * @date           : 2025/2/27
  ******************************************************************************
  */

  #include "riscv_printf.h"
  #include "venus.h"
  #include "stdint.h"
  #include "data_type.h"
  #include "vmath.h"

typedef short __v2048i16 __attribute__((ext_vector_type(2048)));
typedef char  __v4096i8 __attribute__((ext_vector_type(4096)));

// short         Nt           = 4;//Number of transmit antennas
// short         Nr           = 1;//Number of receive antennas
// short         inLength     = 240;

int Task_lteosfbcDecode(__v4096i8 in_real, __v4096i8 in_imag, __v4096i8 hest_real0, __v4096i8 hest_imag0, __v4096i8 hest_real1, __v4096i8 hest_imag1, __v4096i8 hest_real2, __v4096i8 hest_imag2, __v4096i8 hest_real3, __v4096i8 hest_imag3,
  __v2048i16 index0145, __v2048i16 index048, short_struct input_cellrefp, short_struct input_sequence_length) {
  
  short Nt  = input_cellrefp.data;
  short inLength = input_sequence_length.data;

  __v4096i8  out_real;
  __v4096i8  out_imag;
  __v4096i8  csi;
  vclaim(out_imag);
  vclaim(out_real);
  vclaim(csi);

  if(Nt == 2){
  /*-----------------------index----------------------------*/
    __v2048i16 index_240e;
    __v2048i16 index_240o;
    vclaim(index_240e);
    vclaim(index_240o);
    vrange(index_240e, inLength/2);
    index_240e = vmul(index_240e, 2, MASKREAD_OFF, inLength/2);
    index_240o = vsadd(index_240e, 1, MASKREAD_OFF, inLength/2);

  /*------------------------hest----------------------------*/
    __v4096i8 Hp11_real;
    __v4096i8 Hp22_real;
    __v4096i8 Hp12_real;
    __v4096i8 Hp21_real;
    vclaim(Hp11_real);
    vclaim(Hp22_real);
    vclaim(Hp12_real);
    vclaim(Hp21_real);
    vshuffle(Hp11_real, index_240e, hest_real0, SHUFFLE_GATHER, inLength/2);
    vshuffle(Hp22_real, index_240o, hest_real1, SHUFFLE_GATHER, inLength/2);
    vshuffle(Hp12_real, index_240e, hest_real1, SHUFFLE_GATHER, inLength/2);
    vshuffle(Hp21_real, index_240o, hest_real0, SHUFFLE_GATHER, inLength/2);

    __v4096i8 Hp11_imag;
    __v4096i8 Hp22_imag;
    __v4096i8 Hp12_imag;
    __v4096i8 Hp21_imag;
    vclaim(Hp11_imag);
    vclaim(Hp22_imag);
    vclaim(Hp12_imag);
    vclaim(Hp21_imag);
    vshuffle(Hp11_imag, index_240e, hest_imag0, SHUFFLE_GATHER, inLength/2);
    vshuffle(Hp22_imag, index_240o, hest_imag1, SHUFFLE_GATHER, inLength/2);
    vshuffle(Hp12_imag, index_240e, hest_imag1, SHUFFLE_GATHER, inLength/2);
    vshuffle(Hp21_imag, index_240o, hest_imag0, SHUFFLE_GATHER, inLength/2);

  /*----------------------------in----------------------------------*/
  __v4096i8 r1_real;
  __v4096i8 r2_real;
  __v4096i8 y1_real;
  __v4096i8 y2_real;
  __v4096i8 temp;
  vclaim(r1_real);
  vclaim(r2_real);
  vclaim(y1_real);
  vclaim(y2_real);
  __v4096i8 r1_imag;
  __v4096i8 r2_imag;
  __v4096i8 y1_imag;
  __v4096i8 y2_imag;
  vclaim(r1_imag);
  vclaim(r2_imag);
  vclaim(y1_imag);
  vclaim(y2_imag);
  vclaim(temp);  

  vshuffle(r1_real, index_240e, in_real, SHUFFLE_GATHER, inLength/2);
  vshuffle(r2_real, index_240o, in_real, SHUFFLE_GATHER, inLength/2);
  vshuffle(r1_imag, index_240e, in_imag, SHUFFLE_GATHER, inLength/2);
  vshuffle(r2_imag, index_240o, in_imag, SHUFFLE_GATHER, inLength/2);


  //输入1Q6
  //y1为2Q5
/*----------------------------y1----------------------------------*/
  vsetshamt(7);
  y1_real = vmul(r1_real, Hp11_real, MASKREAD_OFF, inLength/2);
  temp = vmul(r1_imag, Hp11_imag, MASKREAD_OFF, inLength/2);
  y1_real = vsadd(y1_real, temp, MASKREAD_OFF, inLength/2);
  temp = vmul(r2_real, Hp22_real, MASKREAD_OFF, inLength/2);
  y1_real = vsadd(y1_real, temp, MASKREAD_OFF, inLength/2);
  temp = vmul(r2_imag, Hp22_imag, MASKREAD_OFF, inLength/2);
  y1_real = vsadd(y1_real, temp, MASKREAD_OFF, inLength/2);

  y1_imag = vmul(r1_imag, Hp11_real, MASKREAD_OFF, inLength/2);
  temp = vmul(r1_real, Hp11_imag, MASKREAD_OFF, inLength/2);
  y1_imag = vssub(temp, y1_imag, MASKREAD_OFF, inLength/2);///注意顺序
  temp = vmul(r2_real, Hp22_imag, MASKREAD_OFF, inLength/2);
  y1_imag = vsadd(y1_imag, temp, MASKREAD_OFF, inLength/2);
  temp = vmul(r2_imag, Hp22_real, MASKREAD_OFF, inLength/2);
  y1_imag = vssub(temp, y1_imag, MASKREAD_OFF, inLength/2);
/*----------------------------y2----------------------------------*/
  y2_real = vmul(r1_real, Hp12_real, MASKREAD_OFF, inLength/2);

  vsetshamt(0);
  y2_real = vmul(y2_real, -1, MASKREAD_OFF, inLength/2);
  vsetshamt(7);
  temp = vmul(r1_imag, Hp12_imag, MASKREAD_OFF, inLength/2);
  y2_real = vssub(temp, y2_real, MASKREAD_OFF, inLength/2);///注意顺序
  temp = vmul(r2_real, Hp21_real, MASKREAD_OFF, inLength/2);
  y2_real = vsadd(y2_real, temp, MASKREAD_OFF, inLength/2);
  temp = vmul(r2_imag, Hp21_imag, MASKREAD_OFF, inLength/2);
  y2_real = vsadd(y2_real, temp, MASKREAD_OFF, inLength/2);

  y2_imag = vmul(r1_imag, Hp12_real, MASKREAD_OFF, inLength/2);
  temp = vmul(r1_real, Hp12_imag, MASKREAD_OFF, inLength/2);
  y2_imag = vssub(temp, y2_imag, MASKREAD_OFF, inLength/2);///注意顺序
  temp = vmul(r2_real, Hp21_imag, MASKREAD_OFF, inLength/2);
  y2_imag = vssub(temp, y2_imag, MASKREAD_OFF, inLength/2);///注意顺序
  temp = vmul(r2_imag, Hp21_real, MASKREAD_OFF, inLength/2);
  y2_imag = vsadd(y2_imag, temp, MASKREAD_OFF, inLength/2);

  vsetshamt(0);

  vsetshamt(7);
  y1_real = vmul(y1_real, 90, MASKREAD_OFF, inLength/2);//根号2
  y1_imag = vmul(y1_imag, 90, MASKREAD_OFF, inLength/2);
  y2_real = vmul(y2_real, 90, MASKREAD_OFF, inLength/2);
  y2_imag = vmul(y2_imag, 90, MASKREAD_OFF, inLength/2);
  vsetshamt(0);
/*----------------------------t1----------------------------------*/
  __v4096i8 t1;
  vclaim(t1);
  vsetshamt(8);
  t1 = vmul(Hp11_real, Hp11_real, MASKREAD_OFF, inLength/2);
  temp = vmul(Hp11_imag, Hp11_imag, MASKREAD_OFF, inLength/2);
  t1 = vsadd(t1, temp, MASKREAD_OFF, inLength/2);
  temp = vmul(Hp22_real, Hp22_real, MASKREAD_OFF, inLength/2);
  t1 = vsadd(t1, temp, MASKREAD_OFF, inLength/2);
  temp = vmul(Hp22_imag, Hp22_imag, MASKREAD_OFF, inLength/2);
  t1 = vsadd(t1, temp, MASKREAD_OFF, inLength/2);
/*----------------------------t2----------------------------------*/
  __v4096i8 t2;
  vclaim(t2);
  t2 = vmul(Hp12_real, Hp12_real, MASKREAD_OFF, inLength/2);
  temp = vmul(Hp12_imag, Hp12_imag, MASKREAD_OFF, inLength/2);
  t2 = vsadd(t2, temp, MASKREAD_OFF, inLength/2);
  temp = vmul(Hp21_real, Hp21_real, MASKREAD_OFF, inLength/2);
  t2 = vsadd(t2, temp, MASKREAD_OFF, inLength/2);
  temp = vmul(Hp21_imag, Hp21_imag, MASKREAD_OFF, inLength/2);
  t2 = vsadd(t2, temp, MASKREAD_OFF, inLength/2);
  vsetshamt(0);

  vsetshamt(6);
  y1_real = vdiv(t1 ,y1_real, MASKREAD_OFF, inLength/2);
  y1_imag = vdiv(t1 ,y1_imag, MASKREAD_OFF, inLength/2);
  y2_real = vdiv(t2 ,y2_real, MASKREAD_OFF, inLength/2);
  y2_imag = vdiv(t2 ,y2_imag, MASKREAD_OFF, inLength/2);
  vsetshamt(0);
  
/*----------------------------out and csi----------------------------------*/  
  vshuffle(out_real, index_240e, y1_real, SHUFFLE_SCATTER, inLength/2);
  vshuffle(out_real, index_240o, y2_real, SHUFFLE_SCATTER, inLength/2); 
  vshuffle(csi, index_240e, t1, SHUFFLE_SCATTER, inLength/2);
  vshuffle(csi, index_240o, t2, SHUFFLE_SCATTER, inLength/2); 

  vshuffle(out_imag, index_240e, y1_imag, SHUFFLE_SCATTER, inLength/2);
  vshuffle(out_imag, index_240o, y2_imag, SHUFFLE_SCATTER, inLength/2); 
  }
  else if(Nt == 4){
    /*----------------------------hestnew----------------------------------*/
    __v4096i8 hestnew_real_1;
    __v4096i8 hestnew_imag_1;
    __v4096i8 hestnew_real_2;
    __v4096i8 hestnew_imag_2;
    __v4096i8 temp;
    __v4096i8 tempback;
    __v2048i16 indexback;
    __v2048i16 index2367;
    vclaim(hestnew_real_1);
    vclaim(hestnew_imag_1);
    vclaim(hestnew_real_2);
    vclaim(hestnew_imag_2);
    vclaim(temp); 
    vclaim(tempback);
    vclaim(indexback); 
    vclaim(index2367);

    index2367 = vsadd(index0145,2,MASKREAD_OFF,inLength/2);

    vbrdcst(hestnew_real_1,0,MASKREAD_OFF,inLength);
    vshuffle(hestnew_real_1, index0145, hest_real0, SHUFFLE_GATHER, inLength/2);
    //vshuffle(temp, index2367, hest_real1, SHUFFLE_GATHER, 120);
    vshuffle(temp, index2367, hest_real1, SHUFFLE_GATHER, inLength/2);
    vrange(indexback,inLength/2);
    indexback = vsadd(indexback,inLength/2,MASKREAD_OFF,inLength/2);
    vbrdcst(tempback,0,MASKREAD_OFF,inLength);
    vshuffle(tempback,indexback,temp,SHUFFLE_SCATTER,inLength/2);
    hestnew_real_1 = vsadd(tempback,hestnew_real_1,MASKREAD_OFF,inLength);

    vbrdcst(hestnew_real_2,0,MASKREAD_OFF,inLength);
    vshuffle(hestnew_real_2, index0145, hest_real2, SHUFFLE_GATHER, inLength/2);
    vshuffle(temp, index2367, hest_real3, SHUFFLE_GATHER, inLength/2);
    vbrdcst(tempback,0,MASKREAD_OFF,inLength);
    vshuffle(tempback,indexback,temp,SHUFFLE_SCATTER,inLength/2);
    hestnew_real_2 = vsadd(tempback,hestnew_real_2,MASKREAD_OFF,inLength);

 
    vbrdcst(hestnew_imag_1,0,MASKREAD_OFF,inLength);
    vshuffle(hestnew_imag_1, index0145, hest_imag0, SHUFFLE_GATHER, inLength/2);
    vshuffle(temp, index2367, hest_imag1, SHUFFLE_GATHER, inLength/2);
    vbrdcst(tempback,0,MASKREAD_OFF,inLength);
    vshuffle(tempback,indexback,temp,SHUFFLE_SCATTER,inLength/2);
    hestnew_imag_1 = vsadd(tempback,hestnew_imag_1,MASKREAD_OFF,inLength);

    vbrdcst(hestnew_imag_2,0,MASKREAD_OFF,inLength);
    vshuffle(hestnew_imag_2, index0145, hest_imag2, SHUFFLE_GATHER, inLength/2);
    vshuffle(temp, index2367, hest_imag3, SHUFFLE_GATHER, inLength/2);
    vbrdcst(tempback,0,MASKREAD_OFF,inLength);
    vshuffle(tempback,indexback,temp,SHUFFLE_SCATTER,inLength/2);
    hestnew_imag_2 = vsadd(tempback,hestnew_imag_2,MASKREAD_OFF,inLength);


  //   /*----------------------------innew----------------------------------*/
    __v4096i8 innew_real;
    __v4096i8 innew_imag;
    vclaim(innew_real);
    vclaim(innew_imag);

    vbrdcst(innew_real,0,MASKREAD_OFF,inLength);
    vshuffle(innew_real, index0145, in_real, SHUFFLE_GATHER, inLength/2);
    vshuffle(temp, index2367, in_real, SHUFFLE_GATHER, inLength/2);
    vbrdcst(tempback,0,MASKREAD_OFF,inLength);
    vshuffle(tempback,indexback,temp,SHUFFLE_SCATTER,inLength/2);
    innew_real = vsadd(tempback,innew_real,MASKREAD_OFF,inLength);

    vbrdcst(innew_imag,0,MASKREAD_OFF,inLength);
    vshuffle(innew_imag, index0145, in_imag, SHUFFLE_GATHER, inLength/2);
    vshuffle(temp, index2367, in_imag, SHUFFLE_GATHER, inLength/2);
    vbrdcst(tempback,0,MASKREAD_OFF,inLength);
    vshuffle(tempback,indexback,temp,SHUFFLE_SCATTER,inLength/2);
    innew_imag = vsadd(tempback,innew_imag,MASKREAD_OFF,inLength);


     /*-----------------------index----------------------------*/
     __v2048i16 index_240e;
     __v2048i16 index_240o;
     vclaim(index_240e);
     vclaim(index_240o);
     vrange(index_240e, inLength/2);
     index_240e = vmul(index_240e, 2, MASKREAD_OFF, inLength/2);
     index_240o = vsadd(index_240e, 1, MASKREAD_OFF, inLength/2);
 
   /*------------------------hest----------------------------*/
     __v4096i8 Hp11_real;
     __v4096i8 Hp22_real;
     __v4096i8 Hp12_real;
     __v4096i8 Hp21_real;
     vclaim(Hp11_real);
     vclaim(Hp22_real);
     vclaim(Hp12_real);
     vclaim(Hp21_real);
     vshuffle(Hp11_real, index_240e, hestnew_real_1, SHUFFLE_GATHER, inLength/2);
     vshuffle(Hp22_real, index_240o, hestnew_real_2, SHUFFLE_GATHER, inLength/2);
     vshuffle(Hp12_real, index_240e, hestnew_real_2, SHUFFLE_GATHER, inLength/2);
     vshuffle(Hp21_real, index_240o, hestnew_real_1, SHUFFLE_GATHER, inLength/2);
 
     __v4096i8 Hp11_imag;
     __v4096i8 Hp22_imag;
     __v4096i8 Hp12_imag;
     __v4096i8 Hp21_imag;
     vclaim(Hp11_imag);
     vclaim(Hp22_imag);
     vclaim(Hp12_imag);
     vclaim(Hp21_imag);
     vshuffle(Hp11_imag, index_240e, hestnew_imag_1, SHUFFLE_GATHER, inLength/2);
     vshuffle(Hp22_imag, index_240o, hestnew_imag_2, SHUFFLE_GATHER, inLength/2);
     vshuffle(Hp12_imag, index_240e, hestnew_imag_2, SHUFFLE_GATHER, inLength/2);
     vshuffle(Hp21_imag, index_240o, hestnew_imag_1, SHUFFLE_GATHER, inLength/2);
 
   /*----------------------------in----------------------------------*/
   __v4096i8 r1_real;
   __v4096i8 r2_real;
   __v4096i8 y1_real;
   __v4096i8 y2_real;
   vclaim(r1_real);
   vclaim(r2_real);
   vclaim(y1_real);
   vclaim(y2_real);
   __v4096i8 r1_imag;
   __v4096i8 r2_imag;
   __v4096i8 y1_imag;
   __v4096i8 y2_imag;
   vclaim(r1_imag);
   vclaim(r2_imag);
   vclaim(y1_imag);
   vclaim(y2_imag); 
 
   vshuffle(r1_real, index_240e, innew_real, SHUFFLE_GATHER, inLength/2);
   vshuffle(r2_real, index_240o, innew_real, SHUFFLE_GATHER, inLength/2);
   vshuffle(r1_imag, index_240e, innew_imag, SHUFFLE_GATHER, inLength/2);
   vshuffle(r2_imag, index_240o, innew_imag, SHUFFLE_GATHER, inLength/2);
 
 
   //输入1Q6
   //y1为2Q5
   vbrdcst(temp,0,MASKREAD_OFF,inLength);
 /*----------------------------y1----------------------------------*/
   vsetshamt(7);
   y1_real = vmul(r1_real, Hp11_real, MASKREAD_OFF, inLength/2);
   temp = vmul(r1_imag, Hp11_imag, MASKREAD_OFF, inLength/2);
   y1_real = vsadd(y1_real, temp, MASKREAD_OFF, inLength/2);
   temp = vmul(r2_real, Hp22_real, MASKREAD_OFF, inLength/2);
   y1_real = vsadd(y1_real, temp, MASKREAD_OFF, inLength/2);
   temp = vmul(r2_imag, Hp22_imag, MASKREAD_OFF, inLength/2);
   y1_real = vsadd(y1_real, temp, MASKREAD_OFF, inLength/2);
 
   y1_imag = vmul(r1_imag, Hp11_real, MASKREAD_OFF, inLength/2);
   temp = vmul(r1_real, Hp11_imag, MASKREAD_OFF, inLength/2);
   y1_imag = vssub(temp, y1_imag, MASKREAD_OFF, inLength/2);///注意顺序
   temp = vmul(r2_real, Hp22_imag, MASKREAD_OFF, inLength/2);
   y1_imag = vsadd(y1_imag, temp, MASKREAD_OFF, inLength/2);
   temp = vmul(r2_imag, Hp22_real, MASKREAD_OFF, inLength/2);
   y1_imag = vssub(temp, y1_imag, MASKREAD_OFF, inLength/2);
 /*----------------------------y2----------------------------------*/
   y2_real = vmul(r1_real, Hp12_real, MASKREAD_OFF, inLength/2);
 
   vsetshamt(0);
   y2_real = vmul(y2_real, -1, MASKREAD_OFF, inLength/2);
   vsetshamt(7);
   temp = vmul(r1_imag, Hp12_imag, MASKREAD_OFF, inLength/2);
   y2_real = vssub(temp, y2_real, MASKREAD_OFF, inLength/2);///注意顺序
   temp = vmul(r2_real, Hp21_real, MASKREAD_OFF, inLength/2);
   y2_real = vsadd(y2_real, temp, MASKREAD_OFF, inLength/2);
   temp = vmul(r2_imag, Hp21_imag, MASKREAD_OFF, inLength/2);
   y2_real = vsadd(y2_real, temp, MASKREAD_OFF, inLength/2);
 
   y2_imag = vmul(r1_imag, Hp12_real, MASKREAD_OFF, inLength/2);
   temp = vmul(r1_real, Hp12_imag, MASKREAD_OFF, inLength/2);
   y2_imag = vssub(temp, y2_imag, MASKREAD_OFF, inLength/2);///注意顺序
   temp = vmul(r2_real, Hp21_imag, MASKREAD_OFF, inLength/2);
   y2_imag = vssub(temp, y2_imag, MASKREAD_OFF, inLength/2);///注意顺序
   temp = vmul(r2_imag, Hp21_real, MASKREAD_OFF, inLength/2);
   y2_imag = vsadd(y2_imag, temp, MASKREAD_OFF, inLength/2);
 
   vsetshamt(0);
 
   // 输出2Q5
   vsetshamt(6);
   y1_real = vmul(y1_real, 90, MASKREAD_OFF, inLength/2);//根号2
   y1_imag = vmul(y1_imag, 90, MASKREAD_OFF, inLength/2);
   y2_real = vmul(y2_real, 90, MASKREAD_OFF, inLength/2);
   y2_imag = vmul(y2_imag, 90, MASKREAD_OFF, inLength/2);
   vsetshamt(0);

 /*----------------------------t1----------------------------------*/
   __v4096i8 t1;
   vclaim(t1);
   vsetshamt(7);
   t1 = vmul(Hp11_real, Hp11_real, MASKREAD_OFF, inLength/2);
   temp = vmul(Hp11_imag, Hp11_imag, MASKREAD_OFF, inLength/2);
   t1 = vsadd(t1, temp, MASKREAD_OFF, inLength/2);
   temp = vmul(Hp22_real, Hp22_real, MASKREAD_OFF, inLength/2);
   t1 = vsadd(t1, temp, MASKREAD_OFF, inLength/2);
   temp = vmul(Hp22_imag, Hp22_imag, MASKREAD_OFF, inLength/2);
   t1 = vsadd(t1, temp, MASKREAD_OFF, inLength/2);
 /*----------------------------t2----------------------------------*/
   __v4096i8 t2;
   vclaim(t2);
   t2 = vmul(Hp12_real, Hp12_real, MASKREAD_OFF, inLength/2);
   temp = vmul(Hp12_imag, Hp12_imag, MASKREAD_OFF, inLength/2);
   t2 = vsadd(t2, temp, MASKREAD_OFF, inLength/2);
   temp = vmul(Hp21_real, Hp21_real, MASKREAD_OFF, inLength/2);
   t2 = vsadd(t2, temp, MASKREAD_OFF, inLength/2);
   temp = vmul(Hp21_imag, Hp21_imag, MASKREAD_OFF, inLength/2);
   t2 = vsadd(t2, temp, MASKREAD_OFF, inLength/2);
   vsetshamt(0);
 
   vsetshamt(5);
   y1_real = vdiv(t1 ,y1_real, MASKREAD_OFF, inLength/2);
   y1_imag = vdiv(t1 ,y1_imag, MASKREAD_OFF, inLength/2);
   y2_real = vdiv(t2 ,y2_real, MASKREAD_OFF, inLength/2);
   y2_imag = vdiv(t2 ,y2_imag, MASKREAD_OFF, inLength/2);
   vsetshamt(0);


    /*----------------------------out and csi----------------------------------*/
    __v2048i16 index159;
    vclaim(index159);
    index159 = vsadd(index048,1,MASKREAD_OFF,inLength/4);
    __v2048i16 index26_10;
    vclaim(index26_10);
    index26_10 = vsadd(index048,2,MASKREAD_OFF,inLength/4);
    __v2048i16 index37_11;
    vclaim(index37_11);
    index37_11 = vsadd(index048,3,MASKREAD_OFF,inLength/4);
    __v2048i16 indexfront;
    vclaim(indexfront);
    vrange(indexfront,inLength/4);
    indexfront = vsadd(indexfront,inLength/4,MASKREAD_OFF,inLength/4);

    vshuffle(out_real,index048,y1_real,SHUFFLE_SCATTER,inLength/4);
    vshuffle(out_real,index159,y2_real,SHUFFLE_SCATTER,inLength/4);
    vshuffle(temp,indexfront,y1_real,SHUFFLE_GATHER,inLength/4);
    vshuffle(out_real,index26_10,temp,SHUFFLE_SCATTER,inLength/4);
    vshuffle(temp,indexfront,y2_real,SHUFFLE_GATHER,inLength/4);
    vshuffle(out_real,index37_11,temp,SHUFFLE_SCATTER,inLength/4);


    vshuffle(out_imag,index048,y1_imag,SHUFFLE_SCATTER,inLength/4);
    vshuffle(out_imag,index159,y2_imag,SHUFFLE_SCATTER,inLength/4);
    vshuffle(temp,indexfront,y1_imag,SHUFFLE_GATHER,inLength/4);
    vshuffle(out_imag,index26_10,temp,SHUFFLE_SCATTER,inLength/4);
    vshuffle(temp,indexfront,y2_imag,SHUFFLE_GATHER,inLength/4);
    vshuffle(out_imag,index37_11,temp,SHUFFLE_SCATTER,inLength/4);


    vshuffle(csi,index048,t1,SHUFFLE_SCATTER,inLength/4);
    vshuffle(csi,index159,t2,SHUFFLE_SCATTER,inLength/4);
    vshuffle(temp,indexfront,t1,SHUFFLE_GATHER,inLength/4);
    vshuffle(csi,index26_10,temp,SHUFFLE_SCATTER,inLength/4);
    vshuffle(temp,indexfront,t2,SHUFFLE_GATHER,inLength/4);
    vshuffle(csi,index37_11,temp,SHUFFLE_SCATTER,inLength/4);


  } else{
    printf("error",NULL);
  }
    
  out_real = vsadd(out_real, 0, MASKREAD_OFF, inLength);
  out_imag = vsadd(out_imag, 0, MASKREAD_OFF, inLength);
  csi = vsadd(csi, 0, MASKREAD_OFF, inLength);
  vreturn(out_real, inLength, out_imag, inLength, csi, inLength);
}