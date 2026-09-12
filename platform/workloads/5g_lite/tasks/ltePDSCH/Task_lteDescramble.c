//  lte pdsch 解扰
//  Created by wangqianli

#include "venus.h"
#include <stdint.h>
#include <string.h> 
#include "vmath.h"
#include "riscv_printf.h" 

// typedef short __v2048i16 __attribute__((ext_vector_type(2048)));        //index变量用__v2048i16
// typedef char  __v4096i8 __attribute__((ext_vector_type(4096)));         //数据用4096i8
typedef short __v2048i16 __attribute__((ext_vector_type(4096)));
typedef char __v4096i8 __attribute__((ext_vector_type(8192)));

/* 
输入：
    length：一个子帧内sch的总个数*Qm  （Qm根据调制方式确定，QPSK-2,）
    demodpdschSymb : 解调后的码字
    scrambleSeq: 码字的解扰序列
    
输出：
    cws: 解扰后的码字
*/


typedef struct {
    short data;
} __attribute__((aligned(64))) short_struct;


int Task_lteDescramble(short_struct length, __v4096i8 demodpdschSymb,__v4096i8 scrambleSeq){
    short input_length = length.data;

    __v4096i8 cws;
    vclaim(cws);

    /*  % 解扰：将解调符号与扰码相乘
        cws = {demodpdschSymb .* scramblingSeq};   
    */
    cws = vmul(demodpdschSymb,scrambleSeq, MASKREAD_OFF, input_length);     //



    cws = vsadd(cws,0, MASKREAD_OFF, input_length);

//     char word[20] = "descramble finished";
//    printf("----------- %s -----------\n",&word);      

    // vreturn(cws,sizeof(cws));
    vreturn(cws,input_length);
}