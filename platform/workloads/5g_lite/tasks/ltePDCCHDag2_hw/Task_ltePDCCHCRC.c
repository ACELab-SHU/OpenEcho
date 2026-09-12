/**
 ******************************************************************************
 * @file           : Task_ltePDCCHCRC.c
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

int Task_ltePDCCHCRC(__v4096i8 input, __v4096i8 crcGen, __v4096i8 crcmask, short_struct DCI_Length)
{   
    short K = DCI_Length.data;

    __v2048i16 index_16;
    vclaim(index_16);
    vrange(index_16, 16);
    __v4096i8 temp_crccheck;
    vclaim(temp_crccheck);

    index_16 = vsadd(index_16, K-16, MASKREAD_OFF, 16);
    vshuffle(temp_crccheck, index_16, input, SHUFFLE_GATHER, 16);
    temp_crccheck = vxor(temp_crccheck, crcmask, MASKREAD_OFF, 16);
    vshuffle(input, index_16, temp_crccheck, SHUFFLE_SCATTER, 16);
    // input = vxor(input, crcmask, MASKREAD_OFF, K);

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
    // for (int i = K-16; i < K; i++)
    // {
    //     vbarrier();
    //     VSPM_OPEN();
    //     int input_addr1 = vaddr(input);
    //     sum += *(volatile unsigned char *)(input_addr1 + i);
    //     VSPM_CLOSE();
    // }
    __v2048i16 index_flag;
    vclaim(index_flag);
    vrange(index_flag,16);
    index_flag = vsadd(index_flag,K-16,MASKREAD_OFF,16);

    __v4096i8 sum_flag;
    __v4096i8 input_flag;
    vclaim(sum_flag);
    vclaim(input_flag);
    vshuffle(input_flag,index_flag,input,SHUFFLE_GATHER,16);
    sum_flag = vredsum(input_flag,MASKREAD_OFF,16);//前四个

    vbarrier();
    VSPM_OPEN();
    int sum_flag_addr1 = vaddr(sum_flag);
    sum = *(volatile unsigned char *)(sum_flag_addr1);
    VSPM_CLOSE();

    if(sum == 0){
        printf("crccheck is succeed\n",NULL);
    }
    else{
        printf("crccheck is fail\n",NULL);
    }

    short_struct crc_result;
    crc_result.data = sum;

    //VENUS_PRINTVEC_CHAR(output, K-16);
    output = vsadd(output, 0, MASKREAD_OFF, K-16);
    vreturn(output, K-16, &crc_result, sizeof(short_struct));
}