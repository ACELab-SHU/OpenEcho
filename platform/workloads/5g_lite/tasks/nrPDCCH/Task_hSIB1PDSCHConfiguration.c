/*
 * @Author: YihaoShen
 * @Date: 2024-07-20 23:38:32
 * @LastEditors: YihaoShen
 * @LastEditTime: 2024-07-21 00:01:51
 * @Description: hSIB1PDSCHConfiguration PDSCH configuration for SIB1
 *
 * Copyright (c) 2024 by ACE_Lab, All Rights Reserved.
 */
#include "data_type.h"
#include "riscv_printf.h"
#include "venus.h"
#include "vmath.h"

#define NAN 127

typedef short __v2048i16 __attribute__((ext_vector_type(2048)));
typedef short __v4096i8 __attribute__((ext_vector_type(4096)));

uint8_t ModulationTable[29] = {2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 4, 4, 4, 4, 4, 4, 4, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6};

uint16_t TargetCodeRateTable[29] = {120, 157, 193, 251, 308, 379, 449, 526, 602, 679, 340, 378, 434, 490, 553,
                                    616, 658, 438, 466, 517, 567, 616, 666, 719, 772, 822, 873, 910, 948};

double SpectralEfficiencyTable[29] = {0.2344, 0.3066, 0.377,  0.4902, 0.6016, 0.7402, 0.877,  1.0273, 1.1758, 1.3262,
                                      1.3281, 1.4766, 1.6953, 1.9141, 2.1602, 2.4063, 2.5703, 2.5664, 2.7305, 3.0293,
                                      3.3223, 3.6094, 3.9023, 4.2129, 4.5234, 4.8164, 5.1152, 5.332,  5.5547};

uint8_t restable1[32][6] = {
    1, 2, 0,  0, 2,  12, 1,  3, 0,  0,  3,  11, 2,  2, 0,  0,  2,  10, 2,  3, 0,  0, 3,  9, 3,  2, 0,  0, 2,  9, 3,  3,
    0, 0, 3,  8, 4,  2,  0,  0, 2,  7,  4,  3,  0,  0, 3,  6,  5,  2,  0,  0, 2,  5, 5,  3, 0,  0, 3,  4, 6,  2, 1,  0,
    9, 4, 6,  3, 1,  0,  10, 4, 7,  2,  1,  0,  4,  4, 7,  3,  1,  0,  6,  4, 8,  2, 1,  0, 5,  7, 8,  3, 1,  0, 5,  7,
    9, 2, 1,  0, 5,  2,  9,  3, 1,  0,  5,  2,  10, 2, 1,  0,  9,  2,  10, 3, 1,  0, 9,  2, 11, 2, 1,  0, 12, 2, 11, 3,
    1, 0, 12, 2, 12, 2,  0,  0, 1,  13, 12, 3,  0,  0, 1,  13, 13, 2,  0,  0, 1,  6, 13, 3, 0,  0, 1,  6, 14, 2, 0,  0,
    2, 4, 14, 3, 0,  0,  2,  4, 15, 2,  1,  0,  4,  7, 15, 3,  1,  0,  4,  7, 16, 2, 1,  0, 8,  4, 16, 3, 1,  0, 8,  4};

uint8_t restable2[31][6] = {
    1, 2,  1,  0, 2, 2, 1,  3,  1,  0, 2, 2, 2,  2,  1,  0,  4, 2, 2,  3,  1,  0,  4, 2, 3, 2,  1,   0,  6,   2,   3,
    3, 1,  0,  6, 2, 4, 2,  1,  0,  8, 2, 4, 3,  1,  0,  8,  2, 5, 2,  1,  0,  10, 2, 5, 3, 1,  0,   10, 2,   6,   2,
    1, 1,  2,  2, 6, 3, 1,  1,  2,  2, 7, 2, 1,  1,  4,  2,  7, 3, 1,  1,  4,  2,  8, 2, 1, 0,  2,   4,  8,   3,   1,
    0, 2,  4,  9, 2, 1, 0,  4,  4,  9, 3, 1, 0,  4,  4,  10, 2, 1, 0,  6,  4,  10, 3, 1, 0, 6,  4,   11, 2,   1,   0,
    8, 4,  11, 3, 1, 0, 8,  4,  12, 2, 1, 0, 10, 4,  12, 3,  1, 0, 10, 4,  13, 2,  1, 0, 2, 7,  13,  3,  1,   0,   2,
    7, 14, 2,  0, 0, 2, 12, 14, 3,  0, 0, 3, 11, 15, 2,  1,  1, 2, 4,  15, 3,  1,  1, 2, 4, 16, NAN, -1, NAN, NAN, NAN};
uint8_t restable3[30][6] = {
    1,  2,   1,  0,   2,   2,   1,  3,   1,  0,   2,   2,   2,  2, 1, 0, 4, 2, 2,  3, 1, 0, 4,  2, 3,  2, 1, 0, 6,  2,
    3,  3,   1,  0,   6,   2,   4,  2,   1,  0,   8,   2,   4,  3, 1, 0, 8, 2, 5,  2, 1, 0, 10, 2, 5,  3, 1, 0, 10, 2,
    6,  NAN, -1, NAN, NAN, NAN, 7,  NAN, -1, NAN, NAN, NAN, 8,  2, 1, 0, 2, 4, 8,  3, 1, 0, 2,  4, 9,  2, 1, 0, 4,  4,
    9,  3,   1,  0,   4,   4,   10, 2,   1,  0,   6,   4,   10, 3, 1, 0, 6, 4, 11, 2, 1, 0, 8,  4, 11, 3, 1, 0, 8,  4,
    12, 2,   1,  0,   10,  4,   12, 3,   1,  0,   10,  4,   13, 2, 1, 0, 2, 7, 13, 3, 1, 0, 2,  7, 14, 2, 0, 0, 2,  12,
    14, 3,   0,  0,   3,   11,  15, 2,   0,  0,   0,   6,   15, 3, 0, 0, 0, 6, 16, 2, 0, 0, 2,  6, 16, 3, 0, 0, 2,  6};

// // input parameter for test
// nrPDCCHConfig pdcch;
// pdcch.NSizeBWP = 24;

// DCIFormat1_0_SIRNTI dci;
// dci.FrequencyDomainResources   = 216;
// dci.TimeDomainResources        = 0;
// dci.VRBToPRBMapping            = 0;
// dci.ModulationCoding           = 9;
// dci.RedundancyVersion          = 0;
// dci.SystemInformationIndicator = 0;
// dci.ReservedBits               = 0;
// dci.AlignedWidth               = 0;
// dci.Width                      = 37;
// dci.PaddingWidth               = 0;
// uint8_t DMRSTypeAPosition      = 2;
// short   csetPattern            = 1;

/**
 * @Task_name: Task_hSIB1PDSCHConfiguration
 * @input:
 *  1. pdcch —— nrPDCCHConfig
 *  2. dci —— DCIFormat1_0_SIRNTI
 *  3. DMRSTypeAPosition
 *  4. csetPattern
 * @output:
 *  1. pdsch —— nrPDSCHConfig
 *  2. k_0
 * */
int Task_hSIB1PDSCHConfiguration(nrPDCCHConfig pdcch, DCIFormat1_0_SIRNTI dci, initialSystemInfo initialInfo,
                                 short_struct in_csetPattern, nrCarrierConfig carrier) {
  short   NSizeBWP                 = pdcch.NSizeBWP;
  short   FrequencyDomainResources = dci.FrequencyDomainResources;
  uint8_t DMRSTypeAPosition        = initialInfo.DMRSTypeAPosition;
  short   csetPattern              = in_csetPattern.data;
  short   NCellID                  = carrier.NCellID;

  uint8_t Lrbs    = floor((FrequencyDomainResources * 1.0 / NSizeBWP)) + 1;
  uint8_t RBstart = FrequencyDomainResources - ((Lrbs - 1) * NSizeBWP);

  nrPDSCHConfig pdsch; // output
  uint8_t       k_0;   // output

  // initial config for pdsch
  pdsch.Modulation                          = QPSK;
  pdsch.NumLayers                           = 1;
  pdsch.NumCodewords                        = 1;
  pdsch.VRBToPRBInterleaving                = 0;
  pdsch.VRBBundleSize                       = 2;
  pdsch.MappingType                         = 0;
  pdsch.SymbolAllocation[0]                 = 0;
  pdsch.SymbolAllocation[1]                 = 14;
  pdsch.RNTI                                = 1;
  pdsch.EnablePTRS                          = 0;
  pdsch.DMRS.DMRSConfigurationType          = 1;
  pdsch.DMRS.DMRSReferencePoint             = CRB0;
  pdsch.DMRS.NumCDMGroupsWithoutData        = 2;
  pdsch.DMRS.DMRSTypeAPosition              = 2;
  pdsch.DMRS.DMRSAdditionalPosition         = 0;
  pdsch.DMRS.DMRSLength                     = 1;
  pdsch.DMRS.CustomSymbolSet_length         = 0;
  pdsch.DMRS.DMRSPortSet_length             = 0;
  pdsch.DMRS.NIDNSCID_length                = 0;
  pdsch.DMRS.NSCID                          = 0;
  pdsch.DMRS.CDMGroups                      = 0;
  pdsch.DMRS.DeltaShifts[0]                 = 0;
  pdsch.DMRS.DeltaShifts_length             = 1;
  pdsch.DMRS.FrequencyWeights[0]            = 1;
  pdsch.DMRS.FrequencyWeights[1]            = 1;
  pdsch.DMRS.TimeWeights[0]                 = 1;
  pdsch.DMRS.TimeWeights[1]                 = 1;
  pdsch.DMRS.CDMLengths[0]                  = 1;
  pdsch.DMRS.CDMLengths[1]                  = 1;
  pdsch.DMRS.DMRSSubcarrierLocations_length = 6;
  for (int i = 0; i < 6; i++) {
    /* code */
    pdsch.DMRS.DMRSSubcarrierLocations[i] = 2 * i;
  }

  pdsch.PTRS.TimeDensity      = 1;
  pdsch.PTRS.FrequencyDensity = 2;
  pdsch.PTRS.REOffet          = 0;

  // uint8_t Lrbs = floor((dci.FrequencyDomainResources * 1.0 / NSizeBWP)) + 1;
  // uint8_t RBstart = dci.FrequencyDomainResources - ((Lrbs - 1) * NSizeBWP);

  if (Lrbs > NSizeBWP - RBstart) {
    Lrbs    = NSizeBWP - Lrbs + 2;
    RBstart = NSizeBWP - 1 - RBstart;
  }
  pdsch.PRBSet_length = Lrbs;
  for (int i = 0; i < Lrbs; ++i)
    pdsch.PRBSet[i] = RBstart + i;
  pdsch.VRBToPRBInterleaving = dci.VRBToPRBMapping;
  pdsch.RNTI                 = 65535;

  int pat = csetPattern;

  if (pat == 1) {
    int i = 0;
    for (i = 0; i < 32; ++i)
      if ((restable1[i][0] == (dci.TimeDomainResources + 1)) && (restable1[i][1] == DMRSTypeAPosition))
        break;
    k_0                       = restable1[i][3];
    pdsch.MappingType         = restable1[i][2];
    pdsch.SymbolAllocation[0] = restable1[i][4];
    pdsch.SymbolAllocation[1] = restable1[i][5];
  } else if (pat == 2) {
    int i = 0;
    for (i = 0; i < 31; ++i)
      if ((restable2[i][0] == (dci.TimeDomainResources + 1)) && (restable2[i][1] == DMRSTypeAPosition))
        break;
    k_0                       = restable2[i][3];
    pdsch.MappingType         = restable2[i][2];
    pdsch.SymbolAllocation[0] = restable2[i][4];
    pdsch.SymbolAllocation[1] = restable2[i][5];
  } else if (pat == 3) {
    int i = 0;
    for (i = 0; i < 30; ++i)
      if ((restable3[i][0] == (dci.TimeDomainResources + 1)) && (restable3[i][1] == DMRSTypeAPosition))
        break;
    k_0                       = restable3[i][3];
    pdsch.MappingType         = restable3[i][2];
    pdsch.SymbolAllocation[0] = restable3[i][4];
    pdsch.SymbolAllocation[1] = restable3[i][5];
  }
  // printf("csetPattern:%d\n", &pat);
  // printf("DMRSTypeAPosition:%bd\n", &DMRSTypeAPosition);
  // printf("dci.TimeDomainResources:%hd\n", &dci.TimeDomainResources);
  // printf("pdsch.SymbolAllocation[0]:%bd\n", &pdsch.SymbolAllocation[0]);
  // printf("pdsch.SymbolAllocation[1]:%bd\n", &pdsch.SymbolAllocation[1]);

  pdsch.NID                          = NCellID;
  pdsch.DMRS.DMRSTypeAPosition       = DMRSTypeAPosition;
  pdsch.DMRS.NIDNSCID[0]             = 0;
  pdsch.DMRS.NIDNSCID_length         = 0;
  pdsch.DMRS.NSCID                   = 0;
  pdsch.NumLayers                    = 1;
  pdsch.DMRS.DMRSPortSet_length      = 0;
  pdsch.DMRS.DMRSConfigurationType   = 1;
  pdsch.DMRS.DMRSLength              = 1;
  uint16_t L                         = pdsch.SymbolAllocation[1];
  pdsch.DMRS.NumCDMGroupsWithoutData = (L == 2) ? 1 : 2;
  if (pdsch.MappingType == 0)
    pdsch.DMRS.DMRSAdditionalPosition = 2;
  else {
    if (L == 2 || L == 4)
      pdsch.DMRS.DMRSAdditionalPosition = 0;
    else if (L == 7)
      pdsch.DMRS.DMRSAdditionalPosition = 1;
  }

  pdsch.DMRS.DMRSReferencePoint             = PRB0;
  pdsch.EnablePTRS                          = FALSE;
  pdsch.NSizeBWP                            = NSizeBWP;
  pdsch.NStartBWP                           = 0;
  pdsch.DMRS.CustomSymbolSet_length         = 0;
  pdsch.ReservedPRBSetLen                   = 0;
  pdsch.ReservedRE_length                   = 0;
  pdsch.DMRS.DMRSSubcarrierLocations_length = 6;
  for (int i = 0; i < pdsch.DMRS.DMRSSubcarrierLocations_length; ++i) {
    pdsch.DMRS.DMRSSubcarrierLocations[i] = 2 * i;
  }

  pdsch.TargetCodeRate = TargetCodeRateTable[dci.ModulationCoding] / 1024.0;
  // pdsch.Modulation     = ModulationTable[dci.ModulationCoding];
  pdsch.DMRS.CDMLengths[0] = pdsch.DMRS.CDMLengths[1] = 1;

  pdsch.rv = dci.RedundancyVersion;

  // for SS/PBCH block and CORESET multiplexing pattern 1, K_0 is alway 0
  // short_struct out_k_0;
  // out_k_0.data = k_0;

  short_struct out_pdsch_start_symbol;
  out_pdsch_start_symbol.data = pdsch.SymbolAllocation[0];
  short_struct out_pdsch_symbol_length;
  out_pdsch_symbol_length.data = pdsch.SymbolAllocation[1];
  // printf("out_pdsch_start_symbol:%hd\n", &out_pdsch_start_symbol);
  // printf("out_pdsch_symbol_length:%hd\n", &out_pdsch_symbol_length);

  vreturn(&pdsch, sizeof(nrPDSCHConfig), &out_pdsch_start_symbol, sizeof(out_pdsch_start_symbol),
          &out_pdsch_symbol_length, sizeof(out_pdsch_symbol_length));
}
