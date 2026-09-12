#include "venus.h"

/* Receives the actual upstream sum through a DAG dependency. */
int Task_forgeRestore(__v32i16 sum, __v32i16 b) {
    __v32i16 restored;
    vclaim(restored, 32);
    /* V1 LLVM 27fdd688 binds first C operand as vs2, second as vs1.
       VRSUB implements vs2-vs1 in this Gem5 backend. The asymmetric
       signed-ramp case detects an accidental reversal; no RTL claim. */
    restored = vrsub(sum, b, MASKREAD_OFF, 32);
    vreturn(restored, 64);
    return 0;
}
