#ifndef __SPI_H__
#define __SPI_H__
#include "stdint.h"
#include "config.h"
#include "dw_spi_ll.h"

#define DW_MAX_SPI_TXFIFO_LV       0x20U
#define DW_MAX_SPI_RXFIFO_LV       0x20U
#define DW_DEFAULT_SPI_TXFIFO_LV   0x8U
#define DW_DEFAULT_SPI_RXFIFO_LV   0x10U

typedef enum {
    SPI_MASTER,             ///< SPI Master (Output on MOSI, Input on MISO); arg = Bus Speed in bps
    SPI_SLAVE,              ///< SPI Slave  (Output on MISO, Input on MOSI)
} spi_mode_t;

typedef enum {
    SPI_FORMAT_CPOL0_CPHA0 = 0,  ///< Clock Polarity 0, Clock Phase 0
    SPI_FORMAT_CPOL0_CPHA1,      ///< Clock Polarity 0, Clock Phase 1
    SPI_FORMAT_CPOL1_CPHA0,      ///< Clock Polarity 1, Clock Phase 0
    SPI_FORMAT_CPOL1_CPHA1,      ///< Clock Polarity 1, Clock Phase 1
} spi_cp_format_t;

typedef enum {
    SPI_FRAME_LEN_4  = 4,
    SPI_FRAME_LEN_5,
    SPI_FRAME_LEN_6,
    SPI_FRAME_LEN_7,
    SPI_FRAME_LEN_8,
    SPI_FRAME_LEN_9,
    SPI_FRAME_LEN_10,
    SPI_FRAME_LEN_11,
    SPI_FRAME_LEN_12,
    SPI_FRAME_LEN_13,
    SPI_FRAME_LEN_14,
    SPI_FRAME_LEN_15,
    SPI_FRAME_LEN_16
} spi_frame_len_t;

int32_t     spi_init        (uint32_t spi_base);
void        spi_uninit      (uint32_t spi_base);
int32_t     spi_mode        (uint32_t spi_base, spi_mode_t mode);
int32_t     spi_cp_format   (uint32_t spi_base, spi_cp_format_t format);
uint32_t    spi_baud        (uint32_t spi, uint32_t baud);
int32_t     spi_send        (uint32_t spi_base, void *SendBufPtr, uint32_t size);
int32_t     spi_receive     (uint32_t spi_base, void *RecvBufPtr, uint32_t size);
int32_t     spi_send_receive     (uint32_t spi_base, void *SendBufPtr, void *RecvBufPtr, uint32_t size);
#endif