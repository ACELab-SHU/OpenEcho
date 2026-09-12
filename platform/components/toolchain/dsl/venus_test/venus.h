
#define MASKREAD_ON (1)
#define MASKREAD_OFF (0)
#define MASKWRITE_ON (1)
#define MASKWRITE_OFF (0)

#define SHUFFLE_SCATTER (0)
#define SHUFFLE_GATHER (1)

#define STRINGIFY(X) #X
#define STR(X) STRINGIFY(X)
#define VENUS_INLINE static inline __attribute__((always_inline))
#define VENUS_ALIGN __attribute__((aligned(64)))

#define VCSR_MULSHAMT 0
#define VCSR_MSBHEAD 1
#define VCSR_MULSATURATE 2
#define VCSR_MSBLSU 3
#define VCSR_EXTCFG 4

// gpp -C -x venusbuiltin.pp -o venusbuiltin.h
// or python3 gen_venusbuiltin_h.py
#include "venusbuiltin.h"
#define vcmxmul(a_re_p, a_im_p, a_re, a_im, b_re, b_im, vmr, ...)              \
  __Venus_cmxmul(a_re_p, a_im_p, a_re, a_im, b_re, b_im, vmr, ##__VA_ARGS__)
// #define vmnot(vec, ...) __Venus_mnot(vec, ##__VA_ARGS__)
#define vshuffle_test(retvec, idxvec, tgtvec, sg, ...)                         \
  retvec = __Venus_shuffle_test(retvec, idxvec, tgtvec, sg, ##__VA_ARGS__)
#define vrange(retvec, ...) retvec = __Venus_range(retvec, ##__VA_ARGS__)
#define vclaim(retvec, ...) retvec = __Venus_claim(retvec, ##__VA_ARGS__)
#define vbrdcst(retvec, val, vmr, ...)                                         \
  retvec = __Venus_brdcst(retvec, val, vmr, ##__VA_ARGS__)
// #define vshuffle(retvec, idxvec, tgtvec, sg, ...)                              \
//   retvec = __Venus_shuffle(retvec, idxvec, tgtvec, sg, ##__VA_ARGS__)
#define vshuffle(retvec, idxvec, tgtvec, sg, ...)                              \
  retvec = __Venus_shuffle_test(retvec, idxvec, tgtvec, sg, ##__VA_ARGS__)
// retvec = __Venus_shuffle(retvec, idxvec, tgtvec, sg, ##__VA_ARGS__)
// #define vshuffle(retvec, idxvec, tgtvec, sg, ...)                              
#define vredand(a, vmr, ...) __Venus_redand(a, 0xFFFF, vmr, ##__VA_ARGS__)
#define vredor(a, vmr, ...) __Venus_redor(a, 0, vmr, ##__VA_ARGS__)
#define vredxor(a, vmr, ...) __Venus_redxor(a, 0, vmr, ##__VA_ARGS__)
#define vredmax8(a, vmr, ...) __Venus_redmax(a, 0x8080, vmr, ##__VA_ARGS__)
#define vredmax16(a, vmr, ...) __Venus_redmax(a, 0x8000, vmr, ##__VA_ARGS__)
#define vredmaxu(a, vmr, ...) __Venus_redmaxu(a, 0, vmr, ##__VA_ARGS__)
#define vredmin8(a, vmr, ...) __Venus_redmin(a, 0x7F7F, vmr, ##__VA_ARGS__)
#define vredmin16(a, vmr, ...) __Venus_redmin(a, 0x7FFF, vmr, ##__VA_ARGS__)
#define vredminu(a, vmr, ...) __Venus_redminu(a, 0xFFFF, vmr, ##__VA_ARGS__)
#define vredsum(a, vmr, ...) __Venus_redsum(a, 0, vmr, ##__VA_ARGS__)
#define vaddr(vec) __Venus_vaddr(vec)
#define vreturn(retarg, retlen, ...)                                           \
  __Venus_return(retarg, retlen, ##__VA_ARGS__)
#define vsetcsr(addr, value) __Venus_setcsr(addr, value)
#define vbarrier() __asm__("addi x0,x0,0");__asm__("addi x0,x0,0");__asm__("addi x0,x0,0");__asm__("addi x0,x0,0");__asm__("addi x0,x0,0");__asm__("addi x0,x0,0");__asm__("addi x0,x0,0");__asm__("addi x0,x0,0");__asm__("addi x0,x0,0");__asm__("addi x0,x0,0"); __Venus_barrier()
#define vpseudo(a, b, vmr, ...) __Venus_pseudo(a, b, vmr, ##__VA_ARGS__)
#define vsync() __asm__("addi x0,x0,0");__asm__("addi x0,x0,0");__asm__("addi x0,x0,0");__asm__("addi x0,x0,0");__asm__("addi x0,x0,0");__asm__("addi x0,x0,0");__asm__("addi x0,x0,0");__asm__("addi x0,x0,0");vbarrier();__asm__("wfi");__asm__("addi x0,x0,0");__asm__("addi x0,x0,0");__asm__("addi x0,x0,0");__asm__("addi x0,x0,0");__asm__("addi x0,x0,0");__asm__("addi x0,x0,0");__asm__("addi x0,x0,0");__asm__("addi x0,x0,0");__asm__("addi x0,x0,0");__asm__("addi x0,x0,0")

#define vsetshamt(amount) vsetcsr(VCSR_MULSHAMT, amount)
#define vsetmsbhead(amount) vsetcsr(VCSR_MSBHEAD, amount)
#define vsetsaturate(amount) vsetcsr(VCSR_MULSATURATE, amount)
#define vsetLSUmsb(amount) vsetcsr(VCSR_MSBLSU, amount)
#define vsetEXTENSIONcfg(amount) vsetcsr(VCSR_EXTCFG, amount)

#define vload(retvec, val, vmr, ...) retvec = __Venus_load(retvec, val, vmr, ##__VA_ARGS__)
#define vstore(val, a, vmr, ...) __Venus_store(val, a, vmr, ##__VA_ARGS__)

// python3 gen_venustype_h.py
#include "venustype.h"