/* global variable to save irq mask values */
#include "common.h"
#include "dma.h"
#include "fifo.h"
#include "hal.h"
#include "ulib.h"
#include "venusmmap.h"
#include "config.h"

extern int printf(const char* s, ...);
extern void dma_interrupt_handler(void);
extern void dma_error_handler(void);
extern void dfe_ovf_interrupt_handler(void);
extern void dfe_rx_ram_ready_interrupt_handler(void);
extern void dfe_tx_ram_read_done_interrupt_handler(void);
extern void dfe_peak_interrupt_handler(void);
extern void dfe_sync_interrupt_handler(void);
extern void dma_transfer(uint32_t src, uint32_t dst, uint32_t len, uint32_t last);
extern void cluster_interrupt_handler(uint32_t cluster_intstatusreg, uint32_t cluster_id);
extern void timer_interrupt_handler(int timer_id);
extern dmafifo_t dmafifo;

/* [2024-0708-timer_test] */
extern void Set_timer(int timer);

uint32_t irq_mask;

void enable_irq(uint32_t irq) {
  irq_mask = EN_Interrupts(1 << irq);
}

void disable_irq(uint32_t irq) {
  irq_mask = DIS_Interrupts(1 << irq);
}

/* In start.S: a0 --> *regs  a1 --> cause(q1) */
uint32_t* irq_handler(uint32_t* regs, uint32_t cause) {
  if (cause & (1 << VENUS_IRQ_WDT)) {
    // 关狗的话EOI和tickle都得不处理
    READ_BURST_32(GC0802_WDT_ADDR, VENUS_DW_APB_WDT_EOI);  // clear irq
    WRITE_BURST_32(GC0802_WDT_ADDR, VENUS_DW_APB_WDT_CRR, 0x76);  // kick dog
  } else if (cause & (1 << VENUS_IRQ_DMA)) {
    dma_interrupt_handler();
  } else if (cause & (1 << VENUS_IRQ_CLUSTER(0))) {
    uint32_t cluster_intstatusreg = READ_BURST_32(VENUS_CLUSTER_CFG(0), VENUS_CLUSTER_INTSTATUSREG_OFFSET);
    cluster_interrupt_handler(cluster_intstatusreg, 0);
  } else if (cause & (1 << VENUS_IRQ_SYNC)) {
    dfe_sync_interrupt_handler();
  } else if (cause & (1 << VENUS_IRQ_DFE_RX_RAM_READY)) {
    dfe_rx_ram_ready_interrupt_handler();
  } else if (cause & (1 << VENUS_IRQ_TX_RAM_READ_DONE)) {
    dfe_tx_ram_read_done_interrupt_handler();
  } else if (cause & (1 << VENUS_IRQ_DFE_OVF)) {
    printf("ovf irq\n");
    dfe_ovf_interrupt_handler();
  } else if (cause & (1 << VENUS_IRQ_DFE_PEAK)) {
    dfe_peak_interrupt_handler();
  } else if (cause & (1 << VENUS_IRQ_TIMER(0))) {
    READ_BURST_32(GC0802_TIMER_ADDR, VENUS_DW_APB_TIMER_EOI(0));  // clear irq
    timer_interrupt_handler(0);
  } else if (cause & (1 << VENUS_IRQ_TIMER(1))) {
    READ_BURST_32(GC0802_TIMER_ADDR, VENUS_DW_APB_TIMER_EOI(1));  // clear irq
    timer_interrupt_handler(1);
  } else if (cause & (1 << VENUS_IRQ_TIMER(2))) {
    READ_BURST_32(GC0802_TIMER_ADDR, VENUS_DW_APB_TIMER_EOI(2));  // clear irq
    timer_interrupt_handler(2);
  } else if (cause & (1 << VENUS_IRQ_TIMER(3))) {
    READ_BURST_32(GC0802_TIMER_ADDR, VENUS_DW_APB_TIMER_EOI(3));  // clear irq
    timer_interrupt_handler(3);
  } else if (cause & (1 << VENUS_IRQ_TIMER(4))) {
    READ_BURST_32(GC0802_TIMER_ADDR, VENUS_DW_APB_TIMER_EOI(4));  // clear irq
    timer_interrupt_handler(4);
  } else if (cause & (1 << VENUS_IRQ_DMA_ERR)) {
    dma_error_handler();
  } else if (cause & (1 << PICO_IRQ_TIMER)) {
    printf("Internal timer irq! $stop\n");
  } else if (cause & (1 << VENUS_IRQ_FLASH_SPI)) {
    int stat;
    stat = READ_BURST_32(GC0802_FLASH_SPI_ADDR, 0x30);  // get Interrupt Status
    printf("spi irq stat=%d\n",stat);
    READ_BURST_32(GC0802_FLASH_SPI_ADDR, 0x48);  // clear irq
    WRITE_BURST_32(GC0802_FLASH_SPI_ADDR, 0x2C, 0x0);  // disable Transmit FIFO Empty Interrupt Mask
    printf("fsi\n");
    REG_WRITE(0x1fff4000 + 0x0, 0x010); // gpio4 pluse
    REG_WRITE(0x1fff4000 + 0x0, 0x0);     // gpio4 pluse
  } else if (cause & (1 << VENUS_IRQ_RF_SPI)) {
    READ_BURST_32(GC0802_RF_SPI_ADDR, 0x48);  // clear irq
    WRITE_BURST_32(GC0802_RF_SPI_ADDR, 0x2C, 0x0);  // disable Received Data Available Interrupt
    printf("rsi\n");
    REG_WRITE(0x1fff4000 + 0x0, 0x020); // gpio5 pluse
    REG_WRITE(0x1fff4000 + 0x0, 0x0);     // gpio5 pluse
  } else if (cause & (1 << VENUS_IRQ_UART1)) {
    uart_irq_handler(GC0802_UART1_ADDR);
    WRITE_BURST_32(GC0802_UART1_ADDR, 0x04, 0x0);  // disable Received Data Available Interrupt
    printf("u1i\n");
    REG_WRITE(0x1fff4000 + 0x0, 0x040); // gpio6 pluse
    REG_WRITE(0x1fff4000 + 0x0, 0x0);     // gpio6 pluse
  } else if (cause & (1 << VENUS_IRQ_GPIO)) {
    int intergpiostat = READ_BURST_32(GC0802_GPIO0_ADDR, 0x40);  // Interrupt status of Port A
    if(intergpiostat == 0x01)
    {
      printf("p0i\n");
      WRITE_BURST_32(GC0802_GPIO0_ADDR, 0x0c, 0x1);  // clear irq
    }
    else
    {
      printf("unknown gpio irq\n");
      WRITE_BURST_32(GC0802_GPIO0_ADDR, 0x0c, intergpiostat);  // clear irq
    }
    WRITE_BURST_32(GC0802_GPIO0_ADDR, 0x30, 0x0); // gpio_inten Interrupt disable
    REG_WRITE(0x1fff4000 + 0x0, 0x080); // gpio7 pluse
    REG_WRITE(0x1fff4000 + 0x0, 0x0);     // gpio7 pluse
  } else {
    if ((cause & (1 << PICO_IRQ_BADINSTR)) || (cause & (1 << PICO_IRQ_MEMERROR))) {
      uint32_t pc    = (regs[0] & 1) ? regs[0] - 3 : regs[0] - 4;
      uint32_t instr = *(uint32_t*)pc;
      if (cause & (1 << PICO_IRQ_BADINSTR)) {
        if (instr == 0x00100073 || instr == 0x9002) {
          printf("[SCHEDULER] EBREAK instruction at %p $stop\n", pc);
        } else {
          printf("[SCHEDULER] Illegal instruction at %p $stop\n", pc);
        }
      }
      if (cause & (1 << PICO_IRQ_MEMERROR)) {
        printf("[SCHEDULER] Bus error in Instruction at %p $stop\n", pc);
      }
    }
  }
  return regs;
}
