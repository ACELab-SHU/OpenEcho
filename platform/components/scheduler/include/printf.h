#ifndef __PRINTF_H__
#define __PRINTF_H__

#include "config.h"
#include "stdarg.h"
#include "stddef.h"
#include "stdint.h"

int printf(const char* fmt, ...);
int sprintf(char* buf, const char* fmt, ...);
static int _vprintf(const char* fmt, va_list vl);
static int _vsnprintf(char* out, size_t n, const char* s, va_list vl);

#endif
