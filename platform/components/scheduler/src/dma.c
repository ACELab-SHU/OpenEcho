#include "common.h"
#include "dagproc.h"
#include "dfe.h"
#include "fifo.h"
#include "rfdata.h"
#include "venus.h"

extern volatile uint32_t rx_data_valid;  // for test
extern uint32_t* ram_data_addr;
extern uint32_t HEAP_START;
extern volatile int SEM_dma_initdata_cnt;
extern void evaluate_snr(int data_addr);
extern int update_timing(void);
extern volatile int MUTEX_gc_modify_timing_done;
dagproc_t* p_test;
extern int malloc_addr1;
extern int malloc_addr2;
extern int malloc_addr3;
extern int malloc_addr4;
extern volatile int pseudo_data_trans_done;

/*         dmafifo
 *  +--+--+--+--+--+--+--+
 * --> |  |  |  |  |  |  |  proc struct
 *  +--+--+--+--+--+--+--+
 */
dmafifo_t dmafifo;  // 与DMA的请求fifo一致，每发起一次dma传输（散列）就存储一次DMA的传输信息
int dma_error_hpn = 0;

static inline void dma_available(void) {
  uint32_t statreg = READ_BURST_32(VENUS_DMA_BASE_ADDR, VENUS_L1_DMA_STATREG);
  while (VENUS_L1_DMA_FULL(statreg)) {
    statreg = READ_BURST_32(VENUS_DMA_BASE_ADDR, VENUS_L1_DMA_STATREG);
  }
}

void dma_transfer(uint32_t src, uint32_t dst, uint32_t len, uint32_t last) {
  // printf("Dtfr: src=%p, dst=%p, len=%p\n", src, dst, len);
  if(dma_error_hpn)
  {
    // printf("skpd_t\n");
    return;
  }
  dma_available();
  WRITE_BURST_32(VENUS_DMA_BASE_ADDR, VENUS_L1_DMA_SRCREG, src);
  WRITE_BURST_32(VENUS_DMA_BASE_ADDR, VENUS_L1_DMA_DSTREG, dst);
  WRITE_BURST_32(VENUS_DMA_BASE_ADDR, VENUS_L1_DMA_LENREG, len);
  if (last) {
    WRITE_BURST_32(VENUS_DMA_BASE_ADDR, VENUS_L1_DMA_CFGREG, VENUS_L1_DMA_LAST);
  } else {
    WRITE_BURST_32(VENUS_DMA_BASE_ADDR, VENUS_L1_DMA_CFGREG, VENUS_L1_DMA_PUSH);
  }
}

void dma_interrupt_handler(void) {
  // clear DMA irq
  WRITE_BURST_32(VENUS_DMA_BASE_ADDR, VENUS_L1_DMA_CFGREG, VENUS_L1_DMA_CLEAR);

  dmadsc_t d = dmapop_fifo(&dmafifo);
  if (d.atr == DAGPROC_PSEUDO_DSC) {
    dagproc_t* p = (dagproc_t*)d.ptr;
    cluster_t* c = p->cluster;
    if (p->state == D2C) {
      // printf("Fire Dag done...\n");
      p->state = RUN;
      // enable scheduler...
      WRITE_BURST_32(p->config.config, VENUS_CLUSTER_L2_RESET_REG_ADDR, 1);
      int inputnum = p->inputnum;
      // 判断input的数据是「静态编译分配好的」还是「动态malloc出来的」, 再决定是否回收(free）
      // for (int i = 0; i < inputnum; i++) {
      //   int* ptr = (int*)(p->inputlist[i]);
      //   if (ADDRESS_IN_HEAP_RANGE(ptr)) {
      //     free((int*)(p->inputlist[i]));
      //   }
      // }
    } else if (p->state == C2D) {
      p->state = IDLE;
      c->payload--;
      int outputnum = p->outputnum;
      for (int i = 0; i < outputnum; i++) {
        stdata_t* stdata = (stdata_t*)p->outputlist[i];
        int stdata_ptr   = stdata->atr;
        push_fifo(&stdata->fifo, stdata_ptr);
      }
      MUTEX_dag_done = 1;
      p_test         = p;
      // printf("DAG return transmit done! $stop\n");
    } else if(dma_error_hpn == 1) {
      printf("DMA error caused dma interrupt!");
      return;
    } else {
      panic("Proc state wrong!\n");
    }
  } else if (d.atr == NORMAL_TRANSFER) {
    ;
  } else {
    rfdata_t* rfdata = (rfdata_t*)d.atr;
    if (rfdata == &fft_rfdata) {
      // [上板测试]: DMA中断回调函数里，跳转到执行fft进行snr运算的逻辑
      // 0901: 什么都不做
      // d.ptr是malloc给rx ram data的指针
      // printf("dma trans fft test data done!\n");
      // printf("Malloc addr: %p\n", d.ptr);
      // uint32_t* ptr = (uint32_t*)d.ptr;
      // // if (reg_index == MAX_REG_INDEX){
      // // [上板测试]: 是否要打印数据
      // for (int i = 0; i < 16; i++) {
      //   uint32_t data = *ptr++;
      //   printf("%p\n", data);
      // }
      // dfe rx的片上数据地址送入fft函数进行计算
      // 为了不错过任何一个rx ram中断，不执行fft...[start]
      evaluate_snr(d.ptr);
      MUTEX_gc_modify_timing_done = update_timing();
      free(d.ptr);
      if (!MUTEX_gc_modify_timing_done) {
        // 如果还在调timing，退出中断前需要清理dfe的中断
        CONFIG_DFE_REG(INTR_CTRL1_REG, INTR_CTRL1_CONFIG(1));
      }
      // [end]
      // free(d.ptr);
    } else {
      if (rfdata == &init_rfdata) {
        SEM_dma_initdata_cnt++;
      }
      push_fifo(&(rfdata->fifo), d.ptr);
    }
  }
}

void dma_error_handler(void) {
  uint32_t err_addr = READ_BURST_32(VENUS_DMA_BASE_ADDR, VENUS_L1_DMA_ERRORADDR);
  uint32_t stat_reg = READ_BURST_32(VENUS_DMA_BASE_ADDR, VENUS_L1_DMA_STATREG);
  WRITE_BURST_32(GC0802_DEVCTRL_ADDR, GC0802_AXI_DEV_RST_OFFSET, 0xFFFFFFFE & READ_BURST_32(GC0802_DEVCTRL_ADDR, GC0802_AXI_DEV_RST_OFFSET));
  WRITE_BURST_32(GC0802_DEVCTRL_ADDR, GC0802_CLUSTER0_DEV_RST_OFFSET, 0x00002000);
  WRITE_BURST_32(GC0802_DEVCTRL_ADDR, GC0802_AXI_DEV_RST_OFFSET, 0x1 | READ_BURST_32(GC0802_DEVCTRL_ADDR, GC0802_AXI_DEV_RST_OFFSET));
  WRITE_BURST_32(GC0802_DEVCTRL_ADDR, GC0802_CLUSTER0_DEV_RST_OFFSET, 0xdfff3fff);
  dma_error_hpn = 1;
  printf("DMA status=%d\n",stat_reg);
  if (VENUS_L1_DMA_RD_ERR(stat_reg)) {
    printf("DMA error at 0x%x due to axi read error! $stop\n", err_addr);
  } else if (VENUS_L1_DMA_WR_ERR(stat_reg)) {
    printf("DMA error at 0x%x due to axi write error! $stop\n", err_addr);
  } else if (VENUS_L1_DMA_UNALIGNED_ERR(stat_reg)) {
    printf("DMA error at 0x%x due to address unaligned error! $stop\n", err_addr);
  } else if (VENUS_L1_DMA_NARROW_CROSS_ERR(stat_reg)) {
    printf("DMA error at 0x%x due to narrow cross error! $stop\n", err_addr);
  }
}

// else if (d.atr == 0x12345678) {
//   printf("dma kernel trans test data done!\n");
//   uint32_t* ptr = (uint32_t*)d.ptr;
//   for (int i = 0; i < 2048; i++) {
//     uint32_t data = *ptr++;
//     printf("%p\n", data);
//   }
//   while (1) {};
// }