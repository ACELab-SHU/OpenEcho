#ifndef  __QEC_CHIP_H
#define  __QEC_CHIP_H

#include "qec_common.h"

extern int    fn_rx_run_goertzle(rf_chip_phy_t *phy, int chn, int goertlze_pattern, int wb_div_32_0,int wb_div_32_1, int goertlze_num, int coherent_num ,
                                         int * goertzle_0_i, int * goertzle_0_q, int * goertzle_1_i, int * goertzle_1_q, int * goertzle_shift);
extern int    fn_rx_run_goertzle_cfg(rf_chip_phy_t *phy, int chn, int goertlze_pattern, int wb_div_32_0,int wb_div_32_1, int goertlze_num, int coherent_num);

extern int    fn_rx_run_goertzle_new(rf_chip_phy_t *phy, int chn, int goertlze_pattern, int wb_div_32_0,int wb_div_32_1, int goertlze_num, int coherent_num ,
                                         int * goertzle_0_i, int * goertzle_0_q, int * goertzle_1_i, int * goertzle_1_q, int * goertzle_shift);
extern int    fn_rx_run_qec           (rf_chip_phy_t *phy, 
	                                       int                      chn               ,
                                           RX_QEC_CFG_REGS        * rx_qec_cfg        );

extern int    fn_rx_qec_off           (rf_chip_phy_t *phy, int      chn    );

extern int    fn_rx_qec_cal_words     (
                                           double                 * mag_error_ratio   ,
                                           double                 * phase_error_deg   ,
                                           int                      compen_num        ,
                                           RX_QEC_CFG_REGS        * rx_qec_cfg        );

extern int    fn_tx_send_tone         (rf_chip_phy_t *phy, 
	                                       int                      chn               ,
                                           int                      ena               ,
                                           int                      wb_div_64         );

extern int	 fn_tx_send_tone_neg	 (rf_chip_phy_t *phy, 
	                                      int					   chn				 ,
										  int					   ena				 ,
										  int					   wb_div_64		 );

extern int    fn_tx_qec_gain_set(rf_chip_phy_t *phy, 
	                                       int                      chn               ,
	                                       TX_QEC_CFG_REGS * tx_qec_cfg               ,
                                           int                      scale_num         );

extern int    fn_tx_qec_off           (rf_chip_phy_t *phy, 
                                           int                      chn               ,
                                           int                      fiiq_sel          ,
                                           int                      fdiq_sel         );

extern int    fn_tx_run_qec           (rf_chip_phy_t *phy, 
	                                       int                      chn               ,
                                           TX_QEC_CFG_REGS        * tx_qec_cfg        );

extern int    fn_tx_qec_cal_words     (double                   mag_error_ratio   ,
                                           double                   phase_error_deg   ,
                                           TX_QEC_CFG_REGS        * tx_qec_cfg        );

extern int    fn_tx_run_lol           (rf_chip_phy_t *phy, 
                                           int                      chn               ,
                                           int                      dc_i              ,
                                           int                      dc_q              );


extern int    fn_rx_qec_self_calib    (rf_chip_phy_t *phy, 
	                                       int                      chn               ,
                                           int                      ext_loop          ,
	                                       int                      tdd_sel           ,
                                           unsigned long long       rx_lo             ,
                                           unsigned int             sample_rate       ,
                                           int                      signal_bandwidth  ,
                                           RX_QEC_CFG_REGS        * rx_qec_cfg        );


extern int	 fn_tx_lol_self_calib_rflp	   (rf_chip_phy_t *phy, 
                                          int 					 chn			   ,
										  int					 * dc_i 			 ,
										  int					 * dc_q 			 );

extern int	 fn_tx_qec_lol_self_calib_rflp(rf_chip_phy_t *phy, 
                                           int						chn 			  ,
											   unsigned long long bb_fs,
										  unsigned long long	   tx_lo			 ,
                                          int   ext_loop           ,
                                          int   qec_dbfs           ,
                                          int   lol_dbfs           ,
										  TX_QEC_CFG_REGS		 * tx_qec_cfg		 );

extern int	 fn_tx_qec_5tone_method_rflp  (rf_chip_phy_t         * phy                , 
                                          int					   chn 			      ,
										  TX_QEC_CFG_REGS		 * tx_qec_cfg		  ,
										  TX_QEC_DIAG_INFO        * txqec_diag_info    );

extern int	 fn_tx_lol_5tone_method_rflp  (rf_chip_phy_t *phy, 
                                               int						chn 			  ,
											   TX_QEC_CFG_REGS		 * tx_qec_cfg		  ,
											   TX_LOL_DIAG_INFO      * txlol_diag_info);

extern int	  fn_rx_qec_blind_est		   (rf_chip_phy_t *phy, 
	   int rx_select,
	   int coh_num,
	   int freq_div_32,
	   double *mag_ratio,
	   double *ph_error,
	   long long int *usv_rpt);


extern void	  fn_rx_qec_tracking(rf_chip_phy_t *phy, 
												   int		   rx_select,
												   int                      dbfs_th    ,
												   RX_QEC_CFG_REGS		  * rx_qec_cfg);

extern	int fn_rx_qec_update_fiiq_coeff(rf_chip_phy_t *phy, int chn, double mag_ratio,double ph_error);


extern int    fn_tx_qec_lol_cal_rflp (int chn, int ext_loop, int qec_dbfs, int lol_dbfs, void *arg);
extern int    fn_rx_qec_cal      (int chn, int ext_loop, void *arg);
extern short  fn_tx_qec_lol_lut_update(int chn, void *arg);
extern short  fn_rx_qec_lut_update(int chn, void *arg);

#endif
