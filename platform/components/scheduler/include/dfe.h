#ifndef __DFE_H_
#define __DFE_H_

#include "common.h"
#include "venusmmap.h"

#define INIT_PACKAGE_NUM          4
#define MAX_SYMBOL_IMEM_N         4
#define SYMBOL_IMEM_START_ADDR(N) (0x10100100 + N * 0x22c0)

extern void dma_transfer(uint32_t src, uint32_t dst, uint32_t len, uint32_t last);
extern uint32_t Mask_irq(uint32_t mask_irq);
extern void tx_ram0_transmit(int length, int* data_addr);
extern void tx_ram1_transmit(int length, int* data_addr);

#define CONFIG_DFE_REG(base, value) \
  WRITE_BURST_32(VENUS_DFE_BASE_ADDR, base, value)
#define READ_DFE_REG(base) \
  READ_BURST_32(VENUS_DFE_BASE_ADDR, base)

#define GENERATE_MASK(N) ((1 << (N)) - 1)

#define PROCESS_DMA_TRANSFER(current_write_address_offset, write_mode)                          \
  do {                                                                                          \
    rx_addr         = GC0802_DFE_RX_RAM_ADDR + current_write_address_offset;                    \
    remaining_space = GC0802_DFE_RX_RAM_ADDR_END - rx_addr;                                     \
    if (write_mode == 1) {                                                                      \
      malloc_addr = (uint32_t)malloc(RX_DATA_BLOCKSIZE_8);                                      \
      if (remaining_space >= RX_DATA_EXTENDED_8) {                                              \
        dma_transfer(rx_addr, malloc_addr, RX_DATA_EXTENDED_8, 1);                              \
      } else {                                                                                  \
        excess_space = RX_DATA_EXTENDED_8 - remaining_space;                                    \
        dma_transfer(rx_addr, malloc_addr, remaining_space, 0);                                 \
        dma_transfer(GC0802_DFE_RX_RAM_ADDR, (malloc_addr + remaining_space), excess_space, 1); \
      }                                                                                         \
    } else {                                                                                    \
      malloc_addr = (uint32_t)malloc(RX_DATA_BLOCKSIZE_16);                                     \
      if (remaining_space >= RX_DATA_EXTENDED_16) {                                             \
        dma_transfer(rx_addr, malloc_addr, RX_DATA_EXTENDED_16, 1);                             \
      } else {                                                                                  \
        excess_space = RX_DATA_EXTENDED_16 - remaining_space;                                   \
        dma_transfer(rx_addr, malloc_addr, remaining_space, 0);                                 \
        dma_transfer(GC0802_DFE_RX_RAM_ADDR, (malloc_addr + remaining_space), excess_space, 1); \
      }                                                                                         \
    }                                                                                           \
  } while (0)

/*
 * 16bit:
 *  (160+2048+1)*32bits in rx ram | 2209*4 = 8836bytes
 *  (144+2048+1)*32bits in rx ram | 2193*4 = 8772bytes
 * 8bit:
 *  ((160+2048)*16 + 32)bits in rx ram | 2210*2 = 4420bytes
 *  ((144+2048)*16 + 32)bits in rx ram | 2194*2 = 4388bytes
 */
#define RX_DATA_BLOCKSIZE_16 0x22c0  // align_up(8836,64)
#define RX_DATA_BLOCKSIZE_8  0x1180  // align_up(4420,64)
#define RX_DATA_EXTENDED_16  8836
// #define RX_DATA_EXTENDED_16_ALIGN 8896
// #define RX_DATA_NORMAL_16       8772
#define RX_DATA_EXTENDED_16_ALIGN (mu == 0 ? 8836 : 4452)
#define RX_DATA_NORMAL_16         (mu == 0 ? 8772 : 4388)
#define RX_DATA_NORMAL_16_ALIGN   8832
#define RX_DATA_EXTENDED_8        4420
#define RX_DATA_NORMAL_8          4388
#define MAXTH                     0xffffffff

// Systolic array registers' offset
#define COEFFICIENT_REG 0x00
#define INPUT_CTRL_REG  0x04
#define DFE_ENA_REG     0x08
#define CH_CTRL_REG(N)  (0x0c + N * 0x4)  // N = 0 ~ 14

//tx fir
#define TX_COE_CFG   0x4c
#define TX_CH_CFG(N) (0x50 + N * 0x4)

#define CORRELATION_CTRL_REG  0x64
#define LOW_PASS_FIR_CTRL_REG 0x68
#define RX_FIFO_CTRL_REG      0x6c
#define TX_FIFO_CTRL_REG      0x70

#define TX_RF_DATA_SEL        0x74

// Mixer register's offset
#define RX_MIXER_CTRL_REG2(N) (0x80 + N * 0x4)  // N = 0 ~ 3 [0x80, 0x84, 0x88, 0x8c]
#define RX_MIXER_CTRL_REG(N)  (0x94 + N * 0x4)  // N = 0 ~ 3 [0x94, 0x98, 0x9c, 0xa0]
#define TX_MIXER_CTRL_REG(N)  (0xa8 + N * 0x4)  // N = 0 ~ 3 [0xa8, 0xac, 0xb0, 0xb4]
#define RX_NCO_CTRL_REG       0xa4
#define TX_NCO_CTRL_REG       0xb8

// PSS sychronization register's offset
#define SYNC_CTRL_REG         0xc0
#define DECIMATION_CTRL_REG   0xc4
#define CORRELATION_CTRL0_REG 0xc8
#define PSS_STATUS0_REG(N)    (0xcc + N * 0x04)  // N = 0 ~ 2 [0xcc, 0xd0, 0xd4]
#define PSS_CTRL0_REG         0xd8
#define PEAK_ENABLE_CTRL0_REG 0xdc
#define PEAK_ENABLE_CTRL1_REG 0xe0

// SSB register's offset
#define SSB_STATUS0_REG 0xe4
#define SSB_STATUS1_REG 0xe8
#define SSB_STATUS2_REG 0xec
#define SSB_CTRL0_REG   0xf0

// APB timer registers' offset
#define SYMBOL_CTRL0_REG      0x100
#define SYMBOL_CTRL1_REG      0x104
#define TIMER_STATUS0_REG     0X10c
#define SLOT_TIMER_CTRL0_REG  0x140
#define SLOT_TIMER_CTRL1_REG  0x144
#define SLOT_TIMER_CTRL2_REG  0x148
#define FRAME_TIMER_CTRL0_REG 0x14c

#define TX_SYMBOL_CTRL0_REG      0x150
#define TX_SYMBOL_CTRL1_REG      0x154
#define TX_SLOT_TIMER_CTRL0_REG  0x158
#define TX_SLOT_TIMER_CTRL1_REG  0x15c
#define TX_SLOT_TIMER_CTRL2_REG  0x160
#define TX_FRAME_TIMER_CTRL0_REG 0x164

// RAM read and write control register's offset
#define WRITE_CTRL_REG   0x180
#define WRITE_STATUS_REG 0x184
#define REAL_CTRL_REG    0x188
#define READ_STATUS_REG 0x18c
#define WRITE_CTRL1_REG 0x190

#define INTR_STATUS_REG 0x1bc
#define INTR_CTRL0_REG  0x1c0
#define INTR_CTRL1_REG  0x1c4

// Coefficient bits offset [COEFFICIENT_REG]
#define SYSTOLIC_INDEX_5(N) ((uint32_t)(N) << 27)  // [31-27] index = 0~12
#define PE_INDEX_5(N)       ((uint32_t)(N) << 22)  // [26-22] index = 0~15
#define MEM_ADDR_5(N)       ((uint32_t)(N) << 17)  // [21-17] index = 0~15
#define RESERVED16_1        ((uint32_t)0 << 16)    // [16]
#define COE_VALUE_16(N)     ((uint32_t)(N) << 0)   // [15-00]
// Input select [INPUT_CTRL_REG]
#define RESERVED_1_31     ((uint32_t)0 << 1)    // [31-01]
#define INPUT_SELECT_1(N) ((uint32_t)(N) << 0)  // [00]
// DFE enable bis offset [DFE_ENA_REG]
// #define RESERVED_1_31 ((uint32_t)0 << 1)    // [31-01]
#define DFE_ENA_1(N) ((uint32_t)(N) << 0)  // [00]
// Channel control bits offset [CH_CTRL_REG]
#define INPUT_ENA_1(N)   ((uint32_t)(N) << 31)  // [31] 1 - input data from outside | 0 - input data from last systolic array(cascade)
#define OUTPUT_ENA_1(N)  ((uint32_t)(N) << 30)  // [30]
#define CH_ENA_1(N)      ((uint32_t)(N) << 29)  // [29]
#define SUM_RS_BITS_4(N) ((uint32_t)(N) << 25)  // [28-25] rs = right shift 0~12
#define RESERVED_5_20    ((uint32_t)0 << 5)     // [24-05]
#define CLK_RATE_5(N)    ((uint32_t)(N) << 0)   // [04-00]
// correlational control bit offset [CORRELATION_CTRL_REG]
#define RESERVED_2_30 ((uint32_t)0 << 2)    // [31-02]
#define CASCADE_2(N)  ((uint32_t)(N) << 0)  // [01-00]
// low pass fir control bit offset [LOW_PASS_FIR_CTRL_REG]
// #define RESERVED_1_31 ((uint32_t)0 << 1)    // [31-01]
#define BYPASS_1(N) ((uint32_t)(N) << 0)
// rx fifo control bit offset [RX_FIFO_CTRL_REG]
#define RESERVED_8_24     ((uint32_t)0 << 8)    // [31-08]
#define READ_PERIOOD_8(N) ((uint32_t)(N) << 0)  // [07-00]
// tx fifo control bit offset [TX_FIFO_CTRL_REG]
// #define RESERVED_8_24     ((uint32_t)0 << 8)    // [31-08]
#define WRITE_PERIOOD_8(N) ((uint32_t)(N) << 0)  // [07-00]
// data select bit offset [TX_RF_DATA_SEL_REG]
#define DATA_SELECT_1(N) ((uint32_t)(N) << 0)  // [00-00]
// Mix control bits offset [RX_MIXER_CTRL_REG][TX_MIXER_CTRL_REG][RX_MIXER_CTRL_REG2]
#define NCO_HIGH_BIT(N)      ((uint32_t)((uint64_t)(N) >> 32))  // right shift 32bit to reserve [47:32]bit
#define NCO_LOW_BIT(N)       ((uint32_t)((uint64_t)(N) >> 0))   // reserve [31:00]bit
#define NCO_STEP_LOW_32(N)   ((uint32_t)(N) << 0)               // [31-00] - In mixer ctrl 0
#define NCO_STEP_HIGH_16(N)  ((uint32_t)(N) << 0)               // [15-00] - In mixer ctrl 1
#define NCO_PHASE_LOW_32(N)  ((uint32_t)(N) << 0)               // [31-00] - In mixer ctrl 2
#define NCO_PHASE_HIGH_16(N) ((uint32_t)(N) << 0)               // [15-00] - In mixer ctrl 3
// NCO control bits offset [RX_NCO_CTRL_REG]
// #define RESERVED_1_31 ((uint32_t)0 << 1)  // [31-01]
#define NCO_RESET_1(N) ((uint32_t)(N) << 0)  // [00] Write 1 to reset NCO, then this register is automatically set to 0
// Sync control bits offset [SYNC_CTRL_REG]
#define RESERVED_11_21             ((uint32_t)0 << 11)   // [31-11]
#define SIGNAL_ENERGY_RS_5(N)      ((uint32_t)(N) << 6)  // [10-06]
#define CORRELATION_ENERGY_RS_5(N) ((uint32_t)(N) << 1)  // [05-01]
// #define BYPASS_1(N) ((uint32_t)(N) << 0)  // [00] 0 - use 3 systolic arrays to enable complex fir function | 1 - disable complex function
// Decimation control bits offset [DECIMATION_CTRL_REG]
#define RESERVED_5_27         ((uint32_t)0 << 5)    // [31-05]
#define DECIMATION_RATIO_5(N) ((uint32_t)(N) << 0)  // [04-00] 0 - no dec | n - nx downsampling
// Correlation coe bits offset [CORRELATION_CTRL0_REG]
#define TH_32(N) ((uint32_t)(N) << 0)  // [31-00] product of cross correlation coefficient and enegry
// PSS status bits offset [PSS_STATUS0_REG]
#define TH_WRITEBACK_32(N) ((uint32_t)(N) << 0)  // [31-00]
// PSS control bits offset [PSS_CTRL0_REG]
// #define RESERVED_1_31 ((uint32_t)0 << 1)  // [31-01]
#define TH_WB_CLEAR_1(N) ((uint32_t)(N) << 0)  // [00]
// peak enable control register [PEAK_ENABLE_CTRL0_REG]
#define SYNC_START_TIME_16(N) ((uint32_t)(N) << 16)  // [31-16]
#define SYNC_END_TIME_16(N)   ((uint32_t)(N) << 0)   // [15-00]
// peak enable control register [PEAK_ENABLE_CTRL1_REG]
#define SYNC_PERIOD_16(N)       ((uint32_t)(N) << 16)  // [31:16]
#define RESERVED_1_15           ((uint32_t)0 << 1)     // [15-01]
#define PEAK_WINDOW_ENALBE_1(N) ((uint32_t)(N) << 0)   // [00]
// SSB status bits offset [SSB_STATUS0_REG]
#define SSB_TIME_32(N) ((uint32_t)(N) << 0)  // [31-00]
// SSB control bits offset [SSB_CTRL0_REG]
// #define RESERVED_1_31 ((uint32_t)0 << 1)  // [31-01]
#define SSB_TIME_CLEAR_1(N) ((uint32_t)(N) << 0)  // [00]
// Symbol control 0 register bits offset [SYMBOL_CTRL0_REG]
#define RESERVED_23_9            ((uint32_t)0 << 23)    // [31-23]
#define SYMBOL_LENGTH_VALID_1(N) ((uint32_t)(N) << 22)  // [22]
#define SYMBOL_INDEX_6(N)        ((uint32_t)(N) << 16)  // [21-16]
#define SYMBOL_LENGTH_16(N)      ((uint32_t)(N) << 0)   // [15-00]
// Symbol control 1 register bits offset [SYMBOL_CTRL1_REG]
#define RESERVED_6_26      ((uint32_t)0 << 6)    // [31-06]
#define SYMBOL_NUMBER_6(N) ((uint32_t)(N) << 0)  // [05-00]
// Slot timer control 0 register bits offset [SLOT_TIMER_CTRL0_REG]
#define CNT_PERIOD_26(N)         ((uint32_t)(N) << 6)  // [31-06]
#define SLOT_INDEX_BOUNDARY_6(N) ((uint32_t)(N) << 0)  // [05-00]
// Slot timer control 1 register bits offset [SLOT_TIMER_CTRL1_REG]
#define FRONT_OFFSET_26(N)         ((uint32_t)(N) << 6)  // [31-06]
#define RESERVED_0_6               ((uint32_t)0 << 0)    // [05-00]
#define CURRENT_FRONT_OFFSET_26(N) ((uint32_t)(N) >> 6)  // [31-06]
// Slot timer control 2 register bits offset [SLOT_TIMER_CTRL2_REG]
#define RESERVED_6_26          ((uint32_t)0 << 6)    // [31-06]
#define SLOT_INDEX_OFFSET_6(N) ((uint32_t)(N) << 0)  // [05-00]
// Frame timer control 0 register bits offset [FRAME_TIMER_CTRL0_REG]
// #define RESERVED_16_16     ((uint32_t)0 << 16)  // [31-16]
#define FRAME_INDEX_BOUNDARY_16(N) ((uint32_t)(N) << 0)  // [16-00]
// [WRITE_CTRL_REG]
#define RESERVED_1_31   ((uint32_t)0 << 1)    // [31-01]
#define WRITE_MODE_1(N) ((uint32_t)(N) << 0)  //[00]
#define READ_MODE_1(N)  ((uint32_t)(N) << 0)  //[00]
// Ram write/read status bits offset [WRITE_STATUS_REG] [READ_STATUS_REG]
#define RESERVED_16_16              ((uint32_t)0 << 16)    // [31-16]
#define CURRENT_WRITE_ADDRESS_16(N) ((uint32_t)(N) >> 0)   // [16-00]
#define CURRENT_READ_ADDRESS_16(N)  ((uint32_t)(N) >> 0)   // [16-00]
#define FRAME_INDEX_16(N)           ((uint32_t)(N) >> 16)  // [31-16]
#define RESERVED_14_2               ((uint32_t)0 >> 14)    // [15-14]
#define SLOT_CNT_6(N)               ((uint32_t)(N) >> 8)   // [13-08]
#define RESERVED_6_2                ((uint32_t)0 >> 6)     // [07-06]
#define SYMBOL_CNT_6(N)             ((uint32_t)N >> 0)     // [05-00]

// write control bit offset [WRITE_CTRL1_REG]
#define RESERVED_3_29          ((uint32_t)0 << 3)  // [31-03]
#define WRITE_DATA_SELECT_3(N) ((uint32_t)N << 0)  // [02-00]
// interrupt status [INTR_STATUS_REG]
#define PSS_0_OVF(N)  ((uint32_t)N >> 31)
#define PSS_1_OVF(N)  ((uint32_t)N >> 30)
#define PSS_2_OVF(N)  ((uint32_t)N >> 29)
#define ENERGY_OVF(N) ((uint32_t)N >> 28)
// #define SYNC(N) 		((uint32_t)N >> 18)
// #define PEAK(N) 		((uint32_t)N >> 17)
#define RX_RAM_READY(N) ((uint32_t)N >> 16)
#define CH11_OVF(N)     ((uint32_t)N >> 11)
#define CH10_OVF(N)     ((uint32_t)N >> 10)
#define CH9_OVF(N)      ((uint32_t)N >> 9)
#define CH8_OVF(N)      ((uint32_t)N >> 8)
#define CH7_OVF(N)      ((uint32_t)N >> 7)
#define CH6_OVF(N)      ((uint32_t)N >> 6)
#define CH5_OVF(N)      ((uint32_t)N >> 5)
#define CH4_OVF(N)      ((uint32_t)N >> 4)
#define CH3_OVF(N)      ((uint32_t)N >> 3)
#define CH2_OVF(N)      ((uint32_t)N >> 2)
#define CH1_OVF(N)      ((uint32_t)N >> 1)
#define CH0_OVF(N)      ((uint32_t)N >> 0)
// interrupt clear [INTR_CTRL0_REG] [FOR USE]
#define CLEAR_PSS_0_OVF_1(N)    ((uint32_t)N << 31)
#define CLEAR_PSS_1_OVF_1(N)    ((uint32_t)N << 30)
#define CLEAR_PSS_2_OVF_1(N)    ((uint32_t)N << 29)
#define CLEAR_ENERGY_OVF_1(N)   ((uint32_t)N << 28)
#define CLEAR_RX_RAM_READY_1(N) ((uint32_t)N << 16)
#define CLEAR_CH11_OVF_1(N)     ((uint32_t)N << 11)
#define CLEAR_CH10_OVF_1(N)     ((uint32_t)N << 10)
#define CLEAR_CH9_OVF_1(N)      ((uint32_t)N << 9)
#define CLEAR_CH8_OVF_1(N)      ((uint32_t)N << 8)
#define CLEAR_CH7_OVF_1(N)      ((uint32_t)N << 7)
#define CLEAR_CH6_OVF_1(N)      ((uint32_t)N << 6)
#define CLEAR_CH5_OVF_1(N)      ((uint32_t)N << 5)
#define CLEAR_CH4_OVF_1(N)      ((uint32_t)N << 4)
#define CLEAR_CH3_OVF_1(N)      ((uint32_t)N << 3)
#define CLEAR_CH2_OVF_1(N)      ((uint32_t)N << 2)
#define CLEAR_CH1_OVF_1(N)      ((uint32_t)N << 1)
#define CLEAR_CH0_OVF_1(N)      ((uint32_t)N << 0)
// interrupt clear [INTR_CTRL0_REG]
// #define RESERVED_1_31 ((uint32_t)0 << 1)  // [31-01]
#define CLEAR_ALL_INTER_1(N) ((uint32_t)N << 0)  // [00]

/*
 * [COEFFICIENT_REG]
 * systolic_index: 0~12
 * pe_index: 0~15
 * mem_addr: 0~15
 * coe_data: 16bit signed data
 */
#define COE_VALUE_CONFIG(systolic_index, pe_index, mem_addr, coe_data) \
  ((SYSTOLIC_INDEX_5(systolic_index & GENERATE_MASK(5))) |             \
   (PE_INDEX_5(pe_index & GENERATE_MASK(5))) |                         \
   (MEM_ADDR_5(mem_addr & GENERATE_MASK(5))) |                         \
   (RESERVED16_1) |                                                    \
   (COE_VALUE_16(coe_data & GENERATE_MASK(16))))

/*
 * [INPUT_CTRL_REG]
 * input_select: 0 - normal mode | 1 - swap I channel and Q channel
 */
#define INPUT_CTRL_CONFIG(input_select) \
  ((RESERVED_1_31) |                    \
   (INPUT_SELECT_1(input_select & GENERATE_MASK(1))))

/*
 * [CH_CTRL_REG]
 * input/output_ena: 1 - data from/to ADC or AXI | 0 - cascade
 * ch_ena: enable CH or not
 * sum_rs_bits: right shift count | range: 0~12
 * clk_rate: determine the number of filter coefficients that could be used
 * CH0-CH2:  complex-low-pass fir channel
 * CH3-CH5:  cross-correlation channels for pss0 sequences
 * CH6-CH8:  cross-correlation channels for pss1 sequences
 * CH9-CH11: cross-correlation channels for pss2 sequences
 * CH12: calculate the energy of the sampling data (no sum_rs_bits)
 * CH13-CH14: reserved channel for longer PSS sequences (no sum_rs_bits)
 */
#define CH_CTRL_CONFIG(input_ena, output_ena, ch_ena, sum_rs_bis, clk_rate) \
  ((INPUT_ENA_1(input_ena & GENERATE_MASK(1))) |                            \
   (OUTPUT_ENA_1(output_ena & GENERATE_MASK(1))) |                          \
   (CH_ENA_1(ch_ena & GENERATE_MASK(1))) |                                  \
   (SUM_RS_BITS_4(sum_rs_bis & GENERATE_MASK(4))) |                         \
   (RESERVED_5_20) |                                                        \
   (CLK_RATE_5(clk_rate & GENERATE_MASK(5))))

/*
 * [CORRELATION_CTRL_REG]
 * cascade: 00 - enable three independent channels
 *          01 - PSS 0 channel is cascaded with PSS 1 channel, the PSS sequence length is extended to 512
 *          10 - cascade three PSS channels, the pss sequence length is extended to 768
 */
#define CORRELATION_CTRL_CONFIG(cascade) \
  ((RESERVED_2_30) |                     \
   (CASCADE_2(cascade & GENERATE_MASK(2))))
/*
 * [LOW_PASS_FIR_CTRL_REG]
 * bypass: ignore this component or not
 */
#define LOW_PASS_FIR_CTRL_CONFIG(bypass) \
  ((RESERVED_1_31) |                     \
   (BYPASS_1(bypass & GENERATE_MASK(1))))

/*
 * [RX_FIFO_CTRL_REG]
 * read_period: ADC asy fifo read period
 */
#define RX_FIFO_CTRL_CONFIG(read_period) \
  ((RESERVED_8_24) |                     \
   (READ_PERIOOD_8(read_period & GENERATE_MASK(8))))

/*
 * [TX_FIFO_CTRL_REG]
 * write_period: ADC asy fifo write period
 */
#define TX_FIFO_CTRL_CONFIG(write_period) \
  ((RESERVED_8_24) |                      \
   (WRITE_PERIOOD_8(write_period & GENERATE_MASK(8))))

/*
 * [TX_RF_DATA_SEL_REG]
 * data select: 0 - data from tx ram 
 *              1 - data from rf async fifo
 */
#define TX_RF_DATA_SEL_CONFIG(data_select) \
  ((RESERVED_1_31) |                      \
   (DATA_SELECT_1(data_select & GENERATE_MASK(1))))

/*
 * [RX_MIXER_CTRL_REG][TX_MIXER_CTRL_REG][RX_MIXER_CTRL_REG2]
 * nco_step: 48bit
 * nco_phase: 48bit
 */
#define MIXER_CTRL0_CONFIG(nco_step) \
  (NCO_STEP_LOW_32(NCO_LOW_BIT(nco_step)))
#define MIXER_CTRL1_CONFIG(nco_step) \
  ((RESERVED_16_16) |                \
   (NCO_STEP_HIGH_16(NCO_HIGH_BIT(nco_step))))
#define MIXER_CTRL2_CONFIG(nco_phase) \
  (NCO_PHASE_LOW_32(NCO_LOW_BIT(nco_phase)))
#define MIXER_CTRL3_CONFIG(nco_phase) \
  ((RESERVED_16_16) |                 \
   (NCO_PHASE_HIGH_16(NCO_HIGH_BIT(nco_phase))))

/*
 * [RX_NCO_CTRL_REG]
 * reset: write 1 to reset NCO, then this register will automatically set to 0
 */
#define NCO_CTRL_CONFIG(reset) \
  ((RESERVED_1_31) |           \
   (NCO_RESET_1(reset & GENERATE_MASK(1))))

/*
 * [SYNC_CTRL_REG]
 * signal_energy_rs: 5bit
 * correlation_energy_rs: 5bit
 * bypass: 0 - use 3 systolic arrays to enable complex fir function
 *             (ch0-ch2, ch3-ch5, ch6-ch8, ch9-ch11 are 4 complex fir filter)
 *         1 - disable complex function
 */
#define SYNC_CTRL_CONFIG(bypass, signal_energy_rs, correlation_energy_rs) \
  ((RESERVED_1_31) |                                                      \
   (SIGNAL_ENERGY_RS_5(signal_energy_rs & GENERATE_MASK(5))) |            \
   (CORRELATION_ENERGY_RS_5(correlation_energy_rs & GENERATE_MASK(5))) |  \
   (BYPASS_1(bypass & GENERATE_MASK(1))))

/*
 * [DECIMATION_CTRL_REG]
 * decimation_ratio: times of downsampling
 *                   0  - no decimation
 *                   1  - 2x downsampling
 *                   ...
 *                   31 - 32x downsampling
 */
#define DECIMATION_CTRL_CONFIG(decimation_ratio) \
  ((RESERVED_5_27) |                             \
   (DECIMATION_RATIO_5(decimation_ratio & GENERATE_MASK(5))))

/*
 * [CORRELATION_CTRL0_REG]
 * th: 32bit signed data
 */
#define CORRELATION_CTRL0_CONFIG(th) \
  (TH_32(th))

/*
 * [PSS_STATUS0_REG]
 * th_writeback: DFE computation result [FOR READ]
 */
#define PSS_STATUS0_READ(th_writeback) \
  (TH_WRITEBACK_32(th_writeback))

/*
 * [PSS_CTRL0_REG]
 * clear: clear all th_wb registers
 */
#define PSS_CTRL0_CONFIG(th_wb_clear) \
  ((RESERVED_1_31) |                  \
   (TH_WB_CLEAR_1(th_wb_clear & GENERATE_MASK(1))))

/*
 * [PEAK_ENABLE_CTRL0_REG]
 * disable_time [31:16]
 * enable_time  [15:00]
 */
#define PEAK_ENABLE_CTRL0_CONFIG(sync_start_time, sync_end_time) \
  ((SYNC_START_TIME_16(sync_start_time & GENERATE_MASK(16))) |   \
   (SYNC_END_TIME_16(sync_end_time & GENERATE_MASK(16))))

/*
 * [PEAK_ENABLE_CTRL1_REG]
 */
#define PEAK_ENABLE_CTRL1_CONFIG(sync_period, peak_window_enable) \
  ((SYNC_PERIOD_16(sync_period & GENERATE_MASK(16))) |            \
   (RESERVED_1_15) |                                              \
   (PEAK_WINDOW_ENALBE_1(peak_window_enable & GENERATE_MASK(1))))

/*
 * [SSB_STATUS0_REG]
 * pss_time: DFE computation result [FOR READ]
 */
#define SSB_STATUS0_READ(pss_time) \
  (SSB_TIME_32(pss_time))

/*
 * [SSB_CTRL0_REG]
 * clear: clear all th_wb registers
 */
#define SSB_CTRL0_CONFIG(pss_time_clear) \
  ((RESERVED_1_31) |                     \
   (SSB_TIME_CLEAR_1(pss_time_clear & GENERATE_MASK(1))))

/*
 * [SYMBOL_CTRL0_REG]
 * symbol_length_valid: told DFE how to read
 * symbol_index: to config which symbol
 * symbol_length: config length
 * The symbol length is written to the ram address space indicated by the symbol index
 * Maximum value of the symbol index is 64, so a maximum of 64 symbol lengths can be stored
 * e.g. symbol length = 144 + 2048 - 1 = 2191
 */
#define SYMBOL_CTRL0_CONFIG(symbol_length_valid, symbol_index, symbol_length) \
  ((RESERVED_23_9) |                                                          \
   (SYMBOL_LENGTH_VALID_1(symbol_length_valid & GENERATE_MASK(1))) |          \
   (SYMBOL_INDEX_6(symbol_index & GENERATE_MASK(6))) |                        \
   (SYMBOL_LENGTH_16(symbol_length & GENERATE_MASK(16))))

/*
 * [SYMBOL_CTRL1_REG]
 * symbol_number: Indicate the number of symbol length stored in ram,
 *                if user uses this register to indicate that the number of symbol length stored in ram is 14,
 *                this register should be configured as 13.
 *                This register can be configured as any integer between 0 and 63.
 */
#define SYMBOL_CTRL1_CONFIG(symbol_number) \
  ((RESERVED_6_26) |                       \
   (SYMBOL_NUMBER_6(symbol_number & GENERATE_MASK(6))))

/* [FRAME_TIMER_CTRL0_REG]
 * frame_index_boundary: 16bit
 */
#define FRAME_TIMER_CTRL0_CONFIG(frame_index_boundary) \
  ((RESERVED_16_16) |                                  \
   (FRAME_INDEX_BOUNDARY_16(frame_index_boundary & GENERATE_MASK(16))))

/*
 * [SLOT_TIMER_CTRL0_REG]
 * cnt_period: 26bit
 *             The initial value of the counter is cnt_period.
 *             After user enable dfe, counter decrements by 1 each clock cycle.
 *             When the counter counts to 0, it will be set to cnt_period on the arrival of the next rising edge of the clock.
 *             PSS peak also set counter to cnt_period.
 *             While counter = 0, slot_start signal assert 1 clk at the arrival of the next rising edge of the clock.
 * slot_index_boundary: 6bit
 *                      If slot_index_boundary = 9, slot index counter counts from 0 to 9
 */
#define SLOT_TIMER_CTRL0_CONFIG(cnt_period, slot_index_boundary) \
  ((CNT_PERIOD_26(cnt_period & GENERATE_MASK(26))) |             \
   (SLOT_INDEX_BOUNDARY_6(slot_index_boundary & GENERATE_MASK(6))))

/*
 * [SLOT_TIMER_CTRL1_REG]
 * front_offset: 26bit
 *               If front_offset = 10, while counter = 10,
 *               slot_front_offset start  signal assert 1 clk at the arrival of the next rising edge of the clock.
 */
#define SLOT_TIMER_CTRL1_CONFIG(front_offset)            \
  ((FRONT_OFFSET_26(front_offset & GENERATE_MASK(26))) | \
   (RESERVED_0_6))
#define FRONT_OFFSEST_READ(slot_timter_ctrl1_reg) \
  (CURRENT_FRONT_OFFSET_26(slot_timter_ctrl1_reg) & GENERATE_MASK(26))

/*
 * [SLOT_TIMER_CTRL2_REG]
 * slot_index_offset: 6bit
 */
#define SLOT_TIMER_CTRL2_CONFIG(slot_index_offset) \
  ((RESERVED_6_26) |                               \
   (SLOT_INDEX_OFFSET_6(slot_index_offset & GENERATE_MASK(6))))

/* [WRITE_CTRL_REG] : 0 - 16bit | 1 - 8bit*/
#define WRITE_CTRL_CONFIG(write_mode) \
  ((RESERVED_1_31) |                  \
   (WRITE_MODE_1(write_mode)))

/*
 * [WRITE_STATUS_REG] [READ_STATUS_REG]
 * current_write_address: 16bit [FOR READ] */
#define WRITE_STATUS_READ(current_write_address) \
  ((RESERVED_16_16) |                            \
   (CURRENT_WRITE_ADDRESS_16(current_write_address & GENERATE_MASK(16))))
#define REAL_STATUS_READ(current_write_address) \
  ((RESERVED_16_16) |                           \
   (CURRENT_WRITE_ADDRESS_16(current_write_address & GENERATE_MASK(16))))
/*{ 16'frame_index, 2'd0, 6'slot_cnt, 2'd0, 6'symbol_cnt }*/
#define FRAME_INDEX_READ(raw_dout_reg) \
  (FRAME_INDEX_16(raw_dout_reg) & GENERATE_MASK(16))
#define SLOT_CNT_READ(raw_dout_reg) \
  (SLOT_CNT_6(raw_dout_reg) & GENERATE_MASK(6))
#define SYMBOL_CNT_READ(raw_dout_reg) \
  (SYMBOL_CNT_6(raw_dout_reg) & GENERATE_MASK(6))

/*
 * [WRITE_CTRL1_REG]
 * write_data_select: 000 - A | 001 - B | 010 - C | 011 - D | 100 - E
 */
#define WRITE_CTRL1_CONFIG(write_data_select) \
  ((RESERVED_3_29) |                          \
   (WRITE_DATA_SELECT_3(write_data_select & GENERATE_MASK(3))))

#define READ_CTRL_CONFIG(tx_write_mode) \
  ((RESERVED_1_31) |                    \
   (READ_MODE_1(tx_write_mode)))
/*
 * [INTR_CTRL1_REG]
 * clear: clear all th_wb registers
 */
#define INTR_CTRL1_CONFIG(clear_all_intr) \
  ((RESERVED_1_31) |                      \
   (CLEAR_ALL_INTER_1(clear_all_intr & GENERATE_MASK(1))))

/*
 * [INTR_STATUS_REG]
 * [FOR READ]
 */
#define PSS_0_OVF_READ(intr_status_reg)    (PSS_0_OVF(intr_status_reg) & GENERATE_MASK(1))
#define PSS_1_OVF_READ(intr_status_reg)    (PSS_1_OVF(intr_status_reg) & GENERATE_MASK(1))
#define PSS_2_OVF_READ(intr_status_reg)    (PSS_2_OVF(intr_status_reg) & GENERATE_MASK(1))
#define ENERGY_OVF_READ(intr_status_reg)   (ENERGY_OVF(intr_status_reg) & GENERATE_MASK(1))
#define RX_RAM_READY_READ(intr_status_reg) (RX_RAM_READY(intr_status_reg) & GENERATE_MASK(1))
#define CH11_OVF_READ(intr_status_reg)     (CH11_OVF(intr_status_reg) & GENERATE_MASK(1))
#define CH10_OVF_READ(intr_status_reg)     (CH10_OVF(intr_status_reg) & GENERATE_MASK(1))
#define CH9_OVF_READ(intr_status_reg)      (CH9_OVF(intr_status_reg) & GENERATE_MASK(1))
#define CH8_OVF_READ(intr_status_reg)      (CH8_OVF(intr_status_reg) & GENERATE_MASK(1))
#define CH7_OVF_READ(intr_status_reg)      (CH7_OVF(intr_status_reg) & GENERATE_MASK(1))
#define CH6_OVF_READ(intr_status_reg)      (CH6_OVF(intr_status_reg) & GENERATE_MASK(1))
#define CH5_OVF_READ(intr_status_reg)      (CH5_OVF(intr_status_reg) & GENERATE_MASK(1))
#define CH4_OVF_READ(intr_status_reg)      (CH4_OVF(intr_status_reg) & GENERATE_MASK(1))
#define CH3_OVF_READ(intr_status_reg)      (CH3_OVF(intr_status_reg) & GENERATE_MASK(1))
#define CH2_OVF_READ(intr_status_reg)      (CH2_OVF(intr_status_reg) & GENERATE_MASK(1))
#define CH1_OVF_READ(intr_status_reg)      (CH1_OVF(intr_status_reg) & GENERATE_MASK(1))
#define CH0_OVF_READ(intr_status_reg)      (CH0_OVF(intr_status_reg) & GENERATE_MASK(1))
// if ch3_ovf - ch11_ovf set to 1, add 1 to sum_rs_bits
#define CH3_11_OVF_READ(intr_status_reg) (CH3_OVF(intr_status_reg) & GENERATE_MASK(9))
// if energy_ovf set to 1, add 1 to signal_energy_rs
// #define ENERGY_OVF_READ(intr_status_reg)   (ENERGY_OVF(intr_status_reg) & GENERATE_MASK(1))
// if pss_0_ovf - pss_2_ovf set to 1, add 1 to correlation_energy_rs
#define PSS_0_2_OVF_READ(intr_status_reg) (PSS_2_OVF(intr_status_reg) & GENERATE_MASK(3))

#define DFE_SET_NCO_FREQ_OFFSET_FRONTEND(FREQ)                      \
  do {                                                              \
    CONFIG_DFE_REG(RX_MIXER_CTRL_REG(0), MIXER_CTRL0_CONFIG(FREQ)); \
    CONFIG_DFE_REG(RX_MIXER_CTRL_REG(1), MIXER_CTRL1_CONFIG(FREQ)); \
  } while (0);

#define DFE_SET_NCO_FREQ_OFFSET_BACKEND(FREQ)                        \
  do {                                                               \
    CONFIG_DFE_REG(RX_MIXER_CTRL_REG2(0), MIXER_CTRL0_CONFIG(FREQ)); \
    CONFIG_DFE_REG(RX_MIXER_CTRL_REG2(1), MIXER_CTRL1_CONFIG(FREQ)); \
  } while (0);

long long nco_step_caculate(double freqOffset);

extern volatile int SEM_dfe_initdata_cnt;
extern volatile int SHARED_after_sync_slot;
extern volatile int SHARED_req_pss_slot;
extern volatile int SHARED_req_pss_frame;
extern volatile int SEM_dma_initdata_cnt;
extern volatile int init_sync_int_cnt;
extern volatile int MUTEX_gc_modify_timing_done;
extern volatile int symbol_imem_addr_cnt;
extern volatile int SHARED_pss0_status0_reg;
extern volatile int SHARED_pss1_status0_reg;
extern volatile int SHARED_pss2_status0_reg;
extern volatile int SHARED_req_ssb_start_symbol;
extern volatile int MUTEX_first_rx_ram_ready_done;
extern volatile int SHARED_frame_offset;
extern volatile int SEM_dfe_data_cnt;
extern volatile int SEM_max_dfe_data_cnt;

#endif /* __DFE_H_ */
