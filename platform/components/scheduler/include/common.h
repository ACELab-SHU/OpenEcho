#ifndef __COMMON_H__
#define __COMMON_H__

#include "assert.h"
#include "types.h"
#include "ulib.h"

#define DELAY 500

/* printf */
extern void uart_putc(char ch);
extern void uart_puts(char* s);
extern void uart_fence();
extern int printf(const char* s, ...);             // only understands %d, %x, %p, %s
extern int panic(const char* s, ...);              // only understands %d, %x, %p, %s
extern void print_memory(void* ptr, size_t size);  // print out memory data in bit

/* start.S */
extern void vcs_stop(void);  // assemble codes for stop vcs simulation

/* heap memory management */
extern void* malloc(int size);  // input the bit size
extern void free(void* ptr);

/* string tools */
extern void delay(volatile int count);

/* Tools: Round to multiple of n */
static inline uint32_t _align_up(uint32_t x, uint32_t n) { return n * ((x + n - 1) / n); }  // 将x向上对齐到最接近n的倍数
static inline uint32_t _align_down(uint32_t x, uint32_t n) { return n * (x / n); }          // 将x向下对齐到最接近n的倍数

extern volatile int wdt_intr_cnt;
extern volatile int timer0_intr_cnt;
extern volatile int timer1_intr_cnt;
extern volatile int timer2_intr_cnt;
extern volatile int timer3_intr_cnt;
extern volatile int timer4_intr_cnt;
#endif /* __COMMON_H__ */
