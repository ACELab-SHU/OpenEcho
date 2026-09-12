/**
 * ****************************************
 * @file        Task_nrPBCHIndices.c
 * @brief       generate pbch indices
 * @author      yuanfeng
 * @date        2024.6.6
 * @copyright   ACE-Lab(Shanghai University)
 * ****************************************
 */

#include "riscv_printf.h"
#include "venus.h"

typedef short __v2048i16 __attribute__((ext_vector_type(2048)));
typedef short __v4096i16 __attribute__((ext_vector_type(4096)));
typedef char  __v4096i8 __attribute__((ext_vector_type(4096)));

// short input_dmrs_index[144] = {
//     0,   4,   8,   12,  16,  20,  24,  28,  32,  36,  40,  44,  48,  52,  56,
//     60,  64,  68,  72,  76,  80,  84,  88,  92,  96,  100, 104, 108, 112, 116,
//     120, 124, 128, 132, 136, 140, 144, 148, 152, 156, 160, 164, 168, 172, 176,
//     180, 184, 188, 192, 196, 200, 204, 208, 212, 216, 220, 224, 228, 232, 236,
//     240, 244, 248, 252, 256, 260, 264, 268, 272, 276, 280, 284, 432, 436, 440,
//     444, 448, 452, 456, 460, 464, 468, 472, 476, 480, 484, 488, 492, 496, 500,
//     504, 508, 512, 516, 520, 524, 528, 532, 536, 540, 544, 548, 552, 556, 560,
//     564, 568, 572, 576, 580, 584, 588, 592, 596, 600, 604, 608, 612, 616, 620,
//     624, 628, 632, 636, 640, 644, 648, 652, 656, 660, 664, 668, 672, 676, 680,
//     684, 688, 692, 696, 700, 704, 708, 712, 716};

VENUS_INLINE __v2048i16 pbch_index_generate(__v2048i16 dmrs_index, short ncellid) {
  __v2048i16 pbch_index_shuffle_index;
  vclaim(pbch_index_shuffle_index);
  vrange(pbch_index_shuffle_index, 144);
  pbch_index_shuffle_index = vmul(pbch_index_shuffle_index, 3, MASKREAD_OFF, 144);

  __v2048i16 pbch_index;
  __v2048i16 dmrs_index_tmp;
  vclaim(pbch_index);
  // The ABI returns all 2048 elements; only the first 432 carry indices.
  vbrdcst(pbch_index, 0, MASKREAD_OFF, 2048);
  switch (ncellid % 4) {
  case 0: // 1 2 3 (v = 0)
    // 1
    dmrs_index_tmp = vsadd(dmrs_index, 1, MASKREAD_OFF, 144);
    vshuffle(pbch_index, pbch_index_shuffle_index, dmrs_index_tmp, SHUFFLE_SCATTER, 144);
    // 2
    dmrs_index_tmp           = vsadd(dmrs_index, 2, MASKREAD_OFF, 144);
    pbch_index_shuffle_index = vsadd(pbch_index_shuffle_index, 1, MASKREAD_OFF, 144);
    vshuffle(pbch_index, pbch_index_shuffle_index, dmrs_index_tmp, SHUFFLE_SCATTER, 144);
    // 3
    dmrs_index_tmp           = vsadd(dmrs_index, 3, MASKREAD_OFF, 144);
    pbch_index_shuffle_index = vsadd(pbch_index_shuffle_index, 1, MASKREAD_OFF, 144);
    vshuffle(pbch_index, pbch_index_shuffle_index, dmrs_index_tmp, SHUFFLE_SCATTER, 144);
    break;
  case 1: // 0 2 3 (v = 1)
    // 0
    vshuffle(pbch_index, pbch_index_shuffle_index, dmrs_index, SHUFFLE_SCATTER, 144);
    // 2
    dmrs_index_tmp           = vsadd(dmrs_index, 2, MASKREAD_OFF, 144);
    pbch_index_shuffle_index = vsadd(pbch_index_shuffle_index, 1, MASKREAD_OFF, 144);
    vshuffle(pbch_index, pbch_index_shuffle_index, dmrs_index_tmp, SHUFFLE_SCATTER, 144);
    // 3
    dmrs_index_tmp           = vsadd(dmrs_index, 3, MASKREAD_OFF, 144);
    pbch_index_shuffle_index = vsadd(pbch_index_shuffle_index, 1, MASKREAD_OFF, 144);
    vshuffle(pbch_index, pbch_index_shuffle_index, dmrs_index_tmp, SHUFFLE_SCATTER, 144);
    break;
  case 2: // 0 1 3 (v = 2)
    // 0
    vshuffle(pbch_index, pbch_index_shuffle_index, dmrs_index, SHUFFLE_SCATTER, 144);
    // 1
    dmrs_index_tmp           = vsadd(dmrs_index, 1, MASKREAD_OFF, 144);
    pbch_index_shuffle_index = vsadd(pbch_index_shuffle_index, 1, MASKREAD_OFF, 144);
    vshuffle(pbch_index, pbch_index_shuffle_index, dmrs_index_tmp, SHUFFLE_SCATTER, 144);
    // 3
    dmrs_index_tmp           = vsadd(dmrs_index, 3, MASKREAD_OFF, 144);
    pbch_index_shuffle_index = vsadd(pbch_index_shuffle_index, 1, MASKREAD_OFF, 144);
    vshuffle(pbch_index, pbch_index_shuffle_index, dmrs_index_tmp, SHUFFLE_SCATTER, 144);
    break;
  case 3: // 0 1 2 (v = 3)
    // 0
    vshuffle(pbch_index, pbch_index_shuffle_index, dmrs_index, SHUFFLE_SCATTER, 144);
    // 1
    dmrs_index_tmp           = vsadd(dmrs_index, 1, MASKREAD_OFF, 144);
    pbch_index_shuffle_index = vsadd(pbch_index_shuffle_index, 1, MASKREAD_OFF, 144);
    vshuffle(pbch_index, pbch_index_shuffle_index, dmrs_index_tmp, SHUFFLE_SCATTER, 144);
    // 2
    dmrs_index_tmp           = vsadd(dmrs_index, 2, MASKREAD_OFF, 144);
    pbch_index_shuffle_index = vsadd(pbch_index_shuffle_index, 1, MASKREAD_OFF, 144);
    vshuffle(pbch_index, pbch_index_shuffle_index, dmrs_index_tmp, SHUFFLE_SCATTER, 144);
    break;
  default: // 1 2 3 (v = 0)
    // 1
    dmrs_index_tmp = vsadd(dmrs_index, 1, MASKREAD_OFF, 144);
    vshuffle(pbch_index, pbch_index_shuffle_index, dmrs_index_tmp, SHUFFLE_SCATTER, 144);
    // 2
    dmrs_index_tmp           = vsadd(dmrs_index, 2, MASKREAD_OFF, 144);
    pbch_index_shuffle_index = vsadd(pbch_index_shuffle_index, 1, MASKREAD_OFF, 144);
    vshuffle(pbch_index, pbch_index_shuffle_index, dmrs_index_tmp, SHUFFLE_SCATTER, 144);
    // 3
    dmrs_index_tmp           = vsadd(dmrs_index, 3, MASKREAD_OFF, 144);
    pbch_index_shuffle_index = vsadd(pbch_index_shuffle_index, 1, MASKREAD_OFF, 144);
    vshuffle(pbch_index, pbch_index_shuffle_index, dmrs_index_tmp, SHUFFLE_SCATTER, 144);
    break;
  }
  return pbch_index;
}

typedef struct {
  short data;
} __attribute__((aligned(64))) short_struct;

int Task_nrPBCHIndices(__v2048i16 init_dmrs_index, short_struct input_ncellid) __attribute__((aligned(64))) {
  short ncellid = input_ncellid.data;

  __v2048i16 pbch_index;
  pbch_index = pbch_index_generate(init_dmrs_index, ncellid);

  vreturn(pbch_index, sizeof(pbch_index)); // this is bytes number!
}