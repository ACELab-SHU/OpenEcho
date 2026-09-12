#include "common.h"
#include "config.h"
#include "venus.h"

#include "irq_instr.S"
#include "../dags/generated/oai_nr_pdcch_cch_rx1_inputs.inc"
#include "../dags/generated/oai_nr_pdcch_cch_rx1_golden.inc"

#define INPUT_DESC(name) static stdata_t name##_s
INPUT_DESC(oai_rx0_real);
INPUT_DESC(oai_rx0_imag);
INPUT_DESC(oai_rx1_real);
INPUT_DESC(oai_rx1_imag);
INPUT_DESC(oai_rx2_real);
INPUT_DESC(oai_rx2_imag);
INPUT_DESC(oai_rx3_real);
INPUT_DESC(oai_rx3_imag);
INPUT_DESC(oai_pilot_real);
INPUT_DESC(oai_pilot_imag);
INPUT_DESC(oai_num_rx);
INPUT_DESC(csetNRB);
INPUT_DESC(oai_pdcch_index);
INPUT_DESC(oai_output_shift);
INPUT_DESC(oai_data_re);
INPUT_DESC(oai_llr_clip_limit);
INPUT_DESC(pdcch_config);
INPUT_DESC(initialInfo);
INPUT_DESC(csetPattern);
INPUT_DESC(c0Carrier);
#undef INPUT_DESC

static stdata_t pdcchbits_s;
static stdata_t dci_s;
static stdata_t crc_result_s;
static stdata_t pdsch_config_out_s;
static stdata_t pdsch_start_symbol_s;
static stdata_t pdsch_symbol_length_s;

#define INIT_AND_PUSH(name)                 \
  do {                                      \
    init_fifo(&name##_s.fifo);              \
    push_fifo(&name##_s.fifo, (int)&name);  \
  } while (0)

static void init_inputs(void) {
  INIT_AND_PUSH(oai_rx0_real);
  INIT_AND_PUSH(oai_rx0_imag);
  INIT_AND_PUSH(oai_rx1_real);
  INIT_AND_PUSH(oai_rx1_imag);
  INIT_AND_PUSH(oai_rx2_real);
  INIT_AND_PUSH(oai_rx2_imag);
  INIT_AND_PUSH(oai_rx3_real);
  INIT_AND_PUSH(oai_rx3_imag);
  INIT_AND_PUSH(oai_pilot_real);
  INIT_AND_PUSH(oai_pilot_imag);
  INIT_AND_PUSH(oai_num_rx);
  INIT_AND_PUSH(csetNRB);
  INIT_AND_PUSH(oai_pdcch_index);
  INIT_AND_PUSH(oai_output_shift);
  INIT_AND_PUSH(oai_data_re);
  INIT_AND_PUSH(oai_llr_clip_limit);
  INIT_AND_PUSH(pdcch_config);
  INIT_AND_PUSH(initialInfo);
  INIT_AND_PUSH(csetPattern);
  INIT_AND_PUSH(c0Carrier);
}
#undef INIT_AND_PUSH

static void init_outputs(void) {
  init_fifo(&pdcchbits_s.fifo);
  init_fifo(&dci_s.fifo);
  init_fifo(&crc_result_s.fifo);
  init_fifo(&pdsch_config_out_s.fifo);
  init_fifo(&pdsch_start_symbol_s.fifo);
  init_fifo(&pdsch_symbol_length_s.fifo);
}

static int compare_bytes(const unsigned char* actual,
                         const unsigned char* expected, int length) {
  int errors = 0;
  for (int i = 0; i < length; ++i) {
    errors += actual[i] != expected[i];
  }
  return errors;
}

__attribute__((optimize("O0"))) void main(void) {
  int errors;
  unsigned char* pdcchbits;
  unsigned char* dci;
  unsigned char* crc_result;
  unsigned char* pdsch_config_out;
  unsigned char* pdsch_start_symbol;
  unsigned char* pdsch_symbol_length;

  printf("OAI PDCCH CCH RX1 product DAG launch\n");
  REG_WRITE(0x1fff4000, 0x2);
  REG_WRITE(0x1fff4000, 0x0);
  init_inputs();
  init_outputs();

  /* Order is generated from first unique DAG-input offsets in dag1.json. */
  fire_dag(dag1, 20, 6,
           &oai_rx0_real_s, &oai_rx0_imag_s,
           &oai_rx1_real_s, &oai_rx1_imag_s,
           &oai_rx2_real_s, &oai_rx2_imag_s,
           &oai_rx3_real_s, &oai_rx3_imag_s,
           &oai_pilot_real_s, &oai_pilot_imag_s,
           &oai_num_rx_s, &csetNRB_s,
           &oai_pdcch_index_s, &oai_output_shift_s,
           &oai_data_re_s, &oai_llr_clip_limit_s,
           &pdcch_config_s, &initialInfo_s,
           &csetPattern_s, &c0Carrier_s,
           &pdcchbits_s, &dci_s, &crc_result_s,
           &pdsch_config_out_s, &pdsch_start_symbol_s,
           &pdsch_symbol_length_s);
  fire_dag_fence();

  REG_WRITE(0x1fff4000, 0x4);
  REG_WRITE(0x1fff4000, 0x0);
  pdcchbits = (unsigned char*)pop_fifo(&pdcchbits_s.fifo);
  dci = (unsigned char*)pop_fifo(&dci_s.fifo);
  crc_result = (unsigned char*)pop_fifo(&crc_result_s.fifo);
  pdsch_config_out = (unsigned char*)pop_fifo(&pdsch_config_out_s.fifo);
  pdsch_start_symbol = (unsigned char*)pop_fifo(&pdsch_start_symbol_s.fifo);
  pdsch_symbol_length =
      (unsigned char*)pop_fifo(&pdsch_symbol_length_s.fifo);

  errors = compare_bytes(pdcchbits, pdcchbits_expected, 128);
  errors += compare_bytes(dci, dci_expected, 64);
  errors += compare_bytes(crc_result, crc_result_expected, 2);
  errors += compare_bytes(pdsch_config_out, pdsch_config_expected, 1984);
  errors += compare_bytes(pdsch_start_symbol,
                          pdsch_start_symbol_expected, 2);
  errors += compare_bytes(pdsch_symbol_length,
                          pdsch_symbol_length_expected, 2);

  printf("OAI PDCCH CCH RX1 compare errors=%d\n", errors);
  REG_WRITE(0x1fff4000, errors ? 0x18 : 0x8);
  REG_WRITE(0x1fff4000, 0x0);
  while (1) {
  }
}
