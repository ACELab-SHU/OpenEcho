#ifndef __GC0802_DRIVER_CONFIG_H__
#define __GC0802_DRIVER_CONFIG_H__

#define REG_WRITE(address, value) (*(volatile uint32_t*)(address)) = (value)
#define REG_READ(address)         (*(volatile uint32_t*)(address))

#define CPU_CYCLE_MHZ 150
#define APB_CYCLE_MHZ 50

/****************************************************************************************************/
/*											dw_spi											    */
/****************************************************************************************************/
// 选择用 DW 的 SPI 还是 XILINX SPI
#define USE_DW_SPI       1
#define SPI0_BASEADDRESS 0x1fff3000
#define SPI0_CLK_OUT     12500000

/****************************************************************************************************/
/*											uartlite											    */
/****************************************************************************************************/
// #define PRINT_BASEADDRESS  0x80010000
// #define PRINT_ADDRESS_RX   (PRINT_BASEADDRESS + 0x00000000)
// #define PRINT_ADDRESS_TX   (PRINT_BASEADDRESS + 0x00000004)
// #define PRINT_ADDRESS_STAT (PRINT_BASEADDRESS + 0x00000008)
// #define PRINT_ADDRESS_CTRL (PRINT_BASEADDRESS + 0x0000000C)

/****************************************************************************************************/
/*											malloc mamoey space									    */
/****************************************************************************************************/
// #define _MEM_END   0x40100000  // 可用内存的结束地址, 512k
// #define _MEM_START 0x40080000  // 内存分配开始地址，0x0 开始作为操作系统自身的代码, 512k

// #define SPI_RW_ADDDRESS 0xC0000000
// #define GC0802_DEVCTRL_ADDR                                    0x1fff0800UL
#define SPI_RW_ADDDRESS 0x1fff0800UL

/****************************************************************************************************/
/*											SPI									                    */
/****************************************************************************************************/
/* Definitions for driver SPI */
// #define XPAR_XSPI_NUM_INSTANCES 1U

// /* Definitions for peripheral AXI_QUAD_SPI_0 */
// #define XPAR_AXI_QUAD_SPI_0_DEVICE_ID              0U
// #define XPAR_AXI_QUAD_SPI_0_BASEADDR               0x80020000U
// #define XPAR_AXI_QUAD_SPI_0_HIGHADDR               0x8002FFFFU
// #define XPAR_AXI_QUAD_SPI_0_FIFO_EXIST             1U
// #define XPAR_AXI_QUAD_SPI_0_FIFO_DEPTH             16U
// #define XPAR_AXI_QUAD_SPI_0_SPI_SLAVE_ONLY         0U
// #define XPAR_AXI_QUAD_SPI_0_NUM_SS_BITS            1U
// #define XPAR_AXI_QUAD_SPI_0_NUM_TRANSFER_BITS      8U
// #define XPAR_AXI_QUAD_SPI_0_SPI_MODE               0U
// #define XPAR_AXI_QUAD_SPI_0_TYPE_OF_AXI4_INTERFACE 0U
// #define XPAR_AXI_QUAD_SPI_0_AXI4_BASEADDR          0U
// #define XPAR_AXI_QUAD_SPI_0_AXI4_HIGHADDR          0U
// #define XPAR_AXI_QUAD_SPI_0_XIP_MODE               0U

// /* Canonical definitions for peripheral AXI_QUAD_SPI_0 */
#define XPAR_SPI_0_DEVICE_ID 0U
// #define XPAR_SPI_0_BASEADDR               0x80020000U
// #define XPAR_SPI_0_HIGHADDR               0x8002FFFFU
// #define XPAR_SPI_0_FIFO_EXIST             1U
// #define XPAR_SPI_0_FIFO_DEPTH             16U
// #define XPAR_SPI_0_SPI_SLAVE_ONLY         0U
// #define XPAR_SPI_0_NUM_SS_BITS            1U
// #define XPAR_SPI_0_NUM_TRANSFER_BITS      8U
// #define XPAR_SPI_0_SPI_MODE               0U
// #define XPAR_SPI_0_TYPE_OF_AXI4_INTERFACE 0U
// #define XPAR_SPI_0_AXI4_BASEADDR          0U
// #define XPAR_SPI_0_AXI4_HIGHADDR          0U
// #define XPAR_SPI_0_XIP_MODE               0U
// #define XPAR_SPI_0_USE_STARTUP            0U

#endif /* __GC0802_DRIVER_CONFIG_H__ */