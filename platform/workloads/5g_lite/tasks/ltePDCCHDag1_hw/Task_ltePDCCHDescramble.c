/**
 * ****************************************
 * @file        Task_ltePDCCHDescramble.c
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
typedef short __v4096i16 __attribute__((ext_vector_type(4096)));
typedef char __v4096i8 __attribute__((ext_vector_type(4096)));

int Task_ltePDCCHDescramble(__v4096i8 demod,__v4096i8 scrambleseq, short_struct demod_length)
{
  
  int length = demod_length.data;
scrambleseq = vsadd(scrambleseq, 0, MASKREAD_OFF, length);
  __v4096i8 seq_choose;
  vclaim(seq_choose);
  __v4096i16 nfmodindex;
  vclaim(nfmodindex);
  vrange(nfmodindex,length);
  vshuffle(seq_choose,nfmodindex,scrambleseq,SHUFFLE_GATHER,length);

  __v4096i8 pbchllr;
  pbchllr = vmul(demod, seq_choose, MASKREAD_OFF, length);

  vreturn(pbchllr, length);
}
