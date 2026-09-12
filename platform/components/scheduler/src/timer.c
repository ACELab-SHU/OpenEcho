#include "common.h"

extern long long freq_offset[];
static int freq_offset_id = 0;
#define MAX_FREQ_OFFSET_ID 10
#ifdef ENABLE_DRIVER
#include "custom_cfg.h"
#include "main_init.h"
void timer_interrupt_handler(int timer_id) {
  // trx_lo_change(rf_chip_phy_t *phy, unsigned long long  txlo, unsigned long long  rxlo)
  trx_lo_change(&g_phy_obj[g_phy_select], 0, (unsigned long long)freq_offset[freq_offset_id]);
  freq_offset_id++;
  if (freq_offset_id == MAX_FREQ_OFFSET_ID) {
    printf("No more freq! $stop\n");
  }
}
#else
void timer_interrupt_handler(int timer_id) {
  printf("T%di\n", timer_id);
}
#endif
