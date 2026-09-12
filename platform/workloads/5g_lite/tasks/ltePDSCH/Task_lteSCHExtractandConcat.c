//  lte 提取sch（slot）然后拼接
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

// typedef char __v8400i8 __attribute__((ext_vector_type(8400)));

/* 
输入：
    dataSlot0: slot0数据
    dataSlot1: slot1数据
    schindex: SCH index
    schLength: SCH长度
    nDLRB: 总RB数

输出：
    fulldata: slot0和slot1数据拼接后提出的sch数据

*/


typedef struct {
    short data;
} __attribute__((aligned(64))) short_struct;


int Task_lteSCHExtractandConcat(__v4096i8 datain0,__v4096i8 datain1,
__v2048i16 schindex0,__v2048i16 schindex1, short_struct schLength0,short_struct schLength1
){
    short length0 = schLength0.data;
    short length1 = schLength1.data;

    datain0 = vsadd(datain0,0,MASKREAD_OFF,8400);
    datain1 = vsadd(datain1,0,MASKREAD_OFF,8400);
    schindex0 = vsadd(schindex0,0,MASKREAD_OFF,length0);
    schindex1 = vsadd(schindex1,0,MASKREAD_OFF,length1);


    __v4096i8 pdschdata0;
    __v4096i8 pdschdata1;
    vclaim(pdschdata0); 
    vclaim(pdschdata1);
    short_struct FullLength;
    int full_length;
    full_length = length0 + length1;
    FullLength.data  = length0 + length1;

    vshuffle(pdschdata0,schindex0,datain0,SHUFFLE_GATHER,length0);
    vshuffle(pdschdata1,schindex1,datain1,SHUFFLE_GATHER,length1);

    __v2048i16 tempindex;
    vclaim(tempindex);
    vrange(tempindex,8400);
    tempindex = vsadd(tempindex,length0,MASKREAD_OFF,length1);
    vshuffle(pdschdata0,tempindex,pdschdata1,SHUFFLE_SCATTER,length1);

    printf("full_length = %hd\n",&full_length);
    pdschdata0 =  vsadd(pdschdata0,0,MASKREAD_OFF,length0 + length1);
    


    // char word[16] = "extract finished";
    // printf("----------- %s -----------\n",&word);

    // vreturn(pdschdata,sizeof(pdschdata)  );
    vreturn(pdschdata0,length0 + length1, &FullLength,sizeof(short));

}

