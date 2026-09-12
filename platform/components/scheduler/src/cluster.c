#include "common.h"
#include "dagproc.h"
#include "dma.h"
#include "fifo.h"
#include "rfdata.h"
#include "venusmmap.h"

extern cluster_t clusters[NCLUSTER];
extern dmafifo_t dmafifo;
extern void dma_transfer(uint32_t src, uint32_t dst, uint32_t len, uint32_t last);
extern volatile int return_tmp_addr[];

void cluster_interrupt_handler(uint32_t cluster_intstatusreg, uint32_t cluster_id) {

  // clear cluster's interrupt
  WRITE_BURST_32(VENUS_CLUSTER_CFG(cluster_id), VENUS_CLUSTER_INTCLEARREG_OFFSET, 1);
  // printf("Cluster compute done!\n");
  // DAG0 irq...
  if (cluster_intstatusreg & (1 << VENUS_CLUSTER_DAG_IRQ(0))) {
    // 1. stop L2 scheduler
    WRITE_BURST_32(VENUS_CLUSTER_L2_CFG(cluster_id), VENUS_CLUSTER_L2_RESET_REG_ADDR, 0);
    cluster_t* c       = &clusters[cluster_id];
    dagproc_t* p       = &(c->dagproc[0]);  // 目前cluster上只有一个dag，所以是dagproc[0]
    p->state           = C2D;
    int outputnum      = p->outputnum;
    int cluster_offset = p->config.prog;
    // printf("Cluster DONE! $stop\n");
    for (int i = 0; i < outputnum; i++) {
      uint32_t ret_addr = READ_BURST_32(VENUS_CLUSTER_L2_CFG(0), VENUS_CLUSTER_L2_DAG_RETURN_ADDR_OFFSET(i));
      uint32_t ret_len  = READ_BURST_32(VENUS_CLUSTER_L2_CFG(0), VENUS_CLUSTER_L2_DAG_RETURN_LENGTH_OFFSET(i));
      // uint32_t* malloc_ptr = malloc(ret_len);
      uint32_t ret_len_aligned = _align_up(ret_len, 64);
      uint32_t* malloc_ptr     = malloc(ret_len_aligned);
      // uint32_t* malloc_ptr = &return_tmp_addr;
      stdata_t* stdata = (stdata_t*)p->outputlist[i];
      stdata->atr      = (int)malloc_ptr;
      // printf("DMA from %p to %p, len = %p\n", (ret_addr + cluster_offset), (int)malloc_ptr, ret_len_aligned);
      ///////// [1015test] ///////
      // if (i == 1) {  // 第二个返回值
      //   int* rfdata_ptr;
      //   int* ptr = (int*)ret_addr;
      //   int data = *ptr & 0xffff;
      //   if (data != 1) {  // data wrong
      //     printf("wrong data2 = %p\n", data);
      //     ret_addr = READ_BURST_32(VENUS_CLUSTER_L2_CFG(0), VENUS_CLUSTER_L2_DAG_RETURN_ADDR_OFFSET(0));
      //     ptr      = (int*)ret_addr;
      //     data     = *ptr & 0xffff;
      //     printf("wrong data1 = %p\n", data);
      //     for (int i = 0; i < 4; i++) {
      //       printf("data[%d]:", i);
      //       rfdata_ptr = (int*)p->inputlist[i];
      //       for (int j = 0; j < 1105; j++) {
      //         printf("%p\n", *rfdata_ptr);
      //         rfdata_ptr++;
      //       }
      //     }
      //   }
      //   while (1) {};
      // }
      ///////////////////////
      if (i == outputnum - 1) {
        dma_transfer((ret_addr + cluster_offset), (int)malloc_ptr, ret_len, 1);
      } else {
        dma_transfer((ret_addr + cluster_offset), (int)malloc_ptr, ret_len, 0);
      }
    }
    dmadsc_t dmadsc;
    dmadsc.ptr = (uint32_t)p;
    dmadsc.atr = DAGPROC_PSEUDO_DSC;
    dmapush_fifo(&dmafifo, dmadsc);
  } else if (cluster_intstatusreg & (1 << VENUS_CLUSTER_L2_DMA_ERR_IRQ)) {
    printf("Cluster L2 DMA error!\n");
    while (1) {};
  } else if (cluster_intstatusreg & (1 << VENUS_CLUSTER_L2_DBG_UART_IRQ)) {
    printf("Cluster L2 debug irq!\n");
    while (1) {};
  }
}
