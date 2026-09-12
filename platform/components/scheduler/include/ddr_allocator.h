/* ref: https://github.com/yangminz/bcst_csapp */
#ifndef __ALLOCATOR_H_
#define __ALLOCATOR_H_
#include "assert.h"
#include "common.h"
#include "types.h"

extern uint32_t HEAP_START;
extern uint32_t HEAP_END;

extern uint32_t alloc_start;
extern uint32_t alloc_end;

extern uint32_t free_list_head;
extern uint32_t free_list_counter;

#define FREE          (0)
#define ALLOCATED     (1)
#define MIN_BLOCKSIZE (16)
#define BOUNDART_SIZE (8)

/*
 * Block operations:
 * 	1. boundary_addr: header_addr or footer_addr
 * 	2. hp_addr: header_addr or payload_addr
 *
 * Boundary: The boundary of the block, header or footer,
 * 	we design the block's size is always 8-byte(DWORD) aligned,
 * 	so the low 3 digit can be used as some flag bits.
 *    1. lowerest bit represent allocated or not
 *    2. reserve
 *    3. reserve
 *    +---------------------------+-------+
 *    |        block_size         |  000  | 32bits == 4byte
 *    +---------------------------+-------+
 *
 * Note: Make sure that the start address of the payload is 8-byte aligned
 * 	and the boundary (32bit) address is 4-byte aligned
 *
 * Free block consists of:
 *    1. a footer, 4 bytes
 *    2. a header, 4 bytes
 *    3. a successor, the next free block header, 4 bytes
 *    4. a predecessor, the previous freeblock header, 4 bytes
 *    +-----------------------------------+
 *    |               footer              |
 *    +-----------------------------------+ %64 == 56 aligned
 *    |                                   |
 *    |                                   |
 *    |                FREE               |
 *    |                                   |
 *    |                                   |
 *    +-----------------------------------+ %64 == 56 aligned
 *    |              successor            |
 *    +-----------------------------------+ %64 == 60 aligned
 *    |             predecessor           |
 *    +-----------------------------------+ %64 == 0  aligned
 *    |               header              |
 *    +-----------------------------------+ %64 == 60 aligned
 *    Therefore, MIN_BLOCKSIZE = 16
 *
 * Allocated block consists of:
 *    1. a header, 4 bytes
 *    2. a footer, 4 bytes
 *    3. a payload
 *    4. a padding, to make sure the size of the block is 8-byte aligned
 *    +-----------------------------------+
 *    |               footer              |
 *    +-----------------------------------+ %64 == 56 aligned
 *    |                                   |
 *    |               padding             |
 *    |                                   |
 *    +-----------------------------------+
 *    |                                   |
 *    |                                   |
 *    |                                   |
 *    |               payload             |
 *    |                                   |
 *    |                                   |
 *    |                                   |
 *    +-----------------------------------+ %64 == 0 aligned
 *    |               header              |
 *    +-----------------------------------+ %64 == 60 aligned
 */
/* make sure the block_size is a multiple of 8-byte */
static inline uint32_t get_blocksize(uint32_t boundary_addr) {
  return *(uint32_t*)boundary_addr & 0xFFFFFFF8;
}
/* blocksize is the total size of header, footer and payload */
static inline void set_blocksize(uint32_t boundary_addr, uint32_t blocksize) {
  *(uint32_t*)boundary_addr = (*(uint32_t*)boundary_addr & 0x7) | blocksize;
}
static inline uint32_t get_allocated(uint32_t boundary_addr) {
  return *(uint32_t*)boundary_addr & 0x1;
}
static inline void set_allocated(uint32_t boundary_addr, uint32_t allocated) {
  *(uint32_t*)boundary_addr = (*(uint32_t*)boundary_addr & 0xFFFFFFFE) | (allocated & 0x1);
}
static inline void set_new_allocated(uint32_t boundary_addr, uint32_t allocated) {
  *(uint32_t*)boundary_addr = (allocated & 0x1);
}
static inline uint32_t get_payload(uint32_t hp_addr) { return _align_up(hp_addr, 8); }
static inline uint32_t get_header(uint32_t hp_addr) {
  return _align_up(hp_addr, 8) - 4;
}
static inline uint32_t get_footer(uint32_t hp_addr) {
  uint32_t header_addr = get_header(hp_addr);
  uint32_t footer_addr = header_addr + get_blocksize(header_addr) - 4;
  return footer_addr;
}

/* Heap operations */
static inline uint32_t get_prevfree(uint32_t block_header) {
  if (!block_header) {
    return 0;
  }
  return *(uint32_t*)(block_header + 4);
}
static inline uint32_t get_nextfree(uint32_t block_header) {
  if (!block_header) {
    return 0;
  }
  return *(uint32_t*)(block_header + 8);
}
static inline void set_prevfree(uint32_t block_header, uint32_t prev_header) {
  *(uint32_t*)(block_header + 4) = (uint32_t)prev_header;
}
static inline void set_nextfree(uint32_t block_header, uint32_t prev_header) {
  *(uint32_t*)(block_header + 8) = (uint32_t)prev_header;
}
static inline uint32_t get_nextheader(uint32_t hp_addr) {
  uint32_t header_addr      = get_header(hp_addr);
  uint32_t block_size       = get_blocksize(header_addr);
  uint32_t next_header_addr = header_addr + block_size;
  return next_header_addr;
}
static inline uint32_t get_prevheader(uint32_t hp_addr) {
  uint32_t header_addr      = get_header(hp_addr);
  uint32_t prev_footer_addr = header_addr - 4;
  uint32_t prev_blocksize   = get_blocksize(prev_footer_addr);
  uint32_t prev_header_addr = header_addr - prev_blocksize;
  return prev_header_addr;
}

/*
 * Heap sentinel:
 *  1. prologue and epilogue is sentinel block of the heap
 *  2. has no payload, marked as allocated
 *  3. cannot be dynamically allocated/free
 */
static inline uint32_t get_prologue() { return alloc_start + 60; }
static inline uint32_t get_epilogue() { return alloc_end - 4; }
/* first and last block could be allocated */
static inline uint32_t get_firstblock() { return get_prologue() + 64; }
static inline uint32_t get_lastblock() { return get_prevheader(get_epilogue()); }

/* Free list operations */
static inline void free_list_insert(uint32_t block_header) {
  // if list is empty
  if (free_list_head == 0 || free_list_counter == 0) {
    set_prevfree(block_header, block_header);
    set_nextfree(block_header, block_header);
    free_list_head    = block_header;
    free_list_counter = 1;
    return;
  }
  /*
   * Next free list:
   *                +--------------------------------------+
   *     Before:    |                                      |
   *                +-->old_head--->node--->node--->tail---+
   *
   *
   *                +------------------------------------------------------+
   *     After:     |                                                      |
   *                +-->block_header--->old_head--->node--->node--->tail---+
   *                     (new_head)
   */
  uint32_t tail = get_prevfree(free_list_head);

  set_nextfree(block_header, free_list_head);
  set_prevfree(free_list_head, block_header);
  set_nextfree(tail, block_header);
  set_prevfree(block_header, tail);

  free_list_head = block_header;
  free_list_counter += 1;
}

static inline void free_list_delete(uint32_t block_header) {
  if (free_list_head == 0 || free_list_counter == 0) {
    return;
  }
  if (free_list_counter == 1) {
    free_list_head    = 0;
    free_list_counter = 0;
    return;
  }

  uint32_t prev_header = get_prevfree(block_header);
  uint32_t next_header = get_nextfree(block_header);

  // if want to delete list header, assign header next block as the new header
  if (block_header == free_list_head) {
    free_list_head = next_header;
  }
  /*
   * prev_header <---> block_header <---> next_header
   * prev_header <---> next_header
   */
  set_prevfree(next_header, prev_header);
  set_nextfree(prev_header, next_header);
  free_list_counter -= 1;
}

static inline uint32_t merge_free_blocks(uint32_t low_header, uint32_t high_header) {
  /*
   *
   * Before merge:
   *       |<------------- low_blocksize ------------->|<-------------
   * high_blocksize ------------>|
   *     --+-----------+-------------------+-----------+-----------+-------------------+-----------+
   *       | header(?) |                   | footer(?) | header(?) | | footer(?)
   * |
   *     --+-----------+-------------------+-----------+-----------+-------------------+-----------+
   *    low_header high_footer
   *
   * After merge:
   *       |<--------------------------- low_blocksize + high_blocksize
   * -------------------------->|
   *     --+-----------+---------------------------------------------------------------+-----------+
   *       | header(F) | | footer(F) |
   *     --+-----------+---------------------------------------------------------------+-----------+
   */
  uint32_t blocksize = get_blocksize(low_header) + get_blocksize(high_header);
  set_blocksize(low_header, blocksize);
  set_allocated(low_header, FREE);

  // set merged block's footer
  uint32_t high_footer = get_footer(high_header);
  set_blocksize(high_footer, blocksize);
  set_allocated(high_footer, FREE);

  return low_header;
}

/* Split block */
static inline uint32_t try_alloc_with_splitting(uint32_t block_header,
                                                uint32_t request_blocksize) {
  if (request_blocksize < MIN_BLOCKSIZE) {
    return 0;
  }

  uint32_t blocksize = get_blocksize(block_header);
  uint32_t allocated = get_allocated(block_header);

  if (allocated == FREE && blocksize >= request_blocksize) {
    // allocate this block
    if (blocksize - request_blocksize >= MIN_BLOCKSIZE) {
      /*
       * Before split:
       *       |<------------------------------------------ blocksize ----------------------------------------------->|
       *     --+-----------+----------+----------+--------------------------------------------------------+-----------+--
       *       | header(F) | prevfree | nextfree |                          FREE                          | footer(F) |
       *     --+-----------+----------+----------+--------------------------------------------------------+-----------+--
       *  block_header block_footer
       *
       * After split:
       *       |<---------- request_blocksize ------------>|<------------blocksize - request_blocksize -------------->|
       *     --+-----------+-------------------+-----------+-----------+----------+----------+------------+-----------+--
       *       | header(A) |      payload      | footer(A) | header(F) | prevfree | nextfree |    FREE    | footer(F) |
       *     --+-----------+-------------------+-----------+-----------+----------+----------+------------+-----------+--
       *  block_header                     block_footer  new_header new_footer
       */
      uint32_t new_footer = get_footer(block_header);
      set_blocksize(new_footer, blocksize - request_blocksize);

      set_allocated(block_header, ALLOCATED);
      set_blocksize(block_header, request_blocksize);

      // WRITE_BURST_32(get_footer(block_header), 0, 0);
      uint32_t block_footer = get_footer(block_header);
      set_new_allocated(block_footer, ALLOCATED);
      set_blocksize(block_footer, request_blocksize);

      // WRITE_BURST_32(get_nextheader(block_header), 0, 0);
      uint32_t new_header = get_nextheader(block_header);
      set_new_allocated(new_header, FREE);
      set_blocksize(new_header, blocksize - request_blocksize);

      return get_payload(block_header);

    } else if (blocksize - request_blocksize < MIN_BLOCKSIZE) {
      // if splitted, it could't contain header + prevfree + nextfree + footer
      // descriptor

      // set_allocated(block_header, ALLOCATED);
      // uint32_t block_footer = get_footer(block_header);
      // set_allocated(block_footer, ALLOCATED);

      // return get_payload(block_header);
      return 0;
    }
  }
  return 0;
}

#endif /* __ALLOCATOR_H_ */
