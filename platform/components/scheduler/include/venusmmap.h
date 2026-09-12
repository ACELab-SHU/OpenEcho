#ifndef __VENUSMMAP_H_
#define __VENUSMMAP_H_

/* Venus SoC Address Map [GC0802] */

// Test configurations
#define NUM_BLOCKS   6
#define NUM_CLUSTERS 8

// Maximum number of configurations supported
#define MAX_NUM_CLUSTERS 8
#define MAX_NUM_BLOCKS   12

/* L1 Scheduler External Interrupts, Level Triggered [P.11] */
#define VENUS_IRQ_CLUSTER(N) (3 + (N))  // 3,4,5,6,7,8,9,10
#define VENUS_IRQ_DMA        11
#define VENUS_IRQ_DMA_ERR    12
#define VENUS_IRQ_DFE_OVF    13
#define VENUS_IRQ_FLASH_SPI  14
#define VENUS_IRQ_RF_SPI     15
#define VENUS_IRQ_UART1      16
#define VENUS_IRQ_GPIO       17
#define VENUS_IRQ_WDT        18
#define VENUS_IRQ_TIMER(N)   (19 + (N))  // 19,20,21,22,23
// RESERVED: 24
#define VENUS_IRQ_DFE_RX_RAM_READY 25
#define VENUS_IRQ_DFE_PEAK         26
#define VENUS_IRQ_SYNC             27
#define VENUS_IRQ_TX_RAM_READ_DONE 28
#define VENUS_IRQ_USB              29
#define VENUS_IRQ_JESD             30
#define VENUS_IRQ_USBSCH           31

/* VENUS GC0802 AXI4 */
#define VENUS_BOOTROM_BASE_ADDR      0x00000000UL
#define VENUS_FLASH_APB_BASE_ADDR    0x00020000UL
#define VENUS_APB_BASE_ADDR          0x1fff0000UL
#define VENUS_DFE_BASE_ADDR          0x1fff6000UL
#define VENUS_L1_SCHEDULER_PROG_ADDR 0x10000000UL
#define VENUS_DDR_ADDR               0x10020000UL
#define VENUS_DMA_BASE_ADDR          0x1ffe0000UL
#define VENUS_CLUSTER_BASE_ADDR      0x20000000UL

/* VENUS GC0802 Prototype */
#define GC0802_CCM_ADDR                                        0x1fff0000UL
#define GC0802_DEVCTRL_ADDR                                    0x1fff0800UL
#define GLOBAL_VENUS_CLUSTER0_L2_SCHEDULER_PROGRAM_MEMORY_ADDR 0x20000000UL
#define GLOBAL_VENUS_CLUSTER0_L2_MEMORY_ADDR                   0x20020000UL
#define GC0802_DFE_RX_RAM_ADDR                                 0x01000000UL
#define GC0802_DFE_RX_RAM_ADDR_END                             0x01004500UL
#define GC0802_DFE_TX_RAM_ADDR                                 0x01100000UL
#define GC0802_FLASH_SPI_ADDR                                  0x00020000UL
#define GC0802_UART0_ADDR                                      0x1fff1000UL
#define GC0802_UART1_ADDR                                      0x1fff1400UL
#define GC0802_UART2_ADDR                                      0x1fff1800UL
#define GC0802_RF_SPI_ADDR                                     0x1fff3000UL
#define GC0802_GPIO0_ADDR                                      0x1fff4000UL
#define GC0802_DFE_CFG_ADDR                                    0x1fff6000UL
#define GC0802_WDT_ADDR                                        0x1fff7000UL
#define GC0802_TIMER_ADDR                                      0x1fff7800UL
#define GC0802_IMEM_ADDR                                       0x10000000UL
#define GC0802_DMEM_ADDR                                       0x10020000UL
#define GC0802_USB2_ADDR                                       0xF0000000UL
#define GC0802_JESD_ADDR                                       0xF0080000UL

/* CCM */
/* PLL
 * bit[31] PLL锁定输出。指示PLL已锁定：0b：未锁定 1b：已锁定
 * bit[25] PLL输出选择。0h：正常模式 1h：旁路模式 CLKOUT=CLKIN
 * bit[24] 关机。关闭模拟模块并复位数字D触发器。在上电PLL（90%*vdd）或更改M/N设置后，应在PLL锁定前至少激活10ns的关机状态。0b：正常操作。1b：关机模式，CLK_OUT为0且锁定为0。
 * bit[17:16] 输出分频器控制。设置分频控制。
   - 00b：除以1，OD=1。01b：除以2，OD=2。10b：除以4，OD=4。11b：除以8，OD=8。
 * bit[11:8] 输入分频器控制。设置输入分频系数。
   - N = dn[3:0]
 * bit[7:01] 反馈分频器控制。设置反馈分频系数。
   - M= dm[7:0]
 */
#define GC0802_MPLL_REG_OFFSET       0x00
#define GC0802_DFEPLL_REG_OFFSET     0x04
#define GC0802_BPLL_REG_OFFSET       0x08
#define GC0802_AXI_DIV_REG_OFFSET    0x0c
#define GC0802_APB_DIV_REG_OFFSET    0x10
#define GC0802_RF_CFG_DIV_REG_OFFSET 0x14

/* DEVCTRL */
#define GC0802_AXI_DEV_RST_OFFSET                   0x04
#define GC0802_APB_DEV_RST_OFFSET                   0x08
#define GC0802_RF_DFE_DEV_RST_OFFSET                0x0c
#define GC0802_CLUSTER0_DEV_RST_OFFSET              0x10
#define GC0802_DEBUG_OFFSET                         0x14
#define GC0802_FLASH_SPI_CTRL_REG_OFFSET            0xfc
#define GC0802_FLASH_PROGRAMMING_MODE_SWITCH_OFFSET 0x80
#define USB3320_POWER_SWITCH_OFFSET                 0x100
#define AD9361_RST_CTRL_OFFSET                      0x108
#define AD9361_IO_LOGIC_LEVEL_CTRL_OFFSET           0x10c
#define AD9361_DAC_IO_MODE_SWITCH_OFFSET            0x134
#define AD9361_BBP_IO_MODE_SWITCH_OFFSET            0x13c
#define GC0802_IOPAD_CFG_OFFSET                     0x200


// #define GC0802_IOPAD_CTRL_OUT0                      (0)
// #define GC0802_IOPAD_CTRL_OUT1                      (1)
// #define GC0802_IOPAD_CTRL_OUT2                      (2)
// #define GC0802_IOPAD_CTRL_OUT3                      (3)
#define GC0802_ULPI_STP                             (3)
#define GC0802_ULPI_DATA_0                          (4)
#define GC0802_ULPI_DATA_1                          (5)
#define GC0802_ULPI_DATA_2                          (6)
#define GC0802_ULPI_DATA_3                          (7)
#define GC0802_ULPI_DATA_4                          (8)
#define GC0802_ULPI_DATA_5                          (9)
#define GC0802_ULPI_DATA_6                          (10)
#define GC0802_ULPI_DATA_7                          (11)
#define GC0802_IOPAD_P0_D9                          (12)
#define GC0802_IOPAD_P0_D7                          (13)
#define GC0802_IOPAD_P0_D5                          (14)
#define GC0802_IOPAD_P0_D3                          (15)
#define GC0802_IOPAD_P0_D1                          (16)
#define GC0802_IOPAD_P0_D11                         (17)
#define GC0802_IOPAD_P0_D8                          (18)
#define GC0802_IOPAD_P0_D6                          (19)
#define GC0802_IOPAD_P0_D4                          (20)
#define GC0802_IOPAD_P0_D2                          (21)
#define GC0802_IOPAD_P0_D0                          (22)
#define GC0802_IOPAD_P0_D10                         (23)
#define GC0802_IOPAD_FB_CLK                         (24)
#define GC0802_IOPAD_EN_AGC                         (25)
#define GC0802_IOPAD_ENABLE                         (26)
#define GC0802_IOPAD_TX_FRAME                       (28)
#define GC0802_IOPAD_TXNRX                          (30)        
#define GC0802_IOPAD_SYNC_OUT                       (31)
#define GC0802_IOPAD_SPI_DO                         (33)        
#define GC0802_IOPAD_SPI_CLK                        (34)
#define GC0802_IOPAD_PWR_EN                         (35)
#define GC0802_IOPAD_RESETB                         (42)        
#define GC0802_IOPAD_SPI_ENB                        (43)
#define USB3320_PWR_SW                              (50)
#define GC0802_IOPAD_UART_TXD                       (63)
#define GC0802_IOPAD_FLASH_CSn                      (75)
#define GC0802_IOPAD_FLASH_CLK                      (80)
#define GC0802_IOPAD_UART_DEBUG_TXD                 (81)



// #define GC0802_IOPAD_CTRL_IN0                      (4)
// #define GC0802_IOPAD_CTRL_IN1                      (5)
// #define GC0802_IOPAD_CTRL_IN2                      (6)
// #define GC0802_IOPAD_CTRL_IN3                      (7)
// #define GC0802_IOPAD_CTRL_IN4                      (8)
// #define GC0802_IOPAD_CTRL_IN5                      (9)
// #define GC0802_IOPAD_CTRL_IN6                      (10)
// #define GC0802_IOPAD_CTRL_IN7                      (11)
#define GC0802_ULPI_CLK                             (0)
#define GC0802_ULPI_DIR                             (1)
#define GC0802_ULPI_NXT                             (2)
#define GC0802_IOPAD_RX_FRAME                      (27)
#define GC0802_IOPAD_P1_D11                        (32)
#define GC0802_IOPAD_P1_D10                        (36)        
#define GC0802_IOPAD_P1_D9                         (37)
#define GC0802_IOPAD_P1_D7                         (38)
#define GC0802_IOPAD_P1_D5                         (39)
#define GC0802_IOPAD_P1_D3                         (40)
#define GC0802_IOPAD_P1_D1                         (41)
#define GC0802_IOPAD_P1_D8                         (44) 
#define GC0802_IOPAD_P1_D6                         (45) 
#define GC0802_IOPAD_P1_D4                         (46) 
#define GC0802_IOPAD_P1_D2                         (47) 
#define GC0802_IOPAD_P1_D0                         (48) 
#define GC0802_IOPAD_SPI_DI                        (49)
#define GC0802_IOPAD_UART_RXD                      (64)
#define GC0802_IOPAD_WAKE_UP                       (82)
#define GC0802_IOPAD_SPI2_CS                       (100)    
#define GC0802_IOPAD_SPI2_CLK                      (101)

/* Cluster*/

/* VENUS AHB [P.110] */
#define VENUS_DMAC_ADDR 0x1ffe0000UL  // Venus Cluster DMA Slave
/* VENUS APB [P.110] */
#define VENUS_CCM_ADDR        0x1fff0000UL
#define VENUS_DEVCTRL_ADDR    0x1fff0800UL
#define GC0802_PG_ADDR        0x1fff2000UL
#define GC0802_GPIO_ADDR      0x1fff4000UL
#define VENUS_DEBUG_UART_ADDR 0x1fff1000UL
#define VENUS_USER_UART_ADDR  0x1fff1400UL
#define VENUS_USBSCH_UART_ADDR  0x1fff1800UL
#define VENUS_DDR_PHY_ADDR    0x1fff4000UL
#define VENUS_DDR_REG_ADDR    0x1fff8000UL

/* 16 * 4MB for each */
#define VENUS_CLUSTER_ADDR 0x20000000UL

/* VENUS CLUSTER AXI4 [P.111] */
#define CLUSTER_OFFSET(N)             (N << 26)
#define BLOCK_OFFSET(K)               (K << 22)
#define VENUS_CLUSTER_AHB(N)          0x21ffd000 + CLUSTER_OFFSET(N)
#define VENUS_CLUSTER_APB(N)          0x21ffe000 + CLUSTER_OFFSET(N)
#define VENUS_CLUSTER_BLOCK_BASE_ADDR 0x22000000
/* VENUS CLUSTER AHB [P.111] */
#define VENUS_CLUSTER_DMA_SLAVE_ADDR(N) 0x21ffd000 + CLUSTER_OFFSET(N)
/* VENUS CLUSTER APB [P.111] */
#define VENUS_CLUSTER_DEBUG_UART_ADDR(N) 0x21ffe000 + CLUSTER_OFFSET(N)
#define VENUS_CLUSTER_CTRLREG_ADDR(N)    0x21fff000 + CLUSTER_OFFSET(N)
#define CLUSTER_CTRLREGS_OFFSET          0x01fff000UL
#define BLOCK_DEBUGUART_OFFSET           0x001fffffUL

/* [PPT.69]  Venus DevCtrl Module Register address offset and initial values */
/* Update: [P.115] Soc DevCtrl Register Map (Multi-level) */
#define VENUS_SCHEDULER_CORE_RST_OFFSET         0x00
#define VENUS_AHB_DEV_RST_OFFSET                0x04  // VENUS_AXI_DEV_RST_OFFSET in `venus_soc_pkg.sv`
#define VENUS_APB_DEV_RST_OFFSET                0x08
#define VENUS_CLUSTER_DEV_RST_OFFSET_CLUSTER(n) (0x10 + ((n)*0x04))

// #define VENUS_SCHEDULER_EN    (1 << 0)
// #define VENUS_AHB_BUS_EN      (1 << 31)
// #define VENUS_DMA_EN          (1 << 0)
// #define VENUS_AHB_DMA_EN      0x80000001
// #define VENUS_UART_DDR_EN     0x00000003
// #define VENUS_L2_SCHEDULER_EN (1 << 12)

// #define CLUSTER_AXI_BUS_EN       (1 << 31)
// #define VENUS_BLOCK_EN(K)        (1 << K)
// #define CLUSTER_AXI_BLOCK_ALL_EN 0x8000ffff

/* New hardware L2 scheduler mapping */
#define VENUS_CLUSTER_L2_PROG(N) (0x20000000 + N * 0x4000000)
#define VENUS_CLUSTER_L2_MEM(N)  (0x20020000 + N * 0x4000000)
#define VENUS_CLUSTER_L2_CFG(N)  (0x21ff0000 + N * 0x4000000)
#define VENUS_CLUSTER_L2_APB(N)  (0x21ffe000 + N * 0x4000000)

#define VENUS_CLUSTER_L2_DATA_CONTAINER_OFFSET(N)        (N * 0x1000)
#define VENUS_CLUSTER_L2_GLOBAL_PARA_CONTAINER_OFFSET(N) (N * 0x1000 + 0x6000)
#define VENUS_CLUSTER_L2_OUTPUT_NUM_REG_ADDR(N)          (N * 0x40 + 0x8100)
#define VENUS_CLUSTER_L2_TASK_NUM_REG_ADDR(N)            (N * 0x40 + 0x8200)
#define VENUS_CLUSTER_L2_RESET_REG_ADDR                  0x8300  // release L2 scheduler
#define VENUS_CLUSTER_L2_RETURN_VALUE_ADDR(N)            (N * 40 + 0x8400)
#define VENUS_CLUSTER_L2_DAG_RETURN_ADDR                 0x8500  // dag compute done return address offset
#define VENUS_CLUSTER_L2_MALLOC_INIT_ADDRESS             0x8600
#define VENUS_CLUSTER_L2_SHIELD_OFFSET                   0x8640
#define VENUS_CLUSTER_L2_DAG_OUTPUT_ADDR_OFFSET          0x8700
#define VENUS_CLUSTER_L2_DAG_RETURN_ADDR_OFFSET(N)   (0x8500 + (N * 0x8))  // dag compute done return address offset
#define VENUS_CLUSTER_L2_DAG_RETURN_LENGTH_OFFSET(N) (0x8504 + (N * 0x8))  // dag compute done return address offset

/* [P.111] Memory Map (Multi-level) */
#define VENUS_CLUSTER_CFG(N) (0x21fff000 + N * 0x4000000)
/* [P.118] Venus Cluster Control Register Map */
#define VENUS_CLUSTER_CFGREG_OFFSET       0x00
#define VENUS_CLUSTER_INTSTATUSREG_OFFSET 0x08
#define VENUS_CLUSTER_INTCLEARREG_OFFSET  0x10
#define VENUS_CLUSTER_PROGRESSREG_OFFSET  0x18
#define VENUS_CLUSTER_RUNTIMEREG_OFFSET   0x20
#define VENUS_CLUSTER_NUMRETREG_OFFSET    0x28

/* [P.118] Venus Cluster Control Register Map */
#define VENUS_CLUSTER_L2_DMA_ERR_IRQ  0
#define VENUS_CLUSTER_L2_DBG_UART_IRQ 1
#define VENUS_CLUSTER_TILE_IRQ(N)     (2 + N)
#define VENUS_CLUSTER_DAG_IRQ(N)      (14 + N)

/* DMA register */
#define VENUS_L1_DMA_CFGREG                    0x00
#define VENUS_L1_DMA_SRCREG                    0x08
#define VENUS_L1_DMA_DSTREG                    0x10
#define VENUS_L1_DMA_LENREG                    0x18
#define VENUS_L1_DMA_STATREG                   0x20
#define VENUS_L1_DMA_ERRORADDR                 0x28
#define VENUS_L1_DMA_PUSH                      1              // CFGREG[0] == 1, 产生dma_ctrl_write_o
#define VENUS_L1_DMA_LAST                      3              // CFGREG[0] == 1&&CFGREG[1] == 1, 告诉DMA这是此次散列传输的最后一个块
#define VENUS_L1_DMA_CLEAR                     4              // CFGREG[2] == 1&&CFGREG[1] == 1, 清除DMA的中断
#define VENUS_L1_DMA_FULL(STATREG)             (STATREG & 1)  // STATREG[0] == 1, DMA csr的fifo满了，cpu需要hold
#define VENUS_L1_DMA_RD_ERR(STATREG)           ((STATREG >> 1) == 0b00)
#define VENUS_L1_DMA_WR_ERR(STATREG)           ((STATREG >> 1) == 0b01)
#define VENUS_L1_DMA_UNALIGNED_ERR(STATREG)    ((STATREG >> 1) == 0b10)
#define VENUS_L1_DMA_NARROW_CROSS_ERR(STATREG) ((STATREG >> 1) == 0b11)

/* DW Timer */
#define VENUS_DW_APB_WDT_CR  0x00
#define VENUS_DW_APB_WDT_TORR 0x04
#define VENUS_DW_APB_WDT_CRR 0x0c
#define VENUS_DW_APB_WDT_EOI 0x14

#define VENUS_DW_APB_TIMER_LOAD_COUNT(N) (N * 0x14)
#define VENUS_DW_APB_TIMER_CTRL_REG(N)   (0x08 + (N * 0x14))
#define VENUS_DW_APB_TIMER_EOI(N)        (0x0c + (N * 0x14))

#define VENUS_DW_APB_RTC_CLR   0x08
#define VENUS_DW_APB_RTC_CCR   0x0c
#define VENUS_DW_APB_RTC_EOI   0x18
#define VENUS_DW_APB_RTC_SLEEP 0x100

/* POWER GATING CONTROL */
#define GC0802_PG_RTC_CCR_OFFSET 0xC
#define GC0802_PG_RTC_EOI_OFFSET 0x18
#define GC0802_PG_PWREN_OFFSET   0x100

/* GPIO */
#define GPIO_DATA_OFFSET 0x0
#define GPIO_DIR_OFFSET  0x4

#define CONFIG_GC0802_IOPAD_REG(base, value) \
  WRITE_BURST_32((GC0802_DEVCTRL_ADDR + GC0802_IOPAD_CFG_OFFSET), base, value)

#endif /* __VENUSMMAP_H_ */