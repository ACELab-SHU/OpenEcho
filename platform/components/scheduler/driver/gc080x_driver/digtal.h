#ifndef  __DIGTAL_H
#define  __DIGTAL_H

#include "platform.h"

int DIG_LDO_CFG(rf_chip_phy_t *phy, BANDWITH_ENUM bandwith);
int DIG_FIR_FILTER_CFG_WITH_MANUAL(rf_chip_phy_t *phy, short manual_on);
int DIG_RX_FILTER_CFG(rf_chip_phy_t *phy, TRX_CHN_ENUM chn, BANDWITH_ENUM bandwith, short fir_en);
int DIG_TX_FILTER_CFG(rf_chip_phy_t *phy, TRX_CHN_ENUM chn, BANDWITH_ENUM bandwith, short fir_en);
int DIG_FIR_FILTER_READ(rf_chip_phy_t *phy, short dir, TRX_CHN_ENUM chn, BANDWITH_ENUM bandwith, short fir_tap_n, unsigned short *fir_tap_coeff);
int DIG_FIR_FILTER_CFG(rf_chip_phy_t *phy, short manual_on, short dir, TRX_CHN_ENUM chn, BANDWITH_ENUM bandwith, short fir_tap_n, int *fir_tap_coeff, short rw_lut_flag);
int DIG_TX_DAC_MANUAL_ENA(rf_chip_phy_t *phy, TRX_CHN_ENUM chn, short manual_on);
int DIG_INTF_CFG(rf_chip_phy_t *phy, DIG_IF_ENUM dif, IF_TYPE_ENUM port, DATA_RATE_ENUM rate, PHY_MODE_ENUM mode);
int DIG_ENSM_CFG(rf_chip_phy_t *phy, short manual_on);
int DIG_INTF_CLK_CFG(rf_chip_phy_t *phy, BANDWITH_ENUM bandwith, DATA_RATE_MODE_ENUM data_rate);
int DIG_CHAN_ENA(rf_chip_phy_t *phy, short dir, TRX_CHN_ENUM chn, short enable);
int ENSM_MANAUL_ENA(rf_chip_phy_t *phy, short manual_on, ENSM_STATE_ENUM stat);
int DIG_SOFT_RST_N(rf_chip_phy_t *phy, short dig_reset_enable);
int DIG_DCLK_DELAY_CFG(rf_chip_phy_t *phy, short fpga_num, short val);

#endif //__DIGTAL_H

