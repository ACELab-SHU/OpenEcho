#include "venus.h"

/* Two signed i16[32] inputs, |input| <= 1024. No overflow or CSR changes. */
int Task_forgeAdd(__v32i16 a, __v32i16 b) {
    __v32i16 sum;
    vclaim(sum, 32);
    sum = vadd(a, b, MASKREAD_OFF, 32);
    vreturn(sum, 64);
    return 0;
}
