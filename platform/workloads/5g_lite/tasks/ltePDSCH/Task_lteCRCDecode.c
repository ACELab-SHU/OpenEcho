//  lte crcDecoder 
//  Created by wangqianli

#include "venus.h"
#include <stdint.h>
#include <string.h> 
#include "vmath.h"
#include "riscv_printf.h" 

// typedef short __v2048i16 __attribute__((ext_vector_type(2048)));        //index变量用__v2048i16
// typedef char  __v4096i8 __attribute__((ext_vector_type(4096)));         //数据用4096i8
typedef short __v2048i16 __attribute__((ext_vector_type(5000)));        //index变量用__v2048i16
typedef char  __v4096i8 __attribute__((ext_vector_type(5000)));         //数据用4096i8

typedef short __v6148i16 __attribute__((ext_vector_type(6148)));        //index变量用__v2048i16
typedef char  __v6148i8 __attribute__((ext_vector_type(6148)));         //数据用4096i8


/* [output,errFlag] = CrcDecoder(input,crcType)
输入：
    input：输入数据
    trblklen: 原始数据长度
    crcGen24A: cyclic generator crcGen24Anomials
    //crcType： 24A
输出：
    output/msg：输出数据
    errFlag：0（正确） or 1(错误)
*/

typedef struct {
    short data;
} __attribute__((aligned(64))) short_struct;


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


int Task_lteCRCDecode(__v6148i8 input, __v6148i8 crcGen24A, short_struct trblklen){
    short K = trblklen.data;    //原始数据长度
    int L = 24;                 //crc长度
    int pariLen = L+1;          //crcGen24A的长度
    int crcblklen = K +24;      // input的长度

    int tmp = 0;
    int errFlag = 1;

    __v6148i8  buf;
    __v6148i8  msg;     //crcDecode输出数据
    __v6148i8  intemp;
    __v6148i8 crcResult;
    __v6148i16 index;
    vclaim(buf);
    vclaim(msg);
    vclaim(intemp);
    vclaim(crcResult);
    vclaim(index);

    vrange(index, K);
    vbrdcst(msg, 0, MASKREAD_OFF, crcblklen);
    vshuffle(msg, index, input, SHUFFLE_GATHER, K);        // output = input(1 : end-24);

    /*  len = length(input);
        for i = 1 :len-24
            if input(i) == 1
                input(i : i+24) = xor(input(i : i+24),crcGen);
            end
        end
    */
    // Divide the complete received codeword, including its 24 CRC bits.
    intemp = vsadd(input, 0, MASKREAD_OFF, crcblklen);
    for (int i = 0; i < K; i++) {
        int m_addr = vaddr(intemp);
        vbarrier();
        VSPM_OPEN();
            unsigned int addr = m_addr + i;
            tmp               = *(volatile unsigned char *)(addr);
        VSPM_CLOSE();

        if (tmp == 1) {
        vrange(index, pariLen);
        index = vsadd(index, i, MASKREAD_OFF, pariLen);
        vshuffle(buf, index, intemp, SHUFFLE_GATHER, pariLen);         //把数据取出来
        buf = vxor(buf, crcGen24A, MASKREAD_OFF, pariLen);             //做异或
        vshuffle(intemp, index, buf, SHUFFLE_SCATTER, pariLen);        //再把数据放回去
        }
    }

    /*
        if sum(input(len-24+1 : len)) == 0
            errFlag=0;
        end  
    */
    vrange(index, L);
    index = vsadd(index, K, MASKREAD_OFF, L);
    vshuffle(buf, index, intemp, SHUFFLE_GATHER, L);
    buf = vredsum(buf,MASKREAD_OFF,L);  
    vbarrier();
    VSPM_OPEN();
        tmp = *(volatile unsigned short *) (vaddr(buf));
    VSPM_CLOSE();
    if(tmp == 0){
        errFlag = 0;        //正确
    }

    short_struct out_crc_result;
    out_crc_result.data = errFlag;
    printf("errFlag: %hd\n", &errFlag);



    // char words[26] = "lte CRC decode finished";
    // printf("----------------%s------------------\n",&words);
    msg = vsadd(msg, 0, MASKREAD_OFF, K);
    VENUS_PRINTVEC_CHAR(msg, K);

    // vreturn(msg,sizeof(msg),  &out_crc_result,sizeof(out_crc_result));

    vreturn(msg,K,  &out_crc_result,sizeof(short));


}

/* 输出 
out_crc_result = 0；
msg[176] = {0,1,0,0,0,0,0,0,0,1,1,0,0,1,1,0,0,1,1,0,0,1,1,1,0,0,1,1,0,0,1,1,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}
*/



