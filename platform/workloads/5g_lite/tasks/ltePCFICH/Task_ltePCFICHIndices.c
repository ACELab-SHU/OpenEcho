/**
  ******************************************************************************
  * @file           : ltePCFICHIndices.c
  * @author         : XiaoxiaoChen
  * @brief          : LTE PCFICH Indices Generation
  * @attention      : Indices start from 0 of a slot
  * @date           : 2024/1/3
  ******************************************************************************
  */

#include "data_type.h"
#include "riscv_printf.h"
#include "vmath.h"
#include "venus.h"
#include <stdint.h>

typedef char __v4096i8 __attribute__((ext_vector_type(4096)));
typedef short __v2048i16 __attribute__((ext_vector_type(2048)));

//17, 0, 50, 0, 1, 0,
int Task_ltePCFICHIndices(short_struct NCellID, short_struct NDLRB) {

    uint16_t NcellID = NCellID.data;//17
    uint16_t NDLrb = NDLRB.data;//50
    //short cellrefnum = CellRefNum.data;
    int NRBSC = 12;       //RB的子载波数
    //int l = 0;                //一时隙符号索引
    int v_shift = NcellID % 6;
    int K_prime = (NRBSC / 2) * (NcellID % (2 * NDLrb));
    int pcfichindices[16];
    int v = 0;
    int sccrs;

    // if(cellrefnum == 1){
    //     sccrs = (v + v_shift) % 6;
    //     for (int i = 0; i < 4; i++)
    //     {
    //         int temp = i * NDLrb / 2;
    //         pcfichindices[4 * i] = K_prime + temp * NRBSC / 2;
    //         int RB = pcfichindices[4 * i] / 12;
    //         int scRB = RB * 12;
    //         int n = 0;
    //         for(int q = pcfichindices[4 * i] % 12; q < pcfichindices[4 * i] % 12 + 5; q++){
    //             if(q != sccrs && q != sccrs + 6 && q != sccrs + 12){
    //                 pcfichindices[4 * i + n] = (scRB + q) % (NDLrb * 12);
    //                 n++;
    //             }
    //             if(n == 4) break;
    //         }
    //     }
    // }
    // else{
        sccrs = (v + v_shift) % 3;
        for (int i = 0; i < 4; i++)
        {
            int temp = i * NDLrb / 2;
            pcfichindices[4 * i] = K_prime + temp * NRBSC / 2;
            int RB = pcfichindices[4 * i] / 12;
            int scRB = RB * 12;
            int n = 0;
            for(int q = pcfichindices[4 * i] % 12; q < pcfichindices[4 * i] % 12 + 6; q++){
                if(q != sccrs && q != sccrs + 3 && q != sccrs + 6 && q != sccrs + 9 && q != sccrs + 12 && q != sccrs + 15){
                    pcfichindices[4 * i + n] = (scRB + q) % (NDLrb * 12);
                    n++;
                }
                if(n == 4) break;
            }
        }
    // }

    __v2048i16 out_pcfichindices;
    vclaim(out_pcfichindices);
    vbarrier();
    VSPM_OPEN();
    int out_pcfichindices_addr = vaddr(out_pcfichindices);
    for (int i = 0; i < 16; ++i)
    {
        *(volatile short *)(out_pcfichindices_addr + (i << 1)) = pcfichindices[i];
    }
    VSPM_CLOSE();

    vreturn(out_pcfichindices, sizeof(out_pcfichindices)); //8bit字节长度 sizeof 标量要加上取地址
}