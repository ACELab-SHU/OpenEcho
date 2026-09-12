#ifndef __TYPES_H__
#define __TYPES_H__

/* <stdint.h> */
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef unsigned long long uint64_t;

/* <stddef.h> */
#define NULL ((void*)0)
typedef unsigned int size_t;

/* <stdarg.h> */
typedef __builtin_va_list va_list;
#define va_start(ap, param) __builtin_va_start(ap, param)
#define va_arg(ap, type)    __builtin_va_arg(ap, type)
#define va_end(ap)          __builtin_va_end(ap)

/* <stdbool.h> */
#define true 1
#define false 0

// /* RISCV32: register is 32bits width */
// typedef uint32_t reg_t;

#endif /* __TYPES_H__ */
