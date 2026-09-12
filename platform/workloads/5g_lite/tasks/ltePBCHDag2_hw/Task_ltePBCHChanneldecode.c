/**
 ******************************************************************************
 * @file           : Task_ltePBCHChanneldecode.c
 * @author         : XiaoxiaoChen
 * @date           : 2024/11/25
 * @brief          : LTE PBCH Rate Dematch 3GPP 36.212
 * @attention      :
 * @LastEditors    :
 * @LastEditData   : 2024/11/13
 ******************************************************************************
 * Copyright (c) 2024 by ACE_Lab, All Rights Reserved.
 */

#include "data_type.h"
#include "riscv_printf.h"
#include "stdint.h"
#include "venus.h"
#include "vmath.h"

typedef short __v2048i16 __attribute__((ext_vector_type(2048)));
typedef char  __v4096i8 __attribute__((ext_vector_type(4096)));

int K = 40; // 假设的输入长度，调整大小以适应你的具体输入

int Task_ltePBCHChanneldecode(__v4096i8 Out0x_0, __v4096i8 Out0x_1, __v4096i8 Out0x_2, __v4096i8 Out1x_0,
                              __v4096i8 Out1x_1, __v4096i8 Out1x_2, __v4096i8 index00_8bit, __v4096i8 index01_8bit,
                              __v4096i8 P0, __v4096i8 P1, __v4096i8 P2, __v4096i8 range64_8bit) {
  // P0 = vsadd(P0,0,MASKREAD_OFF,40);
  // P1 = vsadd(P1,0,MASKREAD_OFF,40);
  // P2 = vsadd(P2,0,MASKREAD_OFF,40);
  __v4096i8 P0_1;
  __v4096i8 P1_1;
  __v4096i8 P2_1;
  __v4096i8 P0_2;
  __v4096i8 P1_2;
  __v4096i8 P2_2;
  vclaim(P0_1);
  vclaim(P1_1);
  vclaim(P2_1);
  vclaim(P0_2);
  vclaim(P1_2);
  vclaim(P2_2);

  // int K = 80;// 假设的输入长度，调整大小以适应你的具体输入
  __v2048i16 index_K0;
  __v2048i16 index_62;
  __v2048i16 index_K_62;
  __v2048i16 temp_62;
  vclaim(index_K0);
  vclaim(index_62);
  vclaim(index_K_62);
  vclaim(temp_62);

  if (K <= 62) // 分两个向量存储数据 一个向量存储62个
  {
    vrange(index_K0, K);
    vshuffle(P0_1, index_K0, P0, SHUFFLE_GATHER, K);
    vshuffle(P1_1, index_K0, P1, SHUFFLE_GATHER, K);
    vshuffle(P2_1, index_K0, P2, SHUFFLE_GATHER, K);
  } else {
    vrange(index_62, 62);
    vbrdcst(temp_62, 62, MASKREAD_OFF, K - 62);
    vrange(index_K_62, K - 62);
    index_K_62 = vsadd(index_K_62, 62, MASKREAD_OFF, K - 62);
    vshuffle(P0_1, index_62, P0, SHUFFLE_GATHER, 62);
    vshuffle(P0_2, index_K_62, P0, SHUFFLE_GATHER, K - 62);
    vshuffle(P1_1, index_62, P1, SHUFFLE_GATHER, 62);
    vshuffle(P1_2, index_K_62, P1, SHUFFLE_GATHER, K - 62);
    vshuffle(P2_1, index_62, P2, SHUFFLE_GATHER, 62);
    vshuffle(P2_2, index_K_62, P2, SHUFFLE_GATHER, K - 62);
  }

  // 状态量度、路径、分支矩阵、输出
  __v4096i8 state;
  __v4096i8 Path_1;
  __v4096i8 Path_2;
  __v4096i8 Branch_1;
  __v4096i8 Branch_2;
  __v4096i8 Out;

  vclaim(state);
  vclaim(Path_1);
  vclaim(Branch_1);
  vclaim(Out);
  // vbrdcst(Out, 0, MASKREAD_OFF,K);

  // 初始化
  vbrdcst(state, 0, MASKREAD_OFF, (62 + 1) * 64);
  vbrdcst(Path_1, 0, MASKREAD_OFF, 62 * 64);
  vbrdcst(Path_2, 0, MASKREAD_OFF, (62 + 1) * 64);
  vbrdcst(Branch_1, 0, MASKREAD_OFF, (62 + 1) * 64);
  vbrdcst(Branch_2, 0, MASKREAD_OFF, (62 + 1) * 64);

  int Flag    = 0;
  int iternum = 3;   // 迭代次数
  int instanceStart; // 时刻
  int instanceEnd;
  int Tail_state;

  __v4096i8 temp8bit_0;
  __v2048i16 temp_1;
  __v2048i16 temp_2;
  __v2048i16 temp_5;
  __v2048i16 temp_32;
  __v2048i16 temp_64;
  __v2048i16 temp_N1;
  __v2048i16 index_a;
  vclaim(temp8bit_0);
  vclaim(temp_1);
  vclaim(temp_2);
  vclaim(temp_5);
  vclaim(temp_32);
  vclaim(temp_64);
  vclaim(temp_N1);
  vclaim(index_a);
  vbrdcst(temp8bit_0, 0, MASKREAD_OFF, 64);
  vbrdcst(temp_1, 1, MASKREAD_OFF, 64); //
  vbrdcst(temp_2, 2, MASKREAD_OFF, 32); //
  vbrdcst(temp_32, 32, MASKREAD_OFF, 32);
  vbrdcst(temp_64, 64, MASKREAD_OFF, 64);
  vbrdcst(temp_N1, -1, MASKREAD_OFF, 64);
  vbrdcst(index_a, 0, MASKREAD_OFF, 64); // 放前面

  __v2048i16 index_state_64;
  //__v2048i16 index_state_32;
  vclaim(index_state_64);
  // vclaim(index_state_32);
  // vrange(index_state_64, 64);
  // vrange(index_state_32, 32);
  __v2048i16 index_Out;
  vclaim(index_Out);
  vrange(index_Out, 5);

  __v2048i16 index_32;
  __v2048i16 index_64;
  __v2048i16 index_K;
  vclaim(index_32);
  vclaim(index_64);
  vclaim(index_K);
  vrange(index_32, 32);
  vrange(index_64, 64);
  vrange(index_K, K);
  __v2048i16 index_state_32_63;
  vclaim(index_state_32_63);
  // index_state_32_63 = vsadd(index_state_32, temp_32, MASKREAD_OFF, 32);

  __v4096i8 temp_zero;
  __v4096i8 temp_one;
  __v4096i8 temp_255;
  vclaim(temp_zero);
  vclaim(temp_one);
  vclaim(temp_255);
  vbrdcst(temp_zero, 0, MASKREAD_OFF, 64);
  vbrdcst(temp_one, 1, MASKREAD_OFF, 64);
  vbrdcst(temp_255, 0xFF, MASKREAD_OFF, 64);

  __v2048i16 index00;
  __v2048i16 index01;
  vclaim(index00);
  vclaim(index01);
  index00 = vmul(index_32, 2, MASKREAD_OFF, 32); // 查找输入为0和1时，当前状态istate的起始状态值
  index01 = vmul(index_32, 2, MASKREAD_OFF, 32);
  index01 = vsadd(index01, 1, MASKREAD_OFF, 32);

  __v2048i16 temp_instance;
  __v2048i16 index_i;
  __v2048i16 index_max;
  __v4096i8 temp_index_out;
  __v2048i16 index_Branch1;
  __v2048i16 Flag_out;
  __v2048i16 temp_i;
  __v2048i16 indexout;
  __v2048i16 index_b;
  vclaim(temp_instance);
  vclaim(index_i);
  vclaim(index_max);
  vclaim(temp_index_out);
  vclaim(index_Branch1);
  vclaim(Flag_out);
  vclaim(temp_i);
  vclaim(indexout);
  vclaim(index_b);

  __v4096i8 P0_instance;
  __v4096i8 P1_instance;
  __v4096i8 P2_instance;
  __v4096i8 BranchMetric0x;
  __v4096i8 BranchMetric1x;
  __v4096i8 StateMetric0x;
  __v4096i8 StateMetric1x;
  __v4096i8 StateMetricTemp;
  __v4096i8 temp_a;
  __v4096i8 temp_b;
  __v4096i8 state_survive;
  __v4096i8 temp_branch;
  __v4096i8 temp_Branch1;
  __v4096i8 branch_survive;
  __v4096i8 state_instance;
  __v4096i8 sort_state;
  __v4096i8 state_K;
  __v4096i8 Max;
  __v4096i8 temp_out_survive;
  __v4096i8 temp_out;
  __v4096i8 state_K62;
  __v4096i8 state_62;

  vclaim(P0_instance);
  vclaim(P1_instance);
  vclaim(P2_instance);
  vclaim(BranchMetric0x);
  vclaim(BranchMetric1x);
  vclaim(StateMetric0x);
  vclaim(StateMetric1x);
  vclaim(StateMetricTemp);
  vclaim(temp_a);
  vclaim(temp_b);
  vclaim(state_survive);
  vclaim(temp_branch);
  vclaim(temp_Branch1);
  vclaim(branch_survive);
  vclaim(state_instance);
  vclaim(sort_state);
  vclaim(state_K);
  vclaim(Max);
  vclaim(temp_out_survive);
  vclaim(temp_out);
  vclaim(state_K62);
  vclaim(state_62);

  // __v2048i16 index_outistate;
  // __v4096i8 temp_istate;
  // __v4096i8 outistate;
  // vclaim(index_outistate);
  // vclaim(temp_istate);
  // vclaim(outistate);
  // P0_1 = vsadd(P0_1,0,MASKREAD_OFF,40);//5
  // P1_1 = vsadd(P1_1,0,MASKREAD_OFF,40);
  // P2_1 = vsadd(P2_1,0,MASKREAD_OFF,40);//7
  if (K <= 62) {
    for (int iRun = 1; iRun <= iternum; iRun++) {
      // instanceStart = 1;
      // instanceEnd = K;
      vrange(index_state_32_63, 32);
      index_state_32_63 = vsadd(index_state_32_63, 32, MASKREAD_OFF, 32);
      vrange(index_state_64, 64);
      for (int instance = 1; instance <= K; instance++) {
        vbrdcst(temp_instance, instance - 1, MASKREAD_OFF, 64);

        vshuffle(P0_instance, temp_instance, P0_1, SHUFFLE_GATHER, 64);
        vshuffle(P1_instance, temp_instance, P1_1, SHUFFLE_GATHER, 64);
        vshuffle(P2_instance, temp_instance, P2_1, SHUFFLE_GATHER, 64);

        // 从状态index0x，index1x，输入0和1，到达状态istate时的分支量度
        BranchMetric0x = vmul(Out0x_0, P0_instance, MASKREAD_OFF, 64);
        BranchMetric1x = vmul(Out1x_0, P0_instance, MASKREAD_OFF, 64);

        StateMetric0x = vmul(Out0x_1, P1_instance, MASKREAD_OFF, 64);
        StateMetric1x = vmul(Out1x_1, P1_instance, MASKREAD_OFF, 64);

        StateMetric0x = vsadd(StateMetric0x, BranchMetric0x, MASKREAD_OFF, 64);
        StateMetric1x = vsadd(StateMetric1x, BranchMetric1x, MASKREAD_OFF, 64);

        BranchMetric0x = vmul(Out0x_2, P2_instance, MASKREAD_OFF, 64);
        BranchMetric1x = vmul(Out1x_2, P2_instance, MASKREAD_OFF, 64);

        StateMetric0x = vsadd(StateMetric0x, BranchMetric0x, MASKREAD_OFF, 64);
        StateMetric1x = vsadd(StateMetric1x, BranchMetric1x, MASKREAD_OFF, 64);

        // 从状态index0x，index1x，输入0和1，到达状态istate时的状态量度
        vshuffle(StateMetricTemp, index_state_64, state, SHUFFLE_GATHER, 64); // 取出前一时刻的累计度量
        StateMetric0x = vsadd(StateMetric0x, StateMetricTemp, MASKREAD_OFF, 64);
        StateMetric1x = vsadd(StateMetric1x, StateMetricTemp, MASKREAD_OFF, 64);

        // 获取输入为0的幸存路径
        vshuffle(temp_a, index00, StateMetric0x, SHUFFLE_GATHER, 32); // 取出index00的累计状态度量
        vshuffle(temp_b, index01, StateMetric0x, SHUFFLE_GATHER, 32); // 取出index01的累计状态度量

        vsle(temp_b, temp_a, MASKREAD_OFF, MASKWRITE_ON, 32);
        temp_a        = vxor(temp_a, temp_a, MASKREAD_ON, 32);
        state_survive = vsadd(temp_a, temp_b, MASKREAD_ON, 32); // 得到最新state

        temp_branch    = vxor(index00_8bit, index00_8bit, MASKREAD_ON, 32);
        branch_survive = vsadd(temp_branch, index01_8bit, MASKREAD_ON, 32); // 得到最新Branch 16
        // 存值
        // vshuffle(Path_1, index_state_32, temp_zero, SHUFFLE_SCATTER, 32);
        index_state_32_63 = vsadd(index_state_32_63, 32, MASKREAD_OFF, 32);
        vshuffle(state, index_state_32_63, state_survive, SHUFFLE_SCATTER, 32);
        vshuffle(Branch_1, index_state_32_63, branch_survive, SHUFFLE_SCATTER, 32);

        // 获取输入为1的幸存路径
        vshuffle(temp_a, index00, StateMetric1x, SHUFFLE_GATHER, 32); // 取出index00的累计状态度量
        vshuffle(temp_b, index01, StateMetric1x, SHUFFLE_GATHER, 32); // 取出index01的累计状态度量

        vsle(temp_b, temp_a, MASKREAD_OFF, MASKWRITE_ON, 32);
        temp_a        = vxor(temp_a, temp_a, MASKREAD_ON, 32);
        state_survive = vsadd(temp_a, temp_b, MASKREAD_ON, 32); // 得到最新state

        temp_branch    = vxor(index00_8bit, index00_8bit, MASKREAD_ON, 32);
        branch_survive = vsadd(temp_branch, index01_8bit, MASKREAD_ON, 32); // 得到最新Branch
        // 存值
        // vshuffle(Path_1, index_state_32_63, temp_one, SHUFFLE_SCATTER, 32);
        index_state_32_63 = vsadd(index_state_32_63, 32, MASKREAD_OFF, 32);
        vshuffle(state, index_state_32_63, state_survive, SHUFFLE_SCATTER, 32);
        vshuffle(Branch_1, index_state_32_63, branch_survive, SHUFFLE_SCATTER, 32);

        index_state_64 = vsadd(index_state_64, 64, MASKREAD_OFF, 64);

        // 防溢出处理
        vshuffle(state_instance, index_state_64, state, SHUFFLE_GATHER, 64);

        sort_state = vxor(state_instance, temp_255, MASKREAD_OFF, 64); // 转最小
        sort_state = vsadd(sort_state, temp_one, MASKREAD_OFF, 64);    //
        // sort_state = vmul(state_instance, -1, MASKREAD_OFF, 64);
        sort_state = vredmin8(sort_state, MASKREAD_OFF, 64);

        __asm__("addi x0, x0, 0");__asm__("addi x0, x0, 0");
        __asm__("addi x0, x0, 0");__asm__("addi x0, x0, 0");
        __asm__("addi x0, x0, 0");__asm__("addi x0, x0, 0");
        __asm__("addi x0, x0, 0");__asm__("addi x0, x0, 0");
        __asm__("addi x0, x0, 0");__asm__("addi x0, x0, 0");
        vbarrier();
        VSPM_OPEN();
        int  state_addr = vaddr(sort_state);
        char state_max  = *(volatile unsigned char *)(state_addr);
        VSPM_CLOSE();

        vbrdcst(Max, state_max, MASKREAD_OFF, 64);
        state_instance = vsadd(state_instance, Max, MASKREAD_OFF, 64); //
        vshuffle(state, index_state_64, state_instance, SHUFFLE_SCATTER, 64);
      }

      // 更新迭代
      if (iRun != iternum) {
        // index_state_64 = vsadd(temp_0, index_state_64, MASKREAD_OFF, 64);
        vshuffle(state_K, index_state_64, state, SHUFFLE_GATHER, 64);
        vbrdcst(state, 0, MASKREAD_OFF, (K + 1) * 64);
        vshuffle(state, index_64, state_K, SHUFFLE_SCATTER, 64);
      }
    }

    Branch_1 = vsadd(Branch_1, 0, MASKREAD_OFF, (62 + 1) * 64);
    // 回溯路径
    vshuffle(state_K, index_state_64, state, SHUFFLE_GATHER, 64);
    state_K = vxor(state_K, temp_255, MASKREAD_OFF, 64);  //
    state_K = vsadd(state_K, temp_one, MASKREAD_OFF, 64); //
    for (int ccc = 0; ccc < 5; ccc++) {

      // state_K = vsadd(state_K, 0, MASKREAD_OFF, 2048); //
      // state_K = vsadd(state_K, 0, MASKREAD_OFF, 2048); //
      sort_state = vredmin8(state_K, MASKREAD_OFF, 64);

      __asm__("addi x0, x0, 0");__asm__("addi x0, x0, 0");
      __asm__("addi x0, x0, 0");__asm__("addi x0, x0, 0");
      __asm__("addi x0, x0, 0");__asm__("addi x0, x0, 0");
      __asm__("addi x0, x0, 0");__asm__("addi x0, x0, 0");
      __asm__("addi x0, x0, 0");__asm__("addi x0, x0, 0");
      vbarrier();
      VSPM_OPEN();
      int  state_addr = vaddr(sort_state);
      char state_max  = *(volatile unsigned char *)(state_addr);
      VSPM_CLOSE();

      vbrdcst(Max, state_max, MASKREAD_OFF, 64);

      vseq(Max, state_K, MASKREAD_OFF, MASKWRITE_ON, 64);
      temp_index_out = vsadd(temp8bit_0, range64_8bit, MASKREAD_ON, 64);

      // index_b        = vseq(temp_0, index_a, MASKREAD_OFF, MASKWRITE_OFF, 64);
      // temp_index_out = vmul(temp_index_out, index_b, MASKREAD_OFF, 64);
      temp_index_out = vmul(temp_index_out, -1, MASKREAD_OFF, 64); //
      temp_index_out = vredmin8(temp_index_out, MASKREAD_OFF, 64);
      temp_index_out = vmul(temp_index_out, -1, MASKREAD_OFF, 1); //

      __asm__("addi x0, x0, 0");__asm__("addi x0, x0, 0");
      __asm__("addi x0, x0, 0");__asm__("addi x0, x0, 0");
      __asm__("addi x0, x0, 0");__asm__("addi x0, x0, 0");
      __asm__("addi x0, x0, 0");__asm__("addi x0, x0, 0");
      __asm__("addi x0, x0, 0");__asm__("addi x0, x0, 0");
      vbarrier();
      VSPM_OPEN();
      int index_max_addr = vaddr(temp_index_out);
      int istate         = *(volatile unsigned char *)(index_max_addr);
      VSPM_CLOSE();

      __asm__("addi x0, x0, 0");__asm__("addi x0, x0, 0");
      __asm__("addi x0, x0, 0");__asm__("addi x0, x0, 0");
      __asm__("addi x0, x0, 0");__asm__("addi x0, x0, 0");
      __asm__("addi x0, x0, 0");__asm__("addi x0, x0, 0");
      __asm__("addi x0, x0, 0");__asm__("addi x0, x0, 0");
      vbarrier();
      VSPM_OPEN();
      int state_K_addr = vaddr(state_K);
       *(volatile unsigned char *)(state_K_addr + istate) = 127;
      VSPM_CLOSE();


      for (int instance = K; instance > 0; instance--) {
        vbrdcst(index_Out, instance - 1, MASKREAD_OFF, 1);
        if (istate < 32) {
          // temp_zero = vsadd(temp_zero, 0, MASKREAD_OFF, 2048);
          // index_Out = vsadd(index_Out, 0, MASKREAD_OFF, 2048);
          vshuffle(Out, index_Out, temp_zero, SHUFFLE_SCATTER, 1);
          // Out = vsadd(Out, 0, MASKREAD_OFF, 2048);
        } else {
          // temp_one = vsadd(temp_one, 0, MASKREAD_OFF, 2048);
          // index_Out = vsadd(index_Out, 0, MASKREAD_OFF, 2048);
          vshuffle(Out, index_Out, temp_one, SHUFFLE_SCATTER, 1);
          // Out = vsadd(Out, 0, MASKREAD_OFF, 2048);
        }
        if (instance == K) {
          Tail_state = istate;
        }
        // Out = vsadd(Out,0,MASKREAD_OFF,K);
        vbrdcst(index_Branch1, instance * 64 + istate, MASKREAD_OFF, 1);
        vshuffle(temp_Branch1, index_Branch1, Branch_1, SHUFFLE_GATHER, 1);

        __asm__("addi x0, x0, 0");__asm__("addi x0, x0, 0");
        __asm__("addi x0, x0, 0");__asm__("addi x0, x0, 0");
        __asm__("addi x0, x0, 0");__asm__("addi x0, x0, 0");
        __asm__("addi x0, x0, 0");__asm__("addi x0, x0, 0");
        __asm__("addi x0, x0, 0");__asm__("addi x0, x0, 0");
        vbarrier();
        VSPM_OPEN();
        int temp_Branch1_addr = vaddr(temp_Branch1);
        istate                = *(volatile unsigned char *)(temp_Branch1_addr);
        VSPM_CLOSE();

      }
      if (istate == Tail_state) {
        Flag = 1;
        break;
      }
      // printf("\n",NULL);
    }
  }
  // 如果数据大于62个
  else {
    vbrdcst(state, 0, MASKREAD_OFF, (62 + 1) * 64);
    for (int iRun = 1; iRun <= iternum; iRun++) {
      // instanceStart = 1;
      // instanceEnd = 62;
      vrange(index_state_32_63, 32);
      index_state_32_63 = vsadd(index_state_32_63, 32, MASKREAD_OFF, 32);
      vrange(index_state_64, 64);
      for (int instance = 1; instance <= 62; instance++) {
        vbrdcst(temp_instance, instance - 1, MASKREAD_OFF, 64);

        vshuffle(P0_instance, temp_instance, P0_1, SHUFFLE_GATHER, 64);
        vshuffle(P1_instance, temp_instance, P1_1, SHUFFLE_GATHER, 64);
        vshuffle(P2_instance, temp_instance, P2_1, SHUFFLE_GATHER, 64);

        // 从状态index0x，index1x，输入0和1，到达状态istate时的分支量度
        BranchMetric0x = vmul(Out0x_0, P0_instance, MASKREAD_OFF, 64);
        BranchMetric1x = vmul(Out1x_0, P0_instance, MASKREAD_OFF, 64);

        StateMetric0x = vmul(Out0x_1, P1_instance, MASKREAD_OFF, 64);
        StateMetric1x = vmul(Out1x_1, P1_instance, MASKREAD_OFF, 64);

        StateMetric0x = vsadd(StateMetric0x, BranchMetric0x, MASKREAD_OFF, 64);
        StateMetric1x = vsadd(StateMetric1x, BranchMetric1x, MASKREAD_OFF, 64);

        BranchMetric0x = vmul(Out0x_2, P2_instance, MASKREAD_OFF, 64);
        BranchMetric1x = vmul(Out1x_2, P2_instance, MASKREAD_OFF, 64);

        StateMetric0x = vsadd(StateMetric0x, BranchMetric0x, MASKREAD_OFF, 64);
        StateMetric1x = vsadd(StateMetric1x, BranchMetric1x, MASKREAD_OFF, 64);

        // 从状态index0x，index1x，输入0和1，到达状态istate时的状态量度

        vshuffle(StateMetricTemp, index_state_64, state, SHUFFLE_GATHER, 64); // 取出前一时刻的累计度量
        StateMetric0x = vsadd(StateMetric0x, StateMetricTemp, MASKREAD_OFF, 64);
        StateMetric1x = vsadd(StateMetric1x, StateMetricTemp, MASKREAD_OFF, 64);

        // 获取输入为0的幸存路径
        vshuffle(temp_a, index00, StateMetric0x, SHUFFLE_GATHER, 32); // 取出index00的累计状态度量
        vshuffle(temp_b, index01, StateMetric0x, SHUFFLE_GATHER, 32); // 取出index01的累计状态度量

        vsle(temp_b, temp_a, MASKREAD_OFF, MASKWRITE_ON, 32);
        temp_a        = vxor(temp_a, temp_a, MASKREAD_ON, 32);
        state_survive = vsadd(temp_a, temp_b, MASKREAD_ON, 32); // 得到最新state //a->b

        temp_branch    = vxor(index00_8bit, index00_8bit, MASKREAD_ON, 32);
        branch_survive = vsadd(temp_branch, index01_8bit, MASKREAD_ON, 32); // 得到最新Branch
        // 存值
        // vshuffle(Path_1, index_state_32_63, temp_zero, SHUFFLE_SCATTER, 32);
        index_state_32_63 = vsadd(index_state_32_63, 32, MASKREAD_OFF, 32);
        vshuffle(state, index_state_32_63, state_survive, SHUFFLE_SCATTER, 32);
        vshuffle(Branch_1, index_state_32_63, branch_survive, SHUFFLE_SCATTER, 32);

        // 获取输入为1的幸存路径
        vshuffle(temp_a, index00, StateMetric1x, SHUFFLE_GATHER, 32); // 取出index00的累计状态度量
        vshuffle(temp_b, index01, StateMetric1x, SHUFFLE_GATHER, 32); // 取出index01的累计状态度量

        vsle(temp_b, temp_a, MASKREAD_OFF, MASKWRITE_ON, 32);
        temp_a        = vxor(temp_a, temp_a, MASKREAD_ON, 32);
        state_survive = vsadd(temp_a, temp_b, MASKREAD_ON, 32); // 得到最新state //a->b

        temp_branch    = vxor(index00_8bit, index00_8bit, MASKREAD_ON, 32);
        branch_survive = vsadd(temp_branch, index01_8bit, MASKREAD_ON, 32); // 得到最新Branch
        // 存值
        // vshuffle(Path_1, index_state_32_63, temp_one, SHUFFLE_SCATTER, 32);
        index_state_32_63 = vsadd(index_state_32_63, 32, MASKREAD_OFF, 32);
        vshuffle(state, index_state_32_63, state_survive, SHUFFLE_SCATTER, 32);
        vshuffle(Branch_1, index_state_32_63, branch_survive, SHUFFLE_SCATTER, 32); // 第2806条

        index_state_64 = vsadd(index_state_64, 64, MASKREAD_OFF, 64); // 放后面

        // 防溢出处理
        vshuffle(state_instance, index_state_64, state, SHUFFLE_GATHER, 64);

        sort_state = vxor(state_instance, temp_255, MASKREAD_OFF, 64); // 转最小
        sort_state = vsadd(sort_state, temp_one, MASKREAD_OFF, 64);
        sort_state = vredmin8(sort_state, MASKREAD_OFF, 64);

        __asm__("addi x0, x0, 0");__asm__("addi x0, x0, 0");
        __asm__("addi x0, x0, 0");__asm__("addi x0, x0, 0");
        __asm__("addi x0, x0, 0");__asm__("addi x0, x0, 0");
        __asm__("addi x0, x0, 0");__asm__("addi x0, x0, 0");
        __asm__("addi x0, x0, 0");__asm__("addi x0, x0, 0");
        vbarrier();
        VSPM_OPEN();
        int  state_addr = vaddr(sort_state);
        char state_max  = *(volatile unsigned char *)(state_addr);
        VSPM_CLOSE();

        // vbrdcst(Max, state_max, MASKREAD_OFF, 64);
        state_instance = vsadd(state_instance, state_max, MASKREAD_OFF, 64);
        vshuffle(state, index_state_64, state_instance, SHUFFLE_SCATTER, 64);
      }
      vshuffle(state_62, index_state_64, state, SHUFFLE_GATHER, 64);
      vbrdcst(state, 0, MASKREAD_OFF, (62 + 1) * 64);
      vshuffle(state, index_64, state_62, SHUFFLE_SCATTER, 64);

      // instanceStart = 1;
      // instanceEnd = K - 62;
      vrange(index_state_32_63, 32);
      index_state_32_63 = vsadd(index_state_32_63, 32, MASKREAD_OFF, 32);
      vrange(index_state_64, 64); // 2820条
      for (int instance = 1; instance <= K - 62; instance++) {
        vbrdcst(temp_instance, instance - 1, MASKREAD_OFF, 64);

        vshuffle(P0_instance, temp_instance, P0_2, SHUFFLE_GATHER, 64);
        vshuffle(P1_instance, temp_instance, P1_2, SHUFFLE_GATHER, 64);
        vshuffle(P2_instance, temp_instance, P2_2, SHUFFLE_GATHER, 64);

        // 从状态index0x，index1x，输入0和1，到达状态istate时的分支量度
        BranchMetric0x = vmul(Out0x_0, P0_instance, MASKREAD_OFF, 64);
        BranchMetric1x = vmul(Out1x_0, P0_instance, MASKREAD_OFF, 64);

        StateMetric0x = vmul(Out0x_1, P1_instance, MASKREAD_OFF, 64);
        StateMetric1x = vmul(Out1x_1, P1_instance, MASKREAD_OFF, 64);

        StateMetric0x = vsadd(StateMetric0x, BranchMetric0x, MASKREAD_OFF, 64);
        StateMetric1x = vsadd(StateMetric1x, BranchMetric1x, MASKREAD_OFF, 64);

        BranchMetric0x = vmul(Out0x_2, P2_instance, MASKREAD_OFF, 64);
        BranchMetric1x = vmul(Out1x_2, P2_instance, MASKREAD_OFF, 64);

        StateMetric0x = vsadd(StateMetric0x, BranchMetric0x, MASKREAD_OFF, 64);
        StateMetric1x = vsadd(StateMetric1x, BranchMetric1x, MASKREAD_OFF, 64);

        // 从状态index0x，index1x，输入0和1，到达状态istate时的状态量度

        vshuffle(StateMetricTemp, index_state_64, state, SHUFFLE_GATHER, 64); // 取出前一时刻的累计度量
        StateMetric0x = vsadd(StateMetric0x, StateMetricTemp, MASKREAD_OFF, 64);
        StateMetric1x = vsadd(StateMetric1x, StateMetricTemp, MASKREAD_OFF, 64);

        // 获取输入为0的幸存路径
        vshuffle(temp_a, index00, StateMetric0x, SHUFFLE_GATHER, 32); // 取出index00的累计状态度量
        vshuffle(temp_b, index01, StateMetric0x, SHUFFLE_GATHER, 32); // 取出index01的累计状态度量

        vsle(temp_b, temp_a, MASKREAD_OFF, MASKWRITE_ON, 32);
        temp_a        = vxor(temp_a, temp_a, MASKREAD_ON, 32);
        state_survive = vsadd(temp_a, temp_b, MASKREAD_ON, 32); // 得到最新state

        temp_branch    = vxor(index00_8bit, index00_8bit, MASKREAD_ON, 32);
        branch_survive = vsadd(temp_branch, index01_8bit, MASKREAD_ON, 32); // 得到最新Branch
        // 存值
        // vshuffle(Path_2, index_state_32, temp_zero, SHUFFLE_SCATTER, 32);
        index_state_32_63 = vsadd(index_state_32_63, 32, MASKREAD_OFF, 32);
        vshuffle(state, index_state_32_63, state_survive, SHUFFLE_SCATTER, 32);
        vshuffle(Branch_2, index_state_32_63, branch_survive, SHUFFLE_SCATTER, 32);

        // 获取输入为1的幸存路径
        vshuffle(temp_a, index00, StateMetric1x, SHUFFLE_GATHER, 32); // 取出index00的累计状态度量
        vshuffle(temp_b, index01, StateMetric1x, SHUFFLE_GATHER, 32); // 取出index01的累计状态度量

        vsle(temp_b, temp_a, MASKREAD_OFF, MASKWRITE_ON, 32);
        temp_a        = vxor(temp_a, temp_a, MASKREAD_ON, 32);
        state_survive = vsadd(temp_a, temp_b, MASKREAD_ON, 32); // 得到最新state

        temp_branch    = vxor(index00_8bit, index00_8bit, MASKREAD_ON, 32);
        branch_survive = vsadd(temp_branch, index01_8bit, MASKREAD_ON, 32); // 得到最新Branch
        // 存值
        // vshuffle(Path_2, index_state_32_63, temp_one, SHUFFLE_SCATTER, 32);
        index_state_32_63 = vsadd(index_state_32_63, 32, MASKREAD_OFF, 32);
        vshuffle(state, index_state_32_63, state_survive, SHUFFLE_SCATTER, 32);
        vshuffle(Branch_2, index_state_32_63, branch_survive, SHUFFLE_SCATTER, 32);

        index_state_64 = vsadd(index_state_64, 64, MASKREAD_OFF, 64); // 放后面

        // 防溢出处理
        vshuffle(state_instance, index_state_64, state, SHUFFLE_GATHER, 64);

        sort_state = vxor(state_instance, temp_255, MASKREAD_OFF, 64); // 转最小
        sort_state = vsadd(sort_state, temp_one, MASKREAD_OFF, 64);
        sort_state = vredmin8(sort_state, MASKREAD_OFF, 64);

        __asm__("addi x0, x0, 0");__asm__("addi x0, x0, 0");
        __asm__("addi x0, x0, 0");__asm__("addi x0, x0, 0");
        __asm__("addi x0, x0, 0");__asm__("addi x0, x0, 0");
        __asm__("addi x0, x0, 0");__asm__("addi x0, x0, 0");
        __asm__("addi x0, x0, 0");__asm__("addi x0, x0, 0");
        vbarrier();
        VSPM_OPEN();
        int  state_addr = vaddr(sort_state);
        char state_max  = *(volatile unsigned char *)(state_addr);
        VSPM_CLOSE();

        vbrdcst(Max, state_max, MASKREAD_OFF, 64);
        state_instance = vsadd(state_instance, Max, MASKREAD_OFF, 64);
        vshuffle(state, index_state_64, state_instance, SHUFFLE_SCATTER, 64);
      }

      if (iRun != iternum) {
        vshuffle(state_K62, index_state_64, state, SHUFFLE_GATHER, 64);
        vbrdcst(state, 0, MASKREAD_OFF, (62 + 1) * 64);
        vshuffle(state, index_64, state_K62, SHUFFLE_SCATTER, 64);
      }
    }

    // Branch_1 = vsadd(Branch_1,0,MASKREAD_OFF,(62 + 1)*64);
    // Branch_2 = vsadd(Branch_2,0,MASKREAD_OFF,(62 + 1)*64);
    // 回溯路径
    vshuffle(state_K62, index_state_64, state, SHUFFLE_GATHER, 64);
    // state_K62 = vmul(state_K62, temp_None, MASKREAD_OFF);
    state_K62 = vxor(state_K62, temp_255, MASKREAD_OFF, 64);
    state_K62 = vsadd(state_K62, temp_one, MASKREAD_OFF, 64);
    for (int ccc = 0; ccc < 5; ccc++) {

      sort_state = vredmin8(state_K62, MASKREAD_OFF, 64);

      __asm__("addi x0, x0, 0");__asm__("addi x0, x0, 0");
      __asm__("addi x0, x0, 0");__asm__("addi x0, x0, 0");
      __asm__("addi x0, x0, 0");__asm__("addi x0, x0, 0");
      __asm__("addi x0, x0, 0");__asm__("addi x0, x0, 0");
      __asm__("addi x0, x0, 0");__asm__("addi x0, x0, 0");
      vbarrier();
      VSPM_OPEN();
      int  state_addr = vaddr(sort_state);
      char state_max  = *(volatile unsigned char *)(state_addr);
      VSPM_CLOSE();

      vbrdcst(Max, state_max, MASKREAD_OFF, 64);

      vseq(Max, state_K62, MASKREAD_OFF, MASKWRITE_ON, 64);
      temp_index_out = vsadd(temp8bit_0, range64_8bit, MASKREAD_ON, 64);


      temp_index_out = vmul(temp_index_out, -1, MASKREAD_OFF, 64);
      temp_index_out = vredmin8(temp_index_out, MASKREAD_OFF, 64);
      temp_index_out = vmul(temp_index_out, -1, MASKREAD_OFF, 1);

      __asm__("addi x0, x0, 0");__asm__("addi x0, x0, 0");
      __asm__("addi x0, x0, 0");__asm__("addi x0, x0, 0");
      __asm__("addi x0, x0, 0");__asm__("addi x0, x0, 0");
      __asm__("addi x0, x0, 0");__asm__("addi x0, x0, 0");
      __asm__("addi x0, x0, 0");__asm__("addi x0, x0, 0");
      vbarrier();
      VSPM_OPEN();
      int index_max_addr = vaddr(temp_index_out);
      int istate         = *(volatile unsigned char *)(index_max_addr);
      VSPM_CLOSE();

      __asm__("addi x0, x0, 0");__asm__("addi x0, x0, 0");
      __asm__("addi x0, x0, 0");__asm__("addi x0, x0, 0");
      __asm__("addi x0, x0, 0");__asm__("addi x0, x0, 0");
      __asm__("addi x0, x0, 0");__asm__("addi x0, x0, 0");
      __asm__("addi x0, x0, 0");__asm__("addi x0, x0, 0");
      vbarrier();
      VSPM_OPEN();
      int state_K62_addr = vaddr(state_K62);
       *(volatile unsigned char *)(state_K62_addr + istate) = 127;
      VSPM_CLOSE();


      for (int instance = K - 62; instance > 0; instance--) {
        vbrdcst(index_Out, instance + 62 - 1, MASKREAD_OFF, 1);
        if (istate < 32) {
          temp_zero = vsadd(temp_zero, 0, MASKREAD_OFF, 2048);
          index_Out = vsadd(index_Out, 0, MASKREAD_OFF, 2048);
          vshuffle(Out, index_Out, temp_zero, SHUFFLE_SCATTER, 1);
          Out = vsadd(Out, 0, MASKREAD_OFF, 2048);
        } else {
          temp_one = vsadd(temp_one, 0, MASKREAD_OFF, 2048);
          index_Out = vsadd(index_Out, 0, MASKREAD_OFF, 2048);
          vshuffle(Out, index_Out, temp_one, SHUFFLE_SCATTER, 1);
          Out = vsadd(Out, 0, MASKREAD_OFF, 2048);
        }
        if (instance == K - 62) {
          Tail_state = istate;
        }
        vbrdcst(index_Branch1, instance * 64 + istate, MASKREAD_OFF, 1);
        vshuffle(temp_Branch1, index_Branch1, Branch_2, SHUFFLE_GATHER, 1);

        __asm__("addi x0, x0, 0");__asm__("addi x0, x0, 0");
        __asm__("addi x0, x0, 0");__asm__("addi x0, x0, 0");
        __asm__("addi x0, x0, 0");__asm__("addi x0, x0, 0");
        __asm__("addi x0, x0, 0");__asm__("addi x0, x0, 0");
        __asm__("addi x0, x0, 0");__asm__("addi x0, x0, 0");
        vbarrier();
        VSPM_OPEN();
        int temp_Branch1_addr = vaddr(temp_Branch1);
        istate                = *(volatile unsigned char *)(temp_Branch1_addr);
        VSPM_CLOSE();
        // printf("%d ",&istate);//
      }
      for (int instance = 62; instance > 0; instance--) {
        vbrdcst(index_Out, instance - 1, MASKREAD_OFF, 1);
        if (istate < 32) {
          vshuffle(Out, index_Out, temp_zero, SHUFFLE_SCATTER, 1);
        } else {
          vshuffle(Out, index_Out, temp_one, SHUFFLE_SCATTER, 1);
        }

        vbrdcst(index_Branch1, instance * 64 + istate, MASKREAD_OFF, 1);
        vshuffle(temp_Branch1, index_Branch1, Branch_1, SHUFFLE_GATHER, 1);

        __asm__("addi x0, x0, 0");__asm__("addi x0, x0, 0");
        __asm__("addi x0, x0, 0");__asm__("addi x0, x0, 0");
        __asm__("addi x0, x0, 0");__asm__("addi x0, x0, 0");
        __asm__("addi x0, x0, 0");__asm__("addi x0, x0, 0");
        __asm__("addi x0, x0, 0");__asm__("addi x0, x0, 0");
        vbarrier();
        VSPM_OPEN();
        int temp_Branch2_addr = vaddr(temp_Branch1);
        istate                = *(volatile unsigned char *)(temp_Branch2_addr);
        VSPM_CLOSE();
        // printf("%d ",&istate);//
      }
      if (istate == Tail_state) {
        Flag = 1;
        break;
      }
    }
  }
  // short a = 123;
  // printf("%hd",&a);
  Out = vsadd(Out, 0, MASKREAD_OFF, 40);

  vreturn(Out, 40);
}