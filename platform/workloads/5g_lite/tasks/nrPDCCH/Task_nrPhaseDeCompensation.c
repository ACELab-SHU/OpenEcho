#include "data_type.h"
#include "riscv_printf.h"
#include "venus.h"

typedef short __v2048i16 __attribute__((ext_vector_type(2048)));
typedef char  __v4096i8 __attribute__((ext_vector_type(4096)));

int Task_nrPhaseDeCompensation(__v4096i8 OFDM_OutReal, __v4096i8 OFDM_OutImag, __v4096i8 Phase_cos, __v4096i8 Phase_sin,
                               short_struct subCarrierSpace, short_struct symbolNum, short_struct NRB) {

  // input parameter
  short scs        = subCarrierSpace.data;
  short symbol_num = symbolNum.data;
  short nRB        = NRB.data;

  if ((symbol_num <= 13) && (symbol_num >= 0)) {
    short nSubcarrier    = nRB * 12;
    int   fractionLength = 7;

    __v4096i8 acos_temp;
    __v4096i8 bsin_temp;
    __v4096i8 bcos_temp;
    __v4096i8 asin_temp;
    vclaim(acos_temp);
    vclaim(bsin_temp);
    vclaim(bcos_temp);
    vclaim(asin_temp);

    vsetshamt(fractionLength);
    acos_temp = vmul(OFDM_OutReal, Phase_cos, MASKREAD_OFF, nSubcarrier);
    bsin_temp = vmul(OFDM_OutImag, Phase_sin, MASKREAD_OFF, nSubcarrier);
    bcos_temp = vmul(OFDM_OutImag, Phase_cos, MASKREAD_OFF, nSubcarrier);
    asin_temp = vmul(OFDM_OutReal, Phase_sin, MASKREAD_OFF, nSubcarrier);
    vsetshamt(0);

    __v4096i8 DeModulation_real;
    __v4096i8 DeModulation_imag;
    vclaim(DeModulation_real);
    vclaim(DeModulation_imag);
    vbrdcst(DeModulation_real, 0, MASKREAD_OFF, nSubcarrier);
    vbrdcst(DeModulation_imag, 0, MASKREAD_OFF, nSubcarrier);
    DeModulation_real = vsadd(acos_temp, bsin_temp, MASKREAD_OFF, nSubcarrier);
    DeModulation_imag = vssub(asin_temp, bcos_temp, MASKREAD_OFF, nSubcarrier);

    vreturn(DeModulation_real, 1024, DeModulation_imag, 1024);
  } else {
    short nSubcarrier = nRB * 12;

    __v4096i8 DeModulation_real;
    __v4096i8 DeModulation_imag;
    vclaim(DeModulation_real);
    vclaim(DeModulation_imag);
    vbrdcst(DeModulation_real, 0, MASKREAD_OFF, 1);
    vbrdcst(DeModulation_imag, 0, MASKREAD_OFF, 1);
    vreturn(DeModulation_real, 1024, DeModulation_imag, 1024);
  }
}