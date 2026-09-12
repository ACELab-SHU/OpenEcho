//  lte sch 拼接，并输出sch总长度
//  Created by wangqianli

#include "venus.h"
#include <stdint.h>
#include <string.h> 
#include "vmath.h"
#include "riscv_printf.h" 

// typedef short __v2048i16 __attribute__((ext_vector_type(2048)));        //index变量用__v2048i16
// typedef char  __v4096i8 __attribute__((ext_vector_type(4096)));         //数据用4096i8
typedef short __v2048i16 __attribute__((ext_vector_type(8400)));
typedef char __v4096i8 __attribute__((ext_vector_type(8400)));


typedef short __v6148i16 __attribute__((ext_vector_type(6148)));        //index变量用__v2048i16
typedef char  __v6148i8 __attribute__((ext_vector_type(6148)));         //数据用4096i8

/* 
输入：
    nCFI：
    nDLRB : 总RB数
    outlength: 每一段index的长度
    schx: 第x个符号的sch index

输出：
    pdsch_full: 单个子帧的PDSCH index （除去CFI，没有CFI的第一个符号开始）
    HalfLength: 单个子帧内PDSCH的总RE数
*/


typedef struct {
    short data;
} __attribute__((aligned(64))) short_struct;


int Task_SCHConcat_Slot0(short_struct nCFI, short_struct nDLRB,
short_struct outlength1,short_struct outlength2,short_struct outlength3,short_struct outlength4,short_struct outlength5,short_struct outlength6,
// short_struct outlength7,short_struct outlength8,short_struct outlength9,short_struct outlength10,short_struct outlength11,short_struct outlength12,short_struct outlength13,
 __v2048i16 sch1, __v2048i16 sch2, __v2048i16 sch3, __v2048i16 sch4, __v2048i16 sch5,__v2048i16 sch6
//  __v2048i16 sch7, __v2048i16 sch8, __v2048i16 sch9, __v2048i16 sch10,__v2048i16 sch11, __v2048i16 sch12, __v2048i16 sch13 
){

    short CFI = nCFI.data;
    short RB  = nDLRB.data;
    short length1 = outlength1.data;
    short length2 = outlength2.data;
    short length3 = outlength3.data;
    short length4 = outlength4.data;
    short length5 = outlength5.data;
    short length6 = outlength6.data;
    // short length7 = outlength7.data;
    // short length8 = outlength8.data;
    // short length9 = outlength9.data;
    // short length10 = outlength10.data;
    // short length11 = outlength11.data;
    // short length12 = outlength12.data;
    // short length13 = outlength13.data;
    short half_length = 0;
    short subNum = 0;       // 需要减去的偏置
    short flag = 0;



    __v2048i16 pdsch_full;     
    vclaim(pdsch_full);

    short_struct HalfLength;

    __v2048i16 temp_index1;
    __v4096i8 temp_index2;
    // __v2048i16 temp_index3;
    vclaim(temp_index1);
    vclaim(temp_index2);
    // vclaim(temp_index3);


    // 为index叠加偏置
    short bias = RB *12;
    sch1 = vsadd(sch1,bias,MASKREAD_OFF,length1);
    sch2 = vsadd(sch2,bias*2,MASKREAD_OFF,length2);
    sch3 = vsadd(sch3,bias*3,MASKREAD_OFF,length3);
    sch4 = vsadd(sch4,bias*4,MASKREAD_OFF,length4);
    sch5 = vsadd(sch5,bias*5,MASKREAD_OFF,length5);
    sch6 = vsadd(sch6,bias*6,MASKREAD_OFF,length6);
    // sch7 = vsadd(sch7,bias*7,MASKREAD_OFF,length7);
    // sch8 = vsadd(sch8,bias*8,MASKREAD_OFF,length8);
    // sch9 = vsadd(sch9,bias*9,MASKREAD_OFF,length9); 
    // sch10 = vsadd(sch10,bias*10,MASKREAD_OFF,length10);
    // sch11 = vsadd(sch11,bias*11,MASKREAD_OFF,length11);
    // sch12 = vsadd(sch12,bias*12,MASKREAD_OFF,length12);
    // sch13 = vsadd(sch13,bias*13,MASKREAD_OFF,length13);


    //根据CFI拼接index
    if (CFI == 1){      // 符号1~13
        pdsch_full = vsadd(sch1,0,MASKREAD_OFF,1200);
        vrange(temp_index1,1200);

        temp_index1 = vsadd(temp_index1,length1,MASKREAD_OFF,1200);
        vshuffle(pdsch_full,temp_index1,sch2,SHUFFLE_SCATTER,length2);

        temp_index1 = vsadd(temp_index1,length2,MASKREAD_OFF,1200);
        vshuffle(pdsch_full,temp_index1,sch3,SHUFFLE_SCATTER,length3);

        temp_index1 = vsadd(temp_index1,length3,MASKREAD_OFF,1200);
        vshuffle(pdsch_full,temp_index1,sch4,SHUFFLE_SCATTER,length4);

        temp_index1 = vsadd(temp_index1,length4,MASKREAD_OFF,1200);
        vshuffle(pdsch_full,temp_index1,sch5,SHUFFLE_SCATTER,length5);

        temp_index1 = vsadd(temp_index1,length5,MASKREAD_OFF,1200);
        vshuffle(pdsch_full,temp_index1,sch6,SHUFFLE_SCATTER,length6);

        // temp_index1 = vsadd(temp_index1,length6,MASKREAD_OFF,1200);
        // vshuffle(pdsch_full,temp_index1,sch7,SHUFFLE_SCATTER,length7);

        // temp_index1 = vsadd(temp_index1,length7,MASKREAD_OFF,1200);
        // vshuffle(pdsch_full,temp_index1,sch8,SHUFFLE_SCATTER,length8);

        // temp_index1 = vsadd(temp_index1,length8,MASKREAD_OFF,1200);
        // vshuffle(pdsch_full,temp_index1,sch9,SHUFFLE_SCATTER,length9);

        // temp_index1 = vsadd(temp_index1,length9,MASKREAD_OFF,1200);
        // vshuffle(pdsch_full,temp_index1,sch10,SHUFFLE_SCATTER,length10);

        // temp_index1 = vsadd(temp_index1,length10,MASKREAD_OFF,1200);
        // vshuffle(pdsch_full,temp_index1,sch11,SHUFFLE_SCATTER,length11);

        // temp_index1 = vsadd(temp_index1,length11,MASKREAD_OFF,1200);
        // vshuffle(pdsch_full,temp_index1,sch12,SHUFFLE_SCATTER,length12);

        // temp_index1 = vsadd(temp_index1,length12,MASKREAD_OFF,1200);
        // vshuffle(pdsch_full,temp_index1,sch13,SHUFFLE_SCATTER,length13);

        // subNum = 12*RB;
        // half_length = length1 + length2 + length3 + length4 + length5 + length6 + length7 + length8 + length9 + length10 + length11 + length12 + length13 ;
        half_length = length1 + length2 + length3 + length4 + length5 + length6 ;
        flag = 1;

    }
    else if(CFI == 2){     // 符号2~13 
        pdsch_full = vsadd(sch2,0,MASKREAD_OFF,1200);
        vrange(temp_index1,1200);

        temp_index1 = vsadd(temp_index1,length2,MASKREAD_OFF,1200);
        vshuffle(pdsch_full,temp_index1,sch3,SHUFFLE_SCATTER,length3);

        temp_index1 = vsadd(temp_index1,length3,MASKREAD_OFF,1200);
        vshuffle(pdsch_full,temp_index1,sch4,SHUFFLE_SCATTER,length4);

        temp_index1 = vsadd(temp_index1,length4,MASKREAD_OFF,1200);
        vshuffle(pdsch_full,temp_index1,sch5,SHUFFLE_SCATTER,length5);

        temp_index1 = vsadd(temp_index1,length5,MASKREAD_OFF,1200);
        vshuffle(pdsch_full,temp_index1,sch6,SHUFFLE_SCATTER,length6);

        // temp_index1 = vsadd(temp_index1,length6,MASKREAD_OFF,1200);
        // vshuffle(pdsch_full,temp_index1,sch7,SHUFFLE_SCATTER,length7);

        // temp_index1 = vsadd(temp_index1,length7,MASKREAD_OFF,1200);
        // vshuffle(pdsch_full,temp_index1,sch8,SHUFFLE_SCATTER,length8);

        // temp_index1 = vsadd(temp_index1,length8,MASKREAD_OFF,1200);
        // vshuffle(pdsch_full,temp_index1,sch9,SHUFFLE_SCATTER,length9);
        
        // temp_index1 = vsadd(temp_index1,length9,MASKREAD_OFF,1200);
        // vshuffle(pdsch_full,temp_index1,sch10,SHUFFLE_SCATTER,length10);

        // temp_index1 = vsadd(temp_index1,length10,MASKREAD_OFF,1200);
        // vshuffle(pdsch_full,temp_index1,sch11,SHUFFLE_SCATTER,length11);

        // temp_index1 = vsadd(temp_index1,length11,MASKREAD_OFF,1200);
        // vshuffle(pdsch_full,temp_index1,sch12,SHUFFLE_SCATTER,length12);

        // temp_index1 = vsadd(temp_index1,length12,MASKREAD_OFF,1200);
        // vshuffle(pdsch_full,temp_index1,sch13,SHUFFLE_SCATTER,length13);

        // subNum = 12*RB *2;
        // half_length = length2 + length3 + length4 + length5 + length6 + length7 + length8 + length9 + length10 + length11 + length12 + length13 ;
        half_length = length2 + length3 + length4 + length5 + length6 ;
        flag = 2;

    }
    else if(CFI == 3){     // 符号3~13
        pdsch_full = vsadd(sch3,0,MASKREAD_OFF,1200);
        vrange(temp_index1,1200);

        temp_index1 = vsadd(temp_index1,length3,MASKREAD_OFF,1200);
        vshuffle(pdsch_full,temp_index1,sch4,SHUFFLE_SCATTER,length4);

        temp_index1 = vsadd(temp_index1,length4,MASKREAD_OFF,1200);
        vshuffle(pdsch_full,temp_index1,sch5,SHUFFLE_SCATTER,length5);

        temp_index1 = vsadd(temp_index1,length5,MASKREAD_OFF,1200);
        vshuffle(pdsch_full,temp_index1,sch6,SHUFFLE_SCATTER,length6);

        // temp_index1 = vsadd(temp_index1,length6,MASKREAD_OFF,1200);
        // vshuffle(pdsch_full,temp_index1,sch7,SHUFFLE_SCATTER,length7);

        // temp_index1 = vsadd(temp_index1,length7,MASKREAD_OFF,1200);
        // vshuffle(pdsch_full,temp_index1,sch8,SHUFFLE_SCATTER,length8);

        // temp_index1 = vsadd(temp_index1,length8,MASKREAD_OFF,1200);
        // vshuffle(pdsch_full,temp_index1,sch9,SHUFFLE_SCATTER,length9);

        // temp_index1 = vsadd(temp_index1,length9,MASKREAD_OFF,1200);
        // vshuffle(pdsch_full,temp_index1,sch10,SHUFFLE_SCATTER,length10);

        // temp_index1 = vsadd(temp_index1,length10,MASKREAD_OFF,1200);
        // vshuffle(pdsch_full,temp_index1,sch11,SHUFFLE_SCATTER,length11);

        // temp_index1 = vsadd(temp_index1,length11,MASKREAD_OFF,1200);
        // vshuffle(pdsch_full,temp_index1,sch12,SHUFFLE_SCATTER,length12);

        // temp_index1 = vsadd(temp_index1,length12,MASKREAD_OFF,1200);
        // vshuffle(pdsch_full,temp_index1,sch13,SHUFFLE_SCATTER,length13);

        // subNum = 12*RB*3;
        // half_length = length3 + length4 + length5 + length6 + length7 + length8 + length9 + length10 + length11 + length12 + length13 ;
        half_length = length3 + length4 + length5 + length6 ;
        flag = 3;

    }



    
    // vbrdcst(temp_index2,subNum,MASKREAD_OFF,half_length);
    // pdsch_full = vsub(temp_index2,pdsch_full,MASKREAD_OFF,half_length);

    pdsch_full = vsadd(pdsch_full,0,MASKREAD_OFF,half_length);
    // printf("subNum = %hd\n",&subNum);
    printf("half_length = %hd\n",&half_length);
    printf("flag = %hd\n",&flag);
    HalfLength.data = half_length;

    // char word[15] = "concat finished";
    // printf("----------- %s -----------\n",&word);

    // vreturn(pdsch_full,sizeof(pdsch_full) ,&HalfLength,sizeof(HalfLength) );
    vreturn(pdsch_full,half_length*2 ,&HalfLength,sizeof(short) );


}