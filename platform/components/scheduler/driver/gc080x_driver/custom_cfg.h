#ifndef  __USER_CONFIG_H
#define  __USER_CONFIG_H

#include "platform.h"

#define  RF_PHY_NUMBER    (1)

extern short g_phy_select;
extern rf_chip_phy_t g_phy_obj[RF_PHY_NUMBER];
extern chip_config_t g_phy_config[RF_PHY_NUMBER];
extern RX_GCTRL_CFG_T gctrl_dflt_cfg;
extern short g_rx_gain_full_table[43][3];

extern const char g_tx_gain_index[51][3];
extern const unsigned char g_tx_dig_gain[51][4];

#endif

