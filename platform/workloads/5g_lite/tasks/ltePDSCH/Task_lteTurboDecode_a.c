// lte Turbo译码,采用max-log-MAP算法
/*
 * @input   soft_in 软输入
 * @input   alphain 交织序列
 * @output  hard_out 译码输出
 *
 * Created by wangqianli
*/
/* 拆分的Turbodecode a：
    SISO1:计算分支、前向
*/

#include "venus.h"
#include <stdint.h>
#include "vmath.h"
#include "riscv_printf.h"


// typedef short __v2048i16 __attribute__((ext_vector_type(3300)));        //index变量用__v2048i16
// typedef char  __v4096i8 __attribute__((ext_vector_type(6600)));         //数据用4096i8
typedef short __v2048i16 __attribute__((ext_vector_type(2000)));        //index变量用__v2048i16
typedef char  __v4096i8 __attribute__((ext_vector_type(2000)));         //数据用4096i8

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



int Task_lteTurboDecode_a(
    __v6148i8 softin1_x_B,__v6148i8 softin1_y_B ,__v4096i8 state0_8, __v6148i16 alphain_N,
    __v4096i8 a_add_d1_index,__v4096i8 a_add_d2_index,__v4096i8 a_add_d3_index,__v4096i8 a_add_d4_index,
    __v2048i16 atemp1_index_8,__v2048i16 atemp2_index_8,
    short_struct trblklength,short_struct SW_value
){



  short reg  = 4;                         //寄存器个数
  short trblklen = trblklength.data;      //原始信息长度
  short N  = trblklen + 24;               //码块长度
  short B  = reg+N;   
  printf("B = %hd\n",&B);    

  short ii = 3 ;                  //迭代次数
  short SW = SW_value.data;       //分的段数
  printf("trblklen = %hd\n",&trblklen);
  printf("SW = %hd\n",&SW);




  softin1_x_B = vsadd(softin1_x_B,0,MASKREAD_OFF,B);   
  softin1_y_B = vsadd(softin1_y_B,0,MASKREAD_OFF,B);    
  state0_8 = vsadd(state0_8,0,MASKREAD_OFF,8*SW);   
  alphain_N = vsadd(alphain_N,0,MASKREAD_OFF,N);     
  a_add_d1_index = vsadd(a_add_d1_index,0,MASKREAD_OFF,8);  
  a_add_d2_index = vsadd(a_add_d2_index,0,MASKREAD_OFF,8);  
  a_add_d3_index = vsadd(a_add_d3_index,0,MASKREAD_OFF,8);  
  a_add_d4_index = vsadd(a_add_d4_index,0,MASKREAD_OFF,8);  
  atemp1_index_8 = vsadd(atemp1_index_8,0,MASKREAD_OFF,8*SW);  
  atemp2_index_8 = vsadd(atemp2_index_8,0,MASKREAD_OFF,8*SW);  




  int d1;
  int d2;
  int addr0;
  int addr1;
  int addr2;
  int addr3;
  int addr4;
  int addr5;
  int addr6;
  int addr7;
  int tmp0;
  int tmp1;
  int tmp2;
  int tmp3;
  int tmp4;
  int tmp5;
  int tmp6;
  int tmp7;




  softin1_x_B = vsadd(softin1_x_B,0,MASKREAD_OFF,B);
  softin1_y_B = vsadd(softin1_y_B,0,MASKREAD_OFF,B);



  short kminus = B*10/SW;
  short kplus = 0;
  if(kminus%10 == 0){
      kminus = B/SW;
      kplus = kminus;
  }else{
      kminus = B/SW;
      kplus = kminus+1;
  }
  printf("kminus = %hd\n",&kminus);
  // printf("kplus = %hd\n",&kplus);




  // short half_length = 8*SW;
  // short length = half_length*2;
  short length = 8*SW;
  short half_length = length;
  // short length1= 512/SW;               //(8192/(8*SW*2))
  // // short length1= 256/SW;                  //(4096/(8*SW*2))
  // short length2= length1*SW;
  // short length3;
  // short length4;
  // short length5;
  // short length6;
  // short length7 = B-((SW-1)*kminus+1) +1 + 1;    
  short length7 = B-((SW-1)*kminus+1) +1 +1;     



  short j=0;





  /**************** 分支度量 变量申明 *****************/
  __v6148i8 d1_B;     //分支量度
  __v6148i8 d2_B;
  __v6148i8 d3_B;
  __v6148i8 d4_B;
  vclaim(d1_B);
  vclaim(d2_B);
  vclaim(d3_B);
  vclaim(d4_B);

  __v4096i8 d1_B_ksub1;
  __v4096i8 d2_B_ksub1;
  __v4096i8 d3_B_ksub1;
  __v4096i8 d4_B_ksub1;
  vclaim(d1_B_ksub1);
  vclaim(d2_B_ksub1);
  vclaim(d3_B_ksub1);
  vclaim(d4_B_ksub1);

  //加分支度量的位置
  // __v2048i16 a_add_index5;        //a_add_index1的备份
  // __v2048i16 a_add_index6;
  // __v2048i16 a_add_index7;
  // __v2048i16 a_add_index8;
  // vclaim(a_add_index6);
  // vclaim(a_add_index7);
  // vclaim(a_add_index8);
  // __v2048i16 b_add_index3;
  // vclaim(b_add_index3);


  // __v2048i16 ad_vseqindex;
  // __v2048i16 bd_vseqindex;
  // vclaim(ad_vseqindex);
  // vclaim(bd_vseqindex);


  /**************** 前向度量 变量申明 *****************/
  //前向度量
  __v4096i8 atemp;
  __v4096i8 atemp1_8;         //左边的和
  __v4096i8 atemp2_8;         //右边的和
  __v4096i8 atemp1_8temp;         //左边的和
  __v4096i8 atemp2_8temp;         //右边的和
  vclaim(atemp);
  vclaim(atemp1_8);
  vclaim(atemp2_8);
  vclaim(atemp1_8temp);
  vclaim(atemp2_8temp);


  //数据过长需要分段放，创建存放位置
  // __v4096i8 atemp1_8B;
  // __v4096i8 atemp2_8B;
  // __v4096i8 atemp3_8B;
  // __v4096i8 atemp4_8B;
  // __v4096i8 atemp5_8B;
  // __v4096i8 atemp6_8B;
  // __v4096i8 atemp7_8B;
  // vclaim(atemp1_8B);
  // vclaim(atemp2_8B);
  // vclaim(atemp3_8B);
  // vclaim(atemp4_8B);
  // vclaim(atemp5_8B);
  // vclaim(atemp6_8B);
  // vclaim(atemp7_8B);

  // __v2048i16 atemp_index1_8;
  // __v2048i16 atemp_index2_8;
  // __v2048i16 atemp1_index_8;              //加分支度量前，需要变换前向度量的顺序
  // __v2048i16 atemp2_index_8;              //加分支度量前，需要变换前向度量的顺序
  // vclaim(atemp1_index_8);
  // vclaim(atemp2_index_8);

  __v6148i8 a1_B;         //前向度量
  __v6148i8 a2_B;
  __v6148i8 a3_B;
  __v6148i8 a4_B;
  __v6148i8 a5_B;
  __v6148i8 a6_B;
  __v6148i8 a7_B;
  __v6148i8 a8_B;
  
  vclaim(a1_B);
  vclaim(a2_B);
  vclaim(a3_B);
  vclaim(a4_B);
  vclaim(a5_B);
  vclaim(a6_B);
  vclaim(a7_B);
  vclaim(a8_B);

  __v6148i8 a_B_temp; 
  vclaim(a_B_temp);
  __v6148i16 a_B_tempindex; 
  vclaim(a_B_tempindex);

  // /**************** 后向度量 变量申明 *****************/
  // __v4096i8 b1_reverse_B;         //b1_B的倒序
  // __v4096i8 b2_reverse_B;
  // __v4096i8 b3_reverse_B;
  // __v4096i8 b4_reverse_B;
  // __v4096i8 b5_reverse_B;
  // __v4096i8 b6_reverse_B;
  // __v4096i8 b7_reverse_B;
  // __v4096i8 b8_reverse_B;
  // vclaim(b1_reverse_B);
  // vclaim(b2_reverse_B);
  // vclaim(b3_reverse_B);
  // vclaim(b4_reverse_B);
  // vclaim(b5_reverse_B);
  // vclaim(b6_reverse_B);
  // vclaim(b7_reverse_B);
  // vclaim(b8_reverse_B);

  // __v6148i8 b1_B;     //后向度量
  // __v6148i8 b2_B;
  // __v6148i8 b3_B;
  // __v6148i8 b4_B;
  // __v6148i8 b5_B;
  // __v6148i8 b6_B;
  // __v6148i8 b7_B;
  // __v6148i8 b8_B;
  // vclaim(b1_B);
  // vclaim(b2_B);
  // vclaim(b3_B);
  // vclaim(b4_B);
  // vclaim(b5_B);
  // vclaim(b6_B);
  // vclaim(b7_B);
  // vclaim(b8_B);



  // /**************** 拆分index 变量申明 *****************/
  // __v2048i16 split_index1;
  // __v2048i16 split_index2;
  // __v2048i16 split_index3;
  // __v2048i16 split_index4;
  // __v2048i16 split_index5;
  // __v2048i16 split_index6;
  // __v2048i16 split_index7;
  // __v2048i16 split_index8;
  // vclaim(split_index1);
  // vclaim(split_index2);
  // vclaim(split_index3);
  // vclaim(split_index4);
  // vclaim(split_index5);
  // vclaim(split_index6);
  // vclaim(split_index7);
  // vclaim(split_index8);

  // /**************** 似然比 变量申明 *****************/
  // __v4096i8 adb1_B;
  // __v4096i8 adb2_B;
  // __v4096i8 adb3_B;
  // __v4096i8 adb4_B;
  // __v4096i8 adb5_B;
  // __v4096i8 adb6_B;
  // __v4096i8 adb7_B;
  // __v4096i8 adb8_B;
  // vclaim(adb1_B);
  // vclaim(adb2_B);
  // vclaim(adb3_B);
  // vclaim(adb4_B);
  // vclaim(adb5_B);
  // vclaim(adb6_B);
  // vclaim(adb7_B);
  // vclaim(adb8_B);

  // __v4096i8 adb1_B_2;
  // __v4096i8 adb2_B_2;
  // __v4096i8 adb3_B_2;
  // __v4096i8 adb4_B_2;
  // __v4096i8 adb5_B_2;
  // __v4096i8 adb6_B_2;
  // __v4096i8 adb7_B_2;
  // __v4096i8 adb8_B_2;
  // vclaim(adb1_B_2);
  // vclaim(adb2_B_2);
  // vclaim(adb3_B_2);
  // vclaim(adb4_B_2);
  // vclaim(adb5_B_2);
  // vclaim(adb6_B_2);
  // vclaim(adb7_B_2);
  // vclaim(adb8_B_2);

  // __v4096i8 adbtemp1_B;
  // __v4096i8 adbtemp2_B;
  // vclaim(adbtemp1_B);
  // vclaim(adbtemp2_B);

  // __v4096i8 ltemp1_B;         //被减数
  // __v4096i8 ltemp2_B;         //减数
  // vclaim(ltemp1_B);
  // vclaim(ltemp2_B);



  /**************** 其他变量申明 *****************/
  __v6148i8 a_p_B;
  __v6148i8 e_p_B;
  vclaim(a_p_B);
  vclaim(e_p_B);


  __v2048i16 SW_tempindex;
  __v2048i16 SW_tempindex2;
  __v2048i16 SW_tempindex3;
  __v2048i16 SW_tempindex4;
  __v2048i16 SW_tempindex5;
  __v2048i16 SW_tempindex6;
  __v2048i16 SW_tempindex7;
  // __v2048i16 SW_tempindex8;
  // __v2048i16 SW_tempindex9;
  // __v2048i16 SW_tempindex10;
  // __v2048i16 SW_tempindex11;
  // __v2048i16 SW_tempindex12;
  // __v2048i16 SW_tempindex13;
  // __v2048i16 SW_tempindex14;
  // __v2048i16 SW_tempindex15;
  // __v2048i16 SW_tempindex16;
  // __v2048i16 SW_tempindex17;
  vclaim(SW_tempindex);
  vclaim(SW_tempindex2);
  vclaim(SW_tempindex3);
  vclaim(SW_tempindex4);
  vclaim(SW_tempindex5);
  vclaim(SW_tempindex6);
  vclaim(SW_tempindex7);
  // vclaim(SW_tempindex8);
  // vclaim(SW_tempindex9);
  // vclaim(SW_tempindex10);
  // vclaim(SW_tempindex11);
  // vclaim(SW_tempindex12);
  // vclaim(SW_tempindex13);
  // vclaim(SW_tempindex14);
  // vclaim(SW_tempindex15);
  // vclaim(SW_tempindex16);
  // vclaim(SW_tempindex17);

  __v4096i8 SW_temp;
  __v4096i8 SW_temp2;
  // __v6148i8 SW_temp3;
  // __v4096i8 SW_temp4;
  // __v4096i8 SW_temp5;
  // __v4096i8 SW_temp6;
  // __v4096i8 SW_temp7;
  // __v4096i8 SW_temp8;
  vclaim(SW_temp);
  vclaim(SW_temp2);
  // vclaim(SW_temp3);
  // vclaim(SW_temp4);
  // vclaim(SW_temp5);
  // vclaim(SW_temp6);
  // vclaim(SW_temp7);
  // vclaim(SW_temp8);


  __v4096i8 pos64_8;      //阈值，2^{量化字长-2}=64，防上溢
  vclaim(pos64_8);

  __v4096i8 pos126_B;
  __v4096i8 pos64_B;
  vclaim(pos126_B);
  vclaim(pos64_B);

  __v4096i8 ones_B;
  vclaim(ones_B);

  __v4096i8 zero_8;
  vclaim(zero_8);
  __v2048i16 ones_8;
  __v4096i8 ones;
  __v2048i16 negones;;
  __v2048i16 zeros_8;
  __v2048i16 zeros;
  vclaim(ones_8);
  vclaim(ones);
  vclaim(negones);
  vclaim(zeros_8);
  vclaim(zeros);


  __v4096i8 m;
  __v4096i8 n;
  vclaim(m);
  vclaim(n);


  /**************************************************************** */
  /*************************************************************** */
  vbrdcst(zero_8,0,MASKREAD_OFF,8);
  vbrdcst(ones_8,1,MASKREAD_OFF,length);
  vbrdcst(ones,1,MASKREAD_OFF,8);
  vbrdcst(negones,-1,MASKREAD_OFF,length);
  vbrdcst(zeros,0,MASKREAD_OFF,8);
  vbrdcst(zeros_8,0,MASKREAD_OFF,length);
  vbrdcst(pos64_8,64,MASKREAD_OFF,length);
  vbrdcst(e_p_B,0,MASKREAD_OFF,B);




  /****************** 缩放数据 *******************/
  // softin1_x_B = vsra(softin1_x_B,3,MASKREAD_OFF,B);
  // softin1_y_B = vsra(softin1_y_B,3,MASKREAD_OFF,B);
  // softin2_x_B = vsra(softin2_x_B,3,MASKREAD_OFF,B);
  // softin2_y_B = vsra(softin2_y_B,3,MASKREAD_OFF,B);



  /******************************************************************************** */
  /******************************* SISO1 ************************************** */
  /*
      a_p(alphain)=e_p(1:L_seq-m);    %解交织
      a_p(L_seq-m+1:L_seq)=0;         %尾比特部分不计算外部信息
  */

  // vbrdcst(a_p_B,0,MASKREAD_OFF,4096);
  vbrdcst(a_p_B,0,MASKREAD_OFF,B);
  vshuffle(a_p_B,alphain_N,e_p_B,SHUFFLE_SCATTER,N);


  /***********************计算分支度量*********************** */
  /*
      d1(k) = -floor(0.75*(priori(k)+x(k)+y(k)));
      d2(k) = -floor(0.75*(priori(k)+x(k)-y(k)));
      d3(k) =  floor(0.75*(priori(k)+x(k)+y(k)));
      d4(k) =  floor(0.75*(priori(k)+x(k)-y(k)));
  */
  //初始化
  // vbrdcst(d1_B,0,MASKREAD_OFF,B);
  // vbrdcst(d2_B,0,MASKREAD_OFF,B);

  // softin1_x_B = vsra(softin1_x_B,2,MASKREAD_OFF,B);           //缩放数据
  // softin1_y_B = vsra(softin1_y_B,2,MASKREAD_OFF,B);

  d1_B = vsadd(softin1_x_B,softin1_y_B,MASKREAD_OFF,B);
  d1_B = vsadd(d1_B,a_p_B,MASKREAD_OFF,B);
  d1_B = vsra(d1_B,1,MASKREAD_OFF,B);
  d1_B = vmul(d1_B,3,MASKREAD_OFF,B);
  d1_B = vsra(d1_B,1,MASKREAD_OFF,B);
  //d3_B=d1_B; d1_B使用时用减号

  d2_B = vrsub(softin1_x_B,softin1_y_B,MASKREAD_OFF,B);
  d2_B = vsadd(d2_B,a_p_B,MASKREAD_OFF,B);
  d2_B = vsra(d2_B,1,MASKREAD_OFF,B);
  d2_B = vmul(d2_B,3,MASKREAD_OFF,B);
  d2_B = vsra(d2_B,1,MASKREAD_OFF,B);
  //d4_B=d2_B; d2_B使用时用减号


  softin1_x_B = vsadd(softin1_x_B,0,MASKREAD_OFF,B);
  softin1_y_B = vsadd(softin1_y_B,0,MASKREAD_OFF,B);
  // softin2_x_B = vsadd(softin2_x_B,0,MASKREAD_OFF,4096);
  // softin2_y_B = vsadd(softin2_y_B,0,MASKREAD_OFF,4096);

  // vrange(SW_tempindex2,B);
  // vshuffle(SW_temp2,SW_tempindex2,d1_B,SHUFFLE_GATHER,B);
  // vbrdcst(d1_B,0,MASKREAD_OFF,B);
  // vshuffle(d1_B,SW_tempindex2,SW_temp2,SHUFFLE_GATHER,B);
  // vshuffle(SW_temp2,SW_tempindex2,d2_B,SHUFFLE_GATHER,B);
  // vbrdcst(d2_B,0,MASKREAD_OFF,B);
  // vshuffle(d2_B,SW_tempindex2,SW_temp2,SHUFFLE_GATHER,B);



  d1_B = vsadd(d1_B, 0,MASKREAD_OFF,B);       //
  d2_B = vsadd(d2_B, 0,MASKREAD_OFF,B);  



  /***********************计算前向后向度量*********************** */
  // //计算前向后向时，分支度量的index会被更新，所以在开头要重新初始化
  // a_add_index1 = vsadd(a_add_index5, 0,MASKREAD_OFF,length);  //
  // a_add_index2 = vsadd(a_add_index6, 0,MASKREAD_OFF,length);
  // a_add_index3 = vsadd(a_add_index7, 0,MASKREAD_OFF,length);
  // a_add_index4 = vsadd(a_add_index8, 0,MASKREAD_OFF,length);  //


  atemp = vsadd(state0_8, 0,MASKREAD_OFF,length);
  vrange(SW_tempindex2,length);         //用于把计算出来的值搬到atemp_8B上: 0,1,2,...,length-1
  vrange(a_B_tempindex,length);  

  // vbrdcst(d1_B_ksub1,0,MASKREAD_OFF,length);
  // vbrdcst(d2_B_ksub1,0,MASKREAD_OFF,length);
  // vbrdcst(d3_B_ksub1,0,MASKREAD_OFF,length);
  // vbrdcst(d4_B_ksub1,0,MASKREAD_OFF,length);

  
  vbrdcst(a2_B,0,MASKREAD_OFF,B);
  vbrdcst(a1_B,176,MASKREAD_OFF,B);
  vshuffle(a1_B,a_B_tempindex,a2_B,SHUFFLE_GATHER,1);

  vbrdcst(a2_B,176,MASKREAD_OFF,B);
  vbrdcst(a3_B,176,MASKREAD_OFF,B);
  vbrdcst(a4_B,176,MASKREAD_OFF,B);
  vbrdcst(a5_B,176,MASKREAD_OFF,B);
  vbrdcst(a6_B,176,MASKREAD_OFF,B);
  vbrdcst(a7_B,176,MASKREAD_OFF,B);
  vbrdcst(a8_B,176,MASKREAD_OFF,B);

  

  for(short k=1; k<length7;k++){
  // for(short k=1; k<11;k++){

    /*  if k>1
        a(1,k)=max((a(1,k-1)+d1(k-1)),  (a(2,k-1)+d3(k-1))  );
            a(2,k)=max((a(3,k-1)+d4(k-1)),  (a(4,k-1)+d2(k-1))  );
            a(3,k)=max((a(5,k-1)+d2(k-1)),  (a(6,k-1)+d4(k-1))  );
            a(4,k)=max((a(7,k-1)+d3(k-1)),  (a(8,k-1)+d1(k-1))  );
            a(5,k)=max((a(1,k-1)+d3(k-1)),  (a(2,k-1)+d1(k-1))  );
            a(6,k)=max((a(3,k-1)+d2(k-1)),  (a(4,k-1)+d4(k-1))  );
            a(7,k)=max((a(5,k-1)+d4(k-1)),  (a(6,k-1)+d2(k-1))  );
        a(8,k)=max((a(7,k-1)+d1(k-1)),  (a(8,k-1)+d3(k-1))  );
    */
    /*
    for k=in_length:-1:1
        b(1,k)=max((b(1,k+1)+d1(k)),    (b(5,k+1)+d3(k))    );
        b(2,k)=max((b(5,k+1)+d1(k)),    (b(1,k+1)+d3(k))    );
            b(3,k)=max((b(6,k+1)+d2(k)),    (b(2,k+1)+d4(k))    );
            b(4,k)=max((b(2,k+1)+d2(k)),    (b(6,k+1)+d4(k))    );
            b(5,k)=max((b(3,k+1)+d2(k)),    (b(7,k+1)+d4(k))    );
            b(6,k)=max((b(7,k+1)+d2(k)),    (b(3,k+1)+d4(k))    );
        b(7,k)=max((b(8,k+1)+d1(k)),    (b(4,k+1)+d3(k))    );
        b(8,k)=max((b(4,k+1)+d1(k)),    (b(8,k+1)+d3(k))    );
    end
    */

    //step1:改变a[k-1]的顺序
    // atemp = vsadd(atemp, 0,MASKREAD_OFF,length);  //
    // atemp1_index_8 = vsadd(atemp1_index_8, 0,MASKREAD_OFF,length); 
    // atemp2_index_8 = vsadd(atemp2_index_8, 0,MASKREAD_OFF,length); 
    vshuffle(atemp1_8,atemp1_index_8,atemp,SHUFFLE_GATHER,length);  //
    // atemp1_8 = vsadd(atemp1_8, 0,MASKREAD_OFF,length);  //
    vshuffle(atemp2_8,atemp2_index_8,atemp,SHUFFLE_GATHER,length);  //
    // atemp2_8 = vsadd(atemp2_8, 0,MASKREAD_OFF,length);  //



    // //step2:计算atemp1和atemp2 (用vsadd防止下溢出)
    //---------- 串行版本
    vrange(SW_tempindex,8); 
    
    for(int i=0; i<SW; i++){
      int addr1 = vaddr(d1_B) +k-1 + i*kminus;
      int addr2 = vaddr(d2_B) +k-1 + i*kminus;
      vbarrier();
      VSPM_OPEN();  
      d1 = *(volatile unsigned char *) (addr1);
      d2 = *(volatile unsigned char *) (addr2);
      VSPM_CLOSE();
      // int m =k-1 + i*kminus;
      // printf("k-1 + i*kminus = %d\t",&m);
      // printf("d1 = %d\t",&d1);
      // printf("d2 = %d\n",&d2);
      vbrdcst(d1_B_ksub1,(-1)*d1,MASKREAD_OFF,8); //75   52 
      vbrdcst(d2_B_ksub1,(-1)*d2,MASKREAD_OFF,8); //76   53
      vbrdcst(d3_B_ksub1,d1,MASKREAD_OFF,8);      //77   54
      vbrdcst(d4_B_ksub1,d2,MASKREAD_OFF,8);      //78   55
      
      vshuffle(atemp1_8temp,SW_tempindex,atemp1_8,SHUFFLE_GATHER,8);  //79  56
      vshuffle(atemp2_8temp,SW_tempindex,atemp2_8,SHUFFLE_GATHER,8);  //80  57

      atemp1_8temp = vsadd(atemp1_8temp,0,MASKREAD_OFF,8);    //81  58
      atemp2_8temp = vsadd(atemp2_8temp,0,MASKREAD_OFF,8);    //82  59

      vsgt(a_add_d1_index,ones,MASKREAD_OFF,MASKWRITE_ON,8);        //83  60
      atemp1_8temp = vsadd(atemp1_8temp,d1_B_ksub1,MASKREAD_ON,8);  //84  61
      atemp2_8temp = vsadd(atemp2_8temp,d3_B_ksub1,MASKREAD_ON,8);  //85  62

      // atemp1_8temp = vsadd(atemp1_8temp,0,MASKREAD_OFF,8); 
      // atemp2_8temp = vsadd(atemp2_8temp,0,MASKREAD_OFF,8); 

      vsgt(a_add_d2_index,ones,MASKREAD_OFF,MASKWRITE_ON,8);        //86  63
      atemp1_8temp = vsadd(atemp1_8temp,d2_B_ksub1,MASKREAD_ON,8);  //87  64
      atemp2_8temp = vsadd(atemp2_8temp,d4_B_ksub1,MASKREAD_ON,8);  //88  65

      vsgt(a_add_d3_index,ones,MASKREAD_OFF,MASKWRITE_ON,8);        //89  66
      atemp1_8temp = vsadd(atemp1_8temp,d3_B_ksub1,MASKREAD_ON,8);  //90  67
      atemp2_8temp = vsadd(atemp2_8temp,d1_B_ksub1,MASKREAD_ON,8);  //91  68

      vsgt(a_add_d4_index,ones,MASKREAD_OFF,MASKWRITE_ON,8);        //92  69
      atemp1_8temp = vsadd(atemp1_8temp,d4_B_ksub1,MASKREAD_ON,8);  //93  70
      atemp2_8temp = vsadd(atemp2_8temp,d2_B_ksub1,MASKREAD_ON,8);  //94  71

      vshuffle(atemp1_8,SW_tempindex,atemp1_8temp,SHUFFLE_SCATTER,8);   //95  72
      vshuffle(atemp2_8,SW_tempindex,atemp2_8temp,SHUFFLE_SCATTER,8);   //96  73 189

      
      SW_tempindex = vsadd(SW_tempindex,8,MASKREAD_OFF,8);  //97  74 190
    }
    atemp1_8 = vsadd(atemp1_8,0,MASKREAD_OFF,length);   //98 191
    atemp2_8 = vsadd(atemp2_8,0,MASKREAD_OFF,length);   //99 192
      


    //step3:max(atemp1_8,atemp2_8),作为a[k]
    vsgt(atemp1_8,atemp2_8,MASKREAD_OFF,MASKWRITE_ON,length);    //atemp1_8小于atemp2_8 100 193
    // vsgt(atemp2_8,atemp1_8,MASKREAD_OFF,MASKWRITE_ON,length);    //atemp1_8小于等于atemp2_8？？反的？？？？？
    // SW_temp2 = vsgt(atemp1_8,atemp2_8,MASKREAD_OFF,MASKWRITE_OFF,length); 
    atemp1_8 = vxor(atemp1_8, atemp1_8, MASKREAD_ON, length);     // 101 194  1047 938
    atemp1_8 = vsadd(atemp1_8,0, MASKREAD_OFF, length);   //102 195 1048
    // atemp1_8 = vsadd(atemp1_8, atemp2_8, MASKREAD_ON, length);   //得到a[k]
    atemp1_8 = vsadd(atemp1_8,atemp2_8, MASKREAD_ON, length);   //得到a[k] 103 196 1049
    atemp = vsadd(atemp1_8,0, MASKREAD_OFF, length);  //104 197 1050 

    //step4:防上溢出,大于64则减去64
    // m = vslt(atemp1_8,pos64_8,MASKREAD_OFF,MASKWRITE_OFF,length);     //atemp1_8大于64
    m = vsle(pos64_8,atemp1_8,MASKREAD_OFF,MASKWRITE_OFF,length);     //atemp1_8小于等于64的位置为1  105 198
    n = vredmin8(m,MASKREAD_OFF,length);  //106 199
    int t =0;
    vbarrier();
    VSPM_OPEN();
      t = *(volatile unsigned char *) (vaddr(n));
    VSPM_CLOSE();
    if(t ==0){     //若m中的最小值为0，表明atemp1_8存在大于64的值
        //判定前向是否溢出
        vrange(SW_tempindex,half_length);
        vshuffle(SW_temp,SW_tempindex,atemp1_8,SHUFFLE_GATHER,half_length);
        m = vsle(pos64_8,SW_temp,MASKREAD_OFF,MASKWRITE_OFF,half_length);
        n = vredmin8(m,MASKREAD_OFF,half_length);
        vbarrier();
        VSPM_OPEN();
          t = *(volatile unsigned char *) (vaddr(n));
        VSPM_CLOSE();
        if(t ==0){
            for(short i=0;i<SW;i++){
                if(i==0){
                    vrange(SW_tempindex3,8);
                }
                vshuffle(SW_temp2,SW_tempindex3,atemp1_8,SHUFFLE_GATHER,8);
                m = vsle(pos64_8,SW_temp2,MASKREAD_OFF,MASKWRITE_OFF,8);
                n = vredmin8(m,MASKREAD_OFF,8);
                vbarrier();
                VSPM_OPEN();
                  t = *(volatile unsigned char *) (vaddr(n));
                VSPM_CLOSE();
                if(t ==0){
                    // printf("a_k=%d\t",&k);
                    // printf("a_i=%d\n",&i);
                    // SW_temp2 = vrsub(SW_temp2,pos64_8,MASKREAD_OFF,8);
                    // vshuffle(atemp1_8,SW_tempindex2,SW_temp2,SHUFFLE_SCATTER,8);
                    vshuffle(atemp1_8,SW_tempindex3,vrsub(SW_temp2,pos64_8,MASKREAD_OFF,8),SHUFFLE_SCATTER,8);
                }
                SW_tempindex3 = vadd(SW_tempindex3, 8,MASKREAD_OFF,8);
            }
        }
        // //判定后向是否溢出
        // SW_tempindex = vsadd(SW_tempindex,half_length,MASKREAD_OFF,half_length);
        // vshuffle(SW_temp,SW_tempindex,atemp1_8,SHUFFLE_GATHER,half_length);
        // m = vsle(pos64_8,SW_temp,MASKREAD_OFF,MASKWRITE_OFF,half_length);
        // n = vredmin8(m,MASKREAD_OFF,half_length);
        // if(*(volatile unsigned char *) (vaddr(n)) ==0){
        //     for(short i=0;i<SW;i++){
        //         if(i==0){
        //             vrange(SW_tempindex3,8);
        //             SW_tempindex3 = vadd(SW_tempindex3,half_length,MASKREAD_OFF,8);
        //         }
        //         vshuffle(SW_temp2,SW_tempindex3,atemp1_8,SHUFFLE_GATHER,8);
        //         m = vsle(pos64_8,SW_temp2,MASKREAD_OFF,MASKWRITE_OFF,8);
        //         n = vredmin8(m,MASKREAD_OFF,8);
        //         if(*(volatile unsigned char *) (vaddr(n)) ==0){
        //             // printf("b_k=%d\t",&k);
        //             // printf("b_i=%d\n",&i);
        //             // SW_temp2 = vrsub(SW_temp2,pos64_8,MASKREAD_OFF,8);
        //             // vshuffle(atemp1_8,SW_tempindex2,SW_temp2,SHUFFLE_SCATTER,8);
        //             vshuffle(atemp1_8,SW_tempindex3,vrsub(SW_temp2,pos64_8,MASKREAD_OFF,8),SHUFFLE_SCATTER,8);
        //         }
        //         SW_tempindex3 = vadd(SW_tempindex3, 8,MASKREAD_OFF,8);
        //     }
        // }

    }
    atemp = vsadd(atemp1_8, 0,MASKREAD_OFF,length);     //107



    //step5:移到atemp_8B上，index++
    //---串行版本
    for(short i=0;i<SW;i++){
      addr0 = vaddr(atemp)+ 8*i;
      addr1 = vaddr(atemp)+ 1 + 8*i;
      addr2 = vaddr(atemp)+ 2 + 8*i;
      addr3 = vaddr(atemp)+ 3 + 8*i;
      addr4 = vaddr(atemp)+ 4 + 8*i;
      addr5 = vaddr(atemp)+ 5 + 8*i;
      addr6 = vaddr(atemp)+ 6 + 8*i;
      addr7 = vaddr(atemp)+ 7 + 8*i;

      vbarrier();
      VSPM_OPEN();
          tmp0 = *(volatile unsigned char *) (addr0);
          tmp1 = *(volatile unsigned char *) (addr1);
          tmp2 = *(volatile unsigned char *) (addr2);
          tmp3 = *(volatile unsigned char *) (addr3);
          tmp4 = *(volatile unsigned char *) (addr4);
          tmp5 = *(volatile unsigned char *) (addr5);
          tmp6 = *(volatile unsigned char *) (addr6);
          tmp7 = *(volatile unsigned char *) (addr7);    
      VSPM_CLOSE();
      // printf("tmp0=%d\t",&tmp0);
      // printf("tmp1=%d\t",&tmp1);
      // printf("tmp2=%d\n",&tmp2);
      vbrdcst(a_B_tempindex,k + i*kminus,MASKREAD_OFF,1);           //108

      vbrdcst(a_B_temp,tmp0,MASKREAD_OFF,1);
      vshuffle(a1_B,a_B_tempindex,a_B_temp,SHUFFLE_SCATTER,1);

      vbrdcst(a_B_temp,tmp1,MASKREAD_OFF,1);
      vshuffle(a2_B,a_B_tempindex,a_B_temp,SHUFFLE_SCATTER,1);

      vbrdcst(a_B_temp,tmp2,MASKREAD_OFF,1);
      vshuffle(a3_B,a_B_tempindex,a_B_temp,SHUFFLE_SCATTER,1);

      vbrdcst(a_B_temp,tmp3,MASKREAD_OFF,1);
      vshuffle(a4_B,a_B_tempindex,a_B_temp,SHUFFLE_SCATTER,1);

      vbrdcst(a_B_temp,tmp4,MASKREAD_OFF,1);
      vshuffle(a5_B,a_B_tempindex,a_B_temp,SHUFFLE_SCATTER,1);

      vbrdcst(a_B_temp,tmp5,MASKREAD_OFF,1);
      vshuffle(a6_B,a_B_tempindex,a_B_temp,SHUFFLE_SCATTER,1);

      vbrdcst(a_B_temp,tmp6,MASKREAD_OFF,1);
      vshuffle(a7_B,a_B_tempindex,a_B_temp,SHUFFLE_SCATTER,1);

      vbrdcst(a_B_temp,tmp7,MASKREAD_OFF,1);
      vshuffle(a8_B,a_B_tempindex,a_B_temp,SHUFFLE_SCATTER,1);



      // vbrdcst(SW_tempindex3,0 + 16*i,MASKREAD_OFF,1);
      // vshuffle(SW_temp,SW_tempindex3,atemp,SHUFFLE_GATHER,1);
      // vbrdcst(SW_tempindex3,k + i*kminus,MASKREAD_OFF,1);
      // vshuffle(a1_B,SW_tempindex3,SW_temp,SHUFFLE_SCATTER,1);

      // vshuffle(SW_temp,1 + 16*i,atemp,SHUFFLE_GATHER,1);
      // vshuffle(a2_B,k + i*kminus,SW_temp,SHUFFLE_SCATTER,1);

      // vshuffle(SW_temp,2 + 16*i,atemp,SHUFFLE_GATHER,1);
      // vshuffle(a3_B,k + i*kminus,SW_temp,SHUFFLE_SCATTER,1);

      // vshuffle(SW_temp,3 + 16*i,atemp,SHUFFLE_GATHER,1);
      // vshuffle(a4_B,k + i*kminus,SW_temp,SHUFFLE_SCATTER,1);

      // vshuffle(SW_temp,4 + 16*i,atemp,SHUFFLE_GATHER,1);
      // vshuffle(a5_B,k + i*kminus,SW_temp,SHUFFLE_SCATTER,1);

      // vshuffle(SW_temp,5 + 16*i,atemp,SHUFFLE_GATHER,1);
      // vshuffle(a6_B,k + i*kminus,SW_temp,SHUFFLE_SCATTER,1);

      // vshuffle(SW_temp,6 + 16*i,atemp,SHUFFLE_GATHER,1);
      // vshuffle(a7_B,k + i*kminus,SW_temp,SHUFFLE_SCATTER,1);

      // vshuffle(SW_temp,7 + 16*i,atemp,SHUFFLE_GATHER,1);
      // vshuffle(a8_B,k + i*kminus,SW_temp,SHUFFLE_SCATTER,1);

      // vshuffle(SW_temp,8 + 16*i,atemp,SHUFFLE_GATHER,1);
      // vshuffle(b1_B,k + B-2 - i*kminus,SW_temp,SHUFFLE_SCATTER,1);

      // vshuffle(SW_temp,9 + 16*i,atemp,SHUFFLE_GATHER,1);
      // vshuffle(b2_B,k + B-2 - i*kminus,SW_temp,SHUFFLE_SCATTER,1);

      // vshuffle(SW_temp,10 + 16*i,atemp,SHUFFLE_GATHER,1);
      // vshuffle(b3_B,k + B-2 - i*kminus,SW_temp,SHUFFLE_SCATTER,1);

      // vshuffle(SW_temp,11 + 16*i,atemp,SHUFFLE_GATHER,1);
      // vshuffle(b4_B,k + B-2 - i*kminus,SW_temp,SHUFFLE_SCATTER,1);

      // vshuffle(SW_temp,12 + 16*i,atemp,SHUFFLE_GATHER,1);
      // vshuffle(b5_B,k + B-2 - i*kminus,SW_temp,SHUFFLE_SCATTER,1);

      // vshuffle(SW_temp,13 + 16*i,atemp,SHUFFLE_GATHER,1);
      // vshuffle(b6_B,k + B-2 - i*kminus,SW_temp,SHUFFLE_SCATTER,1);

      // vshuffle(SW_temp,14 + 16*i,atemp,SHUFFLE_GATHER,1);
      // vshuffle(b7_B,k + B-2 - i*kminus,SW_temp,SHUFFLE_SCATTER,1);

      // vshuffle(SW_temp,15 + 16*i,atemp,SHUFFLE_GATHER,1);
      // vshuffle(b8_B,k + B-2 - i*kminus,SW_temp,SHUFFLE_SCATTER,1);

    }     
  }




  // char words[30] = "turbodecode finished";
  // printf("------%s---------\n",&words);

  // __v4096i8 softin1_x_B_out;
  // vclaim(softin1_x_B_out);
  // // vbrdcst(softin1_x_B_out,0,MASKREAD_OFF,10);
  // softin1_x_B_out = vsadd(softin1_x_B,0,MASKREAD_OFF,B);

  d1_B = vsadd(d1_B,0,MASKREAD_OFF,B);
  d2_B = vsadd(d2_B,0,MASKREAD_OFF,B);

  a1_B = vsadd(a1_B,0,MASKREAD_OFF,B);
  a2_B = vsadd(a2_B,0,MASKREAD_OFF,B);
  a3_B = vsadd(a3_B,0,MASKREAD_OFF,B);
  a4_B = vsadd(a4_B,0,MASKREAD_OFF,B);
  a5_B = vsadd(a5_B,0,MASKREAD_OFF,B);
  a6_B = vsadd(a6_B,0,MASKREAD_OFF,B);
  a7_B = vsadd(a7_B,0,MASKREAD_OFF,B);
  a8_B = vsadd(a8_B,0,MASKREAD_OFF,B);

  // b1_B = vsadd(b1_B,0,MASKREAD_OFF,B);
  // b2_B = vsadd(b2_B,0,MASKREAD_OFF,B);
  // b3_B = vsadd(b3_B,0,MASKREAD_OFF,B);
  // b4_B = vsadd(b4_B,0,MASKREAD_OFF,B);
  // b5_B = vsadd(b5_B,0,MASKREAD_OFF,B);
  // b6_B = vsadd(b6_B,0,MASKREAD_OFF,B);
  // b7_B = vsadd(b7_B,0,MASKREAD_OFF,B);
  // b8_B = vsadd(b8_B,0,MASKREAD_OFF,B);
 


  /**************** 缩放，防止溢出 5/16=0.3125 ************** */ 
  vsetshamt(5);
  a1_B = vmul(a1_B,5,MASKREAD_OFF,B);
  a2_B = vmul(a2_B,5,MASKREAD_OFF,B);
  a3_B = vmul(a3_B,5,MASKREAD_OFF,B);
  a4_B = vmul(a4_B,5,MASKREAD_OFF,B);
  a5_B = vmul(a5_B,5,MASKREAD_OFF,B);
  a6_B = vmul(a6_B,5,MASKREAD_OFF,B);
  a7_B = vmul(a7_B,5,MASKREAD_OFF,B);
  a8_B = vmul(a8_B,5,MASKREAD_OFF,B);
  
  // a1_B = vsra(a1_B,4,MASKREAD_OFF,B);
  // a2_B = vsra(a2_B,4,MASKREAD_OFF,B);
  // a3_B = vsra(a3_B,4,MASKREAD_OFF,B);
  // a4_B = vsra(a4_B,4,MASKREAD_OFF,B);
  // a5_B = vsra(a5_B,4,MASKREAD_OFF,B);
  // a6_B = vsra(a6_B,4,MASKREAD_OFF,B);
  // a7_B = vsra(a7_B,4,MASKREAD_OFF,B);
  // a8_B = vsra(a8_B,4,MASKREAD_OFF,B);

  // // b1_B = vsra(b1_B,2,MASKREAD_OFF,B);
  // // b2_B = vsra(b2_B,2,MASKREAD_OFF,B);
  // // b3_B = vsra(b3_B,2,MASKREAD_OFF,B);
  // // b4_B = vsra(b4_B,2,MASKREAD_OFF,B);
  // // b5_B = vsra(b5_B,2,MASKREAD_OFF,B);
  // // b6_B = vsra(b6_B,2,MASKREAD_OFF,B);
  // // b7_B = vsra(b7_B,2,MASKREAD_OFF,B);
  // // b8_B = vsra(b8_B,2,MASKREAD_OFF,B);

  // a1_B = vmul(a1_B,5,MASKREAD_OFF,B);
  // a2_B = vmul(a2_B,5,MASKREAD_OFF,B);
  // a3_B = vmul(a3_B,5,MASKREAD_OFF,B);
  // a4_B = vmul(a4_B,5,MASKREAD_OFF,B);
  // a5_B = vmul(a5_B,5,MASKREAD_OFF,B);
  // a6_B = vmul(a6_B,5,MASKREAD_OFF,B);
  // a7_B = vmul(a7_B,5,MASKREAD_OFF,B);
  // a8_B = vmul(a8_B,5,MASKREAD_OFF,B);

  // // b1_B = vmul(b1_B,5,MASKREAD_OFF,B);
  // // b2_B = vmul(b2_B,5,MASKREAD_OFF,B);
  // // b3_B = vmul(b3_B,5,MASKREAD_OFF,B);
  // // b4_B = vmul(b4_B,5,MASKREAD_OFF,B);
  // // b5_B = vmul(b5_B,5,MASKREAD_OFF,B);
  // // b6_B = vmul(b6_B,5,MASKREAD_OFF,B);
  // // b7_B = vmul(b7_B,5,MASKREAD_OFF,B);
  // // b8_B = vmul(b8_B,5,MASKREAD_OFF,B);


  // a1_B = vsra(a1_B,1,MASKREAD_OFF,B);
  // a2_B = vsra(a2_B,1,MASKREAD_OFF,B);
  // a3_B = vsra(a3_B,1,MASKREAD_OFF,B);
  // a4_B = vsra(a4_B,1,MASKREAD_OFF,B);
  // a5_B = vsra(a5_B,1,MASKREAD_OFF,B);
  // a6_B = vsra(a6_B,1,MASKREAD_OFF,B);
  // a7_B = vsra(a7_B,1,MASKREAD_OFF,B);
  // a8_B = vsra(a8_B,1,MASKREAD_OFF,B);

  // // b1_B = vsra(b1_B,2,MASKREAD_OFF,B);
  // // b2_B = vsra(b2_B,2,MASKREAD_OFF,B);
  // // b3_B = vsra(b3_B,2,MASKREAD_OFF,B);
  // // b4_B = vsra(b4_B,2,MASKREAD_OFF,B);
  // // b5_B = vsra(b5_B,2,MASKREAD_OFF,B);
  // // b6_B = vsra(b6_B,2,MASKREAD_OFF,B);
  // // b7_B = vsra(b7_B,2,MASKREAD_OFF,B);
  // // b8_B = vsra(b8_B,2,MASKREAD_OFF,B);




  // __v6148i8 a0;
  // vclaim(a0);
  // vreturn(a0,sizeof(a0));

  vreturn(a1_B,B, a2_B,B, a3_B,B, a4_B,B, a5_B,B, a6_B,B, a7_B,B, a8_B,B, softin1_x_B,B, d1_B,B,d2_B,B , a_p_B,B, e_p_B,B );
  // vreturn(a1_B,B, a2_B,B, a3_B,B, a4_B,B, a5_B,B, a6_B,B, a7_B,B, a8_B,B, b1_B,B, b2_B,B, b3_B,B, b4_B,B, b5_B,B, b6_B,B, b7_B,B, b8_B,B, softin1_x_B,B, d1_B,B,d2_B,B , a_p_B,B, e_p_B,B );

}