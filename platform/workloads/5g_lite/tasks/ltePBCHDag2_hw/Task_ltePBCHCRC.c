/*
 * @Author: Yihao Shen shenyihao@shu.edu.cn
 * @Date: 2025-05-06 09:42:04
 * @LastEditors: Yihao Shen shenyihao@shu.edu.cn
 * @LastEditTime: 2025-06-18 15:43:07
 * @FilePath: /VEMU/AceEcho/tasks/ltePBCHDag2_hw/Task_ltePBCHCRC.c
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
/**
 ******************************************************************************
 * @file           : Task_ltePBCHCRC.c
 * @author         : XiaoxiaoChen
 * @date           : 2024/11/25
 * @brief          : LTE PBCH Rate Dematch 3GPP 36.212
 * @attention      :
 * @LastEditors    :
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
 typedef char __v4096i8 __attribute__((ext_vector_type(4096)));

int K = 40;// 假设的输入长度，调整大小以适应你的具体输入

int Task_ltePBCHCRC(__v4096i8 input, __v4096i8 crcGen, __v4096i8 crcmask, short_struct input_cellrefp4)
{
    input = vxor(input, crcmask, MASKREAD_OFF, 40);
    __v4096i8 output;
    vclaim(output);

    __v2048i16 index_K16;
    vclaim(index_K16);
    vrange(index_K16,K-16);

    vshuffle(output, index_K16, input, SHUFFLE_GATHER, K-16);

    __v2048i16 index_17;
    __v2048i16 index_i17;
    vclaim(index_17);
    vclaim(index_i17);
    vrange(index_17, 17);

    __v4096i8 temp;
    vclaim(temp);

    for(int i = 0; i < K-16; i++){
        vclaim(input);
        vbarrier();
        VSPM_OPEN();
        int input_addr = vaddr(input);
        int temp_input = *(volatile unsigned char *)(input_addr + i);
        VSPM_CLOSE();
        
        if(temp_input){
            index_i17 = vsadd(index_17, i, MASKREAD_OFF, 17);
            vshuffle(temp, index_i17, input, SHUFFLE_GATHER, 17);
            temp = vxor(temp, crcGen, MASKREAD_OFF, 17);
            vshuffle(input, index_i17, temp, SHUFFLE_SCATTER, 17);
        }
    }

    int sum = 0;
    for (int i = K-16; i < K; i++)
    {
        vbarrier();
        VSPM_OPEN();
        int input_addr1 = vaddr(input);
        sum += *(volatile unsigned char *)(input_addr1 + i);
        VSPM_CLOSE();
    }
    int cellref = input_cellrefp4.data;
    short_struct checkFlag;
    if(sum == 0){
        // printf("%d crccheck is succeed\n", &cellref);
        checkFlag.data = 1;
    }
    else{
        // printf("%d crccheck is fail\n", &cellref);
        checkFlag.data = 0;
    }

    // short a = 123;
    // printf("checkFlag: %d\n", &checkFlag.data);
    //VENUS_PRINTVEC_CHAR(output, K-16);
    output = vsadd(output, 0, MASKREAD_OFF, K-16);
    vreturn(output, K-16, &checkFlag, sizeof(checkFlag));
}