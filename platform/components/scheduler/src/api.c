#include "common.h"
#include "fifo.h"
#include "venus.h"

__attribute__((optimize("O0"))) int check_data_ready(int data_num, ...) {
  va_list vl;
  va_start(vl, data_num);
  fifo_t* fifo;
  int ready = 1;
  for (int i = 0; i < data_num; i++) {
    fifo  = (fifo_t*)va_arg(vl, int);
    ready = ready && (!fifo_empty(fifo));
  }
  va_end(vl);
  return ready;
}

__attribute__((optimize("O0"))) void fire_dag_fence(void) {
  while (!MUTEX_dag_done) {}
}