#include "common.h"
#include "sleep.h"
#include "venusmmap.h"
#include "w25q128_driver.h"

#define PAGESIZE 256
uint8_t test_buf[(PAGESIZE * 64)];

void spi_std_rs_fast_4(uint32_t address, uint16_t num, uint16_t test_buf_off, uint8_t* data_array);
void write_to_sram(uint32_t address, uint16_t num, uint16_t test_buf_off, uint8_t* data_array);
void display_data_array(uint32_t offset, uint32_t num, uint8_t* data_array);
int32_t spi_send_receive_without_config(uint32_t spi_base, void* SendBufPtr, void* RecvBufPtr, uint32_t size);

void gpio_out_init(uint32_t gpio_addr) {
  REG_WRITE(gpio_addr + GPIO_DIR_OFFSET, 0xFFFFFFFF);
}

void boot_start(uint32_t gpio_addr) {
  REG_WRITE(gpio_addr + GPIO_DATA_OFFSET, 0b1);
}

void boot_finish(uint32_t gpio_addr) {
  REG_WRITE(gpio_addr + GPIO_DATA_OFFSET, 0b10);
}

void boot_error(uint32_t gpio_addr) {
  REG_WRITE(gpio_addr + GPIO_DATA_OFFSET, 0b100);
}

void flash_move(uint32_t flash_addr, uint32_t sram_addr, uint32_t byte) {
  WRITE_BURST_32(GC0802_PG_ADDR, GC0802_PG_PWREN_OFFSET, 0x1);
  READ_BURST_32(GC0802_PG_ADDR, GC0802_PG_RTC_EOI_OFFSET);
  WRITE_BURST_32(GC0802_PG_ADDR, GC0802_PG_RTC_CCR_OFFSET, 0b10);

  gpio_out_init(GC0802_GPIO_ADDR);
  boot_start(GC0802_GPIO_ADDR);
  // set chip select
  WRITE_BURST_32(GC0802_DEVCTRL_ADDR, GC0802_IOPAD_CFG_OFFSET + GC0802_IOPAD_FLASH_CSn * 0x4, READ_BURST_32(GC0802_DEVCTRL_ADDR, GC0802_IOPAD_CFG_OFFSET + GC0802_IOPAD_FLASH_CSn * 0x4) & 0xFFFFFFF6);
  WRITE_BURST_32(GC0802_DEVCTRL_ADDR, GC0802_IOPAD_CFG_OFFSET + GC0802_IOPAD_FLASH_CLK * 0x4, READ_BURST_32(GC0802_DEVCTRL_ADDR, GC0802_IOPAD_CFG_OFFSET + GC0802_IOPAD_FLASH_CLK * 0x4) & 0xFFFFFFF6);
  // QE = 0 (fixed)
  // GPIO Init
  // set CS = 1, WPn = 1, HOLDn = 1, Std SPI Rec On
  CSn_Off;
  Std_Rec_On;
  Only_Write_Off;
  Only_Read_Off;
  IO2_3_High_On;
  // initialize spi and configure spi mode 0
  uint32_t ret;
  ret = boot_spi_init(GC0802_FLASH_SPI_ADDR);
  dw_spi_disable_slave(GC0802_FLASH_SPI_ADDR, 0);  // 和0号设备通信
  ret = boot_spi_cp_format(GC0802_FLASH_SPI_ADDR, SPI_FORMAT_CPOL0_CPHA0);
  ret = boot_spi_mode(GC0802_FLASH_SPI_ADDR, SPI_MASTER);
  ret = boot_spi_baud(GC0802_FLASH_SPI_ADDR, SPI0_CLK_OUT);  // baudr的低0bit无法写入，分频必须是2的倍数
  // printf("Spi Init Finish\n");

  boot_spi_set_std_mode(GC0802_FLASH_SPI_ADDR, 0b00, no_address, no_instruction, no_wait_cycles, SPI_FRAME_LEN_8, 0);
  // display("Read JEDEC ID");
  uint8_t manufactuer;
  uint8_t id1;
  uint8_t id2;
  spi_JEDEC_id(&manufactuer, &id1, &id2);
  // display("Read Manufacturer's ID");
  spi_rd_id(0, &id1, &id2);
  spi_rd_id(1, &id1, &id2);
  // display("Read Sector Fast");

  /* set tx rx mode*/
  dw_spi_disable(GC0802_FLASH_SPI_ADDR);
  dw_spi_set_tx_rx_mode(GC0802_FLASH_SPI_ADDR);
  dw_spi_config_tx_fifo_threshold(GC0802_FLASH_SPI_ADDR, DW_DEFAULT_SPI_TXFIFO_LV);
  dw_spi_config_rx_fifo_threshold(GC0802_FLASH_SPI_ADDR, DW_DEFAULT_SPI_RXFIFO_LV);
  dw_spi_enable(GC0802_FLASH_SPI_ADDR);
  boot_spi_set_std_mode(GC0802_FLASH_SPI_ADDR, 0b00, no_address, no_instruction, no_wait_cycles, SPI_FRAME_LEN_8, 0);

  uint8_t data_array[PAGESIZE];
  uint32_t length = (byte - 1) / 0x100 + 1;
  // // printf("length=%d\n", length);
  // uint32_t Begin_Time, End_Time, User_Time;
  // Begin_Time = get_time((long*)0);

  // spi_std_rs_fast_4(0, 8, 0, data_array);
  // for (int i = 0; i < 8; i++) {
  //   printf("8byte data[%d]=%p\n", i, data_array[i]);
  // }

  // for (int i = 0; i < PAGESIZE; i++) {
  //   data_array[i] = (uint8_t)(i + 1);
  // }

  // while (1) {
  for (int i = 0; i < length; ++i) {
    spi_std_rs_fast_4(i * 0x100 + 0x8 + flash_addr, PAGESIZE, 0, data_array);
    write_to_sram(i * 0x100 + sram_addr, PAGESIZE, 0, data_array);
  }
  // spi_std_rs_fast_4(0x8 + flash_addr, byte, 0, sram_addr);

  // End_Time  = get_time((long*)0);
  // User_Time = End_Time - Begin_Time;
  // display("User_Time: %d", User_Time);
  // display("User_Time: %x", User_Time);
}

// 一次至少读 4 byte
void spi_std_rs_fast_4(uint32_t address, uint16_t num, uint16_t test_buf_off, uint8_t* data_array) {
  uint32_t temp;
  uint32_t null = 0;
  CSn_On;
  output_dut_byte(CMD_READ_DATA_FAST);
  output_dut_byte((address >> 16) & 0xFF);
  output_dut_byte((address >> 8) & 0xFF);
  output_dut_byte((address >> 0) & 0xFF);
  output_dut_byte(0);

  /* set tx rx mode*/
  dw_spi_disable(GC0802_FLASH_SPI_ADDR);
  dw_spi_set_tx_rx_mode(GC0802_FLASH_SPI_ADDR);
  dw_spi_config_tx_fifo_threshold(GC0802_FLASH_SPI_ADDR, DW_DEFAULT_SPI_TXFIFO_LV);
  dw_spi_config_rx_fifo_threshold(GC0802_FLASH_SPI_ADDR, DW_DEFAULT_SPI_RXFIFO_LV);
  dw_spi_set_dfs_32(GC0802_FLASH_SPI_ADDR, 32);
  dw_spi_enable(GC0802_FLASH_SPI_ADDR);

  Std_Rec_On;
  for (int x = 0; x < num; x = x + 4) {
    spi_send_receive_without_config(GC0802_FLASH_SPI_ADDR, &null, &temp, 1);
    data_array[x + 0 + test_buf_off] = ((temp >> 24) & 0xFF);
    data_array[x + 1 + test_buf_off] = ((temp >> 16) & 0xFF);
    data_array[x + 2 + test_buf_off] = ((temp >> 8) & 0xFF);
    data_array[x + 3 + test_buf_off] = ((temp >> 0) & 0xFF);
    // printf("%x\n", temp);
  }
  Std_Rec_Off;

  dw_spi_disable(GC0802_FLASH_SPI_ADDR);
  dw_spi_set_dfs_32(GC0802_FLASH_SPI_ADDR, 8);
  dw_spi_enable(GC0802_FLASH_SPI_ADDR);
  // display("Read Page Fast. (Address = %x, Num = %x)",address,num);
  CSn_Off;
}

void write_to_sram(uint32_t address, uint16_t num, uint16_t test_buf_off, uint8_t* data_array) {
  for (int x = 0; x < num / 4; x = x + 1) {
    REG_WRITE(address + 0x4 * x, (data_array[0x4 * x + test_buf_off] << 24) | (data_array[0x4 * x + 1 + test_buf_off] << 16) | (data_array[0x4 * x + 2 + test_buf_off] << 8) | (data_array[0x4 * x + 3 + test_buf_off] << 0));
  }
}

void display_data_array(uint32_t offset, uint32_t num, uint8_t* data_array) {
  uint32_t temp;
  temp = (offset & 16);
  for (int col = 0; col < temp; ++col) {
    if (col == 0)
      write("%x - ", offset - temp);
    if (!(col % 8) && (col % 16))
      write("- ");
    else
      write("   ");
  }
  for (int col = offset; col < num + offset; ++col) {
    if (!(col % 8) && (col % 16))
      write("- ");
    if (col % 16)
      write("%h ", data_array[col]);
    else
      write("\n%x - %h ", col, data_array[col]);
  }
  write("\n");
}

int32_t spi_send_receive_without_config(uint32_t spi_base, void* SendBufPtr, void* RecvBufPtr, uint32_t size) {
  uint32_t count = 0U;
  int32_t ret    = 0;
  uint32_t value;
  uint32_t tx_size;
  uint32_t rx_size;
  uint32_t* tx_data;
  uint32_t* rx_data;
  uint32_t current_size;

  tx_data = (uint32_t*)SendBufPtr;
  tx_size = size;
  rx_data = (uint32_t*)RecvBufPtr;
  rx_size = size;

  dw_spi_enable_slave(GC0802_FLASH_SPI_ADDR, 0);  // 和0号设备通信
  /* transfer loop */
  while ((tx_size > 0U) || (rx_size > 0U)) {
    /* process tx fifo empty */
    if (tx_size > 0U) {
      current_size = DW_MAX_SPI_TXFIFO_LV - dw_spi_get_tx_fifo_level(spi_base);

      if (current_size > tx_size) {
        current_size = tx_size;
      }

      while (current_size--) {
        value = (uint32_t)(*(uint32_t*)tx_data);
        dw_spi_transmit_data(spi_base, value);
        tx_data += 1;
        count += 1U;
        tx_size--;
      }
    }

    /* process rx fifo not empty */
    if (rx_size > 0U) {
      current_size = dw_spi_get_rx_fifo_level(spi_base);

      if (current_size > rx_size) {
        current_size = rx_size;
      }

      while (current_size--) {
        if (RecvBufPtr == NULL) {
          // 这时候的值不能接收
          dw_spi_receive_data(spi_base);
        } else {
          *(uint32_t*)rx_data = (uint32_t)dw_spi_receive_data(spi_base);
          rx_data += 1;
        }
        rx_size--;
      }
    }
  }

  /* wait end of transcation */
  while (dw_spi_get_status(spi_base) & DW_SPI_SR_BUSY)
    ;

  if (ret >= 0) {
    ret = (int32_t)count;
  }

  dw_spi_disable_slave(GC0802_FLASH_SPI_ADDR, 0);  // 和0号设备通信
  return ret;
}

