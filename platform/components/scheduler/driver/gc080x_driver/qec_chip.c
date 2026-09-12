#include "qec_common.h"
#include "qec_chip.h"
#include "platform.h"
#include "driver.h"
#include "register.h"

#undef PI
#define PI (3.14159265359)

#define LLABS(a)   ((a)<0? (-1ll*(a)) : ((a)))
//#define QEC_DELAY 1
//#define QEC_PRINT 1
//#define RX_QEC_BYPASS 1
//#define TX_QEC_BYPASS 1
//#define CH_BYPASS 1
/****************************************************************************************/
// qec test
/****************************************************************************************/
int write_rx_qec_cal_word(rf_chip_phy_t *phy, TRX_CHN_ENUM chn, BANDWITH_ENUM bandwith, RX_QEC_CFG_REGS *rx_qec_cfg)
{
    int i=0, base;
    unsigned short h8, l8;
	unsigned char vals[4];

    base = chn ? 856: 344;
    for (i=0; i<16; i++)
    {
        //h8 = (rx_qec_cfg->fdiq_fir[2*i] & 0xffff) >> 8;
        //l8 = rx_qec_cfg->fdiq_fir[2*i] & 0xff;
        //vals[0] = h8;
        //vals[1] = l8;
        h8 = (rx_qec_cfg->fdiq_fir[2*i+1] & 0xffff) >> 8;
        l8 = rx_qec_cfg->fdiq_fir[2*i+1] & 0xff;
        vals[0] = l8;
        vals[1] = h8;
        h8 = (rx_qec_cfg->fdiq_fir[2*i] & 0xffff) >> 8;
        l8 = rx_qec_cfg->fdiq_fir[2*i] & 0xff;
        vals[2] = l8;
        vals[3] = h8;
        write_lut_word(phy, TXRX_IQ_LUT, base+i+bandwith*16, vals);
    }

    return 0;
}

int write_tx_qec_lol_cal_word(rf_chip_phy_t *phy, TRX_CHN_ENUM chn, BANDWITH_ENUM bandwith, short band_index, short bw_index, short bb_gain, TX_QEC_CFG_REGS *tx_qec_cfg)
{
    int i=0, end, offset, base;
    unsigned short h8, l8;
	unsigned char vals[4];

    offset = bb_gain * 2;
	base = chn ? 512 : 0;
    for (i=0; i<4; i++)
    {
        h8 = (tx_qec_cfg->dc_q & 0xffff) >> 8;
        l8 = tx_qec_cfg->dc_q & 0xff;
        vals[0] = l8;
        vals[1] = h8;
        h8 = (tx_qec_cfg->dc_i & 0xffff) >> 8;
        l8 = tx_qec_cfg->dc_i & 0xff;
        vals[2] = l8;
        vals[3] = h8;
        write_lut_word(phy, TXRX_IQ_LUT, base+i*50+offset, vals);

        //h8 = (tx_qec_cfg->fiiq_imag & 0xffff) >> 8;
        //l8 = tx_qec_cfg->fiiq_imag & 0xff;
        //vals[0] = h8;
        //vals[1] = l8;
        h8 = (tx_qec_cfg->fiiq_real & 0xffff) >> 8;
        l8 = tx_qec_cfg->fiiq_real & 0xff;
        vals[0] = l8;
        vals[1] = h8;
        h8 = (tx_qec_cfg->fiiq_imag & 0xffff) >> 8;
        l8 = tx_qec_cfg->fiiq_imag & 0xff;
        vals[2] = l8;
        vals[3] = h8;
        write_lut_word(phy, TXRX_IQ_LUT, base+i*50+1+offset, vals);
    }

    end = chn ? 712: 200;
    for (i=0; i<16; i++)
    {
        //h8 = (tx_qec_cfg->fdiq_fir[2*i] & 0xffff) >> 8;
        //l8 = tx_qec_cfg->fdiq_fir[2*i] & 0xff;
        //vals[0] = h8;
        //vals[1] = l8;
        h8 = (tx_qec_cfg->fdiq_fir[2*i+1] & 0xffff) >> 8;
        l8 = tx_qec_cfg->fdiq_fir[2*i+1] & 0xff;
        vals[0] = l8;
        vals[1] = h8;
        h8 = (tx_qec_cfg->fdiq_fir[2*i] & 0xffff) >> 8;
        l8 = tx_qec_cfg->fdiq_fir[2*i] & 0xff;
        vals[2] = l8;
        vals[3] = h8;
        write_lut_word(phy, TXRX_IQ_LUT, end+i+bandwith*16, vals);
    }

    return 0;
}


int rx_band_selection(rf_chip_phy_t *phy, int channel, unsigned long long flo)
{
	//LOG_MDEBUG(phy, TRX_QEC_CAL, "chn=%d, rxlo=%llu\n", channel, flo);
	set_trx_lo(phy, RX_DIR, channel, flo);
	
	return 0;
}

int tx_band_selection(rf_chip_phy_t *phy, int channel, unsigned long long flo)
{
	//LOG_MDEBUG(phy, TRX_QEC_CAL, "chn=%d, txlo=%llu\n", channel, flo);
	set_trx_lo(phy, TX_DIR, channel, flo);
	
	return 0;
}

int fn_tx_qec_lol_cal_rflp (int chn, int ext_loop, int qec_dbfs, int lol_dbfs, void *arg)
{
	int sample_rate, signal_bandwidth[]={1080000, 2700000, 4500000, 9000000, 18000000, 38000000, 58000000, 78000000, 98000000};
	short map_bw;
	rf_chip_phy_t *phy = (rf_chip_phy_t *)arg;
	int bb_index;

	TX_QEC_CFG_REGS tx_qec_cfg;
	unsigned long long txlo;
	short tdd_sel=0;
	int flag_qec_ok = 0;
	int flag_lol_ok = 0;
	RF_LOOPBACK_REGS rf_loopback_reg;
	unsigned long long tx_freq_offset;
	unsigned int reg_tia_offset;
	unsigned int reg_bbf_offset;
	int mean_power;
	double rssi_0,rssi_1,rssi_2,rssi_3;
	int bbf_gain;
	int tia_gain;
	int error_cnt = 0;
	double rssi;
	unsigned char reg_val;
	int loopback_type = 0;  // 0 rf loopback | 1 rf low gain loopback;
	int lpf_index;

	
	map_bw = user_map_to_standard(phy->config->bandwith);
	if ((phy->config->custom_bandwith_flag) || (phy->config->syspll_cfg_flag))
		sample_rate = phy->config->bb_sample_rate;
	else
		sample_rate = g_trx_sample_rate_table[map_bw].bb_sample_rate;
	txlo = phy->config->tx_flo;
	LOG_MAIN("[%s,%d] sample_rate=%d\n", __func__, __LINE__, sample_rate);

	TX_QEC_DIAG_INFO  tx_qec_diag_info;
	TX_LOL_DIAG_INFO  tx_lol_diag_info;

	// FORCE 
	memset(&tx_qec_cfg,0,sizeof(TX_QEC_CFG_REGS));

	tx_qec_cfg.debug_level = 1;
	tx_qec_cfg.speed_level = 0;
	
#ifdef TX_QEC_BYPASS
	return;
#endif

//#if CH_BYPASS == 0
//	if(chn==0)
//		return;
//#endif
//
//#if CH_BYPASS == 1
//	if(chn==1)
//		return;
//#endif
	if(tx_qec_cfg.debug_level>=1)
	{
		LOG_MDEBUG(phy,TRX_QEC_CAL,"+++++++++++++++[txqec_cal_rflp]+++++ debug info\n");
	}
	
	if(tx_qec_cfg.debug_level>=4)
	{
		phy->module_debug = 0x40;
		LOG_MDEBUG(phy, TRX_QEC_CAL, "tx qec cal, chn: %d ,extloop: %d \n",chn ,ext_loop);
	}

    // set pa index = 0
	tdd_sel = ((phy->config->mode >= CH1_TDD) && (phy->config->mode <= CH1_CH2_TDD));
    set_tx_atten_ext(phy, chn, 1, txlo, 0);
	set_tx_atten_ext(phy, chn ,0, txlo, 0);
	fn_tx_hbf_reset (phy, chn);
	
	if(tdd_sel == 1)
	{
		 fn_connect_rxlo_to_sxrx(phy,chn,txlo);
	}
	
	txqec_diag_info_init(&tx_qec_diag_info,6,tx_qec_cfg.debug_level);
	txlol_diag_info_init(&tx_lol_diag_info,6,tx_qec_cfg.debug_level);	
		
	fn_open_loop_back(
		phy             , 
		chn             ,
		ext_loop        ,
		tdd_sel         , 
		0               ,    // tx qec
		qec_dbfs        ,    // backup power
		txlo            ,
		sample_rate     ,
		&loopback_type  ,
		&rf_loopback_reg
	);
	tx_qec_diag_info.loopback_type = loopback_type;

	fn_rx_run_goertzle_cfg(phy, chn,0,1 ,1, 4096, 8 );
	
	// ------------------ tx qec calibration --------------------- //
	tx_qec_cfg.cal_init = 0;
	
	if(tx_qec_cfg.debug_level>=3)
	{
		LOG_MDEBUG(phy,TRX_QEC_CAL,"========== tx qec info ===========\n");
	}
	
    for(bb_index=0; bb_index<=5; bb_index++)
    {
        set_tx_atten_ext(phy, chn ,0, txlo, bb_index);
		if(tx_qec_cfg.debug_level>=3)
		{
			LOG_MDEBUG(phy,TRX_QEC_CAL,"--- bb_index %d ---\n",bb_index);
		}
		
		if(bb_index ==0)
		{
			tx_freq_offset = sample_rate/32ull*2ull;
			rx_band_selection (phy,  chn, txlo-tx_freq_offset);
			fn_tx_qec_off     (phy,  chn, 1 , 1);
			fn_tx_send_tone   (phy,  chn, 1 , 2);
			fn_tx_qec_gain_set(phy,  chn, &tx_qec_cfg , qec_dbfs*2);
			
			fn_lpf_index_auto_change(
				phy          ,
				chn          ,
				&lpf_index
			);
			tx_qec_diag_info.lpf_index     = lpf_index;
			
			//rx_dc_offset_cal_setting(phy,chn,phy->config->rx_port[chn],tia_gain,12-bbf_gain);
			//fn_rx_ana_offset_cal(phy, chn,10);
			fn_rx_ana_offset_cal_V2(phy,chn);
			
			fn_rx_get_adc_offset(phy, chn,10,&tx_qec_diag_info.rx_dc_i,&tx_qec_diag_info.rx_dc_q);
			fn_rx_get_adc_power (phy, chn,4, &tx_qec_diag_info.rx_power);	
			
			if(abs(tx_qec_diag_info.rx_dc_i)/10 > 400 || abs(tx_qec_diag_info.rx_dc_q)/10 > 400 )
			{
				LOG_MDEBUG(phy,TRX_QEC_CAL,"TX QEC ERROR: DC cannot remove\n");
				if(tx_qec_cfg.debug_level>=2)
				{
					while(1){}
				}
				else
				{
					flag_qec_ok = -1;
				}
			}
			else if(abs(tx_qec_diag_info.rx_dc_i)/10 > 100 || abs(tx_qec_diag_info.rx_dc_q)/10 > 100 )
			{
				LOG_MDEBUG(phy,TRX_QEC_CAL,"TX QEC WARNING: DC cannot remove\n");
			}
			
			rssi = 20.0*log10(sqrt((double)tx_qec_diag_info.rx_power/4.0)/pow(2.0,9.0));	
			if(rssi > -3 )
			{
				LOG_MDEBUG(phy,TRX_QEC_CAL,"TX QEC ERROR  : LOOPBACK power is too large\n");
				if(tx_qec_cfg.debug_level>=2)
				{
					while(1){}
				}
				else
				{
					flag_qec_ok = -1;
				}
			}
			else if(rssi> -6 )
			{
				LOG_MDEBUG(phy,TRX_QEC_CAL,"TX QEC WARNING: LOOPBACK power is too large\n");	
			}
			else if(rssi < -50 )
			{
				LOG_MDEBUG(phy,TRX_QEC_CAL,"TX QEC ERROR  : LOOPBACK power is too small\n");
				if(tx_qec_cfg.debug_level>=2)
				{
					while(1){}
				}
				else
				{
					flag_qec_ok = -1;
				}
			}	
			else if (rssi < -30 )
			{
				LOG_MDEBUG(phy,TRX_QEC_CAL,"TX QEC WARNING : LOOPBACK power is too small\n");
			}
		}
		if(flag_qec_ok != -1)
		{
			flag_qec_ok = fn_tx_qec_5tone_method_rflp(phy, chn,&tx_qec_cfg,&tx_qec_diag_info);
		}
		tx_qec_cfg.cal_init = 1;

		if(flag_qec_ok==-1)
		{
			tx_qec_cfg.fiiq_imag = 0x4000;
			tx_qec_cfg.fiiq_real = 0x0;
		}
		memcpy(&(phy->tx_qec[chn][bb_index]), &tx_qec_cfg, sizeof(TX_QEC_CFG_REGS));
		
		if(tx_qec_cfg.debug_level>=4)
		{
			LOG_MDEBUG(phy, TRX_QEC_CAL, "tx qec bb index %d done !\n",bb_index);
			hal_spi_write_reg(phy, 0x700,0x2);
			hal_spi_write_reg(phy, 0x748,0x2);
			CHIP_UDELAY(8000000);
		}
		
	}
	
	// ------------------ tx lol calibration --------------------- //
	for(bb_index=0; bb_index<=5; bb_index++)
	{
		set_tx_atten_ext(phy, chn ,0, txlo, bb_index);
		memcpy(&tx_qec_cfg,&(phy->tx_qec[chn][bb_index]),sizeof(TX_QEC_CFG_REGS));
		fn_tx_run_qec(phy, chn, &tx_qec_cfg );
		
		if(bb_index == 0 && tx_qec_cfg.debug_level>=3)
		{
			LOG_MDEBUG(phy,TRX_QEC_CAL,"========== tx lol info ===========\n");
		}
		if(tx_qec_cfg.debug_level>=3)
		{
			LOG_MDEBUG(phy,TRX_QEC_CAL,"--- bb_index %d ---\n",bb_index);
		}		
		if(bb_index==0)
		{
			tx_qec_cfg.cal_init = 0;
			fn_tx_qec_gain_set(phy,  chn, &tx_qec_cfg , lol_dbfs*2);
			tx_freq_offset = sample_rate/32ull*1ull;
			rx_band_selection	  (phy,  chn, txlo-tx_freq_offset);
			fn_lpf_index_auto_change(
				phy          ,
				chn          ,
				&lpf_index
			);
			tx_lol_diag_info.lpf_index     = lpf_index;
			fn_rx_ana_offset_cal_V2(phy,chn);
			fn_rx_get_adc_offset(phy, chn,10,&tx_lol_diag_info.rx_dc_i,&tx_lol_diag_info.rx_dc_q);
			fn_rx_get_adc_power (phy, chn,4, &tx_lol_diag_info.rx_power);	
			if(abs(tx_lol_diag_info.rx_dc_i)/10 > 400 || abs(tx_lol_diag_info.rx_dc_q)/10 > 400 )
			{
				LOG_MDEBUG(phy,TRX_QEC_CAL,"TX LOL ERROR: DC cannot remove\n");
				if(tx_qec_cfg.debug_level>=2)
				{
					while(1){}
				}
				else
				{
					flag_lol_ok = -1;
				}
			}
			else if(abs(tx_lol_diag_info.rx_dc_i)/10 > 100 || abs(tx_lol_diag_info.rx_dc_q)/10 > 100 )
			{
				LOG_MDEBUG(phy,TRX_QEC_CAL,"TX LOL WARNING: DC cannot remove\n");
			}
			
			rssi = 20.0*log10(sqrt((double)tx_lol_diag_info.rx_power/4.0)/pow(2.0,9.0));	
			if(rssi > -3 )
			{
				LOG_MDEBUG(phy,TRX_QEC_CAL,"TX LOL ERROR  : LOOPBACK power is too large\n");
				if(tx_qec_cfg.debug_level>=2)
				{
					while(1){}
				}
				else
				{
					flag_lol_ok = -1;
				}
			}
			else if(rssi> -6 )
			{
				LOG_MDEBUG(phy,TRX_QEC_CAL,"TX LOL WARNING: LOOPBACK power is too large\n");		
			}
			else if(rssi < -50 )
			{
				LOG_MDEBUG(phy,TRX_QEC_CAL,"TX LOL ERROR  : LOOPBACK power is too small\n");
				if(tx_qec_cfg.debug_level>=2)
				{
					while(1){}
				}
				else
				{
					flag_lol_ok = -1;
				}
			}	
			else if (rssi < -30 )
			{
				LOG_MDEBUG(phy,TRX_QEC_CAL,"TX LOL WARNING : LOOPBACK power is too small\n");
			}			
		}

		if(flag_lol_ok!=-1)
		{
			flag_lol_ok = fn_tx_lol_5tone_method_rflp(phy, chn, &tx_qec_cfg , &tx_lol_diag_info);
		}
		
		tx_qec_cfg.cal_init = 1;
		
		if(flag_lol_ok==-1)
		{
			tx_qec_cfg.dc_i = 0x0;
			tx_qec_cfg.dc_q = 0x0;
		}
		
		if(tx_qec_cfg.debug_level>=4)
		{
			LOG_MDEBUG(phy, TRX_QEC_CAL, "tx lol bb index %d done !\n",bb_index);
			hal_spi_write_reg(phy, 0x700,0x2);
			hal_spi_write_reg(phy, 0x748,0x2);
			CHIP_UDELAY(8000000);
		}

		memcpy(&(phy->tx_qec[chn][bb_index]), &tx_qec_cfg, sizeof(TX_QEC_CFG_REGS));

		if(bb_index!=5)
		{
			phy->tx_qec[chn][bb_index+1].dc_i= tx_qec_cfg.dc_i;
			phy->tx_qec[chn][bb_index+1].dc_q= tx_qec_cfg.dc_q;
		}	
	}

	fn_tx_qec_gain_set(phy,  chn, &tx_qec_cfg , 2);
	for(bb_index = 0;bb_index <=5;bb_index++)
	{
		phy->tx_qec[chn][bb_index].fdiq_fir[16] = tx_qec_cfg.fdiq_fir[16];
		write_tx_qec_lol_cal_word(phy, (TRX_CHN_ENUM)chn, phy->config->bandwith, 0, 0, bb_index, &phy->tx_qec[chn][bb_index]);
	}

	if(tx_qec_cfg.debug_level>=4)
	{
		LOG_MDEBUG(phy, TRX_QEC_CAL, "QEC and LOL done \n");
	}

	// step 6 close if loopback route
	fn_close_loop_back(
		phy           ,
		chn           ,
		loopback_type ,
		&rf_loopback_reg);


    // set qec parametere bb index =0 back into RG register, because of TDD.
    set_tx_atten_ext(phy, chn ,0,txlo,0);
    fn_tx_run_qec(phy, chn, &(phy->tx_qec[chn][0]));
    fn_tx_run_lol(phy, chn,
                      phy->tx_qec[chn][0].dc_i,
                      phy->tx_qec[chn][0].dc_q);

    if ((phy->config->bandwidthswitch_flag != 1) || (phy->config->bandwidthswitch_length <= 1))
    {
        phy->tx_qec_flag[chn] = 1;
    }

	// step 8 close tone and close backoff.
	fn_tx_qec_gain_set(phy, chn,&tx_qec_cfg,2);
	fn_tx_send_tone   (phy,  chn, 0 , 2 );
	
	if(tx_qec_cfg.debug_level>=1)
	{
		fn_tx_qec_diag_print(phy,&tx_qec_diag_info);
		fn_tx_lol_diag_print(phy,&tx_lol_diag_info);		
	}

	txqec_diag_info_free(&tx_qec_diag_info);
	txlol_diag_info_free(&tx_lol_diag_info);	
	
	if(flag_qec_ok == -1 || flag_lol_ok == -1)
	{
		return -1 ;
	}
	else
	{
		return 0;
	}
}


int fn_rx_qec_cal(int chn, int ext_loop, void *arg)
{
	int sample_rate, signal_bandwidth[]={1080000, 2700000, 4500000, 9000000, 18000000, 38000000, 58000000, 78000000, 98000000}, bw_hz;
	short map_bw, mode;
	rf_chip_phy_t *phy = (rf_chip_phy_t *)arg;
	RX_QEC_CFG_REGS rx_qec_cfg;
	
	#ifdef RX_QEC_BYPASS
	return 0;
	#endif

	map_bw = user_map_to_standard(phy->config->bandwith);
	mode = phy->config->mode;
	mode = (mode>=5)? 1: 0;

	if ((phy->config->syspll_cfg_flag) || (phy->config->custom_bandwith_flag))
		sample_rate =  phy->config->bb_sample_rate;
	else
		sample_rate = g_trx_sample_rate_table[map_bw].bb_sample_rate;

	if (ext_loop == 1)
	{
		if (chn==0)
			hal_spi_write_reg(phy, 0x700, 0x2);
		else
			hal_spi_write_reg(phy, 0x748, 0x2);
	}
	
	//fn_rx_qec_self_calib(phy, chn, ext_loop, mode, 2595000000ull, sample_rate, signal_bandwidth[phy->config->bandwith], &rx_qec_cfg);
	if (phy->config->custom_bandwith_flag)
		bw_hz = phy->config->custom_bandwith;
	else
    	bw_hz = signal_bandwidth[phy->config->bandwith];
	fn_rx_qec_self_calib(phy, chn, ext_loop, mode, phy->config->rx_flo, sample_rate, bw_hz, &rx_qec_cfg);
	LOG_MAIN("[%s,%d] sample_rate=%d\n", __func__, __LINE__, sample_rate);

	memcpy(&(phy->rx_qec[chn]), &rx_qec_cfg, sizeof(RX_QEC_CFG_REGS));
	write_rx_qec_cal_word(phy, (TRX_CHN_ENUM)chn, phy->config->bandwith, &rx_qec_cfg);

    if ((phy->config->bandwidthswitch_flag != 1) || (phy->config->bandwidthswitch_length <= 1))
    {
        phy->rx_qec_flag[chn] = 1;
    }

	if (chn==0)
		hal_spi_write_reg(phy, 0x700, 0x0);
	else
		hal_spi_write_reg(phy, 0x748, 0x0);
		
    return 0;
}

short fn_tx_qec_lol_lut_update(int chn, void *arg)
{
	rf_chip_phy_t *phy = (rf_chip_phy_t *)arg;
	short bb_index;

    for(bb_index=0; bb_index<=5; bb_index++)
        write_tx_qec_lol_cal_word(phy, (TRX_CHN_ENUM)chn, phy->config->bandwith, 0, 0, bb_index, &(phy->tx_qec[chn][bb_index]));

	return 0;
}

short fn_rx_qec_lut_update(int chn, void *arg)
{
	short offset;
	rf_chip_phy_t *phy = (rf_chip_phy_t *)arg;
	
	offset = (chn==TRX_CHN1) ? 0: 0x800;
	write_rx_qec_cal_word(phy, (TRX_CHN_ENUM)chn, phy->config->bandwith, &(phy->rx_qec[chn]));
	hal_spi_write_reg(phy, 0x220+offset, (phy->rx_qec[chn].fiiq_real) & 0xff);
	hal_spi_write_reg(phy, 0x221+offset, (phy->rx_qec[chn].fiiq_real >> 8) & 0xff);
	hal_spi_write_reg(phy, 0x224+offset, (phy->rx_qec[chn].fiiq_imag) & 0xff);
	hal_spi_write_reg(phy, 0x225+offset, (phy->rx_qec[chn].fiiq_imag >> 8) & 0xff);
	return 0;
}

/****************************************************************************************/
// chip qec module
/****************************************************************************************/
int fn_rx_qec_update_fiiq_coeff(rf_chip_phy_t *phy, int chn, double mag_ratio,double ph_error)
{
	double       fiiq_rad_error;
	double       fiiq_k_coeff  ;
	double       fiiq_k_real   ;
	double       fiiq_k_imag   ;
	int          k_real        ;
	int          k_imag        ;
	unsigned int w_addr        ;
	unsigned int w_data        ;
	unsigned int r_addr        ;
	unsigned int r_data        ;

	fiiq_rad_error = ph_error / 360.0 * 2 * PI;
    //fiiq_k_coeff = 1.0 / (pow(((1.0 + cos(fiiq_rad_error)) * mag_ratio), 2.0) + pow((mag_ratio * sin(fiiq_rad_error)), 2.0));
    fiiq_k_coeff = 1.0 / (pow((1.0 + cos(fiiq_rad_error) * mag_ratio), 2.0) + pow((mag_ratio * sin(fiiq_rad_error)), 2.0));
	fiiq_k_real = fiiq_k_coeff * ( 1.0 - mag_ratio * mag_ratio);
	fiiq_k_imag = fiiq_k_coeff * (-2.0 * mag_ratio * sin(fiiq_rad_error));

	k_real = (int)floor(((fiiq_k_real * pow(2.0, 15)) + 0.5));
	k_imag = (int)floor(((fiiq_k_imag * pow(2.0, 15)) + 0.5));
	
	w_addr = 0xa21 - (1-chn)*2048;
	w_data = ((k_real & 0xFFFF) >> 8)&0xFF;
	hal_spi_write_reg(phy, w_addr,w_data);

	// fiiq_real low 8bit
	w_addr = 0xa20 - (1-chn)*2048;
	w_data = ((k_real & 0xFFFF) >> 0)&0xFF;
	hal_spi_write_reg(phy, w_addr,w_data);

	// fiiq_imag high 8bit
	w_addr = 0xa25 - (1-chn)*2048;
	w_data = ((k_imag & 0xFFFF) >> 8)&0xFF;
	hal_spi_write_reg(phy, w_addr,w_data);

	// fiiq_imag low 8bit
	w_addr = 0xa24 - (1-chn)*2048;
	w_data = ((k_imag & 0xFFFF) >> 0)&0xFF;
	hal_spi_write_reg(phy, w_addr,w_data);

	// set latch 0->1->0
	r_addr = 0xa03 - (1-chn)*2048;
	r_data = hal_spi_read_reg(phy, r_addr);
	w_addr = r_addr;
	w_data = (r_data & 0xfe) ;
	hal_spi_write_reg(phy, w_addr,w_data);
	w_data = (r_data & 0xfe) | 0x01;
	hal_spi_write_reg(phy, w_addr,w_data);
	w_data = (r_data & 0xfe);
	hal_spi_write_reg(phy, w_addr,w_data);

	return 1;
}

int fn_rx_qec_cal_words(double* mag_error_ratio, double* phase_error_deg, int compen_num, RX_QEC_CFG_REGS* rx_qec_cfg)
{
    double fiiq_mag_ratio = mag_error_ratio[0];
    double fiiq_deg_error = phase_error_deg[0];
    double fiiq_rad_error = 0;
    double fiiq_k_coeff;
    double fiiq_k_real;
    double fiiq_k_imag;
    double fdiq_mag_ratio[16];
    double fdiq_deg_error[16];
    double fdiq_fir_coeff[32];

    int k_real;
    int k_imag;
    int i;


    if (compen_num > 15 || compen_num<0)
        return -1;

    for (i = 0; i <16; i++)
    {
        if (i <= compen_num)
        {
            fdiq_mag_ratio[i] = mag_error_ratio[i] / mag_error_ratio[0];
            fdiq_deg_error[i] = phase_error_deg[i] - phase_error_deg[0];
        }
        else
        {
            //fdiq_mag_ratio[i] = mag_error_ratio[compen_num-1] / mag_error_ratio[0];
            //fdiq_deg_error[i] = phase_error_deg[compen_num-1] - phase_error_deg[0];
			
			fdiq_mag_ratio[i] = (1.0 - fdiq_mag_ratio[compen_num])/((double)(15-compen_num)) * (i-compen_num) + fdiq_mag_ratio[compen_num];
			fdiq_deg_error[i] = (0.0 - fdiq_deg_error[compen_num])/((double)(15-compen_num)) * (i-compen_num) + fdiq_deg_error[compen_num];
			
        }
		//LOG_MDEBUG(phy, TRX_QEC_CAL, "\tRX QEC CAL Words %4d| mag_ratio %.4f deg_error %.4f \n",i,fdiq_mag_ratio[i],fdiq_deg_error[i]);
    }
	
    //fdiq_mag_ratio[0] = fdiq_mag_ratio[1];
    //fdiq_deg_error[0] = fdiq_deg_error[1];


    /*
    for (i = 0; i <16; i++)
    {
        if (i < compen_num)
        {
            fdiq_mag_ratio[i] = mag_error_ratio[i] / mag_error_ratio[0];
            fdiq_deg_error[i] = phase_error_deg[i] - phase_error_deg[0];
        }
        else
        {
            fdiq_mag_ratio[i] = mag_error_ratio[compen_num] / mag_error_ratio[0];
            fdiq_deg_error[i] = phase_error_deg[compen_num] - phase_error_deg[0];
        }

    }
    */
    // FIIQ compensation
    fiiq_rad_error = fiiq_deg_error / 360.0 * 2 * PI;
    fiiq_k_coeff = 1.0 / (pow((1.0 + cos(fiiq_rad_error) * fiiq_mag_ratio), 2.0) + pow((fiiq_mag_ratio * sin(fiiq_rad_error)), 2.0));
    fiiq_k_real = fiiq_k_coeff * (1.0 - fiiq_mag_ratio * fiiq_mag_ratio);
    fiiq_k_imag = fiiq_k_coeff * (-2.0 * fiiq_mag_ratio * sin(fiiq_rad_error));

    k_real = (int)floor(((fiiq_k_real * pow(2.0, 15)) + 0.5));
    k_imag = (int)floor(((fiiq_k_imag * pow(2.0, 15)) + 0.5));

    rx_qec_cfg->fiiq_real = k_real;
    rx_qec_cfg->fiiq_imag = k_imag;

    double dft_i[32];
    double dft_q[32];

    for (i = 0; i < 16; i++)
    {
        dft_i[i] =  fdiq_mag_ratio[i] * cos(2.0*PI*fdiq_deg_error[i]/360.0) * pow(-1.0,(double)i) ;
        dft_q[i] =  fdiq_mag_ratio[i] * sin(2.0*PI*fdiq_deg_error[i]/360.0) * pow(-1.0,(double)i) ;
    }

    dft_i[0] = 1;
    dft_q[0] = 0;

	dft_i[16] = -1*dft_i[15];
	dft_q[16] = -1*dft_q[15];
	//dft_i[16] = 1.0;
	//dft_q[16] = 0;

    for (i = 17; i < 32; i++)
    {
        dft_i[i] = dft_i[32-i];
        dft_q[i] = dft_q[32-i] * -1.0;
    }


    fn_cal_inverse_dft32_point(dft_i, dft_q,fdiq_fir_coeff );


    for (i = 0; i < 32; i++)
    {
        rx_qec_cfg->fdiq_fir[i] = (int) (fdiq_fir_coeff[i] * pow(2.0,14.0)+ 0.5);
    }

    return 0;
}



int fn_rx_run_qec(rf_chip_phy_t *phy, int chn, RX_QEC_CFG_REGS* rx_qec_cfg)
{

    unsigned int w_addr   ;
    unsigned int w_data   ;
    unsigned int r_addr   ;
    unsigned int r_data   ;
    int i;


    // STEP 3 Write FDIQ and FIIQ coeffcient

    if (1)
    {
        // 3.1 fiiq_real high 8bit
        w_addr = 0xa21 - (1-chn)*2048;
        w_data = ((rx_qec_cfg->fiiq_real & 0xFFFF) >> 8)&0xFF;
        hal_spi_write_reg(phy, w_addr,w_data);

        // fiiq_real low 8bit
        w_addr = 0xa20 - (1-chn)*2048;
        w_data = ((rx_qec_cfg->fiiq_real & 0xFFFF) >> 0)&0xFF;
        hal_spi_write_reg(phy, w_addr,w_data);

        // fiiq_imag high 8bit
        w_addr = 0xa25 - (1-chn)*2048;
        w_data = ((rx_qec_cfg->fiiq_imag & 0xFFFF) >> 8)&0xFF;
        hal_spi_write_reg(phy, w_addr,w_data);

        // fiiq_imag low 8bit
        w_addr = 0xa24 - (1-chn)*2048;
        w_data = ((rx_qec_cfg->fiiq_imag & 0xFFFF) >> 0)&0xFF;
        hal_spi_write_reg(phy, w_addr,w_data);

        // set latch 0->1->0
        r_addr = 0xa03 - (1-chn)*2048;
        r_data = hal_spi_read_reg(phy, r_addr);
        w_addr = r_addr;
        w_data = (r_data & 0xfe) ;
        hal_spi_write_reg(phy, w_addr,w_data);
        w_data = (r_data & 0xfe) | 0x01;
        hal_spi_write_reg(phy, w_addr,w_data);
        w_data = (r_data & 0xfe);
        hal_spi_write_reg(phy, w_addr,w_data);

        // fdiq fir setting
        for (i = 0; i < 32; i++)
        {
            w_addr = 0xC00 - (1-chn)*2048 + i*4;
            w_data = ((rx_qec_cfg->fdiq_fir[i] & 0xFFFF) >> 0) & 0xFF;
            hal_spi_write_reg(phy, w_addr,w_data);

            w_addr = 0xC01 - (1-chn)*2048 + i*4;
            w_data = ((rx_qec_cfg->fdiq_fir[i] & 0xFFFF) >> 8) & 0xFF;
            hal_spi_write_reg(phy, w_addr,w_data);
        }

        // set i delay ena = 1 and valid
        r_addr = 0xa03 - (1-chn)*2048;
        r_data = hal_spi_read_reg(phy, r_addr);
        w_addr = r_addr;
        w_data = (r_data & 0xef) | 0x10;
        hal_spi_write_reg(phy, r_addr,w_data) ;

        // set latch 0->1->0
        r_addr = 0xa03 - (1-chn)*2048;
        r_data = hal_spi_read_reg(phy, r_addr);
        w_addr = r_addr;
        w_data = r_data & 0xfe ;
        hal_spi_write_reg(phy, r_addr,w_data) ;
        w_data = (r_data & 0xfe) | 0x01;
        hal_spi_write_reg(phy, r_addr,w_data) ;
        w_data = (r_data & 0xfe) ;
        hal_spi_write_reg(phy, r_addr,w_data) ;


        // rx fiiq enable and fdiq enable
        r_addr = 0xa00 - (1-chn)*2048;
        r_data = hal_spi_read_reg(phy, r_addr);
        w_addr = r_addr;
        w_data = (r_data & 0xf9 ) | 0x06;
        //w_data = (r_data & 0xf9 ) | 0x04;
        hal_spi_write_reg(phy, w_addr,w_data);
    }


    return 0;
}

int fn_rx_qec_off(rf_chip_phy_t *phy, int chn)
{
    unsigned int w_addr   ;
    unsigned int w_data   ;
    unsigned int r_addr   ;
    unsigned int r_data   ;

    // STEP 3
    // rx fiiq enable and fdiq enable
    r_addr = 0xa00 - (1-chn)*2048;
    r_data = hal_spi_read_reg(phy, r_addr);
    w_addr = r_addr;
    w_data = (r_data & 0xf9 );
    hal_spi_write_reg(phy, w_addr,w_data);


    return 1;
}

static unsigned int COS_VALUE_25B[16] = { 0x0FFFFFF,0x0FB14BE,0x0EC835E,0x0D4DB30,0x0B504F2,0x08E39D9,0x061F78A,0x031F170,0x0000000,0x1CE0E90,0x19E0876,0x171C627,0x14AFB0E,0x12B24D0,0x1137CA2,0x104EB42 };
static unsigned int SIN_VALUE_25B[16] = { 0x0000000,0x031F170,0x061F78A,0x08E39D9,0x0B504F2,0x0D4DB30,0x0EC835E,0x0FB14BE,0x0FFFFFF,0x0FB14BE,0x0EC835E,0x0D4DB30,0x0B504F2,0x08E39D9,0x061F78A,0x031F170 };
int fn_rx_run_goertzle(rf_chip_phy_t *phy, int chn, int goertlze_pattern, int wb_div_32_0,int wb_div_32_1, int goertlze_num, int coherent_num ,
    int * goertzle_0_i, int * goertzle_0_q, int * goertzle_1_i, int * goertzle_1_q, int * goertzle_shift)
{
    // Set Goertzle pattern : 0 -> G0 on I, G1 on Q
    unsigned int w_addr   ;
    unsigned int w_data   ;
    unsigned int r_addr   ;
    unsigned int r_data   ;
    unsigned int w_data_t ;
    unsigned int r_flag   ;
    unsigned int r_cnt    ;

    long long r_data_i_0_s64   = 0;
    long long r_data_q_0_s64   = 0;
    long long r_data_i_1_s64   = 0;
    long long r_data_q_1_s64   = 0;
    long long r_data_max_abs   = 0;
    long long r_data_shift_val = 0;
    long long i;

    int wb_div_32_0_r = wb_div_32_0;
    int wb_div_32_1_r = wb_div_32_1;
	
	int print_flag = 0;
	
	if (wb_div_32_0==-1 && wb_div_32_1==-1)
	{
		print_flag = 0;
	}
	
	if(print_flag==1) {LOG_MDEBUG(phy,TRX_QEC_CAL,"chn                : %d \n",chn             );}
	if(print_flag==1) {LOG_MDEBUG(phy,TRX_QEC_CAL,"goertlze_pattern   : %d \n",goertlze_pattern);}
	if(print_flag==1) {LOG_MDEBUG(phy,TRX_QEC_CAL,"wb_div_32_0        : %d \n",wb_div_32_0     );}
	if(print_flag==1) {LOG_MDEBUG(phy,TRX_QEC_CAL,"wb_div_32_0        : %d \n",wb_div_32_1     );}
	if(print_flag==1) {LOG_MDEBUG(phy,TRX_QEC_CAL,"goertlze_num       : %d \n",goertlze_num    );}
	if(print_flag==1) {LOG_MDEBUG(phy,TRX_QEC_CAL,"coherent_num       : %d \n",coherent_num    );}

    if(wb_div_32_0<0)
    {
        wb_div_32_0_r = -1*wb_div_32_0;
    }

    if(wb_div_32_1<0)
    {
        wb_div_32_1_r = -1*wb_div_32_1;
    }

    r_addr = REG_RX2_QEC_CTRL_1 - (1-chn)*2048;
    r_data = hal_spi_read_reg(phy, r_addr);
    w_addr = r_addr;
    w_data = (r_data & 0xcf ) | ((((unsigned int)goertlze_pattern)&0x3)<<4);
    hal_spi_write_reg(phy, w_addr,w_data);
	if(print_flag==1) {LOG_MDEBUG(phy,TRX_QEC_CAL,"./ad9361 w 0x%x 0x%x\n",w_addr,w_data);}
	
	
    // Set Goertzle length
    w_addr = REG_RX2_GOER_LEN - (1-chn)*2048;
    w_data = goertlze_num / 8 - 1;
    hal_spi_write_reg(phy, w_addr,w_data);
	if(print_flag==1) {LOG_MDEBUG(phy,TRX_QEC_CAL,"./ad9361 w 0x%x 0x%x\n",w_addr,w_data);}

    // Set Goertzle accu length
    w_addr = REG_RX2_GOER_ACC - (1-chn)*2048;
    w_data = coherent_num - 1;
    hal_spi_write_reg(phy, w_addr,w_data);
	if(print_flag==1) {LOG_MDEBUG(phy,TRX_QEC_CAL,"./ad9361 w 0x%x 0x%x\n",w_addr,w_data);}

    // Set Goertzle Freq
    w_data_t = COS_VALUE_25B[wb_div_32_0_r];
    w_data = (w_data_t >> 0)&0xFF;
    w_addr = REG_RX2_GOER_FRQ0I_0 - (1-chn)*2048;
    hal_spi_write_reg(phy, w_addr,w_data);
	if(print_flag==1) {LOG_MDEBUG(phy,TRX_QEC_CAL,"./ad9361 w 0x%x 0x%x\n",w_addr,w_data);}

    w_data = (w_data_t >> 8)&0xFF;
    w_addr = REG_RX2_GOER_FRQ0I_1 - (1-chn)*2048;
    hal_spi_write_reg(phy, w_addr,w_data);
	if(print_flag==1) {LOG_MDEBUG(phy,TRX_QEC_CAL,"./ad9361 w 0x%x 0x%x\n",w_addr,w_data);}

    w_data = (w_data_t >> 16)&0xFF;
    w_addr = REG_RX2_GOER_FRQ0I_2 - (1-chn)*2048;
    hal_spi_write_reg(phy, w_addr,w_data);
	if(print_flag==1) {LOG_MDEBUG(phy,TRX_QEC_CAL,"./ad9361 w 0x%x 0x%x\n",w_addr,w_data);}

    w_data = (w_data_t >> 24)&0x01;
    w_addr = REG_RX2_GOER_FRQ0I_3 - (1-chn)*2048;
    hal_spi_write_reg(phy, w_addr,w_data);
	if(print_flag==1) {LOG_MDEBUG(phy,TRX_QEC_CAL,"./ad9361 w 0x%x 0x%x\n",w_addr,w_data);}

    if(wb_div_32_0<0)
    {
        w_data_t = (~SIN_VALUE_25B[wb_div_32_0_r]+1) & 0x01FFFFFF;
    }
    else
    {
        w_data_t = SIN_VALUE_25B[wb_div_32_0_r];
    }

    w_data = (w_data_t >> 0)&0xFF;
    w_addr = REG_RX2_GOER_FRQ0Q_0 - (1-chn)*2048;
    hal_spi_write_reg(phy, w_addr,w_data);
	if(print_flag==1) {LOG_MDEBUG(phy,TRX_QEC_CAL,"./ad9361 w 0x%x 0x%x\n",w_addr,w_data);}

    w_data = (w_data_t >> 8)&0xFF;
    w_addr = REG_RX2_GOER_FRQ0Q_1 - (1-chn)*2048;
    hal_spi_write_reg(phy, w_addr,w_data);
	if(print_flag==1) {LOG_MDEBUG(phy,TRX_QEC_CAL,"./ad9361 w 0x%x 0x%x\n",w_addr,w_data);}

    w_data = (w_data_t >> 16)&0xFF;
    w_addr = REG_RX2_GOER_FRQ0Q_2 - (1-chn)*2048;
    hal_spi_write_reg(phy, w_addr,w_data);
	if(print_flag==1) {LOG_MDEBUG(phy,TRX_QEC_CAL,"./ad9361 w 0x%x 0x%x\n",w_addr,w_data);}

    w_data = (w_data_t >> 24)&0x01;
    w_addr = REG_RX2_GOER_FRQ0Q_3 - (1-chn)*2048;
    hal_spi_write_reg(phy, w_addr,w_data);
	if(print_flag==1) {LOG_MDEBUG(phy,TRX_QEC_CAL,"./ad9361 w 0x%x 0x%x\n",w_addr,w_data);}

    w_data_t = COS_VALUE_25B[wb_div_32_1_r];
    w_data = (w_data_t >> 0)&0xFF;
    w_addr = REG_RX2_GOER_FRQ1I_0 - (1-chn)*2048;
    hal_spi_write_reg(phy, w_addr,w_data);
	if(print_flag==1) {LOG_MDEBUG(phy,TRX_QEC_CAL,"./ad9361 w 0x%x 0x%x\n",w_addr,w_data);}

    w_data = (w_data_t >> 8)&0xFF;
    w_addr = REG_RX2_GOER_FRQ1I_1 - (1-chn)*2048;
    hal_spi_write_reg(phy, w_addr,w_data);
	if(print_flag==1) {LOG_MDEBUG(phy,TRX_QEC_CAL,"./ad9361 w 0x%x 0x%x\n",w_addr,w_data);}

    w_data = (w_data_t >> 16)&0xFF;
    w_addr = REG_RX2_GOER_FRQ1I_2 - (1-chn)*2048;
    hal_spi_write_reg(phy, w_addr,w_data);
	if(print_flag==1) {LOG_MDEBUG(phy,TRX_QEC_CAL,"./ad9361 w 0x%x 0x%x\n",w_addr,w_data);}

    w_data = (w_data_t >> 24)&0x01;
    w_addr = REG_RX2_GOER_FRQ1I_3 - (1-chn)*2048;
    hal_spi_write_reg(phy, w_addr,w_data);
	if(print_flag==1) {LOG_MDEBUG(phy,TRX_QEC_CAL,"./ad9361 w 0x%x 0x%x\n",w_addr,w_data);}

    if(wb_div_32_1<0)
    {
        w_data_t = (~SIN_VALUE_25B[wb_div_32_1_r]+1) & 0x01FFFFFF;
    }
    else
    {
        w_data_t = SIN_VALUE_25B[wb_div_32_1_r];
    }

    w_data = (w_data_t >> 0)&0xFF;
    w_addr = REG_RX2_GOER_FRQ1Q_0 - (1-chn)*2048;
    hal_spi_write_reg(phy, w_addr,w_data);
	if(print_flag==1) {LOG_MDEBUG(phy,TRX_QEC_CAL,"./ad9361 w 0x%x 0x%x\n",w_addr,w_data);}

    w_data = (w_data_t >> 8)&0xFF;
    w_addr = REG_RX2_GOER_FRQ1Q_1 - (1-chn)*2048;
    hal_spi_write_reg(phy, w_addr,w_data);
	if(print_flag==1) {LOG_MDEBUG(phy,TRX_QEC_CAL,"./ad9361 w 0x%x 0x%x\n",w_addr,w_data);}

    w_data = (w_data_t >> 16)&0xFF;
    w_addr = REG_RX2_GOER_FRQ1Q_2 - (1-chn)*2048;
    hal_spi_write_reg(phy, w_addr,w_data);
	if(print_flag==1) {LOG_MDEBUG(phy,TRX_QEC_CAL,"./ad9361 w 0x%x 0x%x\n",w_addr,w_data);}

    w_data = (w_data_t >> 24)&0x01;
    w_addr = REG_RX2_GOER_FRQ1Q_3 - (1-chn)*2048;
    hal_spi_write_reg(phy, w_addr,w_data);
	if(print_flag==1) {LOG_MDEBUG(phy,TRX_QEC_CAL,"./ad9361 w 0x%x 0x%x\n",w_addr,w_data);}

    // Set Goertzle Start!
    r_addr = REG_RX2_QEC_CTRL_0 - (1-chn)*2048;
    r_data = hal_spi_read_reg(phy, r_addr);
    w_addr = r_addr;
    w_data = (r_data & 0xfe ) & 0xfe;
    hal_spi_write_reg(phy, w_addr,w_data);
	if(print_flag==1) {LOG_MDEBUG(phy,TRX_QEC_CAL,"./ad9361 w 0x%x 0x%x\n",w_addr,w_data);}
    w_data = (r_data & 0xfe ) | 0x01;
    hal_spi_write_reg(phy, w_addr,w_data);
	if(print_flag==1) {LOG_MDEBUG(phy,TRX_QEC_CAL,"./ad9361 w 0x%x 0x%x\n",w_addr,w_data);}
    w_data = (r_data & 0xfe ) & 0xfe;
    hal_spi_write_reg(phy, w_addr,w_data);
	if(print_flag==1) {LOG_MDEBUG(phy,TRX_QEC_CAL,"./ad9361 w 0x%x 0x%x\n",w_addr,w_data);}

    // Wait until Goertzle done
    r_cnt  = 0;
    r_addr = REG_RX2_GOER_DONE - (1-chn)*2048;
    do
    {
        r_cnt++;
        //QThread::msleep(1);
        CHIP_UDELAY(1000);
        r_data = hal_spi_read_reg(phy, r_addr);
		if(print_flag==1) {LOG_MDEBUG(phy,TRX_QEC_CAL,"./ad9361 r 0x%x = 0x%x\n",r_addr,r_data);}
        r_flag = (r_data >> 0x0) & 0x1;
		if (r_cnt > 100)
        {
            break;
        }
    }
    while(r_flag == 0);

    r_addr = REG_RX2_GOER_PRT0I_0 - (1-chn)*2048;
	r_data = hal_spi_read_reg(phy, r_addr);
	if(print_flag==1) {LOG_MDEBUG(phy,TRX_QEC_CAL,"./ad9361 r 0x%x = 0x%x\n",r_addr,r_data);}
    r_data_i_0_s64 += (long long)r_data;
	
    r_addr = REG_RX2_GOER_PRT0I_1 - (1-chn)*2048;
	r_data = hal_spi_read_reg(phy, r_addr);
	if(print_flag==1) {LOG_MDEBUG(phy,TRX_QEC_CAL,"./ad9361 r 0x%x = 0x%x\n",r_addr,r_data);}
    r_data_i_0_s64 += ((long long)r_data)<<8ll;

    r_addr = REG_RX2_GOER_PRT0I_2 - (1-chn)*2048;
	r_data = hal_spi_read_reg(phy, r_addr);
	if(print_flag==1) {LOG_MDEBUG(phy,TRX_QEC_CAL,"./ad9361 r 0x%x = 0x%x\n",r_addr,r_data);}
    r_data_i_0_s64 += ((long long)r_data)<<16ll;

    r_addr = REG_RX2_GOER_PRT0I_3 - (1-chn)*2048;
	r_data = hal_spi_read_reg(phy, r_addr);
	if(print_flag==1) {LOG_MDEBUG(phy,TRX_QEC_CAL,"./ad9361 r 0x%x = 0x%x\n",r_addr,r_data);}
    r_data_i_0_s64 += ((long long)r_data)<<24ll;

    r_addr = REG_RX2_GOER_PRT0I_4 - (1-chn)*2048;
	r_data = hal_spi_read_reg(phy, r_addr);
	if(print_flag==1) {LOG_MDEBUG(phy,TRX_QEC_CAL,"./ad9361 r 0x%x = 0x%x\n",r_addr,r_data);}
    r_data_i_0_s64 += ((long long)r_data)<<32ll;


    r_addr = REG_RX2_GOER_PRT0Q_0 - (1-chn)*2048;
	r_data = hal_spi_read_reg(phy, r_addr);
	if(print_flag==1) {LOG_MDEBUG(phy,TRX_QEC_CAL,"./ad9361 r 0x%x = 0x%x\n",r_addr,r_data);}
    r_data_q_0_s64 += (long long)r_data;

    r_addr = REG_RX2_GOER_PRT0Q_1 - (1-chn)*2048;
	r_data = hal_spi_read_reg(phy, r_addr);
	if(print_flag==1) {LOG_MDEBUG(phy,TRX_QEC_CAL,"./ad9361 r 0x%x = 0x%x\n",r_addr,r_data);}
    r_data_q_0_s64 += ((long long)r_data)<<8ll;

    r_addr = REG_RX2_GOER_PRT0Q_2 - (1-chn)*2048;
	r_data = hal_spi_read_reg(phy, r_addr);
	if(print_flag==1) {LOG_MDEBUG(phy,TRX_QEC_CAL,"./ad9361 r 0x%x = 0x%x\n",r_addr,r_data);}
    r_data_q_0_s64 += ((long long)r_data)<<16ll;

    r_addr = REG_RX2_GOER_PRT0Q_3 - (1-chn)*2048;
	r_data = hal_spi_read_reg(phy, r_addr);
	if(print_flag==1) {LOG_MDEBUG(phy,TRX_QEC_CAL,"./ad9361 r 0x%x = 0x%x\n",r_addr,r_data);}
    r_data_q_0_s64 += ((long long)r_data)<<24ll;

    r_addr = REG_RX2_GOER_PRT0Q_4 - (1-chn)*2048;
	r_data = hal_spi_read_reg(phy, r_addr);
	if(print_flag==1) {LOG_MDEBUG(phy,TRX_QEC_CAL,"./ad9361 r 0x%x = 0x%x\n",r_addr,r_data);}
    r_data_q_0_s64 += ((long long)r_data)<<32ll;

    if (r_data_i_0_s64 >= (1ll<<39ll))
    {
        r_data_i_0_s64 = r_data_i_0_s64 - (1ll<<40ll);
    }

    if (r_data_q_0_s64 >= (1ll<<39ll))
    {
        r_data_q_0_s64 = r_data_q_0_s64 - (1ll<<40ll);
    }

    r_addr = REG_RX2_GOER_RPT0_PWR - (1-chn)*2048;
    r_data = hal_spi_read_reg(phy, r_addr);
	if(print_flag==1) {LOG_MDEBUG(phy,TRX_QEC_CAL,"./ad9361 r 0x%x = 0x%x\n",r_addr,r_data);}
    r_data_i_0_s64 = (r_data_i_0_s64) << ((long long)r_data);
    r_data_q_0_s64 = (r_data_q_0_s64) << ((long long)r_data);


    r_addr = REG_RX2_GOER_PRT1I_0 - (1-chn)*2048;
	r_data = hal_spi_read_reg(phy, r_addr);
	if(print_flag==1) {LOG_MDEBUG(phy,TRX_QEC_CAL,"./ad9361 r 0x%x = 0x%x\n",r_addr,r_data);}
    r_data_i_1_s64 += (long long)(r_data);

    r_addr = REG_RX2_GOER_PRT1I_1 - (1-chn)*2048;
	r_data = hal_spi_read_reg(phy, r_addr);
	if(print_flag==1) {LOG_MDEBUG(phy,TRX_QEC_CAL,"./ad9361 r 0x%x = 0x%x\n",r_addr,r_data);}
    r_data_i_1_s64 += ((long long)r_data) <<8ll;

    r_addr = REG_RX2_GOER_PRT1I_2 - (1-chn)*2048;
	r_data = hal_spi_read_reg(phy, r_addr);
	if(print_flag==1) {LOG_MDEBUG(phy,TRX_QEC_CAL,"./ad9361 r 0x%x = 0x%x\n",r_addr,r_data);}
    r_data_i_1_s64 += ((long long)r_data) <<16ll;

    r_addr = REG_RX2_GOER_PRT1I_3 - (1-chn)*2048;
	r_data = hal_spi_read_reg(phy, r_addr);
	if(print_flag==1) {LOG_MDEBUG(phy,TRX_QEC_CAL,"./ad9361 r 0x%x = 0x%x\n",r_addr,r_data);}
    r_data_i_1_s64 += ((long long)r_data) <<24ll;

    r_addr = REG_RX2_GOER_PRT1I_4 - (1-chn)*2048;
	r_data = hal_spi_read_reg(phy, r_addr);
	if(print_flag==1) {LOG_MDEBUG(phy,TRX_QEC_CAL,"./ad9361 r 0x%x = 0x%x\n",r_addr,r_data);}
    r_data_i_1_s64 += ((long long)r_data) <<32ll;

    r_addr = REG_RX2_GOER_PRT1Q_0 - (1-chn)*2048;
	r_data = hal_spi_read_reg(phy, r_addr);
	if(print_flag==1) {LOG_MDEBUG(phy,TRX_QEC_CAL,"./ad9361 r 0x%x = 0x%x\n",r_addr,r_data);}
    r_data_q_1_s64 += (long long)r_data;

    r_addr = REG_RX2_GOER_PRT1Q_1 - (1-chn)*2048;
	r_data = hal_spi_read_reg(phy, r_addr);
	if(print_flag==1) {LOG_MDEBUG(phy,TRX_QEC_CAL,"./ad9361 r 0x%x = 0x%x\n",r_addr,r_data);}
    r_data_q_1_s64 += ((long long)r_data) <<8ll;

    r_addr = REG_RX2_GOER_PRT1Q_2 - (1-chn)*2048;
	r_data = hal_spi_read_reg(phy, r_addr);
	if(print_flag==1) {LOG_MDEBUG(phy,TRX_QEC_CAL,"./ad9361 r 0x%x = 0x%x\n",r_addr,r_data);}
    r_data_q_1_s64 += ((long long)r_data) <<16ll;

    r_addr = REG_RX2_GOER_PRT1Q_3 - (1-chn)*2048;
	r_data = hal_spi_read_reg(phy, r_addr);
	if(print_flag==1) {LOG_MDEBUG(phy,TRX_QEC_CAL,"./ad9361 r 0x%x = 0x%x\n",r_addr,r_data);}
    r_data_q_1_s64 += ((long long)r_data) <<24ll;

    r_addr = REG_RX2_GOER_PRT1Q_4 - (1-chn)*2048;
	r_data = hal_spi_read_reg(phy, r_addr);
	if(print_flag==1) {LOG_MDEBUG(phy,TRX_QEC_CAL,"./ad9361 r 0x%x = 0x%x\n",r_addr,r_data);}
    r_data_q_1_s64 += ((long long)r_data) <<32ll;

    if (r_data_i_1_s64 >= (1ll<<39ll))
    {
        r_data_i_1_s64 = r_data_i_1_s64 - (1ll<<40ll);
    }

    if (r_data_q_1_s64 >= (1ll<<39ll))
    {
        r_data_q_1_s64 = r_data_q_1_s64 - (1ll<<40ll);
    }

    r_addr = REG_RX2_GOER_RPT1_PWR - (1-chn)*2048;
    r_data = hal_spi_read_reg(phy, r_addr);
	if(print_flag==1) {LOG_MDEBUG(phy,TRX_QEC_CAL,"./ad9361 r 0x%x = 0x%x\n",r_addr,r_data);}
    r_data_i_1_s64 = (r_data_i_1_s64) << ((long long)r_data);
    r_data_q_1_s64 = (r_data_q_1_s64) << ((long long)r_data);

    r_data_max_abs   = 0;
    r_data_shift_val = 0;

    r_data_max_abs = (r_data_max_abs < LLABS(r_data_i_0_s64)) ? LLABS(r_data_i_0_s64) : r_data_max_abs;
    r_data_max_abs = (r_data_max_abs < LLABS(r_data_q_0_s64)) ? LLABS(r_data_q_0_s64) : r_data_max_abs;
    r_data_max_abs = (r_data_max_abs < LLABS(r_data_i_1_s64)) ? LLABS(r_data_i_1_s64) : r_data_max_abs;
    r_data_max_abs = (r_data_max_abs < LLABS(r_data_q_1_s64)) ? LLABS(r_data_q_1_s64) : r_data_max_abs;
	
	if(print_flag==1) {LOG_MDEBUG(phy,TRX_QEC_CAL,"r_data_max_abs %lld\n",r_data_max_abs);}
	
    r_data_shift_val = 28ll;
    for (i = 63; i >= 28; i--)
    {
        if (((r_data_max_abs >> i) & ((long long)0x1)) == (long long)0x1)
        {
            r_data_shift_val = i;
            break;
        }
    }
	if(print_flag==1) {LOG_MDEBUG(phy,TRX_QEC_CAL,"r_data_i_0_s64 %lld\n",r_data_i_0_s64 );}
	if(print_flag==1) {LOG_MDEBUG(phy,TRX_QEC_CAL,"r_data_i_1_s64 %lld\n",r_data_i_1_s64 );}
	if(print_flag==1) {LOG_MDEBUG(phy,TRX_QEC_CAL,"r_data_q_0_s64 %lld\n",r_data_q_0_s64 );}
	if(print_flag==1) {LOG_MDEBUG(phy,TRX_QEC_CAL,"r_data_q_1_s64 %lld\n",r_data_q_1_s64 );}
	if(print_flag==1) {LOG_MDEBUG(phy,TRX_QEC_CAL,"r_data_shift_val %lld\n",r_data_shift_val );}
	
    r_data_i_0_s64	  =   (r_data_i_0_s64 >> (r_data_shift_val-28ll));
    r_data_i_1_s64	  =   (r_data_i_1_s64 >> (r_data_shift_val-28ll));
    r_data_q_0_s64	  =   (r_data_q_0_s64 >> (r_data_shift_val-28ll));
    r_data_q_1_s64	  =   (r_data_q_1_s64 >> (r_data_shift_val-28ll));

    *goertzle_shift = (int ) (r_data_shift_val-28ll);
    *goertzle_0_i = (int )r_data_i_0_s64;
    *goertzle_0_q = (int )r_data_q_0_s64;
    *goertzle_1_i = (int )r_data_i_1_s64;
    *goertzle_1_q = (int )r_data_q_1_s64;

	if(print_flag==1) {LOG_MDEBUG(phy,TRX_QEC_CAL,"goertzle_shift %d\n",*goertzle_shift );}
	if(print_flag==1) {LOG_MDEBUG(phy,TRX_QEC_CAL,"goertzle_0_i   %d\n",*goertzle_0_i   );}
	if(print_flag==1) {LOG_MDEBUG(phy,TRX_QEC_CAL,"goertzle_0_q   %d\n",*goertzle_0_q   );}
	if(print_flag==1) {LOG_MDEBUG(phy,TRX_QEC_CAL,"goertzle_1_i   %d\n",*goertzle_1_i   );}	
	if(print_flag==1) {LOG_MDEBUG(phy,TRX_QEC_CAL,"goertzle_1_q   %d\n",*goertzle_1_q   );}
    return 0;
}

int fn_rx_run_goertzle_cfg(rf_chip_phy_t *phy, int chn, int goertlze_pattern, int wb_div_32_0,int wb_div_32_1, int goertlze_num, int coherent_num)
{
    // Set Goertzle pattern : 0 -> G0 on I, G1 on Q
    unsigned int w_addr   ;
    unsigned int w_data   ;
    unsigned int r_addr   ;
    unsigned int r_data   ;
    unsigned int w_data_t ;
    unsigned int r_flag   ;
    unsigned int r_cnt    ;

    long long r_data_i_0_s64   = 0;
    long long r_data_q_0_s64   = 0;
    long long r_data_i_1_s64   = 0;
    long long r_data_q_1_s64   = 0;
    long long r_data_max_abs   = 0;
    long long r_data_shift_val = 0;
    long long i;

    int wb_div_32_0_r = wb_div_32_0;
    int wb_div_32_1_r = wb_div_32_1;
    if(wb_div_32_0<0)
    {
        wb_div_32_0_r = -1*wb_div_32_0;
    }

    if(wb_div_32_1<0)
    {
        wb_div_32_1_r = -1*wb_div_32_1;
    }

    r_addr = REG_RX2_QEC_CTRL_1 - (1-chn)*2048;
    r_data = hal_spi_read_reg(phy, r_addr);
    w_addr = r_addr;
    w_data = (r_data & 0xcf ) | ((((unsigned int)goertlze_pattern)&0x3)<<4);
    hal_spi_write_reg(phy, w_addr,w_data);

    // Set Goertzle length
    w_addr = REG_RX2_GOER_LEN - (1-chn)*2048;
    w_data = goertlze_num / 8 - 1;
    hal_spi_write_reg(phy, w_addr,w_data);

    // Set Goertzle accu length
    w_addr = REG_RX2_GOER_ACC - (1-chn)*2048;
    w_data = coherent_num - 1;
    hal_spi_write_reg(phy, w_addr,w_data);

    // Set Goertzle Freq
    w_data_t = COS_VALUE_25B[wb_div_32_0_r];
    w_data = (w_data_t >> 0)&0xFF;
    w_addr = REG_RX2_GOER_FRQ0I_0 - (1-chn)*2048;
    hal_spi_write_reg(phy, w_addr,w_data);

    w_data = (w_data_t >> 8)&0xFF;
    w_addr = REG_RX2_GOER_FRQ0I_1 - (1-chn)*2048;
    hal_spi_write_reg(phy, w_addr,w_data);

    w_data = (w_data_t >> 16)&0xFF;
    w_addr = REG_RX2_GOER_FRQ0I_2 - (1-chn)*2048;
    hal_spi_write_reg(phy, w_addr,w_data);

    w_data = (w_data_t >> 24)&0x01;
    w_addr = REG_RX2_GOER_FRQ0I_3 - (1-chn)*2048;
    hal_spi_write_reg(phy, w_addr,w_data);

    if(wb_div_32_0<0)
    {
        w_data_t = (~SIN_VALUE_25B[wb_div_32_0_r]+1) & 0x01FFFFFF;
    }
    else
    {
        w_data_t = SIN_VALUE_25B[wb_div_32_0_r];
    }

    w_data = (w_data_t >> 0)&0xFF;
    w_addr = REG_RX2_GOER_FRQ0Q_0 - (1-chn)*2048;
    hal_spi_write_reg(phy, w_addr,w_data);

    w_data = (w_data_t >> 8)&0xFF;
    w_addr = REG_RX2_GOER_FRQ0Q_1 - (1-chn)*2048;
    hal_spi_write_reg(phy, w_addr,w_data);

    w_data = (w_data_t >> 16)&0xFF;
    w_addr = REG_RX2_GOER_FRQ0Q_2 - (1-chn)*2048;
    hal_spi_write_reg(phy, w_addr,w_data);

    w_data = (w_data_t >> 24)&0x01;
    w_addr = REG_RX2_GOER_FRQ0Q_3 - (1-chn)*2048;
    hal_spi_write_reg(phy, w_addr,w_data);

    w_data_t = COS_VALUE_25B[wb_div_32_1_r];
    w_data = (w_data_t >> 0)&0xFF;
    w_addr = REG_RX2_GOER_FRQ1I_0 - (1-chn)*2048;
    hal_spi_write_reg(phy, w_addr,w_data);

    w_data = (w_data_t >> 8)&0xFF;
    w_addr = REG_RX2_GOER_FRQ1I_1 - (1-chn)*2048;
    hal_spi_write_reg(phy, w_addr,w_data);

    w_data = (w_data_t >> 16)&0xFF;
    w_addr = REG_RX2_GOER_FRQ1I_2 - (1-chn)*2048;
    hal_spi_write_reg(phy, w_addr,w_data);

    w_data = (w_data_t >> 24)&0x01;
    w_addr = REG_RX2_GOER_FRQ1I_3 - (1-chn)*2048;
    hal_spi_write_reg(phy, w_addr,w_data);

    if(wb_div_32_1<0)
    {
        w_data_t = (~SIN_VALUE_25B[wb_div_32_1_r]+1) & 0x01FFFFFF;
    }
    else
    {
        w_data_t = SIN_VALUE_25B[wb_div_32_1_r];
    }

    w_data = (w_data_t >> 0)&0xFF;
    w_addr = REG_RX2_GOER_FRQ1Q_0 - (1-chn)*2048;
    hal_spi_write_reg(phy, w_addr,w_data);

    w_data = (w_data_t >> 8)&0xFF;
    w_addr = REG_RX2_GOER_FRQ1Q_1 - (1-chn)*2048;
    hal_spi_write_reg(phy, w_addr,w_data);

    w_data = (w_data_t >> 16)&0xFF;
    w_addr = REG_RX2_GOER_FRQ1Q_2 - (1-chn)*2048;
    hal_spi_write_reg(phy, w_addr,w_data);

    w_data = (w_data_t >> 24)&0x01;
    w_addr = REG_RX2_GOER_FRQ1Q_3 - (1-chn)*2048;
    hal_spi_write_reg(phy, w_addr,w_data);
	
	return 0 ;
}

int fn_rx_run_goertzle_new(rf_chip_phy_t *phy, int chn, int goertlze_pattern, int wb_div_32_0,int wb_div_32_1, int goertlze_num, int coherent_num ,
    int * goertzle_0_i, int * goertzle_0_q, int * goertzle_1_i, int * goertzle_1_q, int * goertzle_shift)
{
    // Set Goertzle pattern : 0 -> G0 on I, G1 on Q
    unsigned int w_addr   ;
    unsigned int w_data   ;
    unsigned int r_addr   ;
    unsigned int r_data   ;
    unsigned int w_data_t ;
    unsigned int r_flag   ;
    unsigned int r_cnt    ;

    long long r_data_i_0_s64   = 0;
    long long r_data_q_0_s64   = 0;
    long long r_data_i_1_s64   = 0;
    long long r_data_q_1_s64   = 0;
    long long r_data_max_abs   = 0;
    long long r_data_shift_val = 0;
    long long i;

     // Set Goertzle Start!
    r_addr = REG_RX2_QEC_CTRL_0 - (1-chn)*2048;
    r_data = hal_spi_read_reg(phy, r_addr);
    w_addr = r_addr;
    w_data = (r_data & 0xfe ) & 0xfe;
    hal_spi_write_reg(phy, w_addr,w_data);
    w_data = (r_data & 0xfe ) | 0x01;
    hal_spi_write_reg(phy, w_addr,w_data);
    w_data = (r_data & 0xfe ) & 0xfe;
    hal_spi_write_reg(phy, w_addr,w_data);

    // Wait until Goertzle done
    r_cnt  = 0;
    r_addr = REG_RX2_GOER_DONE - (1-chn)*2048;
    do
    {
        r_cnt++;
        //QThread::msleep(1);
        CHIP_UDELAY(100);
        r_data = hal_spi_read_reg(phy, r_addr);
        r_flag = (r_data >> 0x0) & 0x1;
      if (r_cnt > 100)
        {
            break;
        }
    }
    while(r_flag == 0);

    //r_addr = REG_RX2_GOER_PRT0I_0 - (1-chn)*2048;
    //r_data_i_0_s64 += (long long)(hal_spi_read_reg(phy, r_addr));
	
    //r_addr = REG_RX2_GOER_PRT0I_1 - (1-chn)*2048;
    //r_data_i_0_s64 += ((long long)(hal_spi_read_reg(phy, r_addr)))<<8ll;

    r_addr = REG_RX2_GOER_PRT0I_2 - (1-chn)*2048;
    r_data_i_0_s64 += ((long long)(hal_spi_read_reg(phy, r_addr)))<<16ll;

    r_addr = REG_RX2_GOER_PRT0I_3 - (1-chn)*2048;
    r_data_i_0_s64 += ((long long)(hal_spi_read_reg(phy, r_addr)))<<24ll;

    r_addr = REG_RX2_GOER_PRT0I_4 - (1-chn)*2048;
    r_data_i_0_s64 += ((long long)(hal_spi_read_reg(phy, r_addr)))<<32ll;


    //r_addr = REG_RX2_GOER_PRT0Q_0 - (1-chn)*2048;
    //r_data_q_0_s64 += (long long)(hal_spi_read_reg(phy, r_addr));

    //r_addr = REG_RX2_GOER_PRT0Q_1 - (1-chn)*2048;
    //r_data_q_0_s64 += ((long long)(hal_spi_read_reg(phy, r_addr)))<<8ll;

    r_addr = REG_RX2_GOER_PRT0Q_2 - (1-chn)*2048;
    r_data_q_0_s64 += ((long long)(hal_spi_read_reg(phy, r_addr)))<<16ll;

    r_addr = REG_RX2_GOER_PRT0Q_3 - (1-chn)*2048;
    r_data_q_0_s64 += ((long long)(hal_spi_read_reg(phy, r_addr)))<<24ll;

    r_addr = REG_RX2_GOER_PRT0Q_4 - (1-chn)*2048;
    r_data_q_0_s64 += ((long long)(hal_spi_read_reg(phy, r_addr)))<<32ll;

    if (r_data_i_0_s64 >= (1ll<<39ll))
    {
        r_data_i_0_s64 = r_data_i_0_s64 - (1ll<<40ll);
    }

    if (r_data_q_0_s64 >= (1ll<<39ll))
    {
        r_data_q_0_s64 = r_data_q_0_s64 - (1ll<<40ll);
    }
	//LOG_MDEBUG(phy,TRX_QEC_CAL,"log2 abs(r_data_i_0_s64) %.2f\n",log2((double)abs(r_data_i_0_s64)));
	//LOG_MDEBUG(phy,TRX_QEC_CAL,"log2 abs(r_data_q_0_s64) %.2f\n",log2((double)abs(r_data_q_0_s64)));
	
    //r_addr = REG_RX2_GOER_RPT0_PWR - (1-chn)*2048;
    //r_data = hal_spi_read_reg(phy, r_addr);
    //r_data_i_0_s64 = (r_data_i_0_s64) << ((long long)r_data);
    //r_data_q_0_s64 = (r_data_q_0_s64) << ((long long)r_data);
	//LOG_MDEBUG(phy,TRX_QEC_CAL,"shift %d\n",r_data);

    //r_addr = REG_RX2_GOER_PRT1I_0 - (1-chn)*2048;
    //r_data_i_1_s64 += (long long)(hal_spi_read_reg(phy, r_addr ));

    //r_addr = REG_RX2_GOER_PRT1I_1 - (1-chn)*2048;
    //r_data_i_1_s64 += ((long long)(hal_spi_read_reg(phy, r_addr))) <<8ll;

    r_addr = REG_RX2_GOER_PRT1I_2 - (1-chn)*2048;
    r_data_i_1_s64 += ((long long)(hal_spi_read_reg(phy, r_addr))) <<16ll;

    r_addr = REG_RX2_GOER_PRT1I_3 - (1-chn)*2048;
    r_data_i_1_s64 += ((long long)(hal_spi_read_reg(phy, r_addr))) <<24ll;

    r_addr = REG_RX2_GOER_PRT1I_4 - (1-chn)*2048;
    r_data_i_1_s64 += ((long long)(hal_spi_read_reg(phy, r_addr))) <<32ll;

    //r_addr = REG_RX2_GOER_PRT1Q_0 - (1-chn)*2048;
    //r_data_q_1_s64 += (long long)(hal_spi_read_reg(phy, r_addr));

    //r_addr = REG_RX2_GOER_PRT1Q_1 - (1-chn)*2048;
    //r_data_q_1_s64 += ((long long)(hal_spi_read_reg(phy, r_addr))) <<8ll;

    r_addr = REG_RX2_GOER_PRT1Q_2 - (1-chn)*2048;
    r_data_q_1_s64 += ((long long)(hal_spi_read_reg(phy, r_addr))) <<16ll;

    r_addr = REG_RX2_GOER_PRT1Q_3 - (1-chn)*2048;
    r_data_q_1_s64 += ((long long)(hal_spi_read_reg(phy, r_addr))) <<24ll;

    r_addr = REG_RX2_GOER_PRT1Q_4 - (1-chn)*2048;
    r_data_q_1_s64 += ((long long)(hal_spi_read_reg(phy, r_addr))) <<32ll;

    if (r_data_i_1_s64 >= (1ll<<39ll))
    {
        r_data_i_1_s64 = r_data_i_1_s64 - (1ll<<40ll);
    }

    if (r_data_q_1_s64 >= (1ll<<39ll))
    {
        r_data_q_1_s64 = r_data_q_1_s64 - (1ll<<40ll);
    }
	//LOG_MDEBUG(phy,TRX_QEC_CAL,"log2 abs(r_data_i_1_s64) %.2f\n",log2((double)abs(r_data_i_1_s64)));
	//LOG_MDEBUG(phy,TRX_QEC_CAL,"log2 abs(r_data_q_1_s64) %.2f\n",log2((double)abs(r_data_q_1_s64)));
	
	
    //r_addr = REG_RX2_GOER_RPT1_PWR - (1-chn)*2048;
    //r_data = hal_spi_read_reg(phy, r_addr);
    //r_data_i_1_s64 = (r_data_i_1_s64) << ((long long)r_data);
    //r_data_q_1_s64 = (r_data_q_1_s64) << ((long long)r_data);
	//LOG_MDEBUG(phy,TRX_QEC_CAL,"shift %d\n",r_data);
	
    r_data_max_abs   = 0;
    r_data_shift_val = 0;

    r_data_max_abs = (r_data_max_abs < LLABS(r_data_i_0_s64)) ? LLABS(r_data_i_0_s64) : r_data_max_abs;
    r_data_max_abs = (r_data_max_abs < LLABS(r_data_q_0_s64)) ? LLABS(r_data_q_0_s64) : r_data_max_abs;
    r_data_max_abs = (r_data_max_abs < LLABS(r_data_i_1_s64)) ? LLABS(r_data_i_1_s64) : r_data_max_abs;
    r_data_max_abs = (r_data_max_abs < LLABS(r_data_q_1_s64)) ? LLABS(r_data_q_1_s64) : r_data_max_abs;

    r_data_shift_val = 28ll;
    for (i = 63; i >= 28; i--)
    {
        if (((r_data_max_abs >> i) & 0x1) == 0x1)
        {
            r_data_shift_val = i;
            break;
        }
    }

    r_data_i_0_s64	  =   (r_data_i_0_s64 >> (r_data_shift_val-28ll));
    r_data_i_1_s64	  =   (r_data_i_1_s64 >> (r_data_shift_val-28ll));
    r_data_q_0_s64	  =   (r_data_q_0_s64 >> (r_data_shift_val-28ll));
    r_data_q_1_s64	  =   (r_data_q_1_s64 >> (r_data_shift_val-28ll));

    *goertzle_shift = (int ) (r_data_shift_val-28ll);
    *goertzle_0_i = (int )r_data_i_0_s64;
    *goertzle_0_q = (int )r_data_q_0_s64;
    *goertzle_1_i = (int )r_data_i_1_s64;
    *goertzle_1_q = (int )r_data_q_1_s64;


    return 0;
}


int fn_tx_send_tone (rf_chip_phy_t *phy, int chn, int ena ,int wb_div_64)
{
    unsigned int w_addr   ;
    unsigned int w_data   ;
    unsigned int r_addr   ;
    unsigned int r_data   ;

    if (ena == 1)
    {
        r_addr = 0xA00 - (1-chn)*2048;
        r_data = hal_spi_read_reg(phy, r_addr);
        w_addr = r_addr;
        w_data = (r_data & 0xf7) | 0x08;
        hal_spi_write_reg(phy, w_addr,w_data);

        w_addr = 0xA02 - (1-chn)*2048;
        w_data = (0<<7) + (0x3<<5) + (wb_div_64 & 0x1f);
        hal_spi_write_reg(phy, w_addr,w_data);
    }
    else
    {
        r_addr = 0xA00 - (1-chn)*2048;
        r_data = hal_spi_read_reg(phy, r_addr);
        w_addr = r_addr;
        w_data = r_data & 0xf7;;
        hal_spi_write_reg(phy, w_addr,w_data);
    }
    return 0;
}

int fn_tx_send_tone_neg (rf_chip_phy_t *phy, int chn, int ena ,int wb_div_64)
{
    unsigned int w_addr   ;
    unsigned int w_data   ;
    unsigned int r_addr   ;
    unsigned int r_data   ;

    if (ena == 1)
    {
        r_addr = 0xA00 - (1-chn)*2048;
        r_data = hal_spi_read_reg(phy, r_addr);
        w_addr = r_addr;
        w_data = (r_data & 0xf7) | 0x08;
        hal_spi_write_reg(phy, w_addr,w_data);

        w_addr = 0xA02 - (1-chn)*2048;
        w_data = (0<<7) + (0x2<<5) + (wb_div_64 & 0x1f);
        hal_spi_write_reg(phy, w_addr,w_data);
    }
    else
    {
        r_addr = 0xA00 - (1-chn)*2048;
        r_data = hal_spi_read_reg(phy, r_addr);
        w_addr = r_addr;
        w_data = r_data & 0xf7;;
        hal_spi_write_reg(phy, w_addr,w_data);
    }
    return 0;
}

void fn_tx_qec_coeff_scale(rf_chip_phy_t *phy, unsigned int addr_lower ,unsigned int addr_upper, int scale_val)
{
    unsigned int addr ;
    unsigned int data ;
    signed   int coeff;

    // read fiiq and set back
    coeff = 0;
    addr  = addr_lower;
    data  = hal_spi_read_reg(phy, addr);
    coeff = data;

    addr  = addr_upper;
    data  = hal_spi_read_reg(phy, addr);
    coeff = ((int)(data<<8)) + coeff;
    if( (coeff >> 15) &0x1)
    {
        coeff = coeff - (1<<16);
    }
/*
    if( scale_val >0)
    {
        coeff = (coeff >> (   scale_val));
    }
    else
    {
        coeff = (coeff << (-1*scale_val));
    }
*/

	coeff = (int)floor((((double)coeff * pow(10.0,((double)scale_val)*-1.5/20.0))+0.5));
	addr  = addr_lower;
	data  = (unsigned int) ( (coeff   ) & 0xFF);
	hal_spi_write_reg(phy,  addr, data );

    addr  = addr_upper;
    data  = (unsigned int) ( (coeff>>8) & 0xFF);
    hal_spi_write_reg(phy, addr ,data);
}

void fn_tx_qec_coeff_valid(rf_chip_phy_t *phy,  int chn)
{
    unsigned int addr      ;
    unsigned int data      ;

    addr = 0xA03 - (1-chn)*2048;
    data = hal_spi_read_reg(phy, addr);

    data = (data & 0xfd);
    hal_spi_write_reg(phy, addr,data);

    data = (data & 0xfd)|0x02;
    hal_spi_write_reg(phy, addr,data);

    data = (data & 0xfd);
    hal_spi_write_reg(phy, addr,data);

}

int fn_tx_qec_gain_set(rf_chip_phy_t *phy, int chn, TX_QEC_CFG_REGS * tx_qec_cfg ,int scale_num)
{
    /* original
    signed   int scale_val ;
    unsigned int index     ;
    unsigned int data      ;
    unsigned int addr      ;

    // if fdiq is not open, then open fdiq
    addr = 0xA00 - 2048*(1-chn);
    data = hal_spi_read_reg(phy, addr);
    if (((data >> 0x4) &0x1) == 0)
    {
        data = (data & 0xef) | 0x10;
        hal_spi_write_reg(phy, addr,data);

        // i_tx_delay_en must be 1
        addr = 0xA01 - 2048*(1-chn);
        data = hal_spi_read_reg(phy, addr);
        data = (data & 0xf7) | 0x08;
        hal_spi_write_reg(phy, addr,data);

        // write fiiq coeff
        addr = 0xA27 - 2048*(1-chn);
        data = 0x40;
        hal_spi_write_reg(phy, addr,data);

        addr = 0xA26 - 2048*(1-chn);
        data = 0x00;
        hal_spi_write_reg(phy, addr,data);

        // write fdiq tap 15 coeff
        addr = 0xB41 - 2048*(1-chn);
        data = 0x40;
        hal_spi_write_reg(phy, addr,data);

        addr = 0xB40 - 2048*(1-chn);
        data = 0x00;
        hal_spi_write_reg(phy, addr,data);

        // make coeff valid
        fn_tx_qec_coeff_valid( chn);
    }

    scale_val= backoff_val - tx_qec_gain_backoff[chn];
    if( scale_val == 0)
    {
        return 1;
    }
    else
    {
        // read fiiq and set back
        fn_tx_qec_coeff_scale((0xA26 - (1-chn)*2048) ,(0xA27 - (1-chn)*2048), scale_val);

        // read fdiq coeff and set back
        for(index =0; index<32;index ++)
        {
            fn_tx_qec_coeff_scale((0xB00 + index*4 - (1-chn)*2048) ,(0xB01 + index*4 - (1-chn)*2048), scale_val);
        }

        // set coefficience valid
         fn_tx_qec_coeff_valid( chn);
    }
    tx_qec_gain_backoff[chn] = backoff_val;
    */
    unsigned int index     ;
    unsigned int data      ;
    unsigned int addr      ;
    signed   int fiiq_coeff;
    signed   int i         ;
    tx_qec_cfg->fdiq_ena = 1;
    memset(tx_qec_cfg->fdiq_fir,0,sizeof(int)*32);
    tx_qec_cfg->fdiq_fir[16] = (int)floor(((((double)0x4000) * pow(10.0,((double)scale_num)*-0.5/20.0))+0.5));


    // fdiq q coeff setting
    for (i = 0; i < 32; i++)
    {
        addr = 0xB00 + i*4 - (1-chn)*2048;
        data = ((tx_qec_cfg->fdiq_fir[i] & 0xFFFF) >> 0) & 0xFF;
        hal_spi_write_reg(phy, addr,data);

        addr = 0xB01 + i*4 - (1-chn)*2048;
        data = ((tx_qec_cfg->fdiq_fir[i] & 0xFFFF) >> 8) & 0xFF;
        hal_spi_write_reg(phy, addr,data);
    }

    // fdiq i coeff setting
    fiiq_coeff = (int)floor((((double)(0x4000) * pow(10.0,((double)scale_num)*-0.5/20.0))+0.5));
    addr = 0xA26 - (1-chn)*2048;
    data = (fiiq_coeff >> 0) &0xFF;
    hal_spi_write_reg(phy, addr,data);

    addr = 0xA27 - (1-chn)*2048;
    data = (fiiq_coeff >> 8) &0xFF;
    hal_spi_write_reg(phy, addr,data);

    // tx delay enable
    addr = 0xA01 - (1-chn)*2048;
    data = hal_spi_read_reg(phy, addr);
    data = (data & 0xf7) | 0x08;
    hal_spi_write_reg(phy, addr,data);

    // let coeff valid
    fn_tx_qec_coeff_valid(phy, chn);

    // tx fdiq enable
    addr = 0xa00 - (1-chn)*2048;
    data = hal_spi_read_reg(phy, addr);
    data = (data & 0xef ) | 0x10;  // enable fiiq
    hal_spi_write_reg(phy, addr,data);

    return 1;
}

int fn_tx_qec_off           (rf_chip_phy_t *phy, 
                                 int                      chn               ,
                                 int                      fiiq_off          ,
                                 int                      fdiq_off         )
{
    unsigned int addr;
    unsigned int data;
    signed   int index;

    // FDIQ and FIIQ keep open
    addr = 0xA00 - 2048*(1-chn);
    data = hal_spi_read_reg(phy, addr);
    data = data | 0x30;
    hal_spi_write_reg(phy, addr,data);



    if(fiiq_off == 1)
    {
        addr = 0xA48 - 2048*(1-chn);
        data = 0x00;
        hal_spi_write_reg(phy, addr,data);

        addr = 0xA49 - 2048*(1-chn);
        data = 0x00;
        hal_spi_write_reg(phy, addr,data);

        addr = 0xA4C - 2048*(1-chn);
        data = 0x00;
        hal_spi_write_reg(phy, addr,data);

        addr = 0xA4D - 2048*(1-chn);
        data = 0x40;
        hal_spi_write_reg(phy, addr,data);
        fn_tx_qec_coeff_valid(phy, chn);
    }

    if(fdiq_off == 1)
    {
        // write fiiq i coeff
        addr = 0xA27 - 2048*(1-chn);
        data = 0x40;
        hal_spi_write_reg(phy, addr,data);

        addr = 0xA26 - 2048*(1-chn);
        data = 0x00;
        hal_spi_write_reg(phy, addr,data);

        // write fdiq q 15 coeff
        for(index =0; index<32;index ++)
        {
            addr = 0xB00 - 2048*(1-chn);
            data = 0x00;
            hal_spi_write_reg(phy, addr,data);

            addr = 0xB01 - 2048*(1-chn);
            data = 0x00;
            hal_spi_write_reg(phy, addr,data);
        }

        addr = 0xB41 - 2048*(1-chn);
        data = 0x40;
        hal_spi_write_reg(phy, addr,data);

        addr = 0xB40 - 2048*(1-chn);
        data = 0x00;
        hal_spi_write_reg(phy, addr,data);

        fn_tx_qec_coeff_valid(phy, chn);
    }

    return 1;
}

int fn_tx_run_qec(rf_chip_phy_t *phy, int chn, TX_QEC_CFG_REGS* tx_qec_cfg)
{
    unsigned int w_addr   ;
    unsigned int w_data   ;
    unsigned int r_addr   ;
    unsigned int r_data   ;
    int i;


    if(tx_qec_cfg->fiiq_ena == 1)
    {
        // STEP 3 Write FDIQ and FIIQ coeffcient
        // 3.1 fiiq_real high 8bit
        w_addr = 0xa49 - (1-chn)*2048;
        w_data = ((tx_qec_cfg->fiiq_imag & 0xFFFF) >> 8)&0xFF;
        hal_spi_write_reg(phy, w_addr,w_data);

        // fiiq_real low 8bit
        w_addr = 0xa48 - (1-chn)*2048;
        w_data = ((tx_qec_cfg->fiiq_imag & 0xFFFF) >> 0)&0xFF;
        hal_spi_write_reg(phy, w_addr,w_data);

        // fiiq_imag high 8bit
        w_addr = 0xa4D - (1-chn)*2048;
        w_data = ((tx_qec_cfg->fiiq_real & 0xFFFF) >> 8)&0xFF;
        hal_spi_write_reg(phy, w_addr,w_data);

        // fiiq_imag low 8bit
        w_addr = 0xa4C - (1-chn)*2048;
        w_data = ((tx_qec_cfg->fiiq_real & 0xFFFF) >> 0)&0xFF;
        hal_spi_write_reg(phy, w_addr,w_data);

        // txiq_coe_load_spi set 0->1->0
        fn_tx_qec_coeff_valid(phy, chn);

        // rx fiiq enable
        r_addr = 0xa00 - (1-chn)*2048;
        r_data = hal_spi_read_reg(phy, r_addr);
        w_addr = r_addr;
        //w_data = (r_data & 0xcf ) | 0x30;  // enable fiiq  and enable  fdiq
        w_data = (r_data & 0xdf ) | 0x20;  // enable fiiq  and disable fdiq
        hal_spi_write_reg(phy, w_addr,w_data);

    }
    return 0;

}

int fn_tx_qec_cal_words(
        double            mag_error_ratio,
        double            phase_error_deg    ,
        TX_QEC_CFG_REGS * tx_qec_cfg         )
{
    double fiiq_mag_ratio = mag_error_ratio;
    double fiiq_deg_error = phase_error_deg;
    double fiiq_rad_error = 0;
    double fiiq_gm_cos_inv;
    double fiiq_tan;
    int i;

    // FIIQ compensation
    tx_qec_cfg->fiiq_ena = 1;
    fiiq_rad_error = fiiq_deg_error / 360.0 * 2 * PI;
    fiiq_gm_cos_inv = 1.0/(fiiq_mag_ratio * cos(fiiq_rad_error));
    fiiq_gm_cos_inv = floor(fiiq_gm_cos_inv * pow(2.0,14)+0.5);
    if(fiiq_gm_cos_inv <0)
        fiiq_gm_cos_inv = fiiq_gm_cos_inv + pow(2.0,16);

    tx_qec_cfg->fiiq_real = (int)fiiq_gm_cos_inv;

    fiiq_tan = tan(fiiq_rad_error);
    fiiq_tan = floor(fiiq_tan * pow(2.0,14)+0.5);
    fiiq_tan = (fiiq_tan <0)? ( fiiq_tan + pow(2.0,16) ) :  fiiq_tan;

    tx_qec_cfg->fiiq_imag = (int)fiiq_tan;

    return 0;
}

int fn_tx_run_lol(rf_chip_phy_t *phy, int chn, int dc_i, int dc_q)
{
    int w_addr;
    int w_data;
    int r_addr;
    int r_data;


    // SET FIIQ ENABLE
    r_addr = 0xa00 - (1-chn)*2048;
    r_data = hal_spi_read_reg(phy, r_addr);
    w_addr = r_addr;
    w_data = (r_data & 0xdf ) | 0x20;
    hal_spi_write_reg(phy, w_addr,w_data);

    if(dc_i<0)
        dc_i = dc_i + (1<<16);

    if(dc_q<0)
        dc_q = dc_q + (1<<16);

    w_addr = 0xA40 - (1-chn)*2048;
    w_data = (dc_i >> 0)&0xFF;
    hal_spi_write_reg(phy, w_addr,w_data);

    w_addr = 0xA41 - (1-chn)*2048;
    w_data = (dc_i >> 8)&0xFF;
    hal_spi_write_reg(phy, w_addr,w_data);

    w_addr = 0xA44 - (1-chn)*2048;
    w_data = (dc_q >> 0)&0xFF;
    hal_spi_write_reg(phy, w_addr,w_data);

    w_addr = 0xA45 - (1-chn)*2048;
    w_data = (dc_q >> 8)&0xFF;
    hal_spi_write_reg(phy, w_addr,w_data);

    // txiq_coe_load_spi set 0->1->0
    fn_tx_qec_coeff_valid(phy, chn);


    return 1;
}


//-------------------------------
int fn_rx_qec_self_calib(rf_chip_phy_t *phy, 
                              int                chn              ,
                              int                ext_loop         ,
                              int                tdd_sel          ,
                              unsigned long long rx_lo            ,
                              unsigned int       sample_rate      ,
                              int                signal_bandwidth ,
                              RX_QEC_CFG_REGS   *rx_qec_cfg
                              )
{
    int goertlze_pattern = 0;
    int wb_div_32_0      = 1;
    int wb_div_32_1      = 1;
    int goertlze_num     = 4096;
    int coherent_num     = 8;
    int goertzle_0_i     = 0;
    int goertzle_0_q     = 0;
    int goertzle_1_i     = 0;
    int goertzle_1_q     = 0;
    int goertzle_shift   = 0;

    double * mag_error_ratio;
    double * ph_error_deg   ;
    double * mag_error_ratio_check;
    double * ph_error_deg_check   ;
	double   mag_goertzle_linear;

	double fdiq_mag_ratio[32];
	double fdiq_deg_error[32];

    int wb_num;
    int wb;
    int i,j;

    int mean_power;
    double rssi;
	double mag_diff;
	double phg_diff;
	int tia_gain;
    int bbf_gain;
	int flag_agc_ok = 0;
	double target_db;
	int tia_gain_new;
	int flag_qec_ok = 0;
	int error_cnt   = 0;
	double irr_result;
	unsigned short reg_val;
    RF_LOOPBACK_REGS  rf_loopback_regs;
    TX_QEC_CFG_REGS ** p_tx_qec_cfg    ;
    TX_QEC_CFG_REGS   tx_qec_cfg_tmp  ;
	TX_QEC_DIAG_INFO  tx_qec_diag_info;
	RX_QEC_DIAG_INFO  rx_qec_diag_info;
	int               lpf_index       ;
	
	// FORCE
	rx_qec_cfg->debug_level = 1;
	if(rx_qec_cfg->debug_level>=1)
	{
		LOG_MDEBUG(phy, TRX_QEC_CAL, "+++++++++++++++[rxqec_cal]+++++ debug info\n");
	}
	
	if(rx_qec_cfg->debug_level>=4)
	{
		LOG_MDEBUG(phy, TRX_QEC_CAL, "QEC start !\n");
		phy->module_debug = 0x40;
	}

    //wb_num = (signal_bandwidth/1000*32/2) / (sample_rate/1000 );

	if(phy->config->custom_bandwith_flag)
	{
        wb_num = (int)round(32 * phy->config->custom_bandwith / 2.0 / phy->config->bb_sample_rate/1.0);
	}
	else if (phy->config->syspll_cfg_flag)
	{
		wb_num = (int)round(32.0* (double)g_3db_band[phy->config->bandwith] / phy->config->bb_sample_rate/1.0);
		LOG_MDEBUG(phy,TRX_QEC_CAL,"wb_num=%d, 3db_band=%.4ld , bb sample rate(KHz)=%.4u\n", wb_num, g_3db_band[phy->config->bandwith], phy->config->bb_sample_rate/1000);
	}
	else	
	{
		wb_num = (int)round(32.0* (double)g_3db_band[phy->config->bandwith] / (double)g_band_bbrate[phy->config->bandwith] / 1000.0/1.0);
		LOG_MDEBUG(phy,TRX_QEC_CAL," %.4ld , %.4u\n",g_3db_band[phy->config->bandwith],g_band_bbrate[phy->config->bandwith]);
	}
	
	int loopback_type;
	rxqec_diag_info_init(&rx_qec_diag_info,wb_num+1,wb_num+1,rx_qec_cfg->debug_level);
	txqec_diag_info_init(&tx_qec_diag_info,wb_num,rx_qec_cfg->debug_level);
	
	//fn_rx_qec_off(phy, chn);
	fn_tx_qec_off(phy,chn,1,1);
	fn_rx_qec_off(phy,chn);
	
	if(tdd_sel==1)
	{
		p_tx_qec_cfg = (TX_QEC_CFG_REGS**)malloc(wb_num*sizeof(TX_QEC_CFG_REGS *));
		for(i=0;i<wb_num;i++)
		{
			p_tx_qec_cfg[i] = (TX_QEC_CFG_REGS*)malloc(sizeof(TX_QEC_CFG_REGS));
			memset(p_tx_qec_cfg[i],0,sizeof(TX_QEC_CFG_REGS));
			p_tx_qec_cfg[i]->debug_level = rx_qec_cfg->debug_level;
			p_tx_qec_cfg[i]->speed_level = rx_qec_cfg->speed_level;
		}
		fn_connect_rxlo_to_sxrx(phy,chn,rx_lo);
		fn_tx_hbf_reset(phy,chn);
	}
	

	
	fn_open_loop_back(
		phy             , 
		chn             ,
		ext_loop        ,
		tdd_sel         , 
		1               ,
		2               ,   // backoff 2dB 
		rx_lo           ,
		sample_rate     ,
		& loopback_type   ,
		& rf_loopback_regs
	);
	tx_qec_diag_info.loopback_type = loopback_type;
	
	fn_lpf_index_auto_change(
		phy          ,
		chn          ,
		&lpf_index
	);
	tx_qec_diag_info.lpf_index     = lpf_index;
	
	fn_rx_ana_offset_cal_V2(phy,chn);
	fn_rx_get_adc_offset(phy, chn,10,&tx_qec_diag_info.rx_dc_i,&tx_qec_diag_info.rx_dc_q);
	fn_rx_get_adc_power(phy, chn,4, &tx_qec_diag_info.rx_power);
	
	rx_qec_diag_info.rx_dc_i       = tx_qec_diag_info.rx_dc_i;
	rx_qec_diag_info.rx_dc_q       = tx_qec_diag_info.rx_dc_q;
	rx_qec_diag_info.rx_power      = tx_qec_diag_info.rx_power;
	rx_qec_diag_info.lpf_index     = tx_qec_diag_info.lpf_index;
	rx_qec_diag_info.loopback_type = tx_qec_diag_info.loopback_type;
	
	if(abs(rx_qec_diag_info.rx_dc_i)/10 > 400 || abs(rx_qec_diag_info.rx_dc_q)/10 > 400 )
	{
		LOG_MDEBUG(phy,TRX_QEC_CAL,"RX QEC ERROR: DC cannot remove\n");
		if(rx_qec_cfg->debug_level>=2)
		{
			while(1){}
		}
		else
		{
			flag_qec_ok = -1;
		}
		
	}
	else if(abs(rx_qec_diag_info.rx_dc_i)/10 > 100 || abs(rx_qec_diag_info.rx_dc_q)/10 > 100 )
	{
		LOG_MDEBUG(phy,TRX_QEC_CAL,"RX QEC WARNING: DC cannot remove\n");
	}
	
	rssi = 20.0*log10(sqrt((double)rx_qec_diag_info.rx_power/4.0)/pow(2.0,9.0));	
	if(rssi > -3 )
	{
		LOG_MDEBUG(phy,TRX_QEC_CAL,"RX QEC ERROR  : LOOPBACK power is too large\n");
		if(rx_qec_cfg->debug_level>=2)
		{
			while(1){}
		}
		else
		{
			flag_qec_ok = -1;
		}
	}
	else if(rssi> -6 )
	{
		LOG_MDEBUG(phy,TRX_QEC_CAL,"RX QEC WARNING: LOOPBACK power is too large\n");		
	}
	else if(rssi < -50 )
	{
		LOG_MDEBUG(phy,TRX_QEC_CAL,"RX QEC ERROR  : LOOPBACK power is too small\n");
		if(rx_qec_cfg->debug_level>=2)
		{
			while(1){}
		}
		else
		{
			flag_qec_ok = -1;
		}
	}	
	else if (rssi < -30 )
	{
		LOG_MDEBUG(phy,TRX_QEC_CAL,"RX QEC WARNING : LOOPBACK power is too small\n");
	}	
	
    if(tdd_sel == 1)
    {	
        for(i=1;i<=wb_num;i++)
        {
            fn_tx_send_tone(phy, chn,1,2*i);
			if(1)
            //if(i%4==1||i>(wb_num-3))
            //if(i==1)
            {
            	if(i==1)
            	{
	                fn_tx_qec_gain_set(phy,  chn, p_tx_qec_cfg[i-1] , 4); // backoff 2dB
	                //fn_tx_qec_gain_set(phy,  chn, &p_tx_qec_cfg[i-1] , 20); // backoff 10dB
					p_tx_qec_cfg[i-1]->cal_init = 0;
					fn_rx_run_goertzle_cfg(phy, chn, 0,1 ,1, 4096, 8 );				
            	}
			
              	if(i>1)
              	{
              		memcpy(p_tx_qec_cfg[i-1],p_tx_qec_cfg[i-2],sizeof(TX_QEC_CFG_REGS));
					p_tx_qec_cfg[i-1]->cal_init = 1;
              	}

				if(rx_qec_cfg->debug_level>=4)
				{
					LOG_MDEBUG(phy, TRX_QEC_CAL, "rxb bandselection %lld\n",rx_lo-(unsigned long long)(sample_rate / 32*(i+1)));
				}
				if(rx_qec_cfg->debug_level>=3)
				{
					LOG_MDEBUG(phy, TRX_QEC_CAL, "--- wb %d ---\n",i);
				}
                rx_band_selection(phy, chn,rx_lo-(unsigned long long)(sample_rate / 32*(i+1)));	
                flag_qec_ok = fn_tx_qec_5tone_method_rflp(phy, chn,p_tx_qec_cfg[i-1] , & (tx_qec_diag_info));
            }
            else
            {
                memcpy(p_tx_qec_cfg[i-1],p_tx_qec_cfg[i-2],sizeof(TX_QEC_CFG_REGS));
                fn_tx_run_qec(phy, chn,p_tx_qec_cfg[i-1]);
            }

			if(rx_qec_cfg->debug_level>=4)
			{
				hal_spi_write_reg(phy, 0x700,0x2);
				hal_spi_write_reg(phy, 0x480,i%2+1);
				LOG_MDEBUG(phy, TRX_QEC_CAL, "TX qec wb %d\n",i);
				CHIP_UDELAY(10000000);  // wait for 10 second
				hal_spi_write_reg(phy, 0x480,i%2+2);
			}
        }// end of for wb loop
		
		
        fn_tx_send_tone(phy, chn,1,2);
        fn_tx_run_qec(phy, chn,p_tx_qec_cfg[0]);
		
		//if(rx_qec_cfg->debug_level>=1)
		//{
		//	for(i=1;i<=wb_num;i++)
		//	{
		//		LOG_MDEBUG(phy, TRX_QEC_CAL, "%d fiiq %d %d\n",i,p_tx_qec_cfg[i-1].fiiq_real,p_tx_qec_cfg[i-1].fiiq_imag);
		//	}
		//}

		fn_recovery_txlo_to_sxrx(phy);
    }

    // STEP 3  run TX band selection to get quadrature imbalance information	
    mag_error_ratio        = (double*)malloc((wb_num+1)*sizeof(double));
    ph_error_deg           = (double*)malloc((wb_num+1)*sizeof(double));
    mag_error_ratio_check  = (double*)malloc((wb_num+1)*sizeof(double));
    ph_error_deg_check     = (double*)malloc((wb_num+1)*sizeof(double));

    if (mag_error_ratio == NULL || ph_error_deg == NULL || mag_error_ratio_check ==NULL || ph_error_deg_check==NULL)
    {
        flag_qec_ok = -1;
    }
	
    for(wb=0;wb<=wb_num;wb++)
    {
        // change TX LO
        if(wb!=0)
        {
            if(tdd_sel==0)
            {
                tx_band_selection(phy, chn, rx_lo + (unsigned long long)(sample_rate / 32*wb));
            }
            else
            {
                fn_tx_send_tone(phy, chn,1,2*wb);
                fn_tx_run_qec(phy, chn,p_tx_qec_cfg[wb-1]);
            }
        }
        else
        {
            if(tdd_sel==0)
            {
                tx_band_selection(phy, chn, rx_lo - (unsigned long long)(sample_rate / 32*1));
            }
            else
            {
                fn_tx_send_tone_neg(phy, chn,1,2*1);
                fn_tx_run_qec(phy, chn,p_tx_qec_cfg[0]);
            }
        }

		//usleep(100000);
        // Get Goertzle result and calculate for gain error and phase error.
        if(wb!=0)
        {
            //fn_rx_qec_blind_est(phy, chn,30720*80,wb,&mag_ratio_blind,&ph_error_blind,&usv);
            wb_div_32_0 = wb;
            wb_div_32_1 = wb;
        }
        else
        {
            //fn_rx_qec_blind_est(phy, chn,30720*80,1,&mag_ratio_blind,&ph_error_blind,&usv);
            wb_div_32_0 = -1;
            wb_div_32_1 = -1;
        }
        fn_rx_run_goertzle(phy, 
                    chn,
                    goertlze_pattern,
                    wb_div_32_0, wb_div_32_1,
                    goertlze_num,  coherent_num ,
                    &goertzle_0_i, &goertzle_0_q,
                    &goertzle_1_i, &goertzle_1_q,
                    &goertzle_shift);
		
		mag_goertzle_linear  = 	(pow((double)(goertzle_0_i-goertzle_1_q),2.0) + pow((double)(goertzle_0_q+goertzle_1_i),2.0)) * pow(4.0,(double)goertzle_shift);
		
		
        fn_get_quad_ph_err_deg(
                    goertzle_0_i,goertzle_0_q,
                    goertzle_1_i,goertzle_1_q,
                    &mag_error_ratio[wb],&ph_error_deg[wb]);
	
	
		// ------------------ below to handle error --------------------- //
		rx_qec_diag_info.rx_lpbk_power [rx_qec_diag_info.wb_cnt]  = (int)((10.0*log10(mag_goertzle_linear ) -234.79)*10.0);
		rx_qec_diag_info.rx_qec_mag_err[rx_qec_diag_info.wb_cnt]  = (int)(floor(mag_error_ratio[wb]*10000+0.5));
		rx_qec_diag_info.rx_qec_phg_err[rx_qec_diag_info.wb_cnt]  = (int)(floor(ph_error_deg   [wb]*10000+0.5));

		//mag_diff = 20*log10(( fabs(1.0-((double)rx_qec_diag_info.rx_qec_mag_err[rx_qec_diag_info.wb_cnt])/10000.0)));
		//if( mag_diff > -15.0)
		//{
		//	LOG_MDEBUG(phy,TRX_QEC_CAL,"RX QEC ERROR  : DETECT INIT wb %d magnitude error is too large : %.4f db . \n",
		//		rx_qec_diag_info.wb_cnt,
		//		mag_diff);
		//	if(rx_qec_cfg->debug_level>=2)
		//	{
		//		while(1){}
		//	}
		//	else
		//	{
		//		flag_qec_ok = -1;
		//	}
		//}
		//else if ( mag_diff> -30.0)
		//{
		//	LOG_MDEBUG(phy,TRX_QEC_CAL,"RX QEC WARNING  : DETECT INIT wb %d magnitude error is too large : %.4f db . \n",
		//		rx_qec_diag_info.wb_cnt,
		//		mag_diff);			
		//}
		//
		//if( fabs(((double)(rx_qec_diag_info.rx_qec_phg_err[rx_qec_diag_info.wb_cnt])/10000.0)) > 10.0)
		//{
		//	LOG_MDEBUG(phy,TRX_QEC_CAL,"RX QEC ERROR  : DETECT INIT wb %d phase error is too large : %.4f degree . \n",
		//		rx_qec_diag_info.wb_cnt,
		//		(double)(rx_qec_diag_info.rx_qec_phg_err[rx_qec_diag_info.wb_cnt])/10000.0);
		//	if(rx_qec_cfg->debug_level>=2)
		//	{
		//		while(1){}
		//	}
		//	else
		//	{
		//		flag_qec_ok = -1;
		//	}
		//}		
		//
		//if(rx_qec_diag_info.wb_cnt>1)
		//{
		//	mag_diff = fabs( (((double)rx_qec_diag_info.rx_qec_mag_err[rx_qec_diag_info.wb_cnt])/10000.0) - (((double)rx_qec_diag_info.rx_qec_mag_err[rx_qec_diag_info.wb_cnt-1])/10000.0));
		//	if ( mag_diff>0.01 )
		//	{
		//		LOG_MDEBUG(phy,TRX_QEC_CAL,"RX QEC ERROR  : DETECT large magnitude change %.4f between %d and %d \n",
		//			mag_diff,
		//			rx_qec_diag_info.wb_cnt,
		//			rx_qec_diag_info.wb_cnt-1);
		//		if(rx_qec_cfg->debug_level>=2)
		//		{
		//			while(1){}
		//		}
		//		else
		//		{
		//			flag_qec_ok = -1;
		//		}				
		//	}
		//	//else
		//	//{
		//	//	LOG_MDEBUG(phy,TRX_QEC_CAL,"RX QEC INFO  : magnitude change %.4f between %d and %d \n",
		//	//			mag_diff,
		//	//			rx_qec_diag_info.wb_cnt,
		//	//			rx_qec_diag_info.wb_cnt-1);				
		//	//}
		//	phg_diff = fabs( (((double)rx_qec_diag_info.rx_qec_phg_err[rx_qec_diag_info.wb_cnt])/10000.0) - (((double)rx_qec_diag_info.rx_qec_phg_err[rx_qec_diag_info.wb_cnt-1])/10000.0));
		//	if ( phg_diff >1.0)
		//	{
		//		LOG_MDEBUG(phy,TRX_QEC_CAL,"RX QEC WARNING  : DETECT large phase change %.4f between %d and %d \n",
		//			phg_diff,
		//			rx_qec_diag_info.wb_cnt,
		//			rx_qec_diag_info.wb_cnt-1);
		//	}
		//	else if ( phg_diff >0.5)
		//	{
		//		LOG_MDEBUG(phy,TRX_QEC_CAL,"RX QEC ERROR  : DETECT large phase change %.4f between %d and %d \n",
		//			phg_diff,
		//			rx_qec_diag_info.wb_cnt,
		//			rx_qec_diag_info.wb_cnt-1);
		//		if(rx_qec_cfg->debug_level>=2)
		//		{
		//			while(1){}
		//		}
		//		else
		//		{
		//			flag_qec_ok = -1;
		//		}	
		//	}
		//	//else
		//	//{
		//	//	LOG_MDEBUG(phy,TRX_QEC_CAL,"RX QEC INFO  : phase change %.4f between %d and %d \n",
		//	//			phg_diff,
		//	//			rx_qec_diag_info.wb_cnt,
		//	//			rx_qec_diag_info.wb_cnt-1);
		//	//}				
		//}
		rx_qec_diag_info.wb_cnt ++;
        // recommnded breakpoint , Get inital iq mismatch information of each frequency point
		#ifdef QEC_DELAY
			//hal_spi_write_reg(phy, 0x480, wb%2 + 3);
			//LOG_MDEBUG(phy, TRX_QEC_CAL, "Detect End | wb %d\n",wb);
			//usleep(10000000);  // wait for 10 second
			//hal_spi_write_reg(phy, 0x480, wb%2 + 4);
		#endif
        //printf("\tmag error %f | phase error %f\n",mag_error_ratio[wb-1],ph_error_deg[wb-1]);
    }
	
    mag_error_ratio[0] = (mag_error_ratio[0] + mag_error_ratio[1])/2;
    ph_error_deg   [0] = (ph_error_deg   [0] + ph_error_deg   [1])/2;

    rx_qec_cfg->mag_ratio_adj = mag_error_ratio[0]/mag_error_ratio[1];
    rx_qec_cfg->ph_error_adj  = ph_error_deg   [0]-ph_error_deg   [1];

    // STEP 4  translate rx quadrature imblance information to register
    fn_rx_qec_cal_words( mag_error_ratio, ph_error_deg, wb_num, rx_qec_cfg);
	
    //for (i = 0; i <16; i++)
    //{
    //    if (i <= wb_num)
    //    {
    //        fdiq_mag_ratio[i] = mag_error_ratio[i] / mag_error_ratio[0];
    //        fdiq_deg_error[i] = ph_error_deg[i] - ph_error_deg[0];
    //    }
    //    else
    //    {
    //        fdiq_mag_ratio[i] = mag_error_ratio[wb_num-1] / mag_error_ratio[0];
    //        fdiq_deg_error[i] = ph_error_deg[wb_num-1] - ph_error_deg[0];
	//		
	//		//fdiq_mag_ratio[i] = (1.0 - fdiq_mag_ratio[wb_num])/((double)(15-wb_num)) * (i-wb_num) + fdiq_mag_ratio[wb_num];
	//		//diq_deg_error[i] = (0.0 - fdiq_deg_error[wb_num])/((double)(15-wb_num)) * (i-wb_num) + fdiq_deg_error[wb_num];
	//		
    //    }
	//	LOG_MDEBUG(phy, TRX_QEC_CAL, "\tRX QEC CAL Words %4d| mag_ratio %.4f deg_error %.4f \n",i,fdiq_mag_ratio[i],fdiq_deg_error[i]);
    //}
	
    // STEP 5
    fn_rx_run_qec(phy, chn, rx_qec_cfg);
	//usleep(1000);
	
	if(flag_qec_ok != -1)
	{
		if(rx_qec_cfg->debug_level>=4)
		{
			LOG_MDEBUG(phy, TRX_QEC_CAL, "start to check qec result  !\n");
		}
	    for(wb=0;wb<=wb_num;wb++)
	    {
	        // change TX LO
	        if(wb!=0)
	        {
	            if(tdd_sel==0)
	            {
	                tx_band_selection(phy, chn, rx_lo + (unsigned long long)(sample_rate / 32*wb));
	            }
	            else
	            {
	                fn_tx_send_tone(phy, chn,1,2*wb);
	                fn_tx_run_qec(phy, chn,p_tx_qec_cfg[wb-1]);
	            }
	        }
	        else
	        {
	            if(tdd_sel==0)
	            {
	                tx_band_selection(phy, chn, rx_lo - (unsigned long long)(sample_rate / 32*1));
	            }
	            else
	            {
	                fn_tx_send_tone_neg(phy, chn,1,2*1);
	                fn_tx_run_qec(phy, chn,p_tx_qec_cfg[0]);
	            }
	        }
				
	        //usleep(100000);

	        // Get Goertzle result and calculate for gain error and phase error.
	        if(wb!=0)
	        {
	            wb_div_32_0 = wb;
	            wb_div_32_1 = wb;
	        }
	        else
	        {
	            wb_div_32_0 = -1;
	            wb_div_32_1 = -1;
	        }
	        fn_rx_run_goertzle(phy, 
	                    chn,
	                    goertlze_pattern,
	                    wb_div_32_0, wb_div_32_1,
	                    goertlze_num,  coherent_num ,
	                    &goertzle_0_i, &goertzle_0_q,
	                    &goertzle_1_i, &goertzle_1_q,
	                    &goertzle_shift);
						
			if(rx_qec_cfg->debug_level>=4)
			{
				LOG_MDEBUG(phy, TRX_QEC_CAL, "Check wb %d\n",wb);
				LOG_MDEBUG(phy, TRX_QEC_CAL, "\tgoertzle br0 |  i %d | q %d\n",goertzle_0_i,goertzle_0_q);
				LOG_MDEBUG(phy, TRX_QEC_CAL, "\tgoertzle br1 |  i %d | q %d\n",goertzle_1_i,goertzle_1_q);
			}

	        fn_get_quad_ph_err_deg(
	                    goertzle_0_i,goertzle_0_q,
	                    goertzle_1_i,goertzle_1_q,
	                    &mag_error_ratio_check[wb],&ph_error_deg_check[wb]);
						
			rx_qec_diag_info.rx_qec_mag_chk[rx_qec_diag_info.chk_cnt] = (int)(floor(mag_error_ratio_check[wb]*10000+0.5));
			rx_qec_diag_info.rx_qec_phg_chk[rx_qec_diag_info.chk_cnt] = (int)(floor(ph_error_deg_check   [wb]*10000+0.5));
			
			//irr_result = fn_cal_irr(
			//	(double)(rx_qec_diag_info.rx_qec_mag_chk[rx_qec_diag_info.chk_cnt])/10000.0,
			//	(double)(rx_qec_diag_info.rx_qec_phg_chk[rx_qec_diag_info.chk_cnt])/10000.0);
			//
			//if(irr_result > 50.0 && irr_result <58.0)
			//{
			//	LOG_MDEBUG(phy,TRX_QEC_CAL,"RX QEC WARNING  : DETECT a unsatisfactory result on %d : %.4f \n",
			//		rx_qec_diag_info.chk_cnt,irr_result);
			//}
			//else if(irr_result <= 50.0)
			//{
			//	LOG_MDEBUG(phy,TRX_QEC_CAL,"RX QEC WARNING  : DETECT a very bad calibration result on %d : %.4f \n",
			//		rx_qec_diag_info.chk_cnt,irr_result);
			//	if(rx_qec_cfg->debug_level>=2)
			//	{
			//		while(1){}
			//	}
			//	else
			//	{
			//		flag_qec_ok = -1;
			//	}	
			//}
		
			rx_qec_diag_info.chk_cnt ++;
			
			if(rx_qec_cfg->debug_level>=4)
			{
				LOG_MDEBUG(phy, TRX_QEC_CAL, "mag error %f | phase error %f\n",mag_error_ratio_check[wb],ph_error_deg_check[wb]);
			}
			
	        
			if(rx_qec_cfg->debug_level>=4)
			{
				LOG_MDEBUG(phy, TRX_QEC_CAL, "Check End | wb %d\n",wb);
				CHIP_UDELAY(10000000);  // wait for 10 second
			}
	    }
	    //mag_error_ratio_check[0] = (mag_error_ratio_check[0] + mag_error_ratio_check[1])/2;
	    //ph_error_deg_check   [0] = (ph_error_deg_check   [0] + ph_error_deg_check   [1])/2;


	}
	
	
	if(0)
	{
	    for(wb=0;wb<=wb_num;wb++)
	    {
	       ph_error_deg[wb ] =  ph_error_deg[wb ] + ph_error_deg_check[0];
	       mag_error_ratio[wb ] =  mag_error_ratio[wb ] * mag_error_ratio_check[0];
	    }
	    fn_rx_qec_cal_words( mag_error_ratio, ph_error_deg, wb_num, rx_qec_cfg);
	    fn_rx_run_qec(phy, chn, rx_qec_cfg);
		//usleep(100000);
		
		if(rx_qec_cfg->debug_level>=4)
		{
			LOG_MDEBUG(phy, TRX_QEC_CAL, "start to check qec result  !\n");
		}

	    for(wb=0;wb<=wb_num;wb++)
	    {
	        // change TX LO
	        if(wb!=0)
	        {
	            if(tdd_sel==0)
	            {
	                tx_band_selection(phy, chn, rx_lo + (unsigned long long)(sample_rate / 32*wb));
	            }
	            else
	            {
	                fn_tx_send_tone(phy, chn,1,2*wb);
	                fn_tx_run_qec(phy, chn,p_tx_qec_cfg[wb-1]);
	            }
	        }
	        else
	        {
	            if(tdd_sel==0)
	            {
	                tx_band_selection(phy, chn, rx_lo - (unsigned long long)(sample_rate / 32*1));
	            }
	            else
	            {
	                fn_tx_send_tone_neg(phy, chn,1,2*1);
	                fn_tx_run_qec(phy, chn,p_tx_qec_cfg[0]);
	            }
	        }
			
			//usleep(100000);

	        // Get Goertzle result and calculate for gain error and phase error.
	        if(wb!=0)
	        {
	            wb_div_32_0 = wb;
	            wb_div_32_1 = wb;
	        }
	        else
	        {
	            wb_div_32_0 = -1;
	            wb_div_32_1 = -1;
	        }
	        fn_rx_run_goertzle(phy, 
	                    chn,
	                    goertlze_pattern,
	                    wb_div_32_0, wb_div_32_1,
	                    goertlze_num,  coherent_num ,
	                    &goertzle_0_i, &goertzle_0_q,
	                    &goertzle_1_i, &goertzle_1_q,
	                    &goertzle_shift);

	        fn_get_quad_ph_err_deg(
	                    goertzle_0_i,goertzle_0_q,
	                    goertzle_1_i,goertzle_1_q,
	                    &mag_error_ratio_check[wb],&ph_error_deg_check[wb]);
						
			if(rx_qec_cfg->debug_level>=4)
			{
				LOG_MDEBUG(phy, TRX_QEC_CAL, "Confirm wb %d\n",wb);
				LOG_MDEBUG(phy, TRX_QEC_CAL, "\tgoertzle br0 |  i %d | q %d\n",goertzle_0_i,goertzle_0_q);
				LOG_MDEBUG(phy, TRX_QEC_CAL, "\tmag error %f | phase error %f\n",mag_error_ratio_check[wb],ph_error_deg_check[wb]);
			}

			#ifdef QEC_DELAY
				hal_spi_write_reg(phy, 0x480,wb%2 + 0x5);
				LOG_MDEBUG(phy, TRX_QEC_CAL, "Confirm End | wb %d\n",wb);
				CHIP_UDELAY(10000000);  // wait for 10 second
				hal_spi_write_reg(phy, 0x480,wb%2 + 0x6);
			#endif
	        // recommnded breakpoint , Get calibrated iq mismatch informationof each frequency point
	        int a = 0;
	    }
	    mag_error_ratio_check[0] = (mag_error_ratio_check[0] + mag_error_ratio_check[1])/2;
	    ph_error_deg_check   [0] = (ph_error_deg_check   [0] + ph_error_deg_check   [1])/2;

		if(rx_qec_cfg->debug_level>=4)
		{
			hal_spi_write_reg(phy, 0x480,0x7);
			LOG_MDEBUG(phy, TRX_QEC_CAL, "RX QEC Confirmed\n");
			CHIP_UDELAY(120000000);  // wait for 10*10 second
			hal_spi_write_reg(phy, 0x480,0x8);
		}

	    // STEP 5 all band is complete under high gain. switch to low gain
		/*
	    if (ext_loop!=1)
	    {
		    fn_rx_ana_set_gain(phy, chn,2,6);

		    if(tdd_sel==0)
		    {
		        tx_band_selection(phy, chn, rx_lo + (unsigned long long)(sample_rate / 32*1));
		    }
		    else
		    {
		        fn_tx_send_tone(phy, chn,1,2);
		        fn_tx_run_qec(phy, chn,&p_tx_qec_cfg[0]);
		    }
			
			usleep(100000);

		    fn_rx_ana_offset_cal(phy, chn,10);

		    double mag_ratio_all;
		    double ph_error_all;
		    double mag_ratio_curr;
		    double ph_error_curr;

		    mag_ratio_all = 0;
		    ph_error_all  = 0;

		    int non_coh_num = 40;
		    for(j=0;j<non_coh_num;j++)
		    {
		        fn_rx_run_goertzle(phy, 
		                    chn,
		                    goertlze_pattern,
		                    1, 1,
		                    goertlze_num,  coherent_num ,
		                    &goertzle_0_i, &goertzle_0_q,
		                    &goertzle_1_i, &goertzle_1_q,
		                    &goertzle_shift);

		        fn_get_quad_ph_err_deg(
		                        goertzle_0_i,goertzle_0_q,
		                        goertzle_1_i,goertzle_1_q,
		                        &mag_ratio_curr,&ph_error_curr);
		            mag_ratio_all += mag_ratio_curr;
		            ph_error_all  += ph_error_curr;

		        LOG_MDEBUG(phy, TRX_QEC_CAL, "\t%d mag error %f | phase error %f\n",j,mag_ratio_curr,ph_error_curr);
		    }

		    mag_ratio_all = mag_ratio_all / (double)non_coh_num;
		    ph_error_all  = ph_error_all  / (double)non_coh_num;
			
			LOG_MDEBUG(phy, TRX_QEC_CAL, " low gain mag error %f | phase error %f\n",mag_ratio_all,ph_error_all);

		    for(wb=0;wb<=wb_num;wb++)
		    {
				LOG_MDEBUG(phy, TRX_QEC_CAL, "%d | mag_ratio %.4f ph_error %.4f\n",wb,mag_error_ratio[wb ],ph_error_deg[wb ]  );
		    }

		    //mag_ratio_all = mag_ratio_all / mag_error_ratio[1 ];
		    //ph_error_all  = ph_error_all - ph_error_deg[1];
		    for(wb=0;wb<=wb_num;wb++)
		    {
				ph_error_deg[wb ] =  ph_error_deg[wb ] + ph_error_all;
				mag_error_ratio[wb ] =  mag_error_ratio[wb ] * mag_ratio_all;
			   
				LOG_MDEBUG(phy, TRX_QEC_CAL, "%d | mag_ratio %.4f ph_error %.4f\n",wb,mag_error_ratio[wb ],ph_error_deg[wb ]  );
		    }
		    fn_rx_qec_cal_words( mag_error_ratio, ph_error_deg, wb_num, rx_qec_cfg);
		    fn_rx_run_qec(phy, chn, rx_qec_cfg);
		}
		*/
	}

	if(tdd_sel == 1)
	{
		for(wb=0;wb<wb_num;wb++)
		{
			free(p_tx_qec_cfg[wb]);
		}
		free(p_tx_qec_cfg);
	}
    free(mag_error_ratio);
    free(ph_error_deg);
    free(mag_error_ratio_check);
    free(ph_error_deg_check);

	if(tdd_sel==1)
    {
		fn_tx_qec_diag_print(
			phy           ,
			&tx_qec_diag_info );
	}
	
	if(rx_qec_cfg->debug_level>=1)
	{
		fn_rx_qec_diag_print(
			phy           ,
			&rx_qec_diag_info );
	}

	flag_qec_ok = rxqec_diag_info_diag(phy,rx_qec_cfg->debug_level,&rx_qec_diag_info);
	
	// step 6 close if loopback route
	fn_close_loop_back(
		phy           ,
		chn           ,
		loopback_type ,
		&rf_loopback_regs);
		


    if(tdd_sel==1)
    {
        fn_tx_send_tone(phy, chn,0,2*wb);
    }
	
	if(tdd_sel==0)
    {
         fn_tx_set_dc_manual_ena(phy, chn,0,0);
    }
	
	if(rx_qec_cfg->debug_level>=4)
	{
		hal_spi_write_reg(phy, 0x480,0xA);
		LOG_MDEBUG(phy, TRX_QEC_CAL, "RX QEC done\n");
		//usleep(120000000);  // wait for 10*10 second
		hal_spi_write_reg(phy, 0x480,0xB);
	}

	rxqec_diag_info_free(&rx_qec_diag_info);
	txqec_diag_info_free(&tx_qec_diag_info);
	
	if(flag_qec_ok == -1)
	{
		rx_qec_cfg->fiiq_imag = 0xFFFF;
		rx_qec_cfg->fiiq_real = 0x0;;
		memset(rx_qec_cfg->fdiq_fir,0,sizeof(int)*32);
		rx_qec_cfg->fdiq_fir[16] = 0x4000;
		rx_qec_cfg->mag_ratio_adj = 1.0;
		rx_qec_cfg->ph_error_adj  = 0.0;
	}
	//else
	//{
	//	LOG_MDEBUG(phy,TRX_QEC_CAL,"FIIQ IMAG 0x%x\n",rx_qec_cfg->fiiq_real);
	//	LOG_MDEBUG(phy,TRX_QEC_CAL,"FIIQ IMAG 0x%x\n",rx_qec_cfg->fiiq_imag);
	//	LOG_MDEBUG(phy,TRX_QEC_CAL,"FDIQ TAP16 0x%x\n",rx_qec_cfg->fdiq_fir[16]);
	//	LOG_MDEBUG(phy,TRX_QEC_CAL,"mag_ratio_adj %.4f\n",rx_qec_cfg->mag_ratio_adj);
	//	LOG_MDEBUG(phy,TRX_QEC_CAL,"ph_error_adj  %.4f\n",rx_qec_cfg->ph_error_adj);
	//}
    return flag_qec_ok;
}


int fn_tx_qec_lol_self_calib_rflp(rf_chip_phy_t *phy, int chn ,unsigned long long fs_bb ,unsigned long long tx_lo, int ext_loop, int qec_dbfs, int lol_dbfs, TX_QEC_CFG_REGS * tx_qec_cfg)
{
	RF_LOOPBACK_REGS rf_loopback_reg;
	unsigned long long tx_freq_offset;
	unsigned int reg_tia_offset;
	unsigned int reg_bbf_offset;
	int mean_power;
	double rssi;
	int bbf_gain;
	int tia_gain;
	int flag_qec_ok = 0;
	int error_cnt = 0;
	unsigned char reg_val;
	//phy->module_debug = 0x40;
	
	// step 1 open if loopback route , and relock tx lo.
	tx_band_selection     (phy,  chn, tx_lo);

	// -------------- debug --------------- //
	// iio_daemon_1
	// LOG_MDEBUG(phy, TRX_QEC_CAL, "DEBUG QEC ROBUST RETURN POS 1 \n");
	// return -1;
	// -------------- debug --------------- //


	tx_freq_offset = fs_bb/32ull*2ull;
	rx_band_selection     (phy,  chn, tx_lo-tx_freq_offset);

	if (ext_loop!=1)
	{
		fn_rf_loop_back_ena(phy, chn,1,0,&rf_loopback_reg);
	}
	
	// -------------- debug --------------- //
	// iio_daemon_2
	// LOG_MDEBUG(phy, TRX_QEC_CAL, "DEBUG QEC ROBUST RETURN POS 2 \n");
	// return -1;
	// -------------- debug --------------- //
	
	// step 2 send TX test tone
	fn_tx_qec_off     (phy,  chn, 1 , 1);
	fn_tx_send_tone   (phy,  chn, 1 , 2);
	fn_tx_qec_gain_set(phy,  chn, tx_qec_cfg , qec_dbfs*2);

	// step3 set rx mixer to max gain
	if (ext_loop!=1)
	{
		if(tx_lo >2300000000ull)
		{
			fn_rf_loop_back_lg_ena(phy, chn , 1, &rf_loopback_reg);
		}
		else
		{
			//set rx mixer to max gain , lpf to min gain
			fn_rx_ana_set_gain(phy, chn,0,12);
		}
	}
	else
	{
        tia_gain = 5 - (qec_dbfs-10)/6;
        if(tia_gain>5)
            tia_gain = 5;
        else if(tia_gain<0)
            tia_gain = 0;
		tia_gain = 5;
        fn_rx_ana_set_gain(phy, chn,tia_gain,12);
	}
	
	// -------------- debug --------------- //
	//iio_daemon_3
	//LOG_MDEBUG(phy, TRX_QEC_CAL, "DEBUG QEC ROBUST RETURN POS 3 \n");
	//return -1;
	
	
	fn_rx_get_adc_power(phy, chn,4, &mean_power);
	mean_power = mean_power / 4;
	rssi = 20*log10(sqrt((double)mean_power)/pow(2.0,9.0));
	if(rssi<-12.0)
	{
		bbf_gain = (int) (  (-12.0)-rssi);
		if(bbf_gain>12)
		{
			bbf_gain = 12;
		}
		if(chn==0)
		{
			hal_spi_write_reg(phy, 0x65C,12-bbf_gain);
		}
		else
		{
			hal_spi_write_reg(phy, 0x67A,12-bbf_gain);
		}
	}

	//usleep(10000);
	//fn_rx_ana_offset_cal(phy, chn,10);
	fn_rx_ana_offset_cal_V2(phy,chn);
	
	// --------------- debug ---------------------- //
    // iio_daemon_4
	//LOG_MDEBUG(phy, TRX_QEC_CAL, "DEBUG QEC ROBUST RETURN POS 4 \n");
	//return -1;
	if(chn==0)
	{
		reg_val = hal_spi_read_reg(phy, 0x791);
		reg_val = (reg_val & 0xEF );
		hal_spi_write_reg(phy,0x791,reg_val);
		reg_val = (reg_val | 0x10 );
		hal_spi_write_reg(phy,0x791,reg_val);
	}
	else
	{	
		reg_val = hal_spi_read_reg(phy, 0x7A4);
		reg_val = (reg_val & 0xEF );
		hal_spi_write_reg(phy,0x7A4,reg_val);
		reg_val = (reg_val | 0x10 );
		hal_spi_write_reg(phy,0x7A4,reg_val);
	}
						
	// step 4 cal qec algorithm
	flag_qec_ok = fn_tx_qec_5tone_method_rflp(phy, chn,tx_qec_cfg,NULL);
	if(flag_qec_ok == -1)
	{
		LOG_MDEBUG(phy, TRX_QEC_CAL, "detect hbf need reset\n");
		//for(error_cnt=0;error_cnt<5;error_cnt++)
		//
		//	if(chn==0)
		//	{
		//		reg_val = hal_spi_read_reg(phy, 0x791);
		//		reg_val = (reg_val & 0xEF );
		//		hal_spi_write_reg(phy,0x791,reg_val);
		//		reg_val = (reg_val | 0x10 );
		//		hal_spi_write_reg(phy,0x791,reg_val);
		//	}
		//	else
		//	{	
		//		reg_val = hal_spi_read_reg(phy, 0x7A4);
		//		reg_val = (reg_val & 0xEF );
		//		hal_spi_write_reg(phy,0x7A4,reg_val);
		//		reg_val = (reg_val | 0x10 );
		//		hal_spi_write_reg(phy,0x7A4,reg_val);
		//	}
		//
		//	flag_qec_ok = fn_tx_qec_5tone_method_rflp(phy, chn,tx_qec_cfg);
		//	if (flag_qec_ok == -1)
		//	{
		//		LOG_MDEBUG(phy, TRX_QEC_CAL, "reset %d times | not work\n",error_cnt+1);
		//	}
		//	else 
		//	{
		//		LOG_MDEBUG(phy, TRX_QEC_CAL, "reset %d times | worked \n",error_cnt+1);
		//		break;
		//	}
		//
		flag_qec_ok = 0;
	}
	
	//if(flag_qec_ok==-1)
	//{
	//	return -1;
	//}
	
	LOG_MDEBUG(phy, TRX_QEC_CAL, "QEC done \n");
	// step 5 cal lol algorithm
	//fn_tx_send_tone   ( chn, 1 , 4);
	fn_tx_qec_gain_set(phy,  chn, tx_qec_cfg , lol_dbfs*2);
	tx_band_selection     (phy,  chn, tx_lo);
	tx_freq_offset = fs_bb/32ull*1ull;
	rx_band_selection     (phy,  chn, tx_lo-tx_freq_offset);

    if(ext_loop==1)
    {
        tia_gain = 5 - (lol_dbfs-10)/6;
        if(tia_gain>5)
            tia_gain = 5;
        else if(tia_gain<0)
            tia_gain = 0;

        tia_gain = 5;
        fn_rx_ana_set_gain(phy, chn,tia_gain,12);

        fn_rx_get_adc_power(phy, chn,4, &mean_power);
        mean_power = mean_power / 4;
        rssi = 20*log10(sqrt((double)mean_power)/pow(2.0,9.0));
        if(rssi<-12.0)
        {
            bbf_gain = (int) (  (-12.0)-rssi);
            if(bbf_gain>12)
            {
                bbf_gain = 12;
            }
            if(chn==0)
            {
                hal_spi_write_reg(phy, 0x65C,12-bbf_gain);
            }
            else
            {
                hal_spi_write_reg(phy, 0x67A,12-bbf_gain);
            }
        }
        fn_rx_ana_offset_cal(phy, chn,10);
    }
	
	flag_qec_ok = fn_tx_lol_5tone_method_rflp(phy, chn, tx_qec_cfg , NULL);

	LOG_MDEBUG(phy, TRX_QEC_CAL, "QEC and LOL done \n");
	// step 6 close if loopback route
    if(ext_loop!=1)
    {
        fn_rf_loop_back_ena(phy, chn,0,0,&rf_loopback_reg);
        if(tx_lo >2300000000ull)
        {
            fn_rf_loop_back_lg_ena(phy, chn , 0, &rf_loopback_reg);
        }
    }

	// step 7 set back



	return flag_qec_ok;
}

int fn_tx_qec_5tone_method_rflp(rf_chip_phy_t *phy, int chn, TX_QEC_CFG_REGS * tx_qec_cfg ,TX_QEC_DIAG_INFO * tx_qec_diag_info)
{
	int    iter_cnt = 0 ;
	int    ncoh_num = 1 ;
	int    ncoh_cnt = 0 ;
	int    rsb_cnt  = 0 ;
	int    confirm_cnt = 0;
	double mag_error_db_step;
	double ph_error_rad_step;
	double mag_error_ratio_set;
	double ph_error_deg_set;
	double mag_error_ratio_est;
	double ph_error_deg_est;
	double mag_goertzle_linear[7] = { 0 };
	double mag_goertzle_dB[7] = { 0 };
	double mag_error_ratio ;
	double ph_error_deg    ;
	int    flag_ok = 0;
	int    iter_num = 0;
	
	int    goertzle_0_i      ;
	int    goertzle_0_q      ;
	int    goertzle_1_i      ;
	int    goertzle_1_q      ;
	int    goertzle_shift    ;
	
	double mag_diff          = 0;
	double phg_diff          = 0;
	double rssi              = 0;
	double irr_result        = 0;
	
	int min_index;
	double min_power;
	if(tx_qec_cfg->cal_init==1)
	{
	 	mag_error_db_step   = 0.5  / 4.0;
	 	ph_error_rad_step   = 0.05 / 4.0;
	 	mag_error_ratio     = tx_qec_cfg->mag_ratio;
	 	ph_error_deg        = tx_qec_cfg->ph_error ;
	 	mag_error_ratio_est = 0;
	 	ph_error_deg_est    = 0;
		iter_num            = 5;
	}
	else
	{
		mag_error_ratio = 1;
			ph_error_deg  = 0;
		//mag_error_db_step   = 0.5;
		//ph_error_rad_step   = 0.05;
		mag_error_db_step   = 0.1;
		ph_error_rad_step   = 0.035;
		mag_error_ratio_est =  0;
		ph_error_deg_est    =  0;
		iter_num            =  5;
	}
	
	if(tx_qec_diag_info!=NULL)
	{
		fn_tx_qec_cal_words(mag_error_ratio, ph_error_deg, tx_qec_cfg);
		fn_tx_run_qec(phy, chn,tx_qec_cfg);
		
		mag_goertzle_linear[6]  = 0;
		for (ncoh_cnt = 0; ncoh_cnt < ncoh_num; ncoh_cnt++)
		{
			fn_rx_run_goertzle_new(phy, chn,0,1 ,1, 4096, 8 ,
				&goertzle_0_i, &goertzle_0_q, &goertzle_1_i, &goertzle_1_q, &goertzle_shift);
			//mag_goertzle_linear[6] += 
			//(pow((double)goertzle_1_i,2.0) + pow((double)goertzle_1_q,2.0)) * pow(4.0,(double)goertzle_shift)+
			//	(pow((double)goertzle_0_i,2.0) + pow((double)goertzle_0_q,2.0)) * pow(4.0,(double)goertzle_shift);
			mag_goertzle_linear[6] += 	(pow((double)(goertzle_0_i-goertzle_1_q),2.0) + pow((double)(goertzle_0_q+goertzle_1_i),2.0)) * pow(4.0,(double)goertzle_shift);
		}
		mag_goertzle_linear[6] = mag_goertzle_linear[6] / (double)ncoh_num;
		mag_goertzle_dB[6]     = (10.0*log10(mag_goertzle_linear[6] ) -234.79);
		tx_qec_diag_info->tx_qec_rsb_init[tx_qec_diag_info->wb_cnt] = (int)(mag_goertzle_dB[6]*10.0);
	}
	
	int exit_cnt = 0;
	for (iter_cnt = 0; iter_cnt < iter_num; iter_cnt++)
	{		
		exit_cnt ++;
		if (((fabs(mag_error_ratio_est) <0.05 && fabs(ph_error_deg_est) <0.02) ) && iter_cnt!=0)
		{
			if(exit_cnt>0)
			{
				exit_cnt = 0;
				mag_error_db_step = mag_error_db_step / 4.0;
				ph_error_rad_step = ph_error_rad_step / 4.0;
				confirm_cnt ++;
				//LOG_MDEBUG(phy, TRX_QEC_CAL, "narrow  !\n");
			}
		}
		else
		{
			exit_cnt = 0;
		}
		
		for (rsb_cnt = 0; rsb_cnt < 5; rsb_cnt++)
		{
			mag_goertzle_linear[rsb_cnt] = 0;
			mag_goertzle_dB[rsb_cnt] = 0;
			switch(rsb_cnt)
			{
			case 0:
				mag_error_ratio_set = mag_error_ratio;
				ph_error_deg_set    = ph_error_deg + ph_error_rad_step / 2.0 / PI * 360.0;
				break;
			case 1:
				mag_error_ratio_set = mag_error_ratio;
				ph_error_deg_set    = ph_error_deg - ph_error_rad_step / 2.0 / PI * 360.0;
				break;
			case 2:
				mag_error_ratio_set = mag_error_ratio + (pow(10.0,mag_error_db_step / 20.0) - 1);
				ph_error_deg_set    = ph_error_deg ;
				break;
			case 3:
				mag_error_ratio_set = mag_error_ratio - (pow(10.0,mag_error_db_step / 20.0) - 1);
				ph_error_deg_set    = ph_error_deg ;
				break;
			case 4:
				mag_error_ratio_set = mag_error_ratio + (pow(10.0,mag_error_db_step / 20.0) - 1);
				ph_error_deg_set    = ph_error_deg + ph_error_rad_step / 2.0 / PI * 360.0;
				break;
			default:
				return -1;
			}

			fn_tx_qec_cal_words(mag_error_ratio_set, ph_error_deg_set, tx_qec_cfg);
			fn_tx_run_qec(phy, chn,tx_qec_cfg);

			for (ncoh_cnt = 0; ncoh_cnt < ncoh_num; ncoh_cnt++)
			{
				fn_rx_run_goertzle_new(phy, chn, 0,1 ,1, 4096, 8 ,
				    &goertzle_0_i, &goertzle_0_q, &goertzle_1_i, &goertzle_1_q, &goertzle_shift);
				//mag_goertzle_linear[rsb_cnt] += sqrt(pow((double)goertzle_1_i,2.0) + pow((double)goertzle_1_q,2.0)) * pow(2.0,(double)goertzle_shift);
				//mag_goertzle_linear[rsb_cnt] += (pow((double)goertzle_1_i,2.0) + pow((double)goertzle_1_q,2.0)) * pow(4.0,(double)goertzle_shift);
				//mag_goertzle_linear[rsb_cnt] += (pow((double)goertzle_0_i,2.0) + pow((double)goertzle_0_q,2.0)) * pow(4.0,(double)goertzle_shift);
				mag_goertzle_linear[rsb_cnt] += 	(pow((double)(goertzle_0_i-goertzle_1_q),2.0) + pow((double)(goertzle_0_q+goertzle_1_i),2.0)) * pow(4.0,(double)goertzle_shift);
			}
			mag_goertzle_linear[rsb_cnt] = (mag_goertzle_linear[rsb_cnt] / (double)ncoh_num);
			mag_goertzle_dB[rsb_cnt]     = (10.0*log10(mag_goertzle_linear[rsb_cnt] ) -234.79);		
			
			if(tx_qec_diag_info!=NULL)
			{
				if(rsb_cnt==4)
				{
					tx_qec_diag_info->iter_cnt[tx_qec_diag_info->wb_cnt]++;
				}
			}
			if(tx_qec_cfg->debug_level>=3)
			{
				LOG_MDEBUG(phy, TRX_QEC_CAL, "%-6.2f ", mag_goertzle_dB[rsb_cnt]);
			}
		}

		mag_error_ratio_est = ((mag_goertzle_linear[2] - mag_goertzle_linear[3])/2.0) / (2.0*(mag_goertzle_linear[4] - mag_goertzle_linear[0]) - (mag_goertzle_linear[2] - mag_goertzle_linear[3])) *  (pow(10.0,mag_error_db_step / 20.0) - 1.0);
		ph_error_deg_est   =  ((mag_goertzle_linear[0] - mag_goertzle_linear[1])/2.0) / (2.0*(mag_goertzle_linear[4] - mag_goertzle_linear[2]) - (mag_goertzle_linear[0] - mag_goertzle_linear[1])) *  (ph_error_rad_step / 2.0 / PI * 360.0);

		//if((fabs(mag_error_ratio_est ) > 0.2) || (fabs(ph_error_deg_est ) > 2.5))
		//{
		//	flag_ok = -1;
		//}

		//mag_error_ratio = mag_error_ratio - mag_error_ratio_est * 0.8;
		//ph_error_deg    = ph_error_deg - ph_error_deg_est * 0.8;

		mag_error_ratio = mag_error_ratio - mag_error_ratio_est * 1;
		ph_error_deg    = ph_error_deg - ph_error_deg_est *1;

		fn_tx_qec_cal_words(mag_error_ratio, ph_error_deg,  tx_qec_cfg);
		fn_tx_run_qec(phy, chn,tx_qec_cfg);
		tx_qec_cfg->mag_ratio = mag_error_ratio;
		tx_qec_cfg->ph_error  = ph_error_deg;
		
		mag_goertzle_linear[5]  = 0;
		for (ncoh_cnt = 0; ncoh_cnt < ncoh_num; ncoh_cnt++)
		{
			fn_rx_run_goertzle_new(phy, chn,0,1 ,1, 4096, 8 ,
				&goertzle_0_i, &goertzle_0_q, &goertzle_1_i, &goertzle_1_q, &goertzle_shift);
			//mag_goertzle_linear[5] += 
			//(pow((double)goertzle_1_i,2.0) + pow((double)goertzle_1_q,2.0)) * pow(4.0,(double)goertzle_shift)+
			//	(pow((double)goertzle_0_i,2.0) + pow((double)goertzle_0_q,2.0)) * pow(4.0,(double)goertzle_shift);
			mag_goertzle_linear[5] += 	(pow((double)(goertzle_0_i-goertzle_1_q),2.0) + pow((double)(goertzle_0_q+goertzle_1_i),2.0)) * pow(4.0,(double)goertzle_shift);			
		}
		mag_goertzle_linear[5] = mag_goertzle_linear[5] / (double)ncoh_num;
		mag_goertzle_dB[5]     = (10.0*log10(mag_goertzle_linear[5] ) -234.79);			
			
		if(tx_qec_cfg->debug_level>=3)
		{
			
			LOG_MDEBUG(phy, TRX_QEC_CAL, "| %-7.4f %-7.4f %-7.4f %-7.4f | %-7.4f %-7.4f | %.2f\n", 
				mag_error_ratio_est,ph_error_deg_est,mag_error_db_step,ph_error_rad_step,mag_error_ratio,ph_error_deg,mag_goertzle_dB[5]);
		}	
		
		if(abs(mag_error_ratio_est)<=0.003 && fabs(ph_error_deg_est)<=0.1 && mag_goertzle_dB[5] < -85.0)
		{
			break;
		}
		
		min_index = 0;
		min_power = mag_goertzle_dB[0];
		for(rsb_cnt=1;rsb_cnt<7;rsb_cnt++)
		{
			if(mag_goertzle_dB[rsb_cnt] < min_power)
			{
				min_index = rsb_cnt;
				min_power = mag_goertzle_dB[rsb_cnt];
			}
		}
		
		if(min_index == 0)
		{
			mag_error_ratio = mag_error_ratio + mag_error_ratio_est ;
			ph_error_deg    = ph_error_deg    + ph_error_deg_est    + ph_error_rad_step / 2.0 / PI * 360.0;		
			mag_goertzle_dB[6] = mag_goertzle_dB[0];
		}
		else if(min_index == 1)
		{
			mag_error_ratio = mag_error_ratio + mag_error_ratio_est ;
			ph_error_deg    = ph_error_deg    + ph_error_deg_est    - ph_error_rad_step / 2.0 / PI * 360.0;			
			mag_goertzle_dB[6] = mag_goertzle_dB[1];
		}
		else if(min_index == 2)
		{
			mag_error_ratio = mag_error_ratio + mag_error_ratio_est + (pow(10.0,mag_error_db_step / 20.0) - 1);
			ph_error_deg    = ph_error_deg    + ph_error_deg_est    ;
			mag_goertzle_dB[6] = mag_goertzle_dB[2];			
		}
		else if(min_index == 3)
		{
			mag_error_ratio = mag_error_ratio + mag_error_ratio_est - (pow(10.0,mag_error_db_step / 20.0) - 1);
			ph_error_deg    = ph_error_deg    + ph_error_deg_est    ;	
			mag_goertzle_dB[6] = mag_goertzle_dB[3];				
		}
		else if(min_index == 4)
		{
			mag_error_ratio = mag_error_ratio + mag_error_ratio_est + (pow(10.0,mag_error_db_step / 20.0) - 1);
			ph_error_deg    = ph_error_deg    + ph_error_deg_est    +  ph_error_rad_step / 2.0 / PI * 360.0;		
			mag_goertzle_dB[6] = mag_goertzle_dB[4];	
		}
		else if(min_index == 5)
		{
			mag_goertzle_dB[6] = mag_goertzle_dB[5];	
		}
		else if(min_index == 6)
		{
			mag_error_ratio = mag_error_ratio + mag_error_ratio_est ;
			ph_error_deg    = ph_error_deg    + ph_error_deg_est    ;	
			mag_goertzle_dB[5] = mag_goertzle_dB[6];			
		}
	}
	
	if(tx_qec_diag_info!=NULL)
	{
		tx_qec_diag_info->tx_qec_mag_err [tx_qec_diag_info->wb_cnt] = (int)floor(mag_error_ratio*10000+0.5);
		tx_qec_diag_info->tx_qec_phg_err [tx_qec_diag_info->wb_cnt] = (int)floor(ph_error_deg   *10000+0.5);

		tx_qec_diag_info->tx_qec_rsb_final[tx_qec_diag_info->wb_cnt] = (int)(mag_goertzle_dB[5]*10.0);			
		
		if( fabs(1.0-((double)tx_qec_diag_info->tx_qec_mag_err[tx_qec_diag_info->wb_cnt])/10000.0) > 0.12)
		{
			LOG_MDEBUG(phy,TRX_QEC_CAL,"TX QEC ERROR  : DETECT INIT wb %d magnitude error is too large : %.4f db . \n",
				tx_qec_diag_info->wb_cnt,
				20*log10(( fabs(1.0-((double)tx_qec_diag_info->tx_qec_mag_err[tx_qec_diag_info->wb_cnt])/10000.0))));
			if(tx_qec_cfg->debug_level>=2)
			{
				while(1){}
			}
			else
			{
				return -1;
			}		
		}
		
		if( fabs(((double)(tx_qec_diag_info->tx_qec_phg_err[tx_qec_diag_info->wb_cnt])/10000.0)) > 10.0)
		{
			LOG_MDEBUG(phy,TRX_QEC_CAL,"TX QEC ERROR  : DETECT INIT wb %d phase error is too large : %.4f degree . \n",
				tx_qec_diag_info->wb_cnt,
				(double)(tx_qec_diag_info->tx_qec_phg_err[tx_qec_diag_info->wb_cnt])/10000.0);
			if(tx_qec_cfg->debug_level>=2)
			{
				while(1){}
			}
			else
			{
				return -1;
			}	
		}
		
		if(tx_qec_diag_info->wb_cnt>1)
		{
			mag_diff = fabs( (((double)tx_qec_diag_info->tx_qec_mag_err[tx_qec_diag_info->wb_cnt])/10000.0) - (((double)tx_qec_diag_info->tx_qec_mag_err[tx_qec_diag_info->wb_cnt-1])/10000.0));
			if ( mag_diff>0.01 )
			{
				LOG_MDEBUG(phy,TRX_QEC_CAL,"TX QEC ERROR  : DETECT large magnitude change %.4f between %d and %d \n",
					mag_diff,
					tx_qec_diag_info->wb_cnt,
					tx_qec_diag_info->wb_cnt-1);
				if(tx_qec_cfg->debug_level>=2)
				{
					while(1){}
				}
				else
				{
					return -1;
				}	
			}
			//else
			//{
			//	LOG_MDEBUG(phy,TRX_QEC_CAL,"RX QEC INFO  : magnitude change %.4f between %d and %d \n",
			//			mag_diff,
			//			tx_qec_diag_info->wb_cnt,
			//			tx_qec_diag_info->wb_cnt-1);				
			//}
		}
		
		if(tx_qec_diag_info->wb_cnt>1)
		{
			phg_diff = fabs( (((double)tx_qec_diag_info->tx_qec_phg_err[tx_qec_diag_info->wb_cnt])/10000.0) - (((double)tx_qec_diag_info->tx_qec_phg_err[tx_qec_diag_info->wb_cnt-1])/10000.0));
			if ( phg_diff >0.5)
			{
				LOG_MDEBUG(phy,TRX_QEC_CAL,"TX QEC ERROR  : DETECT large phase change %.4f between %d and %d \n",
					phg_diff,
					tx_qec_diag_info->wb_cnt,
					tx_qec_diag_info->wb_cnt-1);
				if(tx_qec_cfg->debug_level>=2)
				{
					while(1){}
				}
				else
				{
					return -1;
				}	
			}
			//else
			//{
			//	LOG_MDEBUG(phy,TRX_QEC_CAL,"RX QEC INFO  : phase change %.4f between %d and %d \n",
			//			phg_diff,
			//			tx_qec_diag_info->wb_cnt,
			//			tx_qec_diag_info->wb_cnt-1);
			//}
		}

		
		irr_result = (rssi - ((double)tx_qec_diag_info->tx_qec_rsb_final[tx_qec_diag_info->wb_cnt])/10.0);
		if(irr_result > 40.0 && irr_result <58.0)
		{
			LOG_MDEBUG(phy,TRX_QEC_CAL,"TX QEC WARNING  : DETECT a unsatisfactory result on %d : %.4f \n",
				tx_qec_diag_info->wb_cnt,irr_result);
		}
		else if(irr_result <= 40.0)
		{
			LOG_MDEBUG(phy,TRX_QEC_CAL,"TX QEC ERROR  : DETECT a very bad calibration result on %d : %.4f \n",
				tx_qec_diag_info->wb_cnt,irr_result);
			if(tx_qec_cfg->debug_level>=2)
			{
				while(1){}
			}
			else
			{
				return -1;
			}
		}		
		tx_qec_diag_info->wb_cnt ++;
	}
	return flag_ok;
}

int fn_tx_lol_5tone_method_rflp    (rf_chip_phy_t *phy, int chn,  TX_QEC_CFG_REGS * tx_qec_cfg , TX_LOL_DIAG_INFO * tx_lol_diag_info)
{
	int    iter_cnt = 0 ;
	int    ncoh_num = 1 ;
	int    ncoh_cnt = 0 ;
	int    rsb_cnt  = 0 ;
	int    i_branch_dc_step;
	int    q_branch_dc_step;
	int    i_branch_dc_set ;
	int    q_branch_dc_set ;
	int    i_branch_dc_adj ;
	int    q_branch_dc_adj ;
	double mag_goertzle_linear[7] = { 0 };
	double mag_goertzle_dB[7] = { 0 };
	int    i_branch_dc_est   ;
	int    q_branch_dc_est   ;
	int    goertzle_0_i      ;
	int    goertzle_0_q      ;
	int    goertzle_1_i      ;
	int    goertzle_1_q      ;
	int    goertzle_shift    ;
	int    confirm_cnt      = 0  ;
	int    i;
	int    flag_qec_ok      = 0 ;
	int    iter_num;
	int    dc_diff;
	double rssi              = 0;
	double irr_result        = 0;
	int    min_step          = 50;
	int    min_index         = 0;
	double min_power         = 0;
	
	if(tx_qec_cfg->cal_init==1)
	{
		//i_branch_dc_step = 200 / 8; //50*8; // s(15,14)   -> 12bit 96code
		//q_branch_dc_step = 200 / 8; //50*8;
		//i_branch_dc_est  = tx_qec_cfg->dc_i;
		//q_branch_dc_est  = tx_qec_cfg->dc_q;
		//i_branch_dc_adj  = 100;
		//q_branch_dc_adj  = 100;
		//iter_num         = 1;

		i_branch_dc_step = min_step; //50*8; // s(15,14)   -> 12bit 96code
		q_branch_dc_step = min_step; //50*8;
		i_branch_dc_est  = tx_qec_cfg->dc_i;
		q_branch_dc_est  = tx_qec_cfg->dc_q;
		i_branch_dc_adj  = 100;
		q_branch_dc_adj  = 100;
		iter_num         = 5;
		
		//i_branch_dc_step = 200; //50*8; // s(15,14)   -> 12bit 96code
		//q_branch_dc_step = 200; //50*8;
		//i_branch_dc_est  = tx_qec_cfg->dc_i;
		//q_branch_dc_est  = tx_qec_cfg->dc_q;
		//i_branch_dc_adj  = 100;
		//q_branch_dc_adj  = 100;	
		//iter_num         = 5;

	}
	else
	{
		i_branch_dc_step = 200; //50*8; // s(15,14)   -> 12bit 96code
		q_branch_dc_step = 200; //50*8;
		i_branch_dc_est  = 0;
		q_branch_dc_est  = 0;
		i_branch_dc_adj  = 100;
		q_branch_dc_adj  = 100;	
		iter_num         = 5;
	}


	if(tx_lol_diag_info!=NULL)
	{
		fn_tx_run_lol(phy, chn,i_branch_dc_est, q_branch_dc_est);
		mag_goertzle_linear[6]  = 0;
		for (ncoh_cnt = 0; ncoh_cnt < ncoh_num; ncoh_cnt++)
		{
			fn_rx_run_goertzle_new(phy, chn,0,1 ,1, 4096, 8 ,
				&goertzle_0_i, &goertzle_0_q, &goertzle_1_i, &goertzle_1_q, &goertzle_shift);
			//mag_goertzle_linear[6] += 
			//(pow((double)goertzle_1_i,2.0) + pow((double)goertzle_1_q,2.0)) * pow(4.0,(double)goertzle_shift)+
			//	(pow((double)goertzle_0_i,2.0) + pow((double)goertzle_0_q,2.0)) * pow(4.0,(double)goertzle_shift);
			mag_goertzle_linear[6] += 	(pow((double)(goertzle_0_i-goertzle_1_q),2.0) + pow((double)(goertzle_0_q+goertzle_1_i),2.0)) * pow(4.0,(double)goertzle_shift);
		}
		mag_goertzle_linear[6] = mag_goertzle_linear[6] / (double)ncoh_num;
		mag_goertzle_dB[6]     = (10.0*log10(mag_goertzle_linear[6] ) -234.79);
		tx_lol_diag_info->tx_lol_rsb_init[tx_lol_diag_info->wb_cnt] = (int)(mag_goertzle_dB[6]*10.0);
	}
		
	int cnt_exit = 0;
	for (iter_cnt = 0; iter_cnt < iter_num; iter_cnt++)
	{
		cnt_exit ++;
		//if ( ((iter_cnt > 2 ) || (abs(i_branch_dc_adj)<50  && abs(q_branch_dc_adj)<50)) && (cnt_exit>1) && (i_branch_dc_step>16) && (q_branch_dc_step>16))
		if (( (abs(i_branch_dc_adj)<40  && abs(q_branch_dc_adj)<40)) && iter_cnt !=0)
		{
			i_branch_dc_step = i_branch_dc_step >>1;
			q_branch_dc_step = q_branch_dc_step >>1;
			cnt_exit = 0;
			confirm_cnt ++;
			if( i_branch_dc_step < min_step || q_branch_dc_step < min_step )
			{
				i_branch_dc_step = min_step;
				q_branch_dc_step = min_step;
			}
		}

		for (rsb_cnt = 0; rsb_cnt < 5; rsb_cnt++)
		{
			mag_goertzle_linear[rsb_cnt] = 0;
			mag_goertzle_dB[rsb_cnt] = 0;
			switch(rsb_cnt)
			{
			case 0:
				i_branch_dc_set = i_branch_dc_est;
				q_branch_dc_set = q_branch_dc_est + q_branch_dc_step ;
				break;
			case 1:
				i_branch_dc_set = i_branch_dc_est;
				q_branch_dc_set = q_branch_dc_est - q_branch_dc_step;
				break;
			case 2:
				i_branch_dc_set = i_branch_dc_est + i_branch_dc_step;
				q_branch_dc_set = q_branch_dc_est ;
				break;
			case 3:
				i_branch_dc_set = i_branch_dc_est - i_branch_dc_step;
				q_branch_dc_set = q_branch_dc_est ;
				break;
			case 4:
				i_branch_dc_set = i_branch_dc_est + i_branch_dc_step;
				q_branch_dc_set = q_branch_dc_est + q_branch_dc_step;
				break;
			default:
				return -1;
			}

			fn_tx_run_lol(phy, chn,i_branch_dc_set, q_branch_dc_set);

			for (ncoh_cnt = 0; ncoh_cnt < ncoh_num; ncoh_cnt++)
			{
				fn_rx_run_goertzle_new(phy, chn,0,1 ,1, 4096, 8 ,
				&goertzle_0_i, &goertzle_0_q, &goertzle_1_i, &goertzle_1_q, &goertzle_shift);
				
				//mag_goertzle_linear[rsb_cnt] += (pow((double)goertzle_1_i,2.0) + pow((double)goertzle_1_q,2.0)) * pow(4.0,(double)goertzle_shift)+
				//(pow((double)goertzle_0_i,2.0) + pow((double)goertzle_0_q,2.0)) * pow(4.0,(double)goertzle_shift);
				
				mag_goertzle_linear[rsb_cnt] += 	(pow((double)(goertzle_0_i-goertzle_1_q),2.0) + pow((double)(goertzle_0_q+goertzle_1_i),2.0)) * pow(4.0,(double)goertzle_shift);
			}
			mag_goertzle_linear[rsb_cnt] = mag_goertzle_linear[rsb_cnt] / (double)ncoh_num;
			mag_goertzle_dB[rsb_cnt]     = (10.0*log10(mag_goertzle_linear[rsb_cnt] ) -234.79);
			if(tx_qec_cfg->debug_level>=3)
			{
				LOG_MDEBUG(phy, TRX_QEC_CAL, "%-6.2f ", mag_goertzle_dB[rsb_cnt]);
			}
			
			if(rsb_cnt==4)
			{
				tx_lol_diag_info->iter_cnt[tx_lol_diag_info->wb_cnt]++;
			}
		}
		i_branch_dc_adj = (int)(floor(((mag_goertzle_linear[2] - mag_goertzle_linear[3])/2.0) / (2.0*(mag_goertzle_linear[4] - mag_goertzle_linear[0]) - (mag_goertzle_linear[2] - mag_goertzle_linear[3])) * (double)i_branch_dc_step+0.5));
		q_branch_dc_adj = (int)(floor(((mag_goertzle_linear[0] - mag_goertzle_linear[1])/2.0) / (2.0*(mag_goertzle_linear[4] - mag_goertzle_linear[2]) - (mag_goertzle_linear[0] - mag_goertzle_linear[1])) * (double)q_branch_dc_step+0.5));
		
			
		//i_branch_dc_est = i_branch_dc_est - (i_branch_dc_adj >> 1) - (i_branch_dc_adj >> 2);
		//q_branch_dc_est = q_branch_dc_est - (q_branch_dc_adj >> 1) - (q_branch_dc_adj >> 2);

		i_branch_dc_est = i_branch_dc_est - i_branch_dc_adj;
		q_branch_dc_est = q_branch_dc_est - q_branch_dc_adj;
		
		fn_tx_run_lol(phy, chn,i_branch_dc_est, q_branch_dc_est);
		

		mag_goertzle_linear[5]  = 0;
		for (ncoh_cnt = 0; ncoh_cnt < ncoh_num; ncoh_cnt++)
		{
			fn_rx_run_goertzle_new(phy, chn,0,1 ,1, 4096, 8 ,
				&goertzle_0_i, &goertzle_0_q, &goertzle_1_i, &goertzle_1_q, &goertzle_shift);
			//mag_goertzle_linear[0] += 
			//(pow((double)goertzle_1_i,2.0) + pow((double)goertzle_1_q,2.0)) * pow(4.0,(double)goertzle_shift)+
			//	(pow((double)goertzle_0_i,2.0) + pow((double)goertzle_0_q,2.0)) * pow(4.0,(double)goertzle_shift);
			mag_goertzle_linear[5] += 	(pow((double)(goertzle_0_i-goertzle_1_q),2.0) + pow((double)(goertzle_0_q+goertzle_1_i),2.0)) * pow(4.0,(double)goertzle_shift);			
		}
		mag_goertzle_linear[5] = mag_goertzle_linear[5] / (double)ncoh_num;
		mag_goertzle_dB[5]     = (10.0*log10(mag_goertzle_linear[5] ) -234.79);		
		
		if(tx_qec_cfg->debug_level>=3)
		{			
			LOG_MDEBUG(phy, TRX_QEC_CAL, "| %-4d %-4d %-4d %-4d | %-4d %-4d | %.2f\n", 
				i_branch_dc_adj,q_branch_dc_adj,i_branch_dc_step,q_branch_dc_step,i_branch_dc_est,q_branch_dc_est,mag_goertzle_dB[5]);
		}
		
		
		if(abs(i_branch_dc_adj)<=10 && abs(q_branch_dc_adj)<=10 && mag_goertzle_dB[5] < -85.0)
		{
			break;
		}
		
		min_index = 0;
		min_power = mag_goertzle_dB[0];
		for(rsb_cnt=1;rsb_cnt<7;rsb_cnt++)
		{
			if(mag_goertzle_dB[rsb_cnt] < min_power)
			{
				min_index = rsb_cnt;
				min_power = mag_goertzle_dB[rsb_cnt];
			}
		}
		
		if(min_index == 0)
		{
			i_branch_dc_est = i_branch_dc_est + i_branch_dc_adj ;
			q_branch_dc_est = q_branch_dc_est + q_branch_dc_adj + q_branch_dc_step;		
			mag_goertzle_dB[6] = mag_goertzle_dB[0];
		}
		else if(min_index == 1)
		{
			i_branch_dc_est = i_branch_dc_est + i_branch_dc_adj ;
			q_branch_dc_est = q_branch_dc_est + q_branch_dc_adj - q_branch_dc_step;			
			mag_goertzle_dB[6] = mag_goertzle_dB[1];
		}
		else if(min_index == 2)
		{
			i_branch_dc_est = i_branch_dc_est + i_branch_dc_adj + i_branch_dc_step;
			q_branch_dc_est = q_branch_dc_est + q_branch_dc_adj;	
			mag_goertzle_dB[6] = mag_goertzle_dB[2];			
		}
		else if(min_index == 3)
		{
			i_branch_dc_est = i_branch_dc_est + i_branch_dc_adj - i_branch_dc_step;
			q_branch_dc_est = q_branch_dc_est + q_branch_dc_adj ;	
			mag_goertzle_dB[6] = mag_goertzle_dB[3];				
		}
		else if(min_index == 4)
		{
			i_branch_dc_est = i_branch_dc_est + i_branch_dc_adj + i_branch_dc_step;
			q_branch_dc_est = q_branch_dc_est + q_branch_dc_adj - i_branch_dc_step;				
			mag_goertzle_dB[6] = mag_goertzle_dB[4];	
		}
		else if(min_index == 5)
		{
			mag_goertzle_dB[6] = mag_goertzle_dB[5];	
		}
		else if(min_index == 6)
		{
			i_branch_dc_est = i_branch_dc_est + i_branch_dc_adj;
			q_branch_dc_est = q_branch_dc_est + q_branch_dc_adj;
			mag_goertzle_dB[5] = mag_goertzle_dB[6];
		}
		//LOG_MDEBUG(phy, TRX_QEC_CAL, "---- set final magratio %d phase degree %d\n",
		//LOG_MDEBUG(phy, TRX_QEC_CAL, "---- set final magratio %d phase degree %d\n",
		//i_branch_dc_est, q_branch_dc_est);
	
	}


	/* 

	double min_power = 1000;
	int min_i;
	int min_q;
	for(int i = -128;i<=128;i = i+16)
	{
	for(int j = -128;j<=128;j= j+16)
	{
	fn_tx_run_lol(chn,i_branch_dc_est+i, q_branch_dc_est+j);
	mag_goertzle_linear[rsb_cnt] = 0;
	for (ncoh_cnt = 0; ncoh_cnt < ncoh_num; ncoh_cnt++)
	{
	fn_rx_run_goertzle(chn,0,1 ,1, 4096, 128 ,
	&goertzle_0_i, &goertzle_0_q, &goertzle_1_i, &goertzle_1_q, &goertzle_shift);
	mag_goertzle_linear[rsb_cnt] += (pow((double)goertzle_1_i,2.0) + pow((double)goertzle_1_q,2.0)) * pow(4.0,(double)goertzle_shift)+
	(pow((double)goertzle_0_i,2.0) + pow((double)goertzle_0_q,2.0)) * pow(4.0,(double)goertzle_shift);
	}
	mag_goertzle_linear[rsb_cnt] = mag_goertzle_linear[rsb_cnt] / (double)ncoh_num;
	mag_goertzle_dB[rsb_cnt]     = 20.0*log10(mag_goertzle_linear[rsb_cnt] );
	if(mag_goertzle_dB[rsb_cnt] <min_power)
	{
	min_power = mag_goertzle_dB[rsb_cnt] ;
	min_i = i;
	min_q = j;
	}
	tmp = "";
	tmp.sprintf("%4d %4d RSB%d | %f dB", i,j,rsb_cnt, mag_goertzle_dB[rsb_cnt]);
	qDebug() << "" << tmp;
	//LOG_MDEBUG(phy, TRX_QEC_CAL, "RSB%d | %f dB\n", rsb_cnt, mag_goertzle_dB[rsb_cnt]);
	}
	}

	i_branch_dc_est= i_branch_dc_est+min_i;
	q_branch_dc_est= q_branch_dc_est+min_q;

	fn_tx_run_lol(chn,i_branch_dc_est, q_branch_dc_est);

	*/
	tx_qec_cfg->dc_i = i_branch_dc_est;
	tx_qec_cfg->dc_q = q_branch_dc_est;
	
	if(tx_lol_diag_info!=NULL)
	{
		tx_lol_diag_info->tx_lol_dc_i [tx_lol_diag_info->wb_cnt] = i_branch_dc_est;
		tx_lol_diag_info->tx_lol_dc_q [tx_lol_diag_info->wb_cnt] = q_branch_dc_est;
		
		tx_lol_diag_info->tx_lol_rsb_final[tx_lol_diag_info->wb_cnt] = (int)(mag_goertzle_dB[5]*10.0);

		if( abs(tx_lol_diag_info->tx_lol_dc_i[tx_lol_diag_info->wb_cnt] > 500) || abs(tx_lol_diag_info->tx_lol_dc_q [tx_lol_diag_info->wb_cnt]> 500))
		{
			LOG_MDEBUG(phy,TRX_QEC_CAL,"TX LOL ERROR  : DETECT INIT wb %d DCI and DCQ error is too large : %4d %4d \n",
				tx_lol_diag_info->wb_cnt,
				tx_lol_diag_info->tx_lol_dc_i[tx_lol_diag_info->wb_cnt],
				tx_lol_diag_info->tx_lol_dc_q[tx_lol_diag_info->wb_cnt]);
			if(tx_qec_cfg->debug_level>=2)
			{
				while(1){}
			}
			else
			{
				return -1;
			}	
		}
		else if( abs(tx_lol_diag_info->tx_lol_dc_i[tx_lol_diag_info->wb_cnt] > 200) || abs(tx_lol_diag_info->tx_lol_dc_q[tx_lol_diag_info->wb_cnt] > 200))
		{
			LOG_MDEBUG(phy,TRX_QEC_CAL,"TX LOL WARNING  : DETECT INIT wb %d DCI and DCQ error is too large : %4d %4d \n",
				tx_lol_diag_info->wb_cnt,
				tx_lol_diag_info->tx_lol_dc_i,
				tx_lol_diag_info->tx_lol_dc_q);	
		}
		
		if(tx_lol_diag_info->wb_cnt>1)
		{
			dc_diff = abs( tx_lol_diag_info->tx_lol_dc_i[tx_lol_diag_info->wb_cnt]  - tx_lol_diag_info->tx_lol_dc_i[tx_lol_diag_info->wb_cnt-1]);
			if ( dc_diff > 200 )
			{
				LOG_MDEBUG(phy,TRX_QEC_CAL,"TX LOL ERROR  : DETECT large magnitude change %d on I between %d and %d \n",
					dc_diff,
					tx_lol_diag_info->wb_cnt,
					tx_lol_diag_info->wb_cnt-1);
				if(tx_qec_cfg->debug_level>=2)
				{
					while(1){}
				}
				else
				{
					return -1;
				}				
			}
			dc_diff = abs( tx_lol_diag_info->tx_lol_dc_q[tx_lol_diag_info->wb_cnt]  - tx_lol_diag_info->tx_lol_dc_q[tx_lol_diag_info->wb_cnt-1]);
			if ( dc_diff > 200 )
			{
				LOG_MDEBUG(phy,TRX_QEC_CAL,"TX LOL ERROR  : DETECT large magnitude change %d on Q between %d and %d \n",
					dc_diff,
					tx_lol_diag_info->wb_cnt,
					tx_lol_diag_info->wb_cnt-1);
				if(tx_qec_cfg->debug_level>=2)
				{
					while(1){}
				}
				else
				{
					return -1;
				}				
			}
		}
		
		irr_result = (rssi - ((double)tx_lol_diag_info->tx_lol_rsb_final[tx_lol_diag_info->wb_cnt])/10.0);
		if(irr_result > 40.0 && irr_result <58.0)
		{
			LOG_MDEBUG(phy,TRX_QEC_CAL,"TX LOL WARNING  : DETECT a unsatisfactory calibration result on %d : %.4f \n",
				tx_lol_diag_info->wb_cnt,irr_result);
		}
		else if(irr_result <= 40.0)
		{
			LOG_MDEBUG(phy,TRX_QEC_CAL,"TX LOL ERROR    : DETECT a very bad calibration result on %d : %.4f \n",
				tx_lol_diag_info->wb_cnt,irr_result);
			if(tx_qec_cfg->debug_level>=2)
			{
				while(1){}
			}
			else
			{
				return -1;
			}
		}
		tx_lol_diag_info->wb_cnt ++;
		
	}
	//LOG_MDEBUG(phy, TRX_QEC_CAL, "---- set final dci %d phase dcq %d\n",
	//			i_branch_dc_est, q_branch_dc_est);
	
	return flag_qec_ok;
}

void fn_rx_qec_tracking(rf_chip_phy_t *phy, 
                            int                      rx_select,
                            int                      dbfs_th   ,
                            RX_QEC_CFG_REGS        * rx_qec_cfg)
{
	double    mag_ratio;
	double    ph_error ;
	double    mag_ratio_adj;
	double    ph_error_adj ;
	long long usv      ;
	double    usv_db       ;
	long long usv_max  ;


	int       update_cnt = 0;

	while (1)
	{
		usv_max = 0;
		fn_rx_qec_blind_est(phy, 0,122880*80,2,&mag_ratio,&ph_error,&usv);
		usv_db = 10*log10((double)usv);
		LOG_MDEBUG(phy, TRX_QEC_CAL, "usv_db %.3f\n",usv_db);

		if( usv_db > ( 141.4 + (double)dbfs_th   ))
		{
			//mag_ratio_adj = mag_ratio/mag_ratio_1*mag_ratio_0;
			//ph_error_adj  = ph_error - ph_error_1 + ph_error_0;
			//rx_qec_cfg->mag_ratio_adj = mag_error_ratio[0]/mag_error_ratio[1];
			//rx_qec_cfg->ph_error_adj = ph_error_deg[0] - ph_error_deg[1];
			mag_ratio_adj = mag_ratio * rx_qec_cfg->mag_ratio_adj;
			ph_error_adj  = ph_error + rx_qec_cfg->ph_error_adj;

			LOG_MDEBUG(phy, TRX_QEC_CAL, "====== update =============\n");
			LOG_MDEBUG(phy, TRX_QEC_CAL, "mag_ratio %.8f ph_error %.8f | final %.8f %.8f\n",
                       mag_ratio,
                       ph_error,
					   mag_ratio_adj,
					   ph_error_adj);

            //-----------------
            fn_rx_qec_update_fiiq_coeff(phy, rx_select, mag_ratio_adj, ph_error_adj);

			//-----------------
			update_cnt ++;
			//usleep(1000000);
		}
	}
}

int fn_rx_qec_blind_est(rf_chip_phy_t *phy, int rx_select,int coh_num,int freq_div_32,double *mag_ratio,double *ph_error,long long int *usv_rpt)
{
	long long int umv_i    = 0;
	long long int umv_q    = 0;
	long long int usv      = 0;
	unsigned int  addr1    ;
	unsigned int  addr2    ;
	unsigned int  addr3    ;
	unsigned int  addr     ;
	unsigned int  read_val ;
	unsigned int  addr_real;
	int           i        ;
	int           wait_cnt ;
	int           flg_ok   ;
	double        K1K2_real;
	double        K1K2_imag;

	if (rx_select == 0)
	{
		addr1 = 0x480;
		addr2 = 0x481;
		addr3 = 0x482;
	}
	else
	{
		addr1 = 0xC80;
		addr2 = 0xC81;
		addr3 = 0xC82;
	}
	hal_spi_write_reg(phy, addr1,(coh_num>> 0)&0xff);
	hal_spi_write_reg(phy, addr2,(coh_num>> 8)&0xff);
	hal_spi_write_reg(phy, addr3,(coh_num>>16)&0xff);

	if (rx_select == 0)
	{
		addr1 = 0x483;
	}
	else
	{
		addr1 = 0xC83;
	}
	hal_spi_write_reg(phy, addr1,freq_div_32);
	hal_spi_write_reg(phy, addr1,freq_div_32+128); // start

	wait_cnt = 100;
	flg_ok   = 0;
	while(wait_cnt> 0)
	{
		wait_cnt--;
		CHIP_UDELAY(10000);
		read_val = hal_spi_read_reg(phy, addr1);
		if(((read_val>>6)&0x1) == 1)
		{
			flg_ok = 1;
			break;
		}
	}

	if(flg_ok==0)
	{
		*mag_ratio = 0;
		*ph_error  = 0;
	}
	else
	{
		umv_i = 0;
		if(rx_select == 0)
			addr = 0x484;
		else
			addr = 0xC84;

		for(i = 0;i<=6;i++)
		{
			addr_real = addr + i;
			read_val  = hal_spi_read_reg(phy, addr_real);
			umv_i     = umv_i + (((long long int)(read_val))<<(8ll*((long long int)i)));
		}

		if( umv_i > (1ll<<48ll))
		{
			umv_i = umv_i - (1ll<<49ll);
		}

		umv_q = 0;
		if(rx_select == 0)
		{
			addr = 0x48C;
		}
		else
		{
			addr = 0xC8C;
		}

		for( i=0;i<=6;i++)
		{
			addr_real = addr + i;
			read_val  = hal_spi_read_reg(phy, addr_real);
			umv_q     = umv_q + (((int long long)(read_val))<<(8ll*((int long long )i)));
		}

		if (umv_q > (1ll<<48ll))
		{
			umv_q = umv_q - (1ll<<49ll);
		}

		usv = 0;
		if(rx_select == 0)
		{
			addr = 0x494;
		}
		else
		{
			addr = 0xC94;
		}

		for (i=0; i<=6; i++)
		{
			addr_real = addr + i;
			read_val  = hal_spi_read_reg(phy, addr_real);
			usv       = usv + (((long long int)(read_val))<<(8ll*((long long int)i)));
		}

		if(usv > (1ll<<48ll))
		usv = usv - (1ll<<49ll);

		K1K2_real = (double)umv_i/(double)usv ;
		K1K2_imag = (double)umv_q/(double)usv ;

		//LOG_MDEBUG(phy, TRX_QEC_CAL, "%d | umv_i %lld umv_q %lld usv %lld\n",
		//freq_div_32,umv_i,umv_q,usv);
		*mag_ratio = sqrt(1.0-4.0*K1K2_real);
		*ph_error  = asin(-2.0/(*mag_ratio)*K1K2_imag) /2.0/PI * 360.0;
		*mag_ratio =  1.0/(*mag_ratio);
		*ph_error  = -1.0*(*ph_error );
		*usv_rpt = usv;
	}

	return 0;
}


