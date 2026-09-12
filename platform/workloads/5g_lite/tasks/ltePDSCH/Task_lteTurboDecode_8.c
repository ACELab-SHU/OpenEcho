// lte Turbo译码,采用max-log-MAP算法
/*
 * @input   soft_in 软输入
 * @input   alphain 交织序列
 * @output  hard_out 译码输出
 *
 * Created by wangqianli
*/
/* 第二次迭代
    拆分的Turbodecode 7：
    SISO2:计算分支、前向、后向度量
*/

#include "venus.h"
#include <stdint.h>
#include "vmath.h"
#include "riscv_printf.h"


typedef short __v2048i16 __attribute__((ext_vector_type(5000)));        //index变量用__v2048i16
typedef char  __v4096i8 __attribute__((ext_vector_type(5000)));         //数据用4096i8
// typedef short __v2048i16 __attribute__((ext_vector_type(2048)));        //index变量用__v2048i16
// typedef char  __v4096i8 __attribute__((ext_vector_type(4096)));         //数据用4096i8

typedef short __v6148i16 __attribute__((ext_vector_type(6148)));        //index变量用__v2048i16
typedef char  __v6148i8 __attribute__((ext_vector_type(6148)));         //数据用4096i8



//打印向量
#define VENUS_PRINTVEC_CHAR(name, len)                                                                                 \
do {                                                                                                                 \
short array_##name[len];                                                                                           \
int   vecaddr_##name = vaddr(name);                                                                                \
vbarrier();                                                                                                        \
VSPM_OPEN();                                                                                                       \
for (int _____ = 0; _____ < len; _____++) {                                                                        \
    array_##name[_____] = *(volatile unsigned char *)(vecaddr_##name + _____);                                       \
    printf("%hd\n", &array_##name[_____]);                                                                           \
}                                                                                                                  \
VSPM_CLOSE();                                                                                                      \
} while (0)







typedef struct {
    short data;
} __attribute__((aligned(64))) short_struct;


int Task_lteTurboDecode_8(__v6148i8 soft_out_B,__v6148i16 alphain_N,
    short_struct trblklength ){
    
    short reg  = 4;                         //寄存器个数
    short trblklen = trblklength.data;      //原始信息长度
    short N  = trblklen + 24;               //码块长度
    short B  = reg+N;   
    printf("B = %hd\n",&B);    


    soft_out_B = vsadd(soft_out_B,0,MASKREAD_OFF,B);
    alphain_N = vsadd(alphain_N,0,MASKREAD_OFF,N);


    /*****************************解码结束，输出******************************
        soft_out(alphain)=so(1:L_seq-m);
        soft_out(soft_out==0)=1;    %因为fix函数使得0.xx的数为0，需要将0抬到正数
        hard_out=(sign(soft_out)+1)/2;
    */
    __v6148i8 soft_out_N;   //最终软输出
    vclaim(soft_out_N);
    vshuffle(soft_out_N,alphain_N,soft_out_B,SHUFFLE_SCATTER,N);
    soft_out_N = vsadd(soft_out_N,0,MASKREAD_OFF,N);

    __v6148i8 zeros_N;
    vclaim(zeros_N);
    vbrdcst(zeros_N,0,MASKREAD_OFF,N);
    __v6148i8 ones_N;
    vclaim(ones_N);
    vbrdcst(ones_N,1,MASKREAD_OFF,N);
    //soft_out_N>0的判定为1
    vsgt(zeros_N,soft_out_N,MASKREAD_OFF,MASKWRITE_ON,N);
    soft_out_N = vxor(soft_out_N,soft_out_N,MASKREAD_ON,N);
    soft_out_N = vsadd(soft_out_N,ones_N,MASKREAD_ON,N);
    //soft_out_N=0的判定为1
    vseq(zeros_N,soft_out_N,MASKREAD_OFF,MASKWRITE_ON,N);
    soft_out_N = vxor(soft_out_N,soft_out_N,MASKREAD_ON,N);
    soft_out_N = vsadd(soft_out_N,ones_N,MASKREAD_ON,N);
    //soft_out_N<0的判定为0
    __v6148i8 hard_out_N;
    vclaim(hard_out_N);
    vslt(zeros_N,soft_out_N,MASKREAD_OFF,MASKWRITE_ON,N);
    soft_out_N = vxor(soft_out_N,soft_out_N,MASKREAD_ON,N);
    hard_out_N = vsadd(soft_out_N,zeros_N,MASKREAD_ON,N);


    hard_out_N = vsadd(hard_out_N,0,MASKREAD_OFF,N);



    // char words[26] = "lte turbo decode finished";
    // printf("-----------------%s-------------------\n",&words);

    // vreturn(hard_out_N,sizeof(hard_out_N));
    vreturn(hard_out_N,N);
    

}  


/* 硬判决结果：
    0,1,0,0,0,0,0,0,0,1,1,0,0,1,1,0,0,1,1,0,0,1,1,1,0,0,1,1,0,0,1,1,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,0,0,1,1,1,0,0,0,0,1,0,0,1,1,0,0,1,1,1,0,0,0
*/