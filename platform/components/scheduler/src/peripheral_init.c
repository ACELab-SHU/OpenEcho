
#include "types.h"
#include "ulib.h"
#include "venusmmap.h"

/* hal.S */
extern uint32_t Mask_irq(uint32_t mask_irq);

// 1111_0100_0000_0000_0010_0000_0000_0000
void irq_init(void) {
  /*
   * 可以直接在start.S里面写进去...picorv32_maskirq_insn(zero, zero)
   * mask bit = 1 means disable this irq
   * | sync | peak | ram | reserved | ... | rf | flash | ovf | dma err |
   * |  1   |  1   |  0  |    0     | ... |  0 |   0   |  1  |    0    |
   */
  // Mask_irq(0xfc002000);
  Mask_irq(0xfe002000);
}

// {8'h02, r_rsp[23:0]}: {head,r_rsp}
#define RSP_CONCAT(head, rsp) (((head) << 24) | ((rsp)&0x00ffffff))

__attribute__((optimize("O0"))) void GC0802_ioconfig() {
  // Scheduler config IO PAD
  // CONFIG_GC0802_IOPAD_REG(GC0802_IOPAD_CTRL_OUT0 * 0x4, 0x030);
  // CONFIG_GC0802_IOPAD_REG(GC0802_IOPAD_CTRL_OUT1 * 0x4, 0x030);
  // CONFIG_GC0802_IOPAD_REG(GC0802_IOPAD_CTRL_OUT2 * 0x4, 0x030);
  // CONFIG_GC0802_IOPAD_REG(GC0802_IOPAD_CTRL_OUT3 * 0x4, 0x030);
  CONFIG_GC0802_IOPAD_REG(GC0802_IOPAD_P0_D9 * 0x4, 0x030);
  CONFIG_GC0802_IOPAD_REG(GC0802_IOPAD_P0_D7 * 0x4, 0x030);
  CONFIG_GC0802_IOPAD_REG(GC0802_IOPAD_P0_D5 * 0x4, 0x030);
  CONFIG_GC0802_IOPAD_REG(GC0802_IOPAD_P0_D3 * 0x4, 0x030);
  CONFIG_GC0802_IOPAD_REG(GC0802_IOPAD_P0_D1 * 0x4, 0x030);
  CONFIG_GC0802_IOPAD_REG(GC0802_IOPAD_P0_D11 * 0x4, 0x030);
  CONFIG_GC0802_IOPAD_REG(GC0802_IOPAD_P0_D8 * 0x4, 0x030);
  CONFIG_GC0802_IOPAD_REG(GC0802_IOPAD_P0_D6 * 0x4, 0x030);
  CONFIG_GC0802_IOPAD_REG(GC0802_IOPAD_P0_D4 * 0x4, 0x030);
  CONFIG_GC0802_IOPAD_REG(GC0802_IOPAD_P0_D2 * 0x4, 0x030);
  CONFIG_GC0802_IOPAD_REG(GC0802_IOPAD_P0_D0 * 0x4, 0x030);
  CONFIG_GC0802_IOPAD_REG(GC0802_IOPAD_P0_D10 * 0x4, 0x030);
  CONFIG_GC0802_IOPAD_REG(GC0802_IOPAD_FB_CLK * 0x4, 0x030);
  CONFIG_GC0802_IOPAD_REG(GC0802_IOPAD_EN_AGC * 0x4, 0x030);
  CONFIG_GC0802_IOPAD_REG(GC0802_IOPAD_ENABLE * 0x4, 0x030);
  CONFIG_GC0802_IOPAD_REG(GC0802_IOPAD_TX_FRAME * 0x4, 0x030);
  CONFIG_GC0802_IOPAD_REG(GC0802_IOPAD_TXNRX * 0x4, 0x030);
  CONFIG_GC0802_IOPAD_REG(GC0802_IOPAD_SYNC_OUT * 0x4, 0x030);
  CONFIG_GC0802_IOPAD_REG(GC0802_IOPAD_SPI_DO * 0x4, 0x030);
  CONFIG_GC0802_IOPAD_REG(GC0802_IOPAD_SPI_CLK * 0x4, 0x030);
  CONFIG_GC0802_IOPAD_REG(GC0802_IOPAD_PWR_EN * 0x4, 0x030);
  CONFIG_GC0802_IOPAD_REG(GC0802_IOPAD_RESETB * 0x4, 0x030);
  CONFIG_GC0802_IOPAD_REG(GC0802_IOPAD_SPI_ENB * 0x4, 0x030);
  CONFIG_GC0802_IOPAD_REG(GC0802_IOPAD_UART_TXD * 0x4, 0x030);
  CONFIG_GC0802_IOPAD_REG(GC0802_IOPAD_FLASH_CSn * 0x4, 0x030);
  CONFIG_GC0802_IOPAD_REG(GC0802_IOPAD_FLASH_CLK * 0x4, 0x030);
  CONFIG_GC0802_IOPAD_REG(GC0802_IOPAD_UART_DEBUG_TXD * 0x4, 0x030);

  // CONFIG_GC0802_IOPAD_REG(GC0802_IOPAD_CTRL_IN0 * 0x4, 0x039);
  // CONFIG_GC0802_IOPAD_REG(GC0802_IOPAD_CTRL_IN1 * 0x4, 0x039);
  // CONFIG_GC0802_IOPAD_REG(GC0802_IOPAD_CTRL_IN2 * 0x4, 0x039);
  // CONFIG_GC0802_IOPAD_REG(GC0802_IOPAD_CTRL_IN3 * 0x4, 0x039);
  // CONFIG_GC0802_IOPAD_REG(GC0802_IOPAD_CTRL_IN4 * 0x4, 0x039);
  // CONFIG_GC0802_IOPAD_REG(GC0802_IOPAD_CTRL_IN5 * 0x4, 0x039);
  // CONFIG_GC0802_IOPAD_REG(GC0802_IOPAD_CTRL_IN6 * 0x4, 0x039);
  // CONFIG_GC0802_IOPAD_REG(GC0802_IOPAD_CTRL_IN7 * 0x4, 0x039);
  CONFIG_GC0802_IOPAD_REG(GC0802_IOPAD_RX_FRAME * 0x4, 0x039);
  CONFIG_GC0802_IOPAD_REG(GC0802_IOPAD_P1_D11 * 0x4, 0x039);
  CONFIG_GC0802_IOPAD_REG(GC0802_IOPAD_P1_D10 * 0x4, 0x039);
  CONFIG_GC0802_IOPAD_REG(GC0802_IOPAD_P1_D9 * 0x4, 0x039);
  CONFIG_GC0802_IOPAD_REG(GC0802_IOPAD_P1_D7 * 0x4, 0x039);
  CONFIG_GC0802_IOPAD_REG(GC0802_IOPAD_P1_D5 * 0x4, 0x039);
  CONFIG_GC0802_IOPAD_REG(GC0802_IOPAD_P1_D3 * 0x4, 0x039);
  CONFIG_GC0802_IOPAD_REG(GC0802_IOPAD_P1_D1 * 0x4, 0x039);
  CONFIG_GC0802_IOPAD_REG(GC0802_IOPAD_P1_D8 * 0x4, 0x039);
  CONFIG_GC0802_IOPAD_REG(GC0802_IOPAD_P1_D6 * 0x4, 0x039);
  CONFIG_GC0802_IOPAD_REG(GC0802_IOPAD_P1_D4 * 0x4, 0x039);
  CONFIG_GC0802_IOPAD_REG(GC0802_IOPAD_P1_D2 * 0x4, 0x039);
  CONFIG_GC0802_IOPAD_REG(GC0802_IOPAD_P1_D0 * 0x4, 0x039);
  CONFIG_GC0802_IOPAD_REG(GC0802_IOPAD_SPI_DI * 0x4, 0x039);
  CONFIG_GC0802_IOPAD_REG(GC0802_IOPAD_UART_RXD * 0x4, 0x039);
  CONFIG_GC0802_IOPAD_REG(GC0802_IOPAD_WAKE_UP * 0x4, 0x039);
  CONFIG_GC0802_IOPAD_REG(GC0802_IOPAD_SPI2_CS * 0x4, 0x039);
  CONFIG_GC0802_IOPAD_REG(GC0802_IOPAD_SPI2_CLK * 0x4, 0x039);
}

__attribute__((optimize("O0"))) void GC0802_change_pll_settings(uint32_t base_addr, uint32_t offset_addr, uint32_t target_settings) {
  volatile uint32_t r_rsp;
  r_rsp = READ_BURST_32(base_addr, offset_addr);
  WRITE_BURST_32(base_addr, offset_addr, RSP_CONCAT(0x52, r_rsp));
  WRITE_BURST_32(base_addr, offset_addr, RSP_CONCAT(0x7A, r_rsp));
  WRITE_BURST_32(base_addr, offset_addr, RSP_CONCAT(0x7A, target_settings));
  WRITE_BURST_32(base_addr, offset_addr, RSP_CONCAT(0x52, target_settings));
  volatile uint32_t response = READ_BURST_32(base_addr, offset_addr);
  // GC0802_L1_SCHEDULER_BFM_wait_a_bit_change
  while ((response & 0x80000000) != 0x80000000) {
    response = READ_BURST_32(base_addr, offset_addr);
  }
  WRITE_BURST_32(base_addr, offset_addr, RSP_CONCAT(0x50, target_settings));
}

__attribute__((optimize("O0"))) void devctrl_init(void) {
  // 时钟初始化过程
  GC0802_change_pll_settings(GC0802_CCM_ADDR, GC0802_MPLL_REG_OFFSET, 0x00481014);
  // 时钟初始化过程（DFE时钟）
  GC0802_change_pll_settings(GC0802_CCM_ADDR, GC0802_DFEPLL_REG_OFFSET, 0x00481010);
  // 时钟初始化过程（TILE BOOST时钟）
  GC0802_change_pll_settings(GC0802_CCM_ADDR, GC0802_BPLL_REG_OFFSET, 0x00441014);
  
  // /* Venus GC0802 CCM Register Map */
  // // 需要先把分频设置好，等几拍再使能这个时钟
  WRITE_BURST_32(GC0802_CCM_ADDR, GC0802_AXI_DIV_REG_OFFSET, 0xf0000000);
  WRITE_BURST_32(GC0802_CCM_ADDR, GC0802_APB_DIV_REG_OFFSET, 0x80000004);
  WRITE_BURST_32(GC0802_CCM_ADDR, GC0802_RF_CFG_DIV_REG_OFFSET, 0xc0000007);

  /* Venus GC0802 DevCtrl Register Map */
  WRITE_BURST_32(GC0802_DEVCTRL_ADDR, GC0802_AXI_DEV_RST_OFFSET, 0x80000003);
  WRITE_BURST_32(GC0802_DEVCTRL_ADDR, GC0802_APB_DEV_RST_OFFSET, 0x80003fff);
  WRITE_BURST_32(GC0802_DEVCTRL_ADDR, GC0802_RF_DFE_DEV_RST_OFFSET, 0x80000003);
  WRITE_BURST_32(GC0802_DEVCTRL_ADDR, GC0802_CLUSTER0_DEV_RST_OFFSET, 0xdfff3fff);
  WRITE_BURST_32(GC0802_DEVCTRL_ADDR, AD9361_RST_CTRL_OFFSET, 0x00000003);
  WRITE_BURST_32(GC0802_DEVCTRL_ADDR, AD9361_DAC_IO_MODE_SWITCH_OFFSET, 0x00000400);

  GC0802_ioconfig();


  //power up cluster0
  // WRITE_BURST_32(GC0802_DEVCTRL_ADDR, GC0802_CLUSTER0_DEV_RST_OFFSET, READ_BURST_32(GC0802_DEVCTRL_ADDR, GC0802_CLUSTER0_DEV_RST_OFFSET) | 0x2000);
  //power up dfe
  // WRITE_BURST_32(GC0802_DEVCTRL_ADDR, GC0802_RF_DFE_DEV_RST_OFFSET, READ_BURST_32(GC0802_DEVCTRL_ADDR, GC0802_RF_DFE_DEV_RST_OFFSET) | 0x2);
  //wait cluster0 and dfe power stable
  // for (int wait = 0; wait < 1000; wait++) {
  //   ;
  // }

  // USB2 TRY READ

  // =========================Venus Watchdog Timer======================== //
  // /* Timer config */
  // /* 0x04 - Timeout Range Register - bit[7:4]: TOP_INIT, bit[3:0]: TOP */
  // WRITE_BURST_32(GC0802_WDT_ADDR, VENUS_DW_APB_WDT_TORR, 0x00);  // Timeout period = 2^16 ticks (1.31072ms)
  // /* 0x00 - Control Register - bit[0]: WDT_EN */
  // uint32_t dwt_cr = READ_BURST_32(GC0802_WDT_ADDR, VENUS_DW_APB_WDT_CR) | 1;
  // WRITE_BURST_32(GC0802_WDT_ADDR, VENUS_DW_APB_WDT_CR, dwt_cr);  // start wdt
  // /* 0x0c - Counter Restart Register */
  // // This register is used to restart the WDT counter. As a safety feature to
  // // prevent accidental restarts, the value 0x76 must be written.
  // WRITE_BURST_32(GC0802_WDT_ADDR, VENUS_DW_APB_WDT_CRR, 0x76);  // kick dog
  // ===================================================================== //

  // =============================Venus Timers============================ //
  // /* 0x08 * (N*0x14) [TimerN Load Count Register] */
  // /* [1]=1: Timer Mode is User-defined count mode, [0]=1: enable */
  // WRITE_BURST_32(GC0802_TIMER_ADDR, VENUS_DW_APB_TIMER_CTRL_REG(0), 0x3);
  // /* 0x00 * (N*0x14) [TimerN Load Counter Register] */
  // WRITE_BURST_32(GC0802_TIMER_ADDR, VENUS_DW_APB_TIMER_LOAD_COUNT(0), 25000);  // 0.5ms 
  // WRITE_BURST_32(GC0802_TIMER_ADDR, VENUS_DW_APB_TIMER_CTRL_REG(1), 0x3); 
  // WRITE_BURST_32(GC0802_TIMER_ADDR, VENUS_DW_APB_TIMER_LOAD_COUNT(1), 30000);  // 0.6ms
  // WRITE_BURST_32(GC0802_TIMER_ADDR, VENUS_DW_APB_TIMER_CTRL_REG(2), 0x3);
  // WRITE_BURST_32(GC0802_TIMER_ADDR, VENUS_DW_APB_TIMER_LOAD_COUNT(2), 50000);  // 1ms 
  // WRITE_BURST_32(GC0802_TIMER_ADDR, VENUS_DW_APB_TIMER_CTRL_REG(3), 0x3); 
  // WRITE_BURST_32(GC0802_TIMER_ADDR, VENUS_DW_APB_TIMER_LOAD_COUNT(3), 20000);  // 0.4ms
  // WRITE_BURST_32(GC0802_TIMER_ADDR, VENUS_DW_APB_TIMER_CTRL_REG(4), 0x3);
  // WRITE_BURST_32(GC0802_TIMER_ADDR, VENUS_DW_APB_TIMER_LOAD_COUNT(4), 40000);  // 0.8ms
  // // deassert timer1 resetn - bit[12:8] [timer4:timer0] rstn
  // uint32_t response = READ_BURST_32(GC0802_DEVCTRL_ADDR,
  // GC0802_APB_DEV_RST_OFFSET) | 0x1f00; WRITE_BURST_32(GC0802_DEVCTRL_ADDR,
  // GC0802_APB_DEV_RST_OFFSET, response);
  // ===================================================================== //

  // ===============================Venus RTC============================= //
  // printf("Start config rtc: %x...\n", READ_BURST_32(GC0802_PGC_ADDR, 0x1C));
  // RTC
  // load counter
  // This register is 32-bit wide. Set this regsister and RTC will increase by 1
  // every clock cycle. When the counter reaches 0, it will generate a interrupt
  // if it is enabled and not masked. The clock frequency is 32.768kHz. So the
  // loaded value should be: (2^32 - 32768*t), where t is in second. 4294967296
  // - 327680
  /* Counter Load Register */
  // WRITE_BURST_32(GC0802_PGC_ADDR, VENUS_DW_APB_RTC_CLR, 4294639616);
  /* Counter Control Register
   * bit[0] - rtc_ien
   */
  // uint32_t rtc_ccr = 0x1;
  // WRITE_BURST_32(GC0802_PGC_ADDR, VENUS_DW_APB_RTC_CCR, rtc_ccr);
  // // READ_BURST_32(GC0802_PGC_ADDR, VENUS_DW_APB_RTC_SLEEP);
  // WRITE_BURST_32(GC0802_PGC_ADDR, VENUS_DW_APB_RTC_SLEEP, 0);
  // ==================================================================== //

  // rx/tx ram test
  // WRITE_BURST_32(GC0802_DFE_RX_RAM_ADDR, 0x0, 0x624a970f);
  // printf("GC0802_DFE_RX_RAM: %p\n", READ_BURST_32(GC0802_DFE_RX_RAM_ADDR, 0x0));
  // WRITE_BURST_32(GC0802_DFE_TX_RAM_ADDR, 0x0, 0x624a970f);
  // printf("GC0802_DFE_TX_RAM: %p\n", READ_BURST_32(GC0802_DFE_TX_RAM_ADDR, 0x0));
}
