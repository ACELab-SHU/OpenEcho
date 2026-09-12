#include "common.h"
#include "config.h"
#include "venus.h"

#include "irq_instr.S"

#include "../dags/generated/nrPDCCH_tail18_23_a27_data.inc"
#include "../dags/generated/nrPDCCH_tv9_golden.inc"

static stdata_t csetNRB_s;
static stdata_t demod_length_s;
static stdata_t pdcch_llr_s;
static stdata_t pdcch_config_s;
static stdata_t initialInfo_s;
static stdata_t csetPattern_s;
static stdata_t c0Carrier_s;

static stdata_t pdcchbits_s;
static stdata_t dci_s;
static stdata_t crc_result_s;
static stdata_t pdsch_config_out_s;
static stdata_t pdsch_start_symbol_s;
static stdata_t pdsch_symbol_length_s;

static void init_tail_fifos(void) {
  init_fifo(&csetNRB_s.fifo);
  init_fifo(&demod_length_s.fifo);
  init_fifo(&pdcch_llr_s.fifo);
  init_fifo(&pdcch_config_s.fifo);
  init_fifo(&initialInfo_s.fifo);
  init_fifo(&csetPattern_s.fifo);
  init_fifo(&c0Carrier_s.fifo);

  init_fifo(&pdcchbits_s.fifo);
  init_fifo(&dci_s.fifo);
  init_fifo(&crc_result_s.fifo);
  init_fifo(&pdsch_config_out_s.fifo);
  init_fifo(&pdsch_start_symbol_s.fifo);
  init_fifo(&pdsch_symbol_length_s.fifo);
}

static int compare_output(const unsigned char *actual,
                          const unsigned char *expected, int length) {
  int error_count = 0;
  for (int index = 0; index < length; ++index) {
    if (actual[index] != expected[index])
      ++error_count;
  }
  return error_count;
}

static void report_and_stop(int failed) {
  REG_WRITE(0x1fff4000, failed ? 0x18 : 0x8);
  REG_WRITE(0x1fff4000, 0x0);
  while (1) {
  }
}

__attribute__((optimize("O0"))) void main(void) {
  unsigned char *pdcchbits;
  unsigned char *dci;
  unsigned char *crc_result;
  unsigned char *pdsch_config_out;
  unsigned char *pdsch_start_symbol;
  unsigned char *pdsch_symbol_length;
  int error_count;

  printf("PDCCH A27 Task18-23 tail launch\n");
  REG_WRITE(0x1fff4000, 0x2);
  REG_WRITE(0x1fff4000, 0x0);

  init_tail_fifos();

  push_fifo(&csetNRB_s.fifo, (int)&tail_csetNRB);
  push_fifo(&demod_length_s.fifo, (int)&tail_demod_length);
  push_fifo(&pdcch_llr_s.fifo, (int)&tail_pdcch_llr);
  push_fifo(&pdcch_config_s.fifo, (int)&tail_pdcch_config);
  push_fifo(&initialInfo_s.fifo, (int)&tail_initialInfo);
  push_fifo(&csetPattern_s.fifo, (int)&tail_csetPattern);
  push_fifo(&c0Carrier_s.fifo, (int)&tail_c0Carrier);

  /*
   * Input and output orders are generated from the JSON ABI.  In particular,
   * pdcch_llr is a 2048 B pointer DAG input whose first 864 B are Task17's
   * qualified output; the remaining ABI padding is deterministic zero.
   */
  fire_dag(nrPDCCH_tail18_23_a27, 7, 6,
           &csetNRB_s, &demod_length_s, &pdcch_llr_s,
           &pdcch_config_s, &initialInfo_s, &csetPattern_s, &c0Carrier_s,
           &pdcchbits_s, &dci_s, &crc_result_s,
           &pdsch_config_out_s, &pdsch_start_symbol_s,
           &pdsch_symbol_length_s);
  fire_dag_fence();

  REG_WRITE(0x1fff4000, 0x4);
  REG_WRITE(0x1fff4000, 0x0);

  if (!check_data_ready(6,
                        &pdcchbits_s.fifo, &dci_s.fifo, &crc_result_s.fifo,
                        &pdsch_config_out_s.fifo,
                        &pdsch_start_symbol_s.fifo,
                        &pdsch_symbol_length_s.fifo)) {
    printf("PDCCH A27 tail outputs not ready\n");
    report_and_stop(1);
  }

  pdcchbits = (unsigned char *)pop_fifo(&pdcchbits_s.fifo);
  dci = (unsigned char *)pop_fifo(&dci_s.fifo);
  crc_result = (unsigned char *)pop_fifo(&crc_result_s.fifo);
  pdsch_config_out = (unsigned char *)pop_fifo(&pdsch_config_out_s.fifo);
  pdsch_start_symbol =
      (unsigned char *)pop_fifo(&pdsch_start_symbol_s.fifo);
  pdsch_symbol_length =
      (unsigned char *)pop_fifo(&pdsch_symbol_length_s.fifo);

  error_count = compare_output(pdcchbits, pdcchbits_expected, 128);
  error_count += compare_output(dci, dci_expected, 64);
  error_count += compare_output(crc_result, crc_result_expected, 2);
  error_count +=
      compare_output(pdsch_config_out, pdsch_config_expected, 1984);
  error_count += compare_output(pdsch_start_symbol,
                                pdsch_start_symbol_expected, 2);
  error_count += compare_output(pdsch_symbol_length,
                                pdsch_symbol_length_expected, 2);

  printf("PDCCH A27 Task18-23 tail errors=%d\n", error_count);
  report_and_stop(error_count != 0);
}
