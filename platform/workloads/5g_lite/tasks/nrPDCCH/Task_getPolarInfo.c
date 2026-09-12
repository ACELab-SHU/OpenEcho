/**
 * ****************************************
 * @file        Task_getPolarInfo.c
 * @brief       get polar decoder infomation
 * @author      yuanfeng
 * @date        2024.7.29
 * @copyright   ACE-Lab(Shanghai University)
 * ****************************************
 */

#include "data_type.h"
#include "riscv_printf.h"
#include "venus.h"
#include "vmath.h"

typedef short __v2048i16 __attribute__((ext_vector_type(2048)));
typedef char  __v4096i8 __attribute__((ext_vector_type(4096)));

static uint16_t polarSequence[1024] = {
    0,    1,    2,    4,   8,   16,   32,   3,    5,    64,   9,    6,   17,   10,   18,   128,  12,  33,   65,   20,
    256,  34,   24,   36,  7,   129,  66,   512,  11,   40,   68,   130, 19,   13,   48,   14,   72,  257,  21,   132,
    35,   258,  26,   513, 80,  37,   25,   22,   136,  260,  264,  38,  514,  96,   67,   41,   144, 28,   69,   42,
    516,  49,   74,   272, 160, 520,  288,  528,  192,  544,  70,   44,  131,  81,   50,   73,   15,  320,  133,  52,
    23,   134,  384,  76,  137, 82,   56,   27,   97,   39,   259,  84,  138,  145,  261,  29,   43,  98,   515,  88,
    140,  30,   146,  71,  262, 265,  161,  576,  45,   100,  640,  51,  148,  46,   75,   266,  273, 517,  104,  162,
    53,   193,  152,  77,  164, 768,  268,  274,  518,  54,   83,   57,  521,  112,  135,  78,   289, 194,  85,   276,
    522,  58,   168,  139, 99,  86,   60,   280,  89,   290,  529,  524, 196,  141,  101,  147,  176, 142,  530,  321,
    31,   200,  90,   545, 292, 322,  532,  263,  149,  102,  105,  304, 296,  163,  92,   47,   267, 385,  546,  324,
    208,  386,  150,  153, 165, 106,  55,   328,  536,  577,  548,  113, 154,  79,   269,  108,  578, 224,  166,  519,
    552,  195,  270,  641, 523, 275,  580,  291,  59,   169,  560,  114, 277,  156,  87,   197,  116, 170,  61,   531,
    525,  642,  281,  278, 526, 177,  293,  388,  91,   584,  769,  198, 172,  120,  201,  336,  62,  282,  143,  103,
    178,  294,  93,   644, 202, 592,  323,  392,  297,  770,  107,  180, 151,  209,  284,  648,  94,  204,  298,  400,
    608,  352,  325,  533, 155, 210,  305,  547,  300,  109,  184,  534, 537,  115,  167,  225,  326, 306,  772,  157,
    656,  329,  110,  117, 212, 171,  776,  330,  226,  549,  538,  387, 308,  216,  416,  271,  279, 158,  337,  550,
    672,  118,  332,  579, 540, 389,  173,  121,  553,  199,  784,  179, 228,  338,  312,  704,  390, 174,  554,  581,
    393,  283,  122,  448, 353, 561,  203,  63,   340,  394,  527,  582, 556,  181,  295,  285,  232, 124,  205,  182,
    643,  562,  286,  585, 299, 354,  211,  401,  185,  396,  344,  586, 645,  593,  535,  240,  206, 95,   327,  564,
    800,  402,  356,  307, 301, 417,  213,  568,  832,  588,  186,  646, 404,  227,  896,  594,  418, 302,  649,  771,
    360,  539,  111,  331, 214, 309,  188,  449,  217,  408,  609,  596, 551,  650,  229,  159,  420, 310,  541,  773,
    610,  657,  333,  119, 600, 339,  218,  368,  652,  230,  391,  313, 450,  542,  334,  233,  555, 774,  175,  123,
    658,  612,  341,  777, 220, 314,  424,  395,  673,  583,  355,  287, 183,  234,  125,  557,  660, 616,  342,  316,
    241,  778,  563,  345, 452, 397,  403,  207,  674,  558,  785,  432, 357,  187,  236,  664,  624, 587,  780,  705,
    126,  242,  565,  398, 346, 456,  358,  405,  303,  569,  244,  595, 189,  566,  676,  361,  706, 589,  215,  786,
    647,  348,  419,  406, 464, 680,  801,  362,  590,  409,  570,  788, 597,  572,  219,  311,  708, 598,  601,  651,
    421,  792,  802,  611, 602, 410,  231,  688,  653,  248,  369,  190, 364,  654,  659,  335,  480, 315,  221,  370,
    613,  422,  425,  451, 614, 543,  235,  412,  343,  372,  775,  317, 222,  426,  453,  237,  559, 833,  804,  712,
    834,  661,  808,  779, 617, 604,  433,  720,  816,  836,  347,  897, 243,  662,  454,  318,  675, 618,  898,  781,
    376,  428,  665,  736, 567, 840,  625,  238,  359,  457,  399,  787, 591,  678,  434,  677,  349, 245,  458,  666,
    620,  363,  127,  191, 782, 407,  436,  626,  571,  465,  681,  246, 707,  350,  599,  668,  790, 460,  249,  682,
    573,  411,  803,  789, 709, 365,  440,  628,  689,  374,  423,  466, 793,  250,  371,  481,  574, 413,  603,  366,
    468,  655,  900,  805, 615, 684,  710,  429,  794,  252,  373,  605, 848,  690,  713,  632,  482, 806,  427,  904,
    414,  223,  663,  692, 835, 619,  472,  455,  796,  809,  714,  721, 837,  716,  864,  810,  606, 912,  722,  696,
    377,  435,  817,  319, 621, 812,  484,  430,  838,  667,  488,  239, 378,  459,  622,  627,  437, 380,  818,  461,
    496,  669,  679,  724, 841, 629,  351,  467,  438,  737,  251,  462, 442,  441,  469,  247,  683, 842,  738,  899,
    670,  783,  849,  820, 728, 928,  791,  367,  901,  630,  685,  844, 633,  711,  253,  691,  824, 902,  686,  740,
    850,  375,  444,  470, 483, 415,  485,  905,  795,  473,  634,  744, 852,  960,  865,  693,  797, 906,  715,  807,
    474,  636,  694,  254, 717, 575,  913,  798,  811,  379,  697,  431, 607,  489,  866,  723,  486, 908,  718,  813,
    476,  856,  839,  725, 698, 914,  752,  868,  819,  814,  439,  929, 490,  623,  671,  739,  916, 463,  843,  381,
    497,  930,  821,  726, 961, 872,  492,  631,  729,  700,  443,  741, 845,  920,  382,  822,  851, 730,  498,  880,
    742,  445,  471,  635, 932, 687,  903,  825,  500,  846,  745,  826, 732,  446,  962,  936,  475, 853,  867,  637,
    907,  487,  695,  746, 828, 753,  854,  857,  504,  799,  255,  964, 909,  719,  477,  915,  638, 748,  944,  869,
    491,  699,  754,  858, 478, 968,  383,  910,  815,  976,  870,  917, 727,  493,  873,  701,  931, 756,  860,  499,
    731,  823,  922,  874, 918, 502,  933,  743,  760,  881,  494,  702, 921,  501,  876,  847,  992, 447,  733,  827,
    934,  882,  937,  963, 747, 505,  855,  924,  734,  829,  965,  938, 884,  506,  749,  945,  966, 755,  859,  940,
    830,  911,  871,  639, 888, 479,  946,  750,  969,  508,  861,  757, 970,  919,  875,  862,  758, 948,  977,  923,
    972,  761,  877,  952, 495, 703,  935,  978,  883,  762,  503,  925, 878,  735,  993,  885,  939, 994,  980,  926,
    764,  941,  967,  886, 831, 947,  507,  889,  984,  751,  942,  996, 971,  890,  509,  949,  973, 1000, 892,  950,
    863,  759,  1008, 510, 979, 953,  763,  974,  954,  879,  981,  982, 927,  995,  765,  956,  887, 985,  997,  986,
    943,  891,  998,  766, 511, 988,  1001, 951,  1002, 893,  975,  894, 1009, 955,  1004, 1010, 957, 983,  958,  987,
    1012, 999,  1016, 767, 989, 1003, 990,  1005, 959,  1011, 1013, 895, 1006, 1014, 1017, 1018, 991, 1020, 1007, 1015,
    1019, 1021, 1022, 1023};

static uint8_t pi[32]  = {0,  1,  2,  4,  3,  5,  6,  7,  8,  16, 9,  17, 10, 18, 11, 19,
                         12, 20, 13, 21, 14, 22, 15, 23, 24, 25, 26, 28, 27, 29, 30, 31};
static uint8_t wg[256] = {
    1,  2,  2,  4,  2,  4,   4,  8,  2,  4,   4,  8,  4,  8,   8,  16,  2,  4,   4,  8,   4,   8,  8,  16,  4,  8,
    8,  16, 8,  16, 16, 32,  2,  4,  4,  8,   4,  8,  8,  16,  4,  8,   8,  16,  8,  16,  16,  32, 4,  8,   8,  16,
    8,  16, 16, 32, 8,  16,  16, 32, 16, 32,  32, 64, 2,  4,   4,  8,   4,  8,   8,  16,  4,   8,  8,  16,  8,  16,
    16, 32, 4,  8,  8,  16,  8,  16, 16, 32,  8,  16, 16, 32,  16, 32,  32, 64,  4,  8,   8,   16, 8,  16,  16, 32,
    8,  16, 16, 32, 16, 32,  32, 64, 8,  16,  16, 32, 16, 32,  32, 64,  16, 32,  32, 64,  32,  64, 64, 128, 2,  4,
    4,  8,  4,  8,  8,  16,  4,  8,  8,  16,  8,  16, 16, 32,  4,  8,   8,  16,  8,  16,  16,  32, 8,  16,  16, 32,
    16, 32, 32, 64, 4,  8,   8,  16, 8,  16,  16, 32, 8,  16,  16, 32,  16, 32,  32, 64,  8,   16, 16, 32,  16, 32,
    32, 64, 16, 32, 32, 64,  32, 64, 64, 128, 4,  8,  8,  16,  8,  16,  16, 32,  8,  16,  16,  32, 16, 32,  32, 64,
    8,  16, 16, 32, 16, 32,  32, 64, 16, 32,  32, 64, 32, 64,  64, 128, 8,  16,  16, 32,  16,  32, 32, 64,  16, 32,
    32, 64, 32, 64, 64, 128, 16, 32, 32, 64,  32, 64, 64, 128, 32, 64,  64, 128, 64, 128, 128, 256};

VENUS_INLINE short ceillog2_Venus(short in) {
  short out  = -1;
  short temp = in;
  while (temp > 0) {
    temp = temp >> 1;
    out  = out + 1;
  }
  if (in > (1 << out)) {
    out = out + 1;
  }
  return out;
}

VENUS_INLINE short ceil_math(float in) {
  int inum = (int)in;
  if (in == (float)inum) {
    return inum;
  }
  return inum + 1;
}

// VENUS_INLINE short polarGetN(short K, short E, uint8_t nMax) {
//   short   Ceil_Log2_E = ceillog2_Venus(E);
//   uint8_t n1          = 0;
//   // if ((E <= (9.0 / 8.0) * (1 << (Ceil_Log2_E - 1)) && ((K * 1.0) / E < 9.0 / 16.0))) {
//   if (E <= 1.125 * (1 << (Ceil_Log2_E - 1)) && ((16 * K) < (9 * E))) {
//     n1 = Ceil_Log2_E - 1;
//   } else {
//     n1 = Ceil_Log2_E;
//   }

//   uint8_t n2    = ceillog2_Venus((short)8 * K);
//   uint8_t nMin  = 5;
//   uint8_t n     = 0;
//   uint8_t min_n = n1;
//   if (n2 < min_n) {
//     min_n = n2;
//   }
//   if (nMax < min_n) {
//     min_n = nMax;
//   }
//   if (min_n >= nMin) {
//     n = min_n;
//   } else {
//     n = nMin;
//   }
//   uint16_t N_temp = 1 << n;
//   return N_temp;
// }

VENUS_INLINE short polarGetN(short K, short E, uint8_t nMax) {
  short   Ceil_Log2_E = ceillog2_Venus(E);
  uint8_t n1          = 0;
  if (E <= 1.125 * (1 << (Ceil_Log2_E - 1)) && ((16 * K) < (9 * E))) {
    n1 = Ceil_Log2_E - 1;
  } else {
    n1 = Ceil_Log2_E;
  }

  uint8_t n2    = ceillog2_Venus((short)8 * K);
  uint8_t nMin  = 5;
  uint8_t n     = 0;
  uint8_t min_n = n1;
  if (n2 < min_n) {
    min_n = n2;
  }
  if (nMax < min_n) {
    min_n = nMax;
  }
  if (min_n >= nMin) {
    n = min_n;
  } else {
    n = nMin;
  }
  uint16_t N_temp = 1 << n;
  return N_temp;
}

// VENUS_INLINE __v2048i16 polarConstruct(uint16_t K, uint16_t E, uint16_t N, uint8_t nPC, uint8_t nPCwm, uint8_t nMax)
// {
//   // Get sequence for N, ascending ordered, Section 5.3.1.2
//   uint16_t s10[1024];
//   for (int i = 0; i < 1024; i++) {
//     s10[i] = polarSequence[i];
//   }

//   uint16_t qSeq[1024];
//   int      j = 0;
//   for (int i = 0; i < 1024; i++) {
//     if (s10[i] < N) {
//       qSeq[j] = s10[i];
//       j++;
//     }
//   }

//   uint16_t jn[1024];
//   for (int i = 0; i < N; i++) {
//     jn[i] = 0;
//   }
//   uint8_t ii = 0;
//   // uint16_t N_32    = N / 32 ;
//   uint16_t N_32    = N >> 5;
//   short    N_log   = ceillog2_Venus(N);
//   uint8_t  logN_32 = 0;
//   uint16_t temp    = N_32;
//   while (temp > 1) {
//     logN_32 = logN_32 + 1;
//     temp    = temp >> 1;
//   }
//   for (int n = 0; n < N; n++) {
//     // ii    = 32 * n / N;
//     ii    = (32 * n) >> N_log;
//     jn[n] = pi[ii] * N_32 + (n - ((n >> logN_32) << logN_32));
//   }

//   uint16_t qFtmp[1024];
//   int      length_qFtmp = 0;
//   if (E < N) {
//     // if ((1.0 * K) / (1.0 * E) <= 7.0 / 16.0) // puncturing
//     if (16 * K <= 7 * E) // puncturing
//     {
//       // if (E >= 3.0 * N / 4.0) {
//       if (E >= 0.75 * N) {
//         // int uLim     = ceil_math(3.0 * N / 4.0 - E / 2.0);
//         int uLim     = ceil_math(0.75 * N - 0.5 * E);
//         length_qFtmp = N - E + uLim;
//         for (int i = 0; i < N - E; i++) {
//           qFtmp[i] = jn[i];
//         }
//         int k = 0;
//         for (int i = 0; i < uLim; i++) {
//           int label_same = 0;
//           for (int l = 0; l < N - E; l++) {
//             if (i == jn[l]) {
//               break;
//             } else {
//               label_same = label_same + 1;
//             }
//           }
//           if (label_same == N - E) {
//             qFtmp[N - E + k] = i;
//             k                = k + 1;
//           }
//         }
//         length_qFtmp = N - E + k;
//       } else {
//         int uLim     = ceil_math(9.0 * N / 16.0 - E / 4.0);
//         length_qFtmp = N - E + uLim;
//         for (int i = 0; i < N - E; i++) {
//           qFtmp[i] = jn[i];
//         }
//         int k = 0;
//         for (int i = 0; i < uLim; i++) {
//           int label_same = 0;
//           for (int l = 0; l < N - E; l++) {
//             if (i == jn[l]) {
//               break;
//             } else {
//               label_same = label_same + 1;
//             }
//           }
//           if (label_same == N - E) {
//             qFtmp[N - E + k] = i;
//             k                = k + 1;
//           }
//         }
//         length_qFtmp = N - E + k;
//       }
//     } else // shortening
//     {
//       length_qFtmp = N - E;
//       for (int i = 0; i < N - E; i++) {
//         qFtmp[i] = jn[E + i];
//       }
//     }
//   } else {
//     length_qFtmp = 0;
//   }

//   //  Get qI from qFtmp and qSeq
//   int qI[1024];
//   for (int i = 0; i < K + nPC; i++) {
//     qI[i] = 0;
//   }
//   j = 0;
//   for (int i = 1; i <= N; i++) {
//     int ind = qSeq[N - i];
//     //  后续思考优化
//     //  if any(ind == qFtmp)
//     //      continue;
//     //  end
//     int label = 0;
//     for (int k = 0; k < length_qFtmp; k++) {
//       if (ind == qFtmp[k]) {
//         label = 1;
//         break;
//       }
//     }
//     if (label == 1) {
//       continue;
//     }
//     qI[j] = ind;
//     j     = j + 1;
//     if (j == (K + nPC)) {
//       break;
//     }
//   }

//   //  Form the frozen bit vector
//   int qF[1024];

//   int length = N - (K + nPC);
//   for (int i = 0; i < length; i++) {
//     qF[i] = 0;
//   }
//   int c = 0;
//   for (int i = 0; i < N; i++) {
//     int tmp = qSeq[i];
//     int k   = 0;
//     for (int j = 0; j < K + nPC; j++) {
//       if (qI[j] == tmp) {
//         break;
//       } else {
//         k = k + 1;
//       }
//     }
//     if (k < K + nPC) {
//       qF[c] = tmp;
//       c     = c + 1;
//     }
//   }

//   // PC - Polar
//   int qPC[1024];
//   for (int i = 0; i < nPC; i++) {
//     qPC[i] = 0;
//   }
//   if (nPC > 0) {
//     for (int i = 0; i < nPC - nPCwm; i++) {
//       qPC[i] = qI[K + nPCwm + i];
//     }

//     if (nPCwm > 0) {
//       int wt_qtildeI[1024];
//       for (int i = 0; i < K; i++) {
//         wt_qtildeI[i] = wg[qI[i]];
//       }

//       int minwt = wt_qtildeI[0];
//       for (int i = 1; i < K; i++) {
//         if (wt_qtildeI[i] < minwt) {
//           minwt = wt_qtildeI[i];
//         }
//       }
//       for (int i = 0; i < K; i++) {
//         if (minwt == wt_qtildeI[i]) {
//           qPC[nPC - 1] = qI[i];
//           break;
//         }
//       }
//     }
//   }

//   for (int i = 0; i < K; i++) {

//     for (int j = 1; j < K - 1; j++) {

//       if (qF[j - 1] > qF[j]) {

//         int temp = qF[j - 1];

//         qF[j - 1] = qF[j];

//         qF[j] = temp;
//       }
//     }
//   }

//   __v2048i16 out;
//   vclaim(out);
//   vbarrier();
//   VSPM_OPEN();
//   int out_addr = vaddr(out);
//   for (int i = 0; i < N; i++) {
//     *(volatile unsigned short *)(out_addr + (i << 1)) = qF[i];
//   }
//   VSPM_CLOSE();

//   return out;
// }

typedef struct {
  short data[2048];
} __attribute__((aligned(64))) shortArray_struct;

volatile short out[2048] = {0};
// volatile uint16_t qSeq[1024];
// volatile uint16_t jn[1024];
// volatile uint16_t qFtmp[1024];
// volatile short    Index_Value[1024];
// volatile short    qI[1024];
// volatile int      qPC[1024];
// volatile int      wt_qtildeI[1024];

// VENUS_INLINE __v2048i16 polarConstruct(uint16_t K, uint16_t E, uint16_t N, uint8_t nPC, uint8_t nPCwm, uint8_t nMax)
// {
VENUS_INLINE void polarConstruct(uint16_t K, uint16_t E, uint16_t N, uint8_t nPC, uint8_t nPCwm, uint8_t nMax) {
  // Get sequence for N, ascending ordered, Section 5.3.1.2
  uint16_t qSeq[1024];
  int      j = 0;
  for (int i = 0; i < 1024; i++) {
    if (polarSequence[i] < N) {
      qSeq[j] = polarSequence[i];
      j++;
    }
  }

  uint16_t jn[1024];
  for (int i = 0; i < N; i++) {
    jn[i] = 0;
  }
  uint8_t ii = 0;
  // uint16_t N_32    = N / 32 ;
  uint16_t N_32    = N >> 5;
  short    N_log   = ceillog2_Venus(N);
  uint8_t  logN_32 = 0;
  uint16_t temp    = N_32;
  while (temp > 1) {
    logN_32 = logN_32 + 1;
    temp    = temp >> 1;
  }
  for (int n = 0; n < N; n++) {
    // ii    = 32 * n / N;
    ii    = (32 * n) >> N_log;
    jn[n] = pi[ii] * N_32 + (n - ((n >> logN_32) << logN_32));
  }

  uint16_t qFtmp[1024];
  int      length_qFtmp = 0;
  if (E < N) {
    if (16 * K <= 7 * E) // puncturing
    {
      if (E >= 0.75 * N) {
        int uLim     = ceil_math(0.75 * N - 0.5 * E);
        length_qFtmp = N - E + uLim;
        for (int i = 0; i < N - E; i++) {
          qFtmp[i] = jn[i];
        }
        int k = 0;
        for (int i = 0; i < uLim; i++) {
          int label_same = 0;
          for (int l = 0; l < N - E; l++) {
            if (i == jn[l]) {
              break;
            } else {
              label_same = label_same + 1;
            }
          }
          if (label_same == N - E) {
            qFtmp[N - E + k] = i;
            k                = k + 1;
          }
        }
        length_qFtmp = N - E + k;
      } else {
        int uLim     = ceil_math(9.0 * N / 16.0 - E / 4.0);
        length_qFtmp = N - E + uLim;
        for (int i = 0; i < N - E; i++) {
          qFtmp[i] = jn[i];
        }
        int k = 0;
        for (int i = 0; i < uLim; i++) {
          int label_same = 0;
          for (int l = 0; l < N - E; l++) {
            if (i == jn[l]) {
              break;
            } else {
              label_same = label_same + 1;
            }
          }
          if (label_same == N - E) {
            qFtmp[N - E + k] = i;
            k                = k + 1;
          }
        }
        length_qFtmp = N - E + k;
      }
    } else // shortening
    {
      length_qFtmp = N - E;
      for (int i = 0; i < N - E; i++) {
        qFtmp[i] = jn[E + i];
      }
    }
  } else {
    length_qFtmp = 0;
  }

  // 将qSeq不在qFtmp中的最可靠的K+nPC个信号提取出来放入qI
  short Index_Value[1024];
  for (int i = 0; i < N; i++) {
    Index_Value[i] = 0;
  }
  for (int i = 0; i < length_qFtmp; i++) {
    Index_Value[qFtmp[i]] = 1;
  }

  short qI[1024];
  short index = 0;
  for (int i = 0; i < N; i++) {
    if (index == K + nPC) {
      break;
    }
    if (Index_Value[qSeq[N - 1 - i]] == 0) {
      qI[index] = qSeq[N - 1 - i];
      index++;
    }
  }

  for (int i = 0; i < K; i++) {
    for (int j = 1; j < K; j++) {
      if (qI[j - 1] > qI[j]) {

        int temp = qI[j - 1];

        qI[j - 1] = qI[j];

        qI[j] = temp;
      }
    }
  }

  // for (int i = 0; i < index; i++) {
  //   out[i] = qI[i];
  // }

  // PC - Polar   暂不支持
  short qPC[1024];
  for (int i = 0; i < nPC; i++) {
    qPC[i] = 0;
  }
  if (nPC > 0) {
    for (int i = 0; i < nPC - nPCwm; i++) {
      qPC[i] = qI[K + nPCwm + i];
    }

    if (nPCwm > 0) {
      short wt_qtildeI[1024];
      for (int i = 0; i < K; i++) {
        wt_qtildeI[i] = wg[qI[i]];
      }

      int minwt = wt_qtildeI[0];
      for (int i = 1; i < K; i++) {
        if (wt_qtildeI[i] < minwt) {
          minwt = wt_qtildeI[i];
        }
      }
      for (int i = 0; i < K; i++) {
        if (minwt == wt_qtildeI[i]) {
          qPC[nPC - 1] = qI[i];
          break;
        }
      }
    }
  }

  // __v2048i16 out;
  // vclaim(out);
  // vbarrier();
  // VSPM_OPEN();
  // int out_addr = vaddr(out);
  // for (int i = 0; i < K + nPC; i++) {
  //   *(volatile unsigned short *)(out_addr + (i << 1)) = qI[i];
  // }
  // VSPM_CLOSE();

  for (int i = 0; i < K + nPC; i++) {
    out[i] = qI[i];
  }

  // return out;
}

int Task_getPolarInfo(short_struct input_csetNRB, short_struct input_E) {
  // __asm__("lui sp, 0x140");
  VSPM_OPEN();

  short csetNRB = input_csetNRB.data;
  short E       = input_E.data;
  short nMax    = 9;

  short K = 28 + ceil_log2(csetNRB * (csetNRB + 1) / 2) + 24; // dci.Width+24

  uint16_t N = polarGetN(K, E, nMax);
  // __v2048i16 InfoIndex;
  // vclaim(InfoIndex);
  // InfoIndex = polarConstruct(K, E, N, 0, 0, nMax);
  polarConstruct(K, E, N, 0, 0, nMax);

  shortArray_struct outArr;
  for (int i = 0; i < K; i++) {
    outArr.data[i] = out[i];
  }

  short_struct out_N, out_K;
  out_N.data = N;
  out_K.data = K;

  // VSPM_CLOSE();
  // vreturn(InfoIndex, sizeof(InfoIndex), &out_N, sizeof(out_N), &out_K, sizeof(out_K));
  vreturn(&outArr, sizeof(outArr), &out_N, sizeof(out_N), &out_K, sizeof(out_K));
}