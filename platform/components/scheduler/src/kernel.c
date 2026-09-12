#include "custom_cfg.h"
#include "dma.h"
#include "main_init.h"
#include "venus.h"

extern void devctrl_init(void);
extern void venus_uart_init(void);
extern void irq_init(void);
extern void heap_init(void);
extern void dfe_init(void);
extern void dagproc_init(void);
extern void gc0802_init(void);
extern void main(void);

extern int printf(const char* s, ...);
extern void uart_init(void);
extern void vcs_stop();                   // stop vcs simulation
extern void Forloop_timer(int loop_cnt);  // software for loop timer
extern void Set_timer(int timer);         // PicoRv32 interrnal timer
extern volatile int MUTEX_gc_modify_timing_done;
extern uint32_t Mask_irq(uint32_t mask_irq);
extern uint32_t HEAP_START;
extern volatile unsigned int padding_bin[];

extern uint32_t BSS_START;
extern uint32_t BSS_END;
// extern unsigned int dag1_task_container[];

void change_dcache_end_addr(uint32_t addr) {
  unsigned int old_menvcfgh;
  __asm__ volatile(
    "csrrw x0, menvcfgh, %1"
    : "=r"(old_menvcfgh)
    : "r"(addr)
    : "memory");
}
void enable_idcache() {
  unsigned int old_menvcfg;
  __asm__ volatile(
      "csrrs %0, menvcfg, %1"
      : "=r"(old_menvcfg)
      : "r"((1 << 9)|(1 << 8))
      : "memory");
}
void send_irq_to_usbscheduler() {
  unsigned int old_menvcfg;
  __asm__ volatile(
      "csrrs %0, menvcfg, %1"
      : "=r"(old_menvcfg)
      : "r"((1 << 31))
      : "memory");
}
void clear_usbscheduler_irq() {
  unsigned int old_menvcfg;
  __asm__ volatile(
      "csrrs %0, menvcfg, %1"
      : "=r"(old_menvcfg)
      : "r"((1 << 30))
      : "memory");
}

// uint32_t* irq_handler(uint32_t* regs, uint32_t cause) {
//   while (1) {};
// }
extern void dma_transfer(uint32_t src, uint32_t dst, uint32_t len, uint32_t last);
extern dmafifo_t dmafifo;

extern int sum_rs_bits;
extern int signal_energy_rs;
extern int correlation_energy_rs;

bool qpi_mode = 0;
extern unsigned int _cacheram_end_addr;
void start_kernel(void) {
  change_dcache_end_addr(&_cacheram_end_addr);
  enable_idcache();
  // printf("hello world by sram\n\r");
  devctrl_init();

  REG_WRITE(0x1fff4000 + 0x4, 0xfffff); // all gpio output
  REG_WRITE(0x1fff4000 + 0x0, 0x0); // all gpio low

  REG_WRITE(0x1fff4000 + 0x0, 0x1); // gpio0 pluse
  REG_WRITE(0x1fff4000 + 0x0, 0x0); // gpio0 pluse

  venus_uart_init();
  venus_user_uart_init();
  irq_init();
  heap_init();

  dagproc_init();
  padding_bin[0] = padding_bin[0] + 1;
  // // // gc080x Driver
  // printf("before gc\n");
  // gc0802_init();
  // printf("after gc\n");
  // printf("bss start: %p, end: %p\n", BSS_START, BSS_END);
  // printf("start change rx lo");
  // trx_lo_change(&g_phy_obj[0], 0, 2230150000ULL);
  // printf("end change rx lo");
  // int reg880 = hal_spi_read_reg(&g_phy_obj[0], 0x880);
  // int reg880 = hal_spi_write_reg(&g_phy_obj[0], 0x888, 0x99);
  // printf("880 reg value is : %p\n", reg880);
  // dfe_init();
  // while (1) {
  //   // fft
  //   /*
  //  * | sync | peak | ram | gpiox |
  //  * |  1   |  1   |  0  |   0   |
  //  */
  //   Mask_irq(0xfc002000);  // 打开ram, 关闭sync, peak, ovf
  //   if (MUTEX_gc_modify_timing_done == 1) {
  //     break;
  //   }
  // }

  /*
   * | sync | peak | ram | gpiox |
   * |  1   |  1   |  1  |   0   |
   */
  // Mask_irq(0xfe000000);  // 打开ovf, 关闭sync, peak, ram
  // sleep(5);
  // printf("sum_rs_bits: %p\n", sum_rs_bits);
  // printf("signal_energy_rs: %p\n", signal_energy_rs);
  // printf("correlation_energy_rs: %p\n", correlation_energy_rs);
  // while (1) {};
  //
  // while (1) {
  //   if (MUTEX_gc_modify_timing_done == 1) {
  //     // 2.13015GHZ
  //     // trx_lo_change(&g_phy_obj[g_phy_select], 2130150000, 0);
  //     trx_lo_change(&g_phy_obj[g_phy_select], 0, 2132650000);
  //     printf("GC timing modify done, enable DFE modify...\n");
  //     break;
  //   }
  // }
  main();
}
