/*
 * @Author: shenyihao shenyihao@shu.edu.cn
 * @Date: 2025-03-05 22:35:15
 * @LastEditors: Yihao Shen shenyihao@shu.edu.cn
 * @LastEditTime: 2025-04-25 11:19:30
 * @FilePath: /VEMU/AceEcho/tasks/lteChannelEst/Task_ltePBCHDmrsIndices.c
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
/**
 * ****************************************
 * @file        Task_ltePBCHDmrsGen.c
 * @brief       LTE PBCH Dmrs Generation
 * @author      shenyihao
 * @date        2025.2.27
 * @copyright   ACE-Lab(Shanghai University)
 * ****************************************
 */

 #include "data_type.h"
 #include "riscv_printf.h"
 #include "venus.h"
 
 typedef short __v2048i16 __attribute__((ext_vector_type(2048)));
 typedef short __v4096i16 __attribute__((ext_vector_type(4096)));
 typedef char __v4096i8 __attribute__((ext_vector_type(4096)));
 
 
 int Task_ltePBCHDmrsIndices(
     __v2048i16 init_indices, short_struct nCellID, short_struct port_num)
 {

  int NID = nCellID.data;
  int port = port_num.data;

  int v = 0;
  int l_num = 0;

  if(port == 0){
    v = 0;
    l_num = 0;
  }
  else if(port == 1){
    v = 3;
    l_num = 0;
  }
  else if(port == 2){
    v = 3;
    l_num = 1;
  }
  else if(port ==3){
    v = 0;
    l_num = 1;
  }
  else{
    v = 0;
    l_num = 0;
  }

  int shift_l = 72 * l_num;
  int shift = (v + NID) % 6;
  // printf("shift:%ld\n",&shift); 


  __v2048i16 shift_pos;
  vclaim(shift_pos);
  if(shift == 0 || shift == 1 || shift == 2){
    vbrdcst(shift_pos, shift_l + shift, MASKREAD_OFF, 24);
  }
  else
  {
    vbrdcst(shift_pos, shift_l + shift - 6, MASKREAD_OFF, 24);
    vbrdcst(shift_pos, shift_l + shift, MASKREAD_OFF, 12);
  }


  __v2048i16 seq;
  seq = vadd(init_indices,shift_pos,MASKREAD_OFF,24);
   vreturn(seq, 48);
 }
 