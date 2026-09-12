
/**
 * ****************************************
 * @file        Task_nrPBCHDMRSIndices.c
 * @brief       generate pbch dmrs indices
 * @author      yuanfeng
 * @date        2024.6.6
 * @copyright   ACE-Lab(Shanghai University)
 * ****************************************
 */

#include "riscv_printf.h"
#include "venus.h"

#define DMRS_INDEX_LENGTH 144

typedef short __v2048i16 __attribute__((ext_vector_type(2048)));
typedef short __v4096i16 __attribute__((ext_vector_type(4096)));
typedef char  __v4096i8 __attribute__((ext_vector_type(4096)));

// struct Task_nrPBCHDMRSIndices_input {
//   short ncellid;
// } __attribute__((aligned(64)));

typedef struct {
  short data;
} __attribute__((aligned(64))) short_struct;

int Task_nrPBCHDMRSIndices(__v2048i16 init_dmrs_index, short_struct input_ncellid) __attribute__((aligned(64))) {
  short v = input_ncellid.data % 4;
  // short v = 0;
  __v2048i16 dmrs_index;
  dmrs_index = vsadd(init_dmrs_index, v, MASKREAD_OFF, DMRS_INDEX_LENGTH);

  //   short L_max = 4;
  //   if (L_max == 4) {
  //     v = ibar_SSB % 4;
  //   } else {
  //     v = ibar_SSB;
  //   }
  vreturn(dmrs_index, sizeof(dmrs_index));
}