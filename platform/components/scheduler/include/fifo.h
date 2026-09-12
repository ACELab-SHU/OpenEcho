#ifndef __FIFO_H__
#define __FIFO_H__

#include "dma.h"
#include "types.h"
#define MAXFIFO    32
#define MAXDMAFIFO 32

typedef struct fifo {
  int data[MAXFIFO];  // data descriptor's pointer
  uint8_t wptr;
  uint8_t rptr;
} fifo_t; /* fifo template */

/* fifo */
void init_fifo(fifo_t* F);
uint8_t fifo_full(fifo_t* F);
uint8_t fifo_empty(fifo_t* F);
uint8_t fifo_size(fifo_t* F);
void push_fifo(fifo_t* F, int ptr);
int pop_fifo(fifo_t* F);
int read_fifo(fifo_t* F);

typedef struct dmafifo {
  dmadsc_t dsc[MAXDMAFIFO];
  uint8_t wptr;
  uint8_t rptr;
} dmafifo_t; /* dmafifo template */

void dmainit_fifo(dmafifo_t* F);
uint8_t dmafifo_full(dmafifo_t* F);
uint8_t dmafifo_empty(dmafifo_t* F);
uint8_t dmafifo_size(dmafifo_t* F);
void dmapush_fifo(dmafifo_t* F, dmadsc_t dsc);
dmadsc_t dmapop_fifo(dmafifo_t* F);

#endif /* __FIFO_H__ */
