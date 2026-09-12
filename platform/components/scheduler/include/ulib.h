#ifndef __ULIB_H_
#define __ULIB_H_

#include "common.h"

#define READ_BURST_64(base, offset)         (*(volatile uint64_t*)((base) + (offset)))
#define WRITE_BURST_64(base, offset, value) (*(volatile uint64_t*)((base) + (offset)) = (value))
#define READ_BURST_32(base, offset)         (*(volatile uint32_t*)((base) + (offset)))
#define WRITE_BURST_32(base, offset, value) (*(volatile uint32_t*)((base) + (offset)) = (value))
#define READ_BURST_16(base, offset)         (*(volatile uint16_t*)((base) + (offset)))
#define WRITE_BURST_16(base, offset, value) (*(volatile uint16_t*)((base) + (offset)) = (value))

void* memset(void*, int, uint32_t);
void delay(volatile int count);

#endif /* __ULIB_H_ */
