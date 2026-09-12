#include "dw_spi_ll.h"

void dw_spi_config_sclk_clock(uint32_t spi_base, uint32_t clock_in, uint32_t clock_out) {
  uint32_t div;

  div = clock_in / clock_out;
  div = (div > 65534U) ? 65534U : div;
  REG_WRITE(spi_base + BAUDR, REG_READ(spi_base + BAUDR) & DW_SPI_BAUDR_SCKDV_Msk);
  REG_WRITE(spi_base + BAUDR, div);
}

uint32_t dw_spi_get_sclk_clock_div(uint32_t spi_base) {
  return REG_READ(spi_base + BAUDR);
}

uint32_t dw_spi_get_data_frame_len(uint32_t spi_base) {
  uint32_t len = REG_READ(spi_base + CTRLR0) & DW_SPI_CTRLR0_DFS_Msk;
  len >>= DW_SPI_CTRLR0_DFS_Pos;
  len++;
  return len;
}

void dw_spi_config_data_frame_len(uint32_t spi_base, uint32_t size) {
  uint32_t temp;

  if ((size >= 4U) & (size <= 16U)) {
    temp = REG_READ(spi_base + CTRLR0);
    temp &= ~DW_SPI_CTRLR0_DFS_Msk;
    temp |= ((size - 1U) << DW_SPI_CTRLR0_DFS_Pos);
    REG_WRITE(spi_base + CTRLR0, temp);
  }
}

void dw_spi_reset_regs(uint32_t spi_base) {
  REG_WRITE(spi_base + CTRLR0, 7U);
  REG_WRITE(spi_base + CTRLR1, 0U);
  REG_WRITE(spi_base + SSIENR, 0U);
  REG_WRITE(spi_base + SER, 0U);
  REG_WRITE(spi_base + BAUDR, 0U);
  REG_WRITE(spi_base + TXFTLR, 0x10U);
  REG_WRITE(spi_base + RXFTLR, 0x10U);
  REG_WRITE(spi_base + IMR, 0x00U);
  REG_WRITE(spi_base + SPIMSSEL, 0x1U);
}
