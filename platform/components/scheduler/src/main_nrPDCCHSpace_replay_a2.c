#include "common.h"
#include "config.h"
#include "venus.h"

#include "irq_instr.S"

static const unsigned char replay_trigger[1] = {0};
static stdata_t replay_trigger_s;
static stdata_t pdcch_index_s;
static stdata_t pdcch_length_s;
static stdata_t dmrs_real_s;
static stdata_t dmrs_imag_s;
static stdata_t dmrs_index_s;
static stdata_t dmrs_interval_s;
static stdata_t dmrs_ref_length_s;
static stdata_t dmrs_symbol_index_s;
static stdata_t dmrs_symbol_length_s;

static void init_output_fifos(void) {
  init_fifo(&replay_trigger_s.fifo);
  init_fifo(&pdcch_index_s.fifo);
  init_fifo(&pdcch_length_s.fifo);
  init_fifo(&dmrs_real_s.fifo);
  init_fifo(&dmrs_imag_s.fifo);
  init_fifo(&dmrs_index_s.fifo);
  init_fifo(&dmrs_interval_s.fifo);
  init_fifo(&dmrs_ref_length_s.fifo);
  init_fifo(&dmrs_symbol_index_s.fifo);
  init_fifo(&dmrs_symbol_length_s.fifo);
}

__attribute__((optimize("O0"))) void main(void) {
  printf("Task_nrPDCCHSpace replay a2 launch\n");
  REG_WRITE(0x1fff4000, 0x2);
  REG_WRITE(0x1fff4000, 0x0);

  init_output_fifos();
  push_fifo(&replay_trigger_s.fifo, (int)&replay_trigger);
  fire_dag(nrPDCCHSpace_replay_a2, 1, 9,
           &replay_trigger_s,
           &pdcch_index_s, &pdcch_length_s,
           &dmrs_real_s, &dmrs_imag_s, &dmrs_index_s,
           &dmrs_interval_s, &dmrs_ref_length_s,
           &dmrs_symbol_index_s, &dmrs_symbol_length_s);
  fire_dag_fence();

  REG_WRITE(0x1fff4000, 0x8);
  REG_WRITE(0x1fff4000, 0x0);

  while (1) {
  }
}
