#include "main_init.h"
#include "platform.h"
#include "driver.h"
#include "custom_cfg.h"
#include "app_cmd.h"
//#include "qec_tracking_task.h"
#include "qec_chip.h"


/***********************************************************************************/
// test app
/***********************************************************************************/
static void device_version(char *argv[])
{
	LOG_MAIN("device version: %s\n", DEVICE_VERSION);
}

static void chip_set_trx_flo(char *argv[])
{
	unsigned long long flo=0;
	unsigned char dir=0;

	dir = atoi(argv[2]);
	flo = strtoull(argv[3], NULL, 10);
	LOG_MAIN("lo set, dir:%d, freq:%llu\n", dir, flo);

	if(dir == RX_DIR) {
		trx_lo_change(&g_phy_obj[g_phy_select], 0, flo);
	} else if (dir == TX_DIR) {
		trx_lo_change(&g_phy_obj[g_phy_select], flo, 0);
	} else {
		LOG_ERROR("Invalid direction(trx), dir=%d\r\n", dir);
	
	}
}

static void chip_select(char *argv[])
{
	unsigned char sel=0;
	sel = atoi(argv[2]);
	g_phy_select = sel;
	cmd_api_phy_obj_init(g_phy_select);
}

static void fpga_reset(char *argv[])
{
	/* reset fpga */
	LOG_MAIN("fpga_reset\n");
	hal_fpga_write_reg(&g_phy_obj[g_phy_select], 0x70, 0x3);
	hal_fpga_write_reg(&g_phy_obj[g_phy_select], 0x8, 0x3);
}

static void chip_debug_on(char *argv[])
{
	unsigned char on=0;
	on = atoi(argv[2]);
	LOG_MAIN("debug %s\n", on? "on": "off");
	g_phy_obj[g_phy_select].debug_on = on;
}

static void chip_module_debug_on(char *argv[])
{
	unsigned long on=0;
	on = strtoul(argv[2], NULL, 10);
	cmd_api_module_debug_on(on);
}

static void chip_write_reg(char *argv[])
{
	int reg, val;
	reg = strtol(argv[2], NULL, 16);
	val = strtol(argv[3], NULL, 16);
	cmd_api_write_reg(reg, val);
}

static void chip_read_reg(char *argv[])
{
	int reg;
	reg = strtol(argv[2], NULL, 16);
	cmd_api_read_reg(reg);
}

static void chip_read_reg_file(char *argv[])
{
#if HAVE_FS
	unsigned short  reg = 0x0;
    unsigned char   val;
    int             reg_max = 0xFFF;
    FILE            *fpOut = NULL;
    char            *filename = argv[2];
    char            buf[128] = {0};
    
    /* write the result to $outfile. */    
    fpOut = fopen(filename, "w");
    if (fpOut == NULL)
    {
        LOG_MAIN("[INFO], the %s open fail, go ahead!\r\n", fpOut);
    }    
    else
    {
        for (; reg < reg_max; reg ++)
        {
            memset(buf, 0, 128 * sizeof(char));
            sprintf(buf, "[0x%03X] = 0x%02X\n", reg, hal_spi_read_reg(&g_phy_obj[g_phy_select], reg));
            fputs(buf, fpOut);
        }
        LOG_MAIN("%s store the result in the file %s\n", __FUNCTION__, filename);
        fclose(fpOut);
    }
#endif
}

static void chip_fwrite_reg(char *argv[])
{
	int reg, val;

	reg = strtol(argv[2], NULL, 16);
	val = strtol(argv[3], NULL, 16);
	cmd_api_fwrite_reg(reg, val);
}

static void chip_fread_reg(char *argv[])
{
	int reg;
	reg = strtol(argv[2], NULL, 16);
	cmd_api_fread_reg(reg);
}

static void chip_power_init(char *argv[])
{	
	cmd_api_power_init();
}

static void chip_rcal(char *argv[])
{
	unsigned short rcal_read=0;
	rcal_read = atoi(argv[2]);
	cmd_api_rcal(rcal_read);
}

static void chip_sys_clock_init(char *argv[])
{
	unsigned long freq=0;
	unsigned long long flo=0;
	
	freq = strtoul(argv[2], NULL, 10);
	flo = strtoull(argv[3], NULL, 10);
	cmd_api_sys_clock_init(freq, flo);
}

static void chip_lut_load(char *argv[])
{
	unsigned char mode=0, use_hybrid=0, hybrid_mode=0;
	mode = atoi(argv[2]);
	use_hybrid = atoi(argv[3]);
	hybrid_mode = atoi(argv[4]);
	cmd_api_lut_load(mode, use_hybrid, hybrid_mode);
}

static void chip_lut_readword(char *argv[])
{
    LUT_INDEX_ENUM lut = 0;
    int lut_addr = 0;
    unsigned char vals[4];
    
    lut = atoi(argv[2]);
    lut_addr = strtol(argv[3], NULL, 16);

    if(0 == cmd_api_read_lut_word(lut, lut_addr, vals))
    {
        LOG_MAIN("R:lut_idx:%2d, lut_addr:[0x%03X] = [3->0]    %02X%02X%02X%02X\n", 
            lut, lut_addr, vals[3], vals[2], vals[1], vals[0]);
    }    
}

static void dump_lut_tofile(LUT_INDEX_ENUM  lut, int lut_addr_start, int lut_addr_length, char *filename)
{
#if HAVE_FS
    int             lut_addr = 0x00;
    FILE            *fpOut = NULL;
    char            buf[128] = {0};    
    unsigned char   vals[4];

    /* write the result to $outfile. */    
    fpOut = fopen(filename, "a+");
    if (fpOut == NULL)
    {
        LOG_MAIN("[INFO], the %s open fail, go ahead!\r\n", fpOut);
    }    
    else
    {    
        fseek(fpOut, 0, SEEK_END); 
        for (lut_addr = lut_addr_start; lut_addr < lut_addr_length; lut_addr++)
        {
            memset(vals, 0, 4 * sizeof(char));
            if(0 == cmd_api_read_lut_word(lut, lut_addr, vals))
            {
                memset(buf, 0, 128 * sizeof(char));
                sprintf(buf, "lut_idx:%2d, lut_addr:[0x%03X] = [3->0]    %02X%02X%02X%02X\n", 
                    lut, lut_addr, vals[3], vals[2], vals[1], vals[0]);
                fputs(buf, fpOut);
            }
        }
        fclose(fpOut);
    }
#endif
}


static void chip_lut_readfile(char *argv[])
{
#if HAVE_FS
    LUT_INDEX_ENUM  lut = atoi(argv[2]);
    int             lut_addr_start = 0x00;
    int             lut_addr_length = 0x00;
    int             lut_addr = 0x00;
    FILE            *fpOut = NULL;
    char            *filename = argv[3];
    char            buf[128] = {0};    
    unsigned char   vals[4];

    /* write the result to $outfile. */    
    fpOut = fopen(filename, "w");
    if (fpOut == NULL)
    {
        LOG_MAIN("[INFO], the %s open fail, go ahead!\r\n", fpOut);
    }    
    else
    {
        fclose(fpOut);
        switch(lut)
        {
            case MAIN_ENSM_LUT:
                dump_lut_tofile(lut, TRANSCV_LUT_MAIN_ADDR_START, TRANSCV_LUT_MAIN_ADDR_LENGTH, filename);
                break;
                
            case RX1_AGC_LUT:
                LOG_MAIN("So far, we have not use RX1_AGC_LUT!\n");
                break;

            case RX2_AGC_LUT:
                LOG_MAIN("So far, we have not use RX2_AGC_LUT!\n");
                break;
                
            case TXRX_IQ_LUT:
                dump_lut_tofile(lut, TRANSCV_LUT_TXRSB_ADDR_START, TRANSCV_LUT_TXRSB_ADDR_LENGTH, filename);
                break;
                
            case SX_CONFIG_LUT:
                dump_lut_tofile(lut, TRANSCV_LUT_SXCONFIG_ADDR_START, TRANSCV_LUT_SXCONFIG_ADDR_LENGTH, filename);
                break;
                
            case TX_GAIN_LUT:
                dump_lut_tofile(lut, TRANSCV_LUT_TXGAIN_ADDR_START, TRANSCV_LUT_TXGAIN_ADDR_LENGTH, filename);
                break;
                
            case SX_CAL_LUT:
                dump_lut_tofile(lut, TRANSCV_LUT_SXCAL_ADDR_START, TRANSCV_LUT_SXCAL_ADDR_LENGTH, filename);                
                break;
                
            case TXLO_CAL_LUT:
                dump_lut_tofile(lut, TRANSCV_LUT_TXLOCAL_ADDR_START, TRANSCV_LUT_TXLOCAL_ADDR_LENGTH , filename);
                break;
                
            case TRX_BW_LUT:
                dump_lut_tofile(lut, TRANSCV_LUT_TRXBW_ADDR_START, TRANSCV_LUT_TRXBW_ADDR_LENGTH, filename);
                break;

            case RXIP2_CAL_LUT:
                dump_lut_tofile(lut, TRANSCV_LUT_RXIP2CAL_ADDR_START, TRANSCV_LUT_RXIP2CAL_ADDR_LENGTH, filename);
                break;

            case TRX_BAND_LUT:
                dump_lut_tofile(lut, TRANSCV_LUT_TRXBAND_ADDR_START, TRANSCV_LUT_TRXBAND_ADDR_LENGTH, filename);                
                break;

            default:
                /* dump all lut table*/
                dump_lut_tofile(MAIN_ENSM_LUT, TRANSCV_LUT_MAIN_ADDR_START, TRANSCV_LUT_MAIN_ADDR_LENGTH, filename);
                dump_lut_tofile(TXRX_IQ_LUT, TRANSCV_LUT_TXRSB_ADDR_START, TRANSCV_LUT_TXRSB_ADDR_LENGTH, filename);
                dump_lut_tofile(SX_CONFIG_LUT, TRANSCV_LUT_SXCONFIG_ADDR_START, TRANSCV_LUT_SXCONFIG_ADDR_LENGTH, filename);
                dump_lut_tofile(TX_GAIN_LUT, TRANSCV_LUT_TXGAIN_ADDR_START, TRANSCV_LUT_TXGAIN_ADDR_LENGTH, filename);
                dump_lut_tofile(SX_CAL_LUT, TRANSCV_LUT_SXCAL_ADDR_START, TRANSCV_LUT_SXCAL_ADDR_LENGTH, filename);
                dump_lut_tofile(TXLO_CAL_LUT, TRANSCV_LUT_TXLOCAL_ADDR_START, TRANSCV_LUT_TXLOCAL_ADDR_LENGTH , filename);
                dump_lut_tofile(TRX_BW_LUT, TRANSCV_LUT_TRXBW_ADDR_START, TRANSCV_LUT_TRXBW_ADDR_LENGTH, filename);
                dump_lut_tofile(RXIP2_CAL_LUT, TRANSCV_LUT_RXIP2CAL_ADDR_START, TRANSCV_LUT_RXIP2CAL_ADDR_LENGTH, filename);
                dump_lut_tofile(TRX_BAND_LUT, TRANSCV_LUT_TRXBAND_ADDR_START, TRANSCV_LUT_TRXBAND_ADDR_LENGTH, filename); 
                break;
        } 
        LOG_MAIN("%s store the result in the file %s\n", __FUNCTION__, filename);
    }
#endif
}

static void chip_dciq_dumpfile(char *argv[])
{
#if HAVE_FS
    int             cnt = atoi(argv[2]);
    FILE            *fpOut = NULL;
    char            *filename = argv[3];
    char            buf[128] = {0};
    int             i =0;
    int             try_num = 1;
    int             dc_offset_i[TRX_CH_CNT] = {0};
    int             dc_offset_q[TRX_CH_CNT] = {0};
    

    if (cnt <= 0)
    {
        LOG_MAIN("[INFO], dump cnt: %d not valid!\r\n", cnt);
        return ;
    }

    /* write the result to $outfile. */    
    fpOut = fopen(filename, "w");
    if (fpOut == NULL)
    {
        LOG_MAIN("[INFO], the %s open fail, go ahead!\r\n", fpOut);
    }    
    else
    {
        for (i = 0; i < cnt; i++)
        {
            memset(dc_offset_i, 0, TRX_CH_CNT * sizeof(int));
            memset(dc_offset_q, 0, TRX_CH_CNT * sizeof(int)); 
            
            fn_rx_get_adc_offset(&g_phy_obj[g_phy_select], TRX_CHN1, try_num, &dc_offset_i[TRX_CHN1], &dc_offset_q[TRX_CHN1]);
            fn_rx_get_adc_offset(&g_phy_obj[g_phy_select], TRX_CHN2, try_num, &dc_offset_i[TRX_CHN2], &dc_offset_q[TRX_CHN2]);
       
            memset(buf, 0, 128 * sizeof(char));
            sprintf(buf, "idx:%5d, RX1:%5d, %5d; RX2:%5d, %5d\n", 
                i+1, 
                dc_offset_i[TRX_CHN1]/try_num, dc_offset_q[TRX_CHN1]/try_num, 
                dc_offset_i[TRX_CHN2]/try_num, dc_offset_q[TRX_CHN2]/try_num);
            fputs(buf, fpOut);
            
            CHIP_DELAY(1000);
        }
        LOG_MAIN("%s store the result in the file %s\n", __FUNCTION__, filename);
        fclose(fpOut);
    }
#endif
}

static void chip_get_dciq(char *argv[])
{
    int             chn         = atoi(argv[2]);
    int             try_num     = atoi(argv[3]);
    int             dc_offset_i = 0;
    int             dc_offset_q = 0;

    if (try_num <= 0 || chn < 0 || chn >= 2)
    {
        LOG_MAIN("[INFO], chn: %d try_num: %d , not valid!\r\n", chn, try_num);
        return ;
    }
   
    fn_rx_get_adc_offset(&g_phy_obj[g_phy_select], chn, try_num, &dc_offset_i, &dc_offset_q);
    LOG_MAIN("dciq RESULT ====> TRX_CHN%d, try_num:%d,  DCI: %5d, DCQ: %5d\n",  
           chn + 1, try_num, dc_offset_i, dc_offset_q);    

}

static void chip_set_tia_setting(char *argv[])
{
    int ret = 0;
    TRX_CHN_ENUM    chn    = atoi(argv[2]);
    unsigned char   i_data = strtol(argv[3], NULL, 16);
    unsigned char   q_data = strtol(argv[4], NULL, 16);    
    GCTRL_GET_RPT_T cfg = {0};
    int             pos_stat = 0;
    rf_chip_phy_t   *phy = &g_phy_obj[g_phy_select];

    if (rx_dc_get_tia_lut_pos(phy, chn, &pos_stat))
    {
        LOG_ERROR("%s in line %d REPORTED ERR for get pos fail\n", __FUNCTION__, __LINE__);
        return;
    }
    
    GCTRL_GET_RPT(phy, chn, &cfg);    
    
    ret = rx_dc_set_tia_lut_setting_by_idx(phy, chn, pos_stat, cfg.rpt_lmt_index, i_data, q_data);
    if (ret != 0)
    {
        LOG_MAIN("%s in line %d REPORTED ERR for ret is %d\n", __FUNCTION__, __LINE__, ret);
    
    }
    LOG_MAIN("Update success: cur:pos_stat: %d, lmt_index:%d, i_data: 0x%02X, q_data: 0x%02x.\n", 
        pos_stat, cfg.rpt_lmt_index, i_data, q_data);
}

static void chip_set_tia_setting_by_index(char *argv[])
{
    int ret = 0;
    TRX_CHN_ENUM    chn    = atoi(argv[2]); 
    short           lmt_index = atoi(argv[3]);
    unsigned char   i_data = strtol(argv[4], NULL, 16);
    unsigned char    q_data = strtol(argv[5], NULL, 16);   
    int             pos_stat = 0;
    rf_chip_phy_t   *phy = &g_phy_obj[g_phy_select];

    if (rx_dc_get_tia_lut_pos(phy, chn, &pos_stat))
    {
        LOG_ERROR("%s in line %d REPORTED ERR for get pos fail\n", __FUNCTION__, __LINE__);
        return;
    }

    ret = rx_dc_set_tia_lut_setting_by_idx(phy, chn, pos_stat, lmt_index, i_data, q_data);
    if (ret != 0)
    {
        LOG_MAIN("%s in line %d REPORTED ERR for ret is %d\n", __FUNCTION__, __LINE__, ret);
    
    }
    LOG_MAIN("Update success: point: pos:%d, lmt_index:%d, i_data: 0x%02X, q_data: 0x%02x.\n", pos_stat, lmt_index, 
        i_data, q_data);
}

static void chip_get_tia_setting(char *argv[])
{
    int ret = 0;
    TRX_CHN_ENUM    chn    = atoi(argv[2]);
    unsigned char   i_data = 0; 
    unsigned char   q_data = 0;    
    GCTRL_GET_RPT_T cfg = {0};
    rf_chip_phy_t   *phy = &g_phy_obj[g_phy_select];
    int             pos_stat = 0;    

    if (rx_dc_get_tia_lut_pos(phy, chn, &pos_stat))
    {
        LOG_ERROR("%s in line %d REPORTED ERR for get pos fail\n", __FUNCTION__, __LINE__);
        return;
    }

    GCTRL_GET_RPT(phy, chn, &cfg);    

    ret = rx_dc_get_tia_lut_setting_by_idx(phy, chn, pos_stat, cfg.rpt_lmt_index, &i_data, &q_data);
    if (ret != 0)
    {
        LOG_MAIN("%s in line %d REPORTED ERR for ret is %d\n", __FUNCTION__, __LINE__, ret);
    
    }
    LOG_MAIN("Result: cur:pos:%d, lmt_index:%d, i_data: 0x%02X, q_data: 0x%02x.\n", pos_stat, cfg.rpt_lmt_index, 
        i_data, q_data);
}

static void chip_get_tia_setting_by_index(char *argv[])
{
    int ret = 0;
    TRX_CHN_ENUM    chn    = atoi(argv[2]);
    short           lmt_index = atoi(argv[3]);  
    unsigned char   i_data = 0; 
    unsigned char   q_data = 0;    
    GCTRL_GET_RPT_T cfg = {0};
    rf_chip_phy_t   *phy = &g_phy_obj[g_phy_select];
    int             pos_stat = 0;    

    if (rx_dc_get_tia_lut_pos(phy, chn, &pos_stat))
    {
        LOG_ERROR("%s in line %d REPORTED ERR for get pos fail\n", __FUNCTION__, __LINE__);
        return;
    }

    ret = rx_dc_get_tia_lut_setting_by_idx(phy, chn, pos_stat, lmt_index, &i_data, &q_data);
    if (ret != 0)
    {
        LOG_MAIN("%s in line %d REPORTED ERR for ret is %d\n", __FUNCTION__, __LINE__, ret);
    
    }
    LOG_MAIN("Result: point:pos:%d, lmt_index:%d, i_data: 0x%02X, q_data: 0x%02x.\n", pos_stat, lmt_index, 
        i_data, q_data);
}


static void chip_set_bq_setting(char *argv[])
{
    int ret = 0;
    TRX_CHN_ENUM    chn    = atoi(argv[2]);
    unsigned char   i_data = strtol(argv[3], NULL, 16);
    unsigned char   q_data = strtol(argv[4], NULL, 16);    
    GCTRL_GET_RPT_T cfg = {0};
    rf_chip_phy_t   *phy = &g_phy_obj[g_phy_select];
    
    GCTRL_GET_RPT(phy, chn, &cfg);    

    ret = rx_dc_set_bq_lut_setting_by_idx(phy, chn, cfg.rpt_lmt_index, cfg.rpt_lpf_index, i_data, q_data);
    if (ret != 0)
    {
        LOG_MAIN("%s in line %d REPORTED ERR for ret is %d\n", __FUNCTION__, __LINE__, ret);
    
    }
    LOG_MAIN("Update success: cur:lmt_index:%d, lpf_index:%d, i_data: 0x%02X, q_data: 0x%02x.\n", cfg.rpt_lmt_index, 
        cfg.rpt_lpf_index, i_data, q_data);
}

static void chip_set_bq_setting_by_index(char *argv[])
{
    int ret = 0;
    TRX_CHN_ENUM    chn    = atoi(argv[2]); 
    short           lmt_index = atoi(argv[3]);
    short           lpf_index = atoi(argv[4]); 
    unsigned char   i_data = strtol(argv[5], NULL, 16);
    unsigned char   q_data = strtol(argv[6], NULL, 16);   
    rf_chip_phy_t   *phy = &g_phy_obj[g_phy_select];

    ret = rx_dc_set_bq_lut_setting_by_idx(phy, chn, lmt_index, lpf_index, i_data, q_data);
    if (ret != 0)
    {
        LOG_MAIN("%s in line %d REPORTED ERR for ret is %d\n", __FUNCTION__, __LINE__, ret);
    
    }
    LOG_MAIN("Update success: point:lmt_index:%d, lpf_index:%d, i_data: 0x%02X, q_data: 0x%02x.\n", lmt_index, 
        lpf_index, i_data, q_data);
}

static void chip_get_bq_setting(char *argv[])
{
    int ret = 0;
    TRX_CHN_ENUM    chn    = atoi(argv[2]);
    unsigned char   i_data = 0; 
    unsigned char   q_data = 0;    
    GCTRL_GET_RPT_T cfg = {0};
    rf_chip_phy_t   *phy = &g_phy_obj[g_phy_select];
    
    GCTRL_GET_RPT(phy, chn, &cfg);    

    ret = rx_dc_get_bq_lut_setting_by_idx(phy, chn, cfg.rpt_lmt_index, cfg.rpt_lpf_index, &i_data, &q_data);
    if (ret != 0)
    {
        LOG_MAIN("%s in line %d REPORTED ERR for ret is %d\n", __FUNCTION__, __LINE__, ret);
    
    }
    LOG_MAIN("Result: cur:lmt_index:%d, lpf_index:%d, i_data: 0x%02X, q_data: 0x%02x.\n", cfg.rpt_lmt_index, 
        cfg.rpt_lpf_index, i_data, q_data);
}

static void chip_get_bq_setting_by_index(char *argv[])
{
    int ret = 0;
    TRX_CHN_ENUM    chn    = atoi(argv[2]);
    short           lmt_index = atoi(argv[3]);
    short           lpf_index = atoi(argv[4]);     
    unsigned char   i_data = 0; 
    unsigned char   q_data = 0;    
    GCTRL_GET_RPT_T cfg = {0};
    rf_chip_phy_t   *phy = &g_phy_obj[g_phy_select];

    ret = rx_dc_get_bq_lut_setting_by_idx(phy, chn, lmt_index, lpf_index, &i_data, &q_data);
    if (ret != 0)
    {
        LOG_MAIN("%s in line %d REPORTED ERR for ret is %d\n", __FUNCTION__, __LINE__, ret);
    
    }
    LOG_MAIN("Result: point:lmt_index:%d, lpf_index:%d, i_data: 0x%02X, q_data: 0x%02x.\n", lmt_index, 
        lpf_index, i_data, q_data);
}


static void chip_turn_bq_setting(char *argv[])
{
    int i = 0;
    int ret = 0;
    TRX_CHN_ENUM    chn  = atoi(argv[2]);
    int             cnt  = atoi(argv[3]);
    int             turn = atoi(argv[4]);
    unsigned char   i_data = 0; 
    unsigned char   q_data = 0;      
    GCTRL_GET_RPT_T cfg  = {0};
    rf_chip_phy_t   *phy = &g_phy_obj[g_phy_select];

    GCTRL_GET_RPT(phy, chn, &cfg);    

    ret = rx_dc_get_bq_lut_setting_by_idx(phy, chn, cfg.rpt_lmt_index, cfg.rpt_lpf_index, &i_data, &q_data);
    if (ret != 0)
    {
        LOG_MAIN("%s in line %d REPORTED ERR for ret is %d\n", __FUNCTION__, __LINE__, ret);
    
    }

    LOG_MAIN("Chn:%d, cnt: %d, base i_data: 0x%02x, q_data: 0x%02x, turn %d.\n", 
        chn, cnt, i_data, q_data, turn);     

    for (i = 0; i < cnt; i++)
    {
        if (i % 2)
        {
            i_data -= turn;
            q_data -= turn;
        }
        else
        {
            i_data += turn;
            q_data += turn;
        }
        
        ret = rx_dc_set_bq_lut_setting_by_idx(phy, chn, cfg.rpt_lmt_index, cfg.rpt_lpf_index, i_data, q_data);
        if (ret != 0)
        {
            LOG_MAIN("%s in line %d REPORTED ERR for ret is %d\n", __FUNCTION__, __LINE__, ret);
        
        }
        if (i % 100 == 0 || i % 100 == 1)
        {
            LOG_MAIN("Cur:%d  chn: %d, lmt_index: %d, lpt_index: %d, i_data: 0x%02X, q_data: 0x%02x.\n", 
                i, chn, cfg.rpt_lmt_index, cfg.rpt_lpf_index, i_data, q_data);        
        }
        CHIP_UDELAY(1000);
    }
}

static void chip_x4_enable(char *argv[])
{
	unsigned char enable=0;
	enable = atoi(argv[2]);
	cmd_api_x4_enable(enable, 1);
}

static void chip_analog_init(char *argv[])
{
	unsigned char mode=0;
	mode = atoi(argv[2]);
	cmd_api_analog_init(mode);
}

static void chip_digtal_init(char *argv[])
{
	unsigned char bw;
	unsigned char dif;
	unsigned char port;
	unsigned char rate;
	unsigned char step;
	
	bw = atoi(argv[2]);
	dif = atoi(argv[3]);
	port = atoi(argv[4]);
	rate = atoi(argv[5]);
	step = atoi(argv[6]);
	cmd_api_digtal_init(dif, bw, port, rate, step);
}

static void chip_trx_lut_load(char *argv[])
{
	unsigned long long flo=0;
	unsigned char bw=0;
	
	flo = strtoull(argv[2], NULL, 10);
	bw = atoi(argv[3]);
	cmd_api_trx_lut_load(flo, bw);
}

static void chip_custom_bw_init(char *argv[])
{
	unsigned char flag=0, bw_index, dac_div, adc_div;
	unsigned long bb_sample_rate, bw=0;

	flag = atoi(argv[2]);
	bw = strtoul(argv[3], NULL, 10);
	bw_index = atoi(argv[4]);
	bb_sample_rate = strtoul(argv[5], NULL, 10);
	dac_div = atoi(argv[6]);
	adc_div = atoi(argv[7]);
	cmd_api_custom_bw_init(flag, bw, bw_index, bb_sample_rate, dac_div, adc_div);
}

static void chip_wait_init(char *argv[])
{
	cmd_api_wait_init();
}

static void chip_rf_bandwidth_set(char *argv[])
{
	unsigned char bw;
	bw = atoi(argv[2]);
	cmd_api_rf_bandwidth_set(bw);
}

static void chip_rx_port(char *argv[])
{
	unsigned char chn=0;
	unsigned char port=0;
	unsigned char gain=0;
	unsigned char en=0;
	
	chn = atoi(argv[2]);
	port = atoi(argv[3]);
	gain = atoi(argv[4]);
	en = atoi(argv[5]);
	cmd_api_rx_port(chn, port, gain, en);
}

static void chip_rx_port_man(char *argv[])
{
	unsigned char chn=0;
	unsigned char port=0;
	unsigned char gain=0;
	
	chn = atoi(argv[2]);
	port = atoi(argv[3]);
	gain = atoi(argv[4]);
	cmd_api_rx_port_man(chn, port, gain);
}

static void chip_tx_port(char *argv[])
{
	unsigned char chn=0;
	unsigned char port=0;
	unsigned char en=0;
	
	chn = atoi(argv[2]);
	port = atoi(argv[3]);
	en = atoi(argv[4]);
	cmd_api_tx_port(chn, port, en);
}

static void chip_fsm_init(char *argv[])
{
	cmd_api_fsm_init();
}

static void chip_manual_enable(char *argv[])
{
	unsigned char on=0;
	on = atoi(argv[2]);
	cmd_api_manual_enable(on);
}

static void chip_tx_atten(char *argv[])
{
	unsigned short val=0;
	short immed;

	val = atoi(argv[2]);
	immed = atoi(argv[3]);
	cmd_api_tx_atten(val, immed);
}

static void chip_tx_atten_chn(char *argv[])
{
	unsigned char chn;
	unsigned short val=0;
	short immed;

	chn = atoi(argv[2]);
	val = atoi(argv[3]);
	immed = atoi(argv[4]);
	cmd_api_tx_atten_chn(chn, val, immed);
}

static void chip_tx_dig_atten(char *argv[])
{
	unsigned char chn=0;
	short index;

	chn = atoi(argv[2]);
	index = atoi(argv[3]);
	cmd_api_tx_dig_atten(chn, index);
}

static void chip_rx_mgc_gain(char *argv[])
{
	unsigned short val=0;
	unsigned char chn=0;
	unsigned char tb=0;
	
	chn = atoi(argv[2]);
	tb = atoi(argv[3]);
	val = atoi(argv[4]);
	cmd_api_rx_mgc_gain(chn, tb, val);
}

static void chip_rx_mgc_max_gain(char *argv[])
{
	unsigned char chn=0;
	chn = atoi(argv[2]);
	cmd_api_rx_mgc_max_gain(chn);
}

//---------------------------------------------------------
//fsm control
static void chip_fdd_force_wait(char *argv[])
{
	cmd_api_fdd_force_wait();
}

static void chip_fdd_wait_to_alert(char *argv[])
{
	cmd_api_fdd_wait_to_alert();
}

static void chip_fdd_alert_to_fsm(char *argv[])
{
	cmd_api_fdd_alert_to_fsm();
}

static void chip_fdd_fsm_to_alert(char *argv[])
{
	cmd_api_fdd_fsm_to_alert();
}

//--------------------TDD-----------------
static void chip_wire_control_en(char *argv[])
{
	unsigned char en;
	unsigned char pulse;
	
	en = atoi(argv[2]);
	pulse = atoi(argv[3]);
	cmd_api_wire_control_en(en, pulse);
}

static void chip_tdd_wait_to_alert(char *argv[])
{
	cmd_api_tdd_wait_to_alert();
}

static void chip_tdd_alert_to_rx(char *argv[])
{
	cmd_api_tdd_alert_to_rx();
}

static void chip_tdd_rx_to_wait(char *argv[])
{
	cmd_api_tdd_rx_to_wait();
}

static void chip_tdd_alert_to_tx(char *argv[])
{
	cmd_api_tdd_alert_to_tx();
}

static void chip_tdd_tx_to_wait(char *argv[])
{
	cmd_api_tdd_tx_to_wait();
}
//---------------------------------------------------------

static void chip_rxlo_fsm(char *argv[])
{
	unsigned long long flo=0;
	flo = strtoull(argv[2], NULL, 10);
	cmd_api_rxlo_fsm(flo);
}

static void chip_txlo_fsm(char *argv[])
{
	unsigned long long flo=0;
	unsigned char core2_en=0;
	
	flo = strtoull(argv[2], NULL, 10);
	core2_en = atoi(argv[3]);
	cmd_api_txlo_fsm(flo, core2_en);
}

static void chip_rxlo_set(char *argv[])
{
	unsigned long long flo=0;
	unsigned char chn=0;

	chn = atoi(argv[2]);
	flo = strtoull(argv[3], NULL, 10);
	cmd_api_rxlo_set(chn, flo);
}

static void chip_txlo_set(char *argv[])
{
	unsigned long long flo=0;
	unsigned char chn=0;

	chn = atoi(argv[2]);
	flo = strtoull(argv[3], NULL, 10);
	cmd_api_txlo_set(chn, flo);
}

static void chip_rxadc_on(char *argv[])
{
	unsigned char chn=0;
	unsigned char en=0;
	
	chn = atoi(argv[2]);
	en = atoi(argv[3]);
	cmd_api_rxadc_on(chn, en);
}

static void chip_rxifbuf_on(char *argv[])
{
	unsigned char chn=0;
	unsigned char en=0;
	
	chn = atoi(argv[2]);
	en = atoi(argv[3]);
	cmd_api_rxifbuf_on(chn, en);
}

static void chip_fcal_s2_bypass(char *argv[])
{
	unsigned char en=0;
	en = atoi(argv[2]);
	cmd_api_fcal_s2_bypass(en);
}

static void chip_core2_s7_s8_s10_s11_s12_bypass(char *argv[])
{
	unsigned char en=0;
	en = atoi(argv[2]);
	cmd_api_core2_s7_s8_s10_s11_s12_bypass(en);
}


//---------------------------------------------------------
//cal
static void chip_sx_cal(char *argv[])
{
	cmd_api_sx_cal();
}

static void chip_txlo_cal(char *argv[])
{
	cmd_api_txlo_cal();
}

static void chip_txdc_offset_cal(char *argv[])
{
	unsigned char chn;
	chn = atoi(argv[2]);
	cmd_api_txdc_offset_cal(chn);
}

static void chip_rxadc_cal(char *argv[])
{
	unsigned char chn;
	chn = atoi(argv[2]);
	cmd_api_rxadc_cal(chn);
}

static void chip_txdac_cal(char *argv[])
{
	unsigned char chn;
	chn = atoi(argv[2]);
	cmd_api_txdac_cal(chn);
}

static void chip_rxdc_offset_cal(char *argv[])
{
	unsigned char chn=0;
	chn = atoi(argv[2]);
	cmd_api_rxdc_offset_cal(chn);
}

static void chip_rx_bw_cal(char *argv[])
{
	unsigned char chn;
	chn = atoi(argv[2]);
	LOG_MAIN("rx bw cal, chn:%d.\n", chn);
	g_phy_obj[g_phy_select].rx_bw_cal_flag[chn] = 0;
	rx_bw_cal(&g_phy_obj[g_phy_select], chn);
}

static void chip_rx_rssi_get(char *argv[])
{
	unsigned char chn;
	chn = atoi(argv[2]);
	LOG_MAIN("RSSI chn:%d.\n", chn);
	cmd_api_rx_rssi_get(chn);
}

static void chip_rxqec_cal(char *argv[])
{
	unsigned char chn, ext_loop;
	chn = atoi(argv[2]);
	ext_loop = atoi(argv[3]);
	cmd_api_rxqec_cal(chn, ext_loop);
}

static void chip_txqec_cal(char *argv[])
{
	unsigned char chn, ext_loop;
	int qec_dbfs, lol_dbfs;
	chn = atoi(argv[2]);
	ext_loop = atoi(argv[3]);
	qec_dbfs = atoi(argv[4]);
	lol_dbfs = atoi(argv[5]);
	cmd_api_txqec_cal(chn, ext_loop, qec_dbfs, lol_dbfs);
}

static void chip_qec_tracking_cal(char *argv[])
{
	unsigned char chn;
	int th;

	chn = atoi(argv[2]);
	th = atoi(argv[3]);
	LOG_MAIN("qec tracking, chn:%d, th:%d\n", chn, th);
	LOG_ERROR("Error!!! The program will use a function that we do not support, rx_dc_trackingaction_start, here\n");
	// qec_tracking_action_start(&g_phy_obj[g_phy_select], chn, th);
	while(1) {}
}

static void chip_rx_dc_tracking_cal(char *argv[])
{
    unsigned char chn;
    int tia_cal;
    int debug;

    chn = atoi(argv[2]);
    tia_cal = atoi(argv[3]);
    debug = atoi(argv[4]);

    LOG_MAIN("rx_dc tracking, chn:%d, tia_cal:%d, debug:%d\n", chn, tia_cal, debug);
    LOG_ERROR("Error!!! The program will use a function that we do not support, rx_dc_trackingaction_start, here\n");
	// rx_dc_tracking_action_start(&g_phy_obj[g_phy_select], chn, tia_cal, debug);
    while(1) {}
}


static void chip_tx_tone(char *argv[])
{
	unsigned char chn=0;
	unsigned char on=0;
	int freq=0;
	
	chn = atoi(argv[2]);
	on = atoi(argv[3]);
	freq  = atoi(argv[4]);
	cmd_api_tx_tone(chn, on, freq);
}

static void chip_txdc_digital_remove(char *argv[])
{
	unsigned char chn=0;
	chn = atoi(argv[2]);
	cmd_api_txdc_digital_remove(chn);
}

static void chip_error(char *argv[])
{
	cmd_api_error();
}

static void chip_intemp_get(char *argv[])
{
	cmd_api_intemp_get();
}

static void chip_lock_status(char *argv[])
{
	unsigned char dir;
	dir = atoi(argv[2]);
	cmd_api_lock_status(dir);
}

static void chip_print_config(char *argv[])
{
	unsigned char sel=0;
	sel = atoi(argv[2]);
	cmd_api_print_config(sel);
}

static void chip_rpt_get(char *argv[])
{
	unsigned char chn;
	chn = atoi(argv[2]);
	cmd_api_rpt_get(chn);
}

static void gctrl_set_mode(rf_chip_phy_t *phy, TRX_CHN_ENUM chn, short gain_mode, short table_mode)
{
	GCTRL_MGC_CFGG_T mgc_cfg;
	GCTRL_BASIC_CFG_T basic_cfg;
	
	basic_cfg.cfg_gain_table_mode = table_mode; // 0 fulltable | 1 split table
	basic_cfg.cfg_agc_mode = gain_mode; // 0 mgc  | 1 slow | 2 fast
	basic_cfg.cfg_pin_pls_gain_change_sel = 0;

	// MGC default SPI mode
	// Only for mgc
	mgc_cfg.cfg_mgc_mode = 0,
	mgc_cfg.cfg_mgc_adj_gain_pos_auto = 0; 
	mgc_cfg.cfg_mgc_adj_gain_pos_lmt = 0;
	//mgc_cfg.spi_mgc_force_enable = 0; 
	mgc_cfg.cfg_mgc_inc_step = 0;
	mgc_cfg.cfg_mgc_dec_step = 0;
	//mgc_cfg.cfg_mgc_res = 0;

	GCTRL_BASIC_CFG(phy, chn, &basic_cfg);

	if (basic_cfg.cfg_agc_mode == 0) {
		GCTRL_MGC_CFGG(phy, chn, &mgc_cfg);
		LOG_MAIN("mgc_mode=%d,mgc_adj_gain_pos_auto=%d, mgc_adj_gain_pos_lmt=%d\n", 
			mgc_cfg.cfg_mgc_mode, mgc_cfg.cfg_mgc_adj_gain_pos_auto, 
			mgc_cfg.cfg_mgc_adj_gain_pos_lmt);
	}
}

static void chip_agc_mode_set(char *argv[])
{
	unsigned char chn;
	short gain_mode;
	short tab_mode;

	chn = atoi(argv[2]);
	tab_mode = atoi(argv[3]);
	gain_mode = atoi(argv[4]);
	cmd_api_agc_mode_set(chn, gain_mode, tab_mode);
}

static void chip_adc_ram_dump(char *argv[])
{
	unsigned char chn;
	double adc_fs;
	chn = atoi(argv[2]);
	cmd_api_adc_ram_dump(chn, argv[3]);
}

static void  gctrl_full_tab_default_init(rf_chip_phy_t *phy, TRX_CHN_ENUM chn, short init_index, short max_index)
{
	int lpf_index;
	int lmt_index;
	GCTRL_FULL_TABLE_CFG_T tab;
	int lna_index = 0;
	int lpf_max_index = 5;
	short addr = 0;

	tab.init_index = init_index;
	tab.max_index = max_index;

	for(lmt_index = 0; lmt_index <= 5; lmt_index++) {
		if(lmt_index == 5) {
			lpf_max_index = 12;	
		}
		for(lpf_index = 0; lpf_index <= lpf_max_index; lpf_index++) {
			tab.content[addr] = ((lna_index << 7) | (lmt_index << 4) | lpf_index);
			addr++;
		}
	}
	GCTRL_FULL_TABLE_CFG(phy, chn,  &tab);
}

static void chip_gain_full_tab__default_init(char *argv[])
{
	int addr;
	unsigned char chn;
	short init_index;
	short max_index;

	chn = atoi(argv[2]);
	init_index = atoi(argv[3]);
	max_index = atoi(argv[4]);
	LOG_MAIN("chn:%d, tab_init_inx:%d, tab_max_inx:%d\r\n", chn, init_index, max_index);
	gctrl_full_tab_default_init(&g_phy_obj[g_phy_select], chn, init_index, max_index);

	LOG_MAIN("Dump full tab start\r\n");
	for(addr = 0; addr <= max_index; addr++) {
		short gain_inx;	
		short ex_lna;
		short lmt_inx;
		short lpf_inx;
		GCTRL_FULL_TABLE_RPT_CONTENT(&g_phy_obj[g_phy_select], chn, addr, &gain_inx);
		ex_lna = ((gain_inx & 0x80) >> 7);
		lmt_inx = ((gain_inx & 0x70) >> 4);
		lpf_inx = (gain_inx & 0x0f);
		LOG_MAIN("addr:%d, ex_lna:%d, lmt_inx:%d, lpf_inx:%d\r\n", addr, ex_lna, lmt_inx, lpf_inx);
	}
	LOG_MAIN("Dump full tab end\r\n");
}

static void chip_rx_gain_full_tab_init(char *argv[])
{
	unsigned char chn;
	short init_index;
	short max_index;

	chn = atoi(argv[2]);
	init_index = atoi(argv[3]);
	max_index = atoi(argv[4]);
	cmd_api_rx_gain_full_tab_init(chn, init_index, max_index);
}

static void chip_rx_gain_mgc_full_tab_index_set(char *argv[])
{
	unsigned char chn;
	int full_tab_inx;

	chn = atoi(argv[2]);
	full_tab_inx = atoi(argv[3]);
	cmd_api_rx_gain_mgc_full_tab_index_set(chn, full_tab_inx);
}

static void chip_tx_qec_gain_set(char *argv[])
{
    unsigned char chn;
    short gain;
    chn = atoi(argv[2]);
    gain = atoi(argv[3]);
    cmd_api_tx_qec_gain_set(chn, gain);
}

static void chip_tx_splite_gain_set(char *argv[])
{
    unsigned char chn,type;
    unsigned int val;
    chn = atoi(argv[2]);
    type = atoi(argv[3]);
    val = atoi(argv[4]);
    LOG_MAIN("tx split atten change, chn=%d, type:%d, index=%d\n", chn, type, val);
	tx_split_atten_change(&g_phy_obj[g_phy_select], chn, type, val, 1);
}

static void chip_rx_dc_tracking_onoff(char *argv[])
{
    unsigned char chn,start;
    chn = atoi(argv[2]);
    start = atoi(argv[3]);
    rf_chip_phy_t *phy=&g_phy_obj[g_phy_select];
    phy->module_debug |=TRX_QEC_CAL;
    if(start)
    {
        phy->rxdc_track_thread[chn].tia=1;
        phy->rxdc_track_thread[chn].hold_cnt=0;
        phy->rxdc_track_thread[chn].track_cnt =0;
        phy->rxdc_track_thread[chn].thread.need_exit=0;
#if LINUX_OS
        (chn == TRX_CHN1) ?  rx1_dc_tracking_task(phy): rx2_dc_tracking_task(phy) ;
#endif
    }
    else{
        phy->rxdc_track_thread[chn].thread.need_exit=1;
        phy->rxdc_track_thread[chn].hold_cnt=0;
        phy->rxdc_track_thread[chn].track_cnt=0; 
    }
}


#define       CMD_VERSION              "version"
#define       CMD_CHIP_SEL             "chip_sel"
#define       CMD_FPGA_RESET           "fpga_reset"
#define       CMD_PRINT_ON             "print_on"
#define       CMD_MODULE_DEBUG         "module_debug"
#define       CMD_W                    "w"
#define       CMD_R                    "r"
#define       CMD_RFILE                "rfile"
#define       CMD_FW                   "fw"
#define       CMD_FR                   "fr"
#define       CMD_POWER_INIT           "power_init"
#define       CMD_RCAL                 "rcal"
#define       CMD_SYS_CLOCK            "sys_clock"
#define       CMD_LUT_LOAD             "lut_load"
#define       CMD_LUT_READ_WORD        "rlut"
#define       CMD_LUT_READ_FILE        "rlutfile"
#define       CMD_X4_ENABLE            "x4_enable"
#define       CMD_ANALOG_INIT          "analog_init"
#define       CMD_DIGTAL_INIT          "digtal_init"
#define       CMD_LUT_TO_REGS          "lut_to_regs"
#define       CMD_CUSTOM_BW            "custom_bw"
#define       CMD_WAIT_INIT            "wait_init"
#define       CMD_RF_BANDWIDTH         "rf_bandwidth"
#define       CMD_RX_PORT              "rx_port"
#define       CMD_RX_PORT_MAN          "rx_port_man"
#define       CMD_TX_PORT              "tx_port"
#define       CMD_FSM_INIT             "fsm_init"
#define       CMD_MANUAL_ON            "manual_on"
#define       CMD_TX_ATTEN             "tx_atten"
#define       CMD_TX_ATTEN_CHN         "tx_atten_chn"
#define       CMD_TX_ATTEN_DIG         "tx_atten_dig"
#define       CMD_RX_MGC_GAIN          "rx_mgc_gain"
#define       CMD_RX_MGC_MAX_GAIN      "rx_mgc_max_gain"
#define       CMD_FDD_FORCE_WAIT       "fdd_force_wait"
#define       CMD_FDD_WAIT_TO_ALERT    "fdd_wait_to_alert"
#define       CMD_FDD_ALERT_TO_FSM     "fdd_alert_to_fsm"
#define       CMD_FDD_FSM_TO_ALERT     "fdd_fsm_to_alert"
#define       CMD_WIRE_CONTROL_EN      "wire_control_en"
#define       CMD_TDD_WAIT_TO_ALERT    "tdd_wait_to_alert"
#define       CMD_TDD_ALERT_TO_RX      "tdd_alert_to_rx"
#define       CMD_TDD_RX_TO_WAIT       "tdd_rx_to_wait"
#define       CMD_TDD_ALERT_TO_TX      "tdd_alert_to_tx"
#define       CMD_TDD_TX_TO_WAIT       "tdd_tx_to_wait"
#define       CMD_RXLO                 "rxlo"
#define       CMD_TXLO                 "txlo"
#define       CMD_TRXLO_CHANGE         "trxlo_change"
#define       CMD_RXLO_FSM             "rxlo_fsm"
#define       CMD_TXLO_FSM             "txlo_fsm"
#define       CMD_RXADC                "rxadc"
#define       CMD_RXIFBUF              "rxifbuf"
#define       CMD_FCAL_S2_BYPASS       "fcal_s2_bypass"
#define       CMD_CORE2_S7_S8_S10_S11_S12_BYPASS      "core2_s7_s8_s10_s11_s12_bypass"
#define       CMD_SX_CAL               "sx_cal"
#define       CMD_TXLO_CAL             "txlo_cal"
#define       CMD_TXDC_OFFSET_CAL      "txdc_offset_cal"
#define       CMD_RXADC_CAL            "rxadc_cal"
#define       CMD_TXDAC_CAL            "txdac_cal"
#define       CMD_RXDC_OFFSET_CAL      "rxdc_offset_cal"
#define       CMD_RX_BW_CAL            "rx_bw_cal"
#define       CMD_RX_RSSI              "rx_rssi"
#define       CMD_RXQEC_CAL            "rxqec_cal"
#define       CMD_TXQEC_CAL            "txqec_cal"
#define       CMD_QEC_TRACKING         "qec_tracking"
#define       CMD_TXQEC_GAIN           "tx_qec_gain"
#define       CMD_RX_DC_TRACKING       "rxdc_tracking"
#define       CMD_TX_TONE              "tx_tone"
#define       CMD_TXDC_REMOVE          "txdc_remove"
#define       CMD_ERROR                "error"
#define       CMD_PRINT_CONFIG         "print_config"
#define       CMD_CHIP_TEMP            "chip_temp"
#define       CMD_LOCK_STATUS          "lock_status"
#define       CMD_ADC_RAM_DUMP         "adc_ram_dump"
#define       CMD_AGC_RPT              "agc_rpt"
#define       CMD_AGC_MODE             "agc_mode"
#define       CMD_FULL_TABLE_INIT      "full_table_init"
#define       CMD_FULL_TAB_INDEX       "full_tab_index"
#define       CMD_PRINT_CMD            "print_cmd"
#define       CMD_DUMP_DCI_DCQ         "dciqfile"
#define       CMD_GET_DCI_DCQ          "dciq"
#define       CMD_SET_TIA_SETTING      "settia"
#define       CMD_GET_TIA_SETTING      "gettia"
#define       CMD_SET_TIA_SETTING_BY_IDX   "settiaidx"
#define       CMD_GET_TIA_SETTING_BY_IDX   "gettiaidx"
#define       CMD_SET_BQ_SETTING       "setbq"
#define       CMD_GET_BQ_SETTING       "getbq"
#define       CMD_SET_BQ_SETTING_BY_IDX   "setbqidx"
#define       CMD_GET_BQ_SETTING_BY_IDX   "getbqidx"
#define       CMD_TURN_BQ_SETTING      "turnbq"
#define       CMD_TX_SP_GAIN           "tx_sp_gain"
#define       CMD_RX_DC_TRACKING_ONOFF "rxdc_tracking_onoff"


static char g_cmd_str[][50] = 
{
	CMD_VERSION,
	CMD_CHIP_SEL,
	CMD_FPGA_RESET,
	CMD_PRINT_ON,
	CMD_MODULE_DEBUG,
	CMD_W,
	CMD_R,
	CMD_FW,
	CMD_FR,
	CMD_POWER_INIT,
	CMD_RCAL,
	CMD_SYS_CLOCK,
	CMD_LUT_LOAD,
	CMD_LUT_READ_WORD,
	CMD_X4_ENABLE,
	CMD_ANALOG_INIT,
	CMD_DIGTAL_INIT,
	CMD_LUT_TO_REGS,
	CMD_CUSTOM_BW,
	CMD_WAIT_INIT,
	CMD_RF_BANDWIDTH,
	CMD_RX_PORT,
	CMD_RX_PORT_MAN,
	CMD_TX_PORT,
	CMD_FSM_INIT,
	CMD_MANUAL_ON,
	CMD_TX_ATTEN,
	CMD_TX_ATTEN_CHN,
	CMD_TX_ATTEN_DIG,
	CMD_RX_MGC_GAIN,
	CMD_RX_MGC_MAX_GAIN,
	CMD_FDD_FORCE_WAIT,
	CMD_FDD_WAIT_TO_ALERT,
	CMD_FDD_ALERT_TO_FSM,
	CMD_FDD_FSM_TO_ALERT,
	CMD_WIRE_CONTROL_EN,
	CMD_TDD_WAIT_TO_ALERT,
	CMD_TDD_ALERT_TO_RX,
	CMD_TDD_RX_TO_WAIT,
	CMD_TDD_ALERT_TO_TX,
	CMD_TDD_TX_TO_WAIT,
	CMD_RXLO,
	CMD_TXLO,
	CMD_TRXLO_CHANGE,
	CMD_RXLO_FSM,
	CMD_TXLO_FSM,
	CMD_RXADC,
	CMD_RXIFBUF,
	CMD_FCAL_S2_BYPASS,
	CMD_CORE2_S7_S8_S10_S11_S12_BYPASS,
	CMD_SX_CAL,
	CMD_TXLO_CAL,
	CMD_TXDC_OFFSET_CAL,
	CMD_RXADC_CAL,
	CMD_TXDAC_CAL,
	CMD_RXDC_OFFSET_CAL,
	CMD_RX_BW_CAL,
	CMD_RX_RSSI,
	CMD_RXQEC_CAL,
	CMD_TXQEC_CAL,
	CMD_QEC_TRACKING,
	CMD_RX_DC_TRACKING,
	CMD_TX_TONE,
	CMD_TXDC_REMOVE,
	CMD_ERROR,
	CMD_PRINT_CONFIG,
	CMD_CHIP_TEMP,
	CMD_LOCK_STATUS,
	CMD_ADC_RAM_DUMP,
	CMD_AGC_RPT,
	CMD_AGC_MODE,
	CMD_FULL_TABLE_INIT,
	CMD_FULL_TAB_INDEX,
	CMD_PRINT_CMD,
    CMD_DUMP_DCI_DCQ,
    CMD_GET_DCI_DCQ,
    CMD_SET_TIA_SETTING,
    CMD_GET_TIA_SETTING,
    CMD_SET_TIA_SETTING_BY_IDX,
    CMD_GET_TIA_SETTING_BY_IDX,
    CMD_SET_BQ_SETTING,
    CMD_GET_BQ_SETTING,
    CMD_SET_BQ_SETTING_BY_IDX,
    CMD_GET_BQ_SETTING_BY_IDX,
    CMD_TURN_BQ_SETTING,
    CMD_TXQEC_GAIN,
    CMD_TX_SP_GAIN,
    CMD_RX_DC_TRACKING_ONOFF,
};

static void chip_print_cmd(char *argv[])
{
	cmd_api_print_cmd();
}

static chip_cmd_t g_full_cmds[] = 
{
	{CMD_VERSION,            0,    &device_version},
	{CMD_CHIP_SEL,           1,    &chip_select},
	{CMD_FPGA_RESET,         0,    &fpga_reset},
	{CMD_PRINT_ON,           1,    &chip_debug_on},
	{CMD_MODULE_DEBUG,       1,    &chip_module_debug_on},
	{CMD_W,                  2,    &chip_write_reg},
	{CMD_R,                  1,    &chip_read_reg},
	{CMD_RFILE,              1,    &chip_read_reg_file},
	{CMD_FW,                 2,    &chip_fwrite_reg},
	{CMD_FR,                 1,    &chip_fread_reg},
	{CMD_POWER_INIT,         0,    &chip_power_init},
	{CMD_RCAL,               1,    &chip_rcal},
	{CMD_SYS_CLOCK,          2,    &chip_sys_clock_init},
	{CMD_LUT_LOAD,           3,    &chip_lut_load},
	{CMD_LUT_READ_WORD,      2,    &chip_lut_readword},
	{CMD_LUT_READ_FILE,      2,    &chip_lut_readfile},
    {CMD_DUMP_DCI_DCQ,       2,    &chip_dciq_dumpfile},
    {CMD_GET_DCI_DCQ,        2,    &chip_get_dciq},
    {CMD_SET_TIA_SETTING,     3,   &chip_set_tia_setting},
    {CMD_SET_TIA_SETTING_BY_IDX,    4,    &chip_set_tia_setting_by_index},
    {CMD_GET_TIA_SETTING,     1,    &chip_get_tia_setting},
    {CMD_GET_TIA_SETTING_BY_IDX,    2,    &chip_get_tia_setting_by_index},
    {CMD_SET_BQ_SETTING,     3,    &chip_set_bq_setting},
    {CMD_SET_BQ_SETTING_BY_IDX,     5,    &chip_set_bq_setting_by_index},
    {CMD_GET_BQ_SETTING,     1,    &chip_get_bq_setting},
    {CMD_GET_BQ_SETTING_BY_IDX,     3,    &chip_get_bq_setting_by_index},
    {CMD_TURN_BQ_SETTING,    3,    &chip_turn_bq_setting},
	{CMD_X4_ENABLE,          1,    &chip_x4_enable},
	{CMD_ANALOG_INIT,        0,    &chip_analog_init},
	{CMD_DIGTAL_INIT,        5,    &chip_digtal_init},
	{CMD_LUT_TO_REGS,        2,    &chip_trx_lut_load},
	{CMD_CUSTOM_BW,          6,    &chip_custom_bw_init},
	{CMD_WAIT_INIT,          0,    &chip_wait_init},
	{CMD_RF_BANDWIDTH,       1,    &chip_rf_bandwidth_set},
	{CMD_RX_PORT,            4,    &chip_rx_port},
	{CMD_RX_PORT_MAN,        3,    &chip_rx_port_man},
	{CMD_TX_PORT,            3,    &chip_tx_port},
	{CMD_FSM_INIT,           0,    &chip_fsm_init},
	{CMD_MANUAL_ON,          1,    &chip_manual_enable},
	{CMD_TX_ATTEN,		     2,	 &chip_tx_atten},
	{CMD_TX_ATTEN_CHN,	     3,	 &chip_tx_atten_chn},
	{CMD_TX_ATTEN_DIG,       2,    &chip_tx_dig_atten},
	{CMD_RX_MGC_GAIN, 	     3,	 &chip_rx_mgc_gain},
	{CMD_RX_MGC_MAX_GAIN,    1,    &chip_rx_mgc_max_gain},
	{CMD_FDD_FORCE_WAIT,     0,    &chip_fdd_force_wait},
	{CMD_FDD_WAIT_TO_ALERT,  0,    &chip_fdd_wait_to_alert},
	{CMD_FDD_ALERT_TO_FSM,   0,    &chip_fdd_alert_to_fsm},
	{CMD_FDD_FSM_TO_ALERT,   0,    &chip_fdd_fsm_to_alert},
	{CMD_WIRE_CONTROL_EN,    2,    &chip_wire_control_en},
	{CMD_TDD_WAIT_TO_ALERT,  0,    &chip_tdd_wait_to_alert},
	{CMD_TDD_ALERT_TO_RX,    0,    &chip_tdd_alert_to_rx},
	{CMD_TDD_RX_TO_WAIT,     0,    &chip_tdd_rx_to_wait},
	{CMD_TDD_ALERT_TO_TX,    0,    &chip_tdd_alert_to_tx},
	{CMD_TDD_TX_TO_WAIT,     0,    &chip_tdd_tx_to_wait},
	{CMD_RXLO,		         2,	 &chip_rxlo_set},
	{CMD_TXLO,		         2,	 &chip_txlo_set},
	{CMD_TRXLO_CHANGE,	     2,	 &chip_set_trx_flo},
	{CMD_RXLO_FSM,		     1,	 &chip_rxlo_fsm},
	{CMD_TXLO_FSM,		     2,	 &chip_txlo_fsm},
	{CMD_RXADC,   		     2,	 &chip_rxadc_on},
	{CMD_RXIFBUF,		     2,	 &chip_rxifbuf_on},
	{CMD_FCAL_S2_BYPASS,     1,    &chip_fcal_s2_bypass},
	{CMD_CORE2_S7_S8_S10_S11_S12_BYPASS, 1, &chip_core2_s7_s8_s10_s11_s12_bypass},
	{CMD_SX_CAL,             0,    &chip_sx_cal},
	{CMD_TXLO_CAL,           0,    &chip_txlo_cal},
	{CMD_TXDC_OFFSET_CAL,    1,    &chip_txdc_offset_cal},
	{CMD_RXADC_CAL,          1,    &chip_rxadc_cal},
	{CMD_TXDAC_CAL,          1,    &chip_txdac_cal},
	{CMD_RXDC_OFFSET_CAL,    1,    &chip_rxdc_offset_cal},
	{CMD_RX_BW_CAL,          1,    &chip_rx_bw_cal},
	{CMD_RX_RSSI,            1,    &chip_rx_rssi_get},
	{CMD_RXQEC_CAL,          2,    &chip_rxqec_cal},
	{CMD_TXQEC_CAL,          4,    &chip_txqec_cal},
	{CMD_QEC_TRACKING,       2,    &chip_qec_tracking_cal},
    {CMD_RX_DC_TRACKING,     3,    &chip_rx_dc_tracking_cal},
	{CMD_TX_TONE,            3,    &chip_tx_tone},
	{CMD_TXDC_REMOVE,        1,    &chip_txdc_digital_remove},
	{CMD_ERROR,              0,    &chip_error},
	{CMD_PRINT_CONFIG,       1,    &chip_print_config},
	{CMD_CHIP_TEMP,          0,    &chip_intemp_get},
	{CMD_LOCK_STATUS,        1,    &chip_lock_status},
	{CMD_ADC_RAM_DUMP,       2,    &chip_adc_ram_dump},
	{CMD_AGC_RPT,            1,    &chip_rpt_get},
	{CMD_AGC_MODE,           3,    &chip_agc_mode_set},
	{CMD_FULL_TABLE_INIT,    3,    &chip_rx_gain_full_tab_init},
	{CMD_FULL_TAB_INDEX,     2,    &chip_rx_gain_mgc_full_tab_index_set},
	{CMD_PRINT_CMD,          0,    &chip_print_cmd},
    {CMD_TXQEC_GAIN,         2,    &chip_tx_qec_gain_set},
    {CMD_TX_SP_GAIN,         3,    &chip_tx_splite_gain_set},
    {CMD_RX_DC_TRACKING_ONOFF,2,  &chip_rx_dc_tracking_onoff},
};

/*----------------------------------------------------------------------------------------------*/
// iio API
/*----------------------------------------------------------------------------------------------*/
static int read_config_from_file(void);
static int write_config_to_file(void);

void CUSTOMER_CMD_API_chip_config_set(chip_config_t *config)
{
	g_phy_obj[g_phy_select].config->mode = config->mode;
	g_phy_obj[g_phy_select].config->dig_if = config->dig_if;
	g_phy_obj[g_phy_select].config->p0p1_port = config->p0p1_port;
	g_phy_obj[g_phy_select].config->data_rate = config->data_rate;
	g_phy_obj[g_phy_select].config->tx_port[0] = config->tx_port[0];
	g_phy_obj[g_phy_select].config->tx_port[1] = config->tx_port[1];
	g_phy_obj[g_phy_select].config->rx_port[0] = config->rx_port[0];
	g_phy_obj[g_phy_select].config->rx_port[1] = config->rx_port[1];
	g_phy_obj[g_phy_select].config->bandwith = config->bandwith;
	g_phy_obj[g_phy_select].config->lo_change_mode = config->lo_change_mode;
	g_phy_obj[g_phy_select].config->wire_ctrl = config->wire_ctrl;
	g_phy_obj[g_phy_select].config->gain_table_mode = config->gain_table_mode;
	g_phy_obj[g_phy_select].config->gain_ctrl_mode = config->gain_ctrl_mode;
	g_phy_obj[g_phy_select].config->x4_enable = config->x4_enable;
	g_phy_obj[g_phy_select].config->fast_sx_lock = config->fast_sx_lock;
	g_phy_obj[g_phy_select].config->core2_enable = config->core2_enable;
	g_phy_obj[g_phy_select].config->tx_atten_chn_flag = config->tx_atten_chn_flag;
	g_phy_obj[g_phy_select].config->rx_bw_cal_flag = config->rx_bw_cal_flag;
	g_phy_obj[g_phy_select].config->rx_dc_cal_flag = config->rx_dc_cal_flag;
	g_phy_obj[g_phy_select].config->rx_qec_flag = config->rx_qec_flag;
	g_phy_obj[g_phy_select].config->tx_dc_cal_flag = config->tx_dc_cal_flag;
	g_phy_obj[g_phy_select].config->tx_dac_cal_flag = config->tx_dac_cal_flag;
	g_phy_obj[g_phy_select].config->tx_qec_flag = config->tx_qec_flag;
	g_phy_obj[g_phy_select].config->sx_cal_flag = config->sx_cal_flag;
	g_phy_obj[g_phy_select].config->txlo_cal_flag = config->txlo_cal_flag;
	g_phy_obj[g_phy_select].config->wire_control_en = config->wire_control_en;
	g_phy_obj[g_phy_select].config->gain_ctrl_pin_flag = config->gain_ctrl_pin_flag;
	g_phy_obj[g_phy_select].config->bandwidthswitch_flag = config->bandwidthswitch_flag;
	g_phy_obj[g_phy_select].config->custom_bandwith_flag = config->custom_bandwith_flag;
	g_phy_obj[g_phy_select].config->rx_ext_loop = config->rx_ext_loop;
	g_phy_obj[g_phy_select].config->tx_ext_loop = config->tx_ext_loop;
	g_phy_obj[g_phy_select].config->qec_dbfs = config->qec_dbfs;
	g_phy_obj[g_phy_select].config->lol_dbfs = config->lol_dbfs;
	g_phy_obj[g_phy_select].config->rx_flo = config->rx_flo;
	g_phy_obj[g_phy_select].config->tx_flo = config->tx_flo;
	g_phy_obj[g_phy_select].config->sys_fvco = config->sys_fvco;
	g_phy_obj[g_phy_select].config->xtal_freq = config->xtal_freq;
	g_phy_obj[g_phy_select].config->custom_bandwith = config->custom_bandwith;
	g_phy_obj[g_phy_select].config->bb_sample_rate = config->bb_sample_rate;
	g_phy_obj[g_phy_select].config->dac_syspll_lo_div = config->dac_syspll_lo_div;
	g_phy_obj[g_phy_select].config->adc_syspll_lo_div = config->adc_syspll_lo_div;
	g_phy_obj[g_phy_select].config->use_bybrid_mode = config->use_bybrid_mode;
	g_phy_obj[g_phy_select].config->hybrid_mode = config->hybrid_mode;
	g_phy_obj[g_phy_select].config->ldo_cal_flag = config->ldo_cal_flag;
	g_phy_obj[g_phy_select].config->sx_vco_ldo = config->sx_vco_ldo;
	g_phy_obj[g_phy_select].config->sxlf_ldo = config->sxlf_ldo;
	g_phy_obj[g_phy_select].config->syspll_ldo = config->syspll_ldo;
	g_phy_obj[g_phy_select].config->txabb_ldo = config->txabb_ldo;
	g_phy_obj[g_phy_select].config->txdac_ldo = config->txdac_ldo;
	g_phy_obj[g_phy_select].config->txfe_ldo = config->txfe_ldo;
	g_phy_obj[g_phy_select].config->txsx_lo_ldo = config->txsx_lo_ldo;
	g_phy_obj[g_phy_select].config->rxadc_ldo = config->rxadc_ldo;
	g_phy_obj[g_phy_select].config->rxfe_ldo = config->rxfe_ldo;
	g_phy_obj[g_phy_select].config->rxsx_lo_ldo = config->rxsx_lo_ldo;
	g_phy_obj[g_phy_select].config->mdig_ldo = config->mdig_ldo;
	g_phy_obj[g_phy_select].config->RxDc_Offset_Ver = config->RxDc_Offset_Ver;
	g_phy_obj[g_phy_select].config->Rx_ImBalance_cal_flag = config->Rx_ImBalance_cal_flag;
	g_phy_obj[g_phy_select].config->lo_leakage_cal_flag = config->lo_leakage_cal_flag;
	g_phy_obj[g_phy_select].config->reg901_val = config->reg901_val;
	g_phy_obj[g_phy_select].config->reg639_620_bit1 = config->reg639_620_bit1;
	g_phy_obj[g_phy_select].config->reg61A_val = config->reg61A_val;
	g_phy_obj[g_phy_select].config->reg600_val = config->reg600_val;
	g_phy_obj[g_phy_select].config->reg602_val = config->reg602_val;
	g_phy_obj[g_phy_select].config->vol_low_limit = config->vol_low_limit;
	g_phy_obj[g_phy_select].config->vol_up_limit = config->vol_up_limit;
	g_phy_obj[g_phy_select].config->vol_margin= config->vol_margin;
	g_phy_obj[g_phy_select].config->trx_lo_cal_mode = config->trx_lo_cal_mode;
	g_phy_obj[g_phy_select].config->fvco_min = config->fvco_min;
	g_phy_obj[g_phy_select].config->r_cal_flag = config->r_cal_flag;
	g_phy_obj[g_phy_select].config->imb_rx_cfg[0][0] = config->imb_rx_cfg[0][0];
	g_phy_obj[g_phy_select].config->imb_rx_cfg[0][1] = config->imb_rx_cfg[0][1];
	g_phy_obj[g_phy_select].config->imb_rx_cfg[1][0] = config->imb_rx_cfg[1][0];
	g_phy_obj[g_phy_select].config->imb_rx_cfg[1][1] = config->imb_rx_cfg[1][1];
	g_phy_obj[g_phy_select].config->lvds_cal_flag = config->lvds_cal_flag;
	g_phy_obj[g_phy_select].config->syspll_cfg_flag = config->syspll_cfg_flag;
	cmd_api_print_config(g_phy_select);
}

void CUSTOMER_CMD_API_chip_config_read(chip_config_t *config)
{
	config->mode = g_phy_obj[g_phy_select].config->mode;
	config->dig_if = g_phy_obj[g_phy_select].config->dig_if;
	config->p0p1_port = g_phy_obj[g_phy_select].config->p0p1_port;
	config->data_rate = g_phy_obj[g_phy_select].config->data_rate;
	config->tx_port[0] = g_phy_obj[g_phy_select].config->tx_port[0];
	config->tx_port[1] = g_phy_obj[g_phy_select].config->tx_port[1];
	config->rx_port[0] = g_phy_obj[g_phy_select].config->rx_port[0];
	config->rx_port[1] = g_phy_obj[g_phy_select].config->rx_port[1];
	config->bandwith = g_phy_obj[g_phy_select].config->bandwith;
	config->lo_change_mode = g_phy_obj[g_phy_select].config->lo_change_mode;
	config->wire_ctrl = g_phy_obj[g_phy_select].config->wire_ctrl;
	config->gain_table_mode = g_phy_obj[g_phy_select].config->gain_table_mode;
	config->gain_ctrl_mode = g_phy_obj[g_phy_select].config->gain_ctrl_mode;
	config->x4_enable = g_phy_obj[g_phy_select].config->x4_enable;
	config->fast_sx_lock = g_phy_obj[g_phy_select].config->fast_sx_lock;
	config->core2_enable = g_phy_obj[g_phy_select].config->core2_enable;
	config->tx_atten_chn_flag = g_phy_obj[g_phy_select].config->tx_atten_chn_flag;
	config->rx_bw_cal_flag = g_phy_obj[g_phy_select].config->rx_bw_cal_flag;
	config->rx_dc_cal_flag = g_phy_obj[g_phy_select].config->rx_dc_cal_flag;
	config->rx_qec_flag = g_phy_obj[g_phy_select].config->rx_qec_flag;
	config->tx_dc_cal_flag = g_phy_obj[g_phy_select].config->tx_dc_cal_flag;
	config->tx_dac_cal_flag = g_phy_obj[g_phy_select].config->tx_dac_cal_flag;
	config->tx_qec_flag = g_phy_obj[g_phy_select].config->tx_qec_flag;
	config->sx_cal_flag = g_phy_obj[g_phy_select].config->sx_cal_flag;
	config->txlo_cal_flag = g_phy_obj[g_phy_select].config->txlo_cal_flag;
	config->wire_control_en = g_phy_obj[g_phy_select].config->wire_control_en;
	config->gain_ctrl_pin_flag = g_phy_obj[g_phy_select].config->gain_ctrl_pin_flag;
	config->bandwidthswitch_flag = g_phy_obj[g_phy_select].config->bandwidthswitch_flag;
	config->custom_bandwith_flag = g_phy_obj[g_phy_select].config->custom_bandwith_flag;
	config->rx_ext_loop = g_phy_obj[g_phy_select].config->rx_ext_loop;
	config->tx_ext_loop = g_phy_obj[g_phy_select].config->tx_ext_loop;
	config->qec_dbfs = g_phy_obj[g_phy_select].config->qec_dbfs;
	config->lol_dbfs = g_phy_obj[g_phy_select].config->lol_dbfs;
	config->rx_flo = g_phy_obj[g_phy_select].config->rx_flo;
	config->tx_flo = g_phy_obj[g_phy_select].config->tx_flo;
	config->sys_fvco = g_phy_obj[g_phy_select].config->sys_fvco;
	config->xtal_freq = g_phy_obj[g_phy_select].config->xtal_freq;
	config->custom_bandwith = g_phy_obj[g_phy_select].config->custom_bandwith;
	config->bb_sample_rate = g_phy_obj[g_phy_select].config->bb_sample_rate;
	config->dac_syspll_lo_div = g_phy_obj[g_phy_select].config->dac_syspll_lo_div;
	config->adc_syspll_lo_div = g_phy_obj[g_phy_select].config->adc_syspll_lo_div;
	config->use_bybrid_mode = g_phy_obj[g_phy_select].config->use_bybrid_mode;
	config->hybrid_mode = g_phy_obj[g_phy_select].config->hybrid_mode;
	config->ldo_cal_flag = g_phy_obj[g_phy_select].config->ldo_cal_flag;
	config->sx_vco_ldo = g_phy_obj[g_phy_select].config->sx_vco_ldo;
	config->sxlf_ldo = g_phy_obj[g_phy_select].config->sxlf_ldo;
	config->syspll_ldo = g_phy_obj[g_phy_select].config->syspll_ldo;
	config->txabb_ldo = g_phy_obj[g_phy_select].config->txabb_ldo;
	config->txdac_ldo = g_phy_obj[g_phy_select].config->txdac_ldo;
	config->txfe_ldo = g_phy_obj[g_phy_select].config->txfe_ldo;
	config->txsx_lo_ldo = g_phy_obj[g_phy_select].config->txsx_lo_ldo;
	config->rxadc_ldo = g_phy_obj[g_phy_select].config->rxadc_ldo;
	config->rxfe_ldo = g_phy_obj[g_phy_select].config->rxfe_ldo;
	config->rxsx_lo_ldo = g_phy_obj[g_phy_select].config->rxsx_lo_ldo;
	config->mdig_ldo = g_phy_obj[g_phy_select].config->mdig_ldo;
	config->RxDc_Offset_Ver = g_phy_obj[g_phy_select].config->RxDc_Offset_Ver;
	config->Rx_ImBalance_cal_flag = g_phy_obj[g_phy_select].config->Rx_ImBalance_cal_flag;
	config->lo_leakage_cal_flag = g_phy_obj[g_phy_select].config->lo_leakage_cal_flag;
	config->reg901_val = g_phy_obj[g_phy_select].config->reg901_val;
	config->reg639_620_bit1 = g_phy_obj[g_phy_select].config->reg639_620_bit1;
	config->reg61A_val = g_phy_obj[g_phy_select].config->reg61A_val;
	config->reg600_val = g_phy_obj[g_phy_select].config->reg600_val;
	config->reg602_val = g_phy_obj[g_phy_select].config->reg602_val;
	config->scap_min = g_phy_obj[g_phy_select].config->scap_min;
	config->scap_max = g_phy_obj[g_phy_select].config->scap_max;
	config->scap_cnt = g_phy_obj[g_phy_select].config->scap_cnt;
    config->vol_low_limit = g_phy_obj[g_phy_select].config->vol_low_limit;
    config->vol_up_limit = g_phy_obj[g_phy_select].config->vol_up_limit;
    config->vol_margin = g_phy_obj[g_phy_select].config->vol_margin;
    config->trx_lo_cal_mode = g_phy_obj[g_phy_select].config->trx_lo_cal_mode;
    config->fvco_min = g_phy_obj[g_phy_select].config->fvco_min;
	config->r_cal_flag = g_phy_obj[g_phy_select].config->r_cal_flag;
	config->imb_rx_cfg[0][0] = g_phy_obj[g_phy_select].config->imb_rx_cfg[0][0];
	config->imb_rx_cfg[0][1] = g_phy_obj[g_phy_select].config->imb_rx_cfg[0][1];
	config->imb_rx_cfg[1][0] = g_phy_obj[g_phy_select].config->imb_rx_cfg[1][0];
	config->imb_rx_cfg[1][1] = g_phy_obj[g_phy_select].config->imb_rx_cfg[1][1];
	config->lvds_cal_flag = g_phy_obj[g_phy_select].config->lvds_cal_flag;
	config->syspll_cfg_flag = g_phy_obj[g_phy_select].config->syspll_cfg_flag;
	cmd_api_print_config(g_phy_select);
}

int CUSTOMER_CMD_API_chip_init(void)
{
	unsigned long module_debug;
	int ret=0;
	
	LOG_MAIN("start to init chip...\n");
	read_config_from_file();
	if (g_phy_obj[g_phy_select].init_flag)
	{
		ret = main_init(&g_phy_obj[g_phy_select]);
	}
	else
	{
		module_debug = g_phy_obj[g_phy_select].module_debug;
		customer_init(&g_phy_obj[g_phy_select], &g_phy_config[g_phy_select]);
		g_phy_obj[g_phy_select].module_debug = module_debug;
		g_phy_obj[g_phy_select].debug_on = 1;
		ret = main_init(&g_phy_obj[g_phy_select]);
	}
	write_config_to_file();
	
	return ret;
}

int CUSTOMER_CMD_API_calibr_buffer_del(void)
{
	int ret=0;
	
	LOG_MAIN("del calibr buffer\n");

	#if HAVE_FS
	remove("./gc080x_config");
	//unlink("/home/gc080x_config");
	#else
	#endif
	
	return ret;
}

int CUSTOMER_CMD_API_rf_bandwith_change(BANDWITH_ENUM bandwith)
{
	LOG_MAIN("change bandwith: %d\n", bandwith);
	return rf_bandwith_change(&g_phy_obj[g_phy_select], bandwith);
}

int CUSTOMER_CMD_API_trx_lo_change(unsigned long long  txlo, unsigned long long  rxlo, unsigned char rxdc_offst_cal_flag, unsigned char qec_flag)
{
	short ret=0;
	char rxdc_temp_flag, rx_qec_temp_flag, tx_qec_temp_flag;
	LOG_MAIN("change lo, txlo=%llu, rxlo=%llu, rxdc_offst_cal_flag=%d, qec_flag=%d\n", txlo, rxlo, rxdc_offst_cal_flag, qec_flag);
	rxdc_temp_flag = g_phy_obj[g_phy_select].config->rx_dc_cal_flag;
	rx_qec_temp_flag = g_phy_obj[g_phy_select].config->rx_qec_flag;
	tx_qec_temp_flag = g_phy_obj[g_phy_select].config->tx_qec_flag;
	g_phy_obj[g_phy_select].config->rx_dc_cal_flag = rxdc_offst_cal_flag;
	g_phy_obj[g_phy_select].config->rx_qec_flag = qec_flag;
	g_phy_obj[g_phy_select].config->tx_qec_flag = qec_flag;
	ret = trx_lo_change(&g_phy_obj[g_phy_select], txlo, rxlo);
	g_phy_obj[g_phy_select].config->rx_dc_cal_flag = rxdc_temp_flag;
	g_phy_obj[g_phy_select].config->rx_qec_flag = rx_qec_temp_flag;
	g_phy_obj[g_phy_select].config->tx_qec_flag = tx_qec_temp_flag;
	return ret;
}

int CUSTOMER_CMD_API_lo_change_ready(unsigned char lo_change_mode, unsigned long long  txlo, unsigned long long  rxlo)
{
	short ret=0;
	LOG_MAIN("lo_change_mode=%d, txlo=%llu, rxlo=%llu\n", lo_change_mode, txlo, rxlo);
	ret = trx_lo_change_ext(&g_phy_obj[g_phy_select], lo_change_mode, txlo, rxlo);
	return ret;
}

int CUSTOMER_CMD_API_lo_change_act(void)
{
	short ret=0;
	ret = trx_lo_change_act(&g_phy_obj[g_phy_select]);
	return ret;
}

int CUSTOMER_CMD_API_tx_atten_change(TRX_CHN_ENUM chn, int val, short immed)
{
	LOG_MAIN("tx atten change, chn=%d, index=%d, immed=%d\n", chn, val, immed);
	return tx_atten_change(&g_phy_obj[g_phy_select], chn, val, immed);
}

int CUSTOMER_CMD_API_tx_split_atten_change(TRX_CHN_ENUM chn, TX_SPLIT_ATTEN_ENUM type, unsigned int val, short immed)
{
	LOG_MAIN("tx split atten change, chn=%d, type:%d, index=%d, immed=%d\n", chn, type, val, immed);
	return tx_split_atten_change(&g_phy_obj[g_phy_select], chn, type, val, immed);
}

int CUSTOMER_CMD_API_tdd_trx_change(FSM_ST_ENUM st)
{
	if (st==0)
		st = AT_FSM_TX;
	else
		st = AT_FSM_RX;
	LOG_MAIN("tdd trx dir change, status=%d\n", st);
	return tdd_trx_change(&g_phy_obj[g_phy_select], st);
}

int CUSTOMER_CMD_API_rx_gain_mgc_change(TRX_CHN_ENUM chn, RX_MGC_GAIN_ENUM tb, int val)
{
	LOG_MAIN("rx gain mgc change, chn=%d, tb=%d, index=%d\n", chn, tb, val);
	return rx_gain_mgc_change(&g_phy_obj[g_phy_select], chn, tb, val);
}

int CUSTOMER_CMD_API_tx_port_enable(TRX_CHN_ENUM chn, short en)
{
	LOG_MAIN("tx port enable/disable, chn=%d, on=%d\n", chn, en);
	return tx_port_enable(&g_phy_obj[g_phy_select], chn, en);
}

int CUSTOMER_CMD_API_rx_port_enable(TRX_CHN_ENUM chn, short en)
{
	LOG_MAIN("rx port enable/disable, chn=%d, on=%d\n", chn, en);
	return rx_port_enable(&g_phy_obj[g_phy_select], chn, en);
}

int CUSTOMER_CMD_API_tx_dig_atten_change(TRX_CHN_ENUM chn, unsigned short index)
{
	LOG_MAIN("tx dig atten change, chn=%d, index=%d\n", chn, index);
	return tx_dig_atten_change(&g_phy_obj[g_phy_select], chn, index);
}

int CUSTOMER_CMD_API_rf_chip_suspend(void)
{
	LOG_MAIN("rf chip suspend\n");
	rf_chip_suspend(&g_phy_obj[g_phy_select]);
	return 0;
}

int CUSTOMER_CMD_API_rf_chip_resume(void)
{
	LOG_MAIN("rf chip resume\n");
	return rf_chip_resume(&g_phy_obj[g_phy_select]);
}

void CUSTOMER_CMD_API_test_tx_fir_coeff_clear(TRX_CHN_ENUM chn)
{
	LOG_MAIN("test tx fir coeff clear, chn=%d\n", chn);
	test_tx_fir_coeff_clear(&g_phy_obj[g_phy_select], chn);
}

void CUSTOMER_CMD_API_test_adc_dac_off(short dir, TRX_CHN_ENUM chn)
{
	LOG_MAIN("test adc dac off, dir=%d, chn=%d\n", dir, chn);
	test_adc_dac_off(&g_phy_obj[g_phy_select], dir, chn);
}

void CUSTOMER_CMD_API_trx_ana_bw_adj(short dir, TRX_CHN_ENUM chn, unsigned char val[])
{
	TRX_ENUM dire = (dir == 0)? TX_DIR : RX_DIR;
	LOG_MAIN("trx_ana_bw_adj, dir=%d, chn=%d, val1=0x%x, val2=0x%x\n", dire, chn, val[0], val[1]);
	trx_ana_bw_adj(&g_phy_obj[g_phy_select], dire, chn, val);
}

void CUSTOMER_CMD_API_fir_coef_set(short dir, TRX_CHN_ENUM chn, int count, unsigned long long fir_coef[])
{
	int i, real_fir_coef[128];
	TRX_ENUM dire = (dir == 0)? TX_DIR : RX_DIR;
	my_memset(real_fir_coef, 0, sizeof(real_fir_coef));
	LOG_MAIN("fir_coef_set, dir=%d, chn=%d, count=%d\n", dire, chn, count);
	for (i=0; i<count/2; i++)
	{
		real_fir_coef[i*2] = fir_coef[i] & 0xffffffff;
		real_fir_coef[i*2+1] = (fir_coef[i] >> 32) & 0xffffffff;
		LOG_MAIN("real_fir_coef[%d]=%d\n", i*2, real_fir_coef[i*2]);
		LOG_MAIN("real_fir_coef[%d]=%d\n", i*2+1, real_fir_coef[i*2+1]);
	}
	trx_fir_coef_set(&g_phy_obj[g_phy_select], dire, chn, count, real_fir_coef);
}

void CUSTOMER_CMD_API_fpga_tail_set(short dir, TRX_CHN_ENUM chn)
{
	TRX_ENUM dire = (dir == 0)? TX_DIR : RX_DIR;
	LOG_MAIN("fpga_tail_set, dir=%d, chn=%d\n", dire, chn);
	fpga_tail_set(&g_phy_obj[g_phy_select], dire, chn, 0);
}

/*----------------------------------------------------------------------------------------------*/
// debug API step by step
/*----------------------------------------------------------------------------------------------*/
void cmd_api_error_set(short val)
{
	g_phy_obj[g_phy_select].error = val;
}

short cmd_api_error_get(void)
{
	return g_phy_obj[g_phy_select].error;
}

void cmd_api_phy_obj_init(short sel)
{
	LOG_MAIN("select chip %d\n", sel);
	if (sel < RF_PHY_NUMBER)
		g_phy_select = sel;
	else
		g_phy_select = RF_PHY_NUMBER-1;
	customer_init(&g_phy_obj[g_phy_select], &g_phy_config[g_phy_select]);
}

void cmd_api_module_debug_on(unsigned long on)
{
	LOG_MAIN("module debug on: 0x%x\n", on);
	module_debug_onoff(&g_phy_obj[g_phy_select], on);
}

void cmd_api_write_reg(unsigned short reg, unsigned char val)
{
	//LOG_MAIN("W:0x%x=0x%x\n", reg, val);
	hal_spi_write_reg(&g_phy_obj[g_phy_select], reg, val);
}

unsigned char cmd_api_read_reg(unsigned short reg)
{
	unsigned char val;
	val = hal_spi_read_reg(&g_phy_obj[g_phy_select], reg);
	//LOG_MAIN("R:0x%x=0x%x\n", reg, val);
	return val;
}

void cmd_api_fwrite_reg(unsigned int reg, unsigned int val)
{
	LOG_MAIN("FPAGE_W:0x%x=0x%x\n", reg, val);
	hal_fpga_write_reg(&g_phy_obj[g_phy_select], reg, val);
}

unsigned int cmd_api_fread_reg(unsigned int reg)
{
	int val;
	val = hal_fpga_read_reg(&g_phy_obj[g_phy_select], reg);
	LOG_MAIN("FPGA_R:0x%x=0x%x\n", reg, val);
}

int cmd_api_read_lut_byte(LUT_INDEX_ENUM lut, short index, unsigned short reg, int lut_addr, unsigned char offset, unsigned char *pval)
{
	read_lut_byte(&g_phy_obj[g_phy_select], lut, index, reg, lut_addr, offset, pval);
	return 0;
}

int cmd_api_write_lut_byte(LUT_INDEX_ENUM lut, short index, unsigned short reg, int lut_addr, unsigned char offset, unsigned char val)
{
	write_lut_byte(&g_phy_obj[g_phy_select], lut, index, reg, lut_addr, offset, val);
	return 0;
}

int cmd_api_read_lut_word(LUT_INDEX_ENUM lut, int lut_addr, unsigned char vals[])
{
	read_lut_word(&g_phy_obj[g_phy_select], lut, lut_addr, vals);
	return 0;
}

int cmd_api_write_lut_word(LUT_INDEX_ENUM lut, int lut_addr, unsigned char vals[])
{
	write_lut_word(&g_phy_obj[g_phy_select], lut, lut_addr, vals);
	return 0;
}

short cmd_api_power_init(void)
{
	short ret=0;
	LOG_MAIN("power init\n");
	ret = power_init(&g_phy_obj[g_phy_select]);
	if (ret < 0)
		LOG_MAIN("spi w/r error!\n");

	sxtrx_cfg_switch(&g_phy_obj[g_phy_select]);

	#if 0
	g_phy_obj[g_phy_select].config->chip_ver = confim_config_info(&g_phy_obj[g_phy_select]);
	if (g_phy_obj[g_phy_select].config->chip_ver < 0)
	{
		LOG_ERROR("config error!\n");
		goto base_error;
	}
	#endif
base_error:	
	return ret;
}

void cmd_api_rcal(int rcal_read)
{
	g_phy_obj[g_phy_select].r_cal_flag = 0;
	g_phy_obj[g_phy_select].config->rcal_read = rcal_read;
	LOG_MAIN("rcal calibration, rcal_read = %d.\n", rcal_read);
	rcal(&g_phy_obj[g_phy_select]);
}

void cmd_api_sys_clock_init(unsigned long freq, unsigned long long vco_freq)
{
	LOG_MAIN("config sys clock, freq:%lu, vco:%llu\n", freq, vco_freq);
	g_phy_obj[g_phy_select].config->xtal_freq = freq;
	g_phy_obj[g_phy_select].config->sys_fvco = vco_freq;
	config_syspll(&g_phy_obj[g_phy_select], g_phy_obj[g_phy_select].config->xtal_freq, g_phy_obj[g_phy_select].config->sys_fvco);
}

void cmd_api_rx_imb_set(unsigned char rmb[])
{
	g_phy_obj[g_phy_select].config->imb_rx_cfg[0][0] = rmb[0];
	g_phy_obj[g_phy_select].config->imb_rx_cfg[0][1] = rmb[1];
	g_phy_obj[g_phy_select].config->imb_rx_cfg[1][0] = rmb[2];
	g_phy_obj[g_phy_select].config->imb_rx_cfg[1][1] = rmb[3];
	LOG_MAIN("rx imb set, 0xD32:0x%02x, 0xD36:0x%02x, 0xD31:0x%02x, 0xD37:0x%02x\n",
		g_phy_obj[g_phy_select].config->imb_rx_cfg[0][0],
		g_phy_obj[g_phy_select].config->imb_rx_cfg[0][1],
		g_phy_obj[g_phy_select].config->imb_rx_cfg[1][0], 
		g_phy_obj[g_phy_select].config->imb_rx_cfg[1][1]);
	hal_spi_write_reg(&g_phy_obj[g_phy_select], 0xD32, g_phy_obj[g_phy_select].config->imb_rx_cfg[0][0]);
	hal_spi_write_reg(&g_phy_obj[g_phy_select], 0xD36, g_phy_obj[g_phy_select].config->imb_rx_cfg[0][1]);
	hal_spi_write_reg(&g_phy_obj[g_phy_select], 0xD31, g_phy_obj[g_phy_select].config->imb_rx_cfg[1][0]);
	hal_spi_write_reg(&g_phy_obj[g_phy_select], 0xD37, g_phy_obj[g_phy_select].config->imb_rx_cfg[1][1]);
}

short cmd_api_lut_load(char mode, char use_hybrid, char hybrid_mode)
{
	short ret=0;
	unsigned char lut_ver[5];
	
	LOG_MAIN("load lut, mode:%d, use_hybrid:%d, hybrid_mode:%d\n", mode, use_hybrid, hybrid_mode);
	g_phy_obj[g_phy_select].config->mode = mode;
	g_phy_obj[g_phy_select].config->use_bybrid_mode = use_hybrid;
	g_phy_obj[g_phy_select].config->hybrid_mode = hybrid_mode;
	
	g_phy_obj[g_phy_select].loading_lut = 1;
	ret = load_lut(&g_phy_obj[g_phy_select], g_phy_obj[g_phy_select].config->mode);
	if (ret < 0)
	{
		g_phy_obj[g_phy_select].loading_lut = 0;
		LOG_MAIN("load lut failed!\n");
		return ret;
	}
	g_phy_obj[g_phy_select].loading_lut = 0;
	LOG_MAIN("load lut success\n");

	ret = lut_version_get(&g_phy_obj[g_phy_select], lut_ver);
	if (ret < 0)
	{
		LOG_ERROR("lut version error!\n");
		return ret;
	}
	LOG_MAIN("[0x%02x 0x%02x 0x%02x] [0x%02x 0x%02x]\n", lut_ver[0], lut_ver[1], lut_ver[2], lut_ver[3], lut_ver[4]);

	return ret;
}

void cmd_api_dig_trx_filter_use_fir(BANDWITH_ENUM bw)
{
	LOG_MAIN("Enable Filter used Fir, bw=%d.\n", bw);
	g_phy_obj[g_phy_select].config->bandwith = bw;
	dig_trx_filter_use_fir(&g_phy_obj[g_phy_select]);
}

void cmd_api_x4_enable(short enable, short flag)
{
	LOG_MAIN("x4_enable: %d, flag:%d\n", enable, flag);
	g_phy_obj[g_phy_select].config->x4_enable = enable;
	if (flag)
		refclk_config(&g_phy_obj[g_phy_select], g_phy_obj[g_phy_select].config->x4_enable);
}

void cmd_api_fvco_min(unsigned long long fvco_min)
{
	LOG_MAIN("fvco_min: %lld\n", fvco_min);
	g_phy_obj[g_phy_select].config->fvco_min = fvco_min;
}

void cmd_api_analog_init(CHIP_MODE_ENUM mode)
{
	LOG_MAIN("analog init, mode=%d\n", mode);
	g_phy_obj[g_phy_select].config->mode = mode;
	analog_init(&g_phy_obj[g_phy_select]);
}

void cmd_api_digtal_init(DIG_IF_ENUM dif, BANDWITH_ENUM bw, IF_TYPE_ENUM port, DATA_RATE_ENUM rate, short step)
{
	LOG_MAIN("digtal init, bw:%d, dif:%d, port:%d, rate:%d, step:%d\n", bw, dif, port, rate, step);
	g_phy_obj[g_phy_select].config->bandwith = bw;
	g_phy_obj[g_phy_select].config->dig_if = dif;
	g_phy_obj[g_phy_select].config->p0p1_port = port;
	g_phy_obj[g_phy_select].config->data_rate = rate;
	digital_init(&g_phy_obj[g_phy_select], g_phy_obj[g_phy_select].config->mode, g_phy_obj[g_phy_select].config->bandwith,
		g_phy_obj[g_phy_select].config->dig_if, g_phy_obj[g_phy_select].config->p0p1_port, g_phy_obj[g_phy_select].config->data_rate, step);
}

void cmd_api_trx_lut_load(unsigned long long txlo, BANDWITH_ENUM bw)
{
	LOG_MAIN("trx lut load, freq:%llu, bw:%d\n", txlo, bw);
	trx_lut_load(&g_phy_obj[g_phy_select], txlo, bw, g_phy_obj[g_phy_select].config->custom_bandwith_flag);
}

int cmd_api_detect_chip(void)
{
    if (detect_chip(&g_phy_obj[g_phy_select]) < 0)
        return -1;
	return 0;
}

void cmd_api_custom_bw_init(short flag, unsigned long bw, short bw_index, unsigned long bb_sample_rate, unsigned char dac_div, unsigned char adc_div)
{
	LOG_MAIN("custom bw init, en:%d, bw:%lu, bw_index:%d, bb sample rate:%lu, dac div:0x%x, adc div:0x%x\n", flag, bw, bw_index, bb_sample_rate, dac_div, adc_div);
	g_phy_obj[g_phy_select].config->custom_bandwith_flag = flag;
	g_phy_obj[g_phy_select].config->custom_bandwith = bw;
	g_phy_obj[g_phy_select].config->bandwith = bw_index;
	g_phy_obj[g_phy_select].config->bb_sample_rate = bb_sample_rate;
	g_phy_obj[g_phy_select].config->dac_syspll_lo_div = dac_div;
	g_phy_obj[g_phy_select].config->adc_syspll_lo_div = adc_div;
	if (g_phy_obj[g_phy_select].config->custom_bandwith_flag)
		g_phy_obj[g_phy_select].config->bandwith = proximity_bandwith_seek(&g_phy_obj[g_phy_select], g_phy_obj[g_phy_select].config->custom_bandwith);
}

void cmd_api_wait_init(void)
{
	LOG_MAIN("wait_init.\n");
	g_phy_obj[g_phy_select].st = AT_FSM_WAIT;
	wait_init(&g_phy_obj[g_phy_select]);
}

void cmd_api_band_dep_calibr_start(void)
{
	int ret=0;
	
	LOG_MAIN("band_dep_calibr_start.\n");
	g_phy_obj[g_phy_select].st = AT_FSM_WAIT;
	if (g_phy_obj[g_phy_select].config->use_bybrid_mode)
	{
		g_phy_obj[g_phy_select].spi = g_phy_obj[g_phy_select].config->mode;
		ret = bybrid_mode_cal_switch(&g_phy_obj[g_phy_select]);
		if(ret < 0)
		{
			LOG_ERROR("ERROR: hybrid mode switch failed!\n");
			return;
		}
	}
}

void cmd_api_trx_bw_lut_load(void)
{
	LOG_MAIN("trx_bw_lut_load.\n");
	trx_bw_lut_load(&g_phy_obj[g_phy_select], g_phy_obj[g_phy_select].config->bandwith, g_phy_obj[g_phy_select].config->custom_bandwith_flag);
}

void cmd_api_band_dep_calibr_flag_clear(void)
{
	LOG_MAIN("band_dep_calibr_flag_clear.\n");
	band_dep_calibr_phase_of_bandwidth_calflag_clr(&g_phy_obj[g_phy_select]);
}

void cmd_api_band_dep_calflag_set(void)
{
	LOG_MAIN("band_dep_calflag_set.\n");
	band_dep_calibr_phase_of_bandwidth_calflag_set(&g_phy_obj[g_phy_select]);
}

void cmd_api_band_dep_calibr_end(void)
{
	LOG_MAIN("band_dep_calibr_end.\n");
	if (g_phy_obj[g_phy_select].config->use_bybrid_mode)
		g_phy_obj[g_phy_select].config->mode = g_phy_obj[g_phy_select].spi;

	g_phy_obj[g_phy_select].txlo = g_phy_obj[g_phy_select].config->tx_flo;
	g_phy_obj[g_phy_select].rxlo = g_phy_obj[g_phy_select].config->rx_flo;
	g_phy_obj[g_phy_select].bandwith = g_phy_obj[g_phy_select].config->bandwith;
}

void cmd_api_misc_init(void)
{
	LOG_MAIN("misc_init.\n");
	misc_init(&g_phy_obj[g_phy_select]);
}

void cmd_api_auxadc_cal(void)
{
	LOG_MAIN("auxadc_cal.\n");
	g_phy_obj[g_phy_select].auxadc1_cal_flag = 0;
	auxadc_cal(&g_phy_obj[g_phy_select]);
}


void cmd_api_rf_bandwidth_set(BANDWITH_ENUM bw)
{
	LOG_MAIN("bandwidth:%d.\n", bw);
	g_phy_obj[g_phy_select].config->bandwith = bw;
	set_rf_bandwidth(&g_phy_obj[g_phy_select], g_phy_obj[g_phy_select].config->bandwith);
}

void cmd_api_rx_port(TRX_CHN_ENUM chn, RX_PORT_ENUM port, RXFE_GAIN_ENUM gain, short en)
{
	LOG_MAIN("rx port, chn:%d, port:%d, gain:%d, en:%d\n", chn, port, gain, en);
	g_phy_obj[g_phy_select].config->rx_port[chn] = port;
	g_phy_obj[g_phy_select].config->rxfe_gain[chn] = gain;
	set_rx_port(&g_phy_obj[g_phy_select], chn, g_phy_obj[g_phy_select].config->rx_port[chn], gain, en);
	
	fsm_status_get_only(&g_phy_obj[g_phy_select]);
	if (g_phy_obj[g_phy_select].st == AT_FSM_FDD)
	{
		fdd_fsm_to_alert(&g_phy_obj[g_phy_select]);
		if (fsm_status_get(&g_phy_obj[g_phy_select], AT_FSM_ALERT) < 0)
		{
			LOG_MAIN("FDD --> ALERT: ERROR\n");
			return;
		}

		CHIP_DELAY(100);
		fdd_alert_to_fsm(&g_phy_obj[g_phy_select]);
		if (fsm_status_get(&g_phy_obj[g_phy_select], AT_FSM_FDD) < 0)
		{
			LOG_MAIN("ALERT --> FDD: ERROR\n");
			return;
		}
	}
	else if (g_phy_obj[g_phy_select].st == AT_FSM_RX)
	{
		if (g_phy_obj[g_phy_select].config->wire_control_en)
		{
			tdd_rx_to_wait(&g_phy_obj[g_phy_select]);
			if (fsm_status_get(&g_phy_obj[g_phy_select], AT_FSM_WAIT) < 0)
			{
				LOG_MAIN("RX --> WAIT: ERROR\n");
				return;
			}
			
			tdd_wait_to_alert(&g_phy_obj[g_phy_select]);
			if (fsm_status_get(&g_phy_obj[g_phy_select], AT_FSM_ALERT) < 0)
			{
				LOG_MAIN("WAIT --> ALERT: ERROR\n");
				return;
			}

			CHIP_DELAY(100);
			tdd_alert_to_rx(&g_phy_obj[g_phy_select]);
			if (fsm_status_get(&g_phy_obj[g_phy_select], AT_FSM_RX) < 0)
			{
				LOG_MAIN("ALERT --> RX: ERROR\n");
				return;
			}
		}
		else
		{
			hal_spi_write_reg(&g_phy_obj[g_phy_select], 0x0cc, 0x00);
			if (fsm_status_get(&g_phy_obj[g_phy_select], AT_FSM_WAIT) < 0)
			{
				LOG_MAIN("RX --> WAIT: ERROR\n");
				return;
			}
			
			hal_spi_write_reg(&g_phy_obj[g_phy_select], 0x0cc, 0x40);
			if (fsm_status_get(&g_phy_obj[g_phy_select], AT_FSM_ALERT) < 0)
			{
				LOG_MAIN("WAIT --> ALERT: ERROR\n");
				return;
			}

			CHIP_DELAY(100);
			hal_spi_write_reg(&g_phy_obj[g_phy_select], 0x0cc, 0x04);
			if (fsm_status_get(&g_phy_obj[g_phy_select], AT_FSM_RX) < 0)
			{
				LOG_MAIN("ALERT --> RX: ERROR\n");
				return;
			}
		}
	}
}

void cmd_api_rx_port_man(TRX_CHN_ENUM chn, RX_PORT_ENUM port, RXFE_GAIN_ENUM gain)
{
	LOG_MAIN("rx port, chn:%d, port:%d, gain:%d\n", chn, port, gain);
	g_phy_obj[g_phy_select].config->rx_port[chn] = port;
	g_phy_obj[g_phy_select].config->rxfe_gain[chn] = gain;
	set_rx_port_man(&g_phy_obj[g_phy_select], chn, g_phy_obj[g_phy_select].config->rx_port[chn], gain);
}

void cmd_api_tx_port(TRX_CHN_ENUM chn, TX_PORT_ENUM port, short en)
{
	LOG_MAIN("tx port, chn:%d, port:%d, en=%d\n", chn, port, en);
	g_phy_obj[g_phy_select].config->tx_port[chn] = port;
	//DIG_CHAN_ENA(0, chn, en);
	set_tx_port(&g_phy_obj[g_phy_select], chn, g_phy_obj[g_phy_select].config->tx_port[chn], en);
	fsm_status_get_only(&g_phy_obj[g_phy_select]);
	if (g_phy_obj[g_phy_select].st == AT_FSM_FDD)
	{
		fdd_fsm_to_alert(&g_phy_obj[g_phy_select]);
		if (fsm_status_get(&g_phy_obj[g_phy_select], AT_FSM_ALERT) < 0)
		{
			LOG_MAIN("FDD --> ALERT: ERROR\n");
			return;
		}

		CHIP_DELAY(100);
		fdd_alert_to_fsm(&g_phy_obj[g_phy_select]);
		if (fsm_status_get(&g_phy_obj[g_phy_select], AT_FSM_FDD) < 0)
		{
			LOG_MAIN("ALERT --> FDD: ERROR\n");
			return;
		}
	}
	else if (g_phy_obj[g_phy_select].st == AT_FSM_TX)
	{
		if (g_phy_obj[g_phy_select].config->wire_control_en)
		{
			tdd_tx_to_wait(&g_phy_obj[g_phy_select]);
			if (fsm_status_get(&g_phy_obj[g_phy_select], AT_FSM_WAIT) < 0)
			{
				LOG_MAIN("TX --> WAIT: ERROR\n");
				return;
			}
			
			tdd_wait_to_alert(&g_phy_obj[g_phy_select]);
			if (fsm_status_get(&g_phy_obj[g_phy_select], AT_FSM_ALERT) < 0)
			{
				LOG_MAIN("WAIT --> ALERT: ERROR\n");
				return;
			}

			CHIP_DELAY(100);
			tdd_alert_to_tx(&g_phy_obj[g_phy_select]);
			if (fsm_status_get(&g_phy_obj[g_phy_select], AT_FSM_TX) < 0)
			{
				LOG_MAIN("ALERT --> TX: ERROR\n");
				return;
			}
		}
		else
		{
			hal_spi_write_reg(&g_phy_obj[g_phy_select], 0x0cc, 0x00);
			if (fsm_status_get(&g_phy_obj[g_phy_select], AT_FSM_WAIT) < 0)
			{
				LOG_MAIN("TX --> WAIT: ERROR\n");
				return;
			}
			
			hal_spi_write_reg(&g_phy_obj[g_phy_select], 0x0cc, 0x40);
			if (fsm_status_get(&g_phy_obj[g_phy_select], AT_FSM_ALERT) < 0)
			{
				LOG_MAIN("WAIT --> ALERT: ERROR\n");
				return;
			}

			CHIP_DELAY(100);
			hal_spi_write_reg(&g_phy_obj[g_phy_select], 0x0cc, 0x02);
			if (fsm_status_get(&g_phy_obj[g_phy_select], AT_FSM_TX) < 0)
			{
				LOG_MAIN("ALERT --> TX: ERROR\n");
				return;
			}
		}
	}
}

void cmd_api_fsm_init(void)
{
	LOG_MAIN("fsm init.\n");
	g_phy_obj[g_phy_select].st = AT_FSM_WAIT;
	fsm_init(&g_phy_obj[g_phy_select]);
}

void cmd_api_manual_enable(short on)
{
	LOG_MAIN("manual enable, on:%d.\n", on);
	manual_enable(&g_phy_obj[g_phy_select], on);
}

void cmd_api_tx_atten(unsigned char val, short immed)
{
	LOG_MAIN("tx atten, val:%d, immed:%d\n", val, immed);
	if (set_tx_atten(&g_phy_obj[g_phy_select], g_phy_obj[g_phy_select].config->tx_flo, val, immed) < 0)
		return;
	g_phy_obj[g_phy_select].tx_atten[0] = val;
	g_phy_obj[g_phy_select].tx_atten[1] = val;
	g_phy_obj[g_phy_select].tx_curr_index = val;
}

void cmd_api_tx_atten_chn(int chn, unsigned char val, short immed)
{
	LOG_MAIN("tx atten, chn:%d, val:%d, immed:%d\n", chn, val, immed);
	if (set_tx_atten_chn(&g_phy_obj[g_phy_select], chn, g_phy_obj[g_phy_select].config->tx_flo, val, immed) < 0)
		return;
	g_phy_obj[g_phy_select].tx_atten[chn] = val;
}

void cmd_api_tx_dig_atten(TRX_CHN_ENUM chn, unsigned short index)
{
	LOG_MAIN("tx_atten_dig, chn:%d, index:%d\n", chn, index);
	set_tx_dig_atten(&g_phy_obj[g_phy_select], chn, index);
}

void cmd_api_rx_mgc_gain(TRX_CHN_ENUM chn, RX_MGC_GAIN_ENUM tb, unsigned char val)
{
	short index;
	LOG_MAIN("rx mgc gain, chn:%d, table:%d, val:%d\n", chn, tb, val);
	if (rx_mgc_gain(&g_phy_obj[g_phy_select], chn, tb, val) < 0)
		return;

	index = (chn==TRX_CHN1)?0:1;
	switch (tb) {
		case LMT_G: g_phy_obj[g_phy_select].rx_lmt_gain[index]=val; break;
		case LPF_G: g_phy_obj[g_phy_select].rx_lpf_gain[index]=val; break;
		case DIG_G: g_phy_obj[g_phy_select].rx_dig_gain[index]=val; break;
	}
}

void cmd_api_rx_mgc_max_gain(TRX_CHN_ENUM chn)
{
	LOG_MAIN("rx mgc max gain, chn:%d\n", chn);
	rx_mgc_max_gain(&g_phy_obj[g_phy_select], chn);
	rx_gain_mgc_change(&g_phy_obj[g_phy_select], chn, LMT_G, 5);
	rx_gain_mgc_change(&g_phy_obj[g_phy_select], chn, LPF_G, 12);
	rx_gain_mgc_change(&g_phy_obj[g_phy_select], chn, DIG_G, 0);
}

int cmd_api_rxgain_force_valid(char enable)
{
	LOG_MAIN("rxgain_force_valid, enable=%d\n", enable);
	g_phy_obj[g_phy_select].config->rxgain_force_valid_flag = enable;
	rxgain_force_valid_config(&g_phy_obj[g_phy_select], enable);
	return 0;
}

int cmd_api_rxgain_set(TRX_CHN_ENUM chn, char enable, TABLE_MODE_ENUM split, GCTRL_MODE_ENUM ctrl)
{
	LOG_MAIN("rxgain_set, chn=%d, enable=%d, split=%d, gain mode=%d\n", chn, enable, split, ctrl);
	g_phy_obj[g_phy_select].config->gain_ctrl_pin_flag = enable;
	g_phy_obj[g_phy_select].config->gain_table_mode = split;
	g_phy_obj[g_phy_select].config->gain_ctrl_mode = ctrl;
	test_rxgain_ctrl_set(&g_phy_obj[g_phy_select], chn);
	return 0;
}

void cmd_api_fdd_force_wait(void)
{
	LOG_MAIN("fdd force wait.\n");
	fdd_force_wait(&g_phy_obj[g_phy_select]);
	if (fsm_status_get(&g_phy_obj[g_phy_select], AT_FSM_WAIT) < 0)
	{
		LOG_MAIN("FORCE WAIT: ERROR\n");
		return;
	}
}

void cmd_api_fdd_wait_to_alert(void)
{
	LOG_MAIN("fdd_wait_to_alert.\n");
	fdd_wait_to_alert(&g_phy_obj[g_phy_select]);
	if (fsm_status_get(&g_phy_obj[g_phy_select], AT_FSM_ALERT) < 0)
	{
		LOG_MAIN("WAIT --> ALERT: ERROR\n");
		return;
	}
	CHIP_DELAY(100);
}

void cmd_api_fdd_alert_to_fsm(void)
{
	LOG_MAIN("fdd_alert_to_fsm.\n");
	fdd_alert_to_fsm(&g_phy_obj[g_phy_select]);
	if (fsm_status_get(&g_phy_obj[g_phy_select], AT_FSM_FDD) < 0)
	{
		LOG_MAIN("ALERT --> FDD: ERROR\n");
		return;
	}
}

void cmd_api_dig_fir_cfg_manual(char manual_on)
{
	LOG_MAIN("dig_fir_cfg_manual, manual_on=%d.\n", manual_on);
	DIG_FIR_FILTER_CFG_WITH_MANUAL(&g_phy_obj[g_phy_select], manual_on);
}

void cmd_api_fdd_fsm_to_alert(void)
{
	LOG_MAIN("fdd_fsm_to_alert.\n");
	fdd_fsm_to_alert(&g_phy_obj[g_phy_select]);
	if (fsm_status_get(&g_phy_obj[g_phy_select], AT_FSM_ALERT) < 0)
	{
		LOG_MAIN("FDD --> ALERT: ERROR\n");
		return;
	}
	CHIP_DELAY(100);
}

void cmd_api_tx_atten_init(char index)
{
	LOG_MAIN("tx_atten_init, val:%d.\n", index);
	set_tx_atten_cfg(&g_phy_obj[g_phy_select], g_phy_obj[g_phy_select].config->tx_flo, index);
	if (set_tx_atten(&g_phy_obj[g_phy_select], g_phy_obj[g_phy_select].config->tx_flo, index, 0) < 0)
	{
		LOG_MAIN("tx_atten_init error.\n");
		return;
	}
	g_phy_obj[g_phy_select].tx_atten[0] = index;
	g_phy_obj[g_phy_select].tx_atten[1] = index;
	g_phy_obj[g_phy_select].tx_curr_index = index;
}

void cmd_api_wire_control_en(short en, WIRE_CTRL_ENUM pulse)
{
	LOG_MAIN("wire control en:%d, pulse:%d.\n", en, pulse);	
	g_phy_obj[g_phy_select].config->wire_control_en = en;
	g_phy_obj[g_phy_select].config->wire_ctrl = pulse;
	wire_control_en(&g_phy_obj[g_phy_select], g_phy_obj[g_phy_select].config->wire_control_en, g_phy_obj[g_phy_select].config->wire_ctrl);
}

void cmd_api_tdd_wait_to_alert(void)
{
	LOG_MAIN("tdd_wait_to_alert.\n");
	if (g_phy_obj[g_phy_select].config->wire_control_en)
		tdd_wait_to_alert(&g_phy_obj[g_phy_select]);
	else
		hal_spi_write_reg(&g_phy_obj[g_phy_select], 0x0cc, 0x40);
	if (fsm_status_get(&g_phy_obj[g_phy_select], AT_FSM_ALERT) < 0)
	{
		LOG_MAIN("WAIT --> ALERT: ERROR\n");
		return;
	}
	CHIP_DELAY(100);
}

void cmd_api_tdd_alert_to_rx(void)
{
	LOG_MAIN("tdd_alert_to_rx.\n");
	if (g_phy_obj[g_phy_select].config->wire_control_en)
		tdd_alert_to_rx(&g_phy_obj[g_phy_select]);
	else
		hal_spi_write_reg(&g_phy_obj[g_phy_select], 0x0cc, 0x04);
	if (fsm_status_get(&g_phy_obj[g_phy_select], AT_FSM_RX) < 0)
	{
		LOG_MAIN("ALERT --> RX: ERROR\n");
		return;
	}
}

void cmd_api_tdd_rx_to_wait(void)
{
	LOG_MAIN("tdd_rx_to_wait.\n");
	if (g_phy_obj[g_phy_select].config->wire_control_en)
		tdd_rx_to_wait(&g_phy_obj[g_phy_select]);
	else
		hal_spi_write_reg(&g_phy_obj[g_phy_select], 0x0cc, 0x00);
	if (fsm_status_get(&g_phy_obj[g_phy_select], AT_FSM_WAIT) < 0)
	{
		LOG_MAIN("RX --> WAIT: ERROR\n");
		return;
	}
}

void cmd_api_tdd_alert_to_tx(void)
{
	LOG_MAIN("tdd_alert_to_tx.\n");
	if (g_phy_obj[g_phy_select].config->wire_control_en)
		tdd_alert_to_tx(&g_phy_obj[g_phy_select]);
	else
		hal_spi_write_reg(&g_phy_obj[g_phy_select], 0x0cc, 0x02);
	if (fsm_status_get(&g_phy_obj[g_phy_select], AT_FSM_TX) < 0)
	{
		LOG_MAIN("ALERT --> TX: ERROR\n");
		return;
	}
}

void cmd_api_tdd_tx_to_wait(void)
{
	LOG_MAIN("tdd_tx_to_wait.\n");
	if (g_phy_obj[g_phy_select].config->wire_control_en)
		tdd_tx_to_wait(&g_phy_obj[g_phy_select]);
	else
		hal_spi_write_reg(&g_phy_obj[g_phy_select], 0x0cc, 0x00);
	if (fsm_status_get(&g_phy_obj[g_phy_select], AT_FSM_WAIT) < 0)
	{
		LOG_MAIN("TX --> WAIT: ERROR\n");
		return;
	}
}

void cmd_api_rxlo_fsm(unsigned long long flo)
{
	LOG_MAIN("rxlo fsm, freq:%llu\n", flo);
	sxrx_band_fsm(&g_phy_obj[g_phy_select], flo);
}

void cmd_api_txlo_fsm(unsigned long long flo, short core2_en)
{
	LOG_MAIN("txlo fsm, freq:%llu, core2_en:%d\n", flo, core2_en);
	sxtx_band_fsm(&g_phy_obj[g_phy_select], flo, core2_en);
}

void cmd_api_sw_cal_set_reg(char reg901_val, char reg639_620_bit1, char reg61A_val, char reg600_val, char reg602_val)
{
    LOG_MAIN("set sw cal reg, reg901_val=0x%02x, reg639_bit1=0x%02x, reg61A_val=0x%02x, reg600_val=0x%02x, reg602_val=0x%02x\n", reg901_val, reg639_620_bit1, reg61A_val, reg600_val, reg602_val);
    g_phy_obj[g_phy_select].config->reg901_val = reg901_val & 0xff;
    g_phy_obj[g_phy_select].config->reg639_620_bit1 = reg639_620_bit1 & 0xff;
    g_phy_obj[g_phy_select].config->reg61A_val = reg61A_val & 0xff;
    g_phy_obj[g_phy_select].config->reg600_val = reg600_val & 0xff;
    g_phy_obj[g_phy_select].config->reg602_val = reg602_val & 0xff;
}

void cmd_api_trx_lo_cal_mode_set(TRX_LO_CAL_MODE_ENUM trx_lo_cal_mode)
{
    LOG_MAIN("set trx lo cal mode, mode:%d(0-hw cal, 1-sw cal, 2-hybrid cal)\n", trx_lo_cal_mode);
    g_phy_obj[g_phy_select].config->trx_lo_cal_mode = trx_lo_cal_mode;
}

void cmd_api_auxadc_lock_status_vol_range_set(unsigned int vol_low_limit, unsigned int vol_up_limit)
{
    LOG_MAIN("set lock status vol range, low limit=%d, up limit=%d\n", vol_low_limit, vol_up_limit);
    g_phy_obj[g_phy_select].config->vol_low_limit = vol_low_limit;
    g_phy_obj[g_phy_select].config->vol_up_limit = vol_up_limit;
}

void cmd_api_auxadc_lock_status_vol_range_set_ext(unsigned int vol_low_limit, unsigned int vol_up_limit, 
                                                            unsigned int vol_margin, unsigned long long fvco_limit)
{
    LOG_MAIN("set lock status vol range, low limit=%d, up limit=%d, margin=%d, fvco limit:%lld\n", vol_low_limit, vol_up_limit, vol_margin, fvco_limit);
    g_phy_obj[g_phy_select].config->vol_low_limit = vol_low_limit;
    g_phy_obj[g_phy_select].config->vol_up_limit = vol_up_limit;
    g_phy_obj[g_phy_select].config->vol_margin = vol_margin;
    g_phy_obj[g_phy_select].config->fvco_limit = fvco_limit;
}

void cmd_api_rxlo_set(TRX_CHN_ENUM chn, unsigned long long flo)
{
	LOG_MAIN("rxlo set, chn:%d, freq:%llu, x4_enable:%d, trx_lo_cal_mode:%d, vol low limit:%d, vol up limit:%d\n", 
            chn, flo, g_phy_obj[g_phy_select].config->x4_enable, g_phy_obj[g_phy_select].config->trx_lo_cal_mode, 
            g_phy_obj[g_phy_select].config->vol_low_limit, g_phy_obj[g_phy_select].config->vol_up_limit);
	set_trx_lo(&g_phy_obj[g_phy_select], RX_DIR, chn, flo);
}

void cmd_api_txlo_set(TRX_CHN_ENUM chn, unsigned long long flo)
{
	LOG_MAIN("txlo set, chn:%d, freq:%llu, x4_enable:%d, trx_lo_cal_mode:%d, vol low limit:%d, vol up limit:%d\n", 
            chn, flo, g_phy_obj[g_phy_select].config->x4_enable, g_phy_obj[g_phy_select].config->trx_lo_cal_mode, 
            g_phy_obj[g_phy_select].config->vol_low_limit, g_phy_obj[g_phy_select].config->vol_up_limit);
	set_trx_lo(&g_phy_obj[g_phy_select], TX_DIR, chn, flo);
}

void cmd_api_sx_temperature_track_set(TRX_ENUM trx, TRX_CHN_ENUM chn, SX_TEMP_TRACK_MODE_ENUM track_mode)
{
	LOG_MAIN("sx_temperature_track set, dir:%s, chn:%d, sx_temp_track_mode:%d, vol low limit:%d, vol up limit:%d, vol margin:%d, fvco limit:%lld\n", 
            trx?"TX":"RX", chn, track_mode, 
            g_phy_obj[g_phy_select].config->vol_low_limit, g_phy_obj[g_phy_select].config->vol_up_limit, g_phy_obj[g_phy_select].config->vol_margin, 
            g_phy_obj[g_phy_select].config->fvco_limit);
    if (SX_TEMP_TRACK_V1 == track_mode)
    {
        sx_temperature_track_v1(&g_phy_obj[g_phy_select], trx);
    }
    else if (SX_TEMP_TRACK_V2 == track_mode)
    {
        sx_temperature_track_v2(&g_phy_obj[g_phy_select], trx);
    }
    else
    {
        LOG_ERROR("the sx temp track mode do not support! track_version:%d", track_mode);
    }
}


void cmd_api_rxadc_on(TRX_CHN_ENUM chn, short en)
{
	LOG_MAIN("rxadc_on, chn:%d, on:%d\n", chn, en);
	rxadc_on(&g_phy_obj[g_phy_select], chn, en);
}

void cmd_api_rxifbuf_on(TRX_CHN_ENUM chn, short en)
{
	LOG_MAIN("rxifbuf_on, chn:%d, on:%d\n", chn, en);
	rxifbuf_on(&g_phy_obj[g_phy_select], chn, en);
}

void cmd_api_fcal_s2_bypass(char en)
{
	LOG_MAIN("fcal_s2_bypass, fast_sx_lock:%d\n", en);
	g_phy_obj[g_phy_select].config->fast_sx_lock = en;
	fcal_s2_bypass(&g_phy_obj[g_phy_select], g_phy_obj[g_phy_select].config->fast_sx_lock);
}

void cmd_api_core2_s7_s8_s10_s11_s12_bypass(char en)
{
	LOG_MAIN("core2_s7_s8_s10_s11_s12_bypass, core2_enable:%d\n", en);
	g_phy_obj[g_phy_select].config->core2_enable = en;
	core2_s7_s8_s10_s11_s12_bypass(&g_phy_obj[g_phy_select], g_phy_obj[g_phy_select].config->core2_enable);
}

void cmd_api_sx_cal(void)
{
	g_phy_obj[g_phy_select].sx_cal_flag = 0;
	LOG_MAIN("sx_cal_lut_update.\n");
	sx_cal_lut_update(&g_phy_obj[g_phy_select]);
}

void cmd_api_txlo_cal(void)
{
	g_phy_obj[g_phy_select].txlo_cal_flag = 0;
	LOG_MAIN("txlo_cal_lut_update.\n");
	txlo_cal_lut_update(&g_phy_obj[g_phy_select]);
}

int cmd_api_txdc_offset_cal(TRX_CHN_ENUM chn)
{
	int ret=0;
	LOG_MAIN("tx dc offset cal, chn:%d.\n", chn);
	g_phy_obj[g_phy_select].tx_dc_cal_flag[chn] = 0;
	tx_dc_offset_cal(&g_phy_obj[g_phy_select], chn);
	return ret;
}

void cmd_api_rxadc_cal(TRX_CHN_ENUM chn)
{
	LOG_MAIN("rx adc cal, chn:%d.\n", chn);
	rx_adc_cal(&g_phy_obj[g_phy_select], chn);
}

void cmd_api_txdac_cal(TRX_CHN_ENUM chn)
{
	LOG_MAIN("tx dac cal, chn:%d.\n", chn);
	g_phy_obj[g_phy_select].tx_dac_cal_flag[chn] = 0;
	tx_dac_cal(&g_phy_obj[g_phy_select], chn);
}

int cmd_api_lo_leakage_cal(TRX_CHN_ENUM chn)
{
	int ret=0;
	LOG_MAIN("lo leakage cal, chn:%d\n", chn);
	lo_leakage_cal(&g_phy_obj[g_phy_select], chn);
	return ret;
}


short cmd_api_rxdc_offset_cal(TRX_CHN_ENUM chn)
{
	short ret=0;
	LOG_MAIN("rx dc offset cal, chn:%d\n", chn);
	g_phy_obj[g_phy_select].rx_dc_cal_flag[chn] = 0;
	ret = rx_dc_offset_cal(&g_phy_obj[g_phy_select], chn);
	return ret;
}


int cmd_api_rx_imbalance_cal(TRX_CHN_ENUM chn)
{
	int ret=0;
	LOG_MAIN("rx imbalance cal, chn:%d.\n", chn);
	ret=inbalance_cal(&g_phy_obj[g_phy_select], chn);	
	return ret;
}


void cmd_api_rx_bw_cal(TRX_CHN_ENUM chn, BANDWITH_ENUM bandwith, RX_PORT_ENUM port)
{
	LOG_MAIN("rx bw cal, chn:%d, bw:%d, port:%d.\n", chn, bandwith, port);
	g_phy_obj[g_phy_select].config->rx_port[chn] = port;
	g_phy_obj[g_phy_select].config->rxfe_gain[chn] = RX_PORT_G0;
	g_phy_obj[g_phy_select].config->bandwith = bandwith;
	set_rx_port_man(&g_phy_obj[g_phy_select], chn, port, RX_PORT_G0);
	set_rf_bandwidth(&g_phy_obj[g_phy_select], bandwith);
	g_phy_obj[g_phy_select].rx_bw_cal_flag[chn] = 0;
	rx_bw_cal(&g_phy_obj[g_phy_select], chn);
}

int cmd_api_rx_rssi_get(TRX_CHN_ENUM chn)
{
	int rssi_val;
	rssi_val = rx_rssi_get(&g_phy_obj[g_phy_select], chn);
	LOG_MAIN("rx rssi, chn:%d, RSSI:%d.%03d dBfs.\n", chn, rssi_val/1000,  abs(rssi_val)%1000);
	return rssi_val;
}

int cmd_api_rxqec_cal(TRX_CHN_ENUM chn, int ext_loop)
{
	int ret=0;
	LOG_MAIN("rx qec cal, chn:%d, ext_loop:%d.\n", chn, ext_loop);
	g_phy_obj[g_phy_select].rx_qec_flag[chn] = 0;
	g_phy_obj[g_phy_select].config->rx_ext_loop = ext_loop;
	rxqec_cal(&g_phy_obj[g_phy_select], chn);
	return ret;
}

int cmd_api_txqec_cal(TRX_CHN_ENUM chn, int ext_loop, int qec_dbfs, int lol_dbfs)
{
	int ret=0;
	LOG_MAIN("tx qec/lol cal, chn:%d, ext_loop:%d, qec_dbfs:%d, lol_dbfs:%d.\n", chn, ext_loop, qec_dbfs, lol_dbfs);
	g_phy_obj[g_phy_select].tx_qec_flag[chn] = 0;
	g_phy_obj[g_phy_select].config->tx_ext_loop = ext_loop;
	g_phy_obj[g_phy_select].config->qec_dbfs = qec_dbfs;
	g_phy_obj[g_phy_select].config->lol_dbfs = lol_dbfs;
	//txqec_cal(&g_phy_obj[g_phy_select], chn);
	ret = txqec_cal_rflp(&g_phy_obj[g_phy_select], chn);
	return ret;
}

// lyt: We don't use this function
// int cmd_api_qec_tracking_cal(TRX_CHN_ENUM chn, int start, int th)
// {
// 	LOG_MAIN("qec tracking, chn:%d, start:%d, th:%d\n", chn, start, th);
// 	if(start) {
// 		qec_tracking_action_start(&g_phy_obj[g_phy_select], chn, th);
// 	} else {
// 		qec_tracking_action_stop(&g_phy_obj[g_phy_select], chn);
	
// 	}
// 	return 0;
// }

// lyt: We don't use this function
// int cmd_api_rx_dc_tracking_cal(TRX_CHN_ENUM chn, int start, int tia, int debug)
// {
//     LOG_MAIN("qec tracking, chn:%d, start:%d, tia:%d, debug:%d\n", chn, start, tia, debug);
//     if(start) {
//         rx_dc_tracking_action_start(&g_phy_obj[g_phy_select], chn, tia, debug);
//     } else {
//         rx_dc_tracking_action_stop(&g_phy_obj[g_phy_select], chn);
//     }
//     return 0;
// }

void cmd_api_tx_tone(TRX_CHN_ENUM chn, short on, long freq)
{
	LOG_MAIN("tx tone, chn:%d, on:%d, freq=%d\n", chn, on, freq);
	//fn_tx_send_tone(chn, on, freq);
	send_cordic_signal(&g_phy_obj[g_phy_select], chn, g_phy_obj[g_phy_select].config->bandwith, on, freq);
}

int cmd_api_ldo_cal(int *ref_voltage, int cnt)
{
    int i;
    if (ref_voltage == NULL || cnt < 11)
    {
        LOG_ERROR("%s in line %d run fail for invalid parameter.\n", __FUNCTION__, __LINE__);
    }
    
    for (i=0; i<cnt; i++)
        LOG_MAIN("cnt=%d, ref_voltage[%d]=%d\n", cnt, i, ref_voltage[i]);
    
    g_phy_obj[g_phy_select].config->sx_vco_ldo = ref_voltage[0];
    g_phy_obj[g_phy_select].config->sxlf_ldo = ref_voltage[1];
    g_phy_obj[g_phy_select].config->syspll_ldo = ref_voltage[2];
    g_phy_obj[g_phy_select].config->txabb_ldo = ref_voltage[3];
    g_phy_obj[g_phy_select].config->txdac_ldo = ref_voltage[4];
    g_phy_obj[g_phy_select].config->txfe_ldo = ref_voltage[5];
    g_phy_obj[g_phy_select].config->txsx_lo_ldo = ref_voltage[6];
    g_phy_obj[g_phy_select].config->rxadc_ldo = ref_voltage[7];
    g_phy_obj[g_phy_select].config->rxfe_ldo = ref_voltage[8];
    g_phy_obj[g_phy_select].config->rxsx_lo_ldo = ref_voltage[9];
    g_phy_obj[g_phy_select].config->mdig_ldo = ref_voltage[10];

	g_phy_obj[g_phy_select].ldo_cal_flag = 0;
    ldo_cal(&g_phy_obj[g_phy_select]);

    return 0;
}

void cmd_api_lvds_cal(void)
{
	LOG_MAIN("lvds delay cal.\n");
	lvds_delay_cal(&g_phy_obj[g_phy_select]);
}

void cmd_api_txdc_digital_remove(TRX_CHN_ENUM chn)
{
	LOG_MAIN("tx chn:%d\n", chn);
	txdc_digtial_remove(&g_phy_obj[g_phy_select], chn);
}

void cmd_api_error(void)
{
	LOG_MAIN("error code: %d\n", g_phy_obj[g_phy_select].error);
}

void cmd_api_print_config(short sel)
{
	short i, j;
	LOG_MAIN("###############chip config###############\n");
	LOG_MAIN("phy->config->mode=%d\n", g_phy_obj[sel].config->mode);
	LOG_MAIN("phy->config->dig_if=%d\n", g_phy_obj[sel].config->dig_if);
	LOG_MAIN("phy->config->p0p1_port=%d\n", g_phy_obj[sel].config->p0p1_port);
	LOG_MAIN("phy->config->data_rate=%d\n", g_phy_obj[sel].config->data_rate);
	LOG_MAIN("phy->config->tx_port[0]=%d\n", g_phy_obj[sel].config->tx_port[0]);
	LOG_MAIN("phy->config->tx_port[1]=%d\n", g_phy_obj[sel].config->tx_port[1]);
	LOG_MAIN("phy->config->rx_port[0]=%d\n", g_phy_obj[sel].config->rx_port[0]);
	LOG_MAIN("phy->config->rx_port[1]=%d\n", g_phy_obj[sel].config->rx_port[1]);
	LOG_MAIN("phy->config->bandwith=%d\n", g_phy_obj[sel].config->bandwith);
	LOG_MAIN("phy->config->xtal_freq=%lu\n", g_phy_obj[sel].config->xtal_freq);
	LOG_MAIN("phy->config->sys_fvco=%llu\n", g_phy_obj[sel].config->sys_fvco);
	LOG_MAIN("phy->config->x4_enable=%d\n", g_phy_obj[sel].config->x4_enable);
	LOG_MAIN("phy->config->fast_sx_lock=%d\n", g_phy_obj[sel].config->fast_sx_lock);
	LOG_MAIN("phy->config->tx_atten_chn_flag=%d\n", g_phy_obj[sel].config->tx_atten_chn_flag);
	LOG_MAIN("phy->config->rx_flo=%llu\n", g_phy_obj[sel].config->rx_flo);
	LOG_MAIN("phy->config->rx_twin_fxo=%d\n", g_phy_obj[sel].config->rx_twin_fxo);
	LOG_MAIN("phy->config->rx_tsu_fxo=%d\n", g_phy_obj[sel].config->rx_tsu_fxo);
	LOG_MAIN("phy->config->rx_tstate2_fsys_us=%d\n", g_phy_obj[sel].config->rx_tstate2_fsys_us);
	LOG_MAIN("phy->config->tx_flo=%llu\n", g_phy_obj[sel].config->tx_flo);
	LOG_MAIN("phy->config->tx_twin_fxo=%d\n", g_phy_obj[sel].config->tx_twin_fxo);
	LOG_MAIN("phy->config->tx_tsu_fxo=%d\n", g_phy_obj[sel].config->tx_tsu_fxo);
	LOG_MAIN("phy->config->tx_tstate2_fsys_us=%d\n", g_phy_obj[sel].config->tx_tstate2_fsys_us);
	LOG_MAIN("phy->config->core2_enable=%d\n", g_phy_obj[sel].config->core2_enable);
	LOG_MAIN("phy->config->dll_twin_fxo=%d\n", g_phy_obj[sel].config->dll_twin_fxo);
	LOG_MAIN("phy->config->dll_tsu_fxo=%d\n", g_phy_obj[sel].config->dll_tsu_fxo);
	LOG_MAIN("phy->config->tstate7_fsys_us=%d\n", g_phy_obj[sel].config->tstate7_fsys_us);
	LOG_MAIN("phy->config->rx_bw_cal_flag=%d\n", g_phy_obj[sel].config->rx_bw_cal_flag);
	LOG_MAIN("phy->config->rx_dc_cal_flag=%d\n", g_phy_obj[sel].config->rx_dc_cal_flag);
	LOG_MAIN("phy->config->rx_adc_cal_flag=%d\n", g_phy_obj[sel].config->rx_adc_cal_flag);
	LOG_MAIN("phy->config->rx_qec_flag=%d\n", g_phy_obj[sel].config->rx_qec_flag);
	LOG_MAIN("phy->config->tx_bw_cal_flag=%d\n", g_phy_obj[sel].config->tx_bw_cal_flag);
	LOG_MAIN("phy->config->tx_dc_cal_flag=%d\n", g_phy_obj[sel].config->tx_dc_cal_flag);
	LOG_MAIN("phy->config->tx_dac_cal_flag=%d\n", g_phy_obj[sel].config->tx_dac_cal_flag);
	LOG_MAIN("phy->config->tx_qec_flag=%d\n", g_phy_obj[sel].config->tx_qec_flag);
	LOG_MAIN("phy->config->ldo_cal_flag=%d\n", g_phy_obj[sel].config->ldo_cal_flag);
	LOG_MAIN("phy->config->sx_vco_ldo=%d\n", g_phy_obj[sel].config->sx_vco_ldo);
	LOG_MAIN("phy->config->sxlf_ldo=%d\n", g_phy_obj[sel].config->sxlf_ldo);
	LOG_MAIN("phy->config->syspll_ldo=%d\n", g_phy_obj[sel].config->syspll_ldo);
	LOG_MAIN("phy->config->txabb_ldo=%d\n", g_phy_obj[sel].config->txabb_ldo);
	LOG_MAIN("phy->config->txdac_ldo=%d\n", g_phy_obj[sel].config->txdac_ldo);
	LOG_MAIN("phy->config->txfe_ldo=%d\n", g_phy_obj[sel].config->txfe_ldo);
	LOG_MAIN("phy->config->txsx_lo_ldo=%d\n", g_phy_obj[sel].config->txsx_lo_ldo);
	LOG_MAIN("phy->config->rxadc_ldo=%d\n", g_phy_obj[sel].config->rxadc_ldo);
	LOG_MAIN("phy->config->rxfe_ldo=%d\n", g_phy_obj[sel].config->rxfe_ldo);
	LOG_MAIN("phy->config->rxsx_lo_ldo=%d\n", g_phy_obj[sel].config->rxsx_lo_ldo);
	LOG_MAIN("phy->config->mdig_ldo=%d\n", g_phy_obj[sel].config->mdig_ldo);
	LOG_MAIN("phy->config->sx_cal_flag=%d\n", g_phy_obj[sel].config->sx_cal_flag);
	LOG_MAIN("phy->config->txlo_cal_flag=%d\n", g_phy_obj[sel].config->txlo_cal_flag);
	LOG_MAIN("phy->config->auxadc1_cal_flag=%d\n", g_phy_obj[sel].config->auxadc1_cal_flag);
	LOG_MAIN("phy->config->tx_atten_chn_flag=%d\n", g_phy_obj[sel].config->tx_atten_chn_flag);
	LOG_MAIN("phy->config->wire_ctrl=%d\n", g_phy_obj[sel].config->wire_ctrl);
	LOG_MAIN("phy->config->wire_control_en=%d\n", g_phy_obj[sel].config->wire_control_en);
	LOG_MAIN("phy->config->gain_ctrl_mode=%d\n", g_phy_obj[sel].config->gain_ctrl_mode);
	LOG_MAIN("phy->config->gain_table_mode=%d\n", g_phy_obj[sel].config->gain_table_mode);
	LOG_MAIN("phy->config->gain_ctrl_pin_flag=%d\n", g_phy_obj[sel].config->gain_ctrl_pin_flag);
	LOG_MAIN("phy->config->rxfe_gain[0]=%d\n", g_phy_obj[sel].config->rxfe_gain[0]);
	LOG_MAIN("phy->config->rxfe_gain[1]=%d\n", g_phy_obj[sel].config->rxfe_gain[1]);
	LOG_MAIN("phy->config->bandwidthswitch_flag=%d\n", g_phy_obj[sel].config->bandwidthswitch_flag);
	LOG_MAIN("phy->config->custom_bandwith_flag=%d\n", g_phy_obj[sel].config->custom_bandwith_flag);
	LOG_MAIN("phy->config->custom_bandwith=%d\n", g_phy_obj[sel].config->custom_bandwith);
	LOG_MAIN("phy->config->bb_sample_rate=%d\n", g_phy_obj[sel].config->bb_sample_rate);
	LOG_MAIN("phy->config->lo_change_mode=%d\n", g_phy_obj[sel].config->lo_change_mode);
	LOG_MAIN("phy->config->dac_syspll_lo_div=0x%x\n", g_phy_obj[sel].config->dac_syspll_lo_div);
	LOG_MAIN("phy->config->adc_syspll_lo_div=0x%x\n", g_phy_obj[sel].config->adc_syspll_lo_div);
	LOG_MAIN("phy->config->rx_ext_loop=%d\n", g_phy_obj[sel].config->rx_ext_loop);
	LOG_MAIN("phy->config->tx_ext_loop=%d\n", g_phy_obj[sel].config->tx_ext_loop);
	LOG_MAIN("phy->config->qec_dbfs=%d\n", g_phy_obj[sel].config->qec_dbfs);
	LOG_MAIN("phy->config->lol_dbfs=%d\n", g_phy_obj[sel].config->lol_dbfs);
	LOG_MAIN("phy->config->scap_min=%d\n", g_phy_obj[sel].config->scap_min);
	LOG_MAIN("phy->config->scap_max=%d\n", g_phy_obj[sel].config->scap_max);
	LOG_MAIN("phy->config->scap_cnt=%d\n", g_phy_obj[sel].config->scap_cnt);	
	LOG_MAIN("phy->config->chip_ver=%d\n", g_phy_obj[sel].config->chip_ver);
	LOG_MAIN("phy->config->use_bybrid_mode=%d\n", g_phy_obj[sel].config->use_bybrid_mode);
	LOG_MAIN("phy->config->hybrid_mode=%d\n", g_phy_obj[sel].config->hybrid_mode);
	LOG_MAIN("phy->config->rxgain_force_valid_flag=%d\n", g_phy_obj[sel].config->rxgain_force_valid_flag);
	LOG_MAIN("phy->config->RxDc_Offset_Ver=%d\n", g_phy_obj[sel].config->RxDc_Offset_Ver);
	LOG_MAIN("phy->config->Rx_ImBalance_cal_flag=%d\n", g_phy_obj[sel].config->Rx_ImBalance_cal_flag);
	LOG_MAIN("phy->config->lo_leakage_cal_flag=%d\n", g_phy_obj[sel].config->lo_leakage_cal_flag);
	LOG_MAIN("phy->config->reg901_val=%d\n", g_phy_obj[sel].config->reg901_val);
	LOG_MAIN("phy->config->reg639_620_bit1=%d\n", g_phy_obj[sel].config->reg639_620_bit1);
	LOG_MAIN("phy->config->reg61A_val=%d\n", g_phy_obj[sel].config->reg61A_val);
	LOG_MAIN("phy->config->reg600_val=%d\n", g_phy_obj[sel].config->reg600_val);
	LOG_MAIN("phy->config->reg602_val=%d\n", g_phy_obj[sel].config->reg602_val);
	LOG_MAIN("phy->config->vol_low_limit=%d\n", g_phy_obj[sel].config->vol_low_limit);
	LOG_MAIN("phy->config->vol_up_limit=%d\n", g_phy_obj[sel].config->vol_up_limit);
	LOG_MAIN("phy->config->vol_margin=%d\n", g_phy_obj[sel].config->vol_margin);
	LOG_MAIN("phy->config->trx_lo_cal_mode=%d\n", g_phy_obj[sel].config->trx_lo_cal_mode);
	LOG_MAIN("phy->config->fvco_min=%lld\n", g_phy_obj[sel].config->fvco_min);
	LOG_MAIN("phy->config->lvds_cal_flag=%d\n", g_phy_obj[sel].config->lvds_cal_flag);
	LOG_MAIN("phy->config->syspll_cfg_flag=%d\n", g_phy_obj[sel].config->syspll_cfg_flag);
	LOG_MAIN("phy->config->imb_rx_cfg[0][0]=0x%02x\n", g_phy_obj[sel].config->imb_rx_cfg[0][0]);
	LOG_MAIN("phy->config->imb_rx_cfg[0][1]=0x%02x\n", g_phy_obj[sel].config->imb_rx_cfg[0][1]);
	LOG_MAIN("phy->config->imb_rx_cfg[1][0]=0x%02x\n", g_phy_obj[sel].config->imb_rx_cfg[1][0]);
	LOG_MAIN("phy->config->imb_rx_cfg[1][1]=0x%02x\n", g_phy_obj[sel].config->imb_rx_cfg[1][1]);
	LOG_MAIN("###############chip status###############\n");
	LOG_MAIN("g_phy_obj[%d].tx_bb_gain_config:\n", sel);

    LOG_MAIN("phy->lvds_cal_flag=%d\n", g_phy_obj[sel].lvds_cal_flag);
    LOG_MAIN("phy->lvds_rx=0x%02x\n", g_phy_obj[sel].lvds_rx);
    LOG_MAIN("phy->lvds_tx=0x%02x\n", g_phy_obj[sel].lvds_tx);

    LOG_MAIN("phy->auxadc1_cal_flag=%d\n", g_phy_obj[sel].auxadc1_cal_flag);
    LOG_MAIN("phy->auxadccal_slope=%d\n", g_phy_obj[sel].auxadccal_slope);
    LOG_MAIN("phy->auxadccal_ordinate=%d\n", g_phy_obj[sel].auxadccal_ordinate);

    LOG_MAIN("phy->r_cal_flag=%d\n", g_phy_obj[sel].r_cal_flag);
    LOG_MAIN("phy->r_cal=0x%02x\n", g_phy_obj[sel].r_cal);

    LOG_MAIN("phy->ldo_cal_flag=%d\n", g_phy_obj[sel].ldo_cal_flag);
    for (i=0; i<5; i++)
    {
        LOG_MAIN("0x%02x ", g_phy_obj[sel].ldo_cal[i]);
    }
    LOG_MAIN("0x%02x\n", g_phy_obj[sel].ldo_cal[5] & 0x0F);

    LOG_MAIN("tx_bb_gain_config[50 lines x 16 cols]:\n");
	for (i=0; i<50; i++)
	{
		for (j=0; j<16; j++)
		{
			LOG_MAIN("0x%02x ", g_phy_obj[sel].tx_bb_gain_config[i][j]);
		}
		LOG_MAIN("\n");
	}
	
	LOG_MAIN("phy->tx_dc_cal_flag[0]=%d\n", g_phy_obj[sel].tx_dc_cal_flag[0]);
	LOG_MAIN("phy->tx_dc_cal_flag[1]=%d\n", g_phy_obj[sel].tx_dc_cal_flag[1]);
	for (i=0; i<2; i++)
	{
		LOG_MAIN("channel %d: ", i+1);
		for (j=0; j<4; j++)
		{
			LOG_MAIN("0x%02x ", g_phy_obj[sel].tx_dc_cal[i][j]);
		}
		LOG_MAIN("\n");
	}

	LOG_MAIN("phy->tx_dac_cal_flag[0]=%d\n", g_phy_obj[sel].tx_dac_cal_flag[0]);
	LOG_MAIN("phy->tx_dac_cal_flag[1]=%d\n", g_phy_obj[sel].tx_dac_cal_flag[1]);
	for (i=0; i<2; i++)
	{
		LOG_MAIN("channel %d: ", i+1);
		for (j=0; j<65; j++)
		{
			if ((j != 0) && (j % 4 == 0))
				LOG_MAIN("\n");
			
			LOG_MAIN("[0x%02x 0x%02x] ", g_phy_obj[sel].tx_dac_cal[i][j*2], g_phy_obj[sel].tx_dac_cal[i][j*2+1]);
		}
		LOG_MAIN("\n");
	}
	
	LOG_MAIN("phy->rx_dc_lut_update=%d\n", g_phy_obj[sel].rx_dc_lut_update);
	LOG_MAIN("phy->rx_dc_cal_flag[0]=%d\n", g_phy_obj[sel].rx_dc_cal_flag[0]);
	LOG_MAIN("phy->rx_dc_cal_flag[1]=%d\n", g_phy_obj[sel].rx_dc_cal_flag[1]);
	for (i=0; i<2; i++)
	{
		LOG_MAIN("channel %d, tia:\n", i+1);
		for (j=0; j<12; j++)
		{
			if ((j != 0) && (j % 2 == 0))
				LOG_MAIN("\n");
			LOG_MAIN("0x%02x ", g_phy_obj[sel].tia[i][j]);
		}
		LOG_MAIN("\n");
	}

	LOG_MAIN("phy->bq_rx1: \n");
	for (i=0; i<6; i++)
	{
		LOG_MAIN("RX1 PORT G%d:\n", i);
		for (j=0; j<26; j++)
		{
			if ((j != 0) && (j % 2 == 0))
				LOG_MAIN("\n");
			LOG_MAIN("0x%02x ", g_phy_obj[sel].bq_rx1[i][j]);
		}
		LOG_MAIN("\n");
	}

	LOG_MAIN("phy->bq_rx2: \n");
	for (i=0; i<6; i++)
	{
		LOG_MAIN("RX2 PORT G%d:\n", i);
		for (j=0; j<26; j++)
		{
			if ((j != 0) && (j % 2 == 0))
				LOG_MAIN("\n");
			LOG_MAIN("0x%02x ", g_phy_obj[sel].bq_rx2[i][j]);
		}
		LOG_MAIN("\n");
	}

	LOG_MAIN("phy->rx_bw_cal_flag[0]=%d\n", g_phy_obj[sel].rx_bw_cal_flag[0]);
	LOG_MAIN("phy->rx_bw_cal_flag[1]=%d\n", g_phy_obj[sel].rx_bw_cal_flag[1]);
	for (i=0; i<2; i++)
	{
		LOG_MAIN("channel %d, rx_imbalance_cal:", i+1);
		for (j=0; j<2; j++)
		{
			LOG_MAIN("0x%02x ", g_phy_obj[sel].rx_imbalance_cal[i][j]);
		}
		LOG_MAIN("\n");

		LOG_MAIN("channel %d, rx_bw_cal:", i+1);
		for (j=0; j<2; j++)
		{
			LOG_MAIN("0x%02x ", g_phy_obj[sel].rx_bw_cal[i][j]);
		}
		LOG_MAIN("\n");
	}

	LOG_MAIN("phy->tx_qec_flag[0]=%d\n", g_phy_obj[sel].tx_qec_flag[0]);
	LOG_MAIN("phy->tx_qec_flag[1]=%d\n", g_phy_obj[sel].tx_qec_flag[1]);
	LOG_MAIN("phy->rx_qec_flag[0]=%d\n", g_phy_obj[sel].rx_qec_flag[0]);
	LOG_MAIN("phy->rx_qec_flag[1]=%d\n", g_phy_obj[sel].rx_qec_flag[1]);
	for (i=0; i<2; i++)
	{
		for (j=0; j<6; j++)
			LOG_MAIN("channel%d, bb_index=%d, dc_i=%d, dc_q=%d, fiiq_real=%d, fiiq_imag=%d\n", i, j, 
			     g_phy_obj[sel].tx_qec[i][j].dc_i, g_phy_obj[sel].tx_qec[i][j].dc_q,
			     g_phy_obj[sel].tx_qec[i][j].fiiq_real, g_phy_obj[sel].tx_qec[i][j].fiiq_imag);
		
		LOG_MAIN("channel%d, fiiq_real=%d, fiiq_imag=%d, mag_ratio_adj=%lf, ph_error_adj=%lf\n", i,
			 g_phy_obj[sel].rx_qec[i].fiiq_real, g_phy_obj[sel].rx_qec[i].fiiq_imag,
			 g_phy_obj[sel].rx_qec[i].mag_ratio_adj, g_phy_obj[sel].rx_qec[i].ph_error_adj);
		for (j=0; j<32; j++)
			LOG_MAIN("channel%d, fdiq_fir[%d]=%d\n", i, j, g_phy_obj[sel].rx_qec[i].fdiq_fir[j]);
	}

	LOG_MAIN("phy->sx_cal_flag=%d\n", g_phy_obj[sel].sx_cal_flag);
	for (i=0; i<512; i++)
		LOG_MAIN("0x%02x, 0x%02x\n", g_phy_obj[sel].sx_cal_1[i], g_phy_obj[sel].sx_cal_2[i]);

	LOG_MAIN("phy->tx_atten[0]=%d\n", g_phy_obj[sel].tx_atten[0]);
	LOG_MAIN("phy->tx_atten[1]=%d\n", g_phy_obj[sel].tx_atten[1]);
	LOG_MAIN("phy->tx_curr_index=%d\n", g_phy_obj[sel].tx_curr_index);
	LOG_MAIN("phy->rx_lmt_gain[0]=%d\n", g_phy_obj[sel].rx_lmt_gain[0]);
	LOG_MAIN("phy->rx_lmt_gain[1]=%d\n", g_phy_obj[sel].rx_lmt_gain[1]);
	LOG_MAIN("phy->rx_lpf_gain[0]=%d\n", g_phy_obj[sel].rx_lpf_gain[0]);
	LOG_MAIN("phy->rx_lpf_gain[1]=%d\n", g_phy_obj[sel].rx_lpf_gain[1]);
	LOG_MAIN("phy->rx_dig_gain[0]=%d\n", g_phy_obj[sel].rx_dig_gain[0]);
	LOG_MAIN("phy->rx_dig_gain[1]=%d\n", g_phy_obj[sel].rx_dig_gain[1]);

	LOG_MAIN("phy->bandwith=%d\n", g_phy_obj[sel].bandwith);
	LOG_MAIN("phy->txlo=%llu\n", g_phy_obj[sel].txlo);
	LOG_MAIN("phy->rxlo=%llu\n", g_phy_obj[sel].rxlo);

	LOG_MAIN("phy->module_debug=%d\n", g_phy_obj[sel].module_debug);
	LOG_MAIN("phy->debug_on=%d\n", g_phy_obj[sel].debug_on);
	LOG_MAIN("phy->loading_lut=%d\n", g_phy_obj[sel].loading_lut);
	LOG_MAIN("phy->init_flag=%d\n", g_phy_obj[sel].init_flag);
	LOG_MAIN("phy->spi=%d\n", g_phy_obj[sel].spi);
	LOG_MAIN("phy->st=%d\n", g_phy_obj[sel].st);
	LOG_MAIN("phy->error=%d\n", g_phy_obj[sel].error);
	LOG_MAIN("phy->min_vco=%llu\n", g_phy_obj[sel].min_vco);
}

int cmd_api_intemp_get(void)
{
	int ret;
	ret = chip_temp_get(&g_phy_obj[g_phy_select]);
	LOG_MAIN("temp: %d\n", ret);
	return ret;
}

int cmd_api_extpin_vol_get(void)
{
	int ret;
	ret = extpin_voltage_read(&g_phy_obj[g_phy_select]);
	LOG_MAIN("extpin vol: %d (mv)\n", ret);
	return ret;
}

short cmd_api_lock_status(short dir)
{
	short ret=0;
    int Voltage = 0;
	int vctrl=0;

	if (dir < 2)
	{
		if (sxtrx_lock_status(&g_phy_obj[g_phy_select], dir, &Voltage) < 0)
		{
			LOG_MAIN("lock status: failed! VolLowLimit=%dmV, VolUpLimit=%dmV, VolMargin=%dmV, Voltage=%dmV\n", 
						g_phy_obj[g_phy_select].config->vol_low_limit, g_phy_obj[g_phy_select].config->vol_up_limit, g_phy_obj[g_phy_select].config->vol_margin, Voltage);
			ret = -1;
		}
		else
		{
			LOG_MAIN("lock status: ok! VolLowLimit=%dmV, VolUpLimit=%dmV, VolMargin=%dmV, Voltage=%dmV\n", 
						g_phy_obj[g_phy_select].config->vol_low_limit, g_phy_obj[g_phy_select].config->vol_up_limit, g_phy_obj[g_phy_select].config->vol_margin, Voltage);
			ret = 0;
		}
	}
	else if (dir == 2)
	{
		vctrl = get_syspll_status(&g_phy_obj[g_phy_select]);
		LOG_MAIN("sys pll Vctrl: %d\n", vctrl);
		if ((vctrl > 300) && (vctrl < 900))
		{
			LOG_MAIN("sys pll status: %d, OK\n", vctrl);
			ret = 0;
		}
		else
		{
			LOG_MAIN("sys pll status: %d, ERR\n", vctrl);
			ret = -1;
		}
	}

	return ret;
}

void cmd_api_fir_dump(rf_chip_phy_t *phy,bool rx_select)
{

	unsigned short reg_0xf8_high_hex=0x20;	
	unsigned int I_data[1024]={0};
	unsigned int Q_data[1024]={0};
	unsigned short addr=0;
	unsigned short reg_rx_sel;
	if(rx_select==0)
		reg_rx_sel=0x0f8;
	else
		reg_rx_sel=0x0f9;

	hal_spi_write_reg(phy,reg_rx_sel,0x10);
	hal_spi_write_reg(phy,reg_rx_sel,reg_0xf8_high_hex|0x00);
	hal_spi_write_reg(phy,reg_rx_sel,reg_0xf8_high_hex|0x04);
	hal_spi_write_reg(phy,reg_rx_sel,reg_0xf8_high_hex|0x00);

	//adc reset 
	hal_spi_write_reg(phy,reg_rx_sel,reg_0xf8_high_hex|0x08);
	CHIP_UDELAY(2);
	hal_spi_write_reg(phy,reg_rx_sel,reg_0xf8_high_hex|0x00);

	//wait 2us
	CHIP_UDELAY(2);
	hal_spi_write_reg(phy,reg_rx_sel,reg_0xf8_high_hex|0x04);
	hal_spi_write_reg(phy,reg_rx_sel,reg_0xf8_high_hex|0x00);

	//I Branch

	for(addr=0;addr<1024;addr++)
	{
		hal_spi_write_reg(phy,reg_rx_sel,reg_0xf8_high_hex|0x04);
		hal_spi_write_reg(phy,0x00b6,addr>>8);
		hal_spi_write_reg(phy,0x00b7,addr&0xff);

		hal_spi_write_reg(phy,reg_rx_sel,reg_0xf8_high_hex|0x0c);

		//read I data
		char flg_ok=0;
		while(flg_ok==0)
		{
			unsigned char result = hal_spi_read_reg(phy,0x00be);
			I_data[addr]=result*256;
			flg_ok=1;
		}

		flg_ok=0;
		while(flg_ok==0)
		{
			unsigned char result = hal_spi_read_reg(phy,0x00bf);
			I_data[addr]=I_data[addr]+result;
			flg_ok=1;
		}

		hal_fpga_write_reg(phy,reg_rx_sel,reg_0xf8_high_hex|0x04);
		if(I_data[addr]>=32768)
			I_data[addr]=I_data[addr]-65536;
		LOG_MAIN("FIR I_data[%d]=%d\n",addr,I_data[addr]);
	}

	//Q Branch
	for(addr=1024;addr<2;addr++)
	{
		hal_spi_write_reg(phy,reg_rx_sel,reg_0xf8_high_hex|0x04);

		hal_spi_write_reg(phy,0x00b6,addr>>8);
		hal_spi_write_reg(phy,0x00b7,addr&0xff);

		hal_spi_write_reg(phy,reg_rx_sel,reg_0xf8_high_hex|0x0c);

		//read Q data
		char flg_ok=0;
		while(flg_ok==0)
		{
			unsigned char result = hal_spi_read_reg(phy,0x00be);
			Q_data[addr-1023]=result*256;
			flg_ok=1;
		}

		flg_ok=0;
		while(flg_ok==0)
		{
			unsigned char result = hal_spi_read_reg(phy,0x00bf);
			Q_data[addr-1023]=Q_data[addr-1023]+result;
			flg_ok=1;
		}

		hal_fpga_write_reg(phy,reg_rx_sel,reg_0xf8_high_hex|0x04);
		if(Q_data[addr-1023]>=32768)
			Q_data[addr-1023]=Q_data[addr-1023]-65536;
		LOG_MAIN("FIR Q_data[%d]=%d\n",addr-1023,Q_data[addr-1023]);
	}

	//final step
	hal_spi_write_reg(phy,reg_rx_sel,reg_0xf8_high_hex|0x00);

}

static void I_Q_DFT_Print( int * I_data, int * Q_data,double adc_fs)
{
	int i=0,fre=0;
	double dft_mag[1024]={0};
	double dft_i[1024]={0};
	double dft_q[1024]={0};
	double PI=3.141592653589793;
	for(fre=0;fre<1024;fre++)
	{
		dft_i[fre]=0;
		dft_q[fre]=0;
		for(i=0;i<1024;i++)
		{
			dft_i[fre] =  dft_i[fre] +(double)I_data[i]*cos(2*PI*i*fre/1024) - (double)Q_data[i]*sin(2*PI*i*fre/1024);
        	dft_q[fre] =  dft_q[fre] -(double)I_data[i]*sin(2*PI*i*fre/1024) - (double)Q_data[i]*cos(2*PI*i*fre/1024);
		}
		dft_i[fre]=dft_i[fre]/512.0/1024.0;
		dft_q[fre]=dft_i[fre]/512.0/1024.0;
		dft_mag[fre]=(double)10.0*log10(dft_i[fre]*dft_i[fre]+dft_q[fre]*dft_q[fre]);
	}
	LOG_MAIN("INDEX		frequence		I_data			Q_data		FFT_MAG\n");
	for(i=0;i<512;i++)
	{
		int fre_point = (int)((double)adc_fs * (double)(i-512) / 1024.0);
		LOG_MAIN("%8d	%9d		%8d		%8d		%.6f \n",i,fre_point,I_data[i],Q_data[i],dft_mag[i+512]);	
	}
	for(i=512;i<1024;i++)
	{
		int fre_point = (int)((double)adc_fs * (double)(i-512) / 1024.0+0.5);
		LOG_MAIN("%8d	%9d		%8d		%8d		%.6f \n",i,fre_point,I_data[i],Q_data[i],dft_mag[i-512]);	
	}
}

void cmd_api_adc_ram_dump(TRX_CHN_ENUM chn, char *name)
{
	#if HAVE_FS
	if(g_phy_obj[g_phy_select].config->chip_ver< 2)
	{
		unsigned int  reg_address, reg_w_value, reg_r_value1, reg_r_value2, cat_val;
		unsigned short adc_init[4][2] = 
		{
			{0x0f8, 0x10},
			{0x0f8, 0x50},
			{0x0f8, 0x54},
			{0x0f8, 0x50},
		};
		unsigned short adc_reset[4][2] = 
		{
			{0x0f8, 0x58},
			{0x0f8, 0x50},
			{0x0f8, 0x54},
			{0x0f8, 0x50},
		};
		int i, j, cat_val_sign, tmp_int;
		char convert_result[17], fullN[4][200];
		FILE *fp_i_dec, *fp_q_dec, *fp_i_bin, *fp_q_bin;

		LOG_MAIN("\n\nreading adc ram data...\n");
		memset(fullN, 0, sizeof(fullN));
		sprintf(fullN[0], "%s_I_DEC.txt", name);
		sprintf(fullN[1], "%s_Q_DEC.txt", name);
		sprintf(fullN[2], "%s_I_BIN.txt", name);
		sprintf(fullN[3], "%s_Q_BIN.txt", name);
		remove(fullN[0]);
		remove(fullN[1]);
		remove(fullN[2]);
		remove(fullN[3]);

		if((fp_i_dec = fopen(fullN[0],"w")) == NULL)
		{
		LOG_ERROR("Failed to Open File For I_DEC\n");
		return;
		}

		if((fp_q_dec = fopen(fullN[1],"w")) == NULL)
		{
		LOG_ERROR("Failed to Open File For Q_DEC\n");
		return;
		}

		if((fp_i_bin = fopen(fullN[2],"w")) == NULL)
		{
		LOG_ERROR("Failed to Open File For I_BIN\n");
		return;
		}

		if((fp_q_bin = fopen(fullN[3],"w")) == NULL)
		{
		LOG_ERROR("Failed to Open File For Q_BIN\n");
		return;
		}

		//step1, adc init
		for (i=0; i<4; i++)
		hal_spi_write_reg(&g_phy_obj[g_phy_select], chn? (adc_init[i][0] +1):(adc_init[i][0]), adc_init[i][1]);

		//step2, adc reset
		for (i=0; i<4; i++)
		hal_spi_write_reg(&g_phy_obj[g_phy_select], chn? (adc_reset[i][0]+1):(adc_reset[i][0]), adc_reset[i][1]);

		//step3, read 0 to 1023 for I data
		for (i=0; i<1024; i++)
		{
		//step3.1, adc set
		hal_spi_write_reg(&g_phy_obj[g_phy_select], chn?0x0f9:0x0f8, 0x54);
		
		//step3.2, addr set
		hal_spi_write_reg(&g_phy_obj[g_phy_select], 0x0b6, (i >> 8) & 0xff);
		hal_spi_write_reg(&g_phy_obj[g_phy_select], 0x0b7, i & 0xff);
		
		//step3.3, adc set
		hal_spi_write_reg(&g_phy_obj[g_phy_select], chn?0x0f9:0x0f8, 0x5C);
		
		//step3.4, read
		reg_r_value1 = hal_spi_read_reg(&g_phy_obj[g_phy_select], 0x0be);//h
		reg_r_value2 = hal_spi_read_reg(&g_phy_obj[g_phy_select], 0x0bf);//l
		
		//step3.5, adc set
		hal_spi_write_reg(&g_phy_obj[g_phy_select], chn?0x0f9:0x0f8, 0x54);

		//step3.6, data
		cat_val = ((reg_r_value1 & 0xff) << 8) | (reg_r_value2 & 0xff);
		cat_val_sign = cat_val;
		if (cat_val >= 32768)
			cat_val_sign = cat_val_sign - 65536;
		fprintf(fp_i_dec, "%d\n", cat_val_sign);

		cat_val = cat_val & 0xffff;
		M16bithex2bin(cat_val, convert_result);
		convert_result[16] = 0;
		fprintf(fp_i_bin, "%s\n", convert_result);
		}

		//step4, read 1024 to 2047 for Q data
		for (i=1024; i<2048; i++)
		{
		//step3.1, adc set
		hal_spi_write_reg(&g_phy_obj[g_phy_select], chn?0x0f9:0x0f8, 0x54);
		
		//step3.2, addr set
		hal_spi_write_reg(&g_phy_obj[g_phy_select], 0x0b6, (i >> 8) & 0xff);
		hal_spi_write_reg(&g_phy_obj[g_phy_select], 0x0b7, i & 0xff);
		
		//step3.3, adc set
		hal_spi_write_reg(&g_phy_obj[g_phy_select], chn?0x0f9:0x0f8, 0x5C);
		
		//step3.4, read
		reg_r_value1 = hal_spi_read_reg(&g_phy_obj[g_phy_select], 0x0be);//h
		reg_r_value2 = hal_spi_read_reg(&g_phy_obj[g_phy_select], 0x0bf);//l
		
		//step3.5, adc set
		hal_spi_write_reg(&g_phy_obj[g_phy_select], chn?0x0f9:0x0f8, 0x54);
		
		//step3.6, data
		cat_val = ((reg_r_value1 & 0xff) << 8) | (reg_r_value2 & 0xff);
		cat_val_sign = cat_val;
		if (cat_val >= 32768)
			cat_val_sign = cat_val_sign - 65536;
		fprintf(fp_q_dec, "%d\n", cat_val_sign);

		cat_val = cat_val & 0xffff;
		M16bithex2bin(cat_val, convert_result);
		convert_result[16] = 0;
		fprintf(fp_q_bin, "%s\n", convert_result);
		}

		LOG_MAIN("finished!\n");

		fclose(fp_i_dec);
		fclose(fp_q_dec);
		fclose(fp_i_bin);
		fclose(fp_q_bin);

	}
	else if(g_phy_obj[g_phy_select].config->chip_ver == 2)
	{
		char convert_result[17], fullN[4][200];
		FILE *fp_i_dec, *fp_q_dec, *fp_i_bin, *fp_q_bin;
		int addr=0;

		LOG_MAIN("\n\nreading adc ram data...\n");
		memset(fullN, 0, sizeof(fullN));
		sprintf(fullN[0], "%s_I_DEC.txt", name);
		sprintf(fullN[1], "%s_Q_DEC.txt", name);
		sprintf(fullN[2], "%s_I_BIN.txt", name);
		sprintf(fullN[3], "%s_Q_BIN.txt", name);
		remove(fullN[0]);
		remove(fullN[1]);
		remove(fullN[2]);
		remove(fullN[3]);

		if((fp_i_dec = fopen(fullN[0],"w")) == NULL)
		{
		LOG_ERROR("Failed to Open File For I_DEC\n");
		return;
		}

		if((fp_q_dec = fopen(fullN[1],"w")) == NULL)
		{
		LOG_ERROR("Failed to Open File For Q_DEC\n");
		return;
		}

		if((fp_i_bin = fopen(fullN[2],"w")) == NULL)
		{
		LOG_ERROR("Failed to Open File For I_BIN\n");
		return;
		}

		if((fp_q_bin = fopen(fullN[3],"w")) == NULL)
		{
		LOG_ERROR("Failed to Open File For Q_BIN\n");
		return;
		}

		LOG_MAIN("\n\nreading adc ram data...\n");

		hal_spi_write_reg(&g_phy_obj[g_phy_select], 0x14E, 0x00);

		hal_spi_write_reg(&g_phy_obj[g_phy_select], 0x0f8, 0x00);
		hal_spi_write_reg(&g_phy_obj[g_phy_select], 0x0f8, 0x10);

		hal_spi_write_reg(&g_phy_obj[g_phy_select], 0x0f8, 0x10);
		hal_spi_write_reg(&g_phy_obj[g_phy_select], 0x0f8, 0x14);
		hal_spi_write_reg(&g_phy_obj[g_phy_select], 0x0f8, 0x10);

		hal_spi_write_reg(&g_phy_obj[g_phy_select], 0x0f8, 0x10);
		hal_spi_write_reg(&g_phy_obj[g_phy_select], 0x0f8, 0x18);
		CHIP_DELAY(10);
		hal_spi_write_reg(&g_phy_obj[g_phy_select], 0x0f8, 0x10);
		hal_spi_write_reg(&g_phy_obj[g_phy_select], 0x0f8, 0x00);
		for( addr =128;addr<1023;addr++)
		{
			unsigned short ram_rdata_i=0,ram_rdata_q=0;
			hal_spi_write_reg(&g_phy_obj[g_phy_select], 0x0f8, 0x04);

			hal_spi_write_reg(&g_phy_obj[g_phy_select], 0x00b6, (addr>>8)&0x07);
			hal_spi_write_reg(&g_phy_obj[g_phy_select], 0x00b7, addr&0xff);

			hal_spi_write_reg(&g_phy_obj[g_phy_select], 0x0f8, 0x0c);

			//read I data
			ram_rdata_i = hal_spi_read_reg(&g_phy_obj[g_phy_select], 0x00b7);
			ram_rdata_i = ram_rdata_i<<8;
			ram_rdata_i |= hal_spi_read_reg(&g_phy_obj[g_phy_select], 0x00b6);

			fprintf(fp_i_dec, "%d\n", ram_rdata_i);
            ram_rdata_i = ram_rdata_i & 0xffff;
		    M16bithex2bin(ram_rdata_i, convert_result);
		    convert_result[16] = 0;
			fprintf(fp_i_bin, "%s\n", convert_result);


			hal_spi_write_reg(&g_phy_obj[g_phy_select], 0x0f8, 0x04);

			hal_spi_write_reg(&g_phy_obj[g_phy_select], 0x00b6, (addr>>8)&0x03);
			hal_spi_write_reg(&g_phy_obj[g_phy_select], 0x00b7, addr&0xff);

			hal_spi_write_reg(&g_phy_obj[g_phy_select], 0x0f8, 0x0c);

			//read Q data
			ram_rdata_q = hal_spi_read_reg(&g_phy_obj[g_phy_select], 0x00bf);
			ram_rdata_q = ram_rdata_q<<8;
			ram_rdata_q |= hal_spi_read_reg(&g_phy_obj[g_phy_select], 0x00be);

			fprintf(fp_q_dec, "%d\n", ram_rdata_q);
            ram_rdata_q = ram_rdata_q & 0xffff;
		    M16bithex2bin(ram_rdata_q, convert_result);
		    convert_result[16] = 0;
			fprintf(fp_q_bin, "%s\n", convert_result);
			hal_spi_write_reg(&g_phy_obj[g_phy_select], 0x0f8, 0x04);

		}
		fclose(fp_i_dec);
		fclose(fp_q_dec);
		fclose(fp_i_bin);
		fclose(fp_q_bin);

		LOG_MAIN("finished!\n");
	}
	#else
    unsigned int  reg_address, reg_w_value, reg_r_value1, reg_r_value2, cat_val;
	//int I_data[1024]={0},Q_data[1024]={0};
    unsigned short adc_init[4][2] = 
    {
        {0x0f8, 0x10},
        {0x0f8, 0x50},
        {0x0f8, 0x54},
        {0x0f8, 0x50},
    };
    unsigned short adc_reset[4][2] = 
    {
        {0x0f8, 0x58},
        {0x0f8, 0x50},
        {0x0f8, 0x54},
        {0x0f8, 0x50},
    };
    int i, j, cat_val_sign, tmp_int;
    char convert_result[17], fullN[4][200];

    LOG_MAIN("\n\nreading adc ram data...\n");

    LOG_MAIN("______________________I_PATH______________________\n");
    //step1, adc init
    for (i=0; i<4; i++)
    hal_spi_write_reg(&g_phy_obj[g_phy_select], chn? (adc_init[i][0] +1):(adc_init[i][0]), adc_init[i][1]);

    //step2, adc reset
    for (i=0; i<4; i++)
    hal_spi_write_reg(&g_phy_obj[g_phy_select], chn? (adc_reset[i][0]+1):(adc_reset[i][0]), adc_reset[i][1]);

    //step3, read 0 to 1023 for I data
    for (i=0; i<1024; i++)
    {
    //step3.1, adc set
    hal_spi_write_reg(&g_phy_obj[g_phy_select], chn?0x0f9:0x0f8, 0x54);
    
    //step3.2, addr set
    hal_spi_write_reg(&g_phy_obj[g_phy_select], 0x0b6, (i >> 8) & 0xff);
    hal_spi_write_reg(&g_phy_obj[g_phy_select], 0x0b7, i & 0xff);
    
    //step3.3, adc set
    hal_spi_write_reg(&g_phy_obj[g_phy_select], chn?0x0f9:0x0f8, 0x5C);
    
    //step3.4, read
    reg_r_value1 = hal_spi_read_reg(&g_phy_obj[g_phy_select], 0x0be);//h
    reg_r_value2 = hal_spi_read_reg(&g_phy_obj[g_phy_select], 0x0bf);//l
    
    //step3.5, adc set
    hal_spi_write_reg(&g_phy_obj[g_phy_select], chn?0x0f9:0x0f8, 0x54);

    //step3.6, data
    cat_val = ((reg_r_value1 & 0xff) << 8) | (reg_r_value2 & 0xff);
    cat_val_sign = cat_val;
    if (cat_val >= 32768)
        cat_val_sign = cat_val_sign - 65536;
    LOG_MAIN("%d\n",cat_val_sign);
	//I_data[i]=cat_val_sign;
    }
    LOG_MAIN("______________________Q_PATH______________________\n");
    //step4, read 1024 to 2047 for Q data
    for (i=1024; i<2048; i++)
    {
    //step3.1, adc set
    hal_spi_write_reg(&g_phy_obj[g_phy_select], chn?0x0f9:0x0f8, 0x54);
    
    //step3.2, addr set
    hal_spi_write_reg(&g_phy_obj[g_phy_select], 0x0b6, (i >> 8) & 0xff);
    hal_spi_write_reg(&g_phy_obj[g_phy_select], 0x0b7, i & 0xff);
    
    //step3.3, adc set
    hal_spi_write_reg(&g_phy_obj[g_phy_select], chn?0x0f9:0x0f8, 0x5C);
    
    //step3.4, read
    reg_r_value1 = hal_spi_read_reg(&g_phy_obj[g_phy_select], 0x0be);//h
    reg_r_value2 = hal_spi_read_reg(&g_phy_obj[g_phy_select], 0x0bf);//l
    
    //step3.5, adc set
    hal_spi_write_reg(&g_phy_obj[g_phy_select], chn?0x0f9:0x0f8, 0x54);
    
    //step3.6, data
    cat_val = ((reg_r_value1 & 0xff) << 8) | (reg_r_value2 & 0xff);
    cat_val_sign = cat_val;
    if (cat_val >= 32768)
        cat_val_sign = cat_val_sign - 65536;
    LOG_MAIN("%d\n",cat_val_sign);
	//Q_data[i-1024]=cat_val_sign;
    }
	//I_Q_DFT_Print(I_data,Q_data,adc_fs);

    LOG_MAIN("finished!\n");
	#endif

}


void cmd_api_rpt_get(TRX_CHN_ENUM chn)
{
	GCTRL_GET_RPT_T cfg;
	GCTRL_GET_RPT(&g_phy_obj[g_phy_select], chn, &cfg);
	//LOG_MAIN("lna_index=%d,rpt_lmt_index=%d, rpt_lpf_index=%d,rpt_dig_index=%d, rpt_full_index=%d \n", 
	//		cfg.rpt_lna_index, cfg.rpt_lmt_index, cfg.rpt_lpf_index, cfg.rpt_dig_index, cfg.rpt_full_index);

#if 1
	LOG_MAIN("======================= Cliker on GCTRL report =====================\n");
	LOG_MAIN("rpt_lmt_overload			 : %d\n",cfg.rpt_lmt_flg_overload);
	LOG_MAIN("rpt_lmt_prevent_inc		 : %d\n",cfg.rpt_lmt_flg_prevent_inc);
	LOG_MAIN("rpt_lmt_underload 		 : %d\n",cfg.rpt_lmt_flg_underload);
	LOG_MAIN("rpt_adc_overload_lg		 : %d\n",cfg.rpt_adc_flg_overload_lg);
	LOG_MAIN("rpt_adc_overload_sm		 : %d\n",cfg.rpt_adc_flg_overload_sm);
	LOG_MAIN("rpt_adc_prevent_inc		 : %d\n",cfg.rpt_adc_flg_prevent_inc);
	LOG_MAIN("rpt_adc_underload 		 : %d\n",cfg.rpt_adc_flg_underload);
	LOG_MAIN("rpt_hbf_flg_ovf			 : %d\n",cfg.rpt_hbf_flg_overflow);
	LOG_MAIN("rpt_hbf_low_power 		 : %d\n",cfg.rpt_hbf_flg_low_power);
	LOG_MAIN("rpt_lmt_value 			 : %d\n",cfg.rpt_lmt_pdt_value);
	LOG_MAIN("rpt_hbf_average_power 	 : %d | %.1f\n",cfg.rpt_hbf_avp,-1*((float)cfg.rpt_hbf_avp)/2.0);
	LOG_MAIN("rpt_lna_index : %d | rpt_lmt_index : %d | rpt_lpf_index : %d\n",
				cfg.rpt_lna_index,cfg.rpt_lmt_index,cfg.rpt_lpf_index);
	LOG_MAIN("rpt_full_index : %d\n",cfg.rpt_full_index);
#endif
}

void cmd_api_agc_mode_set(TRX_CHN_ENUM chn, short gain_mode, short tab_mode)
{
	LOG_MAIN("chn:%d, tab_mode:%d, gain_mode:%d\r\n", chn, tab_mode, gain_mode);
	gctrl_set_mode(&g_phy_obj[g_phy_select], chn, gain_mode, tab_mode);
}

void cmd_api_rx_gain_full_tab_init(TRX_CHN_ENUM chn, short init_index, short max_index)
{
	int addr;
	short i;
	short gain_inx;	
	short ex_lna;
	short lmt_inx;
	short lpf_inx;
	GCTRL_FULL_TABLE_CFG_T tab;
	
	LOG_MAIN("chn:%d, tab_init_inx:%d, tab_max_inx:%d\n", chn, init_index, max_index);
	tab.init_index = init_index;
	tab.max_index = max_index;
	for (i=0; i<max_index; i++)
		tab.content[i] = ((g_rx_gain_full_table[i][0] << 7) | (g_rx_gain_full_table[i][1] << 4) | g_rx_gain_full_table[i][2]);
	GCTRL_FULL_TABLE_CFG(&g_phy_obj[g_phy_select], chn,  &tab);

	LOG_MAIN("Dump full tab start\n");
	for(addr = 0; addr <= max_index; addr++)
	{
		GCTRL_FULL_TABLE_RPT_CONTENT(&g_phy_obj[g_phy_select], chn, addr, &gain_inx);
		ex_lna = ((gain_inx & 0x80) >> 7);
		lmt_inx = ((gain_inx & 0x70) >> 4);
		lpf_inx = (gain_inx & 0x0f);
		LOG_MAIN("addr:%d, ex_lna:%d, lmt_inx:%d, lpf_inx:%d\n", addr, ex_lna, lmt_inx, lpf_inx);
	}
	LOG_MAIN("Dump full tab end\n");
}

void cmd_api_rx_gain_mgc_full_tab_index_set(TRX_CHN_ENUM chn, short full_tab_inx)
{
	LOG_MAIN("chn:%d, full_table_inx:%d\r\n", chn, full_tab_inx);
	GCTRL_MGC_SET_FULL_GAIN(&g_phy_obj[g_phy_select], chn, full_tab_inx);
}


void cmd_api_tx_qec_gain_set(TRX_CHN_ENUM chn, short val)
{
    TX_QEC_CFG_REGS  tx_qec_cfg ;
    LOG_MAIN("Tx qec gain set chn:%d, val:%d add %f db\r\n", chn, val,(double)val/-2.0);
    //step:0.5db size:-12~128  -12==>6db  128==>-64db
    fn_tx_qec_gain_set(&g_phy_obj[g_phy_select], chn,&tx_qec_cfg, val);
}

void cmd_api_chip_ver_select(char sel)
{
	LOG_MAIN("chip ver: %d\n", sel);
	g_phy_obj[g_phy_select].config->chip_ver = sel;
}

void cmd_api_print_cmd(void)
{
	int i, n=sizeof(g_cmd_str) / 50;
	LOG_MAIN("---------------------------------------\n");
	for (i=0; i<n; i++)
		LOG_MAIN("%s\n", g_cmd_str[i]);
	LOG_MAIN("---------------------------------------\n");
}


static int run_cmd(int argc, char *argv[])
{
	int cnt, i;

	cnt = sizeof(g_full_cmds) / sizeof(g_full_cmds[0]);
	for (i=0; i<cnt; i++)
	{
		if ((strcmp(g_full_cmds[i].cmd, argv[1])==0) && (g_full_cmds[i].param==(argc-2)))
		{
			//LOG_MAIN("start to run cmd...\n");
			g_full_cmds[i].fun(argv);
			//LOG_MAIN("finish cmd!\n");
			return 0;
		}
	}

	return -1;
}

static int read_config_from_file(void)
{
#if HAVE_FS
	unsigned char *config;
	int fd, i, j, size;
	
	//open file
	fd = open("./gc080x_config", O_RDONLY);
	if (fd < 0)
	{
		for (i=0; i<RF_PHY_NUMBER; i++)
			g_phy_obj[i].init_flag = 0;
		LOG_MAIN("read chip config file failed, we use default config.\n");
		return -1;
	}

	//read file
	size = RF_PHY_NUMBER*(sizeof(rf_chip_phy_t)+sizeof(chip_config_t)) + 1;
	config = (unsigned char *) malloc (size);
	memset(config, 0, size);
	read(fd, config, size);
	memcpy(g_phy_obj, config, RF_PHY_NUMBER*sizeof(rf_chip_phy_t));
	memcpy(g_phy_config, config+RF_PHY_NUMBER*sizeof(rf_chip_phy_t), RF_PHY_NUMBER*sizeof(chip_config_t));
	g_phy_select = config[size-1];

	for (i=0; i<RF_PHY_NUMBER; i++)
		g_phy_obj[i].config = &g_phy_config[i];

	//clsoe
	close(fd);
	free(config);
#endif
	
	return 0;
}

static int write_config_to_file(void)
{
#if HAVE_FS
	int fd, i, j, size;
	unsigned char *config;
	
	//open file
	fd = open("./gc080x_config", O_RDWR | O_CREAT, 0777);
	if (fd < 0)
	{
		LOG_MAIN("create config file failed, we use default config.\n");
		return - 1;
	}

	//write file
	size = RF_PHY_NUMBER*(sizeof(rf_chip_phy_t)+sizeof(chip_config_t)) + 1;
	config = (unsigned char *) malloc (size);
	memset(config, 0, size);
	memcpy(config, g_phy_obj, RF_PHY_NUMBER*sizeof(rf_chip_phy_t));
	memcpy(config+RF_PHY_NUMBER*sizeof(rf_chip_phy_t), g_phy_config, RF_PHY_NUMBER*sizeof(chip_config_t));
	config[size-1] = g_phy_select;
	write(fd, config, size);

	//clsoe
	close(fd);
	free(config);
#endif

	return 0;
}

/*----------------------------------------------------------------------------------------------*/
// freq change demo
/*----------------------------------------------------------------------------------------------*/
typedef struct lo_change_demo_struct
{
	unsigned long long lo_change_val[2][2];
	unsigned long long tx;
	unsigned long long rx;
	unsigned char enable_run;
	unsigned char init_flag;
	unsigned char change_mode;
	unsigned char switch_index;
	int delay;
	task_t demo_tid;
} lo_change_demo_t;
static lo_change_demo_t g_lo_change_demo = 
{
	.init_flag = 0,
	.switch_index = 0,
	.delay = 10,
	.tx = 0,
	.rx = 0,
};

#define   HYBRID_MODE_TEST          0
#define   HYBRID_MODE_TEST_EXT      1

static void *tdd_lo_change_task(void *arg)
{
	unsigned char trx_flag=0;
	unsigned long long txlo=g_lo_change_demo.tx, rxlo=g_lo_change_demo.rx;
	
	while (1)
	{
		if (g_lo_change_demo.enable_run)
		{
#if HYBRID_MODE_TEST
			//g_phy_obj[g_phy_select].config.wire_control_en = 1;
			//g_phy_obj[g_phy_select].config.wire_ctrl = LEVEL_CTRL;
			//txlo = g_lo_change_demo.lo_change_val[g_lo_change_demo.switch_index][0];
			//rxlo = g_lo_change_demo.lo_change_val[g_lo_change_demo.switch_index][1];
			trx_lo_change_ext(&g_phy_obj[g_phy_select], g_lo_change_demo.change_mode, txlo, rxlo);
			//trx_lo_change_act(&g_phy_obj[g_phy_select]);

			//if (!flag)
			//{
			//	hal_spi_write_reg(&g_phy_obj[g_phy_select], 0x0cc, 0xf0);//alert
			//	flag = 1;
			//}

			if (!trx_flag)
			{
				//Rx to alert
				hal_fpga_write_reg(&g_phy_obj[g_phy_select], 0x0c, 0x00);
				CHIP_UDELAY(1);
				//alert to Tx
				hal_fpga_write_reg(&g_phy_obj[g_phy_select], 0x0c, 0x20);
				hal_fpga_write_reg(&g_phy_obj[g_phy_select], 0x0c, 0x30);
				//LOG_MAIN(">>>>>>>>>>>>>>>>>>>>>>>>>>>>> TX.\n");
			}
			else
			{
				//Tx to alert
				hal_fpga_write_reg(&g_phy_obj[g_phy_select], 0x0c, 0x20);
				CHIP_UDELAY(1);
				//alert to Rx
				hal_fpga_write_reg(&g_phy_obj[g_phy_select], 0x0c, 0x00);
				hal_fpga_write_reg(&g_phy_obj[g_phy_select], 0x0c, 0x10);
				txlo = g_lo_change_demo.lo_change_val[0][g_lo_change_demo.switch_index];
				rxlo = g_lo_change_demo.lo_change_val[0][!g_lo_change_demo.switch_index];
				//LOG_MAIN(">>>>>>>>>>>>>>>>>>>>>>>>>>>>> RX.\n");
				//LOG_MAIN("under rx, change txlo=%llu, rxlo=%llu.\n", txlo, rxlo);
				g_lo_change_demo.switch_index = !g_lo_change_demo.switch_index;
			}
#elif HYBRID_MODE_TEST_EXT
			trx_lo_change_ext(&g_phy_obj[g_phy_select], g_lo_change_demo.change_mode, txlo, rxlo);
			if (!trx_flag)
			{
				hal_fpga_write_reg(&g_phy_obj[g_phy_select], 0x314, 0x01);
				//LOG_MAIN(">>>>>>>>>>>>>>>>>>>>>>>>>>>>> TX.\n");
			}
			else
			{
				hal_fpga_write_reg(&g_phy_obj[g_phy_select], 0x318, 0x01);
				txlo = g_lo_change_demo.lo_change_val[0][g_lo_change_demo.switch_index];
				rxlo = g_lo_change_demo.lo_change_val[0][!g_lo_change_demo.switch_index];
				//LOG_MAIN(">>>>>>>>>>>>>>>>>>>>>>>>>>>>> RX.\n");
				//LOG_MAIN("under rx, change txlo=%llu, rxlo=%llu.\n", txlo, rxlo);
				g_lo_change_demo.switch_index = !g_lo_change_demo.switch_index;
			}
#else
			trx_lo_change_ext(&g_phy_obj[g_phy_select], g_lo_change_demo.change_mode, txlo, rxlo);
			if (!trx_flag)
			{
				//Rx to alert
				hal_fpga_write_reg(&g_phy_obj[g_phy_select], 0x0c, 0x00);
				CHIP_UDELAY(1);
				//alert to Tx
				hal_fpga_write_reg(&g_phy_obj[g_phy_select], 0x0c, 0x20);
				hal_fpga_write_reg(&g_phy_obj[g_phy_select], 0x0c, 0x30);
				//LOG_MAIN(">>>>>>>>>>>>>>>>>>>>>>>>>>>>> TX.\n");
			}
			else
			{
				//Tx to alert
				hal_fpga_write_reg(&g_phy_obj[g_phy_select], 0x0c, 0x20);
				CHIP_UDELAY(1);
				//alert to Rx
				hal_fpga_write_reg(&g_phy_obj[g_phy_select], 0x0c, 0x00);
				hal_fpga_write_reg(&g_phy_obj[g_phy_select], 0x0c, 0x10);
				//LOG_MAIN(">>>>>>>>>>>>>>>>>>>>>>>>>>>>> RX.\n");
				txlo = g_lo_change_demo.lo_change_val[0][g_lo_change_demo.switch_index];
				rxlo = txlo;
				//LOG_MAIN("change txlo=%llu, rxlo=%llu.\n", txlo, rxlo);
				g_lo_change_demo.switch_index = !g_lo_change_demo.switch_index;
			}
#endif

			CHIP_UDELAY(g_lo_change_demo.delay);
			trx_flag = !trx_flag;
		}
		else
		{
			LOG_MAIN("tdd_lo_change_task thread is alive.\n");
			CHIP_DELAY(2000);
		}
	}
}

int tdd_lo_change_demo(unsigned char enable, unsigned char lo_change_mode, unsigned long long  lo1, unsigned long long  lo2, int delay)
{
	g_lo_change_demo.lo_change_val[0][0] = lo1;
	g_lo_change_demo.lo_change_val[0][1] = lo2;
	g_lo_change_demo.lo_change_val[1][0] = lo2;
	g_lo_change_demo.lo_change_val[1][1] = lo1;
	g_lo_change_demo.tx = lo1;
	g_lo_change_demo.rx = lo2;
	g_lo_change_demo.enable_run = enable;
	g_lo_change_demo.change_mode = lo_change_mode;
	g_lo_change_demo.delay = delay;
	LOG_MAIN("[%s,%d] enable=%d, mode=%d, lo1=%llu, lo2=%llu, delay=%d.\n", __func__, __LINE__, enable, lo_change_mode, lo1, lo2, delay);

	hal_spi_write_reg(&g_phy_obj[g_phy_select], 0x7c2, 0x90);
	hal_spi_write_reg(&g_phy_obj[g_phy_select], 0x7c4, 0x90);
	hal_fpga_write_reg(&g_phy_obj[g_phy_select], 0x300, 0x800);
	hal_fpga_write_reg(&g_phy_obj[g_phy_select], 0x304, 0x1000);
	hal_fpga_write_reg(&g_phy_obj[g_phy_select], 0x308, 0x800);
	hal_fpga_write_reg(&g_phy_obj[g_phy_select], 0x30C, 0x1000);
	hal_fpga_write_reg(&g_phy_obj[g_phy_select], 0xc, 0);
	if (g_lo_change_demo.enable_run)
		hal_fpga_write_reg(&g_phy_obj[g_phy_select], 0x310, 0x1);
	else
		hal_fpga_write_reg(&g_phy_obj[g_phy_select], 0x310, 0x0);
	
	if (!g_lo_change_demo.init_flag)
	{
		LOG_MAIN("create tdd_lo_change_task thread.\n");
		g_lo_change_demo.init_flag = 1;
		create_task(&g_lo_change_demo.demo_tid, tdd_lo_change_task, &g_lo_change_demo);
	}
	return 0;
}

int test_main(int argc,char *argv[])
{
	unsigned long module_debug;
	int i, ret=0;
    int cnt = 0;  

	if (read_config_from_file() < 0)
	{
		g_phy_select = 0;
		module_debug = g_phy_obj[g_phy_select].module_debug;
		customer_init(&g_phy_obj[g_phy_select], &g_phy_config[g_phy_select]);
		g_phy_obj[g_phy_select].module_debug = module_debug;
	}
	
	if (argc < 2)
	{
		for (i=0; i<RF_PHY_NUMBER; i++)
		{
			ret = gc080x_init(&g_phy_obj[i], &g_phy_config[i]);
			if (ret < 0)
				LOG_MAIN("chip[%d] init failed!\n", i);
		}
	}
	else
	{
		ret = run_cmd(argc, argv);
	}

	if (ret < 0)
	{
		LOG_MAIN("usage: ./gc080x_daemon   cmd  parameter...\n");      
        cnt = sizeof(g_full_cmds) / sizeof(g_full_cmds[0]);
        for (i = 0; i < cnt; i++)
        {
            LOG_MAIN("         %s  contains %d parameters\n", g_full_cmds[i].cmd, g_full_cmds[i].param);
        }

		return -1;
	}

	write_config_to_file();
	
	return 0;
}


