#include "spi.h"

int32_t boot_spi_init(uint32_t spi_base) {
  dw_spi_disable_all_irq(spi_base);
  dw_spi_disable(spi_base);

  return 0;
}

void boot_spi_uninit(uint32_t spi_base) {
  /* reset all registers */
  dw_spi_reset_regs(spi_base);
}

int32_t boot_spi_mode(uint32_t spi_base, spi_mode_t mode) {
  int32_t ret = 0;

  /* configure spi mode */
  switch (mode) {
    case SPI_MASTER:
      dw_spi_set_master_mode(spi_base);
      break;

    case SPI_SLAVE:
      dw_spi_set_slave_mode(spi_base);
      break;

    default:
      ret = 1;
      break;
  }

  return ret;
}

int32_t boot_spi_cp_format(uint32_t spi_base, spi_cp_format_t format) {
  int32_t ret = 0;

  /* configure spi format */
  switch (format) {
    case SPI_FORMAT_CPOL0_CPHA0:
      dw_spi_set_cpol0(spi_base);
      dw_spi_set_cpha0(spi_base);
      break;

    case SPI_FORMAT_CPOL0_CPHA1:
      dw_spi_set_cpol0(spi_base);
      dw_spi_set_cpha1(spi_base);
      break;

    case SPI_FORMAT_CPOL1_CPHA0:
      dw_spi_set_cpol1(spi_base);
      dw_spi_set_cpha0(spi_base);
      break;

    case SPI_FORMAT_CPOL1_CPHA1:
      dw_spi_set_cpol1(spi_base);
      dw_spi_set_cpha1(spi_base);
      break;

    default:
      ret = 1;
      break;
  }

  return ret;
}

uint32_t boot_spi_baud(uint32_t spi_base, uint32_t baud) {

  uint32_t div;
  uint32_t freq = 0U;

  dw_spi_config_sclk_clock(spi_base, CPU_CYCLE_MHZ * 1000000, baud);

  div = dw_spi_get_sclk_clock_div(spi_base);

  if (div > 0U) {
    freq = (CPU_CYCLE_MHZ * 1000000) / div;
  }

  return freq;
}

int32_t boot_spi_send(uint32_t spi_base, void* SendBufPtr, uint32_t size, uint32_t frame_length) {
  uint32_t value;
  uint32_t count = 0U;
  int32_t ret    = 0;
  uint8_t* tx_data;
  uint32_t current_size;

  tx_data = (uint8_t*)SendBufPtr;

  /* set tx mode */
  dw_spi_disable(spi_base);
  dw_spi_set_tx_mode(spi_base);
  dw_spi_config_tx_fifo_threshold(spi_base, DW_DEFAULT_SPI_TXFIFO_LV);
  dw_spi_enable(spi_base);

  dw_spi_enable_slave(spi_base, 0);  // 和0号设备通信

  if (frame_length == SPI_FRAME_LEN_32) {
    /* transfer loop */
    while (size > 0U) {
      current_size = DW_MAX_SPI_TXFIFO_LV - dw_spi_get_tx_fifo_level(spi_base);

      if (current_size > size) {
        current_size = size;
      }

      while (current_size--) {
        value = (uint32_t)(*(uint32_t*)tx_data);
        dw_spi_transmit_data(spi_base, value);
        tx_data += 1;
        count += 1U;
        size--;
      }
    }
  }

  if (frame_length == SPI_FRAME_LEN_16) {
    /* transfer loop */
    while (size > 0U) {
      current_size = DW_MAX_SPI_TXFIFO_LV - dw_spi_get_tx_fifo_level(spi_base);

      if (current_size > size) {
        current_size = size;
      }

      while (current_size--) {
        value = (uint32_t)(*(uint16_t*)tx_data);
        dw_spi_transmit_data(spi_base, value);
        tx_data += 1;
        count += 1U;
        size--;
      }
    }
  }

  if (frame_length == SPI_FRAME_LEN_8) {
    /* transfer loop */
    while (size > 0U) {
      current_size = DW_MAX_SPI_TXFIFO_LV - dw_spi_get_tx_fifo_level(spi_base);

      if (current_size > size) {
        current_size = size;
      }

      while (current_size--) {
        value = (uint32_t)(*(uint8_t*)tx_data);
        dw_spi_transmit_data(spi_base, value);
        tx_data += 1;
        count += 1U;
        size--;
      }
    }
  }

  uint32_t wait_count = 0;
  while ((dw_spi_get_status(spi_base) & DW_SPI_SR_BUSY) && (wait_count < 1000000U)) {
    wait_count++;
  }

  if (wait_count >= 1000000U) {
    ret = -1;
  }

  /* close spi */
  dw_spi_config_tx_fifo_threshold(spi_base, 0U);

  if (ret >= 0) {
    ret = (int32_t)count;
  }

  dw_spi_disable_slave(spi_base, 0);  // 和0号设备通信
  return ret;
}

int32_t boot_spi_receive(uint32_t spi_base, void* RecvBufPtr, uint32_t size, uint32_t frame_length) {
  uint32_t count = 0U;
  int32_t ret    = 0;
  uint8_t* rx_data;
  uint32_t current_size;

  rx_data = (uint8_t*)RecvBufPtr;

  /* set rx mode*/
  if (dw_spi_get_slave_mode(spi_base) & DW_SPI_SPIMSSEL_MASTER) {
    dw_spi_disable(spi_base);
  }

  dw_spi_set_rx_mode(spi_base);
  dw_spi_config_rx_data_len(spi_base, size - 1U);
  dw_spi_enable(spi_base);

  if (dw_spi_get_slave_mode(spi_base) & DW_SPI_SPIMSSEL_MASTER) {
    dw_spi_transmit_data(spi_base, 0U);
  }

  if (frame_length == SPI_FRAME_LEN_32) {
    /* transfer loop */
    while (size > 0U) {
      current_size = dw_spi_get_rx_fifo_level(spi_base);

      if (current_size > size) {
        current_size = size;
      }

      while (current_size--) {
        *(uint32_t*)rx_data = (uint32_t)dw_spi_receive_data(spi_base);
        rx_data += 1;
        size--;
        count++;
      }
    }
  }

  if (frame_length == SPI_FRAME_LEN_16) {
    /* transfer loop */
    while (size > 0U) {
      current_size = dw_spi_get_rx_fifo_level(spi_base);

      if (current_size > size) {
        current_size = size;
      }

      while (current_size--) {
        *(uint16_t*)rx_data = (uint16_t)dw_spi_receive_data(spi_base);
        rx_data += 1;
        size--;
        count++;
      }
    }
  }

  if (frame_length == SPI_FRAME_LEN_8) {
    /* transfer loop */
    while (size > 0U) {
      current_size = dw_spi_get_rx_fifo_level(spi_base);

      if (current_size > size) {
        current_size = size;
      }

      while (current_size--) {
        *(uint8_t*)rx_data = (uint8_t)dw_spi_receive_data(spi_base);
        rx_data += 1;
        size--;
        count++;
      }
    }
  }

  /* wait end of transcation */
  while ((dw_spi_get_status(spi_base) & DW_SPI_SR_BUSY)) {
    // printf("wait end of transcation\n");
  }

  /* close spi */
  if (dw_spi_get_slave_mode(spi_base) & DW_SPI_SPIMSSEL_MASTER) {
    dw_spi_config_rx_data_len(spi_base, 0U);
    dw_spi_config_rx_fifo_threshold(spi_base, 0U);
  }

  if (ret >= 0) {
    ret = (int32_t)count;
  }

  return ret;
}

int32_t boot_spi_send_receive(uint32_t spi_base, void* SendBufPtr, void* RecvBufPtr, uint32_t size, uint32_t frame_length) {
  uint32_t count = 0U;
  int32_t ret    = 0;
  uint32_t value;
  uint32_t tx_size;
  uint32_t rx_size;
  uint8_t* tx_data;
  uint8_t* rx_data;
  uint32_t current_size;

  tx_data = (uint8_t*)SendBufPtr;
  tx_size = size;
  rx_data = (uint8_t*)RecvBufPtr;
  rx_size = size;

  /* set tx rx mode*/
  dw_spi_disable(spi_base);
  dw_spi_set_tx_rx_mode(spi_base);
  dw_spi_config_tx_fifo_threshold(spi_base, DW_DEFAULT_SPI_TXFIFO_LV);
  dw_spi_config_rx_fifo_threshold(spi_base, DW_DEFAULT_SPI_RXFIFO_LV);
  dw_spi_enable(spi_base);

  dw_spi_enable_slave(spi_base, 0);  // 和0号设备通信

  if (frame_length == SPI_FRAME_LEN_32) {
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
  }

  if (frame_length == SPI_FRAME_LEN_16) {
    /* transfer loop */
    while ((tx_size > 0U) || (rx_size > 0U)) {
      /* process tx fifo empty */
      if (tx_size > 0U) {
        current_size = DW_MAX_SPI_TXFIFO_LV - dw_spi_get_tx_fifo_level(spi_base);

        if (current_size > tx_size) {
          current_size = tx_size;
        }

        while (current_size--) {
          value = (uint16_t)(*(uint16_t*)tx_data);
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
          *(uint16_t*)rx_data = (uint16_t)dw_spi_receive_data(spi_base);
          rx_data += 1;
          rx_size--;
        }
      }
    }
  }

  if (frame_length == SPI_FRAME_LEN_8) {
    /* transfer loop */
    while ((tx_size > 0U) || (rx_size > 0U)) {
      /* process tx fifo empty */
      if (tx_size > 0U) {
        current_size = DW_MAX_SPI_TXFIFO_LV - dw_spi_get_tx_fifo_level(spi_base);

        if (current_size > tx_size) {
          current_size = tx_size;
        }

        while (current_size--) {
          value = (uint8_t)(*(uint8_t*)tx_data);
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
          *(uint8_t*)rx_data = (uint8_t)dw_spi_receive_data(spi_base);
          rx_data += 1;
          rx_size--;
        }
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

int32_t boot_spi_frame_len(uint32_t spi_base, spi_frame_len_t length) {
  int32_t ret = 0;
  if ((length < SPI_FRAME_LEN_4) || (length > SPI_FRAME_LEN_16)) {
    ret = -1;
  } else {
    /* configura data frame width*/
    dw_spi_config_data_frame_len(spi_base, (uint32_t)length);
  }

  return ret;
}

void boot_spi_select_slave(uint32_t spi_base, uint32_t slave_num) {
  dw_spi_enable_slave(spi_base, slave_num);
}

void boot_spi_set_std_mode(uint32_t spi_base, uint8_t trans_type, addr_l_t addr_l, inst_l_t inst_len, wait_cycles_t wait_cycles, spi_frame_len_t fram_len, uint32_t fram_num) {
  dw_spi_disable(spi_base);
  dw_spi_set_trans_type(spi_base, trans_type);
  dw_spi_set_spi_frf(spi_base, 0b00);     // specifies the format in which the transmission happens for the frame
  dw_spi_set_addr_l(spi_base, addr_l);    // specifies length of address
  dw_spi_set_inst_l(spi_base, inst_len);  // specifies length of instruction
  dw_spi_set_wait_cycles(spi_base, wait_cycles);
  dw_spi_set_dfs_32(spi_base, fram_len);  // specifies length of data
  // dw_spi_set_dfs(spi_base, fram_len); // specifies length of data
  dw_spi_set_ndf(spi_base, fram_num);
}

void boot_spi_set_dual_mode(uint32_t spi_base, uint8_t trans_type, addr_l_t addr_l, inst_l_t inst_len, wait_cycles_t wait_cycles, spi_frame_len_t fram_len, uint32_t fram_num) {
  dw_spi_disable(spi_base);
  dw_spi_set_trans_type(spi_base, trans_type);
  dw_spi_set_spi_frf(spi_base, 0b01);     // specifies the format in which the transmission happens for the frame
  dw_spi_set_addr_l(spi_base, addr_l);    // specifies length of address
  dw_spi_set_inst_l(spi_base, inst_len);  // specifies length of instruction
  dw_spi_set_wait_cycles(spi_base, wait_cycles);
  dw_spi_set_dfs_32(spi_base, fram_len);  // specifies length of data
  // dw_spi_set_dfs(spi_base, fram_len); // specifies length of data
  dw_spi_set_ndf(spi_base, fram_num);
}

void boot_spi_set_quad_mode(uint32_t spi_base, uint8_t trans_type, addr_l_t addr_l, inst_l_t inst_len, wait_cycles_t wait_cycles, spi_frame_len_t fram_len, uint32_t fram_num) {
  dw_spi_disable(spi_base);
  dw_spi_set_trans_type(spi_base, trans_type);
  dw_spi_set_spi_frf(spi_base, 0b10);     // specifies the format in which the transmission happens for the frame
  dw_spi_set_addr_l(spi_base, addr_l);    // specifies length of address
  dw_spi_set_inst_l(spi_base, inst_len);  // specifies length of instruction
  dw_spi_set_wait_cycles(spi_base, wait_cycles);
  dw_spi_set_dfs_32(spi_base, fram_len);  // specifies length of data
  // dw_spi_set_dfs(spi_base, fram_len); // specifies length of data
  dw_spi_set_ndf(spi_base, fram_num);
}
