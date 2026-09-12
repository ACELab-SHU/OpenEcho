#include "stdlib.h"

int abs(int __n) {
  return (__n < 0) ? -__n : __n;
}

long long llabs(long long int __n) {
  return (__n < 0) ? -__n : __n;
}

int atoi(const char* str) {
  int c;
  unsigned x = 0;
  if (!*str)
    return -1;
  while ((c = *str++)) {
    if ('0' <= c && c <= '9') {
      x *= 10;
      x += c - '0';
    } else {
      return -1;
    }
  }
  return x;
}

unsigned long long int strtoull(const char* ptr, char** end, int base) {
  unsigned long long ret = 0;

  if (base > 36)
    goto out;

  while (*ptr) {
    int digit;

    if (*ptr >= '0' && *ptr <= '9' && *ptr < '0' + base)
      digit = *ptr - '0';
    else if (*ptr >= 'A' && *ptr < 'A' + base - 10)
      digit = *ptr - 'A' + 10;
    else if (*ptr >= 'a' && *ptr < 'a' + base - 10)
      digit = *ptr - 'a' + 10;
    else
      break;

    ret *= base;
    ret += digit;
    ptr++;
  }

out:
  if (end)
    *end = (char*)ptr;

  return ret;
}

long int strtol(const char* nptr, char** endptr, int base) {
  long int val  = 0;
  const char* p = nptr;
  bool is_neg   = FALSE;
  int c;

  /* Optional whitespace prefix. */
  while (isspace(*p))
    p++;
  c = tolower(*p);

  /* Optional sign prefix: +, -. */
  if ((c == '+') || (c == '-')) {
    is_neg = (c == '-');
    c      = tolower(*++p);
  }

  /* Optional base prefix: 0, 0x. */
  if (c == '0') {
    if (base == 0)
      base = 8;
    c = tolower(*++p);
    if (c == 'x') {
      if (base == 0)
        base = 16;
      if (base != 16)
        goto out;
      c = tolower(*++p);
    }
  }

  if (base == 0)
    base = 10;

  /* Digits. */
  for (;;) {
    /* Convert c to a digit [0123456789abcdefghijklmnopqrstuvwxyz]. */
    if ((c >= '0') && (c <= '9'))
      c -= '0';
    else if ((c >= 'a') && (c <= 'z'))
      c -= 'a' - 10;
    else
      break;
    if (c >= base)
      break;
    val = (val * base) + c;
    c   = tolower(*++p);
  }

out:
  if (endptr)
    *endptr = (char*)p;
  return is_neg ? -val : val;
}

int tolower(int c) {
  if ((c >= 'A') && (c <= 'Z'))
    c += 'a' - 'A';
  return c;
}

int isspace(int c) {
  return (c == ' ') || (c == '\t') || (c == '\n') || (c == '\r') || (c == '\f') || (c == '\v');
}

unsigned long strtoul(const char* nptr, char** endptr, int base) {
  const char* s;
  unsigned long acc;
  char c;
  unsigned long cutoff;
  int neg, any, cutlim;

  /*
	 * See strtol for comments as to the logic used.
	 */
  s = nptr;
  do {
    c = *s++;
  } while (isspace((unsigned char)c));
  if (c == '-') {
    neg = 1;
    c   = *s++;
  } else {
    neg = 0;
    if (c == '+')
      c = *s++;
  }
  if ((base == 0 || base == 16) &&
      c == '0' && (*s == 'x' || *s == 'X') &&
      ((s[1] >= '0' && s[1] <= '9') ||
       (s[1] >= 'A' && s[1] <= 'F') ||
       (s[1] >= 'a' && s[1] <= 'f'))) {
    c = s[1];
    s += 2;
    base = 16;
  }
  if (base == 0)
    base = c == '0' ? 8 : 10;
  acc = any = 0;

  cutoff = ULONG_MAX / base;
  cutlim = ULONG_MAX % base;
  for (;; c = *s++) {
    if (c >= '0' && c <= '9')
      c -= '0';
    else if (c >= 'A' && c <= 'Z')
      c -= 'A' - 10;
    else if (c >= 'a' && c <= 'z')
      c -= 'a' - 10;
    else
      break;
    if (c >= base)
      break;
    if (any < 0 || acc > cutoff || (acc == cutoff && c > cutlim))
      any = -1;
    else {
      any = 1;
      acc *= base;
      acc += c;
    }
  }
  if (any < 0) {
    acc = ULONG_MAX;
  } else if (neg)
    acc = -acc;
  if (endptr != NULL)
    *endptr = (char*)(any ? s - 1 : nptr);
  return (acc);
}

int strcmp(const char* s1, const char* s2) {
  while (*s1 == *s2++)
    if (*s1++ == '\0')
      return (0);
  return (*(const unsigned char*)s1 - *(const unsigned char*)(s2 - 1));
}

size_t strlen(const char* str) {
  size_t len;

  for (len = 0; str[len]; len++)
    ;
  return len;
}

void* memcpy(void* dest, const void* src, long n) {
  char* a       = dest;
  const char* b = src;
  while (n--)
    *(a++) = *(b++);
  return dest;
}