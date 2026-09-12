#include "qec_common.h"
#include "platform.h"
#include "driver.h"
#include "register.h"
#include "common.h"

#include	"printf.h"

#undef PI
#define PI (3.14159265359)

#define SIGN(A)   ( ((A)>0) ? 1:-1)
//#define QEC_COMMON_PRINT 1
#ifndef DIG_QEC_D1
int fn_rx_load_lg_bw_coeff(rf_chip_phy_t *phy, int channel_sel)
{
    if(channel_sel == 0)
    {
        hal_spi_write_reg(phy, 0x00A ,0x01   );
        hal_spi_write_reg(phy, 0x00B ,0x0    );
        hal_spi_write_reg(phy, 0x00C ,0xFF   );
        hal_spi_write_reg(phy, 0x00D ,0xFC   );
        hal_spi_write_reg(phy, 0x00A ,0x09   );
        hal_spi_write_reg(phy, 0x00A ,0x01   );
        hal_spi_write_reg(phy, 0x00B ,0x1    );
        hal_spi_write_reg(phy, 0x00C ,0xFF   );
        hal_spi_write_reg(phy, 0x00D ,0xFE   );
        hal_spi_write_reg(phy, 0x00A ,0x09   );
        hal_spi_write_reg(phy, 0x00A ,0x01   );
        hal_spi_write_reg(phy, 0x00B ,0x2    );
        hal_spi_write_reg(phy, 0x00C ,0x00   );
        hal_spi_write_reg(phy, 0x00D ,0x04   );
        hal_spi_write_reg(phy, 0x00A ,0x09   );
        hal_spi_write_reg(phy, 0x00A ,0x01   );
        hal_spi_write_reg(phy, 0x00B ,0x3    );
        hal_spi_write_reg(phy, 0x00C ,0x00   );
        hal_spi_write_reg(phy, 0x00D ,0x02   );
        hal_spi_write_reg(phy, 0x00A ,0x09   );
        hal_spi_write_reg(phy, 0x00A ,0x01   );
        hal_spi_write_reg(phy, 0x00B ,0x4    );
        hal_spi_write_reg(phy, 0x00C ,0xFF   );
        hal_spi_write_reg(phy, 0x00D ,0xFC   );
        hal_spi_write_reg(phy, 0x00A ,0x09   );
        hal_spi_write_reg(phy, 0x00A ,0x01   );
        hal_spi_write_reg(phy, 0x00B ,0x5    );
        hal_spi_write_reg(phy, 0x00C ,0xFF   );
        hal_spi_write_reg(phy, 0x00D ,0xFB   );
        hal_spi_write_reg(phy, 0x00A ,0x09   );
        hal_spi_write_reg(phy, 0x00A ,0x01   );
        hal_spi_write_reg(phy, 0x00B ,0x6    );
        hal_spi_write_reg(phy, 0x00C ,0x00   );
        hal_spi_write_reg(phy, 0x00D ,0x06   );
        hal_spi_write_reg(phy, 0x00A ,0x09   );
        hal_spi_write_reg(phy, 0x00A ,0x01   );
        hal_spi_write_reg(phy, 0x00B ,0x7    );
        hal_spi_write_reg(phy, 0x00C ,0x00   );
        hal_spi_write_reg(phy, 0x00D ,0x08   );
        hal_spi_write_reg(phy, 0x00A ,0x09   );
        hal_spi_write_reg(phy, 0x00A ,0x01   );
        hal_spi_write_reg(phy, 0x00B ,0x8    );
        hal_spi_write_reg(phy, 0x00C ,0xFF   );
        hal_spi_write_reg(phy, 0x00D ,0xFB   );
        hal_spi_write_reg(phy, 0x00A ,0x09   );
        hal_spi_write_reg(phy, 0x00A ,0x01   );
        hal_spi_write_reg(phy, 0x00B ,0x9    );
        hal_spi_write_reg(phy, 0x00C ,0xFF   );
        hal_spi_write_reg(phy, 0x00D ,0xF3   );
        hal_spi_write_reg(phy, 0x00A ,0x09   );
        hal_spi_write_reg(phy, 0x00A ,0x01   );
        hal_spi_write_reg(phy, 0x00B ,0xA    );
        hal_spi_write_reg(phy, 0x00C ,0x00   );
        hal_spi_write_reg(phy, 0x00D ,0x04   );
        hal_spi_write_reg(phy, 0x00A ,0x09   );
        hal_spi_write_reg(phy, 0x00A ,0x01   );
        hal_spi_write_reg(phy, 0x00B ,0xB    );
        hal_spi_write_reg(phy, 0x00C ,0x00   );
        hal_spi_write_reg(phy, 0x00D ,0x11   );
        hal_spi_write_reg(phy, 0x00A ,0x09   );
        hal_spi_write_reg(phy, 0x00A ,0x01   );
        hal_spi_write_reg(phy, 0x00B ,0xC    );
        hal_spi_write_reg(phy, 0x00C ,0xFF   );
        hal_spi_write_reg(phy, 0x00D ,0xFF   );
        hal_spi_write_reg(phy, 0x00A ,0x09   );
        hal_spi_write_reg(phy, 0x00A ,0x01   );
        hal_spi_write_reg(phy, 0x00B ,0xD    );
        hal_spi_write_reg(phy, 0x00C ,0xFF   );
        hal_spi_write_reg(phy, 0x00D ,0xE9   );
        hal_spi_write_reg(phy, 0x00A ,0x09   );
        hal_spi_write_reg(phy, 0x00A ,0x01   );
        hal_spi_write_reg(phy, 0x00B ,0xE    );
        hal_spi_write_reg(phy, 0x00C ,0xFF   );
        hal_spi_write_reg(phy, 0x00D ,0xFC   );
        hal_spi_write_reg(phy, 0x00A ,0x09   );
        hal_spi_write_reg(phy, 0x00A ,0x01   );
        hal_spi_write_reg(phy, 0x00B ,0xF    );
        hal_spi_write_reg(phy, 0x00C ,0x00   );
        hal_spi_write_reg(phy, 0x00D ,0x1C   );
        hal_spi_write_reg(phy, 0x00A ,0x09   );
        hal_spi_write_reg(phy, 0x00A ,0x01   );
        hal_spi_write_reg(phy, 0x00B ,0x10   );
        hal_spi_write_reg(phy, 0x00C ,0x00   );
        hal_spi_write_reg(phy, 0x00D ,0x0C   );
        hal_spi_write_reg(phy, 0x00A ,0x09   );
        hal_spi_write_reg(phy, 0x00A ,0x01   );
        hal_spi_write_reg(phy, 0x00B ,0x11   );
        hal_spi_write_reg(phy, 0x00C ,0xFF   );
        hal_spi_write_reg(phy, 0x00D ,0xDF   );
        hal_spi_write_reg(phy, 0x00A ,0x09   );
        hal_spi_write_reg(phy, 0x00A ,0x01   );
        hal_spi_write_reg(phy, 0x00B ,0x12   );
        hal_spi_write_reg(phy, 0x00C ,0xFF   );
        hal_spi_write_reg(phy, 0x00D ,0xE9   );
        hal_spi_write_reg(phy, 0x00A ,0x09   );
        hal_spi_write_reg(phy, 0x00A ,0x01   );
        hal_spi_write_reg(phy, 0x00B ,0x13   );
        hal_spi_write_reg(phy, 0x00C ,0x00   );
        hal_spi_write_reg(phy, 0x00D ,0x23   );
        hal_spi_write_reg(phy, 0x00A ,0x09   );
        hal_spi_write_reg(phy, 0x00A ,0x01   );
        hal_spi_write_reg(phy, 0x00B ,0x14   );
        hal_spi_write_reg(phy, 0x00C ,0x00   );
        hal_spi_write_reg(phy, 0x00D ,0x25   );
        hal_spi_write_reg(phy, 0x00A ,0x09   );
        hal_spi_write_reg(phy, 0x00A ,0x01   );
        hal_spi_write_reg(phy, 0x00B ,0x15   );
        hal_spi_write_reg(phy, 0x00C ,0xFF   );
        hal_spi_write_reg(phy, 0x00D ,0xDD   );
        hal_spi_write_reg(phy, 0x00A ,0x09   );
        hal_spi_write_reg(phy, 0x00A ,0x01   );
        hal_spi_write_reg(phy, 0x00B ,0x16   );
        hal_spi_write_reg(phy, 0x00C ,0xFF   );
        hal_spi_write_reg(phy, 0x00D ,0xCB   );
        hal_spi_write_reg(phy, 0x00A ,0x09   );
        hal_spi_write_reg(phy, 0x00A ,0x01   );
        hal_spi_write_reg(phy, 0x00B ,0x17   );
        hal_spi_write_reg(phy, 0x00C ,0x00   );
        hal_spi_write_reg(phy, 0x00D ,0x1E   );
        hal_spi_write_reg(phy, 0x00A ,0x09   );
        hal_spi_write_reg(phy, 0x00A ,0x01   );
        hal_spi_write_reg(phy, 0x00B ,0x18   );
        hal_spi_write_reg(phy, 0x00C ,0x00   );
        hal_spi_write_reg(phy, 0x00D ,0x47   );
        hal_spi_write_reg(phy, 0x00A ,0x09   );
        hal_spi_write_reg(phy, 0x00A ,0x01   );
        hal_spi_write_reg(phy, 0x00B ,0x19   );
        hal_spi_write_reg(phy, 0x00C ,0xFF   );
        hal_spi_write_reg(phy, 0x00D ,0xED   );
        hal_spi_write_reg(phy, 0x00A ,0x09   );
        hal_spi_write_reg(phy, 0x00A ,0x01   );
        hal_spi_write_reg(phy, 0x00B ,0x1A   );
        hal_spi_write_reg(phy, 0x00C ,0xFF   );
        hal_spi_write_reg(phy, 0x00D ,0xA6   );
        hal_spi_write_reg(phy, 0x00A ,0x09   );
        hal_spi_write_reg(phy, 0x00A ,0x01   );
        hal_spi_write_reg(phy, 0x00B ,0x1B   );
        hal_spi_write_reg(phy, 0x00C ,0x00   );
        hal_spi_write_reg(phy, 0x00D ,0x01   );
        hal_spi_write_reg(phy, 0x00A ,0x09   );
        hal_spi_write_reg(phy, 0x00A ,0x01   );
        hal_spi_write_reg(phy, 0x00B ,0x1C   );
        hal_spi_write_reg(phy, 0x00C ,0x00   );
        hal_spi_write_reg(phy, 0x00D ,0x6D   );
        hal_spi_write_reg(phy, 0x00A ,0x09   );
        hal_spi_write_reg(phy, 0x00A ,0x01   );
        hal_spi_write_reg(phy, 0x00B ,0x1D   );
        hal_spi_write_reg(phy, 0x00C ,0x00   );
        hal_spi_write_reg(phy, 0x00D ,0x18   );
        hal_spi_write_reg(phy, 0x00A ,0x09   );
        hal_spi_write_reg(phy, 0x00A ,0x01   );
        hal_spi_write_reg(phy, 0x00B ,0x1E   );
        hal_spi_write_reg(phy, 0x00C ,0xFF   );
        hal_spi_write_reg(phy, 0x00D ,0x84   );
        hal_spi_write_reg(phy, 0x00A ,0x09   );
        hal_spi_write_reg(phy, 0x00A ,0x01   );
        hal_spi_write_reg(phy, 0x00B ,0x1F   );
        hal_spi_write_reg(phy, 0x00C ,0xFF   );
        hal_spi_write_reg(phy, 0x00D ,0xC6   );
        hal_spi_write_reg(phy, 0x00A ,0x09   );
        hal_spi_write_reg(phy, 0x00A ,0x01   );
        hal_spi_write_reg(phy, 0x00B ,0x20   );
        hal_spi_write_reg(phy, 0x00C ,0x00   );
        hal_spi_write_reg(phy, 0x00D ,0x85   );
        hal_spi_write_reg(phy, 0x00A ,0x09   );
        hal_spi_write_reg(phy, 0x00A ,0x01   );
        hal_spi_write_reg(phy, 0x00B ,0x21   );
        hal_spi_write_reg(phy, 0x00C ,0x00   );
        hal_spi_write_reg(phy, 0x00D ,0x63   );
        hal_spi_write_reg(phy, 0x00A ,0x09   );
        hal_spi_write_reg(phy, 0x00A ,0x01   );
        hal_spi_write_reg(phy, 0x00B ,0x22   );
        hal_spi_write_reg(phy, 0x00C ,0xFF   );
        hal_spi_write_reg(phy, 0x00D ,0x7B   );
        hal_spi_write_reg(phy, 0x00A ,0x09   );
        hal_spi_write_reg(phy, 0x00A ,0x01   );
        hal_spi_write_reg(phy, 0x00B ,0x23   );
        hal_spi_write_reg(phy, 0x00C ,0xFF   );
        hal_spi_write_reg(phy, 0x00D ,0x6C   );
        hal_spi_write_reg(phy, 0x00A ,0x09   );
        hal_spi_write_reg(phy, 0x00A ,0x01   );
        hal_spi_write_reg(phy, 0x00B ,0x24   );
        hal_spi_write_reg(phy, 0x00C ,0x00   );
        hal_spi_write_reg(phy, 0x00D ,0x7A   );
        hal_spi_write_reg(phy, 0x00A ,0x09   );
        hal_spi_write_reg(phy, 0x00A ,0x01   );
        hal_spi_write_reg(phy, 0x00B ,0x25   );
        hal_spi_write_reg(phy, 0x00C ,0x00   );
        hal_spi_write_reg(phy, 0x00D ,0xCA   );
        hal_spi_write_reg(phy, 0x00A ,0x09   );
        hal_spi_write_reg(phy, 0x00A ,0x01   );
        hal_spi_write_reg(phy, 0x00B ,0x26   );
        hal_spi_write_reg(phy, 0x00C ,0xFF   );
        hal_spi_write_reg(phy, 0x00D ,0xA0   );
        hal_spi_write_reg(phy, 0x00A ,0x09   );
        hal_spi_write_reg(phy, 0x00A ,0x01   );
        hal_spi_write_reg(phy, 0x00B ,0x27   );
        hal_spi_write_reg(phy, 0x00C ,0xFE   );
        hal_spi_write_reg(phy, 0x00D ,0xFD   );
        hal_spi_write_reg(phy, 0x00A ,0x09   );
        hal_spi_write_reg(phy, 0x00A ,0x01   );
        hal_spi_write_reg(phy, 0x00B ,0x28   );
        hal_spi_write_reg(phy, 0x00C ,0x00   );
        hal_spi_write_reg(phy, 0x00D ,0x34   );
        hal_spi_write_reg(phy, 0x00A ,0x09   );
        hal_spi_write_reg(phy, 0x00A ,0x01   );
        hal_spi_write_reg(phy, 0x00B ,0x29   );
        hal_spi_write_reg(phy, 0x00C ,0x01   );
        hal_spi_write_reg(phy, 0x00D ,0x3C   );
        hal_spi_write_reg(phy, 0x00A ,0x09   );
        hal_spi_write_reg(phy, 0x00A ,0x01   );
        hal_spi_write_reg(phy, 0x00B ,0x2A   );
        hal_spi_write_reg(phy, 0x00C ,0x00   );
        hal_spi_write_reg(phy, 0x00D ,0x0D   );
        hal_spi_write_reg(phy, 0x00A ,0x09   );
        hal_spi_write_reg(phy, 0x00A ,0x01   );
        hal_spi_write_reg(phy, 0x00B ,0x2B   );
        hal_spi_write_reg(phy, 0x00C ,0xFE   );
        hal_spi_write_reg(phy, 0x00D ,0x90   );
        hal_spi_write_reg(phy, 0x00A ,0x09   );
        hal_spi_write_reg(phy, 0x00A ,0x01   );
        hal_spi_write_reg(phy, 0x00B ,0x2C   );
        hal_spi_write_reg(phy, 0x00C ,0xFF   );
        hal_spi_write_reg(phy, 0x00D ,0x9B   );
        hal_spi_write_reg(phy, 0x00A ,0x09   );
        hal_spi_write_reg(phy, 0x00A ,0x01   );
        hal_spi_write_reg(phy, 0x00B ,0x2D   );
        hal_spi_write_reg(phy, 0x00C ,0x01   );
        hal_spi_write_reg(phy, 0x00D ,0x99   );
        hal_spi_write_reg(phy, 0x00A ,0x09   );
        hal_spi_write_reg(phy, 0x00A ,0x01   );
        hal_spi_write_reg(phy, 0x00B ,0x2E   );
        hal_spi_write_reg(phy, 0x00C ,0x00   );
        hal_spi_write_reg(phy, 0x00D ,0xD6   );
        hal_spi_write_reg(phy, 0x00A ,0x09   );
        hal_spi_write_reg(phy, 0x00A ,0x01   );
        hal_spi_write_reg(phy, 0x00B ,0x2F   );
        hal_spi_write_reg(phy, 0x00C ,0xFE   );
        hal_spi_write_reg(phy, 0x00D ,0x4F   );
        hal_spi_write_reg(phy, 0x00A ,0x09   );
        hal_spi_write_reg(phy, 0x00A ,0x01   );
        hal_spi_write_reg(phy, 0x00B ,0x30   );
        hal_spi_write_reg(phy, 0x00C ,0xFE   );
        hal_spi_write_reg(phy, 0x00D ,0x9C   );
        hal_spi_write_reg(phy, 0x00A ,0x09   );
        hal_spi_write_reg(phy, 0x00A ,0x01   );
        hal_spi_write_reg(phy, 0x00B ,0x31   );
        hal_spi_write_reg(phy, 0x00C ,0x01   );
        hal_spi_write_reg(phy, 0x00D ,0xB1   );
        hal_spi_write_reg(phy, 0x00A ,0x09   );
        hal_spi_write_reg(phy, 0x00A ,0x01   );
        hal_spi_write_reg(phy, 0x00B ,0x32   );
        hal_spi_write_reg(phy, 0x00C ,0x02   );
        hal_spi_write_reg(phy, 0x00D ,0x11   );
        hal_spi_write_reg(phy, 0x00A ,0x09   );
        hal_spi_write_reg(phy, 0x00A ,0x01   );
        hal_spi_write_reg(phy, 0x00B ,0x33   );
        hal_spi_write_reg(phy, 0x00C ,0xFE   );
        hal_spi_write_reg(phy, 0x00D ,0x73   );
        hal_spi_write_reg(phy, 0x00A ,0x09   );
        hal_spi_write_reg(phy, 0x00A ,0x01   );
        hal_spi_write_reg(phy, 0x00B ,0x34   );
        hal_spi_write_reg(phy, 0x00C ,0xFD   );
        hal_spi_write_reg(phy, 0x00D ,0x1D   );
        hal_spi_write_reg(phy, 0x00A ,0x09   );
        hal_spi_write_reg(phy, 0x00A ,0x01   );
        hal_spi_write_reg(phy, 0x00B ,0x35   );
        hal_spi_write_reg(phy, 0x00C ,0x01   );
        hal_spi_write_reg(phy, 0x00D ,0x38   );
        hal_spi_write_reg(phy, 0x00A ,0x09   );
        hal_spi_write_reg(phy, 0x00A ,0x01   );
        hal_spi_write_reg(phy, 0x00B ,0x36   );
        hal_spi_write_reg(phy, 0x00C ,0x03   );
        hal_spi_write_reg(phy, 0x00D ,0xE6   );
        hal_spi_write_reg(phy, 0x00A ,0x09   );
        hal_spi_write_reg(phy, 0x00A ,0x01   );
        hal_spi_write_reg(phy, 0x00B ,0x37   );
        hal_spi_write_reg(phy, 0x00C ,0xFF   );
        hal_spi_write_reg(phy, 0x00D ,0x67   );
        hal_spi_write_reg(phy, 0x00A ,0x09   );
        hal_spi_write_reg(phy, 0x00A ,0x01   );
        hal_spi_write_reg(phy, 0x00B ,0x38   );
        hal_spi_write_reg(phy, 0x00C ,0xFA   );
        hal_spi_write_reg(phy, 0x00D ,0xCA   );
        hal_spi_write_reg(phy, 0x00A ,0x09   );
        hal_spi_write_reg(phy, 0x00A ,0x01   );
        hal_spi_write_reg(phy, 0x00B ,0x39   );
        hal_spi_write_reg(phy, 0x00C ,0xFF   );
        hal_spi_write_reg(phy, 0x00D ,0x7D   );
        hal_spi_write_reg(phy, 0x00A ,0x09   );
        hal_spi_write_reg(phy, 0x00A ,0x01   );
        hal_spi_write_reg(phy, 0x00B ,0x3A   );
        hal_spi_write_reg(phy, 0x00C ,0x07   );
        hal_spi_write_reg(phy, 0x00D ,0x1D   );
        hal_spi_write_reg(phy, 0x00A ,0x09   );
        hal_spi_write_reg(phy, 0x00A ,0x01   );
        hal_spi_write_reg(phy, 0x00B ,0x3B   );
        hal_spi_write_reg(phy, 0x00C ,0x02   );
        hal_spi_write_reg(phy, 0x00D ,0xA3   );
        hal_spi_write_reg(phy, 0x00A ,0x09   );
        hal_spi_write_reg(phy, 0x00A ,0x01   );
        hal_spi_write_reg(phy, 0x00B ,0x3C   );
        hal_spi_write_reg(phy, 0x00C ,0xF5   );
        hal_spi_write_reg(phy, 0x00D ,0x68   );
        hal_spi_write_reg(phy, 0x00A ,0x09   );
        hal_spi_write_reg(phy, 0x00A ,0x01   );
        hal_spi_write_reg(phy, 0x00B ,0x3D   );
        hal_spi_write_reg(phy, 0x00C ,0xF8   );
        hal_spi_write_reg(phy, 0x00D ,0x00   );
        hal_spi_write_reg(phy, 0x00A ,0x09   );
        hal_spi_write_reg(phy, 0x00A ,0x01   );
        hal_spi_write_reg(phy, 0x00B ,0x3E   );
        hal_spi_write_reg(phy, 0x00C ,0x16   );
        hal_spi_write_reg(phy, 0x00D ,0x04   );
        hal_spi_write_reg(phy, 0x00A ,0x09   );
        hal_spi_write_reg(phy, 0x00A ,0x01   );
        hal_spi_write_reg(phy, 0x00B ,0x3F   );
        hal_spi_write_reg(phy, 0x00C ,0x36   );
        hal_spi_write_reg(phy, 0x00D ,0x6C   );
        hal_spi_write_reg(phy, 0x00A ,0x09   );
        hal_spi_write_reg(phy, 0x00A ,0x01   );
        hal_spi_write_reg(phy, 0x00A ,0x80	);
    }
    else
    {
        hal_spi_write_reg(phy, 0x00A, 0x01   );
        hal_spi_write_reg(phy, 0x006, 0x10   );
        hal_spi_write_reg(phy, 0x00B, 0x0    );
        hal_spi_write_reg(phy, 0x00C, 0xFF   );
        hal_spi_write_reg(phy, 0x00D, 0xFC   );
        hal_spi_write_reg(phy, 0x00A, 0x11   );
        hal_spi_write_reg(phy, 0x00A, 0x01   );
        hal_spi_write_reg(phy, 0x00B, 0x1    );
        hal_spi_write_reg(phy, 0x00C, 0xFF   );
        hal_spi_write_reg(phy, 0x00D, 0xFE   );
        hal_spi_write_reg(phy, 0x00A, 0x11   );
        hal_spi_write_reg(phy, 0x00A, 0x01   );
        hal_spi_write_reg(phy, 0x00B, 0x2    );
        hal_spi_write_reg(phy, 0x00C, 0x00   );
        hal_spi_write_reg(phy, 0x00D, 0x04   );
        hal_spi_write_reg(phy, 0x00A, 0x11   );
        hal_spi_write_reg(phy, 0x00A, 0x01   );
        hal_spi_write_reg(phy, 0x00B, 0x3    );
        hal_spi_write_reg(phy, 0x00C, 0x00   );
        hal_spi_write_reg(phy, 0x00D, 0x02   );
        hal_spi_write_reg(phy, 0x00A, 0x11   );
        hal_spi_write_reg(phy, 0x00A, 0x01   );
        hal_spi_write_reg(phy, 0x00B, 0x4    );
        hal_spi_write_reg(phy, 0x00C, 0xFF   );
        hal_spi_write_reg(phy, 0x00D, 0xFC   );
        hal_spi_write_reg(phy, 0x00A, 0x11   );
        hal_spi_write_reg(phy, 0x00A, 0x01   );
        hal_spi_write_reg(phy, 0x00B, 0x5    );
        hal_spi_write_reg(phy, 0x00C, 0xFF   );
        hal_spi_write_reg(phy, 0x00D, 0xFB   );
        hal_spi_write_reg(phy, 0x00A, 0x11   );
        hal_spi_write_reg(phy, 0x00A, 0x01   );
        hal_spi_write_reg(phy, 0x00B, 0x6    );
        hal_spi_write_reg(phy, 0x00C, 0x00   );
        hal_spi_write_reg(phy, 0x00D, 0x06   );
        hal_spi_write_reg(phy, 0x00A, 0x11   );
        hal_spi_write_reg(phy, 0x00A, 0x01   );
        hal_spi_write_reg(phy, 0x00B, 0x7    );
        hal_spi_write_reg(phy, 0x00C, 0x00   );
        hal_spi_write_reg(phy, 0x00D, 0x08   );
        hal_spi_write_reg(phy, 0x00A, 0x11   );
        hal_spi_write_reg(phy, 0x00A, 0x01   );
        hal_spi_write_reg(phy, 0x00B, 0x8    );
        hal_spi_write_reg(phy, 0x00C, 0xFF   );
        hal_spi_write_reg(phy, 0x00D, 0xFB   );
        hal_spi_write_reg(phy, 0x00A, 0x11   );
        hal_spi_write_reg(phy, 0x00A, 0x01   );
        hal_spi_write_reg(phy, 0x00B, 0x9    );
        hal_spi_write_reg(phy, 0x00C, 0xFF   );
        hal_spi_write_reg(phy, 0x00D, 0xF3   );
        hal_spi_write_reg(phy, 0x00A, 0x11   );
        hal_spi_write_reg(phy, 0x00A, 0x01   );
        hal_spi_write_reg(phy, 0x00B, 0xA    );
        hal_spi_write_reg(phy, 0x00C, 0x00   );
        hal_spi_write_reg(phy, 0x00D, 0x04   );
        hal_spi_write_reg(phy, 0x00A, 0x11   );
        hal_spi_write_reg(phy, 0x00A, 0x01   );
        hal_spi_write_reg(phy, 0x00B, 0xB    );
        hal_spi_write_reg(phy, 0x00C, 0x00   );
        hal_spi_write_reg(phy, 0x00D, 0x11   );
        hal_spi_write_reg(phy, 0x00A, 0x11   );
        hal_spi_write_reg(phy, 0x00A, 0x01   );
        hal_spi_write_reg(phy, 0x00B, 0xC    );
        hal_spi_write_reg(phy, 0x00C, 0xFF   );
        hal_spi_write_reg(phy, 0x00D, 0xFF   );
        hal_spi_write_reg(phy, 0x00A, 0x11   );
        hal_spi_write_reg(phy, 0x00A, 0x01   );
        hal_spi_write_reg(phy, 0x00B, 0xD    );
        hal_spi_write_reg(phy, 0x00C, 0xFF   );
        hal_spi_write_reg(phy, 0x00D, 0xE9   );
        hal_spi_write_reg(phy, 0x00A, 0x11   );
        hal_spi_write_reg(phy, 0x00A, 0x01   );
        hal_spi_write_reg(phy, 0x00B, 0xE    );
        hal_spi_write_reg(phy, 0x00C, 0xFF   );
        hal_spi_write_reg(phy, 0x00D, 0xFC   );
        hal_spi_write_reg(phy, 0x00A, 0x11   );
        hal_spi_write_reg(phy, 0x00A, 0x01   );
        hal_spi_write_reg(phy, 0x00B, 0xF    );
        hal_spi_write_reg(phy, 0x00C, 0x00   );
        hal_spi_write_reg(phy, 0x00D, 0x1C   );
        hal_spi_write_reg(phy, 0x00A, 0x11   );
        hal_spi_write_reg(phy, 0x00A, 0x01   );
        hal_spi_write_reg(phy, 0x00B, 0x10   );
        hal_spi_write_reg(phy, 0x00C, 0x00   );
        hal_spi_write_reg(phy, 0x00D, 0x0C   );
        hal_spi_write_reg(phy, 0x00A, 0x11   );
        hal_spi_write_reg(phy, 0x00A, 0x01   );
        hal_spi_write_reg(phy, 0x00B, 0x11   );
        hal_spi_write_reg(phy, 0x00C, 0xFF   );
        hal_spi_write_reg(phy, 0x00D, 0xDF   );
        hal_spi_write_reg(phy, 0x00A, 0x11   );
        hal_spi_write_reg(phy, 0x00A, 0x01   );
        hal_spi_write_reg(phy, 0x00B, 0x12   );
        hal_spi_write_reg(phy, 0x00C, 0xFF   );
        hal_spi_write_reg(phy, 0x00D, 0xE9   );
        hal_spi_write_reg(phy, 0x00A, 0x11   );
        hal_spi_write_reg(phy, 0x00A, 0x01   );
        hal_spi_write_reg(phy, 0x00B, 0x13   );
        hal_spi_write_reg(phy, 0x00C, 0x00   );
        hal_spi_write_reg(phy, 0x00D, 0x23   );
        hal_spi_write_reg(phy, 0x00A, 0x11   );
        hal_spi_write_reg(phy, 0x00A, 0x01   );
        hal_spi_write_reg(phy, 0x00B, 0x14   );
        hal_spi_write_reg(phy, 0x00C, 0x00   );
        hal_spi_write_reg(phy, 0x00D, 0x25   );
        hal_spi_write_reg(phy, 0x00A, 0x11   );
        hal_spi_write_reg(phy, 0x00A, 0x01   );
        hal_spi_write_reg(phy, 0x00B, 0x15   );
        hal_spi_write_reg(phy, 0x00C, 0xFF   );
        hal_spi_write_reg(phy, 0x00D, 0xDD   );
        hal_spi_write_reg(phy, 0x00A, 0x11   );
        hal_spi_write_reg(phy, 0x00A, 0x01   );
        hal_spi_write_reg(phy, 0x00B, 0x16   );
        hal_spi_write_reg(phy, 0x00C, 0xFF   );
        hal_spi_write_reg(phy, 0x00D, 0xCB   );
        hal_spi_write_reg(phy, 0x00A, 0x11   );
        hal_spi_write_reg(phy, 0x00A, 0x01   );
        hal_spi_write_reg(phy, 0x00B, 0x17   );
        hal_spi_write_reg(phy, 0x00C, 0x00   );
        hal_spi_write_reg(phy, 0x00D, 0x1E   );
        hal_spi_write_reg(phy, 0x00A, 0x11   );
        hal_spi_write_reg(phy, 0x00A, 0x01   );
        hal_spi_write_reg(phy, 0x00B, 0x18   );
        hal_spi_write_reg(phy, 0x00C, 0x00   );
        hal_spi_write_reg(phy, 0x00D, 0x47   );
        hal_spi_write_reg(phy, 0x00A, 0x11   );
        hal_spi_write_reg(phy, 0x00A, 0x01   );
        hal_spi_write_reg(phy, 0x00B, 0x19   );
        hal_spi_write_reg(phy, 0x00C, 0xFF   );
        hal_spi_write_reg(phy, 0x00D, 0xED   );
        hal_spi_write_reg(phy, 0x00A, 0x11   );
        hal_spi_write_reg(phy, 0x00A, 0x01   );
        hal_spi_write_reg(phy, 0x00B, 0x1A   );
        hal_spi_write_reg(phy, 0x00C, 0xFF   );
        hal_spi_write_reg(phy, 0x00D, 0xA6   );
        hal_spi_write_reg(phy, 0x00A, 0x11   );
        hal_spi_write_reg(phy, 0x00A, 0x01   );
        hal_spi_write_reg(phy, 0x00B, 0x1B   );
        hal_spi_write_reg(phy, 0x00C, 0x00   );
        hal_spi_write_reg(phy, 0x00D, 0x01   );
        hal_spi_write_reg(phy, 0x00A, 0x11   );
        hal_spi_write_reg(phy, 0x00A, 0x01   );
        hal_spi_write_reg(phy, 0x00B, 0x1C   );
        hal_spi_write_reg(phy, 0x00C, 0x00   );
        hal_spi_write_reg(phy, 0x00D, 0x6D   );
        hal_spi_write_reg(phy, 0x00A, 0x11   );
        hal_spi_write_reg(phy, 0x00A, 0x01   );
        hal_spi_write_reg(phy, 0x00B, 0x1D   );
        hal_spi_write_reg(phy, 0x00C, 0x00   );
        hal_spi_write_reg(phy, 0x00D, 0x18   );
        hal_spi_write_reg(phy, 0x00A, 0x11   );
        hal_spi_write_reg(phy, 0x00A, 0x01   );
        hal_spi_write_reg(phy, 0x00B, 0x1E   );
        hal_spi_write_reg(phy, 0x00C, 0xFF   );
        hal_spi_write_reg(phy, 0x00D, 0x84   );
        hal_spi_write_reg(phy, 0x00A, 0x11   );
        hal_spi_write_reg(phy, 0x00A, 0x01   );
        hal_spi_write_reg(phy, 0x00B, 0x1F   );
        hal_spi_write_reg(phy, 0x00C, 0xFF   );
        hal_spi_write_reg(phy, 0x00D, 0xC6   );
        hal_spi_write_reg(phy, 0x00A, 0x11   );
        hal_spi_write_reg(phy, 0x00A, 0x01   );
        hal_spi_write_reg(phy, 0x00B, 0x20   );
        hal_spi_write_reg(phy, 0x00C, 0x00   );
        hal_spi_write_reg(phy, 0x00D, 0x85   );
        hal_spi_write_reg(phy, 0x00A, 0x11   );
        hal_spi_write_reg(phy, 0x00A, 0x01   );
        hal_spi_write_reg(phy, 0x00B, 0x21   );
        hal_spi_write_reg(phy, 0x00C, 0x00   );
        hal_spi_write_reg(phy, 0x00D, 0x63   );
        hal_spi_write_reg(phy, 0x00A, 0x11   );
        hal_spi_write_reg(phy, 0x00A, 0x01   );
        hal_spi_write_reg(phy, 0x00B, 0x22   );
        hal_spi_write_reg(phy, 0x00C, 0xFF   );
        hal_spi_write_reg(phy, 0x00D, 0x7B   );
        hal_spi_write_reg(phy, 0x00A, 0x11   );
        hal_spi_write_reg(phy, 0x00A, 0x01   );
        hal_spi_write_reg(phy, 0x00B, 0x23   );
        hal_spi_write_reg(phy, 0x00C, 0xFF   );
        hal_spi_write_reg(phy, 0x00D, 0x6C   );
        hal_spi_write_reg(phy, 0x00A, 0x11   );
        hal_spi_write_reg(phy, 0x00A, 0x01   );
        hal_spi_write_reg(phy, 0x00B, 0x24   );
        hal_spi_write_reg(phy, 0x00C, 0x00   );
        hal_spi_write_reg(phy, 0x00D, 0x7A   );
        hal_spi_write_reg(phy, 0x00A, 0x11   );
        hal_spi_write_reg(phy, 0x00A, 0x01   );
        hal_spi_write_reg(phy, 0x00B, 0x25   );
        hal_spi_write_reg(phy, 0x00C, 0x00   );
        hal_spi_write_reg(phy, 0x00D, 0xCA   );
        hal_spi_write_reg(phy, 0x00A, 0x11   );
        hal_spi_write_reg(phy, 0x00A, 0x01   );
        hal_spi_write_reg(phy, 0x00B, 0x26   );
        hal_spi_write_reg(phy, 0x00C, 0xFF   );
        hal_spi_write_reg(phy, 0x00D, 0xA0   );
        hal_spi_write_reg(phy, 0x00A, 0x11   );
        hal_spi_write_reg(phy, 0x00A, 0x01   );
        hal_spi_write_reg(phy, 0x00B, 0x27   );
        hal_spi_write_reg(phy, 0x00C, 0xFE   );
        hal_spi_write_reg(phy, 0x00D, 0xFD   );
        hal_spi_write_reg(phy, 0x00A, 0x11   );
        hal_spi_write_reg(phy, 0x00A, 0x01   );
        hal_spi_write_reg(phy, 0x00B, 0x28   );
        hal_spi_write_reg(phy, 0x00C, 0x00   );
        hal_spi_write_reg(phy, 0x00D, 0x34   );
        hal_spi_write_reg(phy, 0x00A, 0x11   );
        hal_spi_write_reg(phy, 0x00A, 0x01   );
        hal_spi_write_reg(phy, 0x00B, 0x29   );
        hal_spi_write_reg(phy, 0x00C, 0x01   );
        hal_spi_write_reg(phy, 0x00D, 0x3C   );
        hal_spi_write_reg(phy, 0x00A, 0x11   );
        hal_spi_write_reg(phy, 0x00A, 0x01   );
        hal_spi_write_reg(phy, 0x00B, 0x2A   );
        hal_spi_write_reg(phy, 0x00C, 0x00   );
        hal_spi_write_reg(phy, 0x00D, 0x0D   );
        hal_spi_write_reg(phy, 0x00A, 0x11   );
        hal_spi_write_reg(phy, 0x00A, 0x01   );
        hal_spi_write_reg(phy, 0x00B, 0x2B   );
        hal_spi_write_reg(phy, 0x00C, 0xFE   );
        hal_spi_write_reg(phy, 0x00D, 0x90   );
        hal_spi_write_reg(phy, 0x00A, 0x11   );
        hal_spi_write_reg(phy, 0x00A, 0x01   );
        hal_spi_write_reg(phy, 0x00B, 0x2C   );
        hal_spi_write_reg(phy, 0x00C, 0xFF   );
        hal_spi_write_reg(phy, 0x00D, 0x9B   );
        hal_spi_write_reg(phy, 0x00A, 0x11   );
        hal_spi_write_reg(phy, 0x00A, 0x01   );
        hal_spi_write_reg(phy, 0x00B, 0x2D   );
        hal_spi_write_reg(phy, 0x00C, 0x01   );
        hal_spi_write_reg(phy, 0x00D, 0x99   );
        hal_spi_write_reg(phy, 0x00A, 0x11   );
        hal_spi_write_reg(phy, 0x00A, 0x01   );
        hal_spi_write_reg(phy, 0x00B, 0x2E   );
        hal_spi_write_reg(phy, 0x00C, 0x00   );
        hal_spi_write_reg(phy, 0x00D, 0xD6   );
        hal_spi_write_reg(phy, 0x00A, 0x11   );
        hal_spi_write_reg(phy, 0x00A, 0x01   );
        hal_spi_write_reg(phy, 0x00B, 0x2F   );
        hal_spi_write_reg(phy, 0x00C, 0xFE   );
        hal_spi_write_reg(phy, 0x00D, 0x4F   );
        hal_spi_write_reg(phy, 0x00A, 0x11   );
        hal_spi_write_reg(phy, 0x00A, 0x01   );
        hal_spi_write_reg(phy, 0x00B, 0x30   );
        hal_spi_write_reg(phy, 0x00C, 0xFE   );
        hal_spi_write_reg(phy, 0x00D, 0x9C   );
        hal_spi_write_reg(phy, 0x00A, 0x11   );
        hal_spi_write_reg(phy, 0x00A, 0x01   );
        hal_spi_write_reg(phy, 0x00B, 0x31   );
        hal_spi_write_reg(phy, 0x00C, 0x01   );
        hal_spi_write_reg(phy, 0x00D, 0xB1   );
        hal_spi_write_reg(phy, 0x00A, 0x11   );
        hal_spi_write_reg(phy, 0x00A, 0x01   );
        hal_spi_write_reg(phy, 0x00B, 0x32   );
        hal_spi_write_reg(phy, 0x00C, 0x02   );
        hal_spi_write_reg(phy, 0x00D, 0x11   );
        hal_spi_write_reg(phy, 0x00A, 0x11   );
        hal_spi_write_reg(phy, 0x00A, 0x01   );
        hal_spi_write_reg(phy, 0x00B, 0x33   );
        hal_spi_write_reg(phy, 0x00C, 0xFE   );
        hal_spi_write_reg(phy, 0x00D, 0x73   );
        hal_spi_write_reg(phy, 0x00A, 0x11   );
        hal_spi_write_reg(phy, 0x00A, 0x01   );
        hal_spi_write_reg(phy, 0x00B, 0x34   );
        hal_spi_write_reg(phy, 0x00C, 0xFD   );
        hal_spi_write_reg(phy, 0x00D, 0x1D   );
        hal_spi_write_reg(phy, 0x00A, 0x11   );
        hal_spi_write_reg(phy, 0x00A, 0x01   );
        hal_spi_write_reg(phy, 0x00B, 0x35   );
        hal_spi_write_reg(phy, 0x00C, 0x01   );
        hal_spi_write_reg(phy, 0x00D, 0x38   );
        hal_spi_write_reg(phy, 0x00A, 0x11   );
        hal_spi_write_reg(phy, 0x00A, 0x01   );
        hal_spi_write_reg(phy, 0x00B, 0x36   );
        hal_spi_write_reg(phy, 0x00C, 0x03   );
        hal_spi_write_reg(phy, 0x00D, 0xE6   );
        hal_spi_write_reg(phy, 0x00A, 0x11   );
        hal_spi_write_reg(phy, 0x00A, 0x01   );
        hal_spi_write_reg(phy, 0x00B, 0x37   );
        hal_spi_write_reg(phy, 0x00C, 0xFF   );
        hal_spi_write_reg(phy, 0x00D, 0x67   );
        hal_spi_write_reg(phy, 0x00A, 0x11   );
        hal_spi_write_reg(phy, 0x00A, 0x01   );
        hal_spi_write_reg(phy, 0x00B, 0x38   );
        hal_spi_write_reg(phy, 0x00C, 0xFA   );
        hal_spi_write_reg(phy, 0x00D, 0xCA   );
        hal_spi_write_reg(phy, 0x00A, 0x11   );
        hal_spi_write_reg(phy, 0x00A, 0x01   );
        hal_spi_write_reg(phy, 0x00B, 0x39   );
        hal_spi_write_reg(phy, 0x00C, 0xFF   );
        hal_spi_write_reg(phy, 0x00D, 0x7D   );
        hal_spi_write_reg(phy, 0x00A, 0x11   );
        hal_spi_write_reg(phy, 0x00A, 0x01   );
        hal_spi_write_reg(phy, 0x00B, 0x3A   );
        hal_spi_write_reg(phy, 0x00C, 0x07   );
        hal_spi_write_reg(phy, 0x00D, 0x1D   );
        hal_spi_write_reg(phy, 0x00A, 0x11   );
        hal_spi_write_reg(phy, 0x00A, 0x01   );
        hal_spi_write_reg(phy, 0x00B, 0x3B   );
        hal_spi_write_reg(phy, 0x00C, 0x02   );
        hal_spi_write_reg(phy, 0x00D, 0xA3   );
        hal_spi_write_reg(phy, 0x00A, 0x11   );
        hal_spi_write_reg(phy, 0x00A, 0x01   );
        hal_spi_write_reg(phy, 0x00B, 0x3C   );
        hal_spi_write_reg(phy, 0x00C, 0xF5   );
        hal_spi_write_reg(phy, 0x00D, 0x68   );
        hal_spi_write_reg(phy, 0x00A, 0x11   );
        hal_spi_write_reg(phy, 0x00A, 0x01   );
        hal_spi_write_reg(phy, 0x00B, 0x3D   );
        hal_spi_write_reg(phy, 0x00C, 0xF8   );
        hal_spi_write_reg(phy, 0x00D, 0x00   );
        hal_spi_write_reg(phy, 0x00A, 0x11   );
        hal_spi_write_reg(phy, 0x00A, 0x01   );
        hal_spi_write_reg(phy, 0x00B, 0x3E   );
        hal_spi_write_reg(phy, 0x00C, 0x16   );
        hal_spi_write_reg(phy, 0x00D, 0x04   );
        hal_spi_write_reg(phy, 0x00A, 0x11   );
        hal_spi_write_reg(phy, 0x00A, 0x01   );
        hal_spi_write_reg(phy, 0x00B, 0x3F   );
        hal_spi_write_reg(phy, 0x00C, 0x36   );
        hal_spi_write_reg(phy, 0x00D, 0x6C   );
        hal_spi_write_reg(phy, 0x00A, 0x11   );
        hal_spi_write_reg(phy, 0x00A, 0x01   );
        hal_spi_write_reg(phy, 0x00A, 0x80	);
    }
}
#endif

int fn_rx_set_dc_remove_ena(rf_chip_phy_t *phy, int channel_sel, int ena)
{
    unsigned int w_addr;
    unsigned int w_data;
    unsigned int r_addr;
    unsigned int r_data;
    unsigned int flg_ok = 0;
    if (ena == 0)
    {
        r_addr = RX_DC_CTRL_0;
        r_data = hal_spi_read_reg(phy, r_addr);

        w_addr = r_addr;
        if (channel_sel == TRX_CHN1)
        {
            w_data = (r_data & 0xfa);
        }
        else
        {
            w_data = (r_data & 0xf5);
        }
        hal_spi_write_reg(phy, w_addr,w_data);
    }
    else
    {
        if (channel_sel == TRX_CHN1)
        {
            r_addr = RX_DC_CTRL_0;
            r_data = hal_spi_read_reg(phy, r_addr);

            w_addr = r_addr;
            w_data = (r_data & 0x1a) | (0xe5);
            hal_spi_write_reg(phy, w_addr,w_data);
        }
        else
        {
            r_addr = RX_DC_CTRL_0;
            r_data = hal_spi_read_reg(phy, r_addr);

            w_addr = r_addr;
            w_data = (r_data & 0x15) | (0xea);
            hal_spi_write_reg(phy, w_addr,w_data);
        }
    }
    flg_ok = 0;
    return flg_ok;
}

int fn_rx_get_adc_offset(rf_chip_phy_t *phy, int channel_sel, int try_num , int * dc_offset_i, int * dc_offset_q)
{
    unsigned int r_addr;
    unsigned int r_data;
    unsigned int flg_vld = 0 ;
    unsigned int high_bits = 0;
    unsigned int low_bits  = 0;
    int dc_val = 0;
    int mean_i = 0;
    int mean_q = 0;
    int i;
    int cnt;

    for (i = 0; i < try_num; i++)
    {
        fn_rx_set_dc_remove_ena(phy, channel_sel,0);
        fn_rx_set_dc_remove_ena(phy, channel_sel,1);
        r_addr = RX_DC_PWR_STATE_0;
        flg_vld = 0;
        cnt = 10;
        do
        {
            r_data = hal_spi_read_reg(phy, r_addr);
            flg_vld = (channel_sel == TRX_CHN1) ? (r_data & 0x1 ): ((r_data>>1)&0x1);

            if(--cnt == 0)
            {
                fn_rx_set_dc_remove_ena(phy, channel_sel, 0);
                *dc_offset_i = 0;
                *dc_offset_q = 0;
                LOG_ERROR("Rx%d get_adc_offset Timeout, Err.\n", channel_sel);
                return -1;
            }
        }while(flg_vld==0);

        r_addr = (channel_sel == TRX_CHN1) ? RX1I_DC_OFST_1: RX2I_DC_OFST_1;
        r_data = hal_spi_read_reg(phy, r_addr);
        high_bits = r_data&0x3;

        r_addr = (channel_sel == TRX_CHN1) ? RX1I_DC_OFST_0: RX2I_DC_OFST_0;
        r_data = hal_spi_read_reg(phy, r_addr);
        low_bits = r_data;

        dc_val = (high_bits<<8) + low_bits;
        dc_val = (dc_val >= 512 ) ? dc_val - 1024: dc_val;
        mean_i = mean_i + dc_val;

        r_addr = (channel_sel == TRX_CHN1) ? RX1Q_DC_OFST_1: RX2Q_DC_OFST_1;
        r_data = hal_spi_read_reg(phy, r_addr);
        high_bits = r_data&0x3;

        r_addr = (channel_sel == TRX_CHN1) ? RX1Q_DC_OFST_0: RX2Q_DC_OFST_0;
        r_data = hal_spi_read_reg(phy, r_addr);
        low_bits = r_data;

        dc_val = (high_bits<<8) + low_bits;
        dc_val = (dc_val >= 512 ) ? dc_val - 1024: dc_val;
        mean_q = mean_q + dc_val;
    }

    fn_rx_set_dc_remove_ena(phy, channel_sel,0);
    *dc_offset_i = mean_i;
    *dc_offset_q = mean_q;

    return 0;
}

int fn_rx_get_adc_power(rf_chip_phy_t *phy, int channel_sel, int try_num, int * mean_power)
{
    unsigned int w_addr;
    unsigned int w_data;
    unsigned int r_addr;
    unsigned int r_data;
    unsigned int flg_vld = 0 ;
    int i;

    int power_val = 0;
    int mean_power_t = 0;

    // CONFIG POWER MEAS NUM
    w_addr = RX_PWR_CTRL_0;
    w_data = 0x0A;
    hal_spi_write_reg(phy, w_addr,w_data);

    for (i = 0; i < try_num; i++)
    {
        fn_rx_set_dc_remove_ena(phy, channel_sel,0);
        fn_rx_set_dc_remove_ena(phy, channel_sel,1);
        r_addr = RX_DC_PWR_STATE_0;
        flg_vld = 0;
        do
        {
            r_data = hal_spi_read_reg(phy, r_addr);
            flg_vld = (channel_sel == TRX_CHN1) ? (r_data & 0x1 ): ((r_data>>1)&0x1);
        }
        while(flg_vld==0);

        // RUN POWER MEAS
        w_addr = RX_PWR_CTRL_0;
        w_data = (channel_sel == TRX_CHN1)? 0x1A : 0x2A;
        hal_spi_write_reg(phy, w_addr,w_data);

        r_addr = RX_DC_PWR_STATE_0;
        flg_vld = 0;
        do
        {
            r_data = hal_spi_read_reg(phy, r_addr);
            flg_vld = (channel_sel == TRX_CHN1) ? ((r_data>>2) & 0x1 ): ((r_data>>3)&0x1);
        }
        while(flg_vld==0);

        // read power
        power_val = 0;
        r_addr = (channel_sel == TRX_CHN1) ? RX1_AVG_PWR_2 : RX2_AVG_PWR_2;
        r_data = hal_spi_read_reg(phy, r_addr);
        power_val += ((r_data&0x1f) << 16);
        r_addr = (channel_sel == TRX_CHN1) ? RX1_AVG_PWR_1 : RX2_AVG_PWR_1;
        r_data = hal_spi_read_reg(phy, r_addr);
        power_val += (r_data << 8);
        r_addr = (channel_sel == TRX_CHN1) ? RX1_AVG_PWR_0 : RX2_AVG_PWR_0;
        r_data = hal_spi_read_reg(phy, r_addr);
        power_val += (r_data << 0);
        mean_power_t = mean_power_t + power_val;
    }
    * mean_power = mean_power_t;
    fn_rx_set_dc_remove_ena(phy, channel_sel,0);

    return 0;
}


int fn_tx_set_dc_manual(rf_chip_phy_t *phy, int channel_sel, int dc_i, int dc_q)
{
    unsigned int w_addr ;
    unsigned int w_data ;

    unsigned dc_i_u = (unsigned int)((dc_i < 0) ? (dc_i + (1<<14)) : dc_i);
    unsigned dc_q_u = (unsigned int)((dc_q < 0) ? (dc_q + (1<<14)) : dc_q);

    w_addr = (channel_sel == TRX_CHN1)? RG_TX1_DAC_22 : RG_TX2_DAC_22 ;
    w_data = dc_i_u & 0xFF;
    hal_spi_write_reg(phy, w_addr, w_data);

    w_addr = (channel_sel == TRX_CHN1)? RG_TX1_DAC_21 : RG_TX2_DAC_21 ;
    w_data = (dc_i_u>>8) & 0x3F;
    hal_spi_write_reg(phy, w_addr, w_data);

    w_addr = (channel_sel == TRX_CHN1)? RG_TX1_DAC_24 : RG_TX2_DAC_24 ;
    w_data = dc_q_u & 0xFF;
    hal_spi_write_reg(phy, w_addr, w_data);

    w_addr = (channel_sel == TRX_CHN1)? RG_TX1_DAC_23 : RG_TX2_DAC_23 ;
    w_data = (dc_q_u>>8) & 0x3F;
    hal_spi_write_reg(phy, w_addr, w_data);

    return 0;
}

int fn_tx_set_dc_manual_ena(rf_chip_phy_t *phy, int channel_sel,int ena_I,int ena_Q)
{
    unsigned int w_addr ;
    unsigned int w_data ;
    unsigned int r_addr ;
    unsigned int r_data ;

    // Set DC I enable or off
    r_addr = (channel_sel == TRX_CHN1)? RG_TX1_DAC_21 : RG_TX2_DAC_21 ;
    r_data = hal_spi_read_reg(phy, r_addr);

    w_addr = r_addr;
    w_data = r_data | (ena_I<<7);
    hal_spi_write_reg(phy, w_addr,w_data);

    // Set DC Q enable or off
    r_addr = (channel_sel == TRX_CHN1)? RG_TX1_DAC_23 : RG_TX2_DAC_23 ;
    r_data = hal_spi_read_reg(phy, r_addr);

    w_addr = r_addr;
    w_data = r_data | (ena_Q<<7);
    hal_spi_write_reg(phy, w_addr,w_data);

    return 0;
}

int fn_tx_set_dig_index(rf_chip_phy_t *phy, int channel_sel,int dbfs_p125dB)
{
    unsigned int w_addr   ;
    unsigned int w_data   ;
    int dbfs_p125dB_msb = (dbfs_p125dB>>8) & 0x1;
    int dbfs_p125dB_lsb = (dbfs_p125dB   ) & 0xff;

    if (channel_sel == TRX_CHN1)
    {
        w_addr = 0x0E5;
        w_data = dbfs_p125dB_msb;
        hal_spi_write_reg(phy, w_addr,w_data);

        w_addr = 0x0E6;
        w_data = dbfs_p125dB_lsb;
        hal_spi_write_reg(phy, w_addr,w_data);

		if (phy->config->chip_ver==2)
			HAL_REG_SET_BIT(phy, 0x0E5, BIT7);
    }
    else
    {
        w_addr = 0x0E7;
        w_data = dbfs_p125dB_msb;
        hal_spi_write_reg(phy, w_addr,w_data);

        w_addr = 0x0E8;
        w_data = dbfs_p125dB_lsb;
        hal_spi_write_reg(phy, w_addr,w_data);

		if (phy->config->chip_ver==2)
			HAL_REG_SET_BIT(phy, 0x0E7, BIT7);
    }

    return 1;
}



// to be add when accessed to code
int fn_rf_loop_back_ena(rf_chip_phy_t *phy, int channel_sel,int ena,int tdd_sel,RF_LOOPBACK_REGS * rf_loopback_regs)
{
	int rx_port_type;
#ifdef DIG_QEC_D1
    if(ena == 1)
    {
        if(channel_sel==0)
        {
            rf_loopback_regs->reg_0x65F = hal_spi_read_reg(phy, 0x65F); // LOW GAIN SELECT
            rf_loopback_regs->reg_0x660 = hal_spi_read_reg(phy, 0x660); //
            rf_loopback_regs->reg_0x661 = hal_spi_read_reg(phy, 0x661);
            rf_loopback_regs->reg_0x66D = hal_spi_read_reg(phy, 0x66D);

            rf_loopback_regs->reg_0x6D3 = hal_spi_read_reg(phy, 0x6D3);
            rf_loopback_regs->reg_0x6D4 = hal_spi_read_reg(phy, 0x6D4);
            rf_loopback_regs->reg_0x6D5 = hal_spi_read_reg(phy, 0x6D5);
            rf_loopback_regs->reg_0x6D6 = hal_spi_read_reg(phy, 0x6D6);
			rf_loopback_regs->reg_0x65C = hal_spi_read_reg(phy, 0x65C);

            //hal_spi_write_reg(phy,  0x600 , 0x03);  // 20221122 lix delete this reg

            //hal_spi_write_reg(phy,  0x660 , 0x00);
            //hal_spi_write_reg(phy,  0x65F , 0x00);
            //hal_spi_write_reg(phy,  0x660 , 0x05);

            hal_spi_write_reg(phy,  0x747 , 0x01);
            hal_spi_write_reg(phy,  0x790 , 0x09);

			//----------dac setting ----------------- //
			// blow is comment by liw 2022/12/21 leave DAC register unchanged
			//hal_spi_write_reg(phy,  0x7a3 , 0x01);
			//hal_spi_write_reg(phy,  0x791 , 0x00);
			//hal_spi_write_reg(phy,  0x7a4 , 0x00);
			//hal_spi_write_reg(phy,  0x791 , 0x2E);
			//hal_spi_write_reg(phy,  0x7a4 , 0x20);
			//hal_spi_write_reg(phy,  0x791 , 0x6F);
			//hal_spi_write_reg(phy,  0x7a4 , 0x21);
			//hal_spi_write_reg(phy,  0x791 , 0x7F);

			//hal_spi_write_reg(phy,  0x7C4 , 0x90);
			//hal_spi_write_reg(phy,  0x7C2 , 0x90);
			//if(tdd_sel==0)
			//{
			//    hal_spi_write_reg(phy,  0x702 , 0x0F);
			//}
            rf_loopback_regs->reg_0x74A = hal_spi_read_reg(phy, 0x74A);  // if TX1, then close TX2
            hal_spi_write_reg(phy,  0x74A , 0x00);

            //hal_spi_write_reg(phy,  0x703 , 0x07); // 20221122 lix delete this reg
            rx_port_type = fn_rx_get_mixer_port(phy, channel_sel);
            // 0:AP|1:AN|2:APN
            // 3:BP|4:BN|5:BPN
            // 6:CP|7:CN|8:CPN

            switch( rx_port_type )
            {
                case 0: hal_spi_write_reg(phy,  0x847 , 0x10);break;
                case 1: hal_spi_write_reg(phy,  0x847 , 0x01);break;
                case 2: hal_spi_write_reg(phy,  0x847 , 0x11);break;
                case 3: hal_spi_write_reg(phy,  0x847 , 0x20);break;
                case 4: hal_spi_write_reg(phy,  0x847 , 0x02);break;
                case 5: hal_spi_write_reg(phy,  0x847 , 0x22);break;
                case 6: hal_spi_write_reg(phy,  0x847 , 0x40);break;
                case 7: hal_spi_write_reg(phy,  0x847 , 0x04);break;
                case 8: hal_spi_write_reg(phy,  0x847 , 0x44);break;
            }
			
            hal_spi_write_reg(phy,  0x848 , 0x0C); // rg_lpbk pa or abb
            hal_spi_write_reg(phy,  0x700 , 0x00); // 20221122 by lix
			
			hal_spi_write_reg(phy,  0x73D , 0x0);
			hal_spi_write_reg(phy,  0x73E , 0x0);
			hal_spi_write_reg(phy,  0x73F , 0x0);
        }
        else
        {
            rf_loopback_regs->reg_0x65F = hal_spi_read_reg(phy, 0x67D);
            rf_loopback_regs->reg_0x660 = hal_spi_read_reg(phy, 0x67E);
            rf_loopback_regs->reg_0x661 = hal_spi_read_reg(phy, 0x67F);
            rf_loopback_regs->reg_0x66D = hal_spi_read_reg(phy, 0x68B);

            rf_loopback_regs->reg_0x6D3 = hal_spi_read_reg(phy, 0x6E3);
            rf_loopback_regs->reg_0x6D4 = hal_spi_read_reg(phy, 0x6E4);
            rf_loopback_regs->reg_0x6D5 = hal_spi_read_reg(phy, 0x6E5);
            rf_loopback_regs->reg_0x6D6 = hal_spi_read_reg(phy, 0x6E6);
			rf_loopback_regs->reg_0x65C = hal_spi_read_reg(phy, 0x67A);
			
            //hal_spi_write_reg(phy, 0x600,0x03);// 20221122 lix delete this reg
			rf_loopback_regs->reg_0x74A = hal_spi_read_reg(phy, 0x702);
			hal_spi_write_reg(phy, 0x702,0x00);
            //hal_spi_write_reg(phy, 0x67E,0x00);
            //hal_spi_write_reg(phy, 0x67D,0x00);
            //hal_spi_write_reg(phy, 0x67E,0x05);

			hal_spi_write_reg(phy, 0x78f,0x09);
			// blow is comment by liw 2022/12/21 leave DAC register unchanged
			//hal_spi_write_reg(phy, 0x790,0x01);
			//hal_spi_write_reg(phy, 0x7a3,0x01);
			//hal_spi_write_reg(phy, 0x791,0x00);
			//hal_spi_write_reg(phy, 0x7a4,0x00);
			//hal_spi_write_reg(phy, 0x791,0x20);
			//hal_spi_write_reg(phy, 0x7a4,0x2E);
			//hal_spi_write_reg(phy, 0x791,0x21);
			//hal_spi_write_reg(phy, 0x7a4,0x6F);
			//hal_spi_write_reg(phy, 0x7a4,0x7F);
			//hal_spi_write_reg(phy, 0x790,0x01);

			//hal_spi_write_reg(phy, 0x7d4,0x90);
			//hal_spi_write_reg(phy, 0x7d2,0x90);
			if(tdd_sel==0)
			{
				hal_spi_write_reg(phy, 0x74a,0x0f);
			}
			//hal_spi_write_reg(phy, 0x74b,0x07); // 20221122 lix delete this reg

            rx_port_type = fn_rx_get_mixer_port(phy, channel_sel);
            switch( rx_port_type )
            {
                case 0: hal_spi_write_reg(phy,  0x846 , 0x10);break;
                case 1: hal_spi_write_reg(phy,  0x846 , 0x01);break;
                case 2: hal_spi_write_reg(phy,  0x846 , 0x11);break;
                case 3: hal_spi_write_reg(phy,  0x846 , 0x20);break;
                case 4: hal_spi_write_reg(phy,  0x846 , 0x02);break;
                case 5: hal_spi_write_reg(phy,  0x846 , 0x22);break;
                case 6: hal_spi_write_reg(phy,  0x846 , 0x40);break;
                case 7: hal_spi_write_reg(phy,  0x846 , 0x04);break;
                case 8: hal_spi_write_reg(phy,  0x846 , 0x44);break;
            }

			hal_spi_write_reg(phy, 0x848,0x03);
			hal_spi_write_reg(phy, 0x748,0x00);
			
			hal_spi_write_reg(phy,  0x785 , 0x0);
			hal_spi_write_reg(phy,  0x786 , 0x0);
			hal_spi_write_reg(phy,  0x787 , 0x0);
		}
	}
	else
	{
		hal_spi_write_reg(phy, 0x846,0x00);
		hal_spi_write_reg(phy, 0x847,0x00);
		hal_spi_write_reg(phy, 0x848,0x00);
		hal_spi_write_reg(phy, 0x849,0x00);
		hal_spi_write_reg(phy, 0x84A,0x00);
		hal_spi_write_reg(phy, 0x84B,0x00);

		if(channel_sel==0)
		{
			hal_spi_write_reg(phy, 0x65F,rf_loopback_regs->reg_0x65F); // MIXER GAIN
			hal_spi_write_reg(phy, 0x660,rf_loopback_regs->reg_0x660); // MIXER HIGH/LOW GAIN SELECT
			hal_spi_write_reg(phy, 0x661,rf_loopback_regs->reg_0x661); // MIXER GAIN
			hal_spi_write_reg(phy, 0x66D,rf_loopback_regs->reg_0x66D); // MIXER HIGH/LOW GAIN SELECT
			
			hal_spi_write_reg(phy, 0x6D3,rf_loopback_regs->reg_0x6D3);
			hal_spi_write_reg(phy, 0x6D4,rf_loopback_regs->reg_0x6D4);
			hal_spi_write_reg(phy, 0x6D5,rf_loopback_regs->reg_0x6D5);
			hal_spi_write_reg(phy, 0x6D6,rf_loopback_regs->reg_0x6D6);
			hal_spi_write_reg(phy, 0x65C,rf_loopback_regs->reg_0x65C);
			hal_spi_write_reg(phy, 0x74A,rf_loopback_regs->reg_0x74A);
		}
		else
		{
			hal_spi_write_reg(phy, 0x67D,rf_loopback_regs->reg_0x65F); // MIXER GAIN
			hal_spi_write_reg(phy, 0x67E,rf_loopback_regs->reg_0x660); // MIXER HIGH/LOW GAIN SELECT
			hal_spi_write_reg(phy, 0x67F,rf_loopback_regs->reg_0x661); // MIXER GAIN
			hal_spi_write_reg(phy, 0x68B,rf_loopback_regs->reg_0x66D); // MIXER HIGH/LOW GAIN SELECT
			
			hal_spi_write_reg(phy, 0x6E3,rf_loopback_regs->reg_0x6D3);
			hal_spi_write_reg(phy, 0x6E4,rf_loopback_regs->reg_0x6D4);
			hal_spi_write_reg(phy, 0x6E5,rf_loopback_regs->reg_0x6D5);
			hal_spi_write_reg(phy, 0x6E6,rf_loopback_regs->reg_0x6D6);
			hal_spi_write_reg(phy, 0x67A,rf_loopback_regs->reg_0x65C);
			hal_spi_write_reg(phy, 0x702,rf_loopback_regs->reg_0x74A);
		}
	}
#else
    if(ena == 1)
    {
        rf_loopback_regs->reg_0x604 = hal_spi_read_reg(phy, 0x604);
        if (channel_sel == 0)
        {
            hal_spi_write_reg(phy, 0x600, 0x03);
            hal_spi_write_reg(phy, 0x604, 0x0B);

            hal_spi_write_reg(phy, 0x747, 0x01);
            hal_spi_write_reg(phy, 0x790, 0x01);

            //TXDAC DC
            hal_spi_write_reg(phy, 0x790, 0x01);
            hal_spi_write_reg(phy, 0x791, 0x0F);
            hal_spi_write_reg(phy, 0x791, 0x2F);
            hal_spi_write_reg(phy, 0x7A1, 0x00);

            //CHI
            hal_spi_write_reg(phy, 0x7C4, 0x90);
            //CHQ
            hal_spi_write_reg(phy, 0x7C2, 0x90);

            hal_spi_write_reg(phy, 0x700, 0x04);
            hal_spi_write_reg(phy, 0x702, 0x0F);
            hal_spi_write_reg(phy, 0x703, 0x07);
            hal_spi_write_reg(phy, 0x739, 0x88);

            //RFLB
            //TX1to RX1C
            hal_spi_write_reg(phy, 0x847, 0x11);
            hal_spi_write_reg(phy, 0x848, 0x0C);

            //RXBBF to TP
            hal_spi_write_reg(phy, 0x837, 0x00);

            //TX1port off
            hal_spi_write_reg(phy, 0x700, 0x00);
        }
        else
        {
            hal_spi_write_reg(phy, 0x600, 0x03);
            hal_spi_write_reg(phy, 0x604, 0x03);

            hal_spi_write_reg(phy, 0x78f, 0x01);
            hal_spi_write_reg(phy, 0x7a3, 0x01);

            //TXDAC DC
            hal_spi_write_reg(phy, 0x7a3, 0x01);
            hal_spi_write_reg(phy, 0x7a4, 0x0f);
            hal_spi_write_reg(phy, 0x7a4, 0x2f);
            hal_spi_write_reg(phy, 0x7b4, 0x00);

            //CHI
            hal_spi_write_reg(phy, 0x7d4, 0x90);
            //CHQ
            hal_spi_write_reg(phy, 0x7d2, 0x90);

            hal_spi_write_reg(phy, 0x748, 0x04);
            hal_spi_write_reg(phy, 0x74a, 0x0f);
            hal_spi_write_reg(phy, 0x74b, 0x07);
            hal_spi_write_reg(phy, 0x781, 0x88);

            //RFLB
            //TX1to RX1C
            hal_spi_write_reg(phy, 0x846, 0x11);
            hal_spi_write_reg(phy, 0x848, 0x03);

            //RXBBF to TP
            hal_spi_write_reg(phy, 0x836, 0x00);

            //TX1port off
            hal_spi_write_reg(phy, 0x748, 0x00);
        }
    }
    else
    {
        hal_spi_write_reg(phy, 0x846, 0x00);
        hal_spi_write_reg(phy, 0x847, 0x00);
        hal_spi_write_reg(phy, 0x848, 0x00);
        hal_spi_write_reg(phy, 0x849, 0x00);
        hal_spi_write_reg(phy, 0x84A, 0x00);
        hal_spi_write_reg(phy, 0x84B, 0x00);
        hal_spi_write_reg(phy, 0x604, rf_loopback_regs->reg_0x604);
    }
#endif
    return 0;
}


int fn_rf_loop_back_lg_ena(rf_chip_phy_t *phy, int channel_sel,int ena,RF_LOOPBACK_REGS * rf_loopback_regs)
{
	if(ena==1)
	{
		rf_loopback_regs->reg_0xD33 = hal_spi_read_reg(phy, 0xD33);
		//LOG_MDEBUG(phy,TRX_QEC_CAL,"read 0xD33 0x%x",rf_loopback_regs->reg_0xD33);
		if(channel_sel==TRX_CHN1)
		{
			rf_loopback_regs->reg_0x71B = hal_spi_read_reg(phy, 0x71B);
			rf_loopback_regs->reg_0x71C = hal_spi_read_reg(phy, 0x71C);
			rf_loopback_regs->reg_0x71F = hal_spi_read_reg(phy, 0x71F);
			//hal_spi_write_reg(phy, 0x65F,0x00);
			//hal_spi_write_reg(phy, 0x660,0x0A);
			//hal_spi_write_reg(phy, 0x65C,0x0C);

            fn_rx_ana_set_gain(phy, channel_sel,2,12);
            hal_spi_write_reg(phy, 0x65F,0x00);
			hal_spi_write_reg(phy, 0xD33,((rf_loopback_regs->reg_0xD33 & 0xF0 ) | 0x02));
			hal_spi_write_reg(phy, 0x71B,0x4D);
			hal_spi_write_reg(phy, 0x71C,0x4D);
			hal_spi_write_reg(phy, 0x71F,0x00);
		}
		else
		{
			rf_loopback_regs->reg_0x71B = hal_spi_read_reg(phy, 0x763);
			rf_loopback_regs->reg_0x71C = hal_spi_read_reg(phy, 0x764);
			rf_loopback_regs->reg_0x71F = hal_spi_read_reg(phy, 0x767);
			//hal_spi_write_reg(phy, 0x67D,0x00);
			//hal_spi_write_reg(phy, 0x67F,0x0A);
			//hal_spi_write_reg(phy, 0x67A,0x0C);

            fn_rx_ana_set_gain(phy, channel_sel,2,12);
            hal_spi_write_reg(phy, 0x67D,0x00);
			hal_spi_write_reg(phy, 0xD33,((rf_loopback_regs->reg_0xD33 & 0xF0 ) | 0x01));
			hal_spi_write_reg(phy, 0x763,0x4D);
			hal_spi_write_reg(phy, 0x764,0x4D);
			hal_spi_write_reg(phy, 0x767,0x00);
		}
	}
	else
	{
		hal_spi_write_reg(phy, 0xD33,rf_loopback_regs->reg_0xD33);
		//LOG_MDEBUG(phy,TRX_QEC_CAL,"write back 0xD33 0x%x\n",hal_spi_read_reg(phy, 0xD33));
	
		if(channel_sel==TRX_CHN1)
		{
			hal_spi_write_reg(phy, 0x71B,rf_loopback_regs->reg_0x71B);
			hal_spi_write_reg(phy, 0x71C,rf_loopback_regs->reg_0x71C);
			hal_spi_write_reg(phy, 0x71F,rf_loopback_regs->reg_0x71F);
		}
		else
		{
			hal_spi_write_reg(phy, 0x763,rf_loopback_regs->reg_0x71B);
			hal_spi_write_reg(phy, 0x764,rf_loopback_regs->reg_0x71C);
			hal_spi_write_reg(phy, 0x767,rf_loopback_regs->reg_0x71F);
		}
	}

	return 1;
}


int fn_if_loop_back_ena(rf_chip_phy_t *phy, int channel_sel , int ena ,IF_LOOPBACK_REGS *if_loopback_reg)
{
	if (ena)
	{

		//hal_spi_write_reg(phy, 0x790,0x09);
		//hal_spi_write_reg(phy, 0x7a3,0x09);
		//hal_spi_write_reg(phy, 0x791,0x00);
		//hal_spi_write_reg(phy, 0x7a4,0x00);
		//hal_spi_write_reg(phy, 0x791,0x2E);
		//hal_spi_write_reg(phy, 0x7a4,0x2E);
		//hal_spi_write_reg(phy, 0x791,0x6F);
		//hal_spi_write_reg(phy, 0x7a4,0x6F);
		//hal_spi_write_reg(phy, 0x791,0x7F);
		//hal_spi_write_reg(phy, 0x7a4,0x7F);
		if (channel_sel == 0)
		{
            if_loopback_reg->reg_0x742 = hal_spi_read_reg(phy, 0x742);
            if_loopback_reg->reg_0x743 = hal_spi_read_reg(phy, 0x743);
            if_loopback_reg->reg_0x744 = hal_spi_read_reg(phy, 0x744);
            if_loopback_reg->reg_0x745 = hal_spi_read_reg(phy, 0x745);
            if_loopback_reg->reg_0x65F = hal_spi_read_reg(phy, 0x65F);
            if_loopback_reg->reg_0x660 = hal_spi_read_reg(phy, 0x660);
            if_loopback_reg->reg_0x661 = hal_spi_read_reg(phy, 0x661);
            if_loopback_reg->reg_0x66D = hal_spi_read_reg(phy, 0x66D);

			hal_spi_write_reg(phy, 0x790, 0x09);
			hal_spi_write_reg(phy, 0x848, 0x00);
			hal_spi_write_reg(phy, 0x849, 0x0F);
			hal_spi_write_reg(phy, 0x84B, 0x10);

            //SQ
            hal_spi_write_reg(phy, 0x742, 0x42);
            hal_spi_write_reg(phy, 0x743, 0x86);
            hal_spi_write_reg(phy, 0x744, 0x01);
            hal_spi_write_reg(phy, 0x745, 0x14);

            //RXFE OFF
            hal_spi_write_reg(phy, 0x660, 0x0);
            hal_spi_write_reg(phy, 0x661, 0x0);

            //TX port OFF
            hal_spi_write_reg(phy, 0x700, 0x0);

            //TX DAC DC OFF
            hal_spi_write_reg(phy, 0x7c2, 0x0);
            hal_spi_write_reg(phy, 0x7c4, 0x0);
        }
        else
        {

			if_loopback_reg->reg_0x742 = hal_spi_read_reg(phy, 0x78A);
			if_loopback_reg->reg_0x743 = hal_spi_read_reg(phy, 0x78B);
			if_loopback_reg->reg_0x744 = hal_spi_read_reg(phy, 0x78C);
			if_loopback_reg->reg_0x745 = hal_spi_read_reg(phy, 0x78D);
			if_loopback_reg->reg_0x65F = hal_spi_read_reg(phy, 0x67D);
			if_loopback_reg->reg_0x660 = hal_spi_read_reg(phy, 0x67E);
			if_loopback_reg->reg_0x661 = hal_spi_read_reg(phy, 0x67F);
			if_loopback_reg->reg_0x66D = hal_spi_read_reg(phy, 0x68B);


			hal_spi_write_reg(phy, 0x7A3,0x09);

			hal_spi_write_reg(phy, 0x848, 0x00);
			hal_spi_write_reg(phy, 0x849, 0xF0);
			hal_spi_write_reg(phy, 0x84B, 0x01);
			//SQ
			hal_spi_write_reg(phy, 0x78A, 0x42);
			hal_spi_write_reg(phy, 0x78B, 0x86);
			hal_spi_write_reg(phy, 0x78C, 0x01);
			hal_spi_write_reg(phy, 0x78D, 0x14);

            //RXFE OFF
            hal_spi_write_reg(phy, 0x67E, 0x0);
            hal_spi_write_reg(phy, 0x67F, 0x0);

            //TX port OFF
            hal_spi_write_reg(phy, 0x748, 0x0);

            //TX DAC DC OFF
            hal_spi_write_reg(phy, 0x7D2, 0x0);
            hal_spi_write_reg(phy, 0x7D4, 0x0);
        }
    }
    else
    {
        hal_spi_write_reg(phy, 0x848, 0x0  );
        hal_spi_write_reg(phy, 0x849, 0x0  );
        hal_spi_write_reg(phy, 0x84B, 0x0  );
        if (channel_sel == 0)
        {
            hal_spi_write_reg(phy, 0x742,if_loopback_reg->reg_0x742);
            hal_spi_write_reg(phy, 0x743,if_loopback_reg->reg_0x743);
            hal_spi_write_reg(phy, 0x744,if_loopback_reg->reg_0x744);
            hal_spi_write_reg(phy, 0x745,if_loopback_reg->reg_0x745);

            hal_spi_write_reg(phy, 0x65F,if_loopback_reg->reg_0x65F);
            hal_spi_write_reg(phy, 0x660,if_loopback_reg->reg_0x660);
            hal_spi_write_reg(phy, 0x661,if_loopback_reg->reg_0x661);
            hal_spi_write_reg(phy, 0x66D,if_loopback_reg->reg_0x66D);

        }
        else
        {
            hal_spi_write_reg(phy, 0x78A,if_loopback_reg->reg_0x742);
            hal_spi_write_reg(phy, 0x78B,if_loopback_reg->reg_0x743);
            hal_spi_write_reg(phy, 0x74C,if_loopback_reg->reg_0x744);
            hal_spi_write_reg(phy, 0x74D,if_loopback_reg->reg_0x745);

            hal_spi_write_reg(phy, 0x67D,if_loopback_reg->reg_0x65F);
            hal_spi_write_reg(phy, 0x67E,if_loopback_reg->reg_0x660);
            hal_spi_write_reg(phy, 0x67F,if_loopback_reg->reg_0x661);
            hal_spi_write_reg(phy, 0x68B,if_loopback_reg->reg_0x66D);

        }

    }

    return 0;
}

//-------------------------------
int fn_cal_inverse_dft32_point(double din_i[32], double din_q[32], double dout[32])
{
    int fq_idx, ph_idx;

    memset(dout, 0, sizeof(double) * 32);
    for (fq_idx = 0; fq_idx < 32; fq_idx++)
    {
        for (ph_idx = 0; ph_idx < 32; ph_idx++)
        {
            dout[fq_idx] = dout[fq_idx] + din_i[ph_idx] * cos(2 * PI * 1 / 32.0 * ph_idx * fq_idx) - din_q[ph_idx] * sin(2 * PI * 1 / 32.0 * ph_idx * fq_idx);
        }
        dout[fq_idx] = dout[fq_idx] / 32.0;
    }
    return 0;

}

int fn_get_quad_ph_err_deg(int i_dft_real, int i_dft_imag, int q_dft_real, int q_dft_imag, double *mag_err_ratio , double *ph_err_deg)
{
    double ph_error_deg = 0;
    double ph_i_rad = atan2((double)i_dft_imag,(double)i_dft_real);
    double ph_q_rad = atan2((double)q_dft_imag,(double)q_dft_real);
    double comparator_0 = ph_q_rad - ph_i_rad + PI/2;
    double comparator_1 = comparator_0 + 2*PI;
    double comparator_2 = comparator_0 - 2*PI;

    ph_error_deg = (fabs(comparator_0) > fabs(comparator_1)) ? comparator_1 : comparator_0;
    ph_error_deg = (fabs(ph_error_deg) > fabs(comparator_2)) ? comparator_2 : ph_error_deg;

    ph_error_deg = ph_error_deg / 2.0/PI *360.0;
    *ph_err_deg = ph_error_deg;

    *mag_err_ratio =  sqrt(pow((double)q_dft_real,2.0)+pow((double)q_dft_imag,2.0)) /  sqrt(pow((double)i_dft_real,2.0)+pow((double)i_dft_imag,2.0));

    return 0;
    //return ph_error_deg;
}

int    fn_rx_ana_offset_cal_1brach   (rf_chip_phy_t *phy, int chn,int iq_sel)
{
	int          stage;;
	unsigned int reg_tia_offset;
	unsigned int reg_bbf_offset;
	int          i_mean        ;
	int          q_mean        ;
	int          current_offset;
	int          decay_coef_acc;
	int          decay_coef_k0 ;
	int          acc           ;
	int          cnt_255       ;
	int          cnt_0         ;
	int          sign_table[2][2];

    sign_table[0][0] =  1;
    sign_table[0][1] = -1;
    sign_table[1][0] = -1;
    sign_table[1][1] =  1;

    if(chn==0)
    {
        if(iq_sel==0)
        {
            reg_tia_offset = 0x6D5;
            reg_bbf_offset = 0x6D6;
        }
        else
        {
            reg_tia_offset = 0x6D3;
            reg_bbf_offset = 0x6D4;
        }
    }
    else
    {
        if(iq_sel==0)
        {
            reg_tia_offset = 0x6E5;
            reg_bbf_offset = 0x6E6;
        }
        else
        {
            reg_tia_offset = 0x6E3;
            reg_bbf_offset = 0x6E4;
        }

    }
    hal_spi_write_reg(phy, reg_tia_offset,0x80);
    hal_spi_write_reg(phy, reg_bbf_offset,0x80);

    // search for tia offset
    for(stage = 0; stage  < 2; stage ++)
    {
        current_offset = 128;
        decay_coef_acc = 7;
        decay_coef_k0  = 54;
        acc = 0;
        cnt_255 = 0;
        cnt_0   = 0;

        if(stage==0)
        {
            //tmp = "";
            //tmp.sprintf("search for tia offset");
            //qDebug() << "" << tmp;
            LOG_MDEBUG(phy, TRX_QEC_CAL, "search for tia offset\n");
        }
        else
        {
            //tmp = "";
            //tmp.sprintf("search for bbf offset");
            //qDebug() << "" << tmp;
            LOG_MDEBUG(phy, TRX_QEC_CAL, "search for bbf offset\n");
        }


        while(1)
        {
            fn_rx_get_adc_offset(phy, chn,10,&i_mean,&q_mean);
            //tmp = "";
            if(iq_sel==0)
                //tmp.sprintf("mean_i : %.1f set reg 0x%x\n",((float)i_mean)/10.0,current_offset);
                LOG_MDEBUG(phy, TRX_QEC_CAL, "mean_i : %.1f set reg 0x%x\n",((float)i_mean)/10.0,current_offset);
            else
                //tmp.sprintf("mean_q : %.1f set reg 0x%x\n",((float)i_mean)/10.0,current_offset);
                LOG_MDEBUG(phy, TRX_QEC_CAL, "mean_q : %.1f set reg 0x%x\n",((float)i_mean)/10.0,current_offset);
            //qDebug() << "" << tmp;

            if ( abs(i_mean / 10) < 50 )
                break;

            acc = acc + i_mean;

            current_offset = ( current_offset*16384 + acc*decay_coef_acc*sign_table[iq_sel][stage] + i_mean*decay_coef_k0*sign_table[iq_sel][stage]) / 16384;

            if(current_offset >= 255)
            {
                current_offset = 255;
                cnt_255 = cnt_255 + 1;
            }
            else if(current_offset<0)
            {
                current_offset = 0;
                cnt_0 = cnt_0 + 1;
            }
            else
            {
                cnt_255 = 0;
                cnt_0   = 0;
            }

            if( cnt_0 > 3 || cnt_255 >3)
            {
                break;
            }

			if(stage == 0)
			{
				hal_spi_write_reg(phy, reg_tia_offset,current_offset);
			}
			else
			{
				hal_spi_write_reg(phy, reg_bbf_offset,current_offset);
			}
		}
	}
	return 0;
}

void fn_float_add(int A_val, int A_exp, int B_val, int B_exp,int * C_val, int *C_exp)
{
    long long max_val;
    long long max_exp;
    long long min_val;
    long long min_exp;

    if(A_exp>B_exp)
    {
        max_exp = (long long)A_exp;
        max_val = (long long)A_val;
        min_exp = (long long)B_exp;
        min_val = (long long)B_val;
    }
    else
    {
        max_exp = (long long)B_exp;
        max_val = (long long)B_val;
        min_exp = (long long)A_exp;
        min_val = (long long)A_val;
    }

    max_val = max_val << (long long)(max_exp - min_exp);
    min_val = max_val + min_val;

    while( llabs(min_val) > (1ll<<31ll) )
    {
        min_val = min_val >> 1ll;
        min_exp ++;
    }
    *C_val = (int)( min_val );
    *C_exp = (int)( min_exp );
}


void fn_float_mul(int A_val, int A_exp, int B_val,int B_exp,int * C_val, int *C_exp)
{
    long long result_val = (long long)A_val * (long long)B_val;
    long long result_exp = (long long)(A_exp + B_exp);
    while( llabs(result_val) > (1ll<<31ll) )
    {
        result_val = result_val >> 1ll;
        result_exp++;
    }
    *C_val = (int)( result_val );
    *C_exp = (int)( result_exp );
}

void fn_float_div(int A_val, int A_exp, int B_val,int B_exp,int * C_val, int *C_exp)
{
    long long result_val = (((long long)A_val)<<32ll) / (long long)B_val;
    long long result_exp = A_exp - B_exp-32LL;
    while( llabs(result_val) > (1ll<<31ll) )
    {
        result_val = result_val >> 1ll;
        result_exp ++;
    }
    *C_val = (int)( result_val );
    *C_exp = (int)( result_exp );
}

int fn_rx_ana_offset_cal_V2 (rf_chip_phy_t *phy,int chn)
{
	int tia_offset_i;
	int tia_offset_q;
	int DC_I[3];
	int DC_Q[3];
	int x0,y0,x1,y1;
	int xm_0,ym_0,xm_1,ym_1;
	int A,B,C,D,E,F;
	int I_code = 128;
	int Q_code = 128;
	int Br0,Br1,Br2,Br3,Br4;
	int cnt = 0;
	int reg_tia_offset_i;
	int reg_tia_offset_q;
	int flag_ok = 1;
	if(chn==0)
	{
		reg_tia_offset_i = 0x6D5;
		reg_tia_offset_q = 0x6D3;
	}
	else
	{
		reg_tia_offset_i = 0x6E5;
		reg_tia_offset_q = 0x6E3;
	}	
	
	tia_offset_i = 64;
	tia_offset_q = 64;
	hal_spi_write_reg(phy, reg_tia_offset_i,tia_offset_i);
	hal_spi_write_reg(phy, reg_tia_offset_q,tia_offset_q);
	fn_rx_get_adc_offset(phy, chn,10,&DC_I[0],&DC_Q[0]);

	tia_offset_i = 93;
	tia_offset_q = 77;
	hal_spi_write_reg(phy, reg_tia_offset_i,tia_offset_i);
	hal_spi_write_reg(phy, reg_tia_offset_q,tia_offset_q);
	fn_rx_get_adc_offset(phy, chn,10,&DC_I[1],&DC_Q[1]);

	tia_offset_i = 192;
	tia_offset_q = 192;
	hal_spi_write_reg(phy, reg_tia_offset_i,tia_offset_i);
	hal_spi_write_reg(phy, reg_tia_offset_q,tia_offset_q);
	fn_rx_get_adc_offset(phy, chn,10,&DC_I[2],&DC_Q[2]);	
	
	//LOG_MDEBUG(phy, TRX_QEC_CAL,"DC_I0 %10d DC_I1 %10d DCI2 %10d\n",DC_I[0],DC_I[1],DC_I[2]);
	//LOG_MDEBUG(phy, TRX_QEC_CAL,"DC_Q0 %10d DC_Q1 %10d DCQ2 %10d\n",DC_Q[0],DC_Q[1],DC_Q[2]);
		
	if(abs(DC_I[0]-DC_I[1]) <100 && abs(DC_I[0]-DC_I[2]) <100 && abs(DC_Q[0]-DC_Q[1]) <100 && abs(DC_Q[0]-DC_Q[2]) <100)
	{
		LOG_ERROR("dc remove error\n");
		LOG_ERROR("DC_I0 %10d DC_I1 %10d DCI2 %10d\n",DC_I[0],DC_I[1],DC_I[2]);
		LOG_ERROR("DC_Q0 %10d DC_Q1 %10d DCQ2 %10d\n",DC_Q[0],DC_Q[1],DC_Q[2]);
		return -1;
	}
	
	
	x0 = 29;y0=13;x1=128;y1=128;
	xm_0 = DC_I[1] - DC_I[0];
	xm_1 = DC_I[2] - DC_I[0];
	ym_0 = DC_Q[1] - DC_Q[0];
	ym_1 = DC_Q[2] - DC_Q[0];
	
	B =     (xm_0*y1 - xm_1*y0);
	//B =  B>>11;
   
	C =  -1*(xm_0*x1 - xm_1*x0);
	//C =  C>>11;

	A =  (DC_I[0]<<11) - B*64 - C*64;

	E =     (ym_0*y1 - ym_1*y0);
	//E =  E>>11;

	F =  -1*(ym_0*x1 - ym_1*x0);
	//F =  F>>11;

	D =  (DC_Q[0]<<11) - E*64 - F*64;		
	

	int T0_val,T0_exp;
	int T1_val,T1_exp;
	int H0_val,H0_exp;
	int H1_val,H1_exp;
	int H2_val,H2_exp;
	fn_float_mul(-1*A,-11,   F,-11,&T0_val,&T0_exp);
	fn_float_mul(   C,-11,-1*D,-11,&T1_val,&T1_exp);
	fn_float_add(T0_val,T0_exp,-1*T1_val,T1_exp,&H0_val,&H0_exp);
	fn_float_mul( 1*B,-11,F,-11,&T0_val,&T0_exp);
	fn_float_mul( 1*C,-11,E,-11,&T1_val,&T1_exp);	
	fn_float_add(T0_val,T0_exp,-1*T1_val,T1_exp,&H1_val,&H1_exp);
	fn_float_div(H0_val,H0_exp,H1_val,H1_exp,&H2_val,&H2_exp);
	I_code = ROUND_SHIFT(H2_val , (-1*H2_exp));
	//LOG_MDEBUG(phy,TRX_QEC_CAL,"I_code = %d | H2 val %d H2 exp %d\n",I_code,H2_val,H2_exp);
	fn_float_mul(   B,-11,-1*D,-11,&T0_val,&T0_exp);
	fn_float_mul(-1*A,-11,   E,-11,&T1_val,&T1_exp);
	fn_float_add(T0_val,T0_exp,-1*T1_val,T1_exp,&H0_val,&H0_exp);
	fn_float_mul( 1*B,-11,F,-11,&T0_val,&T0_exp);
	fn_float_mul( 1*C,-11,E,-11,&T1_val,&T1_exp);	
	fn_float_add(T0_val,T0_exp,-1*T1_val,T1_exp,&H1_val,&H1_exp);
	fn_float_div(H0_val,H0_exp,H1_val,H1_exp,&H2_val,&H2_exp);	
	Q_code = ROUND_SHIFT(H2_val , (-1*H2_exp));
	//LOG_MDEBUG(phy,TRX_QEC_CAL,"I_code = %d | H2 val %d H2 exp %d\n",I_code,H2_val,H2_exp);


	// ------------- set with target code ------------------ //
	hal_spi_write_reg(phy, reg_tia_offset_i,I_code);
	hal_spi_write_reg(phy, reg_tia_offset_q,Q_code);
	//fn_rx_get_adc_offset(phy, chn,10,&DC_I[0],&DC_Q[0]);
	//LOG_MDEBUG(phy, TRX_QEC_CAL," analog dc offset cal result:\n");
	//LOG_MDEBUG(phy, TRX_QEC_CAL, "mean_i : %.1f set reg 0x%x\n",((_SFlt)DC_I[0])/10.0,I_code);
	//LOG_MDEBUG(phy, TRX_QEC_CAL, "mean_q : %.1f set reg 0x%x\n",((_SFlt)DC_Q[0])/10.0,Q_code);
	
	return 0;
}

int    fn_rx_ana_offset_cal   (rf_chip_phy_t *phy, int chn,int target)
{
	int sign_table[2][2];
	int tia_offset_i;
	int tia_offset_q;
	int bbf_offset_i;
	int bbf_offset_q;
	unsigned int reg_tia_offset_i;
	unsigned int reg_tia_offset_q;
	unsigned int reg_bbf_offset_i;
	unsigned int reg_bbf_offset_q;
	int i_mean;
	int q_mean;
	int acc_i;
	int acc_q;
	int decay_coef_acc;
	int decay_coef_k0;
	int try_cnt = 0;

	sign_table[0][0] =  1;
	sign_table[0][1] = -1;
	sign_table[1][0] = -1;
	sign_table[1][1] =  1;

	if(chn==0)
	{
		reg_tia_offset_i = 0x6D5;
		reg_bbf_offset_i = 0x6D6;
		reg_tia_offset_q = 0x6D3;
		reg_bbf_offset_q = 0x6D4;
	}
	else
	{
		reg_tia_offset_i = 0x6E5;
		reg_bbf_offset_i = 0x6E6;
		reg_tia_offset_q = 0x6E3;
		reg_bbf_offset_q = 0x6E4;
	}

	tia_offset_i = hal_spi_read_reg(phy, reg_tia_offset_i);
	tia_offset_q = hal_spi_read_reg(phy, reg_tia_offset_q);
	bbf_offset_i = hal_spi_read_reg(phy, reg_bbf_offset_i);
	bbf_offset_q = hal_spi_read_reg(phy, reg_bbf_offset_q);

	tia_offset_i = 128+64;
	tia_offset_q = 128+65;
	hal_spi_write_reg(phy, reg_tia_offset_i,tia_offset_i);
	hal_spi_write_reg(phy, reg_tia_offset_q,tia_offset_q);

	acc_i = 0;
	acc_q = 0;

    decay_coef_acc = 70/2;
    decay_coef_k0  = 54*8/2;

	while(1)
	{
		fn_rx_get_adc_offset(phy, chn,10,&i_mean,&q_mean);
		LOG_MDEBUG(phy, TRX_QEC_CAL, "mean_i : %.1f set reg 0x%x\n",((float)i_mean)/10.0,tia_offset_i);
		LOG_MDEBUG(phy, TRX_QEC_CAL, "mean_q : %.1f set reg 0x%x\n",((float)q_mean)/10.0,tia_offset_q);
		if ((abs(i_mean) < (target*10)) && (abs(q_mean)< (target*10)))
			break;

		acc_i = acc_i + i_mean;
		tia_offset_i = (tia_offset_i*16384 + acc_i*sign_table[0][0]*decay_coef_acc + i_mean*decay_coef_k0*sign_table[0][0])/16384;

		if(tia_offset_i<0)
		{
			try_cnt ++;
			tia_offset_i = 0;
		}
		else if(tia_offset_i>=256)
		{
			try_cnt ++;
			tia_offset_i = 255;
		}
		hal_spi_write_reg(phy, reg_tia_offset_i,(tia_offset_i));

		acc_q = acc_q + q_mean;
		tia_offset_q = (tia_offset_q*16384 - acc_q*sign_table[0][1]*decay_coef_acc - q_mean*decay_coef_k0*sign_table[0][1])/16384;

		if(tia_offset_q<0)
		{
			try_cnt ++;
			tia_offset_q = 0;
		}
		else if(tia_offset_q>=256)
		{
			try_cnt ++;
			tia_offset_q = 255;
		}
		else
		{
			try_cnt ++;
		}
		hal_spi_write_reg(phy, reg_tia_offset_q,tia_offset_q);

		if(try_cnt > 20)
		{
			LOG_MDEBUG(phy, TRX_QEC_CAL, "dc remove cannot remove\n");
			break;
		}
	}

	return 1;
}

//-------------------------------
int fn_tx_dac_autoscale(rf_chip_phy_t *phy, int channel_sel,int *final_dci,int*  final_dcq)
{
    int tx_dac_val;
    int mean_power;
    int adc_power;
    int dc_i = 0;
    int dc_q = 0;
    int tx_dac_val_i_ref ;
    int tx_dac_val_q_ref ;
    unsigned int cal_num = 1;
    //QString tmp;

    // get dc value
    fn_rx_get_adc_offset(phy, channel_sel,10,&dc_i,&dc_q);
    dc_i = dc_i / 10;
    dc_q = dc_q / 10;

    // i auto scale
    //tmp = "";
    //tmp.sprintf("========= i dac scale ===============");
    //qDebug() << "r: " << tmp;
    //LOG_MDEBUG(phy, TRX_QEC_CAL, "r: ========= i dac scale ===============\n");
#ifdef DIG_QEC_D1
    for (tx_dac_val = 4; tx_dac_val <= 13; tx_dac_val++)
#else
    for (tx_dac_val = 10; tx_dac_val <= 13; tx_dac_val++)
#endif
    {
        fn_tx_set_dc_manual(phy, channel_sel,(1 << tx_dac_val),0);
        fn_tx_set_dc_manual_ena(phy, channel_sel,1,0);
        fn_rx_get_adc_power(phy, channel_sel,cal_num, &mean_power);
        mean_power = mean_power / cal_num;
        //tmp = "";
        //tmp.sprintf("dc value : %d | mean power %d ",tx_dac_val,mean_power);
        //qDebug() << "r: " << tmp;
        //LOG_MDEBUG(phy, TRX_QEC_CAL, "r: dc value : %d | mean power %d \n",tx_dac_val,mean_power);
        if (mean_power > 3000 || (((int)sqrt((double)mean_power) + abs(dc_i)) > 500) )
        {
            break;
        }
    }

    tx_dac_val_i_ref = (7000*1000 / mean_power) * (1<<tx_dac_val) / 1000;


    // q auto scale
    //tmp = "";
    //tmp.sprintf("========= q dac scale ===============");
    //qDebug() << "r: " << tmp;
    //LOG_MDEBUG(phy, TRX_QEC_CAL, "r: ========= q dac scale ===============\n");

#ifdef DIG_QEC_D1
    for (tx_dac_val = 4; tx_dac_val <= 13; tx_dac_val++)
#else
    for (tx_dac_val = 10; tx_dac_val <= 13; tx_dac_val++)
#endif
    {
        fn_tx_set_dc_manual(phy, channel_sel,0,(1 << tx_dac_val));
        fn_tx_set_dc_manual_ena(phy, channel_sel,0,1);
        fn_rx_get_adc_power(phy, channel_sel,cal_num, &mean_power);
        mean_power = mean_power / cal_num;
        //tmp = "";
        //tmp.sprintf("dc value : %d | mean power %d ",tx_dac_val,mean_power);
        //qDebug() << "r: " << tmp;
        //LOG_MDEBUG(phy, TRX_QEC_CAL, "r: dc value : %d | mean power %d \n",tx_dac_val,mean_power);
        if (mean_power > 3000 || (((int)sqrt((double)mean_power) + abs(dc_q)) > 500) )
        {
            break;
        }
    }

	tx_dac_val_q_ref = (7000*1000 / mean_power) * (1<<tx_dac_val) / 1000;
	if(tx_dac_val_i_ref > ((1<<13)-1))
	{
		tx_dac_val_i_ref = ((1<<13)-1);
	}
	if(tx_dac_val_q_ref > ((1<<13)-1))
	{
		tx_dac_val_q_ref = ((1<<13)-1);
	}
	
	fn_tx_set_dc_manual(phy, channel_sel,tx_dac_val_i_ref,tx_dac_val_q_ref);
	fn_tx_set_dc_manual_ena(phy, channel_sel,1,1);
	fn_rx_get_adc_power(phy, channel_sel,4, &mean_power);
	mean_power = mean_power / 4;
	//LOG_MDEBUG(phy, TRX_QEC_CAL, "final mean power %d |i %x q %x\n",mean_power,tx_dac_val_i_ref,tx_dac_val_q_ref);
	*final_dci = tx_dac_val_i_ref;
	*final_dcq = tx_dac_val_q_ref;
    return 1;
}

int    fn_rx_get_mixer_port        (rf_chip_phy_t *phy, int channel_sel )
{
    int port_type  ; // 0:AP|1:AN|2:APN
                     // 3:BP|4:BN|5:BPN
                     // 6:CP|7:CN|8:CPN
    int an,ap,bn,bp,cn,cp;
    unsigned int addr;
    unsigned int data;
    // ---------- get current port type -------------------- //
    addr = (channel_sel == TRX_CHN1)? 0x660 : 0x67E;
    data = hal_spi_read_reg(phy, addr);
    an = ((data>>0)&0x1) || ((data>>1)&0x1);
    ap = ((data>>2)&0x1) || ((data>>3)&0x1);
    bn = ((data>>4)&0x1) || ((data>>5)&0x1);
    bp = ((data>>6)&0x1) || ((data>>7)&0x1);

    port_type = 99;
    if(an==1 && ap==1)
    {
        port_type = 2;
    }
    else if(an==1)
    {
        port_type = 1;
    }
    else if(ap==1)
    {
        port_type = 0;
    }

    if(bn==1 && bp==1)
    {
        port_type = 5;
    }
    else if(bn==1)
    {
        port_type = 4;
    }
    else if(bp==1)
    {
        port_type = 3;
    }

    addr = (channel_sel == TRX_CHN1)? 0x661 : 0x67F;
    data = hal_spi_read_reg(phy, addr);
    cn = ((data>>0)&0x1) || ((data>>1)&0x1);
    cp = ((data>>2)&0x1) || ((data>>3)&0x1);

    if(cn==1 && cp==1)
    {
        port_type = 8;
    }
    else if(cn==1)
    {
        port_type = 7;
    }
    else if(cp==1)
    {
        port_type = 6;
    }

    if(port_type == 99)
        return -1;
    return port_type;
}


int fn_rx_ana_set_gain (rf_chip_phy_t *phy, int channel_sel , int mixer_att ,int lpf_att)
{
    unsigned int addr,addr_t[4];
    unsigned int data ;
    int i;
    int port_type  ; // 0:AP|1:AN|2:APN
                     // 3:BP|4:BN|5:BPN
                     // 6:CP|7:CN|8:CPN
    int an,ap,bn,bp,cn,cp;
	unsigned int G0_reg[4][9] = {
        {0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0},
        {0x4,0x1,0x5,0x40,0x10,0x50,0x0,0x0,0x0},
        {0x0,0x0,0x0,0x0,0x0,0x0,0x4,0x1,0x5},
        {0xFF,0xFF,0x0,0xFF,0xFF,0x0,0xFF,0xFF,0x0}};

	
	unsigned int G1_reg[4][9] = {
        {0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30},
        {0x4,0x1,0x5,0x40,0x10,0x50,0x0,0x0,0x0},
        {0x0,0x0,0x0,0x0,0x0,0x0,0x4,0x1,0x5},
        {0xFF,0xFF,0x0,0xFF,0xFF,0x0,0xFF,0xFF,0x0}};
	
    unsigned int G2_reg[4][9] = {
        {0x31,0x31,0x31,0x31,0x31,0x31,0x31,0x31,0x31},
        {0x8,0x2,0x0A,0x80,0x20,0xA0,0x0,0x0,0x0},
        {0x0,0x0,0x0,0x0,0x0,0x0,0x8,0x2,0x0A},
        {0xFF,0xFF,0x0,0xFF,0xFF,0x0,0xFF,0xFF,0x0}};
	
	unsigned int G3_reg[4][9] = {
        {0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33},
        {0x8,0x2,0x0A,0x80,0x20,0xA0,0x0,0x0,0x0},
        {0x0,0x0,0x0,0x0,0x0,0x0,0x8,0x2,0x0A},
        {0xFF,0xFF,0x0,0xFF,0xFF,0x0,0xFF,0xFF,0x0}};
	
	unsigned int G4_reg[4][9] = {
        {0x37,0x37,0x37,0x37,0x37,0x37,0x37,0x37,0x37},
        {0x8,0x2,0x0A,0x80,0x20,0xA0,0x0,0x0,0x0},
        {0x0,0x0,0x0,0x0,0x0,0x0,0x8,0x2,0x0A},
        {0xFF,0xFF,0x0,0xFF,0xFF,0x0,0xFF,0xFF,0x0}};
	
	unsigned int G5_reg[4][9] = {
        {0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F},
        {0x8,0x2,0x0A,0x80,0x20,0xA0,0x0,0x0,0x0},
        {0x0,0x0,0x0,0x0,0x0,0x0,0x8,0x2,0x0A},
        {0xFF,0xFF,0x0,0xFF,0xFF,0x0,0xFF,0xFF,0x0}};

    // get port
    port_type = fn_rx_get_mixer_port(phy, channel_sel);

    // start to set regs
    addr_t[0] = (channel_sel == TRX_CHN1)? 0x65F : 0x67D;
    addr_t[1] = (channel_sel == TRX_CHN1)? 0x660 : 0x67E;
    addr_t[2] = (channel_sel == TRX_CHN1)? 0x661 : 0x67F;
    addr_t[3] = (channel_sel == TRX_CHN1)? 0x66D : 0x68B;

    switch (mixer_att)
    {
        case 0:
        {
            for(i=0;i<4;i++)
            {
                hal_spi_write_reg(phy, addr_t[i],G0_reg[i][port_type]);
            }
            break;
        }
        case 1:
        {
            for(i=0;i<4;i++)
            {
                hal_spi_write_reg(phy, addr_t[i],G1_reg[i][port_type]);
            }
            break;
        }
        case 2:
        {
            for(i=0;i<4;i++)
            {
                hal_spi_write_reg(phy, addr_t[i],G2_reg[i][port_type]);
            }
            break;
        }
        case 3:
        {
            for(i=0;i<4;i++)
            {
                hal_spi_write_reg(phy, addr_t[i],G3_reg[i][port_type]);
            }
            break;
        }
        case 4:
        {
            for(i=0;i<4;i++)
            {
                hal_spi_write_reg(phy, addr_t[i],G4_reg[i][port_type]);
            }
            break;
        }
        case 5:
        {
            for(i=0;i<4;i++)
            {
                hal_spi_write_reg(phy, addr_t[i],G5_reg[i][port_type]);
            }
            break;
        }
    default: return -1;break;
    }

    if(channel_sel==0)
    {
        hal_spi_write_reg(phy, 0x65C,lpf_att);
    }
    else
    {
        hal_spi_write_reg(phy, 0x67A,lpf_att);
    }
}

void fn_connect_rxlo_to_sxrx(rf_chip_phy_t * phy,int channel,unsigned long long rx_lo)
{
	rx_band_selection(phy, channel,rx_lo);
	// IIO_DEMO\IIO_FSM_Script\Power_Up_E_0_2_SXRX_wFCAL_ON_VCO9600_XO30p72_X4.txt
    hal_spi_write_reg(phy, 0x634,0x04);
    hal_spi_write_reg(phy, 0x61E,0x12);
    hal_spi_write_reg(phy, 0x00E,0xCE);
    hal_spi_write_reg(phy, 0x61F,0x1f);
    hal_spi_write_reg(phy, 0x620,0x14);
    hal_spi_write_reg(phy, 0x62C,0x1F);
    hal_spi_write_reg(phy, 0x630,0xBA);
    hal_spi_write_reg(phy, 0x62C,0x20);
    hal_spi_write_reg(phy, 0x62D,0x00);
    hal_spi_write_reg(phy, 0x621,0x1A);
    hal_spi_write_reg(phy, 0x622,0xD0);
    hal_spi_write_reg(phy, 0x628,0xCC);
    hal_spi_write_reg(phy, 0x629,0x0F);
    hal_spi_write_reg(phy, 0x62A,0x29);
    hal_spi_write_reg(phy, 0x62B,0x19);
    hal_spi_write_reg(phy, 0x630,0xBF);
    hal_spi_write_reg(phy, 0x635,0x00);
    hal_spi_write_reg(phy, 0x623,0x08);
    hal_spi_write_reg(phy, 0x624,0x00);
    hal_spi_write_reg(phy, 0x625,0x00);
    hal_spi_write_reg(phy, 0x626,0x00);
    hal_spi_write_reg(phy, 0x627,0x27);
    hal_spi_write_reg(phy, 0x010,0x27);
    hal_spi_write_reg(phy, 0x011,0x10);
    hal_spi_write_reg(phy, 0x00E,0xDE);
    hal_spi_write_reg(phy, 0x61E,0x16);
    hal_spi_write_reg(phy, 0x61E,0x12);
    hal_spi_write_reg(phy, 0x61F,0x9F);
    hal_spi_write_reg(phy, 0x00E,0xCE);
    hal_spi_write_reg(phy, 0x61E,0x02);

    //IIO_DEMO\IIO_FSM_Script\Power_Up_E_0_3_LO_CONFIG_2T2R.txt
    hal_spi_write_reg(phy, 0x639,0xCC);
    hal_spi_write_reg(phy, 0x620,0x3C);
    hal_spi_write_reg(phy, 0x6D1,0x7F);
    hal_spi_write_reg(phy, 0x6E1,0x7F);
    hal_spi_write_reg(phy, 0x702,0x0F);
    hal_spi_write_reg(phy, 0x74A,0x0F);
    hal_spi_write_reg(phy, 0x803,0x0F);
	rx_band_selection(phy, channel,rx_lo);
}



void fn_recovery_txlo_to_sxrx(rf_chip_phy_t * phy)	
{
    // close RX LO, and connext TXLO to RX MIXER
    // IIO_DEMO\IIO_FSM_Script\Power_Up_E_0_4_LO_CONFIG_2T2R_TDD.txt
    hal_spi_write_reg(phy, 0x6D1,0x5F);
    hal_spi_write_reg(phy, 0x6E1,0x5F);
    hal_spi_write_reg(phy, 0x702,0x2F);
    hal_spi_write_reg(phy, 0x74A,0x2F);
    hal_spi_write_reg(phy, 0x639,0xFC);
    hal_spi_write_reg(phy, 0x803,0x07);
    hal_spi_write_reg(phy, 0x620,0x10);
    hal_spi_write_reg(phy, 0x61F,0x00);	
}


void fn_tx_hbf_reset(rf_chip_phy_t * phy,int chn)
{
	unsigned short  reg_val;
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
}

// Name     : rxqec_diag_info_init
// Author   : liw
// Function : re malloc internal memory before
// Input    : 
//   1.RXQEC_DAG_INFO * rxqec_diag
//      if rxqec_diag == NULL  , then function will return directly.
//   2.speed level :
//   	0 all debug memory on . most slow .
// 		1 limit debug memory on. medium slow 
//      2 very large debug memory used to log.

void txqec_diag_info_init(
	TX_QEC_DIAG_INFO * txqec_diag                  ,
	int                wb_num                      ,
	int                debug_level                ) 

{
	if(txqec_diag != NULL)
	{	
		txqec_diag->wb_num         = wb_num   ;
	}
	txqec_diag->debug_level      =  debug_level;
	txqec_diag->tx_qec_mag_err   = (int *) malloc ( txqec_diag->wb_num   * sizeof(int));   // 10 or 30
	txqec_diag->tx_qec_phg_err   = (int *) malloc ( txqec_diag->wb_num   * sizeof(int));   // 10 or 30
	txqec_diag->tx_qec_rsb_init  = (int *) malloc ( txqec_diag->wb_num   * sizeof(int));
	txqec_diag->tx_qec_rsb_final = (int *) malloc ( txqec_diag->wb_num   * sizeof(int));
	txqec_diag->iter_cnt         = (int *) malloc ( txqec_diag->wb_num   * sizeof(int));
	
	memset(txqec_diag->tx_qec_rsb_init ,0,txqec_diag->wb_num   *sizeof(int));
	memset(txqec_diag->tx_qec_mag_err  ,0,txqec_diag->wb_num   *sizeof(int));
	memset(txqec_diag->tx_qec_phg_err  ,0,txqec_diag->wb_num   *sizeof(int));
	memset(txqec_diag->tx_qec_rsb_final,0,txqec_diag->wb_num   *sizeof(int));
	memset(txqec_diag->iter_cnt        ,0,txqec_diag->wb_num   *sizeof(int));
	
	txqec_diag->wb_cnt        =  0;
	txqec_diag->wb_num        =  0;
	txqec_diag->lmt_index     = -1;
	txqec_diag->lpf_index     = -1;
	txqec_diag->rx_dc_i       =  0;
	txqec_diag->rx_dc_q       =  0;
	txqec_diag->rx_power      =  0;
	txqec_diag->loopback_type =  0;
}

void txlol_diag_info_init(
	TX_LOL_DIAG_INFO * txlol_diag                  ,
	int                wb_num                      ,
	int                debug_level               
) 
{
	if(txlol_diag != NULL)
	{	
		txlol_diag->wb_num         = wb_num ;
	}
	txlol_diag->debug_level      =  debug_level;
	txlol_diag->tx_lol_dc_i      = (int *) malloc ( txlol_diag->wb_num   * sizeof(int));   // 10 or 30
	txlol_diag->tx_lol_dc_q      = (int *) malloc ( txlol_diag->wb_num   * sizeof(int));   // 10 or 30
	txlol_diag->tx_lol_rsb_final = (int *) malloc ( txlol_diag->wb_num   * sizeof(int));
	txlol_diag->iter_cnt         = (int *) malloc ( txlol_diag->wb_num   * sizeof(int));
	txlol_diag->tx_lol_rsb_init  = (int *) malloc ( txlol_diag->wb_num   * sizeof(int));
	
	
	memset(txlol_diag->tx_lol_rsb_init ,0,txlol_diag->wb_num   *sizeof(int));
	memset(txlol_diag->tx_lol_dc_i     ,0,txlol_diag->wb_num   *sizeof(int));
	memset(txlol_diag->tx_lol_dc_q     ,0,txlol_diag->wb_num   *sizeof(int));
	memset(txlol_diag->tx_lol_rsb_final,0,txlol_diag->wb_num   *sizeof(int));
	memset(txlol_diag->iter_cnt        ,0,txlol_diag->wb_num   *sizeof(int));
	
	txlol_diag->wb_cnt        = 0;
	txlol_diag->lmt_index     =-1;
	txlol_diag->lpf_index     =-1;
	txlol_diag->rx_dc_i       = 0;
	txlol_diag->rx_dc_q       = 0;
	txlol_diag->rx_power      = 0;
	txlol_diag->loopback_type = 0;
}


void rxqec_diag_info_init(
	RX_QEC_DIAG_INFO * rxqec_diag                  ,
	int                wb_num                      ,
	int                chk_num                     ,
	int                debug_level                 )
{
	if(rxqec_diag != NULL)
	{
		rxqec_diag->debug_level    = debug_level;
		rxqec_diag->wb_num         = wb_num     ;
		rxqec_diag->chk_num        = chk_num    ;
		rxqec_diag->wb_cnt         = 0 ;
		rxqec_diag->chk_cnt        = 0 ;
		rxqec_diag->rx_qec_mag_err = (int *) malloc ( wb_num                     * sizeof(int)); 
		rxqec_diag->rx_qec_phg_err = (int *) malloc ( wb_num                     * sizeof(int));
		rxqec_diag->rx_lpbk_power  = (int *) malloc ( wb_num                     * sizeof(int)); 
		rxqec_diag->rx_qec_mag_chk = (int *) malloc ( chk_num                    * sizeof(int)); 
		rxqec_diag->rx_qec_phg_chk = (int *) malloc ( chk_num                    * sizeof(int)); 
		
		memset(rxqec_diag->rx_qec_mag_err ,0,wb_num  * sizeof(int));
		memset(rxqec_diag->rx_qec_phg_err ,0,wb_num  * sizeof(int));
		memset(rxqec_diag->rx_lpbk_power  ,0,wb_num  * sizeof(int));
		memset(rxqec_diag->rx_qec_mag_chk ,0,chk_num * sizeof(int));
		memset(rxqec_diag->rx_qec_phg_chk ,0,chk_num * sizeof(int));
		
		rxqec_diag->rx_dc_i       =   0;
		rxqec_diag->rx_dc_q       =   0;
		rxqec_diag->rx_power      =   0;
		rxqec_diag->lpf_index     =  -1;
		rxqec_diag->loopback_type = 100;
	}
	return;
}


void rxqec_diag_info_free(
		RX_QEC_DIAG_INFO * rxqec_diag                  )
{
	if(rxqec_diag==NULL)
	{
		return;
	}
	else
	{
		free(rxqec_diag->rx_qec_mag_err );
		free(rxqec_diag->rx_qec_phg_err	);
		free(rxqec_diag->rx_qec_mag_chk	);	
		free(rxqec_diag->rx_qec_phg_chk	);	
		free(rxqec_diag->rx_lpbk_power	);	
	}
}

void txqec_diag_info_free(
		TX_QEC_DIAG_INFO * txqec_diag                  )
{
	if(txqec_diag==NULL)
	{
		return;
	}
	else
	{
		free(txqec_diag->iter_cnt);
		free(txqec_diag->tx_qec_rsb_final);
		free(txqec_diag->tx_qec_mag_err  );
		free(txqec_diag->tx_qec_phg_err  );
		free(txqec_diag->tx_qec_rsb_init );
	}
}

void txlol_diag_info_free(
		TX_LOL_DIAG_INFO * txlol_diag                  )
{
	if(txlol_diag==NULL)
	{
		return;
	}
	else
	{
		free(txlol_diag->iter_cnt);
		free(txlol_diag->tx_lol_dc_i     );
		free(txlol_diag->tx_lol_dc_q     );
		free(txlol_diag->tx_lol_rsb_final);
		free(txlol_diag->tx_lol_rsb_init );
	}
}


void fn_open_loop_back(
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
	)
{	
	int flg_loop_back_type = 5;
	double rssi_0,rssi_1,rssi_2,rssi_3;
	double rssi_00,rssi_11,rssi_22,rssi_33;
	int mean_power;
	
	// change pa_index =0 and bb_index=0 to avoid someone change the pa_index and bb_index.
	set_tx_atten_ext(phy, chn ,0, lo_freq, 0);
	//LOG_MDEBUG(phy,TRX_QEC_CAL,"chn %d ; extloop_sel %d; tdd_sel %d; rxqec_sel %d; backoff_dbfs %d ; LO %llu ; bb_fs %d\n",
	//	chn,extloop_sel,tdd_sel,rxqec_sel,backoff_dbfs,lo_freq,sample_rate);
	// handle TXLO and RXLO.
	if(tdd_sel == 0) // FDD model, no i have RXLO and TX LO to adjust
	{
		if(rxqec_sel == 1) // FDD RXQEC , use TX DC ,change TX lo to fullfill it.
		{
			tx_band_selection(phy, chn, lo_freq + (unsigned long long)(sample_rate / 32*1));
			rx_band_selection(phy, chn, lo_freq);
		}
		else // FDD TXQEC, use rx bandslection to 2*unit_freq
		{
			tx_band_selection(phy, chn, lo_freq);
			rx_band_selection(phy, chn, lo_freq - (unsigned long long)(sample_rate / 32*2));	
		}
	}
	else // TDD model, now i have only TX LO to adjust.
	{
		tx_band_selection(phy, chn, lo_freq);
	}
	
	// handle Test Vector Amplitude and Type.
	if(tdd_sel == 0 && rxqec_sel==1) // only when FDD mode and RXQEC, we use TX DC as test signal
	{
		int tx_dc_i = (int)(pow(10.0, -1.0*((double)abs(backoff_dbfs)-3.0)/20.0)* pow(2.0,12.0));
		int tx_dc_q = (int)(pow(10.0, -1.0*((double)abs(backoff_dbfs)-3.0)/20.0)* pow(2.0,12.0));
		fn_tx_set_dc_manual(phy, chn,tx_dc_i,tx_dc_q);
		fn_tx_set_dc_manual_ena(phy,chn,1,1);
	}
	else
	{
		fn_tx_send_tone(phy, chn,1,2*1);
		TX_QEC_CFG_REGS tx_qec_cfg_tmp;
		memset(&tx_qec_cfg_tmp,0, sizeof(TX_QEC_CFG_REGS));
		tx_qec_cfg_tmp.dc_i = 0;
        tx_qec_cfg_tmp.dc_q = 0;
        tx_qec_cfg_tmp.fiiq_imag = 0x4000;
        tx_qec_cfg_tmp.fiiq_real = 0x0;
		fn_tx_qec_gain_set(phy,  chn, &tx_qec_cfg_tmp , 2*abs(backoff_dbfs)); // backoff 2dB
	}
	
	if(extloop_sel==0) // internal loop backoff
	{
		fn_rf_loop_back_ena(phy,chn,1,tdd_sel,rf_loopback_reg);
		
		fn_rx_ana_set_gain(phy,  chn, 0,12);
		fn_rx_ana_offset_cal_V2(phy,chn);
		fn_rx_get_adc_power(phy, chn, 4, &mean_power);
		mean_power = mean_power / 4;
		rssi_0 = 20*log10(sqrt((double)mean_power)/pow(2.0,9.0));
		
		//while(1)
		//{
		//}
		fn_rx_ana_set_gain(phy,  chn, 1,12);
		fn_rx_ana_offset_cal_V2(phy,chn);
		fn_rx_get_adc_power(phy, chn, 4, &mean_power);
		mean_power = mean_power / 4;
		rssi_1 = 20*log10(sqrt((double)mean_power)/pow(2.0,9.0));	
		
		fn_rf_loop_back_lg_ena(phy, chn , 1, rf_loopback_reg);
		fn_rx_ana_offset_cal_V2(phy,chn);
		fn_rx_get_adc_power(phy, chn, 4, &mean_power);
		mean_power = mean_power / 4;
		rssi_2 = 20*log10(sqrt((double)mean_power)/pow(2.0,9.0));

		if(chn==0)
		{
			hal_spi_write_reg(phy,0x65f,0x31);
		}
		else
		{
			hal_spi_write_reg(phy,0x67D,0x31);
		}
		
		fn_rx_ana_offset_cal_V2(phy,chn);
		fn_rx_get_adc_power(phy, chn, 4, &mean_power);
		mean_power = mean_power / 4;
		rssi_3 = 20*log10(sqrt((double)mean_power)/pow(2.0,9.0));
		
#ifdef QEC_COMMON_PRINT
		LOG_MDEBUG(phy, TRX_QEC_CAL, "rf loop back rssi %.4f %.4f | low gain loop back rssi %.4f %.4f\n",
			rssi_0,rssi_1,rssi_2,rssi_3);
#endif

		rssi_00 = fabs(rssi_0 - (-20.0));
		rssi_11 = fabs(rssi_1 - (-20.0));
		rssi_22 = fabs(rssi_2 - (-20.0));
		rssi_33 = fabs(rssi_3 - (-20.0));
		
		
		if(rssi_00 < rssi_11 && rssi_00 < rssi_22 && rssi_00 < rssi_33)
		{
			flg_loop_back_type = 0;
			hal_spi_write_reg(phy, 0xD33,0x00);
			fn_rx_ana_set_gain(phy, chn,0,12);
		
#ifdef QEC_COMMON_PRINT
			LOG_MDEBUG(phy,TRX_QEC_CAL,"using rf normal loop back\n");
#endif
		}
		else if(rssi_11 < rssi_00 && rssi_11 < rssi_22 && rssi_11 < rssi_33)
		{
			flg_loop_back_type = 1;
			hal_spi_write_reg(phy, 0xD33,0x00);
			fn_rx_ana_set_gain(phy, chn, 1,12);
#ifdef QEC_COMMON_PRINT			
			LOG_MDEBUG(phy,TRX_QEC_CAL,"using rf normal loop back\n");		
#endif
		}
		else if(rssi_22 < rssi_00 && rssi_22 < rssi_11 && rssi_22 < rssi_33)
		{
			flg_loop_back_type = 2;
			if(chn==0)
			{
				hal_spi_write_reg(phy,0x65f,0x00);
			}
			else
			{
				hal_spi_write_reg(phy,0x67D,0x00);
			}
#ifdef QEC_COMMON_PRINT					
			LOG_MDEBUG(phy,TRX_QEC_CAL,"using rf low gain loop back\n");		
#endif
		}
		else
		{
			flg_loop_back_type = 3;
			if(chn==0)
			{
				hal_spi_write_reg(phy,0x65f,0x31);
			}
			else
			{
				hal_spi_write_reg(phy,0x67D,0x31);
			}
#ifdef QEC_COMMON_PRINT					
			LOG_MDEBUG(phy,TRX_QEC_CAL,"using rf low gain loop back\n");		
#endif
		}
	
	}
	else
	{
		fn_rx_ana_set_gain(phy,  chn, 5,12);  // Set to Mininal Gain
		flg_loop_back_type = 4;
	}
	*loopback_type = flg_loop_back_type;
}

void fn_close_loop_back(
	rf_chip_phy_t    * phy           ,
	int                chn           ,
	int                loopback_type ,
	RF_LOOPBACK_REGS * rf_loopback_reg)
{
	if(loopback_type!=4)
	{
		fn_rf_loop_back_ena(phy, chn,0,0,rf_loopback_reg);
		fn_rf_loop_back_lg_ena(phy, chn , 0, rf_loopback_reg);
	}
}


void fn_lpf_index_auto_change(
	rf_chip_phy_t    * phy          ,
	int                chn          ,
	int              * lpf_index
)
{
	double rssi      ;
	int    bbf_gain  ;
	int    mean_power;
	if(chn==0)
	{
		hal_spi_write_reg(phy, 0x65C,12);
	}
	else
	{
		hal_spi_write_reg(phy, 0x67A,12);
	}
	
	fn_rx_get_adc_power(phy, chn,4, &mean_power);
	mean_power = mean_power / 4;
	rssi = 20*log10(sqrt((double)mean_power)/pow(2.0,9.0));

	bbf_gain = (int) (	(-12.0)-rssi);
	if(bbf_gain>12)
	{
		bbf_gain = 12;
	}
	else if(bbf_gain <0)
	{
		bbf_gain = 0;
	}
	if(chn==0)
	{
		hal_spi_write_reg(phy, 0x65C,12-bbf_gain);
	}
	else
	{
		hal_spi_write_reg(phy, 0x67A,12-bbf_gain);
	}

	*lpf_index = bbf_gain;
}


void fn_tx_qec_diag_print(
	rf_chip_phy_t    * phy           ,
	TX_QEC_DIAG_INFO * tx_qec_diag_info
)
{
	int i;
	int rsb_cnt;
	double rssi;
	int iter_num = 0;
	LOG_MDEBUG(phy,TRX_QEC_CAL,"========= txqec raw info =========\n");

	if (tx_qec_diag_info->loopback_type ==0 )
	{
		LOG_MDEBUG(phy,TRX_QEC_CAL,"intloop %d;5,%d;",tx_qec_diag_info->loopback_type ,tx_qec_diag_info->lpf_index);
	}
	else if (tx_qec_diag_info->loopback_type ==1 )
	{
		LOG_MDEBUG(phy,TRX_QEC_CAL,"intloop %d;4,%d;",tx_qec_diag_info->loopback_type ,tx_qec_diag_info->lpf_index);
	}
	else if (tx_qec_diag_info->loopback_type ==2 )
	{
		LOG_MDEBUG(phy,TRX_QEC_CAL,"intloop %d;1,%d;",tx_qec_diag_info->loopback_type ,tx_qec_diag_info->lpf_index);
	}	
	else if (tx_qec_diag_info->loopback_type ==3 )
	{
		LOG_MDEBUG(phy,TRX_QEC_CAL,"intloop %d;0,%d;",tx_qec_diag_info->loopback_type ,tx_qec_diag_info->lpf_index);
	}
	else if (tx_qec_diag_info->loopback_type ==4 )
	{
		LOG_MDEBUG(phy,TRX_QEC_CAL,"extloop %d;0,%d;",tx_qec_diag_info->loopback_type ,tx_qec_diag_info->lpf_index);
	}	
	
	rssi = 20.0*log10(sqrt((double)tx_qec_diag_info->rx_power/4.0)/pow(2.0,9.0));	
	LOG_MDEBUG(phy,TRX_QEC_CAL,"%.1f,%.1f,%.1f\n",
		((double)tx_qec_diag_info->rx_dc_i)/10.0,
		((double)tx_qec_diag_info->rx_dc_q)/10.0,
		rssi
	);
	LOG_MDEBUG(phy,TRX_QEC_CAL,"NUM:");
	for(i=0;i<tx_qec_diag_info->wb_cnt;i++)
	{
		LOG_MDEBUG(phy,TRX_QEC_CAL,"%d;",
			(tx_qec_diag_info->iter_cnt[i]));
		iter_num = iter_num + (tx_qec_diag_info->iter_cnt[i]);
	}
	LOG_MDEBUG(phy,TRX_QEC_CAL,"%d;\n",
			iter_num);	
			
	LOG_MDEBUG(phy,TRX_QEC_CAL,"MAG:");
	for(i=0;i<tx_qec_diag_info->wb_cnt;i++)
	{
		LOG_MDEBUG(phy,TRX_QEC_CAL,"+");
		LOG_MDEBUG(phy,TRX_QEC_CAL,"%.4f",
			((double)tx_qec_diag_info->tx_qec_mag_err[i])/10000.0);

		if(i!=(tx_qec_diag_info->wb_cnt-1))
		{
			LOG_MDEBUG(phy,TRX_QEC_CAL,";");
		}
		else
		{
			LOG_MDEBUG(phy,TRX_QEC_CAL,"\n");
		}
	}
	LOG_MDEBUG(phy,TRX_QEC_CAL,"PHG:");
	for(i=0;i<tx_qec_diag_info->wb_cnt;i++)
	{
		if(tx_qec_diag_info->tx_qec_phg_err[i]>0)
		{
			LOG_MDEBUG(phy,TRX_QEC_CAL,"+");
		}
		else
		{
			LOG_MDEBUG(phy,TRX_QEC_CAL,"-");
		}
		LOG_MDEBUG(phy,TRX_QEC_CAL,"%.4f",
			((double)fabs(tx_qec_diag_info->tx_qec_phg_err[i])/10000.0));

		if(i!=(tx_qec_diag_info->wb_cnt-1))
		{
			LOG_MDEBUG(phy,TRX_QEC_CAL,";");
		}
		else
		{
			LOG_MDEBUG(phy,TRX_QEC_CAL,"\n");
		}
	}
	
	LOG_MDEBUG(phy,TRX_QEC_CAL,"IMGI:");
	for(i=0;i<tx_qec_diag_info->wb_cnt;i++)
	{
		if(tx_qec_diag_info->tx_qec_rsb_init[i]>0)
		{
			LOG_MDEBUG(phy,TRX_QEC_CAL,"+");
		}
		else
		{
			LOG_MDEBUG(phy,TRX_QEC_CAL,"-");
		}
		LOG_MDEBUG(phy,TRX_QEC_CAL,"%.1f",
			((double)fabs(tx_qec_diag_info->tx_qec_rsb_init[i])/10.0));

		if(i!=(tx_qec_diag_info->wb_cnt-1))
		{
			LOG_MDEBUG(phy,TRX_QEC_CAL,";");
		}
		else
		{
			LOG_MDEBUG(phy,TRX_QEC_CAL,"\n");
		}
	}		
	

	LOG_MDEBUG(phy,TRX_QEC_CAL,"IMGO:");
	for(i=0;i<tx_qec_diag_info->wb_cnt;i++)
	{
		if(tx_qec_diag_info->tx_qec_rsb_final[i]>0)
		{
			LOG_MDEBUG(phy,TRX_QEC_CAL,"+");
		}
		else
		{
			LOG_MDEBUG(phy,TRX_QEC_CAL,"-");
		}
		LOG_MDEBUG(phy,TRX_QEC_CAL,"%.1f",
			((double)fabs(tx_qec_diag_info->tx_qec_rsb_final[i])/10.0));

		if(i!=(tx_qec_diag_info->wb_cnt-1))
		{
			LOG_MDEBUG(phy,TRX_QEC_CAL,";");
		}
		else
		{
			LOG_MDEBUG(phy,TRX_QEC_CAL,"\n");
		}
	}	
}


int fn_rx_qec_diag_print(
	rf_chip_phy_t    * phy           ,
	RX_QEC_DIAG_INFO * rx_qec_diag_info
)
{
	int i;
	int rsb_cnt;
	double rssi;
	double irr_result;
	int flag_qec_ok = 0;
	LOG_MDEBUG(phy,TRX_QEC_CAL,"========= rxqec raw info =========\n");
	if (rx_qec_diag_info->loopback_type ==0 )
	{
		LOG_MDEBUG(phy,TRX_QEC_CAL,"intloop %d;5,%d;",rx_qec_diag_info->loopback_type ,rx_qec_diag_info->lpf_index);
	}
	else if (rx_qec_diag_info->loopback_type ==1 )
	{
		LOG_MDEBUG(phy,TRX_QEC_CAL,"intloop %d;4,%d;",rx_qec_diag_info->loopback_type ,rx_qec_diag_info->lpf_index);
	}
	else if (rx_qec_diag_info->loopback_type ==2 )
	{
		LOG_MDEBUG(phy,TRX_QEC_CAL,"intloop %d;1,%d;",rx_qec_diag_info->loopback_type ,rx_qec_diag_info->lpf_index);
	}	
	else if (rx_qec_diag_info->loopback_type ==3 )
	{
		LOG_MDEBUG(phy,TRX_QEC_CAL,"intloop %d;0,%d;",rx_qec_diag_info->loopback_type ,rx_qec_diag_info->lpf_index);
	}
	else if (rx_qec_diag_info->loopback_type ==4 )
	{
		LOG_MDEBUG(phy,TRX_QEC_CAL,"extloop %d;0,%d;",rx_qec_diag_info->loopback_type ,rx_qec_diag_info->lpf_index);
	}
	
	rssi = 20.0*log10(sqrt((double)rx_qec_diag_info->rx_power/4.0)/pow(2.0,9.0));	
	LOG_MDEBUG(phy,TRX_QEC_CAL,"%.1f,%.1f,%.1f\n",
		((double)rx_qec_diag_info->rx_dc_i)/10.0,
		((double)rx_qec_diag_info->rx_dc_q)/10.0,
		rssi
	);
	
	int * curr_pt;
	LOG_MDEBUG(phy,TRX_QEC_CAL,"PWR:");	
	for(i=0;i<rx_qec_diag_info->wb_cnt;i++)
	{
		LOG_MDEBUG(phy,TRX_QEC_CAL,"%.1f",
			((double)rx_qec_diag_info->rx_lpbk_power[i])/10.0);

		if(i!=(rx_qec_diag_info->wb_cnt-1))
		{
			LOG_MDEBUG(phy,TRX_QEC_CAL,";");
		}
		else
		{
			LOG_MDEBUG(phy,TRX_QEC_CAL,"\n");
		}
	}
	
	int flag_all_error = 0;
	for(i=0;i<rx_qec_diag_info->wb_cnt;i++)
	{
		if(rx_qec_diag_info->rx_lpbk_power[i] <-300)
		{
			LOG_MDEBUG(phy,TRX_QEC_CAL,"RX QEC ERROR : target index power is too low, Maybe RX or TX is not lock or sample rate is not right please check .\n");	
			if(rx_qec_diag_info->debug_level>=2)
			{
				while(1){}
			}
			else
			{
				flag_qec_ok = -1;
			}
		}
		else if(rx_qec_diag_info->rx_lpbk_power[i] <-200)
		{
			LOG_MDEBUG(phy,TRX_QEC_CAL,"RX QEC WARNING: target index power is too low, Maybe RX or TX is not lock or sample rate is not right please check .\n");	
		}
	}
	
	int target_index ;
	if(phy->config->custom_bandwith_flag)
	{
        target_index = (int)round(32 * phy->config->custom_bandwith / 2.0 / phy->config->bb_sample_rate/1.0);
	}
	else if (phy->config->syspll_cfg_flag)
	{
		target_index = (int)round(32.0* (double)g_3db_band[phy->config->bandwith] / phy->config->bb_sample_rate/1.0);
	}
	else	
	{
		target_index = (int)round(32.0* (double)g_3db_band[phy->config->bandwith] / (double)g_band_bbrate[phy->config->bandwith] / 1000.0/1.0);
		//LOG_MDEBUG(phy,TRX_QEC_CAL," %.4ld , %.4u\n",g_3db_band[phy->config->bandwith],g_band_bbrate[phy->config->bandwith]);
	}
		
//	if ( (rx_qec_diag_info->rx_lpbk_power[1] - rx_qec_diag_info->rx_lpbk_power[target_index]) > 50 || (rx_qec_diag_info->rx_lpbk_power[1] - rx_qec_diag_info->rx_lpbk_power[target_index]) < 20)
//	{
//		LOG_MDEBUG(phy,TRX_QEC_CAL,"RX QEC ERROR : target index power is not fit | index 0 : %d index %d : %d, please check RX BW CAL\n",
//			rx_qec_diag_info->rx_lpbk_power[0],
//			target_index,
//			rx_qec_diag_info->rx_lpbk_power[target_index]);		
//			
//			LOG_MDEBUG(phy,TRX_QEC_CAL,"RX QEC ERROR : target index power is not fit | index 0 : %d index %d : %d, please check RX BW CAL\n",
//			rx_qec_diag_info->rx_lpbk_power[0],
//			rx_qec_diag_info->rx_lpbk_power[target_index],target_index
//			);				
//			
//		if(rx_qec_diag_info->debug_level>=2)
//		{
//			while(1){}
//		}
//		else
//		{
//			flag_qec_ok = -1;
//		}
//	}
//	else 
    if( (rx_qec_diag_info->rx_lpbk_power[1] - rx_qec_diag_info->rx_lpbk_power[target_index]) > 35 || (rx_qec_diag_info->rx_lpbk_power[1] - rx_qec_diag_info->rx_lpbk_power[target_index]) < 25)
	{
		LOG_MDEBUG(phy,TRX_QEC_CAL,"RX QEC WARNING : target index power is not fit | index 0 : %d index %d : %d, please check RX BW CAL\n",
			rx_qec_diag_info->rx_lpbk_power[0],
			target_index,
			rx_qec_diag_info->rx_lpbk_power[target_index]);		
	}
	
	LOG_MDEBUG(phy,TRX_QEC_CAL,"MAG:");
	for(i=0;i<rx_qec_diag_info->wb_cnt;i++)
	{
		LOG_MDEBUG(phy,TRX_QEC_CAL,"+");
		LOG_MDEBUG(phy,TRX_QEC_CAL,"%.4f",
			((double)rx_qec_diag_info->rx_qec_mag_err[i])/10000.0);

		if(i!=(rx_qec_diag_info->wb_cnt-1))
		{
			LOG_MDEBUG(phy,TRX_QEC_CAL,";");
		}
		else
		{
			LOG_MDEBUG(phy,TRX_QEC_CAL,"\n");
		}
	}
	LOG_MDEBUG(phy,TRX_QEC_CAL,"PHG:");
	for(i=0;i<rx_qec_diag_info->wb_cnt;i++)
	{
		if(rx_qec_diag_info->rx_qec_phg_err[i]>0)
		{
			LOG_MDEBUG(phy,TRX_QEC_CAL,"+");
		}
		else
		{
			LOG_MDEBUG(phy,TRX_QEC_CAL,"-");
		}
		LOG_MDEBUG(phy,TRX_QEC_CAL,"%.4f",
			((double)fabs(rx_qec_diag_info->rx_qec_phg_err[i]))/10000.0);

		if(i!=(rx_qec_diag_info->wb_cnt-1))
		{
			LOG_MDEBUG(phy,TRX_QEC_CAL,";");
		}
		else
		{
			LOG_MDEBUG(phy,TRX_QEC_CAL,"\n");
		}
	}
		
	LOG_MDEBUG(phy,TRX_QEC_CAL,"MAG:");
	for(i=0;i<rx_qec_diag_info->wb_cnt;i++)
	{
		LOG_MDEBUG(phy,TRX_QEC_CAL,"+");
		LOG_MDEBUG(phy,TRX_QEC_CAL,"%.4f",
			((double)rx_qec_diag_info->rx_qec_mag_chk[i])/10000.0);

		if(i!=(rx_qec_diag_info->wb_cnt-1))
		{
			LOG_MDEBUG(phy,TRX_QEC_CAL,";");
		}
		else
		{
			LOG_MDEBUG(phy,TRX_QEC_CAL,"\n");
		}
	}

	LOG_MDEBUG(phy,TRX_QEC_CAL,"PHG:");
	for(i=0;i<rx_qec_diag_info->wb_cnt;i++)
	{
		if(rx_qec_diag_info->rx_qec_phg_chk[i]>0)
		{
			LOG_MDEBUG(phy,TRX_QEC_CAL,"+");
		}
		else
		{
			LOG_MDEBUG(phy,TRX_QEC_CAL,"-");
		}
		
		LOG_MDEBUG(phy,TRX_QEC_CAL,"%.4f",
			((double)fabs(rx_qec_diag_info->rx_qec_phg_chk[i]))/10000.0);

		if(i!=(rx_qec_diag_info->wb_cnt-1))
		{
			LOG_MDEBUG(phy,TRX_QEC_CAL,";");
		}
		else
		{
			LOG_MDEBUG(phy,TRX_QEC_CAL,"\n");
		}
	}
	LOG_MDEBUG(phy,TRX_QEC_CAL,"IRR:");
	for(i=0;i<rx_qec_diag_info->wb_cnt;i++)
	{
		irr_result = fn_cal_irr(
		(double)(rx_qec_diag_info->rx_qec_mag_chk[i])/10000.0,
		(double)(rx_qec_diag_info->rx_qec_phg_chk[i])/10000.0);
		
		LOG_MDEBUG(phy,TRX_QEC_CAL,"%.1f",irr_result);
		if(i!=(rx_qec_diag_info->wb_cnt-1))
		{
			LOG_MDEBUG(phy,TRX_QEC_CAL,";");
		}
		else
		{
			LOG_MDEBUG(phy,TRX_QEC_CAL,"\n");
		}
	}	
	return flag_qec_ok;
}

void fn_tx_lol_diag_print(
	rf_chip_phy_t    * phy           ,
	TX_LOL_DIAG_INFO * tx_lol_diag_info
)
{
	int i;
	int rsb_cnt;
	int iter_num = 0;
	double rssi;
	LOG_MDEBUG(phy,TRX_QEC_CAL,"========= txlol info =========\n");

	if (tx_lol_diag_info->loopback_type ==0 )
	{
		LOG_MDEBUG(phy,TRX_QEC_CAL,"intloop %d;5,%d;",tx_lol_diag_info->loopback_type ,tx_lol_diag_info->lpf_index);
	}
	else if (tx_lol_diag_info->loopback_type ==1 )
	{
		LOG_MDEBUG(phy,TRX_QEC_CAL,"intloop %d;4,%d;",tx_lol_diag_info->loopback_type ,tx_lol_diag_info->lpf_index);
	}
	else if (tx_lol_diag_info->loopback_type ==2 )
	{
		LOG_MDEBUG(phy,TRX_QEC_CAL,"intloop %d;1,%d;",tx_lol_diag_info->loopback_type ,tx_lol_diag_info->lpf_index);
	}	
	else if (tx_lol_diag_info->loopback_type ==3 )
	{
		LOG_MDEBUG(phy,TRX_QEC_CAL,"intloop %d;0,%d;",tx_lol_diag_info->loopback_type ,tx_lol_diag_info->lpf_index);
	}
	else if (tx_lol_diag_info->loopback_type ==4 )
	{
		LOG_MDEBUG(phy,TRX_QEC_CAL,"extloop %d;0,%d;",tx_lol_diag_info->loopback_type ,tx_lol_diag_info->lpf_index);
	}	
	
	rssi = 20.0*log10(sqrt((double)tx_lol_diag_info->rx_power/4.0)/pow(2.0,9.0));	
	LOG_MDEBUG(phy,TRX_QEC_CAL,"%.1f,%.1f,%.1f\n",
		((double)tx_lol_diag_info->rx_dc_i)/10.0,
		((double)tx_lol_diag_info->rx_dc_q)/10.0,
		rssi
	);
	
	LOG_MDEBUG(phy,TRX_QEC_CAL,"NUM:");
	for(i=0;i<tx_lol_diag_info->wb_cnt;i++)
	{
		LOG_MDEBUG(phy,TRX_QEC_CAL,"%d;",
			(tx_lol_diag_info->iter_cnt[i]));
		iter_num = iter_num + tx_lol_diag_info->iter_cnt[i];
	}
	LOG_MDEBUG(phy,TRX_QEC_CAL,"%d\n",
			iter_num);	
			
	LOG_MDEBUG(phy,TRX_QEC_CAL,"DCI:");
	for(i=0;i<tx_lol_diag_info->wb_cnt;i++)
	{
		if(tx_lol_diag_info->tx_lol_dc_i[i]>0)
		{
			LOG_MDEBUG(phy,TRX_QEC_CAL,"+");
		}
		else
		{
			LOG_MDEBUG(phy,TRX_QEC_CAL,"-");
		}

		LOG_MDEBUG(phy,TRX_QEC_CAL,"%d",
			abs(tx_lol_diag_info->tx_lol_dc_i[i]));

		if(i!=(tx_lol_diag_info->wb_cnt-1))
		{
			LOG_MDEBUG(phy,TRX_QEC_CAL,";");
		}
		else
		{
			LOG_MDEBUG(phy,TRX_QEC_CAL,"\n");
		}
	}
	LOG_MDEBUG(phy,TRX_QEC_CAL,"DCQ:");
	for(i=0;i<tx_lol_diag_info->wb_cnt;i++)
	{
		if(tx_lol_diag_info->tx_lol_dc_q[i]>0)
		{
			LOG_MDEBUG(phy,TRX_QEC_CAL,"+");
		}
		else
		{
			LOG_MDEBUG(phy,TRX_QEC_CAL,"-");
		}
		
		LOG_MDEBUG(phy,TRX_QEC_CAL,"%d",
			abs(tx_lol_diag_info->tx_lol_dc_q[i]));

		if(i!=(tx_lol_diag_info->wb_cnt-1))
		{
			LOG_MDEBUG(phy,TRX_QEC_CAL,";");
		}
		else
		{
			LOG_MDEBUG(phy,TRX_QEC_CAL,"\n");
		}
	}
	
	LOG_MDEBUG(phy,TRX_QEC_CAL,"LOLI:");
	for(i=0;i<tx_lol_diag_info->wb_cnt;i++)
	{
		if(tx_lol_diag_info->tx_lol_rsb_init[i]>0)
		{
			LOG_MDEBUG(phy,TRX_QEC_CAL,"+");
		}
		else
		{
			LOG_MDEBUG(phy,TRX_QEC_CAL,"-");
		}
		LOG_MDEBUG(phy,TRX_QEC_CAL,"%.1f",
			((double)fabs(tx_lol_diag_info->tx_lol_rsb_init[i])/10.0));

		if(i!=(tx_lol_diag_info->wb_cnt-1))
		{
			LOG_MDEBUG(phy,TRX_QEC_CAL,";");
		}
		else
		{
			LOG_MDEBUG(phy,TRX_QEC_CAL,"\n");
		}
	}		
	
	LOG_MDEBUG(phy,TRX_QEC_CAL,"LOLO:");
	for(i=0;i<tx_lol_diag_info->wb_cnt;i++)
	{
		if(tx_lol_diag_info->tx_lol_rsb_final[i]>0)
		{
			LOG_MDEBUG(phy,TRX_QEC_CAL,"+");
		}
		else
		{
			LOG_MDEBUG(phy,TRX_QEC_CAL,"-");
		}
		LOG_MDEBUG(phy,TRX_QEC_CAL,"%.1f",
			((double)fabs(tx_lol_diag_info->tx_lol_rsb_final[i])/10.0));

		if(i!=(tx_lol_diag_info->wb_cnt-1))
		{
			LOG_MDEBUG(phy,TRX_QEC_CAL,";");
		}
		else
		{
			LOG_MDEBUG(phy,TRX_QEC_CAL,"\n");
		}
	}		
}



double fn_cal_irr(double mag,double phg)
{
	double A, B , C;
	double K1_real,K1_imag,K2_real,K2_imag;
	double K1_mag,K2_mag,irr;
	//mag = 0.9899;
	//phg = 0.6298;
	A = mag;
	B =  1.0*cos(phg/360.0*2*PI);
	C = -1.0*sin(phg/360.0*2*PI);
	K1_real = 1+A*B;
	K1_imag = A*C;
	K2_real = 1-A*B;
	K2_imag = -1*A*C;
	K1_mag = sqrt(   pow(K1_real,2.0) +  pow(K1_imag,2.0));
	K2_mag = sqrt(   pow(K2_real,2.0) +  pow(K2_imag,2.0));
	irr = 20.0*log10((K1_mag/K2_mag));
	
	if(fabs(mag -1.0) < 0.0001 && fabs(phg) < 0.0001)
	{
		irr = 90.0;
	}
	return irr;
}


int rxqec_diag_info_diag (
	rf_chip_phy_t    * phy           ,
	int                debug_level   ,
	RX_QEC_DIAG_INFO * rx_qec_diag_info)
{
	int wb_cnt;
	double mag_diff;
	double phg_diff;
	double irr_result;
	double mag_error_dB;
	double rssi;
	int    flag_qec_ok = 0;
	if(abs(rx_qec_diag_info->rx_dc_i)/10 > 400 || abs(rx_qec_diag_info->rx_dc_q)/10 > 400 )
	{
		LOG_MDEBUG(phy,TRX_QEC_CAL,"RX QEC ERROR: DC cannot remove\n");
		return -1;
	}
	else if(abs(rx_qec_diag_info->rx_dc_i)/10 > 100 || abs(rx_qec_diag_info->rx_dc_q)/10 > 100 )
	{
		LOG_MDEBUG(phy,TRX_QEC_CAL,"RX QEC WARNING: DC cannot remove\n");
	}
	
	rssi = 20.0*log10(sqrt((double)rx_qec_diag_info->rx_power/4.0)/pow(2.0,9.0));	
	if(rssi > -3 )
	{
		LOG_MDEBUG(phy,TRX_QEC_CAL,"RX QEC ERROR    : LOOPBACK power is too large\n");
		return -1;
	}
	else if(rssi> -6 )
	{
		LOG_MDEBUG(phy,TRX_QEC_CAL,"RX QEC WARNING: LOOPBACK power is too large\n");		
	}
	else if(rssi < -50 )
	{
		LOG_MDEBUG(phy,TRX_QEC_CAL,"RX QEC ERROR    : LOOPBACK power is too small\n");
		return -1;
	}	
	else if (rssi < -30 )
	{
		LOG_MDEBUG(phy,TRX_QEC_CAL,"RX QEC WARNING : LOOPBACK power is too small\n");
	}
	
	for(wb_cnt=0;wb_cnt<rx_qec_diag_info->wb_num;wb_cnt++)
	{
		mag_diff = 20*log10(( fabs(1.0-((double)rx_qec_diag_info->rx_qec_mag_err[wb_cnt])/10000.0)));
		if( mag_diff > -15.0)
		{
			LOG_MDEBUG(phy,TRX_QEC_CAL,"RX QEC ERROR    : DETECT INIT wb %d magnitude error is too large : %.4f db . BW CAL Maybe not right\n",
				wb_cnt,
				mag_diff);
			if(debug_level>=2)
			{
				while(1){}
			}
			else
			{
				flag_qec_ok = -1;
			}
		}
		else if ( mag_diff> -30.0)
		{
			LOG_MDEBUG(phy,TRX_QEC_CAL,"RX QEC WARNING  : DETECT INIT wb %d magnitude error is too large : %.4f db . \n",
				wb_cnt,
				mag_diff);			
		}
		
		phg_diff = fabs(((double)(rx_qec_diag_info->rx_qec_phg_err[wb_cnt])/10000.0));

		if( phg_diff > 10.0)
		{
			LOG_MDEBUG(phy,TRX_QEC_CAL,"RX QEC ERROR    : DETECT INIT wb %d phase error is too large : %.4f degree . BW CAL Maybe not right \n",
				wb_cnt,
				phg_diff);
			if(debug_level>=2)
			{
				while(1){}
			}
			else
			{
				flag_qec_ok = -1;
			}
		}		
		else if ( phg_diff > 5.0)
		{
			LOG_MDEBUG(phy,TRX_QEC_CAL,"RX QEC WARNING  : DETECT INIT wb %d phase error is too large : %.4f degree . \n",
				wb_cnt,
				phg_diff);			
		}
		
		if(wb_cnt>1)
		{
			mag_diff = fabs( (((double)rx_qec_diag_info->rx_qec_mag_err[wb_cnt])/10000.0) - (((double)rx_qec_diag_info->rx_qec_mag_err[wb_cnt-1])/10000.0));
			if ( mag_diff>0.05)
			{	
				LOG_MDEBUG(phy,TRX_QEC_CAL,"RX QEC ERROR    : DETECT large magnitude change %.4f between %d and %d ,BW CAL Maybe not right\n",
					mag_diff,
					wb_cnt,
					wb_cnt-1);
				if(debug_level>=2)
				{
					while(1){}
				}
				else
				{
					flag_qec_ok = -1;
				}						
			}
			else if( mag_diff>0.01 )
			{
				LOG_MDEBUG(phy,TRX_QEC_CAL,"RX QEC WARNING  : DETECT large magnitude change %.4f between %d and %d \n",
					mag_diff,
					wb_cnt,
					wb_cnt-1);				
			}
			//else
			//{
			//	LOG_MDEBUG(phy,TRX_QEC_CAL,"RX QEC INFO  : magnitude change %.4f between %d and %d \n",
			//			mag_diff,
			//			wb_cnt,
			//			wb_cnt-1);				
			//}
			
			phg_diff = fabs( (((double)rx_qec_diag_info->rx_qec_phg_err[wb_cnt])/10000.0) - (((double)rx_qec_diag_info->rx_qec_phg_err[wb_cnt-1])/10000.0));
			if ( phg_diff >1.0)
			{
				LOG_MDEBUG(phy,TRX_QEC_CAL,"RX QEC ERROR    : DETECT large phase change %.4f between %d and %d ,BW CAL Maybe not right\n",
					phg_diff,
					wb_cnt,
					wb_cnt-1);
				if(debug_level>=2)
				{
					while(1){}
				}
				else
				{
					flag_qec_ok = -1;
				}					

			}
			else if ( phg_diff >0.5)
			{
				LOG_MDEBUG(phy,TRX_QEC_CAL,"RX QEC WARNING  : DETECT large phase change %.4f between %d and %d \n",
					phg_diff,
					wb_cnt,
					wb_cnt-1);
			}
			//else
			//{
			//	LOG_MDEBUG(phy,TRX_QEC_CAL,"RX QEC INFO  : phase change %.4f between %d and %d \n",
			//			phg_diff,
			//			wb_cnt,
			//			wb_cnt-1);
			//}				
		}		
		
	}
	
	for(wb_cnt=0;wb_cnt<rx_qec_diag_info->chk_num;wb_cnt++)
	{
		irr_result = fn_cal_irr(
			(double)(rx_qec_diag_info->rx_qec_mag_chk[wb_cnt])/10000.0,
			(double)(rx_qec_diag_info->rx_qec_phg_chk[wb_cnt])/10000.0);
		
		if(irr_result > 40.0 && irr_result <58.0)
		{
			LOG_MDEBUG(phy,TRX_QEC_CAL,"RX QEC WARNING  : DETECT a unsatisfactory result on %d : %.4f \n",
				wb_cnt,irr_result);
		}
		else if(irr_result <= 40.0)
		{
			LOG_MDEBUG(phy,TRX_QEC_CAL,"RX QEC ERROR    : DETECT a very bad calibration result on %d : %.4f \n",
				wb_cnt,
				irr_result
			);
			if(debug_level>=2)
			{
				while(1){}
			}
			else
			{
				flag_qec_ok = -1;
			}	
		}		
		
	}
	
	//LOG_MDEBUG(phy,TRX_QEC_CAL,"RX QEC DIAG END \n");
	
	return flag_qec_ok ;
}
	

int txqec_diag_info_diag (
	rf_chip_phy_t    * phy           ,
	TX_QEC_DIAG_INFO * tx_qec_diag_info)
{
	double irr_result;
	double rssi;
	double mag_diff;
	double phg_diff;
	int    wb_cnt;
	if(abs(tx_qec_diag_info->rx_dc_i)/10 > 400 || abs(tx_qec_diag_info->rx_dc_q)/10 > 400 )
	{
		LOG_MDEBUG(phy,TRX_QEC_CAL,"TX QEC ERROR: DC cannot remove\n");
		return -1;
	}
	else if(abs(tx_qec_diag_info->rx_dc_i)/10 > 100 || abs(tx_qec_diag_info->rx_dc_q)/10 > 100 )
	{
		LOG_MDEBUG(phy,TRX_QEC_CAL,"TX QEC WARNING: DC cannot remove\n");
	}
	
	rssi = 20.0*log10(sqrt((double)tx_qec_diag_info->rx_power/4.0)/pow(2.0,9.0));	
	if(rssi > -3 )
	{
		LOG_MDEBUG(phy,TRX_QEC_CAL,"TX QEC ERROR    : LOOPBACK power is too large\n");
		return -1;
	}
	else if(rssi> -6 )
	{
		LOG_MDEBUG(phy,TRX_QEC_CAL,"TX QEC WARNING: LOOPBACK power is too large\n");		
	}
	else if(rssi < -50 )
	{
		LOG_MDEBUG(phy,TRX_QEC_CAL,"TX QEC ERROR    : LOOPBACK power is too small\n");
		return -1;
	}	
	else if (rssi < -30 )
	{
		LOG_MDEBUG(phy,TRX_QEC_CAL,"TX QEC WARNING : LOOPBACK power is too small\n");
	}
	
	for(wb_cnt=0;wb_cnt<tx_qec_diag_info->wb_num;wb_cnt++)
	{
		if( fabs(1.0-((double)tx_qec_diag_info->tx_qec_mag_err[wb_cnt])/10000.0) > 0.12)
		{
			LOG_MDEBUG(phy,TRX_QEC_CAL,"TX QEC ERROR    : DETECT INIT wb %d magnitude error is too large : %.4f db . \n",
				wb_cnt,
				20*log10(( fabs(1.0-((double)tx_qec_diag_info->tx_qec_mag_err[wb_cnt])/10000.0))));
			return -1;		
		}
		
		if( fabs(((double)(tx_qec_diag_info->tx_qec_phg_err[wb_cnt])/10000.0)) > 10.0)
		{
			LOG_MDEBUG(phy,TRX_QEC_CAL,"TX QEC ERROR    : DETECT INIT wb %d phase error is too large : %.4f degree . \n",
				wb_cnt,
				(double)(tx_qec_diag_info->tx_qec_phg_err[wb_cnt])/10000.0);
			return -1;		
		}
		
		if(wb_cnt>1)
		{
			mag_diff = fabs( (((double)tx_qec_diag_info->tx_qec_mag_err[wb_cnt])/10000.0) - (((double)tx_qec_diag_info->tx_qec_mag_err[wb_cnt-1])/10000.0));
			if ( mag_diff>0.01 )
			{
				LOG_MDEBUG(phy,TRX_QEC_CAL,"TX QEC ERROR    : DETECT large magnitude change %.4f between %d and %d \n",
					mag_diff,
					wb_cnt,
					wb_cnt-1);
				return -1;					
			}
			//else
			//{
			//	LOG_MDEBUG(phy,TRX_QEC_CAL,"RX QEC INFO  : magnitude change %.4f between %d and %d \n",
			//			mag_diff,
			//			wb_cnt,
			//			wb_cnt-1);				
			//}
		}
		
		if(wb_cnt>1)
		{
			phg_diff = fabs( (((double)tx_qec_diag_info->tx_qec_phg_err[wb_cnt])/10000.0) - (((double)tx_qec_diag_info->tx_qec_phg_err[wb_cnt-1])/10000.0));
			if ( phg_diff >0.5)
			{
				LOG_MDEBUG(phy,TRX_QEC_CAL,"TX QEC ERROR    : DETECT large phase change %.4f between %d and %d \n",
					phg_diff,
					wb_cnt,
					wb_cnt-1);
				return -1;
			}
			//else
			//{
			//	LOG_MDEBUG(phy,TRX_QEC_CAL,"RX QEC INFO  : phase change %.4f between %d and %d \n",
			//			phg_diff,
			//			wb_cnt,
			//			wb_cnt-1);
			//}
		}

		
		irr_result = (rssi - ((double)tx_qec_diag_info->tx_qec_rsb_final[wb_cnt])/10.0);
		if(irr_result > 50.0 && irr_result <58.0)
		{
			LOG_MDEBUG(phy,TRX_QEC_CAL,"TX QEC WARNING  : DETECT a bad calibration result on %d : %.4f \n",
				wb_cnt,irr_result);
		}
		else if(irr_result <= 50.0)
		{
			LOG_MDEBUG(phy,TRX_QEC_CAL,"TX QEC WARNING  : DETECT a very bad calibration result on %d : %.4f \n",
				wb_cnt,irr_result);
			return -1;
		}
	}
	
	return 0 ;	
}



int txlol_diag_info_diag (
	rf_chip_phy_t    * phy           ,
	TX_LOL_DIAG_INFO * tx_lol_diag_info)
{
	double irr_result;
	double rssi;
	int    dc_diff;
	int    wb_cnt;
	if(abs(tx_lol_diag_info->rx_dc_i)/10 > 400 || abs(tx_lol_diag_info->rx_dc_q)/10 > 400 )
	{
		LOG_MDEBUG(phy,TRX_QEC_CAL,"TX LOL ERROR: DC cannot remove\n");
		return -1;
	}
	else if(abs(tx_lol_diag_info->rx_dc_i)/10 > 100 || abs(tx_lol_diag_info->rx_dc_q)/10 > 100 )
	{
		LOG_MDEBUG(phy,TRX_QEC_CAL,"TX LOL WARNING: DC cannot remove\n");
	}
	
	rssi = 20.0*log10(sqrt((double)tx_lol_diag_info->rx_power/4.0)/pow(2.0,9.0));	
	if(rssi > -3 )
	{
		LOG_MDEBUG(phy,TRX_QEC_CAL,"TX LOL ERROR    : LOOPBACK power is too large\n");
		return -1;
	}
	else if(rssi> -6 )
	{
		LOG_MDEBUG(phy,TRX_QEC_CAL,"TX LOL WARNING: LOOPBACK power is too large\n");		
	}
	else if(rssi < -50 )
	{
		LOG_MDEBUG(phy,TRX_QEC_CAL,"TX LOL ERROR    : LOOPBACK power is too small\n");
		return -1;
	}	
	else if (rssi < -30 )
	{
		LOG_MDEBUG(phy,TRX_QEC_CAL,"TX LOL WARNING : LOOPBACK power is too small\n");
	}
	
	for(wb_cnt=0;wb_cnt<tx_lol_diag_info->wb_num;wb_cnt++)
	{
		if( abs(tx_lol_diag_info->tx_lol_dc_i[wb_cnt] > 500) || abs(tx_lol_diag_info->tx_lol_dc_q [wb_cnt]> 500))
		{
			LOG_MDEBUG(phy,TRX_QEC_CAL,"TX LOL ERROR    : DETECT INIT wb %d DCI and DCQ error is too large : %4d %4d \n",
				wb_cnt,
				tx_lol_diag_info->tx_lol_dc_i[wb_cnt],
				tx_lol_diag_info->tx_lol_dc_q[wb_cnt]);
			return -1;		
		}
		else if( abs(tx_lol_diag_info->tx_lol_dc_i[wb_cnt] > 200) || abs(tx_lol_diag_info->tx_lol_dc_q[wb_cnt] > 200))
		{
			LOG_MDEBUG(phy,TRX_QEC_CAL,"TX LOL WARNING  : DETECT INIT wb %d DCI and DCQ error is too large : %4d %4d \n",
				wb_cnt,
				tx_lol_diag_info->tx_lol_dc_i,
				tx_lol_diag_info->tx_lol_dc_q);	
		}
		
		if(wb_cnt>1)
		{
			dc_diff = abs( tx_lol_diag_info->tx_lol_dc_i[wb_cnt]  - tx_lol_diag_info->tx_lol_dc_i[wb_cnt-1]);
			if ( dc_diff >100 )
			{
				LOG_MDEBUG(phy,TRX_QEC_CAL,"TX LOL ERROR    : DETECT large magnitude change %d on I between %d and %d \n",
					dc_diff,
					wb_cnt,
					wb_cnt-1);
				return -1;					
			}
			dc_diff = abs( tx_lol_diag_info->tx_lol_dc_q[wb_cnt]  - tx_lol_diag_info->tx_lol_dc_q[wb_cnt-1]);
			if ( dc_diff >0.01 )
			{
				LOG_MDEBUG(phy,TRX_QEC_CAL,"TX LOL ERROR    : DETECT large magnitude change %d on I between %d and %d \n",
					dc_diff,
					wb_cnt,
					wb_cnt-1);
				return -1;					
			}
		}
		
		irr_result = (rssi - ((double)tx_lol_diag_info->tx_lol_rsb_final[wb_cnt])/10.0);
		if(irr_result > 50.0 && irr_result <58.0)
		{
			LOG_MDEBUG(phy,TRX_QEC_CAL,"TX LOL WARNING  : DETECT a bad calibration result on %d : %.4f \n",
				wb_cnt,irr_result);
		}
		else if(irr_result <= 50.0)
		{
			LOG_MDEBUG(phy,TRX_QEC_CAL,"TX LOL WARNING  : DETECT a very bad calibration result on %d : %.4f \n",
				wb_cnt,irr_result);
			return -1;
		}
	}
	
	return 0 ;		
}
