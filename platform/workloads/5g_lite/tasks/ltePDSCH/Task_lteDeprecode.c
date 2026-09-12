//  lte pdsch 预解码和层映射处理
//  Created by wangqianli
// 包含 SFBCdecode(Created by shenyihao) 和lteEqualizeMMSE(Created by yuanfeng)

#include "venus.h"
#include <stdint.h>
#include <string.h> 
#include "vmath.h"
#include "riscv_printf.h" 

// typedef short __v2048i16 __attribute__((ext_vector_type(2048)));        //index变量用__v2048i16
// typedef char  __v4096i8 __attribute__((ext_vector_type(4096)));         //数据用4096i8
typedef short __v2048i16 __attribute__((ext_vector_type(4096)));
typedef char __v4096i8 __attribute__((ext_vector_type(8192)));

/* 
输入：
    nModulation ：0-BPSK*(pdsch无), 1-QPSK, 2-16QAM, 3-64QAM, 4-256QAM, 5-1024QAM
    nTxScheme：传输方案：0-TxDiversity, 1-Port0, 2-Port5, 3-Port7-8, 4-Port8
    antenna：天线数

    nschlength:sch的RE个数

    noiseEst：噪声估计
    pdschSymbols/rxSubframe：资源提取得到的子帧中的sch符号
    hest/chSubframe: 资源提取得到的 信道估计矩阵的sch部分



输出：
    symCombining_real: 输出，实部
    symCombining_imag:
    csi:  

*/


typedef struct {
    short data;
} __attribute__((aligned(64))) short_struct;


int Task_lteDeprecode(short_struct nModulation, short_struct nTxScheme, short_struct nschlength,short_struct antenna,
__v4096i8 pdschSymbols_real, __v4096i8 pdschSymbols_imag,
__v4096i8 hest_port0_real,__v4096i8 hest_port0_imag,__v4096i8 hest_port1_real,__v4096i8 hest_port1_imag,
__v4096i8 hest_port2_real,__v4096i8 hest_port2_imag,__v4096i8 hest_port3_real,__v4096i8 hest_port3_imag,
__v2048i16 index0145, __v2048i16 index048){

    short Modulation = nModulation.data;
    short TxScheme = nTxScheme.data;
    short schlength = nschlength.data;
    short Nt = antenna.data;

  
    int fractionLength = 6;     // 用于缩放
    int nVar           = 0;


    pdschSymbols_real = vsadd(pdschSymbols_real,0,MASKREAD_OFF,schlength);
    pdschSymbols_imag = vsadd(pdschSymbols_imag,0,MASKREAD_OFF,schlength);

    hest_port0_real = vsadd(hest_port0_real,0,MASKREAD_OFF,schlength);
    hest_port0_imag = vsadd(hest_port0_imag,0,MASKREAD_OFF,schlength);

    hest_port1_real = vsadd(hest_port1_real,0,MASKREAD_OFF,schlength);
    hest_port1_imag = vsadd(hest_port1_imag,0,MASKREAD_OFF,schlength);

    hest_port2_real = vsadd(hest_port2_real,0,MASKREAD_OFF,schlength);
    hest_port2_imag = vsadd(hest_port2_imag,0,MASKREAD_OFF,schlength);

    hest_port3_real = vsadd(hest_port3_real,0,MASKREAD_OFF,schlength);
    hest_port3_imag = vsadd(hest_port3_imag,0,MASKREAD_OFF,schlength);

    
    index0145 = vsadd(index0145,0,MASKREAD_OFF,schlength/2);
    index048 = vsadd(index048,0,MASKREAD_OFF,schlength/2);









    // 输出
    __v4096i8 symCombining_real;
    __v4096i8 symCombining_imag;
    __v4096i8 csi;
    vclaim(symCombining_real);
    vclaim(symCombining_imag);
    vclaim(csi);



    /************************** 发射分集 ************************/
    if(TxScheme == 0){
        // 开环空频编码（OSFBC）解码: [symCombining, csi] = lteTransmitDiversityDecode(pdschSymbols, hest);
        /*************************** 2天线 *****************************/
        if(Nt == 2){
            /*-----------------------index----------------------------*/
            __v2048i16 index_240e;
            __v2048i16 index_240o;
            vclaim(index_240e);
            vclaim(index_240o);
            vrange(index_240e, schlength/2);        
            index_240e = vmul(index_240e, 2, MASKREAD_OFF, schlength/2);    // 0,2,4,....
            index_240o = vsadd(index_240e, 1, MASKREAD_OFF, schlength/2);   // 1,3,5,....
          
            /*------------------------hest----------------------------*/
            __v4096i8 Hp11_real;
            __v4096i8 Hp22_real;
            __v4096i8 Hp12_real;
            __v4096i8 Hp21_real;
            vclaim(Hp11_real);
            vclaim(Hp22_real);
            vclaim(Hp12_real);
            vclaim(Hp21_real);
            vshuffle(Hp11_real, index_240e, hest_port0_real, SHUFFLE_GATHER, schlength/2);
            vshuffle(Hp22_real, index_240o, hest_port1_real, SHUFFLE_GATHER, schlength/2);
            vshuffle(Hp12_real, index_240e, hest_port1_real, SHUFFLE_GATHER, schlength/2);
            vshuffle(Hp21_real, index_240o, hest_port0_real, SHUFFLE_GATHER, schlength/2);
        
            __v4096i8 Hp11_imag;
            __v4096i8 Hp22_imag;
            __v4096i8 Hp12_imag;
            __v4096i8 Hp21_imag;
            vclaim(Hp11_imag);
            vclaim(Hp22_imag);
            vclaim(Hp12_imag);
            vclaim(Hp21_imag);
            vshuffle(Hp11_imag, index_240e, hest_port0_imag, SHUFFLE_GATHER, schlength/2);
            vshuffle(Hp22_imag, index_240o, hest_port1_imag, SHUFFLE_GATHER, schlength/2);
            vshuffle(Hp12_imag, index_240e, hest_port1_imag, SHUFFLE_GATHER, schlength/2);
            vshuffle(Hp21_imag, index_240o, hest_port0_imag, SHUFFLE_GATHER, schlength/2);
        
          
          
          
            /*----------------------------in----------------------------------*/
            __v4096i8 r1_real;
            __v4096i8 r2_real;
            __v4096i8 y1_real;
            __v4096i8 y2_real;
            __v4096i8 temp;
            vclaim(r1_real);
            vclaim(r2_real);
            vclaim(y1_real);
            vclaim(y2_real);
            __v4096i8 r1_imag;
            __v4096i8 r2_imag;
            __v4096i8 y1_imag;
            __v4096i8 y2_imag;
            vclaim(r1_imag);
            vclaim(r2_imag);
            vclaim(y1_imag);
            vclaim(y2_imag);
            vclaim(temp);  
          
            vshuffle(r1_real, index_240e, pdschSymbols_real, SHUFFLE_GATHER, schlength/2);
            vshuffle(r2_real, index_240o, pdschSymbols_real, SHUFFLE_GATHER, schlength/2);
            vshuffle(r1_imag, index_240e, pdschSymbols_imag, SHUFFLE_GATHER, schlength/2);
            vshuffle(r2_imag, index_240o, pdschSymbols_imag, SHUFFLE_GATHER, schlength/2);
          
          
        
          
          
            //输入1Q6
            //y1为2Q5
            /*----------------------------y1----------------------------------*/
            vsetshamt(7);
            y1_real = vmul(r1_real, Hp11_real, MASKREAD_OFF, schlength/2);
            temp    = vmul(r1_imag, Hp11_imag, MASKREAD_OFF, schlength/2);
            y1_real = vsadd(y1_real, temp, MASKREAD_OFF, schlength/2);
            
            temp    = vmul(r2_real, Hp22_real, MASKREAD_OFF, schlength/2);
            y1_real = vsadd(y1_real, temp, MASKREAD_OFF, schlength/2);
            temp    = vmul(r2_imag, Hp22_imag, MASKREAD_OFF, schlength/2);
            y1_real = vsadd(y1_real, temp, MASKREAD_OFF, schlength/2);
          
            y1_imag = vmul(r1_imag, Hp11_real, MASKREAD_OFF, schlength/2);
            temp    = vmul(r1_real, Hp11_imag, MASKREAD_OFF, schlength/2);
            y1_imag = vssub(temp, y1_imag, MASKREAD_OFF, schlength/2);  ///注意顺序
            
            temp    = vmul(r2_real, Hp22_imag, MASKREAD_OFF, schlength/2);
            y1_imag = vsadd(y1_imag, temp, MASKREAD_OFF, schlength/2);
            temp    = vmul(r2_imag, Hp22_real, MASKREAD_OFF, schlength/2);
            y1_imag = vssub(temp, y1_imag, MASKREAD_OFF, schlength/2);

            /*----------------------------y2----------------------------------*/
            y2_real = vmul(r1_real, Hp12_real, MASKREAD_OFF, schlength/2);
          
            vsetshamt(0);
            y2_real = vmul(y2_real, -1, MASKREAD_OFF, schlength/2);
            vsetshamt(7);
            temp    = vmul(r1_imag, Hp12_imag, MASKREAD_OFF, schlength/2);
            y2_real = vssub(temp, y2_real, MASKREAD_OFF, schlength/2);///注意顺序
            temp    = vmul(r2_real, Hp21_real, MASKREAD_OFF, schlength/2);
            y2_real = vsadd(y2_real, temp, MASKREAD_OFF, schlength/2);
            temp    = vmul(r2_imag, Hp21_imag, MASKREAD_OFF, schlength/2);
            y2_real = vsadd(y2_real, temp, MASKREAD_OFF, schlength/2);
          
            y2_imag = vmul(r1_imag, Hp12_real, MASKREAD_OFF, schlength/2);
            temp    = vmul(r1_real, Hp12_imag, MASKREAD_OFF, schlength/2);
            y2_imag = vssub(temp, y2_imag, MASKREAD_OFF, schlength/2);///注意顺序
            temp    = vmul(r2_real, Hp21_imag, MASKREAD_OFF, schlength/2);
            y2_imag = vssub(temp, y2_imag, MASKREAD_OFF, schlength/2);///注意顺序
            temp    = vmul(r2_imag, Hp21_real, MASKREAD_OFF, schlength/2);
            y2_imag = vsadd(y2_imag, temp, MASKREAD_OFF, schlength/2);
          
            vsetshamt(0);
          
            vsetshamt(7);
            y1_real = vmul(y1_real, 90, MASKREAD_OFF, schlength/2);//根号2
            y1_imag = vmul(y1_imag, 90, MASKREAD_OFF, schlength/2);
            y2_real = vmul(y2_real, 90, MASKREAD_OFF, schlength/2);
            y2_imag = vmul(y2_imag, 90, MASKREAD_OFF, schlength/2);
            vsetshamt(0);

            /*----------------------------t1----------------------------------*/
            __v4096i8 t1;
            vclaim(t1);
            vsetshamt(8);
            t1   = vmul(Hp11_real, Hp11_real, MASKREAD_OFF, schlength/2);
            temp = vmul(Hp11_imag, Hp11_imag, MASKREAD_OFF, schlength/2);
            t1   = vsadd(t1, temp, MASKREAD_OFF, schlength/2);
            temp = vmul(Hp22_real, Hp22_real, MASKREAD_OFF, schlength/2);
            t1   = vsadd(t1, temp, MASKREAD_OFF, schlength/2);
            temp = vmul(Hp22_imag, Hp22_imag, MASKREAD_OFF, schlength/2);
            t1   = vsadd(t1, temp, MASKREAD_OFF, schlength/2);
            
            /*----------------------------t2----------------------------------*/
            __v4096i8 t2;
            vclaim(t2);
            t2   = vmul(Hp12_real, Hp12_real, MASKREAD_OFF, schlength/2);
            temp = vmul(Hp12_imag, Hp12_imag, MASKREAD_OFF, schlength/2);
            t2   = vsadd(t2, temp, MASKREAD_OFF, schlength/2);
            temp = vmul(Hp21_real, Hp21_real, MASKREAD_OFF, schlength/2);
            t2   = vsadd(t2, temp, MASKREAD_OFF, schlength/2);
            temp = vmul(Hp21_imag, Hp21_imag, MASKREAD_OFF, schlength/2);
            t2   = vsadd(t2, temp, MASKREAD_OFF, schlength/2);
            vsetshamt(0);
          
            vsetshamt(6);
            y1_real = vdiv(t1 ,y1_real, MASKREAD_OFF, schlength/2);
            y1_imag = vdiv(t1 ,y1_imag, MASKREAD_OFF, schlength/2);
            y2_real = vdiv(t2 ,y2_real, MASKREAD_OFF, schlength/2);
            y2_imag = vdiv(t2 ,y2_imag, MASKREAD_OFF, schlength/2);
            vsetshamt(0);
            

            /*----------------------------out and csi----------------------------------*/  
            vshuffle(symCombining_real, index_240e, y1_real, SHUFFLE_SCATTER, schlength/2);
            vshuffle(symCombining_real, index_240o, y2_real, SHUFFLE_SCATTER, schlength/2); 
            vshuffle(csi, index_240e, t1, SHUFFLE_SCATTER, schlength/2);
            vshuffle(csi, index_240o, t2, SHUFFLE_SCATTER, schlength/2); 
          
            vshuffle(symCombining_imag, index_240e, y1_imag, SHUFFLE_SCATTER, schlength/2);
            vshuffle(symCombining_imag, index_240o, y2_imag, SHUFFLE_SCATTER, schlength/2); 
        }
         
            


        /*************************** 4天线 *****************************/    
        else if(Nt == 4){
            /*----------------------------hestnew----------------------------------*/
            __v4096i8 hestnew_real_1;
            __v4096i8 hestnew_imag_1;
            __v4096i8 hestnew_real_2;
            __v4096i8 hestnew_imag_2;
            __v4096i8 temp;
            __v4096i8 tempback;
            __v2048i16 indexback;
            __v2048i16 index2367;
            vclaim(hestnew_real_1);
            vclaim(hestnew_imag_1);
            vclaim(hestnew_real_2);
            vclaim(hestnew_imag_2);
            vclaim(temp); 
            vclaim(tempback);
            vclaim(indexback); 
            vclaim(index2367);
        
            index2367 = vadd(index0145,2,MASKREAD_OFF,schlength/2);
        
            vbrdcst(hestnew_real_1,0,MASKREAD_OFF,schlength);
            vshuffle(hestnew_real_1, index0145, hest_port0_real, SHUFFLE_GATHER, schlength/2);
            vshuffle(temp, index2367, hest_port1_real, SHUFFLE_GATHER, 120);
            vrange(indexback,schlength/2);
            indexback = vsadd(indexback,schlength/2,MASKREAD_OFF,schlength/2);
            vbrdcst(tempback,0,MASKREAD_OFF,schlength);
            vshuffle(tempback,indexback,temp,SHUFFLE_SCATTER,schlength);
            hestnew_real_1 = vsadd(tempback,hestnew_real_1,MASKREAD_OFF,schlength);
        
            vbrdcst(hestnew_real_2,0,MASKREAD_OFF,schlength);
            vshuffle(hestnew_real_2, index0145, hest_port2_real, SHUFFLE_GATHER, schlength/2);
            vshuffle(temp, index2367, hest_port3_real, SHUFFLE_GATHER, schlength/2);
            vbrdcst(tempback,0,MASKREAD_OFF,schlength);
            vshuffle(tempback,indexback,temp,SHUFFLE_SCATTER,schlength);
            hestnew_real_2 = vsadd(tempback,hestnew_real_2,MASKREAD_OFF,schlength);
        
        
            vbrdcst(hestnew_imag_1,0,MASKREAD_OFF,schlength);
            vshuffle(hestnew_imag_1, index0145, hest_port0_imag, SHUFFLE_GATHER, schlength/2);
            vshuffle(temp, index2367, hest_port1_imag, SHUFFLE_GATHER, schlength/2);
            vbrdcst(tempback,0,MASKREAD_OFF,schlength);
            vshuffle(tempback,indexback,temp,SHUFFLE_SCATTER,schlength);
            hestnew_imag_1 = vsadd(tempback,hestnew_imag_1,MASKREAD_OFF,schlength);
        
            vbrdcst(hestnew_imag_2,0,MASKREAD_OFF,schlength);
            vshuffle(hestnew_imag_2, index0145, hest_port2_imag, SHUFFLE_GATHER, schlength/2);
            vshuffle(temp, index2367, hest_port3_imag, SHUFFLE_GATHER, schlength/2);
            vbrdcst(tempback,0,MASKREAD_OFF,schlength);
            vshuffle(tempback,indexback,temp,SHUFFLE_SCATTER,schlength);
            hestnew_imag_2 = vsadd(tempback,hestnew_imag_2,MASKREAD_OFF,schlength);
        
        
            /*----------------------------innew----------------------------------*/
            __v4096i8 innew_real;
            __v4096i8 innew_imag;
            vclaim(innew_real);
            vclaim(innew_imag);
        
            vbrdcst(innew_real,0,MASKREAD_OFF,schlength);
            vshuffle(innew_real, index0145, pdschSymbols_real, SHUFFLE_GATHER, schlength/2);
            vshuffle(temp, index2367, pdschSymbols_real, SHUFFLE_GATHER, schlength/2);
            vbrdcst(tempback,0,MASKREAD_OFF,schlength);
            vshuffle(tempback,indexback,temp,SHUFFLE_SCATTER,schlength);
            innew_real = vsadd(tempback,innew_real,MASKREAD_OFF,schlength);
        
            vbrdcst(innew_imag,0,MASKREAD_OFF,schlength);
            vshuffle(innew_imag, index0145, pdschSymbols_imag, SHUFFLE_GATHER, schlength/2);
            vshuffle(temp, index2367, pdschSymbols_imag, SHUFFLE_GATHER, schlength/2);
            vbrdcst(tempback,0,MASKREAD_OFF,schlength);
            vshuffle(tempback,indexback,temp,SHUFFLE_SCATTER,schlength);
            innew_imag = vsadd(tempback,innew_imag,MASKREAD_OFF,schlength);
        
        
            /*-----------------------index----------------------------*/
            __v2048i16 index_240e;
            __v2048i16 index_240o;
            vclaim(index_240e);
            vclaim(index_240o);
            vrange(index_240e, schlength/2);
            index_240e = vmul(index_240e, 2, MASKREAD_OFF, schlength/2);
            index_240o = vsadd(index_240e, 1, MASKREAD_OFF, schlength/2);
        
            /*------------------------hest----------------------------*/
            __v4096i8 Hp11_real;
            __v4096i8 Hp22_real;
            __v4096i8 Hp12_real;
            __v4096i8 Hp21_real;
            vclaim(Hp11_real);
            vclaim(Hp22_real);
            vclaim(Hp12_real);
            vclaim(Hp21_real);
            vshuffle(Hp11_real, index_240e, hestnew_real_1, SHUFFLE_GATHER, schlength/2);
            vshuffle(Hp22_real, index_240o, hestnew_real_2, SHUFFLE_GATHER, schlength/2);
            vshuffle(Hp12_real, index_240e, hestnew_real_2, SHUFFLE_GATHER, schlength/2);
            vshuffle(Hp21_real, index_240o, hestnew_real_1, SHUFFLE_GATHER, schlength/2);
        
            __v4096i8 Hp11_imag;
            __v4096i8 Hp22_imag;
            __v4096i8 Hp12_imag;
            __v4096i8 Hp21_imag;
            vclaim(Hp11_imag);
            vclaim(Hp22_imag);
            vclaim(Hp12_imag);
            vclaim(Hp21_imag);
            vshuffle(Hp11_imag, index_240e, hestnew_imag_1, SHUFFLE_GATHER, schlength/2);
            vshuffle(Hp22_imag, index_240o, hestnew_imag_2, SHUFFLE_GATHER, schlength/2);
            vshuffle(Hp12_imag, index_240e, hestnew_imag_2, SHUFFLE_GATHER, schlength/2);
            vshuffle(Hp21_imag, index_240o, hestnew_imag_1, SHUFFLE_GATHER, schlength/2);
        
            /*----------------------------in----------------------------------*/
            __v4096i8 r1_real;
            __v4096i8 r2_real;
            __v4096i8 y1_real;
            __v4096i8 y2_real;
            vclaim(r1_real);
            vclaim(r2_real);
            vclaim(y1_real);
            vclaim(y2_real);
            __v4096i8 r1_imag;
            __v4096i8 r2_imag;
            __v4096i8 y1_imag;
            __v4096i8 y2_imag;
            vclaim(r1_imag);
            vclaim(r2_imag);
            vclaim(y1_imag);
            vclaim(y2_imag); 
        
            vshuffle(r1_real, index_240e, innew_real, SHUFFLE_GATHER, schlength/2);
            vshuffle(r2_real, index_240o, innew_real, SHUFFLE_GATHER, schlength/2);
            vshuffle(r1_imag, index_240e, innew_imag, SHUFFLE_GATHER, schlength/2);
            vshuffle(r2_imag, index_240o, innew_imag, SHUFFLE_GATHER, schlength/2);
        
        
            //输入1Q6
            //y1为2Q5
            vbrdcst(temp,0,MASKREAD_OFF,schlength);
            /*----------------------------y1----------------------------------*/
            vsetshamt(7);
            y1_real = vmul(r1_real, Hp11_real, MASKREAD_OFF, schlength/2);
            temp = vmul(r1_imag, Hp11_imag, MASKREAD_OFF, schlength/2);
            y1_real = vsadd(y1_real, temp, MASKREAD_OFF, schlength/2);
            temp = vmul(r2_real, Hp22_real, MASKREAD_OFF, schlength/2);
            y1_real = vsadd(y1_real, temp, MASKREAD_OFF, schlength/2);
            temp = vmul(r2_imag, Hp22_imag, MASKREAD_OFF, schlength/2);
            y1_real = vsadd(y1_real, temp, MASKREAD_OFF, schlength/2);
        
            y1_imag = vmul(r1_imag, Hp11_real, MASKREAD_OFF, schlength/2);
            temp = vmul(r1_real, Hp11_imag, MASKREAD_OFF, schlength/2);
            y1_imag = vssub(temp, y1_imag, MASKREAD_OFF, schlength/2);///注意顺序
            temp = vmul(r2_real, Hp22_imag, MASKREAD_OFF, schlength/2);
            y1_imag = vsadd(y1_imag, temp, MASKREAD_OFF, schlength/2);
            temp = vmul(r2_imag, Hp22_real, MASKREAD_OFF, schlength/2);
            y1_imag = vssub(temp, y1_imag, MASKREAD_OFF, schlength/2);
        
            /*----------------------------y2----------------------------------*/
            y2_real = vmul(r1_real, Hp12_real, MASKREAD_OFF, schlength/2);
        
            vsetshamt(0);
            y2_real = vmul(y2_real, -1, MASKREAD_OFF, schlength/2);
            vsetshamt(7);
            temp = vmul(r1_imag, Hp12_imag, MASKREAD_OFF, schlength/2);
            y2_real = vssub(temp, y2_real, MASKREAD_OFF, schlength/2);///注意顺序
            temp = vmul(r2_real, Hp21_real, MASKREAD_OFF, schlength/2);
            y2_real = vsadd(y2_real, temp, MASKREAD_OFF, schlength/2);
            temp = vmul(r2_imag, Hp21_imag, MASKREAD_OFF, schlength/2);
            y2_real = vsadd(y2_real, temp, MASKREAD_OFF, schlength/2);
        
            y2_imag = vmul(r1_imag, Hp12_real, MASKREAD_OFF, schlength/2);
            temp = vmul(r1_real, Hp12_imag, MASKREAD_OFF, schlength/2);
            y2_imag = vssub(temp, y2_imag, MASKREAD_OFF, schlength/2);///注意顺序
            temp = vmul(r2_real, Hp21_imag, MASKREAD_OFF, schlength/2);
            y2_imag = vssub(temp, y2_imag, MASKREAD_OFF, schlength/2);///注意顺序
            temp = vmul(r2_imag, Hp21_real, MASKREAD_OFF, schlength/2);
            y2_imag = vsadd(y2_imag, temp, MASKREAD_OFF, schlength/2);
        
            vsetshamt(0);
        
            // 输出2Q5
            vsetshamt(6);
            y1_real = vmul(y1_real, 90, MASKREAD_OFF, schlength/2);//根号2
            y1_imag = vmul(y1_imag, 90, MASKREAD_OFF, schlength/2);
            y2_real = vmul(y2_real, 90, MASKREAD_OFF, schlength/2);
            y2_imag = vmul(y2_imag, 90, MASKREAD_OFF, schlength/2);
            vsetshamt(0);
        
            /*----------------------------t1----------------------------------*/
            __v4096i8 t1;
            vclaim(t1);
            vsetshamt(7);
            t1 = vmul(Hp11_real, Hp11_real, MASKREAD_OFF, schlength/2);
            temp = vmul(Hp11_imag, Hp11_imag, MASKREAD_OFF, schlength/2);
            t1 = vsadd(t1, temp, MASKREAD_OFF, schlength/2);
            temp = vmul(Hp22_real, Hp22_real, MASKREAD_OFF, schlength/2);
            t1 = vsadd(t1, temp, MASKREAD_OFF, schlength/2);
            temp = vmul(Hp22_imag, Hp22_imag, MASKREAD_OFF, schlength/2);
            t1 = vsadd(t1, temp, MASKREAD_OFF, schlength/2);
        
            /*----------------------------t2----------------------------------*/
            __v4096i8 t2;
            vclaim(t2);
            t2 = vmul(Hp12_real, Hp12_real, MASKREAD_OFF, schlength/2);
            temp = vmul(Hp12_imag, Hp12_imag, MASKREAD_OFF, schlength/2);
            t2 = vsadd(t2, temp, MASKREAD_OFF, schlength/2);
            temp = vmul(Hp21_real, Hp21_real, MASKREAD_OFF, schlength/2);
            t2 = vsadd(t2, temp, MASKREAD_OFF, schlength/2);
            temp = vmul(Hp21_imag, Hp21_imag, MASKREAD_OFF, schlength/2);
            t2 = vsadd(t2, temp, MASKREAD_OFF, schlength/2);
            vsetshamt(0);
        
            vsetshamt(5);
            y1_real = vdiv(t1 ,y1_real, MASKREAD_OFF, schlength/2);
            y1_imag = vdiv(t1 ,y1_imag, MASKREAD_OFF, schlength/2);
            y2_real = vdiv(t2 ,y2_real, MASKREAD_OFF, schlength/2);
            y2_imag = vdiv(t2 ,y2_imag, MASKREAD_OFF, schlength/2);
            vsetshamt(0);
        
        
            /*----------------------------out and csi----------------------------------*/
            __v2048i16 index159;
            vclaim(index159);
            index159 = vsadd(index048,1,MASKREAD_OFF,schlength/4);
            __v2048i16 index26_10;
            vclaim(index26_10);
            index26_10 = vsadd(index048,2,MASKREAD_OFF,schlength/4);
            __v2048i16 index37_11;
            vclaim(index37_11);
            index37_11 = vsadd(index048,3,MASKREAD_OFF,schlength/4);
            __v2048i16 indexfront;
            vclaim(indexfront);
            vrange(indexfront,schlength/4);
            indexfront = vsadd(indexfront,schlength/4,MASKREAD_OFF,schlength/4);
        
            vshuffle(symCombining_real,index048,y1_real,SHUFFLE_SCATTER,schlength/4);
            vshuffle(symCombining_real,index159,y2_real,SHUFFLE_SCATTER,schlength/4);
            vshuffle(temp,indexfront,y1_real,SHUFFLE_GATHER,schlength/4);
            vshuffle(symCombining_real,index26_10,temp,SHUFFLE_SCATTER,schlength/4);
            vshuffle(temp,indexfront,y2_real,SHUFFLE_GATHER,schlength/4);
            vshuffle(symCombining_real,index37_11,temp,SHUFFLE_SCATTER,schlength/4);
        
        
            vshuffle(symCombining_imag,index048,y1_imag,SHUFFLE_SCATTER,schlength/4);
            vshuffle(symCombining_imag,index159,y2_imag,SHUFFLE_SCATTER,schlength/4);
            vshuffle(temp,indexfront,y1_imag,SHUFFLE_GATHER,schlength/4);
            vshuffle(symCombining_imag,index26_10,temp,SHUFFLE_SCATTER,schlength/4);
            vshuffle(temp,indexfront,y2_imag,SHUFFLE_GATHER,schlength/4);
            vshuffle(symCombining_imag,index37_11,temp,SHUFFLE_SCATTER,schlength/4);
        
        
            vshuffle(csi,index048,t1,SHUFFLE_SCATTER,schlength/4);
            vshuffle(csi,index159,t2,SHUFFLE_SCATTER,schlength/4);
            vshuffle(temp,indexfront,t1,SHUFFLE_GATHER,schlength/4);
            vshuffle(csi,index26_10,temp,SHUFFLE_SCATTER,schlength/4);
            vshuffle(temp,indexfront,t2,SHUFFLE_GATHER,schlength/4);
            vshuffle(csi,index37_11,temp,SHUFFLE_SCATTER,schlength/4);
        
        
        } 
        else{
            printf("error",NULL);
        }

    }


    /******************************************************************************* */
    /************************** 非发射分集方案(单天线) *******************/
    else{
        // 忽略'CDD', 'SpatialMux', 'MultiUser'这些情况
        /*  % 单天线或UE特定传输方案（如Port0/Port5等）的MMSE均衡
            [pdschRx, csi] = lteEqualizeMMSE(pdschSymbols, hest, noiseEst);
            % 预解码（根据传输方案去除预编码）
            rxDeprecoded = lteDLDeprecode(enb, pdsch, pdschRx);

            % 层映射（将层映射到码字，支持空间复用等场景）
            symCombining = lteLayerDemap(pdsch, rxDeprecoded);
            csi = lteLayerDemap(pdsch, csi);  % 对信道估计进行同层映射处理
        */
        /*--------------------Equalization--------------------*/
        __v4096i8 csi_tmp1;
        __v4096i8 csi_tmp2;
        vclaim(csi_tmp1);
        vclaim(csi_tmp2);

        vsetshamt(fractionLength);
        csi_tmp1 = vmul(hest_port0_real, hest_port0_real, MASKREAD_OFF, schlength);
        csi_tmp2 = vmul(hest_port0_imag, hest_port0_imag, MASKREAD_OFF, schlength);
        vsetshamt(0);

        csi = vsadd(csi_tmp1, csi_tmp2, MASKREAD_OFF, schlength);
        csi = vsadd(csi, nVar, MASKREAD_OFF, schlength);


        __v4096i8 pbchEq_tmp1;
        __v4096i8 pbchEq_tmp2;
        __v4096i8 pbchEq_tmp3;
        __v4096i8 pbchEq_tmp4;
        vclaim(pbchEq_tmp1);
        vclaim(pbchEq_tmp2);
        vclaim(pbchEq_tmp3);
        vclaim(pbchEq_tmp4);

        vsetshamt(fractionLength);
        hest_port0_real = vdiv(csi, hest_port0_real, MASKREAD_OFF, schlength);
        hest_port0_imag = vdiv(csi, hest_port0_imag, MASKREAD_OFF, schlength);
        vsetshamt(0);

        vsetshamt(fractionLength);
        pbchEq_tmp1 = vmul(hest_port0_real, pdschSymbols_real, MASKREAD_OFF, schlength);
        pbchEq_tmp2 = vmul(hest_port0_imag, pdschSymbols_imag, MASKREAD_OFF, schlength);
        vsetshamt(0);
        symCombining_real = vsadd(pbchEq_tmp1, pbchEq_tmp2, MASKREAD_OFF, schlength);

        vsetshamt(fractionLength);
        pbchEq_tmp3 = vmul(hest_port0_real, pdschSymbols_imag, MASKREAD_OFF, schlength);
        pbchEq_tmp4 = vmul(hest_port0_imag, pdschSymbols_real, MASKREAD_OFF, schlength);
        vsetshamt(0);
        symCombining_imag = vssub(pbchEq_tmp4, pbchEq_tmp3, MASKREAD_OFF, schlength);




    }



    symCombining_real = vsadd(symCombining_real, 0, MASKREAD_OFF, schlength);
    symCombining_imag = vsadd(symCombining_imag, 0, MASKREAD_OFF, schlength);
    csi = vsadd(csi, 0, MASKREAD_OFF, schlength);

    // char word[20] = "deprecode finished";
    // printf("----------- %s -----------\n",&word);


    // vreturn(symCombining_real,sizeof(symCombining_real), symCombining_imag,sizeof(symCombining_imag), csi,sizeof(csi));
    // CSI is internal to equalization; this DAG has no CSI consumer.
    vreturn(symCombining_real,schlength, symCombining_imag,schlength);

}

