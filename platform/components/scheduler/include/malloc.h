#ifndef _MALLOC_H_
#define _MALLOC_H_

#include "memset.h"
#include "stddef.h"
#include "stdint.h"
#include "stdlib.h"

extern void* malloc(int size);  // input the bit size
extern void free(void* ptr);

#endif