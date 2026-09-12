#include "dagproc.h"
#include "common.h"
#include "daginfo.h"
#include "fifo.h"
#include "rfdata.h"
#include "venusmmap.h"

cluster_t clusters[NCLUSTER];
dagproc_t* p;
int data_ready;
int MUTEX_dag_done = 0;
extern dmafifo_t dmafifo;
extern void dma_transfer(uint32_t src, uint32_t dst, uint32_t len, uint32_t last);

void dagproc_init(void) {
  int cluster_id = 0;
  int dagproc_id = 0;
  cluster_t* c;
  dagproc_t* p;
  for (c = clusters; c < &clusters[NCLUSTER]; c++) {
    for (p = c->dagproc; p < &(c->dagproc[NPROC]); p++) {
      p->config.prog                = VENUS_CLUSTER_L2_PROG(cluster_id);  // TODO: 不同的dag的有不同的image的地址???
      p->config.config              = VENUS_CLUSTER_L2_CFG(cluster_id);
      p->config.datacontainer       = VENUS_CLUSTER_L2_CFG(cluster_id) + VENUS_CLUSTER_L2_DATA_CONTAINER_OFFSET(dagproc_id);
      p->config.globalparacontainer = VENUS_CLUSTER_L2_CFG(cluster_id) + VENUS_CLUSTER_L2_GLOBAL_PARA_CONTAINER_OFFSET(dagproc_id);
      p->config.outputnumreg        = VENUS_CLUSTER_L2_CFG(cluster_id) + VENUS_CLUSTER_L2_OUTPUT_NUM_REG_ADDR(dagproc_id);
      p->config.tasknumreg          = VENUS_CLUSTER_L2_CFG(cluster_id) + VENUS_CLUSTER_L2_TASK_NUM_REG_ADDR(dagproc_id);
      p->config.returnvalue         = VENUS_CLUSTER_L2_CFG(cluster_id) + VENUS_CLUSTER_L2_RETURN_VALUE_ADDR(dagproc_id);
      p->config.outputaddr          = VENUS_CLUSTER_L2_CFG(cluster_id) + VENUS_CLUSTER_L2_DAG_OUTPUT_ADDR_OFFSET;
      p->cluster                    = c;
      dagproc_id++;
    }
    cluster_id++;
  }
}

static dagproc_t* get_dagproc(void) {
  cluster_t* c;
  dagproc_t* p;
  for (c = clusters; c < &clusters[NCLUSTER]; c++) {
    if (c->payload < NPROC) {
      for (p = c->dagproc; p < &(c->dagproc[NPROC]); p++) {
        if (p->state == IDLE) {
          // p->state = RUNNING;
          c->payload++;
          return p;
        }
      }
    }
  }
  return 0;
}

dagproc_t* register_dag(uint32_t* dagbin_addr, uint32_t* daginput_offset, uint32_t* daginput_length, uint32_t binandjsonsize, int inputnum, int outputnum, ...) {
  MUTEX_dag_done = 0;
  if (inputnum > DAGPROC_MAX_INPUTS || outputnum > DAGPROC_MAX_OUTPUTS) {
    panic("DAG I/O exceeds runtime capacity: in=%d out=%d\n", inputnum, outputnum);
  }
  // 获取一个空闲的dag计算资源，并配置
  dagproc_t* p = get_dagproc();
  p->state     = D2C;
  p->daguid    = (int)dagbin_addr;

  
  //20250310 sy 新需求配置L2
  // WRITE_BURST_32(p->config.config, VENUS_CLUSTER_L2_SHIELD_OFFSET, 0x0e);
  WRITE_BURST_32(p->config.config, VENUS_CLUSTER_L2_MALLOC_INIT_ADDRESS, (binandjsonsize / 0x40 + (binandjsonsize % 0x40 > 0? 1:0)) * 0x40);
  //printf("l2sharedmemstart: %x\n",(binandjsonsize / 0x40 + (binandjsonsize % 0x40 > 0? 1:0)) * 0x40);


  dmadsc_t dmadsc;
  dmadsc.ptr = (uint32_t)p;
  dmadsc.atr = DAGPROC_PSEUDO_DSC;
  dmapush_fifo(&dmafifo, dmadsc);
  return p;
}


void transfer_dag_inputs(dagproc_t* p, uint32_t* dagbin_addr, uint32_t* daginput_offset, uint32_t* daginput_length, uint32_t binandjsonsize, int inputnum, int outputnum, ...) {
  int input_stru_ptr;
  int input_data_ptr;
  va_list vl;
  va_start(vl, outputnum);
  for (int i = 0; i < inputnum; i++) {
    // TODO: 编译的时候要不要加一个dsl和L1main input个数不相等的警告？
    input_stru_ptr     = va_arg(vl, int);
    fifo_t* input_fifo = (fifo_t*)input_stru_ptr;
    input_data_ptr     = pop_fifo(input_fifo);  // 将这个input数据的地址指针存入DAG proc的属性里, DMA传输完毕后回调会使用
    p->inputlist[i]    = (int)input_data_ptr;
    // printf("input %d\n", i);
    // printf("DMA from %p to %p, len = %p\n", input_data_ptr, p->config.prog + daginput_offset[i], daginput_length[i]);
    // printf("DMA transfer data [%d]: from %p to %p, len = %p\n", i, input_data_ptr, p->config.prog + daginput_offset[i], daginput_length[i]);
    dma_transfer((uint32_t)(input_data_ptr), (p->config.prog + daginput_offset[i]), daginput_length[i], 0);
  }

  int output_stru_ptr;
  for (int i = 0; i < outputnum; i++) {
    output_stru_ptr  = va_arg(vl, int);
    p->outputlist[i] = (int)output_stru_ptr;  // 将这个output数据的结构体指针存入DAG proc的属性里, Cluster算完之后会用
  }
  p->inputnum  = inputnum;
  p->outputnum = outputnum;
  va_end(vl);
}
