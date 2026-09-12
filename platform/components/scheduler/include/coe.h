#ifndef __COE_H_
#define __COE_H_
#include "types.h"

extern int pss0_re[];
extern int pss0_im[];
extern int pss0_re_add_im[];
extern int pss1_re[];
extern int pss1_im[];
extern int pss1_re_add_im[];
extern int pss2_re[];
extern int pss2_im[];
extern int pss2_re_add_im[];

#define foTableItemNumber 5
#define tableItemNumber   5

extern long long freq_offset[];  // for dfe

extern int OperationBand[];
extern int GSCN[];
extern double Frequency[];
extern int SCS[];      // unit: KHz
extern int Pattern[];  //0: Case A || 1: Case B || 2: Case C
// extern short L_max[];

/*
 * In 10.0.0.46 - D:\project_1_cdc_add_mixer > project_1.sdk > 5g_lite_srs > src > common.h
 *  Created on: 2024年10月29日
 *      Author: yuanfeng
 */

// scs:15Khz/30Khz | ssb_pattern:A B C | sample rate:30.72Mhz
#define MAX_SUPPORT_SCS_NUMBER         2
#define MAX_SUPPORT_SSB_PATTERN_NUMBER 3
#define GET_SYMBOL_LENGTH(NSymbol, mu) ((NSymbol == 0) || ((NSymbol == (mu == 0 ? 7 : 0))) ? EXTENDED_CP_SAMPLES[mu] : NORMAL_CP_SAMPLES[mu])

#define TDD 0
#define FDD 1

extern volatile int mu;
extern uint32_t symbolStartTable_Sub3G[][4];
extern uint32_t symbolStartTable_Sub6G[][8];
extern int EXTENDED_CP_SAMPLES[];
extern int NORMAL_CP_SAMPLES[];
extern int SAMPLES_PER_SLOT[];
extern int subframePerFrame;
extern int symbolPerSubframe[];
extern int symbolPerSlot;
extern int NRB_PER_SCS_BW20[];

#endif /* __COE_H_ */
