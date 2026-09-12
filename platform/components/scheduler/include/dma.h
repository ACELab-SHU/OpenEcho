#ifndef __DMA_H__
#define __DMA_H__

#include "types.h"

typedef struct dmadsc {
  uint32_t ptr;  // 所传输数据的指针
  uint32_t atr;  // 所传输数据的描述
} dmadsc_t;

#endif /* __DMA_H__ */