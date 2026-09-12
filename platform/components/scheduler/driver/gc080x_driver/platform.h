/******************************************************************************
*
* Copyright (C) 2008 - 2014 Xilinx, Inc.  All rights reserved.
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.
*
* Use of the Software is limited solely to applications:
* (a) running on a Xilinx device, or
* (b) that interact with a Xilinx device through a bus or interconnect.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
* XILINX  BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
* WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
* OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*
* Except as contained in this notice, the name of the Xilinx shall not be used
* in advertising or otherwise to promote the sale, use or other dealings in
* this Software without prior written authorization from Xilinx.
*
******************************************************************************/

#ifndef __PLATFORM_H_
#define __PLATFORM_H_

#include "platform_config.h"
#include <stdio.h>
// #include <stdlib.h>
// #include <string.h>
#include "common.h"
#include "config.h"
#include "malloc.h"
#include "printf.h"
#include "stdlib.h"
#include "utility.h"
#include <errno.h>
#include <math.h>
#include <time.h>

#define DEVICE_VERSION ("V2.8.25-2023_12_06")
#define LUT_VERSION_D1 ("231116-1.11")
#define LUT_VERSION_D2 ("230915-2.9")
#define LUT_VERSION_E1 ("231010-4.9")

#define USE_LOG_BUFFER (0)
// lyt: We don't use these 2 macros
// #define  VENDOR_SPI_RW           (0)
// #define  LINUX_OS                (0)
#define HAVE_FS (0)

#define LOAD_LUT_FROM_BIN_FILE (0)
#define LUT_BIN_PATH           ("./lut_bin")

#define CH1_FDD_LUT_ON           (1)
#define RX1_TX2_FDD_LUT_ON       (0)
#define RX2_TX1_FDD_LUT_ON       (0)
#define CH2_FDD_LUT_ON           (0)
#define CH1_CH2_FDD_LUT_ON       (0)
#define CH1_TDD_LUT_ON           (1)
#define CH2_TDD_LUT_ON           (0)
#define CH1_CH2_TDD_LUT_ON       (0)
#define HYBRID_FDD_CH1CH2_LUT_ON (0)
#define HYBRID_TDD_CH1CH2_LUT_ON (0)
#define HYBRID_FDD_CH1_LUT_ON    (0)
#define HYBRID_FDD_CH2_LUT_ON    (0)

#define FPGA_CTRL_RST_PERL_MASK UINT8_C(1 << 0)
#define FPGA_CTRL_RST_FPGA_MASK UINT8_C(1 << 1)
#define FPGA_CTRL_RST_QEC_MASK  UINT8_C(1 << 2)

#define RX_DC_TRACK_MAX_CNT (100)

#if LINUX_OS  //+++++++++++++++++++++++++++++
// #include <sys/types.h>
// #include <sys/stat.h>
// #include <fcntl.h>
// #include <unistd.h>
// #include <signal.h>
// #include <sys/param.h>
// #include <syslog.h>
// #include <sys/wait.h>
// #include <sys/sem.h>
// #include <sys/ipc.h>
// #include <pthread.h>

// #define  CHIP_DELAY(time)     usleep(1000*time)
// #define  CHIP_UDELAY(time)    usleep(time)
// #define  CHIP_SDELAY(time)    sleep(time)

// typedef struct task_struct
// {
//     pthread_t pid;
// } task_t;

// #define  TASK_HANDLE_VALID(task)  (task.pid)
// #define  TASK_HANDLE_INVALID(task) (task.pid = 0)

#else  //+++++++++++++++++++++++++++++
#include "sleep.h"
#define CHIP_DELAY(time)  usleep(1000 * time)
#define CHIP_UDELAY(time) usleep(time)
#define CHIP_SDELAY(time) sleep(time)

typedef struct task_struct {
} task_t;

#define TASK_HANDLE_VALID(task)
#define TASK_HANDLE_INVALID(task)

#endif  //+++++++++++++++++++++++++++++

#define RX_BW_CAL        (1 << 0)
#define RX_DC_CAL        (1 << 1)
#define RX_ADC_CAL       (1 << 2)
#define TX_BW_CAL        (1 << 3)
#define TX_DC_CAL        (1 << 4)
#define TX_DAC_CAL       (1 << 5)
#define TRX_QEC_CAL      (1 << 6)
#define SX_TXLO_CAL      (1 << 7)
#define SX_TRX_CAL       (1 << 8)
#define CHIP_LDO_CAL     (1 << 9)
#define LVDS_DELAY_CAL   (1 << 10)
#define RX_DC_OFFSET_CAL RX_DC_CAL
#define SPI_RW_LOG       (1 << 11)

//extern unsigned long g_module_debug;

#if USE_LOG_BUFFER
// #include <sys/time.h>
// #include <unistd.h>
// #include "log.h"
// #undef LOG_INFO

// #define  LOG_INFO(fmt,...)    //printf(fmt, ##__VA_ARGS__)
// #define  LOG_WARN(fmt,...)    \
//     do { \
//         char ring_buffer_temp[512]; \
//         memset(ring_buffer_temp, 0, 512); \
//         sprintf(ring_buffer_temp, fmt, ##__VA_ARGS__); \
//         system_log_append(ring_buffer_temp); \
//     } while(0)

// #define  LOG_ERROR(fmt,...)   \
//     do { \
//         char ring_buffer_temp[512]; \
//         memset(ring_buffer_temp, 0, 512); \
//         sprintf(ring_buffer_temp, fmt, ##__VA_ARGS__); \
//         system_log_append(ring_buffer_temp); \
//     } while(0)

// #define  LOG_MAIN(fmt,...)    \
//     do { \
//         char ring_buffer_temp[512]; \
//         memset(ring_buffer_temp, 0, 512); \
//         sprintf(ring_buffer_temp, fmt, ##__VA_ARGS__); \
//         system_log_append(ring_buffer_temp); \
//     } while(0)

// #define  LOG_MDEBUG(phy, module, fmt,...) \
//     do { \
//         char ring_buffer_temp[512]; \
//         memset(ring_buffer_temp, 0, 512); \
//         if(phy->module_debug & module) {  \
//             sprintf(ring_buffer_temp, fmt, ##__VA_ARGS__); \
//             system_log_append(ring_buffer_temp); \
//         }  \
//     } while(0)

// #define  DEFINE_TIME()  \
//         struct timeval tvBegin, tvEnd; \
//         double dDurationS;

// #define  START_TIME() \
//     do { \
//         gettimeofday(&tvBegin, NULL); \
//     } while (0)

// #define  END_TIME()      \
//     do { \
//         gettimeofday(&tvEnd, NULL); \
//         dDurationS = (tvEnd.tv_sec - tvBegin.tv_sec) + ((tvEnd.tv_usec - tvBegin.tv_usec) / 1000.0) / 1000.0; \
//     } while (0)

// #define  PRINT_TIME(str)  \
//     do { \
//         LOG_MAIN("+++++++++++++++[%s]seconds: %.3f\n", str, dDurationS); \
//     } while (0)
#else
#undef LOG_INFO
#define LOG_INFO(fmt, ...)  printf(fmt, ##__VA_ARGS__)
#define LOG_WARN(fmt, ...)  printf(fmt, ##__VA_ARGS__)
#define LOG_ERROR(fmt, ...) printf(fmt, ##__VA_ARGS__)
#define LOG_MAIN(fmt, ...)  printf(fmt, ##__VA_ARGS__)
#define LOG_MDEBUG(phy, module, fmt, ...) \
  do {                                    \
    if (phy->module_debug & module) {     \
      printf(fmt, ##__VA_ARGS__);         \
    }                                     \
  } while (0)
#define DEFINE_TIME()
#define START_TIME()
#define END_TIME() ;
#define PRINT_TIME(str) \
  printf(str);          \
  printf("\n");
#endif

typedef struct _tracking_thread {
  task_t tid;
  volatile int need_exit;
  volatile int tracking_times;
} tracking_thread;

typedef struct _rx_qec_tracking_thread_var {
  tracking_thread thread;
  int dbfs_threshold;
} rx_qec_tracking_thread_var;

typedef struct _rx_ana_tracking_thread_var {
  tracking_thread thread;
  int debug;
  int tia;
  int hold_cnt;
  int track_cnt;
} rx_ana_tracking_thread_var;

int create_task(task_t* ptask, void* (*start_rtn)(void*), void* arg);

typedef struct rf_chip_phy_struct {
  chip_config_t* config;
  unsigned char tx_bw_setting[15];
  unsigned char rx_bw_setting[13];
  RX_GCTRL_CFG_T rx_gain_ctrl[2];
  unsigned char tx_bb_gain_config[50][16];

  char r_cal_flag;
  unsigned char r_cal;

  char ldo_cal_flag;
  unsigned char ldo_cal[6];

  char lvds_cal_flag;
  unsigned char lvds_rx;
  unsigned char lvds_tx;

  char auxadc1_cal_flag;
  int auxadccal_slope;
  int auxadccal_ordinate;

  char sx_cal_flag;
  unsigned char sx_cal_1[512];
  unsigned char sx_cal_2[512];

  char txlo_cal_flag;
  unsigned char txlo_cal_1[32];
  unsigned char txlo_cal_2[32];

  char tx_dc_cal_flag[2];
  unsigned char tx_dc_cal[2][4];

  char tx_dac_cal_flag[2];
  unsigned char tx_dac_cal[2][65 * 2];

  unsigned char tia[2][RX_PORT_GAIN_MAX * 2];
  unsigned char bq_rx1[RX_PORT_GAIN_MAX][26];
  unsigned char bq_rx2[RX_PORT_GAIN_MAX][26];
  char rx_dc_cal_flag[2];
  char rx_dc_lut_update;

  char rx_bw_cal_flag[2];
  unsigned char rx_imbalance_cal[2][2];
  unsigned char rx_bw_cal[2][2];

  char rx_qec_flag[2];
  RX_QEC_CFG_REGS rx_qec[2];

  char tx_qec_flag[2];
  TX_QEC_CFG_REGS tx_qec[2][6];

  short tx_atten[2];
  short tx_curr_index;
  unsigned char tx_pa_gain[20];
  unsigned char tx_bb_gain[16];

  short rx_lmt_gain[2];
  short rx_lpf_gain[2];
  short rx_dig_gain[2];

  BANDWITH_ENUM bandwith;
  unsigned long long rxlo;
  unsigned long long txlo;
  unsigned long long min_vco;

  unsigned long module_debug;
  char debug_on;
  char loading_lut;
  short error;
  char init_flag;
  FSM_ST_ENUM st;
  int spi;
  rx_qec_tracking_thread_var rxqec_track_thread[TRX_CH_CNT];
  rx_ana_tracking_thread_var rxdc_track_thread[TRX_CH_CNT];
} rf_chip_phy_t;

extern unsigned int hal_fpga_read_reg(rf_chip_phy_t* phy, unsigned int reg);
extern short hal_fpga_write_reg(rf_chip_phy_t* phy, unsigned int reg, unsigned int val);
extern unsigned char hal_spi_read_reg(rf_chip_phy_t* phy, unsigned short reg);
extern short hal_spi_write_reg(rf_chip_phy_t* phy, unsigned short reg, unsigned char val);
extern void HAL_CONFIG_REGS(rf_chip_phy_t* phy, const reg_t* setting, short len);
extern unsigned char HAL_REG_GET_BITS(rf_chip_phy_t* phy, unsigned short reg, REG_BIT bit, char cnt);
extern void HAL_REG_SET_BITS(rf_chip_phy_t* phy, unsigned short reg, REG_BIT bit, char cnt, unsigned char setval);
extern char HAL_REG_GET_BIT(rf_chip_phy_t* phy, unsigned short reg, REG_BIT bit);
extern void HAL_REG_SET_BIT(rf_chip_phy_t* phy, unsigned short reg, REG_BIT bit);
extern void HAL_REG_CLR_BIT(rf_chip_phy_t* phy, unsigned short reg, REG_BIT bit);

extern void wire_control_en(rf_chip_phy_t* phy, short en, WIRE_CTRL_ENUM pulse);
extern int tdd_wait_to_alert(rf_chip_phy_t* phy);
extern int tdd_alert_to_rx(rf_chip_phy_t* phy);
extern int tdd_rx_to_wait(rf_chip_phy_t* phy);
extern int tdd_alert_to_tx(rf_chip_phy_t* phy);
extern int tdd_tx_to_wait(rf_chip_phy_t* phy);
extern int tdd_rx_to_alert_level(rf_chip_phy_t* phy);
extern int tdd_tx_to_alert_level(rf_chip_phy_t* phy);
extern int tdd_alert_to_tx_level(rf_chip_phy_t* phy);
extern int tdd_alert_to_rx_level(rf_chip_phy_t* phy);
extern int fpga_and_chip_reset(rf_chip_phy_t* phy);
extern void chip_reset_withus(rf_chip_phy_t* phy, unsigned int delay);

extern void lvds_delay_cal(rf_chip_phy_t* phy);
extern short tx_lvds_delay_cal(rf_chip_phy_t* phy);
extern short rx_lvds_delay_cal(rf_chip_phy_t* phy);
extern int fpga_tail_set(rf_chip_phy_t* phy, short dir, short chn, short intflag);
extern int fn_fpga_set_if(rf_chip_phy_t* phy, short mode, short dif, short port, short rate, short flag);
extern int valid_channel(TRX_CHN_ENUM chn, CHIP_MODE_ENUM mode);

typedef struct handle_struct {
  int fd;
} handle_t;

extern int read_flash(handle_t handle, int addr, unsigned char buffer[], int size);
extern handle_t get_bin_handle(rf_chip_phy_t* phy, short* ok);
extern void close_bin_handle(handle_t handle);

#if (!LOAD_LUT_FROM_BIN_FILE)

#if (CH1_FDD_LUT_ON)
// extern const unsigned char lut_D1_CH1_FDD[3752][8];
extern const unsigned char lut_D2_CH1_FDD[3752][8];
// extern const unsigned char lut_E1_CH1_FDD[3752][8];
extern const unsigned char lut_E1_CH1_FDD[1][1];
extern const unsigned char lut_D1_CH1_FDD[1][1];

#endif

#if (RX1_TX2_FDD_LUT_ON)
extern const unsigned char lut_D1_RX1TX2_FDD[3752][8];
extern const unsigned char lut_D2_RX1TX2_FDD[3752][8];
extern const unsigned char lut_E1_RX1TX2_FDD[3752][8];

#endif

#if (RX2_TX1_FDD_LUT_ON)
extern const unsigned char lut_D1_RX2TX1_FDD[3752][8];
extern const unsigned char lut_D2_RX2TX1_FDD[3752][8];
extern const unsigned char lut_E1_RX2TX1_FDD[3752][8];
#endif

#if (CH2_FDD_LUT_ON)
extern const unsigned char lut_D1_CH2_FDD[3752][8];
extern const unsigned char lut_D2_CH2_FDD[3752][8];
extern const unsigned char lut_E1_CH2_FDD[3752][8];
#endif

#if (CH1_CH2_FDD_LUT_ON)
extern const unsigned char lut_D1_CH1CH2_FDD[3752][8];
extern const unsigned char lut_D2_CH1CH2_FDD[3752][8];
extern const unsigned char lut_E1_CH1CH2_FDD[3752][8];
#endif

#if (CH1_TDD_LUT_ON)
// extern const unsigned char lut_D1_CH1_TDD[3752][8];
extern const unsigned char lut_D2_CH1_TDD[3752][8];
// extern const unsigned char lut_E1_CH1_TDD[3752][8];
extern const unsigned char lut_D1_CH1_TDD[1][1];
extern const unsigned char lut_E1_CH1_TDD[1][1];
#endif

#if (CH2_TDD_LUT_ON)
extern const unsigned char lut_D1_CH2_TDD[3752][8];
extern const unsigned char lut_D2_CH2_TDD[3752][8];
extern const unsigned char lut_E1_CH2_TDD[3752][8];
#endif

#if (CH1_CH2_TDD_LUT_ON)
extern const unsigned char lut_D1_CH1CH2_TDD[3752][8];
extern const unsigned char lut_D2_CH1CH2_TDD[3752][8];
extern const unsigned char lut_E1_CH1CH2_TDD[3752][8];
#endif

#if (HYBRID_FDD_CH1CH2_LUT_ON)
extern const unsigned char lut_D1_HYBRID_CH1CH2_FDD[3752][8];
extern const unsigned char lut_D2_HYBRID_CH1CH2_FDD[3752][8];
extern const unsigned char lut_E1_HYBRID_CH1CH2_FDD[3752][8];
#endif

#if (HYBRID_TDD_CH1CH2_LUT_ON)
extern const unsigned char lut_D1_HYBRID_CH1CH2_TDD[3752][8];
extern const unsigned char lut_D2_HYBRID_CH1CH2_TDD[3752][8];
extern const unsigned char lut_E1_HYBRID_CH1CH2_TDD[3752][8];
#endif

#if (HYBRID_FDD_CH1_LUT_ON)
// extern const unsigned char lut_D1_HYBRID_CH1_FDD[3752][8];
extern const unsigned char lut_D2_HYBRID_CH1_FDD[3752][8];
// extern const unsigned char lut_E1_HYBRID_CH1_FDD[3752][8];
extern const unsigned char lut_D1_HYBRID_CH1_FDD[1][1];
extern const unsigned char lut_E1_HYBRID_CH1_FDD[1][1];
#endif

#if (HYBRID_FDD_CH2_LUT_ON)
extern const unsigned char lut_D1_HYBRID_CH2_FDD[3752][8];
extern const unsigned char lut_D2_HYBRID_CH2_FDD[3752][8];
extern const unsigned char lut_E1_HYBRID_CH2_FDD[3752][8];
#endif

#endif /* LOAD_LUT_FROM_BIN_FILE */

int32_t gc080x_spi_write(uint32_t val);
int32_t gc080x_spi_read(uint32_t val);

// int32_t spi_init(uint32_t device_id,
// 				 uint8_t  clk_pha,
// 				 uint8_t  clk_pol);

void init_platform();
void cleanup_platform();

#endif
