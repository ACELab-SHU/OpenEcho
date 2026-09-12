#ifndef  _COMMON_H
#define  _COMMON_H

#include "transceiver_def.h"

typedef enum CHIP_MODE
{
	CH1_FDD=0,
	RX1_TX2_FDD,
	RX2_TX1_FDD,
	CH2_FDD,
	CH1_CH2_FDD,
	
	CH1_TDD,
	CH2_TDD,
	CH1_CH2_TDD,
}CHIP_MODE_ENUM;

typedef enum HYBRID_MODE
{
	HYBRID_FDD_SXTX,
	HYBRID_TDD_SXTXRX,
}HYBRID_MODE_ENUM;

typedef enum DIG_IF
{
	LVDS_IF=0,
	CMOS_IF,
}DIG_IF_ENUM;

typedef enum TX_PORT
{
	CH1_A=0,
	CH1_B,
	CH1_A_CH2_A,
	CH1_A_CH2_B,
	CH1_B_CH2_A,
	CH1_B_CH2_B,
}TX_PORT_ENUM;

typedef enum TX_S_PORT
{
	TX_A=0,
	TX_B,
	TX_PORT_CNT,
}TX_S_PORT_ENUM;

typedef enum RX_PORT
{
	MIXAP_SE=0,
	MIXAN_SE,
	MIXA_DIFF,

	MIXBP_SE,
	MIXBN_SE,
	MIXB_DIFF,

	MIXCP_SE,
	MIXCN_SE,
	MIXC_DIFF,
	TXMON,
	MIX_CNT,
}RX_PORT_ENUM;

typedef enum BANDWITH
{
	BW_GSM=0,
	BW_BT,
	BW_LTE5,
	BW_LTE10,
	BW_LTE20,
	BW_LTE40,
	BW_LTE60,
	BW_LTE80,
	BW_LTE100,
	BW_CNT,
}BANDWITH_ENUM;

typedef enum STANDARD_BW
{
	STANDARD_BW1=0,
	STANDARD_BW2,
	STANDARD_BW3,
	STANDARD_BW4,
	STANDARD_BW5,
	LTE1P4,
	LTE3,
	LTE5,
	LTE10,
	LTE15,
	LTE20,
	LTE40,
	LTE100,
	WLAN_80211_FHSS_1M,
	WLAN_80211_FHSS_2M,
	WLAN_80211_DSS,
	WLAN_80211B_HRDSSS,
	WLAN_80211A_OFDM1,
	WLAN_80211A_OFDM2,
	WLAN_80211A_OFDM3,
	WLAN_80211N_HT_OFDM1,
	WLAN_80211N_HT_OFDM2,
	WLAN_80211N_HT_OFDM3,
	WLAN_80211N_HT_OFDM4,
	WLAN_80211AC_VHT_OFDM1,
	WLAN_80211AC_VHT_OFDM2,
	WLAN_80211AH_S1G1,
	WLAN_80211AH_S1G2,
	WLAN_80211AH_S1G3,
	WLAN_80211AH_S1G4,
	WLAN_80211AH_S1G5,
	BLE,
	DIG_BW_MAX,
}STANDARD_BW_ENUM;

typedef enum DATA_RATE_MODE
{
	CMOS_2R2T_SP_FDD_SDR=0,
	CMOS_2R2T_SP_FDD_DDR,
	CMOS_2R2T_SP_TDD_SDR,
	CMOS_2R2T_SP_TDD_DDR,
	CMOS_2R2T_DP_FDD_SDR,
	CMOS_2R2T_DP_FDD_DDR,
	CMOS_2R2T_DP_TDD_SDR,
	CMOS_2R2T_DP_TDD_DDR,
	LVDS_2R2T_DP_FDD_DDR,

	CMOS_1R1T_SP_FDD_SDR,
	CMOS_1R1T_SP_FDD_DDR,
	CMOS_1R1T_SP_TDD_SDR,
	CMOS_1R1T_SP_TDD_DDR,
	CMOS_1R1T_DP_FDD_SDR,
	CMOS_1R1T_DP_FDD_DDR,
	CMOS_1R1T_DP_TDD_SDR,
	CMOS_1R1T_DP_TDD_DDR,
	LVDS_1R1T_DP_FDD_DDR,
}DATA_RATE_MODE_ENUM;

typedef enum IF_TYPE
{
	DUAL_PORT=0,
	SINGLE_PORT,
}IF_TYPE_ENUM;

typedef enum DATA_RATE
{
	SDR=0,
	DDR,
}DATA_RATE_ENUM;

typedef enum PHY_MODE
{
	PHY_FDD=0,
	PHY_TDD,
}PHY_MODE_ENUM;

typedef enum WIRE_CTRL
{
	PULSE_CTRL=0,
	LEVEL_CTRL,
}WIRE_CTRL_ENUM;

typedef enum MT_AUXADC
{
	MT1P_AUXADC=0,
	MT1N_AUXADC,
	MT2P_AUXADC,
	MT2N_AUXADC,
}MT_AUXADC_ENUM;

typedef enum TRX_CHN
{
	TRX_CHN1,
	TRX_CHN2,
	TRX_CH_CNT,
}TRX_CHN_ENUM;

typedef enum ENSM_STATE
{
	FSM_IDLE=0,
	FSM_TDD_RX,
	FSM_TDD_TX,
	FSM_FDD,
}ENSM_STATE_ENUM;

typedef enum TRX_IQ
{
	TRX_I=0,
	TRX_Q,
}TRX_IQ_ENUM;

typedef enum RX_ADC_SUB
{
	RX_ADC_SUB1=0,
	RX_ADC_SUB2,
}RX_ADC_SUB_ENUM;

typedef enum RXFE_GAIN
{
	RX_PORT_G0=0,
	RX_PORT_G1,
	RX_PORT_G2,
	RX_PORT_G3,
	RX_PORT_G4,
	RX_PORT_G5,
	RX_PORT_GAIN_MAX, //total: 6 categorys (RX_PORT_G0~RX_PORT_G5)
}RXFE_GAIN_ENUM;

typedef enum TRX
{
	RX_DIR,
	TX_DIR,
}TRX_ENUM;

typedef enum TX_SPLIT_ATTEN
{
	PA_G_INDEX,
	BB_G_INDEX,
	DIG_G_INDEX,
}TX_SPLIT_ATTEN_ENUM;

typedef enum RX_MGC_GAIN
{
	LMT_G,
	LPF_G,
	DIG_G,
}RX_MGC_GAIN_ENUM;

typedef enum FSM_ST
{
	AT_FSM_SLEEP,
	AT_FSM_WAIT,
	AT_FSM_ALERT,
	AT_FSM_FDD,
	AT_FSM_RX,
	AT_FSM_TX,
}FSM_ST_ENUM;

typedef enum LO_CHANGE_MODE
{
	LO_CHANGE_OLD,
	LO_CHANGE_NORMAL,
	LO_CHANGE_FAST_LOCK,
	LO_CHANGE_TRACKING,
    LO_CHANGE_FAST_LOCK_ATTACH_TRX_LO,
}LO_CHANGE_MODE_ENUM;

typedef enum TABLE_MODE
{
	SPLIT_TBL,
	FULL_TBL,
}TABLE_MODE_ENUM;

typedef enum GCTRL_MODE
{
	MANUAL_CTRL,
	SLOW_ATTACK,
	FAST_ATTACK,
}GCTRL_MODE_ENUM;

typedef enum TRX_LO_CAL_MODE
{
	TRX_LO_HW_CAL,
	TRX_LO_SW_CAL,
	TRX_LO_HYBRID_CAL,
}TRX_LO_CAL_MODE_ENUM;

typedef enum SX_TEMP_TRACK_MODE
{
    SX_TEMP_TRACK_V1 = 0,
    SX_TEMP_TRACK_V2,
}SX_TEMP_TRACK_MODE_ENUM;

typedef struct trx_sample_table_struct
{
	unsigned int bb_sample_rate;

	unsigned int rxadc_sample_rate;
	short rx_oversample_ratio;

	unsigned int txdac_sample_rate;
	short tx_oversample_ratio;
}trx_sample_table_t;

typedef struct __GCTRL_SETUP_DELAY_CFG__
{
	// GCTRL_SETUP_DELAY_CFG
	short cfg_init_setting_delay;
	short cfg_pdt_setting_delay;
	short cfg_hbf_setting_delay;
}GCTRL_SETUP_DELAY_CFG_T;

typedef struct __GCTRL_EXT_LNA_CFG__
{
	// GCTRL_EXT_LNA_CFG
	short cfg_split_ext_lna_force_ena;
	short cfg_split_ext_lna_force_index;
	short cfg_split_ext_lna_polarity;
	short cfg_split_ext_lna_inc_th;
	short cfg_split_ext_lna_dec_th;
	short cfg_split_ext_lna_ena;
	short cfg_split_ext_lna_db;
}GCTRL_EXT_LNA_CFG_T;


typedef struct __GCTRL_FULL_TABLE__
{
	short init_index  ;
    short max_index   ;
	short content[128];
}GCTRL_FULL_TABLE_CFG_T;


typedef struct __GCTRL_SPLIT_TABLE_CFG__
{
	//GCTRL_SPLIT_TABLE_CFG
	short cfg_split_lna_index_limit;
	short cfg_split_lmt_index_limit;
	short cfg_split_init_lna_index;
	short cfg_split_init_lmt_index;
	short cfg_split_init_lpf_index;
}GCTRL_SPLIT_TABLE_CFG_T;

typedef struct __GCTRL_DIG_AGC_CFG_
{
	// GCTRL_DIG_AGC_ENA
	short cfg_digital_gain_ena;
	short cfg_digital_max_index;
	short cfg_digital_init_index;
}GCTRL_DIG_AGC_CFG_T;

typedef struct __GCTRL_HBF_LOW_PWR_CFG__
{
	short cfg_hbf_low_pwr_ena           ;
    short cfg_hbf_low_pwr_th            ; // unit 0.5dB
	short cfg_hbf_low_pwr_meas_num      ;
}GCTRL_HBF_LOW_PWR_CFG_T;

typedef struct __GCTRL_HBF_PWR_MEAS_CFG__
{
	// GCTRL_HBF_PWR_MEAS_CFG
	short cfg_hbf_avp_ena;
	short cfg_hbf_avp_meas_num;
}GCTRL_HBF_PWR_MEAS_CFG_T;

typedef struct __GCTRL_HBF_OVF_DET_CFG__
{
	// GCTRL_HBF_OVF_DET_CFG
	short cfg_hbf_ovf_ena; // digital overflow detection
	short cfg_hbf_ovf_exceed_threshold;
}GCTRL_HBF_OVF_DET_CFG_T;

typedef struct __GCTRL_LMT_PEAK_DET_CFG__
{
	// GCTRL_ANA_PEAK_DET_CFG
	short cfg_lmt_setting_counter            ;
	short cfg_lmt_overload_ena               ;
	short cfg_lmt_prevent_inc_ena            ;
	short cfg_lmt_underload_ena              ;
	short cfg_lmt_overload_threshold         ;
	short cfg_lmt_prevent_inc_threshold      ;
	short cfg_lmt_underload_threshold        ;
	short cfg_lmt_overload_exceed_counter    ;
	short cfg_lmt_prevent_exceed_counter     ;
	short cfg_lmt_underload_exceed_counter   ;
	short cfg_lmt_overload_power_coeff       ;
	short cfg_lmt_underload_power_coeff      ;
}GCTRL_LMT_PEAK_DET_CFG_T;

typedef struct __GCTRL_ADC_PEAK_DET_CFG__
{
	// GCTRL_ADC_PEAK_DET_CFG
    short cfg_adc_overload_lg_ena            ;
    short cfg_adc_overload_sm_ena            ;
    short cfg_adc_prevent_inc_ena            ;
    short cfg_adc_underload_ena              ;
    short cfg_adc_overload_lg_threshold      ;
    short cfg_adc_overload_sm_threshold      ;
    short cfg_adc_prevent_inc_threshold      ;
    short cfg_adc_underload_threshold        ;
    short cfg_adc_overload_lg_exceed_counter ;
    short cfg_adc_overload_sm_exceed_counter ;
    short cfg_adc_prevent_inc_exceed_counter ;
    short cfg_adc_underload_exceed_counter   ;
    short cfg_adc_overload_power_coeff       ;
    short cfg_adc_underload_power_coeff      ;
}GCTRL_ADC_PEAK_DET_CFG_T;

typedef struct __GCTRL_BASIC_CFG__
{
	// GCTRL_BASIC_CFG
    short cfg_gain_table_mode                ; // 0 fulltable | 1 split table
    short cfg_agc_mode                       ; // 0 mgc	| 1 slow | 2 fast
    short cfg_pin_pls_gain_change_sel        ;
}GCTRL_BASIC_CFG_T;

typedef struct __GCTRL_SLOW_AGC_CFG__
{
	// GCTRL_SLOW_AGC_CFG
    short cfg_slow_hbf_overflow_quick_resp     ;
    short cfg_slow_lmt_overload_quick_resp     ;
    short cfg_slow_lmt_underload_quick_resp    ;
    short cfg_slow_adc_overload_lg_quick_resp  ;
    short cfg_slow_adc_overload_sm_quick_resp  ;
    short cfg_slow_adc_underload_quick_resp    ;
    short cfg_slow_pdt_clear_mode              ;
    short cfg_slow_agc_gain_update_mode_ena    ;
    int   cfg_slow_agc_gain_update_counter     ;
    short cfg_slow_full_lmt_overload_step      ;
    short cfg_slow_full_lmt_underload_step     ;
    short cfg_slow_full_adc_overload_step      ;
    short cfg_slow_full_adc_underload_step     ;
    short cfg_slow_calc_time                   ;
    short cfg_slow_full_hbf_overflow_step      ;
    short cfg_slow_pin_sync_ena                ;
    short cfg_slow_agc_outter_high_step        ;
    short cfg_slow_agc_inner_high_step         ;
    short cfg_slow_agc_outter_low_step         ;
    short cfg_slow_agc_inner_low_step          ;
    short cfg_slow_split_lmt_dec_step          ;
    short cfg_slow_split_lpf_dec_step          ;
    short cfg_slow_split_dig_dec_step          ;
    short cfg_slow_split_lmt_inc_step          ;
    short cfg_slow_split_lpf_inc_step          ;
    short cfg_slow_agc_outter_high_th          ; // unit 0.5dBFs
    short cfg_slow_agc_outter_low_th           ;
    short cfg_slow_agc_inner_high_th           ;
    short cfg_slow_agc_inner_low_th            ;
}GCTRL_SLOW_AGC_CFG_T;

typedef struct __GCTRL_FAST_AGC_CFG__
{
	// GCTRL_FAST_AGC_CFG
    short cfg_fast_calc_time                      ;
    short cfg_fast_s1_exit_counter                ;
    short cfg_fast_s1_full_large_dec_step         ;
    short cfg_fast_s1_full_medium_dec_step        ;
    short cfg_fast_s1_full_small_dec_step         ;
    short cfg_fast_s1_split_lmt_dec_step          ;
    short cfg_fast_s1_split_lpf_dec_step          ;
    short cfg_fast_s1_split_dig_dec_step          ;
    short cfg_fast_s2_adjustment_method           ;
    short cfg_fast_s2_lock_power_target           ;
    short cfg_fast_s2_max_inc_step                ;
    short cfg_fast_s2_limit_max_adj_step_ena      ;
    short cfg_fast_s2_to_s2a_mask                 ;
    short cfg_fast_s2_to_s2b_mask                 ;
    short cfg_fast_s2a_inc_step                   ;
    short cfg_fast_s2a_exit_counter               ;
    short cfg_fast_s2b_exit_counter               ;
    short cfg_fast_s2b_to_s2a_mask                ;
    short cfg_fast_s3_overrange_counter           ;
    short cfg_fast_s3_confirm_counter             ;
    short cfg_fast_s3_dec_step                    ;
    short cfg_fast_s3_to_s2a_mask                 ;
    short cfg_fast_s4_avp_meas_num                ;
    short cfg_fast_unlock_mode_energy_lost        ;
    short cfg_fast_unlock_mode_exist_rx           ;
    short cfg_fast_unlock_mode_pin_ena            ;
    short cfg_fast_unlock_mode_spi                ;
    short cfg_fast_set_gain_burst_start_sel       ;
    short cfg_fast_opt_gain_offset                ;
    short cfg_fast_dont_unlock_lmt_pdt_overload   ;
    short cfg_fast_dont_unlock_adc_pdt_overload_lg;
    short cfg_fast_dont_unlock_lmt_pdt_underload  ;
    short cfg_fast_dont_unlock_adc_pdt_underload  ;
    short cfg_fast_dont_unlock_energy_lost        ;
    short cfg_fast_dont_unlock_energy_boost       ;
    short cfg_fast_dont_unlock_pin_agc_ena        ;
    short cfg_fast_energy_lost_threshold_db       ;
    short cfg_fast_energy_boost_threshold_db      ;
    short cfg_fast_energy_change_exit_counter     ;
    short cfg_fast_gain_lock_hold_time            ;
}GCTRL_FAST_AGC_CFG_T;

typedef struct __GCTRL_MGC_CFGG__
{
	// MGC default SPI mode
    short cfg_mgc_mode                       ;
    short cfg_mgc_adj_gain_pos_auto          ;
    short cfg_mgc_adj_gain_pos_lmt           ;
    short cfg_mgc_inc_step                   ;
    short cfg_mgc_dec_step                   ;
}GCTRL_MGC_CFGG_T;


typedef struct __GCTRL_SPLIT_GAIN__
{
	short cfg_mgc_lna_index;
	short cfg_mgc_lmt_index;
	short cfg_mgc_lpf_index;
}GCTRL_SPLIT_GAIN_T;

typedef struct __GCTRL_GET_RPT__
{
	// GCTRL_FORCE_GAIN
	//short spi_read_pulse;// 0->1->0
    short rpt_adc_flg_overload_lg            ;
    short rpt_adc_flg_overload_sm            ;
    short rpt_lmt_flg_overload               ;
    short rpt_hbf_flg_overflow               ;
    short rpt_hbf_flg_low_power              ;
    short rpt_adc_flg_prevent_inc            ;
    short rpt_lmt_flg_prevent_inc            ;
    short rpt_lmt_flg_underload              ;
    short rpt_adc_flg_underload              ;
    short rpt_lmt_pdt_value                  ;
    short rpt_hbf_avp                        ;
    short rpt_lna_index                      ;
    short rpt_lmt_index                      ;
    short rpt_lpf_index                      ;
    short rpt_dig_index                      ;
    short rpt_full_index                     ;
}GCTRL_GET_RPT_T;

typedef struct __rx_gain_ctrl__
{
    GCTRL_BASIC_CFG_T            basic       ;
    GCTRL_SETUP_DELAY_CFG_T      delay       ;
    GCTRL_EXT_LNA_CFG_T          ext_lna     ;
    GCTRL_FULL_TABLE_CFG_T       gain_full   ;
    GCTRL_SPLIT_TABLE_CFG_T      gain_split  ;
	GCTRL_DIG_AGC_CFG_T          gain_digital;
    GCTRL_LMT_PEAK_DET_CFG_T     lmt_pdt     ;
    GCTRL_ADC_PEAK_DET_CFG_T     adc_pdt     ;
    GCTRL_HBF_PWR_MEAS_CFG_T     hbf_pwr     ;
    GCTRL_HBF_OVF_DET_CFG_T      hbf_ovf     ;
    GCTRL_HBF_LOW_PWR_CFG_T      hbf_lpwr    ;
    GCTRL_SLOW_AGC_CFG_T         agc_slow    ;
    GCTRL_FAST_AGC_CFG_T         agc_fast    ;
    GCTRL_MGC_CFGG_T             mgc         ;
}RX_GCTRL_CFG_T;

typedef struct _RX_QEC_CFG_REGS_{
    int fiiq_real;
    int fiiq_imag;
    int fdiq_fir[32];
    double mag_ratio_adj;
    double ph_error_adj;
	int speed_level;
	int debug_level;	
}RX_QEC_CFG_REGS;

typedef struct _TX_QEC_CFG_REGS_{
    int    fiiq_ena ;
    int    fiiq_real;
    int    fiiq_imag;
    int    fdiq_ena ;
    int    fdiq_fir[32];
    int    dc_i     ;
    int    dc_q     ;
	double mag_ratio;
	double ph_error ;
	int    speed_level ;
	int    debug_level ;		
	int    cal_init ;
}TX_QEC_CFG_REGS;

typedef struct chip_config_struct
{
	CHIP_MODE_ENUM mode;
	DIG_IF_ENUM dig_if;
	IF_TYPE_ENUM p0p1_port;
	DATA_RATE_ENUM data_rate;
	TX_PORT_ENUM tx_port[2];
	RX_PORT_ENUM rx_port[2];
	BANDWITH_ENUM bandwith;
	LO_CHANGE_MODE_ENUM lo_change_mode;
    xo_type xo_sel;
	unsigned long xtal_freq;
	unsigned long long sys_fvco;
	char x4_enable;
	char fast_sx_lock;
	char tx_atten_chn_flag;
	RXFE_GAIN_ENUM rxfe_gain[2];
	WIRE_CTRL_ENUM wire_ctrl;
	short wire_control_en;
	TABLE_MODE_ENUM gain_table_mode;
	GCTRL_MODE_ENUM gain_ctrl_mode;
	short gain_ctrl_pin_flag;
    short bandwidthswitch_flag;
    short bandwidthswitch_length;
    BANDWITH_ENUM *pBandwidthloop;
	short custom_bandwith_flag;
	unsigned long custom_bandwith;
	unsigned char dac_syspll_lo_div;
	unsigned char adc_syspll_lo_div;
	unsigned long bb_sample_rate;
	int rx_ext_loop;
	int tx_ext_loop;
	int qec_dbfs;
	int lol_dbfs;
	
	int sx_vco_ldo;
	int sxlf_ldo;
	int syspll_ldo;
	int txabb_ldo;
	int txdac_ldo;
	int txfe_ldo;
	int txsx_lo_ldo;
	int rxadc_ldo;
	int rxfe_ldo;
	int rxsx_lo_ldo;
	int mdig_ldo;

	unsigned char use_bybrid_mode;
	HYBRID_MODE_ENUM hybrid_mode;
	unsigned char RxDc_Offset_Ver;
	char Rx_ImBalance_cal_flag;
	char lo_leakage_cal_flag;

	char lvds_cal_flag;
	char auxadc1_cal_flag;
	char r_cal_flag;
	char rx_bw_cal_flag;
	char rx_dc_cal_flag;
	char rx_adc_cal_flag;
	char rx_qec_flag;
	char tx_bw_cal_flag;
	char tx_dc_cal_flag;
	char tx_dac_cal_flag;
	char tx_qec_flag;
	char txlo_cal_flag;
	char sx_cal_flag;
	char ldo_cal_flag;

	char reg901_val;
	char reg639_620_bit1;
	char reg61A_val;
	char reg600_val;
	char reg602_val;
	int scap_min;
	int scap_max;
	unsigned int scap_cnt;
	unsigned int vol_low_limit;
	unsigned int vol_up_limit;
	unsigned int vol_margin;
    TRX_LO_CAL_MODE_ENUM trx_lo_cal_mode;
	unsigned long long  fvco_min;
	unsigned long long  fvco_limit;

	unsigned char imb_rx_cfg[2][2];

	unsigned long long  rx_flo;
	short rx_twin_fxo;
	short rx_tsu_fxo;
	short rx_tstate2_fsys_us;

	unsigned long long  tx_flo;
	short tx_twin_fxo;
	short tx_tsu_fxo;
	short tx_tstate2_fsys_us;
	char core2_enable;
	short dll_twin_fxo;
	short dll_tsu_fxo;
	short tstate7_fsys_us;
	
	int chip_sel;
	int rxgain_force_valid_flag;
	int chip_ver;
	short sxtrx_perf_setting[10][4];
	int syspll_cfg_flag;

	int rcal_read;
} chip_config_t;

typedef enum LUT_INDEX
{
	MAIN_ENSM_LUT = 0,
	RX1_AGC_LUT = 1,
	RX2_AGC_LUT = 2,
	TXRX_IQ_LUT = 8,
	SX_CONFIG_LUT = 9,
	TX_GAIN_LUT = 10,
	SX_CAL_LUT = 11,
	TXLO_CAL_LUT = 12,
	TRX_BW_LUT = 13,
	RXIP2_CAL_LUT = 14,
	TRX_BAND_LUT = 15,
}LUT_INDEX_ENUM;

typedef enum CHIP_ERR_CODE
{
	NONE_ERR=0,
	SPI_RW_ERR,
	RCAL_FAIL,
	RX_BW_CAL_FAIL,
	RX_DC_CAL_FAIL,
	RX_ADC_CAL_FAIL,
	RX_QEC_CAL_FAIL,
	TX_BW_CAL_FAIL,
	TX_DC_CAL_FAIL,
	TX_DAC_CAL_FAIL,
	TX_QEC_LOL_CAL_FAIL,
	LOAD_LUT_FAIL,
	SYSPLL_LOCK_FAIL,
	SXTX_LOCK_FAIL,
	SXRX_LOCK_FAIL,
	OP_TIMEOUT,
	PARA_INVALID,
}CHIP_ERR_CODE_ENUM;

#define  MHZ_TO_HZ            (1000000)
#define  LUT_DIST_DELTA       (0x331)
#define RX_BW_CAL_TUNE_MAX_VAL 1023
#define  STRX_VCO_JD_FREQ     (6800000000ULL)
#define  U64_MAX_VAL          (0xffffffffffffffff)

#define FVCO_NOCAL_FLAG           (0)
#define FVCO_CAL_FLAG             (1)
#define FLO_CAL_FLAG              (2)

#define FILE_MAX              (1 + 80*4)

#define  GOERTZLE_SHIFT_NUM	  40
#define  GOERTZLE_SHIFT_NUM_IMBALANCE	 30

#define  CUSTOMER_FOR_HHH    (0)

extern const short g_lo_range_trx_band_fsm[9][2];
extern const unsigned short g_tx_pa_gain_regs[20];
extern const unsigned char g_tx_pa_gain_config[81][20];
extern const unsigned short g_tx_bb_gain_regs[16];
extern unsigned char g_tx_bb_gain_config[50][16];
extern const unsigned short g_trx_band_regs[54];
extern const unsigned char g_trx_band_config[9][54];
extern const unsigned short g_sxtrx_regs[2][10];
extern const unsigned char g_rx_port[54][4];
extern const unsigned short g_tx_bw_regs[35];
extern const unsigned short g_rx_bw_regs[27];
extern const trx_sample_table_t g_trx_sample_rate_table[32];
extern const unsigned int g_data_rate[9][32];

extern const long g_bandwith[9];
extern const long g_inband[9];
extern const long g_3db_band[9];
extern const unsigned int g_band_ctune_init[9]; 
extern const unsigned int g_band_bbrate[9];
extern const unsigned long long g_sx_config_vco_d1[71];
extern const unsigned long long g_sx_config_vco_d2[71];
extern const unsigned long long g_sx_config_vco_e1[71];
extern const unsigned long long g_sx_cal_vco_d1[512];
extern const unsigned long long g_sx_cal_vco_d2[512];
extern const unsigned long long g_sx_cal_vco_e1[512];


#endif

