#ifndef _DATA_TYPE_H_
#define _DATA_TYPE_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#define TRUE  1
#define FALSE 0

#define INTERLEAVED 1

#define COMMON 1

#define PRB0 1
#define CRB0 0

#define BPSK   1
#define QPSK   2
#define QAM16  4
#define QAM64  6
#define QAM256 8

#define CASE_A 0
#define CASE_B 1
#define CASE_C 2
#define CASE_D 3
#define CASE_E 4

#define INF 0xFFFFFFFF
#define PI  3.14159265358979323846264338327950288419717

#define FLAGFSYM1 0xFE
#define FLAGFSYM2 0xFF

typedef struct nrSearchSpaceConfig {
  uint16_t SearchSpaceID;
  uint16_t Label;
  uint16_t CORESETID;
  uint16_t SearchSpaceType;
  uint16_t StartSymbolWithinSlot;
  uint16_t SlotPeriodAndOffset[2];
  uint16_t Duration;
  uint16_t NumCandidates[5];
} __attribute__((aligned(64))) nrSearchSpaceConfig;

typedef struct nrCORESETConfig {
  uint16_t CORESETID;
  uint16_t Label;
  uint16_t FrequencyResources[8];
  uint16_t Duration;
  uint16_t CCEREGMapping;
  uint16_t REGBundleSize;
  uint16_t InterleaverSize;
  uint16_t ShiftIndex;
  uint16_t NCCE;
  uint16_t RBOffset; // - RB offset of CORESET in BWP. (integer from 0 to 5)
} __attribute__((aligned(64))) nrCORESETConfig;

typedef struct nrPDCCHPara {
  uint16_t            NSizeBWP;
  uint16_t            NStartBWP;
  uint16_t            RNTI;
  uint16_t            DMRSScramblingID;
  uint16_t            AggregationLevel;
  uint16_t            AllocatedCandidate;
  uint16_t            CCEOffset[8];
  nrCORESETConfig     CORESET;
  nrSearchSpaceConfig SearchSpace;
} __attribute__((aligned(64))) nrPDCCHConfig;

typedef struct Carrier {
  uint16_t NCellID;
  uint16_t NSizeGrid;  // 单位：RB
  uint16_t NStartGrid; // 单位：RB
  uint16_t NSlot;
  uint16_t NFrame;
  uint16_t SymbolsPerSlot;
  uint16_t SlotsPerSubframe;
  uint16_t SlotsPerFrame;
  uint16_t SubCarrierSpacing; // KHz
  uint8_t  CyclicPrefix;      // 0:Normal 1:Extended
} __attribute__((aligned(64))) nrCarrierConfig;

typedef struct nrPDSCHPTRSConfig {
  uint8_t PTRSPortSet[12];
  uint8_t TimeDensity;
  uint8_t FrequencyDensity;
  uint8_t REOffet;
} __attribute__((aligned(64))) nrPDSCHPTRSConfig;

typedef struct nrPDSCHDMRSConfig {
  uint8_t  DMRSConfigurationType;
  uint8_t  DMRSReferencePoint;
  uint8_t  NumCDMGroupsWithoutData;
  uint8_t  DMRSTypeAPosition;
  uint8_t  DMRSAdditionalPosition;
  uint8_t  DMRSLength;
  uint8_t  CustomSymbolSet[14];
  uint8_t  CustomSymbolSet_length;
  uint8_t  DMRSPortSet[12];
  uint8_t  DMRSPortSet_length;
  uint16_t NIDNSCID[2];
  uint8_t  NIDNSCID_length;
  uint8_t  NSCID;
  uint8_t  CDMGroups;
  uint8_t  DeltaShifts[4];
  uint8_t  DeltaShifts_length;
  uint8_t  FrequencyWeights[2];
  uint8_t  TimeWeights[2];
  uint8_t  DMRSSubcarrierLocations[12];
  uint8_t  DMRSSubcarrierLocations_length;
  uint8_t  CDMLengths[2];
} __attribute__((aligned(64))) nrPDSCHDMRSConfig;

typedef struct nrPDSCHReservedConfig {
  uint16_t PRBSet[275];
  uint16_t PRBSet_length;
  uint8_t  SymbolSet[256];
  uint16_t SymbolSet_length;
  uint16_t Period;
} __attribute__((aligned(64))) nrPDSCHReservedConfig;

typedef struct nrPDSCHConfig {
  // Channel Configuration
  uint16_t NSizeBWP;  // 0(default) | 1-275. Number of PRBs in bandwidth part (BWP), specified as an integer from 1 to
                      // 275. Use 0 to set this property to the NSizeGrid property of the nrCarrierConfig object.
  uint16_t NStartBWP; // 0(default) | 0-2473. Starting PRB index of BWP relative to common resource block 0 (CRB 0),
                      // specified as an integer from 0 to 2473. When NSizeBWP is 0 then set this property to the
                      // NStartGrid property of the nrCarrierConfig object.
  uint8_t Modulation; // 1:pi/2-BPSK | 2:QPSK(default) | 4:16QAM | 6:64QAM | 8:256QAM. Modulation scheme.
  uint8_t NumLayers;  // 1(default) | 2 | 3 | 4. Number of transmission layers, specified as 1, 2, 3, or 4.
  uint8_t
          MappingType; // 0:TypeA(default) | 1:TypeB. Mapping type of the physical shared channel, specified as 'A' or 'B'.
  uint8_t SymbolAllocation[2]; // [0,14](default) | two-element vector of nonnegative integers. OFDM symbol allocation
                               // of the physical shared channel, specified as a two-element vector of nonnegative
                               // integers. The first element of this property represents the start of symbol allocation
                               // (0-based). The second element represents the number of allocated OFDM symbols. When
                               // you set this property to [] or the second element of the vector to 0, no symbol is
                               // allocated for the channel.
  uint16_t PRBSet[275]; // [0:51] (default) | vector of integers from 0 to 274. Physical resource block (PRB) allocation
                        // of the PUSCH within the BWP, specified as a vector of integers from 0 to 274.
  uint16_t PRBSet_length;
  // nrPDSCHReservedConfig ReservedPRBSet[275];
  uint32_t ReservedPRBSetLen;
  uint16_t
      NID; // 1024 (default) | integer from 0 to 1023. PUSCH scrambling identity, specified as [] or an integer from 0
           // to 1023. If the higher layer parameter dataScramblingIdentityPUSCH is configured, NID must be an integer
           // from 0 to 1023. If the higher layer parameter dataScramblingIdentityPUSCH is not configured, NID must be
           // an integer from 0 to 1007. When you specify this property as [], the object sets the PUSCH scrambling
           // identity to the physical layer cell identity, specified by the NCellID property of the carrier.
  uint16_t RNTI; // 1 (default) | integer from 0 to 65535. Radio network temporary identifier of the user equipment
                 // (UE), specified as an integer from 0 to 65,535.
  // Reference Signals Configuration
  nrPDSCHDMRSConfig DMRS; // default nrPUSCHDMRSConfig object (default) | nrPUSCHDMRSConfig object. PUSCH DM-RS
                          // configuration parameters, specified as an nrPUSCHDMRSConfig configuration object.
  uint8_t EnablePTRS; // 0:Disabled(default) | 1:Enabled. Enable the PT-RS, specified as one of these values. 0 (false)
                      // — Disable the PT-RS configuration. 1 (true) — Enable the PT-RS configuration.
  nrPDSCHPTRSConfig
      PTRS; // default nrPUSCHPTRSConfig object (default) | nrPUSCHPTRSConfig object. PUSCH phase tracking reference
            // signal (PT-RS) configuration, specified as an nrPUSCHPTRSConfig configuration object.
  float    TargetCodeRate;
  uint16_t rv;
  uint16_t NumCodewords;
  uint8_t  VRBToPRBInterleaving;
  uint16_t VRBBundleSize;
  uint32_t ReservedRE[275];
  uint32_t ReservedRE_length;
} __attribute__((aligned(64))) nrPDSCHConfig;

typedef struct DCIFormat1_0_SIRNTI {
  uint16_t FrequencyDomainResources;
  uint16_t TimeDomainResources;
  uint8_t  VRBToPRBMapping;
  uint8_t  ModulationCoding;
  uint8_t  RedundancyVersion;
  uint8_t  SystemInformationIndicator;
  uint8_t  ReservedBits;
  uint16_t AlignedWidth;
  uint16_t Width;
  uint8_t  PaddingWidth;
} __attribute__((aligned(64))) DCIFormat1_0_SIRNTI;

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

typedef struct MCC_MNC {
  char    MCC[3];
  char    MNC[3];
  uint8_t MNC_Len;
} __attribute__((aligned(64))) MCC_MNC;

/********************** struct data type **************************/
typedef struct {
  char data;
} __attribute__((aligned(64))) char_struct;

typedef struct {
  short data;
} __attribute__((aligned(64))) short_struct;

typedef struct {
  int data;
} __attribute__((aligned(64))) int_struct;

typedef struct {
  float data;
} __attribute__((aligned(64))) float_struct;

typedef struct {
  double data;
} __attribute__((aligned(64))) double_struct;

#ifdef __cplusplus
}
#endif

#endif
