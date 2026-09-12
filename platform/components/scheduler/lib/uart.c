/* Corresponding to HW-BFM src/testbench/peripherals/uart_task_bfm.svh */
#include "types.h"
#include "ulib.h"
#include "venusmmap.h"

/* [P.117] DW_apb_uart Memory Map */
#define UART_RBR_OFFSET       0x00  // Receive Buffer Register - 存放接收到的数据字节，可通过该寄存器读取接收到的数据
#define UART_THR_OFFSET       0x00  // Transmitter Holding Register - 存放要发送的数据字节，可通过该寄存器写入数据进行发送
#define UART_DLL_OFFSET       0x00  // Divisor Latch Low Register - 低位分频器寄存器
#define UART_IER_OFFSET       0x04  // Divisor Latch High Register - 与DLL一起用于设置串口的通信波特率，可通过这两个寄存器设置波特率的分频系数
#define UART_DLH_OFFSET       0x04  // Interrupt Enable Register - 用于控制串口中断的产生和屏蔽
#define UART_IIR_OFFSET       0x08  // Interrupt Identification Register - 用于识别串口中断的类型和原因
#define UART_FCR_OFFSET       0x08  // FIFO Control Register - 用于控制串口的FIFO缓冲区(使能/禁用FIFO、清空FIFO、设置FIFO深度)
#define UART_LCR_OFFSET       0x0c  // Line Control Register - 用于设置数据位数、校验位、停止串口位等通信参数
#define UART_MCR_OFFSET       0x10  // Mode Control Register - 用于控制串口通信的一些额外功能，例如RTS、DTR、Loopback等
#define UART_LSR_OFFSET       0x14  // Line Status Register - 用于反应串口通信状态，比如数据是否准备好、是否接受数据以及是否完成等
#define UART_MSR_OFFSET       0x18  // Mode Status Register - 用于反应串口通信的一些额外状态，例如CTS、DSR、RI等
#define UART_SCR_OFFSET       0x1c  // Scratchpad Register - 用于用户自定义数据的透传，通常不用
#define UART_LPDLL_OFFSET     0x20
#define UART_LPDLH_OFFSET     0x24  // Low-power Divisor Latch Low/High Register - 用于在低功耗模式下设置串口通信的波特率分频系数
#define UART_SRBR_LOW_OFFSET  0x30  // Serial Receive Buffer Register (Low)
#define UART_SRBR_HIGH_OFFSET 0x6c  // Serial Receive Buffer Register (High)
#define UART_STHR_LOW_OFFSET  0x30  // Serial Transmit Holding Register (Low)
#define UART_STHR_HIGH_OFFSET 0x6c  // Serial Transmit Holding Register (High)
#define UART_FAR_OFFSET       0x70  // FIFO Access Register
#define UART_TFR_OFFSET       0x74  // Transmit FIFO Reset
#define UART_RFW_OFFSET       0x78  // Receiver FIFO Reset
#define UART_USR_OFFSET       0x7c  // UART Status Register
#define UART_TFL_OFFSET       0x80  // Transmit FIFO Level
#define UART_RFL_OFFSET       0x84  // Receive FIFO Level
#define UART_SRR_OFFSET       0x88  // Software Reset Register
#define UART_SRTS_OFFSET      0x8c  // Shadow Request to Send
#define UART_SBCR_OFFSET      0x90  // Shadow Break Control Register
#define UART_SDMAM_OFFSET     0x94  // Shadow DMA Mode
#define UART_SFE_OFFSET       0x98  // Shadow FIFO Enable
#define UART_SRT_OFFSET       0x9c  // Shadow RCVR Trigger
#define UART_STET_OFFSET      0xa0  // Shadow TX Empty Trigger
#define UART_HTX_OFFSET       0xa4  // Halt Transmission
#define UART_DMASA_OFFSET     0xa8  // DMA Software Acknowledge

#define UART_FIFO_DEPTH 60

// VENUS_DEBUG_UART_ADDR is VCS console log

/* Function: Transmit a byte */
void uart_putc(char ch) {
  // Option 1. 读取FIFO(缓冲区)中的字节数，并与FIFO深度比较，如果填充水平大于FIFO深度，则说明FIFO有可用空间，可以继续发送数据。
  while (READ_BURST_32(VENUS_DEBUG_UART_ADDR, UART_TFL_OFFSET) > UART_FIFO_DEPTH)
    ;
  // Option 2. 读取LSR寄存器，并检查第五位(THRE)，该位可以指示FIFO缓冲区是否为空，如果THRE位为1，则说明FIFO有可用空间。因为LSR中含有缓冲区的其它信息，如果后续要用的话可以直接读这个。
  // uint32_t lsr = READ_BURST_32(VENUS_DEBUG_UART_ADDR, UART_LSR_OFFSET);
  // while (lsr & (1 << 5) == 0)
  //   ;
  WRITE_BURST_32(VENUS_DEBUG_UART_ADDR, UART_THR_OFFSET, (uint32_t)ch);
}
void uart_fence() {
  // Option 1. 读取FIFO(缓冲区)中的字节数，并与FIFO深度比较，如果填充水平大于FIFO深度，则说明FIFO有可用空间，可以继续发送数据。
  while (READ_BURST_32(VENUS_DEBUG_UART_ADDR, UART_TFL_OFFSET) > 0)
    ;
  while(READ_BURST_32(VENUS_DEBUG_UART_ADDR, UART_USR_OFFSET) & 0x1 == 0x1)
    ;
}


/* Function: Transmit a string */
void uart_puts(char* s) {
  while (*s) {
    uart_putc(*s++);
  }
}

/* Function: Receive a byte */
uint8_t uart_recvc(uint32_t uart_addr) {
  return (uint8_t)READ_BURST_32(uart_addr, UART_RBR_OFFSET);
}

/* Function: Receive a byte on software */
uint8_t uart_revc_sw(uint32_t uart_addr) {
  uint32_t receive_fifo_level = READ_BURST_32(uart_addr, UART_RFL_OFFSET);
  while (receive_fifo_level == 0) {
    // delay(100000);
    printf("VENUS_SOC_CPU: Uart_receive_byte timeout! uart_addr = %d", uart_addr);
    return -1;
  }
  return (uint8_t)READ_BURST_32(uart_addr, UART_RBR_OFFSET);
}

void uart_irq_handler(uint32_t uart_addr) {
  uint8_t iid = (uint8_t)(READ_BURST_32(uart_addr, UART_IIR_OFFSET) & 0x0f);
  switch (iid) {
    case 0b0000:
      printf("VENUS_SOC_CPU: modem status interrupt detected! $stop\n");
      break;
    case 0b0010:
      printf("VENUS_SOC_CPU: THR empty interrupt detected! $stop\n");
      break;
    case 0b0100: {
      printf("VENUS_SOC_CPU: received data available interrupt detected! $stop\n");
      uint8_t receive_data_byte = uart_recvc(uart_addr);
      printf("[VENUS_SOC_UART0: recv char is: %s $stop\n", receive_data_byte);
      break;
    }
    case 0b0110:
      printf("VENUS_SOC_CPU: receiver line status interrupt detected! $stop\n");
      break;
    case 0b0111:
      printf("VENUS_SOC_CPU: busy detect interrupt detected! $stop\n");
      break;
    case 0b1100:
      printf("VENUS_SOC_CPU: character timeout interrupt detected! $stop\n");
      break;
    default:
      printf("VENUS_SOC_CPU: unknown uart interrupt type! $stop\n");
      break;
  }
}

/*
 * [2024-07-25] 5Glite test: System clock = 12.5M
 *    - Baud Rate Divisor = (System Clock Frequency) / (16 * Baud Rate)
 *    - 12,500,000 / (16 * 115200) ≈ 8
 *    - [DLH]: Divisor Latch High
 *    - [DLL]: Divisor Latch Low
 *    +----------------------------+
 *    |    Reserved      |  [7:0]  |
 *    +----------------------------+
 *    - Baud Rate Divisor = DLH * 256 + DLL
 *    - 8 = [0000_0000 0000_1000]
 *    - DLH = 0x0 DLL = 0x8
 *    - in VCS console do not need this step
 */

void uart_init(uint32_t uart_addr) {
  // Write to Modem Control Register (MCR) to program SIR mode, auto flow, loopback, modem control outputs
  WRITE_BURST_32(uart_addr, UART_MCR_OFFSET, 0x0);
  // LCR enable
  WRITE_BURST_32(uart_addr, UART_LCR_OFFSET, 0x80);
  // DLL set required boud rate (11520 Baud)
  WRITE_BURST_32(uart_addr, UART_DLL_OFFSET, 0x1B);
  // DLH set required baud rate
  WRITE_BURST_32(uart_addr, UART_DLH_OFFSET, 0x00);
  // LCR
  WRITE_BURST_32(uart_addr, UART_LCR_OFFSET, 0x03);
  // FCR enable FIFOs and set Receive FIFO threshold level
  WRITE_BURST_32(uart_addr, UART_FCR_OFFSET, 0x01);
  // Write to IER to enable required interrupts
  WRITE_BURST_32(uart_addr, UART_IER_OFFSET, 0x81);
}

void venus_uart_init(void) {
  // Scheduler config itself debug uart
  uart_init(VENUS_DEBUG_UART_ADDR);
  // Scheduler config all the block's debug uart
  // for (int i = 0; i < NUM_BLOCKS; i++) {
  //   for (int j = 0; j < NUM_CLUSTERS; j++) {
  //     uart_init(VENUS_CLUSTER_BLOCK_BASE_ADDR + CLUSTER_OFFSET(i) + BLOCK_OFFSET(j) + BLOCK_DEBUGUART_OFFSET);
  //   }
  // }
}

void venus_user_uart_init(void) {
  // Scheduler config itself debug uart
  uart_init(VENUS_USER_UART_ADDR);
  // Scheduler config all the block's debug uart
  // for (int i = 0; i < NUM_BLOCKS; i++) {
  //   for (int j = 0; j < NUM_CLUSTERS; j++) {
  //     uart_init(VENUS_CLUSTER_BLOCK_BASE_ADDR + CLUSTER_OFFSET(i) + BLOCK_OFFSET(j) + BLOCK_DEBUGUART_OFFSET);
  //   }
  // }
}
