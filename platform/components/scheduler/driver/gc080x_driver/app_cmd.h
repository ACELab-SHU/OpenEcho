
#ifndef  __APP_CMD_H
#define  __APP_CMD_H

#include "platform.h"
#include    "stdlib.h"

typedef struct chip_cmd_struct
{
	char cmd[50];
	int param;
	void (*fun)(char *argv[]);
}chip_cmd_t;

/*----------------------------------------------------------------------------------------------*/
// iio API
/*----------------------------------------------------------------------------------------------*/
extern void CUSTOMER_CMD_API_chip_config_set(chip_config_t *config);
extern void CUSTOMER_CMD_API_chip_config_read(chip_config_t *config);
extern int CUSTOMER_CMD_API_chip_init(void);
extern int CUSTOMER_CMD_API_calibr_buffer_del(void);
extern int CUSTOMER_CMD_API_rf_bandwith_change(BANDWITH_ENUM bandwith);
extern int CUSTOMER_CMD_API_trx_lo_change(unsigned long long  txlo, unsigned long long  rxlo, unsigned char rxdc_offst_cal_flag, unsigned char qec_flag);
extern int CUSTOMER_CMD_API_lo_change_ready(unsigned char lo_change_mode, unsigned long long  txlo, unsigned long long  rxlo);
extern int CUSTOMER_CMD_API_lo_change_act(void);
extern int CUSTOMER_CMD_API_tx_atten_change(TRX_CHN_ENUM chn, int val, short immed);
extern int CUSTOMER_CMD_API_tx_split_atten_change(TRX_CHN_ENUM chn, TX_SPLIT_ATTEN_ENUM type, unsigned int val, short immed);
extern int CUSTOMER_CMD_API_tdd_trx_change(FSM_ST_ENUM st);
extern int CUSTOMER_CMD_API_rx_gain_mgc_change(TRX_CHN_ENUM chn, RX_MGC_GAIN_ENUM tb, int val);
extern int CUSTOMER_CMD_API_tx_port_enable(TRX_CHN_ENUM chn, short en);
extern int CUSTOMER_CMD_API_rx_port_enable(TRX_CHN_ENUM chn, short en);
extern int CUSTOMER_CMD_API_tx_dig_atten_change(TRX_CHN_ENUM chn, unsigned short index);
extern int CUSTOMER_CMD_API_rf_chip_suspend(void);
extern int CUSTOMER_CMD_API_rf_chip_resume(void);
extern void CUSTOMER_CMD_API_test_tx_fir_coeff_clear(TRX_CHN_ENUM chn);
extern void CUSTOMER_CMD_API_test_adc_dac_off(short dir, TRX_CHN_ENUM chn);
extern void CUSTOMER_CMD_API_trx_ana_bw_adj(short dir, TRX_CHN_ENUM chn, unsigned char val[]);
extern void CUSTOMER_CMD_API_fir_coef_set(short dir, TRX_CHN_ENUM chn, int count, unsigned long long fir_coef[]);
extern void CUSTOMER_CMD_API_fpga_tail_set(short dir, TRX_CHN_ENUM chn);


/*----------------------------------------------------------------------------------------------*/
// debug API step by step
/*----------------------------------------------------------------------------------------------*/
extern int cmd_api_detect_chip(void);
extern void cmd_api_rx_imb_set(unsigned char rmb[]);
extern void cmd_api_error_set(short val);
extern short cmd_api_error_get(void);
extern void cmd_api_phy_obj_init(short sel);
extern void cmd_api_module_debug_on(unsigned long on);
extern void cmd_api_write_reg(unsigned short reg, unsigned char val);
extern unsigned char cmd_api_read_reg(unsigned short reg);
extern void cmd_api_fwrite_reg(unsigned int reg, unsigned int val);
extern unsigned int cmd_api_fread_reg(unsigned int reg);
extern int cmd_api_read_lut_byte(LUT_INDEX_ENUM lut, short index, unsigned short reg, int lut_addr, unsigned char offset, unsigned char *pval);
extern int cmd_api_write_lut_byte(LUT_INDEX_ENUM lut, short index, unsigned short reg, int lut_addr, unsigned char offset, unsigned char val);
extern int cmd_api_read_lut_word(LUT_INDEX_ENUM lut, int lut_addr, unsigned char vals[]);
extern int cmd_api_write_lut_word(LUT_INDEX_ENUM lut, int lut_addr, unsigned char vals[]);
extern short cmd_api_power_init(void);
extern void cmd_api_rcal(int rcal_read);
extern void cmd_api_sys_clock_init(unsigned long freq, unsigned long long vco_freq);
extern short cmd_api_lut_load(char mode, char use_hybrid, char hybrid_mode);
extern void cmd_api_dig_trx_filter_use_fir(BANDWITH_ENUM bw);
extern void cmd_api_x4_enable(short enable, short flag);
extern void cmd_api_fvco_min(unsigned long long fvco_min);
extern void cmd_api_analog_init(CHIP_MODE_ENUM mode);
extern void cmd_api_digtal_init(DIG_IF_ENUM dif, BANDWITH_ENUM bw, IF_TYPE_ENUM port, DATA_RATE_ENUM rate, short step);
extern void cmd_api_trx_lut_load(unsigned long long txlo, BANDWITH_ENUM bw);
extern void cmd_api_custom_bw_init(short flag, unsigned long bw, short bw_index, unsigned long bb_sample_rate, unsigned char dac_div, unsigned char adc_div);
extern void cmd_api_wait_init(void);
extern void cmd_api_band_dep_calibr_start(void);
extern void cmd_api_band_dep_calibr_flag_clear(void);
extern void cmd_api_trx_bw_lut_load(void);
extern void cmd_api_band_dep_calflag_set(void);
extern void cmd_api_band_dep_calibr_end(void);
extern void cmd_api_misc_init(void);
extern void cmd_api_tx_atten_init(char index);
extern void cmd_api_dig_fir_cfg_manual(char manual_on);
extern void cmd_api_rf_bandwidth_set(BANDWITH_ENUM bw);
extern void cmd_api_rx_port(TRX_CHN_ENUM chn, RX_PORT_ENUM port, RXFE_GAIN_ENUM gain, short en);
extern void cmd_api_rx_port_man(TRX_CHN_ENUM chn, RX_PORT_ENUM port, RXFE_GAIN_ENUM gain);
extern void cmd_api_tx_port(TRX_CHN_ENUM chn, TX_PORT_ENUM port, short en);
extern void cmd_api_fsm_init(void);
extern void cmd_api_manual_enable(short on);
extern void cmd_api_tx_atten(unsigned char val, short immed);
extern void cmd_api_tx_atten_chn(int chn, unsigned char val, short immed);
extern void cmd_api_tx_dig_atten(TRX_CHN_ENUM chn, unsigned short index);
extern void cmd_api_rx_mgc_gain(TRX_CHN_ENUM chn, RX_MGC_GAIN_ENUM tb, unsigned char val);
extern void cmd_api_rx_mgc_max_gain(TRX_CHN_ENUM chn);
extern void cmd_api_fdd_force_wait(void);
extern void cmd_api_fdd_wait_to_alert(void);
extern void cmd_api_fdd_alert_to_fsm(void);
extern void cmd_api_fdd_fsm_to_alert(void);
extern void cmd_api_wire_control_en(short en, WIRE_CTRL_ENUM pulse);
extern void cmd_api_tdd_wait_to_alert(void);
extern void cmd_api_tdd_alert_to_rx(void);
extern void cmd_api_tdd_rx_to_wait(void);
extern void cmd_api_tdd_alert_to_tx(void);
extern void cmd_api_tdd_tx_to_wait(void);
extern void cmd_api_rxlo_fsm(unsigned long long flo);
extern void cmd_api_txlo_fsm(unsigned long long flo, short core2_en);
extern void cmd_api_sw_cal_set_reg(char reg901_val, char reg639_bit1, char reg61A_val, char reg600_val, char reg602_val);
extern void cmd_api_trx_lo_cal_mode_set(TRX_LO_CAL_MODE_ENUM trx_lo_cal_mode);
extern void cmd_api_auxadc_lock_status_vol_range_set(unsigned int vol_low_limit, unsigned int vol_up_limit);
extern void cmd_api_auxadc_lock_status_vol_range_set_ext(unsigned int vol_low_limit, unsigned int vol_up_limit, unsigned int vol_margin, unsigned long long fvco_limit);
extern void cmd_api_rxlo_set(TRX_CHN_ENUM chn, unsigned long long flo);
extern void cmd_api_txlo_set(TRX_CHN_ENUM chn, unsigned long long flo);
extern void cmd_api_sx_temperature_track_set(TRX_ENUM trx, TRX_CHN_ENUM chn, SX_TEMP_TRACK_MODE_ENUM track_mode);
extern void cmd_api_rxadc_on(TRX_CHN_ENUM chn, short en);
extern void cmd_api_rxifbuf_on(TRX_CHN_ENUM chn, short en);
extern void cmd_api_fcal_s2_bypass(char en);
extern void cmd_api_core2_s7_s8_s10_s11_s12_bypass(char en);
extern void cmd_api_sx_cal(void);
extern void cmd_api_txlo_cal(void);
extern int cmd_api_txdc_offset_cal(TRX_CHN_ENUM chn);
extern void cmd_api_rxadc_cal(TRX_CHN_ENUM chn);
extern void cmd_api_txdac_cal(TRX_CHN_ENUM chn);
extern short cmd_api_rxdc_offset_cal(TRX_CHN_ENUM chn);
extern int cmd_api_rx_imbalance_cal(TRX_CHN_ENUM chn);
extern void cmd_api_rx_bw_cal(TRX_CHN_ENUM chn, BANDWITH_ENUM bandwith, RX_PORT_ENUM port);
extern int cmd_api_rx_rssi_get(TRX_CHN_ENUM chn);
extern int cmd_api_rxqec_cal(TRX_CHN_ENUM chn, int ext_loop);
extern int cmd_api_txqec_cal(TRX_CHN_ENUM chn, int ext_loop, int qec_dbfs, int lol_dbfs);
extern int cmd_api_qec_tracking_cal(TRX_CHN_ENUM chn, int start, int th);
extern int cmd_api_rx_dc_tracking_cal(TRX_CHN_ENUM chn, int start, int tia, int debug);
extern void cmd_api_tx_tone(TRX_CHN_ENUM chn, short on, long freq);
extern int cmd_api_ldo_cal(int *ref_voltage, int cnt);
extern void cmd_api_txdc_digital_remove(TRX_CHN_ENUM chn);
extern void cmd_api_error(void);
extern void cmd_api_print_config(short sel);
extern int cmd_api_intemp_get(void);
extern short cmd_api_lock_status(short dir);
extern void cmd_api_adc_ram_dump(TRX_CHN_ENUM chn, char *name);
extern void cmd_api_rpt_get(TRX_CHN_ENUM chn);
extern void cmd_api_agc_mode_set(TRX_CHN_ENUM chn, short gain_mode, short tab_mode);
extern void cmd_api_rx_gain_full_tab_init(TRX_CHN_ENUM chn, short init_index, short max_index);
extern void cmd_api_rx_gain_mgc_full_tab_index_set(TRX_CHN_ENUM chn, short full_tab_inx);
extern void cmd_api_print_cmd(void);
extern void cmd_api_lvds_cal(void);
extern void cmd_api_chip_ver_select(char sel);
extern void cmd_api_tx_qec_gain_set(TRX_CHN_ENUM chn, short val);
extern int cmd_api_extpin_vol_get(void);
extern int cmd_api_rxgain_force_valid(char enable);
extern int cmd_api_rxgain_set(TRX_CHN_ENUM chn, char enable, TABLE_MODE_ENUM split, GCTRL_MODE_ENUM ctrl);

extern int tdd_lo_change_demo(unsigned char enable, unsigned char lo_change_mode, unsigned long long  lo1, unsigned long long  lo2, int delay);
extern void cmd_api_fir_dump(rf_chip_phy_t *phy,bool rx_select);

#endif /* __APP_CMD_H */

