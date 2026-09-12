/*
 * @Author: YihaoShen
 * @Date: 2024-07-20 23:25:09
 * @LastEditors: YihaoShen
 * @LastEditTime: 2024-07-28 19:07:34
 * @Description: DCI bits decode (only suitable for format 1_0 with CRC
 * scrambled by SI-RNTI)
 * @Reference: 3GPP 38.212 7.3.1.2 DCI formats for scheduling of PDSCH
 *
 * Copyright (c) 2024 by ACE_Lab, All Rights Reserved.
 */
#include "data_type.h"
#include "riscv_printf.h"
#include "venus.h"
#include "vmath.h"

typedef short __v2048i16 __attribute__((ext_vector_type(2048)));
typedef short __v4096i8 __attribute__((ext_vector_type(4096)));

// // input parameter for test
// short   NSizeBWP    = 24;
// uint8_t dcibits[37] = {0, 1, 1, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0,
// 1,
//                        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

/**
 * @Task_name: Task_nrDCIDecode
 * @input:
 *  1. NSizeBWP —— size of Coreset0
 *  2. dcibits —— pdcch decoder result without crc
 * @output:
 *  1. dci —— DCIFormat1_0_SIRNTI
 * */
int Task_nrDCIDecode(nrPDCCHConfig pdcch, __v4096i8 dci_in) {

  int NSizeBWP = pdcch.NSizeBWP;

  uint8_t dcibits[64];
  vbarrier();
  VSPM_OPEN();
  int dci_addr = vaddr(dci_in);
  for (int i = 0; i < 64; i++) {
    dcibits[i] = (*(volatile char *)(dci_addr + i));
  }
  VSPM_CLOSE();

  DCIFormat1_0_SIRNTI dci;
  uint16_t            offset = 0;

  uint16_t FreDomain_length = 0;
  // FreDomain_length          = ceil(log2(NSizeBWP * (NSizeBWP + 1) / 2.0));
  FreDomain_length = ceil_log2(NSizeBWP * (NSizeBWP + 1) >> 1);

  uint8_t FrequencyDomainREBits[FreDomain_length];
  for (int i = 0; i < FreDomain_length; ++i)
    FrequencyDomainREBits[i] = dcibits[i + offset];

  dci.FrequencyDomainResources = 0;
  for (int i = 0; i < FreDomain_length; ++i) {
    dci.FrequencyDomainResources += FrequencyDomainREBits[i] * (1 << (FreDomain_length - i - 1));
  }
  offset += FreDomain_length;

  uint8_t TimeDomainREBits[4];
  for (int i = 0; i < 4; ++i)
    TimeDomainREBits[i] = dcibits[i + offset];
  dci.TimeDomainResources = 0;
  for (int i = 0; i < 4; ++i) {
    dci.TimeDomainResources += TimeDomainREBits[i] * (1 << (4 - i - 1));
  }
  offset += 4;

  dci.VRBToPRBMapping = dcibits[offset];
  offset += 1;

  uint8_t ModulationBits[5];
  for (int i = 0; i < 5; ++i)
    ModulationBits[i] = dcibits[i + offset];

  dci.ModulationCoding = 0;
  for (int i = 0; i < 5; ++i) {
    dci.ModulationCoding += ModulationBits[i] * (1 << (5 - i - 1));
  }
  offset += 5;

  uint8_t RVBits[2];
  for (int i = 0; i < 2; ++i)
    RVBits[i] = dcibits[i + offset];

  dci.RedundancyVersion = 0;
  for (int i = 0; i < 2; ++i) {
    dci.RedundancyVersion += RVBits[i] * (1 << (2 - i - 1));
  }
  offset += 2;

  dci.SystemInformationIndicator = dcibits[offset];
  offset += 1;

  uint8_t ReservedBBits[15];
  for (int i = 0; i < 15; ++i)
    ReservedBBits[i] = dcibits[i + offset];
  dci.ReservedBits = 0;
  for (int i = 0; i < 15; ++i) {
    dci.ReservedBits += ReservedBBits[i] * (1 << (15 - i - 1));
  }
  offset += 15;
  // printf("FrequencyDomainResources:%hd\n", &dci.FrequencyDomainResources);
  // printf("TimeDomainResources:%hd\n", &dci.TimeDomainResources);
  // printf("VRBToPRBMapping:%bd\n", &dci.VRBToPRBMapping);
  // printf("ModulationCoding:%bd\n", &dci.ModulationCoding);
  // printf("RedundancyVersion:%bd\n", &dci.RedundancyVersion);
  // printf("SystemInformationIndicator:%bd\n", &dci.SystemInformationIndicator);
  // printf("ReservedBits:%bd\n", &dci.ReservedBits);

  vreturn(&dci, sizeof(DCIFormat1_0_SIRNTI));
}
