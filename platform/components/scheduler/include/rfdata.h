#ifndef __RFDATA_H_
#define __RFDATA_H_
#include "fifo.h"

/*
 * uint8_t  - [0-255]
 * uint16_t - [0-65535]
 */
#define RESERVED    0
#define RULES       1
#define SPEC_NUM    2
#define INVALID_NUM 3
// for index[1] - Rules
#define ALL_FRAME_INDEX_VALID   0x8000
#define EVEN_FRAME_INDEX_VALID  0x8001
#define ODD_FRAME_INDEX_VALID   0x8002
#define ALL_SLOT_INDEX_VALID    0x80
#define EVEN_SLOT_INDEX_VALID   0x81
#define ODD_SLOT_INDEX_VALID    0x82
#define PERIOD_SLOT_INDEX_VALID 0x83
// end of specific number
#define LAST_FRAME_NUM  0xffff
#define LAST_SLOT_NUM   0xff
#define LAST_SYMBOL_NUM 0xffffffff

#define IS_ODD(number)  ((number)&1)
#define IS_EVEN(number) (!((number)&1))

/*
 * Rules:
 *  [0] - rules = 0 | specific number
 */

typedef struct rfdata {
  fifo_t fifo;
  uint16_t frame[100];
  uint8_t slot[100];
  uint32_t symbol[100];
} rfdata_t;

typedef struct scalardata {
  fifo_t fifo;
  int atr;
} stdata_t;

extern rfdata_t init_rfdata;
extern rfdata_t fft_rfdata;
extern rfdata_t rfdata[];

#define RFDATA_ITEM_COUNT 5
#define TXDATA_RAM0_PSEUDO_DSC 0x800000ff
#define TXDATA_RAM1_PSEUDO_DSC 0x8000000f

extern uint32_t RFDATA_START;
extern uint32_t RFDATA_END;
#define RFDATA_SIZE                   (sizeof(rfdata_t))
#define ADDRESS_IN_RFDATA_RANGE(addr) ((int)(addr) >= RFDATA_START && (int)(addr) <= RFDATA_END)
extern uint32_t STDATA_START;
extern uint32_t STDATA_END;
#define ADDRESS_IN_STDATA_RANGE(addr) ((int)(addr) >= STDATA_START && (int)(addr) <= STDATA_END)
extern uint32_t HEAP_START;
#define ADDRESS_IN_HEAP_RANGE(addr) ((int)(addr) >= HEAP_START)
#endif /* __RFDATA_H_ */