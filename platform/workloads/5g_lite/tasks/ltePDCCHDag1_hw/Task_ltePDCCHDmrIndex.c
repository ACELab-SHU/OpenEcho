    /**
    ******************************************************************************
    * @file           : Task_ltePDCCHDmrIndex.c
    * @author         : chenxiaoxiao
    * @brief          : LTE PDCCH Extract after DMR index
    * @attention      : Indices start from 0 of four symbols
    * @date           : 2025/4/23
    ******************************************************************************
    */

#include "venus.h"
#include <stdint.h>
#include <string.h> 
#include "riscv_printf.h" 
#include "data_type.h"
// typedef short __v2048i16 __attribute__((ext_vector_type(2048)));  
// typedef char  __v4096i8 __attribute__((ext_vector_type(4096))); 
typedef short __v4800i16 __attribute__((ext_vector_type(4800)));  


int Task_ltePDCCHDmrIndex(short_struct nCellID, short_struct nDLRB, short_struct CellRefNum, __v4800i16 DmrindexRef1,
    __v4800i16 DmrindexRef2_0, __v4800i16 DmrindexRef2_1, __v4800i16 DmrindexRef2_2)
{   
    short NID = nCellID.data;
    short nRB = nDLRB.data; 
    short ant = CellRefNum.data;
    // short CFI = nCFI.data; 

    short vshift = NID % 6;

    short num = 0;
    short PDCCH_num = 0;

    __v4800i16 PDCCH_Dmr_index;
    __v4800i16 PDCCH_index;
    __v4800i16 index_num;
    __v4800i16 index_nRB;
    __v4800i16 index_temp;
    __v4800i16 PDCCH_index_temp;
    vclaim(PDCCH_Dmr_index);
    vclaim(PDCCH_index);
    vclaim(index_num);
    vclaim(index_nRB);
    vclaim(index_temp);
    vclaim(PDCCH_index_temp);

    /*********************** one symbol******************************************/
    // k = 6m + (v + vshift) mod 6
    // CRS:6
    // if (ant == 1){
    //     num = (12 - 2) * nRB; // remained RE afer reduce refsignal
    //     if (vshift == 0){
    //         PDCCH_Dmr_index = vsadd(ltePDCCH_index_one_antenna_ID0, 0, MASKREAD_OFF, num);
    //     }
    //     else if (vshift == 1){
    //         PDCCH_Dmr_index = vsadd(ltePDCCH_index_one_antenna_ID1, 0, MASKREAD_OFF, num);
    //     }
    //     else if (vshift == 2){
    //         PDCCH_Dmr_index = vsadd(ltePDCCH_index_one_antenna_ID2, 0, MASKREAD_OFF, num);
    //     }
    //     else if (vshift == 3){
    //         PDCCH_Dmr_index = vsadd(ltePDCCH_index_one_antenna_ID3, 0, MASKREAD_OFF, num);
    //     }
    //     else if (vshift == 4){
    //         PDCCH_Dmr_index = vsadd(ltePDCCH_index_one_antenna_ID4, 0, MASKREAD_OFF, num);
    //     }
    //     else if (vshift == 5){
    //         PDCCH_Dmr_index = vsadd(ltePDCCH_index_one_antenna_ID5, 0, MASKREAD_OFF, num);
    //     }
    // }

    // CRS:3
    // else if (ant == 2 || ant == 4){
        num = (12 - 4) * nRB; // remained RE afer reduce refsignal
        if (vshift == 0){
            PDCCH_Dmr_index = vsadd(DmrindexRef2_0, 0, MASKREAD_OFF, num);
        }
        else if (vshift == 1){
            PDCCH_Dmr_index = vsadd(DmrindexRef2_1, 0, MASKREAD_OFF, num);
        }
        else if (vshift == 2){
            PDCCH_Dmr_index = vsadd(DmrindexRef2_2, 0, MASKREAD_OFF, num);
        }
        else if (vshift == 3){
            PDCCH_Dmr_index = vsadd(DmrindexRef2_0, 0, MASKREAD_OFF, num);
        }
        else if (vshift == 4){
            PDCCH_Dmr_index = vsadd(DmrindexRef2_1, 0, MASKREAD_OFF, num);
        }
        else if (vshift == 5){
            PDCCH_Dmr_index = vsadd(DmrindexRef2_2, 0, MASKREAD_OFF, num);
        }
    // }
    /***********************contact******************************************/
    vbrdcst(PDCCH_index, 0, MASKREAD_OFF, 12*4*nRB);
    vrange(index_num, num);
    vrange(index_nRB,nRB*12);
    vrange(index_temp, nRB*12 - num);
    vrange(PDCCH_index_temp, nRB*12 - num);
    if(ant == 1 || ant == 2){
        vshuffle(PDCCH_index, index_num, PDCCH_Dmr_index, SHUFFLE_SCATTER, num);
        index_nRB = vsadd(index_nRB, 12*nRB, MASKREAD_OFF, 12*nRB);
        DmrindexRef1 = vsadd(DmrindexRef1, 12*nRB, MASKREAD_OFF, 12*nRB);
        vshuffle(PDCCH_index, index_nRB, DmrindexRef1, SHUFFLE_SCATTER, 12*nRB);
        index_nRB = vsadd(index_nRB, 12*nRB, MASKREAD_OFF, 12*nRB);
        DmrindexRef1 = vsadd(DmrindexRef1, 12*nRB, MASKREAD_OFF, 12*nRB);
        vshuffle(PDCCH_index, index_nRB, DmrindexRef1, SHUFFLE_SCATTER, 12*nRB);
        index_nRB = vsadd(index_nRB, 12*nRB, MASKREAD_OFF, 12*nRB);
        DmrindexRef1 = vsadd(DmrindexRef1, 12*nRB, MASKREAD_OFF, 12*nRB);
        vshuffle(PDCCH_index, index_nRB, DmrindexRef1, SHUFFLE_SCATTER, num);
        PDCCH_num = 1*num + 3*12*nRB;
    }else if (ant == 4){
        vshuffle(PDCCH_index, index_num, PDCCH_Dmr_index, SHUFFLE_SCATTER, num);
        index_num = vsadd(index_num, 12*nRB, MASKREAD_OFF, num);
        PDCCH_Dmr_index = vsadd(PDCCH_Dmr_index, 12*nRB, MASKREAD_OFF, num);
        vshuffle(PDCCH_index, index_num, PDCCH_Dmr_index, SHUFFLE_SCATTER, num);
        index_nRB = vsadd(index_nRB, 2*12*nRB, MASKREAD_OFF, 12*nRB);
        DmrindexRef1 = vsadd(DmrindexRef1, 2*12*nRB, MASKREAD_OFF, 12*nRB);
        vshuffle(PDCCH_index, index_nRB, DmrindexRef1, SHUFFLE_SCATTER, 12*nRB);
        index_nRB = vsadd(index_nRB, 12*nRB, MASKREAD_OFF, 12*nRB);
        DmrindexRef1 = vsadd(DmrindexRef1, 2*12*nRB, MASKREAD_OFF, 12*nRB);
        vshuffle(PDCCH_index, index_nRB, DmrindexRef1, SHUFFLE_SCATTER, num);
        PDCCH_num = 2*num + 2*12*nRB;
    }

    short_struct output_PDCCH_num;
    output_PDCCH_num.data = PDCCH_num;

    vreturn(PDCCH_index, 12*4*nRB*2 ,&output_PDCCH_num, sizeof(short));
}