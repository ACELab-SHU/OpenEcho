/**
 ******************************************************************************
 * @file           : Task_ltePBCHRateDematch.c
 * @author         : XiaoxiaoChen
 * @date           : 2024/11/11
 * @brief          : LTE PBCH Rate Dematch 3GPP 36.212
 * @attention      :
 * @LastEditors    : XiaoxiaoChen
 * @LastEditData   : 2024/11/13
 ******************************************************************************
 * Copyright (c) 2024 by ACE_Lab, All Rights Reserved.
 */

#include "riscv_printf.h"
#include "venus.h"
#include "stdint.h"
#include "data_type.h"
#include "vmath.h"

typedef short __v2048i16 __attribute__((ext_vector_type(2048)));
typedef char  __v4096i8 __attribute__((ext_vector_type(4096)));

// 参数设置
int softBitsLength = 480; // softBits 的长度  此处需要及时修改


int Task_ltePBCHRateDematch(__v4096i8 softBits, __v2048i16 outindex_De)
{
    softBits = vsra(softBits, 2, MASKREAD_OFF, 480);
    // CRC16   LTE PBCH payload length always 24
    int crcLen = 16;  // CRC 长度为 16
    int payLoad = 24; // 负载大小
    int payloadAftConv = (payLoad + crcLen) * 3;
    int K2 = payloadAftConv / 3;// 分割 totalBits 为 v0, v1, v2

    //int nTimes = (int)ceil((double)softBitsLength / payloadAftConv);// 数据处理次数nTimes
    //int nTimes = 4;
    int nTimes;// 数据处理次数nTimes
    if(((softBitsLength * 10 / payloadAftConv) % 10) == 0){
        nTimes = softBitsLength / payloadAftConv;
    }
    else{
        nTimes = softBitsLength / payloadAftConv + 1;
    }
    //printf("%d\n", &nTimes);

    __v4096i8 totalBits;//input data(after processing)
    __v2048i16 index_softbits;

    vclaim(totalBits);
    vclaim(index_softbits);

    vbrdcst(totalBits,0,MASKREAD_OFF);       
    vrange(index_softbits,softBitsLength);

    __v4096i8 paddedSoftBits;
    __v4096i8 paddedSoftBits_1;
    __v4096i8 paddedSoftBits_2;
    __v4096i8 paddedSoftBits_3;
    __v4096i8 paddedSoftBits_4;
    __v4096i8 paddedSoftBits_temp;    
    __v2048i16 index_1;
    __v2048i16 index_2;
    __v2048i16 index_3;
    __v2048i16 index_4;
    __v2048i16 index_temp;

    vclaim(paddedSoftBits);
    vclaim(paddedSoftBits_1);
    vclaim(paddedSoftBits_2);
    vclaim(paddedSoftBits_3);
    vclaim(paddedSoftBits_4);
    vclaim(paddedSoftBits_temp);
    vclaim(index_1);
    vclaim(index_2);
    vclaim(index_3);
    vclaim(index_4);    
    vclaim(index_temp);
    //vclaim(softBits);

    if (payloadAftConv < softBitsLength)
    {
        //将 softBits 填充到长度为 nTimes * payloadAftConv
        vshuffle(paddedSoftBits,index_softbits,softBits,SHUFFLE_GATHER,softBitsLength);

        //进行 nTimes 次累加
        vbrdcst(index_temp,payloadAftConv,MASKREAD_OFF);
        vrange(index_1,payloadAftConv);
        index_2 = vsadd(index_1, index_temp, MASKREAD_OFF);
        index_3 = vsadd(index_2, index_temp, MASKREAD_OFF);
        index_4 = vsadd(index_3, index_temp, MASKREAD_OFF);
        vshuffle(paddedSoftBits_1, index_1, paddedSoftBits, SHUFFLE_GATHER, payloadAftConv);
        vshuffle(paddedSoftBits_2, index_2, paddedSoftBits, SHUFFLE_GATHER, payloadAftConv);
        vshuffle(paddedSoftBits_3, index_3, paddedSoftBits, SHUFFLE_GATHER, payloadAftConv);
        vshuffle(paddedSoftBits_4, index_4, paddedSoftBits, SHUFFLE_GATHER, payloadAftConv);
        paddedSoftBits_temp = vsadd(paddedSoftBits_1, paddedSoftBits_2, MASKREAD_OFF);
        paddedSoftBits_temp = vsadd(paddedSoftBits_temp, paddedSoftBits_3, MASKREAD_OFF);
        totalBits = vsadd(paddedSoftBits_temp, paddedSoftBits_4, MASKREAD_OFF);
    }
    else
    {
         // 如果 payloadAftConv 大于 softBitsLength，则在 totalBits 中补零
         vshuffle(totalBits,index_softbits,softBits,SHUFFLE_GATHER,softBitsLength);
    }


    //outindex_De = vsadd(outindex_De,0,MASKREAD_OFF);
    totalBits = vsadd(totalBits,0,MASKREAD_OFF);
    
    //数据解交织、分流
    __v4096i8 data_de;
    __v4096i8 d0;
    __v4096i8 d1;
    __v4096i8 d2;
    __v2048i16 outIdx_d0;
    __v2048i16 outIdx_d1;
    __v2048i16 outIdx_d2;

    vclaim(data_de);
    vclaim(outIdx_d0);
    vclaim(outIdx_d1);
    vclaim(outIdx_d2);        
    vclaim(d0);
    vclaim(d1);
    vclaim(d2);
    vclaim(totalBits);

    vrange(outIdx_d0,K2);
    outIdx_d1 = vsadd(outIdx_d0,K2,MASKREAD_OFF);
    outIdx_d2 = vsadd(outIdx_d1,K2,MASKREAD_OFF);
    
    //vclaim(outindex_De);
    vshuffle(data_de,outindex_De,totalBits,SHUFFLE_GATHER,payloadAftConv);//解交织移位
    //data_de = vsadd(data_de,0,MASKREAD_OFF);
    vshuffle(d0,outIdx_d0,data_de,SHUFFLE_GATHER,K2);
    vshuffle(d1,outIdx_d1,data_de,SHUFFLE_GATHER,K2);
    vshuffle(d2,outIdx_d2,data_de,SHUFFLE_GATHER,K2); 

    // short a = 123;
    // printf("%hd",&a);
    vreturn(d0,K2,d1,K2,d2,K2);
}
