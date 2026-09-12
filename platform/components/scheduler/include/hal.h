#ifndef __HAL_H__
#define __HAL_H__

#include "types.h"

#define PICO_IRQ_TIMER    0
#define PICO_IRQ_BADINSTR 1
#define PICO_IRQ_MEMERROR 2

typedef void (*VoidFunc)(void);

/* irq.c */
void set_handler(uint32_t irq, VoidFunc callback);
// void enable_irq(uint32_t irq);
// void disable_irq(uint32_t irq);

/* hal.S */
uint32_t EN_Interrupts(uint32_t mask);
uint32_t DIS_Interrupts(uint32_t mask);
// void EN_All_Cluster_Interrupts(void);

#endif /* __HAL_H__ */
