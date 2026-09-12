#ifndef __RISCV_PRINTF_H__
#define __RISCV_PRINTF_H__

#include "venus.h"

#define BARRIER()       __asm__("addi x0,x0,0");__asm__("addi x0,x0,0");__asm__("addi x0,x0,0");__asm__("addi x0,x0,0");__asm__("addi x0,x0,0");__asm__("addi x0,x0,0");__asm__("addi x0,x0,0");__asm__("addi x0,x0,0");__asm__("addi x0,x0,0");__asm__("addi x0,x0,0"); asm volatile ("vns_barrier")
#define VSPM_OPEN()     __asm__("addi x0,x0,0");__asm__("addi x0,x0,0");__asm__("addi x0,x0,0");__asm__("addi x0,x0,0");__asm__("addi x0,x0,0");__asm__("addi x0,x0,0");__asm__("addi x0,x0,0");__asm__("addi x0,x0,0");__asm__("addi x0,x0,0");__asm__("addi x0,x0,0");*(volatile unsigned int*)(0x801ff004) = 0x00000001
#define VSPM_CLOSE()    *(volatile unsigned int*)(0x801ff004) = 0x00000000
#define EBREAK()        asm volatile ("ebreak")

#define VENUS_PRINTVEC_SHORT(name, len)                                        \
  do {                                                                         \
    printf(STR(name) "\n", NULL);                                              \
    short array_##name[len];                                                   \
    int vecaddr_##name = vaddr(name);                                          \
    VSPM_OPEN();                                                               \
    BARRIER();                                                                 \
    for (int i = 0; i < len; i++) {                                            \
      array_##name[i] =                                                        \
          *(volatile unsigned short *)(vecaddr_##name + (i << 1));             \
      printf("%hd\n", &array_##name[i]);                                       \
    }                                                                          \
    VSPM_CLOSE();                                                              \
    printf("________________________\n", NULL);                                \
  } while (0)

#define VENUS_PRINTVEC_CHAR(name, len)                                         \
  do {                                                                         \
    printf(STR(name) "\n", NULL);                                              \
    char array_##name[len];                                                    \
    int vecaddr_##name = vaddr(name);                                          \
    VSPM_OPEN();                                                               \
    BARRIER();                                                                 \
    for (int i = 0; i < len; i++) {                                            \
      array_##name[i] = *(volatile unsigned char *)(vecaddr_##name + i);       \
      printf("%bd\n", &array_##name[i]);                                       \
    }                                                                          \
    VSPM_CLOSE();                                                              \
    printf("________________________\n", NULL);                                \
  } while (0)

/* <stdint.h> */
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef unsigned long long uint64_t;

/* <stddef.h> */
#define NULL ((void *)0)
typedef unsigned int size_t;

/* <stdbool.h> */
#define true 1
#define false 0

/* RISCV32: register is 32bits width */
typedef uint32_t reg_t;



#define BLOCK_CTRLREGS 0x801ff000UL
#define TILE_SOFTRESET_OFFSET        0x00
#define TILE_VRFDIRECTION_OFFSET     0x04
#define TILE_INTSTATUS_OFFSET        0x08
#define TILE_INTCLEAR_OFFSET         0x0C
#define TILE_PROGRESS_OFFSET         0x10
#define TILE_WATCHDOGCOUNTER_OFFSET  0x14
#define TILE_BINARYLANENUM_OFFSET    0x18
#define TILE_CURRENTDAGID_OFFSET     0x20
#define TILE_CURRENTTASKID_OFFSET    0x24
#define TILE_CURRENTTASKCRC_OFFSET   0x28
#define BLOCK_DEBUGUART 0x801fff00UL
#define BLOCK_DEBUG_UART_OFFSET 0x1FFF00
#define UART_FIFO_DEPTH 60
#define UART_RBR_OFFSET       0x00  // Receive Buffer Register - 存放接收到的数据字节，可通过该寄存器读取接收到的数据
#define UART_THR_OFFSET       0x00  // Transmitter Holding Register - 存放要发送的数据字节，可通过该寄存器写入数据进行发送
#define UART_DLL_OFFSET       0x00  // Divisor Latch Low Register - 低位分频器寄存器
#define UART_IER_OFFSET       0x04  // Divisor Latch High Register - 与DLL一起用于设置串口的通信波特率，可通过这两个寄存器设置波特率的分频系数
#define UART_DLH_OFFSET       0x04  // Interrupt Enable Register - 用于控制串口中断的产生和屏蔽
#define UART_IIR_OFFSET       0x08  // Interrupt Identification Register - 用于识别串口中断的类型和原因
#define UART_FCR_OFFSET       0x08  // FIFO Control Register - 用于控制串口的FIFO缓冲区(使能/禁用FIFO、清空FIFO、设置FIFO深度)
#define UART_LCR_OFFSET       0x0c  // Line Control Register - 用于设置数据位数、校验位、停止串口位等通信参数
#define UART_MCR_OFFSET       0x10  // Mode Control Register - 用于控制串口通信的一些额外功能，例如RTS、DTR、Loopback等
#define UART_LSR_OFFSET       0x14  // Line Status Register - 用于反应串口通信状态，比如数据是否准备好、是否接受数据以及是否完成等
#define UART_MSR_OFFSET       0x18  // Mode Status Register - 用于反应串口通信的一些额外状态，例如CTS、DSR、RI等
#define UART_SCR_OFFSET       0x1c  // Scratchpad Register - 用于用户自定义数据的透传，通常不用
#define UART_LPDLL_OFFSET     0x20
#define UART_LPDLH_OFFSET     0x24  // Low-power Divisor Latch Low/High Register - 用于在低功耗模式下设置串口通信的波特率分频系数
#define UART_SRBR_LOW_OFFSET  0x30  // Serial Receive Buffer Register (Low)
#define UART_SRBR_HIGH_OFFSET 0x6c  // Serial Receive Buffer Register (High)
#define UART_STHR_LOW_OFFSET  0x30  // Serial Transmit Holding Register (Low)
#define UART_STHR_HIGH_OFFSET 0x6c  // Serial Transmit Holding Register (High)
#define UART_FAR_OFFSET       0x70  // FIFO Access Register
#define UART_TFR_OFFSET       0x74  // Transmit FIFO Reset
#define UART_RFW_OFFSET       0x78  // Receiver FIFO Reset
#define UART_USR_OFFSET       0x7c  // UART Status Register
#define UART_TFL_OFFSET       0x80  // Transmit FIFO Level
#define UART_RFL_OFFSET       0x84  // Receive FIFO Level
#define UART_SRR_OFFSET       0x88  // Software Reset Register
#define UART_SRTS_OFFSET      0x8c  // Shadow Request to Send
#define UART_SBCR_OFFSET      0x90  // Shadow Break Control Register
#define UART_SDMAM_OFFSET     0x94  // Shadow DMA Mode
#define UART_SFE_OFFSET       0x98  // Shadow FIFO Enable
#define UART_SRT_OFFSET       0x9c  // Shadow RCVR Trigger
#define UART_STET_OFFSET      0xa0  // Shadow TX Empty Trigger
#define UART_HTX_OFFSET       0xa4  // Halt Transmission
#define UART_DMASA_OFFSET     0xa8  // DMA Software Acknowledge



/* block physical interface */
#define READ_BURST_64(base, offset) (*(volatile uint64_t *)((base) + (offset)))
#define WRITE_BURST_64(base, offset, value)                                    \
  (*(volatile uint64_t *)((base) + (offset)) = (value))
#define READ_BURST_32(base, offset) (*(volatile uint32_t *)((base) + (offset)))
#define WRITE_BURST_32(base, offset, value)                                    \
  (*(volatile uint32_t *)((base) + (offset)) = (value))
#define READ_BURST_16(base, offset) (*(volatile uint16_t *)((base) + (offset)))
#define WRITE_BURST_16(base, offset, value)                                    \
  (*(volatile uint16_t *)((base) + (offset)) = (value))

/* Function: Transmit a byte */
VENUS_INLINE void uart_putc(char ch) {
  while (READ_BURST_32(BLOCK_DEBUGUART, UART_TFL_OFFSET) > UART_FIFO_DEPTH)
    ;
  WRITE_BURST_32(BLOCK_DEBUGUART, UART_THR_OFFSET, (uint32_t)ch);
  // *((volatile int *)BLOCK_DEBUG_UART_OFFSET) = ch;
}

/* Function: Transmit a string */
VENUS_INLINE void uart_puts(char *s) {
  while (*s) {
    uart_putc(*s++);
  }
}

VENUS_INLINE int _vsnprintf(char *out, size_t n, const char *s, void *input) {
  int format = 0;
  int longarg = 0;
  int bytes = 4;
  size_t pos = 0;
  for (; *s; s++) {
    if (format) {
      switch (*s) {
      case 'p': {
        longarg = 1;
        if (out && pos < n) {
          out[pos] = '0';
        }
        pos++;
        if (out && pos < n) {
          out[pos] = 'x';
        }
        pos++;
      }
      case 'x': {
        int hexdigits = 2 * sizeof(long) - 1;
        long num;
        switch (bytes) {
        case 1:
          num = *((char *)input);
          break;
        case 2:
          num = *((short *)input);
          break;
        case 4:
          num = *((int *)input);
          break;
        }
        for (int i = hexdigits; i >= 0; i--) {
          int d = (num >> (4 * i)) & 0xF;
          if (out && pos < n) {
            out[pos] = (d < 10 ? '0' + d : 'a' + d - 10);
          }
          pos++;
        }
        longarg = 0;
        format = 0;
        break;
      }
      case 'b': {
        bytes = 1;
        break;
      }
      case 'h': {
        bytes = 2;
        break;
      }
      case 'd': {
        long num;
        switch (bytes) {
        case 1:
          num = *((char *)input);
          break;
        case 2:
          num = *((short *)input);
          break;
        case 4:
          num = *((int *)input);
          break;
        }
        if (num < 0) {
          num = -num;
          if (out && pos < n) {
            out[pos] = '-';
          }
          pos++;
        }
        long digits = 1;
        for (long nn = num; nn /= 10; digits++)
          ;
        for (int i = digits - 1; i >= 0; i--) {
          if (out && pos + i < n) {
            out[pos + i] = '0' + (num % 10);
          }
          num /= 10;
        }
        pos += digits;
        longarg = 0;
        format = 0;
        break;
      }
      case 's': {
        const char *s2 = (const char *)input;
        while (*s2) {
          if (out && pos < n) {
            out[pos] = *s2;
          }
          pos++;
          s2++;
        }
        longarg = 0;
        format = 0;
        break;
      }
      case 'c': {
        if (out && pos < n) {
          out[pos] = *((char *)input);
        }
        pos++;
        longarg = 0;
        format = 0;
        break;
      }
      default:
        break;
      }
    } else if (*s == '%') {
      format = 1;
    } else {
      if (out && pos < n) {
        out[pos] = *s;
      }
      pos++;
    }
  }
  if (out && pos < n) {
    out[pos] = 0;
  } else if (out && n) {
    out[n - 1] = 0;
  }
  return pos;
}

static char out_buf[100] = {0};

VENUS_INLINE int _vprintf(const char *s, void *input) {
  int res = _vsnprintf(NULL, -1, s, input);
  if (res + 1 >= sizeof(out_buf)) {
    uart_puts("error: output string size overflow\n");
    while (1) {
    }
  }
  _vsnprintf(out_buf, res + 1, s, input);
  uart_puts(out_buf);
  return res;
}

// use addresses (& or *) as the input
// support %p, %s, %d, %x only
// for short integer: use %hd
// for char integer: use %bd
VENUS_INLINE int printf(const char *s, void *input) {
  int res = 0;
  res = _vprintf(s, input);
  return res;
}

#endif