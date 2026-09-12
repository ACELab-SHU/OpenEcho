#include "riscv_printf.h"
#include "venus.h"
#include <stdint.h>

// Version 1.0 --- LTE 128点fft    2024/12/02
// Created by SYH

typedef char __v4096i8 __attribute__((ext_vector_type(4096)));
typedef short __v2048i16 __attribute__((ext_vector_type(2048)));

// ------------------------- Set fixed point -------------------------
// 2048点FFT的定点化 [6 6 5 5 5 5 4 3 2 1 0]
short fft_fixed_vec[11] = {8, 7, 7, 7, 8, 7, 7, 8, 8, 8, 8};
short fft_shift_vec[11] = {1, 0, 0, 0, 1, 0, 0, 1, 1, 1, 1};

// 1024点fft的定点化 [6 6 6 5 5 4 4 3 2 2]
short fft1024_fixed_vec[10] = {8, 7, 7, 8, 7, 8, 7, 8, 8, 7};
short fft1024_shift_vec[10] = {1, 0, 0, 1, 0, 1, 0, 1, 1, 0};

// 128点的fft定点化 [6 5 5 5 4 4 4]
short fft128_fixed_vec[7] = {8, 8, 7, 7, 8, 7, 7};

// short fft128_fixed_vec[7] = {8, 8, 7, 7, 8, 7, 7};
short fft128_shift_vec[7] = {1, 1, 0, 0, 1, 0, 0};

short symbol_offset[14] = {5, 4, 4, 4, 4, 4, 4, 5, 4, 4, 4, 4, 4, 4};

// 输入定点化的点数，实现定点乘法的输出
VENUS_INLINE __v4096i8 MUL4096_8_FIXED(__v4096i8 a, __v4096i8 b, int fix_point, int length) {
  __v4096i8 high8;
  __v4096i8 low8;
  __v4096i8 result;
  short high_shift = 8 - fix_point;

  low8 = vmul(a, b, MASKREAD_OFF, length);
  high8 = vmulh(a, b, MASKREAD_OFF, length);
  low8 = vsrl(low8, fix_point, MASKREAD_OFF, length);
  high8 = vsll(high8, high_shift, MASKREAD_OFF, length);
  result = vor(low8, high8, MASKREAD_OFF, length);
  return result;
};

typedef struct {
  short data;
} __attribute__((aligned(64))) short_struct;

int Task_lteOFDMDemodulation128(__v4096i8 vin_real, __v4096i8 vin_imag, short_struct symbolNum, __v4096i8 cos_stage4,
                                __v4096i8 cos_stage5, __v4096i8 cos_stage6, __v4096i8 cos_stage7, __v4096i8 cos_stage8,
                                __v4096i8 cos_stage9, __v4096i8 cos_stage10, __v4096i8 sin_stage4, __v4096i8 sin_stage5,
                                __v4096i8 sin_stage6, __v4096i8 sin_stage7, __v4096i8 sin_stage8, __v4096i8 sin_stage9,
                                __v4096i8 sin_stage10, __v2048i16 shuffle_add_stage0, __v2048i16 shuffle_add_stage1,
                                __v2048i16 shuffle_add_stage2, __v2048i16 shuffle_add_stage3,
                                __v2048i16 shuffle_add_stage4, __v2048i16 shuffle_add_stage5,
                                __v2048i16 shuffle_add_stage6, __v2048i16 shuffle_wn_stage0,
                                __v2048i16 shuffle_wn_stage1, __v2048i16 shuffle_wn_stage2,
                                __v2048i16 shuffle_wn_stage3, __v2048i16 shuffle_wn_stage4,
                                __v2048i16 shuffle_wn_stage5, __v2048i16 shuffle_wn_stage6, __v4096i8 fftPhase_r,
                                __v4096i8 fftPhase_i, __v2048i16 targetIndices) {

  // input parameter
  uint8_t symbol_num = symbolNum.data;
  int N_FFT = 128;


  // -----------------------------正式开始计算OFDM解调-------------------------------------
  int cp_length = 0;

  if (N_FFT == 128) {
    cp_length = symbol_offset[symbol_num];
  }

  // --------STEP 1 : Remove CP


  __v2048i16 Remove_CP_Index;
  vrange(Remove_CP_Index, N_FFT);
  Remove_CP_Index = vadd(Remove_CP_Index, cp_length, MASKREAD_OFF, N_FFT);

  __v4096i8 Data_without_CP_real;
  __v4096i8 Data_without_CP_imag;
  vclaim(Data_without_CP_real);
  vclaim(Data_without_CP_imag);
  vshuffle(Data_without_CP_real, Remove_CP_Index, vin_real, SHUFFLE_GATHER, N_FFT);
  vshuffle(Data_without_CP_imag, Remove_CP_Index, vin_imag, SHUFFLE_GATHER, N_FFT);


//   Data_without_CP_real = vsra(Data_without_CP_real,1,MASKREAD_OFF,N_FFT);
//   Data_without_CP_imag = vsra(Data_without_CP_imag,1,MASKREAD_OFF,N_FFT);
  //  STEP 1 'END'---------

  //  --------STEP 2 : FFT

  __v4096i8 OFDM_OutReal;
  __v4096i8 OFDM_OutImag;
  vclaim(OFDM_OutReal);
  vclaim(OFDM_OutImag);
  short calculate_length = 0;
  if (N_FFT == 128) {
    calculate_length = 64;
  } else {
    calculate_length = 0;
  }

  // 从2048点FFT的Wn中提取128点的Wn的shuffle_Index
  __v2048i16 shuffle_for_128_Wn;
  vclaim(shuffle_for_128_Wn);
  vrange(shuffle_for_128_Wn, 2048);
  shuffle_for_128_Wn = vsll(shuffle_for_128_Wn, 4, MASKREAD_OFF, 2048);

  __v4096i8 Wn_cos;
  vclaim(Wn_cos);
  __v4096i8 Wn_sin;
  vclaim(Wn_sin);

  // 向量搬移index
  __v2048i16 copy_2048; // copy_2048 = [0 1 2 ... 2047]
  vclaim(copy_2048);
  vrange(copy_2048, 2048);
  __v2048i16 moveHalf2Front; // move_128to64 = [64 65 66 ... 127]
  moveHalf2Front = vadd(copy_2048, calculate_length, MASKREAD_OFF, calculate_length);

  //  进行计算的四个向量分别为    Data_without_CP_real（up）    Data_without_CP_imag（up）    data_real_down
  //  data_imag_down
  __v4096i8 data_real_down;
  __v4096i8 data_imag_down;
  vclaim(data_real_down);
  vclaim(data_imag_down);

  // FFT每一级计算过程中的中间变量
  __v4096i8 tempAddResult_real;
  vclaim(tempAddResult_real);
  __v4096i8 tempAddResult_imag;
  vclaim(tempAddResult_imag);
  __v4096i8 tempWnResult_real;
  vclaim(tempWnResult_real);
  __v4096i8 tempWnResult_imag;
  vclaim(tempWnResult_imag);

  __v4096i8 *cmxreal_part = &tempWnResult_real;
  __v4096i8 *cmximag_part = &tempWnResult_imag;

  __v4096i8 a_sub_b_real;
  vclaim(a_sub_b_real);
  __v4096i8 a_sub_b_imag;
  vclaim(a_sub_b_imag);
  __v4096i8 cos_tempWnResult;
  __v4096i8 sin_tempWnResult;
  vclaim(cos_tempWnResult);
  vclaim(sin_tempWnResult);

  static int fraction = 0;

  if (N_FFT == 128) {
    // 蝶形计算
    // a ------- (a + b)
    //      |
    // b ------- (a - b)Wn
    //  Stage 0------------------------------------
    vshuffle(data_real_down, moveHalf2Front, Data_without_CP_real, SHUFFLE_GATHER, calculate_length);
    vshuffle(data_imag_down, moveHalf2Front, Data_without_CP_imag, SHUFFLE_GATHER, calculate_length);
    vshuffle(Wn_cos, shuffle_for_128_Wn, cos_stage4, SHUFFLE_GATHER, calculate_length);
    vshuffle(Wn_sin, shuffle_for_128_Wn, sin_stage4, SHUFFLE_GATHER, calculate_length);
    //  a + b
    tempAddResult_real = vsadd(Data_without_CP_real, data_real_down, MASKREAD_OFF, calculate_length);
    tempAddResult_imag = vsadd(Data_without_CP_imag, data_imag_down, MASKREAD_OFF, calculate_length);
    tempAddResult_real = vsra(tempAddResult_real, fft128_shift_vec[0], MASKREAD_OFF, calculate_length);
    tempAddResult_imag = vsra(tempAddResult_imag, fft128_shift_vec[0], MASKREAD_OFF, calculate_length);

    // (a - b)Wn    (可用复数计算代替)
    a_sub_b_real = vssub(data_real_down, Data_without_CP_real, MASKREAD_OFF, calculate_length);
    a_sub_b_imag = vssub(data_imag_down, Data_without_CP_imag, MASKREAD_OFF, calculate_length);
    
    fraction = fft128_fixed_vec[0];
    vsetshamt(fraction);
    cos_tempWnResult = vmul(a_sub_b_real, Wn_cos, MASKREAD_OFF, calculate_length);
    sin_tempWnResult = vmul(a_sub_b_imag, Wn_sin, MASKREAD_OFF, calculate_length);
    vsetshamt(0);

    // cos_tempWnResult = MUL4096_8_FIXED(a_sub_b_real, Wn_cos, fft128_fixed_vec[0], calculate_length);
    // sin_tempWnResult = MUL4096_8_FIXED(a_sub_b_imag, Wn_sin, fft128_fixed_vec[0], calculate_length);
    tempWnResult_real = vssub(sin_tempWnResult, cos_tempWnResult, MASKREAD_OFF, calculate_length);

    vsetshamt(fraction);
    cos_tempWnResult = vmul(a_sub_b_imag, Wn_cos, MASKREAD_OFF, calculate_length);
    sin_tempWnResult = vmul(a_sub_b_real, Wn_sin, MASKREAD_OFF, calculate_length);
    vsetshamt(0);
//     cos_tempWnResult = MUL4096_8_FIXED(a_sub_b_imag, Wn_cos, fft128_fixed_vec[0], calculate_length);
//     sin_tempWnResult = MUL4096_8_FIXED(a_sub_b_real, Wn_sin, fft128_fixed_vec[0], calculate_length);
    tempWnResult_imag = vsadd(cos_tempWnResult, sin_tempWnResult, MASKREAD_OFF, calculate_length);

    tempWnResult_imag = vsadd(tempWnResult_imag, 1, MASKREAD_OFF, calculate_length); // Function FOR OFFSET

//     fraction = fft128_fixed_vec[0];
//     tempWnResult_real = vssub(data_real_down, Data_without_CP_real, MASKREAD_OFF, calculate_length);
//     tempWnResult_imag = vssub(data_imag_down, Data_without_CP_imag, MASKREAD_OFF, calculate_length);
//     vsetshamt(fraction);
//     vcmxmul(cmximag_part, cmxreal_part, tempWnResult_real, tempWnResult_imag, Wn_sin, Wn_cos, MASKREAD_OFF,
//             calculate_length);
//     //     vcmxmul(cmxreal_part, cmximag_part, tempWnResult_imag, tempWnResult_real, Wn_sin, Wn_cos, MASKREAD_OFF,
//     //             calculate_length);
//     vsetshamt(0);

    // 重排序
    vshuffle(Data_without_CP_real, shuffle_add_stage0, tempAddResult_real, SHUFFLE_SCATTER, calculate_length);
    vshuffle(Data_without_CP_real, shuffle_wn_stage0, tempWnResult_real, SHUFFLE_SCATTER, calculate_length);
    vshuffle(Data_without_CP_imag, shuffle_add_stage0, tempAddResult_imag, SHUFFLE_SCATTER, calculate_length);
    vshuffle(Data_without_CP_imag, shuffle_wn_stage0, tempWnResult_imag, SHUFFLE_SCATTER, calculate_length);
    //---------------------------------------------

    //  Stage 1------------------------------------
    vshuffle(data_real_down, moveHalf2Front, Data_without_CP_real, SHUFFLE_GATHER, calculate_length);
    vshuffle(data_imag_down, moveHalf2Front, Data_without_CP_imag, SHUFFLE_GATHER, calculate_length);
    vshuffle(Wn_cos, shuffle_for_128_Wn, cos_stage5, SHUFFLE_GATHER, calculate_length);
    vshuffle(Wn_sin, shuffle_for_128_Wn, sin_stage5, SHUFFLE_GATHER, calculate_length);
    //  a + b
    tempAddResult_real = vsadd(Data_without_CP_real, data_real_down, MASKREAD_OFF, calculate_length);
    tempAddResult_imag = vsadd(Data_without_CP_imag, data_imag_down, MASKREAD_OFF, calculate_length);
    tempAddResult_real = vsra(tempAddResult_real, fft128_shift_vec[1], MASKREAD_OFF, calculate_length);
    tempAddResult_imag = vsra(tempAddResult_imag, fft128_shift_vec[1], MASKREAD_OFF, calculate_length);

    // (a - b)Wn    (可用复数计算代替)
    a_sub_b_real = vssub(data_real_down, Data_without_CP_real, MASKREAD_OFF, calculate_length);
    a_sub_b_imag = vssub(data_imag_down, Data_without_CP_imag, MASKREAD_OFF, calculate_length);


    fraction = fft128_fixed_vec[1];
    vsetshamt(fraction);
    cos_tempWnResult = vmul(a_sub_b_real, Wn_cos, MASKREAD_OFF, calculate_length);
    sin_tempWnResult = vmul(a_sub_b_imag, Wn_sin, MASKREAD_OFF, calculate_length);
    vsetshamt(0);
//     cos_tempWnResult = MUL4096_8_FIXED(a_sub_b_real, Wn_cos, fft128_fixed_vec[1], calculate_length);
//     sin_tempWnResult = MUL4096_8_FIXED(a_sub_b_imag, Wn_sin, fft128_fixed_vec[1], calculate_length);
    tempWnResult_real = vssub(sin_tempWnResult, cos_tempWnResult, MASKREAD_OFF, calculate_length);

    vsetshamt(fraction);
    cos_tempWnResult = vmul(a_sub_b_imag, Wn_cos, MASKREAD_OFF, calculate_length);
    sin_tempWnResult = vmul(a_sub_b_real, Wn_sin, MASKREAD_OFF, calculate_length);
    vsetshamt(0);
//     cos_tempWnResult = MUL4096_8_FIXED(a_sub_b_imag, Wn_cos, fft128_fixed_vec[1], calculate_length);
//     sin_tempWnResult = MUL4096_8_FIXED(a_sub_b_real, Wn_sin, fft128_fixed_vec[1], calculate_length);
    tempWnResult_imag = vsadd(cos_tempWnResult, sin_tempWnResult, MASKREAD_OFF, calculate_length);

    tempWnResult_imag = vsadd(tempWnResult_imag, 1, MASKREAD_OFF, calculate_length); // Function FOR OFFSET

//     fraction = fft128_fixed_vec[1];
//     tempWnResult_real = vssub(data_real_down, Data_without_CP_real, MASKREAD_OFF, calculate_length);
//     tempWnResult_imag = vssub(data_imag_down, Data_without_CP_imag, MASKREAD_OFF, calculate_length);
//     vsetshamt(fraction);
//     vcmxmul(cmximag_part, cmxreal_part, tempWnResult_real, tempWnResult_imag, Wn_sin, Wn_cos, MASKREAD_OFF,
//             calculate_length);
//     //     vcmxmul(cmxreal_part, cmximag_part, tempWnResult_imag, tempWnResult_real, Wn_sin, Wn_cos, MASKREAD_OFF,
//     //             calculate_length);
//     vsetshamt(0);

    // 重排序
    vshuffle(Data_without_CP_real, shuffle_add_stage1, tempAddResult_real, SHUFFLE_SCATTER, calculate_length);
    vshuffle(Data_without_CP_real, shuffle_wn_stage1, tempWnResult_real, SHUFFLE_SCATTER, calculate_length);
    vshuffle(Data_without_CP_imag, shuffle_add_stage1, tempAddResult_imag, SHUFFLE_SCATTER, calculate_length);
    vshuffle(Data_without_CP_imag, shuffle_wn_stage1, tempWnResult_imag, SHUFFLE_SCATTER, calculate_length);
    //---------------------------------------------

    //  Stage 2------------------------------------
    vshuffle(data_real_down, moveHalf2Front, Data_without_CP_real, SHUFFLE_GATHER, calculate_length);
    vshuffle(data_imag_down, moveHalf2Front, Data_without_CP_imag, SHUFFLE_GATHER, calculate_length);
    vshuffle(Wn_cos, shuffle_for_128_Wn, cos_stage6, SHUFFLE_GATHER, calculate_length);
    vshuffle(Wn_sin, shuffle_for_128_Wn, sin_stage6, SHUFFLE_GATHER, calculate_length);
    //  a + b
    tempAddResult_real = vsadd(Data_without_CP_real, data_real_down, MASKREAD_OFF, calculate_length);
    tempAddResult_imag = vsadd(Data_without_CP_imag, data_imag_down, MASKREAD_OFF, calculate_length);
    tempAddResult_real = vsra(tempAddResult_real, fft128_shift_vec[2], MASKREAD_OFF, calculate_length);
    tempAddResult_imag = vsra(tempAddResult_imag, fft128_shift_vec[2], MASKREAD_OFF, calculate_length);

    // (a - b)Wn    (可用复数计算代替)
    a_sub_b_real = vssub(data_real_down, Data_without_CP_real, MASKREAD_OFF, calculate_length);
    a_sub_b_imag = vssub(data_imag_down, Data_without_CP_imag, MASKREAD_OFF, calculate_length);

    fraction = fft128_fixed_vec[2];
    vsetshamt(fraction);
    cos_tempWnResult = vmul(a_sub_b_real, Wn_cos, MASKREAD_OFF, calculate_length);
    sin_tempWnResult = vmul(a_sub_b_imag, Wn_sin, MASKREAD_OFF, calculate_length);
    vsetshamt(0);
//     cos_tempWnResult = MUL4096_8_FIXED(a_sub_b_real, Wn_cos, fft128_fixed_vec[2], calculate_length);
//     sin_tempWnResult = MUL4096_8_FIXED(a_sub_b_imag, Wn_sin, fft128_fixed_vec[2], calculate_length);
    tempWnResult_real = vssub(sin_tempWnResult, cos_tempWnResult, MASKREAD_OFF, calculate_length);

    vsetshamt(fraction);
    cos_tempWnResult = vmul(a_sub_b_imag, Wn_cos, MASKREAD_OFF, calculate_length);
    sin_tempWnResult = vmul(a_sub_b_real, Wn_sin, MASKREAD_OFF, calculate_length);
    vsetshamt(0);
//     cos_tempWnResult = MUL4096_8_FIXED(a_sub_b_imag, Wn_cos, fft128_fixed_vec[2], calculate_length);
//     sin_tempWnResult = MUL4096_8_FIXED(a_sub_b_real, Wn_sin, fft128_fixed_vec[2], calculate_length);
    tempWnResult_imag = vsadd(cos_tempWnResult, sin_tempWnResult, MASKREAD_OFF, calculate_length);

    tempWnResult_imag = vsadd(tempWnResult_imag, 1, MASKREAD_OFF, calculate_length); // Function FOR OFFSET

//     fraction = fft128_fixed_vec[2];
//     tempWnResult_real = vssub(data_real_down, Data_without_CP_real, MASKREAD_OFF, calculate_length);
//     tempWnResult_imag = vssub(data_imag_down, Data_without_CP_imag, MASKREAD_OFF, calculate_length);
//     vsetshamt(fraction);
//     vcmxmul(cmximag_part, cmxreal_part, tempWnResult_real, tempWnResult_imag, Wn_sin, Wn_cos, MASKREAD_OFF,
//             calculate_length);
//     //     vcmxmul(cmxreal_part, cmximag_part, tempWnResult_imag, tempWnResult_real, Wn_sin, Wn_cos, MASKREAD_OFF,
//     //             calculate_length);
//     vsetshamt(0);

    // 重排序
    vshuffle(Data_without_CP_real, shuffle_add_stage2, tempAddResult_real, SHUFFLE_SCATTER, calculate_length);
    vshuffle(Data_without_CP_real, shuffle_wn_stage2, tempWnResult_real, SHUFFLE_SCATTER, calculate_length);
    vshuffle(Data_without_CP_imag, shuffle_add_stage2, tempAddResult_imag, SHUFFLE_SCATTER, calculate_length);
    vshuffle(Data_without_CP_imag, shuffle_wn_stage2, tempWnResult_imag, SHUFFLE_SCATTER, calculate_length);
    //---------------------------------------------

    //  Stage 3------------------------------------
    vshuffle(data_real_down, moveHalf2Front, Data_without_CP_real, SHUFFLE_GATHER, calculate_length);
    vshuffle(data_imag_down, moveHalf2Front, Data_without_CP_imag, SHUFFLE_GATHER, calculate_length);
    vshuffle(Wn_cos, shuffle_for_128_Wn, cos_stage7, SHUFFLE_GATHER, calculate_length);
    vshuffle(Wn_sin, shuffle_for_128_Wn, sin_stage7, SHUFFLE_GATHER, calculate_length);
    //  a + b
    tempAddResult_real = vsadd(Data_without_CP_real, data_real_down, MASKREAD_OFF, calculate_length);
    tempAddResult_imag = vsadd(Data_without_CP_imag, data_imag_down, MASKREAD_OFF, calculate_length);
    tempAddResult_real = vsra(tempAddResult_real, fft128_shift_vec[3], MASKREAD_OFF, calculate_length);
    tempAddResult_imag = vsra(tempAddResult_imag, fft128_shift_vec[3], MASKREAD_OFF, calculate_length);

    // (a - b)Wn    (可用复数计算代替)
    a_sub_b_real = vssub(data_real_down, Data_without_CP_real, MASKREAD_OFF, calculate_length);
    a_sub_b_imag = vssub(data_imag_down, Data_without_CP_imag, MASKREAD_OFF, calculate_length);

    fraction = fft128_fixed_vec[3];
    vsetshamt(fraction);
    cos_tempWnResult = vmul(a_sub_b_real, Wn_cos, MASKREAD_OFF, calculate_length);
    sin_tempWnResult = vmul(a_sub_b_imag, Wn_sin, MASKREAD_OFF, calculate_length);
    vsetshamt(0);
//     cos_tempWnResult = MUL4096_8_FIXED(a_sub_b_real, Wn_cos, fft128_fixed_vec[3], calculate_length);
//     sin_tempWnResult = MUL4096_8_FIXED(a_sub_b_imag, Wn_sin, fft128_fixed_vec[3], calculate_length);
    tempWnResult_real = vssub(sin_tempWnResult, cos_tempWnResult, MASKREAD_OFF, calculate_length);

    vsetshamt(fraction);
    cos_tempWnResult = vmul(a_sub_b_imag, Wn_cos, MASKREAD_OFF, calculate_length);
    sin_tempWnResult = vmul(a_sub_b_real, Wn_sin, MASKREAD_OFF, calculate_length);
    vsetshamt(0);
//     cos_tempWnResult = MUL4096_8_FIXED(a_sub_b_imag, Wn_cos, fft128_fixed_vec[3], calculate_length);
//     sin_tempWnResult = MUL4096_8_FIXED(a_sub_b_real, Wn_sin, fft128_fixed_vec[3], calculate_length);
    tempWnResult_imag = vsadd(cos_tempWnResult, sin_tempWnResult, MASKREAD_OFF, calculate_length);

    tempWnResult_imag = vsadd(tempWnResult_imag, 1, MASKREAD_OFF, calculate_length); // Function FOR OFFSET

//     fraction = fft128_fixed_vec[3];
//     tempWnResult_real = vssub(data_real_down, Data_without_CP_real, MASKREAD_OFF, calculate_length);
//     tempWnResult_imag = vssub(data_imag_down, Data_without_CP_imag, MASKREAD_OFF, calculate_length);
//     vsetshamt(fraction);
//     vcmxmul(cmximag_part, cmxreal_part, tempWnResult_real, tempWnResult_imag, Wn_sin, Wn_cos, MASKREAD_OFF,
//             calculate_length);
//     //     vcmxmul(cmxreal_part, cmximag_part, tempWnResult_imag, tempWnResult_real, Wn_sin, Wn_cos, MASKREAD_OFF,
//     //             calculate_length);
//     vsetshamt(0);

    // 重排序
    vshuffle(Data_without_CP_real, shuffle_add_stage3, tempAddResult_real, SHUFFLE_SCATTER, calculate_length);
    vshuffle(Data_without_CP_real, shuffle_wn_stage3, tempWnResult_real, SHUFFLE_SCATTER, calculate_length);
    vshuffle(Data_without_CP_imag, shuffle_add_stage3, tempAddResult_imag, SHUFFLE_SCATTER, calculate_length);
    vshuffle(Data_without_CP_imag, shuffle_wn_stage3, tempWnResult_imag, SHUFFLE_SCATTER, calculate_length);
    //---------------------------------------------

    //  Stage 4------------------------------------
    vshuffle(data_real_down, moveHalf2Front, Data_without_CP_real, SHUFFLE_GATHER, calculate_length);
    vshuffle(data_imag_down, moveHalf2Front, Data_without_CP_imag, SHUFFLE_GATHER, calculate_length);
    vshuffle(Wn_cos, shuffle_for_128_Wn, cos_stage8, SHUFFLE_GATHER, calculate_length);
    vshuffle(Wn_sin, shuffle_for_128_Wn, sin_stage8, SHUFFLE_GATHER, calculate_length);
    //  a + b
    tempAddResult_real = vsadd(Data_without_CP_real, data_real_down, MASKREAD_OFF, calculate_length);
    tempAddResult_imag = vsadd(Data_without_CP_imag, data_imag_down, MASKREAD_OFF, calculate_length);
    tempAddResult_real = vsra(tempAddResult_real, fft128_shift_vec[4], MASKREAD_OFF, calculate_length);
    tempAddResult_imag = vsra(tempAddResult_imag, fft128_shift_vec[4], MASKREAD_OFF, calculate_length);

    // (a - b)Wn    (可用复数计算代替)
    a_sub_b_real = vssub(data_real_down, Data_without_CP_real, MASKREAD_OFF, calculate_length);
    a_sub_b_imag = vssub(data_imag_down, Data_without_CP_imag, MASKREAD_OFF, calculate_length);

    fraction = fft128_fixed_vec[4];
    vsetshamt(fraction);
    cos_tempWnResult = vmul(a_sub_b_real, Wn_cos, MASKREAD_OFF, calculate_length);
    sin_tempWnResult = vmul(a_sub_b_imag, Wn_sin, MASKREAD_OFF, calculate_length);
    vsetshamt(0);
//     cos_tempWnResult = MUL4096_8_FIXED(a_sub_b_real, Wn_cos, fft128_fixed_vec[4], calculate_length);
//     sin_tempWnResult = MUL4096_8_FIXED(a_sub_b_imag, Wn_sin, fft128_fixed_vec[4], calculate_length);
    tempWnResult_real = vssub(sin_tempWnResult, cos_tempWnResult, MASKREAD_OFF, calculate_length);

    vsetshamt(fraction);
    cos_tempWnResult = vmul(a_sub_b_imag, Wn_cos, MASKREAD_OFF, calculate_length);
    sin_tempWnResult = vmul(a_sub_b_real, Wn_sin, MASKREAD_OFF, calculate_length);
    vsetshamt(0);
//     cos_tempWnResult = MUL4096_8_FIXED(a_sub_b_imag, Wn_cos, fft128_fixed_vec[4], calculate_length);
//     sin_tempWnResult = MUL4096_8_FIXED(a_sub_b_real, Wn_sin, fft128_fixed_vec[4], calculate_length);
    tempWnResult_imag = vsadd(cos_tempWnResult, sin_tempWnResult, MASKREAD_OFF, calculate_length);

    tempWnResult_imag = vsadd(tempWnResult_imag, 1, MASKREAD_OFF, calculate_length); // Function FOR OFFSET

//     fraction = fft128_fixed_vec[4];
//     tempWnResult_real = vssub(data_real_down, Data_without_CP_real, MASKREAD_OFF, calculate_length);
//     tempWnResult_imag = vssub(data_imag_down, Data_without_CP_imag, MASKREAD_OFF, calculate_length);
//     vsetshamt(fraction);
//     vcmxmul(cmximag_part, cmxreal_part, tempWnResult_real, tempWnResult_imag, Wn_sin, Wn_cos, MASKREAD_OFF,
//             calculate_length);
//     //     vcmxmul(cmxreal_part, cmximag_part, tempWnResult_imag, tempWnResult_real, Wn_sin, Wn_cos, MASKREAD_OFF,
//     //             calculate_length);
//     vsetshamt(0);

    // 重排序
    vshuffle(Data_without_CP_real, shuffle_add_stage4, tempAddResult_real, SHUFFLE_SCATTER, calculate_length);
    vshuffle(Data_without_CP_real, shuffle_wn_stage4, tempWnResult_real, SHUFFLE_SCATTER, calculate_length);
    vshuffle(Data_without_CP_imag, shuffle_add_stage4, tempAddResult_imag, SHUFFLE_SCATTER, calculate_length);
    vshuffle(Data_without_CP_imag, shuffle_wn_stage4, tempWnResult_imag, SHUFFLE_SCATTER, calculate_length);
    //---------------------------------------------

    //  Stage 5------------------------------------
    vshuffle(data_real_down, moveHalf2Front, Data_without_CP_real, SHUFFLE_GATHER, calculate_length);
    vshuffle(data_imag_down, moveHalf2Front, Data_without_CP_imag, SHUFFLE_GATHER, calculate_length);
    vshuffle(Wn_cos, shuffle_for_128_Wn, cos_stage9, SHUFFLE_GATHER, calculate_length);
    vshuffle(Wn_sin, shuffle_for_128_Wn, sin_stage9, SHUFFLE_GATHER, calculate_length);
    //  a + b
    tempAddResult_real = vsadd(Data_without_CP_real, data_real_down, MASKREAD_OFF, calculate_length);
    tempAddResult_imag = vsadd(Data_without_CP_imag, data_imag_down, MASKREAD_OFF, calculate_length);
    tempAddResult_real = vsra(tempAddResult_real, fft128_shift_vec[5], MASKREAD_OFF, calculate_length);
    tempAddResult_imag = vsra(tempAddResult_imag, fft128_shift_vec[5], MASKREAD_OFF, calculate_length);

    // (a - b)Wn    (可用复数计算代替)
    a_sub_b_real = vssub(data_real_down, Data_without_CP_real, MASKREAD_OFF, calculate_length);
    a_sub_b_imag = vssub(data_imag_down, Data_without_CP_imag, MASKREAD_OFF, calculate_length);

    fraction = fft128_fixed_vec[5];
    vsetshamt(fraction);
    cos_tempWnResult = vmul(a_sub_b_real, Wn_cos, MASKREAD_OFF, calculate_length);
    sin_tempWnResult = vmul(a_sub_b_imag, Wn_sin, MASKREAD_OFF, calculate_length);
    vsetshamt(0);
//     cos_tempWnResult = MUL4096_8_FIXED(a_sub_b_real, Wn_cos, fft128_fixed_vec[5], calculate_length);
//     sin_tempWnResult = MUL4096_8_FIXED(a_sub_b_imag, Wn_sin, fft128_fixed_vec[5], calculate_length);
    tempWnResult_real = vssub(sin_tempWnResult, cos_tempWnResult, MASKREAD_OFF, calculate_length);


    vsetshamt(fraction);
    cos_tempWnResult = vmul(a_sub_b_imag, Wn_cos, MASKREAD_OFF, calculate_length);
    sin_tempWnResult = vmul(a_sub_b_real, Wn_sin, MASKREAD_OFF, calculate_length);
    vsetshamt(0);
//     cos_tempWnResult = MUL4096_8_FIXED(a_sub_b_imag, Wn_cos, fft128_fixed_vec[5], calculate_length);
//     sin_tempWnResult = MUL4096_8_FIXED(a_sub_b_real, Wn_sin, fft128_fixed_vec[5], calculate_length);
    tempWnResult_imag = vsadd(cos_tempWnResult, sin_tempWnResult, MASKREAD_OFF, calculate_length);

    tempWnResult_imag = vsadd(tempWnResult_imag, 1, MASKREAD_OFF, calculate_length); // Function FOR OFFSET

//     fraction = fft128_fixed_vec[5];
//     tempWnResult_real = vssub(data_real_down, Data_without_CP_real, MASKREAD_OFF, calculate_length);
//     tempWnResult_imag = vssub(data_imag_down, Data_without_CP_imag, MASKREAD_OFF, calculate_length);
//     vsetshamt(fraction);
//     vcmxmul(cmximag_part, cmxreal_part, tempWnResult_real, tempWnResult_imag, Wn_sin, Wn_cos, MASKREAD_OFF,
//             calculate_length);
//     //     vcmxmul(cmxreal_part, cmximag_part, tempWnResult_imag, tempWnResult_real, Wn_sin, Wn_cos, MASKREAD_OFF,
//     //             calculate_length);
//     vsetshamt(0);

    // 重排序
    vshuffle(Data_without_CP_real, shuffle_add_stage5, tempAddResult_real, SHUFFLE_SCATTER, calculate_length);
    vshuffle(Data_without_CP_real, shuffle_wn_stage5, tempWnResult_real, SHUFFLE_SCATTER, calculate_length);
    vshuffle(Data_without_CP_imag, shuffle_add_stage5, tempAddResult_imag, SHUFFLE_SCATTER, calculate_length);
    vshuffle(Data_without_CP_imag, shuffle_wn_stage5, tempWnResult_imag, SHUFFLE_SCATTER, calculate_length);
    //---------------------------------------------

    //  Stage 6------------------------------------
    vshuffle(data_real_down, moveHalf2Front, Data_without_CP_real, SHUFFLE_GATHER, calculate_length);
    vshuffle(data_imag_down, moveHalf2Front, Data_without_CP_imag, SHUFFLE_GATHER, calculate_length);
    vshuffle(Wn_cos, shuffle_for_128_Wn, cos_stage10, SHUFFLE_GATHER, calculate_length);
    vshuffle(Wn_sin, shuffle_for_128_Wn, sin_stage10, SHUFFLE_GATHER, calculate_length);
    //  a + b
    tempAddResult_real = vsadd(Data_without_CP_real, data_real_down, MASKREAD_OFF, calculate_length);
    tempAddResult_imag = vsadd(Data_without_CP_imag, data_imag_down, MASKREAD_OFF, calculate_length);
    tempAddResult_real = vsra(tempAddResult_real, fft128_shift_vec[6], MASKREAD_OFF, calculate_length);
    tempAddResult_imag = vsra(tempAddResult_imag, fft128_shift_vec[6], MASKREAD_OFF, calculate_length);

    // (a - b)Wn    (可用复数计算代替)
    a_sub_b_real = vssub(data_real_down, Data_without_CP_real, MASKREAD_OFF, calculate_length);
    a_sub_b_imag = vssub(data_imag_down, Data_without_CP_imag, MASKREAD_OFF, calculate_length);

    fraction = fft128_fixed_vec[6];
    vsetshamt(fraction);
    cos_tempWnResult = vmul(a_sub_b_real, Wn_cos, MASKREAD_OFF, calculate_length);
    sin_tempWnResult = vmul(a_sub_b_imag, Wn_sin, MASKREAD_OFF, calculate_length);
    vsetshamt(0);
//     cos_tempWnResult = MUL4096_8_FIXED(a_sub_b_real, Wn_cos, fft128_fixed_vec[6], calculate_length);
//     sin_tempWnResult = MUL4096_8_FIXED(a_sub_b_imag, Wn_sin, fft128_fixed_vec[6], calculate_length);
    tempWnResult_real = vssub(sin_tempWnResult, cos_tempWnResult, MASKREAD_OFF, calculate_length);

    vsetshamt(fraction);
    cos_tempWnResult = vmul(a_sub_b_imag, Wn_cos, MASKREAD_OFF, calculate_length);
    sin_tempWnResult = vmul(a_sub_b_real, Wn_sin, MASKREAD_OFF, calculate_length);
    vsetshamt(0);
//     cos_tempWnResult = MUL4096_8_FIXED(a_sub_b_imag, Wn_cos, fft128_fixed_vec[6], calculate_length);
//     sin_tempWnResult = MUL4096_8_FIXED(a_sub_b_real, Wn_sin, fft128_fixed_vec[6], calculate_length);
    tempWnResult_imag = vsadd(cos_tempWnResult, sin_tempWnResult, MASKREAD_OFF, calculate_length);

    tempWnResult_imag = vsadd(tempWnResult_imag, 1, MASKREAD_OFF, calculate_length); // Function FOR OFFSET

//     fraction = fft128_fixed_vec[6];
//     tempWnResult_real = vssub(data_real_down, Data_without_CP_real, MASKREAD_OFF, calculate_length);
//     tempWnResult_imag = vssub(data_imag_down, Data_without_CP_imag, MASKREAD_OFF, calculate_length);
//     vsetshamt(fraction);
//     vcmxmul(cmximag_part, cmxreal_part, tempWnResult_real, tempWnResult_imag, Wn_sin, Wn_cos, MASKREAD_OFF,
//             calculate_length);
//     //     vcmxmul(cmxreal_part, cmximag_part, tempWnResult_imag, tempWnResult_real, Wn_sin, Wn_cos, MASKREAD_OFF,
//     //             calculate_length);
//     vsetshamt(0);

    // fft shift
    vshuffle(OFDM_OutReal, shuffle_wn_stage6, tempAddResult_real, SHUFFLE_SCATTER, calculate_length);
    vshuffle(OFDM_OutReal, shuffle_add_stage6, tempWnResult_real, SHUFFLE_SCATTER, calculate_length);
    vshuffle(OFDM_OutImag, shuffle_wn_stage6, tempAddResult_imag, SHUFFLE_SCATTER, calculate_length);
    vshuffle(OFDM_OutImag, shuffle_add_stage6, tempWnResult_imag, SHUFFLE_SCATTER, calculate_length);
    //---------------------------------------------
  }

  __v4096i8 real_temp1;
  __v4096i8 real_temp2;
  __v4096i8 imag_temp1;
  __v4096i8 imag_temp2;
  real_temp1 = MUL4096_8_FIXED(OFDM_OutReal, fftPhase_r, fft128_fixed_vec[6], 128);
  real_temp2 = MUL4096_8_FIXED(OFDM_OutImag, fftPhase_i, fft128_fixed_vec[6], 128);
  imag_temp1 = MUL4096_8_FIXED(OFDM_OutReal, fftPhase_i, fft128_fixed_vec[6], 128);
  imag_temp2 = MUL4096_8_FIXED(OFDM_OutImag, fftPhase_r, fft128_fixed_vec[6], 128);

  OFDM_OutReal = vssub(real_temp2, real_temp1, MASKREAD_OFF, 128);
  OFDM_OutImag = vsadd(imag_temp1, imag_temp2, MASKREAD_OFF, 128);

  __v4096i8 OFDM_OutReal_a;
  __v4096i8 OFDM_OutImag_a; 
  vclaim(OFDM_OutReal_a);
  vclaim(OFDM_OutImag_a);
  vshuffle(OFDM_OutReal_a, targetIndices, OFDM_OutReal, SHUFFLE_GATHER, 72);
  vshuffle(OFDM_OutImag_a, targetIndices, OFDM_OutImag, SHUFFLE_GATHER, 72);

  vreturn(OFDM_OutReal_a, 128, OFDM_OutImag_a, 128);
}