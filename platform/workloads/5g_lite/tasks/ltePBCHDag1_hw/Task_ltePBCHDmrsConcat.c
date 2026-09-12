/*
 * @Author: Yihao Shen shenyihao@shu.edu.cn
 * @Date: 2025-04-23 00:42:15
 * @LastEditors: Yihao Shen shenyihao@shu.edu.cn
 * @LastEditTime: 2025-04-25 11:11:15
 * @FilePath: /VEMU/AceEcho/tasks/lteSymProc/Task_lteDmrsConcatExtract.c
 * @Description: 
 */
 #include "data_type.h"
 #include "riscv_printf.h"
 #include "venus.h"
 
 typedef short __v2048i16 __attribute__((ext_vector_type(2048)));
 typedef char __v4096i8 __attribute__((ext_vector_type(4096)));
 
 // typedef struct {
 //     short data;
 //   } __attribute__((aligned(64))) short_struct;
   
 
 int Task_ltePBCHDmrsConcat(__v4096i8 dmrs0_r, __v4096i8 dmrs0_i, __v4096i8 dmrs1_r, __v4096i8 dmrs1_i)
 {
    

    __v2048i16 copyindex;
    vclaim(copyindex);
    vrange(copyindex, 12);

    __v4096i8 dmrs0_r_concat;
    __v4096i8 dmrs0_i_concat;
    vclaim(dmrs0_r_concat);
    vclaim(dmrs0_i_concat);
    vshuffle(dmrs0_r_concat, copyindex, dmrs0_r, SHUFFLE_GATHER, 12);
    vshuffle(dmrs0_i_concat, copyindex, dmrs0_i, SHUFFLE_GATHER, 12);
    copyindex = vsadd(copyindex, 12, MASKREAD_OFF, 12);
    vshuffle(dmrs0_r_concat, copyindex, dmrs1_r, SHUFFLE_SCATTER, 12);
    vshuffle(dmrs0_i_concat, copyindex, dmrs1_i, SHUFFLE_SCATTER, 12);

    vreturn(dmrs0_r_concat, 24, dmrs0_i_concat, 24);


 }
 