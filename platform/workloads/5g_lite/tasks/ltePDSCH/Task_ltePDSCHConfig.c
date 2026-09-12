//  lte 获取PDSCH配置
//  Created by wangqianli

#include "venus.h"
#include <stdint.h>
#include <string.h> 
#include "vmath.h"
#include "riscv_printf.h" 

// typedef short __v2048i16 __attribute__((ext_vector_type(2048)));        //index变量用__v2048i16
// typedef char  __v4096i8 __attribute__((ext_vector_type(4096)));         //数据用4096i8
typedef short __v2048i16 __attribute__((ext_vector_type(4200)));
typedef char __v4096i8 __attribute__((ext_vector_type(8400)));

/* 
输入：  SIB1 PDCCH的输出


输出：  trblklen ：信息位长度
        RV：冗余版本
        TxScheme ： 传输方案  0-TxDiversity(else), 1-Port0 (if CellRefPort== antenna==1)
        Modulation : 调制方式 0-BPSK*(pdsch无), 1-QPSK, 2-16QAM, 3-64QAM, 4-256QAM, 5-1024QAM 

*/


typedef struct {
    short data;
} __attribute__((aligned(64))) short_struct;

typedef struct DCI_SIB1 {
   short            format01_Flag;        //Flag for format0A/1A
   short            VRB_assignment;       //0 Localized 1 Distributed
   short            Resource_assignment;  //Resource block assignment
   short            LCRBs;                //Length of used  PDSCH RBs
   short            RBstart;              //first RB of PDSCH RBs(0-based)
   short            MCS;                  //Modulation and coding scheme 0-28
   short            HARQ_number;          //FDD(3)TDD(4)
   short            Newdata_indicator; 
   short            Redundancy_version;
   short            TPC_command;          //TPC command for PUCCH
   short            DAI_flag;             //only present in TDD (config 1-6)
   short            DAI;                  //Downlink Assignment Index
 } __attribute__((aligned(64))) DCI_SIB1;



int Task_ltePDSCHConfig(short_struct antenna,DCI_SIB1 SIB1, __v2048i16 NPRB1A_itbs2, __v2048i16 NPRB1A_itbs3){

    
    // Keep scalar first-use order consistent with the Venus1 input ABI.
    short nAntenna = antenna.data;
    short nMCS = SIB1.MCS;
    short nRV = SIB1.Redundancy_version;
    short nLCRBs = SIB1.LCRBs;
    short nRBstart = SIB1.RBstart;
    short nFormat01_Flag = SIB1.format01_Flag;
    short nVRB_assignment = SIB1.VRB_assignment;
    short nResource_assignment = SIB1.Resource_assignment;
    short nHARQ_number = SIB1.HARQ_number;
    short nNewdata_indicator = SIB1.Newdata_indicator;
    short nTPC_command = SIB1.TPC_command;
    short nDAI_flag = SIB1.DAI_flag;
    short nDAI = SIB1.DAI;
    

    NPRB1A_itbs2 = vsadd(NPRB1A_itbs2,0,MASKREAD_OFF,29);           
    NPRB1A_itbs3 = vsadd(NPRB1A_itbs3,0,MASKREAD_OFF,29);  



    short_struct trblklen ;
    short_struct RV;
    short_struct TxScheme;
    short_struct Modulation;




    if (nAntenna ==1){
        TxScheme.data =1;   //Port0
    }else{
        TxScheme.data =0;   //TxDiversity
    }

    Modulation.data = 1;    // QPSK (for SIB1)
    RV.data = nRV;          //冗余版本
    

    //************************  for format1A: ************************
    // 参考：36.213 Table 7.1.7.2.1-1
    short tbsIndication = SIB1.TPC_command %2;
    // short NPRB1A = 0; 
    __v4096i8 temp;
    __v2048i16 index;
    vclaim(temp);
    vclaim(index);
    vrange(index,1);
    if (tbsIndication){
        // NPRB1A = 3;
        index = vadd(index,nMCS,MASKREAD_OFF,1); 
        vshuffle(temp,index,NPRB1A_itbs3,SHUFFLE_GATHER,1);       
    }else{
        // NPRB1A = 2;
        index = vadd(index,nMCS,MASKREAD_OFF,1); 
        vshuffle(temp,index,NPRB1A_itbs2,SHUFFLE_GATHER,1);       
    }    
    int ddr = vaddr(temp);
    vbarrier();
    VSPM_OPEN();
        unsigned int addr = ddr ;
        trblklen.data = *(volatile unsigned short *)(addr);
    VSPM_CLOSE();
    printf("TxScheme = %hd\n",&TxScheme.data);
    printf("RV = %hd\n",&RV.data);
    printf("MCS = %hd\n",&nMCS);
    printf("trblklen = %hd\n",&trblklen.data);


    // char word[15] = "PDSCHConfig finished";
    // printf("----------- %s -----------\n",&word);

    

    vreturn( &trblklen,sizeof(trblklen), &RV,sizeof(RV) , &TxScheme,sizeof(TxScheme), &Modulation, sizeof(Modulation) );



}

