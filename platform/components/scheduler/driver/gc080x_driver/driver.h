#ifndef  __DRIVER_H
#define  __DRIVER_H

#include "platform.h"


#define RX_DC_OFFSET_CAL_WITH_DEFAULT  (0)
#define RX_DC_OFFSET_CAL_WITH_3POINT_V2  (1)
#define RX_DC_OFFSET_CAL_WITH_3POINT_V3  (2)
#define RX_DC_OFFSET_CAL_USED_VER  RX_DC_OFFSET_CAL_WITH_DEFAULT


extern int rxqec_cal(rf_chip_phy_t *phy, TRX_CHN_ENUM chn);
extern int txqec_cal(rf_chip_phy_t *phy, TRX_CHN_ENUM chn);
extern int txqec_cal_rflp(rf_chip_phy_t *phy, TRX_CHN_ENUM chn);

extern int rx_cfg_rssi_read(rf_chip_phy_t *phy, int channel);
extern int rx_rssi_get(rf_chip_phy_t *phy, int channel);
extern void  rx_cfg_rssi_init(rf_chip_phy_t *phy, int channel);

extern short rx_bw_cal(rf_chip_phy_t *phy, TRX_CHN_ENUM chn);
extern void init_ctune(rf_chip_phy_t *phy, int channel, BANDWITH_ENUM bandwith);
extern void rx_bw_config_tone_tx(rf_chip_phy_t *phy, int chnl, BANDWITH_ENUM bw, CHIP_MODE_ENUM mode);
extern void if_loopback_rxfe_off(rf_chip_phy_t *phy);
extern int get_dc_power(rf_chip_phy_t *phy, int channel, int *power);
//extern int rx_bw_get_power(TRX_CHN_ENUM channel, unsigned short rx_adc_reg1, unsigned short rx_adc_reg2);
extern int get_edge_power(rf_chip_phy_t *phy, int channel, int *power);
extern void send_cordic_signal(rf_chip_phy_t *phy, int channel, BANDWITH_ENUM bandwith, short en, long freq);
extern void send_edge_signal(rf_chip_phy_t *phy, int channel, BANDWITH_ENUM bandwith);
extern void stop_edge_signal(rf_chip_phy_t *phy, int channel);
extern short rx_bw_lut_update(rf_chip_phy_t *phy, BANDWITH_ENUM bw, unsigned short rx_ctune_reg1, unsigned short rx_ctune_reg2);
extern int do_rx_bw_cal(rf_chip_phy_t *phy, int channel, long long half_power, unsigned int rx_ctune_reg1, unsigned int rx_ctune_reg2, unsigned int rx_adc_reg1, unsigned int rx_adc_reg2, int goertzle_wb_div32);
extern short trx_bw_lut_load(rf_chip_phy_t *phy, BANDWITH_ENUM bw, short custom_bw_flag);
extern long get_goertzle_power(rf_chip_phy_t *phy, int channel, long long flo);

extern int sx_cal_lut_update(rf_chip_phy_t *phy);
extern int txlo_cal_lut_update(rf_chip_phy_t *phy);
extern int tx_dc_offset_cal(rf_chip_phy_t *phy, TRX_CHN_ENUM chn);
extern void reset_adc(rf_chip_phy_t *phy, TRX_CHN_ENUM chn);
extern void rx_adc_ldo_enable(rf_chip_phy_t *phy);
extern void rx_adc_cal(rf_chip_phy_t *phy, TRX_CHN_ENUM chnl);
extern void rx_adc_gain_cal(rf_chip_phy_t *phy, TRX_CHN_ENUM chnl, TRX_IQ_ENUM iq_index);
extern int rx_adc_get_gain_code(rf_chip_phy_t *phy, TRX_CHN_ENUM chnl, TRX_IQ_ENUM iq_index);
extern void rx_adc_set_gain_code(rf_chip_phy_t *phy, TRX_CHN_ENUM chnl, TRX_IQ_ENUM iq_index, int gain_code);
extern void rx_adc_comp_offst_cal(rf_chip_phy_t *phy, TRX_CHN_ENUM chnl, TRX_IQ_ENUM iq_index);
extern int rx_adc_get_sum_power(rf_chip_phy_t *phy, TRX_CHN_ENUM chnl, TRX_IQ_ENUM iq_index, RX_ADC_SUB_ENUM sub_index, int try_num);
extern void rx_adc_c10_restore(rf_chip_phy_t *phy, TRX_CHN_ENUM chnl, TRX_IQ_ENUM iq_index, RX_ADC_SUB_ENUM sub_index);
extern int rx_adc_get_power(rf_chip_phy_t *phy, TRX_CHN_ENUM chnl, TRX_IQ_ENUM iq_index, RX_ADC_SUB_ENUM sub_index);
extern void rx_adc_write_os_cal(rf_chip_phy_t *phy, TRX_CHN_ENUM chnl, TRX_IQ_ENUM iq_index,  RX_ADC_SUB_ENUM sub_index, unsigned int val, unsigned int addr);
extern void rx_adc_munual_mode_enable(rf_chip_phy_t *phy);

extern int tx_dac_cal(rf_chip_phy_t *phy, TRX_CHN_ENUM chn);
extern unsigned int tx_dac_val_cat(unsigned int h_val, unsigned int l_val);
extern unsigned int tx_dac_val_corv(unsigned int val);
extern void Set_TXDAC_n_calc_result(rf_chip_phy_t *phy, int channel, int regNum, int flag, unsigned int cal_val);
extern long get_auxadc_p16_value(rf_chip_phy_t *phy, int channel, int flag);
extern long get_auxadc_n16_value(rf_chip_phy_t *phy, int channel, int flag);
extern long get_auxadc_adc0_value(rf_chip_phy_t *phy, int channel, int flag);
extern long get_auxadc_p255_value(rf_chip_phy_t *phy, int channel, int flag);
extern void tx_dac_cal_n_with_auxadc(rf_chip_phy_t *phy, int channel);
extern int get_dac_cal_val(rf_chip_phy_t *phy, int index,  int channel,  int Q);
extern int print_cal_val(rf_chip_phy_t *phy, int channel);

extern int lo_leakage_cal(rf_chip_phy_t *phy, int channel);

extern int rx_dc_offset_cal(rf_chip_phy_t *phy, TRX_CHN_ENUM chn);
extern void rx_dc_offset_cal_enable(rf_chip_phy_t *phy, TRX_CHN_ENUM chn, unsigned char enable);
extern short rx_dc_offset_cal_status(rf_chip_phy_t *phy, TRX_CHN_ENUM chn);
extern void rx_dc_offset_cal_enable_ext(rf_chip_phy_t *phy, TRX_CHN_ENUM channel_sel, unsigned char ena);
extern short rx_get_adc_offset(rf_chip_phy_t *phy, TRX_CHN_ENUM channel_sel, char try_num , int * dc_offset_i, int * dc_offset_q);
extern int rx_get_adc_power(rf_chip_phy_t *phy, int channel_sel, int try_num, int * mean_power);
extern short do_rx_dc_offset_cal(rf_chip_phy_t *phy, TRX_CHN_ENUM chn, TRX_IQ_ENUM iq, unsigned short tune_reg, unsigned char flag, unsigned char ext);
extern int handle_rx_dc_offset_cal(rf_chip_phy_t *phy, TRX_CHN_ENUM chn, TRX_IQ_ENUM iq, unsigned short tune_reg, unsigned char flag, unsigned char ext);
extern int rx_dc_offset_cal_update_all_gain_tia_lut(rf_chip_phy_t *phy, int channel, int val);
extern int rx_dc_offset_cal_update_bq_lut(rf_chip_phy_t *phy, int channel, RXFE_GAIN_ENUM rxfe_gain_index, unsigned int val[]);
extern int rx_dc_set_tia_lut_setting_by_idx(rf_chip_phy_t *phy, int channel, short mix_pos, short lmt_index, unsigned char i_data, unsigned char q_data);
extern int rx_dc_get_tia_lut_setting_by_idx(rf_chip_phy_t *phy, int channel, short mix_pos, short lmt_index, unsigned char *i_data, unsigned char *q_data);
extern int rx_dc_set_bq_lut_setting_by_idx(rf_chip_phy_t *phy, int channel, short lmt_index, short lpf_index, unsigned char i_data, unsigned char q_data);
extern int rx_dc_get_bq_lut_setting_by_idx(rf_chip_phy_t *phy, int channel, short lmt_index, short lpf_index, unsigned char *i_data, unsigned char *q_data);
extern int rx_dc_offset_cal_in(rf_chip_phy_t *phy, int channel, RX_PORT_ENUM rx_port);

extern unsigned char shift_byte(unsigned char data);
extern int inbalance_cal_with_G0(rf_chip_phy_t *phy, int channel);
extern int inbalance_cal_with_G2(rf_chip_phy_t *phy, int channel);
extern int inbalance_cal(rf_chip_phy_t *phy, int channel);

extern void set_rf_bandwidth(rf_chip_phy_t *phy, BANDWITH_ENUM bandwith);
extern short config_syspll(rf_chip_phy_t *phy, unsigned long freq, unsigned long long vco_freq);
extern void set_rx_port(rf_chip_phy_t *phy, TRX_CHN_ENUM chn, RX_PORT_ENUM port, RXFE_GAIN_ENUM gain, short en);
extern void set_rx_port_man(rf_chip_phy_t *phy, TRX_CHN_ENUM chn, RX_PORT_ENUM port, RXFE_GAIN_ENUM gain);
extern void set_tx_port(rf_chip_phy_t *phy, TRX_CHN_ENUM chn, TX_PORT_ENUM port, short en);
extern short set_tx_atten_cfg(rf_chip_phy_t *phy, unsigned long long flo, unsigned char gain);
extern int write_tx_qec_lol_cal_word_chn(rf_chip_phy_t *phy, TRX_CHN_ENUM chn, short bb_gain, TX_QEC_CFG_REGS *tx_qec_cfg);
extern void real_time_update_tx_qec(rf_chip_phy_t *phy, TRX_CHN_ENUM chn, TX_QEC_CFG_REGS *tx_qec_cfg);
extern short set_tx_atten_chn(rf_chip_phy_t *phy, int chn, unsigned long long flo, unsigned char gain, short immed);
extern short set_tx_atten(rf_chip_phy_t *phy, unsigned long long flo, unsigned char gain, short immed);
extern int set_tx_atten_ext(rf_chip_phy_t *phy, int chn, int type, unsigned long long flo, int gain_index);
extern short set_tx_ana_atten_chn(rf_chip_phy_t *phy, int chn, unsigned long long flo, unsigned char gain, short immed);
extern short set_tx_ana_atten(rf_chip_phy_t *phy, unsigned long long flo, unsigned char gain, short immed);
extern void set_tx_dig_atten(rf_chip_phy_t *phy, TRX_CHN_ENUM chn, unsigned short index);
extern int tx_split_atten_chn(rf_chip_phy_t *phy, TRX_CHN_ENUM chn, unsigned long long flo, TX_SPLIT_ATTEN_ENUM type, unsigned int gain, short immed);
extern int tx_split_atten(rf_chip_phy_t *phy, unsigned long long flo, TX_SPLIT_ATTEN_ENUM type, unsigned int gain, short immed);
extern int rx_mgc_gain(rf_chip_phy_t *phy, TRX_CHN_ENUM chn, RX_MGC_GAIN_ENUM mgc, unsigned char gain);
extern void rx_mgc_max_gain(rf_chip_phy_t *phy, TRX_CHN_ENUM chn);
extern void rxgain_force_valid_config(rf_chip_phy_t *phy, int enable);
extern short write_lut_byte(rf_chip_phy_t *phy, LUT_INDEX_ENUM lut, short index, unsigned short reg, int lut_addr, unsigned char offset, unsigned char val);
extern short read_lut_byte(rf_chip_phy_t *phy, LUT_INDEX_ENUM lut, short index, unsigned short reg, int lut_addr, unsigned char offset, unsigned char *pval);
extern short read_lut_word(rf_chip_phy_t *phy, LUT_INDEX_ENUM lut, int lut_addr, unsigned char vals[]);
extern short write_lut_word(rf_chip_phy_t *phy, LUT_INDEX_ENUM lut, int lut_addr, unsigned char vals[]);
extern short find_case_with_flo(unsigned long long flo);
extern int set_trx_lo(rf_chip_phy_t *phy, TRX_ENUM trx, TRX_CHN_ENUM chn, unsigned long long flo);
extern int sxrx_band_fsm(rf_chip_phy_t *phy, unsigned long long flo);
extern int sxtx_band_fsm(rf_chip_phy_t *phy, unsigned long long flo, short core2_en);
extern short fvco_get_right_index(rf_chip_phy_t *phy, unsigned long long comp_val, char fsm_flag);
extern short calc_lo_div(rf_chip_phy_t *phy, unsigned long long flo);
extern short set_mmd(rf_chip_phy_t *phy, TRX_ENUM trx, unsigned long long fvco, short *zero_flag);
extern unsigned char config_sxtrx(rf_chip_phy_t *phy, TRX_ENUM trx, unsigned long long fvco, short *outi, char fsm_flag, char wflag);
extern void set_target_cnt(rf_chip_phy_t *phy, TRX_ENUM trx, unsigned long long flo, unsigned char flag, int factor, short scale);
extern unsigned char get_reg_val_from_trx_band(char found_case, unsigned short reg);
extern short calc_tx_fsm_lo_div(rf_chip_phy_t *phy, short *flag_div);
extern unsigned char set_fsm_tx_target_cnt(rf_chip_phy_t *phy, unsigned long long flo);
extern char timing_map_to_val(char timing);
extern unsigned short lo_div_map_val(short val, short div);
extern void rxadc_on(rf_chip_phy_t *phy, TRX_CHN_ENUM chn, short on);
extern void rxifbuf_on(rf_chip_phy_t *phy, TRX_CHN_ENUM chn, short on);
extern void fcal_s2_bypass(rf_chip_phy_t *phy, char fast_sx_lock);
extern void core2_s7_s8_s10_s11_s12_bypass(rf_chip_phy_t *phy, char core2_enable);

extern int tx1_band_selection_c2(rf_chip_phy_t *phy);
extern short tx1_c2_lo_config(unsigned long long lo, short fcal_div);
extern short tx1_c2_div_ratio(unsigned long long lo, short *lo_div);
extern int sxtx_fcal_and_on(rf_chip_phy_t *phy);
extern int tx1_c2_on_config(rf_chip_phy_t *phy, unsigned int regs_val[]);
extern int tx1_c2_hw_dll_cal(rf_chip_phy_t *phy);
extern int tx1_c2_win_sw_cal(rf_chip_phy_t *phy);
extern int tx1_c2_win_hw_cal(rf_chip_phy_t *phy);
extern int tx1_c2_reload(rf_chip_phy_t *phy, unsigned int regs_val[]);
extern int user_map_to_standard(BANDWITH_ENUM user_bw);
extern int map_to_data_mode(CHIP_MODE_ENUM mode, DIG_IF_ENUM data_if, IF_TYPE_ENUM if_type, DATA_RATE_ENUM data_rate);
extern void txdc_digtial_remove(rf_chip_phy_t *phy, TRX_CHN_ENUM chn);
extern short digital_init(rf_chip_phy_t *phy, CHIP_MODE_ENUM mode, BANDWITH_ENUM bandwith, DIG_IF_ENUM dif, IF_TYPE_ENUM port, DATA_RATE_ENUM rate, short step);
extern short load_lut(rf_chip_phy_t *phy, CHIP_MODE_ENUM mode);
extern short lut_version_get(rf_chip_phy_t *phy, unsigned char ver[]);
extern int gen_int_val(unsigned char *val);
extern short write_chip_lut(rf_chip_phy_t *phy, unsigned char data[]);
extern void wait_init(rf_chip_phy_t *phy);
extern void fsm_init(rf_chip_phy_t *phy);
extern void manual_enable(rf_chip_phy_t *phy, short en);
extern int fdd_force_wait(rf_chip_phy_t *phy);
extern int fdd_wait_to_alert(rf_chip_phy_t *phy);
extern int fdd_alert_to_fsm(rf_chip_phy_t *phy);
extern int fdd_fsm_to_alert(rf_chip_phy_t *phy);

extern void trx_lut_load(rf_chip_phy_t *phy, unsigned long long tx_flo, BANDWITH_ENUM bw, short custom_bw_flag);
extern int  trx_band_pa_lut_load(rf_chip_phy_t *phy, unsigned long long tx_flo);

extern short proximity_bandwith_seek(rf_chip_phy_t *phy, unsigned long custom_bandwith);
extern short rf_chip_phy_obj_init(rf_chip_phy_t *phy);
extern void sxtrx_cfg_switch(rf_chip_phy_t *phy);

extern int extpin_voltage_read(rf_chip_phy_t *phy);
extern void misc_init(rf_chip_phy_t *phy);
extern void testmux_clear(rf_chip_phy_t *phy);
extern void test_read_enable(rf_chip_phy_t *phy, short reg, short en);
extern short test_voff_get(rf_chip_phy_t *phy, MT_AUXADC_ENUM mt, short ctrl_reg1, short ctrl_reg2, short read_reg1, short read_reg2);
extern short test_voff_get_ext(rf_chip_phy_t *phy, MT_AUXADC_ENUM mt, short ctrl_reg1, short ctrl_reg2, short read_reg1, short read_reg2);
extern int test_mt_auxadc(rf_chip_phy_t *phy, MT_AUXADC_ENUM mt, short connect_flag, short div3_flag, char type_flag);
extern void test_div3_config(rf_chip_phy_t *phy, short en);
extern int chip_temp_get(rf_chip_phy_t *phy);
extern short sxtrx_lock_status(rf_chip_phy_t *phy, short dir, int *pVoltage);
extern int ldo_get_voltage(rf_chip_phy_t *phy);
extern void do_ldo_cal(rf_chip_phy_t *phy, int ref_voltage, unsigned int reg, unsigned int shift_num);
extern void ldo_cal(rf_chip_phy_t *phy);

extern void module_debug_onoff(rf_chip_phy_t *phy, unsigned long en);
extern unsigned long module_debug_get(rf_chip_phy_t *phy);
extern unsigned int get_product_id(rf_chip_phy_t *phy);

extern int sx_temperature_track_v1(rf_chip_phy_t *phy, TRX_ENUM trx);
extern int sx_temperature_track_v2(rf_chip_phy_t *phy, TRX_ENUM trx);

extern int get_syspll_status(rf_chip_phy_t *phy);


#endif  /* __DRIVER_H */

