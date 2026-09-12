#include "fifo.h"
#include "common.h"
/* Reset fifo */
void init_fifo(fifo_t* F) { F->wptr = F->rptr = 0; }

/* Predictate */
uint8_t fifo_full(fifo_t* F) { return (fifo_size(F) == MAXFIFO - 1); }
uint8_t fifo_empty(fifo_t* F) { return (F->wptr == F->rptr); }
/* Get length: maximum length is 255 */
uint8_t fifo_size(fifo_t* F) {
  uint8_t size = F->wptr - F->rptr;
  // printf("wptr: %d, rptr: %d\n", F->wptr, F->rptr);
  if (size < 0) {
    size += MAXFIFO;
  }
  return size;
}
void push_fifo(fifo_t* F, int data) {
  if (fifo_full(F)) {
    panic("FIFO is full!\n");
  } else {
    F->data[F->wptr] = data;
    F->wptr          = (F->wptr + 1) % MAXFIFO;
  }
}
int pop_fifo(fifo_t* F) {
  if (fifo_empty(F)) {
    panic("FIFO is empty!\n");
    return -1;
  } else {
    int r   = F->data[F->rptr];
    F->rptr = (F->rptr + 1) % MAXFIFO;
    return r;
  }
}
int read_fifo(fifo_t* F) {
  if (fifo_empty(F)) {
    panic("FIFO is empty!\n");
    return -1;
  } else {
    int r = F->data[F->rptr];
    // F->rptr = (F->rptr + 1) % MAXFIFO;
    return r;
  }
}

void dmainit_fifo(dmafifo_t* F) { F->wptr = F->rptr = 0; }
uint8_t dmafifo_full(dmafifo_t* F) { return (dmafifo_size(F) == MAXDMAFIFO - 1); }
uint8_t dmafifo_empty(dmafifo_t* F) { return (F->wptr == F->rptr); }
uint8_t dmafifo_size(dmafifo_t* F) {
  uint8_t size = F->wptr - F->rptr;
  if (size < 0) {
    size += MAXDMAFIFO;
  }
  return size;
}

void dmapush_fifo(dmafifo_t* F, dmadsc_t dsc) {
  if (dmafifo_full(F)) {
    panic("SCHEDULER: dma FIFO is full at data\n");
  } else {
    F->dsc[F->wptr] = dsc;
    F->wptr         = (F->wptr + 1) % MAXDMAFIFO;
  }
}

dmadsc_t dmapop_fifo(dmafifo_t* F) {
  if (dmafifo_empty(F)) {
    panic("SCHEDULER: dma FIFO is empty!\n");
    dmadsc_t r = F->dsc[F->rptr];
    return r;
  } else {
    dmadsc_t r = F->dsc[F->rptr];
    F->rptr    = (F->rptr + 1) % MAXDMAFIFO;
    return r;
  }
}
