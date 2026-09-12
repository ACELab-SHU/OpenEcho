#ifndef  __REGS_INIT_H
#define  __REGS_INIT_H

#include "utility.h"

/****************************************************************************************/
// old x4 setting
/****************************************************************************************/
extern const reg_t G_REG_ARRAY_ANA_7B_MAN_X4_EN[];
extern const int G_REG_ARRAY_ANA_7B_MAN_X4_EN_LEN;
extern const reg_t G_REG_ARRAY_ANA_7A_MAN_X2_EN[];
extern const int G_REG_ARRAY_ANA_7A_MAN_X2_EN_LEN;

/****************************************************************************************/
// rx adc cal
/****************************************************************************************/
extern const reg_t G_REG_ARRAY_for_IIP3_RX1_c10_SHA_ibias[];
extern const int G_REG_ARRAY_for_IIP3_RX1_c10_SHA_ibias_LEN;
extern const reg_t G_REG_ARRAY_for_IIP3_RX2_c10_SHA_ibias[];
extern const int G_REG_ARRAY_for_IIP3_RX2_c10_SHA_ibias_LEN;

/****************************************************************************************/
// cal: analog setting
/****************************************************************************************/
extern const reg_t G_REG_ARRAY_tx1_dc[];
extern const int G_REG_ARRAY_tx1_dc_LEN;
extern const reg_t G_REG_ARRAY_tx2_dc[];
extern const int G_REG_ARRAY_tx2_dc_LEN;

extern const reg_t G_REG_ARRAY_rx1_bw_cal1[];
extern const int G_REG_ARRAY_rx1_bw_cal1_LEN;
extern const reg_t G_REG_ARRAY_rx1_bw_cal2[];
extern const int G_REG_ARRAY_rx1_bw_cal2_LEN;
extern const reg_t G_REG_ARRAY_rx1_bw_cal3[];
extern const int G_REG_ARRAY_rx1_bw_cal3_LEN;
extern const reg_t G_REG_ARRAY_rx2_bw_cal1[];
extern const int G_REG_ARRAY_rx2_bw_cal1_LEN;
extern const reg_t G_REG_ARRAY_rx2_bw_cal2[];
extern const int G_REG_ARRAY_rx2_bw_cal2_LEN;
extern const reg_t G_REG_ARRAY_rx2_bw_cal3[];
extern const int G_REG_ARRAY_rx2_bw_cal3_LEN;

extern const reg_t G_REG_ARRAY_6A_TXDAC_ON[];
extern const int G_REG_ARRAY_6A_TXDAC_ON_LEN;
extern const reg_t G_REG_ARRAY_6A_TXDAC_ON_2[];
extern const int G_REG_ARRAY_6A_TXDAC_ON_2_LEN;

/****************************************************************************************/
// power up a: mbias && xo syspll on
/****************************************************************************************/
extern const reg_t G_REG_ARRAY_Power_Up_A_1_SYSPLL_LTE20[];
extern const int G_REG_ARRAY_Power_Up_A_1_SYSPLL_LTE20_LEN;

extern const reg_t G_REG_ARRAY_Power_Up_A_2_ANA_MANUAL_ON[];
extern const int G_REG_ARRAY_Power_Up_A_2_ANA_MANUAL_ON_LEN;

extern const reg_t G_REG_ARRAY_Power_Up_B_1_SXTXRX_INIT_LTE20[];
extern const int G_REG_ARRAY_Power_Up_B_1_SXTXRX_INIT_LTE20_LEN;

extern const reg_t G_REG_ARRAY_Power_Up_B_2_MAN_RVDD18[];
extern const int G_REG_ARRAY_Power_Up_B_2_MAN_RVDD18_LEN;

/****************************************************************************************/
// power up e: sx x4 selection
/****************************************************************************************/
extern const reg_t G_REG_ARRAY_Power_Up_F_1_MAN_X2_EN[];
extern const int G_REG_ARRAY_Power_Up_F_1_MAN_X2_EN_LEN;

extern const reg_t G_REG_ARRAY_Power_Up_F_2_MAN_X4_EN[];
extern const int G_REG_ARRAY_Power_Up_F_2_MAN_X4_EN_LEN;

/****************************************************************************************/
// power up f: initialization(LUT,SX,TX,RX,ADC,DAC...)
/****************************************************************************************/
extern const reg_t G_REG_ARRAY_Power_Up_E_0_1_SXTX_wFCAL_ON_VCO10380_XO30p72_X4[];
extern const int G_REG_ARRAY_Power_Up_E_0_1_SXTX_wFCAL_ON_VCO10380_XO30p72_X4_LEN;

extern const reg_t G_REG_ARRAY_Power_Up_E_0_2_SXRX_wFCAL_ON_VCO9600_XO30p72_X4[];
extern const int G_REG_ARRAY_Power_Up_E_0_2_SXRX_wFCAL_ON_VCO9600_XO30p72_X4_LEN;

extern const reg_t G_REG_ARRAY_Power_Up_E_0_3_LO_CONFIG_2T2R[];
extern const int G_REG_ARRAY_Power_Up_E_0_3_LO_CONFIG_2T2R_LEN;

extern const reg_t G_REG_ARRAY_Power_Up_E_1_RX1FE_BBF_ON_DiffInput_CPCN_PORTA_Remove_6D1[];
extern const int G_REG_ARRAY_Power_Up_E_1_RX1FE_BBF_ON_DiffInput_CPCN_PORTA_Remove_6D1_LEN;

extern const reg_t G_REG_ARRAY_Power_Up_E_2_RX2FE_BBF_ON_DiffInput_CPCN_PORTA_Remove_6E1[];
extern const int G_REG_ARRAY_Power_Up_E_2_RX2FE_BBF_ON_DiffInput_CPCN_PORTA_Remove_6E1_LEN;

extern const reg_t G_REG_ARRAY_Power_Up_E_3_RX1_ADC_with_ofst_gain_cal[];
extern const int G_REG_ARRAY_Power_Up_E_3_RX1_ADC_with_ofst_gain_cal_LEN;

extern const reg_t G_REG_ARRAY_Power_Up_E_4_RX2_ADC_with_ofst_gain_cal[];
extern const int G_REG_ARRAY_Power_Up_E_4_RX2_ADC_with_ofst_gain_cal_LEN;

extern const reg_t G_REG_ARRAY_Power_Up_E_5_TXDAC_TX1[];
extern const int G_REG_ARRAY_Power_Up_E_5_TXDAC_TX1_LEN;

extern const reg_t G_REG_ARRAY_Power_Up_E_6_TXDAC_TX2[];
extern const int G_REG_ARRAY_Power_Up_E_6_TXDAC_TX2_LEN;

extern const reg_t G_REG_ARRAY_Power_Up_E_7_TX1_BBF_L20_IQM_HRMDIV4_PA_PORTA_IFLPBK_Remove_702[];
extern const int G_REG_ARRAY_Power_Up_E_7_TX1_BBF_L20_IQM_HRMDIV4_PA_PORTA_IFLPBK_Remove_702_LEN;

extern const reg_t G_REG_ARRAY_Power_Up_E_8_TX2_BBF_L20_IQM_HRMDIV4_PA_PORTA_IFLPBK_Remove_74A[];
extern const int G_REG_ARRAY_Power_Up_E_8_TX2_BBF_L20_IQM_HRMDIV4_PA_PORTA_IFLPBK_Remove_74A_LEN;

extern const reg_t G_REG_ARRAY_Power_Up_E_0_4_LO_CONFIG_2T2R_TDD[];
extern const int G_REG_ARRAY_Power_Up_E_0_4_LO_CONFIG_2T2R_TDD_LEN;

/****************************************************************************************/
// wait a: FSM Wait State Entry and Manual-Control Enable
/****************************************************************************************/
extern const reg_t G_REG_ARRAY_Wait_A_1_FORECE_WAIT[];
extern const int G_REG_ARRAY_Wait_A_1_FORECE_WAIT_LEN;

extern const reg_t G_REG_ARRAY_Wait_A_5_RG_FC_TURN_OFF[];
extern const int G_REG_ARRAY_Wait_A_5_RG_FC_TURN_OFF_LEN;

/****************************************************************************************/
// wait h: digital config
/****************************************************************************************/
extern const reg_t G_REG_ARRAY_Wait_H_1_DIG_MANUAL_OFF_RSNT[];
extern const int G_REG_ARRAY_Wait_H_1_DIG_MANUAL_OFF_RSNT_LEN;

/****************************************************************************************/
// wait i: manual-control turn-off
/****************************************************************************************/
extern const reg_t G_REG_ARRAY_Wait_I_1_ANA_MANUAL_OFF[];
extern const int G_REG_ARRAY_Wait_I_1_ANA_MANUAL_OFF_LEN;

/****************************************************************************************/
// ENSM Control: fdd
/****************************************************************************************/
extern const reg_t G_REG_ARRAY_DIG_2_FORCE_AlERT[];
extern const int G_REG_ARRAY_DIG_2_FORCE_AlERT_LEN;

extern const reg_t G_REG_ARRAY_DIG_3A_FSM_STATE_EN_FDD_1T1R[];
extern const int G_REG_ARRAY_DIG_3A_FSM_STATE_EN_FDD_1T1R_LEN;

extern const reg_t G_REG_ARRAY_DIG_RX1_mgc_start_maxium_gain[];
extern const int G_REG_ARRAY_DIG_RX1_mgc_start_maxium_gain_LEN;

extern const reg_t G_REG_ARRAY_17C_MGC_DIGITAL_CFG_MAX_GAIN[];
extern const int G_REG_ARRAY_17C_MGC_DIGITAL_CFG_MAX_GAIN_LEN;

extern const reg_t G_REG_ARRAY_DIG_2_2_FDD_FORCE_AlERT[];
extern const int G_REG_ARRAY_DIG_2_2_FDD_FORCE_AlERT_LEN;


/****************************************************************************************/
// ENSM Control: tdd
/****************************************************************************************/
extern const reg_t G_REG_ARRAY_TDD_wirecontrol_ENABLE[];
extern const int G_REG_ARRAY_TDD_wirecontrol_ENABLE_LEN;

extern const reg_t G_REG_ARRAY_TDD_wirecontrol_WAIT_TO_ALERT[];
extern const int G_REG_ARRAY_TDD_wirecontrol_WAIT_TO_ALERT_LEN;

extern const reg_t G_REG_ARRAY_TDD_wirecontrol_ALERT_TO_RX[];
extern const int G_REG_ARRAY_TDD_wirecontrol_ALERT_TO_RX_LEN;

extern const reg_t G_REG_ARRAY_TDD_wirecontrol_RX_TO_WAIT[];
extern const int G_REG_ARRAY_TDD_wirecontrol_RX_TO_WAIT_LEN;

extern const reg_t G_REG_ARRAY_TDD_wirecontrol_ALERT_TO_TX[];
extern const int G_REG_ARRAY_TDD_wirecontrol_ALERT_TO_TX_LEN;

extern const reg_t G_REG_ARRAY_TDD_wirecontrol_TX_TO_WAIT[];
extern const int G_REG_ARRAY_TDD_wirecontrol_TX_TO_WAIT_LEN;

extern const reg_t G_REG_ANA_PEAK_RX1_ENABLE[];
extern const int G_REG_ANA_PEAK_RX1_ENABLE_LEN;

extern const reg_t G_REG_ANA_PEAK_RX2_ENABLE[];
extern const int G_REG_ANA_PEAK_RX2_ENABLE_LEN;

#endif /* __REGS_INIT_H */
