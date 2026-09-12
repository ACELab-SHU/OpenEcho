#ifndef _VMATH_H_
#define _VMATH_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "venus.h"

#define LN10     2.3025850929940456840179914546844
#define LN2      0.69314718055994530941723212145818
#define ACCURACY 0.0000000001

VENUS_INLINE uint32_t min(uint32_t num1, uint32_t num2) { return num1 < num2 ? num1 : num2; }

VENUS_INLINE uint32_t max(uint32_t num1, uint32_t num2) { return num1 > num2 ? num1 : num2; }

VENUS_INLINE double pow(double num, double power) {
  if (power == 0)
    return 1.0;

  double tmp = num;
  for (int i = 1; i < power; i++)
    tmp *= num;

  return tmp;
}

VENUS_INLINE double sqrt(double n) {
  double low, high, mid, tmp;

  if (n > 1) {
    low  = 1;
    high = n;
  } else {
    low  = n;
    high = 1;
  }

  while (low <= high) {
    mid = (low + high) / 2.000;

    tmp = mid * mid;

    if (tmp - n <= ACCURACY && tmp - n >= ACCURACY * -1)
      return mid;
    else if (tmp > n)
      high = mid;
    else
      low = mid;
  }

  return -1.000;
}

VENUS_INLINE double NegativeLog(double q) {
  int    p;
  double pi2  = 6.283185307179586476925286766559;
  double eps2 = 0.00000000000001; // 1e-14
  double eps1;                    // 1e-28
  double r = q, s = q, n = q, q2 = q * q, q1 = q2 * q;

  eps1 = eps2 * eps2;

  for (p = 1; (n *= q1) > eps1; s += n, q1 *= q2)
    r += (p = !p) ? n : -n;

  double u = 1 - 2 * r, v = 1 + 2 * s, t = u / v;
  double a = 1, b = sqrt(1 - t * t * t * t);

  for (; a - b > eps2; b = sqrt(a * b), a = t)
    t = (a + b) / 2;

  return pi2 / (a + b) / v / v;
}

VENUS_INLINE double log(double x) {
  int k = 0;

  if (x <= 0)
    return -1;
  if (x == 1)
    return 0;

  for (; x > 0.1; k++)
    x /= 10;
  for (; x <= 0.01; k--)
    x *= 10;

  return k * LN10 - NegativeLog(x);
}

VENUS_INLINE double log10(double x) { return log(x) / LN10; }

VENUS_INLINE double log2(double x) { return log(x) / LN2; }

VENUS_INLINE int ceil_log2(double x) {
  for (int i = 0; i < 32; ++i) {
    if ((double)(1 << i) >= (x))
      return i;
  }
  return -1;
}

VENUS_INLINE int floor_log2(double x) {
  for (int i = 0; i < 32; ++i) {
    if ((double)(1 << (i + 1)) > (x))
      return i;
  }
  return -1;
}

VENUS_INLINE double floor(double num) {
  int inum = (int)num;
  if (num < 0 && num != inum) {
    return inum - 1;
  }
  return inum;
}

VENUS_INLINE double ceil(double num) {
  int inum = (int)num;
  if (num > 0 && num != inum) {
    return inum + 1;
  }
  return inum;
}

VENUS_INLINE double round(double num) {
  if (num >= 0) {
    return (int)(num + 0.5);
  } else {
    return (int)(num - 0.5);
  }
}

#ifdef __cplusplus
}
#endif

#endif
