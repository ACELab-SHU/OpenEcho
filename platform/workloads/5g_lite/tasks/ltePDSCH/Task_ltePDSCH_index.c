//  lte 生成单个符号的sch index （对于子帧5）
//  Created by wangqianli

#include "venus.h"
#include <stdint.h>
#include <string.h> 
#include "vmath.h"
#include "riscv_printf.h" 

// typedef short __v2048i16 __attribute__((ext_vector_type(2048)));        //index变量用__v2048i16
// typedef char  __v4096i8 __attribute__((ext_vector_type(4096)));         //数据用4096i8
typedef short __v2048i16 __attribute__((ext_vector_type(8400)));
typedef char __v4096i8 __attribute__((ext_vector_type(8400)));

/* 
输入：
    nCellID：小区ID
    nDLRB: RB数
    nPRBSet: 哪些RB分配给SCH  
    antenna：天线数
    nCFI：
    NSymNum: 第几个符号（0~13）
    DuplexMode：TDD（0） or FDD（1）
    


输出：
    PDSCH_index: 单个符号的PDSCH index
*/


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

typedef struct {
    short data;
} __attribute__((aligned(64))) short_struct;


int Task_ltePDSCH_index(short_struct nCellID, short_struct nDLRB,
    short_struct NSymNum, short_struct antenna,short_struct nDuplexMode, DCI_SIB1 SIB1,
__v2048i16 ltePDSCH_index_one_antenna_ID0, __v2048i16 ltePDSCH_index_one_antenna_ID1, __v2048i16 ltePDSCH_index_one_antenna_ID2, __v2048i16 ltePDSCH_index_one_antenna_ID3, __v2048i16 ltePDSCH_index_one_antenna_ID4, __v2048i16 ltePDSCH_index_one_antenna_ID5,
__v2048i16 ltePDSCH_index_two_antenna_ID0, __v2048i16 ltePDSCH_index_two_antenna_ID1, __v2048i16 ltePDSCH_index_two_antenna_ID2
){

    short NID = nCellID.data;
    short nRB = nDLRB.data; 
    // short port = CellRefPort.data;      // 没用上，存表的时候已经考虑这个因素
    short sym = NSymNum.data;
    short ant = antenna.data;
    short DuplexMode = nDuplexMode.data;
    // short CFI = nCFI.data; 

    short vshift = NID % 6;
    short ns;

    // int subshift = RB * 12 * 4;
    // int symshift = RB * 12;
    short num = 0;
    short v = 0;
    short flag = 0;
    // short RB = 0;


    short_struct outlength;


    __v2048i16 PDSCHindex;      // 输出的pdschindex
    vclaim(PDSCHindex);

    
    __v2048i16 tempindex1;
    __v2048i16 tempindex2;
    __v2048i16 tempindex3;
    vclaim(tempindex1);
    vclaim(tempindex2);
    vclaim(tempindex3);

    // ltePDSCH_index_one_antenna_ID0 = vsadd(ltePDSCH_index_one_antenna_ID0,0,MASKREAD_OFF,1000);
    // ltePDSCH_index_one_antenna_ID1 = vsadd(ltePDSCH_index_one_antenna_ID1,0,MASKREAD_OFF,1000);
    // ltePDSCH_index_one_antenna_ID2 = vsadd(ltePDSCH_index_one_antenna_ID2,0,MASKREAD_OFF,1000);
    // ltePDSCH_index_one_antenna_ID3 = vsadd(ltePDSCH_index_one_antenna_ID3,0,MASKREAD_OFF,1000);
    // ltePDSCH_index_one_antenna_ID4 = vsadd(ltePDSCH_index_one_antenna_ID4,0,MASKREAD_OFF,1000);
    // ltePDSCH_index_one_antenna_ID5 = vsadd(ltePDSCH_index_one_antenna_ID5,0,MASKREAD_OFF,1000);

    // ltePDSCH_index_two_antenna_ID0 = vsadd(ltePDSCH_index_two_antenna_ID0,0,MASKREAD_OFF,1000);
    // ltePDSCH_index_two_antenna_ID1 = vsadd(ltePDSCH_index_two_antenna_ID1,0,MASKREAD_OFF,1000);
    // ltePDSCH_index_two_antenna_ID2 = vsadd(ltePDSCH_index_two_antenna_ID2,0,MASKREAD_OFF,1000);


    // // 获取SCH的位置
    short schPRB1= SIB1.RBstart;    //SCH的起始RB
    short schPRB2= (SIB1.RBstart + SIB1.LCRBs - 1);    //SCH的结束RB
    // short tmp[100];

    // int m_addr = vaddr(nPRBSet);
    // for (int i =0; i<100; i++){
    //     vbarrier();
    //     VSPM_OPEN();
    //         unsigned int addr = m_addr + i;
    //         tmp[i]            = *(volatile unsigned short *)(addr);
    //     VSPM_CLOSE();

    //     if (i ==0){
    //         schPRB1 = tmp[i];
    //     }else{
    //         if(tmp[i] < tmp[i-1]){
    //             schPRB2 = tmp[i-1];
    //             i=100;
    //         }
    //     }
    // }
    printf("schPRB1 = %hd\n",&schPRB1);
    printf("schPRB2 = %hd\n",&schPRB2);
    

    
    // RB = schPRB2-schPRB1 +1;    //一定大于等于6
    short RB = SIB1.LCRBs;

    //判定sch所占用RB位置 是否与最中心的72个子载波有重合
    short RBflag = 0;
    short PSS_SSS_start_RB = 0;
    short PSS_SSS_end_RB = 0;
    short bias = 0;
    if (nRB == 6){
        RBflag = 1;        // 一定完全重合
    }
    
    else if(nRB == 15){    // RB4和RB10只有一半是同步信号
        PSS_SSS_start_RB = 4;
        PSS_SSS_end_RB = 10;
        if(schPRB1 == PSS_SSS_start_RB && schPRB2 == PSS_SSS_end_RB){   //完全重合
            RBflag = 13;
        }else if(schPRB1 == PSS_SSS_start_RB && schPRB2 == PSS_SSS_start_RB+5){        //sch在范围内
            RBflag = 5;
        }else if(schPRB1 == PSS_SSS_end_RB-5 && schPRB2 == PSS_SSS_end_RB){       //sch在范围内
            RBflag = 6;
        }else if(schPRB1 == PSS_SSS_start_RB && schPRB2 > PSS_SSS_end_RB){         // 例：
            RBflag = 7;
        }else if(schPRB2 == PSS_SSS_end_RB && schPRB1 < PSS_SSS_start_RB){         // 例：
            RBflag = 8;
        }else if(schPRB2 == PSS_SSS_start_RB ){                        // 例：
            RBflag = 9;
        }else if(schPRB1 == PSS_SSS_end_RB){                        // 例：
            RBflag = 10;
        }else if(schPRB1 > PSS_SSS_start_RB && schPRB1 < PSS_SSS_end_RB && schPRB2 > PSS_SSS_end_RB){      // 例：
            RBflag = 11;
        }else if(schPRB1 < PSS_SSS_start_RB && schPRB2 > PSS_SSS_start_RB  && schPRB2 < PSS_SSS_end_RB){      // 例：
            RBflag = 12;
        }else if(schPRB1 < PSS_SSS_start_RB && schPRB2 > PSS_SSS_end_RB) {       // 例：
            RBflag = 14;
        }else{              // 完全没有重合
            RBflag = 4;
        }  
    }
    
    else if(nRB == 25){    // RB9和RB15只有一半是同步信号
        PSS_SSS_start_RB = 9;
        PSS_SSS_end_RB = 15;
        if(schPRB1 == PSS_SSS_start_RB && schPRB2 == PSS_SSS_end_RB){   //完全重合:9~15
            RBflag = 13;
        }else if(schPRB1 == PSS_SSS_start_RB && schPRB2 == PSS_SSS_start_RB+5){        //sch在范围内:9~14
            RBflag = 5;
        }else if(schPRB1 == PSS_SSS_end_RB-5 && schPRB2 == PSS_SSS_end_RB){       //sch在范围内:10~15
            RBflag = 6;
        }else if(schPRB1 == PSS_SSS_start_RB && schPRB2 > PSS_SSS_end_RB){         // 例：9~17 
            RBflag = 7;
        }else if(schPRB2 == PSS_SSS_end_RB && schPRB1 < PSS_SSS_start_RB){         // 例：7~15
            RBflag = 8;
        }else if(schPRB2 == PSS_SSS_start_RB ){                        // 例：3~9
            RBflag = 9;
        }else if(schPRB1 == PSS_SSS_end_RB){                        // 例：15~21 
            RBflag = 10;
        }else if(schPRB1 > PSS_SSS_start_RB && schPRB1 < PSS_SSS_end_RB && schPRB2 > PSS_SSS_end_RB){      // 例：13~18 
            RBflag = 11;
        }else if(schPRB1 < PSS_SSS_start_RB && schPRB2 > PSS_SSS_start_RB  && schPRB2 < PSS_SSS_end_RB){      // 例：3~13
            RBflag = 12;
        }else if(schPRB1 < PSS_SSS_start_RB && schPRB2 > PSS_SSS_end_RB) {       // 例：7~17
            RBflag = 14;
        }else{              // 完全没有重合
            RBflag = 4;
        }  
    }
    
    else if(nRB == 50){
        PSS_SSS_start_RB = 22;
        PSS_SSS_end_RB = 27;
        if(schPRB1 == PSS_SSS_start_RB && schPRB2 == PSS_SSS_end_RB){   //完全重合
            RBflag = 1;
        }else if(schPRB1 >= PSS_SSS_start_RB && schPRB1 <= PSS_SSS_end_RB){
            RBflag = 2;
        }else if(schPRB2 >= PSS_SSS_start_RB && schPRB2 <= PSS_SSS_end_RB){
            RBflag = 3;
        }else{              // 完全没有重合
            RBflag = 4;
        }
    }
    
    else if(nRB == 75){    // RB34和RB40只有一半是同步信号
        PSS_SSS_start_RB = 34;
        PSS_SSS_end_RB = 40;
        if(schPRB1 == PSS_SSS_start_RB && schPRB2 == PSS_SSS_end_RB){   //完全重合
            RBflag = 13;
        }else if(schPRB1 == PSS_SSS_start_RB && schPRB2 == PSS_SSS_start_RB+5){        //sch在范围内
            RBflag = 5;
        }else if(schPRB1 == PSS_SSS_end_RB-5 && schPRB2 == PSS_SSS_end_RB){       //sch在范围内
            RBflag = 6;
        }else if(schPRB1 == PSS_SSS_start_RB && schPRB2 > PSS_SSS_end_RB){         // 例：
            RBflag = 7;
        }else if(schPRB2 == PSS_SSS_end_RB && schPRB1 < PSS_SSS_start_RB){         // 例：
            RBflag = 8;
        }else if(schPRB2 == PSS_SSS_start_RB ){                        // 例：
            RBflag = 9;
        }else if(schPRB1 == PSS_SSS_end_RB){                        // 例：
            RBflag = 10;
        }else if(schPRB1 > PSS_SSS_start_RB && schPRB1 < PSS_SSS_end_RB && schPRB2 > PSS_SSS_end_RB){      // 例：
            RBflag = 11;
        }else if(schPRB1 < PSS_SSS_start_RB && schPRB2 > PSS_SSS_start_RB  && schPRB2 < PSS_SSS_end_RB){      // 例：
            RBflag = 12;
        }else if(schPRB1 < PSS_SSS_start_RB && schPRB2 > PSS_SSS_end_RB) {       // 例：
            RBflag = 14;
        }else{              // 完全没有重合
            RBflag = 4;
        }  
    }
    
    else if(nRB == 100){
        PSS_SSS_start_RB = 47;
        PSS_SSS_end_RB = 52;
        if(schPRB1 == PSS_SSS_start_RB && schPRB2 == PSS_SSS_end_RB){   //完全重合
            RBflag = 1;
        }else if(schPRB1 >= PSS_SSS_start_RB && schPRB1 <= PSS_SSS_end_RB){     // 例：48~54
            RBflag = 2;
        }else if(schPRB2 >= PSS_SSS_start_RB && schPRB2 <= PSS_SSS_end_RB){     // 例：44~50
            RBflag = 3;
        }else{              // 完全没有重合
            RBflag = 4;
        }
    }
    printf("RBflag = %hd\n",&RBflag);



    /*********************** 判定sym（无参考信号的） 去掉同步信号 ******************/
    // SCH占用的RB不一定会包含同步信号
    // FDD(1):对于子帧5，符号5存在SSS，符号6存在PSS
    // TDD(0)：对于子帧5，符号13存在SSS（72个子载波）
    /*  RB总数：    6个RB       15      25      50      75      100
        PSS、SSS占用的RB位置（0-based）：
                    0-5       4-10*   9-15*   22-27   34-40*   47-52
    */
    if( (DuplexMode == 0 && sym == 13) || (DuplexMode == 1 && (sym == 5 || sym == 6)) ){  
        /**************** sch所占用RB 不在同步信号的RB位置 ****************/   
        if(RBflag == 4){
            num = 12 * RB;
            vrange(PDSCHindex, num);    
            flag = 1;
            bias = schPRB1 * 12;
        }
        /***************** sch所占用RB 与同步信号的RB位置 完全重合 ****************/
        else if(RBflag == 1){
            num = 0;
            vrange(PDSCHindex,num);
            flag = 2;
            bias = 0;

            // num = RB*6 -36;                 // RB*12/2 -36
            // vrange(PDSCHindex,num);         //第一段         
            // tempindex1 = vsadd(PDSCHindex,num+72,MASKREAD_OFF,num);   //第二段
            // tempindex2 = vsadd(PDSCHindex,num,MASKREAD_OFF,num);      //拼接需要的index
            // vshuffle(PDSCHindex,tempindex2,tempindex1,SHUFFLE_SCATTER,num);

            // flag = 1;
            // num = num*2;
            // bias = schPRB1*12;
        }
        /***************** sch所占用RB 与同步信号的RB位置 重合 ****************/
        else if(RBflag == 2){
            num = (schPRB2 - PSS_SSS_end_RB)*12;
            vrange(PDSCHindex,num);  

            flag = 3;
            bias = (PSS_SSS_end_RB+1) * 12 ;
        }
        else if(RBflag == 3){
            num = (PSS_SSS_start_RB - schPRB1)*12;
            vrange(PDSCHindex,num);  

            flag = 4;
            bias = schPRB1 * 12;
        }
        /****************** 几种特殊情况 *********** */
        else if(RBflag == 5){
            num = 6;
            vrange(PDSCHindex,num);

            flag = 5;
            bias = PSS_SSS_start_RB * 12;
        }
        else if(RBflag == 6){
            num = 6;
            vrange(PDSCHindex,num);

            flag = 6;
            bias = PSS_SSS_end_RB * 12 + 6;
        }
        //** */
        else if(RBflag == 7){
            num = 6*2 + (schPRB2 - PSS_SSS_end_RB)*12;
            vrange(PDSCHindex,6);                                      //第一小段
            tempindex1 = vsadd(PDSCHindex,6+72,MASKREAD_OFF,6);       //第二小段
            tempindex2 = vsadd(PDSCHindex,6,MASKREAD_OFF,6);          //拼接需要的index
            vshuffle(PDSCHindex,tempindex2,tempindex1,SHUFFLE_SCATTER,6);

            vrange(tempindex1,(schPRB2 - PSS_SSS_end_RB)*12);           //第三段
            tempindex2 = vsadd(tempindex1,12,MASKREAD_OFF,(schPRB2 - PSS_SSS_end_RB)*12);          //拼接需要的index
            tempindex1 = vsadd(tempindex1,7*12,MASKREAD_OFF,(schPRB2 - PSS_SSS_end_RB)*12); 
            vshuffle(PDSCHindex,tempindex2,tempindex1,SHUFFLE_SCATTER,(schPRB2 - PSS_SSS_end_RB)*12);
        
            flag = 7;
            bias = PSS_SSS_start_RB *12;
        }
        else if(RBflag == 8){
            num = 6*2 + (PSS_SSS_start_RB - schPRB1)*12;
            vrange(PDSCHindex,(PSS_SSS_start_RB - schPRB1)*12+6);      //第一小段
            tempindex2 = vsadd(PDSCHindex,(PSS_SSS_start_RB - schPRB1)*12+6,MASKREAD_OFF,6);  

            vrange(tempindex1,6);                                       
            tempindex1 = vsadd(tempindex1,(PSS_SSS_end_RB-schPRB1)*12+6,MASKREAD_OFF,6);   //第二段+第三段
            vshuffle(PDSCHindex,tempindex2,tempindex1,SHUFFLE_SCATTER,6);  

            flag = 8;
            bias = schPRB1 *12;
        }
        //** */
        else if(RBflag == 9){
            num = (schPRB2 - schPRB1)*12 +6;
            vrange(PDSCHindex,num);
            
            flag = 9;
            bias = schPRB1 * 12;
        }
        else if(RBflag == 10){
            num = (schPRB2 - schPRB1)*12 +6;
            vrange(PDSCHindex,num);

            flag = 10;
            bias = PSS_SSS_end_RB * 12 + 6;
        }
        //** */
        else if(RBflag == 11){
            num = 6 + (schPRB2 - PSS_SSS_end_RB)*12;
            vrange(PDSCHindex,num);
            
            flag = 11;
            bias = (PSS_SSS_end_RB)*12 +6;
        }
        else if(RBflag == 12){
            num = 6 + (PSS_SSS_start_RB - schPRB1)*12;
            vrange(PDSCHindex,num);

            flag = 12;
            bias = schPRB1*12;
        }
        //** */
        else if(RBflag == 13){
            num =12;
            vrange(PDSCHindex,6);      // 第一段
            tempindex1 = vsadd(PDSCHindex,72+6,MASKREAD_OFF,6);   // 第二段
            tempindex2 = vsadd(PDSCHindex,6,MASKREAD_OFF,6);   // 拼接的index
            vshuffle(PDSCHindex,tempindex2,tempindex1,SHUFFLE_SCATTER,6);   

            flag = 13;
            bias = PSS_SSS_start_RB*12;
        }
        //** */
        else if(RBflag == 14){
            num = 12 + (PSS_SSS_start_RB - schPRB1)*12 + (schPRB2 - PSS_SSS_end_RB)*12;
            vrange(PDSCHindex,6+(PSS_SSS_start_RB - schPRB1)*12 );      // 第一段

            vrange(tempindex1,6+(schPRB2 - PSS_SSS_end_RB)*12 );   
            tempindex2 = vsadd(tempindex1,6+(PSS_SSS_start_RB - schPRB1)*12,MASKREAD_OFF,6+(schPRB2 - PSS_SSS_end_RB)*12 );   // 拼接的index
            tempindex1 = vsadd(tempindex1,(PSS_SSS_end_RB-schPRB1)*12+6,MASKREAD_OFF,6+(schPRB2 - PSS_SSS_end_RB)*12 );   // 第二段     
            vshuffle(PDSCHindex,tempindex2,tempindex1,SHUFFLE_SCATTER,6+(schPRB2 - PSS_SSS_end_RB)*12);  

            flag = 14;
            bias = schPRB1*12;
        }
        
    }
    
    /************************ 无参考信号和同步信号的情况  *************************/
    else if( (DuplexMode == 1 && (ant == 1 || ant == 2) && (sym == 1 || sym == 2 || sym == 3 || sym ==8 || sym == 9 ||sym == 10 || sym == 12 || sym == 13))     // FDD 1天线和2天线 符号1 2 3 8 9 10 12 13
            || (DuplexMode == 1 && (ant == 4) && (sym ==2 || sym ==3 || sym == 9 || sym == 10 || sym == 12 || sym == 13))      // // FDD 4天线 符号2 3 9 10 12 13
            || (DuplexMode == 0 && (ant == 1 || ant == 2) && (sym == 1 || sym == 2 || sym == 3 || sym == 5 || sym == 6 || sym == 8 || sym == 9 || sym == 10 || sym == 12))        // TDD 1天线和2天线 符号1 2 3 5 6 8 9 10 12
            || (DuplexMode == 0 && (ant == 4) && (sym == 2 || sym ==3 || sym == 5 || sym ==6 || sym ==9 ||sym ==10 || sym == 12))    // TDD 4天线 符号2 3 5 6 9 10 12
        ){     
        num = 12 * RB;
        vrange(PDSCHindex, num);    
        
        flag = 15;
        bias = schPRB1*12;
    }

    /*********************** 只有参考信号的情况 ************************************/
    // k = 6m + (v + vshift) mod 6
    else{
        // CRS间隔为6
        if (ant == 1){
            num = (12-2) * RB;      // 一个符号内减去参考信号 剩下的RE数
            if (vshift == 0){
                PDSCHindex = vsadd(ltePDSCH_index_one_antenna_ID0,0,MASKREAD_OFF,num); 
            }else if(vshift == 1){
                PDSCHindex = vsadd(ltePDSCH_index_one_antenna_ID1,0,MASKREAD_OFF,num); 
            }else if(vshift == 2){
                PDSCHindex = vsadd(ltePDSCH_index_one_antenna_ID2,0,MASKREAD_OFF,num); 
            }else if(vshift == 3){
                PDSCHindex = vsadd(ltePDSCH_index_one_antenna_ID3,0,MASKREAD_OFF,num); 
            }else if(vshift == 4){
                PDSCHindex = vsadd(ltePDSCH_index_one_antenna_ID4,0,MASKREAD_OFF,num); 
            }else if(vshift == 5){
                PDSCHindex = vsadd(ltePDSCH_index_one_antenna_ID5,0,MASKREAD_OFF,num); 
            }
            flag = 16;
            bias = schPRB1*12;
        }

        // CRS间隔为3
        else if(ant == 2 || ant == 4){
            num = (12-4) * RB;      // 一个符号内减去参考信号 剩下的RE数
            if (vshift == 0){
                PDSCHindex = vsadd(ltePDSCH_index_two_antenna_ID0,0,MASKREAD_OFF,num); 
            }else if(vshift == 1){
                PDSCHindex = vsadd(ltePDSCH_index_two_antenna_ID1,0,MASKREAD_OFF,num); 
            }else if(vshift == 2){
                PDSCHindex = vsadd(ltePDSCH_index_two_antenna_ID2,0,MASKREAD_OFF,num); 
            }else if(vshift == 3){
                PDSCHindex = vsadd(ltePDSCH_index_two_antenna_ID0,0,MASKREAD_OFF,num); 
            }else if(vshift == 4){
                PDSCHindex = vsadd(ltePDSCH_index_two_antenna_ID1,0,MASKREAD_OFF,num); 
            }else if(vshift == 5){
                PDSCHindex = vsadd(ltePDSCH_index_two_antenna_ID2,0,MASKREAD_OFF,num); 
            }
            flag =17;
            bias = schPRB1*12;
        }


        // /************ 叠加偏置 ************/ 
        // 没用上，存表的时候已经考虑这个因素
        // if(sym <= 5){
        //     ns =0;
        // }else {
        //     ns =1;
        // }
        // //
        // if(port == 0){
        //     if (sym == 0 || sym == 7){
        //         v = 0;
        //     }else if(sym == 4 || sym == 11){
        //         v = 3;
        //     }
        // }else if ( port ==1){
        //     if (sym == 0 || sym == 7){
        //         v = 3;
        //     }else if(sym == 4 || sym == 11){
        //         v = 0;
        //     }
        // }else if ( port ==2){
        //     v = 3*(ns % 2);
        // }else if ( port ==3){
        //     v = 3 + 3*(ns % 2);
        // }

        PDSCHindex = vsadd(PDSCHindex,v,MASKREAD_OFF,num);

    }


    


    
    
    // PDSCHindex = vsadd(PDSCHindex,sym*nRB*12,MASKREAD_OFF,num);      // 根据符号叠加

    
    PDSCHindex = vsadd(PDSCHindex,bias,MASKREAD_OFF,num);      // 根据sch占用的RB位置叠加

    // printf("sym = %hd\n",&sym);
    printf("num = %hd\n",&num);
    printf("bias = %hd\n",&bias);
    printf("flag = %hd\n",&flag);

    // char word[20] = "indexgen finished";
    // printf("----------- %s -----------\n",&word);

    outlength.data = num;
    // vreturn(PDSCHindex,sizeof(PDSCHindex) ,&outlength,sizeof(outlength));
    vreturn(PDSCHindex,num*2 ,&outlength,sizeof(short));

}