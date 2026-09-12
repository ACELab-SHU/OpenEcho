/******************************************************************************
*
* Copyright (C) 2010 - 2015 Xilinx, Inc.  All rights reserved.
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

#include "platform.h"
#include "config.h"
#include "driver.h"
#include "main_init.h"
#include "regs_init.h"
#include "utility.h"

void init_platform() {
}

void cleanup_platform() {
}

#if USE_DW_SPI == 1
#include "spi.h"

#else
#include "xspi.h"
/************************ Variables Definitions *******************************/
XSpi_Config* spi_config;
XSpi spi_instance;
/*****************************************************************************/
/**
* SPI Init
* Input: SPI Device ID & SPI Mode
* Output: Success
******************************************************************************/
int32_t xspi_init(uint32_t device_id,
                  uint8_t clk_pha,
                  uint8_t clk_pol) {

  uint32_t base_addr   = 0;
  uint32_t control_val = 0;

  uint32_t spi_options;
  XSpi_Initialize(&spi_instance, device_id);
  XSpi_Stop(&spi_instance);
  spi_config = XSpi_LookupConfig(device_id);
  base_addr  = spi_config->BaseAddress;
  printf("base_addr=%x\n", base_addr);
  XSpi_CfgInitialize(&spi_instance, spi_config, base_addr);
  if (clk_pha)
    spi_options = XSP_MASTER_OPTION |
                  XSP_CLK_PHASE_1_OPTION |
                  XSP_MANUAL_SSELECT_OPTION;
  else
    spi_options = XSP_MASTER_OPTION |
                  XSP_MANUAL_SSELECT_OPTION;
  XSpi_SetOptions(&spi_instance, spi_options);
  XSpi_Start(&spi_instance);
  XSpi_IntrGlobalDisable(&spi_instance);
  XSpi_SetSlaveSelect(&spi_instance, 1);

  return XST_SUCCESS;
}
#endif

/*****************************************************************************/
/**
* ad9361_spi_write
* Input: SPI Write Value
******************************************************************************/
int32_t gc080x_spi_write(uint32_t val) {
  uint8_t buf[3];
  int32_t ret;
  uint32_t cmd = val;

  buf[0] = cmd >> 16;   // 0x65
  buf[1] = cmd >> 8;    // 0x65c
  buf[2] = cmd & 0xFF;  // 0xcd
#if USE_DW_SPI == 1
  ret = spi_send(SPI0_BASEADDRESS, buf, 3);
#else
  ret = XSpi_Transfer(&spi_instance, buf, NULL, 3);
#endif
  if (ret < 0) {
    return ret;
  }

  return 0;
}

/*****************************************************************************/
/**
* ad9361_spi_read
* Input: SPI Read Value
******************************************************************************/
int32_t gc080x_spi_read(uint32_t val) {
  uint8_t buf[3], rbuf[3];
  int32_t ret;
  uint16_t cmd;

  cmd    = (val & 0xFFFF);
  buf[0] = cmd >> 8;
  buf[1] = cmd & 0xFF;

#if USE_DW_SPI == 1
  ret = spi_send_receive(SPI0_BASEADDRESS, buf, rbuf, 3);
#else
  ret = XSpi_Transfer(&spi_instance, buf, rbuf, 3);
#endif
  if (ret < 0)
    return ret;

  uint8_t bufrx = rbuf[2];

  return bufrx;
}

/*-------------------------------------------------------------------------------------------------*/
// hal fpga/spi read/write
/*-------------------------------------------------------------------------------------------------*/
unsigned int hal_fpga_read_reg(rf_chip_phy_t* phy, unsigned int reg) {
  unsigned int ret = 0;

#if VENDOR_SPI_RW
  ret = fpga_read_reg(reg);
#else
  // ret = XBram_ReadReg(SPI_RW_ADDDRESS, reg);
  ret = REG_READ(SPI_RW_ADDDRESS + reg);
#endif

  //if (phy->debug_on)
  LOG_MDEBUG(phy, SPI_RW_LOG, "FPGA_R: addr=0x%x, val=0x%x\n", reg, ret);
  return ret;
}

short hal_fpga_write_reg(rf_chip_phy_t* phy, unsigned int reg, unsigned int val) {
  short ret = 0;

#if VENDOR_SPI_RW
  ret = fpga_write_reg(reg, val);
#else
  REG_WRITE(SPI_RW_ADDDRESS + reg, val);
#endif

  //if (phy->debug_on)
  LOG_MDEBUG(phy, SPI_RW_LOG, "FPGA_W: addr=0x%x, val=0x%x\n", reg, val);
  return ret;
}

unsigned char hal_spi_read_reg(rf_chip_phy_t* phy, unsigned short reg) {
  unsigned char ret = 0;

#if VENDOR_SPI_RW
  ret = spi_read_reg(reg);
#else
  ret       = gc080x_spi_read(reg);
#endif

  //if (phy->debug_on)
  LOG_MDEBUG(phy, SPI_RW_LOG, "R: addr=0x%x, val=0x%x\n", reg, ret);

  return ret;
}

short hal_spi_write_reg(rf_chip_phy_t* phy, unsigned short reg, unsigned char val) {
  short ret = 0;

#if VENDOR_SPI_RW
  ret = spi_write_reg(reg, val);
#else
  ret       = gc080x_spi_write(((0x8000 | reg) << 8) | val);  // ((0x8000 | 0x65c) << 8) | 0xd
#endif

  //if (phy->debug_on)
  LOG_MDEBUG(phy, SPI_RW_LOG, "W: addr=0x%x, val=0x%x\n", reg, val);
  return ret;
}

void HAL_CONFIG_REGS(rf_chip_phy_t* phy, const reg_t* setting, short len) {
  short i;
  for (i = 0; i < len; i++) {
    if (setting[i].flag == FLG_READ)
      hal_spi_read_reg(phy, setting[i].addr);
    else if (setting[i].flag == FLG_WRITE)
      hal_spi_write_reg(phy, setting[i].addr, setting[i].val);
    else if (setting[i].flag == FLG_DELAY)
      CHIP_DELAY(setting[i].addr);
    else if (setting[i].flag == FLG_FPGA_R)
      hal_fpga_read_reg(phy, setting[i].addr);
    else if (setting[i].flag == FLG_FPGA_W)
      hal_fpga_write_reg(phy, setting[i].val, setting[i].addr);
  }
}

unsigned char HAL_REG_GET_BITS(rf_chip_phy_t* phy, unsigned short reg, REG_BIT bit, char cnt) {
  unsigned char val;
  unsigned char ret;

  val = hal_spi_read_reg(phy, reg);
  ret = GET_BITS(val, bit, cnt);

  return ret;
}

void HAL_REG_SET_BITS(rf_chip_phy_t* phy, unsigned short reg, REG_BIT bit, char cnt, unsigned char setval) {
  unsigned char val;

  val = hal_spi_read_reg(phy, reg);
  val = SET_BITS(val, bit, cnt, setval);
  hal_spi_write_reg(phy, reg, val);
}

char HAL_REG_GET_BIT(rf_chip_phy_t* phy, unsigned short reg, REG_BIT bit) {
  unsigned char val;
  char ret;

  val = hal_spi_read_reg(phy, reg);
  ret = GET_BIT(val, bit);

  return ret;
}

void HAL_REG_SET_BIT(rf_chip_phy_t* phy, unsigned short reg, REG_BIT bit) {
  unsigned char val;

  val = hal_spi_read_reg(phy, reg);
  val = SET_BIT(val, bit);
  hal_spi_write_reg(phy, reg, val);
}

void HAL_REG_CLR_BIT(rf_chip_phy_t* phy, unsigned short reg, REG_BIT bit) {
  unsigned char val;

  val = hal_spi_read_reg(phy, reg);
  val = CLR_BIT(val, bit);
  hal_spi_write_reg(phy, reg, val);
}

/*-------------------------------------------------------------------------------------------------*/
// bin handle
/*-------------------------------------------------------------------------------------------------*/
handle_t get_bin_handle(rf_chip_phy_t* phy, short* ok) {
  handle_t handle;
  char path[100];
  int index = 0;
  const char* const lut_bin_filename_d1[] =
      {
          "lut_D1_CH1_FDD.bin",
          "lut_D1_RX1TX2_FDD.bin",
          "lut_D1_RX2TX1_FDD.bin",
          "lut_D1_CH2_FDD.bin",
          "lut_D1_CH1CH2_FDD.bin",
          "lut_D1_CH1_TDD.bin",
          "lut_D1_CH2_TDD.bin",
          "lut_D1_CH1CH2_TDD.bin",

          "lut_D1_HYBRID_CH1_FDD.bin",
          "",
          "",
          "lut_D1_HYBRID_CH2_FDD.bin",
          "lut_D1_HYBRID_CH1CH2_FDD.bin",
          "",
          "",
          "lut_D1_HYBRID_CH1CH2_TDD.bin",
      };
  const char* const lut_bin_filename_d2[] =
      {
          "lut_D2_CH1_FDD.bin",
          "lut_D2_RX1TX2_FDD.bin",
          "lut_D2_RX2TX1_FDD.bin",
          "lut_D2_CH2_FDD.bin",
          "lut_D2_CH1CH2_FDD.bin",
          "lut_D2_CH1_TDD.bin",
          "lut_D2_CH2_TDD.bin",
          "lut_D2_CH1CH2_TDD.bin",

          "lut_D2_HYBRID_CH1_FDD.bin",
          "",
          "",
          "lut_D2_HYBRID_CH2_FDD.bin",
          "lut_D2_HYBRID_CH1CH2_FDD.bin",
          "",
          "",
          "lut_D2_HYBRID_CH1CH2_TDD.bin",
      };
  const char* const lut_bin_filename_e1[] =
      {
          "lut_E1_CH1_FDD.bin",
          "lut_E1_RX1TX2_FDD.bin",
          "lut_E1_RX2TX1_FDD.bin",
          "lut_E1_CH2_FDD.bin",
          "lut_E1_CH1CH2_FDD.bin",
          "lut_E1_CH1_TDD.bin",
          "lut_E1_CH2_TDD.bin",
          "lut_E1_CH1CH2_TDD.bin",

          "lut_E1_HYBRID_CH1_FDD.bin",
          "",
          "",
          "lut_E1_HYBRID_CH2_FDD.bin",
          "lut_E1_HYBRID_CH1CH2_FDD.bin",
          "",
          "",
          "lut_E1_HYBRID_CH1CH2_TDD.bin",
      };

#if HAVE_FS
  //open file
  memset(path, 0, sizeof(path));
  if (phy->config->use_bybrid_mode)
    index = 8 + phy->config->mode;
  else
    index = phy->config->mode;

  if (phy->config->chip_ver == 0)
    sprintf(path, "%s/D1/%s", LUT_BIN_PATH, lut_bin_filename_d1[index]);
  else if (phy->config->chip_ver == 1)
    sprintf(path, "%s/D2/%s", LUT_BIN_PATH, lut_bin_filename_d2[index]);
  else if (phy->config->chip_ver == 2)
    sprintf(path, "%s/E1/%s", LUT_BIN_PATH, lut_bin_filename_e1[index]);

  handle.fd = open(path, O_RDONLY);
  if (handle.fd < 0)
    *ok = 0;
  else
    *ok = 1;
#else
  handle.fd = -1;
  *ok       = 0;
#endif

  return handle;
}

void close_bin_handle(handle_t handle) {
#if HAVE_FS
  close(handle.fd);
#else
#endif
}

int read_flash(handle_t handle, int addr, unsigned char buffer[], int size) {
  int read_size = 0;
#if HAVE_FS
  lseek(handle.fd, addr, SEEK_SET);
  read_size = read(handle.fd, buffer, size);
#else
#endif
  return read_size;
}

/*-------------------------------------------------------------------------------------------------*/
// create task
/*-------------------------------------------------------------------------------------------------*/
#if LINUX_OS
int create_task(task_t* ptask, void* (*start_rtn)(void*), void* arg) {
  return pthread_create(&(ptask->pid), NULL, start_rtn, arg);
}

int stop_task(task_t* ptask) {
  struct timespec ts;
  clock_gettime(CLOCK_REALTIME, &ts);
  ts.tv_sec += 3;
  pthread_timedjoin_np(ptask->pid, NULL, &ts);
  return 0;
}
#else
int create_task(task_t* ptask, void* (*start_rtn)(void*), void* arg) {
  return 0;
}

int stop_task(task_t* ptask) {
  return 0;
}
#endif

/*-------------------------------------------------------------------------------------------------*/
// TDD fsm switch
/*-------------------------------------------------------------------------------------------------*/
void wire_control_en(rf_chip_phy_t* phy, short en, WIRE_CTRL_ENUM pulse) {
  if (en) {
    if (pulse == PULSE_CTRL) {
      hal_spi_write_reg(phy, 0x0cc, 0x80);
    } else {
      hal_spi_write_reg(phy, 0x0cc, 0x90);
      hal_spi_write_reg(phy, 0x0cc, 0xf0);
      CHIP_DELAY(10);

      hal_spi_write_reg(phy, 0x0cf, 0xfe);
      hal_spi_write_reg(phy, 0x0ce, 0x1f);

      hal_spi_write_reg(phy, 0x0d0, 0x00);
      hal_spi_write_reg(phy, 0x0d1, 0x00);
      hal_spi_write_reg(phy, 0x0d2, 0x00);
      hal_spi_write_reg(phy, 0x0d3, 0x00);
      hal_spi_write_reg(phy, 0x0d4, 0x00);
      hal_spi_write_reg(phy, 0x0d5, 0x00);
      hal_spi_write_reg(phy, 0x0d6, 0x00);
    }
  } else {
    HAL_REG_CLR_BIT(phy, 0x0cc, BIT7);
  }
}

int tdd_wait_to_alert(rf_chip_phy_t* phy) {
  hal_fpga_write_reg(phy, 0x280, 0);
  HAL_CONFIG_REGS(phy, G_REG_ARRAY_TDD_wirecontrol_WAIT_TO_ALERT, G_REG_ARRAY_TDD_wirecontrol_WAIT_TO_ALERT_LEN);
  return 0;
}

int tdd_alert_to_rx(rf_chip_phy_t* phy) {
  hal_fpga_write_reg(phy, 0x280, 1);

  if (phy->config->wire_ctrl == PULSE_CTRL)
    HAL_CONFIG_REGS(phy, G_REG_ARRAY_TDD_wirecontrol_ALERT_TO_RX, G_REG_ARRAY_TDD_wirecontrol_ALERT_TO_RX_LEN);
  else
    hal_fpga_write_reg(phy, 0x0c, 0x10);
  return 0;
}

int tdd_rx_to_wait(rf_chip_phy_t* phy) {
  HAL_CONFIG_REGS(phy, G_REG_ARRAY_TDD_wirecontrol_RX_TO_WAIT, G_REG_ARRAY_TDD_wirecontrol_RX_TO_WAIT_LEN);
  return 0;
}

int tdd_alert_to_tx(rf_chip_phy_t* phy) {
  HAL_CONFIG_REGS(phy, G_REG_ARRAY_TDD_wirecontrol_ALERT_TO_TX, G_REG_ARRAY_TDD_wirecontrol_ALERT_TO_TX_LEN);
  return 0;
}

int tdd_tx_to_wait(rf_chip_phy_t* phy) {
  HAL_CONFIG_REGS(phy, G_REG_ARRAY_TDD_wirecontrol_TX_TO_WAIT, G_REG_ARRAY_TDD_wirecontrol_TX_TO_WAIT_LEN);
  return 0;
}

int tdd_rx_to_alert_level(rf_chip_phy_t* phy) {
  //Rx to alert
  hal_fpga_write_reg(phy, 0x0c, 0x00);
  CHIP_UDELAY(1);
  return 0;
}

int tdd_tx_to_alert_level(rf_chip_phy_t* phy) {
  //Tx to alert
  hal_fpga_write_reg(phy, 0x0c, 0x20);
  CHIP_UDELAY(1);
  return 0;
}

int tdd_alert_to_tx_level(rf_chip_phy_t* phy) {
  //alert to Tx
  hal_fpga_write_reg(phy, 0x0c, 0x20);
  hal_fpga_write_reg(phy, 0x0c, 0x30);
  return 0;
}

int tdd_alert_to_rx_level(rf_chip_phy_t* phy) {
  //alert to Rx
  hal_fpga_write_reg(phy, 0x0c, 0x00);
  hal_fpga_write_reg(phy, 0x0c, 0x10);
  return 0;
}

/*-------------------------------------------------------------------------------------------------*/
// fpga setting & lvds delay cal
/*-------------------------------------------------------------------------------------------------*/
int fpga_and_chip_reset(rf_chip_phy_t* phy) {
  hal_fpga_write_reg(phy, 0x8, 0x0);
  CHIP_UDELAY(10);
  hal_fpga_write_reg(phy, 0x8, 0x3);
  hal_fpga_write_reg(phy, 0x34, 0x530);
  hal_fpga_write_reg(phy, 0x38, 0x0);
  return 0;
}

void chip_reset_withus(rf_chip_phy_t* phy, unsigned int delay) {
  unsigned char reg_val            = 0;
  unsigned char reg_val_low_level  = 0;
  unsigned char reg_val_high_level = 0;

  reg_val           = hal_fpga_read_reg(phy, 0x08);
  reg_val_low_level = reg_val & (~FPGA_CTRL_RST_PERL_MASK);
  hal_fpga_write_reg(phy, 0x8, reg_val_low_level);

  CHIP_UDELAY(delay);

  reg_val_high_level = reg_val | FPGA_CTRL_RST_PERL_MASK;
  hal_fpga_write_reg(phy, 0x8, reg_val_high_level);
}

int fpga_tail_set(rf_chip_phy_t* phy, short dir, short chn, short intflag) {
  if (intflag) {
    if ((phy->config->mode >= CH1_FDD) && (phy->config->mode <= CH1_CH2_FDD)) {
      if (phy->config->mode == CH1_CH2_FDD)
        hal_fpga_write_reg(phy, 0x3c, 0x4);
      else
        hal_fpga_write_reg(phy, 0x3c, 0x3);
      hal_fpga_write_reg(phy, 0xa8, 0x31);
      lvds_delay_cal(phy);
    } else {
      hal_fpga_write_reg(phy, 0x3c, 0x9);
      hal_fpga_write_reg(phy, 0xa8, 0x11);
    }
  } else {
    if ((phy->config->mode >= CH1_FDD) && (phy->config->mode <= CH1_CH2_FDD)) {
      if (dir == RX_DIR) {
        if (chn == TRX_CHN1)
          hal_fpga_write_reg(phy, 0xa8, 0x31);
        else
          hal_fpga_write_reg(phy, 0xa8, 0x32);
      }
    } else {
      if (dir == RX_DIR) {
        if (chn == TRX_CHN1)
          hal_fpga_write_reg(phy, 0xa8, 0x11);
        else
          hal_fpga_write_reg(phy, 0xa8, 0x12);
      } else {
        if (chn == TRX_CHN1)
          hal_fpga_write_reg(phy, 0xa8, 0x31);
        else
          hal_fpga_write_reg(phy, 0xa8, 0x32);
      }
    }
  }
}

int fn_fpga_set_if(rf_chip_phy_t* phy, short mode, short dif, short port, short rate, short flag) {
  short chn_flag, phy_flag;
  int r_data = 0, w_data;

  if ((mode == CH1_CH2_FDD) || (mode == CH1_CH2_TDD))
    chn_flag = 1;  //2r2t
  else
    chn_flag = 0;  //1r1t

  if (flag)
    chn_flag = 0;

  if ((mode >= 0) && (mode <= 4))
    phy_flag = 0;  //fdd
  else
    phy_flag = 1;  //tdd

  //   r_data = hal_fpga_read_reg(phy, 0x3c);
  r_data = (*(volatile uint32_t*)((0x1fff0800UL) + (0x13c)));
  r_data = r_data & 0xf8;

  if ((rate == 0) && (dif == 1) && (phy_flag == 0) && (port == 0) && (chn_flag == 0))
    w_data = 0;
  else if ((rate == 0) && (dif == 1) && (phy_flag == 1) && (port == 1) && (chn_flag == 0))
    w_data = 1;
  else if ((rate == 0) && (dif == 1) && (phy_flag == 1) && (port == 1) && (chn_flag == 1))
    w_data = 2;
  else if ((rate == 1) && (dif == 0) && (phy_flag == 0) && (port == 0) && (chn_flag == 0))
    w_data = 3;
  else if ((rate == 1) && (dif == 0) && (phy_flag == 0) && (port == 0) && (chn_flag == 1))
    w_data = 4;
  else if ((rate == 1) && (dif == 1) && (phy_flag == 0) && (port == 0) && (chn_flag == 0))
    w_data = 5;
  else if ((rate == 1) && (dif == 1) && (phy_flag == 1) && (port == 0) && (chn_flag == 1))
    w_data = 6;
  else if ((rate == 0) && (dif == 1) && (phy_flag == 1) && (port == 0) && (chn_flag == 0))
    w_data = 7;

  w_data = r_data | w_data;
  //   hal_fpga_write_reg(phy, 0x3c, w_data);
  *(volatile uint32_t*)((0x1fff0800UL) + (0x13c)) = (w_data);

  hal_fpga_write_reg(phy, 0x38, 0x4);
  hal_fpga_write_reg(phy, 0x34, 0x53B);
  hal_fpga_write_reg(phy, 0x2c, 0x1f);
  CHIP_DELAY(100);
  // hal_fpga_write_reg(phy, 0x8, 0x7);
  *(volatile uint32_t*)((0x1fff0800UL) + (0x108)) = (0x00000003);
  CHIP_DELAY(100);
  return 0;
}

short rx_lvds_delay_cal(rf_chip_phy_t* phy) {
#define RX_LVDS_RETRY_TIMES 10
  int i;
  int times;
  int pre_index = -1;
  short pass    = -1;
  unsigned int e8_val;
  unsigned int r3c_val;
  unsigned int rb8_val = 0;
  int pass_times[16]   = {0};

  r3c_val = hal_fpga_read_reg(phy, 0x3c);
  hal_spi_write_reg(phy, 0x70, 0x04);
  for (i = 0; i <= 0xf; i++) {
    for (times = 0; times < RX_LVDS_RETRY_TIMES; times++) {
      hal_spi_write_reg(phy, 0x6a, i << 4);
      hal_fpga_write_reg(phy, 0x10, 0x0);
      hal_fpga_write_reg(phy, 0x10, 0x1);
      CHIP_DELAY(10);
      e8_val = hal_fpga_read_reg(phy, 0xe8);
      if (r3c_val == 4) {
        rb8_val = hal_fpga_read_reg(phy, 0xb8);
      }
      LOG_MDEBUG(phy, LVDS_DELAY_CAL, "LVDS DELAY rx Cal, %s, reg[6a]=0x%0x, reg[e8] = 0x%0x, reg[b8]=0x%0x\n",
                 r3c_val == 4 ? "2R2T" : "1R1T",
                 i,
                 e8_val, rb8_val);
      if (r3c_val == 4) {
        if (((e8_val & 0x3) == 0x3) && ((rb8_val & 0x3) == 0x3)) {
          pass_times[i]++;
        }
      } else {
        if (((e8_val & 0x3) == 0x3)) {
          pass_times[i]++;
        }
      }
    }
    LOG_MDEBUG(phy, LVDS_DELAY_CAL, "LVDS DELAY rx Cal,  reg[6a]=0x%0x, pass_times=%d\n", (i << 4), pass_times[i]);
  }

  for (i = 0; i <= 0xf; i++) {
    if (pass_times[i] == RX_LVDS_RETRY_TIMES) {
      if (pre_index == -1) {
        pre_index = i;
      }
      if (i < 0xf && pass_times[i + 1] == RX_LVDS_RETRY_TIMES) {
        i++;
        hal_spi_write_reg(phy, 0x6a, i << 4);
        pass = 0;
        break;
      }
    }
  }

  if ((-1 == pass) && (pre_index != -1)) {
    hal_spi_write_reg(phy, 0x6a, pre_index << 4);
    pass = 0;
  }

  hal_spi_write_reg(phy, 0x70, 0x00);
  LOG_MDEBUG(phy, LVDS_DELAY_CAL, "LVDS DELAY rx Cal end,  %s\n", (0 == pass) ? "Pass" : "Fail");
  return pass;
}

short tx_lvds_delay_cal(rf_chip_phy_t* phy) {
#define TX_LVDS_RETRY_TIMES 10
  int i;
  unsigned int e8_val;
  unsigned int r34_val;
  unsigned int r38_val;
  unsigned int r3c_val;
  int times;
  int pre_index        = -1;
  short pass           = -1;
  unsigned int rb8_val = 0;
  int pass_times[16]   = {0};

  hal_spi_write_reg(phy, 0x70, 0x10);
  r34_val = hal_fpga_read_reg(phy, 0x34);
  r38_val = hal_fpga_read_reg(phy, 0x38);
  r3c_val = hal_fpga_read_reg(phy, 0x3c);
  hal_fpga_write_reg(phy, 0x34, 0x532);
  hal_fpga_write_reg(phy, 0x38, 0x0);
  for (i = 0; i <= 0xf; i++) {
    for (times = 0; times < TX_LVDS_RETRY_TIMES; times++) {
      hal_spi_write_reg(phy, 0x6b, i << 4);
      hal_fpga_write_reg(phy, 0x10, 0x0);
      hal_fpga_write_reg(phy, 0x10, 0x1);
      CHIP_DELAY(10);
      e8_val = hal_fpga_read_reg(phy, 0xe8);
      if (r3c_val == 4) {
        rb8_val = hal_fpga_read_reg(phy, 0xb8);
      }
      LOG_MDEBUG(phy, LVDS_DELAY_CAL, "LVDS DELAY tx Cal, %s, i=%d, reg[e8] = 0x%0x, reg[b8]=0x%0x\n",
                 r3c_val == 4 ? "2R2T" : "1R1T",
                 i,
                 e8_val, rb8_val);
      if (r3c_val == 4) {
        if (((e8_val & 0x3) == 0x3) && ((rb8_val & 0x3) == 0x3)) {
          pass_times[i]++;
        }
      } else {
        if (((e8_val & 0x3) == 0x3)) {
          pass_times[i]++;
        }
      }
    }
    LOG_MDEBUG(phy, LVDS_DELAY_CAL, "LVDS DELAY tx Cal,  reg[6b]=0x%0x, pass_times=%d\n", (i << 4), pass_times[i]);
  }
  for (i = 0; i <= 0xf; i++) {
    if (pass_times[i] == TX_LVDS_RETRY_TIMES) {
      if (pre_index == -1) {
        pre_index = i;
      }
      if (i < 0xf && pass_times[i + 1] == TX_LVDS_RETRY_TIMES) {
        i++;
        hal_spi_write_reg(phy, 0x6b, i << 4);
        pass = 0;
        break;
      }
    }
  }

  if ((-1 == pass) && (pre_index != -1)) {
    hal_spi_write_reg(phy, 0x6b, pre_index << 4);
    pass = 0;
  }

  //restore
  hal_spi_write_reg(phy, 0x70, 0x00);
  hal_fpga_write_reg(phy, 0x34, r34_val);
  hal_fpga_write_reg(phy, 0x38, r38_val);
  LOG_MDEBUG(phy, LVDS_DELAY_CAL, "LVDS DELAY tx Cal end,  %s\n", (0 == pass) ? "Pass" : "Fail");
  return pass;
}

void lvds_delay_cal(rf_chip_phy_t* phy) {
  unsigned int ra8_val;
  unsigned int r6a_val;
  unsigned int r6b_val;
  short pass = -1;

  if (1 == phy->config->lvds_cal_flag) {
    if (1 == phy->lvds_cal_flag) {
      hal_spi_write_reg(phy, 0x6a, phy->lvds_rx);
      hal_spi_write_reg(phy, 0x6b, phy->lvds_tx);
    } else {
      r6a_val = hal_spi_read_reg(phy, 0x6a);
      r6b_val = hal_spi_read_reg(phy, 0x6b);
      ra8_val = hal_fpga_read_reg(phy, 0xa8);
      hal_fpga_write_reg(phy, 0xa8, 0x33);
      pass = rx_lvds_delay_cal(phy);
      pass |= tx_lvds_delay_cal(phy);

      if (0 == pass) {
        /* backup cal result when cal success */
        phy->lvds_cal_flag = 1;
        phy->lvds_rx       = hal_spi_read_reg(phy, 0x6a);
        phy->lvds_tx       = hal_spi_read_reg(phy, 0x6b);
      } else {
        /* recover the register when cal fail */
        hal_spi_write_reg(phy, 0x6a, r6a_val);
        hal_spi_write_reg(phy, 0x6b, r6b_val);
        LOG_ERROR("%s REPORTED ERR in line %d cal fail.\n", __FUNCTION__, __LINE__);
      }
      hal_fpga_write_reg(phy, 0xa8, ra8_val);
    }
  }
}

int valid_channel(TRX_CHN_ENUM chn, CHIP_MODE_ENUM mode) {
  if (chn != TRX_CHN1 && chn != TRX_CHN2) {
    LOG_ERROR("%s in line %d REPORTED ERR Invalid TRX_CHN%d\n", __FUNCTION__, __LINE__, chn);
    return -1;
  }

  if ((chn == TRX_CHN2) &&
      (mode == CH1_FDD || mode == RX1_TX2_FDD || mode == CH1_TDD)) {
    LOG_ERROR("%s in line %d REPORTED ERR Invalid TRX_CHN%d with mode:%d \n",
              __FUNCTION__, __LINE__, chn, mode);
    return -1;
  }
  if ((chn == TRX_CHN1) &&
      (mode == CH2_FDD || mode == RX2_TX1_FDD || mode == CH2_TDD)) {
    LOG_ERROR("%s in line %d REPORTED ERR Invalid TRX_CHN%d with mode:%d \n",
              __FUNCTION__, __LINE__, chn, mode);
    return -1;
  }

  return 0;
}
