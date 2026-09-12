#ifndef _H_QEC_COMMON_H_
#define _H_QEC_COMMON_H_

#include "common.h"
#include "platform.h"
#include "driver.h"
#include "register.h"

#define ROUND_SHIFT(x,bit)    ((((x)>0)? 1:-1) * ((abs((x))>>(bit)) + (((abs((x))>>((bit)-1))&0x1)?1:0)))

typedef struct _TXQEC_DIAG_INFO_{
	int * tx_qec_rsb_final;
	int * tx_qec_rsb_init;
	int * tx_qec_mag_err ;   // 10 or 30
	int * tx_qec_phg_err ;   // 10 or 30
	int * iter_cnt       ;
	int   wb_num         ;
	int   wb_cnt         ;
	int   lmt_index      ;    
	int   lpf_index      ;
	int   rx_dc_i        ;
	int   rx_dc_q        ;
	int   rx_power       ; 
	int   loopback_type  ; // 0 rf normal loopback | 1 rf low rf loopback ; 2 external loopback.
	int   debug_level    ;
}TX_QEC_DIAG_INFO;

typedef struct _TXLOL_DIAG_INFO_{
	int * tx_lol_rsb_final;
	int * tx_lol_rsb_init;	
	int * tx_lol_dc_i    ;   // 10 or 30
	int * tx_lol_dc_q    ;   // 10 or 30
	int * iter_cnt       ;
	int   wb_num         ;
	int   wb_cnt         ;
	int   lmt_index      ;    
	int   lpf_index      ;
	int   rx_dc_i        ;
	int   rx_dc_q        ;
	int   rx_power       ; 
	int   loopback_type  ; // 0 rf normal loopback | 1 rf low rf loopback ; 2 external loopback.
	int   debug_level    ;
}TX_LOL_DIAG_INFO;

typedef struct _RXQEC_DIAG_INFO_{
	int * rx_qec_mag_err ; 
	int * rx_qec_phg_err ; 
	int * rx_qec_mag_chk ; 
	int * rx_qec_phg_chk ; 
	int * rx_lpbk_power  ;
	int   wb_num         ;
	int   chk_num        ;
	int   wb_cnt         ;
	int   chk_cnt        ;
	int   rx_dc_i        ;
	int   rx_dc_q        ;
	int   rx_power       ; 
	int   lpf_index      ;
	int   loopback_type  ; // 0 rf normal loopback | 1 rf low 
	int   debug_level    ;
}RX_QEC_DIAG_INFO;

typedef struct _IF_LOOPBACK_REGS_{
    unsigned int reg_0x742;
    unsigned int reg_0x743;
    unsigned int reg_0x744;
    unsigned int reg_0x745;
	unsigned int reg_0x65F;
    unsigned int reg_0x660;
    unsigned int reg_0x661;
	unsigned int reg_0x66D;
}IF_LOOPBACK_REGS;

typedef struct _RF_LOOPBACK_REGS_{
    unsigned int reg_0x604;
    unsigned int reg_0x65F; // MIXER GAIN
    unsigned int reg_0x660; // MIXER HIGH/LOW GAIN SELECT
    unsigned int reg_0x661;
	unsigned int reg_0x66D;
    unsigned int reg_0x6D3;
    unsigned int reg_0x6D4;
    unsigned int reg_0x6D5;
    unsigned int reg_0x6D6;
    unsigned int reg_0x65C;
    unsigned int reg_0xD33;
    unsigned int reg_0x71B;
    unsigned int reg_0x71C;
    unsigned int reg_0x71F;
	unsigned int reg_0x74A;
}RF_LOOPBACK_REGS;


//----------------- there are functions needed @ chip driver ----------
extern int    fn_cal_inverse_dft32_point   (double din_i[32], double din_q[32], double dout[32]);
extern int    fn_get_quad_ph_err_deg       (int i_dft_real, int i_dft_imag, int q_dft_real, int q_dft_imag, double *mag_err_ratio , double *ph_err_deg);
extern int    fn_rx_set_dc_remove_ena  (rf_chip_phy_t *phy, int channel_sel, int ena);
extern int    fn_rx_get_adc_offset     (rf_chip_phy_t *phy, int channel_sel, int try_num , int * dc_offset_i, int * dc_offset_q);
extern int    fn_rx_get_adc_power      (rf_chip_phy_t *phy, int channel_sel, int try_num, int * mean_power);
extern int    fn_tx_set_dc_manual      (rf_chip_phy_t *phy, int channel_sel, int dc_i, int dc_q);
extern int    fn_tx_set_dc_manual_ena  (rf_chip_phy_t *phy, int channel_sel,int ena_I,int ena_Q);
extern int    fn_tx_set_dig_index      (rf_chip_phy_t *phy, int channel_sel,int dbfs_p125dB);
extern int    fn_rf_loop_back_ena      (rf_chip_phy_t *phy, int channel_sel , int ena , int tdd_sel, RF_LOOPBACK_REGS * rf_loopback_reg);
extern int    fn_if_loop_back_ena      (rf_chip_phy_t *phy, int channel_sel , int ena , IF_LOOPBACK_REGS * if_loopback_reg);
extern int    fn_rf_loop_back_lg_ena      (rf_chip_phy_t *phy, int channel_sel,int ena,RF_LOOPBACK_REGS * rf_loopback_regs);
extern int    fn_tx_dac_autoscale         (rf_chip_phy_t *phy, int channel_sel , int *tx_dc_i,int *tx_dc_q);
extern int    fn_rx_ana_offset_cal_1brach (rf_chip_phy_t *phy, int channel_sel , int iq_sel);
extern int    fn_rx_ana_offset_cal        (rf_chip_phy_t *phy, int channel_sel , int target);
extern int    fn_rx_ana_offset_cal_V2     (rf_chip_phy_t *phy, int channel_sel );
extern int    fn_rx_ana_set_gain          (rf_chip_phy_t *phy, int channel_sel , int mixer_atten, int lpf_atten);
extern int    fn_rx_get_mixer_port        (rf_chip_phy_t *phy, int channel_sel );
#define DIG_QEC_D1
#define QEC_DEBUG
#ifndef DIG_QEC_D1
extern int    fn_rx_load_lg_bw_coeff   (rf_chip_phy_t *phy, int channel_sel);
#endif

extern void fn_connect_rxlo_to_sxrx(rf_chip_phy_t * phy,int channel,unsigned long long rx_lo);
extern void fn_recovery_txlo_to_sxrx(rf_chip_phy_t * phy)	;
extern void fn_tx_hbf_reset(rf_chip_phy_t * phy,int chn);

extern void txqec_diag_info_init(
	TX_QEC_DIAG_INFO * txqec_diag                  ,
	int                wb_num                      ,
	int                debug_level                );

extern void rxqec_diag_info_init(
	RX_QEC_DIAG_INFO * rxqec_diag                  ,
	int                wb_num                      ,
	int                chk_num                     ,
	int                debug_level                );

extern void txlol_diag_info_init(
	TX_LOL_DIAG_INFO * txlol_diag                  ,
	int                wb_num                      ,
	int                debug_level                 );
	
extern void rxqec_diag_info_free(
		RX_QEC_DIAG_INFO * rxqec_diag                  );
		
extern void txqec_diag_info_free(
		TX_QEC_DIAG_INFO * txqec_diag                  );

extern void txlol_diag_info_free(
		TX_LOL_DIAG_INFO * txlol_diag                  );
		
extern void fn_open_loop_back(
	rf_chip_phy_t *    phy             , 
	int                chn             ,
	int                extloop_sel     ,
	int                tdd_sel         , 
	int                rxqec_sel       ,
	int                backoff_dbfs    , 
	unsigned long long lo_freq         ,
	unsigned int       sample_rate     ,
	int              * loopback_type   ,
	RF_LOOPBACK_REGS * rf_loopback_reg
	);
	
extern void fn_close_loop_back(
	rf_chip_phy_t    * phy           ,
	int                chn           ,
	int                loopback_type ,
	RF_LOOPBACK_REGS * rf_loopback_reg);
	
extern void fn_lpf_index_auto_change(
	rf_chip_phy_t    * phy          ,
	int                chn          ,
	int              * lpf_index
);

extern void fn_tx_qec_diag_print(
	rf_chip_phy_t    * phy           ,
	TX_QEC_DIAG_INFO * tx_qec_diag_info
);

extern void fn_tx_lol_diag_print(
	rf_chip_phy_t    * phy           ,
	TX_LOL_DIAG_INFO * tx_lol_diag_info
);

extern  int rxqec_diag_info_diag (
	rf_chip_phy_t    * phy           ,
	int                debug_level   ,
	RX_QEC_DIAG_INFO * rx_qec_diag_info);

extern  int txqec_diag_info_diag (
	rf_chip_phy_t    * phy           ,
	TX_QEC_DIAG_INFO * tx_qec_diag_info);

extern  int txlol_diag_info_diag (
	rf_chip_phy_t    * phy           ,
	TX_LOL_DIAG_INFO * tx_lol_diag_info);
	
extern double fn_cal_irr(double mag,double phg);

#endif
