#include "driver.h"
#include "custom_cfg.h"
#include "digtal.h"
#include "gctrl.h"
#include "limit_efuse.h"
#include "main_init.h"
#include "platform.h"
#include "qec_chip.h"
#include "register.h"
#include "regs_init.h"
#include "utility.h"

#define _RX_BBF_6DB_GAIN 0x06
#define _RX_BBF_GAIN     0x0c

typedef struct syspll_select_struct {
  unsigned short addr;
  /*2.5GHz<=Fvco<3G*/
  unsigned char val1;
  /*2.2GHz<=Fvco<2.5G*/
  unsigned char val2;
  /*1.8GHz<=Fvco<2.2G*/
  unsigned char val3;
  /*1.25G<Fvco<1.8G*/
  unsigned char val4;
} syspll_select_t;

typedef struct syspll_range_struct {
  unsigned long long f_max;
  unsigned long long f_min;
} syspll_range_t;

void module_debug_onoff(rf_chip_phy_t* phy, unsigned long en) {
  phy->module_debug = en;
}

unsigned long module_debug_get(rf_chip_phy_t* phy) {
  return phy->module_debug;
}

int tx_custom_bw_setting(rf_chip_phy_t* phy, unsigned long custom_bandwith, BANDWITH_ENUM bw_index) {
  unsigned long val, ctune;

  if (custom_bandwith <= g_bandwith[4]) {
    phy->tx_bw_setting[0] = 0x0;

    val                   = 1600000000UL / custom_bandwith;
    val                   = (val > 255) ? 255 : val;
    phy->tx_bw_setting[1] = val;
    phy->tx_bw_setting[2] = val;

    phy->tx_bw_setting[3] = 128;

    val                   = 14 + 40000000UL / custom_bandwith;
    phy->tx_bw_setting[4] = val;

    phy->tx_bw_setting[5] = 60;
    phy->tx_bw_setting[6] = 154;
    phy->tx_bw_setting[7] = 113;

    val                   = 4 + 80000000UL / custom_bandwith;
    phy->tx_bw_setting[8] = val;

    phy->tx_bw_setting[9]  = 50;
    phy->tx_bw_setting[10] = 50;
    phy->tx_bw_setting[11] = 200;
    phy->tx_bw_setting[12] = 0;
    phy->tx_bw_setting[13] = 128;
    phy->tx_bw_setting[14] = 128;
  } else {
    phy->tx_bw_setting[0] = 58;

    val                   = 2 + 800000000UL / custom_bandwith;
    val                   = (val > 255) ? 255 : val;
    phy->tx_bw_setting[1] = val;

    phy->tx_bw_setting[2] = 58;
    phy->tx_bw_setting[3] = 80;
    phy->tx_bw_setting[4] = 8;
    phy->tx_bw_setting[5] = 60;
    phy->tx_bw_setting[6] = 148;
    phy->tx_bw_setting[7] = 161;
    phy->tx_bw_setting[8] = 4;

    phy->tx_bw_setting[9]  = 50;
    phy->tx_bw_setting[10] = 50;
    phy->tx_bw_setting[11] = 200;
    phy->tx_bw_setting[12] = 128;
    phy->tx_bw_setting[13] = 128;
    phy->tx_bw_setting[14] = 128;
  }

  return 0;
}

int rx_custom_bw_setting(rf_chip_phy_t* phy, unsigned long custom_bandwith, BANDWITH_ENUM bw_index) {
  unsigned long val, ctune;

  if (custom_bandwith <= 1500000) {
    phy->rx_bw_setting[0]  = 127;
    phy->rx_bw_setting[1]  = 63;
    phy->rx_bw_setting[2]  = 63;
    phy->rx_bw_setting[3]  = 63;
    phy->rx_bw_setting[4]  = 63;
    phy->rx_bw_setting[5]  = 63;
    phy->rx_bw_setting[6]  = 16;
    phy->rx_bw_setting[7]  = 144;
    phy->rx_bw_setting[8]  = 16;
    phy->rx_bw_setting[9]  = 16;
    phy->rx_bw_setting[10] = 255;

    ctune                  = 640000000ULL / custom_bandwith;
    val                    = ctune >> 8;
    phy->rx_bw_setting[11] = val;

    val                    = ctune - (val << 8);
    phy->rx_bw_setting[12] = val;
  } else if (custom_bandwith <= 3000000) {
    phy->rx_bw_setting[0]  = 127;
    phy->rx_bw_setting[1]  = 63;
    phy->rx_bw_setting[2]  = 63;
    phy->rx_bw_setting[3]  = 63;
    phy->rx_bw_setting[4]  = 63;
    phy->rx_bw_setting[5]  = 63;
    phy->rx_bw_setting[6]  = 8;
    phy->rx_bw_setting[7]  = 136;
    phy->rx_bw_setting[8]  = 8;
    phy->rx_bw_setting[9]  = 8;
    phy->rx_bw_setting[10] = 255;

    ctune                  = 1280000000ULL / custom_bandwith;
    val                    = ctune >> 8;
    phy->rx_bw_setting[11] = val;

    val                    = ctune - (val << 8);
    phy->rx_bw_setting[12] = val;

  } else if (custom_bandwith < 6000000) {
    phy->rx_bw_setting[0]  = 127;
    phy->rx_bw_setting[1]  = 59;
    phy->rx_bw_setting[2]  = 61;
    phy->rx_bw_setting[3]  = 61;
    phy->rx_bw_setting[4]  = 61;
    phy->rx_bw_setting[5]  = 61;
    phy->rx_bw_setting[6]  = 4;
    phy->rx_bw_setting[7]  = 132;
    phy->rx_bw_setting[8]  = 4;
    phy->rx_bw_setting[9]  = 4;
    phy->rx_bw_setting[10] = 255;

    ctune                  = 2560000000ULL / custom_bandwith;
    val                    = ctune >> 8;
    phy->rx_bw_setting[11] = val;

    val                    = ctune - (val << 8);
    phy->rx_bw_setting[12] = val;
  } else if (custom_bandwith < 12000000) {
    phy->rx_bw_setting[0]  = 127;
    phy->rx_bw_setting[1]  = 59;
    phy->rx_bw_setting[2]  = 61;
    phy->rx_bw_setting[3]  = 61;
    phy->rx_bw_setting[4]  = 61;
    phy->rx_bw_setting[5]  = 61;
    phy->rx_bw_setting[6]  = 2;
    phy->rx_bw_setting[7]  = 130;
    phy->rx_bw_setting[8]  = 2;
    phy->rx_bw_setting[9]  = 2;
    phy->rx_bw_setting[10] = 255;

    ctune                  = 5120000000ULL / custom_bandwith;
    val                    = ctune >> 8;
    phy->rx_bw_setting[11] = val;

    val                    = ctune - (val << 8);
    phy->rx_bw_setting[12] = val;
  } else if (custom_bandwith < 70000000) {
    phy->rx_bw_setting[0]  = 127;
    phy->rx_bw_setting[1]  = 59;
    phy->rx_bw_setting[2]  = 56;
    phy->rx_bw_setting[3]  = 56;
    phy->rx_bw_setting[4]  = 56;
    phy->rx_bw_setting[5]  = 56;
    phy->rx_bw_setting[6]  = 1;
    phy->rx_bw_setting[7]  = 129;
    phy->rx_bw_setting[8]  = 1;
    phy->rx_bw_setting[9]  = 1;
    phy->rx_bw_setting[10] = (int)((56.0 * 1000 * 1000 / custom_bandwith / 1.0) * 20);

    ctune                  = 10240000000ULL / custom_bandwith;
    val                    = ctune >> 8;
    phy->rx_bw_setting[11] = val;

    val                    = ctune - (val << 8);
    phy->rx_bw_setting[12] = val;
  } else {
    phy->rx_bw_setting[0]  = 127;
    phy->rx_bw_setting[1]  = 59;
    phy->rx_bw_setting[2]  = 56;
    phy->rx_bw_setting[3]  = 56;
    phy->rx_bw_setting[4]  = 56;
    phy->rx_bw_setting[5]  = 56;
    phy->rx_bw_setting[6]  = 3;
    phy->rx_bw_setting[7]  = 131;
    phy->rx_bw_setting[8]  = 1;
    phy->rx_bw_setting[9]  = 1;
    phy->rx_bw_setting[10] = 0;

    ctune                  = 5600000000ULL / custom_bandwith;
    val                    = ctune >> 8;
    phy->rx_bw_setting[11] = val;

    val                    = ctune - (val << 8);
    phy->rx_bw_setting[12] = val;
  }

  return 0;
}

short proximity_bandwith_seek(rf_chip_phy_t* phy, unsigned long custom_bandwith) {

  short i;
  BANDWITH_ENUM bw_index = phy->config->bandwith;

  rx_custom_bw_setting(phy, custom_bandwith, bw_index);
  tx_custom_bw_setting(phy, custom_bandwith, bw_index);

  return bw_index;
}

short rf_chip_phy_obj_init(rf_chip_phy_t* phy) {
  short i, j;
  short sxtrx_perf_setting[10][4] = {
      {0x46, 0x16, 0x44, 0x14},  //CH1_FDD
      {0x8A, 0x16, 0x88, 0x14},  //RX1_TX2_FDD
      {0x46, 0x2A, 0x44, 0x28},  //RX2_TX1_FDD
      {0x8A, 0x2A, 0x88, 0x28},  //CH2_FDD
      {0xCE, 0x3E, 0xCC, 0x3C},  //CH1_CH2_FDD
      {0x56, -1, 0x54, -1},      //CH1_TDD
      {0xAA, -1, 0xA8, -1},      //CH2_TDD
      {0xFE, -1, 0xFC, -1},      //CH1_CH2_TDD
      {0xFE, -1, 0xFC, -1},      //H_CH1CH2_FDD
      {0xCE, 0x3E, 0xCC, 0x3C},  //H_CH1CH2_TDD
  };

  for (i = 0; i < 10; i++)
    for (j = 0; j < 4; j++)
      phy->config->sxtrx_perf_setting[i][j] = sxtrx_perf_setting[i][j];

  phy->config->rx_twin_fxo        = 128;
  phy->config->rx_tsu_fxo         = 32;
  phy->config->rx_tstate2_fsys_us = 100;

  phy->config->tx_twin_fxo        = 128;
  phy->config->tx_tsu_fxo         = 32;
  phy->config->tx_tstate2_fsys_us = 100;
  phy->config->dll_twin_fxo       = 128;
  phy->config->dll_tsu_fxo        = 256;
  phy->config->tstate7_fsys_us    = 10;

  phy->config->rx_adc_cal_flag = 0;
  phy->config->tx_bw_cal_flag  = 0;
  phy->config->rxfe_gain[0]    = RX_PORT_G0;
  phy->config->rxfe_gain[1]    = RX_PORT_G0;
  phy->config->rcal_read       = R_CAL_CNT_BIT0_IN_REG850;

  phy->config->fvco_limit = 7500000000ULL;

  phy->bandwith         = BW_CNT;
  phy->st               = AT_FSM_SLEEP;
  phy->rx_dc_lut_update = 1;
  phy->min_vco          = 0;  //U64_MAX_VAL;

  GCTRL_DEFAULT_CFG(phy, &(phy->rx_gain_ctrl[0]));
  GCTRL_DEFAULT_CFG(phy, &(phy->rx_gain_ctrl[1]));

  for (i = 0; i < 50; i++)
    for (j = 0; j < 16; j++)
      phy->tx_bb_gain_config[i][j] = g_tx_bb_gain_config[i][j];

  return 0;
}

void sxtrx_cfg_switch(rf_chip_phy_t* phy) {
  if (!phy->debug_on) {
    switch (phy->config->chip_ver) {
      case 0:
        phy->config->reg61A_val = 0x4f;
        phy->config->reg901_val = 0xd0;
        phy->config->reg600_val = 0x02;
        phy->config->reg602_val = 0x00;
        phy->config->fvco_min   = 6320000000ULL;
        phy->config->sxlf_ldo   = 1000;
        break;
      case 1:
        phy->config->reg61A_val = 0x4f;
        phy->config->reg901_val = 0xd0;
        phy->config->reg600_val = 0x02;
        phy->config->reg602_val = 0x00;
        phy->config->fvco_min   = 6300000000ULL;
        phy->config->sxlf_ldo   = 1200;
        break;
      case 2:
        phy->config->reg61A_val = 0x43;
        phy->config->reg901_val = 0xd0;
        phy->config->reg600_val = 0x02;
        phy->config->reg602_val = 0x00;
        phy->config->fvco_min   = 6300000000ULL;
        phy->config->sxlf_ldo   = 1200;
        break;
    }
  }
}

/****************************************************************************************/
// set rf bandwidth
/****************************************************************************************/
void set_rf_bandwidth(rf_chip_phy_t* phy, BANDWITH_ENUM bandwith) {
  unsigned char bw_config[9] =
      {
          0x00,  //GSM
          0x11,  //BT
          0x22,  //LTE5
          0x33,  //LTE10
          0x44,  //LTE20
          0x55,  //LTE40
          0x66,  //LTE60
          0x77,  //LTE80
          0x88,  //LTE100
      };

  //if (bandwith == BW_LTE100)
  //	hal_spi_write_reg(phy, 0x904, 0xb6);//Supply voltage
  //else
  //	hal_spi_write_reg(phy, 0x904, 0xb4);//Supply voltage
  hal_spi_write_reg(phy, 0x0C0, bw_config[bandwith]);
}

/****************************************************************************************/
// sx & txlo cal
/****************************************************************************************/
int sx_cal_lut_update(rf_chip_phy_t* phy) {
  unsigned int reg_val1, reg_val2;
  unsigned long long flo;
  int fvco_result, old_result, result_buff;
  unsigned long long tmp;
  int i, fvco_cal_delta = 14;
  const unsigned long long* sx_cal_vco = NULL;

  if (phy->sx_cal_flag) {
    for (i = 0; i < 512; i++) {
      write_lut_byte(phy, SX_CAL_LUT, i, 0x647, i / 2, ((i + 1) % 2) ? 0 : 2, phy->sx_cal_1[i]);
      write_lut_byte(phy, SX_CAL_LUT, i, 0x648, i / 2, ((i + 1) % 2) ? 1 : 3, phy->sx_cal_2[i]);
    }
    return 0;
  }

  fvco_result = 12500;
  flo         = phy->config->tx_flo;

  switch (phy->config->chip_ver) {
    case CHIP_VERSION_D1:
      sx_cal_vco = g_sx_cal_vco_d1;
      break;
    case CHIP_VERSION_D2:
      sx_cal_vco = g_sx_cal_vco_d2;
      break;
    case CHIP_VERSION_E1:
      sx_cal_vco = g_sx_cal_vco_e1;
      break;
    default:
      sx_cal_vco = g_sx_cal_vco_d1;
      LOG_MAIN("%s in line %d the sx cal vco list is default(d1)\n", __FUNCTION__, __LINE__);
      break;
  }

  for (i = 0; i < 512; i++) {
    tmp         = sx_cal_vco[i];
    fvco_result = tmp / MHZ_TO_HZ;
    LOG_MDEBUG(phy, SX_TRX_CAL, "sx cal: fvco_result=%dMhz\n", fvco_result);
    if (fvco_result < 6200)
      do_div(&tmp, 256);
    else
      do_div(&tmp, 2);

    set_trx_lo(phy, TX_DIR, TRX_CHN1, tmp);
    //LOG_MDEBUG(phy, SX_TRX_CAL, "sx cal: tmp/2=%llu\n", tmp);

    reg_val1 = hal_spi_read_reg(phy, 0x022);
    reg_val2 = hal_spi_read_reg(phy, 0x023);
    LOG_MDEBUG(phy, SX_TRX_CAL, "sx cal: tmp=%llu, reg_0x022=0x%x, reg_0x023=0x%x\n", tmp, reg_val1, reg_val2);

    phy->sx_cal_1[i] = reg_val1;
    phy->sx_cal_2[i] = reg_val2;
  }

  phy->sx_cal_flag    = 1;
  phy->config->tx_flo = flo;

  return 0;
}

int txlo_cal_lut_update(rf_chip_phy_t* phy) {
  unsigned int reg_val;
  unsigned long long flo;
  int fvco_result, old_result;
  int i, txlo_cal_delta = 35;

  fvco_result = 550;
  flo         = phy->config->tx_flo;

  if (phy->txlo_cal_flag) {
    for (i = 0; i < 32; i++) {
      write_lut_byte(phy, TXLO_CAL_LUT, i, 0x72B, i, 0, phy->txlo_cal_1[i]);
      write_lut_byte(phy, TXLO_CAL_LUT, i, 0x773, i, 2, phy->txlo_cal_1[i]);
      write_lut_byte(phy, TXLO_CAL_LUT, i, 0x72E, i, 1, phy->txlo_cal_2[i]);
      write_lut_byte(phy, TXLO_CAL_LUT, i, 0x776, i, 3, phy->txlo_cal_2[i]);
    }
  } else {
    for (i = 0; i < 32; i++) {
      phy->config->tx_flo = (unsigned long long)((unsigned long long)fvco_result * MHZ_TO_HZ);
      tx1_band_selection_c2(phy);

      reg_val            = hal_spi_read_reg(phy, 0x037);
      phy->txlo_cal_1[i] = reg_val;
      write_lut_byte(phy, TXLO_CAL_LUT, i, 0x72B, i, 0, reg_val);
      write_lut_byte(phy, TXLO_CAL_LUT, i, 0x773, i, 2, reg_val);

      reg_val            = hal_spi_read_reg(phy, 0x044);
      phy->txlo_cal_2[i] = reg_val;
      write_lut_byte(phy, TXLO_CAL_LUT, i, 0x72E, i, 1, reg_val);
      write_lut_byte(phy, TXLO_CAL_LUT, i, 0x776, i, 3, reg_val);

      old_result     = fvco_result;
      fvco_result    = fvco_result + txlo_cal_delta;
      txlo_cal_delta = fvco_result * txlo_cal_delta / old_result;
    }
    phy->txlo_cal_flag = 1;
  }

  phy->config->tx_flo = flo;
  hal_spi_write_reg(phy, 0x701, 0x37);
  hal_spi_write_reg(phy, 0x749, 0x37);

  return 0;
}

/****************************************************************************************/
// set trx lo
/****************************************************************************************/
short fvco_get_right_index(rf_chip_phy_t* phy, unsigned long long comp_val, char fsm_flag) {
  int fvco_result = 12000, old_result, result_buff;
  short i, cnt, store_index = 0;
  long long tmp, curr_delta = 0, old_delta = 0;
  int fvco_cal_delta = 14, txlo_cal_delta = 35, fvco_delta = 134;  //145;
  const unsigned long long* sx_config_vco = NULL;
  unsigned int sx_config_vco_len          = 0;
  const unsigned long long* sx_cal_vco    = NULL;
  unsigned int sx_cal_vco_len             = 0;

  if (fsm_flag == 0) {
    cnt         = 70;
    fvco_result = 12500;  //12000;
    switch (phy->config->chip_ver) {
      case CHIP_VERSION_D1:
        sx_config_vco     = g_sx_config_vco_d1;
        sx_config_vco_len = sizeof(g_sx_config_vco_d1) / sizeof(g_sx_config_vco_d1[0]);
        break;
      case CHIP_VERSION_D2:
        sx_config_vco     = g_sx_config_vco_d2;
        sx_config_vco_len = sizeof(g_sx_config_vco_d2) / sizeof(g_sx_config_vco_d2[0]);
        break;
      case CHIP_VERSION_E1:
        sx_config_vco     = g_sx_config_vco_e1;
        sx_config_vco_len = sizeof(g_sx_config_vco_e1) / sizeof(g_sx_config_vco_e1[0]);
        break;
      default:
        sx_config_vco     = g_sx_config_vco_d1;
        sx_config_vco_len = sizeof(g_sx_config_vco_d1) / sizeof(g_sx_config_vco_d1[0]);
        LOG_WARN("%s in line %d the sx config vco list is default(d1)\n", __FUNCTION__, __LINE__);
        break;
    }
    store_index = get_right_index(comp_val, sx_config_vco, sx_config_vco_len);
    LOG_MDEBUG(phy, SX_TRX_CAL, "[%s][%d], get sx_config_vco, fvco:%lld, index:%d\n", __FUNCTION__, __LINE__, comp_val, store_index);
    return store_index;
  } else if (fsm_flag == 1) {
    cnt         = 511;
    fvco_result = 12500;  //fvco_cal_delta=20
    switch (phy->config->chip_ver) {
      case CHIP_VERSION_D1:
        sx_cal_vco     = g_sx_cal_vco_d1;
        sx_cal_vco_len = sizeof(g_sx_cal_vco_d1) / sizeof(g_sx_cal_vco_d1[0]);
        break;
      case CHIP_VERSION_D2:
        sx_cal_vco     = g_sx_cal_vco_d2;
        sx_cal_vco_len = sizeof(g_sx_cal_vco_d2) / sizeof(g_sx_cal_vco_d2[0]);
        break;
      case CHIP_VERSION_E1:
        sx_cal_vco     = g_sx_cal_vco_e1;
        sx_cal_vco_len = sizeof(g_sx_cal_vco_e1) / sizeof(g_sx_cal_vco_e1[0]);
        break;
      default:
        sx_cal_vco     = g_sx_cal_vco_d1;
        sx_cal_vco_len = sizeof(g_sx_cal_vco_d1) / sizeof(g_sx_cal_vco_d1[0]);
        LOG_WARN("%s in line %d the sx cal vco list is default(d1)\n", __FUNCTION__, __LINE__);
        break;
    }
    store_index = get_right_index(comp_val, sx_cal_vco, sx_cal_vco_len);
    LOG_MDEBUG(phy, SX_TRX_CAL, "[%s][%d], get sx_cal_vco, fvco:%lld, index:%d\n", __FUNCTION__, __LINE__, comp_val, store_index);
    return store_index;
  } else {
    cnt         = 31;
    fvco_result = 550;
  }

  for (i = 0; i <= cnt; i++) {
    tmp        = (long long)((long long)fvco_result * MHZ_TO_HZ);
    curr_delta = IntAbs((long long)comp_val - tmp);

    if (fsm_flag == 0)
      LOG_MDEBUG(phy, SX_TRX_CAL, "SX_Config_LUT: index=%d, fvco(MHz)=%d, delta=%d\n", i, fvco_result, fvco_delta);
    else if (fsm_flag == 1)
      LOG_MDEBUG(phy, SX_TRX_CAL, "SX_CAL_LUT: index=%d, fvco(MHz)=%d, delta=%d\n", i, fvco_result, fvco_cal_delta);
    else
      LOG_MDEBUG(phy, SX_TRX_CAL, "TXLO_CAL_LUT: index=%d, fvco(MHz)=%d, delta=%d\n", i, fvco_result, txlo_cal_delta);

    if (curr_delta == 0) {
      store_index = i;
      goto finished;
    } else {
      if (old_delta == 0) {
        old_delta = curr_delta;
      } else {
        if (curr_delta < old_delta) {
          old_delta   = curr_delta;
          store_index = i;
        }
      }
    }

    if (fsm_flag == 1) {
      old_result  = fvco_result;
      fvco_result = fvco_result - fvco_cal_delta;

      result_buff    = fvco_result * fvco_cal_delta;
      fvco_cal_delta = result_buff / old_result + get_round_case_short(result_buff, old_result);
    } else if (fsm_flag == 0) {
      old_result  = fvco_result;
      fvco_result = fvco_result - fvco_delta;

      result_buff = fvco_result * fvco_delta;
      fvco_delta  = result_buff / old_result + get_round_case_short(result_buff, old_result);
    } else {
      old_result  = fvco_result;
      fvco_result = fvco_result + txlo_cal_delta;

      result_buff    = fvco_result * txlo_cal_delta;
      txlo_cal_delta = result_buff / old_result + get_round_case_short(result_buff, old_result);
    }
  }

finished:
  return store_index;
}

short calc_lo_div(rf_chip_phy_t* phy, unsigned long long flo) {
  unsigned long long lo_config = 42500000UL;
  unsigned long long init_flo  = phy->config->fvco_min >> 7;
  short lo_div                 = 1;
  int i;

  //lo_config = 46875000UL;
  if (phy->config->chip_ver)
    lo_config = init_flo;
  else
    lo_config = 49375000UL;  //6320000000 >> 7

  for (i = 0; i < 7; i++) {
    if (flo <= lo_config) {
      lo_div = 8 - i;
      //LOG_INFO("---flo=%llu, lo_config=%llu, lo_div=%d\n", flo, lo_config, lo_div);
      break;
    }
    //LOG_INFO("flo=%llu, lo_config=%llu, lo_div=%d, i=%d\n", flo, lo_config, lo_div, i);
    lo_config = lo_config << 1;
  }

  return lo_div;
}

short calc_tx_fsm_lo_div(rf_chip_phy_t* phy, short* flag_div) {
  unsigned short lo_div_tx_fsm_config[10][2] =
      {
          {2150, 25},  //val * 10 = 2.5
          {1080, 50},  //val * 10 = 5
          {540, 100},  //val * 10 = 10

          {42, 256},
          {85, 128},
          {170, 64},
          {340, 32},
          {675, 16},
          {1350, 8},
          {2700, 4},
      };
  short i, lo_div = -1;
  unsigned long long compare_flo;
  unsigned long long init_flo = phy->config->fvco_min >> 7;

  *flag_div = -1;
  for (i = 0; i < 10; i++) {
    compare_flo = (unsigned long long)((unsigned long long)(lo_div_tx_fsm_config[i][0]) * MHZ_TO_HZ);
    if (i <= 2) {
      if ((phy->config->tx_flo > compare_flo) && (phy->config->core2_enable)) {
        lo_div    = lo_div_tx_fsm_config[i][1];
        *flag_div = 1;
        break;
      }
    } else {
      //compare_flo = (unsigned long long) (46875000UL << (i-3));
      if (phy->config->chip_ver)
        compare_flo = (unsigned long long)(init_flo << (i - 3));
      else
        compare_flo = (unsigned long long)(49375000UL << (i - 3));  //6320000000 >> 7

      if ((phy->config->tx_flo <= compare_flo) && (phy->config->core2_enable == 0)) {
        lo_div    = lo_div_tx_fsm_config[i][1];
        *flag_div = 0;
        break;
      }
    }
  }

  if (lo_div < 0) {
    lo_div    = 2;
    *flag_div = 0;
  }

  return lo_div;
}

short find_case_with_flo(unsigned long long flo) {
  unsigned long long min_range, max_range;
  short i, index = -1;

  for (i = 0; i <= 8; i++) {
    min_range = (unsigned long long)((unsigned long long)(g_lo_range_trx_band_fsm[i][0]) * MHZ_TO_HZ);
    max_range = (unsigned long long)((unsigned long long)(g_lo_range_trx_band_fsm[i][1]) * MHZ_TO_HZ);

    if ((flo > min_range) && (flo <= max_range)) {
      index = i;
      break;
    }
  }

  if (index == -1)
    index = 8;

  return index;
}

unsigned char get_reg_val_from_trx_band(char found_case, unsigned short reg) {
  short i, j;

  j = -1;
  for (i = 0; i < 54; i++)
    if (g_trx_band_regs[i] == reg)
      j = i;
  if (j < 0)
    j = 0;
  return g_trx_band_config[found_case][j];
}

short set_mmd(rf_chip_phy_t* phy, TRX_ENUM trx, unsigned long long fvco, short* zero_flag) {
  unsigned short sx_dsm_n_int_reg1, sx_dsm_n_int_reg2;
  unsigned short sx_dsm_n_frac_reg1, sx_dsm_n_frac_reg2, sx_dsm_n_frac_reg3;
  unsigned long long div, mmd_mod, mmd_div;
  unsigned long long u64tmp, multi;  //frac;
  char shift;
  unsigned short val[3];

  /* regs */
  if (trx == RX_DIR) {
    sx_dsm_n_int_reg1  = 0x626;
    sx_dsm_n_int_reg2  = 0x627;
    sx_dsm_n_frac_reg1 = 0x623;
    sx_dsm_n_frac_reg2 = 0x624;
    sx_dsm_n_frac_reg3 = 0x625;
  } else {
    sx_dsm_n_int_reg1  = 0x63F;
    sx_dsm_n_int_reg2  = 0x640;
    sx_dsm_n_frac_reg1 = 0x63C;
    sx_dsm_n_frac_reg2 = 0x63D;
    sx_dsm_n_frac_reg3 = 0x63E;
  }

  //LOG_INFO("fvco=%llu\n", fvco);

  /* MMD INT */
  shift = phy->config->x4_enable ? 3 : 2;
  div   = phy->config->xtal_freq << shift;

  //LOG_INFO("fvco=%llu, div=%llu\r\n", fvco, div);
  mmd_mod = do_div(&fvco, div);
  mmd_div = fvco;
  //LOG_INFO("mmd_mod=%llu, mmd_div=%llu\r\n", mmd_mod, mmd_div);

  hal_spi_write_reg(phy, sx_dsm_n_int_reg1, (mmd_div >> 8) & 0xff);
  hal_spi_write_reg(phy, sx_dsm_n_int_reg2, mmd_div & 0xff);

  /* MMD FRAC */
  u64tmp = (mmd_mod << 23);
  multi  = u64tmp;
  do_div(&u64tmp, div);

  //frac = u64tmp;
  u64tmp = u64tmp + get_round_case(multi, div);

  val[0] = (u64tmp >> 16) & 0xff;
  val[1] = (u64tmp >> 8) & 0xff;
  val[2] = u64tmp & 0xff;
  hal_spi_write_reg(phy, sx_dsm_n_frac_reg1, val[0]);
  hal_spi_write_reg(phy, sx_dsm_n_frac_reg2, val[1]);
  hal_spi_write_reg(phy, sx_dsm_n_frac_reg3, val[2]);

  if ((val[0] == 0) && (val[1] == 0) && (val[2] == 0))
    *zero_flag = 1;
  else
    *zero_flag = 0;

  return (mmd_mod ? 1 : 0);
}

unsigned char config_sxtrx(rf_chip_phy_t* phy, TRX_ENUM trx, unsigned long long fvco, short* outi, char fsm_flag, char wflag) {
  unsigned char reg_val, flag = 0x0;
  short index, i;
  unsigned char lut_val = 0, offset = 0;
  int lut_addr            = 0;
  unsigned long long fref = 0;

  index = fvco_get_right_index(phy, fvco, fsm_flag);
  *outi = index;
  //LOG_WARN("fvco=%llu, index=%d\n", fvco, *outi);
  if (phy->config->x4_enable) {
    fref = phy->config->xtal_freq * 4;
  } else {
    fref = phy->config->xtal_freq * 2;
  }

  for (i = 0; i < 10; i++) {
    if (fref >= 90000000UL) {
      //reg_val = g_sx_config_x4_enable[index][i];
      lut_addr = (index + 71) * 3 + i / 4;
      offset   = i % 4;
      read_lut_byte(phy, SX_CONFIG_LUT, index + 71, 0, lut_addr, offset, &lut_val);
      reg_val = lut_val;
    } else {
      //reg_val = g_sx_config_x4_disable[index][i];
      lut_addr = index * 3 + i / 4;
      offset   = i % 4;
      read_lut_byte(phy, SX_CONFIG_LUT, index, 0, lut_addr, offset, &lut_val);
      reg_val = lut_val;
    }

    if (trx == RX_DIR) {
      if (g_sxtrx_regs[0][i] == 0x62A)
        flag = reg_val;
      if (wflag)
        hal_spi_write_reg(phy, g_sxtrx_regs[0][i], reg_val);
    } else {
      if (g_sxtrx_regs[1][i] == 0x643)
        flag = reg_val;
      if (wflag)
        hal_spi_write_reg(phy, g_sxtrx_regs[1][i], reg_val);
    }
  }

  return flag;
}

void set_target_cnt(rf_chip_phy_t* phy, TRX_ENUM trx, unsigned long long flo, unsigned char flag, int factor, short scale) {
  unsigned short sx_fcal_tcnt_reg1, sx_fcal_tcnt_reg2;
  unsigned long long u64tmp;
  char fcal[3] = {0, 2, 1};
  unsigned int reg_val;
  char fcal_div;

  /* regs */
  if (trx == RX_DIR) {
    sx_fcal_tcnt_reg1 = 0x010;
    sx_fcal_tcnt_reg2 = 0x011;
  } else {
    sx_fcal_tcnt_reg1 = 0x01B;
    sx_fcal_tcnt_reg2 = 0x01C;
  }

  reg_val = (flag >> 5) & 0x7;
  if ((reg_val >= 0) && (reg_val <= 2))
    fcal_div = fcal[reg_val];
  else
    fcal_div = fcal[0];

  reg_val = factor >> fcal_div;
  u64tmp  = (reg_val * flo);
  do_div(&u64tmp, phy->config->xtal_freq * scale);

  reg_val = u64tmp;
  hal_spi_write_reg(phy, sx_fcal_tcnt_reg1, (reg_val >> 8) & 0xff);
  hal_spi_write_reg(phy, sx_fcal_tcnt_reg2, reg_val & 0xff);
}

unsigned char set_fsm_tx_target_cnt(rf_chip_phy_t* phy, unsigned long long flo) {
  unsigned long long mmd_div, u64tmp;
  unsigned char ret;
  char fcal_div = 4;
  short i, st1, st2;

  i = my_log2(15, phy->config->dll_twin_fxo);
  if ((flo > 1500000000ULL) && (fcal_div == 2)) {
    ret = 0xE8;
    st1 = i + 3;
    st2 = 22;
  } else if ((flo > 1500000000ULL) && (fcal_div == 4)) {
    ret = 0xC8;
    st1 = i + 3;
    st2 = 44;
  } else if ((flo <= 1500000000ULL) && (fcal_div == 2)) {
    ret = 0xF0;
    st1 = i + 4;
    st2 = 22;
  } else {
    ret = 0xD0;
    st1 = i + 4;
    st2 = 44;
  }

  u64tmp = (flo << st1);
  do_div(&u64tmp, (st2 * phy->config->xtal_freq));
  mmd_div = u64tmp;

  hal_spi_write_reg(phy, 0x030, (mmd_div >> 8) & 0xff);
  hal_spi_write_reg(phy, 0x031, mmd_div & 0xff);

  return ret;
}

char timing_map_to_val(char timing) {
  if (timing == 1)
    return 0;
  else if (timing == 10)
    return 1;
  else if (timing == 100)
    return 2;
  else if (timing > 100)
    return 3;
  else
    return 0;
}

unsigned short lo_div_map_val(short val, short lo_div) {
  short mod, div;
  div = lo_div / 25;
  mod = lo_div % 25;
  return mod ? val : (0x20 * div);
}

int sxtrx_fcal_sw_cal_v2(rf_chip_phy_t* phy, TRX_ENUM trx) {
  unsigned short da_sxtrx_vco_scap_reg_h = 0;
  unsigned short da_sxtrx_vco_scap_reg_l = 0;
  int min                                = 0;
  int max                                = 0;
  int i                                  = 0;
  int scap                               = 0;
  int Vctrl                              = 0;
  int scap_tmp                           = 0;
  int scap_low                           = 0;
  int scap_up                            = 0;
  int final_val                          = 0;

  if (trx == RX_DIR) {
    da_sxtrx_vco_scap_reg_h = 0x62e;
    da_sxtrx_vco_scap_reg_l = 0x62f;
  } else {
    da_sxtrx_vco_scap_reg_h = 0x647;
    da_sxtrx_vco_scap_reg_l = 0x648;
  }

  //init
  min = phy->config->scap_min;
  max = phy->config->scap_max;
  i   = phy->config->scap_cnt;

  //coarse tune
  while (i > 0) {
    scap = (max + min + 1) >> 1;
    hal_spi_write_reg(phy, da_sxtrx_vco_scap_reg_h, (scap >> 8) & 0xff);
    hal_spi_write_reg(phy, da_sxtrx_vco_scap_reg_l, scap & 0xff);

    (void)sxtrx_lock_status(phy, trx, &Vctrl);

    LOG_MDEBUG(phy, SX_TRX_CAL, "low_limit=%dmV, up_limit=%dmV, Voltage=%dmV, reg%03x=0x%02x, reg%03x=0x%02x, reg%03x=0x%02x, count=%d, scap=%d\n",
               phy->config->vol_low_limit, phy->config->vol_up_limit, Vctrl,
               trx ? 0x64d : 0x634, hal_spi_read_reg(phy, trx ? 0x64d : 0x634),
               da_sxtrx_vco_scap_reg_h, hal_spi_read_reg(phy, da_sxtrx_vco_scap_reg_h),
               da_sxtrx_vco_scap_reg_l, hal_spi_read_reg(phy, da_sxtrx_vco_scap_reg_l),
               i, scap);

    if (Vctrl <= phy->config->vol_low_limit) {
      min = scap;
    } else if (Vctrl >= phy->config->vol_up_limit) {
      max = scap;
    } else {
      scap_tmp = scap;
      break;
    }

    i--;
  }

  scap     = scap_tmp;
  i        = 0;
  scap_low = phy->config->scap_min;
  while (scap >= phy->config->scap_min) {
    i++;
    scap--;
    hal_spi_write_reg(phy, da_sxtrx_vco_scap_reg_h, (scap >> 8) & 0xff);
    hal_spi_write_reg(phy, da_sxtrx_vco_scap_reg_l, scap & 0xff);

    (void)sxtrx_lock_status(phy, trx, &Vctrl);
    LOG_MDEBUG(phy, SX_TRX_CAL, "----find scap low, low_limit=%dmV, up_limit=%dmV, Voltage=%dmV, reg%03x=0x%02x, reg%03x=0x%02x, reg%03x=0x%02x, count=%d, scap=%d\n",
               phy->config->vol_low_limit, phy->config->vol_up_limit, Vctrl,
               trx ? 0x64d : 0x634, hal_spi_read_reg(phy, trx ? 0x64d : 0x634),
               da_sxtrx_vco_scap_reg_h, hal_spi_read_reg(phy, da_sxtrx_vco_scap_reg_h),
               da_sxtrx_vco_scap_reg_l, hal_spi_read_reg(phy, da_sxtrx_vco_scap_reg_l),
               i, scap);

    if (Vctrl <= phy->config->vol_low_limit) {
      scap_low = scap;
      break;
    }
  }

  scap    = scap_tmp;
  i       = 0;
  scap_up = phy->config->scap_max;
  while (scap <= phy->config->scap_max) {
    i++;
    scap++;
    hal_spi_write_reg(phy, da_sxtrx_vco_scap_reg_h, (scap >> 8) & 0xff);
    hal_spi_write_reg(phy, da_sxtrx_vco_scap_reg_l, scap & 0xff);

    (void)sxtrx_lock_status(phy, trx, &Vctrl);
    LOG_MDEBUG(phy, SX_TRX_CAL, "----find scap up, low_limit=%dmV, up_limit=%dmV, Voltage=%dmV, reg%03x=0x%02x, reg%03x=0x%02x, reg%03x=0x%02x, count=%d, scap=%d\n",
               phy->config->vol_low_limit, phy->config->vol_up_limit, Vctrl,
               trx ? 0x64d : 0x634, hal_spi_read_reg(phy, trx ? 0x64d : 0x634),
               da_sxtrx_vco_scap_reg_h, hal_spi_read_reg(phy, da_sxtrx_vco_scap_reg_h),
               da_sxtrx_vco_scap_reg_l, hal_spi_read_reg(phy, da_sxtrx_vco_scap_reg_l),
               i, scap);

    if (Vctrl >= phy->config->vol_up_limit) {
      scap_up = scap;
      break;
    }
  }

  final_val = (scap_low + scap_up + 1) >> 1;

  hal_spi_write_reg(phy, da_sxtrx_vco_scap_reg_h, (final_val >> 8) & 0xff);
  hal_spi_write_reg(phy, da_sxtrx_vco_scap_reg_l, final_val & 0xff);
  LOG_MDEBUG(phy, SX_TRX_CAL, "----low scap: %d, up scap: %d, final scap: %d, reg_0x%x=0x%x, reg_0x%x=0x%x\n",
             scap_low, scap_up, final_val,
             da_sxtrx_vco_scap_reg_h, hal_spi_read_reg(phy, da_sxtrx_vco_scap_reg_h),
             da_sxtrx_vco_scap_reg_l, hal_spi_read_reg(phy, da_sxtrx_vco_scap_reg_l));

  return 0;
}

//freq range: 30MHz ~ 6GHz
int set_trx_lo(rf_chip_phy_t* phy, TRX_ENUM trx, TRX_CHN_ENUM chn, unsigned long long flo) {
  int twin_fxo_div, tsu_fxo_div, voltage;
  unsigned long long fvco;
  unsigned short sx_to_lo_reg;
  unsigned short sx_ctrl_reg, sx_ctrl_reg_ext, sx_fcal_ctrl_reg;
  unsigned short sx_enable_reg, sx_vco_scap_h, sx_vco_scap_l;
  unsigned short sx_ctrl_reg_18;
  short twin_fxo, tsu_fxo, i;
  unsigned char lo_div, flag, reg_val, tmp, sx_vco_scap_h_val, sx_vco_scap_l_val;
  short have_frac, zero_flag = 0, lock_status, index, short_val;
  unsigned short u16tmp;
  unsigned short da_sxtrx_vco_scap_reg_h = 0;
  unsigned short da_sxtrx_vco_scap_reg_l = 0;
  unsigned char sx_cal_index_h, sx_cal_index_l;

  /* regs */
  if (trx == RX_DIR) {
    //phy->config->rx_flo = flo;
    twin_fxo                = phy->config->rx_twin_fxo;
    tsu_fxo                 = phy->config->rx_tsu_fxo;
    sx_to_lo_reg            = 0x620;
    sx_ctrl_reg             = 0x634;
    sx_ctrl_reg_18          = 0x630;
    sx_ctrl_reg_ext         = 0x61E;
    sx_fcal_ctrl_reg        = 0x00E;
    sx_enable_reg           = 0x61F;
    sx_vco_scap_h           = 0x017;
    sx_vco_scap_l           = 0x018;
    da_sxtrx_vco_scap_reg_h = 0x62e;
    da_sxtrx_vco_scap_reg_l = 0x62f;
  } else {
    //phy->config->tx_flo = flo;
    twin_fxo                = phy->config->tx_twin_fxo;
    tsu_fxo                 = phy->config->tx_tsu_fxo;
    sx_to_lo_reg            = 0x639;
    sx_ctrl_reg             = 0x64D;
    sx_ctrl_reg_18          = 0x649;
    sx_ctrl_reg_ext         = 0x637;
    sx_fcal_ctrl_reg        = 0x019;
    sx_enable_reg           = 0x638;
    sx_vco_scap_h           = 0x022;
    sx_vco_scap_l           = 0x023;
    da_sxtrx_vco_scap_reg_h = 0x647;
    da_sxtrx_vco_scap_reg_l = 0x648;
  }

  LOG_MDEBUG(phy, SX_TRX_CAL, "--------------------------------------set_trx_lo---------------------------------------------------\n");
  LOG_MDEBUG(phy, SX_TRX_CAL, "flo:%lldhz\n", flo);

  /* calc lo div */
  lo_div = calc_lo_div(phy, flo);

  /* calc fvco */
  fvco = flo << lo_div;

  /* set mmd int & frac */
  have_frac = set_mmd(phy, trx, fvco, &zero_flag);

  /* sxtrx register */
  flag = config_sxtrx(phy, trx, fvco, &i, 0, 1);

  /* X4 config */
  //refclk_config(phy->config->x4_enable);

  /* TARGET CNT */
  set_target_cnt(phy, trx, flo, flag, twin_fxo << lo_div, 1);

  /* search C2&C3 or C4&C5 */
  u16tmp = fvco_get_right_index(phy, fvco, 1);
  u16tmp = u16tmp & 0x1ff;
  if (phy->config->lo_change_mode == LO_CHANGE_FAST_LOCK_ATTACH_TRX_LO) {
    if (trx == RX_DIR) {
      sx_cal_index_h = hal_spi_read_reg(phy, 0x0C4);
      sx_cal_index_l = hal_spi_read_reg(phy, 0x0C5);

    } else {
      sx_cal_index_h = hal_spi_read_reg(phy, 0x0C2);
      sx_cal_index_l = hal_spi_read_reg(phy, 0x0C3);
    }

    if (u16tmp == (((sx_cal_index_h & 0x01) << 8) | sx_cal_index_l)) {
      u16tmp = u16tmp + 1;
      u16tmp = u16tmp & 0x1ff;
    }
  }
  reg_val = (u16tmp >> 8) & 0x1;
  if (trx == RX_DIR) {
    hal_spi_write_reg(phy, 0x0C2, reg_val);
    hal_spi_write_reg(phy, 0x0C3, (u16tmp)&0xff);
  } else {
    hal_spi_write_reg(phy, 0x0C4, reg_val);
    hal_spi_write_reg(phy, 0x0C5, (u16tmp)&0xff);
  }

  if (CHIP_VERSION_E1 == phy->config->chip_ver) {
    //manual sx vldo en
    HAL_REG_SET_BIT(phy, 0x614, BIT4);
  }

  /* SX manual enable, FCAL manual disable */
  hal_spi_write_reg(phy, sx_ctrl_reg, 0x04);

  /* SX VCO fast charge enable */
  HAL_REG_SET_BIT(phy, sx_to_lo_reg, BIT1);

  /* SX FCAL enable (HW FCAL, unused) */
  hal_spi_write_reg(phy, sx_ctrl_reg_ext, 0x12);

  /* Disable HW FCAL */
  twin_fxo_div = twin_fxo >> 4;
  i            = my_log2(8, twin_fxo_div);
  i            = (i >= 3) ? 3 : i;
  reg_val      = (0xD << 4) | ((i << 2) & 0xC);

  tsu_fxo_div = tsu_fxo >> 3;
  i           = my_log2(8, tsu_fxo_div);
  i           = (i >= 3) ? 3 : i;
  reg_val     = reg_val | (i & 0x3);

  flag    = reg_val;
  reg_val = reg_val & ((~(1 << 4)) & 0xff);
  tmp     = reg_val;
  hal_spi_write_reg(phy, sx_fcal_ctrl_reg, reg_val);

  /* SX  blocks enable */
  reg_val = have_frac ? 0x1f : 0x0f;
  hal_spi_write_reg(phy, sx_enable_reg, reg_val);
  CHIP_DELAY(10);
  /* SX to LO enable */
  //spi_write_reg(sx_to_lo_reg, sx_to_lo_val);

  /* HW FCAL enable */
  hal_spi_write_reg(phy, sx_fcal_ctrl_reg, flag);
  CHIP_DELAY(10);
  /* DSM RSTN=0 (reset) */
  hal_spi_write_reg(phy, sx_ctrl_reg_ext, 0x16);

  /* DSM RSTN=1 */
  hal_spi_write_reg(phy, sx_ctrl_reg_ext, 0x12);

  /* Enable SX PFD and CP */
  reg_val = have_frac ? 0x9f : 0x8f;
  hal_spi_write_reg(phy, sx_enable_reg, reg_val);

  /* Disable HW FCAL */
  hal_spi_write_reg(phy, sx_fcal_ctrl_reg, tmp);

  /* Disable SX FCAL, close loop */
  hal_spi_write_reg(phy, sx_ctrl_reg_ext, 0x02);

  /* SX VCO fast charge disable */
  HAL_REG_CLR_BIT(phy, sx_to_lo_reg, BIT1);

  /* LO DIV */
  //hal_spi_write_reg(phy, sx_lo_div_reg, lo_div-1);
  if (trx == TX_DIR) {
    hal_spi_write_reg(phy, 0x72A, lo_div - 1);
    hal_spi_write_reg(phy, 0x772, lo_div - 1);
  } else {
    hal_spi_write_reg(phy, 0x665, lo_div - 1);
    hal_spi_write_reg(phy, 0x683, lo_div - 1);
  }

  /* check lock status */
  lock_status = sxtrx_lock_status(phy, trx, &voltage);

  sx_vco_scap_h_val = hal_spi_read_reg(phy, sx_vco_scap_h);
  sx_vco_scap_l_val = hal_spi_read_reg(phy, sx_vco_scap_l);

  LOG_MDEBUG(phy, SX_TRX_CAL, "hw cal:%s, sx_vco_scap_h(0x%03x)=0x%x, sx_vco_scap_l(0x%03x)=0x%x\n",
             (0 == lock_status) ? "locked" : "unlocked", sx_vco_scap_h, sx_vco_scap_h_val, sx_vco_scap_l, sx_vco_scap_l_val);

  if ((TRX_LO_HW_CAL == phy->config->trx_lo_cal_mode) || ((TRX_LO_HYBRID_CAL == phy->config->trx_lo_cal_mode) && (0 == lock_status))) {
    /* write scap to SX_CAL_LUT of C2&C3 or C4&C5 index */
    index = u16tmp & 0x1ff;
    write_lut_byte(phy, SX_CAL_LUT, index, 0x647, index / 2, ((index + 1) % 2) ? 0 : 2, sx_vco_scap_h_val);
    write_lut_byte(phy, SX_CAL_LUT, index, 0x648, index / 2, ((index + 1) % 2) ? 1 : 3, sx_vco_scap_l_val);

    /* for ATE test */
    hal_spi_write_reg(phy, da_sxtrx_vco_scap_reg_h, sx_vco_scap_h_val);
    hal_spi_write_reg(phy, da_sxtrx_vco_scap_reg_l, sx_vco_scap_l_val);
    LOG_MDEBUG(phy, SX_TRX_CAL, "set_trx_lo, check lock status: %s(hw cal), voltage:%d!, index:0x%x\n", (0 == lock_status) ? "locked" : "unlocked", voltage, index);
  }

  if ((TRX_LO_SW_CAL == phy->config->trx_lo_cal_mode) || ((TRX_LO_HYBRID_CAL == phy->config->trx_lo_cal_mode) && (0 != lock_status))) {
    hal_spi_write_reg(phy, 0x901, phy->config->reg901_val);
    hal_spi_write_reg(phy, 0x61A, phy->config->reg61A_val);
    hal_spi_write_reg(phy, 0x600, phy->config->reg600_val);
    hal_spi_write_reg(phy, 0x602, phy->config->reg602_val);

    if (1 == (phy->config->reg639_620_bit1 & 0x1)) {
      /* 0x639 or 0x620 reg bit1 set to 1 */
      HAL_REG_SET_BIT(phy, sx_to_lo_reg, BIT1);
    } else {
      /* 0x639 or 0x620 reg bit1 set to 0 */
      HAL_REG_CLR_BIT(phy, sx_to_lo_reg, BIT1);
    }

    /* change main lut of 0x639 or 0x620 from s14 to flush*/
    if (trx == RX_DIR) {
      if (1 == (phy->config->reg639_620_bit1 & 0x1))
        short_val = phy->config->sxtrx_perf_setting[phy->config->mode][1];
      else
        short_val = phy->config->sxtrx_perf_setting[phy->config->mode][3];

      if (short_val >= 0) {
        for (i = 14; i < 27; i++)
          write_lut_byte(phy, MAIN_ENSM_LUT, i, sx_to_lo_reg, 2 + 10 * i, 0, short_val);
      }
    } else {
      if (1 == (phy->config->reg639_620_bit1 & 0x1))
        short_val = phy->config->sxtrx_perf_setting[phy->config->mode][0];
      else
        short_val = phy->config->sxtrx_perf_setting[phy->config->mode][2];

      for (i = 14; i < 27; i++)
        write_lut_byte(phy, MAIN_ENSM_LUT, i, sx_to_lo_reg, 2 + 10 * i, 3, short_val);
    }

    /* change main lut of 0x602 to 0x02 from S1 to flush */
    for (i = 0; i < 27; i++)
      write_lut_byte(phy, MAIN_ENSM_LUT, i, 0x602, 1 + 10 * i, 1, phy->config->reg602_val);

    hal_spi_write_reg(phy, sx_ctrl_reg, 0x0c);
    HAL_REG_CLR_BIT(phy, sx_ctrl_reg_18, BIT7);

    /* sw cal */
    sxtrx_fcal_sw_cal_v2(phy, trx);

    HAL_REG_SET_BIT(phy, sx_ctrl_reg_18, BIT7);

    /* set SX_CAL_LUT */
    sx_vco_scap_h_val = hal_spi_read_reg(phy, da_sxtrx_vco_scap_reg_h);
    sx_vco_scap_l_val = hal_spi_read_reg(phy, da_sxtrx_vco_scap_reg_l);

    /* write scap to SX_CAL_LUT of C2&C3 or C4&C5 index */
    index = u16tmp & 0x1ff;
    write_lut_byte(phy, SX_CAL_LUT, index, 0x647, index / 2, ((index + 1) % 2) ? 0 : 2, sx_vco_scap_h_val);
    write_lut_byte(phy, SX_CAL_LUT, index, 0x648, index / 2, ((index + 1) % 2) ? 1 : 3, sx_vco_scap_l_val);
    LOG_MDEBUG(phy, SX_TRX_CAL, "sw cal, write scap, SX_CAL_LUT index=0x%x, sx_vco_scap_h(0x%03x)=0x%x, sx_vco_scap_l(0x%03x)=0x%x\n",
               index, da_sxtrx_vco_scap_reg_h, sx_vco_scap_h_val, da_sxtrx_vco_scap_reg_l, sx_vco_scap_l_val);
  }
  LOG_MDEBUG(phy, SX_TRX_CAL, "--------------------------------------set_trx_lo end---------------------------------------------------\n");

  voltage = 0;
  sxtrx_lock_status(phy, trx, &voltage);
  if (voltage < phy->config->vol_low_limit || voltage > phy->config->vol_up_limit)
    LOG_ERROR("Dir=%d lock failed ! flo=%llu voltage=%d\n", trx, flo, voltage);
  voltage = 0;
  voltage = get_syspll_status(phy);
  uint32_t uint_flo;
  uint_flo = flo / 1000;
  // printf("voltage = %d\n\r",voltage);
  // printf("flo=%d\n\r",flo);
  if (voltage < 300 || voltage > 900) {
    printf("flo=%d, voltage = %d\n", uint_flo, voltage);
    LOG_ERROR("syspll lock failed ! flo=%llu voltage=%d\n", flo, voltage);
  }

  return 0;
}

int sxrx_band_fsm(rf_chip_phy_t* phy, unsigned long long flo) {
  unsigned long long fvco, fref;
  unsigned char regs_val[2], i, flag;
  short lo_div, index;
  int twin_fxo_div, tsu_fxo_div;
  unsigned short u16tmp, reg_val;
  short zero_flag = 0;
  unsigned char mode_flag, mode_setting[2][10] = {
                               {0x06, 0x2, 0x6, 0x2, 0x2, 0x9F, 0x9F, 0x9F, 0x9F, 0x9F},      //for fast & tracking
                               {0x02, 0x12, 0x16, 0x12, 0x12, 0x1F, 0x1F, 0x1F, 0x1F, 0x9F},  //for old & normal
                           };
  int regCF;
  unsigned char sx_cal_index_h, sx_cal_index_l;

  phy->config->rx_flo = flo;

  if ((phy->config->lo_change_mode == LO_CHANGE_FAST_LOCK_ATTACH_TRX_LO) || (phy->config->lo_change_mode == LO_CHANGE_OLD)) {
    hal_spi_write_reg(phy, 0x019, 0xce);
    hal_spi_write_reg(phy, 0x00E, 0xce);
  }

  if (phy->config->lo_change_mode == LO_CHANGE_FAST_LOCK_ATTACH_TRX_LO) {
    set_trx_lo(phy, RX_DIR, TRX_CHN1, phy->config->rx_flo);
  }

  switch (phy->config->lo_change_mode) {
    case LO_CHANGE_OLD:
    case LO_CHANGE_NORMAL:
      mode_flag = 1;
      break;
    case LO_CHANGE_FAST_LOCK:
    case LO_CHANGE_TRACKING:
    case LO_CHANGE_FAST_LOCK_ATTACH_TRX_LO:
      mode_flag = 0;
      break;
  }

  regs_val[0] = hal_spi_read_reg(phy, 0x0C1);
  regs_val[1] = hal_spi_read_reg(phy, 0x0D1);

  /* calc lo div */
  lo_div = calc_lo_div(phy, flo);

  /* calc fvco */
  fvco = flo << lo_div;

  /* set mmd int & frac */
  set_mmd(phy, RX_DIR, fvco, &zero_flag);
  if (phy->config->lo_change_mode == LO_CHANGE_TRACKING) {
    hal_spi_write_reg(phy, 0x00E, 0x8e);
    regCF = hal_spi_read_reg(phy, 0x0CF);
    regCF = (regCF & 0x80) | 0x47;
    hal_spi_write_reg(phy, 0x0CF, regCF);
    hal_spi_write_reg(phy, 0x0d1, 0x00);
    hal_spi_write_reg(phy, 0x0d2, 0x00);
    for (i = 0; i < 10; i++) {
      if (i < 5)
        write_lut_byte(phy, MAIN_ENSM_LUT, i + 1, 0x61e, 1 + (i + 1) * 10, 2, mode_setting[mode_flag][i]);
      else
        write_lut_byte(phy, MAIN_ENSM_LUT, i - 4, 0x61f, 1 + (i - 4) * 10, 3, mode_setting[mode_flag][i]);
    }

    return 0;
  }

  /* sxtrx register */
  flag = config_sxtrx(phy, RX_DIR, fvco, &index, 0, 0);

  /* TARGET CNT */
  if (phy->config->lo_change_mode != LO_CHANGE_FAST_LOCK && phy->config->lo_change_mode != LO_CHANGE_FAST_LOCK_ATTACH_TRX_LO)
    set_target_cnt(phy, RX_DIR, flo, flag, phy->config->rx_twin_fxo << lo_div, 1);

  if (phy->config->lo_change_mode == LO_CHANGE_FAST_LOCK || phy->config->lo_change_mode == LO_CHANGE_FAST_LOCK_ATTACH_TRX_LO) {
    hal_spi_write_reg(phy, 0x019, 0x8e);
    hal_spi_write_reg(phy, 0x00E, 0x8e);

    regCF = hal_spi_read_reg(phy, 0x0CF);
    regCF = (regCF & 0x80) | 0x4d;
    hal_spi_write_reg(phy, 0x0CF, regCF);
    hal_spi_write_reg(phy, 0x0D1, 0x00);
    hal_spi_write_reg(phy, 0x0D2, 0x00);
  } else {
    /* bypass some status */
    regCF = hal_spi_read_reg(phy, 0x0CF);
    regCF = regCF & 0x80;
    hal_spi_write_reg(phy, 0x0CF, regCF);
    hal_spi_write_reg(phy, 0x0D1, 0xa5);
    hal_spi_write_reg(phy, 0x0D2, 0x54);
    regs_val[1] = hal_spi_read_reg(phy, 0x0D1);

    /* Disable HW FCAL */
    twin_fxo_div = phy->config->rx_twin_fxo >> 4;
    i            = my_log2(8, twin_fxo_div);
    i            = (i >= 3) ? 3 : i;
    reg_val      = (0xC << 4) | ((i << 2) & 0xC);

    tsu_fxo_div = phy->config->rx_tsu_fxo >> 3;
    i           = my_log2(8, tsu_fxo_div);
    i           = (i >= 3) ? 3 : i;
    reg_val     = reg_val | (i & 0x3);

    flag    = reg_val;
    reg_val = reg_val & ((~(1 << 4)) & 0xff);
    hal_spi_write_reg(phy, 0x00E, flag);

    /* state cnt selection */
    reg_val = timing_map_to_val(phy->config->rx_tstate2_fsys_us);
    reg_val = (reg_val << 4) | (regs_val[1] & 0xf) | (regs_val[1] & 0xC0);
    hal_spi_write_reg(phy, 0x0D1, reg_val);
  }

  /* update lut */

  /* TRX BAND LUT */
  i = lo_div;  //my_log2(15, lo_div);
  i = (i >= 12) ? 3 : i;
  i = (i >= 1) ? (i - 1) : 0;

  write_lut_byte(phy, TRX_BAND_LUT, 0, 0x665, 12, 0, i);
  write_lut_byte(phy, TRX_BAND_LUT, 1, 0x665, 26, 0, i);
  write_lut_byte(phy, TRX_BAND_LUT, 0, 0x683, 12, 3, i);
  write_lut_byte(phy, TRX_BAND_LUT, 1, 0x683, 26, 3, i);

  /* MAIN_ENSM_LUT */
  write_lut_byte(phy, MAIN_ENSM_LUT, 0, 0x665 + LUT_DIST_DELTA, 272, 0, 0x01);
  write_lut_byte(phy, MAIN_ENSM_LUT, 1, 0x665 + LUT_DIST_DELTA, 274, 0, 0x01);
  write_lut_byte(phy, MAIN_ENSM_LUT, 0, 0x683 + LUT_DIST_DELTA, 272, 1, 0x01);
  write_lut_byte(phy, MAIN_ENSM_LUT, 1, 0x683 + LUT_DIST_DELTA, 274, 1, 0x01);

  for (i = 0; i < 10; i++) {
    if (i < 5)
      write_lut_byte(phy, MAIN_ENSM_LUT, i + 1, 0x61e, 1 + (i + 1) * 10, 2, mode_setting[mode_flag][i]);
    else
      write_lut_byte(phy, MAIN_ENSM_LUT, i - 4, 0x61f, 1 + (i - 4) * 10, 3, mode_setting[mode_flag][i]);
  }

  /* SX_FCAL INDEX */
  flag = (fvco >= 6000000000ULL) ? 0 : 1;
  if (phy->config->x4_enable) {
    fref = phy->config->xtal_freq * 4;
  } else {
    fref = phy->config->xtal_freq * 2;
  }
  i = (fref >= 90000000UL) ? 1 : 0;

  reg_val = (i << 7) | (flag << 6) | (regs_val[0] & 0x3f);
  hal_spi_write_reg(phy, 0x0C1, reg_val);

  /* SXRX_Config_INDEX */
  hal_spi_write_reg(phy, 0x0C7, index);

  u16tmp = fvco_get_right_index(phy, fvco, 1);
  u16tmp = u16tmp & 0x1ff;
  if (phy->config->lo_change_mode == LO_CHANGE_FAST_LOCK_ATTACH_TRX_LO) {
    sx_cal_index_h = hal_spi_read_reg(phy, 0x0C4);
    sx_cal_index_l = hal_spi_read_reg(phy, 0x0C5);
    if (u16tmp == (((sx_cal_index_h & 0x01) << 8) | sx_cal_index_l)) {
      u16tmp = u16tmp + 1;
      u16tmp = u16tmp & 0x1ff;
    }
  }
  reg_val = (u16tmp >> 8) & 0x1;
  hal_spi_write_reg(phy, 0x0C2, reg_val);
  hal_spi_write_reg(phy, 0x0C3, (u16tmp)&0xff);

  /* trx band tx part update */
  index = find_case_with_flo(phy->config->rx_flo);

  reg_val = get_reg_val_from_trx_band(index, 0x66A);
  write_lut_byte(phy, TRX_BAND_LUT, 0, 0x66A, 12, 1, reg_val);
  write_lut_byte(phy, TRX_BAND_LUT, 1, 0x66A, 26, 1, reg_val);

  reg_val = get_reg_val_from_trx_band(index, 0x6D7);
  write_lut_byte(phy, TRX_BAND_LUT, 0, 0x6D7, 12, 2, reg_val);
  write_lut_byte(phy, TRX_BAND_LUT, 1, 0x6D7, 26, 2, reg_val);

  reg_val = get_reg_val_from_trx_band(index, 0x688);
  write_lut_byte(phy, TRX_BAND_LUT, 0, 0x688, 13, 0, reg_val);
  write_lut_byte(phy, TRX_BAND_LUT, 1, 0x688, 27, 0, reg_val);

  reg_val = get_reg_val_from_trx_band(index, 0x6E7);
  write_lut_byte(phy, TRX_BAND_LUT, 0, 0x6E7, 13, 1, reg_val);
  write_lut_byte(phy, TRX_BAND_LUT, 1, 0x6E7, 27, 1, reg_val);

  /* lock */
  if (phy->config->lo_change_mode == LO_CHANGE_OLD)
    set_trx_lo(phy, RX_DIR, TRX_CHN1, phy->config->rx_flo);

  reg_val = zero_flag ? 0x8F : 0x9F;
  hal_spi_write_reg(phy, 0x61F, reg_val);
  for (i = 0; i < 22; i++)
    write_lut_byte(phy, MAIN_ENSM_LUT, 5 + i, 0x61F, 51 + 10 * i, 3, reg_val);

  return 0;
}

int sxtx_band_fsm(rf_chip_phy_t* phy, unsigned long long flo, short core2_en) {
  unsigned long long fvco, fref, u64tmp;
  unsigned char regs_val[3], flag, i, tmp, lut_tmp, lut_val[2];
  unsigned short u16tmp, reg_val;
  int twin_fxo_div, tsu_fxo_div;
  short flag_div = 0;
  short lo_div, index, j, zero_flag = 0;
  unsigned char mlut_val[12] = {0x07, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x37, 0x0f, 0x37, 0x37, 0x08, 0x08};
  unsigned char mode_flag, mode_setting[2][10] = {
                               {0x06, 0x2, 0x6, 0x2, 0x2, 0x9F, 0x9F, 0x9F, 0x9F, 0x9F},      //for fast & tracking
                               {0x02, 0x12, 0x16, 0x12, 0x12, 0x1F, 0x1F, 0x1F, 0x1F, 0x9F},  //for old & normal
                           };
  int regCF;
  unsigned char sx_cal_index_h, sx_cal_index_l;

  //if ((flo<30000000ULL) || (flo>6000000000ULL))
  //{
  //	phy->error = -PARA_INVALID;
  //	return -PARA_INVALID;
  //}

  phy->config->core2_enable = core2_en;
  phy->config->tx_flo       = flo;

  switch (phy->config->lo_change_mode) {
    case LO_CHANGE_OLD:
    case LO_CHANGE_NORMAL:
      mode_flag = 1;
      break;
    case LO_CHANGE_FAST_LOCK:
    case LO_CHANGE_TRACKING:
    case LO_CHANGE_FAST_LOCK_ATTACH_TRX_LO:
      mode_flag = 0;
      break;
  }

  if ((phy->config->lo_change_mode == LO_CHANGE_FAST_LOCK_ATTACH_TRX_LO) || (phy->config->lo_change_mode == LO_CHANGE_OLD)) {
    hal_spi_write_reg(phy, 0x019, 0xce);
    hal_spi_write_reg(phy, 0x00E, 0xce);
  }

  if (phy->config->lo_change_mode == LO_CHANGE_OLD) {
    if (phy->config->core2_enable)
      tx1_band_selection_c2(phy);
    else
      set_trx_lo(phy, TX_DIR, TRX_CHN1, phy->config->tx_flo);
  }

  if (phy->config->lo_change_mode == LO_CHANGE_FAST_LOCK_ATTACH_TRX_LO) {
    set_trx_lo(phy, TX_DIR, TRX_CHN1, phy->config->tx_flo);
  }

  /* tx core1 config */
  regs_val[0] = hal_spi_read_reg(phy, 0x0C1);
  regs_val[1] = hal_spi_read_reg(phy, 0x0D1);
  regs_val[2] = hal_spi_read_reg(phy, 0x0D2);

  /* calc lo div */
  lo_div = calc_tx_fsm_lo_div(phy, &flag_div);

  /* calc fvco */
  fvco = flo * lo_div;
#if 0
	if (phy->config->chip_ver)
	{
		LOG_MDEBUG(phy, SX_TRX_CAL, "(%s, %d)min_vco=%llu, fvco=%llu\n", __func__, __LINE__, phy->min_vco, fvco);
		if (phy->min_vco==0)
		{
			phy->min_vco = fvco;
		}
		else
		{
			if (fvco <= phy->min_vco)
				phy->min_vco = fvco;
		}
		LOG_MDEBUG(phy, SX_TRX_CAL, "(%s, %d)min_vco=%llu, fvco=%llu\n", __func__, __LINE__, phy->min_vco, fvco);

		if (phy->min_vco >= STRX_VCO_JD_FREQ)
		{
			hal_spi_write_reg(phy, 0x619, 0x1c);
			hal_spi_write_reg(phy, 0x61a, 0x4c);
		}
		else
		{
			hal_spi_write_reg(phy, 0x619, 0x1c);
			hal_spi_write_reg(phy, 0x61a, 0x4e);
		}
	}
#endif

  if (flag_div) {
    u64tmp = fvco;
    do_div(&u64tmp, 10);
    fvco = u64tmp;
  }

  /* set mmd int & frac */
  set_mmd(phy, TX_DIR, fvco, &zero_flag);
  if (phy->config->lo_change_mode == LO_CHANGE_TRACKING) {
    hal_spi_write_reg(phy, 0x019, 0x8e);
    regCF = hal_spi_read_reg(phy, 0x0CF);
    regCF = (regCF & 0x80) | 0x47;
    hal_spi_write_reg(phy, 0x0CF, regCF);
    hal_spi_write_reg(phy, 0x0d1, 0x00);
    hal_spi_write_reg(phy, 0x0d2, 0x00);
    for (i = 0; i < 10; i++) {
      if (i < 5)
        write_lut_byte(phy, MAIN_ENSM_LUT, i + 1, 0x637, 2 + (i + 1) * 10, 1, mode_setting[0][i]);
      else
        write_lut_byte(phy, MAIN_ENSM_LUT, i - 4, 0x638, 2 + (i + 1) * 10, 2, mode_setting[0][i]);
    }
    return 0;
  }

  /* sxtrx register */
  flag = config_sxtrx(phy, TX_DIR, fvco, &index, 0, 0);

  /* TARGET CNT */
  if (phy->config->lo_change_mode != LO_CHANGE_FAST_LOCK && phy->config->lo_change_mode != LO_CHANGE_FAST_LOCK_ATTACH_TRX_LO)
    set_target_cnt(phy, TX_DIR, flo, flag, phy->config->tx_twin_fxo * lo_div, flag_div ? 10 : 1);

  /* core2 disable? bypass some status */
  core2_s7_s8_s10_s11_s12_bypass(phy, phy->config->core2_enable);

  if (phy->config->lo_change_mode == LO_CHANGE_FAST_LOCK || phy->config->lo_change_mode == LO_CHANGE_FAST_LOCK_ATTACH_TRX_LO) {
    hal_spi_write_reg(phy, 0x019, 0x8e);
    hal_spi_write_reg(phy, 0x00E, 0x8e);

    regCF = hal_spi_read_reg(phy, 0x0CF);
    regCF = (regCF & 0x80) | 0x4d;
    hal_spi_write_reg(phy, 0x0CF, regCF);
    hal_spi_write_reg(phy, 0x0D1, 0x00);
    hal_spi_write_reg(phy, 0x0D2, 0x00);
  } else {
    regCF = hal_spi_read_reg(phy, 0x0CF);
    regCF = regCF & 0x80;
    hal_spi_write_reg(phy, 0x0CF, regCF);
    hal_spi_write_reg(phy, 0x0D1, 0xa5);
    hal_spi_write_reg(phy, 0x0D2, 0x54);
    regs_val[1] = hal_spi_read_reg(phy, 0x0D1);

    /* Disable HW FCAL */
    twin_fxo_div = phy->config->tx_twin_fxo >> 4;
    i            = my_log2(8, twin_fxo_div);
    i            = (i >= 3) ? 3 : i;
    reg_val      = (0xC << 4) | ((i << 2) & 0xC);

    tsu_fxo_div = phy->config->tx_tsu_fxo >> 3;
    i           = my_log2(8, tsu_fxo_div);
    i           = (i >= 3) ? 3 : i;
    reg_val     = reg_val | (i & 0x3);

    flag    = reg_val;
    reg_val = reg_val & ((~(1 << 4)) & 0xff);
    tmp     = reg_val;
    hal_spi_write_reg(phy, 0x019, flag);

    /* state cnt selection */
    reg_val = timing_map_to_val(phy->config->tx_tstate2_fsys_us);
    reg_val = (reg_val << 4) | (regs_val[1] & 0xf) | (regs_val[1] & 0xC0);
    hal_spi_write_reg(phy, 0x0D1, reg_val);

    reg_val = timing_map_to_val(phy->config->tstate7_fsys_us);
    reg_val = (reg_val << 2) | (regs_val[2] & 0x3) | (regs_val[2] & 0xF0);
    hal_spi_write_reg(phy, 0x0D2, reg_val);
  }

  /* tx core2 cal config */
  i = my_log2(8, phy->config->dll_twin_fxo >> 5);
  i = (i >= 3) ? 3 : i;

  tmp = my_log2(8, phy->config->dll_tsu_fxo >> 6);
  tmp = (tmp >= 3) ? 3 : tmp;

  //flag = (0x4 << 4) | (i << 2) | tmp;
  flag = (0x0 << 4) | (i << 2) | tmp;
  hal_spi_write_reg(phy, 0x02E, flag);
  hal_spi_write_reg(phy, 0x038, flag);

  /* fsm target cnt */
  lut_tmp = set_fsm_tx_target_cnt(phy, phy->config->tx_flo);
  tmp     = (phy->config->tx_flo <= 2800000000ULL) ? 0x7 : 0x4;
  lut_tmp = (lut_tmp & 0xfc) | (tmp & 0x3);
  flag    = lut_tmp;
  lut_tmp = (phy->config->core2_enable) ? lut_tmp : tmp;

  /* TX WIN CAL Config */
  hal_spi_write_reg(phy, 0x06E, 0x01);
  hal_spi_write_reg(phy, 0x06F, 0x40);
  //spi_write_reg(0x042, 0x2E);
  hal_spi_write_reg(phy, 0x042, 0x0E);
  hal_spi_write_reg(phy, 0x043, 0x00);

  /* update lut */

  /* TRX BAND LUT */
  if (phy->config->core2_enable) {
    reg_val = lo_div_map_val(1, lo_div);
  } else {
    i       = my_log2(15, lo_div);
    i       = (i >= 12) ? 3 : i;
    i       = (i >= 1) ? (i - 1) : 1;
    reg_val = i;
  }

  write_lut_byte(phy, TRX_BAND_LUT, 0, 0x72A, 0, 1, reg_val);
  write_lut_byte(phy, TRX_BAND_LUT, 1, 0x72A, 14, 1, reg_val);
  write_lut_byte(phy, TRX_BAND_LUT, 0, 0x772, 6, 1, reg_val);
  write_lut_byte(phy, TRX_BAND_LUT, 1, 0x772, 20, 1, reg_val);

  write_lut_byte(phy, TRX_BAND_LUT, 0, 0x703, 0, 0, lut_tmp);
  write_lut_byte(phy, TRX_BAND_LUT, 1, 0x703, 14, 0, lut_tmp);
  write_lut_byte(phy, TRX_BAND_LUT, 0, 0x74B, 6, 0, lut_tmp);
  write_lut_byte(phy, TRX_BAND_LUT, 1, 0x74B, 20, 0, lut_tmp);

  /* MAIN_ENSM_LUT */
  reg_val = lo_div_map_val(0x20, lo_div);
  reg_val = (reg_val >> 5) & 0x7;

  if (fvco >= 6000000000ULL) {
    lut_val[0] = reg_val << 5;
    lut_val[1] = 0x23;
  } else {
    lut_val[0] = 0x24;
    lut_val[1] = reg_val << 5;
  }

  if (phy->config->core2_enable)
    reg_val = flag;
  else
    reg_val = 0;

  write_lut_byte(phy, MAIN_ENSM_LUT, 0, 0x72A + LUT_DIST_DELTA, 272, 3, lut_val[0]);
  write_lut_byte(phy, MAIN_ENSM_LUT, 1, 0x72A + LUT_DIST_DELTA, 274, 3, lut_val[1]);
  write_lut_byte(phy, MAIN_ENSM_LUT, 0, 0x772 + LUT_DIST_DELTA, 273, 1, lut_val[0]);
  write_lut_byte(phy, MAIN_ENSM_LUT, 1, 0x772 + LUT_DIST_DELTA, 275, 1, lut_val[1]);

  write_lut_byte(phy, MAIN_ENSM_LUT, 0, 0x703 + LUT_DIST_DELTA, 272, 2, reg_val);
  write_lut_byte(phy, MAIN_ENSM_LUT, 1, 0x703 + LUT_DIST_DELTA, 274, 2, reg_val);
  write_lut_byte(phy, MAIN_ENSM_LUT, 0, 0x74B + LUT_DIST_DELTA, 273, 0, reg_val);
  write_lut_byte(phy, MAIN_ENSM_LUT, 1, 0x74B + LUT_DIST_DELTA, 275, 0, reg_val);

  for (i = 0; i < 10; i++) {
    if (i < 5)
      write_lut_byte(phy, MAIN_ENSM_LUT, i + 1, 0x637, 2 + (i + 1) * 10, 1, mode_setting[mode_flag][i]);
    else
      write_lut_byte(phy, MAIN_ENSM_LUT, i - 4, 0x638, 2 + (i - 4) * 10, 2, mode_setting[mode_flag][i]);
  }

  /* SX_FCAL INDEX */
  flag = (fvco >= 6000000000ULL) ? 0 : 1;
  if (phy->config->x4_enable) {
    fref = phy->config->xtal_freq * 4;
  } else {
    fref = phy->config->xtal_freq * 2;
  }
  i       = (fref >= 90000000UL) ? 1 : 0;
  reg_val = (i << 7) | (flag << 6) | (regs_val[0] & 0x3f);
  hal_spi_write_reg(phy, 0x0C1, reg_val);

  /* SXRX_Config_INDEX */
  hal_spi_write_reg(phy, 0x0C8, index);

  u16tmp = fvco_get_right_index(phy, fvco, 1);
  u16tmp = u16tmp & 0x1ff;
  if (phy->config->lo_change_mode == LO_CHANGE_FAST_LOCK_ATTACH_TRX_LO) {
    sx_cal_index_h = hal_spi_read_reg(phy, 0x0C2);
    sx_cal_index_l = hal_spi_read_reg(phy, 0x0C3);
    if (u16tmp == (((sx_cal_index_h & 0x01) << 8) | sx_cal_index_l)) {
      u16tmp = u16tmp + 1;
      u16tmp = u16tmp & 0x1ff;
    }
  }
  reg_val = (u16tmp >> 8) & 0x1;
  hal_spi_write_reg(phy, 0x0C4, reg_val);
  hal_spi_write_reg(phy, 0x0C5, (u16tmp)&0xff);

  index = fvco_get_right_index(phy, phy->config->tx_flo, 2);
  hal_spi_write_reg(phy, 0x0C6, index);

  /* trx band lut tx part update */
  index = find_case_with_flo(phy->config->tx_flo);
  for (i = 0; i < 48; i++) {
    u16tmp = g_trx_band_regs[i];
    if ((u16tmp != 0x703) && (u16tmp != 0x72A) &&
        (u16tmp != 0x74B) && (u16tmp != 0x772)) {
      write_lut_byte(phy, TRX_BAND_LUT, 0, u16tmp, i / 4, i % 4, g_trx_band_config[index][i]);
      write_lut_byte(phy, TRX_BAND_LUT, 1, u16tmp, 14 + i / 4, i % 4, g_trx_band_config[index][i]);
    }
  }

  /* tx pa gain lut update */
  u16tmp = index * 9;
  tmp    = 0;
  for (i = u16tmp; i <= (u16tmp + 8); i++) {
    for (j = 0; j < 20; j++) {
      write_lut_byte(phy, TX_GAIN_LUT, i - u16tmp, g_tx_pa_gain_regs[j], 200 + 5 * tmp + j / 4, j % 4, g_tx_pa_gain_config[i][j]);
    }
    tmp = tmp + 1;
  }

  /* about core2 lut */
  if (phy->config->core2_enable) {
    for (i = 0; i < 12; i++) {
      switch (phy->config->mode) {
        case RX1_TX2_FDD:
        case CH2_FDD:
        case CH2_TDD:
          //(LUT_INDEX_ENUM lut, short index, unsigned short reg, int lut_addr, unsigned char offset, unsigned char val)
          write_lut_byte(phy, MAIN_ENSM_LUT, i + 14, 0x749, 148 + i * 10, 2, mlut_val[i]);
          break;
        case CH1_FDD:
        case CH1_TDD:
        case RX2_TX1_FDD:
          write_lut_byte(phy, MAIN_ENSM_LUT, i + 14, 0x701, 146 + i * 10, 3, mlut_val[i]);
          break;
        case CH1_CH2_FDD:
        case CH1_CH2_TDD:
          write_lut_byte(phy, MAIN_ENSM_LUT, i + 14, 0x701, 146 + i * 10, 3, mlut_val[i]);
          write_lut_byte(phy, MAIN_ENSM_LUT, i + 14, 0x749, 148 + i * 10, 2, mlut_val[i]);
          break;
      }
    }
  }

  hal_spi_write_reg(phy, 0x701, 0x37);
  hal_spi_write_reg(phy, 0x749, 0x37);

  reg_val = zero_flag ? 0x8F : 0x9F;
  hal_spi_write_reg(phy, 0x638, reg_val);
  for (i = 0; i < 22; i++)
    write_lut_byte(phy, MAIN_ENSM_LUT, 5 + i, 0x638, 52 + 10 * i, 2, reg_val);

  return 0;
}

/****************************************************************************************/
// core2
/****************************************************************************************/
int tx1_band_selection_c2(rf_chip_phy_t* phy) {
  unsigned int tmp[10];

  tmp[0] = hal_spi_read_reg(phy, 0x72A);
  tmp[1] = hal_spi_read_reg(phy, 0x702);
  tmp[2] = hal_spi_read_reg(phy, 0x703);
  tmp[3] = hal_spi_read_reg(phy, 0x704);
  tmp[4] = hal_spi_read_reg(phy, 0x747);
  tmp[5] = hal_spi_read_reg(phy, 0x701);

  sxtx_fcal_and_on(phy);
  tx1_c2_on_config(phy, tmp);
  tx1_c2_hw_dll_cal(phy);
  //tx1_c2_win_sw_cal(phy);
  tx1_c2_win_hw_cal(phy);
  tx1_c2_reload(phy, tmp);

  return 0;
}

short tx1_c2_lo_config(unsigned long long lo, short fcal_div) {
  if ((lo > 1500000000ULL) && (fcal_div == 2))
    return 0xE8;
  else if ((lo > 1500000000ULL) && (fcal_div == 4))
    return 0xC8;
  else if ((lo <= 1500000000ULL) && (fcal_div == 2))
    return 0xF0;
  else
    return 0xD0;
}

short tx1_c2_div_ratio(unsigned long long lo, short* lo_div) {
  if (lo > 2150000000ULL) {
    *lo_div = 25;
    return 0x20;
  } else if (lo > 1080000000ULL) {
    *lo_div = 50;
    return 0x40;
  } else if (lo > 540000000ULL) {
    *lo_div = 100;
    return 0x80;
  } else {
    *lo_div = 25;
    return 0x20;
  }
}

int sxtx_fcal_and_on(rf_chip_phy_t* phy) {
  unsigned int reg_val;
  short lo_div;
  unsigned long long fvco;
  int i, j, flag, index, reg_cnt, fcal_div, twin_fxo_div, tsu_fxo_div;
  unsigned int tmp_val;
  unsigned long long u64_div_result, u64_div_mod, u64_div_tmp1, u64_div_tmp2;
  unsigned short lut_val = 0;
  short have_frac, out, zero_flag = 0;

  /* calc lo div */
  tx1_c2_div_ratio(phy->config->tx_flo, &lo_div);

  /* set mmd int & frac */
  fvco = lo_div * phy->config->tx_flo;
#if 0
	if (phy->config->chip_ver)
	{
		LOG_MDEBUG(phy, SX_TRX_CAL, "(%s, %d)min_vco=%llu, fvco=%llu\n", __func__, __LINE__, phy->min_vco, fvco);
		if (phy->min_vco==0)
		{
			phy->min_vco = fvco;
		}
		else
		{
			if (fvco <= phy->min_vco)
				phy->min_vco = fvco;
		}
		LOG_MDEBUG(phy, SX_TRX_CAL, "(%s, %d)min_vco=%llu, fvco=%llu\n", __func__, __LINE__, phy->min_vco, fvco);

		if (phy->min_vco >= STRX_VCO_JD_FREQ)
		{
			hal_spi_write_reg(phy, 0x619, 0x1c);
			hal_spi_write_reg(phy, 0x61a, 0x4c);
		}
		else
		{
			hal_spi_write_reg(phy, 0x619, 0x1c);
			hal_spi_write_reg(phy, 0x61a, 0x4e);
		}
	}
#endif

  do_div(&fvco, 10);
  have_frac = set_mmd(phy, TX_DIR, fvco, &zero_flag);

  /* sxtrx register */
  flag = config_sxtrx(phy, TX_DIR, fvco, &out, 0, 1);

  /* x4 config */
  //refclk_config(phy->config->x4_enable);

  /* TARGET CNT */
  set_target_cnt(phy, TX_DIR, phy->config->tx_flo, flag, (lo_div * phy->config->tx_twin_fxo) / 10, 1);

  /* SX manual enable, CAL manual disable
	*/
  hal_spi_write_reg(phy, 0x64D, 0x04);

  /* TX1 manual enable
	*/
  hal_spi_write_reg(phy, 0x747, 0x01);

  /* SX FCAL enable (HW FCAL, unused)
	*/
  hal_spi_write_reg(phy, 0x637, 0x12);

  /* Disable HW FCAL
	*/
  i       = my_log2(8, phy->config->tx_twin_fxo >> 4);
  i       = (i >= 3) ? 3 : i;
  reg_val = (0xD << 4) | ((i << 2) & 0xC);

  i       = my_log2(8, phy->config->tx_tsu_fxo >> 3);
  i       = (i >= 3) ? 3 : i;
  reg_val = reg_val | (i & 0x3);

  flag    = reg_val;
  reg_val = reg_val & ((~(1 << 4)) & 0xff);
  tmp_val = reg_val;
  hal_spi_write_reg(phy, 0x019, reg_val);

  /* SX  blocks enable
	*/
  if (have_frac == 0)
    reg_val = 0x0f;
  else
    reg_val = 0x1f;
  hal_spi_write_reg(phy, 0x638, reg_val);

  /* SX to LO enable
	*/
  //spi_write_reg(0x639, 0x44);

  /* HW FCAL enable */
  hal_spi_write_reg(phy, 0x019, flag);
  /* DSM RSTN=0 (reset) */
  hal_spi_write_reg(phy, 0x637, 0x16);
  /* DSM RSTN=1 */
  hal_spi_write_reg(phy, 0x637, 0x12);

  /* Enable SX PFD and CP */
  if (have_frac == 0)
    reg_val = 0x8f;
  else
    reg_val = 0x9f;
  hal_spi_write_reg(phy, 0x638, reg_val);

  /* Disable HW FCAL */
  hal_spi_write_reg(phy, 0x019, tmp_val);

  /* Disable SX FCAL, close loop */
  hal_spi_write_reg(phy, 0x637, 0x02);

  return 0;
}

int tx1_c2_on_config(rf_chip_phy_t* phy, unsigned int regs_val[]) {
  unsigned int reg_val;
  int mmd_div, mmd_mod;
  int i, flag, fcal_div, log1, log2, factor1, factor2;
  short lo_div;
  unsigned int tmp_val;
  unsigned long long u64_div_mod, u64_div_tmp1, u64_div_tmp2;

  fcal_div = 4;
  tmp_val  = tx1_c2_lo_config(phy->config->tx_flo, fcal_div);

  log1 = (phy->config->dll_twin_fxo) >> 5;
  i    = my_log2(8, log1);
  log1 = (i >= 3) ? 3 : i;

  log2 = (phy->config->dll_tsu_fxo) >> 6;
  i    = my_log2(8, log2);
  log2 = (i >= 3) ? 3 : i;

  flag = (0x5 << 4) | (log1 << 2) | log2;

  /* LO Config */
  reg_val = (tmp_val & 0xfc) | (regs_val[2] & 0x3);
  hal_spi_write_reg(phy, 0x703, reg_val);
  hal_spi_write_reg(phy, 0x72F, 0x11);

  /* DIV Ratio */
  reg_val = tx1_c2_div_ratio(phy->config->tx_flo, &lo_div);
  hal_spi_write_reg(phy, 0x72A, reg_val);

  /* TARGET CNT */
  i = my_log2(15, phy->config->dll_twin_fxo);
  if (tmp_val == 0xE8) {
    factor1 = 3;
    factor2 = 22;
  } else if (tmp_val == 0xC8) {
    factor1 = 3;
    factor2 = 44;
  } else if (tmp_val == 0xF0) {
    factor1 = 4;
    factor2 = 22;
  } else if (tmp_val == 0xD0) {
    factor1 = 4;
    factor2 = 44;
  } else {
    factor1 = 3;
    factor2 = 22;
  }

  u64_div_tmp1 = (phy->config->tx_flo << (i + factor1));
  u64_div_tmp2 = (factor2 * phy->config->xtal_freq);
  u64_div_mod  = do_div(&u64_div_tmp1, u64_div_tmp2);
  mmd_div      = u64_div_tmp1;
  mmd_mod      = u64_div_mod;

  reg_val = (mmd_div >> 8) & 0xff;
  hal_spi_write_reg(phy, 0x030, reg_val);
  reg_val = mmd_div & 0xff;
  hal_spi_write_reg(phy, 0x031, reg_val);
  //spi_write_reg(0x702, 0x0F);

  return 0;
}

int tx1_c2_hw_dll_cal(rf_chip_phy_t* phy) {
  unsigned int reg_val;
  int i, flag, log1, log2;

  log1 = (phy->config->dll_twin_fxo) >> 5;
  i    = my_log2(8, log1);
  log1 = (i >= 3) ? 3 : i;

  log2 = (phy->config->dll_tsu_fxo) >> 6;
  i    = my_log2(8, log2);
  log2 = (i >= 3) ? 3 : i;

  flag = (0x5 << 4) | (log1 << 2) | log2;

  /* TX1 manunal enable
	*/
  //reg_val = regs_val[4] & 0xfb;
  reg_val = hal_spi_read_reg(phy, 0x747);
  reg_val = reg_val & 0xfb;
  hal_spi_write_reg(phy, 0x747, reg_val);

  /* Core2 RST
	*/
  hal_spi_write_reg(phy, 0x701, 0x3F);

  /* Enable TXLO DLL FCAL
	*/
  hal_spi_write_reg(phy, 0x704, 0x04);

  /* disable TXLO DLL HW FCAL */
  reg_val = CLR_BIT(flag, BIT4);
  hal_spi_write_reg(phy, 0x02E, reg_val);

  /* enable TXLO DLL HW FCAL */
  hal_spi_write_reg(phy, 0x02E, flag);

  /* disable TXLO DLL HW FCAL */
  reg_val = CLR_BIT(flag, BIT4);
  hal_spi_write_reg(phy, 0x02E, reg_val);

  /* disable TXLO DLL FCAL */
  hal_spi_write_reg(phy, 0x704, 0x00);

  /* Core2 RST disable */
  hal_spi_write_reg(phy, 0x701, 0x07);

  return 0;
}

int tx1_c2_win_sw_cal(rf_chip_phy_t* phy) {
  unsigned int reg_val;
  int i;
  int cal1, cal2, cal3, cal4, cal5;
  unsigned int tmp[15];

  reg_val = hal_spi_read_reg(phy, 0x747);
  reg_val = reg_val | 0x8;

  /* TX1 manunal enable */
  hal_spi_write_reg(phy, 0x747, reg_val);
  /* FRAC CAL disable */
  hal_spi_write_reg(phy, 0x704, 0x00);

  /* read AD_TX1SX_LO_FRAC_DCAL[0] */
  tmp[0] = hal_spi_read_reg(phy, 0x851);

  for (i = 1; i <= 14; i++) {
    /* FCAL DLY=i */
    hal_spi_write_reg(phy, 0x72E, i);
    /* FRAC RST */
    hal_spi_write_reg(phy, 0x701, 0x17);
    /* FRAC CAL disable */
    hal_spi_write_reg(phy, 0x704, 0x00);
    /* FRAC CAL enable */
    hal_spi_write_reg(phy, 0x704, 0x01);
    /* FRAC RST disable */
    hal_spi_write_reg(phy, 0x701, 0x07);
    /* read AD_TX1SX_LO_FRAC_DCAL[0] */
    tmp[i] = hal_spi_read_reg(phy, 0x851);
  }

  cal1 = -1;
  cal2 = -1;
  for (i = 0; i <= 14; i++) {
    if (tmp[i] == 1) {
      if (cal1 == -1)
        cal1 = i;

      if (i == 14) {
        cal2 = i;
        break;
      }
    } else {
      if (cal1 >= 0) {
        cal2 = i - 1;
        break;
      }
    }
  }

  if ((cal1 == -1) && (cal2 == -1)) {
    cal3 = 0;
  } else {
    cal4 = (cal1 + cal2) / 2;
    cal5 = (cal1 + cal2) % 2;
    if (cal5)
      cal3 = cal4 + 1;
    else
      cal3 = cal4;

    if (IsParity(cal4) == 0) {
      if (cal5 != 0)
        cal3 = cal3 + 1;
    }
  }

  if (cal3 > 0)
    cal3 = 0;
  hal_spi_write_reg(phy, 0x72E, cal3);

  /* Core2 RST */
  hal_spi_write_reg(phy, 0x701, 0x3F);
  /* Core2 RST disable */
  hal_spi_write_reg(phy, 0x701, 0x0F);

  return 0;
}

int tx1_c2_win_hw_cal(rf_chip_phy_t* phy) {
  unsigned int reg_val;

  /* TX1 manunal enable
	*/
  reg_val = hal_spi_read_reg(phy, 0x747);
  reg_val = reg_val & 0xf7;
  hal_spi_write_reg(phy, 0x747, reg_val);

  /* Core2 RST disable */
  hal_spi_write_reg(phy, 0x701, 0x3F);
  /* Core2 RST disable */
  hal_spi_write_reg(phy, 0x701, 0x0F);
  /* Core2 DLY Start Code(1-1) */
  hal_spi_write_reg(phy, 0x06E, 0x01);
  /* 1bit-DLY Code Period Setting */
  hal_spi_write_reg(phy, 0x06F, 0x40);
  /* WIN CAL Disable */
  hal_spi_write_reg(phy, 0x704, 0x00);
  /* WIN HW CAL EN and DLY End Code Setting(E-14) */
  hal_spi_write_reg(phy, 0x042, 0x2E);
  /* WIN CAL Result ADD */
  hal_spi_write_reg(phy, 0x043, 0x00);
  /* WIN CAL EN */
  hal_spi_write_reg(phy, 0x704, 0x02);
  /* WIN CAL Disable */
  hal_spi_write_reg(phy, 0x704, 0x00);

  return 0;
}

int tx1_c2_reload(rf_chip_phy_t* phy, unsigned int regs_val[]) {
  unsigned int reg_val;
  int fcal_div;
  unsigned int tmp_val;

  fcal_div = 4;
  tmp_val  = tx1_c2_lo_config(phy->config->tx_flo, fcal_div);

  reg_val = (tmp_val & 0xfc) | (regs_val[2] & 0x3);
  hal_spi_write_reg(phy, 0x702, regs_val[1]);
  hal_spi_write_reg(phy, 0x703, reg_val);
  hal_spi_write_reg(phy, 0x701, regs_val[5]);

  reg_val = hal_spi_read_reg(phy, 0x022);
  reg_val = hal_spi_read_reg(phy, 0x023);
  reg_val = hal_spi_read_reg(phy, 0x032);
  reg_val = hal_spi_read_reg(phy, 0x037);
  reg_val = hal_spi_read_reg(phy, 0x043);
  reg_val = hal_spi_read_reg(phy, 0x044);

  hal_spi_write_reg(phy, 0x701, 0x08);

  return 0;
}

/****************************************************************************************/
// config syspll
/****************************************************************************************/
short config_syspll(rf_chip_phy_t* phy, unsigned long freq, unsigned long long vco_freq) {
  unsigned long long mmd_div;
  unsigned long long mmd_mod, bk;
  unsigned long long u64tmp;
  short flag, i;
  syspll_select_t select[] =
      {
          {0x606, 0x08, 0x08, 0x08, 0x08},
          {0x607, 0x88, 0x88, 0x88, 0x08},
          {0x608, 0x08, 0x08, 0x08, 0x08},
          {0x609, 0x08, 0x08, 0x08, 0x08},
          {0x60C, 0x10, 0x10, 0x10, 0x08},
      };
  syspll_range_t f_range[] =
      {
          {2500000000ULL, 3000000000ULL},
          {2200000000ULL, 2500000000ULL},
          {1800000000ULL, 2200000000ULL},
          {1250000000ULL, 1800000000ULL},
      };
  short select_len               = sizeof(select) / sizeof(select[0]);
  short range_len                = sizeof(f_range) / sizeof(f_range[0]);
  unsigned char reg_6f1_val      = 0;
  unsigned char reg_605_val      = 0;
  unsigned char reg_605_read_val = 0;
  unsigned long long syspll_list[][2] =
      {
          {3035000000ULL, 3600000000ULL},
          {2850000000ULL, 3035000000ULL},
          {2700000000ULL, 2850000000ULL},
          {2600000000ULL, 2700000000ULL},
          {2450000000ULL, 2600000000ULL},
          {2300000000ULL, 2450000000ULL},
          {2200000000ULL, 2300000000ULL},
          {2010000000ULL, 2200000000ULL},
          {1800000000ULL, 2010000000ULL},
          {1600000000ULL, 1800000000ULL},
      };
  unsigned char reg_val[][2] =
      {
          {0x0, 0x3},
          {0x1, 0x3},
          {0x2, 0x3},
          {0x1, 0x2},
          {0x2, 0x2},
          {0x1, 0x1},
          {0x2, 0x1},
          {0x3, 0x1},
          {0x2, 0x0},
          {0x3, 0x0},
      };
  unsigned int syspll_list_len = sizeof(syspll_list) / sizeof(syspll_list[0]);

  if (((hal_spi_read_reg(phy, 0xD33) & 0xF0) >> 4) != 0x07) {
    LOG_ERROR("Reg 0xD33 Err.\n");
    return -1;
  }

  if ((vco_freq < 1600000000ULL) || (vco_freq > 3600000000ULL)) {
    phy->error = -PARA_INVALID;
    return -PARA_INVALID;
  }

  /* MMD INT */
  u64tmp = vco_freq;
  //LOG_INFO("syspll: fvco=%llu, xtal=%lu\r\n", u64tmp, freq);
  mmd_mod = do_div(&u64tmp, freq);
  bk      = mmd_mod;
  mmd_div = u64tmp;
  //LOG_INFO("syspll: mmd_mod=%llu, mmd_div=%llu\r\n", mmd_mod, mmd_div);

  hal_spi_write_reg(phy, 0x610, (mmd_div >> 8) & 7);
  hal_spi_write_reg(phy, 0x611, mmd_div & 0xff);

  /* MMD FRAC */
  u64tmp = (mmd_mod << 23);
  //LOG_INFO("syspll: 1=%llu, 2=%lu\r\n", u64tmp, freq);
  do_div(&u64tmp, freq);
  //LOG_INFO("syspll: a1=%llu, a2=%lu\r\n", u64tmp, freq);
  u64tmp = u64tmp + get_round_case(mmd_mod << 23, freq);
  //LOG_INFO("syspll: v=%llu\r\n", u64tmp);

  hal_spi_write_reg(phy, 0x60D, (u64tmp >> 16) & 0xff);
  hal_spi_write_reg(phy, 0x60E, (u64tmp >> 8) & 0xff);
  hal_spi_write_reg(phy, 0x60F, u64tmp & 0xff);

  /* TARGET CNT */
  u64tmp  = (vco_freq << 1);
  mmd_mod = do_div(&u64tmp, freq);
  mmd_div = u64tmp;

  hal_spi_write_reg(phy, 0x026, (mmd_div >> 8) & 0xff);
  hal_spi_write_reg(phy, 0x027, mmd_div & 0xff);

  /* CNT WIN */
  hal_spi_write_reg(phy, 0x024, 0x01);

  for (i = 0; i < syspll_list_len; i++) {
    if ((vco_freq < syspll_list[i][1]) && (vco_freq >= syspll_list[i][0])) {
      reg_6f1_val = reg_val[i][0];
      reg_605_val = reg_val[i][1];
      break;
    }
  }

  /* band related setting */
  hal_spi_write_reg(phy, 0x6F1, (reg_6f1_val << 4) | 0x0b);
  reg_605_read_val = hal_spi_read_reg(phy, 0x605);
  reg_605_read_val = reg_605_read_val & (~0x60);
  hal_spi_write_reg(phy, 0x605, (reg_605_val << 5) | reg_605_read_val);

  if ((phy->config->custom_bandwith_flag) || (phy->config->syspll_cfg_flag)) {
    hal_spi_write_reg(phy, 0x604, phy->config->dac_syspll_lo_div);
    hal_spi_write_reg(phy, 0x6f0, phy->config->adc_syspll_lo_div);
  }

  flag = -1;
  for (i = 0; i < range_len; i++) {
    if ((vco_freq < f_range[i].f_max) && (vco_freq >= f_range[i].f_min)) {
      flag = i;
      break;
    }
  }

  for (i = 0; i < select_len; i++) {
    if (flag == 0)
      hal_spi_write_reg(phy, select[i].addr, select[i].val1);
    else if (flag == 1)
      hal_spi_write_reg(phy, select[i].addr, select[i].val2);
    else if (flag == 2)
      hal_spi_write_reg(phy, select[i].addr, select[i].val3);
    else if (flag == 3)
      hal_spi_write_reg(phy, select[i].addr, select[i].val4);
    else
      hal_spi_write_reg(phy, select[i].addr, select[i].val1);
  }

  /* open fcal */
  hal_spi_write_reg(phy, 0x600, 0x01);

  if (bk == 0) {
    hal_spi_write_reg(phy, 0x613, 0x01);
  } else {
    hal_spi_write_reg(phy, 0x613, 0x1e);
    hal_spi_write_reg(phy, 0x612, 0x94);
  }

  hal_spi_write_reg(phy, 0x602, 0x00);

  if (bk == 0)
    hal_spi_write_reg(phy, 0x601, 0x8F);
  else
    hal_spi_write_reg(phy, 0x601, 0x9F);

  hal_spi_write_reg(phy, 0x60b, 0x03);
  hal_spi_write_reg(phy, 0x600, 0x01);
  hal_spi_write_reg(phy, 0x600, 0x03);
  hal_spi_write_reg(phy, 0x600, 0x07);
  hal_spi_write_reg(phy, 0x600, 0x03);
  hal_spi_write_reg(phy, 0x600, 0x13);

  /* fcal off */
  if (HAL_REG_GET_BIT(phy, 0x028, 5) == 1)
    hal_spi_write_reg(phy, 0x600, 0x03);

  return 0;
}

/****************************************************************************************/
// set port
/****************************************************************************************/
void set_rx_port(rf_chip_phy_t* phy, TRX_CHN_ENUM chn, RX_PORT_ENUM port, RXFE_GAIN_ENUM gain, short en) {
  short i, index, tmp;
  unsigned short rx_port[2][4] =
      {
          {
              0x65F,
              0x660,
              0x661,
              0x66D,
          },
          {
              0x67D,
              0x67E,
              0x67F,
              0x68B,
          },
      };
  unsigned short fsm_about_rx_port[10] =
      {
          0x02,
          0x01,
          0x03,
          0x08,
          0x04,
          0x0C,
          0x20,
          0x10,
          0x30,
          0x40,
      };

  //index = 1 + 9 * (gain) + port;
  index = 9 * (gain) + port;
  tmp   = (chn == TRX_CHN1) ? 0 : 1;

  if (port != TXMON) {
    for (i = 0; i < 4; i++) {
      if (en)
        hal_spi_write_reg(phy, rx_port[tmp][i], g_rx_port[index][i]);
      else
        hal_spi_write_reg(phy, rx_port[tmp][i], 0);
    }
  }

  if (!en) {
    hal_spi_write_reg(phy, chn ? 0x66f : 0x651, 0);
  }

  tmp = (chn == TRX_CHN1) ? 0x0E0 : 0x0E1;
  if (en)
    hal_spi_write_reg(phy, tmp, fsm_about_rx_port[port]);
  else
    hal_spi_write_reg(phy, tmp, 0);
}

void set_rx_port_man(rf_chip_phy_t* phy, TRX_CHN_ENUM chn, RX_PORT_ENUM port, RXFE_GAIN_ENUM gain) {
  short i, index, tmp;
  unsigned short rx_port[2][4] =
      {
          {
              0x65F,
              0x660,
              0x661,
              0x66D,
          },
          {
              0x67D,
              0x67E,
              0x67F,
              0x68B,
          },
      };

  //index = 1 + 9 * (gain) + port;
  index = 9 * (gain) + port;
  tmp   = (chn == TRX_CHN1) ? 0 : 1;
  for (i = 0; i < 4; i++)
    hal_spi_write_reg(phy, rx_port[tmp][i], g_rx_port[index][i]);
}

void set_tx_port(rf_chip_phy_t* phy, TRX_CHN_ENUM chn, TX_PORT_ENUM port, short en) {
  unsigned char config[2] = {0x02, 0x04};  //A, B
  unsigned char lut_val;

  //if (chn == TRX_CHN1)
  //	spi_write_reg(0x700, config[port]);
  //else
  //	spi_write_reg(0x748, config[port]);

  if (en)
    lut_val = config[port];
  else
    lut_val = 0;

  if (chn == TRX_CHN1) {
    write_lut_byte(phy, MAIN_ENSM_LUT, 17, 0x700, 176, 2, lut_val);
    write_lut_byte(phy, MAIN_ENSM_LUT, 18, 0x700, 186, 2, lut_val);
    write_lut_byte(phy, MAIN_ENSM_LUT, 19, 0x700, 196, 2, lut_val);
    write_lut_byte(phy, MAIN_ENSM_LUT, 21, 0x700, 216, 2, lut_val);
    write_lut_byte(phy, MAIN_ENSM_LUT, 24, 0x700, 246, 2, lut_val);
    write_lut_byte(phy, MAIN_ENSM_LUT, 25, 0x700, 256, 2, lut_val);
  } else {
    write_lut_byte(phy, MAIN_ENSM_LUT, 17, 0x748, 178, 1, lut_val);
    write_lut_byte(phy, MAIN_ENSM_LUT, 18, 0x748, 188, 1, lut_val);
    write_lut_byte(phy, MAIN_ENSM_LUT, 19, 0x748, 198, 1, lut_val);
    write_lut_byte(phy, MAIN_ENSM_LUT, 21, 0x748, 218, 1, lut_val);
    write_lut_byte(phy, MAIN_ENSM_LUT, 24, 0x748, 248, 1, lut_val);
    write_lut_byte(phy, MAIN_ENSM_LUT, 25, 0x748, 258, 1, lut_val);
  }
}

/****************************************************************************************/
// tx dac calibration
/****************************************************************************************/
unsigned int tx_dac_val_cat(unsigned int h_val, unsigned int l_val) {
  int count, i;
  unsigned int ret, tmp, tmp1, tmp2;
  char is_ok;

  count = 0;
  ret   = 0;
  tmp1  = (h_val & 0x3) << 5;
  tmp2  = (l_val & 0xf8) >> 3;
  tmp   = tmp1 | tmp2;
  tmp2  = l_val & 0x7;

  for (i = 0; i <= 6; i++) {
    is_ok = (tmp >> i) & 0x1;
    if (is_ok == 1)
      count++;
    else
      break;
  }

  ret = tmp2 | (count << 3);

  return ret;
}

unsigned int tx_dac_val_corv(unsigned int val) {
  int count;
  unsigned int ret, tmp, tmp1, tmp2;
  int calc_factor[8] = {0, 1, 3, 7, 15, 31, 63, 127};

  tmp   = val & 0x7;
  count = (val & 0x38) >> 3;
  tmp1  = calc_factor[count];

  ret = tmp | (tmp1 << 3);

  return ret;
}

static unsigned int iq_dac_val[2][255];

#define MAXCNT   (80)
#define MAXCNT_1 144

#define RG_AUXADC1_861               0x861
#define RG_TEST_TX1ABB_DC_OFFSET_83A 0x83A
#define RG_TEST_TX1ABB_DC_SEL_833    0x833
#define RG_TEST_MT1P1N_AUXAC1_843    0x843
#define RG_TX1_MANUAL_747            0x747
#define RG_TX1DACI_DC_LV_H_7C4       0x7C4
#define RG_TX1DACI_DC_LV_L_7C5       0x7C5
#define RG_TX1DACQ_DC_LV_H_7C2       0x7C2
#define RG_TX1DACQ_DC_LV_L_7C3       0x7C3

#define RG_TX1DAC_0_CTRL_790       0x790
#define RG_TX1DAC_1_CTRL_791       0x791
#define RG_TX1DAC_2_Q_RDATA_H_792  0x792
#define RG_TX1DAC_3_Q_RDATA_L_793  0x793
#define RG_TX1DAC_4_I_RDATA_H_794  0x794
#define RG_TX1DAC_5_I_RDATA_L_795  0x795
#define RG_TX1DAC_6_IQ_WDATA_H_796 0x796
#define RG_TX1DAC_7_IQ_WDATA_L_797 0x797
#define RG_TX1DAC_8_ADDR_798       0x798
#define RG_TX1DAC_18_CTRL_7A2      0x7A2
#define RG_TX1DAC_26_STA_7C7       0x7C7

#define RG_AUXADC1_865        0x865
#define RG_AUXADC1_866        0x866
#define RG_AUXADC1_860        0x860
#define RG_TX1_738            0x738
#define RG_TX1_715            0x715
#define RG_TX1_716            0x716
#define RG_TX1_717            0x717
#define RG_TX1_718            0x718
#define RG_TX1DAC_WDATA_H_796 0x796
#define RG_TX1DAC_WDATA_L_797 0x797
#define RG_TX1DAC_ADDR_798    0x798
#define RG_TX1DAC_CTRL_7A2    0x7A2

#define RG_TX2DAC_0_CTRL_7A3  0x7A3
#define RG_TX2DAC_26_STA_7D7  0x7D7
#define RG_TX2DAC_WDATA_H_7A9 0x7A9
#define RG_TX2DAC_WDATA_L_7AA 0x7AA
#define RG_TX2DAC_ADDR_7AB    0x7AB
#define RG_TX2DAC_CTRL_7B5    0x7B5

void Set_TXDAC_n_calc_result(rf_chip_phy_t* phy, int channel, int regNum, int flag, unsigned int cal_val) {
  int i;
  int tmp, reg_val1, reg_val2;
  unsigned int tmp_iq_dac_val;

  tmp_iq_dac_val = cal_val;
  if (flag)
    LOG_MDEBUG(phy, TX_DAC_CAL, "[I Path] Cal Val: %d\n", tmp_iq_dac_val);
  else
    LOG_MDEBUG(phy, TX_DAC_CAL, "[Q Path] Cal Val: %d\n", tmp_iq_dac_val);
  //LOG_MDEBUG(TX_DAC_CAL, "[%03d] Val: %d\n", flag,  tmp_iq_dac_val);
  if (channel == TRX_CHN1) {
    if (flag) {
      /* i path */
      hal_spi_write_reg(phy, RG_TX1DAC_CTRL_7A2, 0x0);
      hal_spi_write_reg(phy, RG_TX1DAC_ADDR_798, regNum);
      reg_val1 = (tmp_iq_dac_val >> 8) & 0xff;
      reg_val2 = tmp_iq_dac_val & 0xff;
      hal_spi_write_reg(phy, RG_TX1DAC_WDATA_H_796, reg_val1);
      hal_spi_write_reg(phy, RG_TX1DAC_WDATA_L_797, reg_val2);
      hal_spi_write_reg(phy, RG_TX1DAC_CTRL_7A2, 0x3);
      hal_spi_write_reg(phy, RG_TX1DAC_CTRL_7A2, 0x0);
    } else { /* Q path */
      hal_spi_write_reg(phy, RG_TX1DAC_CTRL_7A2, 0x0);
      hal_spi_write_reg(phy, RG_TX1DAC_ADDR_798, regNum);
      reg_val1 = (tmp_iq_dac_val >> 8) & 0xff;
      reg_val2 = tmp_iq_dac_val & 0xff;
      hal_spi_write_reg(phy, RG_TX1DAC_WDATA_H_796, reg_val1);
      hal_spi_write_reg(phy, RG_TX1DAC_WDATA_L_797, reg_val2);
      hal_spi_write_reg(phy, RG_TX1DAC_CTRL_7A2, 0x5);
      hal_spi_write_reg(phy, RG_TX1DAC_CTRL_7A2, 0x0);
    }
  } else {
    if (flag) {
      /* i path */
      hal_spi_write_reg(phy, RG_TX2DAC_CTRL_7B5, 0x0);
      hal_spi_write_reg(phy, RG_TX2DAC_ADDR_7AB, regNum);
      reg_val1 = (tmp_iq_dac_val >> 8) & 0xff;
      reg_val2 = tmp_iq_dac_val & 0xff;
      hal_spi_write_reg(phy, RG_TX2DAC_WDATA_H_7A9, reg_val1);
      hal_spi_write_reg(phy, RG_TX2DAC_WDATA_L_7AA, reg_val2);
      hal_spi_write_reg(phy, RG_TX2DAC_CTRL_7B5, 0x3);
      hal_spi_write_reg(phy, RG_TX2DAC_CTRL_7B5, 0x0);
    } else { /* Q path */
      hal_spi_write_reg(phy, RG_TX2DAC_CTRL_7B5, 0x0);
      hal_spi_write_reg(phy, RG_TX2DAC_ADDR_7AB, regNum);
      reg_val1 = (tmp_iq_dac_val >> 8) & 0xff;
      reg_val2 = tmp_iq_dac_val & 0xff;
      hal_spi_write_reg(phy, RG_TX2DAC_WDATA_H_7A9, reg_val1);
      hal_spi_write_reg(phy, RG_TX2DAC_WDATA_L_7AA, reg_val2);
      hal_spi_write_reg(phy, RG_TX2DAC_CTRL_7B5, 0x5);
      hal_spi_write_reg(phy, RG_TX2DAC_CTRL_7B5, 0x0);
    }
  }
  return;
}

long get_auxadc_p16_value(rf_chip_phy_t* phy, int channel, int flag) {
  long tempVal, sumTemp = 0;
  int i_tempData[MAXCNT];
  int i, cnt, cut_cnt = 8;
  int j          = 0;
  int tmpTestVal = 0x20 | (1 << 7);
  long p16Val    = 0;

  if (channel == TRX_CHN1) {
    tmpTestVal = 0x90;
    sumTemp    = 0;
    my_memset(i_tempData, 0, sizeof(i_tempData));
    if (flag) {
      hal_spi_write_reg(phy, RG_TX1DACI_DC_LV_H_7C4, tmpTestVal);  /// write value 16
      hal_spi_write_reg(phy, RG_TX1DACI_DC_LV_L_7C5, 0x0);
    } else {
      hal_spi_write_reg(phy, RG_TX1DACQ_DC_LV_H_7C2, tmpTestVal);  /// write value  16
      hal_spi_write_reg(phy, RG_TX1DACQ_DC_LV_L_7C3, 0x0);
    }
    hal_spi_write_reg(phy, RG_AUXADC1_865, 0x0);  // latch =0
    CHIP_DELAY(5);                                //50ms
    for (j = 0; j < 16; j++) {
      sumTemp = 0;
      for (i = 0; i < MAXCNT; i++) {
        hal_spi_write_reg(phy, RG_AUXADC1_865, 0x80);  // latch = 1
        tempVal = ((hal_spi_read_reg(phy, RG_AUXADC1_865) & 0x03) << 8) | hal_spi_read_reg(phy, RG_AUXADC1_866);
        if (tempVal & 0x200)
          tempVal = -1 * (1024 - tempVal);
        i_tempData[i] = tempVal;
        sumTemp += tempVal;
        hal_spi_write_reg(phy, RG_AUXADC1_865, 0x0);  // latch =0
      }
      //LOG_MDEBUG(TX_DAC_CAL, "[CH%d] P16 Val: %d, sumTemp = %ld\n", channel+1, tmpTestVal & 0x3F, sumTemp);
      simple_sort(MAXCNT, i_tempData);
      cnt     = MAXCNT - 2 * cut_cnt;
      sumTemp = 0;
      for (i = 1; i <= cnt; i++)
        sumTemp = sumTemp + i_tempData[cut_cnt + i - 1];

      p16Val += sumTemp;
    }
  } else {
    tmpTestVal = 0x90;
    sumTemp    = 0;
    my_memset(i_tempData, 0, sizeof(i_tempData));
    if (flag) {
      hal_spi_write_reg(phy, 0x7D4, tmpTestVal);  /// write value 16
      hal_spi_write_reg(phy, 0x7D5, 0x0);
    } else {
      hal_spi_write_reg(phy, 0x7D2, tmpTestVal);  /// write value  16
      hal_spi_write_reg(phy, 0x7D3, 0x0);
    }

    for (j = 0; j < 16; j++) {
      sumTemp = 0;
      for (i = 0; i < MAXCNT; i++) {
        hal_spi_write_reg(phy, 0x86E, 0x0);   // latch =0
        hal_spi_write_reg(phy, 0x86E, 0x80);  // latch = 1
        tempVal = ((hal_spi_read_reg(phy, 0x86E) & 0x03) << 8) | hal_spi_read_reg(phy, 0x86F);
        if (tempVal & 0x200)
          tempVal = -1 * (1024 - tempVal);
        i_tempData[i] = tempVal;
        sumTemp += tempVal;
        hal_spi_write_reg(phy, 0x86E, 0x0);  // latch =0
      }
      //LOG_MDEBUG(TX_DAC_CAL, "[CH%d] P16 Val: %d, sumTemp = %ld\n", channel+1, tmpTestVal & 0x3F, sumTemp);
      simple_sort(MAXCNT, i_tempData);
      cnt     = MAXCNT - 2 * cut_cnt;
      sumTemp = 0;
      for (i = 1; i <= cnt; i++)
        sumTemp = sumTemp + i_tempData[cut_cnt + i - 1];

      p16Val += sumTemp;
    }
  }
  return p16Val;
}

long get_auxadc_n16_value(rf_chip_phy_t* phy, int channel, int flag) {
  long tempVal, sumTemp = 0;
  int i_tempData[MAXCNT];
  int i, cnt, cut_cnt = 8;
  int j;
  int tmpTestVal = 0x20 | (1 << 5) | (1 << 7);
  int testVal;
  long n16Val = 0;

  if (channel == TRX_CHN1) {
    testVal = 0xB0;  /// write value -16
    sumTemp = 0;
    my_memset(i_tempData, 0, sizeof(i_tempData));
    if (flag) {
      hal_spi_write_reg(phy, RG_TX1DACI_DC_LV_H_7C4, testVal);
      hal_spi_write_reg(phy, RG_TX1DACI_DC_LV_L_7C5, 0x0);
    } else {
      hal_spi_write_reg(phy, RG_TX1DACQ_DC_LV_H_7C2, testVal);
      hal_spi_write_reg(phy, RG_TX1DACQ_DC_LV_L_7C3, 0x0);
    }

    for (j = 0; j < 16; j++) {
      sumTemp = 0;
      for (i = 0; i < MAXCNT; i++) {
        hal_spi_write_reg(phy, RG_AUXADC1_865, 0x0);   // latch =0
        hal_spi_write_reg(phy, RG_AUXADC1_865, 0x80);  // latch = 1
        tempVal = ((hal_spi_read_reg(phy, RG_AUXADC1_865) & 0x03) << 8) | hal_spi_read_reg(phy, RG_AUXADC1_866);
        if (tempVal & 0x200)
          tempVal = -1 * (1024 - tempVal);
        i_tempData[i] = tempVal;
        sumTemp += tempVal;
        hal_spi_write_reg(phy, RG_AUXADC1_865, 0x0);  // latch =0
      }
      //LOG_MDEBUG(TX_DAC_CAL, "[CH%d] N16 Val: %d, sumTemp = %ld\n", channel+1, ((testVal & 0x1F) - 0x20), sumTemp);

      simple_sort(MAXCNT, i_tempData);
      cnt     = MAXCNT - 2 * cut_cnt;
      sumTemp = 0;
      for (i = 1; i <= cnt; i++)
        sumTemp = sumTemp + i_tempData[cut_cnt + i - 1];

      n16Val += sumTemp;
    }
  } else {
    testVal = 0xB0;  /// write value -16
    sumTemp = 0;
    my_memset(i_tempData, 0, sizeof(i_tempData));
    if (flag) {
      hal_spi_write_reg(phy, 0x7D4, testVal);
      hal_spi_write_reg(phy, 0x7D5, 0x0);
    } else {
      hal_spi_write_reg(phy, 0x7D2, testVal);
      hal_spi_write_reg(phy, 0x7D3, 0x0);
    }

    for (j = 0; j < 16; j++) {
      sumTemp = 0;
      for (i = 0; i < MAXCNT; i++) {
        hal_spi_write_reg(phy, 0x86E, 0x0);   // latch =0
        hal_spi_write_reg(phy, 0x86E, 0x80);  // latch = 1
        tempVal = ((hal_spi_read_reg(phy, 0x86E) & 0x03) << 8) | hal_spi_read_reg(phy, 0x86F);
        if (tempVal & 0x200)
          tempVal = -1 * (1024 - tempVal);
        i_tempData[i] = tempVal;
        sumTemp += tempVal;
        hal_spi_write_reg(phy, 0x86E, 0x0);  // latch =0
      }
      //LOG_MDEBUG(TX_DAC_CAL, "[CH%d] N16 Val: %d, sumTemp = %ld\n", channel+1, ((testVal & 0x1F) - 0x20), sumTemp);

      simple_sort(MAXCNT, i_tempData);
      cnt     = MAXCNT - 2 * cut_cnt;
      sumTemp = 0;
      for (i = 1; i <= cnt; i++)
        sumTemp = sumTemp + i_tempData[cut_cnt + i - 1];

      n16Val += sumTemp;
    }
  }

  return n16Val;
}

long get_auxadc_adc0_value(rf_chip_phy_t* phy, int channel, int flag) {
  long i_dac0 = 0;
  int j, n              = 6;
  long tempVal, sumTemp = 0;
  int i_tempData[MAXCNT_1];
  int i, cnt, cut_cnt = 8;

  if (channel == TRX_CHN1) {
    for (j = 0; j < 16; j++) {
      sumTemp = 0;
      my_memset(i_tempData, 0, sizeof(i_tempData));
      if (flag) {
        hal_spi_write_reg(phy, RG_TX1DACI_DC_LV_H_7C4, 0x80);
        hal_spi_write_reg(phy, RG_TX1DACI_DC_LV_L_7C5, 0x0);
      } else {
        hal_spi_write_reg(phy, RG_TX1DACQ_DC_LV_H_7C2, 0x80);
        hal_spi_write_reg(phy, RG_TX1DACQ_DC_LV_L_7C3, 0x0);
      }
      for (i = 0; i < MAXCNT_1; i++) {
        hal_spi_write_reg(phy, RG_AUXADC1_865, 0x0);   // latch =0
        hal_spi_write_reg(phy, RG_AUXADC1_865, 0x80);  // latch = 1
        tempVal = ((hal_spi_read_reg(phy, RG_AUXADC1_865) & 0x03) << 8) | hal_spi_read_reg(phy, RG_AUXADC1_866);
        if (tempVal & 0x200)
          tempVal = -1 * (1024 - tempVal);
        i_tempData[i] = tempVal;
        hal_spi_write_reg(phy, RG_AUXADC1_865, 0x0);  // latch =0
      }

      simple_sort(MAXCNT_1, i_tempData);
      cnt     = MAXCNT_1 - 2 * cut_cnt;
      sumTemp = 0;
      for (i = 1; i <= cnt; i++)
        sumTemp = sumTemp + i_tempData[cut_cnt + i - 1];
      i_dac0 += sumTemp;
    }
  } else {
    for (j = 0; j < 16; j++) {
      sumTemp = 0;
      my_memset(i_tempData, 0, sizeof(i_tempData));
      if (flag) {
        hal_spi_write_reg(phy, 0x7D4, 0x80);
        hal_spi_write_reg(phy, 0x7D5, 0x0);
      } else {
        hal_spi_write_reg(phy, 0x7D2, 0x80);
        hal_spi_write_reg(phy, 0x7D3, 0x0);
      }
      for (i = 0; i < MAXCNT_1; i++) {
        hal_spi_write_reg(phy, 0x86E, 0x0);   // latch =0
        hal_spi_write_reg(phy, 0x86E, 0x80);  // latch = 1
        tempVal = ((hal_spi_read_reg(phy, 0x86E) & 0x03) << 8) | hal_spi_read_reg(phy, 0x86F);
        if (tempVal & 0x200)
          tempVal = -1 * (1024 - tempVal);
        i_tempData[i] = tempVal;
        hal_spi_write_reg(phy, 0x86E, 0x0);  // latch =0
      }

      simple_sort(MAXCNT_1, i_tempData);
      cnt     = MAXCNT_1 - 2 * cut_cnt;
      sumTemp = 0;
      for (i = 1; i <= cnt; i++)
        sumTemp = sumTemp + i_tempData[cut_cnt + i - 1];
      i_dac0 += sumTemp;
    }
  }

  return (i_dac0 * 256 / 255);
}

long get_auxadc_p255_value(rf_chip_phy_t* phy, int channel, int flag) {
  long i_dac_lsb_p255 = 0;
  int j, n              = 6;
  long tempVal, sumTemp = 0;
  int i_tempData[MAXCNT_1];
  int i, cnt, cut_cnt = 8;

  if (channel == TRX_CHN1) {
    for (j = 0; j < 16; j++) {
      sumTemp = 0;
      my_memset(i_tempData, 0, sizeof(i_tempData));
      if (flag) {
        hal_spi_write_reg(phy, RG_TX1DACI_DC_LV_H_7C4, 0x80);
        hal_spi_write_reg(phy, RG_TX1DACI_DC_LV_L_7C5, 0xFF);
      } else {
        hal_spi_write_reg(phy, RG_TX1DACQ_DC_LV_H_7C2, 0x80);
        hal_spi_write_reg(phy, RG_TX1DACQ_DC_LV_L_7C3, 0xFF);
      }
      for (i = 0; i < MAXCNT_1; i++) {
        hal_spi_write_reg(phy, RG_AUXADC1_865, 0x0);   // latch =0
        hal_spi_write_reg(phy, RG_AUXADC1_865, 0x80);  // latch = 1
        tempVal = ((hal_spi_read_reg(phy, RG_AUXADC1_865) & 0x03) << 8) | hal_spi_read_reg(phy, RG_AUXADC1_866);
        if (tempVal & 0x200)
          tempVal = -1 * (1024 - tempVal);
        i_tempData[i] = tempVal;
        hal_spi_write_reg(phy, RG_AUXADC1_865, 0x0);  // latch =0
      }

      simple_sort(MAXCNT_1, i_tempData);
      cnt     = MAXCNT_1 - 2 * cut_cnt;
      sumTemp = 0;
      for (i = 1; i <= cnt; i++)
        sumTemp = sumTemp + i_tempData[cut_cnt + i - 1];
      i_dac_lsb_p255 += sumTemp;
    }
  } else {
    for (j = 0; j < 16; j++) {
      sumTemp = 0;
      my_memset(i_tempData, 0, sizeof(i_tempData));
      if (flag) {
        hal_spi_write_reg(phy, 0x7D4, 0x80);
        hal_spi_write_reg(phy, 0x7D5, 0xFF);
      } else {
        hal_spi_write_reg(phy, 0x7D2, 0x80);
        hal_spi_write_reg(phy, 0x7D3, 0xFF);
      }
      for (i = 0; i < MAXCNT_1; i++) {
        hal_spi_write_reg(phy, 0x86E, 0x0);   // latch =0
        hal_spi_write_reg(phy, 0x86E, 0x80);  // latch = 1
        tempVal = ((hal_spi_read_reg(phy, 0x86E) & 0x03) << 8) | hal_spi_read_reg(phy, 0x86F);
        if (tempVal & 0x200)
          tempVal = -1 * (1024 - tempVal);
        i_tempData[i] = tempVal;
        hal_spi_write_reg(phy, 0x86E, 0x0);  // latch =0
      }

      simple_sort(MAXCNT_1, i_tempData);
      cnt     = MAXCNT_1 - 2 * cut_cnt;
      sumTemp = 0;
      for (i = 1; i <= cnt; i++)
        sumTemp = sumTemp + i_tempData[cut_cnt + i - 1];
      i_dac_lsb_p255 += sumTemp;
    }
  }

  return (i_dac_lsb_p255 * 256 / 255);
}

void tx_dac_cal_n_with_auxadc(rf_chip_phy_t* phy, int channel) {
  int i = 0;
  int tempVal, sumTemp = 0;
  long i_dac_msb_p16  = 0;
  long i_dac_msb_n16  = 0;
  long i_dac_msb_unit = 0;
  long i_dac0         = 0;
  long i_dac_lsb_p255 = 0;
  long i_dac_lsb_tot  = 0;
  int i_tempData[MAXCNT_1];
  int cnt, cut_cnt = 8;
  int j, n         = 30;
  int regNum;
  unsigned int tmp_iq_dac_val;
  int flag           = 0;
  int range          = 1;
  int iqPath         = 0;
  int ChannelOffset  = (channel == TRX_CHN1) ? 0x0 : 0x13;
  int dacCal_min_val = 0;
  int dacCal_max_val = 0x3F;
  int dacCal_mid_val;
  int k;
  unsigned char auxadc1_config_val[10] = {0};
  unsigned char r860_v, r843_v, r83A_v, r833_v, r738_v, r7C4_v, r715_v, r716_v, r717_v, r718_v, r7C5_v;
  unsigned char r7C2_v, r7C3_v;
  unsigned int auxadc1_config[2][10] =
      {
          {0x803, 0x805, 0x600, 0x6f1, 0x861, 0x861, 0x861, 0x861, 0x861, 0x861},
          {0x0F, 0x3F, 0x03, 0x1B, 0x30, 0x00, 0x00, 0x00, 0x30, 0x31}};

  unsigned char auxadc2_config_val[10] = {0};
  unsigned char r834_v, r780_v, r7D4_v, r7D5_v, r75D_v, r75E_v, r75F_v, r760_v;
  unsigned char r7D2_v, r7D3_v;
  unsigned int auxadc2_config[2][10] =
      {
          {0x803, 0x805, 0x600, 0x6f1, 0x86A, 0x86A, 0x86A, 0x86A, 0x86A, 0x86A},
          {0x0F, 0x3F, 0x03, 0x1B, 0x30, 0x00, 0x00, 0x00, 0x30, 0x31}};

  regNum = 64;
  if (TRX_CHN1 == channel) {
    HAL_CONFIG_REGS(phy, G_REG_ARRAY_tx1_dc, G_REG_ARRAY_tx1_dc_LEN);
    for (i = 0; i < 10; i++) {
      auxadc1_config_val[i] = hal_spi_read_reg(phy, auxadc1_config[0][i]);
      hal_spi_write_reg(phy, auxadc1_config[0][i], auxadc1_config[1][i]);
    }

    // Q DC data enable
    r7C2_v = hal_spi_read_reg(phy, RG_TX1DACQ_DC_LV_H_7C2);
    hal_spi_write_reg(phy, RG_TX1DACQ_DC_LV_H_7C2, hal_spi_read_reg(phy, RG_TX1DACQ_DC_LV_H_7C2) | 0x80);

    r7C4_v = hal_spi_read_reg(phy, RG_TX1DACI_DC_LV_H_7C4);
    hal_spi_write_reg(phy, RG_TX1DACI_DC_LV_H_7C4, hal_spi_read_reg(phy, RG_TX1DACI_DC_LV_H_7C4) | 0x80);

    r833_v = hal_spi_read_reg(phy, RG_TEST_TX1ABB_DC_SEL_833);

    // TX1ABB_RR
    r738_v = hal_spi_read_reg(phy, RG_TX1_738);
    hal_spi_write_reg(phy, RG_TX1_738, 0xFF);

/// BB Gain Index 7
#if 0
		printk("BB Gain Index: 7\n");
		r715_v = hal_spi_read_reg(phy, RG_TX1_715);
		hal_spi_write_reg(phy, RG_TX1_715,	0x0);

		r716_v = hal_spi_read_reg(phy, RG_TX1_716);
		hal_spi_write_reg(phy, RG_TX1_716,	0xC0);

		r717_v = hal_spi_read_reg(phy, RG_TX1_717);
		hal_spi_write_reg(phy, RG_TX1_717,	0x1);

		r718_v = hal_spi_read_reg(phy, RG_TX1_718);
		hal_spi_write_reg(phy, RG_TX1_718,	0x04);
#else
    LOG_MDEBUG(phy, TX_DAC_CAL, "BB Gain Index: 1\n");
    r715_v = hal_spi_read_reg(phy, RG_TX1_715);
    hal_spi_write_reg(phy, RG_TX1_715, 0x0);

    r716_v = hal_spi_read_reg(phy, RG_TX1_716);
    hal_spi_write_reg(phy, RG_TX1_716, 0xF0);

    r717_v = hal_spi_read_reg(phy, RG_TX1_717);
    hal_spi_write_reg(phy, RG_TX1_717, 0x0);

    r718_v = hal_spi_read_reg(phy, RG_TX1_718);
    hal_spi_write_reg(phy, RG_TX1_718, 0x18);
#endif

    ////////////////////////////////////////////////////////////////////////
    for (i = 0; i <= 1; i++) {
      n              = 30;
      flag           = 0;
      dacCal_min_val = 0;
      dacCal_max_val = 0x40;

      if (i == 0) {
        hal_spi_write_reg(phy, RG_TEST_TX1ABB_DC_SEL_833, 0x00);  ///  Q :  0x0	I: 0x2
                                                                  //hal_spi_write_reg(phy, RG_TX1DAC_0_CTRL_790+ChannelOffset,  0x05); // Q
      } else {
        hal_spi_write_reg(phy, RG_TEST_TX1ABB_DC_SEL_833, 0x02);  ///  Q :  0x0	I: 0x2
                                                                  //hal_spi_write_reg(phy, RG_TX1DAC_0_CTRL_790+ChannelOffset,  0x03); // I
      }

#if 0
			Set_TXDAC_n_calc_result(phy, channel, regNum,  i,  0x18);
			CHIP_DELAY(50);//50ms
			for( k = 0;  k < 8; k++)
			{
				tmp_iq_dac_val = 0x20;
				i_dac_msb_p16 = get_auxadc_p16_value(phy, channel, i) ;
				i_dac_msb_n16 = get_auxadc_n16_value(phy, channel, i) ;
				i_dac_msb_unit = ( i_dac_msb_p16 - i_dac_msb_n16)/16;

				i_dac0 = get_auxadc_adc0_value(phy, channel, i);
				i_dac_lsb_p255 = get_auxadc_p255_value(phy, channel, i);
				i_dac_lsb_tot = (i_dac_lsb_p255 - i_dac0) ;
				if(i == 0)
					LOG_MDEBUG(phy, TX_DAC_CAL, "[Q Path]dac_msb_unit = %ld,  dac_lsb_tot = %ld,  lsb_16\n", i_dac_msb_unit , i_dac_lsb_tot);
				else
					LOG_MDEBUG(phy, TX_DAC_CAL, "[I Path]dac_msb_unit = %ld,  dac_lsb_tot = %ld,  lsb_16\n",  i_dac_msb_unit , i_dac_lsb_tot);
			}
			continue;
#endif

#if 1
      tmp_iq_dac_val = 0x20;
      i_dac_msb_p16  = get_auxadc_p16_value(phy, channel, i);
      i_dac_msb_n16  = get_auxadc_n16_value(phy, channel, i);
      i_dac_msb_unit = (i_dac_msb_p16 - i_dac_msb_n16) / 16;

      //
      while (dacCal_min_val <= dacCal_max_val) {
        dacCal_mid_val = (dacCal_min_val + dacCal_max_val) / 2;
        Set_TXDAC_n_calc_result(phy, channel, regNum, i, dacCal_mid_val);
        CHIP_DELAY(50);  //50ms

        i_dac0         = get_auxadc_adc0_value(phy, channel, i);
        i_dac_lsb_p255 = get_auxadc_p255_value(phy, channel, i);
        i_dac_lsb_tot  = (i_dac_lsb_p255 - i_dac0) / 4;
        if (i == 0)
          LOG_MDEBUG(phy, TX_DAC_CAL, "[Q Path]dac_msb_unit = %ld,  dac_lsb_tot = %ld\n", i_dac_msb_unit, i_dac_lsb_tot);
        else
          LOG_MDEBUG(phy, TX_DAC_CAL, "[I Path]dac_msb_unit = %ld,  dac_lsb_tot = %ld\n", i_dac_msb_unit, i_dac_lsb_tot);
        if (i_dac_lsb_tot > i_dac_msb_unit) {
          if (flag == -1)
            break;
          flag           = 1;
          dacCal_max_val = dacCal_mid_val + 1;
        } else if (i_dac_lsb_tot < (i_dac_msb_unit + range)) {
          if (flag == 1)
            break;
          flag           = -1;
          dacCal_min_val = dacCal_mid_val - 1;
        } else
          break;
      }

      ///
      flag = 0;
      for (j = 0; j < 8; j++) {
        Set_TXDAC_n_calc_result(phy, channel, regNum, i, dacCal_mid_val);
        CHIP_DELAY(50);  //50ms

        i_dac0         = get_auxadc_adc0_value(phy, channel, i);
        i_dac_lsb_p255 = get_auxadc_p255_value(phy, channel, i);
        i_dac_lsb_tot  = (i_dac_lsb_p255 - i_dac0) / 4;

        if (i == 0)
          LOG_MDEBUG(phy, TX_DAC_CAL, "[Q Path]dac_msb_unit = %ld,  dac_lsb_tot = %ld\n", i_dac_msb_unit, i_dac_lsb_tot);
        else
          LOG_MDEBUG(phy, TX_DAC_CAL, "[I Path]dac_msb_unit = %ld,  dac_lsb_tot = %ld\n", i_dac_msb_unit, i_dac_lsb_tot);

        if (i_dac_lsb_tot > i_dac_msb_unit) {
          if (flag == -1)
            break;
          flag = 1;
          dacCal_mid_val--;
        } else if (i_dac_lsb_tot < i_dac_msb_unit) {
          if (flag == 1)
            break;
          flag = -1;
          dacCal_mid_val++;
        } else
          break;
      }

#endif
    }
    /////////////////////////	recovery  ///////////////////////////////////////////////
    LOG_MDEBUG(phy, TX_DAC_CAL, "tx_dac_cal  recovery.\n");
    for (i = 0; i < 10; i++)
      hal_spi_write_reg(phy, auxadc1_config[0][i], auxadc1_config_val[i]);

    hal_spi_write_reg(phy, 0x7C2, 0x00);
    hal_spi_write_reg(phy, 0x7C3, 0x00);
    hal_spi_write_reg(phy, 0x7C4, 0x00);
    hal_spi_write_reg(phy, 0x7C5, 0x00);

    hal_spi_write_reg(phy, RG_TEST_TX1ABB_DC_SEL_833, r833_v);

    // TX1ABB_RR
    hal_spi_write_reg(phy, RG_TX1_738, r738_v);
/// BB Gain Index 7
#if 1
    hal_spi_write_reg(phy, RG_TX1_715, r715_v);
    hal_spi_write_reg(phy, RG_TX1_716, r716_v);
    hal_spi_write_reg(phy, RG_TX1_717, r717_v);
    hal_spi_write_reg(phy, RG_TX1_718, r718_v);
#endif
  } else  /// CH2
  {
    HAL_CONFIG_REGS(phy, G_REG_ARRAY_tx2_dc, G_REG_ARRAY_tx2_dc_LEN);
    for (i = 0; i < 10; i++) {
      auxadc2_config_val[i] = hal_spi_read_reg(phy, auxadc2_config[0][i]);
      hal_spi_write_reg(phy, auxadc2_config[0][i], auxadc2_config[1][i]);
    }

    // Q DC data enable
    r7D2_v = hal_spi_read_reg(phy, 0x7D2);
    hal_spi_write_reg(phy, 0x7D2, hal_spi_read_reg(phy, 0x7D2) | 0x80);

    r7D4_v = hal_spi_read_reg(phy, 0x7D4);
    hal_spi_write_reg(phy, 0x7D4, hal_spi_read_reg(phy, 0x7D4) | 0x80);

    r834_v = hal_spi_read_reg(phy, 0x834);

    // TX2ABB_RR
    r780_v = hal_spi_read_reg(phy, 0x780);
    hal_spi_write_reg(phy, 0x780, 0xFF);

/// BB Gain Index 7
#if 0
		printk("BB Gain Index: 7\n");
		r75D_v = hal_spi_read_reg(phy, 0x75D);
		hal_spi_write_reg(phy, 0x75D,  0x0);

		r75E_v = hal_spi_read_reg(phy, 0x75E);
		hal_spi_write_reg(phy, 0x75E,  0xC0);

		r75F_v = hal_spi_read_reg(phy, 0x75F);
		hal_spi_write_reg(phy, 0x75F,  0x1);

		r760_v = hal_spi_read_reg(phy, 0x760);
		hal_spi_write_reg(phy, 0x760,  0x04);
#else
    LOG_MDEBUG(phy, TX_DAC_CAL, "BB Gain Index: 1\n");
    r75D_v = hal_spi_read_reg(phy, 0x75D);
    hal_spi_write_reg(phy, 0x75D, 0x0);

    r75E_v = hal_spi_read_reg(phy, 0x75E);
    hal_spi_write_reg(phy, 0x75E, 0xF0);

    r75F_v = hal_spi_read_reg(phy, 0x75F);
    hal_spi_write_reg(phy, 0x75F, 0x0);

    r760_v = hal_spi_read_reg(phy, 0x760);
    hal_spi_write_reg(phy, 0x760, 0x18);
#endif
    ////////////////////////////////////////////////////////////////////////
    for (i = 0; i <= 1; i++) {
      n              = 30;
      flag           = 0;
      dacCal_min_val = 0;
      dacCal_max_val = 0x40;

      if (i == 0) {
        hal_spi_write_reg(phy, 0x834, 0x00);  ///  Q :  0x0	I: 0x2
                                              //spi_write_reg(RG_TX1DAC_0_CTRL_790+ChannelOffset,  0x05); // Q
      } else {
        hal_spi_write_reg(phy, 0x834, 0x02);  ///  Q :  0x0	I: 0x2
                                              //spi_write_reg(RG_TX1DAC_0_CTRL_790+ChannelOffset,  0x03); // I
      }

#if 0
			for( k = 0; k < 8; k++)
			{
				tmp_iq_dac_val = 0x20;
				Set_TXDAC_n_calc_result(channel, regNum,  i,  0x20);
				CHIP_DELAY(50);//50ms
				i_dac_msb_p16 = get_auxadc_p16_value(channel, i) ;
				i_dac_msb_n16 = get_auxadc_n16_value(channel, i);
				i_dac_msb_unit = ( i_dac_msb_p16 - i_dac_msb_n16)/16;

				i_dac0 = get_auxadc_adc0_value(channel, i);
				i_dac_lsb_p255 = get_auxadc_p255_value(channel, i);
				i_dac_lsb_tot = (i_dac_lsb_p255 - i_dac0)/4 ;
				if(i == 0)
					LOG_MDEBUG(TX_DAC_CAL, "[Q Path]dac_msb_unit = %ld,  dac_lsb_tot = %ld\n", i_dac_msb_unit , i_dac_lsb_tot);
				else
					LOG_MDEBUG(TX_DAC_CAL, "[I Path]dac_msb_unit = %ld,  dac_lsb_tot = %ld\n",  i_dac_msb_unit , i_dac_lsb_tot);
			}
			return ;
#endif

#if 1
      tmp_iq_dac_val = 0x20;
      i_dac_msb_p16  = get_auxadc_p16_value(phy, channel, i);
      i_dac_msb_n16  = get_auxadc_n16_value(phy, channel, i);

      i_dac_msb_unit = (i_dac_msb_p16 - i_dac_msb_n16) / 16;
      //
      while (dacCal_min_val <= dacCal_max_val) {
        dacCal_mid_val = (dacCal_min_val + dacCal_max_val) / 2;
        Set_TXDAC_n_calc_result(phy, channel, regNum, i, dacCal_mid_val);
        CHIP_DELAY(50);  //50ms

        i_dac0         = get_auxadc_adc0_value(phy, channel, i);
        i_dac_lsb_p255 = get_auxadc_p255_value(phy, channel, i);
        i_dac_lsb_tot  = (i_dac_lsb_p255 - i_dac0) / 4;
        if (i == 0)
          LOG_MDEBUG(phy, TX_DAC_CAL, "[Q Path]dac_msb_unit = %ld,  dac_lsb_tot = %ld\n", i_dac_msb_unit, i_dac_lsb_tot);
        else
          LOG_MDEBUG(phy, TX_DAC_CAL, "[I Path]dac_msb_unit = %ld,  dac_lsb_tot = %ld\n", i_dac_msb_unit, i_dac_lsb_tot);
        if (i_dac_lsb_tot > i_dac_msb_unit) {
          if (flag == -1)
            break;
          flag           = 1;
          dacCal_max_val = dacCal_mid_val + 1;
        } else if (i_dac_lsb_tot < (i_dac_msb_unit + range)) {
          if (flag == 1)
            break;
          flag           = -1;
          dacCal_min_val = dacCal_mid_val - 1;
        } else
          break;
      }

      ///
      flag = 0;
      for (j = 0; j < 8; j++) {
        Set_TXDAC_n_calc_result(phy, channel, regNum, i, dacCal_mid_val);
        CHIP_DELAY(50);  //50ms

        i_dac0         = get_auxadc_adc0_value(phy, channel, i);
        i_dac_lsb_p255 = get_auxadc_p255_value(phy, channel, i);
        i_dac_lsb_tot  = (i_dac_lsb_p255 - i_dac0) / 4;

        if (i == 0)
          LOG_MDEBUG(phy, TX_DAC_CAL, "[Q Path]dac_msb_unit = %ld,  dac_lsb_tot = %ld\n", i_dac_msb_unit, i_dac_lsb_tot);
        else
          LOG_MDEBUG(phy, TX_DAC_CAL, "[I Path]dac_msb_unit = %ld,  dac_lsb_tot = %ld\n", i_dac_msb_unit, i_dac_lsb_tot);
        if (i_dac_lsb_tot > i_dac_msb_unit) {
          if (flag == -1)
            break;
          flag = 1;
          dacCal_mid_val--;
        } else if (i_dac_lsb_tot < i_dac_msb_unit) {
          if (flag == 1)
            break;
          flag = -1;
          dacCal_mid_val++;
        } else
          break;
      }
#endif
    }
    /////////////////////////	recovery  ///////////////////////////////////////////////
    LOG_MDEBUG(phy, TX_DAC_CAL, "tx_dac_cal  recovery.\n");
    for (i = 0; i < 10; i++)
      hal_spi_write_reg(phy, auxadc2_config[0][i], auxadc2_config_val[i]);

    hal_spi_write_reg(phy, 0x7D2, 0x00);
    hal_spi_write_reg(phy, 0x7D3, 0x00);
    hal_spi_write_reg(phy, 0x7D4, 0x00);
    hal_spi_write_reg(phy, 0x7D5, 0x00);

    hal_spi_write_reg(phy, 0x834, r834_v);

    // TX2ABB_RR
    hal_spi_write_reg(phy, 0x780, r780_v);

/// BB Gain Index 7
#if 1
    hal_spi_write_reg(phy, 0x75D, r75D_v);
    hal_spi_write_reg(phy, 0x75E, r75E_v);
    hal_spi_write_reg(phy, 0x75F, r75F_v);
    hal_spi_write_reg(phy, 0x760, r760_v);
#endif
  }
  ////////////////////////////////////////////////////////////////////////
}

int get_dac_cal_val(rf_chip_phy_t* phy, int index, int channel, int Q) {
  unsigned int val;
  int offset = (channel) ? 0x13 : 0x00;
  if (!Q) {
    // I
    hal_spi_write_reg(phy, 0x7A2 + offset, 0x00);
    hal_spi_write_reg(phy, 0x798 + offset, index);
    hal_spi_write_reg(phy, 0x7A2 + offset, 0x02);
    if (index == 64)
      val = (hal_spi_read_reg(phy, 0x794 + offset) << 8 | hal_spi_read_reg(phy, 0x795 + offset));
    else
      val = tx_dac_val_cat(hal_spi_read_reg(phy, 0x794 + offset), hal_spi_read_reg(phy, 0x795 + offset));
  } else {
    ///Q
    hal_spi_write_reg(phy, 0x7A2 + offset, 0x00);
    hal_spi_write_reg(phy, 0x798 + offset, index);
    hal_spi_write_reg(phy, 0x7A2 + offset, 0x04);
    if (index == 64)
      val = (hal_spi_read_reg(phy, 0x792 + offset) << 8 | hal_spi_read_reg(phy, 0x793 + offset));
    else
      val = tx_dac_val_cat(hal_spi_read_reg(phy, 0x792 + offset), hal_spi_read_reg(phy, 0x793 + offset));
  }

  return val;
}

int print_cal_val(rf_chip_phy_t* phy, int channel) {
  int i;

  LOG_MDEBUG(phy, TX_DAC_CAL, "[I Path]:\n");
  for (i = 0; i <= 64; i++)
    LOG_MDEBUG(phy, TX_DAC_CAL, "%02d:%d ", i, get_dac_cal_val(phy, i, channel, 0));
  LOG_MDEBUG(phy, TX_DAC_CAL, "\n");

  LOG_MDEBUG(phy, TX_DAC_CAL, "[Q Path:]\n");
  for (i = 0; i <= 64; i++)
    LOG_MDEBUG(phy, TX_DAC_CAL, "%02d:%d ", i, get_dac_cal_val(phy, i, channel, 1));
  LOG_MDEBUG(phy, TX_DAC_CAL, "\n");

  return 0;
}

///  D1  Chip
int tx_dac_cal(rf_chip_phy_t* phy, TRX_CHN_ENUM channel) {
  int i, j, repeat_cnt;
  unsigned int reg_val1, reg_val2, tmp;
  unsigned int tmpVal;
  int reg7c7_val, reg7d7_val;
  int reg790_val, reg7a3_val;
  int reg_604_val, reg_600_val;

  reg_600_val = hal_spi_read_reg(phy, 0x600);
  hal_spi_write_reg(phy, 0x600, reg_600_val | 0x01);

  reg_604_val = hal_spi_read_reg(phy, 0x604);
  hal_spi_write_reg(phy, 0x604, 0x1F);

  my_memset(iq_dac_val, 0, sizeof(iq_dac_val));

  if (channel == TRX_CHN1) {
    LOG_MDEBUG(phy, TX_DAC_CAL, "Start Tx1 I Path: Cal \n");
    reg790_val = hal_spi_read_reg(phy, RG_TX1DAC_0_CTRL_790);
    hal_spi_write_reg(phy, RG_TX1DAC_0_CTRL_790, 0x03);  // I
    CHIP_DELAY(100);

    repeat_cnt = 10;
    do {
      reg7c7_val = hal_spi_read_reg(phy, RG_TX1DAC_26_STA_7C7);
      if (((reg7c7_val >> 2) & 0x3) == 0x3)  // I  bi2 = 1 bit 3 =1
        break;
      CHIP_DELAY(100);
    } while (--repeat_cnt);

    if (repeat_cnt <= 0)
      LOG_ERROR("Tx1 I Path Cal Timeout.\n");

    hal_spi_write_reg(phy, RG_TX1DAC_0_CTRL_790, 0x01);
    LOG_MDEBUG(phy, TX_DAC_CAL, "Start Tx1 Q Path: Cal \n");
    hal_spi_write_reg(phy, RG_TX1DAC_0_CTRL_790, 0x05);  //Q
    CHIP_DELAY(100);

    repeat_cnt = 10;
    do {
      reg7c7_val = hal_spi_read_reg(phy, RG_TX1DAC_26_STA_7C7);
      if (((reg7c7_val >> 6) & 0x3) == 0x3)  // I  bi2 = 1 bit 3 =1
        break;
      CHIP_DELAY(100);
    } while (--repeat_cnt);

    if (repeat_cnt <= 0)
      LOG_ERROR("Tx1 Q Path Cal Timeout.\n");

    CHIP_DELAY(100);
    hal_spi_write_reg(phy, RG_TX1DAC_0_CTRL_790, 0x01);

    phy->tx_dac_cal_flag[channel] = 1;
    for (i = 0; i <= 64; i++) {
      phy->tx_dac_cal[channel][i * 2]     = get_dac_cal_val(phy, i, channel, 0);
      phy->tx_dac_cal[channel][i * 2 + 1] = get_dac_cal_val(phy, i, channel, 1);
      if (i % 4 == 0) {
        LOG_MDEBUG(phy, TX_DAC_CAL, "\n");
        LOG_MDEBUG(phy, TX_DAC_CAL, "[0x%02x,0x%02x] ", phy->tx_dac_cal[channel][i * 2],
                   phy->tx_dac_cal[channel][i * 2 + 1]);
      } else
        LOG_MDEBUG(phy, TX_DAC_CAL, "[0x%02x,0x%02x] ", phy->tx_dac_cal[channel][i * 2],
                   phy->tx_dac_cal[channel][i * 2 + 1]);
    }
    LOG_MDEBUG(phy, TX_DAC_CAL, "\n");
  } else {
    LOG_MDEBUG(phy, TX_DAC_CAL, "Start Tx2 I Path: Cal \n");
    reg7a3_val = hal_spi_read_reg(phy, RG_TX2DAC_0_CTRL_7A3);
    hal_spi_write_reg(phy, RG_TX2DAC_0_CTRL_7A3, reg7a3_val | 0x03);  // I
    CHIP_DELAY(100);

    repeat_cnt = 10;
    do {
      reg7d7_val = hal_spi_read_reg(phy, RG_TX2DAC_26_STA_7D7);
      if (((reg7d7_val >> 2) & 0x3) == 0x3)  // I  bi2 = 1 bit 3 =1
        break;
      CHIP_DELAY(100);
    } while (--repeat_cnt);
    if (repeat_cnt <= 0)
      LOG_ERROR("Tx2 I Path Cal Timeout.\n");

    hal_spi_write_reg(phy, RG_TX2DAC_0_CTRL_7A3, 0x01);
    LOG_MDEBUG(phy, TX_DAC_CAL, "Start Tx2 Q Path: Cal \n");
    hal_spi_write_reg(phy, RG_TX2DAC_0_CTRL_7A3, reg7a3_val | 0x05);  //Q
    CHIP_DELAY(100);

    repeat_cnt = 10;
    do {
      reg7d7_val = hal_spi_read_reg(phy, RG_TX2DAC_26_STA_7D7);
      if (((reg7d7_val >> 6) & 0x3) == 0x3)  // I  bi2 = 1 bit 3 =1
        break;
      CHIP_DELAY(100);
    } while (--repeat_cnt);

    if (repeat_cnt <= 0)
      LOG_ERROR("Tx2 Q Path Cal Timeout.\n");

    CHIP_DELAY(100);

    hal_spi_write_reg(phy, RG_TX2DAC_0_CTRL_7A3, 0x01);

    phy->tx_dac_cal_flag[channel] = 1;
    for (i = 0; i <= 64; i++) {
      phy->tx_dac_cal[channel][i * 2]     = get_dac_cal_val(phy, i, channel, 0);
      phy->tx_dac_cal[channel][i * 2 + 1] = get_dac_cal_val(phy, i, channel, 1);
      if (i % 4 == 0) {
        LOG_MDEBUG(phy, TX_DAC_CAL, "\n");
        LOG_MDEBUG(phy, TX_DAC_CAL, "[0x%02x,0x%02x] ", phy->tx_dac_cal[channel][i * 2],
                   phy->tx_dac_cal[channel][i * 2 + 1]);
      } else
        LOG_MDEBUG(phy, TX_DAC_CAL, "[0x%02x,0x%02x] ", phy->tx_dac_cal[channel][i * 2],
                   phy->tx_dac_cal[channel][i * 2 + 1]);
    }
    LOG_MDEBUG(phy, TX_DAC_CAL, "\n");
  }

  hal_spi_write_reg(phy, 0x604, reg_604_val);
  hal_spi_write_reg(phy, 0x600, reg_600_val);

  if ((phy->config->mode == CH1_TDD) || (phy->config->mode == CH2_TDD) || (phy->config->mode == CH1_CH2_TDD)) {
    if (channel == TRX_CHN1) {
      hal_spi_write_reg(phy, 0x799, 0x60);
      hal_spi_write_reg(phy, 0x737, 0xc0);
      hal_spi_write_reg(phy, 0x738, 0xc0);
      for (i = 0; i < BW_CNT; i++)
        write_lut_byte(phy, TRX_BW_LUT, 0, 0, 3 + 73 * i, 2, 0xc0);
    } else {
      hal_spi_write_reg(phy, 0x7ac, 0x60);
      hal_spi_write_reg(phy, 0x77f, 0xc0);
      hal_spi_write_reg(phy, 0x780, 0xc0);
      for (i = 0; i < BW_CNT; i++)
        write_lut_byte(phy, TRX_BW_LUT, 0, 0, 7 + 73 * i, 3, 0xc0);
    }
  }

  return 0;
}

int tx_dc_offset_cal(rf_chip_phy_t* phy, TRX_CHN_ENUM chn) {
  unsigned short auxadc_regs[2][9] =
      {
          {0x803, 0x805, 0x600, 0x861, 0x861, 0x861, 0x861, 0x861, 0x861},
          {0x803, 0x805, 0x600, 0x86A, 0x86A, 0x86A, 0x86A, 0x86A, 0x86A},
      };
  unsigned char auxadc_config[9] =
      {
          0x0F,
          0x3F,
          0x03,
          0x30,
          0x00,
          0x00,
          0x00,
          0x30,
          0x31,
      };
  unsigned short txabb_op_iqch_regs[2] = {0x833, 0x832};
  unsigned char txabb_op_iqch_config[2][4] =
      {
          {0x03, 0x02, 0x01, 0x00},
          {0x30, 0x20, 0x10, 0x00},
      };
  unsigned short tx_cal_iqch_regs[2][4] =
      {
          {0x730, 0x731, 0x732, 0x733},
          {0x778, 0x779, 0x77A, 0x77B},
      };
  unsigned int tx_cal_iqch_retry[2][4] =
      {
          {1, 3, 1, 3},
          {1, 3, 1, 3},
      };
  unsigned short tx_port[2] = {0x700, 0x748};
  unsigned short tx_iq_dac_dcdata_regs[2][2] =
      {
          {0x7C4, 0x7C2},
          {0x7D4, 0x7D2},
      };
  unsigned short txabb_regs[2][3] =
      {
          {0x705, 0x706, 0x707},
          {0x74D, 0x74E, 0x74F},
      };
  unsigned char uctmp, i, lut_val[4], vals[3];
  short lut_addr, j, k;
  auxadc_ctrl ctrl_bak = {0};
  MT_AUXADC_ENUM mt    = (chn == TRX_CHN1) ? MT1P_AUXADC : MT2P_AUXADC;
  int retry            = 3;

  /* init */
  uctmp = (chn == TRX_CHN1) ? 0 : 1;
  auxadcctrl_backup(phy, mt, &ctrl_bak);
  testmux_clear(phy);
  hal_spi_write_reg(phy, 0x843, 0x00);

  //   printf("ruanbo: tx_dc_offset_cal 1\n");
  //   printf("ruanbo: phy->config->rx_dc_cal_flag: %d\n\r", phy->config->rx_dc_cal_flag);
  if (phy->tx_dc_cal_flag[uctmp]) {
    for (i = 0; i < 4; i++)
      hal_spi_write_reg(phy, tx_cal_iqch_regs[uctmp][i], phy->tx_dc_cal[uctmp][i]);
  } else {
    for (i = 0; i < 3; i++)
      vals[i] = hal_spi_read_reg(phy, txabb_regs[uctmp][i]);

    if (!uctmp) {
      HAL_CONFIG_REGS(phy, G_REG_ARRAY_tx1_dc, G_REG_ARRAY_tx1_dc_LEN);
    } else {
      HAL_CONFIG_REGS(phy, G_REG_ARRAY_tx2_dc, G_REG_ARRAY_tx2_dc_LEN);
    }

    /* auxadc setting */
    for (i = 0; i < 9; i++)
      hal_spi_write_reg(phy, auxadc_regs[uctmp][i], auxadc_config[i]);
    for (i = 0; i < 4; i++)
      hal_spi_write_reg(phy, tx_cal_iqch_regs[uctmp][i], 0);

    // printf("ruanbo: else 1\n");
    // printf("ruanbo: phy->config->rx_dc_cal_flag: %d\n\r", phy->config->rx_dc_cal_flag);
    /* start to calib */
    for (i = 0; i < 4; i++) {
      hal_spi_write_reg(phy, txabb_op_iqch_regs[uctmp], txabb_op_iqch_config[uctmp][i]);
      hal_spi_write_reg(phy, tx_port[uctmp], 0x0);
      //   printf("ruanbo: for index i = %d, write reg:\n", i);
      //   printf("ruanbo: phy->config->rx_dc_cal_flag: %d\n\r", phy->config->rx_dc_cal_flag);

      retry = tx_cal_iqch_retry[uctmp][i]; /* try 3 times at most. */
                                           //   printf("ruanbo: tx_cal_iqch_retry\n");
                                           //   printf("ruanbo: phy->config->rx_dc_cal_flag: %d\n\r", phy->config->rx_dc_cal_flag);
      //   tx_dc_offset_cal_with_auxadc(phy, chn, tx_cal_iqch_regs[uctmp][i], tx_cal_iqch_retry[uctmp][i]);
      if (0 != tx_dc_offset_cal_with_auxadc(phy, chn, tx_cal_iqch_regs[uctmp][i], tx_cal_iqch_retry[uctmp][i])) {
        // printf("ruanbo: if\n");
        // printf("ruanbo: phy->config->rx_dc_cal_flag: %d\n\r", phy->config->rx_dc_cal_flag);
        do {
          /*to reset AuxADC */
          (chn == TRX_CHN1) ? auxadc1_reset(phy) : auxadc2_reset(phy);

          if (0 == tx_dc_offset_cal_with_auxadc(phy, chn, tx_cal_iqch_regs[uctmp][i], tx_cal_iqch_retry[uctmp][i])) {
            break;
          }

          retry--;

        } while (retry > 0);

        if ((retry <= 0) && (i % 2 > 0)) {
          LOG_ERROR("%s REPORTED ERR in line %d tune_reg = 0x%x, chn = %d\n",
                    __FUNCTION__, __LINE__, tx_cal_iqch_regs[uctmp][i], chn);
          hal_spi_write_reg(phy, tx_cal_iqch_regs[uctmp][i], 0x0);
          hal_spi_write_reg(phy, tx_cal_iqch_regs[uctmp][i - 1], 0x0);
        }
      }
      //   printf("ruanbo: for index i: %d\n", i);
      //   printf("ruanbo: phy->config->rx_dc_cal_flag: %d\n\r", phy->config->rx_dc_cal_flag);
    }
    for (i = 0; i < 3; i++)
      hal_spi_write_reg(phy, txabb_regs[uctmp][i], vals[i]);

    phy->tx_dc_cal_flag[uctmp] = 1;
  }

  //   printf("ruanbo: tx_dc_offset_cal 2\n");
  //   printf("ruanbo: phy->config->rx_dc_cal_flag: %d\n\r", phy->config->rx_dc_cal_flag);
  /* read result */
  for (i = 0; i < 4; i++) {
    lut_val[i]               = hal_spi_read_reg(phy, tx_cal_iqch_regs[uctmp][i]);
    phy->tx_dc_cal[uctmp][i] = lut_val[i];
    LOG_MDEBUG(phy, TX_DC_CAL, "%s in line %d cal result [0x%03x] = 0x%02x \n",
               __FUNCTION__, __LINE__, tx_cal_iqch_regs[uctmp][i], phy->tx_dc_cal[uctmp][i]);
  }
  hal_spi_write_reg(phy, tx_iq_dac_dcdata_regs[uctmp][0], 0x0);
  hal_spi_write_reg(phy, tx_iq_dac_dcdata_regs[uctmp][1], 0x0);

  /* update lut */
  for (i = 0; i < 4; i++) {
    for (j = 0; j < 16; j++) {
      if (tx_cal_iqch_regs[uctmp][i] == g_tx_bb_gain_regs[j])
        for (k = 0; k < 50; k++) {
          //g_tx_bb_gain_config[k][j] = lut_val[i];
          phy->tx_bb_gain_config[k][j] = lut_val[i];
          //LOG_MDEBUG(TX_DC_CAL, "phy->tx_bb_gain_config[%d][%d]=0x%x\n", k, j, phy->tx_bb_gain_config[k][j]);
        }
    }
  }

  lut_addr = (chn == TRX_CHN1) ? 1 : 3;
  for (i = 0; i < 50; i++) {
    write_lut_byte(phy, TX_GAIN_LUT, i, tx_cal_iqch_regs[uctmp][0], lut_addr + i * 4, 0, lut_val[0]);
    write_lut_byte(phy, TX_GAIN_LUT, i, tx_cal_iqch_regs[uctmp][1], lut_addr + i * 4, 1, lut_val[1]);
    write_lut_byte(phy, TX_GAIN_LUT, i, tx_cal_iqch_regs[uctmp][2], lut_addr + i * 4, 2, lut_val[2]);
    write_lut_byte(phy, TX_GAIN_LUT, i, tx_cal_iqch_regs[uctmp][3], lut_addr + i * 4, 3, lut_val[3]);
  }

  auxadcctrl_recover(phy, mt, &ctrl_bak);
  return 0;
}

/****************************************************************************************/
// rx adc calibration
/****************************************************************************************/
void rx_adc_ldo_enable(rf_chip_phy_t* phy) {
  unsigned int rg_ldo3_val;
  unsigned int rg_ldo5_val;
  //enable ldo
  rg_ldo3_val = hal_spi_read_reg(phy, RG_LDO_3);
  rg_ldo3_val |= 0x1;
  hal_spi_write_reg(phy, RG_LDO_3, rg_ldo3_val);

  //enable  the ldo of rx1 and rx2
  rg_ldo5_val = hal_spi_read_reg(phy, RG_LDO_5);
  rg_ldo5_val |= 0x24;
  hal_spi_write_reg(phy, RG_LDO_5, rg_ldo5_val);
}

void rx_adc_munual_mode_enable(rf_chip_phy_t* phy) {
  /*
	 * RX1 ADC use the same control signal with RX2 ADC
	 * so Setting RX2 ADC make all the RX ADC under the
	 * manual mode
	 */
  hal_spi_write_reg(phy, DA_RX2ADC_2, 0x80);
  hal_spi_write_reg(phy, DA_RX2ADC_0, 0x0);
  hal_spi_write_reg(phy, DA_RX2ADC_0, 0x30);
  hal_spi_write_reg(phy, DA_RX2ADC_2, 0x80);
  hal_spi_write_reg(phy, DA_RX2ADC_1, 0xee);

  hal_spi_write_reg(phy, CRM_CLK_ICG_0, 0x2);
}

void rx_adc_write_os_cal(rf_chip_phy_t* phy, TRX_CHN_ENUM chnl, TRX_IQ_ENUM iq_index,
                         RX_ADC_SUB_ENUM sub_index, unsigned int val, unsigned int addr) {
  unsigned int rg_rx_adc_op = 0;
  unsigned int rg_rx_iq_op  = 0;
  if (iq_index == TRX_I) {
    //i path
    rg_rx_iq_op = 0x08;

  } else if (iq_index == TRX_Q) {
    // q path
    rg_rx_iq_op = 0x04;

  } else {
    LOG_ERROR("Invalid ip_index:%d\nn", iq_index);
  }

  rg_rx_adc_op = rg_rx_iq_op | (sub_index << 1) | 0x1;
  if (chnl == TRX_CHN1) {
    hal_spi_write_reg(phy, RG_RX1ADC_9, 0x00);
    hal_spi_write_reg(phy, RG_RX1ADC_10, val);
    hal_spi_write_reg(phy, RG_RX1ADC_9, rg_rx_adc_op);

  } else if (chnl == TRX_CHN2) {
    hal_spi_write_reg(phy, RG_RX2ADC_9, 0x00);
    hal_spi_write_reg(phy, RG_RX2ADC_10, val);
    hal_spi_write_reg(phy, RG_RX2ADC_9, rg_rx_adc_op);

  } else {
    LOG_ERROR("Failed to set the chnl:%d\nn", chnl);
  }
}

int rx_adc_get_power(rf_chip_phy_t* phy, TRX_CHN_ENUM chnl, TRX_IQ_ENUM iq_index, RX_ADC_SUB_ENUM sub_index) {
  int adc_out;
  short negative = 0;
  unsigned int sub_addr_reg;
  unsigned int adc_out_high_reg;
  unsigned int adc_out_low_reg;
  unsigned int adc_ctrl_reg;
  unsigned int sub_cs;
  unsigned int sub_sel;

  if (chnl == TRX_CHN1) {
    adc_ctrl_reg = RG_RX1ADC_22;
    if (iq_index == TRX_I) {
      //i path
      sub_cs           = 0x80;
      adc_out_high_reg = RG_RX1ADC_22;
      adc_out_low_reg  = RG_RX1ADC_23;
      if (sub_index == RX_ADC_SUB1) {
        sub_sel      = 0x1;
        sub_addr_reg = RG_RX1ADC_3;

      } else if (sub_index == RX_ADC_SUB2) {
        sub_sel      = 0x2;
        sub_addr_reg = RG_RX1ADC_6;

      } else {
        LOG_ERROR("Failed to set the sub_index:%d\nn", sub_index);
      }

    } else if (iq_index == TRX_Q) {
      // q path
      sub_cs           = 0x40;
      adc_out_high_reg = RG_RX1ADC_24;
      adc_out_low_reg  = RG_RX1ADC_25;
      if (sub_index == RX_ADC_SUB1) {
        sub_sel      = 0x1;
        sub_addr_reg = RG_RX1ADC_3;
      } else if (sub_index == RX_ADC_SUB2) {
        sub_sel      = 0x2;
        sub_addr_reg = RG_RX1ADC_6;

      } else {
        LOG_ERROR("Failed to set the sub_index:%d\nn", sub_index);
      }

    } else {
      LOG_ERROR("Failed to set the ip_index:%d\nn", iq_index);
    }

  } else if (chnl == TRX_CHN2) {
    adc_ctrl_reg = RG_RX2ADC_22;
    if (iq_index == TRX_I) {
      //i path
      //remember I/Q cs
      sub_cs           = 0x80;
      adc_out_high_reg = RG_RX2ADC_22;
      adc_out_low_reg  = RG_RX2ADC_23;
      if (sub_index == RX_ADC_SUB1) {
        sub_sel      = 0x4;
        sub_addr_reg = RG_RX2ADC_3;
      } else if (sub_index == RX_ADC_SUB2) {
        sub_sel      = 0x8;
        sub_addr_reg = RG_RX2ADC_6;

      } else {
        LOG_ERROR("Failed to set the sub_index:%d\nn", sub_index);
      }
    } else if (iq_index == TRX_Q) {
      // q path
      //remember I/Q cs
      sub_cs           = 0x40;
      adc_out_high_reg = RG_RX2ADC_24;
      adc_out_low_reg  = RG_RX2ADC_25;
      if (sub_index == RX_ADC_SUB1) {
        sub_sel      = 0x4;
        sub_addr_reg = RG_RX2ADC_3;
      } else if (sub_index == RX_ADC_SUB2) {
        sub_sel      = 0x8;
        sub_addr_reg = RG_RX2ADC_6;
      } else {
        LOG_ERROR("Failed to set the sub_index:%d\nn", sub_index);
      }

    } else {
      LOG_ERROR("Failed to set the ip_index:%d\nn", iq_index);
    }

  } else {
    LOG_ERROR("Failed to set the chnl:%d\nn", chnl);
  }

  hal_spi_write_reg(phy, CRM_CLK_ICG_0, 0xF2);
  hal_spi_write_reg(phy, RG_TEST_19, sub_sel);
  //I/Q CS
  hal_spi_write_reg(phy, sub_addr_reg, sub_cs);

  hal_spi_write_reg(phy, adc_ctrl_reg, 0x00);
  hal_spi_write_reg(phy, adc_ctrl_reg, 0x80);
  CHIP_DELAY(10);
  adc_out = hal_spi_read_reg(phy, adc_out_high_reg);
  if (adc_out & 0x2) {
    negative = 1;
  }
  adc_out = ((adc_out & 0x3) << 8);
  adc_out |= hal_spi_read_reg(phy, adc_out_low_reg);
  if (negative)
    return adc_out - 1024;  //get 10bits complement

  return adc_out;
}

int rx_adc_get_sum_power(rf_chip_phy_t* phy, TRX_CHN_ENUM chnl, TRX_IQ_ENUM iq_index, RX_ADC_SUB_ENUM sub_index, int try_num) {
  int sum_adc_out = 0;
  while (try_num--) {
    sum_adc_out += rx_adc_get_power(phy, chnl, iq_index, sub_index);
  }

  return sum_adc_out;
}

void rx_adc_c10_restore(rf_chip_phy_t* phy, TRX_CHN_ENUM chnl, TRX_IQ_ENUM iq_index, RX_ADC_SUB_ENUM sub_index) {
  if (chnl == TRX_CHN1) {
    if (iq_index == TRX_I) {
      //i path
      if (sub_index == RX_ADC_SUB1) {
        hal_spi_write_reg(phy, 0x699, 0x14);
        hal_spi_write_reg(phy, 0x69a, 0x03);
        hal_spi_write_reg(phy, 0x69b, 0xfa);
        hal_spi_write_reg(phy, 0x699, 0xb4);

        hal_spi_write_reg(phy, 0x699, 0x15);
        hal_spi_write_reg(phy, 0x69a, 0xfc);
        hal_spi_write_reg(phy, 0x69b, 0x06);
        hal_spi_write_reg(phy, 0x699, 0xb5);

        hal_spi_write_reg(phy, 0x699, 0x00);
      } else if (sub_index == RX_ADC_SUB2) {
        hal_spi_write_reg(phy, 0x69c, 0x14);
        hal_spi_write_reg(phy, 0x69d, 0x03);
        hal_spi_write_reg(phy, 0x69e, 0xfa);
        hal_spi_write_reg(phy, 0x69c, 0xb4);

        hal_spi_write_reg(phy, 0x69c, 0x15);
        hal_spi_write_reg(phy, 0x69d, 0xfc);
        hal_spi_write_reg(phy, 0x69e, 0x06);
        hal_spi_write_reg(phy, 0x69c, 0xb5);

        hal_spi_write_reg(phy, 0x69c, 0x00);

      } else {
        LOG_ERROR("Failed to set the sub_index:%d\n", sub_index);
      }
    } else if (iq_index == TRX_Q) {
      // q path
      if (sub_index == RX_ADC_SUB1) {
        hal_spi_write_reg(phy, 0x699, 0x14);
        hal_spi_write_reg(phy, 0x69a, 0x03);
        hal_spi_write_reg(phy, 0x69b, 0xfa);
        hal_spi_write_reg(phy, 0x699, 0x74);

        hal_spi_write_reg(phy, 0x699, 0x15);
        hal_spi_write_reg(phy, 0x69a, 0xfc);
        hal_spi_write_reg(phy, 0x69b, 0x06);
        hal_spi_write_reg(phy, 0x699, 0x75);

        hal_spi_write_reg(phy, 0x699, 0x00);

      } else if (sub_index == RX_ADC_SUB2) {
        hal_spi_write_reg(phy, 0x69c, 0x14);
        hal_spi_write_reg(phy, 0x69d, 0x03);
        hal_spi_write_reg(phy, 0x69e, 0xfa);
        hal_spi_write_reg(phy, 0x69c, 0x74);

        hal_spi_write_reg(phy, 0x69c, 0x15);
        hal_spi_write_reg(phy, 0x69d, 0xfc);
        hal_spi_write_reg(phy, 0x69e, 0x06);
        hal_spi_write_reg(phy, 0x69c, 0x75);

        hal_spi_write_reg(phy, 0x69c, 0x00);

      } else {
        LOG_ERROR("Failed to set the sub_index:%d\n", sub_index);
      }
    } else {
      LOG_ERROR("Failed to set the ip_index:%d\n", iq_index);
    }
  } else if (chnl == TRX_CHN2) {
    if (iq_index == TRX_I) {
      //i path
      if (sub_index == RX_ADC_SUB1) {
        hal_spi_write_reg(phy, 0x6b7, 0x14);
        hal_spi_write_reg(phy, 0x6b8, 0x03);
        hal_spi_write_reg(phy, 0x6b9, 0xfa);
        hal_spi_write_reg(phy, 0x6b7, 0xb4);

        hal_spi_write_reg(phy, 0x6b7, 0x15);
        hal_spi_write_reg(phy, 0x6b8, 0xfc);
        hal_spi_write_reg(phy, 0x6b9, 0x06);
        hal_spi_write_reg(phy, 0x6b7, 0xb5);

        hal_spi_write_reg(phy, 0x6b7, 0x00);
      } else if (sub_index == RX_ADC_SUB2) {
        hal_spi_write_reg(phy, 0x6ba, 0x14);
        hal_spi_write_reg(phy, 0x6bb, 0x03);
        hal_spi_write_reg(phy, 0x6bc, 0xfa);
        hal_spi_write_reg(phy, 0x6ba, 0xb4);

        hal_spi_write_reg(phy, 0x6ba, 0x15);
        hal_spi_write_reg(phy, 0x6bb, 0xfc);
        hal_spi_write_reg(phy, 0x6bc, 0x06);
        hal_spi_write_reg(phy, 0x6ba, 0xb5);

        hal_spi_write_reg(phy, 0x6ba, 0x00);
      } else {
        LOG_ERROR("Failed to set the sub_index:%d\n", sub_index);
      }
    } else if (iq_index == TRX_Q) {
      // q path
      if (sub_index == RX_ADC_SUB1) {
        hal_spi_write_reg(phy, 0x6b7, 0x14);
        hal_spi_write_reg(phy, 0x6b8, 0x03);
        hal_spi_write_reg(phy, 0x6b9, 0xfa);
        hal_spi_write_reg(phy, 0x6b7, 0x74);

        hal_spi_write_reg(phy, 0x6b7, 0x15);
        hal_spi_write_reg(phy, 0x6b8, 0xfc);
        hal_spi_write_reg(phy, 0x6b9, 0x06);
        hal_spi_write_reg(phy, 0x6b7, 0x75);

        hal_spi_write_reg(phy, 0x6b7, 0x00);

      } else if (sub_index == RX_ADC_SUB2) {
        hal_spi_write_reg(phy, 0x6ba, 0x14);
        hal_spi_write_reg(phy, 0x6bb, 0x03);
        hal_spi_write_reg(phy, 0x6bc, 0xfa);
        hal_spi_write_reg(phy, 0x6ba, 0x74);

        hal_spi_write_reg(phy, 0x6ba, 0x15);
        hal_spi_write_reg(phy, 0x6bb, 0xfc);
        hal_spi_write_reg(phy, 0x6bc, 0x06);
        hal_spi_write_reg(phy, 0x6ba, 0x75);

        hal_spi_write_reg(phy, 0x6ba, 0x00);

      } else {
        LOG_ERROR("Failed to set the sub_index:%d\n", sub_index);
      }
    } else {
      LOG_ERROR("Failed to set the ip_index:%d\n", iq_index);
    }

  } else {
    LOG_ERROR("Failed to set the channel:%d\n", chnl);
  }
}

void rx_adc_comp_offst_cal(rf_chip_phy_t* phy, TRX_CHN_ENUM chnl, TRX_IQ_ENUM iq_index) {
  int cal_index;
  unsigned int rg_reg_val;
  unsigned int rx_adc_cal_enable;
  unsigned int sub1adc_cal_code = 0x10;
  unsigned int sub2adc_cal_code = 0x10;
  int sub1_adc_out_sum;
  int sub2_adc_out_sum;
  unsigned int sub1adc_cal_code_wr;
  unsigned int sub2adc_cal_code_wr;

  LOG_MDEBUG(phy, RX_ADC_CAL, "Channel %d RX ADC Comp offst  %s path Cal start\n", chnl, iq_index == TRX_I ? "I" : "Q");
  if (iq_index == TRX_I) {
    //i path
    rx_adc_cal_enable = 0x41;
  } else if (iq_index == TRX_Q) {
    // q path
    rx_adc_cal_enable = 0x21;
  } else {
    LOG_ERROR("Failed to set the ip_index:%d\nn", iq_index);
  }

  if (chnl == TRX_CHN1) {
    //adc maunual cal enable
    rg_reg_val = hal_spi_read_reg(phy, RG_RX1ADC_2);
    rg_reg_val |= rx_adc_cal_enable;
    hal_spi_write_reg(phy, RG_RX1ADC_2, rg_reg_val);

    hal_spi_write_reg(phy, RG_REGS_46, 0x80);
    hal_spi_write_reg(phy, RG_REGS_54, 0x40);

  } else if (chnl == TRX_CHN2) {
    //adc maunual cal enable
    rg_reg_val = hal_spi_read_reg(phy, RG_RX2ADC_2);
    rg_reg_val |= rx_adc_cal_enable;
    hal_spi_write_reg(phy, RG_RX2ADC_2, rg_reg_val);

    hal_spi_write_reg(phy, RG_REGS_46, 0x08);
    hal_spi_write_reg(phy, RG_REGS_55, 0x40);

  } else {
    LOG_ERROR("Failed to set the chnl:%d\nn", chnl);
  }

  for (cal_index = 0; cal_index < 16; cal_index++) {
    sub1adc_cal_code_wr = sub1adc_cal_code & 0x1f;
    sub2adc_cal_code_wr = sub2adc_cal_code & 0x1f;
    rx_adc_write_os_cal(phy, chnl, iq_index, RX_ADC_SUB1, sub1adc_cal_code_wr, 0x16);
    rx_adc_write_os_cal(phy, chnl, iq_index, RX_ADC_SUB2, sub2adc_cal_code_wr, 0x16);

    sub1_adc_out_sum = rx_adc_get_sum_power(phy, chnl, iq_index, RX_ADC_SUB1, 1);
    if (sub1_adc_out_sum < 0) {
      sub1adc_cal_code++;
    } else if (sub1_adc_out_sum > 0) {
      sub1adc_cal_code--;
    } else {
      ;
    }

    sub2_adc_out_sum = rx_adc_get_sum_power(phy, chnl, iq_index, RX_ADC_SUB2, 1);
    if (sub2_adc_out_sum < 0) {
      sub2adc_cal_code++;
    } else if (sub2_adc_out_sum > 0) {
      sub2adc_cal_code--;
    } else {
      ;
    }
    LOG_MDEBUG(phy, RX_ADC_CAL, "cal_index=%d, sub1_adc_out_sum =%d, sub1adc_cal_code=0x%x, sub1adc_cal_code_wr=0x%x,"
                                "sub2_adc_out_sum =%d, sub2adc_cal_code=0x%x, sub2adc_cal_code_wr=0x%x\n",
               cal_index,
               sub1_adc_out_sum,
               sub1adc_cal_code,
               sub1adc_cal_code_wr,
               sub2_adc_out_sum,
               sub2adc_cal_code,
               sub2adc_cal_code_wr);
  }
  LOG_MDEBUG(phy, RX_ADC_CAL, "Channel %d RX ADC Comp offst  %s path Cal end\n", chnl, iq_index == TRX_I ? "I" : "Q");
}

void rx_adc_set_gain_code(rf_chip_phy_t* phy, TRX_CHN_ENUM chnl, TRX_IQ_ENUM iq_index, int gain_code) {
  int gain_code_lsb;
  int gain_code_msb;
  unsigned int rx_adc_sub_ctrl;
  unsigned int rx_adc_sub_data_high;
  unsigned int rx_adc_sub_data_low;
  int bit14 = (gain_code >> 14) & 0x1;

  if (chnl == TRX_CHN1) {
    if (iq_index == TRX_I) {
      //i path
      rx_adc_sub_ctrl      = RG_RX1ADC_3;
      rx_adc_sub_data_high = RG_RX1ADC_4;
      rx_adc_sub_data_low  = RG_RX1ADC_5;
    } else if (iq_index == TRX_Q) {
      // q path
      rx_adc_sub_ctrl      = RG_RX1ADC_6;
      rx_adc_sub_data_high = RG_RX1ADC_7;
      rx_adc_sub_data_low  = RG_RX1ADC_8;
    } else {
      LOG_ERROR("Failed to set the ip_index:%d\nn", iq_index);
    }

  } else if (chnl == TRX_CHN2) {

    if (iq_index == TRX_I) {
      //i path
      //remember I/Q cs
      rx_adc_sub_ctrl      = RG_RX2ADC_3;
      rx_adc_sub_data_high = RG_RX2ADC_4;
      rx_adc_sub_data_low  = RG_RX2ADC_5;

    } else if (iq_index == TRX_Q) {
      // q path
      rx_adc_sub_ctrl      = RG_RX2ADC_6;
      rx_adc_sub_data_high = RG_RX2ADC_7;
      rx_adc_sub_data_low  = RG_RX2ADC_8;

    } else {
      LOG_ERROR("Failed to set the ip_index:%d\nn", iq_index);
    }

  } else {
    LOG_ERROR("Failed to set the chnl:%d\nn", chnl);
  }
  gain_code_lsb = gain_code & 0xff;
  gain_code_msb = bit14 << 7 | bit14 << 6 | ((gain_code >> 8) & 0x3f);
  LOG_MDEBUG(phy, RX_ADC_CAL, "Set gain code, gain_code=0x%0x, gain_code_msb=0x%0x, gain_code_lsb=0x%0x\n",
             gain_code, gain_code_msb, gain_code_lsb);
  hal_spi_write_reg(phy, rx_adc_sub_data_high, gain_code_msb);
  hal_spi_write_reg(phy, rx_adc_sub_data_low, gain_code_lsb);
  hal_spi_write_reg(phy, rx_adc_sub_ctrl, 0x00);
  hal_spi_write_reg(phy, rx_adc_sub_ctrl, 0xb7);
}

int rx_adc_get_gain_code(rf_chip_phy_t* phy, TRX_CHN_ENUM chnl, TRX_IQ_ENUM iq_index) {
  int gain_code = 0;
  int rx_adc_sub1_sum1;
  int rx_adc_sub2_sum1;
  int rx_adc_sub1_sum2;
  int rx_adc_sub2_sum2;

  if (chnl == TRX_CHN1) {
    hal_spi_write_reg(phy, RG_REGS_46, 0x80);
  } else if (chnl == TRX_CHN2) {
    hal_spi_write_reg(phy, RG_REGS_46, 0x08);
  } else {
    LOG_ERROR("Failed to set the chnl:%d\nn", chnl);
    return 0;
  }

  //-0.8v
  if (chnl == TRX_CHN1) {
    hal_spi_write_reg(phy, RG_REGS_54, 0x80);
  } else if (chnl == TRX_CHN2) {
    hal_spi_write_reg(phy, RG_REGS_55, 0x80);
  } else {
    LOG_ERROR("Failed to set the chnl:%d\nn", chnl);
    return 0;
  }
  rx_adc_sub1_sum1 = rx_adc_get_sum_power(phy, chnl, iq_index, RX_ADC_SUB1, 4);
  rx_adc_sub2_sum1 = rx_adc_get_sum_power(phy, chnl, iq_index, RX_ADC_SUB2, 4);

  //0.8v
  if (chnl == TRX_CHN1) {
    hal_spi_write_reg(phy, RG_REGS_54, 0xc0);
  } else if (chnl == TRX_CHN2) {
    hal_spi_write_reg(phy, RG_REGS_55, 0xc0);
  } else {
    LOG_ERROR("Failed to set the chnl:%d\nn", chnl);
    return 0;
  }
  rx_adc_sub1_sum2 = rx_adc_get_sum_power(phy, chnl, iq_index, RX_ADC_SUB1, 4);
  rx_adc_sub2_sum2 = rx_adc_get_sum_power(phy, chnl, iq_index, RX_ADC_SUB2, 4);

  //16384 = 2^14
  if (rx_adc_sub1_sum1 - rx_adc_sub1_sum2) {
    gain_code = 16384 - (16384 * (rx_adc_sub2_sum1 - rx_adc_sub2_sum2)) / (rx_adc_sub1_sum1 - rx_adc_sub1_sum2);
  }
  LOG_MDEBUG(phy, RX_ADC_CAL, "sub1_sum1=%d, sub2_sum1=%d, sub1_sum2=%d, sub2_sum2=%d, gain_code=%d\n",
             rx_adc_sub1_sum1, rx_adc_sub2_sum1, rx_adc_sub1_sum2, rx_adc_sub2_sum2, gain_code);
  return gain_code;
}

void rx_adc_gain_cal(rf_chip_phy_t* phy, TRX_CHN_ENUM chnl, TRX_IQ_ENUM iq_index) {
  int gain_code;
  LOG_MDEBUG(phy, RX_ADC_CAL, "Channel %d RX ADC gain  %s path Cal start\n", chnl, iq_index == TRX_I ? "I" : "Q");

  //only cal the I path firstly
  gain_code = rx_adc_get_gain_code(phy, chnl, iq_index);
  LOG_MDEBUG(phy, RX_ADC_CAL, "Channel %d RX ADC gain  %s path Cal gain_code=%d\n",
             chnl, iq_index == TRX_I ? "I" : "Q", gain_code);
  if (gain_code != 0) {
    rx_adc_set_gain_code(phy, chnl, iq_index, gain_code);
    CHIP_DELAY(100);
    gain_code = rx_adc_get_gain_code(phy, chnl, iq_index);
  }
  LOG_MDEBUG(phy, RX_ADC_CAL, "Channel %d RX ADC gain  %s path Cal end, verify res: gain_code=%d\n",
             chnl, iq_index == TRX_I ? "I" : "Q", gain_code);
}

void rx_adc_cal(rf_chip_phy_t* phy, TRX_CHN_ENUM chnl) {
  //if (phy->config->chip_ver)
  //{
  //	if (chnl==TRX_CHN1)
  //		HAL_CONFIG_REGS(phy, G_REG_ARRAY_for_IIP3_RX1_c10_SHA_ibias, G_REG_ARRAY_for_IIP3_RX1_c10_SHA_ibias_LEN);
  //	else
  //		HAL_CONFIG_REGS(phy, G_REG_ARRAY_for_IIP3_RX2_c10_SHA_ibias, G_REG_ARRAY_for_IIP3_RX2_c10_SHA_ibias_LEN);
  //}
  //return;

  LOG_MDEBUG(phy, RX_ADC_CAL, "Channel %d RX ADC Cal start\n", chnl);
  //reset_adc(chnl);
  //enable rx adc ldo
  rx_adc_ldo_enable(phy);

  //maual mode
  rx_adc_munual_mode_enable(phy);

  //comp offst cal
  //i path
  rx_adc_comp_offst_cal(phy, chnl, TRX_I);
  //q path
  rx_adc_comp_offst_cal(phy, chnl, TRX_Q);

#if 0
	//gain cal
	//i path
	rx_adc_gain_cal(chnl, TRX_I);
	//q path
	rx_adc_gain_cal(chnl, TRX_Q);
#endif

  //restore
  hal_spi_write_reg(phy, RG_TEST_19, 0x00);
  hal_spi_write_reg(phy, RG_RX1ADC_2, 0x61);
  hal_spi_write_reg(phy, RG_RX2ADC_2, 0x61);
  hal_spi_write_reg(phy, RG_REGS_46, 0x00);
  rx_adc_c10_restore(phy, chnl, TRX_I, RX_ADC_SUB1);
  rx_adc_c10_restore(phy, chnl, TRX_I, RX_ADC_SUB2);
  rx_adc_c10_restore(phy, chnl, TRX_Q, RX_ADC_SUB1);
  rx_adc_c10_restore(phy, chnl, TRX_Q, RX_ADC_SUB2);

  LOG_MDEBUG(phy, RX_ADC_CAL, "Channel %d RX ADC Cal end\n", chnl);
}

/****************************************************************************************/
// rx lol cal
/****************************************************************************************/
#if 1
typedef struct rx1rx2_lpbk {
  int rg_846_val;
  int rg_847_val;
} rx1rx2_lpbk_t;

typedef struct sx2rx1_reg {
  int rg_634_val;
  int rg_620_val;
  int rg_64D_val;
  int rg_639_val;
  int rg_650_val;
  int rg_6D1_val;
  int rg_747_val;
  int rg_702_val;
} sx2rx1_reg_t;

typedef struct sx2rx2_reg {
  int rg_634_val;
  int rg_620_val;
  int rg_64D_val;
  int rg_639_val;
  int rg_66E_val;
  int rg_6E1_val;
  int rg_78F_val;
  int rg_74A_val;
} sx2rx2_reg_t;

static long long lol_cal_get_goertzle_power(int channel, rf_chip_phy_t* phy, long long flo, int I_Path) {
  int goertzle_0_i      = 0;
  int goertzle_0_q      = 0;
  int goertzle_1_i      = 0;
  int goertzle_1_q      = 0;
  int goertzle_shift    = 0;
  int goertzle_wb_div32 = 10;

  long long goertzle_0_i_val;
  long long goertzle_0_q_val;
  long long goertzle_1_i_val;
  long long goertzle_1_q_val;
  long long power;

  goertzle_wb_div32 = 1;
  fn_rx_run_goertzle(phy, channel, 0, goertzle_wb_div32, goertzle_wb_div32, 8192, 4,
                     &goertzle_0_i, &goertzle_0_q, &goertzle_1_i, &goertzle_1_q, &goertzle_shift);
  if (I_Path) {
    goertzle_0_i_val = ((long long)goertzle_0_i) << ((long long)goertzle_shift);
    goertzle_0_q_val = ((long long)goertzle_0_q) << ((long long)goertzle_shift);
    goertzle_0_i_val = goertzle_0_i_val >> 8;
    goertzle_0_q_val = goertzle_0_q_val >> 8;
    power            = ((long long)(goertzle_0_i_val * goertzle_0_i_val) +
             (long long)(goertzle_0_q_val * goertzle_0_q_val)) >>
            GOERTZLE_SHIFT_NUM_IMBALANCE;

    return power;
  } else {
    goertzle_1_i_val = ((long long)goertzle_1_i) << ((long long)goertzle_shift);
    goertzle_1_q_val = ((long long)goertzle_1_q) << ((long long)goertzle_shift);
    goertzle_1_i_val = goertzle_1_i_val >> 8;
    goertzle_1_q_val = goertzle_1_q_val >> 8;
    power            = ((long long)(goertzle_1_i_val * goertzle_1_i_val) +
             (long long)(goertzle_1_q_val * goertzle_1_q_val)) >>
            GOERTZLE_SHIFT_NUM_IMBALANCE;
    return power;
  }
}

static void lol_cal_sxtx_to_rx2(rf_chip_phy_t* phy, sx2rx2_reg_t* sx2rx2_regVal) {
  LOG_MDEBUG(phy, RX_DC_CAL, "lol_cal_sxtx_to_rx2\n");
  sx2rx2_regVal->rg_634_val = hal_spi_read_reg(phy, 0x634) & 0xFF;
  sx2rx2_regVal->rg_620_val = hal_spi_read_reg(phy, 0x620) & 0xFF;
  sx2rx2_regVal->rg_64D_val = hal_spi_read_reg(phy, 0x64D) & 0xFF;
  sx2rx2_regVal->rg_639_val = hal_spi_read_reg(phy, 0x639) & 0xFF;
  sx2rx2_regVal->rg_66E_val = hal_spi_read_reg(phy, 0x66E) & 0xFF;
  sx2rx2_regVal->rg_6E1_val = hal_spi_read_reg(phy, 0x6E1) & 0xFF;
  sx2rx2_regVal->rg_78F_val = hal_spi_read_reg(phy, 0x78F) & 0xFF;
  sx2rx2_regVal->rg_74A_val = hal_spi_read_reg(phy, 0x74A) & 0xFF;

  // SXRX TO RX2 OFF
  hal_spi_write_reg(phy, 0x634, 0x04);
  hal_spi_write_reg(phy, 0x620, 0x34);

  // SXTX_Manual EN
  hal_spi_write_reg(phy, 0x64D, 0x04);

  //SXTX to Rx2
  hal_spi_write_reg(phy, 0x639, 0xEC);

  //Diasble RXLO To RX2
  hal_spi_write_reg(phy, 0x66E, 0x01);
  hal_spi_write_reg(phy, 0x6E1, 0x5F);

  // Connect SX to TXLO2
  hal_spi_write_reg(phy, 0x78F, 0x01);
  hal_spi_write_reg(phy, 0x74A, 0x2F);
}

static void lol_cal_sxtx_to_rx2_recovery(rf_chip_phy_t* phy, sx2rx2_reg_t* sx2rx2_regVal) {
  LOG_MDEBUG(phy, RX_DC_CAL, "lol_cal_sxtx_to_rx2_recovery\n");
  hal_spi_write_reg(phy, 0x634, sx2rx2_regVal->rg_634_val);
  hal_spi_write_reg(phy, 0x620, sx2rx2_regVal->rg_620_val);
  hal_spi_write_reg(phy, 0x64D, sx2rx2_regVal->rg_64D_val);
  hal_spi_write_reg(phy, 0x639, sx2rx2_regVal->rg_639_val);
  hal_spi_write_reg(phy, 0x66E, sx2rx2_regVal->rg_66E_val);
  hal_spi_write_reg(phy, 0x6E1, sx2rx2_regVal->rg_6E1_val);
  hal_spi_write_reg(phy, 0x78F, sx2rx2_regVal->rg_78F_val);
  hal_spi_write_reg(phy, 0x74A, sx2rx2_regVal->rg_74A_val);
}

static void lol_cal_sxtx_to_rx1(rf_chip_phy_t* phy, sx2rx1_reg_t* sx2rx1_regVal) {
  LOG_MDEBUG(phy, RX_DC_CAL, "lol_cal_sxtx_to_rx1\n");
  sx2rx1_regVal->rg_634_val = hal_spi_read_reg(phy, 0x634) & 0xFF;
  sx2rx1_regVal->rg_620_val = hal_spi_read_reg(phy, 0x620) & 0xFF;
  sx2rx1_regVal->rg_64D_val = hal_spi_read_reg(phy, 0x64D) & 0xFF;
  sx2rx1_regVal->rg_639_val = hal_spi_read_reg(phy, 0x639) & 0xFF;
  sx2rx1_regVal->rg_650_val = hal_spi_read_reg(phy, 0x650) & 0xFF;
  sx2rx1_regVal->rg_6D1_val = hal_spi_read_reg(phy, 0x6D1) & 0xFF;
  sx2rx1_regVal->rg_747_val = hal_spi_read_reg(phy, 0x747) & 0xFF;
  sx2rx1_regVal->rg_702_val = hal_spi_read_reg(phy, 0x702) & 0xFF;
  // SXRX TO RX1 OFF
  hal_spi_write_reg(phy, 0x634, 0x04);  ///SX RX  En
  hal_spi_write_reg(phy, 0x620, 0x38);

  // SXTX_Manual EN
  hal_spi_write_reg(phy, 0x64D, 0x04);  // SX TX En

  //SXTX to Rx1
  hal_spi_write_reg(phy, 0x639, 0xDC);

  //Diasble RXLO To RX1
  hal_spi_write_reg(phy, 0x650, 0x01);
  hal_spi_write_reg(phy, 0x6D1, 0x5F);

  // Connect SX to TXLO1
  hal_spi_write_reg(phy, 0x747, 0x01);
  hal_spi_write_reg(phy, 0x702, 0x2F);
}

static void lol_cal_sxtx_to_rx1_recovery(rf_chip_phy_t* phy, sx2rx1_reg_t* sx2rx1_regVal) {
  LOG_MDEBUG(phy, RX_DC_CAL, "lol_cal_sxtx_to_rx1_recovery\n");
  hal_spi_write_reg(phy, 0x634, sx2rx1_regVal->rg_634_val);
  hal_spi_write_reg(phy, 0x620, sx2rx1_regVal->rg_620_val);
  hal_spi_write_reg(phy, 0x64D, sx2rx1_regVal->rg_64D_val);
  hal_spi_write_reg(phy, 0x639, sx2rx1_regVal->rg_639_val);
  hal_spi_write_reg(phy, 0x650, sx2rx1_regVal->rg_650_val);
  hal_spi_write_reg(phy, 0x6D1, sx2rx1_regVal->rg_6D1_val);
  hal_spi_write_reg(phy, 0x747, sx2rx1_regVal->rg_747_val);
  hal_spi_write_reg(phy, 0x702, sx2rx1_regVal->rg_702_val);
}

static void lol_cal_rx1rx2_lpbk(rf_chip_phy_t* phy, rx1rx2_lpbk_t* lpbk_val) {
  LOG_MDEBUG(phy, RX_DC_CAL, "lol_cal_rx1rx2_lpbk\n");
  lpbk_val->rg_846_val = hal_spi_read_reg(phy, 0x846) & 0xFF;
  lpbk_val->rg_847_val = hal_spi_read_reg(phy, 0x847) & 0xFF;

  switch (phy->config->rx_port[0]) {
    case MIXAN_SE:
      hal_spi_write_reg(phy, 0x846, 0x01);
      hal_spi_write_reg(phy, 0x847, 0x01);
      break;

    case MIXAP_SE:
      hal_spi_write_reg(phy, 0x846, 0x10);
      hal_spi_write_reg(phy, 0x847, 0x10);
      break;

    case MIXA_DIFF:
      hal_spi_write_reg(phy, 0x846, 0x11);
      hal_spi_write_reg(phy, 0x847, 0x11);
      break;

    case MIXBN_SE:
      hal_spi_write_reg(phy, 0x846, 0x02);
      hal_spi_write_reg(phy, 0x847, 0x02);
      break;

    case MIXBP_SE:
      hal_spi_write_reg(phy, 0x846, 0x20);
      hal_spi_write_reg(phy, 0x847, 0x20);
      break;

    case MIXB_DIFF:
      hal_spi_write_reg(phy, 0x846, 0x22);
      hal_spi_write_reg(phy, 0x847, 0x22);
      break;

    case MIXCN_SE:
      hal_spi_write_reg(phy, 0x846, 0x04);
      hal_spi_write_reg(phy, 0x847, 0x04);
      break;

    case MIXCP_SE:
      hal_spi_write_reg(phy, 0x846, 0x40);
      hal_spi_write_reg(phy, 0x847, 0x40);
      break;

    case MIXC_DIFF:
      hal_spi_write_reg(phy, 0x846, 0x44);
      hal_spi_write_reg(phy, 0x847, 0x44);
      break;

    default:
      LOG_MDEBUG(phy, RX_DC_CAL, "Err. RX Port Don't Support.\n");
      break;
  }
}

static void lol_cal_rx1rx2_lpbk_recovery(rf_chip_phy_t* phy, rx1rx2_lpbk_t* lpbk_val) {
  LOG_MDEBUG(phy, RX_DC_CAL, "lol_cal_rx1rx2_lpbk_recovery\n");
  hal_spi_write_reg(phy, 0x846, lpbk_val->rg_846_val);
  hal_spi_write_reg(phy, 0x847, lpbk_val->rg_847_val);
}

static int lol_cal_I_path(int channel, rf_chip_phy_t* phy, long freq_offset) {
  unsigned int i_left      = 0;
  unsigned int i_right     = 255;
  unsigned int i_middle    = 0;
  unsigned int old_middle  = 0xff;
  unsigned int lol_cal_reg = (channel == TRX_CHN1) ? 0x6D5 : 0x6E5;
  long long I_power1, I_power2;
  int goertzle_power_channel;
  int offset = 4;
  int cal_val;
  int i, j;
  int micr_val;
  long long min_power = 0xFFFFFFFF;
  int min_cal_val     = 0xFF;

  if (channel == TRX_CHN1)
    goertzle_power_channel = TRX_CHN2;
  else
    goertzle_power_channel = TRX_CHN1;

  while (i_left != i_right) {
    i_middle = (i_left + i_right) / 2;
    if (old_middle == i_middle)
      break;
    old_middle = i_middle;

    hal_spi_write_reg(phy, lol_cal_reg, i_middle);
    CHIP_DELAY(1);
    I_power1 = lol_cal_get_goertzle_power(goertzle_power_channel, phy, (long long)freq_offset, 1);

    hal_spi_write_reg(phy, lol_cal_reg, i_middle + offset);
    CHIP_DELAY(1);
    I_power2 = lol_cal_get_goertzle_power(goertzle_power_channel, phy, (long long)freq_offset, 1);
    LOG_MDEBUG(phy, RX_DC_CAL, "[RX%d - I Path] 0x%x = 0x%02x,I_power1 = %lld, 0x%x = 0x%02x,I_power2 = %lld\n",
               goertzle_power_channel + 1, lol_cal_reg, i_middle, I_power1, lol_cal_reg, i_middle + offset, I_power2);

    if (I_power1 == 0)
      return i_middle;

    if (I_power2 == 0)
      return i_middle + offset;

    if (I_power1 > I_power2) {
      hal_spi_write_reg(phy, lol_cal_reg, i_middle + offset);
      i_left  = i_middle + 1;
      i_right = i_right;
    } else if (I_power1 < I_power2) {
      hal_spi_write_reg(phy, lol_cal_reg, i_middle);
      i_left  = i_left;
      i_right = i_middle;
    } else
      break;
  }

  cal_val = hal_spi_read_reg(phy, lol_cal_reg);
  for (i = cal_val - 4; i <= cal_val + 4; i++) {

    hal_spi_write_reg(phy, lol_cal_reg, i);
    CHIP_DELAY(1);
    I_power1 = lol_cal_get_goertzle_power(goertzle_power_channel, phy, (long long)freq_offset, 1);
    LOG_MDEBUG(phy, RX_DC_CAL, "[RX%d - I Path] 0x%x = 0x%02x,I_power = %lld\n",
               goertzle_power_channel + 1, lol_cal_reg, i, I_power1);

    if (I_power1 == 0)
      return i;

    if (I_power1 < min_power) {
      min_power   = I_power1;
      min_cal_val = i;
    }
  }

  if (min_cal_val == (cal_val + 4)) {
    micr_val = min_cal_val;
    while (1) {
      micr_val++;
      hal_spi_write_reg(phy, lol_cal_reg, micr_val);
      CHIP_DELAY(1);

      I_power1 = lol_cal_get_goertzle_power(goertzle_power_channel, phy, (long long)freq_offset, 1);
      LOG_MDEBUG(phy, RX_DC_CAL, "[RX%d - I Path Micr] 0x%x = 0x%02x,I_power = %lld\n",
                 goertzle_power_channel + 1, lol_cal_reg, micr_val, I_power1);

      if (I_power1 == 0)
        return micr_val;

      if (I_power1 < min_power) {
        min_power   = I_power1;
        min_cal_val = micr_val;
      } else
        break;
    }
  }

  hal_spi_write_reg(phy, lol_cal_reg, min_cal_val);

  return hal_spi_read_reg(phy, lol_cal_reg);
}

static int lol_cal_Q_path(int channel, rf_chip_phy_t* phy, long freq_offset) {
  unsigned int i_left      = 0;
  unsigned int i_right     = 255;
  unsigned int i_middle    = 0;
  unsigned int old_middle  = 0xFF;
  unsigned int lol_cal_reg = (channel == TRX_CHN1) ? 0x6D3 : 0x6E3;
  long long I_power1, I_power2;
  int goertzle_power_channel;
  int offset = 4;
  int cal_val;
  int i, j;
  int min_pos;
  int micr_val;
  long long min_power = 0xFFFFFFFF;
  int min_cal_val     = 0xFF;

  if (channel == TRX_CHN1)
    goertzle_power_channel = TRX_CHN2;
  else
    goertzle_power_channel = TRX_CHN1;

  while (i_left != i_right) {
    i_middle = (i_left + i_right) / 2;
    if (old_middle == i_middle)
      break;
    old_middle = i_middle;

    hal_spi_write_reg(phy, lol_cal_reg, i_middle);
    CHIP_DELAY(1);
    I_power1 = lol_cal_get_goertzle_power(goertzle_power_channel, phy, (long long)freq_offset, 0);

    hal_spi_write_reg(phy, lol_cal_reg, i_middle + offset);
    CHIP_DELAY(1);
    I_power2 = lol_cal_get_goertzle_power(goertzle_power_channel, phy, (long long)freq_offset, 0);
    LOG_MDEBUG(phy, RX_DC_CAL, "[RX%d - Q Path] 0x%x = 0x%02x,I_power1 = %lld,0x%x = 0x%02x,I_power2 = %lld\n",
               goertzle_power_channel + 1, lol_cal_reg, i_middle, I_power1, lol_cal_reg, i_middle + offset, I_power2);

    if (I_power1 == 0)
      return i_middle;

    if (I_power2 == 0)
      return i_middle + offset;

    if (I_power1 > I_power2) {
      hal_spi_write_reg(phy, lol_cal_reg, i_middle + offset);
      i_left  = i_middle + 1;
      i_right = i_right;
    } else if (I_power1 < I_power2) {
      hal_spi_write_reg(phy, lol_cal_reg, i_middle);
      i_left  = i_left;
      i_right = i_middle;
    } else
      break;
  }

  cal_val = hal_spi_read_reg(phy, lol_cal_reg);
  for (i = cal_val - 4; i <= cal_val + 4; i++) {
    hal_spi_write_reg(phy, lol_cal_reg, i);
    CHIP_DELAY(1);
    I_power1 = lol_cal_get_goertzle_power(goertzle_power_channel, phy, (long long)freq_offset, 0);
    LOG_MDEBUG(phy, RX_DC_CAL, "[RX%d - Q Path] 0x%x = 0x%02x,I_power = %lld\n",
               goertzle_power_channel + 1, lol_cal_reg, i, I_power1);

    if (I_power1 == 0)
      return i;

    if (I_power1 < min_power) {
      min_power   = I_power1;
      min_cal_val = i;
    }
  }

  if (min_cal_val == (cal_val + 4)) {
    micr_val = min_cal_val;
    while (1) {
      micr_val++;
      hal_spi_write_reg(phy, lol_cal_reg, micr_val);
      CHIP_DELAY(1);

      I_power1 = lol_cal_get_goertzle_power(goertzle_power_channel, phy, (long long)freq_offset, 0);
      LOG_MDEBUG(phy, RX_DC_CAL, "[RX%d - Q Path Micr] 0x%x = 0x%02x,I_power = %lld\n",
                 goertzle_power_channel + 1, lol_cal_reg, micr_val, I_power1);

      if (I_power1 == 0)
        return micr_val;

      if (I_power1 < min_power) {
        min_power   = I_power1;
        min_cal_val = micr_val;
      } else
        break;
    }
  }
  hal_spi_write_reg(phy, lol_cal_reg, min_cal_val);

  return hal_spi_read_reg(phy, lol_cal_reg);
}

int lol_cal_update_all_gain_tia_lut(rf_chip_phy_t* phy, int channel, int* i_val, int* q_val) {
  int i;
  unsigned int addr_reg, i_data_reg, q_data_reg, ctrl_reg;
  unsigned int lut_addr;
  int rx_port_lut_pos[MIX_CNT] = {48, 54, 18, 36, 42, 12, 24, 30, 6};
  int pos;

  if (channel == TRX_CHN1) {
    addr_reg   = 0x102;
    i_data_reg = 0x106;
    q_data_reg = 0x105;
    ctrl_reg   = 0x100;
  } else {
    addr_reg   = 0x101;
    i_data_reg = 0x104;
    q_data_reg = 0x103;
    ctrl_reg   = 0x100;
  }

  pos = rx_port_lut_pos[phy->config->rx_port[channel]];
  for (i = RX_PORT_G0; i < RX_PORT_GAIN_MAX; i++) {
    lut_addr = pos + i;
    /* write addr */
    hal_spi_write_reg(phy, addr_reg, pos + i);

    /* write data */
    hal_spi_write_reg(phy, q_data_reg, q_val[i]);
    hal_spi_write_reg(phy, i_data_reg, i_val[i]);
    LOG_MDEBUG(phy, RX_DC_CAL, "q_val[%d] = 0x%02x, i_val[%d] = 0x%02x.\n", i, q_val[i], i, i_val[i]);

    /* pulse write */
    if (channel == TRX_CHN1) {
      hal_spi_write_reg(phy, ctrl_reg, 0x40);
      hal_spi_write_reg(phy, ctrl_reg, 0x00);
    } else {
      hal_spi_write_reg(phy, ctrl_reg, 0x80);
      hal_spi_write_reg(phy, ctrl_reg, 0x00);
    }
  }

  return 0;
}

int g_reg_6D5_val[RX_PORT_GAIN_MAX], g_reg_6D3_val[RX_PORT_GAIN_MAX];
int g_reg_6E5_val[RX_PORT_GAIN_MAX], g_reg_6E3_val[RX_PORT_GAIN_MAX];
int lo_leakage_cal(rf_chip_phy_t* phy, int channel) {
  unsigned long long freq_RXLO, freq_TXLO, freq_offset;
  unsigned long long tx_flo_bak, rx_flo_bak;
  int i;
  rx1rx2_lpbk_t rx12_regVal;
  sx2rx1_reg_t sx2rx1_regVal;
  sx2rx2_reg_t sx2rx2_regVal;

  tx_flo_bak = phy->config->tx_flo;
  rx_flo_bak = phy->config->rx_flo;

  freq_offset         = 960 * 1 * 1000;
  freq_RXLO           = rx_flo_bak;
  freq_TXLO           = rx_flo_bak - freq_offset;
  phy->config->tx_flo = freq_TXLO;
  phy->config->rx_flo = freq_RXLO;

  lol_cal_rx1rx2_lpbk(phy, &rx12_regVal);
  CHIP_DELAY(1);
  hal_spi_write_reg(phy, 0x650, 0x01);
  hal_spi_write_reg(phy, 0x65C, 0x00);
  hal_spi_write_reg(phy, 0x6D3, 0x80);
  hal_spi_write_reg(phy, 0x6D4, 0x80);
  hal_spi_write_reg(phy, 0x6D5, 0x80);
  hal_spi_write_reg(phy, 0x6D6, 0x80);

  hal_spi_write_reg(phy, 0x66E, 0x01);
  hal_spi_write_reg(phy, 0x67A, 0x00);
  hal_spi_write_reg(phy, 0x6E3, 0x80);
  hal_spi_write_reg(phy, 0x6E4, 0x80);
  hal_spi_write_reg(phy, 0x6E5, 0x80);
  hal_spi_write_reg(phy, 0x6E6, 0x80);

  for (i = RX_PORT_G0; i < RX_PORT_GAIN_MAX; i++) {
    if (channel == TRX_CHN1) {
      LOG_MDEBUG(phy, RX_DC_CAL, "set_rx_port_man: G%d\n", i);

      if (RX_PORT_G1 == i) {
        g_reg_6D5_val[i] = g_reg_6D5_val[RX_PORT_G0];
        g_reg_6D3_val[i] = g_reg_6D3_val[RX_PORT_G0];
        continue;
      }

      if ((RX_PORT_G3 == i) || (RX_PORT_G4 == i) || (RX_PORT_G5 == i)) {
        g_reg_6D5_val[i] = g_reg_6D5_val[RX_PORT_G2];
        g_reg_6D3_val[i] = g_reg_6D3_val[RX_PORT_G2];
        continue;
      }

      set_rx_port_man(phy, channel, phy->config->rx_port[channel], i);
      set_rx_port_man(phy, TRX_CHN2, phy->config->rx_port[TRX_CHN2], RX_PORT_G0);
      CHIP_DELAY(1);
      lol_cal_sxtx_to_rx2(phy, &sx2rx2_regVal);
      CHIP_DELAY(1);

      //sxtx_band_fsm(phy, phy->config->tx_flo, 0);
      set_trx_lo(phy, TX_DIR, TRX_CHN2, phy->config->tx_flo);

      CHIP_DELAY(1);

      g_reg_6D5_val[i] = lol_cal_I_path(TRX_CHN1, phy, freq_offset);
      g_reg_6D3_val[i] = lol_cal_Q_path(TRX_CHN1, phy, freq_offset);

      lol_cal_sxtx_to_rx2_recovery(phy, &sx2rx2_regVal);

    } else {
      LOG_MDEBUG(phy, RX_DC_CAL, "set_rx_port_man: G%d\n", i);

      if (RX_PORT_G1 == i) {
        g_reg_6E5_val[i] = g_reg_6E5_val[RX_PORT_G0];
        g_reg_6E3_val[i] = g_reg_6E3_val[RX_PORT_G0];
        continue;
      }

      if ((RX_PORT_G3 == i) || (RX_PORT_G4 == i) || (RX_PORT_G5 == i)) {
        g_reg_6E5_val[i] = g_reg_6E5_val[RX_PORT_G2];
        g_reg_6E3_val[i] = g_reg_6E3_val[RX_PORT_G2];
        continue;
      }

      set_rx_port_man(phy, channel, phy->config->rx_port[channel], i);
      set_rx_port_man(phy, TRX_CHN1, phy->config->rx_port[TRX_CHN1], RX_PORT_G0);

      CHIP_DELAY(1);
      lol_cal_sxtx_to_rx1(phy, &sx2rx1_regVal);
      //sxtx_band_fsm(phy, phy->config->tx_flo, 0);

      set_trx_lo(phy, TX_DIR, TRX_CHN1, phy->config->tx_flo);

      CHIP_DELAY(1);

      g_reg_6E5_val[i] = lol_cal_I_path(TRX_CHN2, phy, freq_offset);
      g_reg_6E3_val[i] = lol_cal_Q_path(TRX_CHN2, phy, freq_offset);

      lol_cal_sxtx_to_rx1_recovery(phy, &sx2rx1_regVal);
    }
  }
  lol_cal_rx1rx2_lpbk_recovery(phy, &rx12_regVal);
  phy->config->tx_flo = tx_flo_bak;
  phy->config->rx_flo = rx_flo_bak;

  if (channel == TRX_CHN1)
    lol_cal_update_all_gain_tia_lut(phy, channel, g_reg_6D5_val, g_reg_6D3_val);
  else
    lol_cal_update_all_gain_tia_lut(phy, channel, g_reg_6E5_val, g_reg_6E3_val);
  return 0;
}

#endif

/****************************************************************************************/
// rx dc calibration
/****************************************************************************************/
int rx_dc_offset_cal_setting(rf_chip_phy_t* phy, int channel, RX_PORT_ENUM rx_port,
                             RXFE_GAIN_ENUM rxfe_gain_index, int rx_lpf_gain_index) {
  //set the value rxfe rxfe_gain_index  rxbbf rx_lpf_gain_index

  if (rxfe_gain_index > RX_PORT_G5)
    return -1;

  if (rx_lpf_gain_index > 12)
    return -1;

  if (phy->rx_dc_cal_flag[channel]) {
    set_rx_port_man(phy, channel, rx_port, rxfe_gain_index);

    if (channel) {
      hal_spi_write_reg(phy, 0x65C, rx_lpf_gain_index);

      hal_spi_write_reg(phy, 0x6D6, phy->bq_rx2[rxfe_gain_index][rx_lpf_gain_index * 2]);
      hal_spi_write_reg(phy, 0x6D4, phy->bq_rx2[rxfe_gain_index][rx_lpf_gain_index * 2 + 1]);
    } else {
      hal_spi_write_reg(phy, 0x67A, rx_lpf_gain_index);

      hal_spi_write_reg(phy, 0x6E6, phy->bq_rx1[rxfe_gain_index][rx_lpf_gain_index * 2]);
      hal_spi_write_reg(phy, 0x6E4, phy->bq_rx1[rxfe_gain_index][rx_lpf_gain_index * 2 + 1]);
    }
  } else {
    LOG_ERROR("rx dc cal flag failed!\n");
    return -1;
  }

  return 0;
}

void reset_adc(rf_chip_phy_t* phy, TRX_CHN_ENUM chn) {
  CHIP_DELAY(15);
  if (chn == TRX_CHN1) {
    hal_spi_write_reg(phy, 0x698, 0x80);
    CHIP_DELAY(15);
    hal_spi_write_reg(phy, 0x697, 0x00);
    CHIP_DELAY(15);
    hal_spi_write_reg(phy, 0x696, 0x00);
    CHIP_DELAY(15);
    hal_spi_write_reg(phy, 0x696, 0x30);
    CHIP_DELAY(15);
    hal_spi_write_reg(phy, 0x697, 0xee);
  } else {
    hal_spi_write_reg(phy, 0x6b6, 0x80);
    CHIP_DELAY(15);
    hal_spi_write_reg(phy, 0x6b5, 0x00);
    CHIP_DELAY(15);
    hal_spi_write_reg(phy, 0x6b4, 0x00);
    CHIP_DELAY(15);
    hal_spi_write_reg(phy, 0x6b4, 0x30);
    CHIP_DELAY(15);
    hal_spi_write_reg(phy, 0x6b5, 0xee);
  }
}

void rx_dc_offset_cal_enable(rf_chip_phy_t* phy, TRX_CHN_ENUM chn, unsigned char enable) {
  int tmp;

  if (enable) {
    if (chn == TRX_CHN1) {
      hal_spi_write_reg(phy, 0x04B, 0xE1);
      tmp = hal_spi_read_reg(phy, 0x052);
      tmp = tmp & 0xE3;
      hal_spi_write_reg(phy, 0x052, tmp);
    } else {
      hal_spi_write_reg(phy, 0x04B, 0xE2);
      tmp = hal_spi_read_reg(phy, 0x052);
      tmp = tmp | (0x7 << 2);
      hal_spi_write_reg(phy, 0x052, tmp);
    }
  } else {
    hal_spi_write_reg(phy, 0x04B, 0x00);
    tmp = hal_spi_read_reg(phy, 0x052);
    tmp = tmp & 0xE3;
    hal_spi_write_reg(phy, 0x052, tmp);
  }
}

short rx_dc_offset_cal_status(rf_chip_phy_t* phy, TRX_CHN_ENUM chn) {
  unsigned char reg_val;
  unsigned short cal_valid_reg;
  unsigned char i, try
    ;

  try
    = 8;
  cal_valid_reg = 0x04D;
  for (i = 0; i < try; i++) {
    reg_val = 0;
    if (chn == TRX_CHN1)
      reg_val = HAL_REG_GET_BIT(phy, cal_valid_reg, BIT0);
    else
      reg_val = HAL_REG_GET_BIT(phy, cal_valid_reg, BIT1);

    if (reg_val)
      return 1;

    //CHIP_UDELAY(10);//1ms
  }

  return 0;
}

void rx_dc_offset_cal_enable_ext(rf_chip_phy_t* phy, TRX_CHN_ENUM channel_sel, unsigned char ena) {
  unsigned short w_addr;
  unsigned short w_data;
  unsigned short r_addr;
  unsigned short r_data;

  if (ena == 0) {
    r_addr = RX_DC_CTRL_0;
    r_data = hal_spi_read_reg(phy, r_addr);

    w_addr = r_addr;
    if (channel_sel == TRX_CHN1) {
      w_data = (r_data & 0xfa);
    } else {
      w_data = (r_data & 0xf5);
    }
    hal_spi_write_reg(phy, w_addr, w_data);
  } else {
    if (channel_sel == TRX_CHN1) {
      r_addr = RX_DC_CTRL_0;
      r_data = hal_spi_read_reg(phy, r_addr);

      w_addr = r_addr;
      w_data = (r_data & 0x1a) | (0xe5);
      hal_spi_write_reg(phy, w_addr, w_data);
    } else {
      r_addr = RX2I_DC_OFST_1;
      r_data = hal_spi_read_reg(phy, r_addr);
      w_addr = r_addr;
      w_data = (r_data & 0xe3) | (0x1c);
      hal_spi_write_reg(phy, w_addr, w_data);

      r_addr = RX_DC_CTRL_0;
      r_data = hal_spi_read_reg(phy, r_addr);
      w_addr = r_addr;
      w_data = (r_data & 0xf5) | (0xa);
      hal_spi_write_reg(phy, w_addr, w_data);
    }
  }
}

short rx_get_adc_offset(rf_chip_phy_t* phy, TRX_CHN_ENUM channel_sel, char try_num, int* dc_offset_i, int* dc_offset_q) {
  unsigned int r_addr;
  unsigned int r_data;
  unsigned int flg_vld = 0;
  short i, retry_read = 20;
  unsigned int high_bits = 0;
  unsigned int low_bits  = 0;
  int dc_val             = 0;
  int mean_i             = 0;
  int mean_q             = 0;

  for (i = 0; i < try_num; i++) {
    rx_dc_offset_cal_enable_ext(phy, channel_sel, 0);
    rx_dc_offset_cal_enable_ext(phy, channel_sel, 1);
    r_addr  = RX_DC_PWR_STATE_0;
    flg_vld = 0;
    do {
      r_data  = hal_spi_read_reg(phy, r_addr);
      flg_vld = (channel_sel == TRX_CHN1) ? (r_data & 0x1) : ((r_data >> 1) & 0x1);

      //CHIP_DELAY(1);
      if (retry_read < 0) {
        LOG_ERROR("rx get adc offset failed!\n");
        rx_dc_offset_cal_enable_ext(phy, channel_sel, 0);
        return -1;
      }
      retry_read--;
    } while (flg_vld == 0);

    r_addr    = (channel_sel == TRX_CHN1) ? RX1I_DC_OFST_1 : RX2I_DC_OFST_1;
    r_data    = hal_spi_read_reg(phy, r_addr);
    high_bits = r_data & 0x3;

    r_addr   = (channel_sel == TRX_CHN1) ? RX1I_DC_OFST_0 : RX2I_DC_OFST_0;
    r_data   = hal_spi_read_reg(phy, r_addr);
    low_bits = r_data;

    dc_val = (high_bits << 8) + low_bits;
    dc_val = (dc_val >= 512) ? (dc_val - 1024) : dc_val;
    mean_i = mean_i + dc_val;

    r_addr    = (channel_sel == TRX_CHN1) ? RX1Q_DC_OFST_1 : RX2Q_DC_OFST_1;
    r_data    = hal_spi_read_reg(phy, r_addr);
    high_bits = r_data & 0x3;

    r_addr   = (channel_sel == TRX_CHN1) ? RX1Q_DC_OFST_0 : RX2Q_DC_OFST_0;
    r_data   = hal_spi_read_reg(phy, r_addr);
    low_bits = r_data;

    dc_val = (high_bits << 8) + low_bits;
    dc_val = (dc_val >= 512) ? (dc_val - 1024) : dc_val;
    mean_q = mean_q + dc_val;
  }

  rx_dc_offset_cal_enable_ext(phy, channel_sel, 0);
  *dc_offset_i = mean_i;
  *dc_offset_q = mean_q;

  return 0;
}

int rx_get_adc_power(rf_chip_phy_t* phy, int channel_sel, int try_num, int* mean_power) {
  unsigned int w_addr;
  unsigned int w_data;
  unsigned int r_addr;
  unsigned int r_data;
  unsigned int flg_vld    = 0;
  unsigned int bits_seg_0 = 0;
  unsigned int bits_seg_1 = 0;
  int dc_val              = 0;
  int mean_i              = 0;
  int mean_q              = 0;
  int power_val           = 0;
  int mean_power_t        = 0;
  int i, retry_read = 20;
  unsigned int rx_dc_ctrl_val;

  // CONFIG POWER MEAS NUM
  w_addr = RX_PWR_CTRL_0;
  w_data = 0x0F;
  hal_spi_write_reg(phy, w_addr, w_data);

  rx_dc_ctrl_val = hal_spi_read_reg(phy, RX_DC_CTRL_0);
  if (channel_sel == TRX_CHN1) {
    hal_spi_write_reg(phy, RX_DC_CTRL_0, 0xe1);
  } else if (channel_sel == TRX_CHN2) {
    hal_spi_write_reg(phy, RX_DC_CTRL_0, 0xe2);
  }

  for (i = 0; i < try_num; i++) {
    r_addr  = RX_DC_PWR_STATE_0;
    flg_vld = 0;
    do {
      r_data  = hal_spi_read_reg(phy, r_addr);
      flg_vld = (channel_sel == TRX_CHN1) ? (r_data & 0x1) : ((r_data >> 1) & 0x1);
      CHIP_DELAY(1);
      //LOG_INFO("wait flag=1\n");
      if (retry_read < 0) {
        LOG_ERROR("rx get adc power failed!\n");
        return -1;
      }
      retry_read--;
    } while (flg_vld == 0);

    // RUN POWER MEAS
    w_addr = RX_PWR_CTRL_0;
    w_data = (channel_sel == TRX_CHN1) ? 0x1F : 0x2F;
    hal_spi_write_reg(phy, w_addr, w_data);

    retry_read = 20;
    r_addr     = RX_DC_PWR_STATE_0;
    flg_vld    = 0;
    do {
      r_data  = hal_spi_read_reg(phy, r_addr);
      flg_vld = (channel_sel == TRX_CHN1) ? ((r_data >> 2) & 0x1) : ((r_data >> 3) & 0x1);
      CHIP_DELAY(1);
      //LOG_INFO("wait flag=1\n");
      if (retry_read < 0) {
        LOG_ERROR("rx get adc power failed!\n");
        return -1;
      }
      retry_read--;
    } while (flg_vld == 0);

    // read power
    power_val = 0;
    r_addr    = (channel_sel == TRX_CHN1) ? RX1_AVG_PWR_2 : RX2_AVG_PWR_2;
    r_data    = hal_spi_read_reg(phy, r_addr);
    power_val += ((r_data & 0x1f) << 16);
    r_addr = (channel_sel == TRX_CHN1) ? RX1_AVG_PWR_1 : RX2_AVG_PWR_1;
    r_data = hal_spi_read_reg(phy, r_addr);
    power_val += (r_data << 8);
    r_addr = (channel_sel == TRX_CHN1) ? RX1_AVG_PWR_0 : RX2_AVG_PWR_0;
    r_data = hal_spi_read_reg(phy, r_addr);
    power_val += (r_data << 0);
    mean_power_t = mean_power_t + power_val;
  }

  *mean_power = mean_power_t;
  hal_spi_write_reg(phy, RX_DC_CTRL_0, rx_dc_ctrl_val);

  return 0;
}

short do_rx_dc_offset_cal(rf_chip_phy_t* phy, TRX_CHN_ENUM chn,
                          TRX_IQ_ENUM iq, unsigned short tune_reg, unsigned char flag, unsigned char ext) {
  unsigned short i_ofst_reg1;
  unsigned short q_ofst_reg1;
  short tmp_flag, i = 7;
  short ret        = 0;
  short adcCode[9] = {0, 1, 2, 3, 4, 5, 6, 7, 8};
  char bit;
  int dc_offset_i = 0;
  int dc_offset_q = 0;
  int abs_min     = -1;
  int avg         = 0;
  int recode_reg_val, start, end, val, delta, sum;

  tmp_flag = flag ? 0 : 1;
  if (chn == TRX_CHN1) {
    i_ofst_reg1 = 0x04E;
    q_ofst_reg1 = 0x050;
  } else {
    i_ofst_reg1 = 0x052;
    q_ofst_reg1 = 0x054;
  }

  while (i >= 0) {
    //ret = rx_get_adc_offset(phy, chn, 2, &dc_offset_i, &dc_offset_q);
    //if (ret < 0)
    //	return ret;
    //val = hal_spi_read_reg(phy, tune_reg);
    //LOG_MDEBUG(phy, RX_DC_CAL, "coarse tune,  reg[%c]=0x%x, tune val=0x%x, dc_offset_i_val=%d, dc_offst_q_val=%d\n",
    //		(iq==TRX_I)?'I':'Q', tune_reg, val, dc_offset_i, dc_offset_q);

    rx_dc_offset_cal_enable(phy, chn, 0);
    //CHIP_UDELAY(10);
    rx_dc_offset_cal_enable(phy, chn, 1);

    if (rx_dc_offset_cal_status(phy, chn) == 0)
      return -1;

    if (iq == TRX_I)
      bit = HAL_REG_GET_BIT(phy, i_ofst_reg1, BIT1);
    else
      bit = HAL_REG_GET_BIT(phy, q_ofst_reg1, BIT1);

    if (bit == tmp_flag)
      HAL_REG_CLR_BIT(phy, tune_reg, i);
    else
      HAL_REG_SET_BIT(phy, tune_reg, i);

    if (i > 0)
      HAL_REG_SET_BIT(phy, tune_reg, i - 1);

    i = i - 1;
  }

  if (ext) {
    val            = hal_spi_read_reg(phy, tune_reg);
    start          = val;
    end            = val;
    recode_reg_val = val;
    for (i = 8; i >= 0; i--) {
      delta = val - adcCode[i];
      sum   = val + adcCode[i];
      if ((delta >= 0) && (sum <= 255)) {
        start = delta;
        end   = sum;
        break;
      }
    }

    for (i = start; i <= end; i++) {
      hal_spi_write_reg(phy, tune_reg, i);
      //rx_dc_offset_cal_enable(phy, chn, 0);
      //CHIP_UDELAY(10);
      //rx_dc_offset_cal_enable(phy, chn, 1);

      //if (rx_dc_offset_cal_status(phy, chn) == 0)
      //	return -1;

      ret = rx_get_adc_offset(phy, chn, 1, &dc_offset_i, &dc_offset_q);
      if (ret < 0)
        return ret;

      if (iq == TRX_I)
        avg = dc_offset_i / 2;
      else
        avg = dc_offset_q / 2;

      LOG_MDEBUG(phy, RX_DC_CAL, "fine tune reg[%c]=0x%x, tune val=0x%x, dc_offset_i_val=%d, dc_offst_q_val=%d\n",
                 (iq == TRX_I) ? 'I' : 'Q', tune_reg, i, dc_offset_i, dc_offset_q);

      if (abs_min < 0) {
        abs_min = (avg >= 0) ? (avg) : (0 - avg);
      } else {
        avg = (avg >= 0) ? (avg) : (0 - avg);
        if (avg <= abs_min) {
          abs_min        = avg;
          recode_reg_val = i;
        }
      }

      if (abs_min == 0) {
        recode_reg_val = i;
        break;
      }
    }
    hal_spi_write_reg(phy, tune_reg, recode_reg_val);
  }

  return 0;
}

int handle_rx_dc_offset_cal(rf_chip_phy_t* phy, TRX_CHN_ENUM chn,
                            TRX_IQ_ENUM iq, unsigned short tune_reg, unsigned char flag, unsigned char ext) {
#if 0
	unsigned short rxdac_ctrl_reg;
	unsigned short rxbb_vcmsel_reg;
	unsigned short i_ofst_reg;
	short tmp, reg_val;

	if (chn == TRX_CHN1)
	{
		i_ofst_reg = 0x04E;
		rxdac_ctrl_reg = 0x6D2;
		rxbb_vcmsel_reg = 0x667;
	}
	else
	{
		i_ofst_reg = 0x052;
		rxdac_ctrl_reg = 0x6E2;
		rxbb_vcmsel_reg = 0x685;
	}

	//hal_spi_write_reg(phy, 0x0DD, 0xF3);
	hal_spi_write_reg(phy, i_ofst_reg, 0xFF);

	reg_val = hal_spi_read_reg(phy, rxbb_vcmsel_reg);
	reg_val = (reg_val & 0x3) << 2;

	tmp = hal_spi_read_reg(phy, rxdac_ctrl_reg);
	tmp = tmp & ((~(0x3 << 2)) & 0xFF);
	reg_val = reg_val | tmp;
	hal_spi_write_reg(phy, rxdac_ctrl_reg, reg_val);

	//rx_dc_offset_cal_enable(phy, chn, 1);

	/* check cali status */
	//if (rx_dc_offset_cal_status(phy, chn) == 0)
	//	return -1;
#endif

  /* cal */
  if (do_rx_dc_offset_cal(phy, chn, iq, tune_reg, flag, ext) < 0)
    return -1;

  //hal_spi_write_reg(phy, 0x0DD, 0xF0);
  //rx_dc_offset_cal_enable(phy, chn, 0);

  return 0;
}

int rx_dc_offset_cal_update_all_gain_tia_lut(rf_chip_phy_t* phy, int channel, int val) {
  unsigned int lut_addr;
  unsigned int addr_reg, i_data_reg, q_data_reg, ctrl_reg;
  if (channel == TRX_CHN1) {
    addr_reg   = 0x102;
    i_data_reg = 0x106;
    q_data_reg = 0x105;
    ctrl_reg   = 0x100;
  } else {
    addr_reg   = 0x101;
    i_data_reg = 0x104;
    q_data_reg = 0x103;
    ctrl_reg   = 0x100;
  }

  for (lut_addr = 0; lut_addr < 60; lut_addr++) {

    /* write addr */
    hal_spi_write_reg(phy, addr_reg, lut_addr);

    /* write data */
    hal_spi_write_reg(phy, q_data_reg, val);
    hal_spi_write_reg(phy, i_data_reg, val);

    /* pulse write */
    if (channel == TRX_CHN1) {
      hal_spi_write_reg(phy, ctrl_reg, 0x40);
      hal_spi_write_reg(phy, ctrl_reg, 0x00);
    } else {
      hal_spi_write_reg(phy, ctrl_reg, 0x80);
      hal_spi_write_reg(phy, ctrl_reg, 0x00);
    }
    //LOG_MDEBUG(phy, RX_DC_CAL, "Rx dc offst, Write cal val:0x%0x, 0x%0x to lut_addr:%u\n", val, val, lut_addr);
    CHIP_UDELAY(5);
  }

  return 0;
}

int rx_dc_offset_cal_update_bq_lut(rf_chip_phy_t* phy, int channel, RXFE_GAIN_ENUM rxfe_gain_index, unsigned int val[]) {
  int i;
  unsigned int lut_addr;
  unsigned int addr_reg, i_data_reg, q_data_reg, ctrl_reg;

  lut_addr = rxfe_gain_index * 13;

  if (channel == TRX_CHN1) {
    addr_reg   = 0x10d;
    i_data_reg = 0x111;
    q_data_reg = 0x110;
    ctrl_reg   = 0x10b;
  } else {
    addr_reg   = 0x10c;
    i_data_reg = 0x10f;
    q_data_reg = 0x10e;
    ctrl_reg   = 0x10b;
  }

  for (i = 0; i < 13; i++) {
    /* write addr */
    hal_spi_write_reg(phy, addr_reg, lut_addr);

    /* write data */
    hal_spi_write_reg(phy, q_data_reg, val[2 * i + 1]);
    hal_spi_write_reg(phy, i_data_reg, val[2 * i]);

    /* pulse write */
    if (channel == TRX_CHN1) {
      hal_spi_write_reg(phy, ctrl_reg, 0x40);
      hal_spi_write_reg(phy, ctrl_reg, 0x00);
    } else {
      hal_spi_write_reg(phy, ctrl_reg, 0x80);
      hal_spi_write_reg(phy, ctrl_reg, 0x00);
    }
    //LOG_MDEBUG(phy, RX_DC_CAL, "Rx dc offst, Write g%d cal val:0x%0x, 0x%0x to lut_addr:%u\n", i,  val[2*i], val[2*i +1], lut_addr);
    CHIP_UDELAY(5);
    lut_addr++;
  }
  return 0;
}

int rx_dc_get_tia_lut_pos(rf_chip_phy_t* phy, int channel, short* mix_pos) {
  unsigned int addr_mix_reg = (channel == TRX_CHN1) ? 0xE0 : 0xE1;
  unsigned char mix_val     = hal_spi_read_reg(phy, addr_mix_reg);
  unsigned char pos_stat    = 0;
  int ret                   = 0;

  switch (mix_val) {
    case 0x1:
      pos_stat = 54;
      break;

    case 0x2:
      pos_stat = 48;
      break;

    case 0x04:
      pos_stat = 42;
      break;

    case 0x08:
      pos_stat = 36;
      break;

    case 0x10:
      pos_stat = 30;
      break;

    case 0x20:
      pos_stat = 24;
      break;

    case 0x03:
      pos_stat = 18;
      break;

    case 0x0C:
      pos_stat = 12;
      break;

    case 0x30:
      pos_stat = 6;
      break;

    case 0x40:
      pos_stat = 0;
      break;

    default:
      pos_stat = 60;
      ret      = -1;
      break;
  }
  *mix_pos = pos_stat;

  return ret;
}

int rx_dc_set_tia_lut_setting_by_idx(rf_chip_phy_t* phy, int channel, short mix_pos, short lmt_index,
                                     unsigned char i_data, unsigned char q_data) {
  unsigned int addr_reg, i_data_reg, q_data_reg, ctrl_reg;
  unsigned int lut_addr;

  if ((lmt_index > 5) || (mix_pos > 54)) {
    LOG_ERROR("%s in line %d REPORT ERR invalid lmt_index(%d) or mix_pos(%d)\n",
              __FUNCTION__, __LINE__, lmt_index, mix_pos);
    return -1;
  }

  if (channel == TRX_CHN1) {
    addr_reg   = 0x102;
    i_data_reg = 0x106;
    q_data_reg = 0x105;
    ctrl_reg   = 0x100;
  } else {
    addr_reg   = 0x101;
    i_data_reg = 0x104;
    q_data_reg = 0x103;
    ctrl_reg   = 0x100;
  }

  lut_addr = mix_pos + (5 - lmt_index);

  /* write addr */
  hal_spi_write_reg(phy, addr_reg, lut_addr);

  /* write data */
  hal_spi_write_reg(phy, q_data_reg, q_data);
  hal_spi_write_reg(phy, i_data_reg, i_data);

  /* pulse write */
  if (channel == TRX_CHN1) {
    hal_spi_write_reg(phy, ctrl_reg, 0x40);
    hal_spi_write_reg(phy, ctrl_reg, 0x00);
  } else {
    hal_spi_write_reg(phy, ctrl_reg, 0x80);
    hal_spi_write_reg(phy, ctrl_reg, 0x00);
  }

  CHIP_UDELAY(5);

  return 0;
}

int rx_dc_get_tia_lut_setting_by_idx(rf_chip_phy_t* phy, int channel, short mix_pos, short lmt_index,
                                     unsigned char* i_data, unsigned char* q_data) {
  unsigned int addr_reg, i_data_reg, q_data_reg, ctrl_reg;
  unsigned int lut_addr;

  if ((lmt_index > 5) || (mix_pos > 54)) {
    LOG_ERROR("%s in line %d REPORT ERR invalid lmt_index(%d) or mix_pos(%d)\n",
              __FUNCTION__, __LINE__, lmt_index, mix_pos);
    return -1;
  }

  if (channel == TRX_CHN1) {
    addr_reg   = 0x102;
    i_data_reg = 0x107;
    q_data_reg = 0x108;
    ctrl_reg   = 0x100;
  } else {
    addr_reg   = 0x101;
    i_data_reg = 0x109;
    q_data_reg = 0x10A;
    ctrl_reg   = 0x100;
  }

  lut_addr = mix_pos + (5 - lmt_index);

  /* write addr */
  hal_spi_write_reg(phy, addr_reg, lut_addr);

  /* pulse read */
  if (channel == TRX_CHN1) {
    hal_spi_write_reg(phy, ctrl_reg, 0x10);
    hal_spi_write_reg(phy, ctrl_reg, 0x00);
  } else {
    hal_spi_write_reg(phy, ctrl_reg, 0x20);
    hal_spi_write_reg(phy, ctrl_reg, 0x00);
  }

  /* read data */
  *q_data = hal_spi_read_reg(phy, q_data_reg);
  *i_data = hal_spi_read_reg(phy, i_data_reg);

  CHIP_UDELAY(5);

  return 0;
}

int rx_dc_set_bq_lut_setting_by_idx(rf_chip_phy_t* phy, int channel, short lmt_index, short lpf_index,
                                    unsigned char i_data, unsigned char q_data) {
  unsigned int lut_addr;
  unsigned int addr_reg, i_data_reg, q_data_reg, ctrl_reg;

  if ((lmt_index > 5) || (lpf_index > 12)) {
    LOG_ERROR("%s in line %d REPORT ERR invalid lmt_index(%d) or lpf_index(%d)\n",
              __FUNCTION__, __LINE__, lmt_index, lpf_index);
    return -1;
  }

  lut_addr = (5 - lmt_index) * 13 + (12 - lpf_index);

  if (channel == TRX_CHN1) {
    addr_reg   = 0x10d;
    i_data_reg = 0x111;
    q_data_reg = 0x110;
    ctrl_reg   = 0x10b;
  } else {
    addr_reg   = 0x10c;
    i_data_reg = 0x10f;
    q_data_reg = 0x10e;
    ctrl_reg   = 0x10b;
  }

  /* write addr */
  hal_spi_write_reg(phy, addr_reg, lut_addr);

  /* write data */
  hal_spi_write_reg(phy, q_data_reg, q_data);
  hal_spi_write_reg(phy, i_data_reg, i_data);

  /* pulse write */
  if (channel == TRX_CHN1) {
    hal_spi_write_reg(phy, ctrl_reg, 0x40);
    hal_spi_write_reg(phy, ctrl_reg, 0x00);
  } else {
    hal_spi_write_reg(phy, ctrl_reg, 0x80);
    hal_spi_write_reg(phy, ctrl_reg, 0x00);
  }

  CHIP_UDELAY(5);

  return 0;
}

int rx_dc_get_bq_lut_setting_by_idx(rf_chip_phy_t* phy, int channel, short lmt_index, short lpf_index,
                                    unsigned char* i_data, unsigned char* q_data) {
  unsigned int lut_addr;
  unsigned int addr_reg, i_data_read_reg, q_data_read_reg, ctrl_reg;

  if (i_data == NULL || q_data == NULL) {
    LOG_ERROR("%s in line %d REPORT ERR NULL point access\n", __FUNCTION__, __LINE__);
    return -1;
  }

  if ((lmt_index > 5) || (lpf_index > 12)) {
    LOG_ERROR("%s in line %d REPORT ERR invalid lmt_index(%d) or lpf_index(%d)\n",
              __FUNCTION__, __LINE__, lmt_index, lpf_index);
    return -1;
  }

  lut_addr = (5 - lmt_index) * 13 + (12 - lpf_index);

  if (channel == TRX_CHN1) {
    addr_reg        = 0x10d;
    i_data_read_reg = 0x112;
    q_data_read_reg = 0x113;
    ctrl_reg        = 0x10b;
  } else {
    addr_reg        = 0x10c;
    i_data_read_reg = 0x114;
    q_data_read_reg = 0x115;
    ctrl_reg        = 0x10b;
  }

  /* write addr */
  hal_spi_write_reg(phy, addr_reg, lut_addr);

  /* pulse read */
  if (channel == TRX_CHN1) {
    hal_spi_write_reg(phy, ctrl_reg, 0x10);
    hal_spi_write_reg(phy, ctrl_reg, 0x00);
  } else {
    hal_spi_write_reg(phy, ctrl_reg, 0x20);
    hal_spi_write_reg(phy, ctrl_reg, 0x00);
  }

  /* read data */
  *q_data = hal_spi_read_reg(phy, q_data_read_reg);
  *i_data = hal_spi_read_reg(phy, i_data_read_reg);

  CHIP_UDELAY(5);

  return 0;
}

int rx_dc_offset_cal_in(rf_chip_phy_t* phy, int channel, RX_PORT_ENUM rx_port) {
  unsigned int q_rxbbf_offset_tia_reg, i_rxbbf_offset_tia_reg;
  unsigned int q_rxbbf_offset_bq_reg, i_rxbbf_offset_bq_reg;
  int i;
  int rxfe_gain_index;
  //unsigned int tia[RX_PORT_GAIN_MAX * 2];
  unsigned int bq[26];
  unsigned int bbf_gain_reg, lfbuf_reg, reg_val_bak, rx_lfbuf_restore, init_ival;

  unsigned short rxdac_ctrl_reg;
  unsigned short rxbb_vcmsel_reg;
  unsigned short i_ofst_reg;
  short tmp, reg_val;

#define RX_FE_CAL_CODE 0x80

  for (i = 0; i < 26; i++)
    bq[i] = 0;
  for (i = 0; i < RX_PORT_GAIN_MAX * 2; i++)
    phy->tia[channel][i] = RX_FE_CAL_CODE;

  if (channel == TRX_CHN1) {
    q_rxbbf_offset_tia_reg = 0x6D3;
    i_rxbbf_offset_tia_reg = 0x6D5;
    q_rxbbf_offset_bq_reg  = 0x6D4;
    i_rxbbf_offset_bq_reg  = 0x6D6;
    bbf_gain_reg           = 0x65C;
    lfbuf_reg              = 0x6d0;

    i_ofst_reg      = 0x04E;
    rxdac_ctrl_reg  = 0x6D2;
    rxbb_vcmsel_reg = 0x667;
  } else {
    q_rxbbf_offset_tia_reg = 0x6E3;
    i_rxbbf_offset_tia_reg = 0x6E5;
    q_rxbbf_offset_bq_reg  = 0x6E4;
    i_rxbbf_offset_bq_reg  = 0x6E6;
    bbf_gain_reg           = 0x67A;
    lfbuf_reg              = 0x6e0;

    i_ofst_reg      = 0x052;
    rxdac_ctrl_reg  = 0x6E2;
    rxbb_vcmsel_reg = 0x685;
  }

  //rxfe cal: set the fix value 0x80;
  LOG_MDEBUG(phy, RX_DC_CAL, "Update the tia of  rx dc cal to LUT\n");

  if (!phy->config->lo_leakage_cal_flag) {
    rx_dc_offset_cal_update_all_gain_tia_lut(phy, channel, RX_FE_CAL_CODE);
    for (i = RX_PORT_G0; i < RX_PORT_GAIN_MAX; i++) {
      if (channel == 0) {
        g_reg_6D5_val[i] = RX_FE_CAL_CODE;
        g_reg_6D3_val[i] = RX_FE_CAL_CODE;
      } else {
        g_reg_6E5_val[i] = RX_FE_CAL_CODE;
        g_reg_6E3_val[i] = RX_FE_CAL_CODE;
      }
    }

    hal_spi_write_reg(phy, q_rxbbf_offset_tia_reg, RX_FE_CAL_CODE);
    hal_spi_write_reg(phy, i_rxbbf_offset_tia_reg, RX_FE_CAL_CODE);
  }

  reg_val_bak      = hal_spi_read_reg(phy, bbf_gain_reg);
  rx_lfbuf_restore = hal_spi_read_reg(phy, lfbuf_reg);

  hal_spi_write_reg(phy, lfbuf_reg, 0xef);

  if (phy->rx_dc_cal_flag[channel]) {
    for (rxfe_gain_index = 0; rxfe_gain_index < RX_PORT_GAIN_MAX; rxfe_gain_index++) {
      for (i = 0; i < 26; i++) {
        if (channel)
          bq[i] = phy->bq_rx2[rxfe_gain_index][i];
        else
          bq[i] = phy->bq_rx1[rxfe_gain_index][i];
      }

      if (phy->rx_dc_lut_update) {
        //LOG_MDEBUG(phy, RX_DC_CAL, "Update the bq of rx dc cal to LUT\n");
        rx_dc_offset_cal_update_bq_lut(phy, channel, rxfe_gain_index, bq);
      }
    }
  } else {
    hal_spi_write_reg(phy, 0x0DD, 0xF3);
    hal_spi_write_reg(phy, i_ofst_reg, 0xFF);

    reg_val = hal_spi_read_reg(phy, rxbb_vcmsel_reg);
    reg_val = (reg_val & 0x3) << 2;

    tmp     = hal_spi_read_reg(phy, rxdac_ctrl_reg);
    tmp     = tmp & ((~(0x3 << 2)) & 0xFF);
    reg_val = reg_val | tmp;
    hal_spi_write_reg(phy, rxdac_ctrl_reg, reg_val);

    for (rxfe_gain_index = 0; rxfe_gain_index < RX_PORT_GAIN_MAX; rxfe_gain_index++) {
      if (channel == 0) {
        hal_spi_write_reg(phy, q_rxbbf_offset_tia_reg, g_reg_6D3_val[rxfe_gain_index]);
        hal_spi_write_reg(phy, i_rxbbf_offset_tia_reg, g_reg_6D5_val[rxfe_gain_index]);

      } else {
        hal_spi_write_reg(phy, q_rxbbf_offset_tia_reg, g_reg_6E3_val[rxfe_gain_index]);
        hal_spi_write_reg(phy, i_rxbbf_offset_tia_reg, g_reg_6E5_val[rxfe_gain_index]);
      }

      LOG_MDEBUG(phy, RX_DC_CAL, "G%d: 0x%03x = 0x%02x, 0x%03x = 0x%02x\n", rxfe_gain_index,
                 q_rxbbf_offset_tia_reg, hal_spi_read_reg(phy, q_rxbbf_offset_tia_reg),
                 i_rxbbf_offset_tia_reg, hal_spi_read_reg(phy, i_rxbbf_offset_tia_reg));

      //LOG_MDEBUG(phy, RX_DC_CAL, "Rx dc offst, rxfe gain %d start\n", rxfe_gain_index);
      set_rx_port_man(phy, channel, rx_port, rxfe_gain_index);
      phy->config->rxfe_gain[channel] = rxfe_gain_index;

      /* rxbbf gain, 0,1,2,3,..., cal */
      for (i = 0; i <= 12; i++) {
        /* bbf gain */
        hal_spi_write_reg(phy, bbf_gain_reg, i);
        /*LOG_MDEBUG(phy, RX_DC_CAL, "Rx dc offst, rxbbf g%d cal start, Reg[0x%0x]=0x%0x, reg[0x%0x]=0x%0x\n",
                    i, bbf_gain_reg, hal_spi_read_reg(phy, bbf_gain_reg), lfbuf_reg, hal_spi_read_reg(phy, lfbuf_reg)); */

        /* i path cal */
        hal_spi_write_reg(phy, i_rxbbf_offset_bq_reg, 0x80);
        if (handle_rx_dc_offset_cal(phy, channel, TRX_I, i_rxbbf_offset_bq_reg, i + 1, 1) < 0) {
          LOG_ERROR("rx dc cal failed, bq, I-offset\n");
          return -1;
        }
        bq[i * 2] = hal_spi_read_reg(phy, i_rxbbf_offset_bq_reg);
        if ((bq[i * 2] == 0x00) || (bq[i * 2] == 0xFF)) {
          LOG_ERROR("[I path]rx dc cal failed, unexpected value(0x%x)\n", bq[i * 2]);
          //return -1;
        }

        /* Q path cal */
        hal_spi_write_reg(phy, q_rxbbf_offset_bq_reg, 0x80);
        if (handle_rx_dc_offset_cal(phy, channel, TRX_Q, q_rxbbf_offset_bq_reg, i + 1, 1) < 0) {
          LOG_ERROR("rx dc cal failed, bq, Q-offset\n");
          return -1;
        }
        bq[i * 2 + 1] = hal_spi_read_reg(phy, q_rxbbf_offset_bq_reg);
        if ((bq[i * 2 + 1] == 0x00) || (bq[i * 2 + 1] == 0xFF)) {
          LOG_ERROR("[Q path]rx dc cal failed, unexpected value(0x%x)\n", bq[i * 2 + 1]);
          //return -1;
        }

        /*LOG_MDEBUG(phy, RX_DC_CAL, "Rx dc offst, rxbbf g%d cal end, I path tune val:0x%0x Q path tune val:0x%0x\n",
                i, bq[i*2], bq[i*2+1]); */
      }

      for (i = 0; i < 26; i++) {
        if (channel)
          phy->bq_rx2[rxfe_gain_index][i] = bq[i];
        else
          phy->bq_rx1[rxfe_gain_index][i] = bq[i];
      }

      if (phy->rx_dc_lut_update) {
        //LOG_MDEBUG(phy, RX_DC_CAL, "Update the bq of rx dc cal to LUT\n");
        rx_dc_offset_cal_update_bq_lut(phy, channel, rxfe_gain_index, bq);
      }
      //LOG_MDEBUG(phy, RX_DC_CAL, "Rx dc offst, rxfe gain %d end\n", rxfe_gain_index);
    }

    hal_spi_write_reg(phy, 0x0DD, 0xF0);
    rx_dc_offset_cal_enable(phy, channel, 0);

    phy->rx_dc_cal_flag[channel] = 1;
  }

  //set the value rxfe g2 rxbbf g6 for qec
  set_rx_port_man(phy, channel, rx_port, RX_PORT_G2);
  phy->config->rxfe_gain[channel] = RX_PORT_G2;
  hal_spi_write_reg(phy, bbf_gain_reg, 6);
  if (channel) {
    hal_spi_write_reg(phy, i_rxbbf_offset_bq_reg, phy->bq_rx2[2][12]);
    hal_spi_write_reg(phy, q_rxbbf_offset_bq_reg, phy->bq_rx2[2][13]);
  } else {
    hal_spi_write_reg(phy, i_rxbbf_offset_bq_reg, phy->bq_rx1[2][12]);
    hal_spi_write_reg(phy, q_rxbbf_offset_bq_reg, phy->bq_rx1[2][13]);
  }
  hal_spi_write_reg(phy, lfbuf_reg, rx_lfbuf_restore);

  LOG_MDEBUG(phy, RX_DC_CAL, "Rx dc offst, rxbbf end, reg[0x%0x]=0x%0x, reg[0x%0x] = 0x%0x, reg[0x%0x]=0x%0x, reg[0x%0x] = 0x%0x\n",
             i_rxbbf_offset_tia_reg, hal_spi_read_reg(phy, i_rxbbf_offset_tia_reg), q_rxbbf_offset_tia_reg,
             hal_spi_read_reg(phy, q_rxbbf_offset_tia_reg), i_rxbbf_offset_bq_reg,
             hal_spi_read_reg(phy, i_rxbbf_offset_bq_reg), q_rxbbf_offset_bq_reg,
             hal_spi_read_reg(phy, q_rxbbf_offset_bq_reg));
  return 0;
}

int rx_dc_offset_cal(rf_chip_phy_t* phy, TRX_CHN_ENUM chn) {
  int rg702_val, rg74A_val;
  unsigned char lut702_val, lut74A_val;
  unsigned char FuncVer = phy->config->RxDc_Offset_Ver;
  //lo_leakage_cal(phy, chn);
  LOG_MAIN("Rx dc offst cal version=%d \n", FuncVer);
  if (FuncVer == RX_DC_OFFSET_CAL_WITH_3POINT_V2) {
    rx_dc_offset_cal_v2(phy, chn);
    return 0;
  } else if (FuncVer == RX_DC_OFFSET_CAL_WITH_3POINT_V3) {
    //rx_regs_debug_print(phy, chn);
    rx_dc_offset_cal_v3(phy, chn);
    return 0;
  }

  //reset_adc(chn);
  //rx_regs_debug_print(phy, chn);
  if ((phy->config->mode >= CH1_FDD) && (phy->config->mode <= CH1_CH2_FDD)) {
    set_trx_lo(phy, RX_DIR, chn, phy->config->rx_flo);
    CHIP_DELAY(1);
    if (rx_dc_offset_cal_in(phy, chn, phy->config->rx_port[chn]) < 0) {
      phy->error = -RX_DC_CAL_FAIL;
      return -RX_DC_CAL_FAIL;
    }
  } else {
    /// Read Init Val
    if (chn == TRX_CHN1) {
      rg702_val = hal_spi_read_reg(phy, 0x702) & 0xFF;
      /// Read Lut S19 Val
      read_lut_byte(phy, MAIN_ENSM_LUT, 19, 0x702, 197, 0, &lut702_val);
      hal_spi_write_reg(phy, 0x702, lut702_val);
    } else {
      rg74A_val = hal_spi_read_reg(phy, 0x74A) & 0xFF;
      /// Read Lut S19 Val
      read_lut_byte(phy, MAIN_ENSM_LUT, 19, 0x74A, 198, 3, &lut74A_val);
      hal_spi_write_reg(phy, 0x74A, lut74A_val);
    }

    //step1, lock tx
    set_trx_lo(phy, TX_DIR, chn, phy->config->tx_flo);
    CHIP_DELAY(1);

    if (rx_dc_offset_cal_in(phy, chn, phy->config->rx_port[chn]) < 0) {
      phy->error = -RX_DC_CAL_FAIL;
      return -RX_DC_CAL_FAIL;
    }

    //need rxfe g2 gain
    //spi_write_reg(0x67F, 0x0a);
    hal_spi_write_reg(phy, 0x660, 0x0a);

    ///Return: Write Init Val
    if (chn == TRX_CHN1) {
      hal_spi_write_reg(phy, 0x702, rg702_val & 0xFF);
    } else {
      hal_spi_write_reg(phy, 0x74A, rg74A_val & 0xFF);
    }
  }

  LOG_MDEBUG(phy, RX_DC_CAL, "Rx dc offst calibration end\n");

  return 0;
}

int rx_regs_debug_print(rf_chip_phy_t* phy, TRX_CHN_ENUM chn) {
  int i;
  int regs;
  int reg_addr;

  reg_addr = (chn) ? 0x66E : 0x650;
  regs     = 30;
  for (i = 0; i < regs; i++) {
    if (i % 2 == 0)
      LOG_MAIN("\n");
    LOG_MAIN("[0x%03x = 0x%02x], ", reg_addr + i, hal_spi_read_reg(phy, reg_addr + i));
  }
  LOG_MAIN("\n");

  reg_addr = (chn) ? 0x6E0 : 0x6D0;
  regs     = 10;
  for (i = 0; i < regs; i++) {
    if (i % 2 == 0)
      LOG_MAIN("\n");
    LOG_MAIN("[0x%03x = 0x%02x], ", reg_addr + i, hal_spi_read_reg(phy, reg_addr + i));
  }
  LOG_MAIN("\n");

  return 0;
}

/****************************************************************************************/
// RX DC  Offset cal  V2
/****************************************************************************************/
#if 1
typedef enum RX_LMT_GAIN {
  RX_LMT_GAIN_G0 = 0,
  RX_LMT_GAIN_G1,
  RX_LMT_GAIN_G2,
  RX_LMT_GAIN_G3,
  RX_LMT_GAIN_G4,
  RX_LMT_GAIN_G5,
  RX_LMT_GAIN_END,
} RX_LMT_GAIN_ENUM;

typedef enum RX_LPF_GAIN {
  RX_LPF_GAIN_G0 = 0,
  RX_LPF_GAIN_G1,
  RX_LPF_GAIN_G2,
  RX_LPF_GAIN_G3,
  RX_LPF_GAIN_G4,
  RX_LPF_GAIN_G5,
  RX_LPF_GAIN_G6,
  RX_LPF_GAIN_G7,
  RX_LPF_GAIN_G8,
  RX_LPF_GAIN_G9,
  RX_LPF_GAIN_G10,
  RX_LPF_GAIN_G11,
  RX_LPF_GAIN_G12 = 12,
  RX_LPF_GAIN_END,
} RX_LPF_GAIN_ENUM;

typedef enum RX_3POING_CAL_INDEX {
  RX_3POING_CAL_INDEX_G0_G1 = 0,
  RX_3POING_CAL_INDEX_G2_G3,
  RX_3POING_CAL_INDEX_G4_G6,
  RX_3POING_CAL_INDEX_G5_G6,
} RX_3POING_CAL_INDEX_ENUM;

static int do_rx_dc_offset_cal_v2(rf_chip_phy_t* phy, int channel,
                                  TRX_IQ_ENUM iq, unsigned int tune_reg, int flag, int ext, int* offsetVal, int* calCode) {
  unsigned int i_ofst_reg1, i_ofst_reg2;
  unsigned int q_ofst_reg1, q_ofst_reg2;
  int tmp_flag, i, j;
  char bit;
  int dc_offset_i = 0;
  int dc_offset_q = 0;
  int abs_min     = -1;
  int avg         = 0;
  int recode_reg_val, start, end, val, delta, sum;
  int adcCode[9] = {0, 1, 2, 3, 4, 5, 6, 7, 8};
  int ret        = 0;

  if (flag)
    tmp_flag = 0;
  else
    tmp_flag = 1;

  if (channel == TRX_CHN2) {
    i_ofst_reg1 = 0x052;
    i_ofst_reg2 = 0x053;
    q_ofst_reg1 = 0x054;
    q_ofst_reg2 = 0x055;
  } else {
    i_ofst_reg1 = 0x04E;
    i_ofst_reg2 = 0x04F;
    q_ofst_reg1 = 0x050;
    q_ofst_reg2 = 0x051;
  }

  i = 7;
  while (i >= 0) {
    ret = rx_get_adc_offset(phy, channel, 2, &dc_offset_i, &dc_offset_q);
    if (ret < 0)
      return ret;
    val = hal_spi_read_reg(phy, tune_reg);
    if (val == 0x80) {
      if (iq == TRX_I)
        *offsetVal = dc_offset_i;
      else
        *offsetVal = dc_offset_q;
    }

    if (iq == TRX_I) {
      if (!dc_offset_i)
        break;
    } else {
      if (!dc_offset_q)
        break;
    }

    rx_dc_offset_cal_enable(phy, channel, 0);
    CHIP_UDELAY(5);  //50us

    rx_dc_offset_cal_enable(phy, channel, 1);
    if (rx_dc_offset_cal_status(phy, channel) == 0)
      return -1;

    if (iq == TRX_I)
      bit = HAL_REG_GET_BIT(phy, i_ofst_reg1, BIT1);
    else
      bit = HAL_REG_GET_BIT(phy, q_ofst_reg1, BIT1);

    if (bit == tmp_flag)
      HAL_REG_CLR_BIT(phy, tune_reg, i);
    else
      HAL_REG_SET_BIT(phy, tune_reg, i);

    if (i > 0)
      HAL_REG_SET_BIT(phy, tune_reg, i - 1);

    i = i - 1;
  }
  //getTime();

  if (ext) {
    val            = hal_spi_read_reg(phy, tune_reg);
    start          = val;
    end            = val;
    recode_reg_val = val;
    for (i = 8; i >= 0; i--) {
      delta = val - adcCode[i];
      sum   = val + adcCode[i];
      if ((delta >= 0) && (sum <= 255)) {
        start = delta;
        end   = sum;
        break;
      }
    }

    for (i = start; i <= end; i++) {
      hal_spi_write_reg(phy, tune_reg, i);
      ret = rx_get_adc_offset(phy, channel, 2, &dc_offset_i, &dc_offset_q);
      if (ret < 0)
        return ret;

      if (iq == TRX_I)
        avg = dc_offset_i / 2;
      else
        avg = dc_offset_q / 2;

      LOG_MDEBUG(phy, RX_DC_OFFSET_CAL, "fine tune reg[%c]=0x%x, tune val=0x%x, dc_offset_i_val=%d, dc_offst_q_val=%d\n",
                 (iq == TRX_I) ? 'I' : 'Q', tune_reg, i, dc_offset_i, dc_offset_q);

      if (abs_min < 0)
        abs_min = (avg >= 0) ? (avg) : (0 - avg);
      else {
        avg = (avg >= 0) ? (avg) : (0 - avg);
        if (avg <= abs_min) {
          abs_min        = avg;
          recode_reg_val = i;
        }
      }

      if (abs_min == 0) {
        recode_reg_val = i;
        break;
      }
    }
    hal_spi_write_reg(phy, tune_reg, recode_reg_val);
  }
  //getTime();

  *calCode = hal_spi_read_reg(phy, tune_reg);
  LOG_MDEBUG(phy, RX_DC_OFFSET_CAL, "Fine tune reg[%c]=0x%x, tune val = 0x%x, dc_offset_i_val=%d, dc_offst_q_val=%d\n",
             (iq == TRX_I) ? 'I' : 'Q', tune_reg, *calCode, dc_offset_i, dc_offset_q);
  return 0;
}

static int handle_rx_dc_offset_cal_v2(rf_chip_phy_t* phy, int channel, TRX_IQ_ENUM iq, unsigned int tune_reg, int flag, int ext,
                                      int* offsetVal, int* calCode) {
  unsigned int rxdac_ctrl_reg;
  unsigned int rxbb_vcmsel_reg;
  unsigned int i_ofst_reg;
  int i, j, tmp, reg_val;

  if (channel == TRX_CHN2) {
    i_ofst_reg      = 0x052;
    rxdac_ctrl_reg  = 0x6E2;
    rxbb_vcmsel_reg = 0x685;
  } else {
    i_ofst_reg      = 0x04E;
    rxdac_ctrl_reg  = 0x6D2;
    rxbb_vcmsel_reg = 0x667;
  }

  hal_spi_write_reg(phy, 0x0DD, 0xF3);
  hal_spi_write_reg(phy, i_ofst_reg, 0xFF);

  reg_val = hal_spi_read_reg(phy, rxbb_vcmsel_reg);
  reg_val = (reg_val & 0x3) << 2;

  tmp     = hal_spi_read_reg(phy, rxdac_ctrl_reg);
  tmp     = tmp & ((~(0x3 << 2)) & 0xFF);
  reg_val = reg_val | tmp;
  hal_spi_write_reg(phy, rxdac_ctrl_reg, reg_val);

  rx_dc_offset_cal_enable(phy, channel, 1);

  /* check cali status */
  if (rx_dc_offset_cal_status(phy, channel) == 0)
    return -1;

  /* cal */
  if (do_rx_dc_offset_cal_v2(phy, channel, iq, tune_reg, flag, ext, offsetVal, calCode) < 0)
    return -1;

  hal_spi_write_reg(phy, 0x0DD, 0xF0);
  rx_dc_offset_cal_enable(phy, channel, 0);
  return 0;
}

static int get_offset_value_v2(rf_chip_phy_t* phy, int channel, TRX_IQ_ENUM iq, RX_LMT_GAIN_ENUM lmt_gain,
                               RX_LPF_GAIN_ENUM lpt_gain, int* offsetVal, int* calCode) {
  int dc_offset_i, dc_offset_q;
  unsigned int q_rxbbf_offset_tia_reg, i_rxbbf_offset_tia_reg;
  unsigned int q_rxbbf_offset_bq_reg, i_rxbbf_offset_bq_reg;
  unsigned int bbf_gain_reg, lfbuf_reg, reg_val_bak, rx_lfbuf_restore, init_ival;

  if (channel == TRX_CHN2) {
    q_rxbbf_offset_bq_reg = 0x6E4;
    i_rxbbf_offset_bq_reg = 0x6E6;
    bbf_gain_reg          = 0x67A;
    lfbuf_reg             = 0x6e0;
  } else {
    q_rxbbf_offset_bq_reg = 0x6D4;
    i_rxbbf_offset_bq_reg = 0x6D6;
    bbf_gain_reg          = 0x65C;
    lfbuf_reg             = 0x6d0;
  }

  hal_spi_write_reg(phy, bbf_gain_reg, lpt_gain);
  if (TRX_I == iq) {
    ///  I Path
    hal_spi_write_reg(phy, i_rxbbf_offset_bq_reg, 0x80);
    if (handle_rx_dc_offset_cal_v2(phy, channel, iq, i_rxbbf_offset_bq_reg, 1, 0, offsetVal, calCode) < 0)
      return -1;

    //spi_read_reg(i_rxbbf_offset_bq_reg);
  } else {
    /// Q  Path
    hal_spi_write_reg(phy, q_rxbbf_offset_bq_reg, 0x80);
    if (handle_rx_dc_offset_cal_v2(phy, channel, iq, q_rxbbf_offset_bq_reg, 1, 0, offsetVal, calCode) < 0)
      return -1;

    //spi_read_reg(q_rxbbf_offset_bq_reg);
  }
  return 0;
}

static int rx_dc_offset_cal_in_v2(rf_chip_phy_t* phy, int channel, RX_PORT_ENUM rx_port) {
  unsigned int q_rxbbf_offset_tia_reg, i_rxbbf_offset_tia_reg;
  unsigned int q_rxbbf_offset_bq_reg, i_rxbbf_offset_bq_reg;
  int j, i, m, n, k;
  int rxfe_gain_index;
  //unsigned int tia[RX_PORT_GAIN_MAX * 2];
  unsigned int bq[26];
  unsigned int bbf_gain_reg, lfbuf_reg, reg_val_bak, rx_lfbuf_restore, init_ival;
  int C_00, C_0C, C_16;
  int A_Offset_00, B_Offset_0C, C_Offset_16;
  double x_01, y_01, z_01, a_01, b_01;

  int C_20, C_2C, C_30;
  int A_Offset_20, B_Offset_2C, C_Offset_30;
  double x_23, y_23, z_23, a_23, b_23;

  int C_40, C_4C, C_50;
  int A_Offset_40, B_Offset_4C, C_Offset_50;
  double x_45, y_45, z_45, a_45, b_45;

  int C_46, C_56;
  int A_Offset_46, A_Offset_56;

  //int A, B, C;
  int C_mn;
  //double x, y, z, a, b;
  double offset_mn;
  double Gain_TIA[RX_LMT_GAIN_END] = {100.0, 50.1187, 25.1189, 12.5893, 6.3096, 3.1623};
  double Gain_BBF[RX_LPF_GAIN_END] = {3.9811, 3.5481, 3.1623, 2.8184, 2.5119, 2.2387,
                                      1.9953, 1.7783, 1.5849, 1.4125, 1.2589, 1.1220, 1.000};
#define RX_FE_CAL_CODE 0x80

  //g_rx_dc_offset_cal_result =0;

  for (i = 0; i < 26; i++)
    bq[i] = 0;

  if (channel == TRX_CHN2) {
    q_rxbbf_offset_tia_reg = 0x6E3;
    i_rxbbf_offset_tia_reg = 0x6E5;
    q_rxbbf_offset_bq_reg  = 0x6E4;
    i_rxbbf_offset_bq_reg  = 0x6E6;
    bbf_gain_reg           = 0x67A;
    lfbuf_reg              = 0x6e0;
  } else {
    q_rxbbf_offset_tia_reg = 0x6D3;
    i_rxbbf_offset_tia_reg = 0x6D5;
    q_rxbbf_offset_bq_reg  = 0x6D4;
    i_rxbbf_offset_bq_reg  = 0x6D6;
    bbf_gain_reg           = 0x65C;
    lfbuf_reg              = 0x6d0;
  }

  reg_val_bak      = hal_spi_read_reg(phy, bbf_gain_reg);
  rx_lfbuf_restore = hal_spi_read_reg(phy, lfbuf_reg);
  /* set default val */
  if (!phy->config->lo_leakage_cal_flag) {
    rx_dc_offset_cal_update_all_gain_tia_lut(phy, channel, RX_FE_CAL_CODE);
    for (i = RX_PORT_G0; i < RX_PORT_GAIN_MAX; i++) {
      if (channel == 0) {
        g_reg_6D5_val[i] = RX_FE_CAL_CODE;
        g_reg_6D3_val[i] = RX_FE_CAL_CODE;
      } else {
        g_reg_6E5_val[i] = RX_FE_CAL_CODE;
        g_reg_6E3_val[i] = RX_FE_CAL_CODE;
      }
    }

    hal_spi_write_reg(phy, q_rxbbf_offset_tia_reg, RX_FE_CAL_CODE);
    hal_spi_write_reg(phy, i_rxbbf_offset_tia_reg, RX_FE_CAL_CODE);
  }

  hal_spi_write_reg(phy, lfbuf_reg, 0xef);

  if (phy->rx_dc_cal_flag[channel]) {
    for (rxfe_gain_index = RX_PORT_GAIN_MAX - 1; rxfe_gain_index >= 0; rxfe_gain_index--) {
      for (i = 0; i < 26; i++) {
        if (channel)
          bq[i] = phy->bq_rx2[rxfe_gain_index][i];
        else
          bq[i] = phy->bq_rx1[rxfe_gain_index][i];
      }

      if (phy->rx_dc_lut_update) {
        rx_dc_offset_cal_update_bq_lut(phy, channel, rxfe_gain_index, bq);
      }
    }
  } else {
#if 1  /// Ok
    for (k = TRX_I; k <= TRX_Q; k++) {
      ///////////  G0 and G1  /////////////
      LOG_MDEBUG(phy, RX_DC_OFFSET_CAL, "Start G0 and G1 ...\n");

      if (channel == 0) {
        hal_spi_write_reg(phy, q_rxbbf_offset_tia_reg, g_reg_6D3_val[0]);
        hal_spi_write_reg(phy, i_rxbbf_offset_tia_reg, g_reg_6D5_val[0]);

      } else {
        hal_spi_write_reg(phy, q_rxbbf_offset_tia_reg, g_reg_6E3_val[0]);
        hal_spi_write_reg(phy, i_rxbbf_offset_tia_reg, g_reg_6E5_val[0]);
      }

      LOG_MDEBUG(phy, RX_DC_CAL, "G0: 0x%03x = 0x%02x, 0x%03x = 0x%02x\n",
                 q_rxbbf_offset_tia_reg, hal_spi_read_reg(phy, q_rxbbf_offset_tia_reg),
                 i_rxbbf_offset_tia_reg, hal_spi_read_reg(phy, i_rxbbf_offset_tia_reg));

      LOG_MDEBUG(phy, RX_DC_OFFSET_CAL, "G0+G0:\n");
      set_rx_port_man(phy, channel, rx_port, RX_LMT_GAIN_G0);
      phy->config->rxfe_gain[channel] = RX_LMT_GAIN_G0;

      get_offset_value_v2(phy, channel, k, RX_LMT_GAIN_G0, RX_LPF_GAIN_G0, &A_Offset_00, &C_00);

      LOG_MDEBUG(phy, RX_DC_OFFSET_CAL, "G0+G12:\n");
      set_rx_port_man(phy, channel, rx_port, RX_LMT_GAIN_G0);
      phy->config->rxfe_gain[channel] = RX_LMT_GAIN_G0;
      get_offset_value_v2(phy, channel, k, RX_LMT_GAIN_G0, RX_LPF_GAIN_G12, &B_Offset_0C, &C_0C);

      LOG_MDEBUG(phy, RX_DC_OFFSET_CAL, "G1+G6:\n");
      set_rx_port_man(phy, channel, rx_port, RX_LMT_GAIN_G1);
      phy->config->rxfe_gain[channel] = RX_LMT_GAIN_G1;
      get_offset_value_v2(phy, channel, k, RX_LMT_GAIN_G1, RX_LPF_GAIN_G6, &C_Offset_16, &C_16);

      if (C_00 > 0x80)
        C_00 = C_00 - 0x80;
      else
        C_00 = -1 * (0x80 - C_00);

      if (C_0C > 0x80)
        C_0C = C_0C - 0x80;
      else
        C_0C = -1 * (0x80 - C_0C);

      if (C_16 > 0x80)
        C_16 = C_16 - 0x80;
      else
        C_16 = -1 * (0x80 - C_16);

      if (C_00 == C_0C)
        C_00 = C_0C + 1;

      x_01 = -0.003354498 * A_Offset_00 - 0.0066931 * B_Offset_0C + 0.0100476 * C_Offset_16;
      y_01 = 1.00476 * B_Offset_0C - 1.00476 * C_Offset_16;
      z_01 = -0.3354498 * A_Offset_00 + 1.3354498 * B_Offset_0C;
      a_01 = ((A_Offset_00 - B_Offset_0C) * 1.000000) / ((C_00 - C_0C) * 1.000000);
      b_01 = (A_Offset_00 * C_0C * 1.000000 - B_Offset_0C * C_00 * 1.000000) / ((C_0C - C_00) * 1.000000);

      for (m = 0; m <= RX_LMT_GAIN_G1; m++) {
        for (n = 0; n < RX_LPF_GAIN_END; n++) {
          offset_mn = -Gain_TIA[m] * Gain_BBF[n] * x_01 - Gain_BBF[n] * y_01 + z_01;
          C_mn      = (offset_mn - b_01) / a_01 + 0x80;
          if (channel) {
            if (k == TRX_I)
              phy->bq_rx2[m][n * 2] = C_mn;
            else
              phy->bq_rx2[m][n * 2 + 1] = C_mn;
          } else {
            if (k == TRX_I)
              phy->bq_rx1[m][n * 2] = C_mn;
            else
              phy->bq_rx1[m][n * 2 + 1] = C_mn;
          }
        }
      }
      //getTime();
      //////////////////////////////////

      ///////////  G2 and G3  /////////////
      LOG_MDEBUG(phy, RX_DC_OFFSET_CAL, "Start G2 and G3 ...\n");

      if (channel == 0) {
        hal_spi_write_reg(phy, q_rxbbf_offset_tia_reg, g_reg_6D3_val[2]);
        hal_spi_write_reg(phy, i_rxbbf_offset_tia_reg, g_reg_6D5_val[2]);

      } else {
        hal_spi_write_reg(phy, q_rxbbf_offset_tia_reg, g_reg_6E3_val[2]);
        hal_spi_write_reg(phy, i_rxbbf_offset_tia_reg, g_reg_6E5_val[2]);
      }

      LOG_MDEBUG(phy, RX_DC_CAL, "G2: 0x%03x = 0x%02x, 0x%03x = 0x%02x\n",
                 q_rxbbf_offset_tia_reg, hal_spi_read_reg(phy, q_rxbbf_offset_tia_reg),
                 i_rxbbf_offset_tia_reg, hal_spi_read_reg(phy, i_rxbbf_offset_tia_reg));

      LOG_MDEBUG(phy, RX_DC_OFFSET_CAL, "G2+G0:\n");
      set_rx_port_man(phy, channel, rx_port, RX_LMT_GAIN_G2);
      phy->config->rxfe_gain[channel] = RX_LMT_GAIN_G2;

      get_offset_value_v2(phy, channel, k, RX_LMT_GAIN_G2, RX_LPF_GAIN_G0, &A_Offset_20, &C_20);

      LOG_MDEBUG(phy, RX_DC_OFFSET_CAL, "G2+G12:\n");
      set_rx_port_man(phy, channel, rx_port, RX_LMT_GAIN_G2);
      phy->config->rxfe_gain[channel] = RX_LMT_GAIN_G2;
      get_offset_value_v2(phy, channel, k, RX_LMT_GAIN_G2, RX_LPF_GAIN_G12, &B_Offset_2C, &C_2C);

      LOG_MDEBUG(phy, RX_DC_OFFSET_CAL, "G3+G0:\n");
      set_rx_port_man(phy, channel, rx_port, RX_LMT_GAIN_G3);
      phy->config->rxfe_gain[channel] = RX_LMT_GAIN_G3;
      get_offset_value_v2(phy, channel, k, RX_LMT_GAIN_G3, RX_LPF_GAIN_G0, &C_Offset_30, &C_30);

      if (C_20 > 0x80)
        C_20 = C_20 - 0x80;
      else
        C_20 = -1 * (0x80 - C_20);

      if (C_2C > 0x80)
        C_2C = C_2C - 0x80;
      else
        C_2C = -1 * (0x80 - C_2C);

      if (C_30 > 0x80)
        C_30 = C_30 - 0x80;
      else
        C_30 = -1 * (0x80 - C_30);

      if (C_20 == C_2C)
        C_20 = C_2C + 1;

      x_23 = -0.0200476 * A_Offset_20 + 0.0200476 * C_Offset_30;
      y_23 = 0.16812317 * A_Offset_20 + 0.33544983 * B_Offset_2C - 0.5036573 * C_Offset_30;
      z_23 = -0.3354498 * A_Offset_20 + 1.33544983 * B_Offset_2C;
      a_23 = ((A_Offset_20 - B_Offset_2C) * 1.000000) / ((C_20 - C_2C) * 1.000000);
      b_23 = (A_Offset_20 * C_2C * 1.000000 - B_Offset_2C * C_20 * 1.000000) / ((C_2C - C_20) * 1.000000);

      for (m = RX_LMT_GAIN_G2; m <= RX_LMT_GAIN_G3; m++) {
        for (n = 0; n < RX_LPF_GAIN_END; n++) {
          offset_mn = -Gain_TIA[m] * Gain_BBF[n] * x_23 - Gain_BBF[n] * y_23 + z_23;
          C_mn      = (offset_mn - b_23) / a_23 + 0x80;
          if (channel) {
            if (k == TRX_I)
              phy->bq_rx2[m][n * 2] = C_mn;
            else
              phy->bq_rx2[m][n * 2 + 1] = C_mn;
          } else {
            if (k == TRX_I)
              phy->bq_rx1[m][n * 2] = C_mn;
            else
              phy->bq_rx1[m][n * 2 + 1] = C_mn;
          }
        }
      }
      //getTime();
      //////////////////////////////////

      ///////////  G4 and G5  /////////////
      LOG_MDEBUG(phy, RX_DC_OFFSET_CAL, "Start G4 and G5 ...\n");

      if (channel == 0) {
        hal_spi_write_reg(phy, q_rxbbf_offset_tia_reg, g_reg_6D3_val[4]);
        hal_spi_write_reg(phy, i_rxbbf_offset_tia_reg, g_reg_6D5_val[4]);

      } else {
        hal_spi_write_reg(phy, q_rxbbf_offset_tia_reg, g_reg_6E3_val[4]);
        hal_spi_write_reg(phy, i_rxbbf_offset_tia_reg, g_reg_6E5_val[4]);
      }

      LOG_MDEBUG(phy, RX_DC_CAL, "G4: 0x%03x = 0x%02x, 0x%03x = 0x%02x\n",
                 q_rxbbf_offset_tia_reg, hal_spi_read_reg(phy, q_rxbbf_offset_tia_reg),
                 i_rxbbf_offset_tia_reg, hal_spi_read_reg(phy, i_rxbbf_offset_tia_reg));

      LOG_MDEBUG(phy, RX_DC_OFFSET_CAL, "G4+G0:\n");
      set_rx_port_man(phy, channel, rx_port, RX_LMT_GAIN_G4);
      phy->config->rxfe_gain[channel] = RX_LMT_GAIN_G4;

      get_offset_value_v2(phy, channel, k, RX_LMT_GAIN_G4, RX_LPF_GAIN_G0, &A_Offset_40, &C_40);

      LOG_MDEBUG(phy, RX_DC_OFFSET_CAL, "G4+G12:\n");
      set_rx_port_man(phy, channel, rx_port, RX_LMT_GAIN_G4);
      phy->config->rxfe_gain[channel] = RX_LMT_GAIN_G4;
      get_offset_value_v2(phy, channel, k, RX_LMT_GAIN_G4, RX_LPF_GAIN_G12, &B_Offset_4C, &C_4C);

      LOG_MDEBUG(phy, RX_DC_OFFSET_CAL, "G5+G0:\n");
      set_rx_port_man(phy, channel, rx_port, RX_LMT_GAIN_G5);
      phy->config->rxfe_gain[channel] = RX_LMT_GAIN_G5;
      get_offset_value_v2(phy, channel, k, RX_LMT_GAIN_G5, RX_LPF_GAIN_G0, &C_Offset_50, &C_50);
      if (C_40 > 0x80)
        C_40 = C_40 - 0x80;
      else
        C_40 = -1 * (0x80 - C_40);

      if (C_4C > 0x80)
        C_4C = C_4C - 0x80;
      else
        C_4C = -1 * (0x80 - C_4C);

      if (C_50 > 0x80)
        C_50 = C_50 - 0x80;
      else
        C_50 = -1 * (0x80 - C_50);

      if (C_40 == C_4C)
        C_40 = C_4C + 1;

      x_45 = -0.07981094 * A_Offset_40 + 0.07981094 * C_Offset_50;
      y_45 = 0.1681231728 * A_Offset_40 + 0.33544983 * B_Offset_4C - 0.5036573 * C_Offset_50;
      z_45 = -0.3354498 * A_Offset_40 + 1.33544983 * B_Offset_4C;
      a_45 = ((A_Offset_40 - B_Offset_4C) * 1.000000) / ((C_40 - C_4C) * 1.000000);
      b_45 = (A_Offset_40 * C_4C * 1.000000 - B_Offset_4C * C_40 * 1.000000) / ((C_4C - C_40) * 1.000000);

      for (m = RX_LMT_GAIN_G4; m <= RX_LMT_GAIN_G5; m++) {
        for (n = 0; n < RX_LPF_GAIN_END; n++) {
          offset_mn = -Gain_TIA[m] * Gain_BBF[n] * x_45 - Gain_BBF[n] * y_45 + z_45;
          C_mn      = (offset_mn - b_45) / a_45 + 0x80;
          if (channel) {
            if (k == TRX_I)
              phy->bq_rx2[m][n * 2] = C_mn;
            else
              phy->bq_rx2[m][n * 2 + 1] = C_mn;
          } else {
            if (k == TRX_I)
              phy->bq_rx1[m][n * 2] = C_mn;
            else
              phy->bq_rx1[m][n * 2 + 1] = C_mn;
          }
        }
      }
      //getTime();
      //////////////////////////////////
    }

    if (phy->rx_dc_lut_update) {
      LOG_MDEBUG(phy, RX_DC_OFFSET_CAL, "rx_dc_lut_update\n");
      for (rxfe_gain_index = RX_PORT_GAIN_MAX - 1; rxfe_gain_index >= 0; rxfe_gain_index--) {
        for (i = 0; i < 26; i++) {
          if (channel)
            bq[i] = phy->bq_rx2[rxfe_gain_index][i];
          else
            bq[i] = phy->bq_rx1[rxfe_gain_index][i];
        }

        if (phy->rx_dc_lut_update)
          rx_dc_offset_cal_update_bq_lut(phy, channel, rxfe_gain_index, bq);
      }
    }
    phy->rx_dc_cal_flag[channel] = 1;
#endif
  }
#if 1
  //set the value rxfe g2 rxbbf g6 for qec
  set_rx_port_man(phy, channel, rx_port, RX_LMT_GAIN_G2);
  hal_spi_write_reg(phy, bbf_gain_reg, 6);
  if (channel) {
    hal_spi_write_reg(phy, i_rxbbf_offset_bq_reg, phy->bq_rx2[2][12]);
    hal_spi_write_reg(phy, q_rxbbf_offset_bq_reg, phy->bq_rx2[2][13]);
  } else {
    hal_spi_write_reg(phy, i_rxbbf_offset_bq_reg, phy->bq_rx1[2][12]);
    hal_spi_write_reg(phy, q_rxbbf_offset_bq_reg, phy->bq_rx1[2][13]);
  }

  hal_spi_write_reg(phy, lfbuf_reg, rx_lfbuf_restore);
#endif

  return 0;
}

int rx_dc_offset_cal_v2(rf_chip_phy_t* phy, TRX_CHN_ENUM chn) {
  int rg702_val, rg74A_val;
  unsigned short lut702_val, lut74A_val;

  LOG_MDEBUG(phy, RX_DC_OFFSET_CAL, "Rx dc offst calibration v2 start\n");
  if ((phy->config->mode >= CH1_FDD) && (phy->config->mode <= CH1_CH2_FDD)) {
    set_trx_lo(phy, RX_DIR, chn, phy->config->rx_flo);
    CHIP_DELAY(1);
    if (rx_dc_offset_cal_in_v2(phy, chn, phy->config->rx_port[chn]) < 0) {
      phy->error = -RX_DC_CAL_FAIL;
      return -RX_DC_CAL_FAIL;
    }
  } else {
    if (chn == TRX_CHN1) {
      rg702_val = hal_spi_read_reg(phy, 0x702) & 0xFF;
      read_lut_byte(phy, MAIN_ENSM_LUT, 19, 0x702, 197, 0, (unsigned char*)&lut702_val);
      hal_spi_write_reg(phy, 0x702, (unsigned char)lut702_val);
    } else {
      rg74A_val = hal_spi_read_reg(phy, 0x74A) & 0xFF;
      read_lut_byte(phy, MAIN_ENSM_LUT, 19, 0x74A, 198, 3, (unsigned char*)&lut74A_val);
      hal_spi_write_reg(phy, 0x74A, (unsigned char)lut74A_val);
    }

    set_trx_lo(phy, TX_DIR, chn, phy->config->tx_flo);
    CHIP_DELAY(1);
    if (rx_dc_offset_cal_in_v2(phy, chn, phy->config->rx_port[chn]) < 0) {
      phy->error = -RX_DC_CAL_FAIL;
      return -RX_DC_CAL_FAIL;
    }
    hal_spi_write_reg(phy, 0x660, 0x0a);

    if (chn == TRX_CHN1) {
      hal_spi_write_reg(phy, 0x702, rg702_val & 0xFF);
    } else {
      hal_spi_write_reg(phy, 0x74A, rg74A_val & 0xFF);
    }
  }

  LOG_MDEBUG(phy, RX_DC_OFFSET_CAL, "Rx dc offst calibration end\n");
  return 0;
}

#if 1

static int do_rx_dc_offset_cal_v3(rf_chip_phy_t* phy, int channel, TRX_IQ_ENUM iq,
                                  unsigned int tune_reg, int flag, int ext, int* offsetVal, int* calCode) {
  unsigned int i_ofst_reg1, i_ofst_reg2;
  unsigned int q_ofst_reg1, q_ofst_reg2;
  int tmp_flag, i;
  char bit;
  int dc_offset_i = 0;
  int dc_offset_q = 0;
  int val;
  int ret = 0;
  int tune_reg_val_bak, dc_offset_bak;

  if (flag)
    tmp_flag = 0;
  else
    tmp_flag = 1;

  if (channel == TRX_CHN2) {
    i_ofst_reg1 = 0x052;
    i_ofst_reg2 = 0x053;
    q_ofst_reg1 = 0x054;
    q_ofst_reg2 = 0x055;
  } else {
    i_ofst_reg1 = 0x04E;
    i_ofst_reg2 = 0x04F;
    q_ofst_reg1 = 0x050;
    q_ofst_reg2 = 0x051;
  }

  //getTime();
  i = 7;
  while (i >= 0) {
    ret = rx_get_adc_offset(phy, channel, 2, &dc_offset_i, &dc_offset_q);
    if (ret < 0)
      return ret;
    val = hal_spi_read_reg(phy, tune_reg);
    if (val == 0x80) {
      if (iq == TRX_I)
        *offsetVal = dc_offset_i;
      else
        *offsetVal = dc_offset_q;
    }

    LOG_MDEBUG(phy, RX_DC_OFFSET_CAL, "%d,tune reg[%C] 0x%x = 0x%02x, dc_offset_i_val=%d, dc_offst_q_val=%d\n",
               i, (iq == TRX_I) ? 'I' : 'Q', tune_reg, hal_spi_read_reg(phy, tune_reg), dc_offset_i, dc_offset_q);
    if (i == 1) {
      tune_reg_val_bak = hal_spi_read_reg(phy, tune_reg);
      if (iq == TRX_I)
        dc_offset_bak = dc_offset_i;
      else
        dc_offset_bak = dc_offset_q;
    }
    if (i == 0) {
      if (iq == TRX_I) {
        if (abs(dc_offset_bak) < abs(dc_offset_i)) {
          hal_spi_write_reg(phy, tune_reg, tune_reg_val_bak);
        }
      } else {
        if (abs(dc_offset_bak) < abs(dc_offset_i)) {
          hal_spi_write_reg(phy, tune_reg, tune_reg_val_bak);
        }
      }

      break;
    }

    rx_dc_offset_cal_enable(phy, channel, 0);
    CHIP_UDELAY(5);  //50us
    rx_dc_offset_cal_enable(phy, channel, 1);
    if (rx_dc_offset_cal_status(phy, channel) == 0)
      return -1;

    if (iq == TRX_I)
      bit = HAL_REG_GET_BIT(phy, i_ofst_reg1, BIT1);
    else
      bit = HAL_REG_GET_BIT(phy, q_ofst_reg1, BIT1);

    if (bit == tmp_flag)
      HAL_REG_CLR_BIT(phy, tune_reg, i);
    else
      HAL_REG_SET_BIT(phy, tune_reg, i);

    if (i > 0)
      HAL_REG_SET_BIT(phy, tune_reg, i - 1);

    i = i - 1;
  }
  //getTime();

  *calCode = hal_spi_read_reg(phy, tune_reg);
  rx_get_adc_offset(phy, channel, 2, &dc_offset_i, &dc_offset_q);
  LOG_MDEBUG(phy, RX_DC_OFFSET_CAL, "Fine tune reg[%C] 0x%x = 0x%02x, dc_offset_i_val=%d, dc_offst_q_val=%d\n",
             (iq == TRX_I) ? 'I' : 'Q', tune_reg, *calCode, dc_offset_i, dc_offset_q);
  return 0;
}

static int handle_rx_dc_offset_cal_v3(rf_chip_phy_t* phy, int channel, TRX_IQ_ENUM iq,
                                      unsigned int tune_reg, int flag, int ext, int* offsetVal, int* calCode) {
  unsigned int rxdac_ctrl_reg;
  unsigned int rxbb_vcmsel_reg;
  unsigned int i_ofst_reg;
  int i, j, tmp, reg_val;

  if (channel == TRX_CHN2) {
    i_ofst_reg      = 0x052;
    rxdac_ctrl_reg  = 0x6E2;
    rxbb_vcmsel_reg = 0x685;
  } else {
    i_ofst_reg      = 0x04E;
    rxdac_ctrl_reg  = 0x6D2;
    rxbb_vcmsel_reg = 0x667;
  }

  hal_spi_write_reg(phy, 0x0DD, 0xF3);
  hal_spi_write_reg(phy, i_ofst_reg, 0xFF);

  reg_val = hal_spi_read_reg(phy, rxbb_vcmsel_reg);
  reg_val = (reg_val & 0x3) << 2;

  tmp     = hal_spi_read_reg(phy, rxdac_ctrl_reg);
  tmp     = tmp & ((~(0x3 << 2)) & 0xFF);
  reg_val = reg_val | tmp;
  hal_spi_write_reg(phy, rxdac_ctrl_reg, reg_val);

  rx_dc_offset_cal_enable(phy, channel, 1);
  /* check cali status */
  if (rx_dc_offset_cal_status(phy, channel) == 0)
    return -1;

  /* cal */
  if (do_rx_dc_offset_cal_v3(phy, channel, iq, tune_reg, flag, ext, offsetVal, calCode) < 0)
    return -1;

  hal_spi_write_reg(phy, 0x0DD, 0xF0);
  rx_dc_offset_cal_enable(phy, channel, 0);
  return 0;
}

int get_offset_by_code_v3(rf_chip_phy_t* phy, int channel, TRX_IQ_ENUM iq, unsigned int i_tune_reg,
                          unsigned int q_tune_reg, int* i_offsetVal, int* q_offsetVal, int i_Code, int q_Code) {
  unsigned int rxdac_ctrl_reg;
  unsigned int rxbb_vcmsel_reg;
  unsigned int i_ofst_reg;
  int i, j, tmp, reg_val;
  int dc_offset_i = 0;
  int dc_offset_q = 0;
  long result;

  if (channel == TRX_CHN2) {
    i_ofst_reg      = 0x052;
    rxdac_ctrl_reg  = 0x6E2;
    rxbb_vcmsel_reg = 0x685;
  } else {
    i_ofst_reg      = 0x04E;
    rxdac_ctrl_reg  = 0x6D2;
    rxbb_vcmsel_reg = 0x667;
  }

  hal_spi_write_reg(phy, 0x0DD, 0xF3);
  hal_spi_write_reg(phy, i_ofst_reg, 0xFF);

  reg_val = hal_spi_read_reg(phy, rxbb_vcmsel_reg);
  reg_val = (reg_val & 0x3) << 2;

  tmp     = hal_spi_read_reg(phy, rxdac_ctrl_reg);
  tmp     = tmp & ((~(0x3 << 2)) & 0xFF);
  reg_val = reg_val | tmp;
  hal_spi_write_reg(phy, rxdac_ctrl_reg, reg_val);

  rx_dc_offset_cal_enable(phy, channel, 1);

  /* check cali status */
  if (rx_dc_offset_cal_status(phy, channel) == 0)
    return -1;

  /* cal */
  hal_spi_write_reg(phy, i_tune_reg, i_Code);
  hal_spi_write_reg(phy, q_tune_reg, q_Code);
  rx_get_adc_offset(phy, channel, 2, &dc_offset_i, &dc_offset_q);
  *i_offsetVal = dc_offset_i;
  *q_offsetVal = dc_offset_q;

  hal_spi_write_reg(phy, 0x0DD, 0xF0);
  rx_dc_offset_cal_enable(phy, channel, 0);
  return 0;
}

static int get_offset_value_v3(rf_chip_phy_t* phy, int channel, RX_LMT_GAIN_ENUM lmt_gain,
                               RX_LPF_GAIN_ENUM lpt_gain, int* i_offsetVal, int* i_calCode, int* q_offsetVal, int* q_calCode) {
  int dc_offset_i, dc_offset_q;
  unsigned int q_rxbbf_offset_tia_reg, i_rxbbf_offset_tia_reg;
  unsigned int q_rxbbf_offset_bq_reg, i_rxbbf_offset_bq_reg;
  unsigned int bbf_gain_reg, lfbuf_reg, reg_val_bak, rx_lfbuf_restore, init_ival;

  int C_00_i, C_0C_i, C_16_i;
  int A_Offset_00_i, B_Offset_0C_i, C_Offset_16_i;
  double x_01_i, y_01_i, z_01_i, a_01_i, b_01_i;

  int C_00_q, C_0C_q, C_16_q;
  int A_Offset_00_q, B_Offset_0C_q, C_Offset_16_q;
  double x_01_q, y_01_q, z_01_q, a_01_q, b_01_q;

  //int i_calCode, q_calCode;
  int i, j;
  int i_tmpOffset, q_tmpOffset;
  int i_tmpOffset_min, q_tmpOffset_min;
  int i_fine_code, q_fine_code;
  long offset_min;
  long offset_result;

  if (channel == TRX_CHN2) {
    q_rxbbf_offset_tia_reg = 0x6E3;
    i_rxbbf_offset_tia_reg = 0x6E5;
    q_rxbbf_offset_bq_reg  = 0x6E4;
    i_rxbbf_offset_bq_reg  = 0x6E6;
    bbf_gain_reg           = 0x67A;
    lfbuf_reg              = 0x6e0;
  } else {
    q_rxbbf_offset_tia_reg = 0x6D3;
    i_rxbbf_offset_tia_reg = 0x6D5;
    q_rxbbf_offset_bq_reg  = 0x6D4;
    i_rxbbf_offset_bq_reg  = 0x6D6;
    bbf_gain_reg           = 0x65C;
    lfbuf_reg              = 0x6d0;
  }

  hal_spi_write_reg(phy, bbf_gain_reg, lpt_gain);
  {
    ///  I Path
    hal_spi_write_reg(phy, i_rxbbf_offset_bq_reg, 0x80);
    if (handle_rx_dc_offset_cal_v3(phy, channel, TRX_I, i_rxbbf_offset_bq_reg, 1, 0, i_offsetVal, i_calCode) < 0)
      return -1;
  }

  {
    /// Q  Path
    hal_spi_write_reg(phy, q_rxbbf_offset_bq_reg, 0x80);
    if (handle_rx_dc_offset_cal_v3(phy, channel, TRX_Q, q_rxbbf_offset_bq_reg, 1, 0, q_offsetVal, q_calCode) < 0)
      return -1;

    //spi_read_reg(q_rxbbf_offset_bq_reg);
  }

  i_tmpOffset_min = 99999;
  i_fine_code     = 99999;
  q_tmpOffset_min = 99999;
  q_fine_code     = 99999;
  offset_min      = 99999;
  for (i = *i_calCode - 1; i <= *i_calCode + 1; i++) {
    for (j = *q_calCode - 1; j <= *q_calCode + 1; j++) {
      get_offset_by_code_v3(phy, channel, TRX_I, i_rxbbf_offset_bq_reg, q_rxbbf_offset_bq_reg, &i_tmpOffset, &q_tmpOffset, i, j);
      offset_result = i_tmpOffset * i_tmpOffset + q_tmpOffset * q_tmpOffset;
      if (offset_result < offset_min) {
        offset_min  = offset_result;
        i_fine_code = i;
        q_fine_code = j;
      }
    }
  }
  *i_calCode = i_fine_code;
  *q_calCode = q_fine_code;
  return 0;
}

static int rx_dc_offset_cal_in_v3(rf_chip_phy_t* phy, int channel, RX_PORT_ENUM rx_port) {
  unsigned int q_rxbbf_offset_tia_reg, i_rxbbf_offset_tia_reg;
  unsigned int q_rxbbf_offset_bq_reg, i_rxbbf_offset_bq_reg;
  int j, i, m, n, k;

  unsigned short rxdac_ctrl_reg;
  unsigned short rxbb_vcmsel_reg;
  unsigned short i_ofst_reg;

  short tmp, reg_val;
  int rxfe_gain_index;
  //unsigned int tia[RX_PORT_GAIN_MAX * 2];
  unsigned int bq[26];
  unsigned int bbf_gain_reg, lfbuf_reg, reg_val_bak, rx_lfbuf_restore, init_ival;
  int C_00, C_0C, C_16;
  int A_Offset_00, B_Offset_0C, C_Offset_16;
  double x_01, y_01, z_01, a_01, b_01;

  int C_20, C_2C, C_30;
  int A_Offset_20, B_Offset_2C, C_Offset_30;
  double x_23, y_23, z_23, a_23, b_23;

  int C_40, C_4C, C_50;
  int A_Offset_40, B_Offset_4C, C_Offset_50;
  double x_45, y_45, z_45, a_45, b_45;

  int C_46, C_56;
  int A_Offset_46, A_Offset_56;

  int C_00_i, C_0C_i, C_16_i;
  int A_Offset_00_i, B_Offset_0C_i, C_Offset_16_i;
  double x_01_i, y_01_i, z_01_i, a_01_i, b_01_i;

  int C_00_q, C_0C_q, C_16_q;
  int A_Offset_00_q, B_Offset_0C_q, C_Offset_16_q;
  double x_01_q, y_01_q, z_01_q, a_01_q, b_01_q;

  int A, B, C, Code_1, Code_2, Code_3;
  double x, y, z, a, b;
  //int A, B, C;
  int C_mn;
  double offset_mn;
  double Gain_TIA[RX_LMT_GAIN_END] = {100.0, 50.1187, 25.1189, 12.5893, 6.3096, 3.1623};
  double Gain_BBF[RX_LPF_GAIN_END] = {3.9811, 3.5481, 3.1623, 2.8184, 2.5119, 2.2387,
                                      1.9953, 1.7783, 1.5849, 1.4125, 1.2589, 1.1220, 1.000};
#define RX_FE_CAL_CODE 0x80

  //g_rx_dc_offset_cal_result =0;

  for (i = 0; i < 26; i++)
    bq[i] = 0;

  if (channel == TRX_CHN2) {
    q_rxbbf_offset_tia_reg = 0x6E3;
    i_rxbbf_offset_tia_reg = 0x6E5;
    q_rxbbf_offset_bq_reg  = 0x6E4;
    i_rxbbf_offset_bq_reg  = 0x6E6;
    i_ofst_reg             = 0x04E;
    bbf_gain_reg           = 0x67A;
    lfbuf_reg              = 0x6e0;

    i_ofst_reg      = 0x04E;
    rxdac_ctrl_reg  = 0x6D2;
    rxbb_vcmsel_reg = 0x667;
  } else {
    q_rxbbf_offset_tia_reg = 0x6D3;
    i_rxbbf_offset_tia_reg = 0x6D5;
    q_rxbbf_offset_bq_reg  = 0x6D4;
    i_rxbbf_offset_bq_reg  = 0x6D6;
    bbf_gain_reg           = 0x65C;
    lfbuf_reg              = 0x6d0;

    i_ofst_reg      = 0x052;
    rxdac_ctrl_reg  = 0x6E2;
    rxbb_vcmsel_reg = 0x685;
  }

  reg_val_bak      = hal_spi_read_reg(phy, bbf_gain_reg);
  rx_lfbuf_restore = hal_spi_read_reg(phy, lfbuf_reg);

  if (!phy->config->lo_leakage_cal_flag) {
    rx_dc_offset_cal_update_all_gain_tia_lut(phy, channel, RX_FE_CAL_CODE);
    for (i = RX_PORT_G0; i < RX_PORT_GAIN_MAX; i++) {
      if (channel == 0) {
        g_reg_6D5_val[i] = RX_FE_CAL_CODE;
        g_reg_6D3_val[i] = RX_FE_CAL_CODE;
      } else {
        g_reg_6E5_val[i] = RX_FE_CAL_CODE;
        g_reg_6E3_val[i] = RX_FE_CAL_CODE;
      }
    }

    hal_spi_write_reg(phy, q_rxbbf_offset_tia_reg, RX_FE_CAL_CODE);
    hal_spi_write_reg(phy, i_rxbbf_offset_tia_reg, RX_FE_CAL_CODE);
  }

  hal_spi_write_reg(phy, lfbuf_reg, 0xef);

  if (phy->rx_dc_cal_flag[channel]) {
    for (rxfe_gain_index = RX_PORT_GAIN_MAX - 1; rxfe_gain_index >= 0; rxfe_gain_index--) {
      for (i = 0; i < 26; i++) {
        if (channel)
          bq[i] = phy->bq_rx2[rxfe_gain_index][i];
        else
          bq[i] = phy->bq_rx1[rxfe_gain_index][i];
      }

      if (phy->rx_dc_lut_update) {
        rx_dc_offset_cal_update_bq_lut(phy, channel, rxfe_gain_index, bq);
      }
    }
  } else {

    hal_spi_write_reg(phy, 0x0DD, 0xF3);
    hal_spi_write_reg(phy, i_ofst_reg, 0xFF);

    reg_val = hal_spi_read_reg(phy, rxbb_vcmsel_reg);
    reg_val = (reg_val & 0x3) << 2;

    tmp     = hal_spi_read_reg(phy, rxdac_ctrl_reg);
    tmp     = tmp & ((~(0x3 << 2)) & 0xFF);
    reg_val = reg_val | tmp;
    hal_spi_write_reg(phy, rxdac_ctrl_reg, reg_val);

#if 1  ///test
    ///////////  G0 and G1  /////////////
    LOG_MDEBUG(phy, RX_DC_OFFSET_CAL, "Start V3 ...\n");
    //getTime();
    if (channel == 0) {
      hal_spi_write_reg(phy, q_rxbbf_offset_tia_reg, g_reg_6D3_val[0]);
      hal_spi_write_reg(phy, i_rxbbf_offset_tia_reg, g_reg_6D5_val[0]);

    } else {
      hal_spi_write_reg(phy, q_rxbbf_offset_tia_reg, g_reg_6E3_val[0]);
      hal_spi_write_reg(phy, i_rxbbf_offset_tia_reg, g_reg_6E5_val[0]);
    }

    LOG_MDEBUG(phy, RX_DC_CAL, "G0: 0x%03x = 0x%02x, 0x%03x = 0x%02x\n",
               q_rxbbf_offset_tia_reg, hal_spi_read_reg(phy, q_rxbbf_offset_tia_reg),
               i_rxbbf_offset_tia_reg, hal_spi_read_reg(phy, i_rxbbf_offset_tia_reg));

    LOG_MDEBUG(phy, RX_DC_OFFSET_CAL, "G0+G0:\n");
    set_rx_port_man(phy, channel, rx_port, RX_LMT_GAIN_G0);
    phy->config->rxfe_gain[channel] = RX_LMT_GAIN_G0;

    get_offset_value_v3(phy, channel, RX_LMT_GAIN_G0, RX_LPF_GAIN_G0, &A_Offset_00_i, &C_00_i, &A_Offset_00_q, &C_00_q);
    //getTime();
    LOG_MDEBUG(phy, RX_DC_OFFSET_CAL, "G0+G12:\n");
    set_rx_port_man(phy, channel, rx_port, RX_LMT_GAIN_G0);
    phy->config->rxfe_gain[channel] = RX_LMT_GAIN_G0;
    get_offset_value_v3(phy, channel, RX_LMT_GAIN_G0, RX_LPF_GAIN_G12, &B_Offset_0C_i, &C_0C_i, &B_Offset_0C_q, &C_0C_q);
    //getTime();

    LOG_MDEBUG(phy, RX_DC_OFFSET_CAL, "G1+G6:\n");
    set_rx_port_man(phy, channel, rx_port, RX_LMT_GAIN_G1);
    phy->config->rxfe_gain[channel] = RX_LMT_GAIN_G1;
    get_offset_value_v3(phy, channel, RX_LMT_GAIN_G1, RX_LPF_GAIN_G6, &C_Offset_16_i, &C_16_i, &C_Offset_16_q, &C_16_q);
    for (k = TRX_I; k <= TRX_Q; k++) {
      if (k == TRX_I) {
        A      = A_Offset_00_i;
        B      = B_Offset_0C_i;
        C      = C_Offset_16_i;
        Code_1 = C_00_i;
        Code_2 = C_0C_i;
        Code_3 = C_16_i;
      } else {
        A      = A_Offset_00_q;
        B      = B_Offset_0C_q;
        C      = C_Offset_16_q;
        Code_1 = C_00_q;
        Code_2 = C_0C_q;
        Code_3 = C_16_q;
      }
      if (Code_1 > 0x80)
        Code_1 = Code_1 - 0x80;
      else
        Code_1 = -1 * (0x80 - Code_1);

      if (Code_2 > 0x80)
        Code_2 = Code_2 - 0x80;
      else
        Code_2 = -1 * (0x80 - Code_2);

      if (Code_3 > 0x80)
        Code_3 = Code_3 - 0x80;
      else
        Code_3 = -1 * (0x80 - Code_3);

      if (Code_1 == Code_2)
        Code_1 = Code_2 + 1;

      x = -0.003354498 * A - 0.0066931 * B + 0.0100476 * C;
      y = 1.00476 * B - 1.00476 * C;
      z = -0.3354498 * A + 1.3354498 * B;
      a = ((A - B) * 1.000000) / ((Code_1 - Code_2) * 1.000000);
      b = (A * Code_2 * 1.000000 - B * Code_1 * 1.000000) / ((Code_2 - Code_1) * 1.000000);

      for (m = RX_LMT_GAIN_G0; m <= RX_LMT_GAIN_G1; m++) {
        for (n = 0; n < RX_LPF_GAIN_END; n++) {
          offset_mn = -Gain_TIA[m] * Gain_BBF[n] * x - Gain_BBF[n] * y + z;
          C_mn      = (offset_mn - b) / a + 0x80;
          if (channel) {
            if (k == TRX_I)
              phy->bq_rx2[m][n * 2] = C_mn;
            else
              phy->bq_rx2[m][n * 2 + 1] = C_mn;

          } else {
            if (k == TRX_I)
              phy->bq_rx1[m][n * 2] = C_mn;
            else
              phy->bq_rx1[m][n * 2 + 1] = C_mn;
          }
        }
      }
    }
    //////////////////////////////////
    ///////////  G2 and G3  ////////////

    LOG_MDEBUG(phy, RX_DC_OFFSET_CAL, "Start V3 ...\n");
    if (channel == 0) {
      hal_spi_write_reg(phy, q_rxbbf_offset_tia_reg, g_reg_6D3_val[2]);
      hal_spi_write_reg(phy, i_rxbbf_offset_tia_reg, g_reg_6D5_val[2]);

    } else {
      hal_spi_write_reg(phy, q_rxbbf_offset_tia_reg, g_reg_6E3_val[2]);
      hal_spi_write_reg(phy, i_rxbbf_offset_tia_reg, g_reg_6E5_val[2]);
    }

    LOG_MDEBUG(phy, RX_DC_CAL, "G2: 0x%03x = 0x%02x, 0x%03x = 0x%02x\n",
               q_rxbbf_offset_tia_reg, hal_spi_read_reg(phy, q_rxbbf_offset_tia_reg),
               i_rxbbf_offset_tia_reg, hal_spi_read_reg(phy, i_rxbbf_offset_tia_reg));

    //getTime();
    LOG_MDEBUG(phy, RX_DC_OFFSET_CAL, "G2+G0:\n");
    set_rx_port_man(phy, channel, rx_port, RX_LMT_GAIN_G2);
    phy->config->rxfe_gain[channel] = RX_LMT_GAIN_G2;

    get_offset_value_v3(phy, channel, RX_LMT_GAIN_G2, RX_LPF_GAIN_G0, &A_Offset_00_i, &C_00_i, &A_Offset_00_q, &C_00_q);
    //getTime();
    LOG_MDEBUG(phy, RX_DC_OFFSET_CAL, "G2+G12:\n");
    set_rx_port_man(phy, channel, rx_port, RX_LMT_GAIN_G2);
    phy->config->rxfe_gain[channel] = RX_LMT_GAIN_G2;
    get_offset_value_v3(phy, channel, RX_LMT_GAIN_G2, RX_LPF_GAIN_G12, &B_Offset_0C_i, &C_0C_i, &B_Offset_0C_q, &C_0C_q);
    //getTime();

    LOG_MDEBUG(phy, RX_DC_OFFSET_CAL, "G3+G0:\n");
    set_rx_port_man(phy, channel, rx_port, RX_LMT_GAIN_G3);
    phy->config->rxfe_gain[channel] = RX_LMT_GAIN_G3;
    get_offset_value_v3(phy, channel, RX_LMT_GAIN_G3, RX_LPF_GAIN_G0, &C_Offset_16_i, &C_16_i, &C_Offset_16_q, &C_16_q);

    //getTime();
    for (k = TRX_I; k <= TRX_Q; k++) {
      if (k == TRX_I) {
        A      = A_Offset_00_i;
        B      = B_Offset_0C_i;
        C      = C_Offset_16_i;
        Code_1 = C_00_i;
        Code_2 = C_0C_i;
        Code_3 = C_16_i;
      } else {
        A      = A_Offset_00_q;
        B      = B_Offset_0C_q;
        C      = C_Offset_16_q;
        Code_1 = C_00_q;
        Code_2 = C_0C_q;
        Code_3 = C_16_q;
      }

      if (Code_1 > 0x80)
        Code_1 = Code_1 - 0x80;
      else
        Code_1 = -1 * (0x80 - Code_1);

      if (Code_2 > 0x80)
        Code_2 = Code_2 - 0x80;
      else
        Code_2 = -1 * (0x80 - Code_2);

      if (Code_3 > 0x80)
        Code_3 = Code_3 - 0x80;
      else
        Code_3 = -1 * (0x80 - Code_3);

      if (Code_1 == Code_2)
        Code_1 = Code_2 + 1;

      x = -0.0200476 * A + 0.0200476 * C;
      y = 0.16812317 * A + 0.33544983 * B - 0.5036573 * C;
      z = -0.3354498 * A + 1.33544983 * B;
      a = ((A - B) * 1.000000) / ((Code_1 - Code_2) * 1.000000);
      b = (A * Code_2 * 1.000000 - B * Code_1 * 1.000000) / ((Code_2 - Code_1) * 1.000000);

      for (m = RX_LMT_GAIN_G2; m <= RX_LMT_GAIN_G3; m++) {
        for (n = 0; n < RX_LPF_GAIN_END; n++) {
          offset_mn = -Gain_TIA[m] * Gain_BBF[n] * x - Gain_BBF[n] * y + z;
          C_mn      = (offset_mn - b) / a + 0x80;
          if (channel) {
            if (k == TRX_I)
              phy->bq_rx2[m][n * 2] = C_mn;
            else
              phy->bq_rx2[m][n * 2 + 1] = C_mn;
          } else {
            if (k == TRX_I)
              phy->bq_rx1[m][n * 2] = C_mn;
            else
              phy->bq_rx1[m][n * 2 + 1] = C_mn;
          }
        }
      }
      //getTime();
    }
    //////////////////////////////////
    ///////////  G4 and G5  /////////////
    LOG_MDEBUG(phy, RX_DC_OFFSET_CAL, "Start V3 ...\n");
    if (channel == 0) {
      hal_spi_write_reg(phy, q_rxbbf_offset_tia_reg, g_reg_6D3_val[4]);
      hal_spi_write_reg(phy, i_rxbbf_offset_tia_reg, g_reg_6D5_val[4]);

    } else {
      hal_spi_write_reg(phy, q_rxbbf_offset_tia_reg, g_reg_6E3_val[4]);
      hal_spi_write_reg(phy, i_rxbbf_offset_tia_reg, g_reg_6E5_val[4]);
    }

    LOG_MDEBUG(phy, RX_DC_CAL, "G4: 0x%03x = 0x%02x, 0x%03x = 0x%02x\n",
               q_rxbbf_offset_tia_reg, hal_spi_read_reg(phy, q_rxbbf_offset_tia_reg),
               i_rxbbf_offset_tia_reg, hal_spi_read_reg(phy, i_rxbbf_offset_tia_reg));

    //getTime();
    LOG_MDEBUG(phy, RX_DC_OFFSET_CAL, "G4+G0:\n");
    set_rx_port_man(phy, channel, rx_port, RX_LMT_GAIN_G4);
    phy->config->rxfe_gain[channel] = RX_LMT_GAIN_G4;

    get_offset_value_v3(phy, channel, RX_LMT_GAIN_G4, RX_LPF_GAIN_G0, &A_Offset_00_i, &C_00_i, &A_Offset_00_q, &C_00_q);
    //getTime();
    LOG_MDEBUG(phy, RX_DC_OFFSET_CAL, "G4+G12:\n");
    set_rx_port_man(phy, channel, rx_port, RX_LMT_GAIN_G4);
    phy->config->rxfe_gain[channel] = RX_LMT_GAIN_G4;
    get_offset_value_v3(phy, channel, RX_LMT_GAIN_G4, RX_LPF_GAIN_G12, &B_Offset_0C_i, &C_0C_i, &B_Offset_0C_q, &C_0C_q);
    //getTime();

    LOG_MDEBUG(phy, RX_DC_OFFSET_CAL, "G5+G0:\n");
    set_rx_port_man(phy, channel, rx_port, RX_LMT_GAIN_G5);
    phy->config->rxfe_gain[channel] = RX_LMT_GAIN_G5;
    get_offset_value_v3(phy, channel, RX_LMT_GAIN_G5, RX_LPF_GAIN_G0, &C_Offset_16_i, &C_16_i, &C_Offset_16_q, &C_16_q);
    //getTime();
    for (k = TRX_I; k <= TRX_Q; k++) {
      if (k == TRX_I) {
        A      = A_Offset_00_i;
        B      = B_Offset_0C_i;
        C      = C_Offset_16_i;
        Code_1 = C_00_i;
        Code_2 = C_0C_i;
        Code_3 = C_16_i;
      } else {
        A      = A_Offset_00_q;
        B      = B_Offset_0C_q;
        C      = C_Offset_16_q;
        Code_1 = C_00_q;
        Code_2 = C_0C_q;
        Code_3 = C_16_q;
      }
      if (Code_1 > 0x80)
        Code_1 = Code_1 - 0x80;
      else
        Code_1 = -1 * (0x80 - Code_1);

      if (Code_2 > 0x80)
        Code_2 = Code_2 - 0x80;
      else
        Code_2 = -1 * (0x80 - Code_2);

      if (Code_3 > 0x80)
        Code_3 = Code_3 - 0x80;
      else
        Code_3 = -1 * (0x80 - Code_3);

      if (Code_1 == Code_2)
        Code_1 = Code_2 + 1;

      x = -0.07981094 * A + 0.07981094 * C;
      y = 0.1681231728 * A + 0.33544983 * B - 0.5036573 * C;
      z = -0.3354498 * A + 1.33544983 * B;
      a = ((A - B) * 1.000000) / ((Code_1 - Code_2) * 1.000000);
      b = (A * Code_2 * 1.000000 - B * Code_1 * 1.000000) / ((Code_2 - Code_1) * 1.000000);
      for (m = RX_LMT_GAIN_G4; m <= RX_LMT_GAIN_G5; m++) {
        for (n = 0; n < RX_LPF_GAIN_END; n++) {
          offset_mn = -Gain_TIA[m] * Gain_BBF[n] * x - Gain_BBF[n] * y + z;
          C_mn      = (offset_mn - b) / a + 0x80;
          if (channel) {
            if (k == TRX_I)
              phy->bq_rx2[m][n * 2] = C_mn;
            else
              phy->bq_rx2[m][n * 2 + 1] = C_mn;
          } else {
            if (k == TRX_I)
              phy->bq_rx1[m][n * 2] = C_mn;
            else
              phy->bq_rx1[m][n * 2 + 1] = C_mn;
          }
        }
      }
      //getTime();
    }
    //////////////////////////////////
    if (phy->rx_dc_lut_update) {
      LOG_MDEBUG(phy, RX_DC_OFFSET_CAL, "rx_dc_lut_update\n");
      for (rxfe_gain_index = RX_PORT_GAIN_MAX - 1; rxfe_gain_index >= 0; rxfe_gain_index--) {
        for (i = 0; i < 26; i++) {
          if (channel)
            bq[i] = phy->bq_rx2[rxfe_gain_index][i];
          else
            bq[i] = phy->bq_rx1[rxfe_gain_index][i];

          LOG_MDEBUG(phy, RX_DC_OFFSET_CAL, "phy->bq_rx%d[%d][%d] = 0x%x\n", channel + 1, rxfe_gain_index, i,
                     (channel) ? phy->bq_rx2[rxfe_gain_index][i] : phy->bq_rx1[rxfe_gain_index][i]);
        }

        if (phy->rx_dc_lut_update)
          rx_dc_offset_cal_update_bq_lut(phy, channel, rxfe_gain_index, bq);
      }
    }
    phy->rx_dc_cal_flag[channel] = 1;
#endif
  }

#if 1
  //set the value rxfe g2 rxbbf g6 for qec
  set_rx_port_man(phy, channel, rx_port, RX_LMT_GAIN_G2);
  hal_spi_write_reg(phy, bbf_gain_reg, 6);
  if (channel) {
    hal_spi_write_reg(phy, i_rxbbf_offset_bq_reg, phy->bq_rx2[2][12]);
    hal_spi_write_reg(phy, q_rxbbf_offset_bq_reg, phy->bq_rx2[2][13]);
  } else {
    hal_spi_write_reg(phy, i_rxbbf_offset_bq_reg, phy->bq_rx1[2][12]);
    hal_spi_write_reg(phy, q_rxbbf_offset_bq_reg, phy->bq_rx1[2][13]);
  }

  hal_spi_write_reg(phy, lfbuf_reg, rx_lfbuf_restore);
#endif

  return 0;
}
int rx_dc_offset_cal_v3(rf_chip_phy_t* phy, TRX_CHN_ENUM chn) {
  int rg702_val, rg74A_val;
  unsigned short lut702_val, lut74A_val;

  LOG_MDEBUG(phy, RX_DC_OFFSET_CAL, "Rx dc offst calibration V3 start\n");
  if ((phy->config->mode >= CH1_FDD) && (phy->config->mode <= CH1_CH2_FDD)) {
    set_trx_lo(phy, RX_DIR, chn, phy->config->rx_flo);
    CHIP_DELAY(1);
    if (rx_dc_offset_cal_in_v3(phy, chn, phy->config->rx_port[chn]) < 0) {
      phy->error = -RX_DC_CAL_FAIL;
      return -RX_DC_CAL_FAIL;
    }
  } else {
    if (chn == TRX_CHN1) {
      rg702_val = hal_spi_read_reg(phy, 0x702) & 0xFF;
      read_lut_byte(phy, MAIN_ENSM_LUT, 19, 0x702, 197, 0, (unsigned char*)&lut702_val);
      hal_spi_write_reg(phy, 0x702, (unsigned char)lut702_val);
    } else {
      rg74A_val = hal_spi_read_reg(phy, 0x74A) & 0xFF;
      read_lut_byte(phy, MAIN_ENSM_LUT, 19, 0x74A, 198, 3, (unsigned char*)&lut74A_val);
      hal_spi_write_reg(phy, 0x74A, (unsigned char)lut74A_val);
    }

    set_trx_lo(phy, TX_DIR, chn, phy->config->tx_flo);
    CHIP_DELAY(1);
    if (rx_dc_offset_cal_in_v3(phy, chn, phy->config->rx_port[chn]) < 0) {
      phy->error = -RX_DC_CAL_FAIL;
      return -RX_DC_CAL_FAIL;
    }
    hal_spi_write_reg(phy, 0x660, 0x0a);

    if (chn == TRX_CHN1) {
      hal_spi_write_reg(phy, 0x702, rg702_val & 0xFF);
    } else {
      hal_spi_write_reg(phy, 0x74A, rg74A_val & 0xFF);
    }
  }

  LOG_MDEBUG(phy, RX_DC_OFFSET_CAL, "Rx dc offst calibration end\n");
  return 0;
}
#endif
#endif

/****************************************************************************************/
// Open DIG_FIR_FILTER_USE_FIR.
/****************************************************************************************/
int dig_trx_filter_use_fir(rf_chip_phy_t* phy) {
  int size;
  int fir_coef_128[128] = {1, -7, 5, 5, -4, -7, 4, 11, -3, -15, 1, 20, 3, -25, -9, 29, 18, -33, -30, 34, 44, -31, -60, 24, 78, -12, -96, -8, 113, 35,
                           -126, -69, 133, 111, -130, -160, 116, 213, -88, -269, 41, 323, 25, -371, -115, 409, 229, -430, -370, 426, 541, -387, -749, 300, 1005, -139,
                           -1337, -146, 1820, 689, -2707, -2061, 5627, 13943, 13943, 5627, -2061, -2707, 689, 1820, -146, -1337, -139, 1005, 300, -749, -387, 541,
                           426, -370, -430, 229, 409, -115, -371, 25, 323, 41, -269, -88, 213, 116, -160, -130, 111, 133, -69, -126, 35, 113, -8, -96, -12, 78, 24, -60,
                           -31, 44, 34, -30, -33, 18, 29, -9, -25, 3, 1, -15, -3, 11, 4, -7, -4, 5, 5, -7, 1};

  size = sizeof(fir_coef_128) / sizeof(fir_coef_128[0]);
  LOG_MDEBUG(phy, RX_BW_CAL, "Fir Coef Size: %d.\n", size);

  DIG_FIR_FILTER_CFG(phy, 1, TX_DIR, TRX_CHN1, phy->config->bandwith, size, fir_coef_128, 0);
  DIG_FIR_FILTER_CFG(phy, 1, RX_DIR, TRX_CHN1, phy->config->bandwith, size, fir_coef_128, 0);
  DIG_FIR_FILTER_CFG(phy, 1, TX_DIR, TRX_CHN2, phy->config->bandwith, size, fir_coef_128, 0);
  DIG_FIR_FILTER_CFG(phy, 1, RX_DIR, TRX_CHN2, phy->config->bandwith, size, fir_coef_128, 0);

  return 0;
}

/****************************************************************************************/
// imbalance calibration
/****************************************************************************************/
#define RG_RX1_BBF_TIAPPL_RBANK_D32   0xD32
#define RG_RX1_BBF_TIAPPL_RBANKLG_D36 0xD36

unsigned char shift_byte(unsigned char data) {
  data = (data << 4) | (data >> 4);
  data = ((data << 2) & 0xCC) | ((data >> 2) & 0x33);
  data = ((data << 1) & 0xAA) | ((data >> 1) & 0x55);
  return data;
}

int inbalance_cal_with_rx_band_selection(rf_chip_phy_t* phy, int channel) {
  set_trx_lo(phy, RX_DIR, channel, phy->config->rx_flo);
  CHIP_DELAY(1);
  return 0;
}

int inbalance_cal_with_tx_band_selection(rf_chip_phy_t* phy, int channel) {
  set_trx_lo(phy, TX_DIR, channel, phy->config->tx_flo);
  CHIP_DELAY(1);
  return 0;
}

long long get_imbalance_inBand(rf_chip_phy_t* phy) {
  long long freq_inBand;

  if (phy->config->custom_bandwith_flag) {
    freq_inBand = ((phy->config->custom_bandwith / 2 / (phy->config->bb_sample_rate / 32)) *
                   (phy->config->bb_sample_rate / 32));
  } else if (phy->config->syspll_cfg_flag) {
    freq_inBand = (g_3db_band[phy->config->bandwith] / (phy->config->bb_sample_rate / 32)) *
                  (phy->config->bb_sample_rate / 32);
  } else {
    freq_inBand = (g_3db_band[phy->config->bandwith] / (g_band_bbrate[phy->config->bandwith] * 1000 / 32)) *
                  (g_band_bbrate[phy->config->bandwith] * 1000 / 32);
  }

  LOG_MDEBUG(phy, RX_BW_CAL, "[imbalance] inBand:%lld\n", freq_inBand);

  return freq_inBand;
}

int inbalance_cal_with_G0(rf_chip_phy_t* phy, int channel) {
  int rg_rx_hg_offset = (channel == TRX_CHN1) ? 0 : -1;
  int cnt             = 0;
  int tmpVal          = 0x00;
  int H_d32_val;
  int old_d32_val;
  int flag            = 0;
  short find_best_val = 0;
  long Pneg, Ppos, Delta, old_Delta;
  long long freq_inBand;
  int max_cal_val, min_cal_val, tmp_cal_val;
  int tryCnt = 0;
  long long freq_neg, freq_pos;

  LOG_MDEBUG(phy, RX_BW_CAL, "---inbalance_cal_with_G0 ---\n");
  set_rx_port_man(phy, channel, phy->config->rx_port[channel], RX_PORT_G0);
  phy->config->rxfe_gain[channel] = RX_PORT_G0;

  freq_inBand = get_imbalance_inBand(phy);

  freq_neg = phy->config->rx_flo - freq_inBand;
  freq_pos = phy->config->rx_flo + freq_inBand;
  do {
    hal_spi_write_reg(phy, RG_RX1_BBF_TIAPPL_RBANK_D32 + rg_rx_hg_offset, 0);
    tmpVal = hal_spi_read_reg(phy, RG_RX1_BBF_TIAPPL_RBANK_D32 + rg_rx_hg_offset) & 0xFF;
    inbalance_cal_with_rx_band_selection(phy, channel);
    phy->config->tx_flo = freq_neg;
    inbalance_cal_with_tx_band_selection(phy, channel);
    Pneg = get_goertzle_power(phy, channel, freq_inBand);
    LOG_MDEBUG(phy, RX_BW_CAL, "config->tx_flo = %lld, config->rx_flo = %lld, freq_inBand = %lld, Pneg = %ld\n",
               phy->config->tx_flo, phy->config->rx_flo, freq_inBand, Pneg);
    phy->config->tx_flo = freq_pos;
    inbalance_cal_with_tx_band_selection(phy, channel);
    Ppos = get_goertzle_power(phy, channel, freq_inBand);
    LOG_MDEBUG(phy, RX_BW_CAL, "config->tx_flo = %lld, config->rx_flo = %lld, freq_inBand = %lld, Ppos = %ld\n",
               phy->config->tx_flo, phy->config->rx_flo, freq_inBand, Ppos);
    Delta = Pneg - Ppos;
    LOG_MDEBUG(phy, RX_BW_CAL, "Delta = %ld, 0x%x = 0x%x, Pneg = %ld, Ppos = %ld\n",
               Delta, RG_RX1_BBF_TIAPPL_RBANK_D32 + rg_rx_hg_offset,
               hal_spi_read_reg(phy, RG_RX1_BBF_TIAPPL_RBANK_D32 + rg_rx_hg_offset) & 0xFF, Pneg, Ppos);
    if (Delta != 0)
      break;
    else if (tryCnt++ > 5)
      return 0;
    else
      CHIP_DELAY(1);
  } while (1);

  if (Delta > 0) {
    H_d32_val = 0x80;
    hal_spi_write_reg(phy, RG_RX1_BBF_TIAPPL_RBANK_D32 + rg_rx_hg_offset, H_d32_val | tmpVal | 0x01);
    ///////////////////////////////////////////
    Delta       = 0;
    old_Delta   = 0;
    old_d32_val = 0;
    tmpVal      = 0;
    cnt         = 0;
    while (1) {
      inbalance_cal_with_rx_band_selection(phy, channel);
      phy->config->tx_flo = freq_neg;
      inbalance_cal_with_tx_band_selection(phy, channel);
      Pneg                = get_goertzle_power(phy, channel, freq_inBand);
      phy->config->tx_flo = freq_pos;
      inbalance_cal_with_tx_band_selection(phy, channel);
      Ppos      = get_goertzle_power(phy, channel, freq_inBand);
      old_Delta = Delta;
      Delta     = Pneg - Ppos;
      LOG_MDEBUG(phy, RX_BW_CAL, "Delta = %ld, 0x%x = 0x%x, Pneg = %ld, Ppos = %ld\n",
                 Delta, RG_RX1_BBF_TIAPPL_RBANK_D32 + rg_rx_hg_offset,
                 hal_spi_read_reg(phy, RG_RX1_BBF_TIAPPL_RBANK_D32 + rg_rx_hg_offset) & 0xFF, Pneg, Ppos);

      if (cnt++ == 5) {
        old_d32_val = hal_spi_read_reg(phy, RG_RX1_BBF_TIAPPL_RBANK_D32 + rg_rx_hg_offset);
        if ((old_d32_val >> 5) & 0x01)
          tmpVal = CLR_BIT(old_d32_val, 5);
        else
          tmpVal = SET_BIT(old_d32_val, 5);

        hal_spi_write_reg(phy, RG_RX1_BBF_TIAPPL_RBANK_D32 + rg_rx_hg_offset, tmpVal);

        inbalance_cal_with_rx_band_selection(phy, channel);
        phy->config->tx_flo = freq_neg;
        inbalance_cal_with_tx_band_selection(phy, channel);
        Pneg                = get_goertzle_power(phy, channel, freq_inBand);
        phy->config->tx_flo = freq_pos;
        inbalance_cal_with_tx_band_selection(phy, channel);
        Ppos      = get_goertzle_power(phy, channel, freq_inBand);
        old_Delta = Delta;
        Delta     = Pneg - Ppos;
        LOG_MDEBUG(phy, RX_BW_CAL, "Delta = %ld, 0x%x = 0x%x, Pneg = %ld, Ppos = %ld\n",
                   Delta, RG_RX1_BBF_TIAPPL_RBANK_D32 + rg_rx_hg_offset,
                   hal_spi_read_reg(phy, RG_RX1_BBF_TIAPPL_RBANK_D32 + rg_rx_hg_offset) & 0xFF, Pneg, Ppos);
        break;
      }

      old_d32_val = hal_spi_read_reg(phy, RG_RX1_BBF_TIAPPL_RBANK_D32 + rg_rx_hg_offset);
      if (Delta > 0) {
        tmpVal = 1 << cnt;
        hal_spi_write_reg(phy, RG_RX1_BBF_TIAPPL_RBANK_D32 + rg_rx_hg_offset, old_d32_val | tmpVal);
      } else {
        tmpVal = CLR_BIT(old_d32_val, (cnt - 1)) | (1 << cnt);
        hal_spi_write_reg(phy, RG_RX1_BBF_TIAPPL_RBANK_D32 + rg_rx_hg_offset, tmpVal);
      }
    }
  } else if (Delta < 0) {
    H_d32_val = 0x40;
    hal_spi_write_reg(phy, RG_RX1_BBF_TIAPPL_RBANK_D32 + rg_rx_hg_offset, H_d32_val | tmpVal | 0x01);
    ///////////////////////////////////////////
    Delta       = 0;
    old_Delta   = 0;
    old_d32_val = 0;
    tmpVal      = 0;
    cnt         = 0;
    while (1) {
      inbalance_cal_with_rx_band_selection(phy, channel);
      phy->config->tx_flo = freq_neg;
      inbalance_cal_with_tx_band_selection(phy, channel);
      Pneg                = get_goertzle_power(phy, channel, freq_inBand);
      phy->config->tx_flo = freq_pos;
      inbalance_cal_with_tx_band_selection(phy, channel);
      Ppos      = get_goertzle_power(phy, channel, freq_inBand);
      old_Delta = Delta;
      Delta     = Pneg - Ppos;
      LOG_MDEBUG(phy, RX_BW_CAL, "Delta = %ld, 0x%x = 0x%x, Pneg = %ld, Ppos = %ld\n",
                 Delta, RG_RX1_BBF_TIAPPL_RBANK_D32 + rg_rx_hg_offset,
                 hal_spi_read_reg(phy, RG_RX1_BBF_TIAPPL_RBANK_D32 + rg_rx_hg_offset) & 0xFF, Pneg, Ppos);
      if (cnt++ == 5) {
        old_d32_val = hal_spi_read_reg(phy, RG_RX1_BBF_TIAPPL_RBANK_D32 + rg_rx_hg_offset);
        if ((old_d32_val >> 5) & 0x01)
          tmpVal = CLR_BIT(old_d32_val, 5);
        else
          tmpVal = SET_BIT(old_d32_val, 5);

        hal_spi_write_reg(phy, RG_RX1_BBF_TIAPPL_RBANK_D32 + rg_rx_hg_offset, tmpVal);

        inbalance_cal_with_rx_band_selection(phy, channel);
        phy->config->tx_flo = freq_neg;
        inbalance_cal_with_tx_band_selection(phy, channel);
        Pneg                = get_goertzle_power(phy, channel, freq_inBand);
        phy->config->tx_flo = freq_pos;
        inbalance_cal_with_tx_band_selection(phy, channel);
        Ppos      = get_goertzle_power(phy, channel, freq_inBand);
        old_Delta = Delta;
        Delta     = Pneg - Ppos;
        LOG_MDEBUG(phy, RX_BW_CAL, "Delta = %ld, 0x%x = 0x%x, Pneg = %ld, Ppos = %ld\n",
                   Delta, RG_RX1_BBF_TIAPPL_RBANK_D32 + rg_rx_hg_offset,
                   hal_spi_read_reg(phy, RG_RX1_BBF_TIAPPL_RBANK_D32 + rg_rx_hg_offset) & 0xFF, Pneg, Ppos);

        break;
      }

      old_d32_val = hal_spi_read_reg(phy, RG_RX1_BBF_TIAPPL_RBANK_D32 + rg_rx_hg_offset);
      if (Delta < 0) {
        tmpVal = 1 << cnt;
        hal_spi_write_reg(phy, RG_RX1_BBF_TIAPPL_RBANK_D32 + rg_rx_hg_offset, old_d32_val | tmpVal);
      } else {
        tmpVal = CLR_BIT(old_d32_val, (cnt - 1)) | (1 << cnt);
        hal_spi_write_reg(phy, RG_RX1_BBF_TIAPPL_RBANK_D32 + rg_rx_hg_offset, tmpVal);
      }
    }
  }

  if (old_Delta < 0)
    old_Delta = old_Delta * -1;

  if (Delta < 0)
    Delta = Delta * -1;

  if (old_Delta < Delta)
    hal_spi_write_reg(phy, RG_RX1_BBF_TIAPPL_RBANK_D32 + rg_rx_hg_offset, old_d32_val);

  inbalance_cal_with_rx_band_selection(phy, channel);
  phy->config->tx_flo = freq_neg;
  inbalance_cal_with_tx_band_selection(phy, channel);
  Pneg                = get_goertzle_power(phy, channel, freq_inBand);
  phy->config->tx_flo = freq_pos;
  inbalance_cal_with_tx_band_selection(phy, channel);
  Ppos  = get_goertzle_power(phy, channel, freq_inBand);
  Delta = Pneg - Ppos;
  LOG_MDEBUG(phy, RX_BW_CAL, "[Final] Delta = %ld, 0x%x = 0x%x, Pneg = %ld, Ppos = %ld\n",
             Delta, RG_RX1_BBF_TIAPPL_RBANK_D32 + rg_rx_hg_offset,
             hal_spi_read_reg(phy, RG_RX1_BBF_TIAPPL_RBANK_D32 + rg_rx_hg_offset) & 0xFF, Pneg, Ppos);

  phy->rx_imbalance_cal[channel][0] = hal_spi_read_reg(phy, RG_RX1_BBF_TIAPPL_RBANK_D32 + rg_rx_hg_offset) & 0xFF;

  return 0;
}

int inbalance_cal_with_G2(rf_chip_phy_t* phy, int channel) {
  int rg_rx_lg_offset = (channel == TRX_CHN1) ? 0 : 1;
  int rg_rx_offset    = (channel == TRX_CHN1) ? 0 : -1;
  int cnt             = 0;
  int tmpVal          = 0x00;
  int H_d32_val;
  int old_d36_val;
  int flag            = 0;
  short find_best_val = 0;
  long Pneg, Ppos, Delta, old_Delta;
  long long freq_inBand;
  int max_cal_val, min_cal_val, tmp_cal_val;
  int tryCnt = 0;
  long long freq_neg, freq_pos;

  LOG_MDEBUG(phy, RX_BW_CAL, "---inbalance_cal_with_G2 ---\n");
  set_rx_port_man(phy, channel, phy->config->rx_port[channel], RX_PORT_G2);
  phy->config->rxfe_gain[channel] = RX_PORT_G2;

  freq_inBand = get_imbalance_inBand(phy);

  freq_neg = phy->config->rx_flo - freq_inBand;
  freq_pos = phy->config->rx_flo + freq_inBand;
  tmpVal   = hal_spi_read_reg(phy, RG_RX1_BBF_TIAPPL_RBANK_D32 + rg_rx_offset) & 0xFF;
  if ((tmpVal >> 7) & 0x01) {
    hal_spi_write_reg(phy, RG_RX1_BBF_TIAPPL_RBANKLG_D36 + rg_rx_lg_offset, 0x01);
    ///////////////////////////////////////////
    Delta       = 0;
    old_Delta   = 0;
    old_d36_val = 0;
    tmpVal      = 0;
    cnt         = 0;
    while (1) {
      inbalance_cal_with_rx_band_selection(phy, channel);
      phy->config->tx_flo = freq_neg;
      inbalance_cal_with_tx_band_selection(phy, channel);
      Pneg                = get_goertzle_power(phy, channel, freq_inBand);
      phy->config->tx_flo = freq_pos;
      inbalance_cal_with_tx_band_selection(phy, channel);
      Ppos      = get_goertzle_power(phy, channel, freq_inBand);
      old_Delta = Delta;
      Delta     = Pneg - Ppos;
      LOG_MDEBUG(phy, RX_BW_CAL, "Delta = %ld, 0x%x = 0x%x, Pneg = %ld, Ppos = %ld\n",
                 Delta, RG_RX1_BBF_TIAPPL_RBANKLG_D36 + rg_rx_lg_offset,
                 hal_spi_read_reg(phy, RG_RX1_BBF_TIAPPL_RBANKLG_D36 + rg_rx_lg_offset) & 0xFF, Pneg, Ppos);

      if (cnt++ == 5) {
        old_d36_val = hal_spi_read_reg(phy, RG_RX1_BBF_TIAPPL_RBANKLG_D36 + rg_rx_lg_offset);
        if ((old_d36_val >> 5) & 0x01)
          tmpVal = CLR_BIT(old_d36_val, 5);
        else
          tmpVal = SET_BIT(old_d36_val, 5);

        hal_spi_write_reg(phy, RG_RX1_BBF_TIAPPL_RBANKLG_D36 + rg_rx_lg_offset, tmpVal);

        inbalance_cal_with_rx_band_selection(phy, channel);
        phy->config->tx_flo = freq_neg;
        inbalance_cal_with_tx_band_selection(phy, channel);
        Pneg                = get_goertzle_power(phy, channel, freq_inBand);
        phy->config->tx_flo = freq_pos;
        inbalance_cal_with_tx_band_selection(phy, channel);
        Ppos      = get_goertzle_power(phy, channel, freq_inBand);
        old_Delta = Delta;
        Delta     = Pneg - Ppos;
        LOG_MDEBUG(phy, RX_BW_CAL, "Delta = %ld, 0x%x = 0x%x, Pneg = %ld, Ppos = %ld\n",
                   Delta, RG_RX1_BBF_TIAPPL_RBANKLG_D36 + rg_rx_lg_offset,
                   hal_spi_read_reg(phy, RG_RX1_BBF_TIAPPL_RBANKLG_D36 + rg_rx_lg_offset) & 0xFF, Pneg, Ppos);
        break;
      }

      old_d36_val = hal_spi_read_reg(phy, RG_RX1_BBF_TIAPPL_RBANKLG_D36 + rg_rx_lg_offset);
      if (Delta > 0) {
        tmpVal = 1 << cnt;
        hal_spi_write_reg(phy, RG_RX1_BBF_TIAPPL_RBANKLG_D36 + rg_rx_lg_offset, old_d36_val | tmpVal);
      } else {
        tmpVal = CLR_BIT(old_d36_val, (cnt - 1)) | (1 << cnt);
        hal_spi_write_reg(phy, RG_RX1_BBF_TIAPPL_RBANKLG_D36 + rg_rx_lg_offset, tmpVal);
      }
    }
  } else if ((tmpVal >> 6) & 0x01) {
    hal_spi_write_reg(phy, RG_RX1_BBF_TIAPPL_RBANKLG_D36 + rg_rx_lg_offset, 0x01);
    ///////////////////////////////////////////
    Delta       = 0;
    old_Delta   = 0;
    old_d36_val = 0;
    tmpVal      = 0;
    cnt         = 0;
    while (1) {
      inbalance_cal_with_rx_band_selection(phy, channel);
      phy->config->tx_flo = freq_neg;
      inbalance_cal_with_tx_band_selection(phy, channel);
      Pneg                = get_goertzle_power(phy, channel, freq_inBand);
      phy->config->tx_flo = freq_pos;
      inbalance_cal_with_tx_band_selection(phy, channel);
      Ppos      = get_goertzle_power(phy, channel, freq_inBand);
      old_Delta = Delta;
      Delta     = Pneg - Ppos;
      LOG_MDEBUG(phy, RX_BW_CAL, "Delta = %ld, 0x%x = 0x%x, Pneg = %ld, Ppos = %ld\n",
                 Delta, RG_RX1_BBF_TIAPPL_RBANKLG_D36 + rg_rx_lg_offset,
                 hal_spi_read_reg(phy, RG_RX1_BBF_TIAPPL_RBANKLG_D36 + rg_rx_lg_offset) & 0xFF, Pneg, Ppos);

      if (cnt++ == 5) {
        old_d36_val = hal_spi_read_reg(phy, RG_RX1_BBF_TIAPPL_RBANKLG_D36 + rg_rx_lg_offset);
        if ((old_d36_val >> 5) & 0x01)
          tmpVal = CLR_BIT(old_d36_val, 5);
        else
          tmpVal = SET_BIT(old_d36_val, 5);

        hal_spi_write_reg(phy, RG_RX1_BBF_TIAPPL_RBANKLG_D36 + rg_rx_lg_offset, tmpVal);

        inbalance_cal_with_rx_band_selection(phy, channel);
        phy->config->tx_flo = freq_neg;
        inbalance_cal_with_tx_band_selection(phy, channel);
        Pneg                = get_goertzle_power(phy, channel, freq_inBand);
        phy->config->tx_flo = freq_pos;
        inbalance_cal_with_tx_band_selection(phy, channel);
        Ppos      = get_goertzle_power(phy, channel, freq_inBand);
        old_Delta = Delta;
        Delta     = Pneg - Ppos;
        LOG_MDEBUG(phy, RX_BW_CAL, "Delta = %ld, 0x%x = 0x%x, Pneg = %ld, Ppos = %ld\n",
                   Delta, RG_RX1_BBF_TIAPPL_RBANKLG_D36 + rg_rx_lg_offset,
                   hal_spi_read_reg(phy, RG_RX1_BBF_TIAPPL_RBANKLG_D36 + rg_rx_lg_offset) & 0xFF, Pneg, Ppos);
        break;
      }
      old_d36_val = hal_spi_read_reg(phy, RG_RX1_BBF_TIAPPL_RBANKLG_D36 + rg_rx_lg_offset);
      if (Delta < 0) {
        tmpVal = 1 << cnt;
        hal_spi_write_reg(phy, RG_RX1_BBF_TIAPPL_RBANKLG_D36 + rg_rx_lg_offset, old_d36_val | tmpVal);
      } else {
        tmpVal = CLR_BIT(old_d36_val, (cnt - 1)) | (1 << cnt);
        hal_spi_write_reg(phy, RG_RX1_BBF_TIAPPL_RBANKLG_D36 + rg_rx_lg_offset, tmpVal);
      }
    }
  } else {
    LOG_MDEBUG(phy, RX_BW_CAL, "Don't support\n");
    return 0;
  }

  if (old_Delta < 0)
    old_Delta = old_Delta * -1;

  if (Delta < 0)
    Delta = Delta * -1;

  if (old_Delta < Delta)
    hal_spi_write_reg(phy, RG_RX1_BBF_TIAPPL_RBANKLG_D36 + rg_rx_lg_offset, old_d36_val);

  inbalance_cal_with_rx_band_selection(phy, channel);
  phy->config->tx_flo = freq_neg;
  inbalance_cal_with_tx_band_selection(phy, channel);
  Pneg                = get_goertzle_power(phy, channel, freq_inBand);
  phy->config->tx_flo = freq_pos;
  inbalance_cal_with_tx_band_selection(phy, channel);
  Ppos  = get_goertzle_power(phy, channel, freq_inBand);
  Delta = Pneg - Ppos;
  LOG_MDEBUG(phy, RX_BW_CAL, "[Final] Delta = %ld, 0x%x = 0x%x, Pneg = %ld, Ppos = %ld\n",
             Delta, RG_RX1_BBF_TIAPPL_RBANKLG_D36 + rg_rx_lg_offset,
             hal_spi_read_reg(phy, RG_RX1_BBF_TIAPPL_RBANKLG_D36 + rg_rx_lg_offset) & 0xFF, Pneg, Ppos);

  phy->rx_imbalance_cal[channel][1] = hal_spi_read_reg(phy, RG_RX1_BBF_TIAPPL_RBANKLG_D36 + rg_rx_lg_offset) & 0xFF;
  return 0;
}

int inbalance_cal(rf_chip_phy_t* phy, int channel) {
  RF_LOOPBACK_REGS rf_loopback_regs;
  int tdd_sel;
  unsigned long long flo, tx_flo_bak, rx_flo_bak;
  int regTxGan = (channel == TRX_CHN1) ? 0xE6 : 0xE8;
  int tmpTxGainVal;
  int rg_rx_hg_offset = (channel == TRX_CHN1) ? 0 : -1;
  int rg_rx_lg_offset = (channel == TRX_CHN1) ? 0 : 1;
  int rg_634_val, rg_620_val, rg_747_val, rg_702_val, rg_78F_val, rg_74A_val, rg_803_val, rg_650_val;
  int rg_6D1_val, rg_66E_val, rg_6E1_val;
  int rg_7c2_val, rg_7c4_val, rg_7d2_val, rg_7d4_val, rg_790_val, rg_7A3_val;

  if (phy->config->mode >= CH1_TDD) {
    tdd_sel = 1;
    //__set_trxlo_switch(1, channel, tdd_sel, 0);
    ////
    rg_634_val = hal_spi_read_reg(phy, 0x634) & 0xFF;
    rg_620_val = hal_spi_read_reg(phy, 0x620) & 0xFF;

    rg_747_val = hal_spi_read_reg(phy, 0x747) & 0xFF;
    rg_702_val = hal_spi_read_reg(phy, 0x702) & 0xFF;
    rg_78F_val = hal_spi_read_reg(phy, 0x78F) & 0xFF;
    rg_74A_val = hal_spi_read_reg(phy, 0x74A) & 0xFF;

    rg_803_val = hal_spi_read_reg(phy, 0x803) & 0xFF;
    rg_650_val = hal_spi_read_reg(phy, 0x650) & 0xFF;
    rg_6D1_val = hal_spi_read_reg(phy, 0x6D1) & 0xFF;
    rg_66E_val = hal_spi_read_reg(phy, 0x66E) & 0xFF;
    rg_6E1_val = hal_spi_read_reg(phy, 0x6E1) & 0xFF;
    ////

    hal_spi_write_reg(phy, 0x634, 0x04);
    hal_spi_write_reg(phy, 0x620, 0x3C);

    hal_spi_write_reg(phy, 0x747, 0x01);
    hal_spi_write_reg(phy, 0x702, 0x0F);
    hal_spi_write_reg(phy, 0x78F, 0x01);
    hal_spi_write_reg(phy, 0x74A, 0x0F);

    hal_spi_write_reg(phy, 0x803, 0x0F);
    hal_spi_write_reg(phy, 0x650, 0x01);
    hal_spi_write_reg(phy, 0x6D1, 0x7F);
    hal_spi_write_reg(phy, 0x66E, 0x01);
    hal_spi_write_reg(phy, 0x6E1, 0x7F);
  } else {
    tdd_sel = 0;
    // TX1_DAC  I/Q
    rg_790_val = hal_spi_read_reg(phy, 0x790) & 0xFF;
    rg_7c2_val = hal_spi_read_reg(phy, 0x7C2) & 0xFF;
    rg_7c4_val = hal_spi_read_reg(phy, 0x7C4) & 0xFF;
    hal_spi_write_reg(phy, 0x790, rg_790_val | 0x1);
    hal_spi_write_reg(phy, 0x7C2, 0x90);
    hal_spi_write_reg(phy, 0x7C4, 0x90);

    // TX2_DAC  I/Q
    rg_7A3_val = hal_spi_read_reg(phy, 0x7A3) & 0xFF;
    rg_7d2_val = hal_spi_read_reg(phy, 0x7D2) & 0xFF;
    rg_7d4_val = hal_spi_read_reg(phy, 0x7D4) & 0xFF;
    hal_spi_write_reg(phy, 0x7A3, rg_7A3_val | 0x1);
    hal_spi_write_reg(phy, 0x7D2, 0x90);
    hal_spi_write_reg(phy, 0x7D4, 0x90);

    LOG_MDEBUG(phy, RX_BW_CAL, "FDD Mode: 0x7C2 = 0x%x, 0x7C4 = 0x%x, 0x7D2 = 0x%x,  0x7D4 = 0x%x,  0x790 = 0x%x, 0x7A3 = 0x%x\n",
               hal_spi_read_reg(phy, 0x7C2) & 0xFF, hal_spi_read_reg(phy, 0x7C4) & 0xFF,
               hal_spi_read_reg(phy, 0x7D2) & 0xFF, hal_spi_read_reg(phy, 0x7D2) & 0xFF,
               hal_spi_read_reg(phy, 0x790) & 0xFF, hal_spi_read_reg(phy, 0x7A3) & 0xFF);
  }

  rx_bw_config_tone_tx(phy, channel, phy->config->bandwith, phy->config->mode);
  fn_rf_loop_back_ena(phy, channel, 1, tdd_sel, &rf_loopback_regs);
  //reset_adc(channel);

  tx_flo_bak = phy->config->tx_flo;
  rx_flo_bak = phy->config->rx_flo;
  set_tx_atten(phy, phy->config->tx_flo, 0x10, 0);

  if (tdd_sel)
    phy->config->rx_flo = phy->config->tx_flo;
  inbalance_cal_with_G0(phy, channel);

#if 1
  phy->config->tx_flo = tx_flo_bak;
  phy->config->rx_flo = rx_flo_bak;
  if (tdd_sel)
    phy->config->rx_flo = phy->config->tx_flo;
  inbalance_cal_with_G2(phy, channel);
#endif

  LOG_MDEBUG(phy, RX_BW_CAL, "[Imbalance Cal Value]0x%x = 0x%x, 0x%x = 0x%x\n",
             RG_RX1_BBF_TIAPPL_RBANK_D32 + rg_rx_hg_offset,
             hal_spi_read_reg(phy, RG_RX1_BBF_TIAPPL_RBANK_D32 + rg_rx_hg_offset),
             RG_RX1_BBF_TIAPPL_RBANKLG_D36 + rg_rx_lg_offset,
             hal_spi_read_reg(phy, RG_RX1_BBF_TIAPPL_RBANKLG_D36 + rg_rx_lg_offset));
  phy->config->tx_flo = tx_flo_bak;
  phy->config->rx_flo = rx_flo_bak;

  //imbalance_cal_with_rx_band_selection(channel, config);
  //imbalance_cal_with_tx_band_selection(channel, config);

  /// recovery  ////
  LOG_MDEBUG(phy, RX_BW_CAL, "Imbalance Cal recovery.\n");
  fn_rf_loop_back_ena(phy, channel, 0, tdd_sel, &rf_loopback_regs);
  if (phy->config->mode >= CH1_TDD) {
    /// recovery  ////
    hal_spi_write_reg(phy, 0x634, rg_634_val & 0xFF);
    hal_spi_write_reg(phy, 0x620, rg_620_val & 0xFF);

    hal_spi_write_reg(phy, 0x747, rg_747_val & 0xFF);
    hal_spi_write_reg(phy, 0x702, rg_702_val & 0xFF);
    hal_spi_write_reg(phy, 0x78F, rg_78F_val & 0xFF);
    hal_spi_write_reg(phy, 0x74A, rg_74A_val & 0xFF);

    hal_spi_write_reg(phy, 0x803, rg_803_val & 0xFF);
    hal_spi_write_reg(phy, 0x650, rg_650_val & 0xFF);
    hal_spi_write_reg(phy, 0x6D1, rg_6D1_val & 0xFF);
    hal_spi_write_reg(phy, 0x66E, rg_66E_val & 0xFF);
    hal_spi_write_reg(phy, 0x6E1, rg_6E1_val & 0xFF);
  } else {
    hal_spi_write_reg(phy, 0x7C2, rg_7c2_val);
    hal_spi_write_reg(phy, 0x7C4, rg_7c4_val);
    hal_spi_write_reg(phy, 0x7D2, rg_7d2_val);
    hal_spi_write_reg(phy, 0x7D4, rg_7d4_val);
    hal_spi_write_reg(phy, 0x790, rg_790_val);
    hal_spi_write_reg(phy, 0x7A3, rg_7A3_val);
    LOG_MDEBUG(phy, RX_BW_CAL, "FDD Mode: 0x7C2 = 0x%x, 0x7C4 = 0x%x, 0x7D2 = 0x%x,  0x7D4 = 0x%x,  0x790 = 0x%x, 0x7A3 = 0x%x\n",
               hal_spi_read_reg(phy, 0x7C2) & 0xFF, hal_spi_read_reg(phy, 0x7C4) & 0xFF,
               hal_spi_read_reg(phy, 0x7D2) & 0xFF, hal_spi_read_reg(phy, 0x7D2) & 0xFF,
               hal_spi_read_reg(phy, 0x790) & 0xFF, hal_spi_read_reg(phy, 0x7A3) & 0xFF);
  }

  return 0;
}

/****************************************************************************************/
// rx bw calibration
/****************************************************************************************/
int trx_band_pa_lut_load(rf_chip_phy_t* phy, unsigned long long tx_flo) {
  int i;
  int mid;
  int low  = 0;
  int high = 8;
  unsigned long long min_range, max_range;
  int tx_pa_gain_index;
  unsigned char reg_val;
  unsigned int tc_pa_gain_lut_info;

  /* tx pa gain index */
  reg_val          = hal_spi_read_reg(phy, 0x0CB);
  tx_pa_gain_index = reg_val & 0xF;

  /*
	 *get freq case via flo
	 *case 1: 30MHz~600MHz
	 *case 2: 600MHz~900MHz
	 *case 3: 900MHz~1400MHz
	 *case 4: 1400MHz~1900MHz
	 *case 5: 1900MHz~2400MHz
	 *case 6: 2400MHz~2800MHz
	 *case 7: 2800MHz~3200MHz
	 *case 8: 3200MHz~5000MHz
	 *case 9: 5000MHz~6000MHz
	 */

  if (tx_flo < 30000000ULL || tx_flo >= 6125000000ULL) {
    phy->error = -PARA_INVALID;
    LOG_ERROR("The tx flo exceeds the freq range([30, 6125)MHz)\n");
    return -PARA_INVALID;
  }

  while (low <= high) {
    mid = (low + high) / 2;

    min_range = (unsigned long long)((unsigned long long)(g_lo_range_trx_band_fsm[mid][0]) * MHZ_TO_HZ);
    max_range = (unsigned long long)((unsigned long long)(g_lo_range_trx_band_fsm[mid][1]) * MHZ_TO_HZ);

    if (tx_flo >= min_range && tx_flo < max_range) {
      break;
    } else if (tx_flo >= max_range) {
      low = mid + 1;
    } else {
      high = mid - 1;
    }
  }
  //LOG_INFO("The freq case is %d, tx_pa_gain_index=%d\n", mid + 1, tx_pa_gain_index);

  //load trx band lut via tx pa gain index
  for (i = 0; i < 48; i++) {
    CHIP_DELAY(1);
    hal_spi_write_reg(phy, g_trx_band_regs[i], g_trx_band_config[mid][i]);
    //LOG_INFO("trx_band_lut_load reg=0x%0x, val=0x%0x\n", g_trx_band_regs[i], g_trx_band_config[mid][i]);
  }

  //load lut via tx pa gain index and la range index
  for (i = 0; i < 20; i++) {
    tc_pa_gain_lut_info = g_tx_pa_gain_config[mid * 9 + tx_pa_gain_index][i];
    CHIP_DELAY(1);
    hal_spi_write_reg(phy, g_tx_pa_gain_regs[i], tc_pa_gain_lut_info);
    //LOG_INFO("tx_pa_lut_load reg=0x%0x, val=0x%0x\n", g_tx_pa_gain_regs[i], tc_pa_gain_lut_info);
  }

  //load trx band lut via rx gain index
  for (i = 48; i < 54; i++) {
    CHIP_DELAY(1);
    read_lut_byte(phy, TRX_BAND_LUT, 0, g_trx_band_regs[i], i / 4, i % 4, &reg_val);
    hal_spi_write_reg(phy, g_trx_band_regs[i], reg_val);
    //LOG_MAIN("trx_band_lut_load reg=0x%0x, val=0x%0x\n", g_trx_band_regs[i], reg_val);
  }

  return 0;
}

void trx_lut_load(rf_chip_phy_t* phy, unsigned long long tx_flo, BANDWITH_ENUM bw, short custom_bw_flag) {
  trx_bw_lut_load(phy, bw, custom_bw_flag);
  trx_band_pa_lut_load(phy, tx_flo);

  /*set tx dig gain */
  hal_spi_write_reg(phy, 0x0E5, g_tx_dig_gain[0][0]);
  hal_spi_write_reg(phy, 0x0E6, g_tx_dig_gain[0][1]);
  hal_spi_write_reg(phy, 0x0E7, g_tx_dig_gain[0][2]);
  hal_spi_write_reg(phy, 0x0E8, g_tx_dig_gain[0][3]);

  if (phy->config->chip_ver == 2) {
    HAL_REG_SET_BIT(phy, 0x0E5, BIT7);
    HAL_REG_SET_BIT(phy, 0x0E7, BIT7);
  }
}

short trx_bw_lut_load(rf_chip_phy_t* phy, BANDWITH_ENUM bw, short custom_bw_flag) {
  short i, ret;
  unsigned char lut_val;

  if (custom_bw_flag) {
#if 1
    /* tx */
    for (i = 0; i < 35; i++) {
      if (i <= 14) {
        hal_spi_write_reg(phy, g_tx_bw_regs[i], phy->tx_bw_setting[i]);
        write_lut_byte(phy, TRX_BW_LUT, bw * 9, 0x00, 73 * bw + i / 4, i % 4, phy->tx_bw_setting[i]);
      } else if ((i >= 17) && (i <= 31)) {
        hal_spi_write_reg(phy, g_tx_bw_regs[i], phy->tx_bw_setting[i - 17]);
        write_lut_byte(phy, TRX_BW_LUT, bw * 9, 0x00, 73 * bw + i / 4, i % 4, phy->tx_bw_setting[i - 17]);
      } else {
        if (g_tx_bw_regs[i] == 0x604)
          write_lut_byte(phy, TRX_BW_LUT, bw * 9, 0x00, 73 * bw + i / 4, i % 4, phy->config->dac_syspll_lo_div);

        ret = read_lut_byte(phy, TRX_BW_LUT, bw * 9, 0x00, 73 * bw + i / 4, i % 4, &lut_val);
        if (ret != 0) {
          //LOG_INFO("Failed to read the tx lut info for reg 0x%0x \n", reg);
        }
        hal_spi_write_reg(phy, g_tx_bw_regs[i], lut_val);
      }
    }
#endif

    /* rx */
    for (i = 0; i < 27; i++) {
      if (i <= 12) {
        hal_spi_write_reg(phy, g_rx_bw_regs[i], phy->rx_bw_setting[i]);
        write_lut_byte(phy, TRX_BW_LUT, bw * 11, 0x00, 657 + 87 * bw + i / 4, i % 4, phy->rx_bw_setting[i]);
      } else if ((i >= 13) && (i <= 25)) {
        hal_spi_write_reg(phy, g_rx_bw_regs[i], phy->rx_bw_setting[i - 13]);
        write_lut_byte(phy, TRX_BW_LUT, bw * 11, 0x00, 657 + 87 * bw + i / 4, i % 4, phy->rx_bw_setting[i - 13]);
      } else {
        if (g_rx_bw_regs[i] == 0x6f0)
          write_lut_byte(phy, TRX_BW_LUT, bw * 11, 0x00, 657 + 87 * bw + i / 4, i % 4, phy->config->adc_syspll_lo_div);

        ret = read_lut_byte(phy, TRX_BW_LUT, bw * 11, 0x00, 657 + 87 * bw + i / 4, i % 4, &lut_val);
        if (ret != 0) {
          //LOG_INFO("Failed to read the rx lut info for reg 0x%0x \n", reg);
        }
        hal_spi_write_reg(phy, g_rx_bw_regs[i], lut_val);
      }
    }
  } else {
    /* tx */
    for (i = 0; i < 35; i++) {
      ret = read_lut_byte(phy, TRX_BW_LUT, bw * 9, 0x00, 73 * bw + i / 4, i % 4, &lut_val);
      if (ret != 0) {
        //LOG_INFO("Failed to read the tx lut info for reg 0x%0x \n", reg);
      }
      hal_spi_write_reg(phy, g_tx_bw_regs[i], lut_val);
    }

    /* rx */
    for (i = 0; i < 27; i++) {
      ret = read_lut_byte(phy, TRX_BW_LUT, bw * 11, 0x00, 657 + 87 * bw + i / 4, i % 4, &lut_val);
      if (ret != 0) {
        //LOG_INFO("Failed to read the rx lut info for reg 0x%0x \n", reg);
      }
      hal_spi_write_reg(phy, g_rx_bw_regs[i], lut_val);
    }
  }

  if (phy->config->syspll_cfg_flag) {
    i = 34;
    write_lut_byte(phy, TRX_BW_LUT, bw * 9, 0x00, 73 * bw + i / 4, i % 4, phy->config->dac_syspll_lo_div);
    i = 26;
    write_lut_byte(phy, TRX_BW_LUT, bw * 11, 0x00, 657 + 87 * bw + i / 4, i % 4, phy->config->adc_syspll_lo_div);
  }

  return 0;
}

long get_goertzle_power(rf_chip_phy_t* phy, int channel, long long flo) {
  int goertzle_0_i      = 0;
  int goertzle_0_q      = 0;
  int goertzle_1_i      = 0;
  int goertzle_1_q      = 0;
  int goertzle_shift    = 0;
  int goertzle_wb_div32 = 10;

  long long goertzle_0_i_val;
  long long goertzle_0_q_val;
  long long goertzle_1_i_val;
  long long goertzle_1_q_val;
  long power;

  goertzle_wb_div32 = flo / (g_band_bbrate[phy->config->bandwith] * 1000 / 32);

  fn_rx_run_goertzle(phy, channel, 0, goertzle_wb_div32, goertzle_wb_div32, 8192, 4, &goertzle_0_i,
                     &goertzle_0_q, &goertzle_1_i, &goertzle_1_q, &goertzle_shift);
  goertzle_0_i_val = ((long long)goertzle_0_i) << ((long long)goertzle_shift);
  goertzle_0_q_val = ((long long)goertzle_0_q) << ((long long)goertzle_shift);
  goertzle_0_i_val = goertzle_0_i_val >> 8;
  goertzle_0_q_val = goertzle_0_q_val >> 8;
  power            = ((long long)(goertzle_0_i_val * goertzle_0_i_val) +
           (long long)(goertzle_0_q_val * goertzle_0_q_val)) >>
          GOERTZLE_SHIFT_NUM_IMBALANCE;

  return power;
}

long long rx_bw_cal_get_goertzle_power(rf_chip_phy_t* phy, int channel, int goertzle_wb_div32) {
  int goertzle_0_i   = 0;
  int goertzle_0_q   = 0;
  int goertzle_1_i   = 0;
  int goertzle_1_q   = 0;
  int goertzle_shift = 0;
  //int goertzle_wb_div32 = 10;

  long long goertzle_0_i_val;
  long long goertzle_0_q_val;
  long long goertzle_1_i_val;
  long long goertzle_1_q_val;
  long long power;

  fn_rx_run_goertzle(phy, channel, 0, goertzle_wb_div32, goertzle_wb_div32, 8192, 4, &goertzle_0_i,
                     &goertzle_0_q, &goertzle_1_i, &goertzle_1_q, &goertzle_shift);
  goertzle_0_i_val = ((long long)goertzle_0_i) << ((long long)goertzle_shift);
  goertzle_0_q_val = ((long long)goertzle_0_q) << ((long long)goertzle_shift);
  goertzle_0_i_val = goertzle_0_i_val >> 8;
  goertzle_0_q_val = goertzle_0_q_val >> 8;
  power            = ((long long)(goertzle_0_i_val * goertzle_0_i_val) +
           (long long)(goertzle_0_q_val * goertzle_0_q_val)) >>
          40;

  return power;
}

int confim_rx_bw_cal_value(rf_chip_phy_t* phy, int channel, int* calVal) {
  const unsigned short RxBwCal_Val_Def[BW_CNT] = {0x200, 0x200, 0x200, 0x200, 0x200, 0x100, 0x0A0, 0x050, 0x030};

  if (phy->config->custom_bandwith_flag) {

  } else {
    if ((*calVal >= ((unsigned short)RxBwCal_Val_Def[phy->config->bandwith] * 0.7)) &&
        (*calVal <= ((unsigned short)RxBwCal_Val_Def[phy->config->bandwith] * 1.3))) {
      LOG_MDEBUG(phy, RX_BW_CAL, "Cal Value 0x%03x is expectant.\n", *calVal);
    } else {
      *calVal = RxBwCal_Val_Def[phy->config->bandwith];
      LOG_MDEBUG(phy, RX_BW_CAL, "Cal Value 0x%03x, Expectant:0x%03x(+/-30). So, we'll used default value:0x%02x.\n",
                 *calVal, RxBwCal_Val_Def[phy->config->bandwith], RxBwCal_Val_Def[phy->config->bandwith]);
    }
  }

  return 0;
}

int do_rx_bw_cal(rf_chip_phy_t* phy, int channel, long long half_power, unsigned int rx_ctune_reg1,
                 unsigned int rx_ctune_reg2, unsigned int rx_adc_reg1, unsigned int rx_adc_reg2, int goertzle_wb_div32) {

  short ret, find_best_tune_val = 0;
  int tune_val_min = 0;
  int tune_val_max = RX_BW_CAL_TUNE_MAX_VAL;
  int tune_val_mid;
  unsigned int rx_ctune_reg1_val;
  unsigned int rx_ctune_reg2_val;
  int old_ctune_val, old_powerOffset, powerOffset;
  int power, fine_tune_val, fine_power, fine_power_diff, power_diff, mean_power = 0, try_num = 1;

  int goertzle_0_i   = 0;
  int goertzle_0_q   = 0;
  int goertzle_1_i   = 0;
  int goertzle_1_q   = 0;
  int goertzle_shift = 0;
  long long goertzle_0_i_val;
  long long goertzle_0_q_val;
  long long goertzle_1_i_val;
  long long goertzle_1_q_val;
  long long goertzle_power, goertzle_halfPower, old_goertzle_power;

  while (tune_val_min <= tune_val_max) {
    old_goertzle_power = goertzle_power;
    old_ctune_val      = tune_val_mid;
    tune_val_mid       = (tune_val_min + tune_val_max) / 2;
    rx_ctune_reg1_val  = (tune_val_mid >> 8) & 0x3;
    rx_ctune_reg2_val  = tune_val_mid & 0xff;
    hal_spi_write_reg(phy, rx_ctune_reg1, rx_ctune_reg1_val);
    hal_spi_write_reg(phy, rx_ctune_reg2, rx_ctune_reg2_val);
    CHIP_DELAY(1);

    goertzle_power = rx_bw_cal_get_goertzle_power(phy, channel, goertzle_wb_div32);
    LOG_MDEBUG(phy, RX_BW_CAL, "rx bw cal, tune val:%u, 0x%x:0x%x, 0x%x:0x%x, goertzle_power:%lld, half power:%lld\n",
               tune_val_mid, rx_ctune_reg1, rx_ctune_reg1_val, rx_ctune_reg2, rx_ctune_reg2_val, goertzle_power, half_power);
    if (goertzle_power < 0) {
      LOG_ERROR("rx bw cal, Failed to get the rx bw power\n");
      return -1;
    }

    if (goertzle_power > half_power) {
      tune_val_min = tune_val_mid + 1;
    } else if (goertzle_power < half_power) {
      tune_val_max = tune_val_mid - 1;
    } else {
      find_best_tune_val = 1;
      break;
    }
  }

  //fine tune
  if (!find_best_tune_val) {
    old_powerOffset = old_goertzle_power - half_power;
    if (old_powerOffset < 0)
      old_powerOffset = old_powerOffset * -1;
    powerOffset = goertzle_power - half_power;

    if (powerOffset < 0)
      powerOffset = powerOffset * -1;

    if (old_powerOffset < powerOffset)
      fine_tune_val = old_ctune_val;
    else
      fine_tune_val = tune_val_mid;

    confim_rx_bw_cal_value(phy, channel, &fine_tune_val);

    rx_ctune_reg1_val = (fine_tune_val >> 8) & 0x3;
    rx_ctune_reg2_val = fine_tune_val & 0xff;
    hal_spi_write_reg(phy, rx_ctune_reg1, rx_ctune_reg1_val);
    hal_spi_write_reg(phy, rx_ctune_reg2, rx_ctune_reg2_val);

    goertzle_power = rx_bw_cal_get_goertzle_power(phy, channel, goertzle_wb_div32);
    LOG_MDEBUG(phy, RX_BW_CAL, "rx bw cal, the final tune val:%u, 0x%x:0x%x, 0x%x:0x%x, goertzle_power = %lld, half_power = %lld\n",
               fine_tune_val, rx_ctune_reg1, rx_ctune_reg1_val, rx_ctune_reg2, rx_ctune_reg2_val, goertzle_power, half_power);
  }

  return 0;
}

short rx_bw_lut_update(rf_chip_phy_t* phy, BANDWITH_ENUM bw, unsigned short rx_ctune_reg1, unsigned short rx_ctune_reg2) {
  short ret;
  int addr1, addr2;
  unsigned char reg_h_val, reg_l_val, offset1, offset2;

  if (rx_ctune_reg1 == 0x65A) {
    addr1   = 659 + 87 * bw;
    offset1 = 3;

    addr2   = 660 + 87 * bw;
    offset2 = 0;
  } else {
    addr1   = 663 + 87 * bw;
    offset1 = 0;

    addr2   = 663 + 87 * bw;
    offset2 = 1;
  }

  reg_h_val = hal_spi_read_reg(phy, rx_ctune_reg1);
  ret       = write_lut_byte(phy, TRX_BW_LUT, bw * 11, rx_ctune_reg1, addr1, offset1, reg_h_val);
  if (ret != 0) {
    //LOG_INFO("Failed to write the val of reg 0x%0x to LUT\n", rx_ctune_reg1);
    return -1;
  }

  reg_l_val = hal_spi_read_reg(phy, rx_ctune_reg2);
  ret       = write_lut_byte(phy, TRX_BW_LUT, bw * 11, rx_ctune_reg2, addr2, offset2, reg_l_val);
  if (ret != 0) {
    //LOG_INFO("Failed to write the val of reg 0x%0x to LUT\n", rx_ctune_reg2);
    return -1;
  }

  if (((reg_h_val == 0x00) && (reg_l_val == 0x00)) || ((reg_h_val == 0x03) && (reg_l_val == 0xFF))) {

    LOG_ERROR("Rx BW Cal, wrong cal result: reg_h_val=0x%02x, reg_l_val=0x%02x\n", reg_h_val, reg_l_val);
    return -1;
  }

  return 0;
}

void stop_edge_signal(rf_chip_phy_t* phy, int channel) {
  //fn_fpga_tx_send_tone (0, 2, 2);
  fn_tx_send_tone(phy, channel, 0, 0);
}

void send_edge_signal(rf_chip_phy_t* phy, int channel, BANDWITH_ENUM bandwith) {
  int freq_index;
  long g_3dbband_freq;

  freq_index     = g_band_bbrate[bandwith] / 64;
  g_3dbband_freq = g_3db_band[bandwith] / 1000;  //unit kHz
  //fn_fpga_tx_send_tone (1, g_3dbband_freq / freq_index, 2);
  fn_tx_send_tone(phy, channel, 1, g_3dbband_freq / freq_index + 1);
}

void send_cordic_signal(rf_chip_phy_t* phy, int channel, BANDWITH_ENUM bandwith, short en, long freq) {
  int freq_index;
  long g_3dbband_freq;

  //freq_index = g_band_bbrate[bandwith] / 64;
  //g_3dbband_freq = freq / 1000; //unit kHz
  //fn_tx_send_tone(channel ,  en,   g_3dbband_freq / freq_index +1);
  if (phy->config->syspll_cfg_flag || phy->config->custom_bandwith_flag)
    fn_tx_send_tone(phy, channel, en, (freq << 6) / (phy->config->bb_sample_rate));
  else
    fn_tx_send_tone(phy, channel, en, (freq << 6) / (g_band_bbrate[bandwith] * 1000));
}

int get_edge_power(rf_chip_phy_t* phy, int channel, int* power) {
  int try_num    = 20;
  int mean_power = 0;

  hal_spi_write_reg(phy, 0x0DD, 0xF3);
  *power = 0;
  if (rx_get_adc_power(phy, channel, try_num, &mean_power) < 0) {
    hal_spi_write_reg(phy, 0x0DD, 0xF0);
    return -1;
  }
  *power = mean_power / try_num;
  hal_spi_write_reg(phy, 0x0DD, 0xF0);
  return 0;
}

int get_dc_power(rf_chip_phy_t* phy, int channel, int* power) {
  int try_num    = 20;
  int mean_power = 0;
  int max_times  = 10;
  short retry    = 0;

  *power = 0;
  hal_spi_write_reg(phy, 0x0DD, 0xF3);

  if (rx_get_adc_power(phy, channel, try_num, &mean_power) < 0) {
    hal_spi_write_reg(phy, 0x0DD, 0xF0);
    if (channel == TRX_CHN1) {
      hal_spi_write_reg(phy, 0x7c2, 0x90);
      hal_spi_write_reg(phy, 0x7c4, 0x90);
    } else if (channel == TRX_CHN2) {
      hal_spi_write_reg(phy, 0x7d2, 0x90);
      hal_spi_write_reg(phy, 0x7d4, 0x90);
    }
    return -1;
  }
  *power = mean_power / try_num;

  hal_spi_write_reg(phy, 0x0DD, 0xF0);
  return 0;
}

void if_loopback_rxfe_off(rf_chip_phy_t* phy) {
  hal_spi_write_reg(phy, 0x848, 0x30);  // Tx1 Tx2 ABB LPBK  EN
  hal_spi_write_reg(phy, 0x849, 0xff);  /// LBSW  RX1FIP _TX1FIP / RX2FIP _TX2FIP  RX1FQN _TX1FQN / RX2FQN _TX2FIQN  En
  hal_spi_write_reg(phy, 0x84b, 0x11);  // Tx1 Tx2 ABB LPBK  RES

  hal_spi_write_reg(phy, 0x660, 0x0);  /// RX1FE_MIX  BP  BN  AP  AN  disable
  hal_spi_write_reg(phy, 0x661, 0x0);  /// RX1FE_MIX  CN  CP disable

  hal_spi_write_reg(phy, 0x67e, 0x0);  /// RX2FE_MIX  BP  BN  AP  AN  disable
  hal_spi_write_reg(phy, 0x67f, 0x0);  /// RX2FE_MIX  CN  CP disable

  hal_spi_write_reg(phy, 0x700, 0x0);  /// TX1 PA  Port
  hal_spi_write_reg(phy, 0x748, 0x0);  /// TX2 PA  Port
}

void rx_bw_config_tone_tx(rf_chip_phy_t* phy, int chnl, BANDWITH_ENUM bw, CHIP_MODE_ENUM mode) {
  LOG_MDEBUG(phy, RX_BW_CAL, "Used Digit Api. Set Mode: %d\n", mode);
  ENSM_MANAUL_ENA(phy, 1, FSM_IDLE);
  DIG_RX_FILTER_CFG(phy, chnl, bw, 1);
  DIG_TX_FILTER_CFG(phy, chnl, bw, 1);
  DIG_CHAN_ENA(phy, 0, chnl, 1);
  DIG_CHAN_ENA(phy, 1, chnl, 1);

  if (mode >= CH1_TDD)
    ENSM_MANAUL_ENA(phy, 1, FSM_TDD_RX);
  else
    ENSM_MANAUL_ENA(phy, 1, FSM_FDD);
}

void init_ctune(rf_chip_phy_t* phy, int channel, BANDWITH_ENUM bandwith) {
  unsigned int rx_ctune_reg1, rx_ctune_reg2, rx_ctrl_reg;
  unsigned int ctune_val;

  if (channel == TRX_CHN1) {
    rx_ctune_reg1 = 0x65A;
    rx_ctune_reg2 = 0x65B;
    rx_ctrl_reg   = 0x650;
  } else {
    rx_ctune_reg1 = 0x678;
    rx_ctune_reg2 = 0x679;
    rx_ctrl_reg   = 0x66E;
  }

  if (phy->config->custom_bandwith_flag) {
    if (phy->config->custom_bandwith <= 20000000)
      ctune_val = (int)(phy->rx_bw_setting[11] << 8 | phy->rx_bw_setting[12]) / 2 / 4;
    else if ((phy->config->custom_bandwith > 20000000) && (phy->config->custom_bandwith <= 50000000))
      ctune_val = (int)(phy->rx_bw_setting[11] << 8 | phy->rx_bw_setting[12]) / 2 / 3;
    else if ((phy->config->custom_bandwith > 50000000) && (phy->config->custom_bandwith <= 70000000))
      ctune_val = (int)(phy->rx_bw_setting[11] << 8 | phy->rx_bw_setting[12]) / 2 / 2;
    else
      ctune_val = (int)(phy->rx_bw_setting[11] << 8 | phy->rx_bw_setting[12]) / 2;
  } else {
    ctune_val = g_band_ctune_init[bandwith];
#if (CUSTOMER_FOR_HHH)
    if (bandwith == BW_LTE100)
      ctune_val = 0x40;
#endif
  }

  /* config & init ctune */
  hal_spi_write_reg(phy, rx_ctrl_reg, 0x01);
  hal_spi_write_reg(phy, rx_ctune_reg1, ((ctune_val & 0xff00) >> 8));
  hal_spi_write_reg(phy, rx_ctune_reg2, (ctune_val & 0xff));
  LOG_MDEBUG(phy, RX_BW_CAL, "ctune init: ctune_val = 0x%x, reg[0x%0x]=0x%0x, reg[0x%0x]=0x%0x\n",
             ctune_val, rx_ctune_reg1, hal_spi_read_reg(phy, rx_ctune_reg1), rx_ctune_reg2, hal_spi_read_reg(phy, rx_ctune_reg2));
}

short rx_bw_cal(rf_chip_phy_t* phy, TRX_CHN_ENUM channel) {
  int ret = 0;
  unsigned long long flo, flo_bak, tx_flo;
  long bandwith;
  long inband_freq;
  long g_3dbband_freq;
  int freq_index;
  unsigned int rx_ctune_reg1, rx_ctune_reg2, rx_ctrl_reg;
  unsigned int rx_adc_reg1, rx_adc_reg2;
  unsigned int reg_val;
  int dc_power, half_band_power, power, half_power, try_num, mean_power;
  unsigned int q_rxbbf_offset_tia_reg, i_rxbbf_offset_tia_reg;
  unsigned int q_rxbbf_offset_bq_reg, i_rxbbf_offset_bq_reg;
  unsigned int bbf_gain_reg;
  unsigned int rx_bbf_val;
  unsigned int rg_DE_val, rg_604_val;
  int tmpTxGainVal;
  int regTxGan = (channel == TRX_CHN1) ? 0xE6 : 0xE8;

  int i;
  int goertzle_0_i      = 0;
  int goertzle_0_q      = 0;
  int goertzle_1_i      = 0;
  int goertzle_1_q      = 0;
  int goertzle_shift    = 0;
  int goertzle_wb_div32 = 10;

  long long goertzle_0_i_val;
  long long goertzle_0_q_val;
  long long goertzle_1_i_val;
  long long goertzle_1_q_val;
  long long goertzle_power, goertzle_halfPower;
  int wb_div_64;
  int rg_rx_hg_offset;
  int rg_rx_lg_offset;
  long tone_freq;
  int rg_65C_val;
  int rg_67A_val;

  if (((hal_spi_read_reg(phy, 0xD33) & 0xF0) >> 4) != 0x07) {
    LOG_ERROR("Reg 0xD33 Err.\n");
    return -1;
  }

  if (phy->rx_bw_cal_flag[channel]) {
    rg_rx_hg_offset = (channel == TRX_CHN1) ? 0 : -1;
    rg_rx_lg_offset = (channel == TRX_CHN1) ? 0 : 1;

    if (channel == TRX_CHN1) {
      rx_ctune_reg1 = 0x65A;
      rx_ctune_reg2 = 0x65B;
    } else {
      rx_ctune_reg1 = 0x678;
      rx_ctune_reg2 = 0x679;
    }

    hal_spi_write_reg(phy, RG_RX1_BBF_TIAPPL_RBANK_D32 + rg_rx_hg_offset, phy->rx_imbalance_cal[channel][0]);
    hal_spi_write_reg(phy, RG_RX1_BBF_TIAPPL_RBANKLG_D36 + rg_rx_lg_offset, phy->rx_imbalance_cal[channel][1]);
    hal_spi_write_reg(phy, rx_ctune_reg1, phy->rx_bw_cal[channel][0]);
    hal_spi_write_reg(phy, rx_ctune_reg2, phy->rx_bw_cal[channel][1]);

    /* update lut */
    ret = rx_bw_lut_update(phy, phy->config->bandwith, rx_ctune_reg1, rx_ctune_reg2);
    if (ret != 0) {
      phy->error = -RX_BW_CAL_FAIL;
      LOG_ERROR("Failed to update bw info to	the LUT\n");
      //return -1;
    }
  } else {
    //if(! phy->config->custom_bandwith_flag)
    //	inbalance_cal(phy, channel);

    flo = phy->config->rx_flo;
    if (channel == TRX_CHN1) {
      bbf_gain_reg = 0x65C;
      rx_bbf_val   = hal_spi_read_reg(phy, bbf_gain_reg);
      hal_spi_write_reg(phy, bbf_gain_reg, _RX_BBF_GAIN);

      hal_spi_write_reg(phy, 0x837, 0x0);
      rx_ctune_reg1 = 0x65A;
      rx_ctune_reg2 = 0x65B;
      rx_ctrl_reg   = 0x650;

      init_ctune(phy, channel, phy->config->bandwith);
      HAL_REG_CLR_BIT(phy, 0x651, BIT7);

      rx_adc_reg1 = 0x696;
      rx_adc_reg2 = 0x697;

      rg_65C_val = hal_spi_read_reg(phy, 0x65C);
      hal_spi_write_reg(phy, 0x65C, 0x0C);
    } else if (channel == TRX_CHN2) {
      bbf_gain_reg = 0x67A;
      rx_bbf_val   = hal_spi_read_reg(phy, bbf_gain_reg);
      hal_spi_write_reg(phy, bbf_gain_reg, _RX_BBF_GAIN);

      hal_spi_write_reg(phy, 0x836, 0x0);
      rx_ctune_reg1 = 0x678;
      rx_ctune_reg2 = 0x679;
      rx_ctrl_reg   = 0x66E;

      init_ctune(phy, channel, phy->config->bandwith);
      HAL_REG_CLR_BIT(phy, 0x66F, BIT7);

      rx_adc_reg1 = 0x6B4;
      rx_adc_reg2 = 0x6B5;

      rg_67A_val = hal_spi_read_reg(phy, 0x67A);
      hal_spi_write_reg(phy, 0x67A, 0x0C);
    } else {
      LOG_ERROR("Wrongly channel setting\n");
    }

    //rg_DE_val = hal_spi_read_reg(phy, 0x0de);
    //rg_604_val = hal_spi_read_reg(phy, 0x604);

    rx_bw_config_tone_tx(phy, channel, phy->config->bandwith, phy->config->mode);
    if_loopback_rxfe_off(phy);

    tmpTxGainVal = hal_spi_read_reg(phy, regTxGan);
    hal_spi_write_reg(phy, regTxGan, 0x00);  // TX  GAIN

    if (phy->config->custom_bandwith_flag) {
      //goertzle_wb_div32 = (int)round(32 * phy->config->custom_bandwith / 2000.0 / g_band_bbrate[phy->config->bandwith]/1.0);
      goertzle_wb_div32 = (int)round(32 * phy->config->custom_bandwith / 2.0 / phy->config->bb_sample_rate / 1.0);
      LOG_MDEBUG(phy, RX_BW_CAL, "phy->config->bandwith = %d, BB Sampling rate = %ld, custom_bandwith = %ld, custom_bandwith_flag = %d\n",
                 phy->config->bandwith, phy->config->bb_sample_rate / 1000,
                 phy->config->custom_bandwith, phy->config->custom_bandwith_flag);
    } else if (phy->config->syspll_cfg_flag) {
      goertzle_wb_div32 = (int)round(32 * g_3db_band[phy->config->bandwith] / phy->config->bb_sample_rate / 1.0);
      LOG_MDEBUG(phy, RX_BW_CAL, "phy->config->bandwith = %d, BB Sampling rate = %ld, custom_bandwith = %ld, custom_bandwith_flag = %d\n",
                 phy->config->bandwith, phy->config->bb_sample_rate / 1000,
                 phy->config->custom_bandwith, phy->config->custom_bandwith_flag);
    } else {
      goertzle_wb_div32 = (int)round(32 * g_3db_band[phy->config->bandwith] / g_band_bbrate[phy->config->bandwith] / 1000.0 / 1.0);
      LOG_MDEBUG(phy, RX_BW_CAL, "phy->config->bandwith = %d, BB Sampling rate = %ld, custom_bandwith = %ld, custom_bandwith_flag = %d\n",
                 phy->config->bandwith, g_band_bbrate[phy->config->bandwith],
                 phy->config->custom_bandwith, phy->config->custom_bandwith_flag);
    }

    do {
      wb_div_64 = goertzle_wb_div32 * 2;
      if ((phy->config->custom_bandwith_flag) || (phy->config->syspll_cfg_flag))
        tone_freq = goertzle_wb_div32 * phy->config->bb_sample_rate / 32000;
      else
        tone_freq = goertzle_wb_div32 * g_band_bbrate[phy->config->bandwith] / 32;
      fn_tx_send_tone(phy, channel, 1, wb_div_64);
      CHIP_DELAY(1);
      goertzle_power     = rx_bw_cal_get_goertzle_power(phy, channel, goertzle_wb_div32);
      goertzle_halfPower = goertzle_power / 2;
      LOG_MDEBUG(phy, RX_BW_CAL, "rx bw cal, tone = %d K, goertzle_wb_div32 = %d,goertzle_power = %lld,0x604 = 0x%x\n",
                 tone_freq, goertzle_wb_div32, goertzle_power, hal_spi_read_reg(phy, 0x604));
    } while (0);

    /* cali */
    hal_spi_write_reg(phy, 0x0DD, 0xF3);
    if (do_rx_bw_cal(phy, channel, goertzle_halfPower, rx_ctune_reg1, rx_ctune_reg2, rx_adc_reg1, rx_adc_reg2, goertzle_wb_div32) < 0) {
      phy->error = -RX_BW_CAL_FAIL;
      return -1;
    }
    hal_spi_write_reg(phy, 0x0DD, 0xF0);
    if (phy->config->custom_bandwith_flag)
      LOG_MDEBUG(phy, RX_BW_CAL, "rx bw cal, custom %ldMHZ bandwidth tune end\n", phy->config->custom_bandwith / 1000000);
    else
      LOG_MDEBUG(phy, RX_BW_CAL, "rx bw cal, %ldMHZ bandwidth tune end\n", g_bandwith[phy->config->bandwith] / 1000000);
    fn_tx_send_tone(phy, channel, 0, 0);

    //////////////////
    if (channel == TRX_CHN1)
      hal_spi_write_reg(phy, 0x65C, rg_65C_val);
    else
      hal_spi_write_reg(phy, 0x67A, rg_67A_val);

    hal_spi_write_reg(phy, regTxGan, tmpTxGainVal);  // TX  GAIN
    //hal_spi_write_reg(phy, 0x0de, rg_DE_val);
    //hal_spi_write_reg(phy, 0x604, rg_604_val);

    /* update lut */
    phy->rx_bw_cal[channel][0] = hal_spi_read_reg(phy, rx_ctune_reg1);
    phy->rx_bw_cal[channel][1] = hal_spi_read_reg(phy, rx_ctune_reg2);
    ret                        = rx_bw_lut_update(phy, phy->config->bandwith, rx_ctune_reg1, rx_ctune_reg2);
    if (ret != 0) {
      phy->error = -RX_BW_CAL_FAIL;
      LOG_ERROR("Failed to update bw info to	the LUT\n");
      //return -1;
    }

    hal_spi_write_reg(phy, bbf_gain_reg, rx_bbf_val);
    hal_spi_write_reg(phy, 0x660, 0x00);
    hal_spi_write_reg(phy, 0x661, 0x00);
    hal_spi_write_reg(phy, 0x67e, 0x00);
    hal_spi_write_reg(phy, 0x67f, 0x00);

    /* close lookback */
    hal_spi_write_reg(phy, 0x846, 0x00);
    hal_spi_write_reg(phy, 0x847, 0x00);
    hal_spi_write_reg(phy, 0x848, 0x00);

    /* reset digif */
    DIG_SOFT_RST_N(phy, 0);

    if ((phy->config->bandwidthswitch_flag != 1) || (phy->config->bandwidthswitch_length <= 1)) {
      phy->rx_bw_cal_flag[channel] = 1;
    }
  }

  return ret;
}

/****************************************************************************************/
// rx rssi
/****************************************************************************************/
int rx_rssi_get(rf_chip_phy_t* phy, int channel) {
  int rssi_val;

  rx_cfg_rssi_init(phy, channel);
  CHIP_DELAY(1);  //1MS
  rssi_val = rx_cfg_rssi_read(phy, channel);
  rssi_val = 0 - rssi_val + 3000;

  return rssi_val;
}

void rx_cfg_rssi_init(rf_chip_phy_t* phy, int channel) {
#define RSSI_CFG_RX1_0_E59                0xE59
#define RSSI_CFG_RX1_0_SPI_RSSI_RESET     (0 << 7)
#define RSSI_CFG_RX1_0_SPI_RSSI_START_CLR (0 << 6)
#define RSSI_CFG_RX1_0_SPI_RSSI_START_EN  (1 << 6)

#define RSSI_CFG_RX1_0_SPI_RSSI_DIN_SEL     (0 << 4)
#define RSSI_CFG_RX1_0_SPI_RSSI_MEAS_MODE   (0 << 3)
#define RSSI_CFG_RX1_0_SPI_RSSI_TRIGER_MODE (4 << 0)
#define RSSI_CFG_RX1_1_E5A                  0xE5A
#define RSSI_CFG_RX1_1_DELAY_TIME           (20)
#define RSSI_CFG_RX1_2_E5B                  0xE5B
#define RSSI_CFG_RX1_2_WAIT_TIME            (20)
#define RSSI_CFG_RX1_3_E5C                  0xE5C
#define RSSI_CFG_RX1_3_DURATION_0           (10)
#define RSSI_CFG_RX1_3_DURATION_1           (10 << 4)
#define RSSI_CFG_RX1_4_E5D                  0xE5D
#define RSSI_CFG_RX1_4_DURATION_2           (10)
#define RSSI_CFG_RX1_4_DURATION_3           (10 << 4)
#define RSSI_CFG_RX1_5_E5E                  0xE5E
#define RSSI_CFG_RX1_5_WEIGHT_COEFF_0       (0)
#define RSSI_CFG_RX1_6_E5F                  0xE5F
#define RSSI_CFG_RX1_6_WEIGHT_COEFF_1       (0)
#define RSSI_CFG_RX1_7_E60                  0xE60
#define RSSI_CFG_RX1_7_WEIGHT_COEFF_2       (0)
#define RSSI_CFG_RX1_8_E61                  0xE61
#define RSSI_CFG_RX1_8_WEIGHT_COEFF_3       (0)
#define GCTRL_RPT_RX1_0_E63                 0xE63
#define GCTRL_RPT_RX1_0_E63_PULSE_EN        (1 << 7)
#define GCTRL_RPT_RX1_0_E63_PULSE_CLR       (0 << 7)

#define RSSI_CFG_RX2_0_ED9 0xED9
#define RSSI_RX1RX2_OFFSET (RSSI_CFG_RX2_0_ED9 - RSSI_CFG_RX1_0_E59)

  int offset = 0;
  if (TRX_CHN2 == channel)
    offset = RSSI_RX1RX2_OFFSET;
  else
    offset = 0;

  // start = 0;
  hal_spi_write_reg(phy, RSSI_CFG_RX1_0_E59 + offset, RSSI_CFG_RX1_0_SPI_RSSI_RESET | RSSI_CFG_RX1_0_SPI_RSSI_START_CLR | RSSI_CFG_RX1_0_SPI_RSSI_DIN_SEL | RSSI_CFG_RX1_0_SPI_RSSI_MEAS_MODE | RSSI_CFG_RX1_0_SPI_RSSI_TRIGER_MODE);
  hal_spi_write_reg(phy, RSSI_CFG_RX1_1_E5A + offset, RSSI_CFG_RX1_1_DELAY_TIME);
  hal_spi_write_reg(phy, RSSI_CFG_RX1_2_E5B + offset, RSSI_CFG_RX1_2_WAIT_TIME);
  hal_spi_write_reg(phy, RSSI_CFG_RX1_3_E5C + offset, RSSI_CFG_RX1_3_DURATION_0 | RSSI_CFG_RX1_3_DURATION_1);
  hal_spi_write_reg(phy, RSSI_CFG_RX1_4_E5D + offset, RSSI_CFG_RX1_4_DURATION_2 | RSSI_CFG_RX1_4_DURATION_3);
  hal_spi_write_reg(phy, RSSI_CFG_RX1_5_E5E + offset, RSSI_CFG_RX1_5_WEIGHT_COEFF_0);
  hal_spi_write_reg(phy, RSSI_CFG_RX1_6_E5F + offset, RSSI_CFG_RX1_6_WEIGHT_COEFF_1);
  hal_spi_write_reg(phy, RSSI_CFG_RX1_7_E60 + offset, RSSI_CFG_RX1_7_WEIGHT_COEFF_2);
  hal_spi_write_reg(phy, RSSI_CFG_RX1_8_E61 + offset, RSSI_CFG_RX1_8_WEIGHT_COEFF_3);

  // start = 1;
  hal_spi_write_reg(phy, RSSI_CFG_RX1_0_E59 + offset, RSSI_CFG_RX1_0_SPI_RSSI_RESET | RSSI_CFG_RX1_0_SPI_RSSI_START_EN | RSSI_CFG_RX1_0_SPI_RSSI_DIN_SEL | RSSI_CFG_RX1_0_SPI_RSSI_MEAS_MODE | RSSI_CFG_RX1_0_SPI_RSSI_TRIGER_MODE);

  // start = 0;
  hal_spi_write_reg(phy, RSSI_CFG_RX1_0_E59 + offset, RSSI_CFG_RX1_0_SPI_RSSI_RESET | RSSI_CFG_RX1_0_SPI_RSSI_START_CLR | RSSI_CFG_RX1_0_SPI_RSSI_DIN_SEL | RSSI_CFG_RX1_0_SPI_RSSI_MEAS_MODE | RSSI_CFG_RX1_0_SPI_RSSI_TRIGER_MODE);
}

int rx_cfg_rssi_read(rf_chip_phy_t* phy, int channel) {
#define RSSI_RPT_RX1_0_SYMBOL_E72   0xE72
#define RSSI_RPT_RX1_1_PREAMBLE_E73 0xE73
#define RSSI_RPT_RX1_2_PREAMBLE_E74 0xE74
#define RSSI_RPT_RX2_0_SYMBOL_EF2   0xEF2
#define RSSI_RX1RX2_OFFSET1         (RSSI_RPT_RX2_0_SYMBOL_EF2 - RSSI_RPT_RX1_0_SYMBOL_E72)
  int offset = 0;
  int symbolVal_l, symbolVal_h;
  int preambleVal_l, preambleVal_h;
  int rssi_symbol_val;
  int rssi_preamble_val;
  int tmpVal;

  if (TRX_CHN2 == channel)
    offset = RSSI_RX1RX2_OFFSET1;
  else
    offset = 0;

  hal_spi_write_reg(phy, GCTRL_RPT_RX1_0_E63 + offset, GCTRL_RPT_RX1_0_E63_PULSE_EN);
  hal_spi_write_reg(phy, GCTRL_RPT_RX1_0_E63 + offset, GCTRL_RPT_RX1_0_E63_PULSE_CLR);

  symbolVal_l = hal_spi_read_reg(phy, RSSI_RPT_RX1_0_SYMBOL_E72 + offset);
  symbolVal_h = hal_spi_read_reg(phy, RSSI_RPT_RX1_2_PREAMBLE_E74 + offset);

  preambleVal_l = hal_spi_read_reg(phy, RSSI_RPT_RX1_1_PREAMBLE_E73 + offset);
  preambleVal_h = hal_spi_read_reg(phy, RSSI_RPT_RX1_2_PREAMBLE_E74 + offset);

  tmpVal          = symbolVal_h / 8 * 256 + symbolVal_l;
  rssi_symbol_val = ((2048 - tmpVal) * 1000) / 8;

  tmpVal            = (preambleVal_h - preambleVal_h / 8 * 8) * 256 + preambleVal_l;
  rssi_preamble_val = ((2048 - tmpVal) * 1000) / 8;
  return rssi_preamble_val;
}

/****************************************************************************************/
// set tx attenuation
// range: 0 ~ 50
/****************************************************************************************/
int write_tx_qec_lol_cal_word_chn(rf_chip_phy_t* phy, TRX_CHN_ENUM chn, short bb_gain, TX_QEC_CFG_REGS* tx_qec_cfg) {
  int i = 0, end, offset, base;
  unsigned short h8, l8;
  unsigned char vals[4];

  offset = bb_gain * 2;
  base   = chn ? 512 : 0;
  for (i = 0; i < 4; i++) {
    h8      = (tx_qec_cfg->dc_q & 0xffff) >> 8;
    l8      = tx_qec_cfg->dc_q & 0xff;
    vals[0] = l8;
    vals[1] = h8;
    h8      = (tx_qec_cfg->dc_i & 0xffff) >> 8;
    l8      = tx_qec_cfg->dc_i & 0xff;
    vals[2] = l8;
    vals[3] = h8;
    write_lut_word(phy, TXRX_IQ_LUT, base + i * 50 + offset, vals);

    //h8 = (tx_qec_cfg->fiiq_imag & 0xffff) >> 8;
    //l8 = tx_qec_cfg->fiiq_imag & 0xff;
    //vals[0] = h8;
    //vals[1] = l8;
    h8      = (tx_qec_cfg->fiiq_real & 0xffff) >> 8;
    l8      = tx_qec_cfg->fiiq_real & 0xff;
    vals[0] = l8;
    vals[1] = h8;
    h8      = (tx_qec_cfg->fiiq_imag & 0xffff) >> 8;
    l8      = tx_qec_cfg->fiiq_imag & 0xff;
    vals[2] = l8;
    vals[3] = h8;
    write_lut_word(phy, TXRX_IQ_LUT, base + i * 50 + 1 + offset, vals);
  }

  return 0;
}

void real_time_update_tx_qec(rf_chip_phy_t* phy, TRX_CHN_ENUM chn, TX_QEC_CFG_REGS* tx_qec_cfg) {
  short offset;

  offset = (chn == TRX_CHN1) ? 0 : 0x800;
  hal_spi_write_reg(phy, 0x240 + offset, tx_qec_cfg->dc_i & 0xff);
  hal_spi_write_reg(phy, 0x241 + offset, (tx_qec_cfg->dc_i & 0xffff) >> 8);
  hal_spi_write_reg(phy, 0x244 + offset, tx_qec_cfg->dc_q & 0xff);
  hal_spi_write_reg(phy, 0x245 + offset, (tx_qec_cfg->dc_q & 0xffff) >> 8);
  hal_spi_write_reg(phy, 0x248 + offset, tx_qec_cfg->fiiq_imag & 0xff);
  hal_spi_write_reg(phy, 0x249 + offset, (tx_qec_cfg->fiiq_imag & 0xffff) >> 8);
  hal_spi_write_reg(phy, 0x24C + offset, tx_qec_cfg->fiiq_real & 0xff);
  hal_spi_write_reg(phy, 0x24D + offset, (tx_qec_cfg->fiiq_real & 0xffff) >> 8);
  HAL_REG_SET_BIT(phy, 0x203 + offset, BIT1);
  CHIP_DELAY(1);
  HAL_REG_CLR_BIT(phy, 0x203 + offset, BIT1);
}

short set_tx_atten_cfg(rf_chip_phy_t* phy, unsigned long long flo, unsigned char gain) {
  char tx_pa_gain_index, tx_bb_gain_index;
  short index;
  unsigned char i;

  tx_pa_gain_index = g_tx_gain_index[gain][0];
  tx_bb_gain_index = g_tx_gain_index[gain][1];
  index            = find_case_with_flo(flo);

  for (i = 0; i < 20; i++)
    phy->tx_pa_gain[i] = g_tx_pa_gain_config[index * 9 + tx_pa_gain_index][i];
  for (i = 0; i < 16; i++)
    phy->tx_bb_gain[i] = phy->tx_bb_gain_config[tx_bb_gain_index + 0][i];

  return 0;
}

short set_tx_atten_chn(rf_chip_phy_t* phy, int chn, unsigned long long flo, unsigned char gain, short immed) {
  char tx_pa_gain_index, tx_bb_gain_index;
  unsigned short reg_val;
  short index, offset;
  unsigned char i;

  if (gain > 50) {
    phy->error = -PARA_INVALID;
    return -PARA_INVALID;
  }

  tx_pa_gain_index   = g_tx_gain_index[gain][0];
  tx_bb_gain_index   = g_tx_gain_index[gain][1];
  phy->tx_curr_index = !phy->tx_curr_index;

  if (tx_bb_gain_index > 5) {
    phy->error = -PARA_INVALID;
    return -PARA_INVALID;
  }

  if (immed)
    HAL_REG_SET_BIT(phy, 0x0D7, BIT5);
  else
    HAL_REG_CLR_BIT(phy, 0x0D7, BIT5);

  /* find index */
  index = find_case_with_flo(flo);

  /* load tx pa gain lut into phy->tx_curr_index lut */
  offset = chn ? 10 : 0;
  for (i = 0; i < 10; i++)
    phy->tx_pa_gain[offset + i] = g_tx_pa_gain_config[index * 9 + tx_pa_gain_index][offset + i];

  for (i = 0; i < 20; i++) {
    if (phy->tx_curr_index) {
      write_lut_byte(phy, TX_GAIN_LUT, 1, g_tx_pa_gain_regs[i], 205 + i / 4, i % 4, phy->tx_pa_gain[i]);
      write_lut_byte(phy, TX_GAIN_LUT, 10, g_tx_pa_gain_regs[i], 250 + i / 4, i % 4, phy->tx_pa_gain[i]);
    } else {
      write_lut_byte(phy, TX_GAIN_LUT, 0, g_tx_pa_gain_regs[i], 200 + i / 4, i % 4, phy->tx_pa_gain[i]);
      write_lut_byte(phy, TX_GAIN_LUT, 9, g_tx_pa_gain_regs[i], 245 + i / 4, i % 4, phy->tx_pa_gain[i]);
    }
  }

  /* load tx bb gain lut into phy->tx_curr_index lut */
  offset = chn ? 8 : 0;
  for (i = 0; i < 8; i++)
    phy->tx_bb_gain[offset + i] = phy->tx_bb_gain_config[tx_bb_gain_index + 0][offset + i];

  for (i = 0; i < 16; i++) {
    if (phy->tx_curr_index) {
      write_lut_byte(phy, TX_GAIN_LUT, 1, g_tx_bb_gain_regs[i], 4 + i / 4, i % 4, phy->tx_bb_gain[i]);
      write_lut_byte(phy, TX_GAIN_LUT, 26, g_tx_bb_gain_regs[i], 104 + i / 4, i % 4, phy->tx_bb_gain[i]);
    } else {
      write_lut_byte(phy, TX_GAIN_LUT, 0, g_tx_bb_gain_regs[i], 0 + i / 4, i % 4, phy->tx_bb_gain[i]);
      write_lut_byte(phy, TX_GAIN_LUT, 25, g_tx_bb_gain_regs[i], 100 + i / 4, i % 4, phy->tx_bb_gain[i]);
    }
  }

  /* update qec config */
  if (phy->tx_qec_flag[chn]) {
    write_tx_qec_lol_cal_word_chn(phy, chn, phy->tx_curr_index, &(phy->tx_qec[chn][tx_bb_gain_index]));
    real_time_update_tx_qec(phy, chn, &(phy->tx_qec[chn][tx_bb_gain_index]));
  }

  /* tx pa gain index */
  reg_val = hal_spi_read_reg(phy, 0x0CB);
  reg_val = (phy->tx_curr_index & 0xF) | (reg_val & 0xF0);
  hal_spi_write_reg(phy, 0x0CB, reg_val);

  /* tx bb gain index */
  reg_val = hal_spi_read_reg(phy, 0x0C1);
  reg_val = (phy->tx_curr_index & 0x1F) | (reg_val & 0xE0);
  hal_spi_write_reg(phy, 0x0C1, reg_val);

  /* dig gain */
  if (chn == TRX_CHN1) {
    hal_spi_write_reg(phy, 0x0E5, g_tx_dig_gain[gain][0]);
    hal_spi_write_reg(phy, 0x0E6, g_tx_dig_gain[gain][1]);
    if (phy->config->chip_ver == 2)
      HAL_REG_SET_BIT(phy, 0x0E5, BIT7);
  } else {
    hal_spi_write_reg(phy, 0x0E7, g_tx_dig_gain[gain][2]);
    hal_spi_write_reg(phy, 0x0E8, g_tx_dig_gain[gain][3]);
    if (phy->config->chip_ver == 2)
      HAL_REG_SET_BIT(phy, 0x0E7, BIT7);
  }

  HAL_REG_CLR_BIT(phy, 0x0D7, BIT5);

  return 0;
}

short set_tx_atten(rf_chip_phy_t* phy, unsigned long long flo, unsigned char gain, short immed) {
  char tx_pa_gain_index, tx_bb_gain_index;
  unsigned short reg_val;
  short index, offset;
  unsigned char i;

  if (gain > 50) {
    phy->error = -PARA_INVALID;
    return -PARA_INVALID;
  }

  tx_pa_gain_index = g_tx_gain_index[gain][0];
  tx_bb_gain_index = g_tx_gain_index[gain][1];

  if (immed)
    HAL_REG_SET_BIT(phy, 0x0D7, BIT5);
  else
    HAL_REG_CLR_BIT(phy, 0x0D7, BIT5);

  /* tx pa gain index */
  reg_val = hal_spi_read_reg(phy, 0x0CB);
  reg_val = (tx_pa_gain_index & 0xF) | (reg_val & 0xF0);
  hal_spi_write_reg(phy, 0x0CB, reg_val);

  /* tx bb gain index */
  reg_val = hal_spi_read_reg(phy, 0x0C1);
  reg_val = (tx_bb_gain_index & 0x1F) | (reg_val & 0xE0);
  hal_spi_write_reg(phy, 0x0C1, reg_val);

  /* find index */
  index = find_case_with_flo(flo);

  /* load into registers of tx pa gain */
  for (i = 0; i < 20; i++)
    hal_spi_write_reg(phy, g_tx_pa_gain_regs[i], g_tx_pa_gain_config[index * 9 + tx_pa_gain_index][i]);

  /* load into registers of tx bb gain */
  for (i = 0; i < 16; i++)
    //spi_write_reg(g_tx_bb_gain_regs[i], g_tx_bb_gain_config[tx_bb_gain_index+0][i]);
    hal_spi_write_reg(phy, g_tx_bb_gain_regs[i], phy->tx_bb_gain_config[tx_bb_gain_index + 0][i]);

#if 0
	//When Setting tx gain, tx band setting can be ignored
	/* load into registers of tx band */
	for (i=0; i<48; i++)
		spi_write_reg(g_trx_band_regs[i], g_trx_band_config[index][i]);
#endif
#if 1
  /* dig gain */
  hal_spi_write_reg(phy, 0x0E5, g_tx_dig_gain[gain][0]);
  hal_spi_write_reg(phy, 0x0E6, g_tx_dig_gain[gain][1]);
  hal_spi_write_reg(phy, 0x0E7, g_tx_dig_gain[gain][2]);
  hal_spi_write_reg(phy, 0x0E8, g_tx_dig_gain[gain][3]);

  if (phy->config->chip_ver == 2) {
    HAL_REG_SET_BIT(phy, 0x0E5, BIT7);
    HAL_REG_SET_BIT(phy, 0x0E7, BIT7);
  }

#endif
  HAL_REG_CLR_BIT(phy, 0x0D7, BIT5);

  return 0;
}

int set_tx_atten_ext(rf_chip_phy_t* phy, int chn, int type, unsigned long long flo, int gain_index) {
  unsigned long long min_range, max_range;
  int i, index, offset;

  /* find index */
  index = find_case_with_flo(flo);

  if (type) {
    offset = chn ? 10 : 0;
    /* load into registers of tx pa gain */
    for (i = 0; i < 10; i++)
      hal_spi_write_reg(phy, g_tx_pa_gain_regs[i + offset], g_tx_pa_gain_config[index * 9 + gain_index][i + offset]);
  } else {
    offset = chn ? 8 : 0;
    /* load into registers of tx bb gain */
    for (i = 0; i < 8; i++)
      //spi_write_reg(g_tx_bb_gain_regs[i+offset], g_tx_bb_gain_config[gain_index][i+offset]);
      hal_spi_write_reg(phy, g_tx_bb_gain_regs[i + offset], phy->tx_bb_gain_config[gain_index][i + offset]);
  }

  return 0;
}

short set_tx_ana_atten_chn(rf_chip_phy_t* phy, int chn, unsigned long long flo, unsigned char gain, short immed) {
  char tx_pa_gain_index, tx_bb_gain_index;
  unsigned short reg_val;
  short index, offset;
  unsigned char i;

  if (gain > 50) {
    phy->error = -PARA_INVALID;
    return -PARA_INVALID;
  }

  tx_pa_gain_index   = g_tx_gain_index[gain][0];
  tx_bb_gain_index   = g_tx_gain_index[gain][1];
  phy->tx_curr_index = !phy->tx_curr_index;

  if (tx_bb_gain_index > 5) {
    phy->error = -PARA_INVALID;
    return -PARA_INVALID;
  }

  if (immed)
    HAL_REG_SET_BIT(phy, 0x0D7, BIT5);
  else
    HAL_REG_CLR_BIT(phy, 0x0D7, BIT5);

  /* find index */
  index = find_case_with_flo(flo);

  /* load tx pa gain lut into phy->tx_curr_index lut */
  offset = chn ? 10 : 0;
  for (i = 0; i < 10; i++)
    phy->tx_pa_gain[offset + i] = g_tx_pa_gain_config[index * 9 + tx_pa_gain_index][offset + i];

  for (i = 0; i < 20; i++) {
    if (phy->tx_curr_index) {
      write_lut_byte(phy, TX_GAIN_LUT, 1, g_tx_pa_gain_regs[i], 205 + i / 4, i % 4, phy->tx_pa_gain[i]);
      write_lut_byte(phy, TX_GAIN_LUT, 10, g_tx_pa_gain_regs[i], 250 + i / 4, i % 4, phy->tx_pa_gain[i]);
    } else {
      write_lut_byte(phy, TX_GAIN_LUT, 0, g_tx_pa_gain_regs[i], 200 + i / 4, i % 4, phy->tx_pa_gain[i]);
      write_lut_byte(phy, TX_GAIN_LUT, 9, g_tx_pa_gain_regs[i], 245 + i / 4, i % 4, phy->tx_pa_gain[i]);
    }
  }

  /* load tx bb gain lut into phy->tx_curr_index lut */
  offset = chn ? 8 : 0;
  for (i = 0; i < 8; i++)
    phy->tx_bb_gain[offset + i] = phy->tx_bb_gain_config[tx_bb_gain_index + 0][offset + i];

  for (i = 0; i < 16; i++) {
    if (phy->tx_curr_index) {
      write_lut_byte(phy, TX_GAIN_LUT, 1, g_tx_bb_gain_regs[i], 4 + i / 4, i % 4, phy->tx_bb_gain[i]);
      write_lut_byte(phy, TX_GAIN_LUT, 26, g_tx_bb_gain_regs[i], 104 + i / 4, i % 4, phy->tx_bb_gain[i]);
    } else {
      write_lut_byte(phy, TX_GAIN_LUT, 0, g_tx_bb_gain_regs[i], 0 + i / 4, i % 4, phy->tx_bb_gain[i]);
      write_lut_byte(phy, TX_GAIN_LUT, 25, g_tx_bb_gain_regs[i], 100 + i / 4, i % 4, phy->tx_bb_gain[i]);
    }
  }

  /* update qec config */
  if (phy->tx_qec_flag[chn]) {
    write_tx_qec_lol_cal_word_chn(phy, chn, phy->tx_curr_index, &(phy->tx_qec[chn][tx_bb_gain_index]));
    real_time_update_tx_qec(phy, chn, &(phy->tx_qec[chn][tx_bb_gain_index]));
  }

  /* tx pa gain index */
  reg_val = hal_spi_read_reg(phy, 0x0CB);
  reg_val = (phy->tx_curr_index & 0xF) | (reg_val & 0xF0);
  hal_spi_write_reg(phy, 0x0CB, reg_val);

  /* tx bb gain index */
  reg_val = hal_spi_read_reg(phy, 0x0C1);
  reg_val = (phy->tx_curr_index & 0x1F) | (reg_val & 0xE0);
  hal_spi_write_reg(phy, 0x0C1, reg_val);

  HAL_REG_CLR_BIT(phy, 0x0D7, BIT5);

  return 0;
}

short set_tx_ana_atten(rf_chip_phy_t* phy, unsigned long long flo, unsigned char gain, short immed) {
  char tx_pa_gain_index, tx_bb_gain_index;
  unsigned short reg_val;
  short index;
  unsigned char i;

  if (gain > 50) {
    phy->error = -PARA_INVALID;
    return -PARA_INVALID;
  }

  tx_pa_gain_index = g_tx_gain_index[gain][0];
  tx_bb_gain_index = g_tx_gain_index[gain][1];

  if (immed)
    HAL_REG_SET_BIT(phy, 0x0D7, BIT5);
  else
    HAL_REG_CLR_BIT(phy, 0x0D7, BIT5);

  /* tx pa gain index */
  reg_val = hal_spi_read_reg(phy, 0x0CB);
  reg_val = (tx_pa_gain_index & 0xF) | (reg_val & 0xF0);
  hal_spi_write_reg(phy, 0x0CB, reg_val);

  /* tx bb gain index */
  reg_val = hal_spi_read_reg(phy, 0x0C1);
  reg_val = (tx_bb_gain_index & 0x1F) | (reg_val & 0xE0);
  hal_spi_write_reg(phy, 0x0C1, reg_val);

  /* find index */
  index = find_case_with_flo(flo);

  /* load into registers of tx pa gain */
  for (i = 0; i < 20; i++)
    hal_spi_write_reg(phy, g_tx_pa_gain_regs[i], g_tx_pa_gain_config[index * 9 + tx_pa_gain_index][i]);

  /* load into registers of tx bb gain */
  for (i = 0; i < 16; i++)
    //spi_write_reg(g_tx_bb_gain_regs[i], g_tx_bb_gain_config[tx_bb_gain_index+0][i]);
    hal_spi_write_reg(phy, g_tx_bb_gain_regs[i], phy->tx_bb_gain_config[tx_bb_gain_index + 0][i]);

  HAL_REG_CLR_BIT(phy, 0x0D7, BIT5);

  return 0;
}

//unit: 0.125dB
void set_tx_dig_atten(rf_chip_phy_t* phy, TRX_CHN_ENUM chn, unsigned short index) {
  /* dig gain */
  if (chn == TRX_CHN1) {
    hal_spi_write_reg(phy, 0x0E5, (index >> 8) & 0x1);
    hal_spi_write_reg(phy, 0x0E6, index & 0xff);
    if (phy->config->chip_ver == 2)
      HAL_REG_SET_BIT(phy, 0x0E5, BIT7);
  } else {
    hal_spi_write_reg(phy, 0x0E7, (index >> 8) & 0x1);
    hal_spi_write_reg(phy, 0x0E8, index & 0xff);
    if (phy->config->chip_ver == 2)
      HAL_REG_SET_BIT(phy, 0x0E7, BIT7);
  }
}

/****************************************************************************************/
// set tx atten
// PA_Gain_Index --> unit: 3dB, range:0~8
// BB_Gain_Index --> unit: 1dB, range:0~24
// DIG_Gain_dB   --> unit: 0.125dB, range:0~400
/****************************************************************************************/
int tx_split_atten_chn(rf_chip_phy_t* phy, TRX_CHN_ENUM chn, unsigned long long flo,
                       TX_SPLIT_ATTEN_ENUM type, unsigned int gain, short immed) {
#if 0  //not support
    unsigned int tx_pa_gain_index, tx_bb_gain_index;
    unsigned int tx_dig_gain_dB;
    unsigned short reg_val;
    short index, offset;
    unsigned char i;

    if ((PA_G_INDEX == type && gain > 8) ||
        (BB_G_INDEX == type && gain > 24) ||
        (DIG_G_INDEX == type && gain > 400) ||
        (type > DIG_G_INDEX))
    {
        phy->error = -PARA_INVALID;
        return -PARA_INVALID;
    }

    phy->tx_curr_index = !phy->tx_curr_index;

    if (immed)
        HAL_REG_SET_BIT(phy, 0x0D7, BIT5);
    else
        HAL_REG_CLR_BIT(phy, 0x0D7, BIT5);

    if (PA_G_INDEX == type)
    {
        tx_pa_gain_index = gain;

        /* find index */
        index = find_case_with_flo(flo);

        /* load tx pa gain lut into phy->tx_curr_index lut */
        offset = chn ? 10: 0;
        for (i=0; i<10; i++)
            phy->tx_pa_gain[offset+i] = g_tx_pa_gain_config[index*9 + tx_pa_gain_index][offset+i];

        for (i=0; i<20; i++)
        {
            if (phy->tx_curr_index)
            {
                write_lut_byte(phy, TX_GAIN_LUT, 1, g_tx_pa_gain_regs[i], 205+i/4, i%4, phy->tx_pa_gain[i]);
                write_lut_byte(phy, TX_GAIN_LUT, 10, g_tx_pa_gain_regs[i], 250+i/4, i%4, phy->tx_pa_gain[i]);
            }
            else
            {
                write_lut_byte(phy, TX_GAIN_LUT, 0, g_tx_pa_gain_regs[i], 200+i/4, i%4, phy->tx_pa_gain[i]);
                write_lut_byte(phy, TX_GAIN_LUT, 9, g_tx_pa_gain_regs[i], 245+i/4, i%4, phy->tx_pa_gain[i]);
            }
        }

        /* tx pa gain index */
        reg_val = hal_spi_read_reg(phy, 0x0CB);
        reg_val = (phy->tx_curr_index & 0xF) | (reg_val & 0xF0);
        hal_spi_write_reg(phy, 0x0CB, reg_val);
    }

    if (BB_G_INDEX == type)
    {
        tx_bb_gain_index = gain;

        /* load tx bb gain lut into phy->tx_curr_index lut */
        offset = chn ? 8: 0;
        for (i=0; i<8; i++)
            phy->tx_bb_gain[offset+i] = phy->tx_bb_gain_config[tx_bb_gain_index+0][offset+i];

        for (i=0; i<16; i++)
        {
            if (phy->tx_curr_index)
            {
                write_lut_byte(phy, TX_GAIN_LUT, 1, g_tx_bb_gain_regs[i], 4+i/4, i%4, phy->tx_bb_gain[i]);
                write_lut_byte(phy, TX_GAIN_LUT, 26, g_tx_bb_gain_regs[i], 104+i/4, i%4, phy->tx_bb_gain[i]);
            }
            else
            {
                write_lut_byte(phy, TX_GAIN_LUT, 0, g_tx_bb_gain_regs[i], 0+i/4, i%4, phy->tx_bb_gain[i]);
                write_lut_byte(phy, TX_GAIN_LUT, 25, g_tx_bb_gain_regs[i], 100+i/4, i%4, phy->tx_bb_gain[i]);
            }
        }

        if (tx_bb_gain_index <= 5)
        {
            /* update qec config */
            if (phy->tx_qec_flag[chn])
            {
                write_tx_qec_lol_cal_word_chn(phy, chn, phy->tx_curr_index, &(phy->tx_qec[chn][tx_bb_gain_index]));
                real_time_update_tx_qec(phy, chn, &(phy->tx_qec[chn][tx_bb_gain_index]));
            }
        }

        /* tx bb gain index */
        reg_val = hal_spi_read_reg(phy, 0x0C1);
        reg_val = (phy->tx_curr_index & 0x1F) | (reg_val & 0xE0);
        hal_spi_write_reg(phy, 0x0C1, reg_val);
    }

    if (DIG_G_INDEX == type)
    {
        tx_dig_gain_dB = gain;

        /* dig gain */
        if (chn==TRX_CHN1)
        {
            hal_spi_write_reg(phy, 0x0E5, (tx_dig_gain_dB >> 8) & 0x1);
            hal_spi_write_reg(phy, 0x0E6, tx_dig_gain_dB & 0xff);
        }
        else
        {
            hal_spi_write_reg(phy, 0x0E7, (tx_dig_gain_dB >> 8) & 0x1);
            hal_spi_write_reg(phy, 0x0E8, tx_dig_gain_dB & 0xff);
        }
    }

    HAL_REG_CLR_BIT(phy, 0x0D7, BIT5);

    return 0;
#else

  return -PARA_INVALID;
#endif
}

/****************************************************************************************/
// set tx atten
// PA_Gain_Index --> unit: 3dB, range:0~8
// BB_Gain_Index --> unit: 1dB, range:0~24
// DIG_Gain_dB   --> unit: 0.125dB, range:0~400
/****************************************************************************************/
int tx_split_atten(rf_chip_phy_t* phy, unsigned long long flo, TX_SPLIT_ATTEN_ENUM type, unsigned int gain, short immed) {
  unsigned int tx_pa_gain_index, tx_bb_gain_index;
  unsigned int tx_dig_gain_dB;
  unsigned short reg_val;
  short index;
  unsigned char i;

  if ((PA_G_INDEX == type && gain > 8) ||
      (BB_G_INDEX == type && gain > 24) ||
      (DIG_G_INDEX == type && gain > 400) ||
      (type > DIG_G_INDEX)) {
    phy->error = -PARA_INVALID;
    return -PARA_INVALID;
  }

  if (immed)
    HAL_REG_SET_BIT(phy, 0x0D7, BIT5);
  else
    HAL_REG_CLR_BIT(phy, 0x0D7, BIT5);

  if (PA_G_INDEX == type) {
    tx_pa_gain_index = gain;

    /* tx pa gain index */
    reg_val = hal_spi_read_reg(phy, 0x0CB);
    reg_val = (tx_pa_gain_index & 0xF) | (reg_val & 0xF0);
    hal_spi_write_reg(phy, 0x0CB, reg_val);

    /* find index */
    index = find_case_with_flo(flo);

    /* load into registers of tx pa gain */
    for (i = 0; i < 20; i++)
      hal_spi_write_reg(phy, g_tx_pa_gain_regs[i], g_tx_pa_gain_config[index * 9 + tx_pa_gain_index][i]);
  }

  if (BB_G_INDEX == type) {
    tx_bb_gain_index = gain;

    /* tx bb gain index */
    reg_val = hal_spi_read_reg(phy, 0x0C1);
    reg_val = (tx_bb_gain_index & 0x1F) | (reg_val & 0xE0);
    hal_spi_write_reg(phy, 0x0C1, reg_val);

    /* load into registers of tx bb gain */
    for (i = 0; i < 16; i++)
      hal_spi_write_reg(phy, g_tx_bb_gain_regs[i], phy->tx_bb_gain_config[tx_bb_gain_index + 0][i]);
  }

  if (DIG_G_INDEX == type) {
    tx_dig_gain_dB = gain;

    /* dig gain */
    hal_spi_write_reg(phy, 0x0E5, (tx_dig_gain_dB >> 8) & 0x1);
    hal_spi_write_reg(phy, 0x0E6, tx_dig_gain_dB & 0xff);
    hal_spi_write_reg(phy, 0x0E7, (tx_dig_gain_dB >> 8) & 0x1);
    hal_spi_write_reg(phy, 0x0E8, tx_dig_gain_dB & 0xff);

    if (phy->config->chip_ver == 2) {
      HAL_REG_SET_BIT(phy, 0x0E5, BIT7);
      HAL_REG_SET_BIT(phy, 0x0E7, BIT7);
    }
  }

  HAL_REG_CLR_BIT(phy, 0x0D7, BIT5);

  return 0;
}

/****************************************************************************************/
// set rx gain
// lmt --> unit: 6dB, range:0~5
// lpf --> unit: 1dB, range:0~12
// dig --> unit: 0.25dB, range:0~200
/****************************************************************************************/
int rx_mgc_gain(rf_chip_phy_t* phy, TRX_CHN_ENUM chn, RX_MGC_GAIN_ENUM mgc, unsigned char gain) {
  unsigned short mgc_ctr_reg, cfg_mgc_lna_lmt_lpf_reg, cfg_mgc_dig_reg;
  unsigned short lpt_ctrl_reg, lpt_lna_lmt_lpf_reg;
  unsigned char reg_val;
  short range[3] = {5, 12, 200};

  if (gain > range[mgc]) {
    phy->error = -PARA_INVALID;
    return -PARA_INVALID;
  }

  if (chn == TRX_CHN1) {
    mgc_ctr_reg             = 0xE53;
    lpt_ctrl_reg            = 0xE63;
    cfg_mgc_lna_lmt_lpf_reg = 0xE55;
    cfg_mgc_dig_reg         = 0xE56;
    lpt_lna_lmt_lpf_reg     = 0xE68;
  } else {
    mgc_ctr_reg             = 0xED3;
    lpt_ctrl_reg            = 0xEE3;
    cfg_mgc_lna_lmt_lpf_reg = 0xED5;
    cfg_mgc_dig_reg         = 0xED6;
    lpt_lna_lmt_lpf_reg     = 0xEE8;
  }

  if ((mgc == LMT_G) || (mgc == LPF_G)) {
    HAL_REG_CLR_BIT(phy, lpt_ctrl_reg, BIT7);
    HAL_REG_SET_BIT(phy, lpt_ctrl_reg, BIT7);
    HAL_REG_CLR_BIT(phy, lpt_ctrl_reg, BIT7);
  }

  if (mgc == LMT_G) {
    if (gain > 5)
      gain = 5;
    reg_val = hal_spi_read_reg(phy, lpt_lna_lmt_lpf_reg);
    reg_val = (reg_val & 0x8F) | (gain << 4);
    hal_spi_write_reg(phy, cfg_mgc_lna_lmt_lpf_reg, reg_val);
  } else if (mgc == LPF_G) {
    if (gain > 12)
      gain = 12;
    reg_val = hal_spi_read_reg(phy, lpt_lna_lmt_lpf_reg);
    reg_val = (reg_val & 0xF0) | gain;
    hal_spi_write_reg(phy, cfg_mgc_lna_lmt_lpf_reg, reg_val);
  } else if (mgc == DIG_G) {
    if (gain > 200)
      gain = 200;
    hal_spi_write_reg(phy, cfg_mgc_dig_reg, gain);
  }

  HAL_REG_CLR_BIT(phy, mgc_ctr_reg, BIT7);
  HAL_REG_SET_BIT(phy, mgc_ctr_reg, BIT7);
  HAL_REG_CLR_BIT(phy, mgc_ctr_reg, BIT7);

  if (phy->init_flag == 1) {

    phy->rxdc_track_thread[chn].hold_cnt         = 0;
    phy->rxdc_track_thread[chn].track_cnt        = 0;
    phy->rxdc_track_thread[chn].thread.need_exit = 0;
#if LINUX_OS
    rx_dc_tracking_action_start(phy, chn, 1, 0);
#else
    phy->rxdc_track_thread[chn].thread.need_exit = 0;
#endif
  }
  return 0;
}

void rx_mgc_max_gain(rf_chip_phy_t* phy, TRX_CHN_ENUM chn) {
  if (chn)
    HAL_CONFIG_REGS(phy, G_REG_ARRAY_17C_MGC_DIGITAL_CFG_MAX_GAIN, G_REG_ARRAY_17C_MGC_DIGITAL_CFG_MAX_GAIN_LEN);
  else
    HAL_CONFIG_REGS(phy, G_REG_ARRAY_DIG_RX1_mgc_start_maxium_gain, G_REG_ARRAY_DIG_RX1_mgc_start_maxium_gain_LEN);
}

void rxgain_force_valid_config(rf_chip_phy_t* phy, int enable) {
  unsigned char val;

  if (enable) {
    val = hal_spi_read_reg(phy, 0x0dd);
    val = SET_BIT(val, BIT1);
    hal_spi_write_reg(phy, 0x0dd, val);

    val = hal_spi_read_reg(phy, 0x0d7);
    val = CLR_BIT(val, BIT6);
    hal_spi_write_reg(phy, 0x0d7, val);
  }
}

/****************************************************************************************/
// read/write lut
/****************************************************************************************/
short write_lut_byte(rf_chip_phy_t* phy, LUT_INDEX_ENUM lut, short index, unsigned short reg,
                     int lut_addr, unsigned char offset, unsigned char val) {
  unsigned char lut_vals[4];

  read_lut_word(phy, lut, lut_addr, lut_vals);
  lut_vals[offset] = val;
  write_lut_word(phy, lut, lut_addr, lut_vals);

  return 0;
}

short read_lut_byte(rf_chip_phy_t* phy, LUT_INDEX_ENUM lut, short index,
                    unsigned short reg, int lut_addr, unsigned char offset, unsigned char* pval) {
  unsigned char lut_vals[4];

  read_lut_word(phy, lut, lut_addr, lut_vals);
  *pval = lut_vals[offset];

  return 0;
}

short read_lut_word(rf_chip_phy_t* phy, LUT_INDEX_ENUM lut, int lut_addr, unsigned char vals[]) {
  unsigned char reg_val, tmp;

  //set addr
  reg_val = hal_spi_read_reg(phy, 0x0B6);
  tmp     = reg_val & 0xf8;
  reg_val = (lut_addr >> 8) & 0x7;
  hal_spi_write_reg(phy, 0x0B6, reg_val | tmp);
  reg_val = lut_addr & 0xff;
  hal_spi_write_reg(phy, 0x0B7, reg_val);

  //enable read
  HAL_REG_SET_BIT(phy, 0x0B5, BIT7);

  //select lut
  if (lut <= RX2_AGC_LUT) {
    reg_val = hal_spi_read_reg(phy, 0x0B5);
    hal_spi_write_reg(phy, 0x0B5, (1 << lut) | (reg_val & 0x80));
  } else {
    hal_spi_write_reg(phy, 0x0B4, 1 << (lut - 8));
  }

  //read data
  vals[0] = hal_spi_read_reg(phy, 0x0BF);
  vals[1] = hal_spi_read_reg(phy, 0x0BE);
  vals[2] = hal_spi_read_reg(phy, 0x0BD);
  vals[3] = hal_spi_read_reg(phy, 0x0BC);

  //disable lut
  if (lut <= RX2_AGC_LUT) {
    HAL_REG_CLR_BIT(phy, 0x0B5, lut);
  } else {
    HAL_REG_CLR_BIT(phy, 0x0B4, lut - 8);
  }

  return 0;
}

short write_lut_word(rf_chip_phy_t* phy, LUT_INDEX_ENUM lut, int lut_addr, unsigned char vals[]) {
  unsigned char reg_val, tmp;

  //set addr
  reg_val = hal_spi_read_reg(phy, 0x0B6);
  tmp     = reg_val & 0xf8;
  reg_val = (lut_addr >> 8) & 0x7;
  hal_spi_write_reg(phy, 0x0B6, reg_val | tmp);
  reg_val = lut_addr & 0xff;
  hal_spi_write_reg(phy, 0x0B7, reg_val);

  //enable read
  HAL_REG_CLR_BIT(phy, 0x0B5, BIT7);

  //write data
  hal_spi_write_reg(phy, 0x0BB, vals[0]);
  hal_spi_write_reg(phy, 0x0BA, vals[1]);
  hal_spi_write_reg(phy, 0x0B9, vals[2]);
  hal_spi_write_reg(phy, 0x0B8, vals[3]);

  //select lut
  if (lut <= RX2_AGC_LUT) {
    reg_val = hal_spi_read_reg(phy, 0x0B5);
    hal_spi_write_reg(phy, 0x0B5, (1 << lut) | (reg_val & 0x80));
  } else {
    hal_spi_write_reg(phy, 0x0B4, 1 << (lut - 8));
  }

  //disable lut
  if (lut <= RX2_AGC_LUT) {
    HAL_REG_CLR_BIT(phy, 0x0B5, lut);
  } else {
    HAL_REG_CLR_BIT(phy, 0x0B4, lut - 8);
  }

  return 0;
}

/****************************************************************************************/
// rxadc&ifbuf on/off
/****************************************************************************************/
void rxadc_on(rf_chip_phy_t* phy, TRX_CHN_ENUM chn, short on) {
  unsigned short reg, val;
  unsigned char vals[1], offset;
  short i;
  int base;

  reg = (chn) ? 0x6B5 : 0x697;
  val = (on) ? 0xEE : 0x00;
  hal_spi_write_reg(phy, reg, val);

  offset  = (chn) ? 1 : 2;
  base    = (chn) ? 6 : 4;
  vals[0] = val;
  for (i = 0; i < 27; i++)
    write_lut_byte(phy, MAIN_ENSM_LUT, i, reg, base + i * 10, offset, vals[0]);
}

void rxifbuf_on(rf_chip_phy_t* phy, TRX_CHN_ENUM chn, short on) {
  unsigned short reg, val;
  unsigned char vals[1], offset;
  short i;
  int base;

  reg = (chn) ? 0x6E0 : 0x6D0;
  val = (on) ? 0xFF : 0xEF;
  hal_spi_write_reg(phy, reg, val);

  offset  = (chn) ? 2 : 3;
  base    = (chn) ? 5 : 3;
  vals[0] = val;
  for (i = 0; i < 27; i++)
    write_lut_byte(phy, MAIN_ENSM_LUT, i, reg, base + i * 10, offset, vals[0]);
}

/****************************************************************************************/
// qec cal
/****************************************************************************************/
int rxqec_cal(rf_chip_phy_t* phy, TRX_CHN_ENUM chn) {
  unsigned char val;
  short offset;

  if (phy->rx_qec_flag[chn]) {
    fn_rx_qec_lut_update(chn, phy);
    offset = (chn == TRX_CHN1) ? 0x0 : 0x800;
    val    = hal_spi_read_reg(phy, 0x200 + offset);
    val    = SET_BIT(val, BIT1);
    val    = SET_BIT(val, BIT2);
    hal_spi_write_reg(phy, 0x200 + offset, val);
    HAL_REG_SET_BIT(phy, 0x203 + offset, BIT4);
    return 0;
  }

  ENSM_MANAUL_ENA(phy, 1, FSM_IDLE);
  if (chn) {
    DIG_RX_FILTER_CFG(phy, TRX_CHN2, phy->config->bandwith, 1);
    DIG_TX_FILTER_CFG(phy, TRX_CHN2, phy->config->bandwith, 1);
    DIG_CHAN_ENA(phy, 0, TRX_CHN2, 1);
    DIG_CHAN_ENA(phy, 1, TRX_CHN2, 1);
  } else {
    DIG_RX_FILTER_CFG(phy, TRX_CHN1, phy->config->bandwith, 1);
    DIG_TX_FILTER_CFG(phy, TRX_CHN1, phy->config->bandwith, 1);
    DIG_CHAN_ENA(phy, 0, TRX_CHN1, 1);
    DIG_CHAN_ENA(phy, 1, TRX_CHN1, 1);
  }
  ENSM_MANAUL_ENA(phy, 0, FSM_FDD);
  ENSM_MANAUL_ENA(phy, 1, FSM_FDD);

  switch (phy->config->mode) {
    case CH1_TDD:
    case CH2_TDD:
    case CH1_CH2_TDD:
      hal_spi_write_reg(phy, 0x0dc, 0x00);
      if (chn == TRX_CHN1) {
        hal_spi_write_reg(phy, 0x0dc, 0x25);
        hal_spi_write_reg(phy, 0x0dc, 0x35);
      } else {
        hal_spi_write_reg(phy, 0x0dc, 0x2A);
        hal_spi_write_reg(phy, 0x0dc, 0x3A);
      }
      break;
    default:
      break;
  }

  fn_rx_qec_cal(chn, phy->config->rx_ext_loop, phy);

  DIG_SOFT_RST_N(phy, 0);

  return 0;
}

int txqec_cal(rf_chip_phy_t* phy, TRX_CHN_ENUM chn) {
  unsigned char val;
  short reg;

  if (phy->tx_qec_flag[chn]) {
    fn_tx_qec_lol_lut_update(chn, phy);
    reg = (chn == TRX_CHN1) ? 0x200 : 0xa00;
    val = hal_spi_read_reg(phy, reg);
    val = SET_BIT(val, BIT4);
    val = SET_BIT(val, BIT5);
    hal_spi_write_reg(phy, reg, val);
    return 0;
  }

  ENSM_MANAUL_ENA(phy, 1, FSM_IDLE);

  if (chn) {
    hal_spi_write_reg(phy, 0x7a3, 0x9);
    DIG_RX_FILTER_CFG(phy, TRX_CHN2, phy->config->bandwith, 1);
    DIG_TX_FILTER_CFG(phy, TRX_CHN2, phy->config->bandwith, 1);
    DIG_CHAN_ENA(phy, 0, TRX_CHN2, 1);
    DIG_CHAN_ENA(phy, 1, TRX_CHN2, 1);
  } else {
    hal_spi_write_reg(phy, 0x790, 0x9);
    DIG_RX_FILTER_CFG(phy, TRX_CHN1, phy->config->bandwith, 1);
    DIG_TX_FILTER_CFG(phy, TRX_CHN1, phy->config->bandwith, 1);
    DIG_CHAN_ENA(phy, 0, TRX_CHN1, 1);
    DIG_CHAN_ENA(phy, 1, TRX_CHN1, 1);
  }

  ENSM_MANAUL_ENA(phy, 1, FSM_FDD);

#ifdef QEC_DEBUG
  if (chn)
    hal_spi_write_reg(phy, 0x748, 0x2);
  else
    hal_spi_write_reg(phy, 0x700, 0x2);
#endif

  //fn_tx_qec_lol_cal(chn, phy);

  DIG_SOFT_RST_N(phy, 0);

  return 0;
}

int txqec_cal_rflp(rf_chip_phy_t* phy, TRX_CHN_ENUM chn) {
  unsigned char val;
  short tdd_sel = 0, reg;

  if (phy->tx_qec_flag[chn]) {
    fn_tx_qec_lol_lut_update(chn, phy);
    reg = (chn == TRX_CHN1) ? 0x200 : 0xa00;
    val = hal_spi_read_reg(phy, reg);
    val = SET_BIT(val, BIT4);
    val = SET_BIT(val, BIT5);
    hal_spi_write_reg(phy, reg, val);
    return 0;
  }

  if (phy->config->qec_dbfs < 0) {
    //phy->config->qec_dbfs = -1 * phy->config->qec_dbfs;
    phy->config->qec_dbfs = -1 * phy->config->qec_dbfs;
  }

  if (phy->config->lol_dbfs < 0) {
    //phy->config->lol_dbfs = -1 * phy->config->lol_dbfs;
    phy->config->lol_dbfs = -1 * phy->config->lol_dbfs;
  }

  ENSM_MANAUL_ENA(phy, 1, FSM_IDLE);

  if (chn) {
    hal_spi_write_reg(phy, 0x7a3, 0x9);
    DIG_RX_FILTER_CFG(phy, TRX_CHN2, phy->config->bandwith, 1);
    DIG_TX_FILTER_CFG(phy, TRX_CHN2, phy->config->bandwith, 1);
    DIG_CHAN_ENA(phy, 0, TRX_CHN2, 1);
    DIG_CHAN_ENA(phy, 1, TRX_CHN2, 1);
  } else {
    hal_spi_write_reg(phy, 0x790, 0x9);
    DIG_RX_FILTER_CFG(phy, TRX_CHN1, phy->config->bandwith, 1);
    DIG_TX_FILTER_CFG(phy, TRX_CHN1, phy->config->bandwith, 1);
    DIG_CHAN_ENA(phy, 0, TRX_CHN1, 1);
    DIG_CHAN_ENA(phy, 1, TRX_CHN1, 1);
  }

  ENSM_MANAUL_ENA(phy, 1, FSM_FDD);

  if ((phy->config->mode >= CH1_TDD) && (phy->config->mode <= CH1_CH2_TDD)) {
    tdd_sel = 1;
    hal_spi_write_reg(phy, 0x0dc, 0x00);
    if (chn == 0) {
      hal_spi_write_reg(phy, 0x0dc, 0x25);
      hal_spi_write_reg(phy, 0x0dc, 0x35);
    } else {
      hal_spi_write_reg(phy, 0x0dc, 0x2A);
      hal_spi_write_reg(phy, 0x0dc, 0x3A);
    }
  }

  if (phy->config->tx_ext_loop == 1) {
    if (chn == 0)
      hal_spi_write_reg(phy, 0x700, 0x2);
    else
      hal_spi_write_reg(phy, 0x748, 0x2);
  }

#ifdef QEC_DEBUG
  if (chn)
    hal_spi_write_reg(phy, 0x748, 0x2);
  else
    hal_spi_write_reg(phy, 0x700, 0x2);
#endif

  if (-1 == fn_tx_qec_lol_cal_rflp(chn, phy->config->tx_ext_loop, phy->config->qec_dbfs, phy->config->lol_dbfs, phy)) {
    LOG_ERROR("Failed to fn_tx_qec_lol_cal_rflp in chn :%d\n", chn);
    return -1;
  }

  DIG_SOFT_RST_N(phy, 0);

  if (tdd_sel == 1) {
    // IIO_DEMO\IIO_FSM_Script\Power_Up_E_0_4_LO_CONFIG_2T2R_TDD.txt
    hal_spi_write_reg(phy, 0x6D1, 0x5F);
    hal_spi_write_reg(phy, 0x6E1, 0x5F);
    hal_spi_write_reg(phy, 0x702, 0x2F);
    hal_spi_write_reg(phy, 0x74A, 0x2F);
    hal_spi_write_reg(phy, 0x639, 0xFC);
    hal_spi_write_reg(phy, 0x803, 0x07);
    hal_spi_write_reg(phy, 0x620, 0x10);
    hal_spi_write_reg(phy, 0x61F, 0x00);
  }

  return 0;
}

/****************************************************************************************/
// setup digital
/****************************************************************************************/
int map_to_data_mode(CHIP_MODE_ENUM mode, DIG_IF_ENUM data_if, IF_TYPE_ENUM if_type, DATA_RATE_ENUM data_rate) {
  int ret = -1;

  if ((mode >= 0) && (mode <= 4)) {  //fdd
    if ((data_if == 0) && (if_type == 0) && (data_rate == 0))
      ret = LVDS_2R2T_DP_FDD_DDR;
    else if ((data_if == 0) && (if_type == 0) && (data_rate == 1))
      ret = LVDS_2R2T_DP_FDD_DDR;
    else if ((data_if == 0) && (if_type == 1) && (data_rate == 0))
      ret = LVDS_2R2T_DP_FDD_DDR;
    else if ((data_if == 0) && (if_type == 1) && (data_rate == 1))
      ret = LVDS_2R2T_DP_FDD_DDR;
    else if ((data_if == 1) && (if_type == 0) && (data_rate == 0))
      ret = CMOS_2R2T_DP_FDD_SDR;
    else if ((data_if == 1) && (if_type == 0) && (data_rate == 1))
      ret = CMOS_2R2T_DP_FDD_DDR;
    else if ((data_if == 1) && (if_type == 1) && (data_rate == 0))
      ret = CMOS_2R2T_SP_FDD_SDR;
    else if ((data_if == 1) && (if_type == 1) && (data_rate == 1))
      ret = CMOS_2R2T_SP_FDD_DDR;
  } else {  //tdd
    if ((data_if == 0) && (if_type == 0) && (data_rate == 0))
      ret = CMOS_2R2T_DP_TDD_SDR;  //10000;
    else if ((data_if == 0) && (if_type == 0) && (data_rate == 1))
      ret = CMOS_2R2T_DP_TDD_SDR;  //10000;
    else if ((data_if == 0) && (if_type == 1) && (data_rate == 0))
      ret = CMOS_2R2T_DP_TDD_SDR;  //10000;
    else if ((data_if == 0) && (if_type == 1) && (data_rate == 1))
      ret = CMOS_2R2T_DP_TDD_SDR;  //10000;
    else if ((data_if == 1) && (if_type == 0) && (data_rate == 0))
      ret = CMOS_2R2T_DP_TDD_SDR;
    else if ((data_if == 1) && (if_type == 0) && (data_rate == 1))
      ret = CMOS_2R2T_DP_TDD_DDR;
    else if ((data_if == 1) && (if_type == 1) && (data_rate == 0))
      ret = CMOS_2R2T_SP_TDD_SDR;
    else if ((data_if == 1) && (if_type == 1) && (data_rate == 1))
      ret = CMOS_2R2T_SP_TDD_DDR;
  }

  if (ret < 0)
    return ret;

  if ((mode == 7) || (mode == 4))
    return ret;  //2r2t
  else
    return (ret + 9);  //1r1t
}

int user_map_to_standard(BANDWITH_ENUM user_bw) {
  int ret = LTE20;

  switch (user_bw) {
    case BW_GSM:
      ret = LTE1P4;
      break;
    case BW_BT:
      ret = LTE3;
      break;
    case BW_LTE5:
      ret = LTE5;
      break;
    case BW_LTE10:
      ret = LTE10;
      break;
    case BW_LTE20:
      ret = LTE20;
      break;
    case BW_LTE40:
      ret = LTE40;
      break;
    case BW_LTE60:
      ret = LTE100;
      break;
    case BW_LTE80:
      ret = LTE100;
      break;
    case BW_LTE100:
      ret = LTE100;
      break;
    default:
      ret = LTE20;
      break;
  }

  return ret;
}

void txdc_digtial_remove(rf_chip_phy_t* phy, TRX_CHN_ENUM chn) {
  short offset;
  unsigned short val, backup_val[6];

  offset = (chn) ? 0x800 : 0;
  /* save */
  backup_val[0] = hal_spi_read_reg(phy, 0x202 + offset);
  backup_val[1] = hal_spi_read_reg(phy, 0x440 + offset);
  backup_val[2] = hal_spi_read_reg(phy, 0x441 + offset);
  backup_val[3] = hal_spi_read_reg(phy, 0x200 + offset);
  backup_val[4] = hal_spi_read_reg(phy, 0x203 + offset);

  /* remove */
  val = hal_spi_read_reg(phy, 0x202 + offset);
  val = 0x9f & val;
  val = val | 0x20;
  hal_spi_write_reg(phy, 0x202 + offset, val);

  hal_spi_write_reg(phy, 0x440 + offset, 0);
  hal_spi_write_reg(phy, 0x441 + offset, 0);

  val = hal_spi_read_reg(phy, 0x202 + offset);
  val = val & 0xe0;
  hal_spi_write_reg(phy, 0x202 + offset, val);

  val = hal_spi_read_reg(phy, 0x200 + offset);
  val = val | 0x0a;
  hal_spi_write_reg(phy, 0x200 + offset, val);

  HAL_REG_CLR_BIT(phy, 0x203 + offset, BIT1);
  HAL_REG_SET_BIT(phy, 0x203 + offset, BIT1);
  HAL_REG_CLR_BIT(phy, 0x203 + offset, BIT1);

  /* restore */
  hal_spi_write_reg(phy, 0x202 + offset, backup_val[0]);
  hal_spi_write_reg(phy, 0x440 + offset, backup_val[1]);
  hal_spi_write_reg(phy, 0x441 + offset, backup_val[2]);
  hal_spi_write_reg(phy, 0x200 + offset, backup_val[3]);
  hal_spi_write_reg(phy, 0x203 + offset, backup_val[4]);

  HAL_REG_CLR_BIT(phy, 0x203 + offset, BIT1);
  HAL_REG_SET_BIT(phy, 0x203 + offset, BIT1);
  HAL_REG_CLR_BIT(phy, 0x203 + offset, BIT1);
}

short digital_init(rf_chip_phy_t* phy, CHIP_MODE_ENUM mode, BANDWITH_ENUM bandwith,
                   DIG_IF_ENUM dif, IF_TYPE_ENUM port, DATA_RATE_ENUM rate, short step) {
  short map_rate, i, val;
  short rx[8][2] =
      {
          {1, 0},
          {1, 0},
          {0, 1},
          {0, 1},
          {1, 1},
          {1, 0},
          {0, 1},
          {1, 1},
      };
  short tx[8][2] =
      {
          {1, 0},
          {0, 1},
          {1, 0},
          {0, 1},
          {1, 1},
          {1, 0},
          {0, 1},
          {1, 1},
      };
  TX_QEC_CFG_REGS tx_qec_cfg;

  map_rate = map_to_data_mode(mode, dif, port, rate);
  if (map_rate < 0) {
    phy->error = -PARA_INVALID;
    return -PARA_INVALID;
  }

  if (step == 0) {
    //lvds or cmos
    if (dif == LVDS_IF) {
      val = 0x90;
      //LOG_INFO("0x880=0x90\n");
    } else {
      val = 0x92;
      // LOG_INFO("0x880=0x92\n");
    }
    hal_spi_write_reg(phy, 0x880, val);

    DIG_LDO_CFG(phy, bandwith);
    DIG_INTF_CFG(phy, dif, port, rate, (mode <= 4) ? PHY_FDD : PHY_TDD);
    DIG_INTF_CLK_CFG(phy, bandwith, (map_rate > 8) ? map_rate : (map_rate + 9));
    fn_fpga_set_if(phy, mode, dif, port, rate, 1);
    DIG_ENSM_CFG(phy, 1);
  } else if (step == 1) {
    //DIG_INTF_CFG(phy, dif, port, rate, (mode<=4)?PHY_FDD:PHY_TDD);
    DIG_INTF_CLK_CFG(phy, bandwith, (map_rate > 8) ? map_rate : (map_rate + 9));
    if ((!phy->config->custom_bandwith_flag) && (!phy->config->syspll_cfg_flag))
      if (bandwith == BW_LTE100)
        hal_spi_write_reg(phy, 0x604, 0x03);
    //fn_fpga_set_if(phy, mode, dif, port, rate, 1);
    //DIG_ENSM_CFG(phy, 1);
  } else if (step == 2) {
    if (rx[mode][0])
      DIG_RX_FILTER_CFG(phy, TRX_CHN1, bandwith, 1);
    if (rx[mode][1])
      DIG_RX_FILTER_CFG(phy, TRX_CHN2, bandwith, 1);

    if (tx[mode][0])
      DIG_TX_FILTER_CFG(phy, TRX_CHN1, bandwith, 1);
    if (tx[mode][1])
      DIG_TX_FILTER_CFG(phy, TRX_CHN2, bandwith, 1);

    //DIG_FIR_FILTER_CFG(phy, 0, 0, 0, 0, 0, 0, 0);
    //DIG_INTF_CFG(phy, dif, port, rate, (mode<=4)?PHY_FDD:PHY_TDD);
    DIG_INTF_CLK_CFG(phy, bandwith, map_rate);
    //DIG_ENSM_CFG(phy, 0);

    DIG_CHAN_ENA(phy, 1, TRX_CHN1, rx[mode][0]);
    DIG_CHAN_ENA(phy, 1, TRX_CHN2, rx[mode][1]);
    DIG_CHAN_ENA(phy, 0, TRX_CHN1, tx[mode][0]);
    DIG_CHAN_ENA(phy, 0, TRX_CHN2, tx[mode][1]);
    ENSM_MANAUL_ENA(phy, 0, 0);

    switch (mode) {
      case CH1_TDD:
      case CH1_FDD:
        hal_spi_write_reg(phy, 0x805, 0x07);  //turn off rx2adc
        hal_spi_write_reg(phy, 0x806, 0x0f);
        hal_spi_write_reg(phy, 0x698, 0x00);
        break;
      case RX1_TX2_FDD:
        hal_spi_write_reg(phy, 0x805, 0x07);  //turn off rx2adc
        hal_spi_write_reg(phy, 0x806, 0xF0);
        hal_spi_write_reg(phy, 0x698, 0x00);
        break;
      case RX2_TX1_FDD:
        hal_spi_write_reg(phy, 0x805, 0x3c);
        hal_spi_write_reg(phy, 0x806, 0x0f);
        hal_spi_write_reg(phy, 0x698, 0x08);
        break;
      case CH2_TDD:
      case CH2_FDD:
        hal_spi_write_reg(phy, 0x805, 0x3c);
        hal_spi_write_reg(phy, 0x806, 0xf0);
        hal_spi_write_reg(phy, 0x698, 0x08);
        break;
      case CH1_CH2_TDD:
      case CH1_CH2_FDD:
        hal_spi_write_reg(phy, 0x805, 0x3f);
        hal_spi_write_reg(phy, 0x806, 0xff);
        hal_spi_write_reg(phy, 0x698, 0x00);
        break;
    }

    my_memset(&tx_qec_cfg, 0, sizeof(tx_qec_cfg));
    fn_tx_qec_gain_set(phy, 0, &tx_qec_cfg, 2);
    fn_tx_qec_gain_set(phy, 1, &tx_qec_cfg, 2);
  } else if (step == 3) {
    DIG_SOFT_RST_N(phy, 0);
  } else if (step == 4) {
    if (rx[mode][0])
      DIG_RX_FILTER_CFG(phy, TRX_CHN1, bandwith, 1);
    if (rx[mode][1])
      DIG_RX_FILTER_CFG(phy, TRX_CHN2, bandwith, 1);

    if (tx[mode][0])
      DIG_TX_FILTER_CFG(phy, TRX_CHN1, bandwith, 1);
    if (tx[mode][1])
      DIG_TX_FILTER_CFG(phy, TRX_CHN2, bandwith, 1);

    DIG_INTF_CLK_CFG(phy, bandwith, (map_rate > 8) ? map_rate : (map_rate + 9));
    if ((!phy->config->custom_bandwith_flag) && (!phy->config->syspll_cfg_flag))
      if (bandwith == BW_LTE100)
        hal_spi_write_reg(phy, 0x604, 0x03);
  }
  return 0;
}

/****************************************************************************************/
// setup chip
/****************************************************************************************/
void wait_init(rf_chip_phy_t* phy) {
  int32_t rslt = TRANSV_OK;

  rslt |= force_wait(phy);
  rslt |= ana_manual_on(phy);
  rslt |= rxadc_init(phy);
  rslt |= rx1adc_weight_init(phy);
  rslt |= rx2adc_weight_init(phy);
  rslt |= xo_fc_off(phy);
  rslt |= cm_fc_off(phy);
  if (rslt != TRANSV_OK) {
    LOG_ERROR("FAIL to run %s in line%d, rslt: %d\n", __FUNCTION__, __LINE__, rslt);
  }
}

void fsm_init(rf_chip_phy_t* phy) {
  HAL_CONFIG_REGS(phy, G_REG_ARRAY_Wait_H_1_DIG_MANUAL_OFF_RSNT, G_REG_ARRAY_Wait_H_1_DIG_MANUAL_OFF_RSNT_LEN);
  if (phy->config->use_bybrid_mode)
    hal_spi_write_reg(phy, 0x803, 0x0f);
}

void manual_enable(rf_chip_phy_t* phy, short en) {
  if (en)
    HAL_CONFIG_REGS(phy, G_REG_ARRAY_Power_Up_A_2_ANA_MANUAL_ON, G_REG_ARRAY_Power_Up_A_2_ANA_MANUAL_ON_LEN);
  else
    HAL_CONFIG_REGS(phy, G_REG_ARRAY_Wait_I_1_ANA_MANUAL_OFF, G_REG_ARRAY_Wait_I_1_ANA_MANUAL_OFF_LEN);

  if (CHIP_VERSION_E1 == phy->config->chip_ver) {
    if (en) {
      //manual sx vldo enable
      HAL_REG_SET_BIT(phy, 0x614, BIT4);
    } else {
      //manual sx vldo disable
      HAL_REG_CLR_BIT(phy, 0x614, BIT4);
    }
  }
}

unsigned int get_product_id(rf_chip_phy_t* phy) {
  return 0x0801;
}
short write_chip_lut(rf_chip_phy_t* phy, unsigned char data[]) {
  hal_spi_write_reg(phy, 0x0b8, data[7]);
  hal_spi_write_reg(phy, 0x0b9, data[6]);
  hal_spi_write_reg(phy, 0x0ba, data[5]);
  hal_spi_write_reg(phy, 0x0bb, data[4]);

  //LOG_INFO("./ad9361 w 0x0b8 0x%02x\n", data[7]);
  //LOG_INFO("./ad9361 w 0x0b9 0x%02x\n", data[6]);
  //LOG_INFO("./ad9361 w 0x0ba 0x%02x\n", data[5]);
  //LOG_INFO("./ad9361 w 0x0bb 0x%02x\n", data[4]);

  hal_spi_write_reg(phy, 0x0b6, data[1]);
  hal_spi_write_reg(phy, 0x0b7, data[0]);
  hal_spi_write_reg(phy, 0x0b5, data[2]);
  hal_spi_write_reg(phy, 0x0b4, data[3]);
  hal_spi_write_reg(phy, 0x0b5, 0x00);
  hal_spi_write_reg(phy, 0x0b4, 0x00);

  //LOG_INFO("./ad9361 w 0x0b6 0x%x\n", data[1]);
  //LOG_INFO("./ad9361 w 0x0b7 0x%x\n", data[0]);
  //LOG_INFO("./ad9361 w 0x0b5 0x%02x\n", data[2]);
  //LOG_INFO("./ad9361 w 0x0b4 0x%02x\n", data[3]);
  //LOG_INFO("./ad9361 w 0x0b5 0x00\n");
  //LOG_INFO("./ad9361 w 0x0b4 0x00\n");

  return 0;
}

int gen_int_val(unsigned char* val) {
  return ((val[0]) | (val[1] << 8) | (val[2] << 16) | (val[3] << 24));
}

short lut_version_get(rf_chip_phy_t* phy, unsigned char ver[]) {
  int i;
  unsigned char vals[4];
  read_lut_word(phy, MAIN_ENSM_LUT, 276, vals);
  for (i = 0; i < 4; i++)
    ver[i] = vals[i];
  read_lut_word(phy, MAIN_ENSM_LUT, 277, vals);
  ver[4] = vals[0];
  return 0;
}

short load_lut(rf_chip_phy_t* phy, CHIP_MODE_ENUM mode) {
  //read config from flash
  unsigned char buffer[FILE_MAX], cnt = 0;
  int ret, addr = 0, size[8], i, offset = FILE_MAX;
  short ok;
  const unsigned char(*p)[8];
  handle_t handle;

  if (((hal_spi_read_reg(phy, 0xD33) & 0xF0) >> 4) != 0x07) {
    LOG_ERROR("Reg 0xD33 Err.\n");
    return -1;
  }

#if LOAD_LUT_FROM_BIN_FILE
  //get bin handle
  handle = get_bin_handle(phy, &ok);
  if (!ok) {
    phy->error = -LOAD_LUT_FAIL;
    return -LOAD_LUT_FAIL;
  }

  //read bin header
  my_memset(buffer, 0, sizeof(buffer));
  ret = read_flash(handle, addr, buffer, FILE_MAX);

  //get lut size
  cnt = buffer[0];
  for (i = 0; i < 8; i++)
    size[i] = 0;
  for (i = 0; i < cnt; i++) {
    size[i] = gen_int_val(buffer + i * 4 + 1);
    //LOG_INFO("lut size = %d\n", size[i]);
  }

  //get offset
  for (i = 0; i < mode; i++)
    offset += size[i];

  //read val
  offset = FILE_MAX;
  //for (i=0; i<size[mode]; i+=8)
  for (i = 0; i < size[0]; i += 8) {
    my_memset(buffer, 0, 8);
    ret = read_flash(handle, offset + i, buffer, 8);
    write_chip_lut(phy, buffer);
  }

  //close bin handle
  close_bin_handle(handle);
#else
  if (phy->config->use_bybrid_mode) {
    if (phy->config->chip_ver == 0) {
      switch (phy->config->mode) {
        case CH1_CH2_FDD:
#if (HYBRID_FDD_CH1CH2_LUT_ON)
          p = lut_D1_HYBRID_CH1CH2_FDD;
#endif
          break;
        case CH1_CH2_TDD:
#if (HYBRID_TDD_CH1CH2_LUT_ON)
          p = lut_D1_HYBRID_CH1CH2_TDD;
#endif
          break;
        case CH1_FDD:
#if (HYBRID_FDD_CH1_LUT_ON)
          p = lut_D1_HYBRID_CH1_FDD;
#endif
          break;
        case CH2_FDD:
#if (HYBRID_FDD_CH2_LUT_ON)
          p = lut_D1_HYBRID_CH2_FDD;
#endif
          break;
        default:
          phy->error = -LOAD_LUT_FAIL;
          return -LOAD_LUT_FAIL;
          break;
      }
    } else if (phy->config->chip_ver == 1) {
      switch (phy->config->mode) {
        case CH1_CH2_FDD:
#if (HYBRID_FDD_CH1CH2_LUT_ON)
          p = lut_D2_HYBRID_CH1CH2_FDD;
#endif
          break;
        case CH1_CH2_TDD:
#if (HYBRID_TDD_CH1CH2_LUT_ON)
          p = lut_D2_HYBRID_CH1CH2_TDD;
#endif
          break;
        case CH1_FDD:
#if (HYBRID_FDD_CH1_LUT_ON)
          p = lut_D2_HYBRID_CH1_FDD;
#endif
          break;
        case CH2_FDD:
#if (HYBRID_FDD_CH2_LUT_ON)
          p = lut_D2_HYBRID_CH2_FDD;
#endif
          break;
        default:
          phy->error = -LOAD_LUT_FAIL;
          return -LOAD_LUT_FAIL;
          break;
      }
    } else if (phy->config->chip_ver == 2) {
      switch (phy->config->mode) {
        case CH1_CH2_FDD:
#if (HYBRID_FDD_CH1CH2_LUT_ON)
          p = lut_E1_HYBRID_CH1CH2_FDD;
#endif
          break;
        case CH1_CH2_TDD:
#if (HYBRID_TDD_CH1CH2_LUT_ON)
          p = lut_E1_HYBRID_CH1CH2_TDD;
#endif
          break;
        case CH1_FDD:
#if (HYBRID_FDD_CH1_LUT_ON)
          p = lut_E1_HYBRID_CH1_FDD;
#endif
          break;
        case CH2_FDD:
#if (HYBRID_FDD_CH2_LUT_ON)
          p = lut_E1_HYBRID_CH2_FDD;
#endif
          break;
        default:
          phy->error = -LOAD_LUT_FAIL;
          return -LOAD_LUT_FAIL;
          break;
      }
    } else {
      phy->error = -LOAD_LUT_FAIL;
      return -LOAD_LUT_FAIL;
    }
  } else {
    if (phy->config->chip_ver == 0) {
      switch (mode) {
        case CH1_FDD:
#if (CH1_FDD_LUT_ON)
          p = lut_D1_CH1_FDD;
#endif
          break;

        case RX1_TX2_FDD:
#if (RX1_TX2_FDD_LUT_ON)
          p = lut_D1_RX1TX2_FDD;
#endif
          break;

        case RX2_TX1_FDD:
#if (RX2_TX1_FDD_LUT_ON)
          p = lut_D1_RX2TX1_FDD;
#endif
          break;

        case CH2_FDD:
#if (CH2_FDD_LUT_ON)
          p = lut_D1_CH2_FDD;
#endif
          break;

        case CH1_CH2_FDD:
#if (CH1_CH2_FDD_LUT_ON)
          p = lut_D1_CH1CH2_FDD;
#endif
          break;

        case CH1_TDD:
#if (CH1_TDD_LUT_ON)
          p = lut_D1_CH1_TDD;
#endif
          break;

        case CH2_TDD:
#if (CH2_TDD_LUT_ON)
          p = lut_D1_CH2_TDD;
#endif
          break;

        case CH1_CH2_TDD:
#if (CH1_CH2_TDD_LUT_ON)
          p = lut_D1_CH1CH2_TDD;
#endif
          break;

        default:
          phy->error = -LOAD_LUT_FAIL;
          return -LOAD_LUT_FAIL;
      }
    } else if (phy->config->chip_ver == 1) {
      switch (mode) {
        case CH1_FDD:
#if (CH1_FDD_LUT_ON)
          p = lut_D2_CH1_FDD;
#endif
          break;

        case RX1_TX2_FDD:
#if (RX1_TX2_FDD_LUT_ON)
          p = lut_D2_RX1TX2_FDD;
#endif
          break;

        case RX2_TX1_FDD:
#if (RX2_TX1_FDD_LUT_ON)
          p = lut_D2_RX2TX1_FDD;
#endif
          break;

        case CH2_FDD:
#if (CH2_FDD_LUT_ON)
          p = lut_D2_CH2_FDD;
#endif
          break;

        case CH1_CH2_FDD:
#if (CH1_CH2_FDD_LUT_ON)
          p = lut_D2_CH1CH2_FDD;
#endif
          break;

        case CH1_TDD:
#if (CH1_TDD_LUT_ON)
          p = lut_D2_CH1_TDD;
#endif
          break;

        case CH2_TDD:
#if (CH2_TDD_LUT_ON)
          p = lut_D2_CH2_TDD;
#endif
          break;

        case CH1_CH2_TDD:
#if (CH1_CH2_TDD_LUT_ON)
          p = lut_D2_CH1CH2_TDD;
#endif
          break;

        default:
          phy->error = -LOAD_LUT_FAIL;
          return -LOAD_LUT_FAIL;
      }
    } else if (phy->config->chip_ver == 2) {
      switch (mode) {
        case CH1_FDD:
#if (CH1_FDD_LUT_ON)
          p = lut_E1_CH1_FDD;
#endif
          break;

        case RX1_TX2_FDD:
#if (RX1_TX2_FDD_LUT_ON)
          p = lut_E1_RX1TX2_FDD;
#endif
          break;

        case RX2_TX1_FDD:
#if (RX2_TX1_FDD_LUT_ON)
          p = lut_E1_RX2TX1_FDD;
#endif
          break;

        case CH2_FDD:
#if (CH2_FDD_LUT_ON)
          p = lut_E1_CH2_FDD;
#endif
          break;

        case CH1_CH2_FDD:
#if (CH1_CH2_FDD_LUT_ON)
          p = lut_E1_CH1CH2_FDD;
#endif
          break;

        case CH1_TDD:
#if (CH1_TDD_LUT_ON)
          p = lut_E1_CH1_TDD;
#endif
          break;

        case CH2_TDD:
#if (CH2_TDD_LUT_ON)
          p = lut_E1_CH2_TDD;
#endif
          break;

        case CH1_CH2_TDD:
#if (CH1_CH2_TDD_LUT_ON)
          p = lut_E1_CH1CH2_TDD;
#endif
          break;

        default:
          phy->error = -LOAD_LUT_FAIL;
          return -LOAD_LUT_FAIL;
      }
    } else {
      phy->error = -LOAD_LUT_FAIL;
      return -LOAD_LUT_FAIL;
    }
  }

  for (i = 0; i < 3752; i++)
    write_chip_lut(phy, (unsigned char*)(p[i]));
#endif

  return 0;
}

/****************************************************************************************/
// fsm state controll
/****************************************************************************************/
int fdd_force_wait(rf_chip_phy_t* phy) {
  hal_spi_write_reg(phy, 0xcc, 0x08);
  return 0;
}

int fdd_wait_to_alert(rf_chip_phy_t* phy) {
  HAL_CONFIG_REGS(phy, G_REG_ARRAY_DIG_2_FORCE_AlERT, G_REG_ARRAY_DIG_2_FORCE_AlERT_LEN);
  return 0;
}

int fdd_alert_to_fsm(rf_chip_phy_t* phy) {
  HAL_CONFIG_REGS(phy, G_REG_ARRAY_DIG_3A_FSM_STATE_EN_FDD_1T1R, G_REG_ARRAY_DIG_3A_FSM_STATE_EN_FDD_1T1R_LEN);
  return 0;
}

int fdd_fsm_to_alert(rf_chip_phy_t* phy) {
  HAL_CONFIG_REGS(phy, G_REG_ARRAY_DIG_2_2_FDD_FORCE_AlERT, G_REG_ARRAY_DIG_2_2_FDD_FORCE_AlERT_LEN);
  return 0;
}

void fcal_s2_bypass(rf_chip_phy_t* phy, char fast_sx_lock) {
  if (fast_sx_lock)
    HAL_REG_SET_BIT(phy, 0x0CF, BIT2);
  else
    HAL_REG_CLR_BIT(phy, 0x0CF, BIT2);
}

void core2_s7_s8_s10_s11_s12_bypass(rf_chip_phy_t* phy, char core2_enable) {
  unsigned int reg_val;

  if (core2_enable) {
    HAL_REG_CLR_BIT(phy, 0x0CF, BIT7);
    reg_val = hal_spi_read_reg(phy, 0x0CE);
    reg_val = (reg_val & 0xFF) & ((~(0x7 << 2)) & 0xFF) & ((~0x1) & 0xFF);
    hal_spi_write_reg(phy, 0x0CE, reg_val);
  } else {
    HAL_REG_SET_BIT(phy, 0x0CF, BIT7);
    reg_val = hal_spi_read_reg(phy, 0x0CE);
    reg_val = (reg_val & 0xFF) | (0x7 << 2) | 0x1;
    hal_spi_write_reg(phy, 0x0CE, reg_val);
  }
}

/****************************************************************************************/
// ldo cal
/****************************************************************************************/
void misc_init(rf_chip_phy_t* phy) {
  unsigned char val;
  short reg;

  testmux_clear(phy);
  if (phy->config->chip_ver) {
    val = hal_spi_read_reg(phy, 0x666);
    hal_spi_write_reg(phy, 0x666, (val & 0xC7) | 0x08);

    val = hal_spi_read_reg(phy, 0x668);
    hal_spi_write_reg(phy, 0x668, (val & 0xF1) | 0x02);

    val = hal_spi_read_reg(phy, 0x684);
    hal_spi_write_reg(phy, 0x684, (val & 0xC7) | 0x08);

    val = hal_spi_read_reg(phy, 0x686);
    hal_spi_write_reg(phy, 0x686, (val & 0xF1) | 0x02);
  }
}

void testmux_clear(rf_chip_phy_t* phy) {
  int i;
  for (i = 0x828; i <= 0x843; i++) {
    hal_spi_write_reg(phy, i, 0x0);
  }
  hal_spi_write_reg(phy, 0xd2b, 0x0);
}

void test_read_enable(rf_chip_phy_t* phy, short reg, short en) {
  if (en)
    hal_spi_write_reg(phy, reg, 0x80);
  else
    hal_spi_write_reg(phy, reg, 0x0);
}

short test_voff_get(rf_chip_phy_t* phy, MT_AUXADC_ENUM mt, short ctrl_reg1, short ctrl_reg2, short read_reg1, short read_reg2) {
  unsigned char swp_config[4][4] = {
      {0x00, 0x71, 0x80, 0xB1},
      {0x00, 0xB1, 0x80, 0x71},
      {0x00, 0x71, 0x80, 0xB1},
      {0x00, 0xB1, 0x80, 0x71},
  };
  short reg_val1, reg_val2;
  short cat1, cat2, result;

  //non-swapped
  hal_spi_write_reg(phy, ctrl_reg1, swp_config[mt][0]);
  hal_spi_write_reg(phy, ctrl_reg2, swp_config[mt][1]);
  test_read_enable(phy, read_reg1, 0);
  test_read_enable(phy, read_reg1, 1);
  reg_val1 = hal_spi_read_reg(phy, read_reg2);
  reg_val2 = hal_spi_read_reg(phy, read_reg1);
  test_read_enable(phy, read_reg1, 0);
  cat1 = ((reg_val2 & 0x7) << 8) | reg_val1;

  //swapped
  hal_spi_write_reg(phy, ctrl_reg1, swp_config[mt][2]);
  hal_spi_write_reg(phy, ctrl_reg2, swp_config[mt][3]);
  test_read_enable(phy, read_reg1, 0);
  test_read_enable(phy, read_reg1, 1);
  reg_val1 = hal_spi_read_reg(phy, read_reg2);
  reg_val2 = hal_spi_read_reg(phy, read_reg1);
  test_read_enable(phy, read_reg1, 0);

  cat2 = ((reg_val2 & 0x7) << 8) | reg_val1;

  cat2 = cat2 - 2048;

  if (cat1 > 1023)
    cat1 = (cat1 - 2048) * 2;
  else
    cat1 = cat1 * 2;

  if (cat2 > 1023)
    cat2 = (cat2 - 2048) * 2;
  else
    cat2 = cat2 * 2;

  result = (cat1 - cat2) / 2;

  return result;
}

short test_voff_get_ext(rf_chip_phy_t* phy, MT_AUXADC_ENUM mt, short ctrl_reg1, short ctrl_reg2, short read_reg1, short read_reg2) {
  unsigned char swp_config[4][4] = {
      {0x00, 0x71, 0x80, 0xB1},
      {0x00, 0xB1, 0x80, 0x71},
      {0x00, 0x71, 0x80, 0xB1},
      {0x00, 0xB1, 0x80, 0x71},
  };
  short reg_val1, reg_val2;
  short cat1, cat2, result;

  //non-swapped
  hal_spi_write_reg(phy, ctrl_reg1, swp_config[mt][0]);
  hal_spi_write_reg(phy, ctrl_reg2, swp_config[mt][1]);
  test_read_enable(phy, read_reg1, 0);
  test_read_enable(phy, read_reg1, 1);
  reg_val1 = hal_spi_read_reg(phy, read_reg2);  //86e
  reg_val2 = hal_spi_read_reg(phy, read_reg1);  //86f
  test_read_enable(phy, read_reg1, 0);
  cat1 = ((reg_val2 & 0x3) << 8) | reg_val1;

  if (0x2 == (reg_val2 & 0x2)) {
    cat1 = cat1 - 1024;
  }

  //swapped
  hal_spi_write_reg(phy, ctrl_reg1, swp_config[mt][2]);
  hal_spi_write_reg(phy, ctrl_reg2, swp_config[mt][3]);
  test_read_enable(phy, read_reg1, 0);
  test_read_enable(phy, read_reg1, 1);
  reg_val1 = hal_spi_read_reg(phy, read_reg2);
  reg_val2 = hal_spi_read_reg(phy, read_reg1);
  test_read_enable(phy, read_reg1, 0);

  cat2 = ((reg_val2 & 0x3) << 8) | reg_val1;

  if (0x2 == (reg_val2 & 0x2)) {
    cat2 = cat2 - 1024;
  }

  cat1 = cat1 * 2;
  cat2 = cat2 * 2;

  result = (cat1 - cat2) / 2;

  return result;
}

int test_mt_auxadc(rf_chip_phy_t* phy, MT_AUXADC_ENUM mt, short connect_flag, short div3_flag, char type_flag) {
  int i, auxadc_ctrl_reg;
  short auxadc_read_reg1, auxadc_read_reg2;
  short auxadc_swp_reg;
  unsigned char div_config[4] = {0x10, 0x30, 0x01, 0x03};
  int a[20], sum = 0, reg_val_bk, reg_val;

  if (mt <= MT1N_AUXADC) {
    auxadc_ctrl_reg  = 0x861;
    auxadc_read_reg1 = 0x865;
    auxadc_read_reg2 = 0x866;
    auxadc_swp_reg   = 0x860;
  } else {
    auxadc_ctrl_reg  = 0x86A;
    auxadc_read_reg1 = 0x86E;
    auxadc_read_reg2 = 0x86F;
    auxadc_swp_reg   = 0x869;
  }

  //mt to auxadc
  if (connect_flag)
    hal_spi_write_reg(phy, 0x843, 1 << mt);

  //turn on auxadc
  //hal_spi_write_reg(phy, 0x803, 0x0F);
  //hal_spi_write_reg(phy, 0x805, 0x3F);
  //hal_spi_write_reg(phy, 0x600, 0x3);
  //hal_spi_write_reg(phy, 0x6f1, 0x1B);

  reg_val_bk = hal_spi_read_reg(phy, 0x805);
  reg_val    = reg_val_bk | 0x24;
  hal_spi_write_reg(phy, 0x805, reg_val);

  //mt to AVSS
  hal_spi_write_reg(phy, 0x842, 2 << mt);

  //turn on auxadc
  //hal_spi_write_reg(phy, 0x803, 0x0F);
  //hal_spi_write_reg(phy, 0x805, 0x3F);
  //hal_spi_write_reg(phy, 0x600, 0x3);
  //hal_spi_write_reg(phy, 0x6f1, 0x1B);

  hal_spi_write_reg(phy, auxadc_ctrl_reg, 0x30);
  hal_spi_write_reg(phy, auxadc_ctrl_reg, 0x0);
  hal_spi_write_reg(phy, auxadc_ctrl_reg, 0x0);
  hal_spi_write_reg(phy, auxadc_ctrl_reg, 0x0);
  hal_spi_write_reg(phy, auxadc_ctrl_reg, 0x30);
  hal_spi_write_reg(phy, auxadc_ctrl_reg, 0x31);

  if (div3_flag)
    hal_spi_write_reg(phy, 0xD2E, div_config[mt]);
  else
    hal_spi_write_reg(phy, 0xD2E, 0x00);

  //input non-swapped/swapped
  for (i = 1; i <= 20; i++) {
    if (1 == type_flag) {
      a[i - 1] = test_voff_get_ext(phy, mt, auxadc_swp_reg, auxadc_ctrl_reg, auxadc_read_reg1, auxadc_read_reg2);
    } else {
      a[i - 1] = test_voff_get(phy, mt, auxadc_swp_reg, auxadc_ctrl_reg, auxadc_read_reg1, auxadc_read_reg2);
    }
  }
  simple_sort(20, a);
  for (i = 4; i <= 15; i++)
    sum = sum + a[i];
  sum = sum / 12;

  hal_spi_write_reg(phy, 0x805, reg_val_bk);

  return sum;
}

void test_div3_config(rf_chip_phy_t* phy, short en) {
  if (en) {
    HAL_REG_SET_BIT(phy, 0x901, BIT4);
  } else {
    HAL_REG_CLR_BIT(phy, 0x901, BIT4);
    hal_spi_write_reg(phy, 0xD2E, 0x0);
  }
}

int extpin_voltage_read(rf_chip_phy_t* phy) {
  int vol              = 0;
  auxadc_ctrl ctrl_bak = {0};

  //backup registers
  auxadcctrl_backup(phy, MT1P_AUXADC, &ctrl_bak);
  testmux_clear(phy);
  hal_spi_write_reg(phy, 0x843, 0x11);
  vol = test_mt_auxadc(phy, MT1P_AUXADC, 0, 0, 0);
  auxadccal_transfer(phy, &vol);
  //recovery registers
  auxadcctrl_recover(phy, MT1P_AUXADC, &ctrl_bak);

  return vol;
}

int chip_temp_get(rf_chip_phy_t* phy) {
  int temp             = 0;
  auxadc_ctrl ctrl_bak = {0};

  //backup registers
  auxadcctrl_backup(phy, MT1P_AUXADC, &ctrl_bak);
  testmux_clear(phy);
  hal_spi_write_reg(phy, 0x841, 0x10);
  //mt1p to auxadc
  temp = test_mt_auxadc(phy, MT1P_AUXADC, 1, 0, 0);

  temp = temp - 497;
  //recovery registers
  auxadcctrl_recover(phy, MT1P_AUXADC, &ctrl_bak);

  return temp;
}

short sxtrx_lock_status(rf_chip_phy_t* phy, short dir, int* pVoltage) {
  short i, ret = 0;
  int val1[8], val2[8];
  short reg;
  unsigned char reg_val;
  MT_AUXADC_ENUM mt    = dir ? MT2P_AUXADC : MT1P_AUXADC;
  auxadc_ctrl ctrl_bak = {0};

  auxadcctrl_backup(phy, mt, &ctrl_bak);
  testmux_clear(phy);

  //RXPDET_TEST AUXADC TEST CLEAR
  reg_val = hal_spi_read_reg(phy, 0x668);
  reg_val = reg_val & 0xF0;
  hal_spi_write_reg(phy, 0x668, reg_val | 0x02);

  reg_val = hal_spi_read_reg(phy, 0x686);
  reg_val = reg_val & 0xF0;
  hal_spi_write_reg(phy, 0x686, reg_val | 0x02);

  ////Set TestEN and Test SEL
  hal_spi_write_reg(phy, 0x82A, dir ? 0x40 : 0x20);
  hal_spi_write_reg(phy, 0x830, dir ? (3 << 4) : 3);

  val1[3] = test_mt_auxadc(phy, dir ? MT2P_AUXADC : MT1P_AUXADC, 1, 0, 1);
  auxadccal_transfer(phy, &val1[3]);
  LOG_MAIN("voltage[%d]=%d\n", 3, val1[3]);
  /* whether lock or not, save voltage to *pVoltage */
  *pVoltage = val1[3];

  if ((val1[3] < phy->config->vol_low_limit) || (val1[3] > phy->config->vol_up_limit)) {
    if (dir)
      return -SXTX_LOCK_FAIL;
    else
      return -SXRX_LOCK_FAIL;
  }

  auxadcctrl_recover(phy, mt, &ctrl_bak);

  return ret;
}

int ldo_get_voltage(rf_chip_phy_t* phy) {
#define MAX_TRY_TIMES 10
  int non_swap_val;
  int swap_val;
  int mv_val;
  int num;
  int tot_mv = 0;
  ;
  unsigned char reg_d2e_v = 0;

  //mt1p to auxadc
  hal_spi_write_reg(phy, 0x843, 0x01);

  /* backup 0xde2 register */
  reg_d2e_v = hal_spi_read_reg(phy, 0xd2e);
  /* DIV 3 */
  hal_spi_write_reg(phy, 0xd2e, 0x10);

  //get mv val
  for (num = 0; num < MAX_TRY_TIMES; num++) {
    hal_spi_write_reg(phy, 0x860, 0x0);
    hal_spi_write_reg(phy, 0x861, 0x71);
    hal_spi_write_reg(phy, 0x865, 0x80);
    non_swap_val = hal_spi_read_reg(phy, 0x865);
    non_swap_val = (non_swap_val & 0x7) << 8;
    non_swap_val |= hal_spi_read_reg(phy, 0x866);
    hal_spi_write_reg(phy, 0x865, 0x0);
    if (non_swap_val > 1023) {
      non_swap_val = (non_swap_val - 2048) * 2;
    } else {
      non_swap_val = non_swap_val * 2;
    }
    hal_spi_write_reg(phy, 0x865, 0x0);

    hal_spi_write_reg(phy, 0x860, 0x80);
    hal_spi_write_reg(phy, 0x861, 0xb1);
    hal_spi_write_reg(phy, 0x865, 0x80);
    swap_val = hal_spi_read_reg(phy, 0x865);
    swap_val = (swap_val & 0x7) << 8;
    swap_val |= hal_spi_read_reg(phy, 0x866);
    hal_spi_write_reg(phy, 0x865, 0x0);
    if (swap_val > 1023) {
      swap_val = (swap_val - 2048) * 2;
    } else {
      swap_val = swap_val * 2;
    }
    mv_val = (non_swap_val - swap_val) / 2;
    LOG_MDEBUG(phy, CHIP_LDO_CAL, "LDO Cal non_swap_val:%d, swap_val:%d, mv:%d\n", non_swap_val, swap_val, mv_val);
    tot_mv += mv_val;
  }

  /* recover 0xd2e register */
  hal_spi_write_reg(phy, 0xd2e, reg_d2e_v);

  return tot_mv / MAX_TRY_TIMES;
}

void do_ldo_cal(rf_chip_phy_t* phy, int ref_voltage, unsigned int reg, unsigned int shift_num) {
  int res;
  int reg_ori_val;
  int tune_mid;
  int tune_max     = 15;
  int tune_min     = 0;
  int closed_val   = 0xffff;
  int closed_index = 0;
  reg_ori_val      = hal_spi_read_reg(phy, reg);
  if (reg == 0x904) {
    reg_ori_val = 0xa0;
  }

  if (shift_num == 0) {
    reg_ori_val &= 0xf0;
  } else {
    reg_ori_val &= 0x0f;
  }
  while (tune_min <= tune_max) {
    tune_mid = (tune_max + tune_min) / 2;
    hal_spi_write_reg(phy, reg, (reg_ori_val | (tune_mid << shift_num)));
    res = ldo_get_voltage(phy);
    res = res * 3;  //reg 901 bit 4 bypass, result need to be 3 times value;
    LOG_MDEBUG(phy, CHIP_LDO_CAL, "LDO Cal cal val:0x%0x, tune_min:%d, tune_max:%d, real_voltage:%d, ref_voltage:%d\n",
               tune_mid, tune_min, tune_max, res, ref_voltage);
    if (res > ref_voltage) {
      tune_max = tune_mid - 1;
      if (res - ref_voltage < closed_val) {
        closed_index = tune_mid;
        closed_val   = res - ref_voltage;
      }
    } else if (res < ref_voltage) {
      tune_min = tune_mid + 1;
      if (ref_voltage - res < closed_val) {
        closed_index = tune_mid;
        closed_val   = ref_voltage - res;
      }
    } else {
      closed_index = tune_mid;
      closed_val   = res - ref_voltage;
      break;
    }
  }

  hal_spi_write_reg(phy, reg, (reg_ori_val | (closed_index << shift_num)));
  LOG_MDEBUG(phy, CHIP_LDO_CAL, "LDO Cal end reg[0x%0x] = 0x%0x, cal result:%x\n",
             reg, hal_spi_read_reg(phy, reg), closed_index);
}

void ldo_cal(rf_chip_phy_t* phy) {

  unsigned int reg_901_v;
  unsigned int reg_81b_v;
  unsigned int reg_904_v;
  auxadc_ctrl ctrl_bak = {0};

  if (1 != phy->ldo_cal_flag) {
    auxadcctrl_backup(phy, MT1P_AUXADC, &ctrl_bak);

    reg_901_v = hal_spi_read_reg(phy, 0x5d);
    //hal_spi_write_reg(phy, 0x901, (reg_901_v | 0x10));
    hal_spi_write_reg(phy, 0x901, 0x5e);

    ldo_init(phy);
    auxadc1_reset(phy);
#if 0 /* sx_vco is in NC status, and sx_vco not used */
        //ldo_sx_vco
        LOG_MDEBUG(phy, CHIP_LDO_CAL, "LDO Cal start\n");
        LOG_MDEBUG(phy, CHIP_LDO_CAL, "LDO Cal for sx_vco start\n");
        testmux_clear(phy);
        hal_spi_write_reg(phy, 0x828, 0x01);
        hal_spi_write_reg(phy, 0x835, 0x03);
        do_ldo_cal(phy, phy->config->sx_vco_ldo, 0x81a, 0);
        LOG_MDEBUG(phy, CHIP_LDO_CAL, "LDO Cal for sx_vco end\n");
#else
    reg_81b_v = hal_spi_read_reg(phy, 0x81a);
    hal_spi_write_reg(phy, 0x81a, (reg_81b_v | 0x08));
#endif
    //ldo_sxlf
    LOG_MDEBUG(phy, CHIP_LDO_CAL, "LDO Cal for sxlf start\n");
    testmux_clear(phy);
    hal_spi_write_reg(phy, 0x828, 0x02);
    hal_spi_write_reg(phy, 0x835, 0x03);
    do_ldo_cal(phy, phy->config->sxlf_ldo, 0x81a, 4);
    LOG_MDEBUG(phy, CHIP_LDO_CAL, "LDO Cal for sxlf end\n");

    //ldo_syspll
    LOG_MDEBUG(phy, CHIP_LDO_CAL, "LDO Cal for syspll start\n");
    testmux_clear(phy);
    hal_spi_write_reg(phy, 0x828, 0x04);
    hal_spi_write_reg(phy, 0x835, 0x03);
    do_ldo_cal(phy, phy->config->syspll_ldo, 0x819, 4);
    LOG_MDEBUG(phy, CHIP_LDO_CAL, "LDO Cal for syspll end\n");

#if 1
    //ldo_txabb
    LOG_MDEBUG(phy, CHIP_LDO_CAL, "LDO Cal for txabb start\n");
    testmux_clear(phy);
    hal_spi_write_reg(phy, 0x829, 0x01);
    hal_spi_write_reg(phy, 0x835, 0x03);
    do_ldo_cal(phy, phy->config->txabb_ldo, 0x818, 0);
    LOG_MDEBUG(phy, CHIP_LDO_CAL, "LDO Cal for txabb end\n");

    //ldo_txdac
    LOG_MDEBUG(phy, CHIP_LDO_CAL, "LDO Cal for txdac start\n");
    testmux_clear(phy);
    hal_spi_write_reg(phy, 0x829, 0x02);
    hal_spi_write_reg(phy, 0x835, 0x03);
    do_ldo_cal(phy, phy->config->txdac_ldo, 0x818, 4);
    LOG_MDEBUG(phy, CHIP_LDO_CAL, "LDO Cal for txdac end\n");
#else
    hal_spi_write_reg(phy, 0x818, 0x88);
#endif

    //ldo_txfe
    LOG_MDEBUG(phy, CHIP_LDO_CAL, "LDO Cal for txfe start\n");
    testmux_clear(phy);
    hal_spi_write_reg(phy, 0x829, 0x04);
    hal_spi_write_reg(phy, 0x835, 0x03);
    do_ldo_cal(phy, phy->config->txfe_ldo, 0x817, 0);
    LOG_MDEBUG(phy, CHIP_LDO_CAL, "LDO Cal for txfe end\n");

    //ldo_txsx_lo
    LOG_MDEBUG(phy, CHIP_LDO_CAL, "LDO Cal for txsx_lo start\n");
    testmux_clear(phy);
    hal_spi_write_reg(phy, 0x829, 0x08);
    hal_spi_write_reg(phy, 0x835, 0x03);
    do_ldo_cal(phy, phy->config->txsx_lo_ldo, 0x817, 4);
    LOG_MDEBUG(phy, CHIP_LDO_CAL, "LDO Cal for txsx_lo end\n");

#if 0
        //ldo_rxadc
        LOG_MDEBUG(phy, CHIP_LDO_CAL, "LDO Cal for rxadc start\n");
        testmux_clear(phy);
        hal_spi_write_reg(phy, 0x829, 0x10);
        hal_spi_write_reg(phy, 0x835, 0x03);
        do_ldo_cal(phy, phy->config->rxadc_ldo, 0x81b, 0);
        LOG_MDEBUG(phy, CHIP_LDO_CAL, "LDO Cal for rxadc end\n");
#else
    reg_81b_v = hal_spi_read_reg(phy, 0x81b);
    hal_spi_write_reg(phy, 0x81b, (reg_81b_v | 0x08));
#endif

    //ldo_rxfe
    LOG_MDEBUG(phy, CHIP_LDO_CAL, "LDO Cal for rxfe start\n");
    testmux_clear(phy);
    hal_spi_write_reg(phy, 0x829, 0x20);
    hal_spi_write_reg(phy, 0x835, 0x03);
    do_ldo_cal(phy, phy->config->rxfe_ldo, 0x81b, 4);
    /* the high 4bit value should large than 0xA.*/
    reg_81b_v = hal_spi_read_reg(phy, 0x81b);
    if ((reg_81b_v & 0xF0) < 0xA0) {
      hal_spi_write_reg(phy, 0x81b, (0xA0 | (reg_81b_v & 0xF)));
      LOG_MDEBUG(phy, CHIP_LDO_CAL, "%s in line %d register 0x81b[0x%02x] has been change to [0x%02x] by force\n",
                 __FUNCTION__, __LINE__, reg_81b_v, hal_spi_read_reg(phy, 0x81b));
    }
    LOG_MDEBUG(phy, CHIP_LDO_CAL, "LDO Cal for rxfe end\n");

    //ldo_rxsx_lo
    LOG_MDEBUG(phy, CHIP_LDO_CAL, "LDO Cal for rxsx lo start\n");
    testmux_clear(phy);
    hal_spi_write_reg(phy, 0x829, 0x40);
    hal_spi_write_reg(phy, 0x835, 0x03);
    do_ldo_cal(phy, phy->config->rxsx_lo_ldo, 0x819, 0);
    LOG_MDEBUG(phy, CHIP_LDO_CAL, "LDO Cal for rxsx lo end\n");

    //mdig_ldo
    LOG_MDEBUG(phy, CHIP_LDO_CAL, "LDO Cal for mdig start\n");

    testmux_clear(phy);
    hal_spi_write_reg(phy, 0x843, 0x10);
    hal_spi_write_reg(phy, 0x841, 0x10);
    hal_spi_write_reg(phy, 0x835, 0x04);
    hal_spi_write_reg(phy, 0x904, 0xa0);
    do_ldo_cal(phy, phy->config->mdig_ldo, 0x904, 0);
    LOG_MDEBUG(phy, CHIP_LDO_CAL, "LDO Cal for mdig end\n");

    //restore
    hal_spi_write_reg(phy, 0x901, reg_901_v);
    auxadcctrl_recover(phy, MT1P_AUXADC, &ctrl_bak);

    phy->ldo_cal_flag = 1;
    phy->ldo_cal[0]   = hal_spi_read_reg(phy, 0x817);
    phy->ldo_cal[1]   = hal_spi_read_reg(phy, 0x818);
    phy->ldo_cal[2]   = hal_spi_read_reg(phy, 0x819);
    phy->ldo_cal[3]   = hal_spi_read_reg(phy, 0x81A);
    phy->ldo_cal[4]   = hal_spi_read_reg(phy, 0x81B);
    phy->ldo_cal[5]   = hal_spi_read_reg(phy, 0x60) & 0x0F;
  } else {
    hal_spi_write_reg(phy, 0x817, phy->ldo_cal[0]);
    hal_spi_write_reg(phy, 0x818, phy->ldo_cal[1]);
    hal_spi_write_reg(phy, 0x819, phy->ldo_cal[2]);
    hal_spi_write_reg(phy, 0x81A, phy->ldo_cal[3]);
    hal_spi_write_reg(phy, 0x81B, phy->ldo_cal[4]);
    reg_904_v = hal_spi_read_reg(phy, 0x60) & 0xF0;
    hal_spi_write_reg(phy, 0x904, reg_904_v | (phy->ldo_cal[5] & 0x0F));
  }

  LOG_MDEBUG(phy, CHIP_LDO_CAL, "LDO Cal end\n");
}

int sx_temperature_track_v1(rf_chip_phy_t* phy, TRX_ENUM trx) {
  unsigned short da_sxtrx_vco_scap_reg_h, da_sxtrx_vco_scap_reg_l;
  unsigned short rg_sxtrx_vco_scap_reg_h, rg_sxtrx_vco_scap_reg_l;
  unsigned short sxtrx_ctrl_reg;
  unsigned char sxtrx_vco_scap_val_h, sxtrx_vco_scap_val_l;
  int scap, scap_init, scap_low, scap_up, Vctrl, i;
  unsigned char lo_div;
  unsigned long long flo;
  unsigned long long fvco;
  unsigned int write_reg_flag = 0;
  unsigned short sx_to_lo_reg;

  if (trx == RX_DIR) {
    rg_sxtrx_vco_scap_reg_h = 0x017;
    rg_sxtrx_vco_scap_reg_l = 0x018;
    da_sxtrx_vco_scap_reg_h = 0x62e;
    da_sxtrx_vco_scap_reg_l = 0x62f;
    sxtrx_ctrl_reg          = 0x634;
    sx_to_lo_reg            = 0x620;
    flo                     = phy->config->rx_flo;
  } else {
    rg_sxtrx_vco_scap_reg_h = 0x022;
    rg_sxtrx_vco_scap_reg_l = 0x023;
    da_sxtrx_vco_scap_reg_h = 0x647;
    da_sxtrx_vco_scap_reg_l = 0x648;
    sxtrx_ctrl_reg          = 0x64d;
    sx_to_lo_reg            = 0x639;
    flo                     = phy->config->tx_flo;
  }

  /* calc lo div */
  lo_div = calc_lo_div(phy, flo);

  /* calc fvco */
  fvco = flo << lo_div;

  LOG_MDEBUG(phy, SX_TRX_CAL, "--------------------------------------sx_temperature_track_v1---------------------------------------------------\n");
  HAL_REG_SET_BIT(phy, 0xD05, BIT7);  //x4 cal tracking enable

  if (fvco < phy->config->fvco_limit) {
    hal_spi_write_reg(phy, 0x901, phy->config->reg901_val);
    if (CHIP_VERSION_D1 == phy->config->chip_ver) {
      hal_spi_write_reg(phy, 0x61A, phy->config->reg61A_val);
      hal_spi_write_reg(phy, 0x600, phy->config->reg600_val);
      hal_spi_write_reg(phy, 0x602, phy->config->reg602_val);
    }
  }

  sxtrx_vco_scap_val_h = hal_spi_read_reg(phy, rg_sxtrx_vco_scap_reg_h);
  sxtrx_vco_scap_val_l = hal_spi_read_reg(phy, rg_sxtrx_vco_scap_reg_l);
  hal_spi_write_reg(phy, da_sxtrx_vco_scap_reg_h, sxtrx_vco_scap_val_h);
  hal_spi_write_reg(phy, da_sxtrx_vco_scap_reg_l, sxtrx_vco_scap_val_l);

  (void)sxtrx_lock_status(phy, trx, &Vctrl);
  scap_init = sxtrx_vco_scap_val_h << 8 | sxtrx_vco_scap_val_l;
  LOG_MDEBUG(phy, SX_TRX_CAL, "sx_temperature_track_v1 start, low_limit=%dmV, up_limit=%dmV, margin=%dmV, Voltage=%dmV, reg%03x=0x%x, reg%03x=0x%x, reg%03x=0x%x, reg%03x=0x%x, reg%03x=0x%x\n",
             phy->config->vol_low_limit, phy->config->vol_up_limit, phy->config->vol_margin, Vctrl,
             sxtrx_ctrl_reg, hal_spi_read_reg(phy, sxtrx_ctrl_reg),
             rg_sxtrx_vco_scap_reg_h, sxtrx_vco_scap_val_h,
             rg_sxtrx_vco_scap_reg_l, sxtrx_vco_scap_val_l,
             da_sxtrx_vco_scap_reg_h, hal_spi_read_reg(phy, da_sxtrx_vco_scap_reg_h),
             da_sxtrx_vco_scap_reg_l, hal_spi_read_reg(phy, da_sxtrx_vco_scap_reg_l));

  do {
    if (Vctrl > (phy->config->vol_up_limit - phy->config->vol_margin)) {
      i       = 0;
      scap_up = scap_init;
      scap    = scap_init;
      while (scap >= phy->config->scap_min && scap <= phy->config->scap_max) {
        if (Vctrl <= (phy->config->vol_up_limit - phy->config->vol_margin)) {
          scap_up = scap;
          break;
        }

        HAL_REG_SET_BIT(phy, sxtrx_ctrl_reg, BIT3);
        scap--;
        hal_spi_write_reg(phy, da_sxtrx_vco_scap_reg_h, (scap >> 8) & 0xff);
        hal_spi_write_reg(phy, da_sxtrx_vco_scap_reg_l, scap & 0xff);

        (void)sxtrx_lock_status(phy, trx, &Vctrl);
        LOG_MDEBUG(phy, SX_TRX_CAL, "----find track v1 scap running, low_limit=%dmV, up_limit=%dmV, margin=%dmV, Voltage=%dmV, reg%03x=0x%02x, reg%03x=0x%02x, reg%03x=0x%02x, count=%d, scap=%d\n",
                   phy->config->vol_low_limit, phy->config->vol_up_limit, phy->config->vol_margin, Vctrl,
                   sxtrx_ctrl_reg, hal_spi_read_reg(phy, sxtrx_ctrl_reg),
                   da_sxtrx_vco_scap_reg_h, hal_spi_read_reg(phy, da_sxtrx_vco_scap_reg_h),
                   da_sxtrx_vco_scap_reg_l, hal_spi_read_reg(phy, da_sxtrx_vco_scap_reg_l),
                   i, scap);
        i++;
      }

      LOG_MDEBUG(phy, SX_TRX_CAL, "----find track v1 scap running, Voltage=%dmV, find up scap=%d, reg%03x=0x%02x, reg%03x=0x%02x\n",
                 Vctrl, scap_up,
                 da_sxtrx_vco_scap_reg_h, hal_spi_read_reg(phy, da_sxtrx_vco_scap_reg_h),
                 da_sxtrx_vco_scap_reg_l, hal_spi_read_reg(phy, da_sxtrx_vco_scap_reg_l));

      scap_low = scap;
      while (scap >= phy->config->scap_min && scap <= phy->config->scap_max) {
        if (Vctrl <= (phy->config->vol_low_limit + phy->config->vol_margin)) {
          scap_low = scap;
          break;
        }

        HAL_REG_SET_BIT(phy, sxtrx_ctrl_reg, BIT3);
        scap--;
        hal_spi_write_reg(phy, da_sxtrx_vco_scap_reg_h, (scap >> 8) & 0xff);
        hal_spi_write_reg(phy, da_sxtrx_vco_scap_reg_l, scap & 0xff);

        (void)sxtrx_lock_status(phy, trx, &Vctrl);
        LOG_MDEBUG(phy, SX_TRX_CAL, "----find track v1 scap running, low_limit=%dmV, up_limit=%dmV, margin=%dmV, Voltage=%dmV, reg%03x=0x%02x, reg%03x=0x%02x, reg%03x=0x%02x, count=%d, scap=%d\n",
                   phy->config->vol_low_limit, phy->config->vol_up_limit, phy->config->vol_margin, Vctrl,
                   sxtrx_ctrl_reg, hal_spi_read_reg(phy, sxtrx_ctrl_reg),
                   da_sxtrx_vco_scap_reg_h, hal_spi_read_reg(phy, da_sxtrx_vco_scap_reg_h),
                   da_sxtrx_vco_scap_reg_l, hal_spi_read_reg(phy, da_sxtrx_vco_scap_reg_l),
                   i, scap);
        i++;
      }

      LOG_MDEBUG(phy, SX_TRX_CAL, "----find track v1 scap running, Voltage=%dmV, find low scap=%d, reg%03x=0x%02x, reg%03x=0x%02x\n",
                 Vctrl, scap_low,
                 da_sxtrx_vco_scap_reg_h, hal_spi_read_reg(phy, da_sxtrx_vco_scap_reg_h),
                 da_sxtrx_vco_scap_reg_l, hal_spi_read_reg(phy, da_sxtrx_vco_scap_reg_l));
      scap = (scap_up + scap_low) / 2;
    } else if (Vctrl < (phy->config->vol_low_limit + phy->config->vol_margin)) {
      i        = 0;
      scap_low = scap_init;
      scap     = scap_init;
      while (scap >= phy->config->scap_min && scap <= phy->config->scap_max) {
        if (Vctrl >= (phy->config->vol_low_limit + phy->config->vol_margin)) {
          scap_low = scap;
          break;
        }

        HAL_REG_SET_BIT(phy, sxtrx_ctrl_reg, BIT3);
        scap++;
        hal_spi_write_reg(phy, da_sxtrx_vco_scap_reg_h, (scap >> 8) & 0xff);
        hal_spi_write_reg(phy, da_sxtrx_vco_scap_reg_l, scap & 0xff);

        (void)sxtrx_lock_status(phy, trx, &Vctrl);
        LOG_MDEBUG(phy, SX_TRX_CAL, "----find track v1 scap running, low_limit=%dmV, up_limit=%dmV, margin=%dmV, Voltage=%dmV, reg%03x=0x%02x, reg%03x=0x%02x, reg%03x=0x%02x, count=%d, scap=%d\n",
                   phy->config->vol_low_limit, phy->config->vol_up_limit, phy->config->vol_margin, Vctrl,
                   sxtrx_ctrl_reg, hal_spi_read_reg(phy, sxtrx_ctrl_reg),
                   da_sxtrx_vco_scap_reg_h, hal_spi_read_reg(phy, da_sxtrx_vco_scap_reg_h),
                   da_sxtrx_vco_scap_reg_l, hal_spi_read_reg(phy, da_sxtrx_vco_scap_reg_l),
                   i, scap);
        i++;
      }

      LOG_MDEBUG(phy, SX_TRX_CAL, "----find track v1 scap running, Voltage=%dmV, find low scap=%d, reg%03x=0x%02x, reg%03x=0x%02x\n",
                 Vctrl, scap_low,
                 da_sxtrx_vco_scap_reg_h, hal_spi_read_reg(phy, da_sxtrx_vco_scap_reg_h),
                 da_sxtrx_vco_scap_reg_l, hal_spi_read_reg(phy, da_sxtrx_vco_scap_reg_l));

      scap_up = scap;
      while (scap >= phy->config->scap_min && scap <= phy->config->scap_max) {
        if (Vctrl >= (phy->config->vol_up_limit - phy->config->vol_margin)) {
          scap_up = scap;
          break;
        }

        HAL_REG_SET_BIT(phy, sxtrx_ctrl_reg, BIT3);
        scap++;
        hal_spi_write_reg(phy, da_sxtrx_vco_scap_reg_h, (scap >> 8) & 0xff);
        hal_spi_write_reg(phy, da_sxtrx_vco_scap_reg_l, scap & 0xff);

        (void)sxtrx_lock_status(phy, trx, &Vctrl);
        LOG_MDEBUG(phy, SX_TRX_CAL, "----find track v1 scap running, low_limit=%dmV, up_limit=%dmV, margin=%dmV, Voltage=%dmV, reg%03x=0x%02x, reg%03x=0x%02x, reg%03x=0x%02x, count=%d, scap=%d\n",
                   phy->config->vol_low_limit, phy->config->vol_up_limit, phy->config->vol_margin, Vctrl,
                   sxtrx_ctrl_reg, hal_spi_read_reg(phy, sxtrx_ctrl_reg),
                   da_sxtrx_vco_scap_reg_h, hal_spi_read_reg(phy, da_sxtrx_vco_scap_reg_h),
                   da_sxtrx_vco_scap_reg_l, hal_spi_read_reg(phy, da_sxtrx_vco_scap_reg_l),
                   i, scap);
        i++;
      }

      LOG_MDEBUG(phy, SX_TRX_CAL, "----find track v1 scap running, Voltage=%dmV, find up scap=%d, reg%03x=0x%02x, reg%03x=0x%02x\n",
                 Vctrl, scap_up,
                 da_sxtrx_vco_scap_reg_h, hal_spi_read_reg(phy, da_sxtrx_vco_scap_reg_h),
                 da_sxtrx_vco_scap_reg_l, hal_spi_read_reg(phy, da_sxtrx_vco_scap_reg_l));
      scap = (scap_up + scap_low) / 2;

    } else {
      scap = scap_init;
      LOG_MDEBUG(phy, SX_TRX_CAL, "----find track v1 scap running, vctrl=%dmV in range [%dmV, %dmV], so needn't change scap!\n",
                 Vctrl, (phy->config->vol_low_limit + phy->config->vol_margin), (phy->config->vol_up_limit - phy->config->vol_margin));
    }

    if (CHIP_VERSION_D1 == phy->config->chip_ver) {
      i = 0;
    } else {
      if (i >= 30) {
        if (0 == write_reg_flag) {
          hal_spi_write_reg(phy, 0x61A, phy->config->reg61A_val);
          hal_spi_write_reg(phy, 0x600, phy->config->reg600_val);
          hal_spi_write_reg(phy, 0x602, phy->config->reg602_val);
          if (1 == (phy->config->reg639_620_bit1 & 0x1)) {
            /* 0x639 or 0x620 reg bit1 set to 1 */
            HAL_REG_SET_BIT(phy, sx_to_lo_reg, BIT1);
          } else {
            /* 0x639 or 0x620 reg bit1 set to 0 */
            HAL_REG_CLR_BIT(phy, sx_to_lo_reg, BIT1);
          }
          HAL_REG_SET_BIT(phy, sxtrx_ctrl_reg, BIT2);
          LOG_MDEBUG(phy, SX_TRX_CAL, "----find track v1 scap running, count:%d >= 30, restart find scap, reg%03x=0x%02x, reg%03x=0x%02x, reg%03x=0x%02x, reg%03x=0x%02x, reg%03x=0x%02x\n",
                     i,
                     0x61A, hal_spi_read_reg(phy, 0x61A),
                     0x600, hal_spi_read_reg(phy, 0x600),
                     0x602, hal_spi_read_reg(phy, 0x602),
                     sx_to_lo_reg, hal_spi_read_reg(phy, sx_to_lo_reg),
                     sxtrx_ctrl_reg, hal_spi_read_reg(phy, sxtrx_ctrl_reg));

          HAL_REG_SET_BIT(phy, sxtrx_ctrl_reg, BIT3);
          hal_spi_write_reg(phy, da_sxtrx_vco_scap_reg_h, (scap_init >> 8) & 0xff);
          hal_spi_write_reg(phy, da_sxtrx_vco_scap_reg_l, scap_init & 0xff);

          (void)sxtrx_lock_status(phy, trx, &Vctrl);
          LOG_MDEBUG(phy, SX_TRX_CAL, "----find track v1 scap running, set init scap:0x%x, Voltage=%dmV, reg%03x=0x%02x, reg%03x=0x%02x, reg%03x=0x%02x\n",
                     scap_init, Vctrl,
                     sxtrx_ctrl_reg, hal_spi_read_reg(phy, sxtrx_ctrl_reg),
                     da_sxtrx_vco_scap_reg_h, hal_spi_read_reg(phy, da_sxtrx_vco_scap_reg_h),
                     da_sxtrx_vco_scap_reg_l, hal_spi_read_reg(phy, da_sxtrx_vco_scap_reg_l));

          write_reg_flag = 1;
        } else {
          LOG_ERROR("find track v1 scap failed! set scap to init value.\n");
          i    = 0;
          scap = scap_init;
        }
      }
    }
  } while (i >= 30);

  hal_spi_write_reg(phy, da_sxtrx_vco_scap_reg_h, (scap >> 8) & 0xff);
  hal_spi_write_reg(phy, da_sxtrx_vco_scap_reg_l, scap & 0xff);
  (void)sxtrx_lock_status(phy, trx, &Vctrl);
  LOG_MDEBUG(phy, SX_TRX_CAL, "----find track v1 scap finish, low_limit=%dmV, up_limit=%dmV, margin=%dmV, Voltage=%dmV, reg%03x=0x%02x, reg%03x=0x%02x, reg%03x=0x%02x, scap=%d\n",
             phy->config->vol_low_limit, phy->config->vol_up_limit, phy->config->vol_margin, Vctrl,
             sxtrx_ctrl_reg, hal_spi_read_reg(phy, sxtrx_ctrl_reg),
             da_sxtrx_vco_scap_reg_h, hal_spi_read_reg(phy, da_sxtrx_vco_scap_reg_h),
             da_sxtrx_vco_scap_reg_l, hal_spi_read_reg(phy, da_sxtrx_vco_scap_reg_l),
             scap);

  HAL_REG_CLR_BIT(phy, 0xD05, BIT7);  //x4 cal tracking disable

  LOG_MDEBUG(phy, SX_TRX_CAL, "------------------------------------------------end-------------------------------------------------------------\n");

  return 0;
}

int sx_temperature_track_v2(rf_chip_phy_t* phy, TRX_ENUM trx) {
  unsigned short rg_sxtrx_vco_scap_reg_h, rg_sxtrx_vco_scap_reg_l;
  unsigned char sxtrx_vco_scap_val_h, sxtrx_vco_scap_val_l;
  unsigned short sxtrx_vco_scap_reg;
  unsigned short sxtrx_ctrl_reg;
  unsigned char sxtrx_vco_scap_reg_val;
  int scap, Vctrl, i, err;
  unsigned char lo_div;
  unsigned long long flo;
  unsigned long long fvco;

  if (trx == RX_DIR) {
    rg_sxtrx_vco_scap_reg_h = 0x017;
    rg_sxtrx_vco_scap_reg_l = 0x018;
    sxtrx_vco_scap_reg      = 0x00f;
    sxtrx_ctrl_reg          = 0x634;
    flo                     = phy->config->rx_flo;
  } else {
    rg_sxtrx_vco_scap_reg_h = 0x022;
    rg_sxtrx_vco_scap_reg_l = 0x023;
    sxtrx_vco_scap_reg      = 0x01a;
    sxtrx_ctrl_reg          = 0x64d;
    flo                     = phy->config->tx_flo;
  }

  /* calc lo div */
  lo_div = calc_lo_div(phy, flo);

  /* calc fvco */
  fvco = flo << lo_div;

  LOG_MDEBUG(phy, SX_TRX_CAL, "--------------------------------------sx_temperature_track_v2---------------------------------------------------\n");
  HAL_REG_SET_BIT(phy, 0xD05, BIT7);  //x4 cal tracking enable

  if (fvco < phy->config->fvco_limit) {
    hal_spi_write_reg(phy, 0x901, phy->config->reg901_val);
    hal_spi_write_reg(phy, 0x61A, phy->config->reg61A_val);
    hal_spi_write_reg(phy, 0x600, phy->config->reg600_val);
    hal_spi_write_reg(phy, 0x602, phy->config->reg602_val);
  }

  sxtrx_vco_scap_val_h = hal_spi_read_reg(phy, rg_sxtrx_vco_scap_reg_h);
  sxtrx_vco_scap_val_l = hal_spi_read_reg(phy, rg_sxtrx_vco_scap_reg_l);
  scap                 = (sxtrx_vco_scap_val_h << 8) | sxtrx_vco_scap_val_l;

  HAL_REG_CLR_BIT(phy, sxtrx_ctrl_reg, BIT3);

  (void)sxtrx_lock_status(phy, trx, &Vctrl);
  LOG_MDEBUG(phy, SX_TRX_CAL, "sx_temperature_track_v2 start, low_limit=%dmV, up_limit=%dmV, margin=%dmV, Voltage=%dmV, reg%03x=0x%02x, reg%03x=0x%02x, reg%03x=0x%02x, reg%03x=0x%02x\n",
             phy->config->vol_low_limit, phy->config->vol_up_limit, phy->config->vol_margin, Vctrl,
             sxtrx_ctrl_reg, hal_spi_read_reg(phy, sxtrx_ctrl_reg),
             sxtrx_vco_scap_reg, hal_spi_read_reg(phy, sxtrx_vco_scap_reg),
             rg_sxtrx_vco_scap_reg_h, sxtrx_vco_scap_val_h,
             rg_sxtrx_vco_scap_reg_l, sxtrx_vco_scap_val_l);

  if (Vctrl > (phy->config->vol_up_limit - phy->config->vol_margin)) {
    i   = 1;
    err = 3;
    while (i <= 3)  //|| (scap >= phy->config->scap_min && scap <= phy->config->scap_max)
    {
      sxtrx_vco_scap_reg_val = hal_spi_read_reg(phy, sxtrx_vco_scap_reg);
      sxtrx_vco_scap_reg_val = (sxtrx_vco_scap_reg_val & (~0x0e)) | (i << 2);
      hal_spi_write_reg(phy, sxtrx_vco_scap_reg, sxtrx_vco_scap_reg_val);

      (void)sxtrx_lock_status(phy, trx, &Vctrl);
      LOG_MDEBUG(phy, SX_TRX_CAL, "----find track v2 scap running, low_limit=%dmV, up_limit=%dmV, margin=%dmV, Voltage=%dmV, reg%03x=0x%02x, count=%d\n",
                 phy->config->vol_low_limit, phy->config->vol_up_limit, phy->config->vol_margin, Vctrl,
                 sxtrx_vco_scap_reg, hal_spi_read_reg(phy, sxtrx_vco_scap_reg),
                 i);
      if ((Vctrl >= (phy->config->vol_low_limit + phy->config->vol_margin)) && (Vctrl <= (phy->config->vol_up_limit - phy->config->vol_margin))) {
        err = i;
      }

      i++;
    }

    sxtrx_vco_scap_reg_val = hal_spi_read_reg(phy, sxtrx_vco_scap_reg);
    sxtrx_vco_scap_reg_val = (sxtrx_vco_scap_reg_val & (~0x0e)) | (err << 2);
    hal_spi_write_reg(phy, sxtrx_vco_scap_reg, sxtrx_vco_scap_reg_val);
  } else if (Vctrl < (phy->config->vol_low_limit + phy->config->vol_margin)) {
    i   = 1;
    err = 3;
    while (i <= 3)  //|| (scap >= phy->config->scap_min && scap <= phy->config->scap_max)
    {
      sxtrx_vco_scap_reg_val = hal_spi_read_reg(phy, sxtrx_vco_scap_reg);
      sxtrx_vco_scap_reg_val = (sxtrx_vco_scap_reg_val & (~0x0e)) | (0x2 | (i << 2));
      hal_spi_write_reg(phy, sxtrx_vco_scap_reg, sxtrx_vco_scap_reg_val);

      (void)sxtrx_lock_status(phy, trx, &Vctrl);
      LOG_MDEBUG(phy, SX_TRX_CAL, "----find track v2 scap running, low_limit=%dmV, up_limit=%dmV, margin=%dmV, Voltage=%dmV, reg%03x=0x%02x, count=%d\n",
                 phy->config->vol_low_limit, phy->config->vol_up_limit, phy->config->vol_margin, Vctrl,
                 sxtrx_vco_scap_reg, hal_spi_read_reg(phy, sxtrx_vco_scap_reg),
                 i);
      if ((Vctrl >= (phy->config->vol_low_limit + phy->config->vol_margin)) && (Vctrl <= (phy->config->vol_up_limit - phy->config->vol_margin))) {
        err = i;
      }

      i++;
    }

    sxtrx_vco_scap_reg_val = hal_spi_read_reg(phy, sxtrx_vco_scap_reg);
    sxtrx_vco_scap_reg_val = (sxtrx_vco_scap_reg_val & (~0x0e)) | (0x2 | (err << 2));
    hal_spi_write_reg(phy, sxtrx_vco_scap_reg, sxtrx_vco_scap_reg_val);
  } else {
    LOG_MDEBUG(phy, SX_TRX_CAL, "----find track v1 scap running, vctrl=%dmV in range [%dmV, %dmV], so needn't change scap!\n",
               Vctrl, (phy->config->vol_low_limit + phy->config->vol_margin), (phy->config->vol_up_limit - phy->config->vol_margin));
  }

  (void)sxtrx_lock_status(phy, trx, &Vctrl);
  LOG_MDEBUG(phy, SX_TRX_CAL, "----find track v2 scap finish, low_limit=%dmV, up_limit=%dmV, margin=%dmV, Voltage=%dmV, reg%03x=0x%02x, count=%d\n",
             phy->config->vol_low_limit, phy->config->vol_up_limit, phy->config->vol_margin, Vctrl,
             sxtrx_vco_scap_reg, hal_spi_read_reg(phy, sxtrx_vco_scap_reg),
             i);

  HAL_REG_CLR_BIT(phy, 0xD05, BIT7);  //x4 cal tracking disable

  LOG_MDEBUG(phy, SX_TRX_CAL, "------------------------------------------------end-------------------------------------------------------------\n");

  return 0;
}

int get_syspll_status(rf_chip_phy_t* phy) {
  auxadc_ctrl ctrl_bak = {0};
  int reg_val          = 0;
  int reg901 = 0, reg668 = 0, reg686 = 0, reg82a = 0, reg82f = 0, reg834 = 0;
  auxadcctrl_backup(phy, MT1P_AUXADC, &ctrl_bak);
  reg901 = hal_spi_read_reg(phy, 0x5d);
  reg668 = hal_spi_read_reg(phy, 0x668);
  reg686 = hal_spi_read_reg(phy, 0x686);
  reg82a = hal_spi_read_reg(phy, 0x82a);
  reg82f = hal_spi_read_reg(phy, 0x82f);
  reg834 = hal_spi_read_reg(phy, 0x834);

  //Set 901, bypass VIOLDO
  hal_spi_write_reg(phy, 0x901, 0x5e);
  // Open all Erramp
  // hal_spi_write_reg(phy, 0x802, 0xbf);
  // hal_spi_write_reg(phy, 0x803, 0x0f);
  // hal_spi_write_reg(phy, 0x804, 0x1f);

  testmux_clear(phy);

  //RXPDET_TEST AUXADC TEST CLEAR
  hal_spi_write_reg(phy, 0x668, 0x02);
  hal_spi_write_reg(phy, 0x686, 0x02);

  //LDO_SYSPLLDC
  hal_spi_write_reg(phy, 0x82a, 0x10);
  hal_spi_write_reg(phy, 0x82f, 0x08);

  //DC_TEST_SEL
  hal_spi_write_reg(phy, 0x834, 0x20);
  hal_spi_write_reg(phy, 0x82a, 0x14);

  reg_val = test_mt_auxadc(phy, MT1P_AUXADC, 1, 0, 1);
  auxadccal_transfer(phy, &reg_val);

  auxadcctrl_recover(phy, MT1P_AUXADC, &ctrl_bak);

  hal_spi_write_reg(phy, 0x901, reg901);
  hal_spi_write_reg(phy, 0x668, reg668);
  hal_spi_write_reg(phy, 0x686, reg686);
  hal_spi_write_reg(phy, 0x82a, reg82a);
  hal_spi_write_reg(phy, 0x82f, reg82f);
  hal_spi_write_reg(phy, 0x834, reg834);

  return reg_val;
}
