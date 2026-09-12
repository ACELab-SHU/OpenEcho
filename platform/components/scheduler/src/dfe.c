#include "dfe.h"
#include "coe.h"
#include "fifo.h"
#include "rfdata.h"
#include "venus.h"
#include "venusmmap.h"

// CONFIG_DFE_REG(address, value)
// READ_DFE_REG(address)

// int sum_rs_bits           = 5;  // ch3_ovf-ch11_ovf
// int signal_energy_rs      = 3;  // energy_ovf
// int correlation_energy_rs = 8;  // pss_0_ovf-pss_2_ovf
int sum_rs_bits           = 0;  // ch3_ovf-ch11_ovf
int signal_energy_rs      = 0;  // energy_ovf
int correlation_energy_rs = 0;  // pss_0_ovf-pss_2_ovf
int write_mode            = 0;  // write 8bit or 16bit
int tx_write_mode         = 0;  // write 8bit or 16bit

rfdata_t fft_rfdata;
rfdata_t init_rfdata;
int MUTEX_tx_ram0_uploading = 0;
int MUTEX_tx_ram1_uploading = 0;
int MUTEX_tx_ram0_idle      = 1;
int MUTEX_tx_ram1_idle      = 1;
volatile int MUTEX_tx_rrd_cnt = 0;

extern dmafifo_t dmafifo;

// 1218 test
extern uint32_t slot_offset;

extern char rfdata0_15k[];
extern char rfdata1_15k[];
extern char rfdata2_15k[];
extern char rfdata3_15k[];

// mu: μ子载波间隔
void dfe_init() {
  // printf("Initializing DFE...\n");
  // initialize configuration
  // step0. disable dfe [0x01000008]
  CONFIG_DFE_REG(DFE_ENA_REG, DFE_ENA_1(0));
  // printf("dfe loop\n");
  // step1. 向coefficient寄存器写入相应的系数和index
  // 遍历每个Channel的256个滤波器系数的ram
  for (int i = 0; i < 16; i++) {
    for (int j = 0; j < 16; j++) {
      // COE_VALUE_CONFIG(systolic_index, pe_index, mem_addr, coe_data) [0x01000000] 256*9
      CONFIG_DFE_REG(COEFFICIENT_REG, COE_VALUE_CONFIG(3, i, j, pss0_re[16 * i + j]));         // pss0_re
      CONFIG_DFE_REG(COEFFICIENT_REG, COE_VALUE_CONFIG(4, i, j, pss0_im[16 * i + j]));         // pss0_im
      CONFIG_DFE_REG(COEFFICIENT_REG, COE_VALUE_CONFIG(5, i, j, pss0_re_add_im[16 * i + j]));  // pss0_re_add_im

      CONFIG_DFE_REG(COEFFICIENT_REG, COE_VALUE_CONFIG(6, i, j, pss1_re[16 * i + j]));         // pss1_re
      CONFIG_DFE_REG(COEFFICIENT_REG, COE_VALUE_CONFIG(7, i, j, pss1_im[16 * i + j]));         // pss1_im
      CONFIG_DFE_REG(COEFFICIENT_REG, COE_VALUE_CONFIG(8, i, j, pss1_re_add_im[16 * i + j]));  // pss1_re_add_im

      CONFIG_DFE_REG(COEFFICIENT_REG, COE_VALUE_CONFIG(9, i, j, pss2_re[16 * i + j]));          // pss2_re
      CONFIG_DFE_REG(COEFFICIENT_REG, COE_VALUE_CONFIG(10, i, j, pss2_im[16 * i + j]));         // pss2_im
      CONFIG_DFE_REG(COEFFICIENT_REG, COE_VALUE_CONFIG(11, i, j, pss2_re_add_im[16 * i + j]));  // pss2_re_add_im

      CONFIG_DFE_REG(COEFFICIENT_REG, COE_VALUE_CONFIG(12, i, j, 1));
    }
  }
  CONFIG_DFE_REG(COEFFICIENT_REG, COE_VALUE_CONFIG(0, 15, 7, 1));
  CONFIG_DFE_REG(COEFFICIENT_REG, COE_VALUE_CONFIG(2, 15, 7, 1));
  // swap I channel and Q channel
  CONFIG_DFE_REG(INPUT_CTRL_REG, INPUT_CTRL_CONFIG(1));  // INPUT_CTRL_CONFIG(input_select)

  CONFIG_DFE_REG(CH_CTRL_REG(0), CH_CTRL_CONFIG(1, 1, 1, 0, 7));
  CONFIG_DFE_REG(CH_CTRL_REG(1), CH_CTRL_CONFIG(1, 1, 1, 0, 7));
  CONFIG_DFE_REG(CH_CTRL_REG(2), CH_CTRL_CONFIG(1, 1, 1, 0, 7));
  // step2. & step3.
  for (int i = 3; i < 12; i++) {
    // config channel(input_ena, output_ena, ch_ena, sum_rs_bis, clk_rate) [0x01000018][0x0100001c][0x01000020]...[0x0100003c]
    CONFIG_DFE_REG(CH_CTRL_REG(i), CH_CTRL_CONFIG(1, 1, 1, sum_rs_bits, 15));
    // CONFIG_DFE_REG(CH_CTRL_REG(i), CH_CTRL_CONFIG(1, 1, 1, 6, 15));
  }
  CONFIG_DFE_REG(CH_CTRL_REG(12), CH_CTRL_CONFIG(1, 1, 1, 0, 15));

  // step4. CORRELATION_CTRL_CONFIG(cascade)
  CONFIG_DFE_REG(CORRELATION_CTRL_REG, CORRELATION_CTRL_CONFIG(0));
  // step5. LOW_PASS_FIR_CTRL_CONFIG(bypass)
  CONFIG_DFE_REG(LOW_PASS_FIR_CTRL_REG, LOW_PASS_FIR_CTRL_CONFIG(1));
  // step6. RX_FIFO_CTRL_CONFIG(read_period)
  CONFIG_DFE_REG(RX_FIFO_CTRL_REG, RX_FIFO_CTRL_CONFIG(7));
  // step7.
  uint64_t nco_step = 0;  // 48bits
  // long long nco_step  = -17179869184;
  long long nco_phase = 0;  // 48bits
  CONFIG_DFE_REG(RX_MIXER_CTRL_REG(0), MIXER_CTRL0_CONFIG(nco_step));
  CONFIG_DFE_REG(RX_MIXER_CTRL_REG(1), MIXER_CTRL1_CONFIG(nco_step));
  CONFIG_DFE_REG(RX_MIXER_CTRL_REG(2), MIXER_CTRL2_CONFIG(nco_phase));
  CONFIG_DFE_REG(RX_MIXER_CTRL_REG(3), MIXER_CTRL3_CONFIG(nco_phase));
  // step8. SYNC_CTRL_CONFIG(bypass, signal_energy_rs, correlation_energy_rs)
  CONFIG_DFE_REG(SYNC_CTRL_REG, SYNC_CTRL_CONFIG(0, signal_energy_rs, correlation_energy_rs));
  // CONFIG_DFE_REG(SYNC_CTRL_REG, SYNC_CTRL_CONFIG(0, 3, 8));
  // step9. DECIMATION_CTRL_CONFIG(decimation_ratio)
  CONFIG_DFE_REG(DECIMATION_CTRL_REG, DECIMATION_CTRL_CONFIG((mu == 0 ? 7 : 3)));
  // step10. CORRELATION_CTRL0_CONFIG(th)
  // CONFIG_DFE_REG(CORRELATION_CTRL0_REG, CORRELATION_CTRL0_CONFIG(0x360));  // rb init data
  CONFIG_DFE_REG(CORRELATION_CTRL0_REG, CORRELATION_CTRL0_CONFIG(MAXTH));

  for (int i = 0; i < 14; i++) {
    // SYMBOL_TIMER_CTRL0_CONFIG(symbol_length_valid, symbol_index, symbol_length)
    if (i == 0 || i == (mu == 0 ? 7 : 0)) {
      CONFIG_DFE_REG(SYMBOL_CTRL0_REG, SYMBOL_CTRL0_CONFIG(1, i, (EXTENDED_CP_SAMPLES[mu] - 1)));
    } else {
      CONFIG_DFE_REG(SYMBOL_CTRL0_REG, SYMBOL_CTRL0_CONFIG(1, i, (NORMAL_CP_SAMPLES[mu] - 1)));
    }
  }

  // SYMBOL_TIMER_CTRL1_CONFIG(symbol_number)
  CONFIG_DFE_REG(SYMBOL_CTRL1_REG, SYMBOL_CTRL1_CONFIG(13));
  // SLOT_TIMER_CTRL0_CONFIG(cnt_period, slot_index_boundary)
  CONFIG_DFE_REG(SLOT_TIMER_CTRL0_REG, SLOT_TIMER_CTRL0_CONFIG((mu == 0 ? 245759 : 122879), (mu == 0 ? 9 : 19)));
  // SLOT_TIMER_CTRL1_CONFIG(front_offset)
  // CONFIG_DFE_REG(SLOT_TIMER_CTRL1_REG, SLOT_TIMER_CTRL1_CONFIG(17856));
  // CONFIG_DFE_REG(SLOT_TIMER_CTRL1_REG, SLOT_TIMER_CTRL1_CONFIG(17792));
  // CONFIG_DFE_REG(SLOT_TIMER_CTRL1_REG, SLOT_TIMER_CTRL1_CONFIG((mu == 0 ? 17792 : 8896)));
  // CONFIG_DFE_REG(SLOT_TIMER_CTRL1_REG, SLOT_TIMER_CTRL1_CONFIG((mu == 0 ? 17792 : 8237)));
  // CONFIG_DFE_REG(SLOT_TIMER_CTRL1_REG, SLOT_TIMER_CTRL1_CONFIG(17664));
  CONFIG_DFE_REG(SLOT_TIMER_CTRL1_REG, SLOT_TIMER_CTRL1_CONFIG((mu == 0 ? 17792 : (9117 - 15 * 8))));
  // FRAME_TIMER_CTRL0_CONFIG(frame_index_boundary)
  CONFIG_DFE_REG(FRAME_TIMER_CTRL0_REG, FRAME_TIMER_CTRL0_CONFIG(1023));

  // // set DFE 8bit write
  write_mode = 1;
  CONFIG_DFE_REG(WRITE_CTRL_REG, WRITE_CTRL_CONFIG(write_mode));

  // PEAK_ENABLE_CTRL0_CONFIG(sync_start_time, sync_end_time)
  CONFIG_DFE_REG(PEAK_ENABLE_CTRL0_REG, PEAK_ENABLE_CTRL0_CONFIG(1, (mu == 0 ? 278 : 558)));
  // PEAK_ENABLE_CTRL1_CONFIG(sync_period, peak_window_enable)
  CONFIG_DFE_REG(PEAK_ENABLE_CTRL1_REG, PEAK_ENABLE_CTRL1_CONFIG((mu == 0 ? 279 : 559), 0));
  //	CONFIG_DFE_REG(PEAK_ENABLE_CTRL1_REG, PEAK_ENABLE_CTRL1_CONFIG(559, 1));

  // CONFIG_DFE_REG(0x74, 1);

  // step11. dfe enable
  CONFIG_DFE_REG(DFE_ENA_REG, DFE_ENA_1(1));
  // // test version: waiting for interrupt...
  // while (1) {};
}

void dfe_ovf_interrupt_handler(void) {
  int intr_status_reg = READ_DFE_REG(INTR_STATUS_REG);
  // printf("0x1BC csr = %p\n", intr_status_reg);
  // step.12
  if (CH3_11_OVF_READ(intr_status_reg)) {
    sum_rs_bits++;
    for (int i = 3; i <= 12; i++) {
      // CH_CTRL_CONFIG(input_ena, output_ena, ch_ena, sum_rs_bis, clk_rate)
      CONFIG_DFE_REG(CH_CTRL_REG(i), CH_CTRL_CONFIG(1, 1, 1, sum_rs_bits, 15));
    }
  }
  if (ENERGY_OVF_READ(intr_status_reg) | PSS_0_2_OVF_READ(intr_status_reg)) {
    if (ENERGY_OVF_READ(intr_status_reg)) {
      signal_energy_rs++;
    }
    if (PSS_0_2_OVF_READ(intr_status_reg)) {
      correlation_energy_rs++;
    }
    // SYNC_CTRL_CONFIG(bypass, signal_energy_rs, correlation_energy_rs)
    CONFIG_DFE_REG(SYNC_CTRL_REG, SYNC_CTRL_CONFIG(0, signal_energy_rs, correlation_energy_rs));
  }

  int x = sum_rs_bits * 4 - signal_energy_rs * 2 + correlation_energy_rs * 2;
  // printf("\nsignal_energy_rs: %d, correlation_energy_rs: %d, sum_rs_bits: %d\n", signal_energy_rs, correlation_energy_rs, sum_rs_bits);
  uint32_t th;
  if (x < 14) {
    th = MAXTH;
  } else {
    th = (0x300000000000 >> x);
  }
  CONFIG_DFE_REG(CORRELATION_CTRL0_REG, CORRELATION_CTRL0_CONFIG(th));

  CONFIG_DFE_REG(INTR_CTRL1_REG, INTR_CTRL1_CONFIG(1));
  CONFIG_DFE_REG(PSS_CTRL0_REG, PSS_CTRL0_CONFIG(1));  // step.14
}

rfdata_t* dfe_rfdata_valid(int frame, int slot, int symbol) {
  // printf("\nframe: %d, slot: %d\n", frame, slot);
  // 检测所有启用的rfdata描述符
  for (rfdata_t* rf = (rfdata_t*)RFDATA_START; rf < (rfdata_t*)RFDATA_END; rf++) {
    // 检测frame规则
    if (rf->frame[0] == RULES) {
      switch (rf->frame[1]) {
        case ALL_FRAME_INDEX_VALID:
          break;
        case EVEN_FRAME_INDEX_VALID:
          if (!IS_EVEN(frame))
            continue;
          break;
        case ODD_FRAME_INDEX_VALID:
          if (!IS_ODD(frame))
            continue;
          break;
        default:
          continue;
      }
    } else if (rf->frame[0] == SPEC_NUM) {
      int i;
      for (i = 1; rf->frame[i] != LAST_FRAME_NUM; i++)
        if (rf->frame[i] == frame)
          break;
      if (rf->frame[i] == LAST_FRAME_NUM)
        continue;
    } else {
      continue;
    }

    // 检测slot规则
    if (rf->slot[0] == RULES) {
      switch (rf->slot[1]) {
        case ALL_SLOT_INDEX_VALID:
          break;
        case EVEN_SLOT_INDEX_VALID:
          if (!IS_EVEN(slot))
            continue;
          break;
        case ODD_SLOT_INDEX_VALID:
          if (!IS_ODD(slot))
            continue;
          break;
        case PERIOD_SLOT_INDEX_VALID:
          if (((subframePerFrame * (mu + 1) * frame + slot - rf->slot[3]) % rf->slot[2]))
            continue;
          break;
        default:
          continue;
      }
    } else if (rf->slot[0] == SPEC_NUM) {
      int i;
      for (i = 1; rf->slot[i] != LAST_SLOT_NUM; i++)
        if (rf->slot[i] == slot)
          break;
      if (rf->slot[i] == LAST_SLOT_NUM)
        continue;
    } else {
      continue;
    }

    if (rf->symbol[0] == RULES) {
      // TODO: 建立规则
    } else if (rf->symbol[0] == SPEC_NUM) {
      int i;
      for (i = 1; rf->symbol[i] != LAST_SYMBOL_NUM; i++)
        if (rf->symbol[i] == symbol)
          break;
      if (rf->symbol[i] == LAST_SYMBOL_NUM)
        continue;
    } else {
      continue;
    }

    return rf;
  }
  return NULL;
}
// SEM: 所有文件都有可能判断的计数器（信号量）
// SHARED：可能会被不同文件修改的共享变量
// MUTEX：非0或1（可能还有无效值）（互斥锁）
volatile int SEM_dfe_initdata_cnt   = 0;       // dfe irq handler 里面计数的(在初始化数据准备让DMA传的时候++)
volatile int SHARED_after_sync_slot = 0xffff;  // sync之后的第一个ram rx ready中断时所收到的包头
volatile int SHARED_req_pss_slot    = 0xffff;
volatile int SHARED_req_pss_frame   = 0xffff;
// int req_pss_last_slot      = 0xff;
// int req_pss_last_frame     = 0xff;
// static int req_pss_symbol  = 0xff;
volatile int SEM_dma_initdata_cnt        = 0;  // main和DMA里面计数的(在DMA传递完毕的时候++, main里面判断)
volatile int MUTEX_gc_modify_timing_done = 1;  // 初始化为1表示不启用gc timing modify(直接开始等sync)
volatile int SHARED_pss0_status0_reg;
volatile int SHARED_pss1_status0_reg;
volatile int SHARED_pss2_status0_reg;

// [20241203 yf new add]
// volatile int timing_offset_calibration     = 0;
volatile int SHARED_req_ssb_start_symbol   = 0;
volatile int MUTEX_first_rx_ram_ready_done = 0;
volatile int SHARED_frame_offset           = 0;
// volatile int srs_save_flag                 = 0;
// volatile int srs_save_addr                 = 0;
volatile int SEM_dfe_data_cnt     = 0;
volatile int SEM_max_dfe_data_cnt = 0;

volatile int dfe_ram_tail_data[20] = {0};

void dfe_rx_ram_ready_interrupt_handler(void) {
  // printf("rx ram ready!\n");
  uint32_t current_write_address_offset = WRITE_STATUS_READ(READ_DFE_REG(WRITE_STATUS_REG));  // 读取包头地址
  current_write_address_offset          = current_write_address_offset * 64;                  // 编译器会自己优化成shift指令(rx ram地址转换)

  uint32_t raw_dout_reg = READ_BURST_32(GC0802_DFE_RX_RAM_ADDR, current_write_address_offset);  // 读取包头信息
  uint32_t frame        = (FRAME_INDEX_READ(raw_dout_reg) + 1024 + SHARED_frame_offset) % 1024;
  uint32_t slot         = SLOT_CNT_READ(raw_dout_reg);
  uint32_t symbol       = SYMBOL_CNT_READ(raw_dout_reg);

  // printf("frame_index: %p | slot_cnt: %p | symbol: %p\n", frame, slot, symbol);
  uint32_t rx_addr;
  uint32_t remaining_space;
  uint32_t excess_space;
  uint32_t malloc_addr;
  dmadsc_t dmadsc;

  if (!MUTEX_gc_modify_timing_done) {
    // WRITE_BURST_32(dfe_data_package_ptr, 0, raw_dout_reg);
    // [上板测试]: 让DMA传rx ram里面数据的逻辑
    PROCESS_DMA_TRANSFER(current_write_address_offset, write_mode);
    dmadsc.ptr = malloc_addr;
    dmadsc.atr = (int)&fft_rfdata;  // 把结构体指针传过去, DMA传输完毕回调用
    dmapush_fifo(&dmafifo, dmadsc);
  } else {
    // 4个init数据还没到来，要所有的数据(16bit/write_mode=0)
    // slot 等slot+19的包头 【0-19】
    if (SHARED_after_sync_slot == 0xffff) {
      // 第一次rx ram ready不采数据，等待下一次
      if (!MUTEX_first_rx_ram_ready_done) {
        MUTEX_first_rx_ram_ready_done = 1;
        /*    27     26     25    24   23222120 19181716 15 14   13   12
         * | sync | peak | ram | gpiox |  ...  |  ...  | x | x | ovf | x |
         * |  1   |  1   |  1  |   0   |  0x0  |  0x0  | 0 | 0 |  1  | 0 |
         */
        Mask_irq(0xfe002000);  // 关闭dfe的irq
      } else {
        SHARED_after_sync_slot = slot;
      }
      SHARED_req_pss_slot = (slot + (mu == 0 ? 19 : 39)) % (mu == 0 ? 10 : 20);
      if (slot == 0) {
        SHARED_req_pss_frame = (frame + 1) % 1024;
      } else {
        SHARED_req_pss_frame = (frame + 2) % 1024;
      }
      // printf("SHARED_req_pss_slot:%d\n", SHARED_req_pss_slot);
      // printf("SHARED_req_pss_frame:%d\n", SHARED_req_pss_frame);
    } else if ((SEM_dfe_initdata_cnt != INIT_PACKAGE_NUM) && (slot == SHARED_req_pss_slot) && (frame == SHARED_req_pss_frame) && (symbol == (SHARED_req_ssb_start_symbol + SEM_dfe_initdata_cnt))) {
      // printf("rx ram ready!\n");
      SEM_dfe_initdata_cnt++;
      // 宏里面会根据write_mode来malloc相应的数据并让DMA传
      PROCESS_DMA_TRANSFER(current_write_address_offset, write_mode);
      // printf("[0]%p\n", *((int*)(GC0802_DFE_RX_RAM_ADDR + current_write_address_offset)));
      // printf("[1]%p\n", *((int*)(GC0802_DFE_RX_RAM_ADDR + current_write_address_offset) + 1));
      // printf("[2]%p\n", *((int*)(GC0802_DFE_RX_RAM_ADDR + current_write_address_offset) + 2));
      // printf("[3]%p\n", *((int*)(GC0802_DFE_RX_RAM_ADDR + current_write_address_offset) + 3));
          // for (int i = 0; i < 20; i++) {
          //   current_write_address_offset 
          //   dfe_ram_tail_data[i] = 
          // }
      dmadsc.ptr = malloc_addr;
      dmadsc.atr = (int)&init_rfdata;  // 把结构体指针传过去, DMA传输完毕回调
      dmapush_fifo(&dmafifo, dmadsc);
      if (SEM_dfe_initdata_cnt == INIT_PACKAGE_NUM) {
        Mask_irq(0xfe002000);  // 关闭dfe的irq
      }
    } else if (SEM_dfe_data_cnt < SEM_max_dfe_data_cnt) {
      // 这个情况是用来给后面DAG发射流程准备的逻辑
      rfdata_t* rfdata = dfe_rfdata_valid(frame, slot, symbol);
      if (rfdata) {
        // CONFIG_DFE_REG(SLOT_TIMER_CTRL2_REG, SLOT_TIMER_CTRL2_CONFIG(slot_offset));  // slot offset
        PROCESS_DMA_TRANSFER(current_write_address_offset, write_mode);
        dmadsc.ptr = malloc_addr;
        dmadsc.atr = (int)rfdata;
        dmapush_fifo(&dmafifo, dmadsc);
        SEM_dfe_data_cnt++;
        if (SEM_dfe_data_cnt == SEM_max_dfe_data_cnt) {
          Mask_irq(0xfe002000);  // 关闭dfe的irq
        }
      }
    }
  }
  // clear all irq
  CONFIG_DFE_REG(INTR_CTRL1_REG, INTR_CTRL1_CONFIG(1));
}

void dfe_sync_interrupt_handler(void) {
  // printf("sync!\n");
  CONFIG_DFE_REG(INTR_CTRL1_REG, INTR_CTRL1_CONFIG(1));
  /*
   * | sync | peak | ram | gpiox |
   * |  1   |  1   |  0  |   0   |
   */
  Mask_irq(0xfc000000);  // 3. 关闭sync和peak，打开ram irq
  // WRITE_BURST_32(GC0802_TIMER_ADDR, VENUS_DW_APB_TIMER_CTRL_REG(0), 0x00);
  // // deassert timer1 resetn
  // uint32_t response = READ_BURST_32(GC0802_DEVCTRL_ADDR, GC0802_APB_DEV_RST_OFFSET) & 0xfffffeff;
  // WRITE_BURST_32(GC0802_DEVCTRL_ADDR, GC0802_APB_DEV_RST_OFFSET, response);  // 关闭timer1
  // }
}

void dfe_peak_interrupt_handler(void) {
  // printf("peek!\n");
  if (MUTEX_first_rx_ram_ready_done == 1) {
    MUTEX_first_rx_ram_ready_done = 2;  // 设置为一个无效数据
    // 恢复默认同步窗
    // PEAK_ENABLE_CTRL0_CONFIG(sync_start_time, sync_end_time)
    CONFIG_DFE_REG(PEAK_ENABLE_CTRL0_REG, PEAK_ENABLE_CTRL0_CONFIG(1, (mu == 0 ? 278 : 558)));
  }

  CONFIG_DFE_REG(INTR_CTRL1_REG, INTR_CTRL1_CONFIG(1));
  /*
   * | sync | peak | ram | gpiox |
   * |  0   |  1   |  1  |   0   |
   */
  Mask_irq(0xf6002000);  // 2. 打开sync，关掉peak和ovf
  // CONFIG_DFE_REG(PSS_CTRL0_REG, PSS_CTRL0_CONFIG(1));
  SHARED_pss0_status0_reg = READ_DFE_REG(PSS_STATUS0_REG(0));  // 0xcc
  SHARED_pss1_status0_reg = READ_DFE_REG(PSS_STATUS0_REG(1));  // 0xd0
  SHARED_pss2_status0_reg = READ_DFE_REG(PSS_STATUS0_REG(2));  // 0xd4
}

long long nco_step_caculate(double freqOffset) {
  long long step = (long long)(floor(freqOffset / 245.76e6 * (double)((long long)1 << 48) + 0.5));
  return step;
}
void tx_ram0_transmit(int length, int* data_addr) {
  // printf("tx ram0 trans!\n");
  dmadsc_t dmadsc;
  MUTEX_tx_ram0_uploading = 1;
  MUTEX_tx_ram0_idle      = 0;
  uint32_t mask_irq       = Mask_irq(0xffffffff);  // disable all interrupts
  // 配置数据长度
  // CONFIG_DFE_REG(READ_CTRL2_REG, READ_CTRL2_CONFIG(length));  // 可配置第一块存储区域的有效长度
  // 将tx rf data写入tx ram0
  dma_transfer(data_addr, GC0802_DFE_TX_RAM_ADDR, length, 1);
  dmadsc.ptr = data_addr;
  dmadsc.atr = TXDATA_RAM0_PSEUDO_DSC;
  dmapush_fifo(&dmafifo, dmadsc);
  // 配置rx rf data的发射时间
  // READ_CTRL4_CONFIG(frame, slot, symbol)
  // CONFIG_DFE_REG(READ_CTRL4_REG, READ_CTRL4_CONFIG(frame, symbol, length));
  Mask_irq(mask_irq);
}

void tx_ram1_transmit(int length, int* data_addr) {
  // printf("tx ram1 trans!\n");
  dmadsc_t dmadsc;
  MUTEX_tx_ram1_uploading = 1;
  MUTEX_tx_ram1_idle      = 0;
  uint32_t mask_irq       = Mask_irq(0xffffffff);  // disable all interrupts
  // 配置数据长度
  // CONFIG_DFE_REG(READ_CTRL6_REG, READ_CTRL6_CONFIG(length));  // 可配置第二块存储区域的有效长度
  // 将tx rf data写入tx ram1
  dma_transfer(data_addr, (GC0802_DFE_TX_RAM_ADDR + 8832), length, 1);
  dmadsc.ptr = data_addr;
  dmadsc.atr = TXDATA_RAM1_PSEUDO_DSC;
  dmapush_fifo(&dmafifo, dmadsc);
  // 配置rx rf data的发射时间
  // READ_CTRL8_CONFIG(frame, slot, symbol)
  // CONFIG_DFE_REG(READ_CTRL8_REG, READ_CTRL8_CONFIG(frame, symbol, length));
  Mask_irq(mask_irq);
}


void dfe_tx_ram_read_done_interrupt_handler(void) {
  CONFIG_DFE_REG(INTR_CTRL1_REG, INTR_CTRL1_CONFIG(1));
  // usleep(10);
  // if (MUTEX_ram_ready_select == 0) {
  //   MUTEX_tx_ram0_idle = 1;
  //   MUTEX_ram_ready_select == 1;
  // } else {
  //   MUTEX_tx_ram1_idle = 1;
  //   MUTEX_ram_ready_select == 0;
  // }

  if (MUTEX_tx_rrd_cnt == 1) {
    // tx_ram0_transmit(4384, (int*)&rfdata2_15k);
    CONFIG_DFE_REG(0x194, 0x1);
    CONFIG_DFE_REG(0x198, 0x0);
    MUTEX_tx_rrd_cnt = 2;
    // printf("%d",MUTEX_tx_rrd_cnt);
  } else if(MUTEX_tx_rrd_cnt == 2){
    // tx_ram1_transmit(4384, (int*)&rfdata3_15k);
    CONFIG_DFE_REG(0x194, 0x0);
    CONFIG_DFE_REG(0x198, 0x1);
    MUTEX_tx_rrd_cnt = 0;
    // printf("%d",MUTEX_tx_rrd_cnt);
  }
  CONFIG_DFE_REG(INTR_CTRL1_REG, INTR_CTRL1_CONFIG(1));
  // usleep(10);
}
