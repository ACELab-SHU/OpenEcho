/**
 * ****************************************
 * @file        Task_lteChannelEstimate.c
 * @brief       channel estimation using LS
 * @author      shenyihao
 * @date        2025.3.12
 * @copyright   ACE-Lab(Shanghai University)
 * ****************************************
 */

 #include "data_type.h"
 #include "riscv_printf.h"
 #include "venus.h"
 
 typedef short __v2048i16 __attribute__((ext_vector_type(2048)));
 typedef char  __v4096i8 __attribute__((ext_vector_type(4096)));
 
 #define LEFT_SHIFT  0
 #define RIGHT_SHIFT 1
 VENUS_INLINE __v2048i16 cyclic_shift_2048_16(__v2048i16 tgtvec, int shift_length, int shift_direction,
                                              int vectorLength) {
   __v2048i16 result;
   __v2048i16 tmp_index;
   vclaim(tmp_index);
   __v2048i16 tmp_1;
   vclaim(tmp_1);
   vbrdcst(tmp_1, vectorLength, MASKREAD_OFF, vectorLength);
   vrange(tmp_index, vectorLength);
   tmp_index = vsadd(tmp_index, shift_direction == LEFT_SHIFT ? shift_length : (vectorLength - shift_length),
                     MASKREAD_OFF, vectorLength);
   tmp_index = vrem(tmp_1, tmp_index, MASKREAD_OFF, vectorLength);
   vclaim(result);
   vshuffle(result, tmp_index, tgtvec, SHUFFLE_GATHER, vectorLength);
   // vshuffle(retvec, tmp_index, tgtvec, SHUFFLE_GATHER, vectorLength);
   return result;
 }
 
 VENUS_INLINE __v4096i8 cyclic_shift_4096_8(__v4096i8 tgtvec, int shift_length, int shift_direction,
                                              int vectorLength) {
   __v4096i8 result;
   __v2048i16 tmp_index;
   vclaim(tmp_index);
   __v2048i16 tmp_1;
   vclaim(tmp_1);
   vbrdcst(tmp_1, vectorLength, MASKREAD_OFF, vectorLength);
   vrange(tmp_index, vectorLength);
   tmp_index = vsadd(tmp_index, shift_direction == LEFT_SHIFT ? shift_length : (vectorLength - shift_length),
                     MASKREAD_OFF, vectorLength);
   tmp_index = vrem(tmp_1, tmp_index, MASKREAD_OFF, vectorLength);
   vclaim(result);
   vshuffle(result, tmp_index, tgtvec, SHUFFLE_GATHER, vectorLength);
   // vshuffle(retvec, tmp_index, tgtvec, SHUFFLE_GATHER, vectorLength);
   return result;
 }
 

 
 // short dmrs_symbol_index[2] = {1, 3};
 
 int Task_lteChannelEstimate(__v4096i8 rxData_real_in, __v4096i8 rxData_imag_in, __v4096i8 dmrs_real, __v4096i8 dmrs_imag,
                            __v2048i16 dmrs_index_in, __v4096i8 global_coef_h_in, __v2048i16 dmrs_symbol_index, short_struct input_subcarrierLength,
                            short_struct input_data_symbol_length, short_struct input_dmrs_interval,
                            short_struct input_dmrs_ref_length, short_struct input_dmrs_symbol_length) {
 
   /* 输入接收数据，DMRS索引，DMRS参考信号，DMRS间隔，单个符号的DMRS长度，单个符号子载波长度，DMRS时域符号编号和时域符号长度,时域总符号长度。*/
   // short subcarrierLength   = 12;

   
   
   short subcarrierLength   = input_subcarrierLength.data;
   short data_symbol_length = input_data_symbol_length.data;
   short dmrs_interval      = input_dmrs_interval.data;
   short dmrs_ref_length    = input_dmrs_ref_length.data;
   short dmrs_symbol_length = input_dmrs_symbol_length.data;
 

  //  dmrs_symbol_index = vsadd(dmrs_symbol_index, 0, MASKREAD_OFF, 100);
   int fractionLength = 6;
  //  printf("subcarrierLength: %hd\n",&subcarrierLength);
  //  printf("data_symbol_length: %hd\n",&data_symbol_length);
  //  printf("dmrs_interval: %hd\n",&dmrs_interval);
  //  printf("dmrs_ref_length: %hd\n",&dmrs_ref_length);
  //  printf("dmrs_symbol_length: %hd\n",&dmrs_symbol_length);
 
   __v4096i8 rxData_real;
   __v4096i8 rxData_imag;
   rxData_real = vsadd(rxData_real_in, 0, MASKREAD_OFF, subcarrierLength * data_symbol_length);
   rxData_imag = vsadd(rxData_imag_in, 0, MASKREAD_OFF, subcarrierLength * data_symbol_length);
 
 
   __v2048i16 const_value;
   vclaim(const_value);
   vbrdcst(const_value, subcarrierLength, MASKREAD_OFF, (dmrs_ref_length * dmrs_symbol_length));

   __v2048i16 dmrs_index;
   vclaim(dmrs_index);
   dmrs_index = vrem(const_value, dmrs_index_in, MASKREAD_OFF, (dmrs_ref_length * dmrs_symbol_length));
 
   // /*--------------------input global coef--------------------*/
   // __v4096i8 global_coef_h;
   // vclaim(global_coef_h);
   // vbarrier();
   // VSPM_OPEN();
   // int global_coef_h_addr = vaddr(global_coef_h);
   // for (int i = 0; i < 4096; ++i) {
   //   *(volatile unsigned char *)(global_coef_h_addr + i) = input_coef_h[i];
   // }
   // VSPM_CLOSE();
 
   __v4096i8 const_value_8bit;
   vclaim(const_value_8bit);
   vbrdcst(const_value_8bit, dmrs_interval, MASKREAD_OFF, 4096);
   __v4096i8 global_coef_h;
   vclaim(global_coef_h);
   global_coef_h = vrem(const_value_8bit, global_coef_h_in, MASKREAD_OFF, 4096);
 
   /************************************ init output & temp parameters **********************************/
   __v4096i8 h_real;
   __v4096i8 h_imag;
   __v2048i16 global_data_shuffle_index;
   __v2048i16 global_ref_shuffle_index;
   vclaim(h_real);
   vclaim(h_imag);
   vbrdcst(h_real, 1, MASKREAD_OFF, subcarrierLength * data_symbol_length);
   vbrdcst(h_imag, 1, MASKREAD_OFF, subcarrierLength * data_symbol_length);
   vclaim(global_data_shuffle_index);
   vclaim(global_ref_shuffle_index);
 
   /************************************ LS & Frequence Interpolate **********************************/
   for (int i = 0; i < dmrs_symbol_length; ++i) {
     int dmrs_symbol_index_addr = vaddr(dmrs_symbol_index);
     vbarrier();
     VSPM_OPEN();
     int cur_symbol_index = *(volatile short *)(dmrs_symbol_index_addr + (i << 1));
     VSPM_CLOSE();

    //  printf("%d\n",&cur_symbol_index);
 
     vrange(global_data_shuffle_index, subcarrierLength);
     vrange(global_ref_shuffle_index, dmrs_ref_length);
 
     global_data_shuffle_index =
         vadd(global_data_shuffle_index, cur_symbol_index * subcarrierLength, MASKREAD_OFF, subcarrierLength);
     global_ref_shuffle_index = vadd(global_ref_shuffle_index, i * dmrs_ref_length, MASKREAD_OFF, dmrs_ref_length);
 
     /*--------------------split data & dmrs--------------------*/
     __v4096i8 symbol_rxData_real;
     __v4096i8 symbol_rxData_imag;
     vclaim(symbol_rxData_real);
     vclaim(symbol_rxData_imag);
     vshuffle(symbol_rxData_real, global_data_shuffle_index, rxData_real, SHUFFLE_GATHER, subcarrierLength);
     vshuffle(symbol_rxData_imag, global_data_shuffle_index, rxData_imag, SHUFFLE_GATHER, subcarrierLength);
     __v4096i8 symbol_dmrs_real;
     __v4096i8 symbol_dmrs_imag;
     vclaim(symbol_dmrs_real);
     vclaim(symbol_dmrs_imag);
     vshuffle(symbol_dmrs_real, global_ref_shuffle_index, dmrs_real, SHUFFLE_GATHER, dmrs_ref_length);
     vshuffle(symbol_dmrs_imag, global_ref_shuffle_index, dmrs_imag, SHUFFLE_GATHER, dmrs_ref_length);
     __v2048i16 symbol_dmrs_index;
     vclaim(symbol_dmrs_index);
     vshuffle(symbol_dmrs_index, global_ref_shuffle_index, dmrs_index, SHUFFLE_GATHER, dmrs_ref_length);
 
     /*--------------------obtain rx dmrs--------------------*/
     __v4096i8 rxDmrs_real;
     __v4096i8 rxDmrs_imag;
     vclaim(rxDmrs_real);
     vclaim(rxDmrs_imag);
     vshuffle(rxDmrs_real, symbol_dmrs_index, symbol_rxData_real, SHUFFLE_GATHER, dmrs_ref_length);
     vshuffle(rxDmrs_imag, symbol_dmrs_index, symbol_rxData_imag, SHUFFLE_GATHER, dmrs_ref_length);
 
     /*--------------------LS estimate--------------------*/
     __v4096i8 temp_h_real;
     __v4096i8 temp_h_imag;
     __v4096i8 ac_temp_h_real;
     __v4096i8 bd_temp_h_real;
     __v4096i8 ad_temp_h_imag;
     __v4096i8 bc_temp_h_imag;
     __v4096i8 c2_temp_h;
     __v4096i8 d2_temp_h;
     __v4096i8 c2_add_d2_temp_h;
     // rxDmrs./input_dmrs
     vsetshamt(fractionLength);
     ac_temp_h_real = vmul(rxDmrs_real, symbol_dmrs_real, MASKREAD_OFF, dmrs_ref_length);
     bd_temp_h_real = vmul(rxDmrs_imag, symbol_dmrs_imag, MASKREAD_OFF, dmrs_ref_length);
     vsetshamt(0);
     temp_h_real = vsadd(ac_temp_h_real, bd_temp_h_real, MASKREAD_OFF, dmrs_ref_length);
 
     vsetshamt(fractionLength);
     ad_temp_h_imag = vmul(rxDmrs_real, symbol_dmrs_imag, MASKREAD_OFF, dmrs_ref_length);
     bc_temp_h_imag = vmul(rxDmrs_imag, symbol_dmrs_real, MASKREAD_OFF, dmrs_ref_length);
     vsetshamt(0);
     temp_h_imag = vssub(ad_temp_h_imag, bc_temp_h_imag, MASKREAD_OFF, dmrs_ref_length); // bc_temp_h_imag-ad_temp_h_imag
 
     /*--------------------linear interpolation--------------------*/
     __v4096i8 base_h_real;
     __v4096i8 base_h_imag;
     __v4096i8 weight_h_real;
     __v4096i8 weight_h_imag;
     __v4096i8 coef_h;
 
     // prepare weight
     __v4096i8 temp_h_shift_real;
     __v4096i8 temp_h_shift_imag;
 
     temp_h_shift_real = cyclic_shift_4096_8(temp_h_real, 1, LEFT_SHIFT, dmrs_ref_length);
     temp_h_shift_imag = cyclic_shift_4096_8(temp_h_imag, 1, LEFT_SHIFT, dmrs_ref_length);
     weight_h_real     = vssub(temp_h_real, temp_h_shift_real, MASKREAD_OFF, dmrs_ref_length);
     weight_h_imag     = vssub(temp_h_imag, temp_h_shift_imag, MASKREAD_OFF, dmrs_ref_length);
     __v2048i16 dmrs_interval_index_vec;
     __v4096i8 dmrs_interval_vec;
     vclaim(dmrs_interval_vec);
     vclaim(dmrs_interval_index_vec);
     vbrdcst(dmrs_interval_vec, dmrs_interval, MASKREAD_OFF, subcarrierLength);       // trick
     vbrdcst(dmrs_interval_index_vec, dmrs_interval, MASKREAD_OFF, subcarrierLength); // trick
     weight_h_real = vdiv(dmrs_interval_vec, weight_h_real, MASKREAD_OFF, dmrs_ref_length);
     weight_h_imag = vdiv(dmrs_interval_vec, weight_h_imag, MASKREAD_OFF, dmrs_ref_length);
 
     // obtian parameter
     __v2048i16 dmrs_shuffle_index;
     __v2048i16 base_dmrs_shuffle_index;
     __v2048i16 weight_dmrs_shuffle_index;
     vclaim(dmrs_shuffle_index);
     vrange(dmrs_shuffle_index, subcarrierLength);
     base_dmrs_shuffle_index   = vdiv(dmrs_interval_index_vec, dmrs_shuffle_index, MASKREAD_OFF, subcarrierLength);
     weight_dmrs_shuffle_index = vdiv(dmrs_interval_index_vec, dmrs_shuffle_index, MASKREAD_OFF, subcarrierLength);
     vclaim(coef_h);
     vbrdcst(coef_h, 0, MASKREAD_OFF, subcarrierLength);
     coef_h = vadd(coef_h, global_coef_h, MASKREAD_OFF, subcarrierLength);
     // base_h和coef_h前几个位置特殊处理 weight_h前几个和后几个位置特殊处理
     short left_index = 0, right_index = 0;
     int   dmrs_index_addr = vaddr(symbol_dmrs_index);
     vbarrier();
     VSPM_OPEN();
     left_index = *(volatile short *)(dmrs_index_addr + (0 << 1));
     VSPM_CLOSE();
     dmrs_index_addr = vaddr(symbol_dmrs_index);
     vbarrier();
     VSPM_OPEN();
     right_index = *(volatile short *)(dmrs_index_addr + ((dmrs_ref_length - 1) << 1));
     VSPM_CLOSE();
     base_dmrs_shuffle_index = cyclic_shift_2048_16(base_dmrs_shuffle_index, left_index, RIGHT_SHIFT, subcarrierLength);
     weight_dmrs_shuffle_index =
         cyclic_shift_2048_16(weight_dmrs_shuffle_index, left_index, RIGHT_SHIFT, subcarrierLength);
     coef_h = cyclic_shift_4096_8(coef_h, left_index, RIGHT_SHIFT, subcarrierLength);
     vbarrier();
     VSPM_OPEN();
     int base_dmrs_shuffle_index_addr = vaddr(base_dmrs_shuffle_index);
     for (int i = 0; i < left_index; ++i) {
       *(volatile short *)(base_dmrs_shuffle_index_addr + (i << 1)) = 0;
     }
     VSPM_CLOSE();
 
     vbarrier();
     VSPM_OPEN();
     int weight_dmrs_shuffle_index_addr = vaddr(weight_dmrs_shuffle_index);
     for (int i = 0; i < left_index; ++i) {
       *(volatile short *)(weight_dmrs_shuffle_index_addr + (i << 1)) = 0;
     }
     VSPM_CLOSE();
 
     vbarrier();
     VSPM_OPEN();
     int coef_h_addr = vaddr(coef_h);
     for (int i = 0; i < left_index; ++i) {
       *(volatile char *)(coef_h_addr + i) = -(left_index - i);
     }
     VSPM_CLOSE();
 
     vbarrier();
     VSPM_OPEN();
     weight_dmrs_shuffle_index_addr = vaddr(weight_dmrs_shuffle_index);
     for (int i = right_index; i < subcarrierLength; ++i) {
       *(volatile short *)(weight_dmrs_shuffle_index_addr + (i << 1)) = dmrs_ref_length - 1 - 1;
     }
     VSPM_CLOSE();
 
     vclaim(base_h_real);
     vclaim(base_h_imag);
     vshuffle(base_h_real, base_dmrs_shuffle_index, temp_h_real, SHUFFLE_GATHER, subcarrierLength);
     vshuffle(base_h_imag, base_dmrs_shuffle_index, temp_h_imag, SHUFFLE_GATHER, subcarrierLength);

     __v4096i8 weight_h_real_a;
     __v4096i8 weight_h_imag_a;
     vclaim(weight_h_real_a);
     vclaim(weight_h_imag_a);
     vshuffle(weight_h_real_a, weight_dmrs_shuffle_index, weight_h_real, SHUFFLE_GATHER, subcarrierLength);
     vshuffle(weight_h_imag_a, weight_dmrs_shuffle_index, weight_h_imag, SHUFFLE_GATHER, subcarrierLength);
 
     __v4096i8 h_real_symbol;
     __v4096i8 h_imag_symbol;
     // h_real_symbol = vmuladd(weight_h_real, coef_h, base_h_real, MASKREAD_OFF, subcarrierLength);
     // h_imag_symbol = vmuladd(weight_h_imag, coef_h, base_h_imag, MASKREAD_OFF, subcarrierLength);
     h_real_symbol = vmul(weight_h_real_a, coef_h, MASKREAD_OFF, subcarrierLength);
     h_imag_symbol = vmul(weight_h_imag_a, coef_h, MASKREAD_OFF, subcarrierLength);
     h_real_symbol = vsadd(h_real_symbol, base_h_real, MASKREAD_OFF, subcarrierLength);
     h_imag_symbol = vsadd(h_imag_symbol, base_h_imag, MASKREAD_OFF, subcarrierLength);
 
     vshuffle(h_real, global_data_shuffle_index, h_real_symbol, SHUFFLE_SCATTER, subcarrierLength);
     vshuffle(h_imag, global_data_shuffle_index, h_imag_symbol, SHUFFLE_SCATTER, subcarrierLength);
     // VENUS_PRINTVEC_CHAR(h_imag, 10);
   }
 
   /************************************ Time Interpolate **********************************/
   if (dmrs_symbol_length == 1) {
     // 只有一个dmrs，全部最近邻插值
     for (int i = 0; i < data_symbol_length; ++i) {
       int dmrs_symbol_index_addr = vaddr(dmrs_symbol_index);
       vbarrier();
       VSPM_OPEN();
       int one_dmrs_symbol_index = *(volatile short *)(dmrs_symbol_index_addr + (i << 1));
       VSPM_CLOSE();
 
       __v4096i8 h_real_symbol;
       __v4096i8 h_imag_symbol;
       vclaim(h_real_symbol);
       vclaim(h_imag_symbol);
       vrange(global_data_shuffle_index, subcarrierLength);
       global_data_shuffle_index =
           vsadd(global_data_shuffle_index, one_dmrs_symbol_index * subcarrierLength, MASKREAD_OFF, subcarrierLength);
       vshuffle(h_real_symbol, global_data_shuffle_index, h_real, SHUFFLE_GATHER, subcarrierLength);
       vshuffle(h_imag_symbol, global_data_shuffle_index, h_imag, SHUFFLE_GATHER, subcarrierLength);
 
       for (int j = 0; j < one_dmrs_symbol_index; ++j) {
         vrange(global_data_shuffle_index, subcarrierLength);
         global_data_shuffle_index =
             vsadd(global_data_shuffle_index, j * subcarrierLength, MASKREAD_OFF, subcarrierLength);
         vshuffle(h_real, global_data_shuffle_index, h_real_symbol, SHUFFLE_SCATTER, subcarrierLength);
         vshuffle(h_imag, global_data_shuffle_index, h_imag_symbol, SHUFFLE_SCATTER, subcarrierLength);
       }
       for (int j = one_dmrs_symbol_index; j < data_symbol_length; ++j) {
         vrange(global_data_shuffle_index, subcarrierLength);
         global_data_shuffle_index =
             vsadd(global_data_shuffle_index, j * subcarrierLength, MASKREAD_OFF, subcarrierLength);
         vshuffle(h_real, global_data_shuffle_index, h_real_symbol, SHUFFLE_SCATTER, subcarrierLength);
         vshuffle(h_imag, global_data_shuffle_index, h_imag_symbol, SHUFFLE_SCATTER, subcarrierLength);
       }
     }
   } else {
     // 多个dmrs，线性插值
     for (int i = 0; i < dmrs_symbol_length - 1; ++i) {
       __v4096i8 h_real_front_sym;
       __v4096i8 h_imag_front_sym;
       __v4096i8 h_real_tail_sym;
       __v4096i8 h_imag_tail_sym;
       vclaim(h_real_front_sym);
       vclaim(h_imag_front_sym);
       vclaim(h_real_tail_sym);
       vclaim(h_imag_tail_sym);
 
       int dmrs_symbol_index_addr = vaddr(dmrs_symbol_index);
       vbarrier();
       VSPM_OPEN();
       int front_symbol_index = *(volatile short *)(dmrs_symbol_index_addr + (i << 1));
       VSPM_CLOSE();
       vrange(global_data_shuffle_index, subcarrierLength);
       global_data_shuffle_index =
           vsadd(global_data_shuffle_index, front_symbol_index * subcarrierLength, MASKREAD_OFF, subcarrierLength);
       vshuffle(h_real_front_sym, global_data_shuffle_index, h_real, SHUFFLE_GATHER, subcarrierLength);
       vshuffle(h_imag_front_sym, global_data_shuffle_index, h_imag, SHUFFLE_GATHER, subcarrierLength);
 
       int dmrs_symbol_index_addr1 = vaddr(dmrs_symbol_index);
       vbarrier();
       VSPM_OPEN();
       int tail_symbol_index = *(volatile short *)(dmrs_symbol_index_addr1 + ((i + 1) << 1));
       VSPM_CLOSE();
       global_data_shuffle_index =
           vsadd(global_data_shuffle_index, (tail_symbol_index - front_symbol_index) * subcarrierLength, MASKREAD_OFF,
                subcarrierLength);
       vshuffle(h_real_tail_sym, global_data_shuffle_index, h_real, SHUFFLE_GATHER, subcarrierLength);
       vshuffle(h_imag_tail_sym, global_data_shuffle_index, h_imag, SHUFFLE_GATHER, subcarrierLength);
 
       __v4096i8 const_value_vec;
       vclaim(const_value_vec);
       vbrdcst(const_value_vec, (tail_symbol_index - front_symbol_index), MASKREAD_OFF, subcarrierLength);
       __v4096i8 h_real_diff   = vsub(h_real_front_sym, h_real_tail_sym, MASKREAD_OFF, subcarrierLength);
       __v4096i8 h_imag_diff   = vsub(h_imag_front_sym, h_imag_tail_sym, MASKREAD_OFF, subcarrierLength);
       __v4096i8 h_real_weight = vdiv(const_value_vec, h_real_diff, MASKREAD_OFF, subcarrierLength);
       __v4096i8 h_imag_weight = vdiv(const_value_vec, h_imag_diff, MASKREAD_OFF, subcarrierLength);
 
       // fisrt dmrs symbol
       if (i == 0) {
         for (int j = 0; j < front_symbol_index; ++j) {
           vbrdcst(const_value_vec, j - front_symbol_index, MASKREAD_OFF, subcarrierLength);
           __v4096i8 h_real_symbol;
           __v4096i8 h_imag_symbol;
           // h_real_symbol = vmuladd(h_real_weight, const_value_vec, h_real_front_sym, MASKREAD_OFF, subcarrierLength);
           // h_imag_symbol = vmuladd(h_imag_weight, const_value_vec, h_imag_front_sym, MASKREAD_OFF, subcarrierLength);
           h_real_symbol = vmul(h_real_weight, const_value_vec, MASKREAD_OFF, subcarrierLength);
           h_imag_symbol = vmul(h_imag_weight, const_value_vec, MASKREAD_OFF, subcarrierLength);
           h_real_symbol = vsadd(h_real_symbol, h_real_front_sym, MASKREAD_OFF, subcarrierLength);
           h_imag_symbol = vsadd(h_imag_symbol, h_imag_front_sym, MASKREAD_OFF, subcarrierLength);
           vrange(global_data_shuffle_index, subcarrierLength);
           global_data_shuffle_index =
               vsadd(global_data_shuffle_index, j * subcarrierLength, MASKREAD_OFF, subcarrierLength);
           vshuffle(h_real, global_data_shuffle_index, h_real_symbol, SHUFFLE_SCATTER, subcarrierLength);
           vshuffle(h_imag, global_data_shuffle_index, h_imag_symbol, SHUFFLE_SCATTER, subcarrierLength);
         }
       }
 
       for (int j = front_symbol_index + 1; j < tail_symbol_index; ++j) {
         vbrdcst(const_value_vec, j - front_symbol_index, MASKREAD_OFF, subcarrierLength);
         __v4096i8 h_real_symbol;
         __v4096i8 h_imag_symbol;
         // h_real_symbol = vmuladd(h_real_weight, const_value_vec, h_real_front_sym, MASKREAD_OFF, subcarrierLength);
         // h_imag_symbol = vmuladd(h_imag_weight, const_value_vec, h_imag_front_sym, MASKREAD_OFF, subcarrierLength);
         h_real_symbol = vmul(h_real_weight, const_value_vec, MASKREAD_OFF, subcarrierLength);
         h_imag_symbol = vmul(h_imag_weight, const_value_vec, MASKREAD_OFF, subcarrierLength);
         h_real_symbol = vsadd(h_real_symbol, h_real_front_sym, MASKREAD_OFF, subcarrierLength);
         h_imag_symbol = vsadd(h_imag_symbol, h_imag_front_sym, MASKREAD_OFF, subcarrierLength);
         vrange(global_data_shuffle_index, subcarrierLength);
         global_data_shuffle_index =
             vsadd(global_data_shuffle_index, j * subcarrierLength, MASKREAD_OFF, subcarrierLength);
         vshuffle(h_real, global_data_shuffle_index, h_real_symbol, SHUFFLE_SCATTER, subcarrierLength);
         vshuffle(h_imag, global_data_shuffle_index, h_imag_symbol, SHUFFLE_SCATTER, subcarrierLength);
       }
 
       // last dmrs symbol
       if (i == dmrs_symbol_length - 2) {
         for (int j = tail_symbol_index + 1; j < data_symbol_length; ++j) {
           vbrdcst(const_value_vec, j - tail_symbol_index, MASKREAD_OFF, subcarrierLength);
           __v4096i8 h_real_symbol;
           __v4096i8 h_imag_symbol;
           // h_real_symbol = vmuladd(h_real_weight, const_value_vec, h_real_tail_sym, MASKREAD_OFF, subcarrierLength);
           // h_imag_symbol = vmuladd(h_imag_weight, const_value_vec, h_imag_tail_sym, MASKREAD_OFF, subcarrierLength);
           h_real_symbol = vmul(h_real_weight, const_value_vec, MASKREAD_OFF, subcarrierLength);
           h_imag_symbol = vmul(h_imag_weight, const_value_vec, MASKREAD_OFF, subcarrierLength);
           h_real_symbol = vsadd(h_real_symbol, h_real_tail_sym, MASKREAD_OFF, subcarrierLength);
           h_imag_symbol = vsadd(h_imag_symbol, h_imag_tail_sym, MASKREAD_OFF, subcarrierLength);
           vrange(global_data_shuffle_index, subcarrierLength);
           global_data_shuffle_index =
               vsadd(global_data_shuffle_index, j * subcarrierLength, MASKREAD_OFF, subcarrierLength);
           vshuffle(h_real, global_data_shuffle_index, h_real_symbol, SHUFFLE_SCATTER, subcarrierLength);
           vshuffle(h_imag, global_data_shuffle_index, h_imag_symbol, SHUFFLE_SCATTER, subcarrierLength);
         }
       }
     }
   }
   // VENUS_PRINTVEC_CHAR(h_real, subcarrierLength * data_symbol_length);
 
   vreturn(h_real, sizeof(h_real), h_imag, sizeof(h_imag));
 }