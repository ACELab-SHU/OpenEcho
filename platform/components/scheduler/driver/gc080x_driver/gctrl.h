#ifndef  __GCTRL_H
#define  __GCTRL_H

#include "platform.h"

int GCTRL_SETUP_DELAY_CFG(rf_chip_phy_t *phy, TRX_CHN_ENUM chn, GCTRL_SETUP_DELAY_CFG_T *cfg);
int GCTRL_EXT_LNA_CFG(rf_chip_phy_t *phy, TRX_CHN_ENUM chn, GCTRL_EXT_LNA_CFG_T *cfg);
int GCTRL_FULL_TABLE_SET_CONTENT(rf_chip_phy_t *phy, TRX_CHN_ENUM chn, short addr, short data);
int GCTRL_FULL_TABLE_SET_MAX_INDEX(rf_chip_phy_t *phy, TRX_CHN_ENUM chn, short max_full_index);
int GCTRL_FULL_TABLE_SET_INIT_INDEX(rf_chip_phy_t *phy, TRX_CHN_ENUM chn, short init_full_index);
int GCTRL_FULL_TABLE_RPT_CONTENT(rf_chip_phy_t *phy, TRX_CHN_ENUM chn, short addr, short *data);
int GCTRL_FULL_TABLE_CFG(rf_chip_phy_t *phy, TRX_CHN_ENUM chn, GCTRL_FULL_TABLE_CFG_T *cfg);
int GCTRL_FULL_TABLE_CFG_SINGLE_VAL(rf_chip_phy_t *phy, TRX_CHN_ENUM chn, short addr, short data);
int GCTRL_SPLIT_TABLE_CFG(rf_chip_phy_t *phy, TRX_CHN_ENUM chn, GCTRL_SPLIT_TABLE_CFG_T *cfg);
int GCTRL_LMT_PDT_CFG(rf_chip_phy_t *phy, TRX_CHN_ENUM chn, GCTRL_LMT_PEAK_DET_CFG_T *cfg);
int GCTRL_ANA_PEAK_DETECT_ENABLE(rf_chip_phy_t *phy, TRX_CHN_ENUM chn);
int GCTRL_ADC_PDT_CFG(rf_chip_phy_t *phy, TRX_CHN_ENUM chn, GCTRL_ADC_PEAK_DET_CFG_T *cfg);
int GCTRL_BASIC_CFG(rf_chip_phy_t *phy, TRX_CHN_ENUM chn, GCTRL_BASIC_CFG_T *cfg);
int GCTRL_DIG_AGC_ENA(rf_chip_phy_t *phy, TRX_CHN_ENUM chn, GCTRL_DIG_AGC_CFG_T *cfg);
int GCTRL_HBF_PWR_MEAS_CFG(rf_chip_phy_t *phy, TRX_CHN_ENUM chn, GCTRL_HBF_PWR_MEAS_CFG_T *cfg);
int GCTRL_HBF_LPWR_DET_CFG(rf_chip_phy_t *phy, TRX_CHN_ENUM chn, GCTRL_HBF_LOW_PWR_CFG_T *cfg);
int GCTRL_HBF_OVF_DET_CFG(rf_chip_phy_t *phy, TRX_CHN_ENUM chn, GCTRL_HBF_OVF_DET_CFG_T *cfg);
void GCTRL_CFG(rf_chip_phy_t *phy, TRX_CHN_ENUM chn,RX_GCTRL_CFG_T * gctrl_cfg);
int GCTRL_SLOW_AGC_CFG(rf_chip_phy_t *phy, TRX_CHN_ENUM chn, GCTRL_SLOW_AGC_CFG_T *cfg);
int GCTRL_FAST_AGC_CFG(rf_chip_phy_t *phy, TRX_CHN_ENUM chn, GCTRL_FAST_AGC_CFG_T *cfg);
int GCTRL_MGC_CFGG(rf_chip_phy_t *phy, TRX_CHN_ENUM chn, GCTRL_MGC_CFGG_T *cfg);
int GCTRL_GET_RPT(rf_chip_phy_t *phy, TRX_CHN_ENUM chn, GCTRL_GET_RPT_T *cfg);
int GCTRL_GET_SPLIT_GAIN(rf_chip_phy_t *phy, TRX_CHN_ENUM chn, short *lmt_index, short *lpf_index);
int GCTRL_MGC_SET_FULL_GAIN(rf_chip_phy_t *phy, TRX_CHN_ENUM chn, short full_tab_inx);
int GCTRL_MGC_SET_SPLIT_GAIN(rf_chip_phy_t *phy, TRX_CHN_ENUM chn, GCTRL_SPLIT_GAIN_T *cfg);
void GCTRL_DEFAULT_CFG(rf_chip_phy_t *phy, RX_GCTRL_CFG_T * gctrl_cfg);
int GCTRL_DEMO_MGC_SPI_MODE(rf_chip_phy_t *phy, TRX_CHN_ENUM chn, TABLE_MODE_ENUM mode);
int GCTRL_DEMO_MGC_PIN_MODE(rf_chip_phy_t *phy, TRX_CHN_ENUM chn, TABLE_MODE_ENUM mode);
int GCTRL_DEMO_AGC_SLOW(rf_chip_phy_t *phy, TRX_CHN_ENUM chn, TABLE_MODE_ENUM mode);
int GCTRL_DEMO_AGC_FAST(rf_chip_phy_t *phy, TRX_CHN_ENUM chn, TABLE_MODE_ENUM mode);
int GCTRL_DEMO_AGC_FAST_HHH(rf_chip_phy_t *phy, TRX_CHN_ENUM chn, TABLE_MODE_ENUM mode);

#endif //__GCTRL_H

