#include "w25q128_driver.h"

void spi_enable_QPI() {
  CSn_On;
  output_dut_byte(CMD_ENABLE_QPI);
  //display("QPI Enabled.");
  CSn_Off;
}

void spi_disable_QPI() {
  CSn_On;
  output_dut_byte(CMD_DISABLE_QPI);
  //display("QPI Disbled.");
  CSn_Off;
}

void spi_wait_busy(uint32_t delay_time) {
  uint8_t status;
  //display("Ready/Busy Polling with %d us delaytime between status polls", delay_time);
  spi_sr(&status);
  while (status & STATUS_WIP) {
    usleep(delay_time);
    spi_sr(&status);
  }
}

void spi_pd() {
  CSn_On;
  output_dut_byte(CMD_DEEP_POWERDOWN);
  //display("Deep Powerdown.");
  CSn_Off;
}

void spi_DTR() {
  CSn_On;
  output_dut_byte_quad_DTR(CMD_DEEP_POWERDOWN);
  //display("DTR input to device.");
  CSn_Off;
}

void spi_rsig(uint8_t* signature) {
  CSn_On;
  output_dut_byte(CMD_READ_SIGNATURE);
  output_dut_byte(null_reg);
  output_dut_byte(null_reg);
  output_dut_byte(null_reg);
  input_dut_byte(signature);

  //display("Read Signature/Release Deep Powerdown.(%h)", (*signature));
  CSn_Off;
}

void spi_runiqid(uint64_t* id) {
  uint8_t id1, id2, id3, id4, id5, id6, id7, id8;
  CSn_On;
  output_dut_byte(CMD_READ_UNIQUE_ID);
  output_dut_byte(null_reg);
  output_dut_byte(null_reg);
  output_dut_byte(null_reg);
  output_dut_byte(null_reg);
  input_dut_byte(&id1);
  input_dut_byte(&id2);
  input_dut_byte(&id3);
  input_dut_byte(&id4);
  input_dut_byte(&id5);
  input_dut_byte(&id6);
  input_dut_byte(&id7);
  input_dut_byte(&id8);
  (*id) = ((uint64_t)(id1) << 56) | ((uint64_t)(id2) << 48) | ((uint64_t)(id3) << 40) |
          ((uint64_t)(id4) << 32) | ((uint64_t)(id5) << 24) | ((uint64_t)(id6) << 16) |
          ((uint64_t)(id7) << 8) | ((uint64_t)(id8) << 0);

  //display("Read 64 bit unique ID. (%h).", (*id));
  CSn_Off;
}

void spi_sr(uint8_t* status) {
  CSn_On;
  output_dut_byte(CMD_READ_STATUS);
  input_dut_byte(status);

  //display("Read Status Register Low Byte ==> %h.", (*status));
  CSn_Off;
}

void spi_sr2(uint8_t* status) {
  CSn_On;
  output_dut_byte(CMD_READ_STATUS2);
  input_dut_byte(status);

  //display("Read Status Register 2 ==> %h.", (*status));
  CSn_Off;
}

void spi_sr3(uint8_t* status) {
  CSn_On;
  output_dut_byte(CMD_READ_STATUS3);
  input_dut_byte(status);

  //display("Read Status Register 3 ==> %h.", (*status));
  CSn_Off;
}

void spi_wsr(uint16_t status) {
  CSn_On;
  output_dut_byte(CMD_WRITE_STATUS);

#ifdef W25Q128JVSIQ
  output_dut_byte(status & 0xFF);
#else
  output_dut_byte(status & 0xFF);
  output_dut_byte((status >> 8) & 0xFF);
#endif

  //display("Write Status Register ==> %x.", status);
  CSn_Off;
}

void spi_wsr2(uint8_t status) {
  CSn_On;
  output_dut_byte(CMD_WRITE_STATUS2);
  output_dut_byte(status & 0xFF);

  //display("Write Status Register 2 ==> %h.", status);
  CSn_Off;
}

void spi_wsr3(uint8_t status) {
  CSn_On;
  output_dut_byte(CMD_WRITE_STATUS3);
  output_dut_byte(status & 0xFF);

  //display("Write Status Register 3 ==> %h.", status);
  CSn_Off;
}

void spi_write_security_page(uint8_t page, uint16_t num, uint16_t test_buf_offset) {
  uint32_t address;
  CSn_On;
  output_dut_byte(CMD_SREG_PROGRAM);
  address = (page & 0b11) << 12;

  output_dut_byte((address >> 16) & 0xFF);
  output_dut_byte((address >> 8) & 0xFF);
  output_dut_byte((address >> 0) & 0xFF);
  for (int x = 0; x < num; ++x) {
    output_dut_byte(test_buf[x + test_buf_offset]);
  }
  //display("Write to Security Page. (Page ID = %h)", page);
  CSn_Off;
}

void spi_read_security_page(uint8_t page, uint16_t num, uint16_t test_buf_off) {
  uint8_t temp;
  uint32_t address;
  CSn_On;
  address = (page & 0b11) << 12;

  output_dut_byte(CMD_SREG_READ);
  output_dut_byte((address >> 16) & 0xFF);
  output_dut_byte((address >> 8) & 0xFF);
  output_dut_byte((address >> 0) & 0xFF);
  input_dut_byte(&temp);  // Null
  for (int x = 0; x < num; ++x) {
    input_dut_byte(&temp);
    test_buf[x + test_buf_off] = temp;
  }
  //display("Read Security Page. (Page = %h, Num = %h)", page, num);
  CSn_Off;
}

void spi_erase_security_page(uint8_t page) {
  uint32_t address;
  CSn_On;
  address = (page & 0b11) << 12;

  output_dut_byte(CMD_SREG_ERASE);
  output_dut_byte((address >> 16) & 0xFF);
  output_dut_byte((address >> 8) & 0xFF);
  output_dut_byte((address >> 0) & 0xFF);
  //display("Erase Security Page. (Page = %h)", page);
  CSn_Off;
}

void spi_wd() {
  CSn_On;
  output_dut_byte(CMD_WRITE_DISABLE);
  //display("Write Disable.");
  CSn_Off;
}

void spi_we() {
  CSn_On;
  output_dut_byte(CMD_WRITE_ENABLE);
  //display("Write Enable.");
  CSn_Off;
}

void spi_we_vsr() {
  CSn_On;
  output_dut_byte(CMD_WRITE_ENABLE_VSR);
  //display("Write Enable VSR.");
  CSn_Off;
}

void spi_ws(uint32_t address, uint16_t num, uint16_t test_buf_offset) {
  CSn_On;
  output_dut_byte(CMD_PAGE_PROGRAM);
  output_dut_byte((address >> 16) & 0xFF);
  output_dut_byte((address >> 8) & 0xFF);
  output_dut_byte((address >> 0) & 0xFF);
  for (int x = 0; x < num; ++x) {
    output_dut_byte(test_buf[x + test_buf_offset]);
  }
  //display("Write to Page. (Byte Address = %x, Num = %x)", address, num);
  CSn_Off;
}

void spi_ws_quad(uint32_t address, uint16_t num, uint16_t test_buf_offset) {
  CSn_On;
  output_dut_byte(CMD_PAGE_PROGRAM_QUAD);
  output_dut_byte((address >> 16) & 0xFF);
  output_dut_byte((address >> 8) & 0xFF);
  output_dut_byte((address >> 0) & 0xFF);
  for (int x = 0; x < num; ++x) {
    output_dut_byte_quad(test_buf[x + test_buf_offset]);
  }
  //display("Write to Page Quad. (Byte Address = %x, Num = %x)", address, num);
  CSn_Off;
}

void spi_es(uint32_t address) {
  CSn_On;
  output_dut_byte(CMD_SECTOR_ERASE);
  output_dut_byte((address >> 16) & 0xFF);
  output_dut_byte((address >> 8) & 0xFF);
  output_dut_byte((address >> 0) & 0xFF);
  //display("Erase Sector. (Byte Address = %x)", address);
  CSn_Off;
}

void spi_eb() {
  CSn_On;
  output_dut_byte(CMD_BULK_ERASE);
  //display("Bulk Erase.");
  CSn_Off;
}

void spi_eb1() {
  CSn_On;
  output_dut_byte(CMD_BLOCK_ERASE);
  //display("64KB Block Erase.");
  CSn_Off;
}

void spi_eb2() {
  CSn_On;
  output_dut_byte(CMD_HALF_BLOCK_ERASE);
  //display("32KB Block Erase.");
  CSn_Off;
}

void spi_rs(uint32_t address, uint16_t num, uint16_t test_buf_off) {
  uint8_t temp;
  CSn_On;
  output_dut_byte(CMD_READ_DATA);
  output_dut_byte((address >> 16) & 0xFF);
  output_dut_byte((address >> 8) & 0xFF);
  output_dut_byte((address >> 0) & 0xFF);
  for (int x = 0; x < num; ++x) {
    input_dut_byte(&temp);
    test_buf[x + test_buf_off] = temp;
  }
  //display("Read Page. (Address = %x, Num = %x)", address, num);
  CSn_Off;
}

void spi_rs_fast(uint32_t address, uint16_t num, uint16_t test_buf_off) {
  uint8_t temp;
  CSn_On;
  output_dut_byte(CMD_READ_DATA_FAST);
  output_dut_byte((address >> 16) & 0xFF);
  output_dut_byte((address >> 8) & 0xFF);
  output_dut_byte((address >> 0) & 0xFF);
  output_dut_byte(0);
  for (int x = 0; x < num; ++x) {
    input_dut_byte(&temp);
    test_buf[x + test_buf_off] = temp;
  }
  //display("Read Page Fast. (Address = %x, Num = %x)", address, num);
  CSn_Off;
}

void spi_rs_fast_DTR(uint32_t address, uint16_t num, uint16_t test_buf_off) {
  uint8_t temp;
  CSn_On;
  output_dut_byte(CMD_READ_DATA_FAST_DTR);
  output_dut_byte((address >> 16) & 0xFF);
  output_dut_byte((address >> 8) & 0xFF);
  output_dut_byte((address >> 0) & 0xFF);
  output_dut_6clk(0);
  for (int x = 0; x < num; ++x) {
    input_dut_byte_DTR(&temp);
    test_buf[x + test_buf_off] = temp;
  }
  //display("Read Page Fast DTR. (Address = %x, Num = %x)", address, num);
  CSn_Off;
}

void spi_rs_fast_qpi(uint32_t address, uint16_t num, uint16_t test_buf_off) {
  uint8_t temp;
  CSn_On;
  output_dut_byte(CMD_READ_DATA_FAST);
  output_dut_byte((address >> 16) & 0xFF);
  output_dut_byte((address >> 8) & 0xFF);
  output_dut_byte((address >> 0) & 0xFF);
  output_dut_byte(0);
  for (int x = 0; x < num; ++x) {
    input_dut_byte_quad(&temp);
    test_buf[x + test_buf_off] = temp;
  }
  //display("Read Page Fast - QPI. (Address = %x, Num = %x)", address, num);
  CSn_Off;
}

void spi_JEDEC_id(uint8_t* manufacturer, uint8_t* id1, uint8_t* id2) {
  CSn_On;
  output_dut_byte(CMD_READ_JEDEC_ID);
  input_dut_byte(manufacturer);
  input_dut_byte(id1);
  input_dut_byte(id2);

  // //display("Read JEDEC ID. (Manufacturer = %h, ID = %h%h)", (*manufacturer), (*id1), (*id2));
  CSn_Off;
}

void spi_rd_id(uint32_t address, uint8_t* id1, uint8_t* id2) {
  CSn_On;
  output_dut_byte(CMD_READ_ID);
  output_dut_byte((address >> 16) & 0xFF);
  output_dut_byte((address >> 8) & 0xFF);
  output_dut_byte((address >> 0) & 0xFF);

  input_dut_byte(id1);
  input_dut_byte(id2);

  // //display("Read Manufacturers ID. (Address = %x, ID = %h%h)", address, (*id1), (*id2));
  CSn_Off;
}

void spi_rd_id_dual(uint32_t address, uint8_t* id1, uint8_t* id2) {
  CSn_On;
  output_dut_byte(CMD_READ_ID_DUAL);
  output_dut_byte_dual((address >> 16) & 0xFF);
  output_dut_byte_dual((address >> 8) & 0xFF);
  output_dut_byte_dual((address >> 0) & 0xFF);
  output_dut_byte_dual(0xFF);

  input_dut_byte_dual(id1);
  input_dut_byte_dual(id2);

  // //display("Read Manufacturers ID Dual. (Address = %x, ID = %h%h)", address, (*id1), (*id2));
  CSn_Off;
}

void spi_rd_id_quad(uint32_t address, uint8_t* id1, uint8_t* id2) {
  CSn_On;
  output_dut_byte(CMD_READ_ID_QUAD);
  output_dut_byte_quad((address >> 16) & 0xFF);
  output_dut_byte_quad((address >> 8) & 0xFF);
  output_dut_byte_quad((address >> 0) & 0xFF);
  output_dut_byte_quad(0xFF);
  output_dut_byte_quad(0x00);
  output_dut_byte_quad(0x00);

  input_dut_byte_quad(id1);
  input_dut_byte_quad(id2);

  // //display("Read Manufacturers ID Quad. (Address = %x, ID = %h%h)", address, (*id1), (*id2));
  CSn_Off;
}

void spi_suspend() {
  CSn_On;
  output_dut_byte(CMD_SUSPEND);
  //display("Suspend.");
  CSn_Off;
}

void spi_resume() {
  CSn_On;
  output_dut_byte(CMD_RESUME);
  //display("Resume.");
  CSn_Off;
}

void spi_rs_dual(uint32_t address, uint16_t num, uint16_t test_buf_off) {
  uint8_t temp;
  CSn_On;
  output_dut_byte(CMD_READ_DATA_FAST_DUAL);
  output_dut_byte((address >> 16) & 0xFF);
  output_dut_byte((address >> 8) & 0xFF);
  output_dut_byte((address >> 0) & 0xFF);
  output_dut_byte_dual(0xFF);
  output_dut_byte_dual(0x00);

  for (int x = 0; x < num; ++x) {
    input_dut_byte_dual(&temp);
    test_buf[x + test_buf_off] = temp;
  }

  //display("Read Page Dual. (Address = %x, Num = %x)", address, num);
  CSn_Off;
}

void spi_rs_dualio(uint32_t address, uint16_t num, uint8_t mode, bool no_cmd, uint16_t test_buf_off) {
  uint8_t temp;
  CSn_On;
  if (!no_cmd)
    output_dut_byte(CMD_READ_DATA_FAST_DUAL_IO);
  output_dut_byte_dual((address >> 16) & 0xFF);
  output_dut_byte_dual((address >> 8) & 0xFF);
  output_dut_byte_dual((address >> 0) & 0xFF);
  output_dut_byte_dual(mode);

  for (int x = 0; x < num; ++x) {
    input_dut_byte_dual(&temp);
    test_buf[x + test_buf_off] = temp;
  }

  if (no_cmd)
    ;//display("Read Page Dual IO - No CMD. (Address = %x, Num = %x, Mode = %h)", address, num, mode);
  else
    ;//display("Read Page Dual IO. (Address = %x, Num = %x, Mode = %h)", address, num, mode);
  CSn_Off;
}

void spi_rs_dualio_DTR(uint32_t address, uint16_t num, uint8_t mode, bool no_cmd, uint16_t test_buf_off) {
  uint8_t temp;
  CSn_On;
  if (!no_cmd)
    output_dut_byte(CMD_READ_DATA_FAST_DUAL_IO_DTR);
  output_dut_byte_dual_DTR((address >> 16) & 0xFF);
  output_dut_byte_dual_DTR((address >> 8) & 0xFF);
  output_dut_byte_dual_DTR((address >> 0) & 0xFF);
  output_dut_byte_dual_DTR(mode);
  output_dut_4clk(0);

  for (int x = 0; x < num; ++x) {
    input_dut_byte_dual_DTR(&temp);
    test_buf[x + test_buf_off] = temp;
  }

  if (no_cmd)
    ;//display("Read Page Dual IO DTR - No CMD. (Address = %x, Num = %x, Mode = %h)", address, num, mode);
  else
    ;//display("Read Page Dual IO DTR. (Address = %x, Num = %x, Mode = %h)", address, num, mode);
  CSn_Off;
}

void spi_rs_quad(uint32_t address, uint16_t num, uint16_t test_buf_off) {
  uint8_t temp;
  CSn_On;
  output_dut_byte(CMD_READ_DATA_FAST_QUAD);
  output_dut_byte((address >> 16) & 0xFF);
  output_dut_byte((address >> 8) & 0xFF);
  output_dut_byte((address >> 0) & 0xFF);
  output_dut_byte_quad(0xFF);
  output_dut_byte_quad(0x00);
  output_dut_byte_quad(0x00);
  output_dut_byte_quad(0x00);

  for (int x = 0; x < num; ++x) {
    input_dut_byte_quad(&temp);
    test_buf[x + test_buf_off] = temp;
  }

  //display("Read Page Quad. (Address = %x, Num = %x)", address, num);
  CSn_Off;
}

void spi_rs_quad_wrap_qpi(uint32_t address, uint16_t num, uint16_t test_buf_off) {
  uint8_t temp;
  CSn_On;
  output_dut_byte(CMD_READ_DATA_FAST_WRAP);
  output_dut_byte((address >> 16) & 0xFF);
  output_dut_byte((address >> 8) & 0xFF);
  output_dut_byte((address >> 0) & 0xFF);
  output_dut_byte(0x00);

  for (int x = 0; x < num; ++x) {
    input_dut_byte_quad(&temp);
    test_buf[x + test_buf_off] = temp;
  }

  //display("Read Page Quad Wrap QPI. (Address = %x, Num = %x)", address, num);
  CSn_Off;
}

void spi_rs_quadio(uint32_t address, uint16_t num, uint8_t mode, bool no_cmd, uint16_t test_buf_off) {
  uint8_t temp;
  CSn_On;
  if (!no_cmd)
    output_dut_byte(CMD_READ_DATA_FAST_QUAD_IO);
  output_dut_byte_quad((address >> 16) & 0xFF);
  output_dut_byte_quad((address >> 8) & 0xFF);
  output_dut_byte_quad((address >> 0) & 0xFF);
  output_dut_byte_quad(mode);

  input_dut_byte_quad(&temp);
  input_dut_byte_quad(&temp);

  for (int x = 0; x < num; ++x) {
    input_dut_byte_quad(&temp);
    test_buf[x + test_buf_off] = temp;
  }

  if (no_cmd)
    ;//display("Read Page Quad IO - No CMD. (Address = %x, Num = %x, Mode = %h)", address, num, mode);
  else
    ;//display("Read Page Quad IO. (Address = %x, Num = %x, Mode = %h)", address, num, mode);
  CSn_Off;
}

void spi_rs_quadio_DTR(uint32_t address, uint16_t num, uint8_t mode, bool no_cmd, uint16_t test_buf_off) {
  uint8_t temp;
  CSn_On;
  if (!no_cmd)
    output_dut_byte(CMD_READ_DATA_FAST_QUAD_IO_DTR);
  output_dut_byte_quad_DTR((address >> 16) & 0xFF);
  output_dut_byte_quad_DTR((address >> 8) & 0xFF);
  output_dut_byte_quad_DTR((address >> 0) & 0xFF);
  output_dut_byte_quad_DTR(mode);
  output_dut_7clk(0);

  for (int x = 0; x < num; ++x) {
    input_dut_byte_quad_DTR(&temp);
    test_buf[x + test_buf_off] = temp;
  }

  if (no_cmd)
    ;//display("Read Page Quad IO DTR - No CMD. (Address = %x, Num = %x, Mode = %h)", address, num, mode);
  else
    ;//display("Read Page Quad IO DTR. (Address = %x, Num = %x, Mode = %h)", address, num, mode);
  CSn_Off;
}

void spi_set_wrap(uint8_t wrap) {
  CSn_On;
  output_dut_byte(CMD_SET_BURST_WRAP);
  output_dut_byte_quad(0x00);
  output_dut_byte_quad(0x00);
  output_dut_byte_quad(0x00);
  output_dut_byte_quad(wrap);

  //display("Set Burst Wrap. (Wrap Value = %h)", wrap);
  CSn_Off;
}

void spi_set_qpi_param(uint8_t param) {
  CSn_On;
  output_dut_byte(CMD_SET_READ_PARAM);
  output_dut_byte(param);

  //display("Set QPI Read Param. (Param Value = %h)", param);
  CSn_Off;
}

void display_test_buf(uint32_t offset, uint32_t num) {
  uint32_t end = offset + num;

  for (uint32_t col = offset; col < end; ++col) {
    // 每16个字节换行，并打印地址
    if ((col - offset) % 16 == 0) {
      printf("\n0x%x - ", col);  // 地址十六进制
    }
    // 每8个字节插入分隔符
    if ((col - offset) % 8 == 0 && (col - offset) % 16 != 0) {
      printf("- ");
    }
    // 打印数据，如果小于0x10前面补0
    if (test_buf[col] < 0x10)
      printf("0x%x ", test_buf[col]);
    else
      printf("0x%x ", test_buf[col]);
  }
  printf("\n");
}

void check_test_buf(uint32_t offset, uint32_t num, uint8_t mode) {
  uint32_t end = offset + num;

  for (uint32_t col = offset; col < end; ++col) {
    if (mode == 0) { // increase
      if (test_buf[col] != col - offset) {
        printf("\nError at offset %x: except %x, get %x\n", col, 0xff, test_buf[col]);
      }
    } else if (mode == 1) { // all 0xff
      if (test_buf[col] != 0xff) {
        printf("\nError at offset %x: except %x, get %x\n", col, 0xff, test_buf[col]);
      }
    }
  }
  // printf("\n");
}

void pattern_test_buf(uint32_t offset, uint32_t num, uint32_t pattern) {
  for (int x = offset; x < num + offset; x = x + 4) {
    test_buf[x]     = (pattern >> 24) & 0xFF;
    test_buf[x + 1] = (pattern >> 16) & 0xFF;
    test_buf[x + 2] = (pattern >> 8) & 0xFF;
    test_buf[x + 3] = (pattern >> 0) & 0xFF;
  }
  //display("Set offset %x of test_buf to pattern %x for %x bytes.", offset, pattern, num);
}

void uniq_test_buf() {
  for (int x = 0; x < PAGESIZE; ++x) {
    test_buf[x] = x;
  }
  //display("Set test_buf to index pattern.");
}

void output_dut_byte(uint8_t data) {
  if (qpi_mode) {
    Std_Rec_Off;
    output_dut_byte_quad(data);
  } else {
    boot_spi_set_std_mode(GC0802_FLASH_SPI_ADDR, 0b00, no_address, no_instruction, no_wait_cycles, SPI_FRAME_LEN_8, 0);
    Std_Rec_On;
    boot_spi_send_receive(GC0802_FLASH_SPI_ADDR, &data, NULL, 1, SPI_FRAME_LEN_8);
    Std_Rec_Off;
  }
}

void output_dut_7clk(uint8_t data) {
  if (qpi_mode) {
    Std_Rec_Off;
    output_dut_byte_quad(data);
  } else {
    // 这里会多一个cycle, 当前 dw ip 导致的，暂时无法解决
    boot_spi_set_std_mode(GC0802_FLASH_SPI_ADDR, 0b00, no_address, no_instruction, no_wait_cycles, SPI_FRAME_LEN_8, 0);
    Std_Rec_On;
    boot_spi_send_receive(GC0802_FLASH_SPI_ADDR, &data, NULL, 1, SPI_FRAME_LEN_8);
    Std_Rec_Off;
  }
}

void output_dut_6clk(uint8_t data) {
  if (qpi_mode) {
    Std_Rec_Off;
    output_dut_byte_quad(data);
  } else {
    // 这里会多 2 个cycle, 当前 dw ip 导致的，暂时无法解决
    boot_spi_set_std_mode(GC0802_FLASH_SPI_ADDR, 0b00, no_address, no_instruction, no_wait_cycles, SPI_FRAME_LEN_8, 0);
    Std_Rec_On;
    boot_spi_send_receive(GC0802_FLASH_SPI_ADDR, &data, NULL, 1, SPI_FRAME_LEN_8);
    Std_Rec_Off;
  }
}

void output_dut_4clk(uint8_t data) {
  if (qpi_mode) {
    Std_Rec_Off;
    output_dut_byte_quad(data);
  } else {
    boot_spi_set_std_mode(GC0802_FLASH_SPI_ADDR, 0b00, no_address, no_instruction, no_wait_cycles, SPI_FRAME_LEN_4, 0);
    Std_Rec_On;
    boot_spi_send_receive(GC0802_FLASH_SPI_ADDR, &data, NULL, 1, SPI_FRAME_LEN_4);
    Std_Rec_Off;
  }
}

void output_dut_byte_DTR(uint8_t data) {
  // 当前 dw ip 不支持
  if (qpi_mode) {
    Std_Rec_Off;
    output_dut_byte_quad(data);
  } else {
    boot_spi_set_std_mode(GC0802_FLASH_SPI_ADDR, 0b00, no_address, no_instruction, no_wait_cycles, SPI_FRAME_LEN_8, 0);
    Std_Rec_On;
    boot_spi_send_receive(GC0802_FLASH_SPI_ADDR, &data, NULL, 1, SPI_FRAME_LEN_8);
    Std_Rec_Off;
  }
}

void output_dut_byte_dual(uint8_t data) {
  boot_spi_set_dual_mode(GC0802_FLASH_SPI_ADDR, 0b10, _8_bits_addr_l, no_instruction, no_wait_cycles, SPI_FRAME_LEN_8, 0);
  Std_Rec_Off;
  Only_Write_On;
  boot_spi_send_receive(GC0802_FLASH_SPI_ADDR, &data, NULL, 1, SPI_FRAME_LEN_8);
  Only_Write_Off;
}

void output_dut_byte_dual_DTR(uint8_t data) {
  // 当前 dw ip 不支持
  boot_spi_set_dual_mode(GC0802_FLASH_SPI_ADDR, 0b10, _8_bits_addr_l, no_instruction, no_wait_cycles, SPI_FRAME_LEN_8, 0);
  Std_Rec_Off;
  Only_Write_On;
  boot_spi_send_receive(GC0802_FLASH_SPI_ADDR, &data, NULL, 1, SPI_FRAME_LEN_8);
  Only_Write_Off;
}

void output_dut_byte_quad(uint8_t data) {
  boot_spi_set_quad_mode(GC0802_FLASH_SPI_ADDR, 0b10, _8_bits_addr_l, no_instruction, no_wait_cycles, SPI_FRAME_LEN_8, 0);
  Std_Rec_Off;
  Only_Write_On;
  boot_spi_send_receive(GC0802_FLASH_SPI_ADDR, &data, NULL, 1, SPI_FRAME_LEN_8);
  Only_Write_Off;
}

void output_dut_byte_quad_DTR(uint8_t data) {
  // 当前 dw ip 不支持
  boot_spi_set_quad_mode(GC0802_FLASH_SPI_ADDR, 0b10, _8_bits_addr_l, no_instruction, no_wait_cycles, SPI_FRAME_LEN_8, 0);
  Std_Rec_Off;
  Only_Write_On;
  boot_spi_send_receive(GC0802_FLASH_SPI_ADDR, &data, NULL, 1, SPI_FRAME_LEN_8);
  Only_Write_Off;
}

void input_dut_byte_quad(uint8_t* data) {
  uint8_t temp;
  temp = 0x00;
  boot_spi_set_quad_mode(GC0802_FLASH_SPI_ADDR, 0b10, _8_bits_addr_l, no_instruction, no_wait_cycles, SPI_FRAME_LEN_8, 0);
  Std_Rec_Off;
  Only_Read_On;
  boot_spi_send_receive(GC0802_FLASH_SPI_ADDR, &temp, data, 1, SPI_FRAME_LEN_8);
  Only_Read_Off;
}

void input_dut_byte_quad_DTR(uint8_t* data) {
  uint8_t temp;
  temp = 0x00;
  boot_spi_set_quad_mode(GC0802_FLASH_SPI_ADDR, 0b10, _8_bits_addr_l, no_instruction, no_wait_cycles, SPI_FRAME_LEN_8, 0);
  Std_Rec_Off;
  Only_Read_On;
  boot_spi_send_receive(GC0802_FLASH_SPI_ADDR, &temp, data, 1, SPI_FRAME_LEN_8);
  Only_Read_Off;
}

void input_dut_byte_dual(uint8_t* data) {
  uint8_t temp;
  temp = 0x00;
  boot_spi_set_dual_mode(GC0802_FLASH_SPI_ADDR, 0b10, _8_bits_addr_l, no_instruction, no_wait_cycles, SPI_FRAME_LEN_8, 0);
  Std_Rec_Off;
  Only_Read_On;
  boot_spi_send_receive(GC0802_FLASH_SPI_ADDR, &temp, data, 1, SPI_FRAME_LEN_8);
  Only_Read_Off;
}

void input_dut_byte_dual_DTR(uint8_t* data) {
  // 当前 dw ip 不支持
  uint8_t temp;
  temp = 0x00;
  boot_spi_set_dual_mode(GC0802_FLASH_SPI_ADDR, 0b10, _8_bits_addr_l, no_instruction, no_wait_cycles, SPI_FRAME_LEN_8, 0);
  Std_Rec_Off;
  Only_Read_On;
  boot_spi_send_receive(GC0802_FLASH_SPI_ADDR, &temp, data, 1, SPI_FRAME_LEN_8);
  Only_Read_Off;
}

void input_dut_byte(uint8_t* data) {
  if (qpi_mode) {
    Std_Rec_Off;
    input_dut_byte_quad(data);
  } else {
    uint8_t temp;
    temp = 0x00;
    boot_spi_set_std_mode(GC0802_FLASH_SPI_ADDR, 0b00, no_address, no_instruction, no_wait_cycles, SPI_FRAME_LEN_8, 0);
    Std_Rec_On;
    boot_spi_send_receive(GC0802_FLASH_SPI_ADDR, &temp, data, 1, SPI_FRAME_LEN_8);
    Std_Rec_Off;
  }
}

void input_dut_byte_DTR(uint8_t* data) {
  // 当前 dw ip 不支持
  if (qpi_mode) {
    Std_Rec_Off;
    input_dut_byte_quad(data);
  } else {
    uint8_t temp;
    temp = 0x00;
    boot_spi_set_std_mode(GC0802_FLASH_SPI_ADDR, 0b00, no_address, no_instruction, no_wait_cycles, SPI_FRAME_LEN_8, 0);
    Std_Rec_On;
    boot_spi_send_receive(GC0802_FLASH_SPI_ADDR, &temp, data, 1, SPI_FRAME_LEN_8);
    Std_Rec_Off;
  }
}

void read_with_holdn(uint32_t address, uint16_t num, uint16_t test_buf_off) {
  // 这个当前硬件设计不必支持(设置HOLDn引脚电平)
  uint8_t temp;
  CSn_On;
  // need to set HOLDn 0
  output_dut_byte(CMD_READ_STATUS);
  // need to set HOLDn 1

  output_dut_byte(CMD_READ_DATA);
  output_dut_byte((address >> 16) & 0xFF);
  output_dut_byte((address >> 8) & 0xFF);
  output_dut_byte((address >> 0) & 0xFF);

  for (int x = 0; x < num; ++x) {
    input_dut_byte(&temp);
    test_buf[x + test_buf_off] = temp;
  }
  //display("Read Page with HOLDn test. (Address = %x, Num = %x)", address, num);
  CSn_Off;
}

void test_erase_suspend(uint32_t address) {
  spi_we();
  spi_es(address);

  // Delay 50 ms and send
  usleep(50000);

  uint8_t status;
  spi_sr(&status);
  spi_suspend;
  spi_wait_busy(1);

  spi_rs(address, PAGESIZE, PAGESIZE);
  spi_sr(&status);
  display_test_buf(PAGESIZE, PAGESIZE);

  pattern_test_buf(0, PAGESIZE, 0xff00ff00);
  spi_we();
  spi_ws(0, PAGESIZE, 0);
  spi_wait_busy(1000000);
  //display("Should Fail");

  spi_resume();
  spi_wait_busy(1000);

  spi_rs(address, PAGESIZE, PAGESIZE);
  spi_sr(&status);
  display_test_buf(PAGESIZE, PAGESIZE);

  //display("Erase sector with Suspend / Resume (Address = %x)", address);
  //display("First sector dump should be pre erase data. Second sector dump should be erased data.");
}

void test_program_suspend(uint32_t address) {
  pattern_test_buf(0, PAGESIZE, 0x12345678);
  spi_we();
  spi_ws(0, PAGESIZE, 0);

  // Delay 1ms and send
  usleep(1000);

  uint8_t status;
  spi_sr(&status);
  spi_suspend();
  spi_wait_busy(1);

  spi_rs(address, PAGESIZE, PAGESIZE);
  spi_sr(&status);
  display_test_buf(PAGESIZE, PAGESIZE);

  pattern_test_buf(0, PAGESIZE, 0xff00ff00);
  spi_we();
  spi_ws(0, PAGESIZE, 0);
  spi_wait_busy(1000000);
  //display("Should Fail");

  spi_resume();
  spi_wait_busy(1000000);

  spi_rs(address, PAGESIZE, PAGESIZE);
  spi_sr(&status);
  display_test_buf(PAGESIZE, PAGESIZE);

  //display("Program sector with Suspend / Resume (Address = %x)", address);
  //display("First sector dump should be partially written page. Second sector dump should be completely written page.");
}

void spi_enable_reset() {
  CSn_On;
  output_dut_byte(CMD_ENABLE_RESET);
  //display("Enable Reset.");
  CSn_Off;
}

void spi_chip_reset() {
  CSn_On;
  output_dut_byte(CMD_CHIP_RESET);
  //display("Chip Reset.");
  CSn_Off;
}

void spi_r_sfdp(uint32_t address, uint16_t num, uint16_t test_buf_off) {
  uint8_t temp;
  CSn_On;
  output_dut_byte(CMD_READ_SFDP);
  output_dut_byte((address >> 16) & 0xFF);
  output_dut_byte((address >> 8) & 0xFF);
  output_dut_byte((address >> 0) & 0xFF);
  output_dut_byte(0);

  for (int x = 0; x < num; ++x) {
    input_dut_byte(&temp);
    test_buf[x + test_buf_off] = temp;
  }
  //display("Read SFDP Page. (Address = %x, Num = %x)", address, num);
  CSn_Off;
}

void spi_individual_unlock(uint32_t address) {
  CSn_On;
  output_dut_byte(CMD_INDIVIDUAL_UNLOCK);
  output_dut_byte((address >> 24) & 0xFF);
  output_dut_byte((address >> 16) & 0xFF);
  output_dut_byte((address >> 8) & 0xFF);
  output_dut_byte((address >> 0) & 0xFF);

  //display("Lock Block - (Address = %x)", address);
  CSn_Off;
}

void spi_read_block_lock(uint32_t address, uint8_t lockbit) {
  CSn_On;
  output_dut_byte(CMD_READ_BLOCK_LOCK);
  output_dut_byte((address >> 24) & 0xFF);
  output_dut_byte((address >> 16) & 0xFF);
  output_dut_byte((address >> 8) & 0xFF);
  output_dut_byte((address >> 0) & 0xFF);
  output_dut_byte(lockbit);

  //display("Read Block Lock - (Address = %x, Lockbit = %h)", address, lockbit);
  CSn_Off;
}

void spi_global_lock() {
  CSn_On;
  output_dut_byte(CMD_GLOBAL_BLOCK_LOCK);

  //display("Global Lock");
  CSn_Off;
}

void spi_global_unlock() {
  CSn_On;
  output_dut_byte(CMD_GLOBAL_BLOCK_UNLOCK);

  //display("Global Unlock");
  CSn_Off;
}

void clear_display_buf(uint8_t data) {
  for (int x = 0; x < PAGESIZE; ++x) {
    test_buf[x] = data;
  }
  //display("Clear test_buf to 1.");
}