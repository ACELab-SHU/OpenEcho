// lte Turbo译码,采用max-log-MAP算法
/*
 * @input   soft_in 软输入
 * @input   alphain 交织序列
 * @output  hard_out 译码输出
 *
 * Created by wangqianli
*/
/* 拆分的Turbodecode 2：
    SISO2 计算似然比
*/

#include "venus.h"
#include <stdint.h>
#include "vmath.h"
#include "riscv_printf.h"


typedef short __v2048i16 __attribute__((ext_vector_type(5000)));        //index变量用__v2048i16
typedef char  __v4096i8 __attribute__((ext_vector_type(5000)));         //数据用4096i8
// typedef short __v2048i16 __attribute__((ext_vector_type(2048)));        //index变量用__v2048i16
// typedef char  __v6148i8 __attribute__((ext_vector_type(4096)));         //数据用4096i8


typedef short __v6148i16 __attribute__((ext_vector_type(6148)));        //index变量用__v2048i16
typedef char  __v6148i8 __attribute__((ext_vector_type(6148)));         //数据用4096i8




//打印向量
#define VENUS_PRINTVEC_CHAR(name, len)                                                                                 \
do {                                                                                                                 \
short array_##name[len];                                                                                           \
int   vecaddr_##name = vaddr(name);                                                                                \
vbarrier();                                                                                                        \
VSPM_OPEN();                                                                                                       \
for (int _____ = 0; _____ < len; _____++) {                                                                        \
    array_##name[_____] = *(volatile unsigned char *)(vecaddr_##name + _____);                                       \
    printf("%hd\n", &array_##name[_____]);                                                                           \
}                                                                                                                  \
VSPM_CLOSE();                                                                                                      \
} while (0)







typedef struct {
    short data;
} __attribute__((aligned(64))) short_struct;


int Task_lteTurboDecode_6(__v6148i8 softin2_x_B,__v6148i8 d1_B,__v6148i8 d2_B,
__v6148i8 a1_B,__v6148i8 a2_B,__v6148i8 a3_B,__v6148i8 a4_B,__v6148i8 a5_B,__v6148i8 a6_B,__v6148i8 a7_B,__v6148i8 a8_B,
__v6148i8 b1_B,__v6148i8 b2_B,__v6148i8 b3_B,__v6148i8 b4_B,__v6148i8 b5_B,__v6148i8 b6_B,__v6148i8 b7_B,__v6148i8 b8_B,
__v6148i8 a_p_B,__v6148i8 e_p_B,
short_struct trblklength){
   
    
    short reg  = 4;                         //寄存器个数
    short trblklen = trblklength.data;      //原始信息长度
    short N  = trblklen + 24;               //码块长度
    short B  = reg+N;   
    // printf("B = %hd\n",&B);    


    // // 以下保证参数传入正确
    // __v6148i8 temp1;
    // vclaim(temp1);
    // __v2048i16 tempindex;
    // vclaim(tempindex);
    // vrange(tempindex,B);
    // vshuffle(temp1,tempindex,d1_B,SHUFFLE_GATHER,B);

    // d1_B = vsadd(temp1,0,MASKREAD_OFF,B);
    

    a1_B = vsadd(a1_B,0,MASKREAD_OFF,B);
    a2_B = vsadd(a2_B,0,MASKREAD_OFF,B);
    a3_B = vsadd(a3_B,0,MASKREAD_OFF,B);
    a4_B = vsadd(a4_B,0,MASKREAD_OFF,B);
    a5_B = vsadd(a5_B,0,MASKREAD_OFF,B);
    a6_B = vsadd(a6_B,0,MASKREAD_OFF,B);
    a7_B = vsadd(a7_B,0,MASKREAD_OFF,B);
    a8_B = vsadd(a8_B,0,MASKREAD_OFF,B);


    b1_B = vsadd(b1_B,0,MASKREAD_OFF,B);
    b2_B = vsadd(b2_B,0,MASKREAD_OFF,B);
    b3_B = vsadd(b3_B,0,MASKREAD_OFF,B);
    b4_B = vsadd(b4_B,0,MASKREAD_OFF,B);
    b5_B = vsadd(b5_B,0,MASKREAD_OFF,B);
    b6_B = vsadd(b6_B,0,MASKREAD_OFF,B);
    b7_B = vsadd(b7_B,0,MASKREAD_OFF,B);
    b8_B = vsadd(b8_B,0,MASKREAD_OFF,B);


    d1_B = vsadd(d1_B,0,MASKREAD_OFF,B);
    d2_B = vsadd(d2_B,0,MASKREAD_OFF,B);
    softin2_x_B = vsadd(softin2_x_B,0,MASKREAD_OFF,B);

    a_p_B = vsadd(a_p_B,0,MASKREAD_OFF,B);
    e_p_B = vsadd(e_p_B,0,MASKREAD_OFF,B);






    /**************** 似然比 变量申明 *****************/
    __v6148i8 adb1_B;
    __v6148i8 adb2_B;
    __v6148i8 adb3_B;
    __v6148i8 adb4_B;
    __v6148i8 adb5_B;
    __v6148i8 adb6_B;
    __v6148i8 adb7_B;
    __v6148i8 adb8_B;
    vclaim(adb1_B);
    vclaim(adb2_B);
    vclaim(adb3_B);
    vclaim(adb4_B);
    vclaim(adb5_B);
    vclaim(adb6_B);
    vclaim(adb7_B);
    vclaim(adb8_B);

    __v6148i8 adb1_B_2;
    __v6148i8 adb2_B_2;
    __v6148i8 adb3_B_2;
    __v6148i8 adb4_B_2;
    __v6148i8 adb5_B_2;
    __v6148i8 adb6_B_2;
    __v6148i8 adb7_B_2;
    __v6148i8 adb8_B_2;
    vclaim(adb1_B_2);
    vclaim(adb2_B_2);
    vclaim(adb3_B_2);
    vclaim(adb4_B_2);
    vclaim(adb5_B_2);
    vclaim(adb6_B_2);
    vclaim(adb7_B_2);
    vclaim(adb8_B_2);

    __v6148i8 adbtemp1_B;
    __v6148i8 adbtemp2_B;
    vclaim(adbtemp1_B);
    vclaim(adbtemp2_B);

    __v6148i8 ltemp1_B;         //被减数
    __v6148i8 ltemp2_B;         //减数
    vclaim(ltemp1_B);
    vclaim(ltemp2_B);


    /****************  *****************/
    // __v6148i8 a_p_B;
    // __v6148i8 e_p_B;        //外信息
    // vclaim(a_p_B);
    // vclaim(e_p_B);

    __v6148i8 soft_out_B;   //子译码器软输出，非最终软输出
    vclaim(soft_out_B);

    /**************** 其他变量申明 *****************/
    __v6148i8 pos126_B;
    __v6148i8 pos64_B;
    vclaim(pos126_B);
    vclaim(pos64_B);

    __v6148i8 ones_B;
    vclaim(ones_B);








    /*********************求似然比************************ */
    // for (short k = B; k>0; k--){
    /*
        l(k)=max([...
                (a(1,k) +d3(k) +b(5,k+1)),...
                (a(2,k) +d3(k) +b(1,k+1)),...
                (a(3,k) +d4(k) +b(2,k+1)),...
                (a(4,k) +d4(k) +b(6,k+1)),...
                (a(5,k) +d4(k) +b(7,k+1)),...
                (a(6,k) +d4(k) +b(3,k+1)),...
                (a(7,k) +d3(k) +b(4,k+1)),...
                (a(8,k) +d3(k) +b(8,k+1))...
            ])-max([...
                (a(1,k) +d1(k) +b(1,k+1)),...
                (a(2,k) +d1(k) +b(5,k+1)),...
                (a(3,k) +d2(k) +b(6,k+1)),...
                (a(4,k) +d2(k) +b(2,k+1)),...
                (a(5,k) +d2(k) +b(3,k+1)),...
                (a(6,k) +d2(k) +b(7,k+1)),...
                (a(7,k) +d1(k) +b(8,k+1)),...
                (a(8,k) +d1(k) +b(4,k+1))...
            ]);
    */
    // }


    //step1:求m
    adb1_B = vadd(a1_B,d1_B,MASKREAD_OFF,B);
    adb1_B = vadd(adb1_B,b5_B,MASKREAD_OFF,B);
    adb2_B = vadd(a2_B,d1_B,MASKREAD_OFF,B);
    adb2_B = vadd(adb2_B,b1_B,MASKREAD_OFF,B);
    adb3_B = vadd(a3_B,d2_B,MASKREAD_OFF,B);
    adb3_B = vadd(adb3_B,b2_B,MASKREAD_OFF,B);
    adb4_B = vadd(a4_B,d2_B,MASKREAD_OFF,B);
    adb4_B = vadd(adb4_B,b6_B,MASKREAD_OFF,B);
    adb5_B = vadd(a5_B,d2_B,MASKREAD_OFF,B);
    adb5_B = vadd(adb5_B,b7_B,MASKREAD_OFF,B);
    adb6_B = vadd(a6_B,d2_B,MASKREAD_OFF,B);
    adb6_B = vadd(adb6_B,b3_B,MASKREAD_OFF,B);
    adb7_B = vadd(a7_B,d1_B,MASKREAD_OFF,B);
    adb7_B = vadd(adb7_B,b4_B,MASKREAD_OFF,B);
    adb8_B = vadd(a8_B,d1_B,MASKREAD_OFF,B);
    adb8_B = vadd(adb8_B,b8_B,MASKREAD_OFF,B);

    //step2:求n
    adb1_B_2 = vrsub(a1_B,d1_B,MASKREAD_OFF,B);
    adb1_B_2 = vadd(adb1_B_2,b1_B,MASKREAD_OFF,B);
    adb2_B_2 = vrsub(a2_B,d1_B,MASKREAD_OFF,B);
    adb2_B_2 = vadd(adb2_B_2,b5_B,MASKREAD_OFF,B);
    adb3_B_2 = vrsub(a3_B,d2_B,MASKREAD_OFF,B);
    adb3_B_2 = vadd(adb3_B_2,b6_B,MASKREAD_OFF,B);
    adb4_B_2 = vrsub(a4_B,d2_B,MASKREAD_OFF,B);
    adb4_B_2 = vadd(adb4_B_2,b2_B,MASKREAD_OFF,B);
    adb5_B_2 = vrsub(a5_B,d2_B,MASKREAD_OFF,B);
    adb5_B_2 = vadd(adb5_B_2,b3_B,MASKREAD_OFF,B);
    adb6_B_2 = vrsub(a6_B,d2_B,MASKREAD_OFF,B);
    adb6_B_2 = vadd(adb6_B_2,b7_B,MASKREAD_OFF,B);
    adb7_B_2 = vrsub(a7_B,d1_B,MASKREAD_OFF,B);
    adb7_B_2 = vadd(adb7_B_2,b8_B,MASKREAD_OFF,B);
    adb8_B_2 = vrsub(a8_B,d1_B,MASKREAD_OFF,B);
    adb8_B_2 = vadd(adb8_B_2,b4_B,MASKREAD_OFF,B);

    // 防上溢出，大于126则减去126
    vbrdcst(pos126_B,126,MASKREAD_OFF,B);
    vbrdcst(pos64_B,126,MASKREAD_OFF,B);
    adbtemp1_B = vslt(adb1_B,pos126_B,MASKREAD_OFF,MASKWRITE_OFF,B);      //adb1_B大于126
    adbtemp2_B = vslt(adb2_B,pos126_B,MASKREAD_OFF,MASKWRITE_OFF,B);      //adb2_B大于126
    adbtemp1_B = vor(adbtemp1_B,adbtemp2_B, MASKREAD_OFF,B);
    adbtemp2_B = vslt(adb3_B,pos126_B,MASKREAD_OFF,MASKWRITE_OFF,B);      //adb3_B大于126
    adbtemp1_B = vor(adbtemp1_B,adbtemp2_B, MASKREAD_OFF,B);
    adbtemp2_B = vslt(adb4_B,pos126_B,MASKREAD_OFF,MASKWRITE_OFF,B);      //adb4_B大于126
    adbtemp1_B = vor(adbtemp1_B,adbtemp2_B, MASKREAD_OFF,B);
    adbtemp2_B = vslt(adb5_B,pos126_B,MASKREAD_OFF,MASKWRITE_OFF,B);      //adb5_B大于126
    adbtemp1_B = vor(adbtemp1_B,adbtemp2_B, MASKREAD_OFF,B);
    adbtemp2_B = vslt(adb6_B,pos126_B,MASKREAD_OFF,MASKWRITE_OFF,B);      //adb6_B大于126
    adbtemp1_B = vor(adbtemp1_B,adbtemp2_B, MASKREAD_OFF,B);
    adbtemp2_B = vslt(adb7_B,pos126_B,MASKREAD_OFF,MASKWRITE_OFF,B);      //adb7_B大于126
    adbtemp1_B = vor(adbtemp1_B,adbtemp2_B, MASKREAD_OFF,B);
    adbtemp2_B = vslt(adb8_B,pos126_B,MASKREAD_OFF,MASKWRITE_OFF,B);      //adb8_B大于126
    adbtemp1_B = vor(adbtemp1_B,adbtemp2_B, MASKREAD_OFF,B);

    adbtemp2_B = vslt(adb1_B_2,pos126_B,MASKREAD_OFF,MASKWRITE_OFF,B);    //adb1_B_2大于126
    adbtemp1_B = vor(adbtemp1_B,adbtemp2_B, MASKREAD_OFF,B);
    adbtemp2_B = vslt(adb2_B_2,pos126_B,MASKREAD_OFF,MASKWRITE_OFF,B);    //adb2_B_2大于126
    adbtemp1_B = vor(adbtemp1_B,adbtemp2_B, MASKREAD_OFF,B);
    adbtemp2_B = vslt(adb3_B_2,pos126_B,MASKREAD_OFF,MASKWRITE_OFF,B);    //adb3_B_2大于126
    adbtemp1_B = vor(adbtemp1_B,adbtemp2_B, MASKREAD_OFF,B);
    adbtemp2_B = vslt(adb4_B_2,pos126_B,MASKREAD_OFF,MASKWRITE_OFF,B);    //adb4_B_2大于126
    adbtemp1_B = vor(adbtemp1_B,adbtemp2_B, MASKREAD_OFF,B);
    adbtemp2_B = vslt(adb5_B_2,pos126_B,MASKREAD_OFF,MASKWRITE_OFF,B);    //adb5_B_2大于126
    adbtemp1_B = vor(adbtemp1_B,adbtemp2_B, MASKREAD_OFF,B);
    adbtemp2_B = vslt(adb6_B_2,pos126_B,MASKREAD_OFF,MASKWRITE_OFF,B);    //adb6_B_2大于126
    adbtemp1_B = vor(adbtemp1_B,adbtemp2_B, MASKREAD_OFF,B);
    adbtemp2_B = vslt(adb7_B_2,pos126_B,MASKREAD_OFF,MASKWRITE_OFF,B);    //adb7_B_2大于126
    adbtemp1_B = vor(adbtemp1_B,adbtemp2_B, MASKREAD_OFF,B);
    adbtemp2_B = vslt(adb8_B_2,pos126_B,MASKREAD_OFF,MASKWRITE_OFF,B);    //adb8_B_2大于126
    adbtemp1_B = vor(adbtemp1_B,adbtemp2_B, MASKREAD_OFF,B);    //>126的位置标记为1

    vbrdcst(ones_B,1,MASKREAD_OFF,B);
    vseq(adbtemp1_B,ones_B,MASKREAD_OFF,MASKWRITE_ON,B);        //获得>126的位置的掩码
    // adbtemp2_B = vseq(adbtemp1_B,ones_B,MASKREAD_OFF,MASKWRITE_OFF,B);
    // adbtemp2_B = vsadd(adbtemp2_B,0, MASKREAD_OFF,B);
    adb1_B = vsub(adb1_B,pos64_B,MASKREAD_ON,B);
    adb2_B = vsub(adb2_B,pos64_B,MASKREAD_ON,B);
    adb3_B = vsub(adb3_B,pos64_B,MASKREAD_ON,B);
    adb4_B = vsub(adb4_B,pos64_B,MASKREAD_ON,B);
    adb5_B = vsub(adb5_B,pos64_B,MASKREAD_ON,B);
    adb6_B = vsub(adb6_B,pos64_B,MASKREAD_ON,B);
    adb7_B = vsub(adb7_B,pos64_B,MASKREAD_ON,B);
    adb8_B = vsub(adb8_B,pos64_B,MASKREAD_ON,B);
    adb1_B_2 = vsub(adb1_B_2,pos64_B,MASKREAD_ON,B);
    adb2_B_2 = vsub(adb2_B_2,pos64_B,MASKREAD_ON,B);
    adb3_B_2 = vsub(adb3_B_2,pos64_B,MASKREAD_ON,B);
    adb4_B_2 = vsub(adb4_B_2,pos64_B,MASKREAD_ON,B);
    adb5_B_2 = vsub(adb5_B_2,pos64_B,MASKREAD_ON,B);
    adb6_B_2 = vsub(adb6_B_2,pos64_B,MASKREAD_ON,B);
    adb7_B_2 = vsub(adb7_B_2,pos64_B,MASKREAD_ON,B);
    adb8_B_2 = vsub(adb8_B_2,pos64_B,MASKREAD_ON,B);

//         adb1_B = vrsub(adb1_B,0,MASKREAD_OFF,B);
//         adb2_B = vrsub(adb2_B,0,MASKREAD_OFF,B);
//         adb3_B = vrsub(adb3_B,0,MASKREAD_OFF,B);
//         adb4_B = vrsub(adb4_B,0,MASKREAD_OFF,B);
//         adb5_B = vrsub(adb5_B,0,MASKREAD_OFF,B);
//         adb6_B = vrsub(adb6_B,0,MASKREAD_OFF,B);
//         adb7_B = vrsub(adb7_B,0,MASKREAD_OFF,B);
//         adb8_B = vrsub(adb8_B,0,MASKREAD_OFF,B);


    // step3：求max，得到ltemp1
    vsgt(adb1_B,adb2_B,MASKREAD_OFF,MASKWRITE_ON,B);    //adb1_B小于等于adb2_B
    adb1_B = vxor(adb1_B, adb1_B, MASKREAD_ON, B);
    adb1_B = vsadd(adb1_B,adb2_B, MASKREAD_ON, B);
    vsgt(adb1_B,adb3_B,MASKREAD_OFF,MASKWRITE_ON,B);    //adb1_B小于等于adb3_B
    adb1_B = vxor(adb1_B, adb1_B, MASKREAD_ON, B);
    adb1_B = vsadd(adb1_B,adb3_B, MASKREAD_ON, B);
    vsgt(adb1_B,adb4_B,MASKREAD_OFF,MASKWRITE_ON,B);    //adb1_B小于等于adb4_B
    adb1_B = vxor(adb1_B, adb1_B, MASKREAD_ON, B);
    adb1_B = vsadd(adb1_B,adb4_B, MASKREAD_ON, B);
    vsgt(adb1_B,adb5_B,MASKREAD_OFF,MASKWRITE_ON,B);    //adb1_B小于等于adb5_B
    adb1_B = vxor(adb1_B, adb1_B, MASKREAD_ON, B);
    adb1_B = vsadd(adb1_B,adb5_B, MASKREAD_ON, B);
    vsgt(adb1_B,adb6_B,MASKREAD_OFF,MASKWRITE_ON,B);    //adb1_B小于等于adb6_B
    adb1_B = vxor(adb1_B, adb1_B, MASKREAD_ON, B);
    adb1_B = vsadd(adb1_B,adb6_B, MASKREAD_ON, B);
    vsgt(adb1_B,adb7_B,MASKREAD_OFF,MASKWRITE_ON,B);    //adb1_B小于等于adb7_B
    adb1_B = vxor(adb1_B, adb1_B, MASKREAD_ON, B);
    adb1_B = vsadd(adb1_B,adb7_B, MASKREAD_ON, B);
    vsgt(adb1_B,adb8_B,MASKREAD_OFF,MASKWRITE_ON,B);    //adb1_B小于等于adbB_B
    adb1_B = vxor(adb1_B, adb1_B, MASKREAD_ON, B);
    adb1_B = vsadd(adb1_B,adb8_B, MASKREAD_ON, B);
    ltemp1_B = vsadd(adb1_B,0, MASKREAD_OFF, B);


    // step4:求max，得到ltemp2
    vsgt(adb1_B_2,adb2_B_2,MASKREAD_OFF,MASKWRITE_ON,B);    //adb1_B小于等于adb2_B
    adb1_B_2 = vxor(adb1_B_2, adb1_B_2, MASKREAD_ON, B);
    adb1_B_2 = vsadd(adb1_B_2,adb2_B_2, MASKREAD_ON, B);
    vsgt(adb1_B_2,adb3_B_2,MASKREAD_OFF,MASKWRITE_ON,B);    //adb1_B小于等于adb3_B
    adb1_B_2 = vxor(adb1_B_2, adb1_B_2, MASKREAD_ON, B);
    adb1_B_2 = vsadd(adb1_B_2,adb3_B_2, MASKREAD_ON, B);
    vsgt(adb1_B_2,adb4_B_2,MASKREAD_OFF,MASKWRITE_ON,B);    //adb1_B小于等于adb4_B
    adb1_B_2 = vxor(adb1_B_2, adb1_B_2, MASKREAD_ON, B);
    adb1_B_2 = vsadd(adb1_B_2,adb4_B_2, MASKREAD_ON, B);
    vsgt(adb1_B_2,adb5_B_2,MASKREAD_OFF,MASKWRITE_ON,B);    //adb1_B小于等于adb5_B
    adb1_B_2 = vxor(adb1_B_2, adb1_B_2, MASKREAD_ON, B);
    adb1_B_2 = vsadd(adb1_B_2,adb5_B_2, MASKREAD_ON, B);
    vsgt(adb1_B_2,adb6_B_2,MASKREAD_OFF,MASKWRITE_ON,B);    //adb1_B小于等于adb6_B
    adb1_B_2 = vxor(adb1_B_2, adb1_B_2, MASKREAD_ON, B);
    adb1_B_2 = vsadd(adb1_B_2,adb6_B_2, MASKREAD_ON, B);
    vsgt(adb1_B_2,adb7_B_2,MASKREAD_OFF,MASKWRITE_ON,B);    //adb1_B小于等于adb7_B
    adb1_B_2 = vxor(adb1_B_2, adb1_B_2, MASKREAD_ON, B);
    adb1_B_2 = vsadd(adb1_B_2,adb7_B_2, MASKREAD_ON, B);
    vsgt(adb1_B_2,adb8_B_2,MASKREAD_OFF,MASKWRITE_ON,B);    //adb1_B小于等于adbB_B
    adb1_B_2 = vxor(adb1_B_2, adb1_B_2, MASKREAD_ON, B);
    adb1_B_2 = vsadd(adb1_B_2,adb8_B_2, MASKREAD_ON, B);
    ltemp2_B = vsadd(adb1_B_2,0, MASKREAD_OFF, B);
    // ltemp1_B = vsadd(adb1_B,0, MASKREAD_OFF, B);



    /*
        soft_out=l;
        ex_info=soft_out-app-x;
    */
    soft_out_B = vrsub(ltemp1_B,ltemp2_B,MASKREAD_OFF,B);
    e_p_B = vrsub(soft_out_B,softin2_x_B,MASKREAD_OFF,B);
    e_p_B = vrsub(e_p_B,a_p_B,MASKREAD_OFF,B);


    // softin2_x_B = vrsub(softin2_x_B,0,MASKREAD_OFF,B);
    // a_p_B = vrsub(a_p_B,0,MASKREAD_OFF,B);






    // char words[30] = "SISO2 llr computing finished";
    // printf("-------%s---------\n",&words);

    // __v6148i8 hard_out_N;
    // vclaim(hard_out_N);
    // vbrdcst(hard_out_N,0,MASKREAD_OFF,10);

    // vreturn(e_p_B,sizeof(e_p_B),soft_out_B,sizeof(soft_out_B));
    vreturn(e_p_B,B,soft_out_B,B);

}