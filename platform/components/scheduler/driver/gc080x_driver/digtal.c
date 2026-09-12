#include "digtal.h"
#include "custom_cfg.h"
#include "driver.h"
#include "gctrl.h"
#include "main_init.h"
#include "platform.h"
#include "qec_chip.h"
#include "register.h"
#include "regs_init.h"
#include "utility.h"

//digital ldo setting
int DIG_LDO_CFG(rf_chip_phy_t* phy, BANDWITH_ENUM bandwith) {
#if 0  //D1
	if ((bandwith==BW_LTE100) || (bandwith==BW_LTE80) || (bandwith==BW_LTE60))
		spi_write_reg(0x904, 0xb6);//Supply voltage
	else
		spi_write_reg(0x904, 0xb4);//Supply voltage
#endif

  return 0;
}

int DIG_FIR_FILTER_CFG_WITH_MANUAL(rf_chip_phy_t* phy, short manual_on) {
  int tmpVal;
  unsigned short regAddr = 0x00A;

  tmpVal = hal_spi_read_reg(phy, regAddr);
  if (manual_on) {
    tmpVal = tmpVal | 0x80;
    hal_spi_write_reg(phy, regAddr, tmpVal);
  } else {
    tmpVal = tmpVal & 0x7F;
    hal_spi_write_reg(phy, regAddr, tmpVal);
  }
  return 0;
}

//digital rx filter setting
int DIG_RX_FILTER_CFG(rf_chip_phy_t* phy, TRX_CHN_ENUM chn, BANDWITH_ENUM bandwith, short fir_en) {
  short map_bw, ratio, i;
  unsigned int val, filt_ctrl_reg;

  map_bw        = user_map_to_standard(bandwith);
  ratio         = g_trx_sample_rate_table[map_bw].rx_oversample_ratio;
  filt_ctrl_reg = (chn == TRX_CHN1) ? 0x003 : 0x006;

  hal_spi_write_reg(phy, filt_ctrl_reg, 0x00);

  if (ratio >= 64) {
    if (!fir_en)
      return -1;
    val = (chn == TRX_CHN1) ? (0x03) : (0x0c);
    hal_spi_write_reg(phy, 0x009, val);
    hal_spi_write_reg(phy, filt_ctrl_reg, 0xf0);
  } else if (ratio >= 32) {
    if (fir_en) {
      val = (chn == TRX_CHN1) ? (0x01) : (0x04);
      hal_spi_write_reg(phy, 0x009, val);
      hal_spi_write_reg(phy, filt_ctrl_reg, 0xf0);
    } else {
      val = (chn == TRX_CHN1) ? (0x03) : (0x0c);
      hal_spi_write_reg(phy, 0x009, val);
      hal_spi_write_reg(phy, filt_ctrl_reg, 0xe0);
    }
  } else {
    if (ratio < 2)
      return -1;

    if (ratio >= 16) {
      ratio = 16;
      if (fir_en) {
        hal_spi_write_reg(phy, filt_ctrl_reg, 0xf0);
      } else {
        val = (chn == TRX_CHN1) ? (0x01) : (0x04);
        hal_spi_write_reg(phy, 0x009, val);
        hal_spi_write_reg(phy, filt_ctrl_reg, 0xe0);
      }
      return 0;
    }

    ratio = my_log2(8, ratio);
    val   = hal_spi_read_reg(phy, filt_ctrl_reg);
    for (i = 0; i < ratio; i++) {
      if (fir_en)
        val = SET_BIT(val, (i + 4));
      else
        val = SET_BIT(val, (i + 5));
    }
    hal_spi_write_reg(phy, filt_ctrl_reg, val);

    if (!fir_en)
      HAL_REG_CLR_BIT(phy, filt_ctrl_reg, BIT4);
  }

  hal_spi_write_reg(phy, ((chn == TRX_CHN1) ? 0x005 : 0x008), 0x10);

  return 0;
}

//enable or disable tx filter
int DIG_TX_FILTER_CFG(rf_chip_phy_t* phy, TRX_CHN_ENUM chn, BANDWITH_ENUM bandwith, short fir_en) {
  short map_bw, ratio, i;
  unsigned int val, filt_ctrl_reg;

  map_bw        = user_map_to_standard(bandwith);
  ratio         = g_trx_sample_rate_table[map_bw].tx_oversample_ratio;
  filt_ctrl_reg = (chn == TRX_CHN1) ? 0x79F : 0x7B2;

  hal_spi_write_reg(phy, filt_ctrl_reg, 0x00);
  if (fir_en) {
    if (ratio == 2) {
      hal_spi_write_reg(phy, filt_ctrl_reg, 0x00);
    } else {
      if (ratio >= 64)
        ratio = 64;
      ratio = my_log2(8, ratio) - 1;
      val   = hal_spi_read_reg(phy, filt_ctrl_reg);
      for (i = 0; i < ratio; i++)
        val = SET_BIT(val, (i));
      hal_spi_write_reg(phy, filt_ctrl_reg, val);
    }
    hal_spi_write_reg(phy, ((chn == TRX_CHN1) ? 0x001 : 0x002), 0x40);
  } else {
    if (ratio == 2) {
      hal_spi_write_reg(phy, filt_ctrl_reg, 0x01);
    } else {
      if (ratio >= 32)
        ratio = 32;
      ratio = my_log2(8, ratio);
      val   = hal_spi_read_reg(phy, filt_ctrl_reg);
      for (i = 0; i < ratio; i++)
        val = SET_BIT(val, (i));
      hal_spi_write_reg(phy, filt_ctrl_reg, val);
    }
    hal_spi_write_reg(phy, ((chn == TRX_CHN1) ? 0x001 : 0x002), 0x00);
  }

  val = hal_spi_read_reg(phy, (chn ? 0x7A3 : 0x790));
  val = SET_BIT(val, BIT0);
  val = SET_BIT(val, BIT3);
  hal_spi_write_reg(phy, (chn ? 0x7A3 : 0x790), val);

  return 0;
}

int DIG_FIR_FILTER_READ(rf_chip_phy_t* phy, short dir, TRX_CHN_ENUM chn, BANDWITH_ENUM bandwith, short fir_tap_n, unsigned short* fir_tap_coeff) {
  int addr, offset, addr_via_bw;
  unsigned char val[4];
  short i, n;

  addr        = (dir) ? 9 : 664;
  offset      = (chn) ? 32 : 0;
  addr_via_bw = ((dir) ? 73 : 87) * bandwith;
  addr        = addr + offset + addr_via_bw;

  n = fir_tap_n / 2;
  for (i = 0; i < n; i++) {
    read_lut_word(phy, TRX_BW_LUT, addr + i, val);
    fir_tap_coeff[i * 2]     = (val[0] & 0xff) | (val[1] << 8);
    fir_tap_coeff[i * 2 + 1] = (val[2] & 0xff) | (val[3] << 8);
  }

  return 0;
}

//set trx filter coef
int DIG_FIR_FILTER_CFG(rf_chip_phy_t* phy, short manual_on, short dir, TRX_CHN_ENUM chn, BANDWITH_ENUM bandwith, short fir_tap_n, int* fir_tap_coeff, short rw_lut_flag) {
  unsigned char coeff_write_ctrl[2][2] = {{0x09, 0x11}, {0x03, 0x05}};
  int addr, offset, addr_via_bw;
  unsigned char val[4];
  short i, n;

  short fir_order;
  fir_order = 16 - (fir_tap_n / 8);
  addr      = ((chn) ? 2 : 1) * ((dir) ? 0x01 : 0x03);
  hal_spi_write_reg(phy, addr, fir_order | (hal_spi_read_reg(phy, addr) & 0xF0));
  if (manual_on) {
    for (i = 0; i < fir_tap_n / 2; i++) {
      hal_spi_write_reg(phy, 0x0a, 0x01);
      if (fir_tap_coeff[i] < 0)
        fir_tap_coeff[i] = fir_tap_coeff[i] + (1 << 16);
      hal_spi_write_reg(phy, 0x0b, i);
      hal_spi_write_reg(phy, 0x0c, (fir_tap_coeff[i] >> 8) & 0xff);
      hal_spi_write_reg(phy, 0x0d, fir_tap_coeff[i] & 0xff);
      hal_spi_write_reg(phy, 0x0a, coeff_write_ctrl[dir][chn]);
      CHIP_UDELAY(100);
    }

    hal_spi_write_reg(phy, 0x0a, 0x01);
    CHIP_UDELAY(100);
    hal_spi_write_reg(phy, 0x0a, 0x80);

    if (rw_lut_flag) {
      addr        = (dir) ? 9 : 664;
      offset      = (chn) ? 32 : 0;
      addr_via_bw = ((dir) ? 73 : 87) * bandwith;
      addr        = addr + offset + addr_via_bw;

      n = fir_tap_n / 4;
      for (i = 0; i < n; i++) {
        val[0] = fir_tap_coeff[i * 2] & 0xff;
        val[1] = (fir_tap_coeff[i * 2] >> 8) & 0xff;
        val[2] = fir_tap_coeff[i * 2 + 1] & 0xff;
        val[3] = (fir_tap_coeff[i * 2 + 1] >> 8) & 0xff;
        write_lut_word(phy, TRX_BW_LUT, addr + i, val);
      }
    }
  } else {
    for (i = 0; i < fir_tap_n / 2; i++) {
      if (fir_tap_coeff[i] < 0)
        fir_tap_coeff[i] = fir_tap_coeff[i] + (1 << 16);
    }

    addr        = (dir) ? 9 : 664;
    offset      = (chn) ? 32 : 0;
    addr_via_bw = ((dir) ? 73 : 87) * bandwith;
    addr        = addr + offset + addr_via_bw;

    n = fir_tap_n / 4;
    for (i = 0; i < n; i++) {
      val[0] = fir_tap_coeff[i * 2] & 0xff;
      val[1] = (fir_tap_coeff[i * 2] >> 8) & 0xff;
      val[2] = fir_tap_coeff[i * 2 + 1] & 0xff;
      val[3] = (fir_tap_coeff[i * 2 + 1] >> 8) & 0xff;
      write_lut_word(phy, TRX_BW_LUT, addr + i, val);
    }

    hal_spi_write_reg(phy, 0x0a, 0x00);
  }

  return 0;
}

//manual turn on TX DAC
int DIG_TX_DAC_MANUAL_ENA(rf_chip_phy_t* phy, TRX_CHN_ENUM chn, short manual_on) {
  short regs[2][2] = {{0x790, 0x791}, {0x7a3, 0x7a4}};

  if (manual_on) {
    HAL_REG_SET_BIT(phy, regs[chn][0], BIT0);
    hal_spi_write_reg(phy, regs[chn][1], 0x0F);
    hal_spi_write_reg(phy, regs[chn][1], 0x2F);
  } else {
    HAL_REG_CLR_BIT(phy, regs[chn][0], BIT0);
    hal_spi_write_reg(phy, regs[chn][1], 0x00);
  }

  return 0;
}

//digital interface setting
int DIG_INTF_CFG(rf_chip_phy_t* phy, DIG_IF_ENUM dif, IF_TYPE_ENUM port, DATA_RATE_ENUM rate, PHY_MODE_ENUM mode) {
  short bb_interface_config[18] = {0x62, 0x42, 0x6a, 0x4a, 0x62, 0x42, 0x6a, 0x4a, 0x12, 0x62, 0x42, 0x6a, 0x4a, 0x62, 0x42, 0x6a, 0x4a, 0x12};
  unsigned char reg_val;

  hal_spi_write_reg(phy, 0x061, 0x08);
  hal_spi_write_reg(phy, 0x062, 0x00);
  hal_spi_write_reg(phy, 0x064, 0x04);
  hal_spi_write_reg(phy, 0x065, 0x04);

//hal_spi_write_reg(phy, 0x063, 0x52);
#if 1
  //HAL_REG_SET_BIT(phy, 0x063, BIT6);//C1
  //HAL_REG_CLR_BIT(phy, 0x063, BIT6);//D1
  reg_val = hal_spi_read_reg(phy, 0x063);
  reg_val = SET_BIT(reg_val, BIT1);
  reg_val = CLR_BIT(reg_val, BIT0);
  if (rate == SDR)
    reg_val = SET_BIT(reg_val, BIT5);
  else
    reg_val = CLR_BIT(reg_val, BIT5);

  if (dif == LVDS_IF) {
    reg_val = SET_BIT(reg_val, BIT4);
    reg_val = CLR_BIT(reg_val, BIT6);  //D1
  } else {
    reg_val = CLR_BIT(reg_val, BIT4);
    reg_val = SET_BIT(reg_val, BIT6);  //D1
  }

  if (mode == PHY_TDD)
    reg_val = SET_BIT(reg_val, BIT3);
  else
    reg_val = CLR_BIT(reg_val, BIT3);

  if (port == SINGLE_PORT)
    reg_val = SET_BIT(reg_val, BIT2);
  else
    reg_val = CLR_BIT(reg_val, BIT2);
  hal_spi_write_reg(phy, 0x063, reg_val);
#endif

//lvds set for fsm only
#if 1
  //if ((data_rate == LVDS_2R2T_DP_FDD_DDR) || (data_rate == LVDS_1R1T_DP_FDD_DDR))
  if (dif == LVDS_IF) {
    hal_spi_write_reg(phy, 0x06C, 0xA0);
    hal_spi_write_reg(phy, 0x06D, 0xA0);
    hal_spi_write_reg(phy, 0x068, 0x04);
    if (mode == PHY_TDD)
      hal_spi_write_reg(phy, 0x069, 0xC3);
    else
      hal_spi_write_reg(phy, 0x069, 0xC2);
  }
#endif

  return 0;
}

//ENSM para setting
int DIG_ENSM_CFG(rf_chip_phy_t* phy, short manual_on) {
  //hal_spi_write_reg(phy, 0x0D7, 0xC3);//C1
  // LOG_MAIN("DIG_ENSM_CFG IN\n\r");
  hal_spi_write_reg(phy, 0x0D7, 0xCB);  //D1
                                        //   LOG_MAIN("hal_spi_write_reg\n\r");
  if (manual_on) {
    hal_spi_write_reg(phy, 0x0CD, 0x1F);
    hal_spi_write_reg(phy, 0x0CE, 0xFF);
    hal_spi_write_reg(phy, 0x0CF, 0xFF);
    hal_spi_write_reg(phy, 0x0D0, 0x00);
    hal_spi_write_reg(phy, 0x0D1, 0x00);
    hal_spi_write_reg(phy, 0x0D2, 0x00);
    hal_spi_write_reg(phy, 0x0D3, 0x00);
    hal_spi_write_reg(phy, 0x0D4, 0x00);
    hal_spi_write_reg(phy, 0x0D5, 0x00);
    hal_spi_write_reg(phy, 0x0D6, 0x00);
    hal_spi_write_reg(phy, 0x0D8, 0x00);
    hal_spi_write_reg(phy, 0x0D9, 0x00);
  } else {
    hal_spi_write_reg(phy, 0x0CD, 0x00);
    hal_spi_write_reg(phy, 0x0CE, 0x00);
    hal_spi_write_reg(phy, 0x0CF, 0x00);
    hal_spi_write_reg(phy, 0x0D0, 0xFF);
    hal_spi_write_reg(phy, 0x0D1, 0xFF);
    hal_spi_write_reg(phy, 0x0D2, 0xFF);
    hal_spi_write_reg(phy, 0x0D3, 0xFF);
    hal_spi_write_reg(phy, 0x0D4, 0xFF);
    hal_spi_write_reg(phy, 0x0D5, 0xFF);
    hal_spi_write_reg(phy, 0x0D6, 0xFF);
    hal_spi_write_reg(phy, 0x0D8, 0xFF);
    hal_spi_write_reg(phy, 0x0D9, 0xFF);
  }

  return 0;
}

//data interface clock setting
int DIG_INTF_CLK_CFG(rf_chip_phy_t* phy, BANDWITH_ENUM bandwith, DATA_RATE_MODE_ENUM data_rate) {
  unsigned int data_rate_hz, adc_sample_rate_hz;
  short map_bw, data_div;

  if (data_rate <= LVDS_2R2T_DP_FDD_DDR) {
    map_bw             = user_map_to_standard(bandwith);
    data_rate_hz       = g_data_rate[data_rate][map_bw];
    adc_sample_rate_hz = g_trx_sample_rate_table[map_bw].rxadc_sample_rate;
    data_div           = adc_sample_rate_hz / data_rate_hz;
  } else {
    map_bw             = user_map_to_standard(bandwith);
    data_rate_hz       = g_data_rate[data_rate - CMOS_1R1T_SP_FDD_SDR][map_bw];
    adc_sample_rate_hz = g_trx_sample_rate_table[map_bw].rxadc_sample_rate;
    data_div           = adc_sample_rate_hz / (data_rate_hz >> 1);
  }
  //hal_spi_write_reg(phy, 0x0de, 0x00);
  hal_spi_write_reg(phy, 0x0de, data_div);

  return 0;
}

//digital channel enable
int DIG_CHAN_ENA(rf_chip_phy_t* phy, short dir, TRX_CHN_ENUM chn, short enable) {
  unsigned int val;
  REG_BIT dig_chn_config[2][2] = {{BIT0, BIT1}, {BIT2, BIT3}};

  if (enable)
    HAL_REG_SET_BIT(phy, 0x0DC, dig_chn_config[dir][chn]);
  else
    HAL_REG_CLR_BIT(phy, 0x0DC, dig_chn_config[dir][chn]);

  return 0;
}

//manual ENSM force status enable
int ENSM_MANAUL_ENA(rf_chip_phy_t* phy, short manual_on, ENSM_STATE_ENUM stat) {
  unsigned int val;

  if (manual_on) {
    switch (stat) {
      case FSM_IDLE:
        HAL_REG_CLR_BIT(phy, 0x0DD, BIT0);
        HAL_REG_CLR_BIT(phy, 0x0DD, BIT1);
        val = hal_spi_read_reg(phy, 0x0dc);
        hal_spi_write_reg(phy, 0x0dc, val | 0x30);
        break;
      case FSM_TDD_RX:
        HAL_REG_CLR_BIT(phy, 0x0DD, BIT0);
        HAL_REG_SET_BIT(phy, 0x0DD, BIT1);
        val = hal_spi_read_reg(phy, 0x0dc);
        hal_spi_write_reg(phy, 0x0dc, val | 0x30);
        break;
      case FSM_TDD_TX:
        HAL_REG_SET_BIT(phy, 0x0DD, BIT0);
        HAL_REG_CLR_BIT(phy, 0x0DD, BIT1);
        val = hal_spi_read_reg(phy, 0x0dc);
        hal_spi_write_reg(phy, 0x0dc, val | 0x50);
        break;
      case FSM_FDD:
        HAL_REG_SET_BIT(phy, 0x0DD, BIT0);
        HAL_REG_SET_BIT(phy, 0x0DD, BIT1);
        val = hal_spi_read_reg(phy, 0x0dc);
        hal_spi_write_reg(phy, 0x0dc, val | 0x70);
        break;
      default:
        break;
    }

    val = hal_spi_read_reg(phy, 0x0dc);
    hal_spi_write_reg(phy, 0x0dc, (stat << 5) | val);
    hal_spi_write_reg(phy, 0x0dc, (stat << 5) | val | (0x1 << 4));  // manual on need ->0 -> 1

  } else {
    HAL_REG_CLR_BIT(phy, 0x0DD, BIT0);
    HAL_REG_CLR_BIT(phy, 0x0DD, BIT1);
    HAL_REG_CLR_BIT(phy, 0x0DC, BIT4);
  }
  return 0;
}

//digital soft-reset
int DIG_SOFT_RST_N(rf_chip_phy_t* phy, short dig_reset_enable) {
  unsigned int regs[10] = {0x061, 0x062, 0x063, 0x064, 0x065, 0x0de, 0x06C, 0x06D, 0x068, 0x069};
  unsigned int val[10];
  short i;

  if (dig_reset_enable == 0) {  //0, interface reset
    for (i = 0; i < 10; i++)
      val[i] = hal_spi_read_reg(phy, regs[i]);

    HAL_REG_SET_BIT(phy, 0x0f7, BIT1);
    HAL_REG_CLR_BIT(phy, 0x0f7, BIT1);
    HAL_REG_SET_BIT(phy, 0x0f7, BIT1);

    for (i = 0; i < 10; i++)
      hal_spi_write_reg(phy, regs[i], val[i]);
  } else {  //1, agc reset
    HAL_REG_SET_BIT(phy, 0x0f5, BIT2);
    HAL_REG_CLR_BIT(phy, 0x0f5, BIT2);
    HAL_REG_SET_BIT(phy, 0x0f5, BIT2);
  }

  return 0;
}

//adjust interface feedback clock delay
int DIG_DCLK_DELAY_CFG(rf_chip_phy_t* phy, short fpga_num, short val) {
  hal_spi_write_reg(phy, 0x06b, val);
  return 0;
}
