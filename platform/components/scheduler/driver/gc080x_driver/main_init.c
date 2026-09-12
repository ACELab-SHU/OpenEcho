#include "main_init.h"
#include "custom_cfg.h"
#include "driver.h"
#include "limit_efuse.h"
#include "platform.h"
#include "transceiver_api.h"

void band_dep_calibr_phase_of_bandwidth_calflag_clr(rf_chip_phy_t* phy) {
  if ((phy->txlo != phy->config->tx_flo) ||
      (phy->rxlo != phy->config->rx_flo) ||
      (phy->bandwith != phy->config->bandwith)) {
    phy->tx_qec_flag[0]    = 0;
    phy->tx_qec_flag[1]    = 0;
    phy->rx_qec_flag[0]    = 0;
    phy->rx_qec_flag[1]    = 0;
    phy->rx_bw_cal_flag[0] = 0;
    phy->rx_bw_cal_flag[1] = 0;
  }
}

void band_dep_calibr_phase_of_bandwidth_calflag_set(rf_chip_phy_t* phy) {
  switch (phy->config->mode) {
    case CH1_FDD:
    case CH1_TDD:
      phy->tx_qec_flag[TRX_CHN1]    = 1;
      phy->rx_qec_flag[TRX_CHN1]    = 1;
      phy->rx_bw_cal_flag[TRX_CHN1] = 1;
      break;

    case RX1_TX2_FDD:
      phy->tx_qec_flag[TRX_CHN2]    = 1;
      phy->rx_qec_flag[TRX_CHN1]    = 1;
      phy->rx_bw_cal_flag[TRX_CHN1] = 1;
      break;

    case RX2_TX1_FDD:
      phy->tx_qec_flag[TRX_CHN1]    = 1;
      phy->rx_qec_flag[TRX_CHN2]    = 1;
      phy->rx_bw_cal_flag[TRX_CHN2] = 1;
      break;

    case CH2_FDD:
    case CH2_TDD:
      phy->tx_qec_flag[TRX_CHN2]    = 1;
      phy->rx_qec_flag[TRX_CHN2]    = 1;
      phy->rx_bw_cal_flag[TRX_CHN2] = 1;
      break;

    case CH1_CH2_FDD:
    case CH1_CH2_TDD:
      phy->tx_qec_flag[TRX_CHN1]    = 1;
      phy->tx_qec_flag[TRX_CHN2]    = 1;
      phy->rx_qec_flag[TRX_CHN1]    = 1;
      phy->rx_qec_flag[TRX_CHN2]    = 1;
      phy->rx_bw_cal_flag[TRX_CHN1] = 1;
      phy->rx_bw_cal_flag[TRX_CHN2] = 1;
      break;

    default:
      LOG_ERROR("%s in line %d REPORTED ERR for %d mode not support\n",
                __FUNCTION__, __LINE__, phy->config->mode);
      break;
  }
}

static int band_dep_calibr_phase_of_Rx_ImBalance_cal(rf_chip_phy_t* phy) {
  int ret = 0;
  DEFINE_TIME();
  if (phy->config->Rx_ImBalance_cal_flag) {
    START_TIME();
    switch (phy->config->mode) {
      case CH1_FDD:
      case CH1_TDD:
        ret = inbalance_cal(phy, TRX_CHN1);
        break;

      case RX1_TX2_FDD:

        ret = inbalance_cal(phy, TRX_CHN1);
        break;

      case RX2_TX1_FDD:
        ret = inbalance_cal(phy, TRX_CHN2);
        break;

      case CH2_FDD:
      case CH2_TDD:
        ret = inbalance_cal(phy, TRX_CHN2);
        break;

      case CH1_CH2_FDD:
      case CH1_CH2_TDD:
        ret = inbalance_cal(phy, TRX_CHN1);
        ret |= inbalance_cal(phy, TRX_CHN2);
        break;

      default:
        break;
    }

    if (ret < 0) {
      LOG_ERROR("Rx_ImBalance_cal failed!\n");
      return ret;
    }
    END_TIME();
    PRINT_TIME("Rx_ImBalance_cal");
  }
  return ret;
}

static int band_dep_calibr_phase_of_lo_leakage_cal(rf_chip_phy_t* phy) {
  int ret = 0;
  DEFINE_TIME();
  if (phy->config->lo_leakage_cal_flag) {
    START_TIME();
    switch (phy->config->mode) {
      case CH1_FDD:
      case CH1_TDD:
        ret = lo_leakage_cal(phy, TRX_CHN1);
        break;

      case RX1_TX2_FDD:

        ret = lo_leakage_cal(phy, TRX_CHN1);
        break;

      case RX2_TX1_FDD:
        ret = lo_leakage_cal(phy, TRX_CHN2);
        break;

      case CH2_FDD:
      case CH2_TDD:
        ret = lo_leakage_cal(phy, TRX_CHN2);
        break;

      case CH1_CH2_FDD:
      case CH1_CH2_TDD:
        ret = lo_leakage_cal(phy, TRX_CHN1);
        ret |= lo_leakage_cal(phy, TRX_CHN2);
        break;

      default:
        break;
    }

    if (ret < 0) {
      LOG_ERROR("lo_leakage_cal failed!\n");
      return ret;
    }
    END_TIME();
    PRINT_TIME("lo_leakage_cal");
  }
  return ret;
}

static int band_dep_calibr_phase_of_switch_bandwidth(rf_chip_phy_t* phy) {
  int ret = 0;
  DEFINE_TIME();

  LOG_MAIN(" %s in line %d, Cur Bandwidth is %d\n", __FUNCTION__, __LINE__, phy->config->bandwith);
  trx_bw_lut_load(phy, phy->config->bandwith, phy->config->custom_bandwith_flag);

  switch (phy->config->mode) {
    case CH1_FDD:
    case CH1_TDD:
      if (phy->config->rx_bw_cal_flag) {
        START_TIME();
        set_rx_port_man(phy, TRX_CHN1, phy->config->rx_port[0], RX_PORT_G0);
        phy->config->rxfe_gain[0] = RX_PORT_G0;
        set_rf_bandwidth(phy, phy->config->bandwith);
        ret = rx_bw_cal(phy, TRX_CHN1);
        if ((ret < 0) || (phy->error < 0)) {
          LOG_ERROR("rx bw cal failed!\n");
          return -1;
        }
        END_TIME();
        PRINT_TIME("rx_bw_cal");
      }

      // printf("ruanbo: 2nd if\n");
      // printf("ruanbo: phy->config->rx_dc_cal_flag: %d\n\r",phy->config->rx_dc_cal_flag);
      if (phy->config->tx_dc_cal_flag) {
        START_TIME();
        tx_dc_offset_cal(phy, TRX_CHN1);
        END_TIME();
        PRINT_TIME("tx_dc_offset_cal");
      }
      // printf("ruanbo: 3rd if\n");
      // printf("ruanbo: phy->config->rx_dc_cal_flag: %d\n\r",phy->config->rx_dc_cal_flag);
      if (phy->config->rx_dc_cal_flag) {
        START_TIME();
        ret = rx_dc_offset_cal(phy, TRX_CHN1);
        //ret = rx_dc_offset_cal_v2(phy, TRX_CHN1);
        if (ret < 0) {
          LOG_ERROR("rx dc offset cal failed!\n");
          return ret;
        }
        END_TIME();
        PRINT_TIME("rx_dc_offset_cal");
      }

      if (phy->config->tx_qec_flag) {
        START_TIME();
        //txqec_cal(phy, TRX_CHN1);
        ret = txqec_cal_rflp(phy, TRX_CHN1);
        if (ret < 0) {
          phy->error = TX_QEC_LOL_CAL_FAIL;
          LOG_ERROR("tx qec lol cal fail!\n");
          return -1;
        }
        END_TIME();
        PRINT_TIME("txqec_cal_rflp");
      }

      if (phy->config->rx_qec_flag) {
        START_TIME();
        rxqec_cal(phy, TRX_CHN1);
        END_TIME();
        PRINT_TIME("rxqec_cal");
      }

      break;

    case RX1_TX2_FDD:
      if (phy->config->rx_bw_cal_flag) {
        START_TIME();
        set_rx_port_man(phy, TRX_CHN1, phy->config->rx_port[0], RX_PORT_G0);
        phy->config->rxfe_gain[0] = RX_PORT_G0;
        set_rf_bandwidth(phy, phy->config->bandwith);
        ret = rx_bw_cal(phy, TRX_CHN1);
        if ((ret < 0) || (phy->error < 0)) {
          LOG_ERROR("rx bw cal failed!\n");
          return -1;
        }
        END_TIME();
        PRINT_TIME("rx_bw_cal");
      }

      if (phy->config->tx_dc_cal_flag) {
        START_TIME();
        tx_dc_offset_cal(phy, TRX_CHN2);
        END_TIME();
        PRINT_TIME("tx_dc_offset_cal");
      }
      if (phy->config->rx_dc_cal_flag) {
        START_TIME();
        ret = rx_dc_offset_cal(phy, TRX_CHN1);
        //ret = rx_dc_offset_cal_v2(phy, TRX_CHN1);
        if (ret < 0) {
          LOG_ERROR("rx dc offset cal failed!\n");
          return ret;
        }
        END_TIME();
        PRINT_TIME("rx_dc_offset_cal");
      }

      if (phy->config->tx_qec_flag) {
        START_TIME();
        //txqec_cal(phy, TRX_CHN2);
        ret = txqec_cal_rflp(phy, TRX_CHN2);
        if (ret < 0) {
          phy->error = TX_QEC_LOL_CAL_FAIL;
          LOG_ERROR("tx qec lol cal fail!\n");
          return -1;
        }
        END_TIME();
        PRINT_TIME("txqec_cal_rflp");
      }

      if (phy->config->rx_qec_flag) {
        START_TIME();
        rxqec_cal(phy, TRX_CHN1);
        END_TIME();
        PRINT_TIME("rxqec_cal");
      }

      break;

    case RX2_TX1_FDD:
      if (phy->config->rx_bw_cal_flag) {
        START_TIME();
        set_rx_port_man(phy, TRX_CHN2, phy->config->rx_port[1], RX_PORT_G0);
        phy->config->rxfe_gain[1] = RX_PORT_G0;
        set_rf_bandwidth(phy, phy->config->bandwith);
        ret = rx_bw_cal(phy, TRX_CHN2);
        if ((ret < 0) || (phy->error < 0)) {
          LOG_ERROR("rx bw cal failed!\n");
          return -1;
        }
        END_TIME();
        PRINT_TIME("rx_bw_cal");
      }

      if (phy->config->tx_dc_cal_flag) {
        START_TIME();
        tx_dc_offset_cal(phy, TRX_CHN1);
        END_TIME();
        PRINT_TIME("tx_dc_offset_cal");
      }
      if (phy->config->rx_dc_cal_flag) {
        START_TIME();
        ret = rx_dc_offset_cal(phy, TRX_CHN2);
        //ret = rx_dc_offset_cal_v2(phy, TRX_CHN2);
        if (ret < 0) {
          LOG_ERROR("rx dc offset cal failed!\n");
          return ret;
        }
        END_TIME();
        PRINT_TIME("rx_dc_offset_cal");
      }

      if (phy->config->tx_qec_flag) {
        START_TIME();
        //txqec_cal(phy, TRX_CHN1);
        ret = txqec_cal_rflp(phy, TRX_CHN1);
        if (ret < 0) {
          phy->error = TX_QEC_LOL_CAL_FAIL;
          LOG_ERROR("tx qec lol cal fail!\n");
          return -1;
        }
        END_TIME();
        PRINT_TIME("txqec_cal_rflp");
      }

      if (phy->config->rx_qec_flag) {
        START_TIME();
        rxqec_cal(phy, TRX_CHN2);
        END_TIME();
        PRINT_TIME("rxqec_cal");
      }

      break;

    case CH2_FDD:
    case CH2_TDD:
      if (phy->config->rx_bw_cal_flag) {
        START_TIME();
        set_rx_port_man(phy, TRX_CHN2, phy->config->rx_port[1], RX_PORT_G0);
        phy->config->rxfe_gain[1] = RX_PORT_G0;
        set_rf_bandwidth(phy, phy->config->bandwith);
        ret = rx_bw_cal(phy, TRX_CHN2);
        if ((ret < 0) || (phy->error < 0)) {
          LOG_ERROR("rx bw cal failed!\n");
          return -1;
        }
        END_TIME();
        PRINT_TIME("rx_bw_cal");
      }

      if (phy->config->tx_dc_cal_flag) {
        START_TIME();
        tx_dc_offset_cal(phy, TRX_CHN2);
        END_TIME();
        PRINT_TIME("tx_dc_offset_cal");
      }

      if (phy->config->rx_dc_cal_flag) {
        START_TIME();
        ret = rx_dc_offset_cal(phy, TRX_CHN2);
        //ret = rx_dc_offset_cal_v2(phy, TRX_CHN2);
        if (ret < 0) {
          LOG_ERROR("rx dc offset cal failed!\n");
          return ret;
        }
        END_TIME();
        PRINT_TIME("rx_dc_offset_cal");
      }

      if (phy->config->tx_qec_flag) {
        START_TIME();
        //txqec_cal(phy, TRX_CHN2);
        ret = txqec_cal_rflp(phy, TRX_CHN2);
        if (ret < 0) {
          phy->error = TX_QEC_LOL_CAL_FAIL;
          LOG_ERROR("tx qec lol cal fail!\n");
          return -1;
        }
        END_TIME();
        PRINT_TIME("txqec_cal_rflp");
      }

      if (phy->config->rx_qec_flag) {
        START_TIME();
        rxqec_cal(phy, TRX_CHN2);
        END_TIME();
        PRINT_TIME("rxqec_cal");
      }
      break;

    case CH1_CH2_FDD:
    case CH1_CH2_TDD:
      if (phy->config->rx_bw_cal_flag) {
        START_TIME();
        set_rx_port_man(phy, TRX_CHN1, phy->config->rx_port[0], RX_PORT_G0);
        phy->config->rxfe_gain[0] = RX_PORT_G0;
        set_rf_bandwidth(phy, phy->config->bandwith);
        ret = rx_bw_cal(phy, TRX_CHN1);
        if ((ret < 0) || (phy->error < 0)) {
          LOG_ERROR("rx bw cal failed!\n");
          return -1;
        }
        END_TIME();
        PRINT_TIME("rx_bw_cal");

        START_TIME();
        set_rx_port_man(phy, TRX_CHN2, phy->config->rx_port[1], RX_PORT_G0);
        phy->config->rxfe_gain[1] = RX_PORT_G0;
        set_rf_bandwidth(phy, phy->config->bandwith);
        ret = rx_bw_cal(phy, TRX_CHN2);
        if ((ret < 0) || (phy->error < 0)) {
          LOG_ERROR("rx bw cal failed!\n");
          return -1;
        }
        END_TIME();
        PRINT_TIME("rx_bw_cal");
      }

      if (phy->config->tx_dc_cal_flag) {
        START_TIME();
        tx_dc_offset_cal(phy, TRX_CHN1);
        END_TIME();
        PRINT_TIME("tx_dc_offset_cal");

        START_TIME();
        tx_dc_offset_cal(phy, TRX_CHN2);
        END_TIME();
        PRINT_TIME("tx_dc_offset_cal");
      }

      if (phy->config->rx_dc_cal_flag) {
        START_TIME();
        ret = rx_dc_offset_cal(phy, TRX_CHN1);
        //ret = rx_dc_offset_cal_v2(phy, TRX_CHN1);
        if (ret < 0) {
          LOG_ERROR("rx dc offset cal failed!\n");
          return ret;
        }
        END_TIME();
        PRINT_TIME("rx_dc_offset_cal");

        START_TIME();
        ret = rx_dc_offset_cal(phy, TRX_CHN2);
        //ret = rx_dc_offset_cal_v2(phy, TRX_CHN2);
        if (ret < 0) {
          LOG_ERROR("rx dc offset cal failed!\n");
          return ret;
        }
        END_TIME();
        PRINT_TIME("rx_dc_offset_cal");
      }

      if (phy->config->tx_qec_flag) {
        START_TIME();
        //txqec_cal(phy, TRX_CHN1);
        ret = txqec_cal_rflp(phy, TRX_CHN1);
        if (ret < 0) {
          phy->error = TX_QEC_LOL_CAL_FAIL;
          LOG_ERROR("tx qec lol cal fail!\n");
          return -1;
        }
        END_TIME();
        PRINT_TIME("txqec_cal_rflp");

        START_TIME();
        //txqec_cal(phy, TRX_CHN2);
        ret = txqec_cal_rflp(phy, TRX_CHN2);
        if (ret < 0) {
          phy->error = TX_QEC_LOL_CAL_FAIL;
          LOG_ERROR("tx qec lol cal fail!\n");
          return -1;
        }
        END_TIME();
        PRINT_TIME("txqec_cal_rflp");
      }

      if (phy->config->rx_qec_flag) {
        START_TIME();
        rxqec_cal(phy, TRX_CHN1);
        END_TIME();
        PRINT_TIME("rxqec_cal");

        START_TIME();
        rxqec_cal(phy, TRX_CHN2);
        END_TIME();
        PRINT_TIME("rxqec_cal");
      }
      break;

    default:
      break;
  }
  return ret;
}

static int base_init(rf_chip_phy_t* phy) {
  unsigned char lut_ver[5];
  int ret = 0;
  DEFINE_TIME();

  phy->error     = ret;
  phy->st        = AT_FSM_SLEEP;
  phy->init_flag = 0;

  LOG_MAIN("phy version: %s\n", DEVICE_VERSION);
  START_TIME();
  if (phy->config->custom_bandwith_flag)
    proximity_bandwith_seek(phy, phy->config->custom_bandwith);
  END_TIME();
  PRINT_TIME("proximity_bandwith_seek");

  START_TIME();
  ret = power_init(phy);
  if (ret < 0) {
    LOG_ERROR("spi error!\n");
    goto base_error;
  }
  END_TIME();
  PRINT_TIME("power_init");

  START_TIME();
  sxtrx_cfg_switch(phy);
  END_TIME();
  PRINT_TIME("sxtrx_cfg_switch");

  START_TIME();
  if (phy->config->r_cal_flag) {
    rcal(phy);
  }
  END_TIME();
  PRINT_TIME("rcal");

  START_TIME();
  ret = config_syspll(phy, phy->config->xtal_freq, phy->config->sys_fvco);
  if (ret < 0) {
    LOG_ERROR("config_syspll: invalid parameter!\n");
    goto base_error;
  }
  END_TIME();
  PRINT_TIME("config_syspll");

  if (!phy->config->Rx_ImBalance_cal_flag) {
    START_TIME();
    hal_spi_write_reg(phy, 0xD32, phy->config->imb_rx_cfg[0][0]);
    hal_spi_write_reg(phy, 0xD36, phy->config->imb_rx_cfg[0][1]);
    hal_spi_write_reg(phy, 0xD31, phy->config->imb_rx_cfg[1][0]);
    hal_spi_write_reg(phy, 0xD37, phy->config->imb_rx_cfg[1][1]);
    END_TIME();
    PRINT_TIME("rx imb setting");
  }

  if (phy->config->auxadc1_cal_flag) {
    START_TIME();
    auxadc_cal(phy);

    END_TIME();
    PRINT_TIME("auxadc1_cal");
  }

  if (phy->config->ldo_cal_flag) {
    START_TIME();
    ldo_cal(phy);
    END_TIME();
    PRINT_TIME("ldo_cal");
  }

  START_TIME();
  ret = analog_init(phy);
  if (ret < 0) {
    LOG_ERROR("spi error!\n");
    goto base_error;
  }
  END_TIME();
  PRINT_TIME("analog_init");

  START_TIME();
  phy->loading_lut = 1;
  ret              = load_lut(phy, phy->config->mode);
  if (ret < 0) {
    LOG_ERROR("load lut failed!\n");
    phy->loading_lut = 0;
    goto base_error;
  }
  phy->loading_lut = 0;
  END_TIME();
  PRINT_TIME("load_lut");

  START_TIME();
  ret = lut_version_get(phy, lut_ver);
  if (ret < 0) {
    LOG_ERROR("lut version error!\n");
    goto base_error;
  }
  END_TIME();
  PRINT_TIME("lut_version_get");
  LOG_MAIN("[0x%02x 0x%02x 0x%02x] [0x%02x 0x%02x]\n", lut_ver[0], lut_ver[1], lut_ver[2], lut_ver[3], lut_ver[4]);

  LOG_MAIN("Enable Filter used Fir.\n");
  dig_trx_filter_use_fir(phy);

  START_TIME();
  ret = digital_init(phy, phy->config->mode,
                     phy->config->bandwith,
                     phy->config->dig_if,
                     phy->config->p0p1_port,
                     phy->config->data_rate,
                     0);
  if (ret < 0) {
    LOG_ERROR("digital_init0: invalid parameter!\n");
    goto base_error;
  }
  END_TIME();
  PRINT_TIME("digital_init0");

  START_TIME();
  trx_lut_load(phy, phy->config->tx_flo, phy->config->bandwith, phy->config->custom_bandwith_flag);
  END_TIME();
  PRINT_TIME("trx_lut_load");

base_error:
  return ret;
}

static int band_indep_calibr(rf_chip_phy_t* phy) {
  int ret = 0;
  DEFINE_TIME();

  phy->error = ret;
  phy->st    = AT_FSM_SLEEP;

  if (phy->config->txlo_cal_flag) {
    START_TIME();
    txlo_cal_lut_update(phy);

    END_TIME();
    PRINT_TIME("txlo_cal_lut_update");
  }

  switch (phy->config->mode) {
    case CH1_FDD:
    case CH1_TDD:
      if (phy->config->tx_dac_cal_flag) {
        START_TIME();
        tx_dac_cal(phy, TRX_CHN1);
        END_TIME();
        PRINT_TIME("tx_dac_cal");
      }

      if (phy->config->rx_adc_cal_flag) {
        START_TIME();
        rx_adc_cal(phy, TRX_CHN1);
        END_TIME();
        PRINT_TIME("rx_adc_cal");
      }
      break;

    case RX1_TX2_FDD:
      if (phy->config->tx_dac_cal_flag) {
        START_TIME();
        tx_dac_cal(phy, TRX_CHN2);
        END_TIME();
        PRINT_TIME("tx_dac_cal");
      }

      if (phy->config->rx_adc_cal_flag) {
        START_TIME();
        rx_adc_cal(phy, TRX_CHN1);
        END_TIME();
        PRINT_TIME("rx_adc_cal");
      }

      break;

    case RX2_TX1_FDD:
      if (phy->config->tx_dac_cal_flag) {
        START_TIME();
        tx_dac_cal(phy, TRX_CHN1);
        END_TIME();
        PRINT_TIME("tx_dac_cal");
      }

      if (phy->config->rx_adc_cal_flag) {
        START_TIME();
        rx_adc_cal(phy, TRX_CHN2);
        END_TIME();
        PRINT_TIME("rx_adc_cal");
      }

      break;

    case CH2_FDD:
    case CH2_TDD:
      if (phy->config->tx_dac_cal_flag) {
        START_TIME();
        tx_dac_cal(phy, TRX_CHN2);
        END_TIME();
        PRINT_TIME("tx_dac_cal");
      }

      if (phy->config->rx_adc_cal_flag) {
        START_TIME();
        rx_adc_cal(phy, TRX_CHN2);
        END_TIME();
        PRINT_TIME("rx_adc_cal");
      }

      break;

    case CH1_CH2_FDD:
    case CH1_CH2_TDD:
      if (phy->config->tx_dac_cal_flag) {
        START_TIME();
        tx_dac_cal(phy, TRX_CHN1);
        END_TIME();
        PRINT_TIME("tx_dac_cal");

        START_TIME();
        tx_dac_cal(phy, TRX_CHN2);
        END_TIME();
        PRINT_TIME("tx_dac_cal");
      }

      if (phy->config->rx_adc_cal_flag) {
        START_TIME();
        rx_adc_cal(phy, TRX_CHN1);
        END_TIME();
        PRINT_TIME("rx_adc_cal");

        START_TIME();
        rx_adc_cal(phy, TRX_CHN2);
        END_TIME();
        PRINT_TIME("rx_adc_cal");
      }

      break;

    default:
      break;
  }
  //phy->bandwith = phy->config->bandwith;

  return ret;
}

static int rf_ana_init(rf_chip_phy_t* phy) {
  int ret = 0;
  DEFINE_TIME();

  phy->error = ret;
  phy->st    = AT_FSM_WAIT;

  START_TIME();
  wait_init(phy);
  END_TIME();
  PRINT_TIME("wait_init");

  START_TIME();
  ret = digital_init(phy, phy->config->mode,
                     phy->config->bandwith,
                     phy->config->dig_if,
                     phy->config->p0p1_port,
                     phy->config->data_rate,
                     1);
  if (ret < 0) {
    LOG_ERROR("digital_init1: invalid parameter!\n");
    return ret;
  }
  END_TIME();
  PRINT_TIME("digital_init1");

  START_TIME();
  set_rf_bandwidth(phy, phy->config->bandwith);
  switch (phy->config->mode) {
    case CH1_FDD:
    case CH1_TDD:
      set_rx_port(phy, TRX_CHN1, phy->config->rx_port[0], phy->config->rxfe_gain[0], 1);
      set_tx_port(phy, TRX_CHN1, phy->config->tx_port[0], 1);
      break;

    case RX1_TX2_FDD:
      set_rx_port(phy, TRX_CHN1, phy->config->rx_port[0], phy->config->rxfe_gain[0], 1);
      set_tx_port(phy, TRX_CHN2, phy->config->tx_port[1], 1);
      break;

    case RX2_TX1_FDD:
      set_rx_port(phy, TRX_CHN2, phy->config->rx_port[1], phy->config->rxfe_gain[1], 1);
      set_tx_port(phy, TRX_CHN1, phy->config->tx_port[0], 1);
      break;

    case CH2_FDD:
    case CH2_TDD:
      set_rx_port(phy, TRX_CHN2, phy->config->rx_port[1], phy->config->rxfe_gain[1], 1);
      set_tx_port(phy, TRX_CHN2, phy->config->tx_port[1], 1);
      break;

    case CH1_CH2_FDD:
    case CH1_CH2_TDD:
      set_rx_port(phy, TRX_CHN1, phy->config->rx_port[0], phy->config->rxfe_gain[0], 1);
      set_tx_port(phy, TRX_CHN1, phy->config->tx_port[0], 1);
      set_rx_port(phy, TRX_CHN2, phy->config->rx_port[1], phy->config->rxfe_gain[1], 1);
      set_tx_port(phy, TRX_CHN2, phy->config->tx_port[1], 1);
      break;

    default:
      break;
  }
  END_TIME();
  PRINT_TIME("rf bw&port");

  START_TIME();
  if (phy->config->sx_cal_flag)
    sx_cal_lut_update(phy);
  END_TIME();
  PRINT_TIME("sx_cal_lut_update");

  return ret;
}
int bybrid_mode_cal_switch(rf_chip_phy_t* phy) {
  unsigned char mode = phy->config->mode;
  switch (mode) {
    case CH1_CH2_FDD:
      phy->config->mode = CH1_CH2_TDD;
      break;
    case CH1_CH2_TDD:
      phy->config->mode = CH1_CH2_FDD;
      break;
    case CH1_FDD:
      phy->config->mode = CH1_TDD;
      break;
    case CH2_FDD:
      phy->config->mode = CH2_TDD;
      break;
    default:
      LOG_ERROR("ERROR: hybrid mode only support CH1_CH2_FDD ,CH1_CH2_TDD,CH1_TDD,CH2_TDD !\n");
      return -1;
      break;
  }
  return 0;
}
static int band_dep_calibr(rf_chip_phy_t* phy) {
  int i   = 0;
  int ret = 0, tdd_sel = 1;
  unsigned char mode_bak;
  BANDWITH_ENUM bandwith_bak;
  DEFINE_TIME();

  phy->error = ret;
  phy->st    = AT_FSM_WAIT;

  if (phy->config->use_bybrid_mode) {
    mode_bak = phy->config->mode;
    ret      = bybrid_mode_cal_switch(phy);
    if (ret < 0) {
      LOG_ERROR("ERROR: hybrid mode switch failed!\n");
      return ret;
    }
  }

  START_TIME();
  if ((phy->config->mode >= CH1_FDD) && (phy->config->mode <= CH1_CH2_FDD)) {
    ret = sxrx_band_fsm(phy, phy->config->rx_flo);
    if (ret < 0) {
      LOG_ERROR("sxrx_band_fsm: invalid parameter!\n");
      return ret;
    }
    tdd_sel = 0;
  }
  END_TIME();
  PRINT_TIME("sxrx_band_fsm");

  START_TIME();
  ret = sxtx_band_fsm(phy, phy->config->tx_flo, phy->config->core2_enable);
  if (ret < 0) {
    LOG_ERROR("sxtx_band_fsm: invalid parameter!\n");
    return ret;
  }
  END_TIME();
  PRINT_TIME("sxtx_band_fsm");

  band_dep_calibr_phase_of_bandwidth_calflag_clr(phy);

  //to do;
  band_dep_calibr_phase_of_Rx_ImBalance_cal(phy);
  band_dep_calibr_phase_of_lo_leakage_cal(phy);

  if ((phy->config->bandwidthswitch_flag == 1) && (phy->config->bandwidthswitch_length >= 2)) {
    phy->config->pBandwidthloop[phy->config->bandwidthswitch_length - 1] = phy->config->bandwith;

    LOG_MAIN("%s in line %d total %d loops, and the last one is %d\n",
             __FUNCTION__, __LINE__, phy->config->bandwidthswitch_length,
             phy->config->pBandwidthloop[phy->config->bandwidthswitch_length - 1]);

    phy->config->custom_bandwith_flag = 0;
    bandwith_bak                      = phy->config->bandwith;
    for (i = 0; i < phy->config->bandwidthswitch_length; i++) {
      phy->config->bandwith = phy->config->pBandwidthloop[i];
      ret |= digital_init(phy, phy->config->mode,
                          phy->config->bandwith,
                          phy->config->dig_if,
                          phy->config->p0p1_port,
                          phy->config->data_rate,
                          4);
      ret |= band_dep_calibr_phase_of_switch_bandwidth(phy);
    }
    phy->config->bandwith = bandwith_bak;
  } else {
    ret = band_dep_calibr_phase_of_switch_bandwidth(phy);
  }
  if (ret < 0) {
    LOG_ERROR("run band_dep_calibr_phase_of_switch_bandwidth fail!\n");
    return ret;
  }
  band_dep_calibr_phase_of_bandwidth_calflag_set(phy);
  misc_init(phy);

  if (phy->config->use_bybrid_mode)
    phy->config->mode = mode_bak;

  phy->txlo     = phy->config->tx_flo;
  phy->rxlo     = phy->config->rx_flo;
  phy->bandwith = phy->config->bandwith;

  //iip2_cal(phy, TRX_CHN1);
  //iip2_cal(phy, TRX_CHN2);

  return ret;
}

static int do_tx_atten_change(rf_chip_phy_t* phy, TRX_CHN_ENUM chn, int val, short immed) {
  short ret = 0;

  if (phy->config->tx_atten_chn_flag) {
    ret = set_tx_atten_chn(phy, chn, phy->config->tx_flo, val, immed);
    if (ret < 0) {
      LOG_ERROR("set_tx_atten: invalid parameter!\n");
      return ret;
    }
    phy->tx_atten[chn] = val;
  } else {
    ret = set_tx_atten(phy, phy->config->tx_flo, val, immed);
    if (ret < 0) {
      LOG_ERROR("set_tx_atten: invalid parameter!\n");
      return ret;
    }
    phy->tx_atten[0]   = val;
    phy->tx_atten[1]   = val;
    phy->tx_curr_index = val;
  }

  return ret;
}

static int do_tx_ana_atten_change(rf_chip_phy_t* phy, TRX_CHN_ENUM chn, int val, short immed) {
  short ret = 0;

  if (phy->config->tx_atten_chn_flag) {
    ret = set_tx_ana_atten_chn(phy, chn, phy->config->tx_flo, val, immed);
    if (ret < 0) {
      LOG_ERROR("set_tx_ana_atten_chn: invalid parameter!\n");
      return ret;
    }
    phy->tx_atten[chn] = val;
  } else {
    ret = set_tx_ana_atten(phy, phy->config->tx_flo, val, immed);
    if (ret < 0) {
      LOG_ERROR("set_tx_ana_atten: invalid parameter!\n");
      return ret;
    }
    phy->tx_atten[0]   = val;
    phy->tx_atten[1]   = val;
    phy->tx_curr_index = val;
  }

  return ret;
}

static int do_tx_split_atten_change(rf_chip_phy_t* phy, TRX_CHN_ENUM chn, TX_SPLIT_ATTEN_ENUM type, unsigned int val, short immed) {
  short ret = 0;

  if (phy->config->tx_atten_chn_flag) {
    ret = tx_split_atten_chn(phy, chn, phy->config->tx_flo, type, val, immed);
    if (ret < 0) {
      LOG_ERROR("tx_split_atten_chn: invalid parameter!\n");
      return ret;
    }
    //phy->tx_atten[chn] = val;
  } else {
    ret = tx_split_atten(phy, phy->config->tx_flo, type, val, immed);
    if (ret < 0) {
      LOG_ERROR("tx_split_atten: invalid parameter!\n");
      return ret;
    }
    //phy->tx_atten[0] = val;
    //phy->tx_atten[1] = val;
    //phy->tx_curr_index = val;
  }

  return ret;
}

static int do_rx_gain_ctrl_set(TRX_CHN_ENUM chn, rf_chip_phy_t* phy) {
  if (phy->config->gain_ctrl_mode == MANUAL_CTRL) {
    if (phy->config->gain_ctrl_pin_flag)
      GCTRL_DEMO_MGC_PIN_MODE(phy, chn, phy->config->gain_table_mode);
    else
      GCTRL_DEMO_MGC_SPI_MODE(phy, chn, phy->config->gain_table_mode);
  } else if (phy->config->gain_ctrl_mode == SLOW_ATTACK) {
    GCTRL_DEMO_AGC_SLOW(phy, chn, phy->config->gain_table_mode);
  } else if (phy->config->gain_ctrl_mode == FAST_ATTACK) {
    GCTRL_DEMO_AGC_FAST_HHH(phy, chn, phy->config->gain_table_mode);
  } else {
    LOG_ERROR("Invalid ctrl_mode!\n");
  }
  return 0;
}

static int fsm_status_prepare(rf_chip_phy_t* phy) {
  int ret = 0;
  unsigned char mode_bak;
  DEFINE_TIME();

  phy->error = ret;
  phy->st    = AT_FSM_WAIT;

  START_TIME();
  fsm_init(phy);
  END_TIME();
  PRINT_TIME("fsm_init");

  START_TIME();
  ret = digital_init(phy, phy->config->mode,
                     phy->config->bandwith,
                     phy->config->dig_if,
                     phy->config->p0p1_port,
                     phy->config->data_rate,
                     2);
  if (ret < 0) {
    LOG_ERROR("digital_init2: invalid parameter!\n");
    return ret;
  }
  END_TIME();
  PRINT_TIME("digital_init2");

  START_TIME();
  switch (phy->config->mode) {
    case CH1_FDD:
    case CH1_TDD:
      do_rx_gain_ctrl_set(TRX_CHN1, phy);
      break;
    case RX2_TX1_FDD:
      do_rx_gain_ctrl_set(TRX_CHN2, phy);
      break;
    case CH2_FDD:
    case CH2_TDD:
      do_rx_gain_ctrl_set(TRX_CHN2, phy);
      break;
    case RX1_TX2_FDD:
      do_rx_gain_ctrl_set(TRX_CHN1, phy);
      break;
    case CH1_CH2_FDD:
    case CH1_CH2_TDD:
      do_rx_gain_ctrl_set(TRX_CHN1, phy);
      do_rx_gain_ctrl_set(TRX_CHN2, phy);
      break;
  }
  END_TIME();
  PRINT_TIME("rx gain ctrl set");

  if (phy->config->use_bybrid_mode) {
    mode_bak = phy->config->mode;
    ret      = bybrid_mode_cal_switch(phy);
    if (ret < 0) {
      LOG_ERROR("ERROR: hybrid mode switch failed!\n");
      return ret;
    }
  }

  START_TIME();
  if ((phy->config->mode >= CH1_FDD) && (phy->config->mode <= CH1_CH2_FDD)) {
    ret = sxrx_band_fsm(phy, phy->config->rx_flo);
    if (ret < 0) {
      LOG_ERROR("sxrx_band_fsm: invalid parameter!\n");
      return ret;
    }
  }
  END_TIME();
  PRINT_TIME("sxrx_band_fsm");

  START_TIME();
  ret = sxtx_band_fsm(phy, phy->config->tx_flo, phy->config->core2_enable);
  if (ret < 0) {
    LOG_ERROR("sxtx_band_fsm: invalid parameter!\n");
    return ret;
  }
  END_TIME();
  PRINT_TIME("sxtx_band_fsm");

  if (phy->config->use_bybrid_mode)
    phy->config->mode = mode_bak;

  START_TIME();
  rxgain_force_valid_config(phy, phy->config->rxgain_force_valid_flag);
  manual_enable(phy, 0);
  END_TIME();
  PRINT_TIME("manual_enable");

  START_TIME();
  set_tx_atten_cfg(phy, phy->config->tx_flo, 0);
  switch (phy->config->mode) {
    case CH1_FDD:
    case CH1_TDD:
      ret = do_tx_atten_change(phy, TRX_CHN1, phy->tx_atten[0], 0);
      break;
    case RX2_TX1_FDD:
      ret = do_tx_atten_change(phy, TRX_CHN1, phy->tx_atten[0], 0);
      break;
    case CH2_FDD:
    case CH2_TDD:
      ret = do_tx_atten_change(phy, TRX_CHN2, phy->tx_atten[0], 0);
      break;
    case RX1_TX2_FDD:
      ret = do_tx_atten_change(phy, TRX_CHN2, phy->tx_atten[1], 0);
      break;
    case CH1_CH2_FDD:
    case CH1_CH2_TDD:
      ret = do_tx_atten_change(phy, TRX_CHN1, phy->tx_atten[0], 0);
      ret = do_tx_atten_change(phy, TRX_CHN2, phy->tx_atten[1], 0);
      break;
  }
  END_TIME();
  PRINT_TIME("tx_atten");

  return ret;
}

static int fsm_status_enter(rf_chip_phy_t* phy) {
  int ret = 0;
  FSM_ST_ENUM st;
  DEFINE_TIME();

  START_TIME();
  phy->error = ret;
  //misc_init(phy);

#if 0
	if(phy->config->fir_filter_flag)
	{
		switch(phy->config->mode)
		{
			case CH1_FDD:
			case CH1_TDD:
				DIG_FIR_FILTER_CFG(phy, manual_on, RX_DIR, TRX_CHN1, phy->config->bandwith, FIR_FILTER_ORDER_NUM, g_fir_tap_coeff_rx1_fill_in_array, write_lut);
				DIG_FIR_FILTER_CFG(phy, manual_on, TX_DIR, TRX_CHN1, phy->config->bandwith, FIR_FILTER_ORDER_NUM, g_fir_tap_coeff_tx1_fill_in_array, write_lut);
				break;
			case CH2_FDD:
			case CH2_TDD:
				DIG_FIR_FILTER_CFG(phy, manual_on, RX_DIR, TRX_CHN2, phy->config->bandwith, FIR_FILTER_ORDER_NUM, g_fir_tap_coeff_rx2_fill_in_array, write_lut);
				DIG_FIR_FILTER_CFG(phy, manual_on, TX_DIR, TRX_CHN2, phy->config->bandwith, FIR_FILTER_ORDER_NUM, g_fir_tap_coeff_tx2_fill_in_array, write_lut);
				break;

			case RX1_TX2_FDD:
				DIG_FIR_FILTER_CFG(phy, manual_on, RX_DIR, TRX_CHN1, phy->config->bandwith, FIR_FILTER_ORDER_NUM, g_fir_tap_coeff_rx1_fill_in_array, write_lut);
				DIG_FIR_FILTER_CFG(phy, manual_on, TX_DIR, TRX_CHN2, phy->config->bandwith, FIR_FILTER_ORDER_NUM, g_fir_tap_coeff_tx2_fill_in_array, write_lut);

				break;
			case RX2_TX1_FDD:
				DIG_FIR_FILTER_CFG(phy, manual_on, RX_DIR, TRX_CHN2, phy->config->bandwith, FIR_FILTER_ORDER_NUM, g_fir_tap_coeff_rx2_fill_in_array, write_lut);
				DIG_FIR_FILTER_CFG(phy, manual_on, TX_DIR, TRX_CHN1, phy->config->bandwith, FIR_FILTER_ORDER_NUM, g_fir_tap_coeff_tx1_fill_in_array, write_lut);

				break;
			case CH1_CH2_FDD:
			case CH1_CH2_TDD:
				DIG_FIR_FILTER_CFG(phy, manual_on, RX_DIR, TRX_CHN1, phy->config->bandwith, FIR_FILTER_ORDER_NUM, g_fir_tap_coeff_rx1_fill_in_array, write_lut);
				DIG_FIR_FILTER_CFG(phy, manual_on, TX_DIR, TRX_CHN1, phy->config->bandwith, FIR_FILTER_ORDER_NUM, g_fir_tap_coeff_tx1_fill_in_array, write_lut);

				DIG_FIR_FILTER_CFG(phy, manual_on, RX_DIR, TRX_CHN2, phy->config->bandwith, FIR_FILTER_ORDER_NUM, g_fir_tap_coeff_rx2_fill_in_array, write_lut);
				DIG_FIR_FILTER_CFG(phy, manual_on, TX_DIR, TRX_CHN2, phy->config->bandwith, FIR_FILTER_ORDER_NUM, g_fir_tap_coeff_tx2_fill_in_array, write_lut);
				break;
		}
	}
#endif

  DIG_FIR_FILTER_CFG_WITH_MANUAL(phy, 0);

  if ((phy->config->mode >= CH1_FDD) && (phy->config->mode <= CH1_CH2_FDD)) {
    fdd_wait_to_alert(phy);
    st = AT_FSM_ALERT;
    if (fsm_status_get(phy, st) < 0) {
      ret = -1;
      goto fsm_error;
    }

    CHIP_DELAY(100);
    fdd_alert_to_fsm(phy);
    st = AT_FSM_FDD;
    if (fsm_status_get(phy, st) < 0) {
      ret = -1;
      goto fsm_error;
    }
  } else {
    wire_control_en(phy, phy->config->wire_control_en, phy->config->wire_ctrl);
    if (!phy->config->wire_control_en) {
      hal_spi_write_reg(phy, 0xcc, 0x00);
      st = AT_FSM_WAIT;
      if (fsm_status_get(phy, st) < 0) {
        ret = -1;
        goto fsm_error;
      }

      hal_spi_write_reg(phy, 0xcc, 0x40);
      st = AT_FSM_ALERT;
      if (fsm_status_get(phy, st) < 0) {
        ret = -1;
        goto fsm_error;
      }

      CHIP_DELAY(100);
      hal_spi_write_reg(phy, 0xcc, 0x04);
      st = AT_FSM_RX;
      if (fsm_status_get(phy, st) < 0) {
        ret = -1;
        goto fsm_error;
      }
    } else {
      tdd_wait_to_alert(phy);
      st = AT_FSM_ALERT;
      if (fsm_status_get(phy, st) < 0) {
        ret = -1;
        goto fsm_error;
      }

      CHIP_DELAY(100);
      tdd_alert_to_rx(phy);
      st = AT_FSM_RX;
      if (fsm_status_get(phy, st) < 0) {
        ret = -1;
        goto fsm_error;
      }
    }
  }

  if ((phy->config->gain_ctrl_mode == MANUAL_CTRL) && (phy->config->gain_table_mode == SPLIT_TBL) &&
      (phy->config->gain_ctrl_pin_flag == 0)) {
    if (phy->init_flag) {
      switch (phy->config->mode) {
        case CH1_FDD:
        case CH1_TDD:
        case RX1_TX2_FDD:
          rx_mgc_gain(phy, TRX_CHN1, LMT_G, phy->rx_lmt_gain[0]);
          rx_mgc_gain(phy, TRX_CHN1, LPF_G, phy->rx_lpf_gain[0]);
          rx_mgc_gain(phy, TRX_CHN1, DIG_G, phy->rx_dig_gain[0]);
          break;

        case RX2_TX1_FDD:
        case CH2_FDD:
        case CH2_TDD:
          rx_mgc_gain(phy, TRX_CHN2, LMT_G, phy->rx_lmt_gain[1]);
          rx_mgc_gain(phy, TRX_CHN2, LPF_G, phy->rx_lpf_gain[1]);
          rx_mgc_gain(phy, TRX_CHN2, DIG_G, phy->rx_dig_gain[1]);
          break;

        case CH1_CH2_FDD:
        case CH1_CH2_TDD:
          rx_mgc_gain(phy, TRX_CHN1, LMT_G, phy->rx_lmt_gain[0]);
          rx_mgc_gain(phy, TRX_CHN1, LPF_G, phy->rx_lpf_gain[0]);
          rx_mgc_gain(phy, TRX_CHN1, DIG_G, phy->rx_dig_gain[0]);
          rx_mgc_gain(phy, TRX_CHN2, LMT_G, phy->rx_lmt_gain[1]);
          rx_mgc_gain(phy, TRX_CHN2, LPF_G, phy->rx_lpf_gain[1]);
          rx_mgc_gain(phy, TRX_CHN2, DIG_G, phy->rx_dig_gain[1]);
          break;

        default:
          break;
      }
      //fn_tx_send_tone(TRX_CHN1, 1, 2);
    } else {
      switch (phy->config->mode) {
        case CH1_FDD:
        case CH1_TDD:
        case RX1_TX2_FDD:
          rx_mgc_max_gain(phy, TRX_CHN1);
          rx_gain_mgc_change(phy, TRX_CHN1, LMT_G, 5);
          rx_gain_mgc_change(phy, TRX_CHN1, LPF_G, 12);
          rx_gain_mgc_change(phy, TRX_CHN1, DIG_G, 0);
          break;

        case RX2_TX1_FDD:
        case CH2_FDD:
        case CH2_TDD:
          rx_mgc_max_gain(phy, TRX_CHN2);
          rx_gain_mgc_change(phy, TRX_CHN2, LMT_G, 5);
          rx_gain_mgc_change(phy, TRX_CHN2, LPF_G, 12);
          rx_gain_mgc_change(phy, TRX_CHN2, DIG_G, 0);
          break;

        case CH1_CH2_FDD:
        case CH1_CH2_TDD:
          rx_mgc_max_gain(phy, TRX_CHN1);
          rx_mgc_max_gain(phy, TRX_CHN2);
          rx_gain_mgc_change(phy, TRX_CHN1, LMT_G, 5);
          rx_gain_mgc_change(phy, TRX_CHN1, LPF_G, 12);
          rx_gain_mgc_change(phy, TRX_CHN1, DIG_G, 0);
          rx_gain_mgc_change(phy, TRX_CHN2, LMT_G, 5);
          rx_gain_mgc_change(phy, TRX_CHN2, LPF_G, 12);
          rx_gain_mgc_change(phy, TRX_CHN2, DIG_G, 0);
          break;

        default:
          break;
      }
      //fn_tx_send_tone(TRX_CHN1, 1, 2);
    }
  }
  fpga_tail_set(phy, 0, 0, 0);
  END_TIME();
  PRINT_TIME("rx gain");

#if 0
	switch (g_phy_obj[g_phy_select].config->mode)
	{
	case CH1_FDD:
	case CH1_TDD:
	case RX2_TX1_FDD:
		txdc_digtial_remove(TRX_CHN1);
		break;
	case CH2_FDD:
	case CH2_TDD:
	case RX1_TX2_FDD:
		txdc_digtial_remove(TRX_CHN2);
		break;
	case CH1_CH2_FDD:
	case CH1_CH2_TDD:
		txdc_digtial_remove(TRX_CHN1);
		txdc_digtial_remove(TRX_CHN2);
		break;
	}
#endif

  phy->init_flag = 1;

fsm_error:
  if (ret < 0)
    LOG_ERROR("enter fsm error!(in phy current status=%d, target status=%d)\n", phy->st, st);

  return ret;
}

int detect_chip(rf_chip_phy_t* phy) {
  int rslt    = 0;
  int cnt_try = 10; /* only try 10 times. */

  do {
    /*
         * This interface should be modified to align with the specific reset control requirements of the customer.
         * On the customer's motherboard, it is necessary to generate a 1ms low-level pulse to the gc0801 reset pin.
         * Direct usage of the original chip_reset_withus function is not recommended.
         */
    chip_reset_withus(phy, 1000);

    if (TRANSV_OK != cm_init(phy)) {
      LOG_ERROR("%s in line %d REPORT ERR: FAIL to recall the CM module after reset\n",
                __FUNCTION__, __LINE__);
      continue;
    }

    if (TRANSV_OK == transceiver_checkdevice(phy)) {
      break;
    }

  } while (--cnt_try > 0);

  if (cnt_try <= 0) {
    phy->error = -SPI_RW_ERR;
    LOG_ERROR("%s in line %d REPORT ERR: FAIL to detect transceiver device in spi bus\n",
              __FUNCTION__, __LINE__);
    return TRANSV_E_COM_FAIL;
  }

  LOG_MAIN("%s in line %d Success to detect transceiver device in spi bus\n",
           __FUNCTION__, __LINE__);

  return TRANSV_OK;
}

int main_init(rf_chip_phy_t* phy) {
  //    module_debug_onoff(phy, RX_DC_OFFSET_CAL | TX_DC_CAL | TRX_QEC_CAL | TX_DAC_CAL | RX_ADC_CAL | SX_TRX_CAL);

  if (detect_chip(phy) < 0)
    return -1;

  if (base_init(phy) < 0)
    return -1;

  if (band_indep_calibr(phy) < 0)
    return -1;

  if (rf_ana_init(phy) < 0)
    return -1;

  if (band_dep_calibr(phy) < 0)
    return -1;

  if (fsm_status_prepare(phy) < 0)
    return -1;

  if (fsm_status_enter(phy) < 0)
    return -1;

  return 0;
}

short fsm_status_get_only(rf_chip_phy_t* phy) {
  unsigned short val;

  val = hal_spi_read_reg(phy, 0xda);
  switch (val) {
    case 0:
      phy->st = AT_FSM_WAIT;
      break;
    case 1:
      phy->st = AT_FSM_ALERT;
      break;
    case 2:
      phy->st = AT_FSM_TX;
      break;
    case 3:
      phy->st = AT_FSM_RX;
      break;
    case 6:
      phy->st = AT_FSM_FDD;
      break;
  }

  return 0;
}

short fsm_status_get(rf_chip_phy_t* phy, short target_st) {
  unsigned short val;
  short try_cnt = 500;

  do {
    try_cnt--;
    val = hal_spi_read_reg(phy, 0xda);
    switch (val) {
      case 0:
        phy->st = AT_FSM_WAIT;
        break;
      case 1:
        phy->st = AT_FSM_ALERT;
        break;
      case 2:
        phy->st = AT_FSM_TX;
        break;
      case 3:
        phy->st = AT_FSM_RX;
        break;
      case 6:
        phy->st = AT_FSM_FDD;
        break;
    }

    if (phy->st == target_st) {
      return 0;
    }
    //LOG_MAIN("CST=%d,TST=%d, CNT=%d\n", phy->st, target_st, try_cnt);
    CHIP_DELAY(5);
  } while (try_cnt > 0);

  return -1;
}

int rf_bandwith_change(rf_chip_phy_t* phy, BANDWITH_ENUM bandwidth) {
  int i = 0;
  DEFINE_TIME();

  START_TIME();
  if ((phy->config->bandwidthswitch_flag != 1) || (phy->config->bandwidthswitch_length <= 1)) {
    LOG_ERROR("%s in line %d REPORTE ERR not enable bandwidth switch feature\n",
              __FUNCTION__, __LINE__);
    return -1;
  }

  /* find target bandwidth in the switch loop. */
  for (i = 0; i < phy->config->bandwidthswitch_length; i++) {
    if (phy->config->pBandwidthloop[i] == bandwidth)
      break;
  }

  if (i >= phy->config->bandwidthswitch_length) {
    LOG_ERROR("%s in line %d REPORTED ERR for target bandwidth index: %d not intial in the loop.\n",
              __FUNCTION__, __LINE__, bandwidth);
    return -1;
  }

  fsm_status_get_only(phy);
  switch (phy->config->mode) {
    case CH1_TDD:
    case CH2_TDD:
    case CH1_CH2_TDD:
    case RX1_TX2_FDD:
    case RX2_TX1_FDD:
      if (phy->st == AT_FSM_RX) {
        hal_spi_write_reg(phy, 0xc0, (bandwidth << 4) | bandwidth);
        trx_bw_lut_load(phy, phy->config->bandwith, phy->config->custom_bandwith_flag);
        if (phy->config->wire_control_en)
          tdd_rx_to_wait(phy);
        else
          hal_spi_write_reg(phy, 0x0cc, 0x00);
        if (fsm_status_get(phy, AT_FSM_WAIT) < 0) {
          LOG_ERROR("%s in line %d REPORTE ERR: RX--->WAIT, cur %d\n", __FUNCTION__, __LINE__, phy->st);
          return -1;
        }
        digital_init(phy, phy->config->mode,
                     bandwidth,
                     phy->config->dig_if,
                     phy->config->p0p1_port,
                     phy->config->data_rate,
                     2);
        if (phy->config->wire_control_en)
          tdd_wait_to_alert(phy);
        else
          hal_spi_write_reg(phy, 0x0cc, 0x40);
        if (fsm_status_get(phy, AT_FSM_ALERT) < 0) {
          LOG_ERROR("%s in line %d REPORTE ERR: WAIT--->ALERT, cur %d\n", __FUNCTION__, __LINE__, phy->st);
          return -1;
        }
        CHIP_DELAY(5); /* delay 5 ms */
        if (phy->config->wire_control_en)
          tdd_alert_to_rx(phy);
        else
          hal_spi_write_reg(phy, 0x0cc, 0x04);
        if (fsm_status_get(phy, AT_FSM_RX) < 0) {
          LOG_ERROR("%s in line %d REPORTE ERR: ALERT--->RX, cur: %d\n", __FUNCTION__, __LINE__, phy->st);
          return -1;
        }
      } else if (phy->st == AT_FSM_TX) {
        hal_spi_write_reg(phy, 0xc0, (bandwidth << 4) | bandwidth);
        trx_bw_lut_load(phy, phy->config->bandwith, phy->config->custom_bandwith_flag);
        if (phy->config->wire_control_en)
          tdd_tx_to_wait(phy);
        else
          hal_spi_write_reg(phy, 0x0cc, 0x00);
        if (fsm_status_get(phy, AT_FSM_WAIT) < 0) {
          LOG_ERROR("%s in line %d REPORTE ERR: TX--->WAIT, cur: %d\n", __FUNCTION__, __LINE__, phy->st);
          return -1;
        }
        digital_init(phy, phy->config->mode,
                     bandwidth,
                     phy->config->dig_if,
                     phy->config->p0p1_port,
                     phy->config->data_rate,
                     2);
        if (phy->config->wire_control_en)
          tdd_wait_to_alert(phy);
        else
          hal_spi_write_reg(phy, 0x0cc, 0x40);
        if (fsm_status_get(phy, AT_FSM_ALERT) < 0) {
          LOG_ERROR("%s in line %d REPORTE ERR: WAIT--->ALERT, cur: %d\n", __FUNCTION__, __LINE__, phy->st);
          return -1;
        }
        CHIP_DELAY(5); /* delay 5 ms */
        if (phy->config->wire_control_en)
          tdd_alert_to_tx(phy);
        else
          hal_spi_write_reg(phy, 0x0cc, 0x02);
        if (fsm_status_get(phy, AT_FSM_TX) < 0) {
          LOG_ERROR("%s in line %d REPORTE ERR: ALERT--->TX, cur: %d\n", __FUNCTION__, __LINE__, phy->st);
          return -1;
        }
      } else if (phy->st == AT_FSM_ALERT) {
        if (phy->config->wire_control_en)
          tdd_alert_to_rx(phy);
        else
          hal_spi_write_reg(phy, 0x0cc, 0x04);
        if (fsm_status_get(phy, AT_FSM_RX) < 0) {
          LOG_ERROR("%s in line %d REPORTE ERR: ALERT--->RX, cur: %d\n", __FUNCTION__, __LINE__, phy->st);
          return -1;
        }

        hal_spi_write_reg(phy, 0xc0, (bandwidth << 4) | bandwidth);
        trx_bw_lut_load(phy, phy->config->bandwith, phy->config->custom_bandwith_flag);
        if (phy->config->wire_control_en)
          tdd_rx_to_wait(phy);
        else
          hal_spi_write_reg(phy, 0x0cc, 0x00);
        if (fsm_status_get(phy, AT_FSM_WAIT) < 0) {
          LOG_ERROR("%s in line %d REPORTE ERR: RX--->WAIT, cur: %d\n", __FUNCTION__, __LINE__, phy->st);
          return -1;
        }
        digital_init(phy, phy->config->mode,
                     bandwidth,
                     phy->config->dig_if,
                     phy->config->p0p1_port,
                     phy->config->data_rate,
                     2);
        if (phy->config->wire_control_en)
          tdd_wait_to_alert(phy);
        else
          hal_spi_write_reg(phy, 0x0cc, 0x40);
        if (fsm_status_get(phy, AT_FSM_ALERT) < 0) {
          LOG_ERROR("%s in line %d REPORTE ERR: WAIT--->ALERT, cur %d\n", __FUNCTION__, __LINE__, phy->st);
          return -1;
        }
        CHIP_DELAY(5); /*delay 5 ms */
        if (phy->config->wire_control_en)
          tdd_alert_to_rx(phy);
        else
          hal_spi_write_reg(phy, 0x0cc, 0x04);
        if (fsm_status_get(phy, AT_FSM_TX) < 0) {
          LOG_ERROR("%s line %d REPORTE ERR: ALERT--->RX, cur %d\n", __FUNCTION__, __LINE__, phy->st);
          return -1;
        }
      } else {
        LOG_ERROR("ERROR: phy->st is not TX or RX\n");
        return -1;
      }
      break;

    case CH1_FDD:
    case CH2_FDD:
    case CH1_CH2_FDD:
      if (phy->st == AT_FSM_FDD) {
        hal_spi_write_reg(phy, 0xc0, (bandwidth << 4) | bandwidth);
        trx_bw_lut_load(phy, phy->config->bandwith, phy->config->custom_bandwith_flag);
        digital_init(phy, phy->config->mode,
                     bandwidth,
                     phy->config->dig_if,
                     phy->config->p0p1_port,
                     phy->config->data_rate,
                     2);
        fdd_fsm_to_alert(phy);
        if (fsm_status_get(phy, AT_FSM_ALERT) < 0) {
          LOG_ERROR("%s in line %d REPORTE ERR: FDD--->ALERT, cur %d\n", __FUNCTION__, __LINE__, phy->st);
          return -1;
        }
        CHIP_DELAY(5); /* delay 5 ms */
        fdd_alert_to_fsm(phy);
        if (fsm_status_get(phy, AT_FSM_FDD) < 0) {
          LOG_ERROR("%s in line %d REPORTE ERR: ALERT--->FDD, cur %d\n", __FUNCTION__, __LINE__, phy->st);
          return -1;
        }
      } else if (phy->st == AT_FSM_ALERT) {
        fdd_alert_to_fsm(phy);
        if (fsm_status_get(phy, AT_FSM_FDD) < 0) {
          LOG_ERROR("%s in line %d REPORTE ERR: ALERT--->FDD, cur %d\n", __FUNCTION__, __LINE__, phy->st);
          return -1;
        }

        hal_spi_write_reg(phy, 0xc0, (bandwidth << 4) | bandwidth);
        trx_bw_lut_load(phy, phy->config->bandwith, phy->config->custom_bandwith_flag);
        digital_init(phy, phy->config->mode,
                     bandwidth,
                     phy->config->dig_if,
                     phy->config->p0p1_port,
                     phy->config->data_rate,
                     2);
        fdd_fsm_to_alert(phy);
        if (fsm_status_get(phy, AT_FSM_ALERT) < 0) {
          LOG_ERROR("%s in line %d REPORTE ERR: FDD--->ALERT, cur %d\n", __FUNCTION__, __LINE__, phy->st);
          return -1;
        }
        CHIP_DELAY(5); /* delay 5 ms */
        fdd_alert_to_fsm(phy);
        if (fsm_status_get(phy, AT_FSM_FDD) < 0) {
          LOG_ERROR("%s in line %d REPORTE ERR: ALERT--->FDD, cur %d\n", __FUNCTION__, __LINE__, phy->st);
          return -1;
        }
      } else {
        LOG_ERROR("%s in line %d REPORTE ERR: phy->st is not TX or RX, cur %d\n", __FUNCTION__, __LINE__, phy->st);
        return -1;
      }
      break;
  }
  END_TIME();
  PRINT_TIME("Change bandwidth cost:");
  /* update bandwith index */
  phy->config->bandwith = bandwidth;
  return 0;
}

int trx_lo_change(rf_chip_phy_t* phy, unsigned long long txlo, unsigned long long rxlo) {
  phy->config->tx_flo = (txlo == 0) ? phy->config->tx_flo : txlo;
  phy->config->rx_flo = (rxlo == 0) ? phy->config->rx_flo : rxlo;

  fsm_status_get_only(phy);
  switch (phy->config->mode) {
    case CH1_TDD:
    case CH2_TDD:
    case CH1_CH2_TDD:
      if (phy->st == AT_FSM_RX) {
        if (phy->config->wire_control_en)
          tdd_rx_to_wait(phy);
        else
          hal_spi_write_reg(phy, 0x0cc, 0x00);
        if (fsm_status_get(phy, AT_FSM_WAIT) < 0) {
          LOG_ERROR("ERROR: RX--->WAIT\n");
          return -1;
        }
      } else if (phy->st == AT_FSM_TX) {
        if (phy->config->wire_control_en)
          tdd_tx_to_wait(phy);
        else
          hal_spi_write_reg(phy, 0x0cc, 0x00);
        if (fsm_status_get(phy, AT_FSM_WAIT) < 0) {
          LOG_ERROR("ERROR: TX--->WAIT\n");
          return -1;
        }
      } else {
        LOG_ERROR("ERROR: phy->st is not TX or RX\n");
        return -1;
      }
      break;
  }

  if (rf_ana_init(phy) < 0)
    return -1;

  if (band_dep_calibr(phy) < 0)
    return -1;

  if (fsm_status_prepare(phy) < 0)
    return -1;

  if (fsm_status_enter(phy) < 0)
    return -1;

  return 0;
}

int trx_lo_change_ext(rf_chip_phy_t* phy, unsigned char lo_change_mode, unsigned long long txlo, unsigned long long rxlo) {
  phy->config->lo_change_mode = lo_change_mode;
  sxtx_band_fsm(phy, txlo, phy->config->core2_enable);
  sxrx_band_fsm(phy, rxlo);
  return 0;
}

int trx_lo_change_act(rf_chip_phy_t* phy) {
  static unsigned char to_alert_flag = 0;

#if 0
	if (to_alert_flag==0)
	{
		hal_spi_write_reg(phy, 0x7c2, 0x90);
		hal_spi_write_reg(phy, 0x7c4, 0x90);
		to_alert_flag = 1;
	}
#endif

  if ((phy->config->mode >= CH1_FDD) && (phy->config->mode <= CH1_CH2_FDD)) {
    hal_spi_write_reg(phy, 0x0CC, 0x68);
    hal_spi_write_reg(phy, 0x0CC, 0x6a);
    hal_spi_write_reg(phy, 0x0CC, 0x68);
    if (fsm_status_get(phy, AT_FSM_ALERT) < 0) {
      LOG_ERROR("ERROR: FSM--->ALERT\n");
      return -1;
    }

    if (phy->config->lo_change_mode == LO_CHANGE_NORMAL)
      CHIP_UDELAY(50);
    hal_spi_write_reg(phy, 0x0CC, 0x6A);
    if (fsm_status_get(phy, AT_FSM_FDD) < 0) {
      LOG_ERROR("ERROR: ALERT--->FSM\n");
      return -1;
    }
  } else {
    if (!phy->config->wire_control_en) {
      hal_spi_write_reg(phy, 0x0CC, 0x60);
      hal_spi_write_reg(phy, 0x0CC, 0x62);
      hal_spi_write_reg(phy, 0x0CC, 0x60);
      if (fsm_status_get(phy, AT_FSM_ALERT) < 0) {
        LOG_ERROR("ERROR: TX--->ALERT\n");
        return -1;
      }

      if (phy->config->lo_change_mode == LO_CHANGE_NORMAL)
        CHIP_UDELAY(50);
      hal_spi_write_reg(phy, 0x0CC, 0x62);
      if (fsm_status_get(phy, AT_FSM_TX) < 0) {
        LOG_ERROR("ERROR: ALERT--->TX\n");
        return -1;
      }
    } else {
      if (phy->config->wire_ctrl == LEVEL_CTRL) {
        tdd_tx_to_alert_level(phy);
        if (fsm_status_get(phy, AT_FSM_ALERT) < 0) {
          LOG_ERROR("ERROR: TX--->ALERT\n");
          return -1;
        }

        tdd_alert_to_tx_level(phy);
        if (fsm_status_get(phy, AT_FSM_TX) < 0) {
          LOG_ERROR("ERROR: RX--->TX\n");
          return -1;
        }
      } else {
        tdd_tx_to_wait(phy);
        if (fsm_status_get(phy, AT_FSM_WAIT) < 0) {
          LOG_ERROR("ERROR: TX--->WAIT\n");
          return -1;
        }

        tdd_wait_to_alert(phy);
        if (fsm_status_get(phy, AT_FSM_ALERT) < 0) {
          LOG_ERROR("ERROR: WAIT--->ALERT\n");
          return -1;
        }

        CHIP_DELAY(100);
        tdd_alert_to_tx(phy);
        if (fsm_status_get(phy, AT_FSM_TX) < 0) {
          LOG_ERROR("ERROR: ALERT--->TX\n");
          return -1;
        }
      }
    }
  }

  return 0;
}

int tx_atten_change(rf_chip_phy_t* phy, TRX_CHN_ENUM chn, int val, short immed) {
  int ret = 0;

  phy->error = ret;
  if (immed)
    return do_tx_atten_change(phy, chn, val, 1);

  if ((phy->config->mode >= CH1_FDD) && (phy->config->mode <= CH1_CH2_FDD)) {
    fdd_fsm_to_alert(phy);
    if (fsm_status_get(phy, AT_FSM_ALERT) < 0)
      return -1;

    CHIP_DELAY(100);
    ret = do_tx_atten_change(phy, chn, val, 0);
    fdd_alert_to_fsm(phy);
    if (fsm_status_get(phy, AT_FSM_FDD) < 0)
      return -1;
  } else {
    if (phy->config->wire_control_en) {
      tdd_tx_to_wait(phy);
      if (fsm_status_get(phy, AT_FSM_WAIT) < 0)
        return -1;

      tdd_wait_to_alert(phy);
      if (fsm_status_get(phy, AT_FSM_ALERT) < 0)
        return -1;

      CHIP_DELAY(100);
      ret = do_tx_atten_change(phy, chn, val, 0);
      tdd_alert_to_tx(phy);
      if (fsm_status_get(phy, AT_FSM_TX) < 0)
        return -1;
    } else {
      hal_spi_write_reg(phy, 0x0cc, 0x00);
      if (fsm_status_get(phy, AT_FSM_WAIT) < 0)
        return -1;

      hal_spi_write_reg(phy, 0x0cc, 0x40);
      if (fsm_status_get(phy, AT_FSM_ALERT) < 0)
        return -1;

      CHIP_DELAY(100);
      ret = do_tx_atten_change(phy, chn, val, 0);
      hal_spi_write_reg(phy, 0x0cc, 0x02);
      if (fsm_status_get(phy, AT_FSM_TX) < 0)
        return -1;
    }
  }

  return ret;
}

int tx_split_atten_change(rf_chip_phy_t* phy, TRX_CHN_ENUM chn, TX_SPLIT_ATTEN_ENUM type, unsigned int val, short immed) {
  int ret = 0;

  phy->error = ret;
  if (immed)
    return do_tx_split_atten_change(phy, chn, type, val, 1);

  if ((phy->config->mode >= CH1_FDD) && (phy->config->mode <= CH1_CH2_FDD)) {
    fdd_fsm_to_alert(phy);
    if (fsm_status_get(phy, AT_FSM_ALERT) < 0)
      return -1;

    CHIP_DELAY(100);
    ret = do_tx_split_atten_change(phy, chn, type, val, 0);
    fdd_alert_to_fsm(phy);
    if (fsm_status_get(phy, AT_FSM_FDD) < 0)
      return -1;
  } else {
    if (phy->config->wire_control_en) {
      tdd_tx_to_wait(phy);
      if (fsm_status_get(phy, AT_FSM_WAIT) < 0)
        return -1;

      tdd_wait_to_alert(phy);
      if (fsm_status_get(phy, AT_FSM_ALERT) < 0)
        return -1;

      CHIP_DELAY(100);
      ret = do_tx_split_atten_change(phy, chn, type, val, 0);
      tdd_alert_to_tx(phy);
      if (fsm_status_get(phy, AT_FSM_TX) < 0)
        return -1;
    } else {
      hal_spi_write_reg(phy, 0x0cc, 0x00);
      if (fsm_status_get(phy, AT_FSM_WAIT) < 0)
        return -1;

      hal_spi_write_reg(phy, 0x0cc, 0x40);
      if (fsm_status_get(phy, AT_FSM_ALERT) < 0)
        return -1;

      CHIP_DELAY(100);
      ret = do_tx_split_atten_change(phy, chn, type, val, 0);
      hal_spi_write_reg(phy, 0x0cc, 0x02);
      if (fsm_status_get(phy, AT_FSM_TX) < 0)
        return -1;
    }
  }

  return ret;
}

int tx_ana_atten_change(rf_chip_phy_t* phy, TRX_CHN_ENUM chn, int val, short immed) {
  int ret = 0;

  phy->error = ret;
  if (immed)
    return do_tx_ana_atten_change(phy, chn, val, 1);

  if ((phy->config->mode >= CH1_FDD) && (phy->config->mode <= CH1_CH2_FDD)) {
    fdd_fsm_to_alert(phy);
    if (fsm_status_get(phy, AT_FSM_ALERT) < 0)
      return -1;

    CHIP_DELAY(100);
    ret = do_tx_ana_atten_change(phy, chn, val, 0);
    fdd_alert_to_fsm(phy);
    if (fsm_status_get(phy, AT_FSM_FDD) < 0)
      return -1;
  } else {
    if (phy->config->wire_control_en) {
      tdd_tx_to_wait(phy);
      if (fsm_status_get(phy, AT_FSM_WAIT) < 0)
        return -1;

      tdd_wait_to_alert(phy);
      if (fsm_status_get(phy, AT_FSM_ALERT) < 0)
        return -1;

      CHIP_DELAY(100);
      ret = do_tx_ana_atten_change(phy, chn, val, 0);
      tdd_alert_to_tx(phy);
      if (fsm_status_get(phy, AT_FSM_TX) < 0)
        return -1;
    } else {
      hal_spi_write_reg(phy, 0x0cc, 0x00);
      if (fsm_status_get(phy, AT_FSM_WAIT) < 0)
        return -1;

      hal_spi_write_reg(phy, 0x0cc, 0x40);
      if (fsm_status_get(phy, AT_FSM_ALERT) < 0)
        return -1;

      CHIP_DELAY(100);
      ret = do_tx_ana_atten_change(phy, chn, val, 0);
      hal_spi_write_reg(phy, 0x0cc, 0x02);
      if (fsm_status_get(phy, AT_FSM_TX) < 0)
        return -1;
    }
  }

  return ret;
}

int trx_fir_coef_set(rf_chip_phy_t* phy, short dir, TRX_CHN_ENUM chn, short fir_tap_n, int* fir_tap_coeff) {
  DIG_FIR_FILTER_CFG(phy, 0, dir, chn, phy->config->bandwith, fir_tap_n, fir_tap_coeff, 1);
  if ((phy->config->mode >= CH1_FDD) && (phy->config->mode <= CH1_CH2_FDD)) {
    fdd_fsm_to_alert(phy);
    if (fsm_status_get(phy, AT_FSM_ALERT) < 0)
      return -1;

    CHIP_DELAY(100);
    fdd_alert_to_fsm(phy);
    if (fsm_status_get(phy, AT_FSM_FDD) < 0)
      return -1;
  } else {
    if (phy->config->wire_control_en) {
      tdd_tx_to_wait(phy);
      if (fsm_status_get(phy, AT_FSM_WAIT) < 0)
        return -1;

      tdd_wait_to_alert(phy);
      if (fsm_status_get(phy, AT_FSM_ALERT) < 0)
        return -1;

      CHIP_DELAY(100);
      tdd_alert_to_tx(phy);
      if (fsm_status_get(phy, AT_FSM_TX) < 0)
        return -1;
    } else {
      hal_spi_write_reg(phy, 0x0cc, 0x00);
      if (fsm_status_get(phy, AT_FSM_WAIT) < 0)
        return -1;

      hal_spi_write_reg(phy, 0x0cc, 0x40);
      if (fsm_status_get(phy, AT_FSM_ALERT) < 0)
        return -1;

      CHIP_DELAY(100);
      hal_spi_write_reg(phy, 0x0cc, 0x02);
      if (fsm_status_get(phy, AT_FSM_TX) < 0)
        return -1;
    }
  }

  return 0;
}

void test_rxgain_ctrl_set(rf_chip_phy_t* phy, TRX_CHN_ENUM chn) {
  do_rx_gain_ctrl_set(chn, phy);
}

void test_tx_fir_coeff_clear(rf_chip_phy_t* phy, TRX_CHN_ENUM chn) {
  int fir_tap_coeff[64];
  my_memset(fir_tap_coeff, 0, sizeof(fir_tap_coeff));
  DIG_FIR_FILTER_CFG(phy, 1, 1, chn, phy->config->bandwith, 64, fir_tap_coeff, 0);
}

void test_adc_dac_off(rf_chip_phy_t* phy, short dir, TRX_CHN_ENUM chn) {
  if (dir) {
    if (chn == TRX_CHN1) {
      hal_spi_write_reg(phy, 0x790, 0x01);
      hal_spi_write_reg(phy, 0x791, 0x00);
      hal_spi_write_reg(phy, 0x737, 0x00);
      hal_spi_write_reg(phy, 0x73A, 0x07);
    } else {
      hal_spi_write_reg(phy, 0x7A3, 0x01);
      hal_spi_write_reg(phy, 0x7A4, 0x00);
      hal_spi_write_reg(phy, 0x77f, 0x00);
      hal_spi_write_reg(phy, 0x782, 0x07);
    }
  } else {
    if (chn == TRX_CHN1) {
      hal_spi_write_reg(phy, 0x698, 0x80);
      hal_spi_write_reg(phy, 0x696, 0x00);
      hal_spi_write_reg(phy, 0x697, 0x00);
    } else {
      hal_spi_write_reg(phy, 0x6b6, 0x80);
      hal_spi_write_reg(phy, 0x6b4, 0x00);
      hal_spi_write_reg(phy, 0x6b5, 0x00);
    }
  }
}

int tdd_trx_change(rf_chip_phy_t* phy, FSM_ST_ENUM st) {
  if (st == AT_FSM_RX) {
    fsm_status_get(phy, AT_FSM_TX);
  } else if (st == AT_FSM_TX) {
    fsm_status_get(phy, AT_FSM_RX);
  } else {
    LOG_ERROR("ERROR: target st is not TX or RX\n");
    return -1;
  }

  if (phy->st == AT_FSM_RX) {
    if (phy->config->wire_control_en)
      tdd_rx_to_wait(phy);
    else
      hal_spi_write_reg(phy, 0x0cc, 0x00);
    if (fsm_status_get(phy, AT_FSM_WAIT) < 0) {
      LOG_ERROR("ERROR: RX--->WAIT\n");
      return -1;
    }
  } else if (phy->st == AT_FSM_TX) {
    if (phy->config->wire_control_en)
      tdd_tx_to_wait(phy);
    else
      hal_spi_write_reg(phy, 0x0cc, 0x00);
    if (fsm_status_get(phy, AT_FSM_WAIT) < 0) {
      LOG_ERROR("ERROR: TX--->WAIT\n");
      return -1;
    }
  } else {
    LOG_ERROR("ERROR: phy->st is not TX or RX\n");
    return -1;
  }

  if (st == AT_FSM_RX) {
    if (phy->config->wire_control_en) {
      tdd_wait_to_alert(phy);
      if (fsm_status_get(phy, AT_FSM_ALERT) < 0) {
        LOG_ERROR("ERROR: WAIT--->ALERT\n");
        return -1;
      }

      CHIP_DELAY(100);
      tdd_alert_to_rx(phy);
      if (fsm_status_get(phy, AT_FSM_RX) < 0) {
        LOG_ERROR("ERROR: ALERT--->RX\n");
        return -1;
      }
    } else {
      hal_spi_write_reg(phy, 0x0cc, 0x40);
      if (fsm_status_get(phy, AT_FSM_ALERT) < 0) {
        LOG_ERROR("ERROR: WAIT--->ALERT\n");
        return -1;
      }

      CHIP_DELAY(100);
      hal_spi_write_reg(phy, 0x0cc, 0x04);
      if (fsm_status_get(phy, AT_FSM_RX) < 0) {
        LOG_ERROR("ERROR: ALERT--->RX\n");
        return -1;
      }
    }
  } else if (st == AT_FSM_TX) {
    if (phy->config->wire_control_en) {
      tdd_wait_to_alert(phy);
      if (fsm_status_get(phy, AT_FSM_ALERT) < 0) {
        LOG_ERROR("ERROR: WAIT--->ALERT\n");
        return -1;
      }

      CHIP_DELAY(100);
      tdd_alert_to_tx(phy);
      if (fsm_status_get(phy, AT_FSM_TX) < 0) {
        LOG_ERROR("ERROR: ALERT--->TX\n");
        return -1;
      }
    } else {
      hal_spi_write_reg(phy, 0x0cc, 0x40);
      if (fsm_status_get(phy, AT_FSM_ALERT) < 0) {
        LOG_ERROR("ERROR: WAIT--->ALERT\n");
        return -1;
      }

      CHIP_DELAY(100);
      hal_spi_write_reg(phy, 0x0cc, 0x02);
      if (fsm_status_get(phy, AT_FSM_TX) < 0) {
        LOG_ERROR("ERROR: ALERT--->TX\n");
        return -1;
      }
    }
  } else {
    LOG_ERROR("ERROR: target st is not TX or RX\n");
    return -1;
  }

  return 0;
}

int trx_ana_bw_adj(rf_chip_phy_t* phy, TRX_ENUM dir, TRX_CHN_ENUM chn, unsigned char val[]) {
  unsigned int rx_ctune_reg, rx_ctx_reg;
  int addr1, addr2;
  unsigned char offset1, offset2;
  int ret = 0;

  if (dir != RX_DIR) {
    LOG_ERROR("Not support for TX_DIR\n");
    phy->error = -RX_BW_CAL_FAIL;
    return -1;
  }

  if (chn == TRX_CHN1) {
    rx_ctune_reg = 0x65B;
    addr1        = 660 + 87 * phy->config->bandwith;
    offset1      = 0;
    rx_ctx_reg   = 0x662;
    addr2        = 659 + 87 * phy->config->bandwith;
    offset2      = 2;
  } else {
    rx_ctune_reg = 0x679;
    addr1        = 663 + 87 * phy->config->bandwith;
    offset1      = 1;
    rx_ctx_reg   = 0x680;
    addr2        = 662 + 87 * phy->config->bandwith;
    offset2      = 3;
  }
  hal_spi_write_reg(phy, rx_ctune_reg, val[0]);
  hal_spi_write_reg(phy, rx_ctx_reg, val[1]);

  /* update lut */
  ret = write_lut_byte(phy, TRX_BW_LUT, phy->config->bandwith * 11, rx_ctune_reg, addr1, offset1, val[0]);
  if (ret != 0) {
    LOG_ERROR("Failed to write the val(0x%x) of reg 0x%0x to LUT\n", val[0], rx_ctune_reg);
    phy->error = -RX_BW_CAL_FAIL;
    return -1;
  }
  ret = write_lut_byte(phy, TRX_BW_LUT, phy->config->bandwith * 11, rx_ctx_reg, addr2, offset2, val[1]);
  if (ret != 0) {
    LOG_ERROR("Failed to write the val(0x%x) of reg 0x%0x to LUT\n", val[1], rx_ctx_reg);
    phy->error = -RX_BW_CAL_FAIL;
    return -1;
  }

  return 0;
}

int rx_gain_mgc_change(rf_chip_phy_t* phy, TRX_CHN_ENUM chn, RX_MGC_GAIN_ENUM tb, int val) {
  short index = (chn == TRX_CHN1) ? 0 : 1;

  if (rx_mgc_gain(phy, chn, tb, val) < 0)
    return -1;

  switch (tb) {
    case LMT_G:
      phy->rx_lmt_gain[index] = val;
      break;
    case LPF_G:
      phy->rx_lpf_gain[index] = val;
      break;
    case DIG_G:
      phy->rx_dig_gain[index] = val;
      break;
  }

  return 0;
}

void rx_gain_get(rf_chip_phy_t* phy, TRX_CHN_ENUM chn, GCTRL_GET_RPT_T* cfg) {
  GCTRL_GET_RPT(phy, chn, cfg);
}

void rx_gain_mgc_full_tab_index_set(rf_chip_phy_t* phy, TRX_CHN_ENUM chn, short index) {
  GCTRL_MGC_SET_FULL_GAIN(phy, chn, index);
}

int tx_port_enable(rf_chip_phy_t* phy, TRX_CHN_ENUM chn, short en) {
  set_tx_port(phy, chn, phy->config->tx_port[chn], en);
  if (phy->st == AT_FSM_FDD) {
    fdd_fsm_to_alert(phy);
    if (fsm_status_get(phy, AT_FSM_ALERT) < 0)
      return -1;

    CHIP_DELAY(100);
    fdd_alert_to_fsm(phy);
    if (fsm_status_get(phy, AT_FSM_FDD) < 0)
      return -1;
  } else if (phy->st == AT_FSM_TX) {
    if (phy->config->wire_control_en) {
      tdd_tx_to_wait(phy);
      if (fsm_status_get(phy, AT_FSM_WAIT) < 0)
        return -1;

      tdd_wait_to_alert(phy);
      if (fsm_status_get(phy, AT_FSM_ALERT) < 0)
        return -1;

      CHIP_DELAY(100);
      tdd_alert_to_tx(phy);
      if (fsm_status_get(phy, AT_FSM_TX) < 0)
        return -1;
    } else {
      hal_spi_write_reg(phy, 0xcc, 0x0);
      if (fsm_status_get(phy, AT_FSM_WAIT) < 0)
        return -1;

      hal_spi_write_reg(phy, 0xcc, 0x40);
      if (fsm_status_get(phy, AT_FSM_ALERT) < 0)
        return -1;

      CHIP_DELAY(100);
      hal_spi_write_reg(phy, 0xcc, 0x2);
      if (fsm_status_get(phy, AT_FSM_TX) < 0)
        return -1;
    }
  }
  return 0;
}

int rx_port_enable(rf_chip_phy_t* phy, TRX_CHN_ENUM chn, short en) {
  short val, shift;

  shift = chn ? 3 : 0;

  if (en) {
    val = hal_spi_read_reg(phy, 0x805);
    val = val | (0x3 << shift);
    hal_spi_write_reg(phy, 0x805, val);
    set_rx_port(phy, chn, phy->config->rx_port[chn], phy->config->rxfe_gain[chn], en);
  } else {
    val = hal_spi_read_reg(phy, 0x805);
    val = val & (chn ? 0x27 : 0x3c);
    hal_spi_write_reg(phy, 0x805, val);
  }

  return 0;
}

int tx_dig_atten_change(rf_chip_phy_t* phy, TRX_CHN_ENUM chn, unsigned short index) {
  set_tx_dig_atten(phy, chn, index);
  return 0;
}

void rf_chip_suspend(rf_chip_phy_t* phy) {
  hal_spi_write_reg(phy, 0x900, 0x0);
  hal_spi_write_reg(phy, 0x901, 0x0);
  hal_spi_write_reg(phy, 0x904, 0x0);
  fpga_and_chip_reset(phy);
}

int rf_chip_resume(rf_chip_phy_t* phy) {
  return main_init(phy);
}

rf_chip_phy_t* rf_chip_phy_get(short sel) {
  return &g_phy_obj[sel];
}

chip_config_t* rf_chip_config_get(short sel) {
  return &g_phy_config[sel];
}

void customer_init(rf_chip_phy_t* phy, chip_config_t* config) {
  my_memset(phy, 0, sizeof(rf_chip_phy_t));
  phy->config = config;
  rf_chip_phy_obj_init(phy);
}

int gc080x_init(rf_chip_phy_t* phy, chip_config_t* config) {
  int ret = 0;

  customer_init(phy, config);
  ret = main_init(phy);

  // Rx io timing
  // hal_spi_write_reg(&g_phy_obj[0], 0x888, 0x33);
  // hal_spi_write_reg(&g_phy_obj[0], 0x889, 0x33);
  // hal_spi_write_reg(&g_phy_obj[0], 0x88A, 0x77);
  // hal_spi_write_reg(&g_phy_obj[0], 0x88B, 0x33);
  // hal_spi_write_reg(&g_phy_obj[0], 0x88C, 0x33);
  // hal_spi_write_reg(&g_phy_obj[0], 0x88D, 0x11);
  hal_spi_write_reg(&g_phy_obj[0], 0x888, 0xFF);
  hal_spi_write_reg(&g_phy_obj[0], 0x889, 0xFF);
  hal_spi_write_reg(&g_phy_obj[0], 0x88A, 0xFF);
  hal_spi_write_reg(&g_phy_obj[0], 0x88B, 0xFF);
  hal_spi_write_reg(&g_phy_obj[0], 0x88C, 0xFF);
  hal_spi_write_reg(&g_phy_obj[0], 0x88D, 0xFF);

  if (ret < 0) {
    LOG_ERROR("rf phy chip init failed!\n");
    return ret;
  }
  return 0;
}
