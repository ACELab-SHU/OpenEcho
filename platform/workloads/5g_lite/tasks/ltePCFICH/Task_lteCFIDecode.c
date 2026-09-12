/**
  ******************************************************************************
  * @file           : Task_lteCFIDecode.c
  * @author         : XiaoxiaoChen
  * @brief          : LTE PCFICH CFI Decode
  * @attention      : Indices start from 0 of a slot
  * @date           : 2024/1/6
  ******************************************************************************
  */

#include "riscv_printf.h"
#include "venus.h"
#include "stdint.h"
#include "data_type.h"
#include "vmath.h"

typedef short __v2048i16 __attribute__((ext_vector_type(2048)));
typedef short __v4096i16 __attribute__((ext_vector_type(4096)));
typedef char  __v4096i8 __attribute__((ext_vector_type(4096)));

// typedef struct {
//   short data;
// } __attribute__((aligned(64))) short_struct1;

int Task_lteCFIDecode(__v4096i8 softbit, __v4096i8 cfibit1, __v4096i8 cfibit2, __v4096i8 cfibit3) {

    softbit = vslt(softbit, 0, MASKREAD_OFF, MASKWRITE_OFF,32);

    __v4096i8 cfiflag1;
    __v4096i8 cfiflag2;
    __v4096i8 cfiflag3;
    vclaim(cfiflag1);
    vclaim(cfiflag2);
    vclaim(cfiflag3);

    cfiflag1 = vxor(softbit, cfibit1, MASKREAD_OFF, 32);
    cfiflag2 = vxor(softbit, cfibit2, MASKREAD_OFF, 32);
    cfiflag3 = vxor(softbit, cfibit3, MASKREAD_OFF, 32);

    cfiflag1 = vredsum(cfiflag1,MASKREAD_OFF,32);
    cfiflag2 = vredsum(cfiflag2,MASKREAD_OFF,32);
    cfiflag3 = vredsum(cfiflag3,MASKREAD_OFF,32);

    vbarrier();
    VSPM_OPEN();
    int cfiflag1_addr = vaddr(cfiflag1);
      int cfiflagNum1 = *(volatile signed char *)(cfiflag1_addr);
    VSPM_CLOSE();

    vbarrier();
    VSPM_OPEN();
    int cfiflag2_addr = vaddr(cfiflag2);
      int cfiflagNum2 = *(volatile signed char *)(cfiflag2_addr);
    VSPM_CLOSE();

    vbarrier();
    VSPM_OPEN();
    int cfiflag3_addr = vaddr(cfiflag3);
      int cfiflagNum3 = *(volatile signed char *)(cfiflag3_addr);
    VSPM_CLOSE();

    short_struct cfi;
    if(cfiflagNum1 < cfiflagNum2 && cfiflagNum1 < cfiflagNum3) {
      cfi.data = 1;
    } else if(cfiflagNum2 < cfiflagNum1 && cfiflagNum2 < cfiflagNum3) {
      cfi.data = 2;
    } else if(cfiflagNum3 < cfiflagNum1 && cfiflagNum3 < cfiflagNum2) {
      cfi.data = 3;
    }else{//reserved
      cfi.data = 4;
    }

  printf("cfi:%hd \n",&cfi.data);  
  vreturn(&cfi, sizeof(cfi));
}
