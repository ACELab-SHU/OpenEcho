#include "common.h"
#include "config.h"
#include "venus.h"

#include "irq_instr.S"

/* TV9 runtime inputs. This launcher consumes only the three slice inputs. */
#include "../dags/generated/nrPDCCH_tv9_data.inc"

static rfdata_t dfe_input_0_rfdata;
static stdata_t scsSSB_s;
static stdata_t symbolNum0_s;
static stdata_t dfe_output_real_0_s;
static stdata_t dfe_output_imag_0_s;

static void init_inputsplit_fifos(void) {
  init_fifo(&dfe_input_0_rfdata.fifo);
  init_fifo(&scsSSB_s.fifo);
  init_fifo(&symbolNum0_s.fifo);
  init_fifo(&dfe_output_real_0_s.fifo);
  init_fifo(&dfe_output_imag_0_s.fifo);
}

/*
 * Independent boundary oracle for Task_inputSplit: each output byte is read
 * from the original runtime DFE payload, after its four-byte header. It does
 * not supply data to the DAG and does not depend on a fixed output vector.
 */
static int inputsplit_symbol_length(void) {
  short scs = scsSSB[0];
  short symbol_num = symbolNum0[0];

  if (scs == 15) {
    return 2048 + ((symbol_num == 0 || symbol_num == 7) ? 160 : 144);
  }
  if (scs == 30) {
    return 1024 + ((symbol_num == 0) ? 88 : 72);
  }
  return -1;
}

static int verify_inputsplit_outputs(const unsigned char *real,
                                     const unsigned char *imag) {
  const unsigned char *input = (const unsigned char *)dfe_input_0;
  int symbol_length = inputsplit_symbol_length();
  int error_count = 0;

  if (real == 0 || imag == 0 || symbol_length < 0 || symbol_length > 2208 ||
      4 + 2 * symbol_length > (int)sizeof(dfe_input_0)) {
    printf("Task_inputSplit oracle precondition failed: scs=%d symbol=%d len=%d\n",
           scsSSB[0], symbolNum0[0], symbol_length);
    return 1;
  }

  for (int i = 0; i < symbol_length; i++) {
    unsigned char expected_real = input[4 + 2 * i];
    unsigned char expected_imag = input[5 + 2 * i];
    if (real[i] != expected_real || imag[i] != expected_imag) {
      if (error_count < 8) {
        printf("Task_inputSplit mismatch[%d]: real=%u expected=%u, imag=%u expected=%u\n",
               i, real[i], expected_real, imag[i], expected_imag);
      }
      error_count++;
    }
  }
  return error_count;
}

__attribute__((optimize("O0"))) void main(void) {
  unsigned char *real;
  unsigned char *imag;
  int error_count;

  printf("PDCCH Task_inputSplit TV9 slice launch\n");
  REG_WRITE(0x1fff4000, 0x2);
  REG_WRITE(0x1fff4000, 0x0);

  init_inputsplit_fifos();
  push_fifo(&dfe_input_0_rfdata.fifo, (int)&dfe_input_0);
  push_fifo(&scsSSB_s.fifo, (int)&scsSSB);
  push_fifo(&symbolNum0_s.fifo, (int)&symbolNum0);

  fire_dag(nrPDCCH_2p0_slice_inputsplit, 3, 2,
           &dfe_input_0_rfdata, &scsSSB_s, &symbolNum0_s,
           &dfe_output_real_0_s, &dfe_output_imag_0_s);
  fire_dag_fence();

  REG_WRITE(0x1fff4000, 0x4);
  REG_WRITE(0x1fff4000, 0x0);

  real = (unsigned char *)pop_fifo(&dfe_output_real_0_s.fifo);
  imag = (unsigned char *)pop_fifo(&dfe_output_imag_0_s.fifo);
  error_count = verify_inputsplit_outputs(real, imag);
  printf("PDCCH Task_inputSplit TV9 slice errors=%d\n", error_count);

  REG_WRITE(0x1fff4000, error_count ? 0x18 : 0x8);
  REG_WRITE(0x1fff4000, 0x0);

  while (1) {
  }
}
