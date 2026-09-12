/*
 * @Author: shenyihao shenyihao@shu.edu.cn
 * @Date: 2025-03-13 22:30:21
 * @LastEditors: shenyihao shenyihao@shu.edu.cn
 * @LastEditTime: 2025-03-14 13:10:16
 * @FilePath: /VEMU/AceEcho/tasks/ltePBCH/Task_ltePBCHDescramble.c
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */

/**
 * ****************************************
 * @file        Task_ltePCFICHDescramble.c
 * @brief       PDSCH Descramble
 * @author      yuanfeng
 * @date        2024.7.28
 * @copyright   ACE-Lab(Shanghai University)
 * ****************************************
 */

#include "data_type.h"
#include "riscv_printf.h"
#include "venus.h"

typedef short __v2048i16 __attribute__((ext_vector_type(2048)));
typedef char __v4096i8 __attribute__((ext_vector_type(4096)));

int Task_ltePCFICHDescramble(__v4096i8 demod,__v4096i8 scrambleseq, short_struct demod_length)
{
  int length = demod_length.data;
  __v4096i8 seq_choose;
  vclaim(seq_choose);
  __v2048i16 nfmodindex;
  vclaim(nfmodindex);
  vrange(nfmodindex,length);
  vshuffle(seq_choose,nfmodindex,scrambleseq,SHUFFLE_GATHER,length);

  __v4096i8 pbchllr;
  pbchllr = vmul(demod, seq_choose, MASKREAD_OFF, length);

  vreturn(pbchllr, length);
}
