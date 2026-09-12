//lte trubo 解速率匹配
//Created by wangqianli
#include "venus.h"
#include <stdint.h>
#include <string.h> 
#include "vmath.h"
#include "riscv_printf.h" 

typedef short __v2048i16 __attribute__((ext_vector_type(5000)));        //index变量用__v2048i16
typedef char  __v4096i8 __attribute__((ext_vector_type(5000)));         //数据用4096i8
// typedef short __v2048i16 __attribute__((ext_vector_type(2048)));        //index变量用__v2048i16
// typedef char  __v4096i8 __attribute__((ext_vector_type(4096)));         //数据用4096i8

typedef short __v6148i16 __attribute__((ext_vector_type(15600)));        //index变量用__v2048i16
typedef char  __v6148i8 __attribute__((ext_vector_type(15600)));         //数据用4096i8

// typedef char  __v8400i8 __attribute__((ext_vector_type(16800)));         //数据用4096i8

/*
% 输入:
 rmData_E:需要进行解速率的数据
 trblklen:编码前的数据长度
 schfullLength :sch占用的资源个数
 RV : 冗余版本

 
// % pdsch.
// %   CFI：1 or 2 or 3
// %   CellRefP: 几天线 1 or 2 or 4
// %   Nframe: 在第几个子帧上传
// %   PRB: 一共几个RB
// %   Modulation：调制方式,决定了Qm
// %   RV:
// %   Tech：TDD or FDD  
// %   TDDConfig：TDD UL/DL config    0~6
// %   TDDSpecialConfig: DwPTS、GP、UpPTS  0~10
//     %   CyclicPrefix: normal or extended
// %

% 输出：
%   rate_recoverd_cdblksizes：解速率输出数据
*/



typedef struct {
    short data;
} __attribute__((aligned(64))) short_struct;



int Task_lteRateRecover(__v6148i8 rmData_E,__v2048i16 Permute_Col_32,__v6148i16 pi,
                        short_struct trblklength,short_struct schfullLength,short_struct RV ){
    
    short trblklen = trblklength.data;      //原始信息长度
    short interleave_blklen = trblklen +24 ;             //交织长度
    short turboblklen = interleave_blklen +4 ;     //turbo码单个码流长度
    short cdblksizes = (trblklen+24+4)*3;     //解速率后的长度

    int E = schfullLength.data; //速率匹配长度     
    int nRV = RV.data;
    
    rmData_E = vsadd(rmData_E,0,MASKREAD_OFF,E);
    Permute_Col_32 = vsadd(Permute_Col_32,0,MASKREAD_OFF,32);
    pi = vsadd(pi,0,MASKREAD_OFF,interleave_blklen);


    // /******************************** 1.计算速率匹配长度E ********************************** */ 
    // int total = 12 *14 * pdsch.PRB;             // 一个子帧的总RE数：12个子载波*14个符号*RB数
    // int CFI_num = 12 * pdsch.CFI * pdsch.PRB;   // PDCCH symbols 控制信息开销
    // int E = total - CFI_num;
    // int CRS_num;   


    // //参考信号开销：参考TS36.211 6.10.1  忽略前pdsch.CFI个符号的参考信号
    // switch (pdsch.CellRefP){
    //     case 1 :
    //         CRS_num = 6 * pdsch.PRB;  
    //         break;    
    //     case 2 :
    //         CRS_num = 12 * pdsch.PRB;   
    //         break;
    //     case 4 :
    //         if (pdsch.CFI ==1){ CRS_num = 20 * pdsch.PRB; }
    //         else  {CRS_num = 16 * pdsch.PRB;}
    //         break;
    // }

    // // 根据复用方式的不同，计算E
    // /**************************** FDD ********************************/
    // if (pdsch.Tech == 0 ){  
    //     E = E - CRS_num;
    
    //     //同步信号开销
    //     int PSS_SSS_num = 6 * 12 * 2 ;     // PSS和SSS的总开销：6个RB * 12个子载波 * 2个符号
    //     if (pdsch.Nframe == 5){           // 若子帧5上，还需要减去PSS、SSS的开销
    //         E = E- PSS_SSS_num;
    //     }else if (pdsch.Nframe == 0){       // 若子帧0上，除了PSS、SSS的开销、还有PBCH
    //         int CRS_redunt;
    //         int PBCH_num;
    //         if (pdsch.CellRefP == 1){
    //             CRS_redunt = 2 * 6;   
    //         }else if (pdsch.CellRefP == 2){
    //             CRS_redunt = 4 * 6;
    //         }else if (pdsch.CellRefP == 4){
    //             CRS_redunt = 8 * 6;
    //         } 
    //         PBCH_num = 6 * 12 * 4 - CRS_redunt;         
    //         E =  E - PSS_SSS_num - PBCH_num;
    //     }
    //     // else{
    //     //     E = E - 0;
    //     // }


    // /************************* TDD 参考TS36.211 4.2 ***********************/
    // }else{      
    //     // 同步信号开销：6个RB * 12个子载波 * 1个符号
    //     int SSS_num = 6 * 12;           // 位于子帧0和子帧5
    //     int PSS_num = SSS_num;          // 位于子帧1和子帧6

    //     // 特殊子帧上的开销（除了CFI和同步信号占位）
    //     //*******************若在特殊子帧上传输，需要根据pdsch.TDDSpecialConfig计算
    //     //  pdsch.TDDSpecialConfig = 0 : DwPTS:3    | GP:10| UpPTS:1
    //     //  pdsch.TDDSpecialConfig = 1 : DwPTS:9    | GP:4 | UpPTS:1
    //     //  pdsch.TDDSpecialConfig = 2 : DwPTS:10   | GP:3 | UpPTS:1
    //     //  pdsch.TDDSpecialConfig = 3 : DwPTS:11   | GP:2 | UpPTS:1
    //     //  pdsch.TDDSpecialConfig = 4 : DwPTS:12   | GP:1 | UpPTS:1
    //     //  pdsch.TDDSpecialConfig = 5 : DwPTS:3    | GP:9 | UpPTS:2
    //     //  pdsch.TDDSpecialConfig = 6 : DwPTS:9    | GP:3 | UpPTS:2
    //     //  pdsch.TDDSpecialConfig = 7 : DwPTS:10   | GP:2 | UpPTS:2
    //     //  pdsch.TDDSpecialConfig = 8 : DwPTS:11   | GP:1 | UpPTS:2
    //     //  pdsch.TDDSpecialConfig = 9 : DwPTS:6    | GP:6 | UpPTS:2     ？
    //     //  pdsch.TDDSpecialConfig = 10: DwPTS:6    | GP:2 | UpPTS:6     ？
    //     int Spi_num;        // GP和UpPTS的开销
    //     int CRS_Spinum;     // GP和UpPTS位置的CRS
    //     switch (pdsch.TDDSpecialConfig) {
    //         case 0:
    //             Spi_num = pdsch.PRB * 12 * (10 + 1);
    //             // DwPTS 位置的参考信号开销
    //             if (pdsch.CellRefP == 4 && pdsch.CFI == 1) {
    //                 CRS_Spinum = 4 * 6;
    //             } else {
    //                 CRS_Spinum = 0;
    //             }
    //             break;
    //         case 1:
    //             Spi_num = pdsch.PRB * 12 * (4 + 1);
    //             if (pdsch.CellRefP == 1) {
    //                 CRS_Spinum = 4 * 6;
    //             } else if (pdsch.CellRefP == 2) {
    //                 CRS_Spinum = 8 * 6;
    //             } else if (pdsch.CellRefP == 4) {
    //                 if (pdsch.CFI == 1) {
    //                     CRS_Spinum = 16 * 6;
    //                 } else {
    //                     CRS_Spinum = 12 * 6;
    //                 }
    //             }
    //             break;
    //         case 2:
    //             Spi_num = pdsch.PRB * 12 * (3 + 1);
    //             if (pdsch.CellRefP == 1) {
    //                 CRS_Spinum = 4 * 6;
    //             } else if (pdsch.CellRefP == 2) {
    //                 CRS_Spinum = 8 * 6;
    //             } else if (pdsch.CellRefP == 4) {
    //                 if (pdsch.CFI == 1) {
    //                     CRS_Spinum = 16 * 6;
    //                 } else {
    //                     CRS_Spinum = 12 * 6;
    //                 }
    //             }
    //             break;
    //         case 3:
    //             Spi_num = pdsch.PRB * 12 * (2 + 1);
    //             if (pdsch.CellRefP == 1) {
    //                 CRS_Spinum = 4 * 6;
    //             } else if (pdsch.CellRefP == 2) {
    //                 CRS_Spinum = 8 * 6;
    //             } else if (pdsch.CellRefP == 4) {
    //                 if (pdsch.CFI == 1) {
    //                     CRS_Spinum = 16 * 6;
    //                 } else {
    //                     CRS_Spinum = 12 * 6;
    //                 }
    //             }
    //             break;
    //         case 4:
    //             Spi_num = pdsch.PRB * 12 * (1 + 1);
    //             if (pdsch.CellRefP == 1) {
    //                 CRS_Spinum = 6 * 6;
    //             } else if (pdsch.CellRefP == 2) {
    //                 CRS_Spinum = 12 * 6;
    //             } else if (pdsch.CellRefP == 4) {
    //                 if (pdsch.CFI == 1) {
    //                     CRS_Spinum = 20 * 6;
    //                 } else {
    //                     CRS_Spinum = 16 * 6;
    //                 }
    //             }
    //             break;
    //         case 5:
    //             Spi_num = pdsch.PRB * 12 * (9 + 2);
    //             if (pdsch.CellRefP == 4 && pdsch.CFI == 1) {
    //                 CRS_Spinum = 4 * 6;
    //             }
    //             break;
    //         case 6:
    //             Spi_num = pdsch.PRB * 12 * (3 + 2);
    //             if (pdsch.CellRefP == 1) {
    //                 CRS_Spinum = 4 * 6;
    //             } else if (pdsch.CellRefP == 2) {
    //                 CRS_Spinum = 8 * 6;
    //             } else if (pdsch.CellRefP == 4) {
    //                 if (pdsch.CFI == 1) {
    //                     CRS_Spinum = 16 * 6;
    //                 } else {
    //                     CRS_Spinum = 12 * 6;
    //                 }
    //             }
    //             break;
    //         case 7:
    //             Spi_num = pdsch.PRB * 12 * (2 + 2);
    //             if (pdsch.CellRefP == 1) {
    //                 CRS_Spinum = 4 * 6;
    //             } else if (pdsch.CellRefP == 2) {
    //                 CRS_Spinum = 8 * 6;
    //             } else if (pdsch.CellRefP == 4) {
    //                 if (pdsch.CFI == 1) {
    //                     CRS_Spinum = 16 * 6;
    //                 } else {
    //                     CRS_Spinum = 12 * 6;
    //                 }
    //             }
    //             break;
    //         case 8:
    //             Spi_num = pdsch.PRB * 12 * (1 + 2);
    //             if (pdsch.CellRefP == 1) {
    //                 CRS_Spinum = 4 * 6;
    //             } else if (pdsch.CellRefP == 2) {
    //                 CRS_Spinum = 8 * 6;
    //             } else if (pdsch.CellRefP == 4) {
    //                 if (pdsch.CFI == 1) {
    //                     CRS_Spinum = 16 * 6;
    //                 } else {
    //                     CRS_Spinum = 12 * 6;
    //                 }
    //             }
    //             break;
    //         case 9:
    //             Spi_num = pdsch.PRB * 12 * (6 + 2);
    //             if (pdsch.CellRefP == 1) {
    //                 CRS_Spinum = 2 * 6;
    //             } else if (pdsch.CellRefP == 2) {
    //                 CRS_Spinum = 4 * 6;
    //             } else if (pdsch.CellRefP == 4) {
    //                 if (pdsch.CFI == 1) {
    //                     CRS_Spinum = 8 * 6;
    //                 } else {
    //                     CRS_Spinum = 4 * 6;
    //                 }
    //             }
    //             break;
    //         case 10:
    //             Spi_num = pdsch.PRB * 12 * (2 + 6);
    //             if (pdsch.CellRefP == 1) {
    //                 CRS_Spinum = 2 * 6;
    //             } else if (pdsch.CellRefP == 2) {
    //                 CRS_Spinum = 4 * 6;
    //             } else if (pdsch.CellRefP == 4) {
    //                 if (pdsch.CFI == 1) {
    //                     CRS_Spinum = 8 * 6;
    //                 } else {
    //                     CRS_Spinum = 4 * 6;
    //                 }
    //             }
    //             break;
    //         default:
    //             Spi_num = 0;
    //             CRS_Spinum = 0;
    //     }


    //     //******************* 判定下行在哪个子帧 
    //     // pdsch.TDDConfig = 0 : DL - 0,5        | UL - 2-4,7-9  | Spl: - 1,6
    //     // pdsch.TDDConfig = 1 : DL - 0,4-5,9    | UL - 2-3,7-8  | Spl: - 1,6
    //     // pdsch.TDDConfig = 2 : DL - 0,3-5,8-9  | UL - 2,7      | Spl: - 1,6
    //     // pdsch.TDDConfig = 3 : DL - 0,5-9      | UL - 2-4      | Spl: - 1
    //     // pdsch.TDDConfig = 4 : DL - 0,4-9      | UL - 2-3      | Spl: - 1
    //     // pdsch.TDDConfig = 5 : DL - 0,3-9      | UL - 2        | Spl: - 1
    //     // pdsch.TDDConfig = 6 : DL - 0,5,9      | UL - 2-4,7-8  | Spl: - 1,6           
    //     if (pdsch.Nframe == 0) {
    //         int PBCH_num;
    //         int CRS_redunt;
    //         E = E - CRS_num;
    //         if (pdsch.CellRefP == 1) {
    //             CRS_redunt = 2 * 6;
    //         } else if (pdsch.CellRefP == 2) {
    //             CRS_redunt = 4 * 6;
    //         } else if (pdsch.CellRefP == 4) {
    //             CRS_redunt = 8 * 6;
    //         }
    //         PBCH_num = 6 * 12 * 4 - CRS_redunt;
    //         E = E - SSS_num - PBCH_num;
    //     } else if (pdsch.Nframe == 5) {
    //         E = E - CRS_num - SSS_num;
    //     } else if ((pdsch.TDDConfig == 3 || pdsch.TDDConfig == 4 || pdsch.TDDConfig == 5) && (pdsch.Nframe == 6)) {
    //         E = E - CRS_Spinum - PSS_num;
    //     } else if ((pdsch.TDDConfig == 0 || pdsch.TDDConfig == 1 || pdsch.TDDConfig == 2 || pdsch.TDDConfig == 6) && (pdsch.Nframe == 1 || pdsch.Nframe == 6)) {
    //         E = E - CRS_Spinum - PSS_num - Spi_num;
    //     } else if ((pdsch.TDDConfig == 3 || pdsch.TDDConfig == 4 || pdsch.TDDConfig == 5) && (pdsch.Nframe == 1)) {
    //         E = E - CRS_Spinum - PSS_num - Spi_num;
    //     }
    // }    


    // // 根据调制方式，计算Qm
    // int Qm;
    // if (pdsch.Modulation == 0){         //BPSK
    //     Qm = 1;
    // }else if(pdsch.Modulation == 1){      //QPSK
    //     Qm = 2;
    // }else if(pdsch.Modulation == 2){      //16QAM
    //     Qm = 4;
    // }else if(pdsch.Modulation == 3){      //64QAM
    //     Qm = 6;
    // }else if(pdsch.Modulation == 4){      //256QAM
    //     Qm = 8;
    // }else if(pdsch.Modulation == 5){      //1024QAM
    //     Qm = 10;
    // }
    // E = E*Qm;
    printf("E = %d\n",&E);


    /************************************* 2.生成速率匹配的index ***************************** */
    int trblkcrclen = trblklen+24;  // +24位crc校验
    int Nt = trblkcrclen + 4;       // Length of each bit stream
    int C  = 32;                    // 列数     
    int R  = Nt*10 /C;              // 行数 = ceil(Nt/C)
    if(R %10 == 0){
        R = Nt/C;
    }else{
        R = Nt/C +1;
    }
    int Krect = R*C;                // Length of sub-interleaver, three Krect totally
    int Dk = Krect - Nt;            //Dummy bits的数量  ,Dk范围：0（含）~31（含） 
    // int Dk =3;
    int mergedSize = 3*Dk;
    printf("Dk=%d\n",&Dk);

    int Ncb = Krect * 3;
    int temp = Ncb*10 / (8 * R);
    if(temp %10 == 0){
        temp = Ncb / (8 * R);
    }else{
        temp = Ncb / (8 * R) +1;
    }
    int CB_Start = R * (2 * temp * RV.data + 2);  // 比特选择的起始数据位置:R * (2 * ceil(Ncb / (8 * R)) * pdsch.RV + 2)

    int s[Dk],p1[Dk],p2[Dk],p2temp[Dk],p[mergedSize];
    if(Dk>0){
        p2[Dk-1]= Krect-1;
        p2temp[Dk-1]= Krect-1;
    }
    int EdivNcb = E/Ncb;
    
    
    
    // __v2048i16 Permute_Col_32;      // 列置换模式
    __v2048i16 Permute_Col2_32;     // 列置换模式
    __v2048i16 S_P1_change;         // 列置换模式，用于计算dummy的位置
    __v2048i16 extract_cirbuf_index;           // 列置换模式，用于计算dummy的位置(最终)
    // vclaim(Permute_Col_32);
    vclaim(Permute_Col2_32);
    vclaim(S_P1_change);
    vclaim(extract_cirbuf_index);
    // // 搬移数组
    // int Permute_Col_32_addr = vaddr(Permute_Col_32);
    // vbarrier();
    // VSPM_OPEN();
    // for (size_t i = 0; i < 32; i++) {
    //     unsigned int addr                 = Permute_Col_32_addr + (i << 1);
    //     *(volatile unsigned short *)(addr) = Permute_Col[i];
    // }
    // VSPM_CLOSE();
    Permute_Col2_32 = vadd(Permute_Col_32,0,MASKREAD_OFF,32);
    S_P1_change = vadd(Permute_Col_32,0,MASKREAD_OFF,Dk);
    // P2_change = vadd(Permute_Col_32,0,MASKREAD_OFF,Dk-1);       //若Dk=1,则dummybits一定位于第（Krect-1）个，不需要通过置换数组知道


    /*0.子交织：
        定义R行C列的二维数组N_rect,生成从0到R*C-1的等差数组，按行填充到N_rect；
        列置换，按列取出；
    */
    __v2048i16 interleave_index_Krect;                  //子交织器输出索引
    __v2048i16 arithmetic_progression_Krect;            //等差数列
    __v2048i16 Permute_ColmulR_Krect;                   //置换索引
    
    vclaim(interleave_index_Krect);
    vclaim(arithmetic_progression_Krect);
    vclaim(Permute_ColmulR_Krect);

    __v2048i16 index_32;
    vclaim(index_32);
    vrange(index_32,C);

    //*******************  对于信息位和校验位1 *****************
    //拼接列转置向量
    for (int i=0; i<R; i++){
        vshuffle(Permute_ColmulR_Krect,index_32,Permute_Col_32,SHUFFLE_SCATTER,C); 
        index_32 = vadd(index_32,32,MASKREAD_OFF,32);
        Permute_Col_32 = vadd(Permute_Col_32,32,MASKREAD_OFF,32);
    }

    vrange(arithmetic_progression_Krect,Krect);         //生成从0到R*C-1的等差数组  
    vshuffle(interleave_index_Krect,Permute_ColmulR_Krect,arithmetic_progression_Krect,SHUFFLE_GATHER,Krect); //列置换

    //按列取出interleave_index_Krect
    __v2048i16 transpose_index;         //转置序列
    __v2048i16 interleave_out_Krect;    //信息位和校验位1的index
    __v2048i16 ones_Krect;   
    __v2048i16 multipleof32_Krect;
    __v2048i16 index_temp;              //用作索引
    vclaim(transpose_index);
    vclaim(interleave_out_Krect);
    vclaim(ones_Krect);
    vclaim(multipleof32_Krect);
    vclaim(index_temp);

    vbrdcst(transpose_index,0,MASKREAD_OFF,Krect);
    vbrdcst(ones_Krect,1,MASKREAD_OFF,Krect);
    vrange(multipleof32_Krect,Krect);
    vrange(index_temp,R);       
    multipleof32_Krect = vmul(multipleof32_Krect,32,MASKREAD_OFF,R);            //0,31,.....

    for(int i=0; i<C; i++){
        vshuffle(transpose_index,index_temp,multipleof32_Krect,SHUFFLE_SCATTER,R);
        multipleof32_Krect = vadd(multipleof32_Krect,1,MASKREAD_OFF,R);
        index_temp = vadd(index_temp,R,MASKREAD_OFF,R);
    }
    transpose_index = vadd(transpose_index,0,MASKREAD_OFF,Krect);
    vshuffle(interleave_out_Krect,transpose_index,interleave_index_Krect,SHUFFLE_GATHER,Krect);     //按列取出
    // interleave_out_Krect = vadd(interleave_out_Krect,0,MASKREAD_OFF,Krect);

    //*******************  对于校验位2 *****************
    // pai(k+1)= mod( Permute_Col(floor(k/R)+1) +C*mod(k,R) ,R*C);  %for matlab
    __v2048i16 pai_index_Krect;     // 校验位2的index
    __v2048i16 temp1;
    __v2048i16 temp2;
    __v2048i16 temp3;
    __v2048i16 temp4;
    vclaim(pai_index_Krect); 
    vclaim(temp1);
    vclaim(temp2);
    vclaim(temp3);
    vclaim(temp4);
    vrange(temp1,Krect);
    vbrdcst(temp2,R,MASKREAD_OFF,Krect);
    temp3 = vrem(temp2,temp1,MASKREAD_OFF,Krect);   //mod(k,R)
    temp3 = vsll(temp3,5,MASKREAD_OFF,Krect);       //C*mod(k,R)
    temp4 = vdiv(temp2,temp1,MASKREAD_OFF,Krect);   //floor(k/R)
    vshuffle(temp1,temp4,Permute_Col2_32,SHUFFLE_GATHER,Krect); //Permute_Col(floor(k/R))
    // Permute_Col2_32 = vadd(Permute_Col2_32,0,MASKREAD_OFF,Krect);
    // temp1 = vadd(temp1,0,MASKREAD_OFF,Krect);
    temp1 = vadd(temp1,temp3,MASKREAD_OFF,Krect);
    vbrdcst(temp2,Krect,MASKREAD_OFF,Krect);
    pai_index_Krect = vrem(temp2,temp1,MASKREAD_OFF,Krect);




    /* 1.子交织，输出一个R*C行 1列的矩阵 
        CB_Base_Pattern = S_SubInterleaver = interleave_out_Krect   //(上一步)
        P2_SubInterleaver[i] = (CB_Base_Pattern[i] + 1) % Krect;    //P2需要一个偏移量 ？
    */
    __v2048i16 P2_interleave_index_Krect;
    vclaim(P2_interleave_index_Krect);
    P2_interleave_index_Krect = vadd(pai_index_Krect,1,MASKREAD_OFF,Krect);
    P2_interleave_index_Krect = vrem(temp2,P2_interleave_index_Krect,MASKREAD_OFF,Krect);    

    

    /*2.对Filler比特的处理
        int Dummy[Dk];                              //虚比特
        int Filler[fillerBits]; 
        初始化数组 ：Dummy[i] = -1; 
        填充Dummy、Filler和S数组
    */
    // int Dk = Krect - Nt;                        //Dummy bits的数量  ,Dk范围：0（含）~31（含） 

    __v2048i16  S_Krect;
    __v2048i16  P1_Krect;
    __v2048i16  P2_Krect;
    __v2048i16  temp_Krect;
    
    vclaim(S_Krect);
    vclaim(P1_Krect);
    vclaim(P2_Krect);

    // 填充Dummy
    /*  
        S  = 1     :Nt;
        P1 = Nt+1  :Nt*2;
        P2 = Nt*2+1:Nt*3;
        S  = [Dummy,S];
        P1 = [Dummy,P1];             % 只有S和P1有Filler比特
        P2 = [Dummy,P2]; */

    vrange(temp1,Nt);
    temp2 = vadd(temp1,Nt,MASKREAD_OFF,Nt);
    temp3 = vadd(temp2,Nt,MASKREAD_OFF,Nt);
    temp4 = vadd(temp1,Dk,MASKREAD_OFF,Nt);     
    
    //直接先填全-1
    // vbrdcst(S_Krect,-1,MASKREAD_OFF,Krect);          
    // vbrdcst(P1_Krect,-1,MASKREAD_OFF,Krect);
    // vbrdcst(P2_Krect,-1,MASKREAD_OFF,Krect);
    vbrdcst(S_Krect,16383,MASKREAD_OFF,Krect);          
    vbrdcst(P1_Krect,16383,MASKREAD_OFF,Krect);
    vbrdcst(P2_Krect,16383,MASKREAD_OFF,Krect);
    //再放入数据
    vshuffle(S_Krect,temp4,temp1,SHUFFLE_SCATTER,Nt);   
    vshuffle(P1_Krect,temp4,temp2,SHUFFLE_SCATTER,Nt);   
    vshuffle(P2_Krect,temp4,temp3,SHUFFLE_SCATTER,Nt);  

    // temp1 = vadd(temp1,0,MASKREAD_OFF,Nt);  
    // temp2 = vadd(temp2,0,MASKREAD_OFF,Nt);  
    // temp3 = vadd(temp3,0,MASKREAD_OFF,Nt);  
    // temp4 = vadd(temp4,0,MASKREAD_OFF,Nt);  
    // S_Krect = vadd(S_Krect,0,MASKREAD_OFF,Krect);  
    // P1_Krect = vadd(P1_Krect,0,MASKREAD_OFF,Krect);  
    // P2_Krect = vadd(P2_Krect,0,MASKREAD_OFF,Krect);  
    // interleave_out_Krect = vadd(interleave_out_Krect,0,MASKREAD_OFF,Krect);  


    /*
        % Interleaving of S, P1 and P2
        S_Int  = S(S_SubInterleaver + 1);       % matlab是1-based，index要+1
        P1_Int = P1(P1_SubInterleaver + 1);
        P2_Int = P2(P2_SubInterleaver + 1);
    */
    temp_Krect = vsadd(S_Krect,0,MASKREAD_OFF,Krect);  
    vshuffle(S_Krect,interleave_out_Krect,temp_Krect,SHUFFLE_GATHER,Krect); 
    temp_Krect = vsadd(P1_Krect,0,MASKREAD_OFF,Krect); 
    vshuffle(P1_Krect,interleave_out_Krect,temp_Krect,SHUFFLE_GATHER,Krect); 
    temp_Krect = vsadd(P2_Krect,0,MASKREAD_OFF,Krect); 
    vshuffle(P2_Krect,P2_interleave_index_Krect,temp_Krect,SHUFFLE_GATHER,Krect); 
    // S_Krect = vadd(S_Krect,0,MASKREAD_OFF,Krect);  
    // P1_Krect = vadd(P1_Krect,0,MASKREAD_OFF,Krect);  
    // P2_Krect = vadd(P2_Krect,0,MASKREAD_OFF,Krect);  
  


    /*3.根据RV参数将接收数据放到CircularBuffer中 
        系统比特流S_Int放到Buffer的开始;
        P1 P2 比特流交替,放到S后面的Buffer中

        rmData = zeros(1,E);
        k = 0;
        j = 0;
        while k < E
            index = mod(CB_Start + j, Ncb);
            if index < length(CirBuffer) && (CirBuffer(index+1)) ~= -1
                rmData(k+1) = CirBuffer(index+1 );      % MATLAB 1-based索引
                k = k + 1;
            end
            j = j + 1;
        end
    */
    // int Ncb = Krect * 3;
    // int temp = Ncb*10 / (8 * R);
    // if(temp %10 == 0){
    //     temp = Ncb / (8 * R);
    // }else{
    //     temp = Ncb / (8 * R) +1;
    // }
    // int CB_Start = R * (2 * temp * pdsch.RV + 2);  // 比特选择的起始数据位置:R * (2 * ceil(Ncb / (8 * R)) * pdsch.RV + 2)
    // printf("CB_Start = %d\n",&CB_Start);

    __v2048i16 CirBuffer_Ncb;
    __v2048i16 index_out;           //最终输出的index
    __v2048i16 index_S_Int;
    __v2048i16 index_P1_Int;
    __v2048i16 index_P2_Int;

    vclaim(CirBuffer_Ncb);
    vclaim(index_out);
    vclaim(index_S_Int);
    vclaim(index_P1_Int);
    vclaim(index_P2_Int);
    vbrdcst(CirBuffer_Ncb,0,MASKREAD_OFF,Ncb);

    vrange(index_S_Int,Krect);
    vshuffle(CirBuffer_Ncb,index_S_Int,S_Krect,SHUFFLE_GATHER,Krect);   //系统比特流放到Buffer的开始,0~Krect-1

    vrange(index_P2_Int,Krect,MASKREAD_OFF);         //1 ~ Krect
    index_P1_Int = vmul(index_P2_Int,2,MASKREAD_OFF,Krect);                         //偶数序列                     
    index_P2_Int = vadd(index_P1_Int,1,MASKREAD_OFF,Krect);                         //奇数序列  
    index_P2_Int = vadd(index_P2_Int,Krect,MASKREAD_OFF,Krect);         
    index_P1_Int = vadd(index_P1_Int,Krect,MASKREAD_OFF,Krect);  
 
    //P1 P2 比特流交替,放到S后面的Buffer中
    // P1_Krect = vadd(P1_Krect,0,MASKREAD_OFF,Krect);
    // P2_Krect = vadd(P2_Krect,0,MASKREAD_OFF,Krect);
    vshuffle(CirBuffer_Ncb,index_P1_Int,P1_Krect,SHUFFLE_SCATTER,Krect); 
    vshuffle(CirBuffer_Ncb,index_P2_Int,P2_Krect,SHUFFLE_SCATTER,Krect);



    // 计算dummy在CirBuffer_Ncb中的位置
    // if(Dk > 0){     
    //     //step1:计算信息位S中的dummybits位置
    //     S_P1_change = vmul(S_P1_change,R,MASKREAD_OFF,Dk);
    //     vbarrier();
    //     VSPM_OPEN();
    //     for(int i =0; i< Dk;i++){

    //         s[i] = *(volatile unsigned short *) (vaddr(S_P1_change)+(i<<1));   //取出当前向量中的最小值
    //     }
    //     VSPM_CLOSE();
    //     for(int i =0; i<Dk-1; i++){
    //         p2temp[i]= s[i];
    //     }

    //     //step2:冒泡排序：升序
    //     int temp;
    //     for (int i = 0; i < Dk - 1; i++) {
    //         for (int j = 0; j < Dk - i - 1; j++) {
    //             if (s[j] > s[j + 1]) {
    //                 // 交换 arr[j] 和 arr[j+1]
    //                 temp = s[j];
    //                 s[j] = s[j + 1];
    //                 s[j + 1] = temp;
    //             }
    //         }
    //     }
    //     for (int i = 0; i < Dk - 1; i++) {
    //         for (int j = 0; j < Dk - i - 1; j++) {
    //             if (p2temp[j] > p2temp[j + 1]) {
    //                 // 交换 arr[j] 和 arr[j+1]
    //                 temp = p2temp[j];
    //                 p2temp[j] = p2temp[j + 1];
    //                 p2temp[j + 1] = temp;
    //             }
    //         }
    //     }
        

    //     //step3:计算P1中dummybits的位置
    //     p1[0]= s[0]+Krect;
    //     for(int i =1; i<Dk; i++){
    //         temp = (s[i]-s[i-1]) <<1;   //差值*2
    //         p1[i]= p1[i-1]+temp;
    //     }
    //     //step4:计算P2中dummybits的位置
    //     p2[0]= p1[0]+1;
    //     for(int i =1; i<Dk; i++){
    //         temp = (p2temp[i]-p2temp[i-1]) <<1;   //差值*2
    //         p2[i]= p2[i-1]+temp;
    //     }
        

    //     //step5:合并s、p1、p2，并升序排序得到p
    //     int i, j, k = 0;
    //     // 先将 s 数组元素放入 p 数组
    //     for (i = 0; i < Dk; i++) {
    //         p[k++] = s[i];
    //     }
    //     // 将 p1 数组元素放入 p 数组
    //     for (i = 0; i < Dk; i++) {
    //         p[k++] = p1[i];
    //     }
    //     // 再将 p2 数组元素放入 p 数组
    //     for (j = 0; j < Dk; j++) {
    //         p[k++] = p2[j];
    //     } 
    //     // 对合并后的数组进行排序
    //     for (int i = 0; i < mergedSize - 1; i++) {
    //         for (int j = 0; j < mergedSize - i - 1; j++) {
    //             if (p[j] > p[j + 1]) {
    //                 // 交换 arr[j] 和 arr[j+1]
    //                 temp = p[j];
    //                 p[j] = p[j + 1];
    //                 p[j + 1] = temp;
    //             }
    //         }
    //     }
    //     // 去除重复元素
    //     int uniqueIndex = 0;
    //     for (i = 0; i < mergedSize - 1; i++) {
    //         if (p[i] != p[i + 1]) {
    //             p[uniqueIndex++] = p[i];
    //         }
    //     }
    //     p[uniqueIndex++] = p[mergedSize - 1];
        

    //     //step6:计算p中比CB_start大的最小的数CB_start_p和位置pos
    //     int CB_Start_p = 0;
    //     int pos = 0;  
    //     int flag;  
    //     for(pos =0; pos<mergedSize && CB_Start_p < CB_Start; pos++){
    //         if (p[pos] == CB_Start){
    //             CB_Start_p = p[pos];
    //             flag=0;
    //         }else if(p[pos]>CB_Start){
    //             CB_Start_p = p[pos];
    //             flag=1;
    //         } 
    //     }
    //     pos = pos-1;
    //     printf("pos=%d\n",&pos);
    //     printf("CB_Start_p=%d\n",&CB_Start_p);
    //     printf("CB_Start=%d\n",&CB_Start);
    //     printf("flag=%d\n",&flag);


    //     //step7:生成最终的 用于从缓存区取数的index
    //     if(flag == 1){
    //         //先计算从CB_Start开始的第一段
    //         temp = p[pos]- CB_Start;    //计算长度temp
    //         vrange(temp1,temp);         //temp1用作extract_index
    //         vrange(temp2,temp);         //temp2用于拼接temp1生成的index
    //         temp1 = vadd(temp1,CB_Start,MASKREAD_OFF,temp);
    //         vshuffle(extract_cirbuf_index,temp2,temp1,SHUFFLE_SCATTER,temp);
    //         CB_Start_p = temp;
    //         // printf("CB_Start_p=%d\n",&CB_Start_p);

    //         //计算从CB_Start开始的第二段到最后一段
    //         for(int i = pos+1; i< mergedSize;i++){
    //             temp = p[i]- p[i-1]-1;        //计算长度temp
    //             // printf("temp = %d\n", &temp);
    //             if(temp > 0){
    //                 vrange(temp1,temp);         //temp1用作extract_index
    //                 vrange(temp2,temp);         //temp2用于拼接temp1生成的index
    //                 temp1 = vadd(temp1,p[i-1]+1,MASKREAD_OFF,temp);
    //                 temp2 = vadd(temp2,CB_Start_p,MASKREAD_OFF,temp);
    //                 vshuffle(extract_cirbuf_index,temp2,temp1,SHUFFLE_SCATTER,temp);  
    //                 CB_Start_p = CB_Start_p + temp;
    //                 // printf("CB_Start_p = %d\n", &CB_Start_p);
    //             }
    //         }
    //         //计算第一段到第pos段
    //         for(int i =0;i< pos; i++){
    //             temp = p[i+1]- p[i]-1;        //计算长度temp
    //             // printf("temp = %d\n", &temp);
    //             if(temp > 0){
    //                 vrange(temp1,temp);         //temp1用作extract_index
    //                 vrange(temp2,temp);         //temp2用于拼接temp1生成的index
    //                 temp1 = vadd(temp1,p[i]+1,MASKREAD_OFF,temp);
    //                 temp2 = vadd(temp2,CB_Start_p,MASKREAD_OFF,temp);
    //                 vshuffle(extract_cirbuf_index,temp2,temp1,SHUFFLE_SCATTER,temp);  
    //                 CB_Start_p = CB_Start_p + temp;
    //                 // printf("CB_Start_p = %d\n", &CB_Start_p);
    //             }
    //         }
    //         //填充extract_cirbuf_index剩下的部分
    //         k = p[pos]- CB_Start;
    //         temp = CB_Start_p - k;
    //         // printf("k = %d\n", &k);
    //         // printf("temp = %d\n", &temp);
    //         vrange(temp2,temp);
    //         temp2 = vadd(temp2,k,MASKREAD_OFF,temp);
    //         vshuffle(temp1,temp2,extract_cirbuf_index,SHUFFLE_GATHER,temp);     //取出需要重复的部分
    //         for(int i=0; i<EdivNcb; i++){
    //             temp2 = vadd(temp2,temp,MASKREAD_OFF,temp);
    //             vshuffle(extract_cirbuf_index,temp2,temp1,SHUFFLE_SCATTER,temp);
    //         }
    //     }else{      //flag==0
    //         //计算从CB_Start开始的第一段到最后一段
    //         CB_Start_p = 0;
    //         for(int i = pos+1; i< mergedSize;i++){
    //         // for(int i = pos+1; i< pos+3;i++){
    //             temp = p[i]- p[i-1]-1;        //计算长度temp
    //             // printf("temp = %d\n", &temp);    
    //             if(temp > 0){
    //                 vrange(temp1,temp);         //temp1用作extract_index
    //                 vrange(temp2,temp);         //temp2用于拼接temp1生成的index
    //                 temp1 = vadd(temp1,p[i-1]+1,MASKREAD_OFF,temp);
    //                 temp2 = vadd(temp2,CB_Start_p,MASKREAD_OFF,temp);
    //                 vshuffle(extract_cirbuf_index,temp2,temp1,SHUFFLE_SCATTER,temp);  
    //                 CB_Start_p = CB_Start_p + temp;
    //                 // printf("CB_Start_p = %d\n", &CB_Start_p);
    //             }
    //         }
    //         //计算第一段到第pos段
    //         for(int i =0;i< pos; i++){
    //             temp = p[i+1]- p[i]-1;        //计算长度temp
    //             // printf("temp = %d\n", &temp);
    //             if(temp > 0){
    //                 vrange(temp1,temp);         //temp1用作extract_index
    //                 vrange(temp2,temp);         //temp2用于拼接temp1生成的index
    //                 temp1 = vadd(temp1,p[i]+1,MASKREAD_OFF,temp);
    //                 temp2 = vadd(temp2,CB_Start_p,MASKREAD_OFF,temp);
    //                 vshuffle(extract_cirbuf_index,temp2,temp1,SHUFFLE_SCATTER,temp);  
    //                 CB_Start_p = CB_Start_p + temp;
    //                 // printf("CB_Start_p = %d\n", &CB_Start_p);
    //             }
    //         }
    //         //填充extract_cirbuf_index剩下的部分
    //         vrange(temp2,cdblksizes);
    //         vshuffle(temp1,temp2,extract_cirbuf_index,SHUFFLE_GATHER,cdblksizes);     //取出需要重复的部分
    //         for(int i=0; i<EdivNcb; i++){
    //             temp2 = vadd(temp2,cdblksizes,MASKREAD_OFF,cdblksizes);
    //             vshuffle(extract_cirbuf_index,temp2,temp1,SHUFFLE_SCATTER,cdblksizes);
    //         }

    //     }

    // }

    // // for (int i = 0; i < Dk; i++) {
    // //     printf("s[%d]=",&i);
    // //     printf("%d\t", &s[i]);

    // //     printf("p1[%d]=",&i);
    // //     printf("%d\t", &p1[i]);

    // //     printf("p2[%d]=",&i);
    // //     printf("%d\n", &p2[i]);

    // // }
    // // for (int i = 0; i < mergedSize;i++) {
    // //     printf("p[%d]=",&i);
    // //     printf("%d\n", &p[i]);
    // // }
    int k = 0;                                // 已收集的有效比特个数
    int j = 0;                                // 扫描步数

    int cir_addr  = vaddr(CirBuffer_Ncb);          // 向量缓冲基址（16bit 步长）
    int out_addr  = vaddr(extract_cirbuf_index);    // 输出下标向量的缓冲基址

    vbarrier();
    VSPM_OPEN();
    while (k < E) {
        int index = (CB_Start + j) % Ncb;
        unsigned short val = *(volatile unsigned short *)(cir_addr + (index <<1));  // 读取 CirBuffer_Ncb[index]
        if (val != 16383 && (index< Krect*3)) {
            // printf("index = %d \t",&index);
            // printf("Cirbuffer = %hd \t\t\t\t",&val);
            *(volatile unsigned short *)(out_addr + (k <<1)) = (unsigned short)val;  // 写出有效下标
            k++;
        }
        j++;
    }
    VSPM_CLOSE();

    //提取缓冲区的数据
    extract_cirbuf_index = vadd(extract_cirbuf_index,0,MASKREAD_OFF,E);         //371
    // CirBuffer_Ncb = vadd(CirBuffer_Ncb,0,MASKREAD_OFF,Ncb);                     //372
    // vshuffle(index_out,extract_cirbuf_index,CirBuffer_Ncb,SHUFFLE_GATHER,E);    //373
    // index_out = vadd(index_out,0,MASKREAD_OFF,E);           //374
    vbrdcst(index_out,0,MASKREAD_OFF,E);
    index_out = vadd(extract_cirbuf_index,0,MASKREAD_OFF,E); 


    /************************************* 3.解速率匹配 ***************************** */
    // __v4096i8 rmData_E;                     //解速率输入数据
    __v6148i8 rate_recoverd_cdblksizes;     //解速率输出数据
    __v6148i8 dataout_temp1;
    __v6148i8 dataout_temp2;
    __v6148i8 dataout_temp3;
    __v6148i8 dataout_temp4;
    // vclaim(rmData_E);
    vclaim(rate_recoverd_cdblksizes);
    vclaim(dataout_temp1);
    vclaim(dataout_temp2);
    vclaim(dataout_temp3);
    vclaim(dataout_temp4);
    // int rmData_E_addr = vaddr(rmData_E);
    // vbarrier();
    // VSPM_OPEN();
    // for (size_t i = 0; i < E; i++) {
    //     unsigned int addr                 = rmData_E_addr + (i);               //i8不用移位
    //     *(volatile unsigned char *)(addr) = rmData[i];
    // }
    // VSPM_CLOSE();
    rmData_E = vsadd(rmData_E,0,MASKREAD_OFF,E);    
    rmData_E = vsra(rmData_E,1,MASKREAD_OFF,E);     //缩放数据 376
    
    // vbrdcst(dataout_temp2,0,MASKREAD_OFF,15600);
    /*
        iv = indices{r};
        for i=1:length(iv)
            outcb(iv(i)) = outcb(iv(i)) + codeword(bidx+i);
        end
        可以利用分段累加的方式达到以上效果
    */
    //step1:先获得前cdblksizes个输入
    vshuffle(dataout_temp1,index_out,rmData_E,SHUFFLE_SCATTER,cdblksizes);  //377
    dataout_temp1 = vadd(dataout_temp1,0,MASKREAD_OFF,cdblksizes);      //378
    //step2:把从第cdblksizes+1个开始的输入叠加给datatemp，每次cdblksizes个
    vrange(temp1,cdblksizes);       //379
    for(int i=0;i< EdivNcb-1;i++){
    // for(int i=0;i< 1;i++){
        temp1 = vadd(temp1,cdblksizes,MASKREAD_OFF,cdblksizes);              //用于提取数据的index 
        
        vshuffle(temp2,temp1,index_out,SHUFFLE_GATHER,cdblksizes);
        // temp2 = vadd(temp2,0,MASKREAD_OFF,cdblksizes);     
        
        vshuffle(dataout_temp2,temp1,rmData_E,SHUFFLE_GATHER,cdblksizes);   //从输入数据中提取的一小段
        // dataout_temp2 = vadd(dataout_temp2,0,MASKREAD_OFF,cdblksizes);  
        
        vshuffle(dataout_temp3,temp2,dataout_temp2,SHUFFLE_SCATTER,cdblksizes);     
        dataout_temp1 = vadd(dataout_temp3,dataout_temp1,MASKREAD_OFF,cdblksizes);
    }
    //最后一段
    temp = E - EdivNcb*cdblksizes;
    printf("temp = %d\n",&temp);
    printf("EdivNcb = %d\n",&EdivNcb);
    printf("cdblksizes = %hd\n",&cdblksizes);
    vbrdcst(dataout_temp3,0,MASKREAD_OFF,15600);        //380
    vrange(temp1,temp);
    temp1 = vadd(temp1,EdivNcb*cdblksizes,MASKREAD_OFF,temp);   //382

    temp2 = vadd(temp2,0,MASKREAD_OFF,E);   //383
    vshuffle(temp2,temp1,index_out,SHUFFLE_GATHER,temp);    //384×
    temp2 = vadd(temp2,0,MASKREAD_OFF,E); //385

    dataout_temp2 = vadd(dataout_temp2,0,MASKREAD_OFF,E);       //386
    vshuffle(dataout_temp2,temp1,rmData_E,SHUFFLE_GATHER,temp);   //从输入数据中提取的一小段 387×
    dataout_temp2 = vadd(dataout_temp2,0,MASKREAD_OFF,E); //388×

    temp1 = vadd(temp1,0,MASKREAD_OFF,temp);    //389
    temp2 = vadd(temp2,0,MASKREAD_OFF,temp); 
    // dataout_temp2 = vadd(dataout_temp2,0,MASKREAD_OFF,temp); 
    index_out = vadd(index_out,0,MASKREAD_OFF,E);   //391



    vshuffle(dataout_temp3,temp2,dataout_temp2,SHUFFLE_SCATTER,temp);   //392
    dataout_temp3 = vadd(dataout_temp3,0,MASKREAD_OFF,cdblksizes);      //393

    rate_recoverd_cdblksizes = vadd(dataout_temp3,dataout_temp1,MASKREAD_OFF,cdblksizes);   //394



    /**************************分割输出数据 *******************/
    __v2048i16 split_index;
    vclaim(split_index);

    __v6148i8 soft_in1;
    __v6148i8 soft_in2;
    __v6148i8 soft_in3;
    __v6148i8 soft_in4;
    vclaim(soft_in1);
    vclaim(soft_in2);
    vclaim(soft_in3);
    vclaim(soft_in4);

    vrange(split_index,turboblklen);
    soft_in1 = vsadd(rate_recoverd_cdblksizes,0,MASKREAD_OFF,turboblklen);
    split_index = vsadd(split_index,turboblklen,MASKREAD_OFF,turboblklen);
    vshuffle(soft_in2,split_index,rate_recoverd_cdblksizes,SHUFFLE_GATHER,turboblklen);
    split_index = vsadd(split_index,turboblklen,MASKREAD_OFF,turboblklen);
    vshuffle(soft_in4,split_index,rate_recoverd_cdblksizes,SHUFFLE_GATHER,turboblklen);

    // //soft_in3 = soft_in1(pi)
    // vshuffle(soft_in3,pi,soft_in1,SHUFFLE_GATHER,interleave_blklen);

    
    // pi = vsadd(pi,0,MASKREAD_OFF,interleave_blklen);
    // soft_in1 = vsadd(soft_in1,0,MASKREAD_OFF,turboblklen);
    // soft_in2 = vsadd(soft_in2,0,MASKREAD_OFF,turboblklen);
    // soft_in3 = vsadd(soft_in3,0,MASKREAD_OFF,turboblklen);
    // soft_in4 = vsadd(soft_in4,0,MASKREAD_OFF,turboblklen);

    vbrdcst(soft_in3,0,MASKREAD_OFF,6148);
    vshuffle(soft_in3,pi,soft_in1,SHUFFLE_GATHER,interleave_blklen);
    soft_in3 = vsadd(soft_in3,0,MASKREAD_OFF,turboblklen);




    soft_in1 = vsadd(soft_in1,0,MASKREAD_OFF,turboblklen);
    soft_in2 = vsadd(soft_in2,0,MASKREAD_OFF,turboblklen);
    soft_in3 = vsadd(soft_in3,0,MASKREAD_OFF,turboblklen);
    soft_in4 = vsadd(soft_in4,0,MASKREAD_OFF,turboblklen);
    index_out = vsadd(index_out,0,MASKREAD_OFF,E);


    /****************** 缩放数据 *******************/
    soft_in1 = vsra(soft_in1,1,MASKREAD_OFF,turboblklen);
    soft_in2 = vsra(soft_in2,1,MASKREAD_OFF,turboblklen);
    soft_in3 = vsra(soft_in3,1,MASKREAD_OFF,turboblklen);
    soft_in4 = vsra(soft_in4,1,MASKREAD_OFF,turboblklen);

    vsetshamt(7);
    soft_in1 = vmul(soft_in1,17,MASKREAD_OFF,turboblklen);
    soft_in2 = vmul(soft_in2,17,MASKREAD_OFF,turboblklen);
    soft_in3 = vmul(soft_in3,17,MASKREAD_OFF,turboblklen);
    soft_in4 = vmul(soft_in4,17,MASKREAD_OFF,turboblklen);
    

    // soft_in1 = vsll(soft_in1,1,MASKREAD_OFF,turboblklen);
    // soft_in2 = vsll(soft_in2,1,MASKREAD_OFF,turboblklen);
    // soft_in3 = vsll(soft_in3,1,MASKREAD_OFF,turboblklen);
    // soft_in4 = vsll(soft_in4,1,MASKREAD_OFF,turboblklen);
    

    soft_in1 = vsadd(soft_in1,0,MASKREAD_OFF,turboblklen);
    soft_in2 = vsadd(soft_in2,0,MASKREAD_OFF,turboblklen);
    soft_in3 = vsadd(soft_in3,0,MASKREAD_OFF,turboblklen);
    soft_in4 = vsadd(soft_in4,0,MASKREAD_OFF,turboblklen);
    rate_recoverd_cdblksizes = vsadd(rate_recoverd_cdblksizes,0,MASKREAD_OFF,cdblksizes);
 
  
    // // char words[26] = "lte rate recover finished";
    // // printf("----------------%s------------------\n",&words);

    // __v4096i8 soft_in1;
    // __v4096i8 soft_in2;
    // __v4096i8 soft_in3;
    // __v4096i8 soft_in4;
    // vclaim(soft_in1);
    // vclaim(soft_in2);
    // vclaim(soft_in3);
    // vclaim(soft_in4);

    
    vreturn(soft_in1,turboblklen,soft_in2,turboblklen,soft_in3,turboblklen,soft_in4,turboblklen);
    
 
}

/*rate_recoverd_cdblksizes输出数据：
211,35,214,208,209,200,186,184,217,41,51,200,205,35,48,205,204,41,54,188,209,51,48,68,208,209,38,62,213,209,46,53,215,218,33,202,194,224,207,184,205,217,201,199,195,221,215,215,206,218,190,180,216,206,219,188,210,204,231,192,213,218,195,196,222,217,196,194,216,211,207,192,201,222,222,191,215,211,213,209,210,211,203,201,209,208,202,199,210,225,202,176,208,215,206,219,211,211,200,188,211,208,200,193,213,209,206,185,214,218,205,217,207,220,217,184,217,218,191,203,216,221,199,182,204,191,209,199,228,221,211,174,201,204,200,184,217,206,199,198,198,215,205,209,209,212,214,197,207,205,215,186,211,208,197,203,215,212,217,215,210,203,204,200,215,216,197,191,198,206,211,199,216,207,200,215,43,50,209,183,49,25,51,177,197,210,201,58,201,216,57,45,216,208,42,60,38,227,213,192,207,216,214,60,192,52,66,35,67,202,200,40,193,202,64,43,190,194,62,38,181,204,47,61,199,200,46,215,74,204,56,48,202,55,52,34,56,30,189,46,54,209,198,38,185,42,63,52,194,185,55,214,53,50,68,213,186,56,204,35,52,55,183,209,55,204,56,45,61,212,207,49,195,37,65,43,196,211,60,206,69,62,67,212,188,38,201,36,61,53,191,203,72,215,51,36,52,193,193,56,196,36,70,56,188,205,62,210,61,39,46,190,186,50,200,38,65,53,188,212,49,199,46,47,63,212,176,48,192,50,50,41,202,218,57,207,67,44,57,213,192,39,200,51,69,66,187,208,56,204,45,42,64,173,196,47,196,45,68,45,192,201,63,201,50,31,55,208,185,44,196,42,69,39,185,215,58,193,52,37,192,218,180,45,192,41,49,46,62,31,176,48,189,63,56,33,50,47,195,210,61,203,197,207,192,218,198,49,206,184,38,53,55,58,206,196,63,193,43,64,41,192,50,198,59,185,211,195,199,59,192,55,57,56,218,55,199,192,197,65,58,196,213,69,204,60,51,76,207,198,37,58,231,191,201,192,211,198,212,191,209,185,188,196,208,60,44,209,47,202,190,200,221,203,209,192,193,208,216,197,202,178,232,194,212,199,57,50,46,59,202,193,50,190,52,193,216,65,49,60,47,46,53,198,211,207,224,193,203,204,195,183,207,203,215,202,213,203,72,60,42,49,219,192,35,201,50,70,43,57,50,179,33,55,58,197,192,61,205,51,48,68,219,198,46,56,43,55,43,80,206,206,46,193,61,61,45,65,43,66,58,71,46,65,208,198,39,191,44,38,45,187,215,191,53,53,31,203,214,202,205,49,191,59,46,48,209,62,213,65,215,57,39,55,42,57,219,45,47,58,208,192,57,201,46,55,52,189,203,197,216,52
*/

/*soft_in :
softi_in1 : 211,35,214,208,209,200,186,184,217,41,51,200,205,35,48,205,204,41,54,188,209,51,48,68,208,209,38,62,213,209,46,53,215,218,33,202,194,224,207,184,205,217,201,199,195,221,215,215,206,218,190,180,216,206,219,188,210,204,231,192,213,218,195,196,222,217,196,194,216,211,207,192,201,222,222,191,215,211,213,209,210,211,203,201,209,208,202,199,210,225,202,176,208,215,206,219,211,211,200,188,211,208,200,193,213,209,206,185,214,218,205,217,207,220,217,184,217,218,191,203,216,221,199,182,204,191,209,199,228,221,211,174,201,204,200,184,217,206,199,198,198,215,205,209,209,212,214,197,207,205,215,186,211,208,197,203,215,212,217,215,210,203,204,200,215,216,197,191,198,206,211,199,216,207,200,215,43,50,209,183,49,25,51,177,197,210,201,58,201,216,57,45,216,208,42,60,38,227,213,192,207,216,214,60
softi_in2 : 192,52,66,35,67,202,200,40,193,202,64,43,190,194,62,38,181,204,47,61,199,200,46,215,74,204,56,48,202,55,52,34,56,30,189,46,54,209,198,38,185,42,63,52,194,185,55,214,53,50,68,213,186,56,204,35,52,55,183,209,55,204,56,45,61,212,207,49,195,37,65,43,196,211,60,206,69,62,67,212,188,38,201,36,61,53,191,203,72,215,51,36,52,193,193,56,196,36,70,56,188,205,62,210,61,39,46,190,186,50,200,38,65,53,188,212,49,199,46,47,63,212,176,48,192,50,50,41,202,218,57,207,67,44,57,213,192,39,200,51,69,66,187,208,56,204,45,42,64,173,196,47,196,45,68,45,192,201,63,201,50,31,55,208,185,44,196,42,69,39,185,215,58,193,52,37,192,218,180,45,192,41,49,46,62,31,176,48,189,63,56,33,50,47,195,210,61,203,197,207,192,218,198,49
softi_in3 : 211,196,38,225,216,184,213,215,213,191,211,208,215,188,51,221,217,192,33,211,213,182,202,205,207,215,199,35,215,62,57,206,204,209,201,209,216,174,206,212,216,177,214,41,216,202,213,218,208,199,190,220,215,198,200,216,228,45,197,41,49,199,186,211,215,219,231,221,209,197,205,207,217,192,204,209,201,180,48,211,205,193,196,221,208,203,191,25,209,184,211,218,38,192,48,208,206,217,222,206,211,200,209,216,211,205,209,217,209,194,46,215,210,203,203,212,214,199,200,227,210,68,201,218,205,191,207,208,222,199,202,208,217,183,205,200,198,53,42,204,209,201,215,218,201,184,200,203,204,58,215,35,43,184,214,217,213,176,219,218,210,209,206,206,201,60,217,51,197,215,51,222,194,188,195,191,210,186,217,50,198,208,197,209,216,188,54,211,195,185,207,204,211,215,199,210,207,200,200,224,0,0,0,0
softi_in4 : 206,184,38,53,55,58,206,196,63,193,43,64,41,192,50,198,59,185,211,195,199,59,192,55,57,56,218,55,199,192,197,65,58,196,213,69,204,60,51,76,207,198,37,58,231,191,201,192,211,198,212,191,209,185,188,196,208,60,44,209,47,202,190,200,221,203,209,192,193,208,216,197,202,178,232,194,212,199,57,50,46,59,202,193,50,190,52,193,216,65,49,60,47,46,53,198,211,207,224,193,203,204,195,183,207,203,215,202,213,203,72,60,42,49,219,192,35,201,50,70,43,57,50,179,33,55,58,197,192,61,205,51,48,68,219,198,46,56,43,55,43,80,206,206,46,193,61,61,45,65,43,66,58,71,46,65,208,198,39,191,44,38,45,187,215,191,53,53,31,203,214,202,205,49,191,59,46,48,209,62,213,65,215,57,39,55,42,57,219,45,47,58,208,192,57,201,46,55,52,189,203,197,216,52

*/



 