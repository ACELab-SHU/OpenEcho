#ifndef  __MAIN_INIT_H
#define  __MAIN_INIT_H

#include "platform.h"

extern int detect_chip(rf_chip_phy_t *phy);
extern int gc080x_init(rf_chip_phy_t *phy, chip_config_t *config);
extern int rx_gain_mgc_change(rf_chip_phy_t *phy, TRX_CHN_ENUM chn, RX_MGC_GAIN_ENUM tb, int val);
extern int tdd_trx_change(rf_chip_phy_t *phy, FSM_ST_ENUM st);
extern int tx_atten_change(rf_chip_phy_t *phy, TRX_CHN_ENUM chn, int val, short immed);
extern int tx_ana_atten_change(rf_chip_phy_t *phy, TRX_CHN_ENUM chn, int val, short immed);
extern int tx_split_atten_change(rf_chip_phy_t *phy, TRX_CHN_ENUM chn, TX_SPLIT_ATTEN_ENUM type, unsigned int val, short immed);
extern int trx_lo_change(rf_chip_phy_t *phy, unsigned long long  txlo, unsigned long long  rxlo);
extern int trx_lo_change_ext(rf_chip_phy_t *phy, unsigned char lo_change_mode, unsigned long long  txlo, unsigned long long  rxlo);
extern int trx_lo_change_act(rf_chip_phy_t *phy);
extern int rf_bandwith_change(rf_chip_phy_t *phy, BANDWITH_ENUM bandwith);
extern int tx_port_enable(rf_chip_phy_t *phy, TRX_CHN_ENUM chn, short en);
extern int rx_port_enable(rf_chip_phy_t *phy, TRX_CHN_ENUM chn, short en);
extern int tx_dig_atten_change(rf_chip_phy_t *phy, TRX_CHN_ENUM chn, unsigned short index);
extern void rx_gain_get(rf_chip_phy_t *phy, TRX_CHN_ENUM chn, GCTRL_GET_RPT_T *cfg);
extern void rx_gain_mgc_full_tab_index_set(rf_chip_phy_t *phy, TRX_CHN_ENUM chn, short index);
extern void rf_chip_suspend(rf_chip_phy_t *phy);
extern int rf_chip_resume(rf_chip_phy_t *phy);
extern rf_chip_phy_t *rf_chip_phy_get(short sel);
extern chip_config_t *rf_chip_config_get(short sel);
extern void customer_init(rf_chip_phy_t *phy, chip_config_t *config);
extern short fsm_status_get_only(rf_chip_phy_t *phy);
extern short fsm_status_get(rf_chip_phy_t *phy, short target_st);
extern int trx_ana_bw_adj(rf_chip_phy_t *phy, TRX_ENUM dir, TRX_CHN_ENUM chn, unsigned char val[]);
extern int trx_fir_coef_set(rf_chip_phy_t *phy, short dir, TRX_CHN_ENUM chn, short fir_tap_n, int *fir_tap_coeff);
extern int bybrid_mode_cal_switch(rf_chip_phy_t * phy);
extern void band_dep_calibr_phase_of_bandwidth_calflag_clr(rf_chip_phy_t *phy);
extern void band_dep_calibr_phase_of_bandwidth_calflag_set(rf_chip_phy_t *phy);
extern void cmd_api_dig_fir_cfg_manual(char manual_on);

extern void test_tx_fir_coeff_clear(rf_chip_phy_t *phy, TRX_CHN_ENUM chn);
extern void test_adc_dac_off(rf_chip_phy_t *phy, short dir, TRX_CHN_ENUM chn);
extern void test_rxgain_ctrl_set(rf_chip_phy_t *phy, TRX_CHN_ENUM chn);

#endif  /* __MAIN_INIT_H */

