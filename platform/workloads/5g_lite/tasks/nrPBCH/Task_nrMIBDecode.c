/**
 * ****************************************
 * @file        nrMIBDecode.c
 * @brief       MIB decode
 * @author      yuanfeng
 * @date        2024.7.15
 * @copyright   ACE-Lab(Shanghai University)
 * ****************************************
 */

#include "riscv_printf.h"
#include "venus.h"

typedef short __v2048i16 __attribute__((ext_vector_type(2048)));
typedef short __v4096i16 __attribute__((ext_vector_type(4096)));
typedef char  __v4096i8 __attribute__((ext_vector_type(4096)));

typedef struct nrPDCCHConfigSIB1 {
  uint8_t searchSpaceZero;
  uint8_t controlResourceSetZero;
} __attribute__((aligned(64))) nrPDCCHConfigSIB1;

typedef struct initialSystemInfo {
  uint32_t          NFrame;
  uint32_t          SubcarrierSpacingCommon;
  uint32_t          k_SSB;
  uint32_t          DMRSTypeAPosition;
  nrPDCCHConfigSIB1 PDCCHConfigSIB1;
  uint32_t          CellBarred;
  uint32_t          IntraFreqReselection;
} __attribute__((aligned(64))) initialSystemInfo;

typedef struct MIB {
  uint8_t           spare;
  uint8_t           cellBarred;
  uint8_t           intraFreqReselection;
  nrPDCCHConfigSIB1 pdcch_ConfigSIB1;
  uint8_t           dmrs_TypeA_Position;
  uint8_t           ssb_SubcarrierOffset;
  uint8_t           subCarrierSpacingCommon;
  uint8_t           systemFrameNumber;
} __attribute__((aligned(64))) MIB;

typedef struct {
  short data;
} __attribute__((aligned(64))) short_struct;

int Task_nrMIBDecode(__v4096i8 mib, short_struct input_sfn4lsb, short_struct input_msbidxoffset) {
  uint8_t bitOffset = 0;
  MIB     mibInfo;
  int     sfn4lsb      = input_sfn4lsb.data;
  int     msbidxoffset = input_msbidxoffset.data;

  bitOffset += 1;

  short sysFraNum = 0;

  vbarrier();
  VSPM_OPEN();
  int mib_addr1 = vaddr(mib);
  for (int i = 0; i < 6; ++i)
    sysFraNum += (*(volatile char *)(mib_addr1 + bitOffset + i)) << (6 - i - 1);
  VSPM_CLOSE();
  mibInfo.systemFrameNumber = sysFraNum;
  bitOffset += 6;

  vbarrier();
  VSPM_OPEN();
  int mib_addr2                   = vaddr(mib);
  mibInfo.subCarrierSpacingCommon = (*(volatile char *)(mib_addr2 + bitOffset));
  VSPM_CLOSE();

  bitOffset += 1;

  short ssbNum = 0;
  vbarrier();
  VSPM_OPEN();
  int mib_addr3 = vaddr(mib);
  for (int i = 0; i < 4; ++i)
    ssbNum += (*(volatile char *)(mib_addr3 + bitOffset + i)) << (4 - i - 1);
  VSPM_CLOSE();
  mibInfo.ssb_SubcarrierOffset = ssbNum;
  bitOffset += 4;

  vbarrier();
  VSPM_OPEN();
  int mib_addr4               = vaddr(mib);
  mibInfo.dmrs_TypeA_Position = (*(volatile char *)(mib_addr4 + bitOffset));
  VSPM_CLOSE();

  bitOffset += 1;

  short ctrlNum = 0;
  vbarrier();
  VSPM_OPEN();
  int mib_addr5 = vaddr(mib);
  for (int i = 0; i < 4; ++i)
    ctrlNum += (*(volatile char *)(mib_addr5 + bitOffset + i)) << (4 - i - 1);
  VSPM_CLOSE();
  mibInfo.pdcch_ConfigSIB1.controlResourceSetZero = ctrlNum;
  bitOffset += 4;

  short searchNum = 0;
  vbarrier();
  VSPM_OPEN();
  int mib_addr6 = vaddr(mib);
  for (int i = 0; i < 4; ++i)
    searchNum += (*(volatile char *)(mib_addr6 + bitOffset + i)) << (4 - i - 1);
  VSPM_CLOSE();
  mibInfo.pdcch_ConfigSIB1.searchSpaceZero = searchNum;
  bitOffset += 4;

  vbarrier();
  VSPM_OPEN();
  int mib_addr7      = vaddr(mib);
  mibInfo.cellBarred = (*(volatile char *)(mib_addr7 + bitOffset));
  VSPM_CLOSE();
  bitOffset += 1;

  vbarrier();
  VSPM_OPEN();
  int mib_addr8                = vaddr(mib);
  mibInfo.intraFreqReselection = (*(volatile char *)(mib_addr8 + bitOffset));
  VSPM_CLOSE();
  bitOffset += 1;

  vbarrier();
  VSPM_OPEN();
  int mib_addr9 = vaddr(mib);
  mibInfo.spare = (*(volatile char *)(mib_addr9 + bitOffset));
  VSPM_CLOSE();
  bitOffset += 1;

  initialSystemInfo initialInfo;
  initialInfo.NFrame                                 = mibInfo.systemFrameNumber * (1 << 4) + sfn4lsb;
  initialInfo.SubcarrierSpacingCommon                = (mibInfo.subCarrierSpacingCommon + 1) * 15;
  initialInfo.k_SSB                                  = msbidxoffset * 16 + mibInfo.ssb_SubcarrierOffset;
  initialInfo.DMRSTypeAPosition                      = 2 + mibInfo.dmrs_TypeA_Position;
  initialInfo.CellBarred                             = mibInfo.cellBarred;
  initialInfo.PDCCHConfigSIB1.controlResourceSetZero = mibInfo.pdcch_ConfigSIB1.controlResourceSetZero;
  initialInfo.PDCCHConfigSIB1.searchSpaceZero        = mibInfo.pdcch_ConfigSIB1.searchSpaceZero;
  initialInfo.IntraFreqReselection                   = mibInfo.intraFreqReselection;

  vreturn(&initialInfo, sizeof(initialInfo));
}