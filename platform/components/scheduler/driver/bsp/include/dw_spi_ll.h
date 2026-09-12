#ifndef _DW_SPI_LL_H_
#define _DW_SPI_LL_H_
#include "../../../include/config.h"
#include "../../../include/types.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * SPI register bit definitions
 */

/* CTRLR0, offset: 0x00 */
#define DW_SPI_CTRLR0_SPI_FRF_Pos (21U)
#define DW_SPI_CTRLR0_SPI_FRF_Msk (0b11U) << DW_SPI_CTRLR0_SPI_FRF_Pos

#define DW_SPI_CTRLR0_DFS_32_Pos (16U)
#define DW_SPI_CTRLR0_DFS_32_Msk (0b11100U) << DW_SPI_CTRLR0_DFS_32_Pos

#define DW_SPI_CTRLR0_CFS_Pos (12U)
#define DW_SPI_CTRLR0_CFS_Msk (0xFU << DW_SPI_CTRLR0_CFS_Pos)

#define DW_SPI_CTRLR0_SRL_Pos (11U)
#define DW_SPI_CTRLR0_SRL_Msk (0x1U << DW_SPI_CTRLR0_SRL_Pos)
#define DW_SPI_CTRLR0_SRL_EN  DW_SPI_CTRLR0_SRL_Msk

#define DW_SPI_CTRLR0_TMOD_Pos    (8U)
#define DW_SPI_CTRLR0_TMOD_Msk    (0x3U << DW_SPI_CTRLR0_TMOD_Pos)
#define DW_SPI_CTRLR0_TMOD_TX_RX  (0x0U << DW_SPI_CTRLR0_TMOD_Pos)
#define DW_SPI_CTRLR0_TMOD_TX     (0x1U << DW_SPI_CTRLR0_TMOD_Pos)
#define DW_SPI_CTRLR0_TMOD_RX     (0x2U << DW_SPI_CTRLR0_TMOD_Pos)
#define DW_SPI_CTRLR0_TMOD_EEPROM (0x3U << DW_SPI_CTRLR0_TMOD_Pos)

#define DW_SPI_CTRLR0_SCPOL_Pos (7U)
#define DW_SPI_CTRLR0_SCPOL_Msk (0x1U << DW_SPI_CTRLR0_SCPOL_Pos)
#define DW_SPI_CTRLR0_SCPOL_EN  DW_SPI_CTRLR0_SCPOL_Msk

#define DW_SPI_CTRLR0_SCPH_Pos (6U)
#define DW_SPI_CTRLR0_SCPH_Msk (0x1U << DW_SPI_CTRLR0_SCPH_Pos)
#define DW_SPI_CTRLR0_SCPH_EN  DW_SPI_CTRLR0_SCPH_Msk

#define DW_SPI_CTRLR0_FRF_Pos          (4U)
#define DW_SPI_CTRLR0_FRF_Msk          (0x3U << DW_SPI_CTRLR0_FRF_Pos)
#define DW_SPI_CTRLR0_FRF_MOTOROLA_SPI (0x0U << DW_SPI_CTRLR0_FRF_Pos)
#define DW_SPI_CTRLR0_FRF_TI_SSP       (0x1U << DW_SPI_CTRLR0_FRF_Pos)
#define DW_SPI_CTRLR0_FRF_MW_SPI       (0x2U << DW_SPI_CTRLR0_FRF_Pos)

#define DW_SPI_CTRLR0_DFS_Pos (0U)
#define DW_SPI_CTRLR0_DFS_Msk (0xFU << DW_SPI_CTRLR0_DFS_Pos)

/* CTRLR1, offset: 0x04 */
#define DW_SPI_CTRLR1_NDF_Pos (0U)
#define DW_SPI_CTRLR1_NDF_Msk (0xFF << DW_SPI_CTRLR1_NDF_Pos)

/* SSIENR, offset: 0x08 */
#define DW_SPI_SSIENR_SSI_HW1_EN_Pos (2U)
#define DW_SPI_SSIENR_SSI_HW1_EN_Msk (0x1U << DW_SPI_SSIENR_SSI_HW1_EN_Pos)
#define DW_SPI_SSIENR_SSI_HW1_EN     DW_SPI_SSIENR_SSI_HW1_EN_Msk

#define DW_SPI_SSIENR_SSI_HW0_EN_Pos (1U)
#define DW_SPI_SSIENR_SSI_HW0_EN_Msk (0x1U << DW_SPI_SSIENR_SSI_HW0_EN_Pos)
#define DW_SPI_SSIENR_SSI_HW0_EN     DW_SPI_SSIENR_SSI_HW0_EN_Msk

#define DW_SPI_SSIENR_SSI_EN_Pos (0U)
#define DW_SPI_SSIENR_SSI_EN_Msk (0x1U << DW_SPI_SSIENR_SSI_EN_Pos)
#define DW_SPI_SSIENR_SSI_EN     DW_SPI_SSIENR_SSI_EN_Msk

/* MWCR, offset: 0x10 */
#define DW_SPI_MWCR_MHS_Pos (2U)
#define DW_SPI_MWCR_MHS_Msk (0x1U << DW_SPI_MWCR_MHS_Pos)
#define DW_SPI_MWCR_MHS_EN  DW_SPI_MWCR_MHS_Msk

#define DW_SPI_MWCR_MDD_Pos    (1U)
#define DW_SPI_MWCR_MDD_Msk    (0x1U << DW_SPI_MWCR_MHS_Pos)
#define DW_SPI_MWCR_MDD_INPUT  (0x0U << DW_SPI_MWCR_MHS_Pos)
#define DW_SPI_MWCR_MDD_OUTPUT (0x1U << DW_SPI_MWCR_MHS_Pos)

#define DW_SPI_MWCR_MWMOD_Pos         (0U)
#define DW_SPI_MWCR_MWMOD_Msk         (0x1U << DW_SPI_MWCR_MWMOD_Pos)
#define DW_SPI_MWCR_MWMOD_SQUENTIAL   (0x1U << DW_SPI_MWCR_MWMOD_Pos)
#define DW_SPI_MWCR_MWMOD_NOSQUENTIAL (0x0U << DW_SPI_MWCR_MWMOD_Pos)

/* BAUDR, offset: 0x14 */
#define DW_SPI_BAUDR_SCKDV_Pos (0U)
#define DW_SPI_BAUDR_SCKDV_Msk (0xFFU << DW_SPI_BAUDR_SCKDV_Pos)

/* TXFTLR, offset: 0x18 */
#define DW_SPI_TXFTLR_TFT_Pos (0U)
#define DW_SPI_TXFTLR_TFT_Msk (0xFFU << DW_SPI_TXFTLR_TFT_Pos)

/* RXFTLR, offset: 0x1C */
#define DW_SPI_RXFTLR_RFT_Pos (0U)
#define DW_SPI_RXFTLR_RFT_Msk (0xFFU << DW_SPI_RXFTLR_RFT_Pos)

/* SR, offset: 0x28 */
#define DW_SPI_SR_Pos  (0U)
#define DW_SPI_SR_Msk  (0x7FU << DW_SPI_SR_Pos)
#define DW_SPI_SR_DCOL (0x40U << DW_SPI_SR_Pos)
#define DW_SPI_SR_TXE  (0x20U << DW_SPI_SR_Pos)
#define DW_SPI_SR_RFF  (0x10U << DW_SPI_SR_Pos)
#define DW_SPI_SR_RFNE (0x08U << DW_SPI_SR_Pos)
#define DW_SPI_SR_TFE  (0x04U << DW_SPI_SR_Pos)
#define DW_SPI_SR_TFNF (0x02U << DW_SPI_SR_Pos)
#define DW_SPI_SR_BUSY (0x01U << DW_SPI_SR_Pos)

/* IMR, offset: 0x2C */
#define DW_SPI_IMR_MSTIM_Pos (5U)
#define DW_SPI_IMR_MSTIM_Msk (0x1U << DW_SPI_IMR_MSTIM_Pos)
#define DW_SPI_IMR_MSTIM_EN  DW_SPI_IMR_MSTIM_Msk

#define DW_SPI_IMR_RXFIM_Pos (4U)
#define DW_SPI_IMR_RXFIM_Msk (0x1U << DW_SPI_IMR_RXFIM_Pos)
#define DW_SPI_IMR_RXFIM_EN  DW_SPI_IMR_RXFIM_Msk

#define DW_SPI_IMR_RXOIM_Pos (3U)
#define DW_SPI_IMR_RXOIM_Msk (0x1U << DW_SPI_IMR_RXOIM_Pos)
#define DW_SPI_IMR_RXOIM_EN  DW_SPI_IMR_RXOIM_Msk

#define DW_SPI_IMR_RXUIM_Pos (2U)
#define DW_SPI_IMR_RXUIM_Msk (0x1U << DW_SPI_IMR_RXUIM_Pos)
#define DW_SPI_IMR_RXUIM_EN  DW_SPI_IMR_RXUIM_Msk

#define DW_SPI_IMR_TXOIM_Pos (1U)
#define DW_SPI_IMR_TXOIM_Msk (0x1U << DW_SPI_IMR_TXOIM_Pos)
#define DW_SPI_IMR_TXOIM_EN  DW_SPI_IMR_TXOIM_Msk

#define DW_SPI_IMR_TXEIM_Pos (0U)
#define DW_SPI_IMR_TXEIM_Msk (0x1U << DW_SPI_IMR_TXEIM_Pos)
#define DW_SPI_IMR_TXEIM_EN  DW_SPI_IMR_TXEIM_Msk

/* ISR, offset: 0x30 */
#define DW_SPI_ISR_Pos   (0U)
#define DW_SPI_ISR_Msk   (0x3FU << DW_SPI_ISR_Pos)
#define DW_SPI_ISR_MSTIS (0x20U << DW_SPI_ISR_Pos)
#define DW_SPI_ISR_RXFIS (0x10U << DW_SPI_ISR_Pos)
#define DW_SPI_ISR_RXOIS (0x08U << DW_SPI_ISR_Pos)
#define DW_SPI_ISR_RXUIS (0x04U << DW_SPI_ISR_Pos)
#define DW_SPI_ISR_TXOIS (0x02U << DW_SPI_ISR_Pos)
#define DW_SPI_ISR_TXEIS (0x01U << DW_SPI_ISR_Pos)

/* RISR, offset: 0x34 */
#define DW_SPI_RISR_Pos   (0U)
#define DW_SPI_RISR_Msk   (0x3FU << DW_SPI_RISR_Pos)
#define DW_SPI_RISR_MSTIR (0x20U << DW_SPI_RISR_Pos)
#define DW_SPI_RISR_RXFIR (0x10U << DW_SPI_RISR_Pos)
#define DW_SPI_RISR_RXOIR (0x08U << DW_SPI_RISR_Pos)
#define DW_SPI_RISR_RXUIR (0x04U << DW_SPI_RISR_Pos)
#define DW_SPI_RISR_TXOIR (0x02U << DW_SPI_RISR_Pos)
#define DW_SPI_RISR_TXEIR (0x01U << DW_SPI_RISR_Pos)

/* DMACR, offset: 0x4C */
#define DW_SPI_DMACR_TDMAE_Pos (1U)
#define DW_SPI_DMACR_TDMAE_Msk (0x1U << DW_SPI_DMACR_TDMAE_Pos)
#define DW_SPI_DMACR_TDMAE_EN  DW_SPI_DMACR_TDMAE_Msk

#define DW_SPI_DMACR_RDMAE_Pos (0U)
#define DW_SPI_DMACR_RDMAE_Msk (0x1U << DW_SPI_DMACR_RDMAE_Pos)
#define DW_SPI_DMACR_RDMAE_EN  DW_SPI_DMACR_RDMAE_Msk

/* DMATDLR, offset: 0x50 */
#define DW_SPI_DMATDLR_DMATDL_Pos (0U)
#define DW_SPI_DMATDLR_DMATDL_Msk (0x1FU << DW_SPI_DMATDLR_DMATDL_Pos)

/* DMARDLR, offset: 0x54 */
#define DW_SPI_DMARDLR_DMARDL_Pos (0U)
#define DW_SPI_DMARDLR_DMARDL_Msk (0x1FU << DW_SPI_DMARDLR_DMARDL_Pos)

/* SPIMSSEL, offset: 0xA0 */
#define DW_SPI_SPIMSSEL_Pos    (0U)
#define DW_SPI_SPIMSSEL_Msk    (0x1U << DW_SPI_SPIMSSEL_Pos)
#define DW_SPI_SPIMSSEL_MASTER (0x1U << DW_SPI_SPIMSSEL_Pos)
#define DW_SPI_SPIMSSEL_SLAVE  (0x0U << DW_SPI_SPIMSSEL_Pos)

/* SPI_CTRLR0, offset: 0xF4 */
#define DW_SPI_SPI_CTRLR0_WAIT_CYCLES_Pos (11U)
#define DW_SPI_SPI_CTRLR0_WAIT_CYCLES_Msk (0xFU << DW_SPI_SPI_CTRLR0_WAIT_CYCLES_Pos)
#define DW_SPI_SPI_CTRLR0_INST_L_Pos      (8U)
#define DW_SPI_SPI_CTRLR0_INST_L_Msk      (0b11U << DW_SPI_SPI_CTRLR0_INST_L_Pos)
#define DW_SPI_SPI_CTRLR0_ADDR_L_Pos      (2U)
#define DW_SPI_SPI_CTRLR0_ADDR_L_Msk      (0xFU << DW_SPI_SPI_CTRLR0_ADDR_L_Pos)
#define DW_SPI_SPI_CTRLR0_TRANS_TYPE_Pos  (0U)
#define DW_SPI_SPI_CTRLR0_TRANS_TYPE_Msk  (0b11U << DW_SPI_SPI_CTRLR0_TRANS_TYPE_Pos)

// lyt
#define CTRLR0     0x00
#define CTRLR1     0x04
#define SSIENR     0x08
#define MWCR       0x0C
#define SER        0x10
#define BAUDR      0x14
#define TXFTLR     0x18
#define RXFTLR     0x1C
#define TXFLR      0x20
#define RXFLR      0x24
#define SR         0x28
#define IMR        0x2c
#define ISR        0x30
#define RISR       0x34
#define TXOICR     0x38
#define RXOICR     0x3C
#define RXUICR     0x40
#define MSTICR     0x44
#define ICR        0x48
#define DMACR      0x4C
#define DMATDLR    0x50
#define DMARDLR    0x54
#define DR         0x60
#define SPIMSSEL   0xA0
#define SPI_CTRLR0 0xF4

static inline void dw_spi_set_spi_frf(uint32_t spi_base, uint32_t mode) {
  REG_WRITE(spi_base + CTRLR0, REG_READ(spi_base + CTRLR0) & (~DW_SPI_CTRLR0_SPI_FRF_Msk));
  REG_WRITE(spi_base + CTRLR0, REG_READ(spi_base + CTRLR0) | (mode << DW_SPI_CTRLR0_SPI_FRF_Pos));
}

static inline void dw_spi_set_dfs_32(uint32_t spi_base, uint32_t dfs) {
  // dfs_32 does not less than 2
  REG_WRITE(spi_base + CTRLR0, REG_READ(spi_base + CTRLR0) & (~DW_SPI_CTRLR0_DFS_32_Msk));
  // REG_WRITE(spi_base+CTRLR0, REG_READ(spi_base+CTRLR0) | (dfs << DW_SPI_CTRLR0_DFS_32_Pos));
  // REG_WRITE(spi_base+CTRLR0, REG_READ(spi_base+CTRLR0) & ((0b00011U) << DW_SPI_CTRLR0_DFS_32_Pos));
  REG_WRITE(spi_base + CTRLR0, REG_READ(spi_base + CTRLR0) | ((dfs - 1) << DW_SPI_CTRLR0_DFS_32_Pos));
}

static inline void dw_spi_set_dfs(uint32_t spi_base, uint32_t dfs) {
  REG_WRITE(spi_base + CTRLR0, REG_READ(spi_base + CTRLR0) & (~DW_SPI_CTRLR0_DFS_Msk));
  REG_WRITE(spi_base + CTRLR0, REG_READ(spi_base + CTRLR0) | ((dfs - 1) << DW_SPI_CTRLR0_DFS_Pos));
}

static inline void dw_spi_config_ctl_frame_len(uint32_t spi_base, uint32_t len) {
  REG_WRITE(spi_base + CTRLR0, REG_READ(spi_base + CTRLR0) & (~DW_SPI_CTRLR0_CFS_Msk));
  REG_WRITE(spi_base + CTRLR0, REG_READ(spi_base + CTRLR0) | (len << DW_SPI_CTRLR0_CFS_Pos));
}

static inline void dw_spi_enable_test_mode(uint32_t spi_base) {
  REG_WRITE(spi_base + CTRLR0, REG_READ(spi_base + CTRLR0) | DW_SPI_CTRLR0_SRL_EN);
}

static inline void dw_spi_disable_test_mode(uint32_t spi_base) {
  REG_WRITE(spi_base + CTRLR0, REG_READ(spi_base + CTRLR0) & (~DW_SPI_CTRLR0_SRL_EN));
}

static inline void dw_spi_set_tx_rx_mode(uint32_t spi_base) {
  REG_WRITE(spi_base + CTRLR0, REG_READ(spi_base + CTRLR0) & (~DW_SPI_CTRLR0_TMOD_Msk));
  REG_WRITE(spi_base + CTRLR0, REG_READ(spi_base + CTRLR0) | DW_SPI_CTRLR0_TMOD_TX_RX);
}

static inline void dw_spi_set_tx_mode(uint32_t spi_base) {
  REG_WRITE(spi_base + CTRLR0, REG_READ(spi_base + CTRLR0) & (~DW_SPI_CTRLR0_TMOD_Msk));
  REG_WRITE(spi_base + CTRLR0, REG_READ(spi_base + CTRLR0) | DW_SPI_CTRLR0_TMOD_TX);
}

static inline void dw_spi_set_rx_mode(uint32_t spi_base) {
  REG_WRITE(spi_base + CTRLR0, REG_READ(spi_base + CTRLR0) & (~DW_SPI_CTRLR0_TMOD_Msk));
  REG_WRITE(spi_base + CTRLR0, REG_READ(spi_base + CTRLR0) | DW_SPI_CTRLR0_TMOD_RX);
}

static inline uint32_t dw_spi_get_transfer_mode(uint32_t spi_base) {
  return REG_READ(spi_base + CTRLR0) & DW_SPI_CTRLR0_TMOD_Msk;
}

static inline void dw_spi_set_eeprom_mode(uint32_t spi_base) {
  REG_WRITE(spi_base + CTRLR0, REG_READ(spi_base + CTRLR0) & (~DW_SPI_CTRLR0_TMOD_Msk));
  REG_WRITE(spi_base + CTRLR0, REG_READ(spi_base + CTRLR0) | DW_SPI_CTRLR0_TMOD_EEPROM);
}

static inline void dw_spi_set_cpol0(uint32_t spi_base) {
  REG_WRITE(spi_base + CTRLR0, REG_READ(spi_base + CTRLR0) & (~(DW_SPI_CTRLR0_SCPOL_EN)));
}

static inline void dw_spi_set_cpol1(uint32_t spi_base) {
  REG_WRITE(spi_base + CTRLR0, REG_READ(spi_base + CTRLR0) | DW_SPI_CTRLR0_SCPOL_EN);
}

static inline void dw_spi_set_cpha0(uint32_t spi_base) {
  REG_WRITE(spi_base + CTRLR0, REG_READ(spi_base + CTRLR0) & (~(DW_SPI_CTRLR0_SCPH_EN)));
}

static inline void dw_spi_set_cpha1(uint32_t spi_base) {
  REG_WRITE(spi_base + CTRLR0, REG_READ(spi_base + CTRLR0) | DW_SPI_CTRLR0_SCPH_EN);
}

static inline void dw_spi_set_motorola_spi_mode(uint32_t spi_base) {
  REG_WRITE(spi_base + CTRLR0, REG_READ(spi_base + CTRLR0) & (~DW_SPI_CTRLR0_FRF_Msk));
  REG_WRITE(spi_base + CTRLR0, REG_READ(spi_base + CTRLR0) | DW_SPI_CTRLR0_FRF_MOTOROLA_SPI);
}

static inline void dw_spi_set_ti_ssp_mode(uint32_t spi_base) {
  REG_WRITE(spi_base + CTRLR0, REG_READ(spi_base + CTRLR0) & (~DW_SPI_CTRLR0_FRF_Msk));
  REG_WRITE(spi_base + CTRLR0, REG_READ(spi_base + CTRLR0) | DW_SPI_CTRLR0_FRF_TI_SSP);
}

static inline void dw_spi_set_mw_mode(uint32_t spi_base) {
  REG_WRITE(spi_base + CTRLR0, REG_READ(spi_base + CTRLR0) & (~DW_SPI_CTRLR0_FRF_Msk));
  REG_WRITE(spi_base + CTRLR0, REG_READ(spi_base + CTRLR0) | DW_SPI_CTRLR0_FRF_MW_SPI);
}

static inline void dw_spi_config_rx_data_len(uint32_t spi_base, uint32_t len) {
  REG_WRITE(spi_base + CTRLR1, len);
}

static inline void dw_spi_enable_trigger0(uint32_t spi_base) {
  REG_WRITE(spi_base + SSIENR, REG_READ(spi_base + SSIENR) | DW_SPI_SSIENR_SSI_HW0_EN);
}

static inline void dw_spi_disable_trigger0(uint32_t spi_base) {
  REG_WRITE(spi_base + SSIENR, REG_READ(spi_base + SSIENR) & (~DW_SPI_SSIENR_SSI_HW0_EN));
}

static inline void dw_spi_enable_trigger1(uint32_t spi_base) {
  REG_WRITE(spi_base + SSIENR, REG_READ(spi_base + SSIENR) | DW_SPI_SSIENR_SSI_HW1_EN);
}

static inline void dw_spi_disable_trigger1(uint32_t spi_base) {
  REG_WRITE(spi_base + SSIENR, REG_READ(spi_base + SSIENR) & DW_SPI_SSIENR_SSI_HW1_EN);
}

static inline void dw_spi_enable(uint32_t spi_base) {
  REG_WRITE(spi_base + SSIENR, REG_READ(spi_base + SSIENR) | DW_SPI_SSIENR_SSI_EN);
}

static inline void dw_spi_disable(uint32_t spi_base) {
  REG_WRITE(spi_base + SSIENR, REG_READ(spi_base + SSIENR) & (~(DW_SPI_SSIENR_SSI_EN)));
}

static inline void dw_spi_set_mw_sequential_transfer(uint32_t spi_base) {
  REG_WRITE(spi_base + MWCR, REG_READ(spi_base + MWCR) | DW_SPI_MWCR_MWMOD_SQUENTIAL);
}

static inline void dw_spi_set_mw_nonsequential_transfer(uint32_t spi_base) {
  REG_WRITE(spi_base + MWCR, REG_READ(spi_base + MWCR) &= (~DW_SPI_MWCR_MWMOD_Msk));
  REG_WRITE(spi_base + MWCR, REG_READ(spi_base + MWCR) | DW_SPI_MWCR_MWMOD_NOSQUENTIAL);
}

static inline void dw_spi_set_mw_direction_output(uint32_t spi_base) {
  REG_WRITE(spi_base + MWCR, REG_READ(spi_base + MWCR) & (~DW_SPI_MWCR_MDD_Msk));
  REG_WRITE(spi_base + MWCR, REG_READ(spi_base + MWCR) | DW_SPI_MWCR_MDD_OUTPUT);
}

static inline void dw_spi_set_mw_direction_input(uint32_t spi_base) {
  REG_WRITE(spi_base + MWCR, REG_READ(spi_base + MWCR) & (~DW_SPI_MWCR_MDD_Msk));
  REG_WRITE(spi_base + MWCR, REG_READ(spi_base + MWCR) | DW_SPI_MWCR_MDD_INPUT);
}

static inline void dw_spi_enable_mw_handshaking(uint32_t spi_base) {
  REG_WRITE(spi_base + MWCR, REG_READ(spi_base + MWCR) | DW_SPI_MWCR_MHS_EN);
}

static inline void dw_spi_disable_mw_handshaking(uint32_t spi_base) {
  REG_WRITE(spi_base + MWCR, REG_READ(spi_base + MWCR) & DW_SPI_MWCR_MHS_EN);
}

static inline void dw_spi_enable_slave(uint32_t spi_base, uint32_t idx) {
  REG_WRITE(spi_base + SER, REG_READ(spi_base + SER) | ((uint32_t)1U << idx));
}

static inline void dw_spi_disable_slave(uint32_t spi_base, uint32_t idx) {
  REG_WRITE(spi_base + SER, REG_READ(spi_base + SER) & (~((uint32_t)1U << idx)));
}

static inline void dw_spi_disable_all_slave(uint32_t spi_base) {
  REG_WRITE(spi_base + SER, 0U);
}

static inline void dw_spi_config_tx_fifo_threshold(uint32_t spi_base, uint32_t value) {
  REG_WRITE(spi_base + TXFTLR, value & DW_SPI_TXFTLR_TFT_Msk);
}

static inline void dw_spi_config_rx_fifo_threshold(uint32_t spi_base, uint32_t value) {
  REG_WRITE(spi_base + RXFTLR, value & DW_SPI_RXFTLR_RFT_Msk);
}

static inline uint32_t dw_spi_get_tx_fifo_level(uint32_t spi_base) {
  return REG_READ(spi_base + TXFLR);
}

static inline uint32_t dw_spi_get_rx_fifo_level(uint32_t spi_base) {
  return REG_READ(spi_base + RXFLR);
}

static inline uint32_t dw_spi_get_status(uint32_t spi_base) {
  return REG_READ(spi_base + SR);
}

static inline void dw_spi_enable_all_irq(uint32_t spi_base) {
  REG_WRITE(spi_base + IMR, REG_READ(spi_base + IMR) | DW_SPI_IMR_TXEIM_EN);
  REG_WRITE(spi_base + IMR, REG_READ(spi_base + IMR) | DW_SPI_IMR_TXOIM_EN);
  REG_WRITE(spi_base + IMR, REG_READ(spi_base + IMR) | DW_SPI_IMR_RXUIM_EN);
  REG_WRITE(spi_base + IMR, REG_READ(spi_base + IMR) | DW_SPI_IMR_RXOIM_EN);
  REG_WRITE(spi_base + IMR, REG_READ(spi_base + IMR) | DW_SPI_IMR_RXFIM_EN);
  REG_WRITE(spi_base + IMR, REG_READ(spi_base + IMR) | DW_SPI_IMR_MSTIM_EN);
}

static inline void dw_spi_disable_all_irq(uint32_t spi_base) {
  REG_WRITE(spi_base + IMR, 0U);
}

static inline void dw_spi_enable_multi_master_irq(uint32_t spi_base) {
  REG_WRITE(spi_base + IMR, REG_READ(spi_base + IMR) | DW_SPI_IMR_MSTIM_EN);
}

static inline void dw_spi_disable_multi_master_irq(uint32_t spi_base) {
  REG_WRITE(spi_base + IMR, REG_READ(spi_base + IMR) & ~DW_SPI_IMR_MSTIM_EN);
}

static inline void dw_spi_enable_rx_fifo_full_irq(uint32_t spi_base) {
  REG_WRITE(spi_base + IMR, REG_READ(spi_base + IMR) | DW_SPI_IMR_RXFIM_EN);
}

static inline void dw_spi_disable_rx_fifo_full_irq(uint32_t spi_base) {
  REG_WRITE(spi_base + IMR, REG_READ(spi_base + IMR) & ~DW_SPI_IMR_RXFIM_EN);
}

static inline void dw_spi_enable_rx_fifo_overflow_irq(uint32_t spi_base) {
  REG_WRITE(spi_base + IMR, REG_READ(spi_base + IMR) | DW_SPI_IMR_RXOIM_EN);
}

static inline void dw_spi_disable_rx_fifo_overflow_irq(uint32_t spi_base) {
  REG_WRITE(spi_base + IMR, REG_READ(spi_base + IMR) &= ~DW_SPI_IMR_RXOIM_EN);
}

static inline void dw_spi_enable_rx_fifo_underflow_irq(uint32_t spi_base) {
  REG_WRITE(spi_base + IMR, REG_READ(spi_base + IMR) |= DW_SPI_IMR_RXUIM_EN);
}

static inline void dw_spi_disable_rx_fifo_underflow_irq(uint32_t spi_base) {
  REG_WRITE(spi_base + IMR, REG_READ(spi_base + IMR) &= ~DW_SPI_IMR_RXUIM_EN);
}

static inline void dw_spi_enable_tx_fifo_overflow_irq(uint32_t spi_base) {
  REG_WRITE(spi_base + IMR, REG_READ(spi_base + IMR) |= DW_SPI_IMR_TXOIM_EN);
}

static inline void dw_spi_disable_tx_fifo_overflow_irq(uint32_t spi_base) {
  REG_WRITE(spi_base + IMR, REG_READ(spi_base + IMR) &= ~DW_SPI_IMR_TXOIM_EN);
}

static inline void dw_spi_enable_tx_empty_irq(uint32_t spi_base) {
  REG_WRITE(spi_base + IMR, REG_READ(spi_base + IMR) |= DW_SPI_IMR_TXEIM_EN);
}

static inline void dw_spi_disable_tx_empty_irq(uint32_t spi_base) {
  REG_WRITE(spi_base + IMR, REG_READ(spi_base + IMR) &= ~DW_SPI_IMR_TXEIM_EN);
}

static inline uint32_t dw_spi_get_interrupt_status(uint32_t spi_base) {
  return REG_READ(spi_base + ISR);
}

static inline uint32_t dw_spi_get_raw_interrupt_status(uint32_t spi_base) {
  return REG_READ(spi_base + RISR);
}

static inline void dw_spi_clr_tx_fifo_overflow_irq(uint32_t spi_base) {
  spi_base + TXOICR;
}

static inline void dw_spi_clr_rx_fifo_overflow_irq(uint32_t spi_base) {
  spi_base + RXOICR;
}

static inline void dw_spi_clr_rx_fifo_underflow_irq(uint32_t spi_base) {
  spi_base + RXUICR;
}

static inline void dw_spi_clr_multi_master_irq(uint32_t spi_base) {
  spi_base + MSTICR;
}

static inline void dw_spi_clr_all_irqs(uint32_t spi_base) {
  spi_base + ICR;
}

static inline void dw_spi_enable_tx_dma(uint32_t spi_base) {
  REG_WRITE(spi_base + DMACR, REG_READ(spi_base + DMACR) | DW_SPI_DMACR_TDMAE_EN);
}

static inline void dw_spi_disable_tx_dma(uint32_t spi_base) {
  REG_WRITE(spi_base + DMACR, REG_READ(spi_base + DMACR) & (~DW_SPI_DMACR_TDMAE_EN));
}

static inline void dw_spi_enable_rx_dma(uint32_t spi_base) {
  REG_WRITE(spi_base + DMACR, REG_READ(spi_base + DMACR) | DW_SPI_DMACR_RDMAE_EN);
}

static inline void dw_spi_disable_rx_dma(uint32_t spi_base) {
  REG_WRITE(spi_base + DMACR, REG_READ(spi_base + DMACR) & (~DW_SPI_DMACR_RDMAE_EN));
}

static inline void dw_spi_config_dma_tx_data_level(uint32_t spi_base, uint32_t value) {
  REG_WRITE(spi_base + DMATDLR, value & DW_SPI_DMATDLR_DMATDL_Msk);
}

static inline uint32_t dw_spi_get_dma_tx_data_level(uint32_t spi_base) {
  return REG_READ(spi_base + DMATDLR);
}

static inline void dw_spi_config_dma_rx_data_level(uint32_t spi_base, uint32_t value) {
  REG_WRITE(spi_base + DMARDLR, value & DW_SPI_DMARDLR_DMARDL_Msk);
}

static inline uint32_t dw_spi_get_dma_rx_data_level(uint32_t spi_base) {
  return REG_READ(spi_base + DMARDLR);
}

static inline void dw_spi_transmit_data(uint32_t spi_base, uint32_t data) {
  REG_WRITE(spi_base + DR, data);
}

static inline uint32_t dw_spi_receive_data(uint32_t spi_base) {
  return REG_READ(spi_base + DR);
}

static inline void dw_spi_set_master_mode(uint32_t spi_base) {
  REG_WRITE(spi_base + SPIMSSEL, REG_READ(spi_base + SPIMSSEL) & (~DW_SPI_SPIMSSEL_Msk));
  REG_WRITE(spi_base + SPIMSSEL, REG_READ(spi_base + SPIMSSEL) | DW_SPI_SPIMSSEL_MASTER);
}

static inline void dw_spi_set_slave_mode(uint32_t spi_base) {
  REG_WRITE(spi_base + SPIMSSEL, REG_READ(spi_base + SPIMSSEL) & (~DW_SPI_SPIMSSEL_Msk));
  REG_WRITE(spi_base + SPIMSSEL, REG_READ(spi_base + SPIMSSEL) | DW_SPI_SPIMSSEL_SLAVE);
}

static inline uint32_t dw_spi_get_slave_mode(uint32_t spi_base) {
  return REG_READ(spi_base + SPIMSSEL & DW_SPI_SPIMSSEL_Msk);
}

static inline void dw_spi_set_wait_cycles(uint32_t spi_base, uint32_t wait_cycles) {
  REG_WRITE(spi_base + SPI_CTRLR0, REG_READ(spi_base + SPI_CTRLR0) & (~DW_SPI_SPI_CTRLR0_WAIT_CYCLES_Msk));
  REG_WRITE(spi_base + SPI_CTRLR0, REG_READ(spi_base + SPI_CTRLR0) | (wait_cycles << DW_SPI_SPI_CTRLR0_WAIT_CYCLES_Pos));
}

static inline uint32_t dw_spi_get_wait_cycles(uint32_t spi_base) {
  return (REG_READ(spi_base + SPI_CTRLR0) >> DW_SPI_SPI_CTRLR0_WAIT_CYCLES_Pos) & DW_SPI_SPI_CTRLR0_WAIT_CYCLES_Msk;
}

static inline void dw_spi_set_inst_l(uint32_t spi_base, uint32_t inst_length) {
  REG_WRITE(spi_base + SPI_CTRLR0, REG_READ(spi_base + SPI_CTRLR0) & (~DW_SPI_SPI_CTRLR0_INST_L_Msk));
  REG_WRITE(spi_base + SPI_CTRLR0, REG_READ(spi_base + SPI_CTRLR0) | (inst_length << DW_SPI_SPI_CTRLR0_INST_L_Pos));
}

static inline uint32_t dw_spi_get_inst_l(uint32_t spi_base) {
  return (REG_READ(spi_base + SPI_CTRLR0) >> DW_SPI_SPI_CTRLR0_INST_L_Pos) & DW_SPI_SPI_CTRLR0_INST_L_Msk;
}

static inline void dw_spi_set_addr_l(uint32_t spi_base, uint32_t addr_length) {
  REG_WRITE(spi_base + SPI_CTRLR0, REG_READ(spi_base + SPI_CTRLR0) & (~DW_SPI_SPI_CTRLR0_ADDR_L_Msk));
  REG_WRITE(spi_base + SPI_CTRLR0, REG_READ(spi_base + SPI_CTRLR0) | (addr_length << DW_SPI_SPI_CTRLR0_ADDR_L_Pos));
}

static inline uint32_t dw_spi_get_addr_l(uint32_t spi_base) {
  return (REG_READ(spi_base + SPI_CTRLR0) >> DW_SPI_SPI_CTRLR0_ADDR_L_Pos) & DW_SPI_SPI_CTRLR0_ADDR_L_Msk;
}

static inline void dw_spi_set_trans_type(uint32_t spi_base, uint32_t type) {
  REG_WRITE(spi_base + SPI_CTRLR0, REG_READ(spi_base + SPI_CTRLR0) & (~DW_SPI_SPI_CTRLR0_TRANS_TYPE_Msk));
  REG_WRITE(spi_base + SPI_CTRLR0, REG_READ(spi_base + SPI_CTRLR0) | (type << DW_SPI_SPI_CTRLR0_TRANS_TYPE_Pos));
}

static inline uint32_t dw_spi_get_trans_type(uint32_t spi_base) {
  return (REG_READ(spi_base + SPI_CTRLR0) >> DW_SPI_SPI_CTRLR0_TRANS_TYPE_Pos) & DW_SPI_SPI_CTRLR0_TRANS_TYPE_Msk;
}

static inline void dw_spi_set_ndf(uint32_t spi_base, uint32_t type) {
  REG_WRITE(spi_base + CTRLR1, REG_READ(spi_base + CTRLR1) & (~DW_SPI_CTRLR1_NDF_Msk));
  REG_WRITE(spi_base + CTRLR1, REG_READ(spi_base + CTRLR1) | (type << DW_SPI_CTRLR1_NDF_Pos));
}

static inline uint32_t dw_spi_get_ndf(uint32_t spi_base) {
  return (REG_READ(spi_base + CTRLR1) >> DW_SPI_CTRLR1_NDF_Pos) & DW_SPI_CTRLR1_NDF_Msk;
}

extern void dw_spi_config_sclk_clock(uint32_t spi_base, uint32_t clock_in, uint32_t clock_out);
extern uint32_t dw_spi_get_sclk_clock_div(uint32_t spi_base);
extern uint32_t dw_spi_get_data_frame_len(uint32_t spi_base);
extern void dw_spi_config_data_frame_len(uint32_t spi_base, uint32_t size);
extern void dw_spi_reset_regs(uint32_t spi_base);
#ifdef __cplusplus
}
#endif

#endif /* _DW_SPI_LL_H_*/
