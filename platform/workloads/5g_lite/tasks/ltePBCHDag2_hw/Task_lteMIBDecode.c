/**
 * ****************************************
 * @file        Task_lteMIBDecode.c
 * @brief       MIB decode
 * @author      yuanfeng
 * @date        2024.7.15
 * @copyright   ACE-Lab(Shanghai University)
 * ****************************************
 */

 #include "riscv_printf.h"
 #include "venus.h"
 
 
 typedef short __v2048i16 __attribute__((ext_vector_type(2048)));
 typedef char  __v4096i8 __attribute__((ext_vector_type(4096)));
 
 typedef struct ltePHICHConfig {
   short PHICHDuration;//normal, extended
   short PHICHResource;//Ng:oneSixth, half, one, two
 } __attribute__((aligned(64))) ltePHICHConfig;
 
 typedef struct MIB {
   short            NDLRB; //6 15 25 50 75 100
   ltePHICHConfig phich_Config; 
   short            NFrame;//most8+least2
   short            schedulingInfoSIB1_BR_r13;
   short            systemInfoUnchanged_BR_r15;
   short            partEARFCN_r17_flag;//0-space 1-earfn_LSB
   short            earfcn_LSB; 
   short            spare;
 } __attribute__((aligned(64))) MIB;
 
 typedef struct {
   short data;
 } __attribute__((aligned(64))) short_struct;
 
 int Task_lteMIBDecode(__v4096i8 mib, short_struct NFMod4Num) {
   int     bitOffset = 0;
   MIB     mibInfo;
   short   sfn4lsb   = NFMod4Num.data;

   int dl_Bandwidth = 0;
   vbarrier();
   VSPM_OPEN();
   int    mib_addr1 = vaddr(mib);
   for (int i = 0; i < 3; ++i)
    dl_Bandwidth += (*(volatile char *)(mib_addr1 + bitOffset + i)) << (3 - i - 1);
   VSPM_CLOSE();

  if(dl_Bandwidth == 0){
      mibInfo.NDLRB = 6;
   }
   else if(dl_Bandwidth == 1){
      mibInfo.NDLRB = 15;
   }
   else if(dl_Bandwidth == 2){
      mibInfo.NDLRB = 25;
   }
   else if(dl_Bandwidth == 3){
      mibInfo.NDLRB = 50;
   }
  else if(dl_Bandwidth == 4){
      mibInfo.NDLRB = 75;
  }
  else{
      mibInfo.NDLRB = 100;
  }
   bitOffset += 3;

   vbarrier();
   VSPM_OPEN();
   int   mib_addr2                    = vaddr(mib);
   mibInfo.phich_Config.PHICHDuration =  (*(volatile char *)(mib_addr2 + bitOffset));
   VSPM_CLOSE();

   bitOffset += 1;

   int phichResNum = 0;
   vbarrier();
   VSPM_OPEN();
   int   mib_addr3 = vaddr(mib);
   for (int i = 0; i < 2; ++i)
      phichResNum += (*(volatile char *)(mib_addr3 + bitOffset + i)) << (2 - i - 1);
   VSPM_CLOSE();

   mibInfo.phich_Config.PHICHResource = phichResNum;
   bitOffset += 2;
  
   int sysFraNum = 0;
   vbarrier();
   VSPM_OPEN();
   int mib_addr4 = vaddr(mib);
   for (int i = 0; i < 8; ++i)
      sysFraNum += (*(volatile char *)(mib_addr4 + bitOffset + i)) << (8 - i - 1);
   VSPM_CLOSE();

   mibInfo.NFrame = sysFraNum * (1 << 2) + sfn4lsb;
   bitOffset += 8;

   int schInfor13Num = 0;
   vbarrier();
   VSPM_OPEN();
   int mib_addr5     = vaddr(mib);
   for (int i = 0; i < 5; ++i)
      schInfor13Num += (*(volatile char *)(mib_addr5 + bitOffset + i)) << (5 - i - 1);
   VSPM_CLOSE();
   mibInfo.schedulingInfoSIB1_BR_r13 = schInfor13Num;
   bitOffset += 5;

   vbarrier();
   VSPM_OPEN();
   int mib_addr6                      = vaddr(mib);
   mibInfo.systemInfoUnchanged_BR_r15 = (*(volatile char *)(mib_addr6 + bitOffset));
   VSPM_CLOSE();
   bitOffset += 1;

   vbarrier();
   VSPM_OPEN();
   int mib_addr7                      = vaddr(mib);
   mibInfo.partEARFCN_r17_flag = (*(volatile char *)(mib_addr7 + bitOffset));
   VSPM_CLOSE();
   bitOffset += 1;

   int earLSBNum  = 0;
   vbarrier();
   VSPM_OPEN();
   int   mib_addr8  = vaddr(mib);
   for (int i = 0; i < 2; ++i)
    earLSBNum += (*(volatile char *)(mib_addr8 + bitOffset + i)) << (2 - i - 1);
   VSPM_CLOSE();
   mibInfo.earfcn_LSB = earLSBNum;
   bitOffset += 2;

   bitOffset += 1;//space
 
    printf("NDLRB:%hd\n", &(mibInfo.NDLRB));
    printf("PHICHDuration:%hd(normal, extended)\n", &(mibInfo.phich_Config.PHICHDuration));
    printf("PHICHResource:%hd(oneSixth, half, one, two)\n", &(mibInfo.phich_Config.PHICHResource));
    printf("NFrame:%hd\n", &(mibInfo.NFrame));
    printf("schedulingInfoSIB1_BR_r13:%hd\n", &(mibInfo.schedulingInfoSIB1_BR_r13));
    printf("systemInfoUnchanged_BR_r15:%hd\n", &(mibInfo.systemInfoUnchanged_BR_r15));
    printf("partEARFCN_r17_flag:%hd(0-space 1-earfn_LSB)\n", &(mibInfo.partEARFCN_r17_flag));
    printf("earfcn_LSB:%hd\n", &(mibInfo.earfcn_LSB));

   vreturn(&mibInfo, sizeof(mibInfo));
 }