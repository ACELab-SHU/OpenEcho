#include "types.h"

/*
 * 在通过写UART将软件的printf打印到DVE CONSOLE时
 * UART将首先对输入的字符进行记录，
 * 在遇到NUL(0x00) | EOT（0x04）| CR(0x0D) | LF(0x0A)后
 * 才会将先前收到的字符一起打印到CONSOLE上，
 * 并自动换行，无需额外在软件中换行
 * Ref: https://github.com/cccriscv/mini-riscv-os/blob/master/05-Preemptive/lib.c
 */
/* uart.c */
extern void uart_puts(char* s);
extern void uart_putc(char ch);
/* start.S */
extern void vcs_stop(void);
/* hal.S */
extern uint32_t Mask_irq(uint32_t mask_irq);

#ifndef NO_DIV
static int _vsnprintf(char* out, size_t n, const char* s, va_list vl) {
  int format  = 0;
  int longarg = 0;
  size_t pos  = 0;
  for (; *s; s++) {
    if (format) {
      switch (*s) {
        case 'l': {
          longarg = 2;
          if (!pos) {
            pos -= 2;
          }
          break;
        }
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
          if (longarg == 2) {
            long long lower_num  = va_arg(vl, long long);
            long long higher_num = va_arg(vl, long long);
            for (int i = hexdigits; i >= 0; i--) {
              int d = (higher_num >> (4 * i)) & 0xF;
              if (out && pos < n) {
                out[pos] = (d < 10 ? '0' + d : 'a' + d - 10);
              }
              pos++;
            }
            for (int i = hexdigits * 2 + 1; i > hexdigits; i--) {
              int d = (lower_num >> (4 * i)) & 0xF;
              if (out && pos < n) {
                out[pos] = (d < 10 ? '0' + d : 'a' + d - 10);
              }
              pos++;
            }
          } else {
            long num = va_arg(vl, long);
            for (int i = hexdigits; i >= 0; i--) {
              int d = (num >> (4 * i)) & 0xF;
              if (out && pos < n) {
                out[pos] = (d < 10 ? '0' + d : 'a' + d - 10);
              }
              pos++;
            }
          }
          longarg = 0;
          format  = 0;
          break;
        }
        case 'd': {
          long num = va_arg(vl, int);
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
          format  = 0;
          break;
        }
        case 's': {
          const char* s2 = va_arg(vl, const char*);
          while (*s2) {
            if (out && pos < n) {
              out[pos] = *s2;
            }
            pos++;
            s2++;
          }
          longarg = 0;
          format  = 0;
          break;
        }
        case 'c': {
          if (out && pos < n) {
            out[pos] = (char)va_arg(vl, int);
          }
          pos++;
          longarg = 0;
          format  = 0;
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
#else
static int _vsnprintf(char* out, size_t n, const char* s, va_list vl) {
  int format  = 0;
  int longarg = 0;
  size_t pos  = 0;
  for (; *s; s++) {
    if (format) {
      switch (*s) {
        case 'l': {
          longarg = 2;
          if (!pos) {
            pos -= 2;
          }
          break;
        }
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
          int hexdigits = sizeof(long) + sizeof(long) - 1;
          if (longarg == 2) {
            long long lower_num  = va_arg(vl, long long);
            long long higher_num = va_arg(vl, long long);
            for (int i = hexdigits; i >= 0; i--) {
              int d = (higher_num >> (4 * i)) & 0xF;
              if (out && pos < n) {
                out[pos] = (d < 10 ? '0' + d : 'a' + d - 10);
              }
              pos++;
            }
            for (int i = hexdigits + hexdigits + 1; i > hexdigits; i--) {
              int d = (lower_num >> (4 * i)) & 0xF;
              if (out && pos < n) {
                out[pos] = (d < 10 ? '0' + d : 'a' + d - 10);
              }
              pos++;
            }
          } else {
            long num = va_arg(vl, long);
            for (int i = hexdigits; i >= 0; i--) {
              int d = (num >> (4 * i)) & 0xF;
              if (out && pos < n) {
                out[pos] = (d < 10 ? '0' + d : 'a' + d - 10);
              }
              pos++;
            }
          }
          longarg = 0;
          format  = 0;
          break;
        }
        case 'd': {
          long num = va_arg(vl, int);
          if (num < 0) {
            num = -num;
            if (out && pos < n) {
              out[pos] = '-';
            }
            pos++;
          }
          long digits = 1;
          long nn     = num;
          // for (long nn = num; nn /= 10; digits++)
          //   ;
          while (nn >= 10) {
            nn -= 10;
            digits++;
          }
          for (int i = digits - 1; i >= 0; i--) {
            if (out && pos + i < n) {
              out[pos + i] = '0' + (num - nn);  // 使用减法
            }
            num = nn;
            nn  = 0;
          }
          pos += digits;
          longarg = 0;
          format  = 0;
          break;
        }
        case 's': {
          const char* s2 = va_arg(vl, const char*);
          while (*s2) {
            if (out && pos < n) {
              out[pos] = *s2;
            }
            pos++;
            s2++;
          }
          longarg = 0;
          format  = 0;
          break;
        }
        case 'c': {
          if (out && pos < n) {
            out[pos] = (char)va_arg(vl, int);
          }
          pos++;
          longarg = 0;
          format  = 0;
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
#endif

static char out_buf[1000];

static int _vprintf(const char* s, va_list vl) {
  int res = _vsnprintf(NULL, -1, s, vl);
  if (res + 1 >= sizeof(out_buf)) {
    uart_puts("error: output string size overflow\n");
    vcs_stop();
  }
  _vsnprintf(out_buf, res + 1, s, vl);
  uart_puts(out_buf);
  return res;
}

int sprintf(char* buf, const char* fmt, ...) {
  va_list args;
  va_start(args, fmt);
  int res = _vsnprintf(NULL, -1, fmt, args);
  if (res + 1 >= sizeof(out_buf)) {
    uart_puts("error: output string size overflow\n");
  }
  _vsnprintf(out_buf, res + 1, fmt, args);
  va_end(args);
  return res;
}

// support %p, %s, %d, %x only
int printf(const char* s, ...) {
  uint32_t mask_irq = Mask_irq(0xffffffff);  // disable all interrupts
  int res           = 0;
  va_list vl;
  va_start(vl, s);
  res = _vprintf(s, vl);
  va_end(vl);
  Mask_irq(mask_irq);  // enable all interrupts
  return res;
}

void print_memory(void* ptr, size_t size) {
  uint8_t* byte_ptr = (uint8_t*)ptr;
  for (size_t i = 0; i < size; i++) {
    for (int j = 7; j >= 0; j--) {
      uint8_t bit = ((*byte_ptr) >> j) & 0x01;
      uart_putc(bit ? '1' : '0');
    }
    uart_putc(10);
    byte_ptr++;
  }
}

void panic(const char* s, ...) {
  va_list vl;
  va_start(vl, s);
  _vprintf(s, vl);
  va_end(vl);
  // vcs_stop();
  while (1) {};
}