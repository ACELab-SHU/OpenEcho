/**
 * ****************************************
 * @file        Task_lteDCIDecode.c
 * @brief       DCI decode
 * @author      chenxiaoxiao
 * @date        2025.4.18
 * @copyright   ACE-Lab(Shanghai University)
 * ****************************************
 */

 #include "riscv_printf.h"
 #include "venus.h"
 #include "data_type.h"
 #include "vmath.h"

 
 int Task_InfoLength(short_struct NDLRB, short_struct frame_type) {

  short NSizeBWP = NDLRB.data;
  short frame_type_value = frame_type.data;
  //计算DCI比特长度
   int K = 12;//format01_Flag,VRB_assignment,LCRBs,RBstart,MCS,Newdata_indicator,Redundancy_version,TPC_command

   short FreDomain_length = 0;

   FreDomain_length = ceil_log2(NSizeBWP * (NSizeBWP + 1) >> 1);

   K += FreDomain_length;//Resource_assignment
   
   short HARQandDCI_length = 0;

   if(frame_type_value == 2)
   {//TDD
    HARQandDCI_length = 6;//HARQ_number,DCI_length
   }
   else
   {
    HARQandDCI_length = 3;//FDD
   }
   K += HARQandDCI_length;
 
   if (K == 12 || K == 14 || K == 16 || K == 20 || K == 24 || K == 26 || K == 32 || K == 40 || K == 44 || K == 56)
   {
    K += 1;//follow format0 : 27
   }


   K += 16;//CRC16
  printf("K:%d\n", &K);
  short_struct K_out;
   K_out.data = K;
   vreturn(&K_out, sizeof(short_struct));
 }