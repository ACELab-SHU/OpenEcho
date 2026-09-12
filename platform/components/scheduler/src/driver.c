/*
 * helloworld.c: simple test application
 *
 * This application configures UART 16550 to baud rate 9600.
 * PS7 UART (Zynq) is not initialized by this application, since
 * bootrom/bsp configures it to baud rate 115200
 *
 * ------------------------------------------------
 * | UART TYPE   BAUD RATE                        |
 * ------------------------------------------------
 *   uartns550   9600
 *   uartlite    Configurable only in HW design
 *   ps7_uart    115200 (configured by bootrom/bsp)
 */

#include "driver.h"
#include "app_cmd.h"
#include "custom_cfg.h"
#include "main_init.h"
#include "platform.h"
#include "printf.h"
#include "sleep.h"
#include <stdio.h>

#if USE_DW_SPI == 1
#include "spi.h"
#else
#include "xspi.h"
#endif

void gc0802_init(void) {
  // malloc_init(_MEM_START, _MEM_END);
  init_platform();

  // printf("Hello World\n\r");
  // printf("Successfully ran Hello World application\n\r");

  int ret;
#if USE_DW_SPI == 1
  ret = spi_init(SPI0_BASEADDRESS);
  dw_spi_enable_slave(SPI0_BASEADDRESS, XPAR_SPI_0_DEVICE_ID);  // 和0号设备通信

  ret = spi_mode(SPI0_BASEADDRESS, SPI_MASTER);
  // printf("spi_mode return %d\r", ret);

  ret = spi_cp_format(SPI0_BASEADDRESS, SPI_FORMAT_CPOL0_CPHA1);
  // printf("spi_cp_format return %d\r", ret);

  ret = spi_baud(SPI0_BASEADDRESS, SPI0_CLK_OUT);  // baudr的低0bit无法写入，分频必须是2的倍数
  // printf("spi_baud return %d\r", ret);
#else
  xspi_init(XPAR_SPI_0_DEVICE_ID, 1, 0);
#endif
  // printf("Spi Init Finish\n\r");

  // ret       = 0;
  // int vctrl = 0;
  // do {
  //   ret = gc080x_init(&g_phy_obj[0], &g_phy_config[0]);
  //   usleep(1000);
  //   vctrl = get_syspll_status(&g_phy_obj[0]);
  //   LOG_MAIN("sys pll Vctrl: %d\n", vctrl);
  // } while ((ret != 0) || (vctrl <= 300) || (vctrl >= 900));

  // // Check syspll
  // cmd_api_lock_status(2);

  // // while (1) {}
  // cleanup_platform();
  // // spi_uninit(SPI0_BASEADDRESS);
}
