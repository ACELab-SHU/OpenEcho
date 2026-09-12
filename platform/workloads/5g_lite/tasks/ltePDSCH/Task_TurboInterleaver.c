//lteTurbo交织器,并分割解速率数据
//参考：3GPP TS 36.212  5.1.3.2.3 Turbo code internal interleaver
//Created by wangqianli

#include "venus.h"
#include <stdint.h>
#include <string.h> 
#include "vmath.h"
#include "riscv_printf.h" 

typedef short __v2048i16 __attribute__((ext_vector_type(5000)));        //index变量用__v2048i16
typedef char  __v4096i8 __attribute__((ext_vector_type(5000)));         //数据用4096i8
// typedef short __v2048i16 __attribute__((ext_vector_type(2048)));        //index变量用__v2048i16
// typedef char  __v4096i8 __attribute__((ext_vector_type(4096)));         //数据用4096i8

typedef short __v6148i16 __attribute__((ext_vector_type(6148)));        //index变量用__v2048i16
typedef char  __v6148i8 __attribute__((ext_vector_type(6148)));         //数据用4096i8




// #define VENUS_PRINTVEC_SHORT(name, len)                                                                                 \
//   do {                                                                                                                 \
//     short array_##name[len];                                                                                           \
//     int   vecaddr_##name = vaddr(name);                                                                                \
//     VSPM_OPEN();                                                                                                       \
//     vbarrier();                                                                                                        \
//     for (int _____ = 0; _____ < len; _____++) {                                                                        \
//       array_##name[_____] = *(volatile unsigned short *)(vecaddr_##name + _____);                                       \
//       printf("%hd\n", &array_##name[_____]);                                                                           \
//     }                                                                                                                  \
//     VSPM_CLOSE();                                                                                                      \
//   } while (0)
/*
输入：
    input:解速率输出数据
    trblklen：原始数据长度
输出：
    pi : 交织数组
    soft_in1:信息位1
    soft_in2:校验位1
    soft_in3:信息位2 （=信息位1（交织））
    soft_in4:校验位2
*/

typedef struct {
    short data;
} __attribute__((aligned(64))) short_struct;




// int Task_TurboInterleaver( __v4096i8 input, __v2048i16 K_values,  __v2048i16 f1_values,  __v2048i16 f2_values,
// short_struct trblklen){
int Task_TurboInterleaver(__v2048i16 K_values,  __v2048i16 f1_values,  __v2048i16 f2_values,__v2048i16 SW_values,
        short_struct trblklen){

    K_values = vsadd(K_values,0,MASKREAD_OFF,188);
    f1_values = vsadd(f1_values,0,MASKREAD_OFF,188);
    f2_values = vsadd(f2_values,0,MASKREAD_OFF,188);
    SW_values = vsadd(SW_values,0,MASKREAD_OFF,188);


    /********************************** 计算交织index **************************************/ 
    __v2048i16 temp1;
    __v2048i16 temp2;
    __v2048i16 zeros_188;
    __v2048i16 f1_temp;
    __v2048i16 f2_temp;
    __v6148i16 pi_vec1;

    // __v2048i16 pi_vec2;
    // __v2048i16 pi_vec3;
    vclaim(temp1);
    vclaim(temp2);
    vclaim(pi_vec1);
    // vclaim(pi_vec2);
    // vclaim(pi_vec3);
    vclaim(zeros_188);
    vclaim(f1_temp);
    vclaim(f2_temp);



    short K = trblklen.data;
    short interleave_blklen = K +24 ;               //交织长度
    short turboblklen = interleave_blklen +4 ;      //turbo码单个码流长度
    short idx = 0;
    short f1 = 0;
    short f2 = 0;
    short_struct SW_struct;
    

    
    // 取出K在K_values中的索引位置idx
    vbrdcst(temp1,interleave_blklen,MASKREAD_OFF,188);
    vbrdcst(zeros_188,0,MASKREAD_OFF,188);
    vseq(temp1,K_values,MASKREAD_OFF,MASKWRITE_ON,188);
    // temp2 = vseq(temp1,K_values,MASKREAD_OFF,MASKWRITE_OFF,188);
    // K_values = vsadd(K_values,0,MASKREAD_OFF,188);
    // temp2 = vsadd(temp2,0,MASKREAD_OFF,188);

    vrange(temp2,188);
    // zeros_188 = vadd(temp2,zeros_188,MASKREAD_ON,188);
    zeros_188 = vadd(zeros_188,temp2,MASKREAD_ON,188);
    zeros_188 = vmul(zeros_188,-1,MASKREAD_OFF,188);
    zeros_188 = vredmin16(zeros_188,MASKREAD_OFF,188);
    zeros_188 = vmul(zeros_188,-1,MASKREAD_OFF,188);
    vbarrier();
    VSPM_OPEN();
    idx = *(volatile unsigned short *) (vaddr(zeros_188));          //$$$$$$$$$$$

    //根据idx得到f1和f2的值
    f1 = *(volatile unsigned short *) (vaddr(f1_values)+(idx << 1));
    f2 = *(volatile unsigned short *) (vaddr(f2_values)+(idx << 1));
    SW_struct.data = *(volatile unsigned short *) (vaddr(SW_values)+(idx << 1));
    VSPM_CLOSE();
    printf("idx = %hd\n",&idx);
    printf("f1 = %hd\n",&f1);
    printf("f2 = %hd\n",&f2);
    printf("SW = %hd\n",&SW_struct.data);

    /*
        % 向量化计算交织模式
        i = 0:K-1;
        pi = mod(f1*i + f2*i.^2, K);
    */
//    vrange(temp2,interleave_blklen);
//    vbrdcst(temp1,interleave_blklen,MASKREAD_OFF,interleave_blklen);
//    f1_temp = vmul(temp2,f1,MASKREAD_OFF,interleave_blklen);             //f1*i
//    temp2 = vmul(temp2,temp2,MASKREAD_OFF,interleave_blklen);            //i.^2
//    f2_temp = vmul(temp2,f2,MASKREAD_OFF,interleave_blklen);             //f2*i.^2  
//    pi = vrem(temp1,vadd(f1_temp,f2_temp,MASKREAD_OFF,interleave_blklen),MASKREAD_OFF,interleave_blklen);    //pi = mod(f1*i + f2*i.^2, K);
    // interleave_blklen = 6144; f1 = 263; f2 =480;  //测试
    int pi[interleave_blklen];
    int tmp1=0;
    int tmp2=0;
    for(int i =0; i< interleave_blklen; i++){
        // tmp1 = (f1*i) % interleave_blklen;
        // tmp2 = (f2*i) % interleave_blklen;
        // tmp2 = (tmp2*i) % interleave_blklen;
        // pi[i] = (tmp1 + tmp2)% interleave_blklen;
        pi[i] = tmp2;
        tmp1 = tmp1 + (f2<<1);
        tmp2 = (tmp2 + tmp1 + f1-f2) % interleave_blklen;
    }
    // for(int i =0; i< 40; i++){
    //     printf("pi[%d] = ",&i);
    //     printf(" %d\n",&pi[i]);
    // }
    //搬入矢量
    int pi_vec_addr1 = vaddr(pi_vec1);
    // int pi_vec_addr2 = vaddr(pi_vec2);
    // int pi_vec_addr3 = vaddr(pi_vec3);
    int flag = 0;
    vbarrier();
    VSPM_OPEN();
    // for (size_t i = 0; i < interleave_blklen; i++) {
    //     if (i<2048){
    //         unsigned int addr                 = pi_vec_addr1 + (i<<1);
    //         *(volatile unsigned short *)(addr) = pi[i];
    //         flag = 1;
    //     }else if(i>=2048 && i<4096){
    //         unsigned int addr                 = pi_vec_addr2 + ((i-2048)<<1);
    //         *(volatile unsigned short *)(addr) = pi[i];
    //         flag = 2;
    //     }else{
    //         unsigned int addr                 = pi_vec_addr3 + ((i-4096)<<1);
    //         *(volatile unsigned short *)(addr) = pi[i];
    //         flag = 3;
    //     }
        
    // }
    for (size_t i = 0; i < interleave_blklen; i++) {
        unsigned int addr                 = pi_vec_addr1 + (i<<1);
        *(volatile unsigned short *)(addr) = pi[i];
     
        
    }
    VSPM_CLOSE();

    pi_vec1 = vsadd(pi_vec1,0,MASKREAD_OFF,interleave_blklen);


    // // int interleave_blklen = 1;
    printf("interleave_blklen = %hd\n",&interleave_blklen);

    // char words[30] = "lte turbo interleave finished";
    // printf("----------------%s------------------\n",&words);


    // vreturn(pi_vec1,sizeof(pi_vec1));
    // vreturn(pi_vec1,sizeof(pi_vec1),&SW_struct,sizeof(SW_struct));
    vreturn(pi_vec1,interleave_blklen*2,&SW_struct,sizeof(short));
    
}

