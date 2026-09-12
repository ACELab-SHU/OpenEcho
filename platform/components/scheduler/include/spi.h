#ifndef __SPI_H__
#define __SPI_H__
#include "../driver/bsp/include/dw_spi_ll.h"
#include "config.h"
#include "stdint.h"
#include "venusmmap.h"
#include <stddef.h>

#define DW_MAX_SPI_TXFIFO_LV     0x20U
#define DW_MAX_SPI_RXFIFO_LV     0x20U
#define DW_DEFAULT_SPI_TXFIFO_LV 0x8U
#define DW_DEFAULT_SPI_RXFIFO_LV 0x10U

typedef enum {
  SPI_MASTER,  ///< SPI Master (Output on MOSI, Input on MISO); arg = Bus Speed in bps
  SPI_SLAVE,   ///< SPI Slave  (Output on MI SO, Input on MOSI)
} spi_mode_t;

typedef enum {
  SPI_FORMAT_CPOL0_CPHA0 = 0,  ///< Clock Polarity 0, Clock Phase 0
  SPI_FORMAT_CPOL0_CPHA1,      ///< Clock Polarity 0, Clock Phase 1
  SPI_FORMAT_CPOL1_CPHA0,      ///< Clock Polarity 1, Clock Phase 0
  SPI_FORMAT_CPOL1_CPHA1,      ///< Clock Polarity 1, Clock Phase 1
} spi_cp_format_t;

typedef enum {
  SPI_FRAME_LEN_0 = 0,
  SPI_FRAME_LEN_1,
  SPI_FRAME_LEN_2,
  SPI_FRAME_LEN_3,
  SPI_FRAME_LEN_4,
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
  SPI_FRAME_LEN_16,
  SPI_FRAME_LEN_17,
  SPI_FRAME_LEN_18,
  SPI_FRAME_LEN_19,
  SPI_FRAME_LEN_20,
  SPI_FRAME_LEN_21,
  SPI_FRAME_LEN_22,
  SPI_FRAME_LEN_23,
  SPI_FRAME_LEN_24,
  SPI_FRAME_LEN_25,
  SPI_FRAME_LEN_26,
  SPI_FRAME_LEN_27,
  SPI_FRAME_LEN_28,
  SPI_FRAME_LEN_29,
  SPI_FRAME_LEN_30,
  SPI_FRAME_LEN_31,
  SPI_FRAME_LEN_32
} spi_frame_len_t;

typedef enum {
  no_wait_cycles = 0b0000,
  _4_wait_cycles,
  _8_wait_cycles,
  _12_wait_cycles,
  _16_wait_cycles,
  _20_wait_cycles,
  _24_wait_cycles,
  _28_wait_cycles,
  _32_wait_cycles,
  _36_wait_cycles,
  _40_wait_cycles,
  _44_wait_cycles,
  _48_wait_cycles,
  _52_wait_cycles,
  _56_wait_cycles,
  _60_wait_cycles
} wait_cycles_t;

typedef enum {
  no_instruction = 0b00,
  _4_bits_inst_l,
  _8_bits_inst_l,
  _16_bits_inst_l
} inst_l_t;

typedef enum {
  no_address = 0b0000,
  _4_bits_addr_l,
  _8_bits_addr_l,
  _12_bits_addr_l,
  _16_bits_addr_l,
  _20_bits_addr_l,
  _24_bits_addr_l,
  _28_bits_addr_l,
  _32_bits_addr_l,
  _36_bits_addr_l,
  _40_bits_addr_l,
  _44_bits_addr_l,
  _48_bits_addr_l,
  _52_bits_addr_l,
  _56_bits_addr_l,
  _60_bits_addr_l
} addr_l_t;

int32_t boot_spi_init(uint32_t spi_base);
void boot_spi_uninit(uint32_t spi_base);
int32_t boot_spi_mode(uint32_t spi_base, spi_mode_t mode);
int32_t boot_spi_cp_format(uint32_t spi_base, spi_cp_format_t format);
uint32_t boot_spi_baud(uint32_t spi, uint32_t baud);
int32_t boot_spi_send(uint32_t spi_base, void* SendBufPtr, uint32_t size, uint32_t frame_length);
int32_t boot_spi_receive(uint32_t spi_base, void* RecvBufPtr, uint32_t size, uint32_t frame_length);
int32_t boot_spi_send_receive(uint32_t spi_base, void* SendBufPtr, void* RecvBufPtr, uint32_t size, uint32_t frame_length);

int32_t boot_spi_frame_len(uint32_t spi_base, spi_frame_len_t length);
void boot_spi_select_slave(uint32_t spi_base, uint32_t slave_num);

void boot_spi_set_std_mode(uint32_t spi_base, uint8_t trans_type, addr_l_t addr_l, inst_l_t inst_len,
                           wait_cycles_t wait_cycles, spi_frame_len_t fram_len, uint32_t fram_num);
void boot_spi_set_dual_mode(uint32_t spi_base, uint8_t trans_type, addr_l_t addr_l, inst_l_t inst_len,
                            wait_cycles_t wait_cycles, spi_frame_len_t fram_len, uint32_t fram_num);
void boot_spi_set_quad_mode(uint32_t spi_base, uint8_t trans_type, addr_l_t addr_l, inst_l_t inst_len,
                            wait_cycles_t wait_cycles, spi_frame_len_t fram_len, uint32_t fram_num);

#endif