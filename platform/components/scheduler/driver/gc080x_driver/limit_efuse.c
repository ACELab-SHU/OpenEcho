#include "limit_efuse.h"
#include "platform.h"

int get_efuse_info(rf_chip_phy_t *phy, XTAL_EN_INDEX_T xtal_index, CHIP_INFOR_T *chipInfo)
{
	int val;
	int ret = -1;
	hal_spi_write_reg(phy, 0x080, (1 << (xtal_index + 5)) | 0x01);
	CHIP_DELAY(10);
	val = hal_spi_read_reg(phy, 0x0A0) & 0xFF;

	switch(val)
	{
	case 0x00:
		chipInfo->low_freq = FREQ_30M;
		chipInfo->high_freq = FREQ_6_125G;
		chipInfo->low_bandwidth = BANDWIDTH_12K;
		chipInfo->high_bandwidth = BANDWIDTH_100M;
		chipInfo->tx_num = 2;
		chipInfo->rx_num = 2;
		chipInfo->chip_ver = CHIP_VERSION_D1;
		ret = 0;
		break;
	
	case 0x21:
	case 0x31:
		chipInfo->low_freq = FREQ_30M;
		chipInfo->high_freq = FREQ_6G;
		chipInfo->low_bandwidth = BANDWIDTH_12K;
		chipInfo->high_bandwidth = BANDWIDTH_60M;
		chipInfo->tx_num = 2;
		chipInfo->rx_num = 2;
		if(val == 0x31)
			chipInfo->chip_ver = CHIP_VERSION_E1;
		else
			chipInfo->chip_ver = CHIP_VERSION_D2;
		ret = 0;
		break;
		
	case 0x22:
	case 0x32:
		chipInfo->low_freq = FREQ_200M;
		chipInfo->high_freq = FREQ_5G;
		chipInfo->low_bandwidth = BANDWIDTH_12K;
		chipInfo->high_bandwidth = BANDWIDTH_100M;
		chipInfo->tx_num = 2;
		chipInfo->rx_num = 2;
		if(val == 0x32)
			chipInfo->chip_ver = CHIP_VERSION_E1;
		else
			chipInfo->chip_ver = CHIP_VERSION_D2;
		ret = 0;
		break;
		
	case 0x33:
	case 0x38:
	case 0x23:
	case 0x28:
		chipInfo->low_freq = FREQ_200M;
		chipInfo->high_freq = FREQ_3G_800M;
		chipInfo->low_bandwidth = BANDWIDTH_12K;
		chipInfo->high_bandwidth = BANDWIDTH_60M;
		chipInfo->tx_num = 2;
		chipInfo->rx_num = 2;
		if((val == 0x33) || (val == 0x38)) 
			chipInfo->chip_ver = CHIP_VERSION_E1;
		else
			chipInfo->chip_ver = CHIP_VERSION_D2;
		ret = 0;
		break;
		
	case 0x24:
	case 0x34:
		chipInfo->low_freq = FREQ_30M;
		chipInfo->high_freq = FREQ_6G;
		chipInfo->low_bandwidth = BANDWIDTH_12K;
		chipInfo->high_bandwidth = BANDWIDTH_60M;
		chipInfo->tx_num = 1;
		chipInfo->rx_num = 1;
		if(val == 0x34)
			chipInfo->chip_ver = CHIP_VERSION_E1;
		else
			chipInfo->chip_ver = CHIP_VERSION_D2;
		ret = 0;
		break;
		
	case 0x25:
	case 0x35:
		chipInfo->low_freq = FREQ_200M;
		chipInfo->high_freq = FREQ_3G_800M;
		chipInfo->low_bandwidth = BANDWIDTH_12K;
		chipInfo->high_bandwidth = BANDWIDTH_60M;
		chipInfo->tx_num = 1;
		chipInfo->rx_num = 1;
		if(val == 0x35)
			chipInfo->chip_ver = CHIP_VERSION_E1;
		else
			chipInfo->chip_ver = CHIP_VERSION_D2;
		ret = 0;
		break;

	case 0x36:
	case 0x3C:
	case 0x26:
	case 0x2C: 
		chipInfo->low_freq = FREQ_30M;
		chipInfo->high_freq = FREQ_6_125G;
		chipInfo->low_bandwidth = BANDWIDTH_12K;
		chipInfo->high_bandwidth = BANDWIDTH_100M;
		chipInfo->tx_num = 2;
		chipInfo->rx_num = 2;
		if((val == 0x36)||(val == 0x3C))
			chipInfo->chip_ver = CHIP_VERSION_E1;
		else
			chipInfo->chip_ver = CHIP_VERSION_D2;
		ret = 0;
		break;

	case 0x37:
	case 0x27: 
		chipInfo->low_freq = FREQ_30M;
		chipInfo->high_freq = FREQ_5G;
		chipInfo->low_bandwidth = BANDWIDTH_12K;
		chipInfo->high_bandwidth = BANDWIDTH_100M;
		chipInfo->tx_num = 2;
		chipInfo->rx_num = 2;
		if(val == 0x37)
			chipInfo->chip_ver = CHIP_VERSION_E1;
		else
			chipInfo->chip_ver = CHIP_VERSION_D2;
		ret = 0;
		break;

	case 0x3D:	
	case 0x2D:
		chipInfo->low_freq = FREQ_30M;
		chipInfo->high_freq = FREQ_6G;
		chipInfo->low_bandwidth = BANDWIDTH_12K;
		chipInfo->high_bandwidth = BANDWIDTH_100M;
		chipInfo->tx_num = 1;
		chipInfo->rx_num = 1;
		if(val == 0x3D)
			chipInfo->chip_ver = CHIP_VERSION_E1;
		else
			chipInfo->chip_ver = CHIP_VERSION_D2;
		ret = 0;
		break;

	case 0xA1: 
		chipInfo->low_freq = FREQ_30M;
		chipInfo->high_freq = FREQ_6_125G;
		chipInfo->low_bandwidth = BANDWIDTH_12K;
		chipInfo->high_bandwidth = BANDWIDTH_100M;
		chipInfo->tx_num = 2;
		chipInfo->rx_num = 2;
		chipInfo->chip_ver = CHIP_VERSION_D2;
		ret = 0;
		break;
			
	default:
		chipInfo->low_freq = 1;
		chipInfo->high_freq = 1;
		chipInfo->low_bandwidth = 1;
		chipInfo->high_bandwidth = 1;
		chipInfo->tx_num = 0;
		chipInfo->rx_num = 0;
		chipInfo->chip_ver = -1;
		ret = -1;
		break;
	}
	
	LOG_MAIN("Val = 0x%02X.\n", val);
	LOG_MAIN("chipInfo->low_freq = %d MHz.\n", chipInfo->low_freq);
	LOG_MAIN("chipInfo->high_freq = %d MHz.\n", chipInfo->high_freq);
	LOG_MAIN("chipInfo->low_bandwidth = %d KHz.\n", chipInfo->low_bandwidth);
	LOG_MAIN("chipInfo->high_bandwidth = %d KHz.\n", chipInfo->high_bandwidth);
	LOG_MAIN("chipInfo->tx_num = %d.\n", chipInfo->tx_num);
	LOG_MAIN("chipInfo->rx_num = %d.\n", chipInfo->rx_num);
	LOG_MAIN("chipInfo->chip_ver = %d.\n", chipInfo->chip_ver);

	return ret;
}

int confim_config_info(rf_chip_phy_t *phy)
{
	CHIP_INFOR_T info, *pInfo = &info;	
	XTAL_EN_INDEX_T xtal_index;
	
	if(phy->config->xtal_freq <= 34000000)
		xtal_index = XTAL_EN_INDEX_M30_EN;
	else if(phy->config->xtal_freq <= 39000000)
		xtal_index = XTAL_EN_INDEX_M38_EN;
	else 
		xtal_index = XTAL_EN_INDEX_M40_EN;

	hal_spi_write_reg(phy, 0xD33, (hal_spi_read_reg(phy, 0xD33) & 0x0F));
	
	if(! get_efuse_info(phy, xtal_index, pInfo))
	{
		if(((phy->config->tx_flo/1000/1000) >  (pInfo->high_freq )) || 
			((phy->config->rx_flo/1000/1000) > (pInfo->high_freq)))
		{
			LOG_ERROR("Chip info Err, Trx flo parameter Err.\n");
			return -1;
		}
		else
			hal_spi_write_reg(phy, 0xD33, (hal_spi_read_reg(phy, 0xD33) & 0xFF) | 0x40);

		if((phy->config->mode == CH1_CH2_FDD) || (phy->config->mode == CH1_CH2_TDD))
		{
			if((pInfo->tx_num != 2) || (pInfo->rx_num != 2))
			{
				LOG_ERROR("Chip info Err, Trx mode parameter Err.\n");
				return -1;
			}
			else
				hal_spi_write_reg(phy, 0xD33, (hal_spi_read_reg(phy, 0xD33) & 0xFF) | 0x10);
		}
		else
			hal_spi_write_reg(phy, 0xD33, (hal_spi_read_reg(phy, 0xD33) & 0xFF) | 0x10);

		if(phy->config->custom_bandwith_flag)
		{
			if(phy->config->custom_bandwith > (pInfo->high_bandwidth*1000))
			{
				LOG_ERROR("Chip info Err, Trx custom bandwith parameter Err.\n");
				return -1;
			}
			else
				hal_spi_write_reg(phy, 0xD33, (hal_spi_read_reg(phy, 0xD33) & 0xFF) | 0x20);
		}
		else
		{
			if((g_3db_band[phy->config->bandwith]*2) > (pInfo->high_bandwidth*1000+30000))
			{
				LOG_ERROR("Chip info Err, Trx Bandwith parameter Err.\n");
				return -1;
			}		
			else
				hal_spi_write_reg(phy, 0xD33, (hal_spi_read_reg(phy, 0xD33) & 0xFF) | 0x20);
		}
		return pInfo->chip_ver;	
	}
	else
	{
		LOG_ERROR("Chip info Err.\n");
		return -1;
	}
}


int efuse_read_opt(rf_chip_phy_t *phy,unsigned char *data, EFUSE_INFO_SECTION_OPT_T index)
{
	unsigned char rd;
	XTAL_EN_INDEX_T xtal_index;
	unsigned char reg80_val;
	
	
	if(phy->config->xtal_freq <= 34000000)
		xtal_index = XTAL_EN_INDEX_M30_EN;
	else if(phy->config->xtal_freq <= 39000000)
		xtal_index = XTAL_EN_INDEX_M38_EN;
	else 
		xtal_index = XTAL_EN_INDEX_M40_EN;
	
	reg80_val = hal_spi_read_reg(phy, 0x80);
	hal_spi_write_reg(phy, 0x80, (1 << (xtal_index + 5)) | 0x01);
	CHIP_DELAY(10);
	
	rd = hal_spi_read_reg(phy, 0xA0 - index);

	*data = rd;
	hal_spi_write_reg(phy, 0x80, reg80_val);

	return 0;
}



