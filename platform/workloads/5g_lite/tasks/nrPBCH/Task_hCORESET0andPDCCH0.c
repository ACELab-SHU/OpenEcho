/**
 * ****************************************
 * @file        Task_hCORESET0andPDCCH0.c
 * @brief       generate CORESET0 and PDCCH0 parameters
 * @author      yuanfeng
 * @date        2024.7.15
 * @copyright   ACE-Lab(Shanghai University)
 * ****************************************
 */
#include "data_type.h"
#include "riscv_printf.h"
#include "venus.h"
#include "vmath.h"

typedef short __v2048i16 __attribute__((ext_vector_type(2048)));
typedef short __v4096i16 __attribute__((ext_vector_type(4096)));
typedef char  __v4096i8 __attribute__((ext_vector_type(4096)));

typedef struct hCORSET0andPDCCH0outstruct {
  short out_nrb;
  short out_ssSlot;
  short out_ssFirstSym;
  short out_csetDuration;
  short out_csetPattern;
  short out_pdcchFrame;
  char  out_crc;
} __attribute__((aligned(64))) hCORSET0andPDCCH0outstruct;

static uint8_t tab1[5][16]  = {0,  1,  2,  3,   4,  5,  6,  7,   8, 9, 10, 11,  12, 13, 14, 15, 1,  1,  1,  1,
                              1,  1,  1,  1,   1,  1,  1,  1,   1, 1, 1,  INF, 24, 24, 24, 24, 24, 24, 48, 48,
                              48, 48, 48, 48,  96, 96, 96, INF, 2, 2, 2,  3,   3,  3,  1,  1,  2,  2,  3,  3,
                              1,  2,  3,  INF, 0,  2,  4,  0,   2, 4, 12, 16,  12, 16, 12, 16, 38, 38, 38, INF};
static uint8_t tab2[5][16]  = {0,  1,  2,   3,   4,  5,  6,   7,   8, 9, 10, 11,  12, 13, 14, 15, 1,  1,  1,   1,
                              1,  1,  1,   1,   1,  1,  1,   1,   1, 1, 1,  INF, 24, 24, 24, 24, 24, 24, 24,  24,
                              48, 48, 48,  48,  48, 48, INF, INF, 2, 2, 2,  2,   3,  3,  3,  3,  1,  1,  2,   2,
                              3,  3,  INF, INF, 5,  6,  7,   8,   5, 6, 7,  8,   18, 20, 18, 20, 18, 20, INF, INF};
static uint8_t tab3[5][16]  = {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,   10,  11,  12,  13,  14,  15,
                              1,  1,  1,  1,  1,  1,  1,  1,  1,  INF, INF, INF, INF, INF, INF, INF,
                              48, 48, 48, 48, 48, 48, 96, 96, 96, INF, INF, INF, INF, INF, INF, INF,
                              1,  1,  2,  2,  3,  3,  1,  2,  3,  INF, INF, INF, INF, INF, INF, INF,
                              2,  6,  2,  6,  2,  6,  28, 28, 28, INF, INF, INF, INF, INF, INF, INF};
static uint8_t tab4[5][16]  = {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,   10,  11,  12,  13,  14,  15,
                              1,  1,  1,  1,  1,  1,  1,  1,  1,  INF, INF, INF, INF, INF, INF, INF,
                              48, 48, 48, 96, 96, 96, 96, 96, 96, INF, INF, INF, INF, INF, INF, INF,
                              1,  2,  3,  1,  1,  2,  2,  3,  3,  INF, INF, INF, INF, INF, INF, INF,
                              4,  4,  4,  0,  56, 0,  56, 0,  56, INF, INF, INF, INF, INF, INF, INF};
static uint8_t tab5[5][16]  = {0,  1,  2,  3,  4,  5,  6,  7,  8, 9, 10, 11, 12, 13, 14, 15, 1,  1,  1,  1,
                              1,  1,  1,  1,  1,  1,  1,  1,  1, 1, 1,  1,  24, 24, 24, 24, 24, 24, 24, 24,
                              24, 24, 48, 48, 48, 48, 48, 48, 2, 2, 2,  2,  2,  3,  3,  3,  3,  3,  1,  1,
                              1,  2,  2,  2,  0,  1,  2,  3,  4, 0, 1,  2,  3,  4,  12, 14, 16, 12, 14, 16};
static uint8_t tab6[5][16]  = {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10,  11,  12,  13,  14,  15,
                              1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  INF, INF, INF, INF, INF, INF,
                              24, 24, 24, 24, 48, 48, 48, 48, 48, 48, INF, INF, INF, INF, INF, INF,
                              2,  2,  3,  3,  1,  1,  2,  2,  3,  3,  INF, INF, INF, INF, INF, INF,
                              0,  4,  0,  4,  0,  28, 0,  28, 0,  28, INF, INF, INF, INF, INF, INF};
static uint8_t tab7[5][16]  = {0,  1,  2,  3,  4,  5,  6,  7,  8,   9,  10,  11, 12,  13,  14,  15,
                              1,  1,  1,  1,  1,  1,  1,  1,  2,   2,  2,   2,  INF, INF, INF, INF,
                              48, 48, 48, 48, 48, 48, 96, 96, 48,  48, 96,  96, INF, INF, INF, INF,
                              1,  1,  2,  2,  3,  3,  1,  2,  1,   1,  1,   1,  INF, INF, INF, INF,
                              0,  8,  0,  8,  0,  8,  28, 28, -41, 49, -41, 97, INF, INF, INF, INF};
static uint8_t tab8[5][16]  = {0,  1,  2,  3,  4,   5,  6,   7,  8,   9,   10,  11,  12,  13,  14,  15,
                              1,  1,  1,  1,  3,   3,  3,   3,  INF, INF, INF, INF, INF, INF, INF, INF,
                              24, 24, 48, 48, 24,  24, 48,  48, INF, INF, INF, INF, INF, INF, INF, INF,
                              2,  2,  1,  2,  2,   2,  2,   2,  INF, INF, INF, INF, INF, INF, INF, INF,
                              0,  4,  14, 14, -20, 24, -20, 48, INF, INF, INF, INF, INF, INF, INF, INF};
static uint8_t tab9[5][16]  = {0,  1,  2,  3,  4,   5,   6,   7,   8,   9,   10,  11,  12,  13,  14,  15,
                              1,  1,  1,  1,  INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
                              96, 96, 96, 96, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
                              1,  1,  2,  2,  INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
                              0,  16, 0,  16, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF};
static uint8_t tab10[5][16] = {0,  1,  2,  3,  4,   5,  6,   7,  8,   9,   10,  11,  12,  13,  14,  15,
                               1,  1,  1,  1,  2,   2,  2,   2,  INF, INF, INF, INF, INF, INF, INF, INF,
                               48, 48, 48, 48, 24,  24, 48,  48, INF, INF, INF, INF, INF, INF, INF, INF,
                               1,  1,  2,  2,  1,   1,  1,   1,  INF, INF, INF, INF, INF, INF, INF, INF,
                               0,  8,  0,  8,  -41, 25, -41, 49, INF, INF, INF, INF, INF, INF, INF, INF};

static float tab2_1[5][16] = {0, 1,         2, 3,         4, 5,         6, 7,         8, 9, 10, 11, 12, 13, 14, 15,
                              0, 0,         2, 2,         5, 5,         7, 7,         0, 5, 0,  0,  2,  2,  5,  5,
                              1, 2,         1, 2,         1, 2,         1, 2,         1, 1, 1,  1,  1,  1,  1,  1,
                              1, 0.5,       1, 0.5,       1, 0.5,       1, 0.5,       2, 2, 1,  1,  1,  1,  1,  1,
                              0, FLAGFSYM1, 0, FLAGFSYM1, 0, FLAGFSYM1, 0, FLAGFSYM1, 0, 0, 1,  2,  1,  2,  1,  2};

static float tab2_2[5][16] = {
    0,         1,     2,         3,         4, 5,         6,     7,         8,     9,         10,        11,
    12,        13,    14,        15,        0, 0,         2.5,   2.5,       5,     5,         0,         2.5,
    5,         7.5,   7.5,       7.5,       0, 5,         0xFF,  0xFF,      1,     2,         1,         2,
    1,         2,     2,         2,         2, 1,         2,     2,         1,     1,         0xFF,      0xFF,
    1,         1 / 2, 1,         1 / 2,     1, 1 / 2,     1 / 2, 1 / 2,     1 / 2, 1,         1 / 2,     1 / 2,
    2,         2,     0xFF,      0xFF,      0, FLAGFSYM1, 0,     FLAGFSYM1, 0,     FLAGFSYM1, FLAGFSYM2, FLAGFSYM2,
    FLAGFSYM2, 0,     FLAGFSYM1, FLAGFSYM2, 0, 0,         0xFF,  0xFF};
int Task_hCORESET0andPDCCH0(short_struct input_scsSSB, short_struct input_minChannelBW, short_struct input_ssbIdx,
                            short_struct input_ncellid, short_struct input_crc, initialSystemInfo initialInfo) {
  uint8_t  scsSSB                  = input_scsSSB.data;
  uint8_t  minChannelBW            = input_minChannelBW.data;
  uint8_t  ssbIdx                  = input_ssbIdx.data;
  uint32_t ncellid                 = input_ncellid.data;
  uint8_t  crc                     = input_crc.data;
  uint8_t  SubcarrierSpacingCommon = initialInfo.SubcarrierSpacingCommon;

  uint8_t scsPair[2];
  scsPair[0] = scsSSB;
  scsPair[1] = SubcarrierSpacingCommon;

  uint8_t csetNRB, csetDuration, csetOffset, csetPattern;

  uint8_t index = initialInfo.PDCCHConfigSIB1.controlResourceSetZero;

  uint32_t t[5];
  if (scsPair[0] == 15 && scsPair[1] == 15) {
    for (int i = 0; i < 5; i++)
      t[i] = tab1[i][index];
  } else if (scsPair[0] == 15 && scsPair[1] == 30) {
    for (int i = 0; i < 5; i++)
      t[i] = tab2[i][index];
  } else if (scsPair[0] == 30 && scsPair[1] == 15) {
    if (minChannelBW == 5 || minChannelBW == 10) {
      for (int i = 0; i < 5; i++)
        t[i] = tab3[i][index];
    } else if (minChannelBW == 40) {
      for (int i = 0; i < 5; i++)
        t[i] = tab4[i][index];
    }
  } else if (scsPair[0] == 30 && scsPair[1] == 30) {
    if (minChannelBW == 5 || minChannelBW == 10) {
      for (int i = 0; i < 5; i++)
        t[i] = tab5[i][index];
    } else if (minChannelBW == 40) {
      for (int i = 0; i < 5; i++)
        t[i] = tab6[i][index];
    }
  } else if (scsPair[0] == 120 && scsPair[1] == 60) {
  } else if (scsPair[0] == 120 && scsPair[1] == 120) {
  } else if (scsPair[0] == 240 && scsPair[1] == 120) {
  } else if (scsPair[0] == 240 && scsPair[1] == 120) {
  }

  csetPattern  = t[1];
  csetNRB      = t[2];
  csetDuration = t[3];
  csetOffset   = t[4];

  uint8_t c0 = csetOffset + 10 * scsPair[1] / scsPair[0];

  uint8_t nrb = 2 * max(c0, csetNRB - c0);

  uint8_t ssSlot, ssFirstSym, isOccasion, frameOff;

  index              = initialInfo.PDCCHConfigSIB1.searchSpaceZero;
  uint8_t scsCommon  = initialInfo.SubcarrierSpacingCommon;
  uint8_t symPerSlot = 14;
  uint8_t fSym1, fSym2;

  if (csetPattern == 1) {
    if (scsSSB == 15 || scsSSB == 30) {
      if ((ssbIdx % 2) == 0)
        fSym1 = 0;
      else
        fSym1 = csetDuration;
      uint8_t t[5];
      for (int i = 0; i < 5; ++i) {
        if (tab2_1[i][index] == FLAGFSYM1)
          t[i] = fSym1;
        else
          t[i] = tab2_1[i][index];
      }
      uint8_t O  = t[1];
      uint8_t M  = t[3];
      ssFirstSym = t[4];

      // uint8_t mu            = log2(scsCommon / 15);
      uint8_t mu            = floor_log2(scsCommon / 15);
      uint8_t slotsPerFrame = 10 * (1 << mu);

      uint8_t slot = O * (1 << mu) + floor(ssbIdx * M);
      frameOff     = floor(slot / slotsPerFrame);
      ssSlot       = slot % slotsPerFrame;

      isOccasion = (((frameOff % 2) ^ (initialInfo.NFrame % 2)) == 0) ? TRUE : FALSE;
    } else {
      if ((ssbIdx % 2) == 0) {
        fSym1 = 0;
        fSym2 = 0;
      } else {
        fSym1 = 7;
        fSym2 = csetDuration;
      }

      uint8_t t[5];
      for (int i = 0; i < 5; ++i) {
        if (tab2_2[i][index] == FLAGFSYM1 || tab2_2[i][index] == FLAGFSYM2)
          t[i] = (tab2_2[i][index] == FLAGFSYM1) ? fSym1 : fSym2;
        else
          t[i] = tab2_2[i][index];
      }
      uint8_t O  = t[1];
      uint8_t M  = t[3];
      ssFirstSym = t[4];

      // uint8_t mu            = log2(scsCommon / 15);
      uint8_t mu            = floor_log2(scsCommon / 15);
      uint8_t slotsPerFrame = 10 * 2 ^ mu;

      uint8_t slot = O * pow(2, mu) + floor(ssbIdx * M);
      frameOff     = floor(slot / slotsPerFrame);
      ssSlot       = slot % slotsPerFrame;

      isOccasion = ((frameOff % 2) ^ (initialInfo.NFrame % 2)) ? TRUE : FALSE;
    }
  }

  short pdcchFrame = initialInfo.NFrame;

  uint8_t slotsPerFrame = 10 * scsPair[0] / 15;
  if (isOccasion == 0) {
    pdcchFrame += 1;
    // ssSlot = ssSlot + slotsPerFrame;
  }

  // __v2048i16 csetSubcarriers;
  // vclaim(csetSubcarriers);
  // vrange(csetSubcarriers, csetNRB * 12);
  // csetSubcarriers =
  //     vsadd(csetSubcarriers,
  //           12 * (nrb - 20 * scsPair[1] / scsPair[0]) / 2 - csetOffset * 12,
  //           MASKREAD_OFF, csetNRB * 12);

  nrPDCCHConfig pdcch;

  pdcch.AggregationLevel   = 8;
  pdcch.AllocatedCandidate = 1;
  uint8_t msbIdx           = initialInfo.PDCCHConfigSIB1.controlResourceSetZero;
  uint8_t kssb             = initialInfo.k_SSB;

  for (size_t i = 0; i < (csetNRB / 6); i++)
    pdcch.CORESET.FrequencyResources[i] = 1;
  pdcch.CORESET.Duration        = csetDuration;
  pdcch.CORESET.CCEREGMapping   = INTERLEAVED;
  pdcch.CORESET.REGBundleSize   = 6;
  pdcch.CORESET.InterleaverSize = 2;
  pdcch.CORESET.ShiftIndex      = ncellid;
  pdcch.CORESET.CORESETID       = 0;
  pdcch.CORESET.NCCE            = 8;

  uint8_t candidates[]                     = {0, 0, 4, 2, 1};
  pdcch.SearchSpace.SearchSpaceID          = 1;
  pdcch.SearchSpace.CORESETID              = pdcch.CORESET.CORESETID;
  pdcch.SearchSpace.SearchSpaceType        = COMMON;
  pdcch.SearchSpace.StartSymbolWithinSlot  = ssFirstSym;
  pdcch.SearchSpace.SlotPeriodAndOffset[0] = 2 * slotsPerFrame;
  pdcch.SearchSpace.SlotPeriodAndOffset[1] = ssSlot;
  pdcch.SearchSpace.Duration               = (csetPattern == 1) ? 2 : 1;
  for (int i = 0; i < 5; ++i)
    pdcch.SearchSpace.NumCandidates[i] = candidates[i];
  // uint8_t maxAL                          = floor(ceil(log2(1.0 * csetNRB * csetDuration / 6))) + 1;
  uint8_t maxAL                          = floor(ceil_log2(1.0 * csetNRB * csetDuration / 6)) + 1;
  pdcch.SearchSpace.NumCandidates[maxAL] = 0;
  pdcch.NStartBWP                        = 0;
  pdcch.NSizeBWP                         = csetNRB;
  pdcch.RNTI                             = 0;
  pdcch.DMRSScramblingID                 = ncellid;

  nrCarrierConfig c0Carrier;
  c0Carrier.SubCarrierSpacing = initialInfo.SubcarrierSpacingCommon;
  c0Carrier.NStartGrid        = pdcch.NStartBWP;
  c0Carrier.NSizeGrid         = pdcch.NSizeBWP;
  c0Carrier.NSlot             = pdcch.SearchSpace.SlotPeriodAndOffset[1];
  c0Carrier.NFrame            = initialInfo.NFrame;
  c0Carrier.NCellID           = ncellid;
  c0Carrier.SymbolsPerSlot    = 14;
  c0Carrier.SlotsPerSubframe  = 1;
  c0Carrier.SlotsPerFrame     = 10;
  c0Carrier.CyclicPrefix      = 0;

  // nrb
  // csetSubcarriers
  // csetNRB
  // ssSlot
  // ssFirstSym
  // csetDuration
  // pdcch
  // csetPattern
  // carrier
  //   short_struct out_nrb;
  //   out_nrb.data = nrb;
  short_struct out_ssSlot;
  out_ssSlot.data = ssSlot;
  short_struct out_ssFirstSym;
  out_ssFirstSym.data = ssFirstSym;
  //   short_struct out_csetDuration;
  //   out_csetDuration.data = csetDuration;
  //   short_struct out_csetPattern;
  //   out_csetPattern.data = csetPattern;
  //   short_struct out_pdcchFrame;
  //   out_pdcchFrame.data = pdcchFrame;
  hCORSET0andPDCCH0outstruct outstruct;
  outstruct.out_nrb          = nrb;
  outstruct.out_ssSlot       = ssSlot;
  outstruct.out_ssFirstSym   = ssFirstSym;
  outstruct.out_csetDuration = csetDuration;
  outstruct.out_csetPattern  = csetPattern;
  outstruct.out_pdcchFrame   = pdcchFrame;
  outstruct.out_crc          = crc;

  __v2048i16 csetSubcarriers;
  vclaim(csetSubcarriers);
  vrange(csetSubcarriers, csetNRB * 12);
  csetSubcarriers = vsadd(csetSubcarriers, 12 * (nrb - 20 * scsPair[1] / scsPair[0]) / 2 - csetOffset * 12,
                          MASKREAD_OFF, csetNRB * 12);

  //   vreturn(csetSubcarriers, sizeof(csetSubcarriers), &out_nrb, sizeof(out_nrb), &out_ssSlot, sizeof(out_ssSlot),
  //           &out_ssFirstSym, sizeof(out_ssFirstSym), &out_csetDuration, sizeof(out_csetDuration), &pdcch,
  //           sizeof(pdcch), &out_csetPattern, sizeof(out_csetPattern), &c0Carrier, sizeof(c0Carrier), &out_pdcchFrame,
  //           sizeof(out_pdcchFrame));
  vreturn(csetSubcarriers, sizeof(csetSubcarriers), &pdcch, sizeof(pdcch), &c0Carrier, sizeof(c0Carrier), &outstruct,
          sizeof(outstruct));
}
