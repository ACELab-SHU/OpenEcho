/**
 * ****************************************
 * @file        Task_lteDCIDecode.c
 * @brief       DCI decode
 * @author      chenxiaoxiao
 * @date        2025.4.18
 * @copyright   ACE-Lab(Shanghai University)
 * ****************************************
 */

 #include "riscv_printf.h"
 #include "venus.h"
 #include "data_type.h"
 #include "vmath.h"

 typedef short __v2048i16 __attribute__((ext_vector_type(2048)));
 typedef char  __v4096i8 __attribute__((ext_vector_type(4096)));
 
 
 typedef struct DCI_SIB1 {
   short            format01_Flag; //Flag for format0A/1A
   short            VRB_assignment;//0 Localized 1 Distributed
   short            Resource_assignment;//Resource block assignment
   short            LCRBs;//Length of used  PDSCH RBs
   short            RBstart;//first RB of PDSCH RBs
   short            MCS;//Modulation and coding scheme 0-28
   short            HARQ_number;//FDD(3)TDD(4)
   short            Newdata_indicator; 
   short            Redundancy_version;
   short            TPC_command;//TPC command for PUCCH
   short            DAI_flag; //only present in TDD (config 1-6)
   short            DAI; //Downlink Assignment Index
 } __attribute__((aligned(64))) DCI_SIB1;
 
 
 int Task_lteDCIDecode(__v4096i8 dci_in, short_struct NDLRB, short_struct frame_type) {

   DCI_SIB1     dciInfo;
   int          NSizeBWP = NDLRB.data;
   short        offset   = 0;

   uint8_t dcibits[64];
   vbarrier();
   VSPM_OPEN();
   int dci_addr = vaddr(dci_in);
   for (int i = 0; i < 64; i++) {
     dcibits[i] = (*(volatile char *)(dci_addr + i));
   }
   VSPM_CLOSE();
   
   dciInfo.format01_Flag = dcibits[offset];
   offset += 1;

   dciInfo.VRB_assignment = dcibits[offset];
   offset += 1;

   short FreDomain_length = 0;
   FreDomain_length = ceil_log2(NSizeBWP * (NSizeBWP + 1) >> 1);
   
   dciInfo.Resource_assignment = 0;
   for (int i = 0; i < FreDomain_length; ++i) {
    dciInfo.Resource_assignment += dcibits[offset + i] << (FreDomain_length - i - 1);
   }
   offset += FreDomain_length;
   
  //  short temp = 0;
  //  if((dciInfo.Resource_assignment/NSizeBWP) <= (NSizeBWP/2)){
  //   dciInfo.LCRBs = (dciInfo.Resource_assignment/NSizeBWP) + 1;
  //   dciInfo.RBstart = dciInfo.Resource_assignment - (dciInfo.LCRBs - 1) * NSizeBWP;
  //  }else{
  //   temp = (dciInfo.Resource_assignment/NSizeBWP);
  //   dciInfo.LCRBs = 0 - temp + 1 + NSizeBWP;
  //   dciInfo.RBstart = 0 - (dciInfo.Resource_assignment - temp * NSizeBWP) -1 +NSizeBWP;
  //  }
  short a = (dciInfo.Resource_assignment/NSizeBWP) + 1;
  short b = dciInfo.Resource_assignment % NSizeBWP;
  if((a+b) > NSizeBWP){
    dciInfo.LCRBs = NSizeBWP + 2 - a;
    dciInfo.RBstart = NSizeBWP - 1 - b;
  }else{
    dciInfo.LCRBs = a;
    dciInfo.RBstart = b;
  }

   dciInfo.MCS = 0;
   for (int i = 0; i < 5; ++i) {
    dciInfo.MCS += dcibits[offset + i] << (5 - i - 1);
   }
   offset += 5;
 
   short HARQ_length = 0;
   if(frame_type.data == 2){//TDD
    HARQ_length = 4;
   }else{
    HARQ_length = 3;//FDD
   }
   dciInfo.HARQ_number = 0;
   for (int i = 0; i < HARQ_length; ++i) {
    dciInfo.HARQ_number += dcibits[offset + i] << (HARQ_length - i - 1);
   }
   offset += HARQ_length;
 
 
   dciInfo.Newdata_indicator = 0;
   dciInfo.Newdata_indicator = dcibits[offset];
   offset += 1;
   
   dciInfo.Redundancy_version = 0;
   for (int i = 0; i < 2; ++i) {
    dciInfo.Redundancy_version += dcibits[offset + i] << (2 - i - 1);
  }
   offset += 2;
 
   dciInfo.TPC_command = 0;
   for (int i = 0; i < 2; ++i) {
    dciInfo.TPC_command += dcibits[offset + i] << (2 - i - 1);
   }
   offset += 2;

   dciInfo.DAI = 0;
   if(frame_type.data == 2){//TDD
    dciInfo.DAI_flag = 1;
    for (int i = 0; i < 2; ++i) {
    dciInfo.DAI += dcibits[offset + i + 1] << (2 - i - 1);
   }
    offset += 2;
   }else{
    dciInfo.DAI_flag = 0;//FDD
   }
    
   if (offset == 12 || offset == 14 || offset == 16 || offset == 20 || offset == 24 || offset == 26 || offset == 32 || offset == 40 || offset == 44 || offset == 56)
   {
    offset += 1;//follow format0 : 27
   }

   printf("dci_length:%hd\n",&offset);

   printf("format01_Flag:%hd\n", &(dciInfo.format01_Flag));
   printf("VRB_assignment:%hd(Localized / Distributed)\n", &(dciInfo.VRB_assignment));
   printf("Resource_assignment:%hd\n", &(dciInfo.Resource_assignment));
   printf("LCRBs:%hd\n", &(dciInfo.LCRBs));
   printf("RBstart:%hd\n", &(dciInfo.RBstart));
   printf("Modulation and coding scheme:%hd\n", &(dciInfo.MCS));
   printf("HARQ_number:%hd\n", &(dciInfo.HARQ_number));
   printf("Newdata_indicator:%hd\n", &(dciInfo.Newdata_indicator));
   printf("Redundancy_version:%hd\n", &(dciInfo.Redundancy_version));
   printf("TPC_command:%hd\n", &(dciInfo.TPC_command));
   printf("DAI_flag:%hd\n", &(dciInfo.DAI_flag));
   printf("DAI:%hd\n", &(dciInfo.DAI));

   vreturn(&dciInfo, sizeof(DCI_SIB1));
 }