/**
 ******************************************************************************
 * @file           : Task_ltePDCCHRateDematch.c
 * @author         : XiaoxiaoChen
 * @date           : 2024/11/11
 * @brief          : LTE PDCCH Rate Dematch 3GPP 36.212
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
 
 //int softBitsLength = 480; // softBits 的长度
 
 int Permute_Col_32[32] = {1, 17, 9, 25, 5, 21, 13, 29, 3, 19, 11, 27, 7, 23, 15, 31,
                        0, 16, 8, 24, 4, 20, 12, 28, 2, 18, 10, 26, 6, 22, 14, 30}; /// 置换模式
 
 int Task_ltePDCCHRateDematch(__v4096i8 softBits, __v2048i16 Permute_Col, short_struct input_bitLength, short_struct InfoLength)
 {  
    short softBitsLength = input_bitLength.data; // softBits 的长度
    short K = InfoLength.data;  // DCI 比特长度
    softBits = vsra(softBits, 3, MASKREAD_OFF, softBitsLength);

    //  CRC16 
     int payloadAftConv = K * 3;

    //  //int nTimes = 4;
    int nTimes;
    if(((softBitsLength * 10 / payloadAftConv) % 10) == 0){
        nTimes = softBitsLength / payloadAftConv;
    }
    else{
        nTimes = softBitsLength / payloadAftConv + 1;
    }
    // printf("%d\n", &nTimes);
     //int nTimes = (int)floor((double)softBitsLength / payloadAftConv);// 初始化 nTimes
     int K2 = payloadAftConv / 3;// 分割 totalBits 为 v0, v1, v2
     int C = 32;                // 解交织矩阵列数固定为32

     int R;
    if(((K2 * 10 / C) % 10) == 0){
        R = K2 / C;
    }
    else{
        R = K2 / C + 1;
    }
    // printf("%d\n", &R);
    //  int R = (int)ceil((double)K2 / C); // 计算行数
     int nPad = R * C - K2; // 计算填充数量
 
     //获取加入NULL位置的索引数组
     int inputComb_RC[R * C];
     int input[K2];
     // 初始化输入数组
     for (int i = 0; i < K2; i++)
     {
         input[i] = i; // MATLAB是从1开始，C语言从0开始
     }
     // 初始化inputComb数组
     int j = 0, k = 0;
     for (int i = 0; i < R * C; i++)
     {
         if (i % R == 0 || R == 1)
         {
             j++;
             if (Permute_Col_32[j - 1] < nPad)
             { // NULL位置处理
                 inputComb_RC[i] = K2;
             }
             else
             {
                 inputComb_RC[i] = input[k];
                 k++;
             }
         }
         else
         {
             inputComb_RC[i] = input[k];
             k++;
         }
     }
 
     //索引数组搬移
     __v2048i16 inputComb;
     vclaim(inputComb);
 
     vbarrier();
     VSPM_OPEN();
     int inputComb_addr = vaddr(inputComb);
     for (int i = 0; i < R*C; i++) {
         *(volatile unsigned short *)(inputComb_addr + (i << 1)) = inputComb_RC[i];
     }    
     VSPM_CLOSE();
 
     __v2048i16 P1;
     __v2048i16 index_C;
 
     vclaim(P1);
     vclaim(index_C);
 
     vrange(index_C,C);
 
     //列置换索引生成
     vshuffle(P1,Permute_Col,index_C,SHUFFLE_SCATTER,C);
     
     __v2048i16 N_intocol;
     __v2048i16 N_out;
     __v2048i16 N_intocol_temp;
     __v2048i16 N_R;
     __v2048i16 N_temp;    
     __v2048i16 index_ONE;
     __v2048i16 N_P1;
     __v2048i16 N_P1_temp;   
     
     vclaim(N_intocol);
     vclaim(N_out);
     vclaim(N_intocol_temp);
     vclaim(N_R);
     vclaim(N_temp);
     vclaim(index_ONE);
     vclaim(N_P1);
     vclaim(N_P1_temp);
 
     vbrdcst(N_temp,C,MASKREAD_OFF,C);
     vbrdcst(N_R,R,MASKREAD_OFF,C);
     vbrdcst(index_ONE,1,MASKREAD_OFF,C);
     N_intocol_temp = vmul(index_C,N_R,MASKREAD_OFF,C);
     vshuffle(N_P1,index_C,P1,SHUFFLE_GATHER,C);
     vshuffle(N_P1_temp,index_C,P1,SHUFFLE_GATHER,C);
 
     // 将二维交织索引数组列输入行输出
     for (int i = 0; i < R; i++)
     {   
         vshuffle(N_intocol,index_C,N_intocol_temp,SHUFFLE_SCATTER,C);
         index_C = vadd(index_C,N_temp,MASKREAD_OFF,C);
         N_intocol_temp = vadd(N_intocol_temp,index_ONE,MASKREAD_OFF,C);
         //生成列置换索引向量
         N_P1_temp = vadd(N_P1_temp,N_temp,MASKREAD_OFF,C);
         vshuffle(N_P1,index_C,N_P1_temp,SHUFFLE_SCATTER,C);
     }
     //按照P1重新排列
     vshuffle(N_out,N_P1,N_intocol,SHUFFLE_GATHER,R*C);
 
     // 根据N_out重新排列结果，并去掉填充部分
     __v2048i16 temp;
     __v2048i16 outIdx_d0;
     __v2048i16 outIdx;
     __v2048i16 temp_index;
     __v2048i16 temp_K2;
     __v2048i16 index_K2;
     __v2048i16 temp_nPad;
 
     vclaim(temp);
     vclaim(outIdx_d0);
     vclaim(outIdx);
     vclaim(temp_index);
     vclaim(temp_K2);
     vclaim(index_K2);
     vclaim(temp_nPad);
 
     vrange(index_K2,K2);
     vbrdcst(temp_K2,K2,MASKREAD_OFF,K2);
     vbrdcst(temp_nPad,nPad,MASKREAD_OFF,K2);
     temp_index = vadd(temp_nPad,index_K2,MASKREAD_OFF,K2);
     vshuffle(temp,N_out,inputComb,SHUFFLE_GATHER,R*C);
     vshuffle(outIdx_d0,temp_index,temp,SHUFFLE_GATHER,K2);
 
     for(int i = 0;i < 3;i++)
     {
         vshuffle(outIdx,index_K2,outIdx_d0,SHUFFLE_SCATTER,K2);
         index_K2 = vadd(temp_K2,index_K2,MASKREAD_OFF,K2);
         outIdx_d0 = vadd(temp_K2,outIdx_d0,MASKREAD_OFF,K2);
     }
 
     // 初始化 totalBits
     //int totalBits[payloadAftConv];
     __v4096i8 totalBits;//input data(after processing)
     __v2048i16 index_soft;
 
     vclaim(totalBits);
     vclaim(index_soft);
 
     vbrdcst(totalBits,0,MASKREAD_OFF);       
     vrange(index_soft,softBitsLength);
 
     //软比特填充
     __v4096i8 paddedSoftBits;
     __v4096i8 paddedSoftBits_temp;
     //__v4096i8 paddedSoftBits_temp2;
     __v2048i16 index_temp;
 
     vclaim(paddedSoftBits);
     vclaim(paddedSoftBits_temp);
     //vclaim(paddedSoftBits_temp2);
     vclaim(index_temp);    
 
     if (payloadAftConv < softBitsLength)
     {
         // 将 softBits 填充到长度为 nTimes * payloadAftConv
         vbrdcst(paddedSoftBits, 0, MASKREAD_OFF, nTimes * payloadAftConv); // 剩余部分补0，长度 = nTimes * payloadAftConv
         vshuffle(paddedSoftBits, index_soft, softBits, SHUFFLE_GATHER, softBitsLength); // softBitsLength

         // 进行 nTimes 次累加
         vbrdcst(paddedSoftBits_temp, 0, MASKREAD_OFF, payloadAftConv); // payloadAftConv
         vbrdcst(index_temp, payloadAftConv, MASKREAD_OFF, payloadAftConv); // payloadAftConv

         for (int i = 0; i < nTimes; i++)
         {
             vshuffle(paddedSoftBits_temp, index_soft, paddedSoftBits, SHUFFLE_GATHER, payloadAftConv); // payloadAftConv
             totalBits = vadd(totalBits, paddedSoftBits_temp, MASKREAD_OFF, payloadAftConv); // payloadAftConv
             index_soft = vadd(index_soft, index_temp, MASKREAD_OFF, payloadAftConv); // payloadAftConv
         }
     }
     else
     {
         // 如果 payloadAftConv 大于 softBitsLength，则在 totalBits 中补零
         vbrdcst(totalBits, 0, MASKREAD_OFF, payloadAftConv); // payloadAftConv
         vshuffle(totalBits, index_soft, softBits, SHUFFLE_GATHER, softBitsLength); // softBitsLength
     }
 
     //解交织输出
     __v4096i8 OutBits;
     __v4096i8 OutBits_d0;
     __v4096i8 OutBits_d1;
     __v4096i8 OutBits_d2;
     __v2048i16 OutBits_K2;
 
     vclaim(OutBits);
     vclaim(OutBits_d0);
     vclaim(OutBits_d1);
     vclaim(OutBits_d2);
     vclaim(OutBits_K2);
     //vclaim(totalBits);
 
     vrange(OutBits_K2,K2);
 
 
     //测试
     // __v4096i8 ZERO;
     // vclaim(ZERO);
     // vbrdcst(ZERO,0,MASKREAD_OFF);
     // totalBits = vadd(totalBits,ZERO,MASKREAD_OFF);
 
     vshuffle(OutBits,outIdx,totalBits,SHUFFLE_GATHER,payloadAftConv);
     
     vshuffle(OutBits_d0,OutBits_K2,OutBits,SHUFFLE_GATHER,K2);
     OutBits_K2 = vadd(temp_K2,OutBits_K2,MASKREAD_OFF,K2);
     vshuffle(OutBits_d1,OutBits_K2,OutBits,SHUFFLE_GATHER,K2);
     OutBits_K2 = vadd(temp_K2,OutBits_K2,MASKREAD_OFF);
     vshuffle(OutBits_d2,OutBits_K2,OutBits,SHUFFLE_GATHER,K2);

     outIdx = vsadd(outIdx, 0, MASKREAD_OFF, payloadAftConv);
     vreturn(OutBits_d0, K2, OutBits_d1, K2, OutBits_d2, K2);
 }
 