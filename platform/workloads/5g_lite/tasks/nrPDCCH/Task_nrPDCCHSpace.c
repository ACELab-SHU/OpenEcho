#include "data_type.h"
#include "riscv_printf.h"
#include "venus.h"

typedef short __v2048i16 __attribute__((ext_vector_type(2048)));
typedef char  __v4096i8 __attribute__((ext_vector_type(4096)));

#define VENUS_PRINTVEC_CHAR(name, len)                                                                                 \
  do {                                                                                                                 \
    short array_##name[len];                                                                                           \
    int   vecaddr_##name = vaddr(name);                                                                                \
    VSPM_OPEN();                                                                                                       \
    vbarrier();                                                                                                        \
    for (int _____ = 0; _____ < len; _____++) {                                                                        \
      array_##name[_____] = *(volatile unsigned char *)(vecaddr_##name + _____);                                       \
      printf("%hd\n", &array_##name[_____]);                                                                           \
    }                                                                                                                  \
    VSPM_CLOSE();                                                                                                      \
  } while (0)

#define INF 0x7F

uint16_t aggLvls[5] = {1, 2, 4, 8, 16};
// uint16_t _symIdxPerRB[9]  = {0, 2, 3, 4, 6, 7, 8, 10, 11};
// uint16_t _dmrsIdxPerRB[3] = {1, 5, 9};
#define recpSqrt2 0.70710678

#define tabLength 31

VENUS_INLINE __v4096i8
nrPRBS(__v4096i8 seq1_vec, __v4096i8 init2_vec, __v2048i16 seq2_init_table_0_vec, __v2048i16 seq2_init_table_1_vec,
       __v2048i16 seq2_init_table_2_vec, __v2048i16 seq2_init_table_3_vec, __v2048i16 seq2_init_table_4_vec,
       __v2048i16 seq2_init_table_5_vec, __v2048i16 seq2_init_table_6_vec, __v2048i16 seq2_init_table_7_vec,
       __v2048i16 seq2_init_table_8_vec, __v2048i16 seq2_init_table_9_vec, __v2048i16 seq2_init_table_10_vec,
       __v2048i16 seq2_init_table_11_vec, __v2048i16 seq2_init_table_12_vec, __v2048i16 seq2_init_table_13_vec,
       __v2048i16 seq2_init_table_14_vec, __v2048i16 seq2_init_table_15_vec, __v2048i16 seq2_init_table_16_vec,
       __v2048i16 seq2_init_table_17_vec, __v2048i16 seq2_init_table_18_vec, __v2048i16 seq2_init_table_19_vec,
       __v2048i16 seq2_trans_table_0_vec, __v2048i16 seq2_trans_table_1_vec, __v2048i16 seq2_trans_table_2_vec,
       __v2048i16 seq2_trans_table_3_vec, __v2048i16 seq2_trans_table_4_vec, __v2048i16 seq2_trans_table_5_vec,
       __v2048i16 seq2_trans_table_6_vec, int sequenceLength) {
  /*--------------------DMRS GENERATION--------------------*/
  // gengrate init 31 seq
  __v4096i8 seq2_init_table_0_tmp;
  vclaim(seq2_init_table_0_tmp);
  vshuffle(seq2_init_table_0_tmp, seq2_init_table_0_vec, init2_vec, SHUFFLE_GATHER, tabLength);
  __v4096i8 seq2_init_table_1_tmp;
  vclaim(seq2_init_table_1_tmp);
  vshuffle(seq2_init_table_1_tmp, seq2_init_table_1_vec, init2_vec, SHUFFLE_GATHER, tabLength);
  __v4096i8 seq2_init_table_2_tmp;
  vclaim(seq2_init_table_2_tmp);
  vshuffle(seq2_init_table_2_tmp, seq2_init_table_2_vec, init2_vec, SHUFFLE_GATHER, tabLength);
  __v4096i8 seq2_init_table_3_tmp;
  vclaim(seq2_init_table_3_tmp);
  vshuffle(seq2_init_table_3_tmp, seq2_init_table_3_vec, init2_vec, SHUFFLE_GATHER, tabLength);
  __v4096i8 seq2_init_table_4_tmp;
  vclaim(seq2_init_table_4_tmp);
  vshuffle(seq2_init_table_4_tmp, seq2_init_table_4_vec, init2_vec, SHUFFLE_GATHER, tabLength);
  __v4096i8 seq2_init_table_5_tmp;
  vclaim(seq2_init_table_5_tmp);
  vshuffle(seq2_init_table_5_tmp, seq2_init_table_5_vec, init2_vec, SHUFFLE_GATHER, tabLength);
  __v4096i8 seq2_init_table_6_tmp;
  vclaim(seq2_init_table_6_tmp);
  vshuffle(seq2_init_table_6_tmp, seq2_init_table_6_vec, init2_vec, SHUFFLE_GATHER, tabLength);
  __v4096i8 seq2_init_table_7_tmp;
  vclaim(seq2_init_table_7_tmp);
  vshuffle(seq2_init_table_7_tmp, seq2_init_table_7_vec, init2_vec, SHUFFLE_GATHER, tabLength);
  __v4096i8 seq2_init_table_8_tmp;
  vclaim(seq2_init_table_8_tmp);
  vshuffle(seq2_init_table_8_tmp, seq2_init_table_8_vec, init2_vec, SHUFFLE_GATHER, tabLength);
  __v4096i8 seq2_init_table_9_tmp;
  vclaim(seq2_init_table_9_tmp);
  vshuffle(seq2_init_table_9_tmp, seq2_init_table_9_vec, init2_vec, SHUFFLE_GATHER, tabLength);
  __v4096i8 seq2_init_table_10_tmp;
  vclaim(seq2_init_table_10_tmp);
  vshuffle(seq2_init_table_10_tmp, seq2_init_table_10_vec, init2_vec, SHUFFLE_GATHER, tabLength);
  __v4096i8 seq2_init_table_11_tmp;
  vclaim(seq2_init_table_11_tmp);
  vshuffle(seq2_init_table_11_tmp, seq2_init_table_11_vec, init2_vec, SHUFFLE_GATHER, tabLength);
  __v4096i8 seq2_init_table_12_tmp;
  vclaim(seq2_init_table_12_tmp);
  vshuffle(seq2_init_table_12_tmp, seq2_init_table_12_vec, init2_vec, SHUFFLE_GATHER, tabLength);
  __v4096i8 seq2_init_table_13_tmp;
  vclaim(seq2_init_table_13_tmp);
  vshuffle(seq2_init_table_13_tmp, seq2_init_table_13_vec, init2_vec, SHUFFLE_GATHER, tabLength);
  __v4096i8 seq2_init_table_14_tmp;
  vclaim(seq2_init_table_14_tmp);
  vshuffle(seq2_init_table_14_tmp, seq2_init_table_14_vec, init2_vec, SHUFFLE_GATHER, tabLength);
  __v4096i8 seq2_init_table_15_tmp;
  vclaim(seq2_init_table_15_tmp);
  vshuffle(seq2_init_table_15_tmp, seq2_init_table_15_vec, init2_vec, SHUFFLE_GATHER, tabLength);
  __v4096i8 seq2_init_table_16_tmp;
  vclaim(seq2_init_table_16_tmp);
  vshuffle(seq2_init_table_16_tmp, seq2_init_table_16_vec, init2_vec, SHUFFLE_GATHER, tabLength);
  __v4096i8 seq2_init_table_17_tmp;
  vclaim(seq2_init_table_17_tmp);
  vshuffle(seq2_init_table_17_tmp, seq2_init_table_17_vec, init2_vec, SHUFFLE_GATHER, tabLength);
  __v4096i8 seq2_init_table_18_tmp;
  vclaim(seq2_init_table_18_tmp);
  vshuffle(seq2_init_table_18_tmp, seq2_init_table_18_vec, init2_vec, SHUFFLE_GATHER, tabLength);
  __v4096i8 seq2_init_table_19_tmp;
  vclaim(seq2_init_table_19_tmp);
  vshuffle(seq2_init_table_19_tmp, seq2_init_table_19_vec, init2_vec, SHUFFLE_GATHER, tabLength);
  __v4096i8 seq2_tmp;
  vclaim(seq2_tmp);
  vbrdcst(seq2_tmp, 0, MASKREAD_OFF, tabLength + 1);
  seq2_tmp = vxor(seq2_init_table_0_tmp, seq2_init_table_1_tmp, MASKREAD_OFF, tabLength);
  seq2_tmp = vxor(seq2_tmp, seq2_init_table_2_tmp, MASKREAD_OFF, tabLength);
  seq2_tmp = vxor(seq2_tmp, seq2_init_table_3_tmp, MASKREAD_OFF, tabLength);
  seq2_tmp = vxor(seq2_tmp, seq2_init_table_4_tmp, MASKREAD_OFF, tabLength);
  seq2_tmp = vxor(seq2_tmp, seq2_init_table_5_tmp, MASKREAD_OFF, tabLength);
  seq2_tmp = vxor(seq2_tmp, seq2_init_table_6_tmp, MASKREAD_OFF, tabLength);
  seq2_tmp = vxor(seq2_tmp, seq2_init_table_7_tmp, MASKREAD_OFF, tabLength);
  seq2_tmp = vxor(seq2_tmp, seq2_init_table_8_tmp, MASKREAD_OFF, tabLength);
  seq2_tmp = vxor(seq2_tmp, seq2_init_table_9_tmp, MASKREAD_OFF, tabLength);
  seq2_tmp = vxor(seq2_tmp, seq2_init_table_10_tmp, MASKREAD_OFF, tabLength);
  seq2_tmp = vxor(seq2_tmp, seq2_init_table_11_tmp, MASKREAD_OFF, tabLength);
  seq2_tmp = vxor(seq2_tmp, seq2_init_table_12_tmp, MASKREAD_OFF, tabLength);
  seq2_tmp = vxor(seq2_tmp, seq2_init_table_13_tmp, MASKREAD_OFF, tabLength);
  seq2_tmp = vxor(seq2_tmp, seq2_init_table_14_tmp, MASKREAD_OFF, tabLength);
  seq2_tmp = vxor(seq2_tmp, seq2_init_table_15_tmp, MASKREAD_OFF, tabLength);
  seq2_tmp = vxor(seq2_tmp, seq2_init_table_16_tmp, MASKREAD_OFF, tabLength);
  seq2_tmp = vxor(seq2_tmp, seq2_init_table_17_tmp, MASKREAD_OFF, tabLength);
  seq2_tmp = vxor(seq2_tmp, seq2_init_table_18_tmp, MASKREAD_OFF, tabLength);
  seq2_tmp = vxor(seq2_tmp, seq2_init_table_19_tmp, MASKREAD_OFF, tabLength);

  __v2048i16 seq2_shuffle_index;
  vclaim(seq2_shuffle_index);
  vrange(seq2_shuffle_index, tabLength);

  __v4096i8 seq2_vec;
  vclaim(seq2_vec);
  vshuffle(seq2_vec, seq2_shuffle_index, seq2_tmp, SHUFFLE_SCATTER, tabLength);

  // gengrate the rest seqs
  __v4096i8 seq2_trans_0_tmp;
  __v4096i8 seq2_trans_1_tmp;
  __v4096i8 seq2_trans_2_tmp;
  __v4096i8 seq2_trans_3_tmp;
  __v4096i8 seq2_trans_4_tmp;
  __v4096i8 seq2_trans_5_tmp;
  __v4096i8 seq2_trans_6_tmp;
  vclaim(seq2_trans_0_tmp);
  vclaim(seq2_trans_1_tmp);
  vclaim(seq2_trans_2_tmp);
  vclaim(seq2_trans_3_tmp);
  vclaim(seq2_trans_4_tmp);
  vclaim(seq2_trans_5_tmp);
  vclaim(seq2_trans_6_tmp);

  for (int i = 1; i < ((sequenceLength + tabLength - 1) / tabLength); i++) {
    vshuffle(seq2_trans_0_tmp, seq2_trans_table_0_vec, seq2_tmp, SHUFFLE_GATHER, tabLength);
    vshuffle(seq2_trans_1_tmp, seq2_trans_table_1_vec, seq2_tmp, SHUFFLE_GATHER, tabLength);
    vshuffle(seq2_trans_2_tmp, seq2_trans_table_2_vec, seq2_tmp, SHUFFLE_GATHER, tabLength);
    vshuffle(seq2_trans_3_tmp, seq2_trans_table_3_vec, seq2_tmp, SHUFFLE_GATHER, tabLength);
    vshuffle(seq2_trans_4_tmp, seq2_trans_table_4_vec, seq2_tmp, SHUFFLE_GATHER, tabLength);
    vshuffle(seq2_trans_5_tmp, seq2_trans_table_5_vec, seq2_tmp, SHUFFLE_GATHER, tabLength);
    vshuffle(seq2_trans_6_tmp, seq2_trans_table_6_vec, seq2_tmp, SHUFFLE_GATHER, tabLength);
    vbrdcst(seq2_tmp, 0, MASKREAD_OFF, tabLength);
    seq2_tmp           = vxor(seq2_trans_0_tmp, seq2_trans_1_tmp, MASKREAD_OFF, tabLength);
    seq2_tmp           = vxor(seq2_tmp, seq2_trans_2_tmp, MASKREAD_OFF, tabLength);
    seq2_tmp           = vxor(seq2_tmp, seq2_trans_3_tmp, MASKREAD_OFF, tabLength);
    seq2_tmp           = vxor(seq2_tmp, seq2_trans_4_tmp, MASKREAD_OFF, tabLength);
    seq2_tmp           = vxor(seq2_tmp, seq2_trans_5_tmp, MASKREAD_OFF, tabLength);
    seq2_tmp           = vxor(seq2_tmp, seq2_trans_6_tmp, MASKREAD_OFF, tabLength);
    seq2_shuffle_index = vsadd(seq2_shuffle_index, tabLength, MASKREAD_OFF, tabLength);
    vshuffle(seq2_vec, seq2_shuffle_index, seq2_tmp, SHUFFLE_SCATTER, tabLength);
  }

  // generate dmrs
  __v4096i8 seq;
  seq = vxor(seq1_vec, seq2_vec, MASKREAD_OFF, sequenceLength);
  seq = vmul(seq, -2, MASKREAD_OFF, sequenceLength);
  seq = vsadd(seq, 1, MASKREAD_OFF, sequenceLength);
  // seq = vaddmul(seq, -2, 1, MASKREAD_OFF, sequenceLength);
  return seq;
}

int Task_nrPDCCHSpace(
    nrPDCCHConfig pdcch, nrCarrierConfig carrier, short_struct alIdx_st, short_struct cIdx_st, short_struct numCand_st,
    __v2048i16 symIdxPerRB, __v2048i16 dmrsIdxPerRB, __v4096i8 seq1_vec, __v2048i16 seq2_init_table_0_vec,
    __v2048i16 seq2_init_table_1_vec, __v2048i16 seq2_init_table_2_vec, __v2048i16 seq2_init_table_3_vec,
    __v2048i16 seq2_init_table_4_vec, __v2048i16 seq2_init_table_5_vec, __v2048i16 seq2_init_table_6_vec,
    __v2048i16 seq2_init_table_7_vec, __v2048i16 seq2_init_table_8_vec, __v2048i16 seq2_init_table_9_vec,
    __v2048i16 seq2_init_table_10_vec, __v2048i16 seq2_init_table_11_vec, __v2048i16 seq2_init_table_12_vec,
    __v2048i16 seq2_init_table_13_vec, __v2048i16 seq2_init_table_14_vec, __v2048i16 seq2_init_table_15_vec,
    __v2048i16 seq2_init_table_16_vec, __v2048i16 seq2_init_table_17_vec, __v2048i16 seq2_init_table_18_vec,
    __v2048i16 seq2_init_table_19_vec, __v2048i16 seq2_trans_table_0_vec, __v2048i16 seq2_trans_table_1_vec,
    __v2048i16 seq2_trans_table_2_vec, __v2048i16 seq2_trans_table_3_vec, __v2048i16 seq2_trans_table_4_vec,
    __v2048i16 seq2_trans_table_5_vec, __v2048i16 seq2_trans_table_6_vec) {

  uint16_t L           = pdcch.CORESET.REGBundleSize;
  uint16_t R           = pdcch.CORESET.InterleaverSize;
  uint16_t nSym        = pdcch.CORESET.Duration;
  uint16_t nssStartsym = pdcch.SearchSpace.StartSymbolWithinSlot;
  uint16_t n_shift     = pdcch.CORESET.ShiftIndex;
  uint16_t nStartBWP   = pdcch.NStartBWP;
  uint16_t nSizeBWP    = pdcch.NSizeBWP;
  uint16_t rbOffset    = nStartBWP;
  uint16_t nrb         = nSizeBWP;
  uint16_t numCCEs     = pdcch.CORESET.NCCE;

  int symbolperslot = carrier.SymbolsPerSlot;
  int nslot         = carrier.NSlot;
  int ncellid       = carrier.NCellID;

  uint16_t aggLv   = aggLvls[alIdx_st.data];
  uint16_t cIdx    = cIdx_st.data;
  uint16_t numCand = numCand_st.data;

  /////////////////////////////////////
  // getREGBundle

  // for (int i = 0; i < 8; i++)
  //   allocatedRBs += pdcch.CORESET.FrequencyResources[i];
  // allocatedRBs *= 6;
  // uint16_t numREGs = allocatedRBs * nSym;
  uint16_t numREGs = numCCEs * 6;
  uint16_t C       = numREGs / (L * R);
  uint16_t nrb0    = (pdcch.CORESET.CORESETID == 0) ? 0 : (6 * (nStartBWP / 6 + 1) - nStartBWP);

  // printf("L: %hd\n", &L);
  // printf("R: %hd\n", &R);
  // printf("nSym: %hd\n", &nSym);
  // printf("nssStartsym: %hd\n", &nssStartsym);
  // printf("n_shift: %hd\n", &n_shift);
  // printf("rbOffset: %hd\n", &rbOffset);
  // printf("nrb: %hd\n", &nrb);
  // printf("numCCEs: %hd\n", &numCCEs);
  // printf("aggLv: %hd\n", &aggLv);
  // printf("cIdx: %hd\n", &cIdx);
  // printf("numCand: %hd\n", &numCand);
  // printf("C: %hd\n", &C);
  // printf("R: %hd\n", &R);
  // get CCE-REG Mapping
  __v2048i16 f;
  vclaim(f);
  vbrdcst(f, 0, MASKREAD_OFF, C * R);

  if (pdcch.CORESET.CCEREGMapping) {
    vbarrier();
    VSPM_OPEN();
    for (int c = 0; c < C; c++) {
      for (int r = 0; r < R; r++) {
        int          f_addr                = vaddr(f);
        unsigned int addr                  = f_addr + ((c * R + r) << 1);
        *(volatile unsigned short *)(addr) = (r * C + c + n_shift) % (numREGs / L);
      }
    }
    VSPM_CLOSE();
  } else {
    L = 6;
    // vrange(f, R);
    vrange(f, numCCEs);
  }
  f = vsadd(f, 0, MASKREAD_OFF, C * R);

  // nrPDCCHIndex

  __v4096i8  dmrs_pdcch_real;
  __v4096i8  dmrs_seq_tmp;
  __v4096i8  dmrs_pdcch_imag;
  __v4096i8  dmrs_seq_reshape;
  __v2048i16 dmrs_shuffle_index;
  __v2048i16 rbCCEIdx0;
  __v2048i16 rbCCEIdx_sorted;
  vclaim(rbCCEIdx0);
  vclaim(rbCCEIdx_sorted);
  vclaim(dmrs_pdcch_real);
  vclaim(dmrs_seq_tmp);
  vclaim(dmrs_pdcch_imag);
  vclaim(dmrs_seq_reshape);
  vclaim(dmrs_shuffle_index);

  __v2048i16 pdcchIdx;
  __v2048i16 pdcchdmrsIdx;
  __v2048i16 cceIndices;
  // __v2048i16 symIdxPerRB;
  // __v2048i16 dmrsIdxPerRB;
  __v2048i16 index0;
  __v2048i16 index1;
  __v2048i16 index2;
  __v2048i16 mask;
  __v2048i16 cons;
  __v2048i16 rbIdx;
  __v2048i16 prbIdx;
  vclaim(pdcchIdx);
  vclaim(prbIdx);
  vclaim(pdcchdmrsIdx);
  // vclaim(symIdxPerRB);
  // vclaim(dmrsIdxPerRB);
  vclaim(index0);
  vclaim(index1);
  vclaim(index2);
  vclaim(mask);
  vclaim(cons);
  vclaim(rbIdx);
  symIdxPerRB = vadd(symIdxPerRB, 0, MASKREAD_OFF, 2048);
  ///////////////////////////////////////////////////////////////////////////
  /*--------------------sym and dmrs index--------------------*/
  // int symIdxPerRB_addr = vaddr(symIdxPerRB);
  // vbarrier();
  // VSPM_OPEN();
  // for (int i = 0; i < 9; i++)
  // {
  //     int addr = symIdxPerRB_addr + (i<<1);
  //     *(volatile unsigned short *)(addr) = _symIdxPerRB[i];
  // }
  // VSPM_CLOSE();

  // int dmrsIdxPerRB_addr = vaddr(dmrsIdxPerRB);
  // vbarrier();
  // VSPM_OPEN();
  // for (int i = 0; i < 3; i++)
  // {
  //     int addr = dmrsIdxPerRB_addr + (i<<1);
  //     *(volatile unsigned short *)(addr) = _dmrsIdxPerRB[i];
  // }
  // VSPM_CLOSE();

  // if (alIdx)
  // {
  vrange(cceIndices, aggLv);
  cceIndices =
      vadd(cceIndices, ((numCCEs * cIdx / (aggLv * numCand)) % (numCCEs / aggLv)) * aggLv, MASKREAD_OFF, aggLv);
  // VENUS_PRINTVEC_CHAR(cceIndices,aggLv);
  for (int i = 0; i < nSym; i++) {
    vrange(index0, numREGs / nSym);
    index1 = vmul(index0, nSym, MASKREAD_OFF, numREGs / nSym);
    index2 = vadd(index1, i, MASKREAD_OFF, numREGs / nSym);
    index0 = vadd(index0, i * numREGs / nSym, MASKREAD_OFF, numREGs / nSym);
    vshuffle(rbIdx, index2, index0, SHUFFLE_SCATTER, numREGs / nSym);
  }
  for (int i = 0; i < 6; i++) {
    vrange(index0, aggLv);
    vrange(index1, numCCEs);
    index0 = vadd(index0, i * aggLv, MASKREAD_OFF, aggLv);
    index1 = vmul(index1, 6, MASKREAD_OFF, numCCEs);
    index1 = vadd(index1, i, MASKREAD_OFF, numCCEs);
    vshuffle(index2, f, index1, SHUFFLE_GATHER, numCCEs);
    vshuffle(index1, cceIndices, index2, SHUFFLE_GATHER, aggLv);
    vshuffle(rbCCEIdx0, index0, index1, SHUFFLE_SCATTER, aggLv);
  }
  __v2048i16 rbCCEIdx;
  vclaim(rbCCEIdx);
  vshuffle(rbCCEIdx, rbCCEIdx0, rbIdx, SHUFFLE_GATHER, 6 * aggLv);

  int rbCCEIdx_offset[16];
  int rbCCEIdx_offset_index[16];
  vbarrier();
  VSPM_OPEN();
  for (int i = 0; i < aggLv; i++) {
    int rbCCEIdx_addr  = vaddr(rbCCEIdx);
    int current_addr   = rbCCEIdx_addr + (i << 1);
    int current        = *(volatile unsigned short *)(current_addr);
    rbCCEIdx_offset[i] = current;
  }
  VSPM_CLOSE();
  // int temptemp = 11;
  // printf("rbCCEIdx_offset: %d\n", &temptemp);
  // for (int i = 0; i < aggLv; i++)
  // {
  //   printf("rbCCEIdx_offset: %d\n", &rbCCEIdx_offset[i]);
  // }
  // asm("ebreak");
  for (int i = 0; i < aggLv; i++) {
    int min = INF;
    for (int j = 0; j < aggLv; j++) {
      if (rbCCEIdx_offset[j] < min) {
        rbCCEIdx_offset_index[i] = j;
        min                      = rbCCEIdx_offset[j];
      }
    }
    rbCCEIdx_offset[rbCCEIdx_offset_index[i]] = INF;
  }

  // for (int i = 0; i < aggLv; i++)
  // {
  //   printf("rbCCEIdx_offset_index: %d\n", &rbCCEIdx_offset_index[i]);
  // }
  // asm("ebreak");
  vrange(index0, 6);
  index0 = vmul(index0, aggLv, MASKREAD_OFF, 6);
  for (int i = 0; i < aggLv; i++) {
    index1 = vadd(index0, rbCCEIdx_offset_index[i], MASKREAD_OFF, 6);
    vshuffle(index2, index1, rbCCEIdx, SHUFFLE_GATHER, 6);
    vrange(index1, 6);
    index1 = vadd(index1, i * 6, MASKREAD_OFF, 6);
    vshuffle(rbCCEIdx_sorted, index1, index2, SHUFFLE_SCATTER, 6);
  }
  // VENUS_PRINTVEC_SHORT(rbCCEIdx_sorted, 6 * aggLv);

  for (int i = 0; i < nSym; i++) {
    vrange(index0, 6 * aggLv / nSym);
    index1 = vmul(index0, nSym, MASKREAD_OFF, 6 * aggLv / nSym);
    index1 = vadd(index1, i, MASKREAD_OFF, 6 * aggLv / nSym);
    vshuffle(index2, index1, rbCCEIdx_sorted, SHUFFLE_GATHER, 6 * aggLv / nSym);
    index0 = vadd(index0, i * 6 * aggLv / nSym, MASKREAD_OFF, 6 * aggLv / nSym);
    vshuffle(rbCCEIdx, index0, index2, SHUFFLE_SCATTER, 6 * aggLv / nSym);
  }

  for (int i = 0; i < 6 * aggLv; i++) {
    vrange(index0, 9);
    index0                 = vadd(index0, i * 9, MASKREAD_OFF, 9);
    int      rbCCEIdx_addr = vaddr(rbCCEIdx);
    uint16_t idx;
    vbarrier();
    VSPM_OPEN();
    idx = *(volatile unsigned short *)(rbCCEIdx_addr + (i << 1));
    VSPM_CLOSE();
    // printf("idx: %hd\n", &idx);
    index1 = vadd(symIdxPerRB, 12 * idx, MASKREAD_OFF, 9);
    vshuffle(pdcchIdx, index0, index1, SHUFFLE_SCATTER, 9);

    vrange(index0, 3);
    index0 = vadd(index0, i * 3, MASKREAD_OFF, 3);
    index1 = vadd(dmrsIdxPerRB, 12 * idx, MASKREAD_OFF, 3);
    vshuffle(pdcchdmrsIdx, index0, index1, SHUFFLE_SCATTER, 3);
  }
  // VENUS_PRINTVEC_SHORT(symIdxPerRB, 6*4);
  // VENUS_PRINTVEC_SHORT(rbCCEIdx, 48);

  // DMRS
  uint16_t crbo;
  uint16_t c0offset = nrb0;
  uint16_t bwpOffset;
  uint16_t numPRB;
  if (pdcch.SearchSpace.StartSymbolWithinSlot) {
    crbo      = carrier.NStartGrid;
    bwpOffset = carrier.NStartGrid - pdcch.NStartBWP;
    c0offset += bwpOffset;
    vrange(prbIdx, 6 * aggLv);
    prbIdx = vadd(prbIdx, nrb0 + 1 * nssStartsym * carrier.NSizeGrid, MASKREAD_OFF, 6 * aggLv);
    // numPRB = 2*6*aggLv + nrb0 + nssStartsym*carrier.NSizeGrid;
    int rbCCEIdx_addr = vaddr(rbCCEIdx);
    vbarrier();
    VSPM_OPEN();
    numPRB = *(volatile short *)(rbCCEIdx_addr + ((6 * aggLv / nSym - 1) << 1)) + 1;
    VSPM_CLOSE();
  } else {
    crbo = pdcch.NStartBWP;
    vrange(prbIdx, 6 * aggLv);
    prbIdx            = vadd(prbIdx, nrb0 - 1 * nssStartsym * pdcch.NSizeBWP, MASKREAD_OFF, 6 * aggLv);
    int rbCCEIdx_addr = vaddr(rbCCEIdx);
    vbarrier();
    VSPM_OPEN();
    numPRB = *(volatile short *)(rbCCEIdx_addr + ((6 * aggLv / nSym - 1) << 1)) + 1;
    VSPM_CLOSE();
  }
  if (pdcch.CORESET.CORESETID == 0) {
    crbo              = 0;
    prbIdx            = vadd(prbIdx, -1 * c0offset, MASKREAD_OFF, 6 * aggLv);
    int rbCCEIdx_addr = vaddr(rbCCEIdx);
    vbarrier();
    VSPM_OPEN();
    numPRB = *(volatile short *)(rbCCEIdx_addr + ((6 * aggLv / nSym - 1) << 1)) + 1;
    VSPM_CLOSE();
    // numPRB = 2*6*aggLv - c0offset;
  }

  uint16_t seqSampPerRB = 3 * 2; // 3REs per RB, 2 bits per DM-RS symbol
  // int symbolperslot = carrier.SymbolsPerSlot;
  // int nslot = carrier.NSlot;
  // int ncellid = carrier.NCellID;
  ///////////////////////////////////////////////////////////////////
  // input parameters: cinit(need ibar_SSB and ncellid)
  __v4096i8 init_vec;
  vclaim(init_vec);

  int fractionLength = 7;
  int sequenceLength = seqSampPerRB * (crbo + numPRB);
  int prbsLength     = seqSampPerRB * numPRB;

  ///////////////////////////////////////////////////////////////////

  /*--------------------DMRS GENERATION--------------------*/
  __v4096i8 dmrs_seq0;
  __v4096i8 dmrs_real;
  __v4096i8 dmrs_imag;
  vclaim(dmrs_seq0);
  vclaim(dmrs_real);
  vclaim(dmrs_imag);

  ///////////////////////////////////////////////////////////////////
  /*--------------------DMRS GENERATION--------------------*/
  for (int idx = 0; idx < nSym; idx++) {
    unsigned int cinit =
        ((1 << 17) * (symbolperslot * nslot + nssStartsym + idx + 1) * (2 * ncellid + 1) + 2 * ncellid) % (1 << 31);
    vbrdcst(init_vec, 0, MASKREAD_OFF, 32);
    vbarrier();
    VSPM_OPEN();
    int init_addr = vaddr(init_vec);
    for (int i = 0; i < 31; ++i) {
      *(volatile char *)(init_addr + i) = cinit & 0b1;
      cinit                             = cinit >> 1;
    }
    VSPM_CLOSE();
    dmrs_seq0 = nrPRBS(seq1_vec, init_vec, seq2_init_table_0_vec, seq2_init_table_1_vec, seq2_init_table_2_vec,
                       seq2_init_table_3_vec, seq2_init_table_4_vec, seq2_init_table_5_vec, seq2_init_table_6_vec,
                       seq2_init_table_7_vec, seq2_init_table_8_vec, seq2_init_table_9_vec, seq2_init_table_10_vec,
                       seq2_init_table_11_vec, seq2_init_table_12_vec, seq2_init_table_13_vec, seq2_init_table_14_vec,
                       seq2_init_table_15_vec, seq2_init_table_16_vec, seq2_init_table_17_vec, seq2_init_table_18_vec,
                       seq2_init_table_19_vec, seq2_trans_table_0_vec, seq2_trans_table_1_vec, seq2_trans_table_2_vec,
                       seq2_trans_table_3_vec, seq2_trans_table_4_vec, seq2_trans_table_5_vec, seq2_trans_table_6_vec,
                       sequenceLength);
    // VENUS_PRINTVEC_CHAR(dmrs_seq, sequenceLength);
    vrange(dmrs_shuffle_index, prbsLength);
    dmrs_shuffle_index = vadd(dmrs_shuffle_index, sequenceLength - prbsLength, MASKREAD_OFF, prbsLength);
    __v4096i8 dmrs_seq;
    vclaim(dmrs_seq);
    vshuffle(dmrs_seq, dmrs_shuffle_index, dmrs_seq0, SHUFFLE_GATHER, prbsLength);

    for (int i = 0; i < seqSampPerRB; i++) {
      dmrs_shuffle_index = vmul(rbCCEIdx, 6, MASKREAD_OFF, 6 * aggLv / nSym);
      dmrs_shuffle_index = vadd(dmrs_shuffle_index, i, MASKREAD_OFF, 6 * aggLv / nSym);
      vshuffle(dmrs_seq_tmp, dmrs_shuffle_index, dmrs_seq, SHUFFLE_GATHER, 6 * aggLv / nSym);
      vrange(dmrs_shuffle_index, 6 * aggLv / nSym);
      dmrs_shuffle_index = vmul(dmrs_shuffle_index, 6, MASKREAD_OFF, 6 * aggLv / nSym);
      dmrs_shuffle_index = vadd(dmrs_shuffle_index, i, MASKREAD_OFF, 6 * aggLv / nSym);
      vshuffle(dmrs_seq_reshape, dmrs_shuffle_index, dmrs_seq_tmp, SHUFFLE_SCATTER, 6 * aggLv / nSym);
    }
    // prbsLength = seqSampPerRB*6*aggLv;
    vrange(dmrs_shuffle_index, 3 * 6 * aggLv / nSym);
    dmrs_shuffle_index = vmul(dmrs_shuffle_index, 2, MASKREAD_OFF, 3 * 6 * aggLv / nSym);
    vshuffle(dmrs_real, dmrs_shuffle_index, dmrs_seq_reshape, SHUFFLE_GATHER, 3 * 6 * aggLv / nSym);
    dmrs_shuffle_index = vsadd(dmrs_shuffle_index, 1, MASKREAD_OFF, 3 * 6 * aggLv / nSym);
    vshuffle(dmrs_imag, dmrs_shuffle_index, dmrs_seq_reshape, SHUFFLE_GATHER, 3 * 6 * aggLv / nSym);
    vrange(dmrs_shuffle_index, 3 * 6 * aggLv / nSym);
    dmrs_shuffle_index = vsadd(dmrs_shuffle_index, idx * 3 * 6 * aggLv / nSym, MASKREAD_OFF, 3 * 6 * aggLv / nSym);
    vshuffle(dmrs_pdcch_real, dmrs_shuffle_index, dmrs_real, SHUFFLE_SCATTER, 3 * 6 * aggLv / nSym);
    vshuffle(dmrs_pdcch_imag, dmrs_shuffle_index, dmrs_imag, SHUFFLE_SCATTER, 3 * 6 * aggLv / nSym);
  }
  dmrs_pdcch_real = vmul(dmrs_pdcch_real, (int)(recpSqrt2 * (1 << fractionLength)), MASKREAD_OFF, 3 * 6 * aggLv);
  dmrs_pdcch_imag = vmul(dmrs_pdcch_imag, (int)(recpSqrt2 * (1 << fractionLength)), MASKREAD_OFF, 3 * 6 * aggLv);

  uint16_t dmrs_interval       = 4;
  uint16_t pdcchIdx_length     = 9 * 6 * aggLv;
  uint16_t pdcchdmrsIdx_length = 3 * 6 * aggLv;
  uint16_t temp                = -1 * nssStartsym * pdcch.NSizeBWP * 12;

  pdcchIdx     = vsadd(pdcchIdx, -1 * nssStartsym * pdcch.NSizeBWP * 12, MASKREAD_OFF, pdcchIdx_length);
  pdcchdmrsIdx = vsadd(pdcchdmrsIdx, -1 * nssStartsym * pdcch.NSizeBWP * 12, MASKREAD_OFF, pdcchdmrsIdx_length);

  short_struct out_pdcchIdx_length;
  short_struct out_pdcchdmrsIdx_length;
  short_struct out_dmrs_interval;
  short_struct out_nSym;
  // out_pdcchIdx_length.data = pdcchIdx_length / nSym;
  // out_pdcchdmrsIdx_length.data = pdcchdmrsIdx_length / nSym;
  out_pdcchIdx_length.data     = pdcchIdx_length;
  out_pdcchdmrsIdx_length.data = pdcchdmrsIdx_length;
  out_dmrs_interval.data       = dmrs_interval;
  out_nSym.data                = nSym;

  __v2048i16 dmrs_symbol_index;
  vclaim(dmrs_symbol_index);
  vrange(dmrs_symbol_index, nSym);

  vreturn(pdcchIdx, sizeof(pdcchIdx), &out_pdcchIdx_length, sizeof(out_pdcchIdx_length), dmrs_pdcch_real,
          sizeof(dmrs_pdcch_real), dmrs_pdcch_imag, sizeof(dmrs_pdcch_imag), pdcchdmrsIdx, sizeof(pdcchdmrsIdx),
          &out_dmrs_interval, sizeof(out_dmrs_interval), &out_pdcchdmrsIdx_length, sizeof(out_pdcchdmrsIdx_length),
          dmrs_symbol_index, sizeof(dmrs_symbol_index), &out_nSym, sizeof(out_nSym));
}