#ifndef __W25Q128_DRIVER_H
#define __W25Q128_DRIVER_H

/*
 * 芯片类型：W25Q128 是 Winbond 公司的 SPI NOR Flash 存储芯片。
 * 容量：128 Mbit（16 MB）。
 * 接口：SPI（Serial Peripheral Interface）。
 * 用途：用于存储固件、配置数据、日志数据或其他非易失性数据，在嵌入式系统中很常见
 */

#include "config.h"
#include "sleep.h"
#include "spi.h"
#include "ulib.h"
#include "venusmmap.h"
#include <stdbool.h>

#define W25Q128JVSIQ 1

#define ENABLE_UART_DEBUG 1  // uart debug

#define CMD_WRITE_DISABLE              0x04
#define CMD_WRITE_ENABLE               0x06
#define CMD_READ_STATUS                0x05
#define CMD_WRITE_STATUS               0x01
#define CMD_READ_STATUS2               0x35
#define CMD_WRITE_STATUS2              0x31
#define CMD_READ_STATUS3               0x15
#define CMD_WRITE_STATUS3              0x11
#define CMD_READ_DATA                  0x03
#define CMD_READ_DATA_FAST             0x0B
#define CMD_READ_DATA_FAST_WRAP        0x0C
#define CMD_READ_DATA_FAST_DTR         0x0D
#define CMD_READ_DATA_FAST_DTR_WRAP    0x0E
#define CMD_READ_DATA_FAST_DUAL        0x3B
#define CMD_READ_DATA_FAST_DUAL_IO     0xBB
#define CMD_READ_DATA_FAST_DUAL_IO_DTR 0xBD
#define CMD_READ_DATA_FAST_QUAD        0x6B
#define CMD_READ_DATA_FAST_QUAD_IO     0xEB
#define CMD_READ_DATA_FAST_QUAD_IO_DTR 0xED
#define CMD_PAGE_PROGRAM               0x02
#define CMD_PAGE_PROGRAM_QUAD          0x32
#define CMD_BLOCK_ERASE                0xD8
#define CMD_HALF_BLOCK_ERASE           0x52
#define CMD_SECTOR_ERASE               0x20
#define CMD_BULK_ERASE                 0xC7
#define CMD_BULK_ERASE2                0x60
#define CMD_DEEP_POWERDOWN             0xB9
#define CMD_READ_SIGNATURE             0xAB
#define CMD_READ_ID                    0x90
#define CMD_READ_ID_DUAL               0x92
#define CMD_READ_ID_QUAD               0x94
#define CMD_READ_JEDEC_ID              0x9F
#define CMD_READ_UNIQUE_ID             0x4B
#define CMD_SUSPEND                    0x75
#define CMD_RESUME                     0x7A
#define CMD_SET_BURST_WRAP             0x77
#define CMD_MODE_RESET                 0xFF
#define CMD_DISABLE_QPI                0xFF
#define CMD_ENABLE_QPI                 0x38
#define CMD_ENABLE_RESET               0x66
#define CMD_CHIP_RESET                 0x99
#define CMD_SET_READ_PARAM             0xC0
#define CMD_SREG_PROGRAM               0x42
#define CMD_SREG_ERASE                 0x44
#define CMD_SREG_READ                  0x48
#define CMD_WRITE_ENABLE_VSR           0x50
#define CMD_READ_SFDP                  0x5A
#define CMD_INDIVIDUAL_LOCK            0x36
#define CMD_INDIVIDUAL_UNLOCK          0x39
#define CMD_READ_BLOCK_LOCK            0x3D
#define CMD_GLOBAL_BLOCK_LOCK          0x7E
#define CMD_GLOBAL_BLOCK_UNLOCK        0x98
#define READ_JEDEC_ID_CMD              0x9F
#define WRITE_STATUS                   0x01
#define READ_STATU_REGISTER_1          0x05
#define READ_STATU_REGISTER_2          0x35
#define READ_DATA_CMD                  0x03
#define WRITE_ENABLE_CMD               0x06
#define WRITE_DISABLE_CMD              0x04
#define SECTOR_ERASE_CMD               0x20
#define CHIP_ERASE_CMD                 0xC7
#define PAGE_PROGRAM_CMD               0x02

#define PAGESIZE 256

#define FLASH_TEST_ENABLE

// Status register definitions
#define STATUS_HLD_RST 0x800000
#define STATUS_DRV1    0x400000
#define STATUS_DRV0    0x200000
#define STATUS_WPS     0x040000
#define STATUS_SUS     0x008000
#define STATUS_CMP     0x004000
#define STATUS_LB3     0x002000
#define STATUS_LB2     0x001000
#define STATUS_LB1     0x000800
#define STATUS_QE      0x000200
#define STATUS_SRL     0x000100
#define STATUS_SRP     0x000080
#define STATUS_SEC     0x000040
#define STATUS_TB      0x000020
#define STATUS_BP2     0x000010
#define STATUS_BP1     0x000008
#define STATUS_BP0     0x000004
#define STATUS_WEL     0x000002
#define STATUS_WIP     0x000001

#define HLD_RST 23
#define DRV1    22
#define DRV0    21
#define WPS     18
#define SUS     15
#define CMPB    14
#define LB3     13
#define LB2     12
#define LB1     11
#define QE      9
#define SRL     8
#define SRP     7
#define SEC     6
#define TB      5
#define BP2     4
#define BP1     3
#define BP0     2
#define WEL     1
#define WIP     0

#ifndef ASIC_SIM
#define display(fmt, ...)       \
  do {                          \
    printf(fmt, ##__VA_ARGS__); \
    printf("\n");               \
  } while (0)
#else
#define display(fmt, ...) \
  do {                    \
  } while (0)
#endif

#define write(fmt, ...)         \
  do {                          \
    printf(fmt, ##__VA_ARGS__); \
  } while (0)

#define IO2_3_High_On                                                                                                                                           \
  do {                                                                                                                                                          \
    WRITE_BURST_32(GC0802_FLASH_SPI_ADDR, GC0802_FLASH_SPI_CTRL_REG_OFFSET, READ_BURST_32(GC0802_FLASH_SPI_ADDR, GC0802_FLASH_SPI_CTRL_REG_OFFSET) | 0b010000); \
  } while (0)

#define IO2_3_High_Off                                                                                                                                          \
  do {                                                                                                                                                          \
    WRITE_BURST_32(GC0802_FLASH_SPI_ADDR, GC0802_FLASH_SPI_CTRL_REG_OFFSET, READ_BURST_32(GC0802_FLASH_SPI_ADDR, GC0802_FLASH_SPI_CTRL_REG_OFFSET) & 0b101111); \
  } while (0)

#define Only_Read_On                                                                                                                                            \
  do {                                                                                                                                                          \
    WRITE_BURST_32(GC0802_FLASH_SPI_ADDR, GC0802_FLASH_SPI_CTRL_REG_OFFSET, READ_BURST_32(GC0802_FLASH_SPI_ADDR, GC0802_FLASH_SPI_CTRL_REG_OFFSET) | 0b001000); \
  } while (0)

#define Only_Read_Off                                                                                                                                           \
  do {                                                                                                                                                          \
    WRITE_BURST_32(GC0802_FLASH_SPI_ADDR, GC0802_FLASH_SPI_CTRL_REG_OFFSET, READ_BURST_32(GC0802_FLASH_SPI_ADDR, GC0802_FLASH_SPI_CTRL_REG_OFFSET) & 0b110111); \
  } while (0)

#define Only_Write_On                                                                                                                                           \
  do {                                                                                                                                                          \
    WRITE_BURST_32(GC0802_FLASH_SPI_ADDR, GC0802_FLASH_SPI_CTRL_REG_OFFSET, READ_BURST_32(GC0802_FLASH_SPI_ADDR, GC0802_FLASH_SPI_CTRL_REG_OFFSET) | 0b000100); \
  } while (0)

#define Only_Write_Off                                                                                                                                          \
  do {                                                                                                                                                          \
    WRITE_BURST_32(GC0802_FLASH_SPI_ADDR, GC0802_FLASH_SPI_CTRL_REG_OFFSET, READ_BURST_32(GC0802_FLASH_SPI_ADDR, GC0802_FLASH_SPI_CTRL_REG_OFFSET) & 0b111011); \
  } while (0)

#define Std_Rec_On                                                                                                                                              \
  do {                                                                                                                                                          \
    WRITE_BURST_32(GC0802_FLASH_SPI_ADDR, GC0802_FLASH_SPI_CTRL_REG_OFFSET, READ_BURST_32(GC0802_FLASH_SPI_ADDR, GC0802_FLASH_SPI_CTRL_REG_OFFSET) | 0b000010); \
  } while (0)

#define Std_Rec_Off                                                                                                                                             \
  do {                                                                                                                                                          \
    WRITE_BURST_32(GC0802_FLASH_SPI_ADDR, GC0802_FLASH_SPI_CTRL_REG_OFFSET, READ_BURST_32(GC0802_FLASH_SPI_ADDR, GC0802_FLASH_SPI_CTRL_REG_OFFSET) & 0b111101); \
  } while (0)

#define CSn_On                                                                                                                                                  \
  do {                                                                                                                                                          \
    WRITE_BURST_32(GC0802_FLASH_SPI_ADDR, GC0802_FLASH_SPI_CTRL_REG_OFFSET, READ_BURST_32(GC0802_FLASH_SPI_ADDR, GC0802_FLASH_SPI_CTRL_REG_OFFSET) & 0b111110); \
  } while (0)

#define CSn_Off                                                                                                                                                 \
  do {                                                                                                                                                          \
    WRITE_BURST_32(GC0802_FLASH_SPI_ADDR, GC0802_FLASH_SPI_CTRL_REG_OFFSET, READ_BURST_32(GC0802_FLASH_SPI_ADDR, GC0802_FLASH_SPI_CTRL_REG_OFFSET) | 0b000001); \
  } while (0)

#define WPn_On                                                                                                                                                  \
  do {                                                                                                                                                          \
    WRITE_BURST_32(GC0802_FLASH_SPI_ADDR, GC0802_FLASH_SPI_CTRL_REG_OFFSET, READ_BURST_32(GC0802_FLASH_SPI_ADDR, GC0802_FLASH_SPI_CTRL_REG_OFFSET) | 0b100000); \
  } while (0)

#define WPn_Off                                                                                                                                                 \
  do {                                                                                                                                                          \
    WRITE_BURST_32(GC0802_FLASH_SPI_ADDR, GC0802_FLASH_SPI_CTRL_REG_OFFSET, READ_BURST_32(GC0802_FLASH_SPI_ADDR, GC0802_FLASH_SPI_CTRL_REG_OFFSET) & 0b011111); \
  } while (0)

#define null_reg (0x00)

extern bool qpi_mode;
extern uint8_t test_buf[(PAGESIZE * 64)];

void spi_enable_QPI();
void spi_disable_QPI();
void spi_wait_busy(uint32_t delay_time);
void spi_pd();
void spi_DTR();
void spi_rsig(uint8_t* signature);
void spi_runiqid(uint64_t* id);
void spi_sr(uint8_t* status);
void spi_sr2(uint8_t* status);
void spi_sr3(uint8_t* status);
void spi_wsr(uint16_t status);
void spi_wsr2(uint8_t status);
void spi_wsr3(uint8_t status);
void spi_write_security_page(uint8_t page, uint16_t num, uint16_t test_buf_offset);
void spi_read_security_page(uint8_t page, uint16_t num, uint16_t test_buf_off);
void spi_erase_security_page(uint8_t page);
void spi_wd();
void spi_we();
void spi_we_vsr();
void spi_ws(uint32_t address, uint16_t num, uint16_t test_buf_offset);
void spi_ws_quad(uint32_t address, uint16_t num, uint16_t test_buf_offset);
void spi_es(uint32_t address);
void spi_eb();
void spi_eb1();
void spi_eb2();
void spi_rs(uint32_t address, uint16_t num, uint16_t test_buf_off);
void spi_rs_fast(uint32_t address, uint16_t num, uint16_t test_buf_off);
void spi_rs_fast_DTR(uint32_t address, uint16_t num, uint16_t test_buf_off);
void spi_rs_fast_qpi(uint32_t address, uint16_t num, uint16_t test_buf_off);
void spi_JEDEC_id(uint8_t* manufacturer, uint8_t* id1, uint8_t* id2);
void spi_rd_id(uint32_t address, uint8_t* id1, uint8_t* id2);
void spi_rd_id_dual(uint32_t address, uint8_t* id1, uint8_t* id2);
void spi_rd_id_quad(uint32_t address, uint8_t* id1, uint8_t* id2);
void spi_suspend();
void spi_resume();
void spi_rs_dual(uint32_t address, uint16_t num, uint16_t test_buf_off);
void spi_rs_dualio(uint32_t address, uint16_t num, uint8_t mode, bool no_cmd, uint16_t test_buf_off);
void spi_rs_dualio_DTR(uint32_t address, uint16_t num, uint8_t mode, bool no_cmd, uint16_t test_buf_off);
void spi_rs_quad(uint32_t address, uint16_t num, uint16_t test_buf_off);
void spi_rs_quad_wrap_qpi(uint32_t address, uint16_t num, uint16_t test_buf_off);
void spi_rs_quadio(uint32_t address, uint16_t num, uint8_t mode, bool no_cmd, uint16_t test_buf_off);
void spi_rs_quadio_DTR(uint32_t address, uint16_t num, uint8_t mode, bool no_cmd, uint16_t test_buf_off);
void spi_set_wrap(uint8_t wrap);
void spi_set_qpi_param(uint8_t param);
void display_test_buf(uint32_t offset, uint32_t num);
void check_test_buf(uint32_t offset, uint32_t num, uint8_t mode);
void pattern_test_buf(uint32_t offset, uint32_t num, uint32_t pattern);
void uniq_test_buf();
void output_dut_byte(uint8_t data);
void output_dut_7clk(uint8_t data);
void output_dut_6clk(uint8_t data);
void output_dut_4clk(uint8_t data);
void output_dut_byte_DTR(uint8_t data);
void output_dut_byte_dual(uint8_t data);
void output_dut_byte_dual_DTR(uint8_t data);
void output_dut_byte_quad(uint8_t data);
void output_dut_byte_quad_DTR(uint8_t data);
void input_dut_byte_quad(uint8_t* data);
void input_dut_byte_quad_DTR(uint8_t* data);
void input_dut_byte_dual(uint8_t* data);
void input_dut_byte_dual_DTR(uint8_t* data);
void input_dut_byte(uint8_t* data);
void input_dut_byte_DTR(uint8_t* data);
void read_with_holdn(uint32_t address, uint16_t num, uint16_t test_buf_off);
void test_erase_suspend(uint32_t address);
void test_program_suspend(uint32_t address);
void spi_enable_reset();
void spi_chip_reset();
void spi_r_sfdp(uint32_t address, uint16_t num, uint16_t test_buf_off);
void spi_individual_unlock(uint32_t address);
void spi_read_block_lock(uint32_t address, uint8_t lockbit);
void spi_global_lock();
void spi_global_unlock();
void clear_display_buf(uint8_t data);

#endif