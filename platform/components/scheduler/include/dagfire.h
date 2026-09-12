#ifndef __DAGFIRE_H_
#define __DAGFIRE_H_

#include "dagproc.h"
#include "types.h"

extern void dma_transfer(uint32_t src, uint32_t dst, uint32_t len, uint32_t last);
extern dagproc_t* register_dag(uint32_t* dagbin_addr, uint32_t* daginput_offset, uint32_t* daginput_length, uint32_t binandjsonsize, int inputnum, int outputnum, ...);
/* defined in dagproc.c */
extern dagproc_t* p;
extern int dma_error_hpn;

#define fire_dag(DAGname, DAGInputNum, DAGOutputNum, ...)                                                                                          \
  dma_error_hpn = 0;                                                                                                                               \
  p = register_dag(&CONCAT(DAGname, bin), CONCAT(DAGname, input_offset), CONCAT(DAGname, input_length), CONCAT(DAGname, binandjsonsize), DAGInputNum, DAGOutputNum, ##__VA_ARGS__); \
  dma_transfer((uint32_t)(&CONCAT(DAGname, bin)), p->config.prog, (uint32_t)(CONCAT(DAGname, bin_size)), 0);                                       \
  dma_transfer((uint32_t)(&CONCAT(DAGname, task_container)), p->config.datacontainer, (uint32_t)(CONCAT(DAGname, task_container_size)), 0);        \
  dma_transfer((uint32_t)(&CONCAT(DAGname, global_para)), p->config.globalparacontainer, (uint32_t)(CONCAT(DAGname, global_para_size)), 0);        \
  dma_transfer((uint32_t)(&CONCAT(DAGname, output_num)), p->config.outputnumreg, (uint32_t)(CONCAT(DAGname, output_num_size)), 0);                 \
  dma_transfer((uint32_t)(&CONCAT(DAGname, task_num)), p->config.tasknumreg, (uint32_t)(CONCAT(DAGname, task_num_size)), 0);                       \
  for(uint32_t __output_addr_size_terans_index__ = 0; __output_addr_size_terans_index__ < (uint32_t)(CONCAT(DAGname, output_addr_size)); __output_addr_size_terans_index__ = __output_addr_size_terans_index__ + 0x40) { \
    uint32_t remaining_output_addr_size = ((uint32_t)(CONCAT(DAGname, output_addr_size)) - __output_addr_size_terans_index__); \
    uint32_t this_turn_output_addr_tsize; \
    if(remaining_output_addr_size > 0x40) \
      this_turn_output_addr_tsize = 0x40; \
    else \
      this_turn_output_addr_tsize = remaining_output_addr_size; \
    dma_transfer(((uint32_t)(&CONCAT(DAGname, output_addr))+__output_addr_size_terans_index__), ((uint32_t)p->config.outputaddr+__output_addr_size_terans_index__), this_turn_output_addr_tsize, 0); \
  } \
  transfer_dag_inputs(p, &CONCAT(DAGname, bin), CONCAT(DAGname, input_offset), CONCAT(DAGname, input_length), CONCAT(DAGname, binandjsonsize), DAGInputNum, DAGOutputNum, ##__VA_ARGS__); \
  dma_transfer((uint32_t)(&CONCAT(DAGname, return_value)), p->config.returnvalue, (uint32_t)(CONCAT(DAGname, return_value_size)), 1);              \
  if(dma_error_hpn) {                                                                                                                              \
    p->state = IDLE;                                                                                                                               \
    p->cluster->payload--;                                                                                                                         \
  }                                                                                                                                                \
  dma_error_hpn = 0;

// for test 1120
// #define fire_dag_1120_withoutbin(DAGname, DAGInputNum, DAGOutputNum, ...)                                                                          \
//   p = register_dag(&CONCAT(DAGname, bin), CONCAT(DAGname, input_offset), CONCAT(DAGname, input_length), DAGInputNum, DAGOutputNum, ##__VA_ARGS__); \
//   dma_transfer((uint32_t)(&CONCAT(DAGname, task_container)), p->config.datacontainer, (uint32_t)(CONCAT(DAGname, task_container_size)), 0);        \
//   dma_transfer((uint32_t)(&CONCAT(DAGname, global_para)), p->config.globalparacontainer, (uint32_t)(CONCAT(DAGname, global_para_size)), 0);        \
//   dma_transfer((uint32_t)(&CONCAT(DAGname, output_num)), p->config.outputnumreg, (uint32_t)(CONCAT(DAGname, output_num_size)), 0);                 \
//   dma_transfer((uint32_t)(&CONCAT(DAGname, task_num)), p->config.tasknumreg, (uint32_t)(CONCAT(DAGname, task_num_size)), 0);                       \
//   dma_transfer((uint32_t)(&CONCAT(DAGname, return_value)), p->config.returnvalue, (uint32_t)(CONCAT(DAGname, return_value_size)), 1);

#endif /* __DAGFIRE_H_ */