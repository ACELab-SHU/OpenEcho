/**
 ******************************************************************************
 * @file           : Task_ltePDCCHDeinterleave.c
 * @author         : XiaoxiaoChen
 * @date           : 2025/5/11
 * @brief          : LTE PDCCH Deinterleave 3GPP 36.212
 * @attention      :

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

//  0, 16, 8, 24, 4, 20, 12, 28, 2, 18, 10, 26, 6, 22, 14, 30,
//                  1, 17, 9, 25, 5, 21, 13, 29, 3, 19, 11, 27, 7, 23, 15, 31
 int Task_ltePDCCHDeinterleave(__v4096i8 rxData_real, __v4096i8 rxData_imag, __v4096i8 h_real0, __v4096i8 h_imag0, __v4096i8 h_real1, __v4096i8 h_imag1, __v4096i8 h_real2, __v4096i8 h_imag2, __v4096i8 h_real3, __v4096i8 h_imag3, short_struct input_symbolLength, short_struct nCellID)
 {  
     short M_symb = input_symbolLength.data; // rxData_real 的符号长度
     int N_ID_cell =nCellID.data; 
    
    int M_quad = M_symb / 4; //四元组个数
    int index_map[M_quad]; 

    int C = 32;  // 固定列数
    int R = (M_quad + C - 1) / C;  // 计算行数
    int D = R * C;


    // 按置换后的列顺序收集非NULL索引
    int out_idx = 0;
    for (int j = 0; j < C; j++) {
        int perm_col = Permute_Col_32[j];  // 当前置换列
        for (int i = 0; i < R; i++) {
            int flat_idx = i * C + perm_col;  // 矩阵中的线性索引
            if (flat_idx >= D - M_quad) {  // 非NULL位置
                index_map[out_idx++] = flat_idx - (D - M_quad);  // 计算有效索引
            }
        }
    }

    // 循环移位
    int OutIndex[M_quad];
    int OutIndex_temp[M_quad];
    for (int i = 0; i < M_quad; i++) {
        int shifted_pos = (i + N_ID_cell) % M_quad;
        OutIndex_temp[i] = index_map[shifted_pos];
        //printf("%d, ", &OutIndex[i]);
    }

    for (int i = 0; i < M_quad; i++) {
        int temp = OutIndex_temp[i];
        OutIndex[temp] = i;
    }
    
    // 索引数组搬移
    __v2048i16 outIdx;
    vclaim(outIdx);

    vbarrier();
    VSPM_OPEN();
    int outIdx_addr = vaddr(outIdx);
    for (int i = 0; i < M_quad; i++)
    {
        *(volatile unsigned short *)(outIdx_addr + (i << 1)) = OutIndex[i];
    }    
     VSPM_CLOSE();


     outIdx = vadd(outIdx, 0, MASKREAD_OFF, M_quad*4);

    //解交织输出
    __v2048i16 outIdx_0;
    __v2048i16 outIdx_1;
    __v2048i16 outIdx_2;
    __v2048i16 outIdx_3;
    __v2048i16 index048;
    vclaim(outIdx_0);
    vclaim(outIdx_1);
    vclaim(outIdx_2);
    vclaim(outIdx_3);
    vclaim(index048);
    
    outIdx_0 = vmul(outIdx,4,MASKREAD_OFF,M_quad);
    outIdx_1 = vsadd(outIdx_0,1,MASKREAD_OFF,M_quad);
    outIdx_2 = vsadd(outIdx_0,2,MASKREAD_OFF,M_quad);
    outIdx_3 = vsadd(outIdx_0,3,MASKREAD_OFF,M_quad);
    vrange(index048, M_quad);
    index048 = vmul(index048, 4, MASKREAD_OFF, M_quad);


    vshuffle(outIdx, index048, outIdx_0, SHUFFLE_SCATTER, M_quad);
    index048 = vsadd(index048, 1, MASKREAD_OFF, M_quad);
    vshuffle(outIdx, index048, outIdx_1, SHUFFLE_SCATTER, M_quad);
    index048 = vsadd(index048, 1, MASKREAD_OFF, M_quad);
    vshuffle(outIdx, index048, outIdx_2, SHUFFLE_SCATTER, M_quad);
    index048 = vsadd(index048, 1, MASKREAD_OFF, M_quad);
    vshuffle(outIdx, index048, outIdx_3, SHUFFLE_SCATTER, M_quad);

    __v4096i8 pdcchDe_real;
    __v4096i8 pdcchDe_imag;
    __v4096i8 h_real_De0;
    __v4096i8 h_imag_De0;
    __v4096i8 h_real_De1;
    __v4096i8 h_imag_De1;
    __v4096i8 h_real_De2;
    __v4096i8 h_imag_De2;
    __v4096i8 h_real_De3;
    __v4096i8 h_imag_De3;
    vclaim(pdcchDe_real);
    vclaim(pdcchDe_imag);
    vclaim(h_real_De0);
    vclaim(h_imag_De0);
    vclaim(h_real_De1);
    vclaim(h_imag_De1);
    vclaim(h_real_De2);
    vclaim(h_imag_De2);
    vclaim(h_real_De3);
    vclaim(h_imag_De3);

    vshuffle(pdcchDe_real, outIdx, rxData_real, SHUFFLE_GATHER, M_symb);
    vshuffle(pdcchDe_imag, outIdx, rxData_imag, SHUFFLE_GATHER, M_symb);
    vshuffle(h_real_De0, outIdx, h_real0, SHUFFLE_GATHER, M_symb);
    vshuffle(h_imag_De0, outIdx, h_imag0, SHUFFLE_GATHER, M_symb);
    vshuffle(h_real_De1, outIdx, h_real1, SHUFFLE_GATHER, M_symb);
    vshuffle(h_imag_De1, outIdx, h_imag1, SHUFFLE_GATHER, M_symb);
    vshuffle(h_real_De2, outIdx, h_real2, SHUFFLE_GATHER, M_symb);
    vshuffle(h_imag_De2, outIdx, h_imag2, SHUFFLE_GATHER, M_symb);
    vshuffle(h_real_De3, outIdx, h_real3, SHUFFLE_GATHER, M_symb);
    vshuffle(h_imag_De3, outIdx, h_imag3, SHUFFLE_GATHER, M_symb);

     outIdx = vsadd(outIdx, 0, MASKREAD_OFF, M_quad*4);

     vreturn(pdcchDe_real,M_symb,pdcchDe_imag,M_symb,h_real_De0,M_symb,h_imag_De0,M_symb,h_real_De1,M_symb,h_imag_De1,M_symb,h_real_De2,M_symb,h_imag_De2,M_symb,h_real_De3,M_symb,h_imag_De3,M_symb);
     asm("ebreak");
    }
 