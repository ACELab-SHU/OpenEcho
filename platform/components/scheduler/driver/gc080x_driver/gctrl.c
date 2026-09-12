#include "driver.h"
#include "platform.h"
#include "custom_cfg.h"
#include "utility.h"
#include "regs_init.h"
#include "register.h"
#include "qec_chip.h"
#include "main_init.h"
#include "gctrl.h"

/****************************************************************************************/
// rx gain control
/****************************************************************************************/
int GCTRL_SETUP_DELAY_CFG(rf_chip_phy_t *phy, TRX_CHN_ENUM chn, GCTRL_SETUP_DELAY_CFG_T *cfg)
{
    short addr_ofst = chn?0x80:0;
	short cfg_init_setting_delay = cfg->cfg_init_setting_delay & 0x7F;
	short cfg_pdt_setting_delay  = cfg->cfg_pdt_setting_delay  & 0xFF;
	short cfg_hbf_setting_delay  = cfg->cfg_hbf_setting_delay  & 0xFF;
    hal_spi_write_reg(phy, SETTING_DLY_CFG_RX1_0+addr_ofst, cfg->cfg_init_setting_delay);
    hal_spi_write_reg(phy, SETTING_DLY_CFG_RX1_1+addr_ofst, cfg->cfg_pdt_setting_delay );
    hal_spi_write_reg(phy, SETTING_DLY_CFG_RX1_2+addr_ofst, cfg->cfg_hbf_setting_delay );
	return 0;
}

int GCTRL_EXT_LNA_CFG(rf_chip_phy_t *phy, TRX_CHN_ENUM chn, GCTRL_EXT_LNA_CFG_T *cfg)
{
    short addr_ofst = chn?0x80:0;
	short val;

    val = hal_spi_read_reg(phy, SPLIT_GAIN_TABLE_CFG_RX1_0+addr_ofst);
	if (cfg->cfg_split_ext_lna_force_ena)
		val = SET_BIT(val, BIT7);
	else
		val = CLR_BIT(val, BIT7);

	if (cfg->cfg_split_ext_lna_force_index)
		val = SET_BIT(val, BIT6);
	else
		val = CLR_BIT(val, BIT6);

	if (cfg->cfg_split_ext_lna_ena)
		val = SET_BIT(val, BIT5);
	else
		val = CLR_BIT(val, BIT5);

	val = SET_BITS(val, BIT0, 5, cfg->cfg_split_ext_lna_db);
    hal_spi_write_reg(phy, SPLIT_GAIN_TABLE_CFG_RX1_0+addr_ofst, val);

    val = hal_spi_read_reg(phy, SPLIT_GAIN_TABLE_CFG_RX1_1+addr_ofst);
	if (cfg->cfg_split_ext_lna_polarity)
		val = SET_BIT(val, BIT7);
	else
		val = CLR_BIT(val, BIT7);

	val = SET_BITS(val, BIT0, 6, cfg->cfg_split_ext_lna_inc_th);
    hal_spi_write_reg(phy, SPLIT_GAIN_TABLE_CFG_RX1_1+addr_ofst, val);

    hal_spi_write_reg(phy, SPLIT_GAIN_TABLE_CFG_RX1_2+addr_ofst, cfg->cfg_split_ext_lna_dec_th);

	return 0;
}

int GCTRL_FULL_TABLE_CFG_SINGLE_VAL(rf_chip_phy_t *phy, TRX_CHN_ENUM chn, short addr, short data)
{
    short addr_ofst = chn?0x80:0;
	short val;

    val = hal_spi_read_reg(phy, FULL_GAIN_TABLE_CFG_RX1_0+addr_ofst);
	val = SET_BITS(val, BIT1, 7, addr);
    hal_spi_write_reg(phy, FULL_GAIN_TABLE_CFG_RX1_0+addr_ofst, val);
	
    hal_spi_write_reg(phy, FULL_GAIN_TABLE_CFG_RX1_1+addr_ofst, data);

    val = hal_spi_read_reg(phy, FULL_GAIN_TABLE_CFG_RX1_0+addr_ofst);
	val = CLR_BIT(val, BIT0);
    hal_spi_write_reg(phy, FULL_GAIN_TABLE_CFG_RX1_0+addr_ofst, val);
	val = SET_BIT(val, BIT0);
    hal_spi_write_reg(phy, FULL_GAIN_TABLE_CFG_RX1_0+addr_ofst, val);
	val = CLR_BIT(val, BIT0);
    hal_spi_write_reg(phy, FULL_GAIN_TABLE_CFG_RX1_0+addr_ofst, val);
	
	return 0;
}

int GCTRL_FULL_TABLE_SET_CONTENT(rf_chip_phy_t *phy, TRX_CHN_ENUM chn,short addr ,short data)
{
    short addr_ofst = chn?0x80:0;
    short val;

    val = hal_spi_read_reg(phy, FULL_GAIN_TABLE_CFG_RX1_0+addr_ofst);
    val = SET_BITS(val, BIT1, 7, addr);
    hal_spi_write_reg(phy, FULL_GAIN_TABLE_CFG_RX1_0+addr_ofst, val);

    hal_spi_write_reg(phy, FULL_GAIN_TABLE_CFG_RX1_1+addr_ofst, data);

    val = hal_spi_read_reg(phy, FULL_GAIN_TABLE_CFG_RX1_0+addr_ofst);
    val = CLR_BIT(val, BIT0);
    hal_spi_write_reg(phy, FULL_GAIN_TABLE_CFG_RX1_0+addr_ofst, val);
    val = SET_BIT(val, BIT0);
    hal_spi_write_reg(phy, FULL_GAIN_TABLE_CFG_RX1_0+addr_ofst, val);
    val = CLR_BIT(val, BIT0);
    hal_spi_write_reg(phy, FULL_GAIN_TABLE_CFG_RX1_0+addr_ofst, val);

    return 0;
}

int GCTRL_FULL_TABLE_SET_MAX_INDEX(rf_chip_phy_t *phy, TRX_CHN_ENUM chn, short val)
{
	short addr_ofst = chn?0x80:0;
	hal_spi_write_reg(phy, MAX_GAIN_CFG_RX1_0+addr_ofst, val);
	return 0;
}

int GCTRL_FULL_TABLE_SET_INIT_INDEX(rf_chip_phy_t *phy, TRX_CHN_ENUM chn, short val)
{
	short addr_ofst = chn?0x80:0;
	hal_spi_write_reg(phy, GCTRL_INIT_CFG_RX1_3+addr_ofst, val);
	return 0;
}

int GCTRL_FULL_TABLE_RPT_CONTENT(rf_chip_phy_t *phy, TRX_CHN_ENUM chn, short addr, short *data)
{
    short addr_ofst = chn?0x80:0;
	short val;

    val = hal_spi_read_reg(phy, FULL_GAIN_TABLE_CFG_RX1_2+addr_ofst);
	val = SET_BITS(val, BIT1, 7, addr);
    hal_spi_write_reg(phy, FULL_GAIN_TABLE_CFG_RX1_2+addr_ofst, val);

    val = hal_spi_read_reg(phy, FULL_GAIN_TABLE_CFG_RX1_2+addr_ofst);
	val = CLR_BIT(val, BIT0);
    hal_spi_write_reg(phy, FULL_GAIN_TABLE_CFG_RX1_2+addr_ofst, val);
	val = SET_BIT(val, BIT0);
    hal_spi_write_reg(phy, FULL_GAIN_TABLE_CFG_RX1_2+addr_ofst, val);
	val = CLR_BIT(val, BIT0);
    hal_spi_write_reg(phy, FULL_GAIN_TABLE_CFG_RX1_2+addr_ofst, val);

    *data = hal_spi_read_reg(phy, FULL_GAIN_TABLE_CFG_RX1_3+addr_ofst);

	return 0;
}

int GCTRL_FULL_TABLE_CFG(rf_chip_phy_t *phy, TRX_CHN_ENUM chn,  GCTRL_FULL_TABLE_CFG_T *cfg)
{
	int addr;
    for(addr = 0; addr <= cfg->max_index; addr++) {
        GCTRL_FULL_TABLE_SET_CONTENT(phy, chn, addr, cfg->content[addr]);
	}
    GCTRL_FULL_TABLE_SET_MAX_INDEX(phy, chn, cfg->max_index);
    GCTRL_FULL_TABLE_SET_INIT_INDEX(phy, chn,cfg->init_index); //set the defult gain

    return 0;
}

int GCTRL_SPLIT_TABLE_CFG(rf_chip_phy_t *phy, TRX_CHN_ENUM chn, GCTRL_SPLIT_TABLE_CFG_T *cfg)
{
    short addr_ofst = chn?0x80:0;
	short val;
    short addr;

    short cfg_split_lna_index_limit = cfg->cfg_split_lna_index_limit & 0x1;
    short cfg_split_lmt_index_limit = cfg->cfg_split_lmt_index_limit & 0x7;
    short cfg_split_init_lna_index  = cfg->cfg_split_init_lna_index  & 0x1;
    short cfg_split_init_lmt_index  = cfg->cfg_split_init_lmt_index  & 0x7;
    short cfg_split_init_lpf_index  = cfg->cfg_split_init_lpf_index  & 0xF;

    addr = SPLIT_GAIN_TABLE_CFG_RX1_3+addr_ofst;
    val = hal_spi_read_reg(phy, addr);
    val = (val & 0xF0) | (cfg_split_lna_index_limit<<3) | (cfg_split_lmt_index_limit);
    hal_spi_write_reg(phy, addr, val);

    addr = GCTRL_INIT_CFG_RX1_1+addr_ofst;
    val = (cfg_split_init_lna_index<<7) | (cfg_split_init_lmt_index<<4) | cfg_split_init_lpf_index;
    hal_spi_write_reg(phy, addr, val);

	return 0;
}

int GCTRL_DIG_AGC_ENA(rf_chip_phy_t *phy, TRX_CHN_ENUM chn, GCTRL_DIG_AGC_CFG_T *cfg)
{
    short addr_ofst = chn?0x80:0;
	short val;
    short addr;
    short cfg_digital_gain_ena   = cfg->cfg_digital_gain_ena  & 0x1;
    short cfg_digital_max_index  = cfg->cfg_digital_max_index & 0x7FF;
    short cfg_digital_init_index = cfg->cfg_digital_max_index & 0x7FF;

    addr = GCTRL_INIT_CFG_RX1_0+addr_ofst;
    val = hal_spi_read_reg(phy, addr);
    if (cfg_digital_gain_ena)
		val = SET_BIT(val, BIT3);
	else
		val = CLR_BIT(val, BIT3);
    hal_spi_write_reg(phy, addr, val);

    addr = MAX_GAIN_CFG_RX1_1+addr_ofst;
    hal_spi_write_reg(phy, addr, cfg_digital_max_index);

    addr = GCTRL_INIT_CFG_RX1_2+addr_ofst;
    hal_spi_write_reg(phy, addr, cfg_digital_init_index);
	return 0;
}

int GCTRL_HBF_LPWR_DET_CFG(rf_chip_phy_t *phy, TRX_CHN_ENUM chn, GCTRL_HBF_LOW_PWR_CFG_T *cfg)
{
    short val;
    short addr;
    short addr_ofst = chn?0x80:0;

    short cfg_hbf_low_pwr_meas_num = cfg->cfg_hbf_low_pwr_meas_num & 0x7;
    short cfg_hbf_low_pwr_mask     = (~(cfg->cfg_hbf_low_pwr_ena   & 0x1))&0x1;
    short cfg_hbf_low_pwr_th       = cfg->cfg_hbf_low_pwr_th       & 0x7F;

    addr = DETECTOR_CTRL_RX1_1 + addr_ofst;
    val = hal_spi_read_reg(phy, addr);
    if(cfg_hbf_low_pwr_mask==1)
    {
        val = (val & 0xF3) | 0x0C;
        hal_spi_write_reg(phy, addr,val);
    }
    else
    {
        val = (val & 0xF3);
        hal_spi_write_reg(phy, addr,val);

        addr = HBF_AVP_CFG_RX1_4 + addr_ofst;
        val = hal_spi_read_reg(phy, addr);
        val = (val & 0x8F)| ( (cfg_hbf_low_pwr_meas_num)<<4);
        hal_spi_write_reg(phy, addr,val);

        addr = HBF_AVP_CFG_RX1_3 + addr_ofst;
        if(cfg_hbf_low_pwr_meas_num !=0)
        {
            val = cfg_hbf_low_pwr_meas_num + (1<<(cfg_hbf_low_pwr_meas_num-1));
        }
        else
        {
            val = 3;
        }
        hal_spi_write_reg(phy, addr,val);

        addr = HBF_AVP_CFG_RX1_2 + addr_ofst;
        val  = cfg_hbf_low_pwr_th;
        hal_spi_write_reg(phy, addr,val);
    }
    return 0;

}

int GCTRL_HBF_PWR_MEAS_CFG(rf_chip_phy_t *phy, TRX_CHN_ENUM chn, GCTRL_HBF_PWR_MEAS_CFG_T *cfg)
{
    short addr_ofst = chn?0x80:0;
	short val;
    short addr;

    short cfg_hbf_avp_mask      = (~(cfg->cfg_hbf_avp_ena & 0x1))&0x1;
    short cfg_hbf_avp_meas_num = cfg->cfg_hbf_avp_meas_num & 0xf;

    addr = DETECTOR_CTRL_RX1_1 + addr_ofst;
    val = hal_spi_read_reg(phy, addr);
    val = (val & 0xFE ) | cfg_hbf_avp_mask;
    hal_spi_write_reg(phy, addr, val);

    addr = HBF_AVP_CFG_RX1_4 + addr_ofst;
    val  = hal_spi_read_reg(phy, addr);
    val  = (val & 0xF0) | cfg_hbf_avp_meas_num;
    hal_spi_write_reg(phy, addr,val);
  

	return 0;
}

int GCTRL_HBF_OVF_DET_CFG(rf_chip_phy_t *phy, TRX_CHN_ENUM chn, GCTRL_HBF_OVF_DET_CFG_T *cfg)
{
    short addr_ofst = chn?0x80:0;
	short val;
    short addr;
    short cfg_hbf_ovf_mask = (~(cfg->cfg_hbf_ovf_ena&0x1))&0x1;
    short cfg_hbf_ovf_exceed_threshold = (cfg->cfg_hbf_ovf_exceed_threshold)&0xFF;

    addr = DETECTOR_CTRL_RX1_1+addr_ofst;
    val = hal_spi_read_reg(phy, addr);
    val = (val & 0xFD) | (cfg_hbf_ovf_mask<<1);
    hal_spi_write_reg(phy, addr, val);

    if(cfg_hbf_ovf_mask==0)
    {
        addr = HBF_AVP_CFG_RX1_1+addr_ofst;
        hal_spi_write_reg(phy, addr, cfg_hbf_ovf_exceed_threshold);
    }

	return 0;
}

int GCTRL_LMT_PDT_CFG(rf_chip_phy_t *phy, TRX_CHN_ENUM chn, GCTRL_LMT_PEAK_DET_CFG_T *cfg)
{
    short addr_ofst = chn?0x80:0;
	short val;
    short cfg_lmt_overload_mask     = (~(cfg->cfg_lmt_overload_ena    &0x1))&0x1;
    short cfg_lmt_prevent_inc_mask  = (~(cfg->cfg_lmt_prevent_inc_ena &0x1))&0x1;
    short cfg_lmt_underload_mask    = (~(cfg->cfg_lmt_underload_ena   &0x1))&0x1;


    val = hal_spi_read_reg(phy, DETECTOR_CTRL_RX1_0+addr_ofst);
    val = SET_BITS(val, BIT0, 3,  (cfg_lmt_overload_mask<<2)|(cfg_lmt_prevent_inc_mask<<1)|(cfg_lmt_underload_mask<<0));
    hal_spi_write_reg(phy, DETECTOR_CTRL_RX1_0+addr_ofst, val);

    hal_spi_write_reg(phy, LMT_PDT_CFG_RX1_1+addr_ofst, cfg->cfg_lmt_overload_threshold);
    hal_spi_write_reg(phy, LMT_PDT_CFG_RX1_2+addr_ofst, cfg->cfg_lmt_prevent_inc_threshold);
    hal_spi_write_reg(phy, LMT_PDT_CFG_RX1_3+addr_ofst, cfg->cfg_lmt_underload_threshold);
    hal_spi_write_reg(phy, LMT_PDT_CFG_RX1_4+addr_ofst, cfg->cfg_lmt_overload_exceed_counter);
    hal_spi_write_reg(phy, LMT_PDT_CFG_RX1_5+addr_ofst, cfg->cfg_lmt_prevent_exceed_counter);
    hal_spi_write_reg(phy, LMT_PDT_CFG_RX1_6+addr_ofst, cfg->cfg_lmt_underload_exceed_counter);

    val = hal_spi_read_reg(phy, LMT_PDT_CFG_RX1_7+addr_ofst);
	val = SET_BITS(val, BIT3, 3, cfg->cfg_lmt_overload_power_coeff);
	val = SET_BITS(val, BIT0, 3, cfg->cfg_lmt_underload_power_coeff);
    hal_spi_write_reg(phy, LMT_PDT_CFG_RX1_7+addr_ofst, val);

	GCTRL_ANA_PEAK_DETECT_ENABLE(phy, chn);
	return 0;
}

int GCTRL_ADC_PDT_CFG(rf_chip_phy_t *phy, TRX_CHN_ENUM chn, GCTRL_ADC_PEAK_DET_CFG_T *cfg)
{
    short addr_ofst = chn?0x80:0;
	short val;
    short cfg_adc_overload_lg_mask  = (~(cfg->cfg_adc_overload_lg_ena    &0x1))&0x1;
    short cfg_adc_overload_sm_mask  = (~(cfg->cfg_adc_overload_sm_ena    &0x1))&0x1;
    short cfg_adc_prevent_inc_mask  = (~(cfg->cfg_adc_prevent_inc_ena    &0x1))&0x1;
    short cfg_adc_underload_mask    = (~(cfg->cfg_adc_underload_ena      &0x1))&0x1;

    val = hal_spi_read_reg(phy, DETECTOR_CTRL_RX1_0+addr_ofst);
    val = SET_BITS(val, BIT3, 4, (cfg_adc_overload_lg_mask<<3)|(cfg_adc_overload_sm_mask<<2)|(cfg_adc_prevent_inc_mask<<1)|(cfg_adc_underload_mask<<0));
    hal_spi_write_reg(phy, DETECTOR_CTRL_RX1_0+addr_ofst, val);

    hal_spi_write_reg(phy, ADC_PDT_CFG_RX1_0+addr_ofst, cfg->cfg_adc_overload_lg_threshold     );
    hal_spi_write_reg(phy, ADC_PDT_CFG_RX1_1+addr_ofst, cfg->cfg_adc_overload_sm_threshold     );
    hal_spi_write_reg(phy, ADC_PDT_CFG_RX1_2+addr_ofst, cfg->cfg_adc_prevent_inc_threshold     );
    hal_spi_write_reg(phy, ADC_PDT_CFG_RX1_3+addr_ofst, cfg->cfg_adc_underload_threshold       );
    hal_spi_write_reg(phy, ADC_PDT_CFG_RX1_4+addr_ofst, cfg->cfg_adc_overload_lg_exceed_counter);
    hal_spi_write_reg(phy, ADC_PDT_CFG_RX1_5+addr_ofst, cfg->cfg_adc_overload_sm_exceed_counter);
    hal_spi_write_reg(phy, ADC_PDT_CFG_RX1_6+addr_ofst, cfg->cfg_adc_prevent_inc_exceed_counter);
    hal_spi_write_reg(phy, ADC_PDT_CFG_RX1_7+addr_ofst, cfg->cfg_adc_underload_exceed_counter  );

    val = hal_spi_read_reg(phy, ADC_PDT_CFG_RX1_8+addr_ofst);
	val = SET_BITS(val, BIT3, 3, cfg->cfg_adc_overload_power_coeff);
	val = SET_BITS(val, BIT0, 3, cfg->cfg_adc_underload_power_coeff);
    hal_spi_write_reg(phy, ADC_PDT_CFG_RX1_8+addr_ofst, val);
	
	return 0;
}

int GCTRL_BASIC_CFG(rf_chip_phy_t *phy, TRX_CHN_ENUM chn, GCTRL_BASIC_CFG_T *cfg)
{
    short addr_ofst = chn?0x80:0;
	short val;

    val = hal_spi_read_reg(phy, GCTRL_INIT_CFG_RX1_0+addr_ofst);
	if (cfg->cfg_gain_table_mode)
		val = SET_BIT(val, BIT2);
	else
		val = CLR_BIT(val, BIT2);
	val = SET_BITS(val, BIT0, 2, cfg->cfg_agc_mode);

	if (cfg->cfg_pin_pls_gain_change_sel)
		val = SET_BIT(val, BIT4);
	else
		val = CLR_BIT(val, BIT4);
	
    hal_spi_write_reg(phy, GCTRL_INIT_CFG_RX1_0+addr_ofst, val);
	
	return 0;
}

int GCTRL_SLOW_AGC_CFG(rf_chip_phy_t *phy, TRX_CHN_ENUM chn, GCTRL_SLOW_AGC_CFG_T *cfg)
{
    short addr_ofst = chn?0x80:0;
	short val;

    val = hal_spi_read_reg(phy, AGC_SLOW_ATTACK_CFG_RX1_0+addr_ofst);
	if (cfg->cfg_slow_hbf_overflow_quick_resp)
		val = SET_BIT(val, BIT7);
	else
		val = CLR_BIT(val, BIT7);

	if (cfg->cfg_slow_lmt_overload_quick_resp)
		val = SET_BIT(val, BIT6);
	else
		val = CLR_BIT(val, BIT6);

	if (cfg->cfg_slow_lmt_underload_quick_resp)
		val = SET_BIT(val, BIT5);
	else
		val = CLR_BIT(val, BIT5);

	if (cfg->cfg_slow_adc_overload_lg_quick_resp)
		val = SET_BIT(val, BIT4);
	else
		val = CLR_BIT(val, BIT4);

	if (cfg->cfg_slow_adc_overload_sm_quick_resp)
		val = SET_BIT(val, BIT3);
	else
		val = CLR_BIT(val, BIT3);

	if (cfg->cfg_slow_adc_underload_quick_resp)
		val = SET_BIT(val, BIT2);
	else
		val = CLR_BIT(val, BIT2);

	if (cfg->cfg_slow_pdt_clear_mode)
		val = SET_BIT(val, BIT1);
	else
		val = CLR_BIT(val, BIT1);

	if (cfg->cfg_slow_agc_gain_update_mode_ena)
		val = SET_BIT(val, BIT0);
	else
		val = CLR_BIT(val, BIT0);
    hal_spi_write_reg(phy, AGC_SLOW_ATTACK_CFG_RX1_0+addr_ofst, val);

    hal_spi_write_reg(phy, AGC_SLOW_ATTACK_CFG_RX1_2+addr_ofst, (cfg->cfg_slow_agc_gain_update_counter)&0xff);
    hal_spi_write_reg(phy, AGC_SLOW_ATTACK_CFG_RX1_1+addr_ofst, (cfg->cfg_slow_agc_gain_update_counter >> 8)&0xff);

    val = hal_spi_read_reg(phy, AGC_SLOW_ATTACK_CFG_RX1_3+addr_ofst);
	val = SET_BITS(val, BIT4, 4, cfg->cfg_slow_full_lmt_overload_step);
	val = SET_BITS(val, BIT0, 4, cfg->cfg_slow_full_lmt_underload_step);
    hal_spi_write_reg(phy, AGC_SLOW_ATTACK_CFG_RX1_3+addr_ofst, val);

    val = hal_spi_read_reg(phy, AGC_SLOW_ATTACK_CFG_RX1_4+addr_ofst);
	val = SET_BITS(val, BIT4, 4, cfg->cfg_slow_full_adc_overload_step);
	val = SET_BITS(val, BIT0, 4, cfg->cfg_slow_full_adc_underload_step);
    hal_spi_write_reg(phy, AGC_SLOW_ATTACK_CFG_RX1_4+addr_ofst, val);

    val = hal_spi_read_reg(phy, AGC_SLOW_ATTACK_CFG_RX1_5+addr_ofst);
	val = SET_BITS(val, BIT4, 4, cfg->cfg_slow_calc_time);
	val = SET_BITS(val, BIT0, 4, cfg->cfg_slow_full_hbf_overflow_step);
    hal_spi_write_reg(phy, AGC_SLOW_ATTACK_CFG_RX1_5+addr_ofst, val);

    val = hal_spi_read_reg(phy, AGC_SLOW_ATTACK_CFG_RX1_6+addr_ofst);
	if (cfg->cfg_slow_pin_sync_ena)
		val = SET_BIT(val, BIT7);
	else
		val = CLR_BIT(val, BIT7);
	val = SET_BITS(val, BIT3, 4, cfg->cfg_slow_agc_outter_high_step);
	val = SET_BITS(val, BIT0, 3, cfg->cfg_slow_agc_inner_high_step);
    hal_spi_write_reg(phy, AGC_SLOW_ATTACK_CFG_RX1_6+addr_ofst, val);

    val = hal_spi_read_reg(phy, AGC_SLOW_ATTACK_CFG_RX1_7+addr_ofst);
	val = SET_BITS(val, BIT3, 4, cfg->cfg_slow_agc_outter_low_step);
	val = SET_BITS(val, BIT0, 3, cfg->cfg_slow_agc_inner_low_step);
    hal_spi_write_reg(phy, AGC_SLOW_ATTACK_CFG_RX1_7+addr_ofst, val);

    val = hal_spi_read_reg(phy, AGC_SLOW_ATTACK_CFG_RX1_8+addr_ofst);
	val = SET_BITS(val, BIT6, 2, cfg->cfg_slow_split_lmt_dec_step);
	val = SET_BITS(val, BIT3, 3, cfg->cfg_slow_split_lpf_dec_step);
	val = SET_BITS(val, BIT0, 3, cfg->cfg_slow_split_dig_dec_step);
    hal_spi_write_reg(phy, AGC_SLOW_ATTACK_CFG_RX1_8+addr_ofst, val);

    val = hal_spi_read_reg(phy, AGC_SLOW_ATTACK_CFG_RX1_9+addr_ofst);
	val = SET_BITS(val, BIT6, 2, cfg->cfg_slow_split_lmt_inc_step);
	val = SET_BITS(val, BIT3, 3, cfg->cfg_slow_split_lpf_inc_step);
    hal_spi_write_reg(phy, AGC_SLOW_ATTACK_CFG_RX1_9+addr_ofst, val);

    hal_spi_write_reg(phy, AGC_SLOW_ATTACK_CFG_RX1_10+addr_ofst, cfg->cfg_slow_agc_outter_high_th + 3);
    hal_spi_write_reg(phy, AGC_SLOW_ATTACK_CFG_RX1_11+addr_ofst, cfg->cfg_slow_agc_outter_low_th + 3);
    hal_spi_write_reg(phy, AGC_SLOW_ATTACK_CFG_RX1_12+addr_ofst, cfg->cfg_slow_agc_inner_high_th + 3);
    hal_spi_write_reg(phy, AGC_SLOW_ATTACK_CFG_RX1_13+addr_ofst, cfg->cfg_slow_agc_inner_low_th + 3);


	
	return 0;
}

int GCTRL_FAST_AGC_CFG(rf_chip_phy_t *phy, TRX_CHN_ENUM chn, GCTRL_FAST_AGC_CFG_T *cfg)
{
    short addr_ofst = chn?0x80:0;
    short addr;
    short val;

    addr = AGC_FAST_ATTACK_CFG_RX1_1 + addr_ofst;
    val  = (cfg->cfg_fast_calc_time)&0xf;
    hal_spi_write_reg(phy, addr,val);

    addr = AGC_FAST_ATTACK_CFG_RX1_2 + addr_ofst;
    val  = cfg->cfg_fast_s1_exit_counter &0xFF ;
    hal_spi_write_reg(phy, addr,val);

    addr = AGC_FAST_ATTACK_CFG_RX1_3 + addr_ofst;
    val = (
                ((cfg->cfg_fast_s1_full_large_dec_step  & 0xF ) << 4) |
                ((cfg->cfg_fast_s1_full_medium_dec_step & 0x7 ) << 0)
           );
    hal_spi_write_reg(phy, addr,val);

    addr = AGC_FAST_ATTACK_CFG_RX1_4 + addr_ofst;
    val = (
                ((cfg->cfg_fast_s1_split_lpf_dec_step   & 0x7 ) << 5) |
                ((cfg->cfg_fast_s1_split_lmt_dec_step   & 0x3 ) << 3) |
                ((cfg->cfg_fast_s1_full_small_dec_step  & 0x7 ) << 0)
                );
    hal_spi_write_reg(phy, addr,val);

    addr = AGC_FAST_ATTACK_CFG_RX1_5 + addr_ofst;
    val = (
                ((cfg->cfg_fast_s2_adjustment_method    & 0x3 )<< 3) |
                ((cfg->cfg_fast_s1_split_dig_dec_step   & 0x7 )<< 0)
          );
    hal_spi_write_reg(phy, addr,val);

    addr = AGC_FAST_ATTACK_CFG_RX1_6 + addr_ofst;
    val = (
                ((cfg->cfg_fast_s2_lock_power_target    & 0xFF)<< 0)
           );
    hal_spi_write_reg(phy, addr,val);

    addr = AGC_FAST_ATTACK_CFG_RX1_7 + addr_ofst;
    val = (
                ((cfg->cfg_fast_s2_max_inc_step         & 0x3F)<< 0)
           );
    hal_spi_write_reg(phy, addr,val);

    addr = AGC_FAST_ATTACK_CFG_RX1_8 + addr_ofst;
    val = (
                ((cfg->cfg_fast_s2b_exit_counter        & 0xFF)<< 0)
           );
    hal_spi_write_reg(phy, addr,val);

    addr = AGC_FAST_ATTACK_CFG_RX1_9 + addr_ofst;
    val = (
                ((cfg->cfg_fast_s2a_exit_counter        & 0xFF)<< 0)
           );
    hal_spi_write_reg(phy, addr,val);

    addr = AGC_FAST_ATTACK_CFG_RX1_10 + addr_ofst;
    val = (
                ((cfg->cfg_fast_s3_overrange_counter    & 0xFF)<< 0)
           );
    hal_spi_write_reg(phy, addr,val);

    addr = AGC_FAST_ATTACK_CFG_RX1_11 + addr_ofst;
    val = (
                ((cfg->cfg_fast_s3_confirm_counter      & 0xFF)<< 0)
           );
    hal_spi_write_reg(phy, addr,val);

    addr = AGC_FAST_ATTACK_CFG_RX1_12 + addr_ofst;
    val = (
                ((cfg->cfg_fast_s2_limit_max_adj_step_ena  & 0x1 )<< 6) |
                ((cfg->cfg_fast_s3_dec_step                & 0x7 )<< 3) |
                ((cfg->cfg_fast_s2a_inc_step               & 0x7 )<< 0)
           );
    hal_spi_write_reg(phy, addr,val);

    addr = AGC_FAST_ATTACK_CFG_RX1_13 + addr_ofst;
    val = (
                ((cfg->cfg_fast_s4_avp_meas_num         & 0xF )<< 4) |
                ((cfg->cfg_fast_s2_to_s2a_mask          & 0x1 )<< 3) |
                ((cfg->cfg_fast_s3_to_s2a_mask          & 0x1 )<< 2) |
                ((cfg->cfg_fast_s2_to_s2b_mask          & 0x1 )<< 1) |
                ((cfg->cfg_fast_s2b_to_s2a_mask         & 0x1 )<< 0)
          );
    hal_spi_write_reg(phy, addr,val);

    addr = AGC_FAST_ATTACK_CFG_RX1_14 + addr_ofst;
    val = (
                ((cfg->cfg_fast_unlock_mode_energy_lost & 0x3 )<< 6) |
                ((cfg->cfg_fast_unlock_mode_exist_rx    & 0x3 )<< 4) |
                ((cfg->cfg_fast_unlock_mode_pin_ena     & 0x3 )<< 2) |
                ((cfg->cfg_fast_unlock_mode_spi         & 0x3 )<< 0)
        );
    hal_spi_write_reg(phy, addr,val);

    addr = AGC_FAST_ATTACK_CFG_RX1_15 + addr_ofst;
    val = (
                ((cfg->cfg_fast_set_gain_burst_start_sel & 0x1 )<<4) |
                ((cfg->cfg_fast_opt_gain_offset          & 0xF )<<0)
          );
    hal_spi_write_reg(phy, addr,val);

    addr = AGC_FAST_ATTACK_CFG_RX1_16 + addr_ofst;
    val = (
                ((cfg->cfg_fast_dont_unlock_lmt_pdt_overload    & 0x1 )<<6) |
                ((cfg->cfg_fast_dont_unlock_adc_pdt_overload_lg & 0x1 )<<5) |
                ((cfg->cfg_fast_dont_unlock_lmt_pdt_overload    & 0x1 )<<4) |
                ((cfg->cfg_fast_dont_unlock_adc_pdt_underload   & 0x1 )<<3) |
                ((cfg->cfg_fast_dont_unlock_energy_lost         & 0x1 )<<2) |
                ((cfg->cfg_fast_dont_unlock_energy_boost        & 0x1 )<<1) |
                ((cfg->cfg_fast_dont_unlock_pin_agc_ena         & 0x1 )<<0)
          );
    hal_spi_write_reg(phy, addr,val);

    addr = AGC_FAST_ATTACK_CFG_RX1_17 + addr_ofst;
    val = (
                ((cfg->cfg_fast_energy_lost_threshold_db        & 0x3F )<<0)
           );
    hal_spi_write_reg(phy, addr,val);

    addr = AGC_FAST_ATTACK_CFG_RX1_18 + addr_ofst;
    val = (
                ((cfg->cfg_fast_energy_boost_threshold_db       & 0x3F )<<0)
           );
    hal_spi_write_reg(phy, addr,val);

    addr = AGC_FAST_ATTACK_CFG_RX1_19 + addr_ofst;
    val = (
                ((cfg->cfg_fast_energy_change_exit_counter      & 0xFF )<<0)
           );
    hal_spi_write_reg(phy, addr,val);

    addr = AGC_FAST_ATTACK_CFG_RX1_20 + addr_ofst;
    val = (
                ((cfg->cfg_fast_gain_lock_hold_time             & 0xFF )<<0)
           );
    hal_spi_write_reg(phy, addr,val);

	return 0;
}

int GCTRL_MGC_CFGG(rf_chip_phy_t *phy, TRX_CHN_ENUM chn, GCTRL_MGC_CFGG_T *cfg)
{
    short addr_ofst = chn?0x80:0;
	short val;

    val = hal_spi_read_reg(phy, MGC_CFG_RX1_0+addr_ofst);
	if (cfg->cfg_mgc_mode)
		val = SET_BIT(val, BIT0);
	else
		val = CLR_BIT(val, BIT0);

	if (cfg->cfg_mgc_adj_gain_pos_auto)
		val = SET_BIT(val, BIT2);
	else
		val = CLR_BIT(val, BIT2);

	if (cfg->cfg_mgc_adj_gain_pos_lmt)
		val = SET_BIT(val, BIT1);
	else
		val = CLR_BIT(val, BIT1);

    hal_spi_write_reg(phy, MGC_CFG_RX1_0+addr_ofst, val);

    val = hal_spi_read_reg(phy, MGC_CFG_RX1_1+addr_ofst);
	val = SET_BITS(val, BIT3, 3, cfg->cfg_mgc_inc_step);
	val = SET_BITS(val, BIT0, 3, cfg->cfg_mgc_dec_step);
    hal_spi_write_reg(phy, MGC_CFG_RX1_1+addr_ofst, val);

	
	return 0;
}

int GCTRL_MGC_SET_FULL_GAIN(rf_chip_phy_t *phy, TRX_CHN_ENUM chn, short full_tab_inx)
{
    short addr_ofst = chn?0x80:0;
    short val = full_tab_inx;
    short addr;

    addr = MGC_CFG_RX1_4 + addr_ofst;
    hal_spi_write_reg(phy, addr, val);
	
    addr = MGC_CFG_RX1_0 + addr_ofst;
    val = hal_spi_read_reg(phy, addr);

    // write 1 + write 0 to make index work.
	val = ((val & 0x7f) | 0x80);
    hal_spi_write_reg(phy, addr, val);

	val = (val & 0x7f);
    hal_spi_write_reg(phy, addr, val);
	
	return 0;
}


int GCTRL_MGC_SET_SPLIT_GAIN(rf_chip_phy_t *phy, TRX_CHN_ENUM chn, GCTRL_SPLIT_GAIN_T *cfg)
{
    short val;
    short addr;
    short addr_ofst = chn?0x80:0;

    addr = MGC_CFG_RX1_2 + addr_ofst;
	val = (cfg->cfg_mgc_lna_index << 7) | (cfg->cfg_mgc_lmt_index << 4) | (cfg->cfg_mgc_lpf_index);
    hal_spi_write_reg(phy, addr, val);
	
    addr = MGC_CFG_RX1_0 + addr_ofst;
    val = hal_spi_read_reg(phy, addr);

	val = ((val & 0x7f) | 0x80);
    hal_spi_write_reg(phy, addr, val);

	val = (val & 0x7f);
    hal_spi_write_reg(phy, addr, val);

	return 0;

}

int GCTRL_MGC_SET_DIG_GAIN(rf_chip_phy_t *phy, TRX_CHN_ENUM chn, short val)
{
    short addr;
    short addr_ofst = chn?0x80:0;

    addr = MGC_CFG_RX1_3 + addr_ofst;
    hal_spi_write_reg(phy, addr, val);
	
    addr = MGC_CFG_RX1_0 + addr_ofst;
    val = hal_spi_read_reg(phy, addr);

	val = ((val & 0x7f) | 0x80);
    hal_spi_write_reg(phy, addr, val);

	val = (val & 0x7f);
    hal_spi_write_reg(phy, addr, val);

	return 0;

}



int GCTRL_FAST_SET_FULL_GAIN(rf_chip_phy_t *phy, TRX_CHN_ENUM chn, short full_tab_inx)
{
    short addr_ofst = chn?0x80:0;
    short val = full_tab_inx;
    short addr;

    addr = GCTRL_INIT_CFG_RX1_3 + addr_ofst;
    hal_spi_write_reg(phy, addr, val);

    addr = MGC_CFG_RX1_0 + addr_ofst;

    // write 1 + write 0 to make index work.
    val = 0x80;
    hal_spi_write_reg(phy, addr, val);

    val = 0x00;
    hal_spi_write_reg(phy, addr, val);

    return 0;
}


int GCTRL_FAST_SET_SPLIT_GAIN(rf_chip_phy_t *phy, TRX_CHN_ENUM chn, GCTRL_SPLIT_GAIN_T *cfg)
{
    short val;
    short addr;
    short addr_ofst = chn?0x80:0;

    addr = GCTRL_INIT_CFG_RX1_1 + addr_ofst;
    val = (cfg->cfg_mgc_lna_index << 7) | (cfg->cfg_mgc_lmt_index << 4) | (cfg->cfg_mgc_lpf_index);
    hal_spi_write_reg(phy, addr, val);

    addr = MGC_CFG_RX1_0 + addr_ofst;

    // write 1 + write 0 to make index work.
    val = 0x80;
    hal_spi_write_reg(phy, addr, val);

    val = 0x00;
    hal_spi_write_reg(phy, addr, val);

    return 0;

}

int GCTRL_FAST_SET_DIG_GAIN(rf_chip_phy_t *phy, TRX_CHN_ENUM chn, short val)
{
    short addr;
    short addr_ofst = chn?0x80:0;

    addr = GCTRL_INIT_CFG_RX1_2 + addr_ofst;
    hal_spi_write_reg(phy, addr, val);

    addr = MGC_CFG_RX1_0 + addr_ofst;

    // write 1 + write 0 to make index work.
    val = 0x80;
    hal_spi_write_reg(phy, addr, val);

    val = 0x00;
    hal_spi_write_reg(phy, addr, val);
    return 0;

}

int GCTRL_GET_RPT(rf_chip_phy_t *phy, TRX_CHN_ENUM chn, GCTRL_GET_RPT_T *rpt)
{
	short addr_ofst = chn?0x80:0;
	short val;

	val = hal_spi_read_reg(phy, GCTRL_RPT_RX1_0+addr_ofst);
	val = CLR_BIT(val, BIT7);
	hal_spi_write_reg(phy, GCTRL_RPT_RX1_0+addr_ofst, val);
	val = SET_BIT(val, BIT7);
	hal_spi_write_reg(phy, GCTRL_RPT_RX1_0+addr_ofst, val);
	val = CLR_BIT(val, BIT7);
	hal_spi_write_reg(phy, GCTRL_RPT_RX1_0+addr_ofst, val);

	val = hal_spi_read_reg(phy, GCTRL_RPT_RX1_1+addr_ofst);
	rpt->rpt_adc_flg_overload_lg = GET_BIT(val, BIT3);
	rpt->rpt_adc_flg_overload_sm = GET_BIT(val, BIT2);
	rpt->rpt_lmt_flg_overload    = GET_BIT(val, BIT1);
	rpt->rpt_hbf_flg_overflow    = GET_BIT(val, BIT0);

	val = hal_spi_read_reg(phy, GCTRL_RPT_RX1_2+addr_ofst);
	rpt->rpt_hbf_flg_low_power   = GET_BIT(val, BIT7);
	rpt->rpt_adc_flg_prevent_inc = GET_BIT(val, BIT6);
	rpt->rpt_lmt_flg_prevent_inc = GET_BIT(val, BIT5);
	rpt->rpt_lmt_flg_underload   = GET_BIT(val, BIT4);
	rpt->rpt_adc_flg_underload   = GET_BIT(val, BIT3);
	rpt->rpt_lmt_pdt_value       = GET_BITS(val, BIT0, 2) <<8;

	val = hal_spi_read_reg(phy, GCTRL_RPT_RX1_3+addr_ofst);
	rpt->rpt_lmt_pdt_value = rpt->rpt_lmt_pdt_value + val;

	rpt->rpt_hbf_avp = hal_spi_read_reg(phy, GCTRL_RPT_RX1_4+addr_ofst) - 6;

	val = hal_spi_read_reg(phy, GCTRL_RPT_RX1_5+addr_ofst);
	rpt->rpt_lna_index = GET_BIT(val, BIT7);
	rpt->rpt_lmt_index = GET_BITS(val, BIT4, 3);
	rpt->rpt_lpf_index = GET_BITS(val, BIT0, 4);

	rpt->rpt_dig_index = hal_spi_read_reg(phy, GCTRL_RPT_RX1_6+addr_ofst);
	rpt->rpt_full_index = hal_spi_read_reg(phy, GCTRL_RPT_RX1_7+addr_ofst);

	return 0;
}

int GCTRL_GET_SPLIT_GAIN(rf_chip_phy_t *phy, TRX_CHN_ENUM chn, short *lmt_index, short *lpf_index)
{
    short addr_ofst     = chn?0x80:0;
    unsigned char val   = 0;
    
    val = hal_spi_read_reg(phy, GCTRL_RPT_RX1_5+addr_ofst);
    *lmt_index = GET_BITS(val, BIT4, 3);
    *lpf_index = GET_BITS(val, BIT0, 4);

    return 0;
}

int GCTRL_ANA_PEAK_DETECT_ENABLE(rf_chip_phy_t *phy, TRX_CHN_ENUM chn)
{
    unsigned char val;

    testmux_clear(phy);    
    hal_spi_write_reg(phy, 0xD2E,0x00);
    hal_spi_write_reg(phy, 0x803,0x0F);
    //hal_spi_write_reg(phy, 0x6f1,0x1B);
    if(chn == TRX_CHN1)
    {
        hal_spi_write_reg(phy, 0x861,0x30);
        hal_spi_write_reg(phy, 0x861,0x00);
        hal_spi_write_reg(phy, 0x861,0x00);
        hal_spi_write_reg(phy, 0x861,0x00);
        hal_spi_write_reg(phy, 0x861,0x30);
        hal_spi_write_reg(phy, 0x861,0x71);
    }
    else
    {
        hal_spi_write_reg(phy, 0x86a,0x30);
        hal_spi_write_reg(phy, 0x86a,0x00);
        hal_spi_write_reg(phy, 0x86a,0x00);
        hal_spi_write_reg(phy, 0x86a,0x00);
        hal_spi_write_reg(phy, 0x86a,0x30);
        hal_spi_write_reg(phy, 0x86a,0x71);
    }

    if (chn == TRX_CHN1)
    {
        hal_spi_write_reg(phy, 0x860,0x00);
        hal_spi_write_reg(phy, 0x865,0x80);
        hal_spi_write_reg(phy, 0x865,0x00);
    }
    else
    {
        hal_spi_write_reg(phy, 0x869,0x00);
        hal_spi_write_reg(phy, 0x86e,0x80);
        hal_spi_write_reg(phy, 0x86e,0x00);
    }

    val = hal_spi_read_reg(phy, 0x82c);
    if (chn == TRX_CHN1)
    {
        val = (val & 0x7A) | 0x84;
    }
    else
    {
        val = (val & 0x97) | 0x48;
    }

    hal_spi_write_reg(phy, 0x82c,val);

    hal_spi_write_reg(phy, 0x835,0x00);

    val = hal_spi_read_reg(phy, 0x843);
    if (chn == TRX_CHN1)
    {
        val = (val & 0xEC ) | 0x12;
    }
    else
    {
        val = (val & 0xB3 ) | 0x48;
    }
    hal_spi_write_reg(phy, 0x843,val);

    val = TRANSCV_RXPOWDTR_RXPDET_EN_MASK;

    if (chn == TRX_CHN1)
    {
        val |= (TRANSCV_RXPOWDTR_AUXADC1_EN_MASK| TRANSCV_BQOPISEL_BQ20P2_SET);      
        hal_spi_write_reg(phy, 0x668,val);        
    }
    else
    {

        val |= (TRANSCV_RXPOWDTR_AUXADC2_EN_MASK| TRANSCV_BQOPISEL_BQ20P2_SET); 
        hal_spi_write_reg(phy, 0x686, val);
    }

    hal_spi_write_reg(phy, 0xD35,0x00);
    
    return 0;

}

void GCTRL_DEFAULT_CFG(rf_chip_phy_t *phy, RX_GCTRL_CFG_T * gctrl_cfg)
{
    short i;
    short lmt_index;
    short lpf_index;
    short *p;

    if (gctrl_cfg == NULL)
    {
        LOG_ERROR("%s REPORTED ERR in line %d for NULL PTR\n", __FUNCTION__, __LINE__);
        return;
    }

    memcpy((void *)gctrl_cfg, &gctrl_dflt_cfg, sizeof(RX_GCTRL_CFG_T));
        
    // Full gain table setting
    for(i=0;i<=gctrl_cfg->gain_full.max_index;i++)
    {
        lmt_index = (i / 6);
        lmt_index = (lmt_index > 5)? 5:lmt_index;
        lpf_index = i - 6*lmt_index;
        gctrl_cfg->gain_full.content[i] =   (lmt_index<<4) + lpf_index;
    }  
}

void GCTRL_CFG(rf_chip_phy_t *phy, TRX_CHN_ENUM chn,RX_GCTRL_CFG_T * gctrl_cfg)
{
	int addr;
	short gain_inx;	
	short ex_lna;
	short lmt_inx;
	short lpf_inx;	
	
    GCTRL_BASIC_CFG(phy, chn , &gctrl_cfg->basic);
    GCTRL_SETUP_DELAY_CFG(phy, chn,&gctrl_cfg->delay);

	if(gctrl_cfg->ext_lna.cfg_split_ext_lna_ena==1)
    {
        GCTRL_EXT_LNA_CFG(phy, chn,&gctrl_cfg->ext_lna);
    }
	
    if(gctrl_cfg->basic.cfg_gain_table_mode==0)
    {
		GCTRL_FULL_TABLE_CFG(phy, chn,&gctrl_cfg->gain_full);
		LOG_INFO("Dump full tab start\r\n");
		for(addr = 0; addr <= gctrl_cfg->gain_full.max_index; addr++)
		{
			GCTRL_FULL_TABLE_RPT_CONTENT(phy, chn, addr, &gain_inx);
			ex_lna = ((gain_inx & 0x80) >> 7);
			lmt_inx = ((gain_inx & 0x70) >> 4);
			lpf_inx = (gain_inx & 0x0f);
			LOG_INFO("addr:%d, ex_lna:%d, lmt_inx:%d, lpf_inx:%d\r\n", addr, ex_lna, lmt_inx, lpf_inx);
		}
		LOG_INFO("Dump full tab end\r\n");
    }
    else
    {
        GCTRL_SPLIT_TABLE_CFG(phy, chn,&gctrl_cfg->gain_split);
    }

    GCTRL_LMT_PDT_CFG(phy, chn,&gctrl_cfg->lmt_pdt);
    GCTRL_ADC_PDT_CFG(phy, chn,&gctrl_cfg->adc_pdt);
    GCTRL_HBF_PWR_MEAS_CFG(phy, chn,&gctrl_cfg->hbf_pwr);
    GCTRL_HBF_LPWR_DET_CFG(phy, chn,&gctrl_cfg->hbf_lpwr);
    if(gctrl_cfg->gain_digital.cfg_digital_gain_ena==0)
    {
        GCTRL_DIG_AGC_ENA(phy, chn,&gctrl_cfg->gain_digital);
        GCTRL_HBF_OVF_DET_CFG(phy, chn,&gctrl_cfg->hbf_ovf);
    }

    if(gctrl_cfg->basic.cfg_agc_mode==0)
    {
        GCTRL_MGC_CFGG(phy, chn,&gctrl_cfg->mgc);
    }
    else if(gctrl_cfg->basic.cfg_agc_mode==1)
    {
        GCTRL_SLOW_AGC_CFG(phy, chn,&gctrl_cfg->agc_slow);
    }
    else if(gctrl_cfg->basic.cfg_agc_mode==2)
    {
        GCTRL_FAST_AGC_CFG(phy, chn,&gctrl_cfg->agc_fast);
		hal_spi_write_reg(phy, 0x0D7,0x8B); // close auto-reset.
    }

}

int GCTRL_DEMO_MGC_SPI_MODE(rf_chip_phy_t *phy, TRX_CHN_ENUM chn, TABLE_MODE_ENUM mode)
{
    RX_GCTRL_CFG_T gctrl_cfg;
    GCTRL_DEFAULT_CFG(phy, & gctrl_cfg);

    gctrl_cfg.basic.cfg_agc_mode = 0;
    gctrl_cfg.mgc.cfg_mgc_mode   = 0;
    if(mode == FULL_TBL)
    	gctrl_cfg.basic.cfg_gain_table_mode = 0;
    else
    	gctrl_cfg.basic.cfg_gain_table_mode = 1;

    GCTRL_CFG(phy, chn,&gctrl_cfg);
    return 0;
}

int GCTRL_DEMO_MGC_PIN_MODE(rf_chip_phy_t *phy, TRX_CHN_ENUM chn, TABLE_MODE_ENUM mode)
{
    RX_GCTRL_CFG_T gctrl_cfg;
    GCTRL_DEFAULT_CFG(phy, & gctrl_cfg);

    gctrl_cfg.basic.cfg_agc_mode        = 0;
    gctrl_cfg.mgc.cfg_mgc_mode          = 1;
    if(mode == FULL_TBL)
    	gctrl_cfg.basic.cfg_gain_table_mode = 0;
    else
    	gctrl_cfg.basic.cfg_gain_table_mode = 1;

    GCTRL_CFG(phy, chn,&gctrl_cfg);
    return 0;
}


int GCTRL_DEMO_AGC_SLOW(rf_chip_phy_t *phy, TRX_CHN_ENUM chn, TABLE_MODE_ENUM mode)
{
    RX_GCTRL_CFG_T gctrl_cfg;
    GCTRL_DEFAULT_CFG(phy, & gctrl_cfg);

    gctrl_cfg.basic.cfg_agc_mode = 1;
    if(mode == FULL_TBL)
    	gctrl_cfg.basic.cfg_gain_table_mode = 0;
    else
    	gctrl_cfg.basic.cfg_gain_table_mode = 1;

    // set target  to -9 ~ -11 dBfs
    gctrl_cfg.agc_slow.cfg_slow_agc_outter_high_th                = 3;
    gctrl_cfg.agc_slow.cfg_slow_agc_inner_high_th                 = 4;
    gctrl_cfg.agc_slow.cfg_slow_agc_inner_low_th                  = 5;
    gctrl_cfg.agc_slow.cfg_slow_agc_outter_low_th                 = 6;
    GCTRL_CFG(phy, chn,&gctrl_cfg);
    return 0;
}

int GCTRL_DEMO_AGC_FAST(rf_chip_phy_t *phy, TRX_CHN_ENUM chn, TABLE_MODE_ENUM mode)
{
    RX_GCTRL_CFG_T gctrl_cfg;
    GCTRL_DEFAULT_CFG(phy, & gctrl_cfg);

    gctrl_cfg.basic.cfg_agc_mode = 2;
    if(mode == FULL_TBL)
    	gctrl_cfg.basic.cfg_gain_table_mode = 0;
    else
    	gctrl_cfg.basic.cfg_gain_table_mode = 1;

    GCTRL_CFG(phy, chn,&gctrl_cfg);

    return 0;
}

int GCTRL_DEMO_AGC_FAST_HHH(rf_chip_phy_t *phy, TRX_CHN_ENUM chn, TABLE_MODE_ENUM mode)
{
    RX_GCTRL_CFG_T gctrl_cfg;
    GCTRL_DEFAULT_CFG(phy, &gctrl_cfg);

    gctrl_cfg.basic.cfg_agc_mode                = 2;
    gctrl_cfg.basic.cfg_gain_table_mode         = 0;
    gctrl_cfg.lmt_pdt.cfg_lmt_overload_ena      = 1;
    gctrl_cfg.lmt_pdt.cfg_lmt_prevent_inc_ena   = 0;
    gctrl_cfg.lmt_pdt.cfg_lmt_underload_ena     = 0;
    gctrl_cfg.lmt_pdt.cfg_lmt_prevent_inc_ena   = 0;

    gctrl_cfg.adc_pdt.cfg_adc_overload_lg_ena            = 1;
    gctrl_cfg.adc_pdt.cfg_adc_overload_lg_threshold      = (short)(    pow(10,-2.0/20)*255.0 );
    gctrl_cfg.adc_pdt.cfg_adc_overload_lg_exceed_counter = 10;

    gctrl_cfg.adc_pdt.cfg_adc_overload_sm_ena   = 0;
    gctrl_cfg.adc_pdt.cfg_adc_overload_sm_threshold      = (short)(    pow(10,-2.0/20)*255.0 );
    gctrl_cfg.adc_pdt.cfg_adc_overload_sm_exceed_counter = 50;

    gctrl_cfg.adc_pdt.cfg_adc_prevent_inc_ena            = 1;
    gctrl_cfg.adc_pdt.cfg_adc_prevent_inc_threshold      = (short)(    pow(10,-5.0/20)*255.0 );
    gctrl_cfg.adc_pdt.cfg_adc_prevent_inc_exceed_counter = 50;

    gctrl_cfg.adc_pdt.cfg_adc_underload_ena              = 1;
    gctrl_cfg.adc_pdt.cfg_adc_underload_threshold        = (short)(    pow(10,-30.0/20)*255.0 );
    gctrl_cfg.adc_pdt.cfg_adc_underload_exceed_counter   = 250;
    gctrl_cfg.adc_pdt.cfg_adc_underload_power_coeff      = 3;

    gctrl_cfg.hbf_ovf.cfg_hbf_ovf_ena           = 0;
    gctrl_cfg.hbf_lpwr.cfg_hbf_low_pwr_ena      = 0;
    gctrl_cfg.hbf_pwr.cfg_hbf_avp_ena           = 0;
    gctrl_cfg.hbf_pwr.cfg_hbf_avp_meas_num      = 15;


    GCTRL_CFG(phy, chn,&gctrl_cfg);

    return 0;
}

