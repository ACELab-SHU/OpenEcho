/**
 * BP Decoder of Polar for 8bit LLR
 * Created by HorryShen
 */

#include "data_type.h"
#include "riscv_printf.h"
#include "venus.h"
// #include "math.h"

typedef short __v2048i16 __attribute__((ext_vector_type(2048)));
typedef char  __v4096i8 __attribute__((ext_vector_type(4096)));

int Interleaving_pattern_table[164] = {
    0,   2,   4,   7,   9,   14,  19,  20,  24,  25,  26,  28,  31,  34,  42,  45,  49,  50,  51,  53,  54,
    56,  58,  59,  61,  62,  65,  66,  67,  69,  70,  71,  72,  76,  77,  81,  82,  83,  87,  88,  89,  91,
    93,  95,  98,  101, 104, 106, 108, 110, 111, 113, 115, 118, 119, 120, 122, 123, 126, 127, 129, 132, 134,
    138, 139, 140, 1,   3,   5,   8,   10,  15,  21,  27,  29,  32,  35,  43,  46,  52,  55,  57,  60,  63,
    68,  73,  78,  84,  90,  92,  94,  96,  99,  102, 105, 107, 109, 112, 114, 116, 121, 124, 128, 130, 133,
    135, 141, 6,   11,  16,  22,  30,  33,  36,  44,  47,  64,  74,  79,  85,  97,  100, 103, 117, 125, 131,
    136, 142, 12,  17,  23,  37,  48,  75,  80,  86,  137, 143, 13,  18,  38,  144, 39,  145, 40,  146, 41,
    147, 148, 149, 150, 151, 152, 153, 154, 155, 156, 157, 158, 159, 160, 161, 162, 163};
// uint16_t polarSequence[1024] = {
//     0,    1,    2,    4,   8,   16,   32,   3,    5,    64,   9,    6,   17,
//     10,   18,   128,  12,  33,   65,   20, 256,  34,   24,   36,  7,   129,
//     66,   512,  11,   40,   68,   130, 19,   13,   48,   14,   72,  257,  21,
//     132, 35,   258,  26,   513, 80,  37,   25,   22,   136,  260,  264,  38,
//     514,  96,   67,   41,   144, 28,   69,   42, 516,  49,   74,   272, 160,
//     520,  288,  528,  192,  544,  70,   44,  131,  81,   50,   73,   15, 320,
//     133,  52, 23,   134,  384,  76,  137, 82,   56,   27,   97,   39,   259,
//     84,  138,  145,  261,  29,   43,  98,   515,  88, 140,  30,   146,  71,
//     262, 265,  161,  576,  45,   100,  640,  51,  148,  46,   75,   266, 273,
//     517,  104, 162, 53,   193,  152,  77,  164, 768,  268,  274,  518,  54,
//     83,   57,  521,  112,  135,  78,   289, 194,  85, 276, 522,  58,   168,
//     139, 99,  86,   60,   280,  89,   290,  529,  524, 196,  141,  101,  147,
//     176, 142,  530, 321, 31,   200,  90,   545, 292, 322,  532,  263,  149,
//     102,  105,  304, 296,  163,  92,   47,   267, 385,  546, 324, 208,  386,
//     150,  153, 165, 106,  55,   328,  536,  577,  548,  113, 154,  79,   269,
//     108,  578, 224,  166, 519, 552,  195,  270,  641, 523, 275,  580,  291,
//     59,   169,  560,  114, 277,  156,  87,   197,  116, 170,  61, 531, 525,
//     642,  281,  278, 526, 177,  293,  388,  91,   584,  769,  198, 172,  120,
//     201,  336,  62,  282,  143, 103, 178,  294,  93,   644, 202, 592,  323,
//     392,  297,  770,  107,  180, 151,  209,  284,  648,  94,  204,  298, 400,
//     608,  352,  325,  533, 155, 210,  305,  547,  300,  109,  184,  534, 537,
//     115,  167,  225,  326, 306,  772, 157, 656,  329,  110,  117, 212, 171,
//     776,  330,  226,  549,  538,  387, 308,  216,  416,  271,  279, 158, 337,
//     550, 672,  118,  332,  579, 540, 389,  173,  121,  553,  199,  784,  179,
//     228,  338,  312,  704,  390, 174,  554, 581, 393,  283,  122,  448, 353,
//     561,  203,  63,   340,  394,  527,  582, 556,  181,  295,  285,  232,
//     124,  205, 182, 643,  562,  286,  585, 299, 354,  211,  401,  185,  396,
//     344,  586, 645,  593,  535,  240,  206, 95,   327, 564, 800,  402,  356,
//     307, 301, 417,  213,  568,  832,  588,  186,  646, 404,  227,  896,  594,
//     418, 302,  649, 771, 360,  539,  111,  331, 214, 309,  188,  449,  217,
//     408,  609,  596, 551,  650,  229,  159,  420, 310,  541, 773, 610,  657,
//     333,  119, 600, 339,  218,  368,  652,  230,  391,  313, 450,  542,  334,
//     233,  555, 774,  175, 123, 658,  612,  341,  777, 220, 314,  424,  395,
//     673,  583,  355,  287, 183,  234,  125,  557,  660, 616,  342, 316, 241,
//     778,  563,  345, 452, 397,  403,  207,  674,  558,  785,  432, 357,  187,
//     236,  664,  624, 587,  780, 705, 126,  242,  565,  398, 346, 456,  358,
//     405,  303,  569,  244,  595, 189,  566,  676,  361,  706, 589,  215, 786,
//     647,  348,  419,  406, 464, 680,  801,  362,  590,  409,  570,  788, 597,
//     572,  219,  311,  708, 598,  601, 651, 421,  792,  802,  611, 602, 410,
//     231,  688,  653,  248,  369,  190, 364,  654,  659,  335,  480, 315, 221,
//     370, 613,  422,  425,  451, 614, 543,  235,  412,  343,  372,  775,  317,
//     222,  426,  453,  237,  559, 833,  804, 712, 834,  661,  808,  779, 617,
//     604,  433,  720,  816,  836,  347,  897, 243,  662,  454,  318,  675,
//     618,  898, 781, 376,  428,  665,  736, 567, 840,  625,  238,  359,  457,
//     399,  787, 591,  678,  434,  677,  349, 245,  458, 666, 620,  363,  127,
//     191, 782, 407,  436,  626,  571,  465,  681,  246, 707,  350,  599,  668,
//     790, 460,  249, 682, 573,  411,  803,  789, 709, 365,  440,  628,  689,
//     374,  423,  466, 793,  250,  371,  481,  574, 413,  603, 366, 468,  655,
//     900,  805, 615, 684,  710,  429,  794,  252,  373,  605, 848,  690,  713,
//     632,  482, 806,  427, 904, 414,  223,  663,  692, 835, 619,  472,  455,
//     796,  809,  714,  721, 837,  716,  864,  810,  606, 912,  722, 696, 377,
//     435,  817,  319, 621, 812,  484,  430,  838,  667,  488,  239, 378,  459,
//     622,  627,  437, 380,  818, 461, 496,  669,  679,  724, 841, 629,  351,
//     467,  438,  737,  251,  462, 442,  441,  469,  247,  683, 842,  738, 899,
//     670,  783,  849,  820, 728, 928,  791,  367,  901,  630,  685,  844, 633,
//     711,  253,  691,  824, 902,  686, 740, 850,  375,  444,  470, 483, 415,
//     485,  905,  795,  473,  634,  744, 852,  960,  865,  693,  797, 906, 715,
//     807, 474,  636,  694,  254, 717, 575,  913,  798,  811,  379,  697,  431,
//     607,  489,  866,  723,  486, 908,  718, 813, 476,  856,  839,  725, 698,
//     914,  752,  868,  819,  814,  439,  929, 490,  623,  671,  739,  916,
//     463,  843, 381, 497,  930,  821,  726, 961, 872,  492,  631,  729,  700,
//     443,  741, 845,  920,  382,  822,  851, 730,  498, 880, 742,  445,  471,
//     635, 932, 687,  903,  825,  500,  846,  745,  826, 732,  446,  962,  936,
//     475, 853,  867, 637, 907,  487,  695,  746, 828, 753,  854,  857,  504,
//     799,  255,  964, 909,  719,  477,  915,  638, 748,  944, 869, 491,  699,
//     754,  858, 478, 968,  383,  910,  815,  976,  870,  917, 727,  493,  873,
//     701,  931, 756,  860, 499, 731,  823,  922,  874, 918, 502,  933,  743,
//     760,  881,  494,  702, 921,  501,  876,  847,  992, 447,  733, 827, 934,
//     882,  937,  963, 747, 505,  855,  924,  734,  829,  965,  938, 884,  506,
//     749,  945,  966, 755,  859, 940, 830,  911,  871,  639, 888, 479,  946,
//     750,  969,  508,  861,  757, 970,  919,  875,  862,  758, 948,  977, 923,
//     972,  761,  877,  952, 495, 703,  935,  978,  883,  762,  503,  925, 878,
//     735,  993,  885,  939, 994,  980, 926, 764,  941,  967,  886, 831, 947,
//     507,  889,  984,  751,  942,  996, 971,  890,  509,  949,  973, 1000,
//     892, 950, 863,  759,  1008, 510, 979, 953,  763,  974,  954,  879,  981,
//     982, 927,  995,  765,  956,  887, 985,  997, 986, 943,  891,  998,  766,
//     511, 988,  1001, 951,  1002, 893,  975,  894, 1009, 955,  1004, 1010,
//     957, 983,  958, 987, 1012, 999,  1016, 767, 989, 1003, 990,  1005, 959,
//     1011, 1013, 895, 1006, 1014, 1017, 1018, 991, 1020, 1007, 1015, 1019,
//     1021, 1022, 1023};

// uint8_t pi[32]  = {0,  1,  2,  4,  3,  5,  6,  7,  8,  16, 9,  17, 10, 18,
// 11, 19,
//                   12, 20, 13, 21, 14, 22, 15, 23, 24, 25, 26, 28, 27, 29, 30,
//                   31};
// int     wg[256] = {1,  2,  2,  4,  2,  4,  4,  8,   2,  4,  4,  8,   4,  8,
// 8,   16, 2,  4,  4,  8,  4,  8,  8, 16,
//                4,  8,  8,  16, 8,  16, 16, 32,  2,  4,  4,  8,   4,  8,   8,
//                16, 4,  8,  8,  16, 8,  16, 16, 32, 4,  8,  8,  16, 8,  16,
//                16, 32,  8,  16, 16, 32,  16, 32,  32,  64, 2,  4,  4,  8,  4,
//                8,  8,  16, 4,  8,  8,  16, 8,  16, 16, 32,  4,  8,  8,  16,
//                8,  16,  16,  32, 8,  16, 16, 32, 16, 32, 32, 64, 4,  8,  8,
//                16, 8,  16, 16, 32,  8,  16, 16, 32,  16, 32,  32,  64, 8, 16,
//                16, 32, 16, 32, 32, 64, 16, 32, 32, 64, 32, 64, 64, 128, 2, 4,
//                4,  8,   4,  8,   8,   16, 4,  8,  8,  16, 8,  16, 16, 32, 4,
//                8,  8,  16, 8,  16, 16, 32,  8,  16, 16, 32,  16, 32,  32, 64,
//                4,  8,  8,  16, 8,  16, 16, 32, 8,  16, 16, 32, 16, 32, 32,
//                64,  8,  16, 16, 32,  16, 32,  32,  64, 16, 32, 32, 64, 32,
//                64, 64, 128, 4,  8,  8,  16, 8,  16, 16, 32,  8,  16, 16, 32,
//                16, 32,  32,  64, 8,  16, 16, 32, 16, 32, 32, 64, 16, 32, 32,
//                64, 32, 64, 64, 128, 8,  16, 16, 32,  16, 32,  32,  64, 16,
//                32, 32, 64, 32, 64, 64, 128, 16, 32, 32, 64, 32, 64, 64, 128,
//                32, 64, 64, 128, 64, 128, 128, 256};

VENUS_INLINE __v2048i16 nr_polar_interleaving_pattern(uint16_t K, uint8_t I_IL) {
  __v2048i16 out;
  vclaim(out);
  short PI_k_[164];
  if (K > 164) {
    // printf("Error:\nDCI length A for PDCCH must <= 140 , now K is %hd\n",
    // &K);
  }
  uint8_t K_IL_max = 164;
  int     j        = 0;
  if (I_IL == 0) {
    for (; j <= K - 1; j++)
      PI_k_[j] = j;
  } else {
    for (int m = 0; m <= (K_IL_max - 1); m++) {
      if (Interleaving_pattern_table[m] >= (K_IL_max - K)) {
        PI_k_[j] = Interleaving_pattern_table[m] - (K_IL_max - K);
        j++;
      }
    }
  }
  vbarrier();
  VSPM_OPEN();
  int out_addr = vaddr(out);
  for (int i = 0; i < K; i++) {
    *(volatile unsigned short *)(out_addr + (i << 1)) = PI_k_[i];
  }
  VSPM_CLOSE();

  return out;
}

// 输入定点化的点数，实现定点乘法的输出
VENUS_INLINE __v4096i8 MUL4096_8_FIXED(__v4096i8 a, __v4096i8 b, int fix_point, int length) {
  // __v4096i8 result;
  // vsetshamt(fix_point);
  // result = vmul(a, b, MASKREAD_OFF, length);
  // vsetshamt(0);
  // return result;
  __v4096i8 high8;
  __v4096i8 low8;
  __v4096i8 result;
  short     high_shift = 8 - fix_point;

  low8   = vmul(a, b, MASKREAD_OFF, length);
  high8  = vmulh(a, b, MASKREAD_OFF, length);
  low8   = vsrl(low8, fix_point, MASKREAD_OFF, length);
  high8  = vsll(high8, high_shift, MASKREAD_OFF, length);
  result = vor(low8, high8, MASKREAD_OFF, length);
  return result;
};

// char input_llr[512] = {
//     40,  40,  -40, 40,  -40, -40, 40,  41,  39,  41,  40,  41,  -40, 41,  40,
//     40,  -41, -40, 40,  -41, 41,  40,  41, 41,  40,  41,  -40, -40, -40, 41,
//     41,  41,  41,  40,  40,  -40, -40, 41,  41,  -41, 42,  -40, -40, 41, -41,
//     41, 40,  40,  -41, -42, -41, 42,  41,  -40, 41,  -41, 40,  -41, 41,  -41,
//     -41, 40,  42,  42,  -41, -41, -41, -41, -40, -41, 41,  -41, 42,  42, -41,
//     41,  42,  -41, -41, 41,  41,  42,  42,  40,  42,  42,  42,  -42, 42,  42,
//     42, -41, 40,  -41, -41, 41,  -41, -41, 41,  42,  -41, 41,  41,  42,  42,
//     -41, 42,  41,  42,  -41, -43, 42,  41,  42, -42, -42, 42,  -41, 42,  42,
//     42,  -42, -42, -42, 42,  -42, -42, 41,  43,  41,  -41, 41,  -41, -42, 43,
//     42, -43, -41, -41, -43, 42,  -41, -42, -43, 41,  43,  -42, 42,  -42, -42,
//     -42, -42, 42,  42,  -42, -42, -42, 43,  43,  42, 42, 43,  44,  -42, -44,
//     43,  43,  -44, -42, 43,  42,  -42, 43,  -42, -42, -42, 43,  42,  43, -43,
//     -44, 43,  -42, 44, 44,  -43, 43,  -44, -43, 43,  43,  43,  -43, -43, -43,
//     -45, -43, -44, 44,  -43, -44, -44, 44,  -43, -43, 43, 44, -44, -44, -44,
//     -43, -43, -44, -43, -44, 44,  44,  44,  45,  -43, 44,  -44, -44, 43, -45,
//     -44, 43,  44,  -45, 44, 45,  45,  -44, 45,  -44, -44, -43, 43,  43,  -43,
//     -22, -22, 22,  22,  -22, 22,  -23, -21, 19,  -20, -20, -20, 20, -19, -20,
//     20,  -42, -42, -42, 42,  42,  42,  42,  41,  -41, -43, 42,  41,  43, -42,
//     42,  42,  43,  43,  42, -42, -44, -43, 44,  44,  -43, -43, -42, -43, 42,
//     -42, 43,  43,  -43, -40, 42,  -42, 43,  -43, 42,  -42, -43, 43, -42, 43,
//     44,  -43, 43,  43,  43,  43,  -44, 42,  -43, 44,  43,  -44, -43, 43,  43,
//     -43, 45,  -44, 44,  45,  43, 44, -43, -43, 43,  44,  44,  -44, -43, -44,
//     -43, 45,  -44, 45,  -43, 44,  -44, -44, 42,  43,  -43, -43, 42,  -43,
//     -44, -44, 44,  -45, -43, 44,  -44, 43,  43,  45,  44,  44,  42,  -44, 45,
//     44,  -43, 44,  44,  44,  -43, 44, -44, 43, -21, -20, -20, -19, -20, 20,
//     20,  20,  -21, 20,  -20, -21, -21, 21,  -20, 20,  -20, -21, -20, 21,  21,
//     20, 20, 20,  20,  20,  -21, -19, -21, 21,  -20, -20, -21, -20, -21, 21,
//     20,  21,  -21, -20, -20, -20, -21, -21, 19, -20, 21,  21,  -21, -21, 20,
//     -20, 20,  -21, 22,  -21, 20,  -20, 20,  -21, -21, 21,  -20, -20, -21,
//     -21, 21, -21, 22, -21, -21, 21,  -22, 22,  21,  -21, 21,  -21, 22,  22,
//     23,  22,  -22, -21, 22,  22,  22,  -22, 22,  22,  22, -21, 21,  -22, 22,
//     -22, 23,  21,  -22, -22, 22,  22,  -22, 23,  -21, -22, 22,  -22, -22, 22,
//     -23, 22,  22,  21, 22, 22,  22,  -21, 22,  21,  23,  -22, -20, -21, 23,
//     -22, 22,  -22, 22,  21,  22,  22,  22,  -23, -21, -23, -20, 20, -20, -21,
//     -20, 20,  -20, 20};

// short shuffle_index024[256] = {
//     0,   2,   4,   6,   8,   10,  12,  14,  16,  18,  20,  22,  24,  26,  28,
//     30,  32,  34,  36,  38,  40,  42, 44,  46,  48,  50,  52,  54,  56,  58,
//     60,  62,  64,  66,  68,  70,  72,  74,  76,  78,  80,  82,  84,  86, 88,
//     90,  92,  94,  96,  98,  100, 102, 104, 106, 108, 110, 112, 114, 116,
//     118, 120, 122, 124, 126, 128, 130, 132, 134, 136, 138, 140, 142, 144,
//     146, 148, 150, 152, 154, 156, 158, 160, 162, 164, 166, 168, 170, 172,
//     174, 176, 178, 180, 182, 184, 186, 188, 190, 192, 194, 196, 198, 200,
//     202, 204, 206, 208, 210, 212, 214, 216, 218, 220, 222, 224, 226, 228,
//     230, 232, 234, 236, 238, 240, 242, 244, 246, 248, 250, 252, 254, 256,
//     258, 260, 262, 264, 266, 268, 270, 272, 274, 276, 278, 280, 282, 284,
//     286, 288, 290, 292, 294, 296, 298, 300, 302, 304, 306, 308, 310, 312,
//     314, 316, 318, 320, 322, 324, 326, 328, 330, 332, 334, 336, 338, 340,
//     342, 344, 346, 348, 350, 352, 354, 356, 358, 360, 362, 364, 366, 368,
//     370, 372, 374, 376, 378, 380, 382, 384, 386, 388, 390, 392, 394, 396,
//     398, 400, 402, 404, 406, 408, 410, 412, 414, 416, 418, 420, 422, 424,
//     426, 428, 430, 432, 434, 436, 438, 440, 442, 444, 446, 448, 450, 452,
//     454, 456, 458, 460, 462, 464, 466, 468, 470, 472, 474, 476, 478, 480,
//     482, 484, 486, 488, 490, 492, 494, 496, 498, 500, 502, 504, 506, 508,
//     510};
// short shuffle_index135[256] = {
//     1,   3,   5,   7,   9,   11,  13,  15,  17,  19,  21,  23,  25,  27,  29,
//     31,  33,  35,  37,  39,  41,  43, 45,  47,  49,  51,  53,  55,  57,  59,
//     61,  63,  65,  67,  69,  71,  73,  75,  77,  79,  81,  83,  85,  87, 89,
//     91,  93,  95,  97,  99,  101, 103, 105, 107, 109, 111, 113, 115, 117,
//     119, 121, 123, 125, 127, 129, 131, 133, 135, 137, 139, 141, 143, 145,
//     147, 149, 151, 153, 155, 157, 159, 161, 163, 165, 167, 169, 171, 173,
//     175, 177, 179, 181, 183, 185, 187, 189, 191, 193, 195, 197, 199, 201,
//     203, 205, 207, 209, 211, 213, 215, 217, 219, 221, 223, 225, 227, 229,
//     231, 233, 235, 237, 239, 241, 243, 245, 247, 249, 251, 253, 255, 257,
//     259, 261, 263, 265, 267, 269, 271, 273, 275, 277, 279, 281, 283, 285,
//     287, 289, 291, 293, 295, 297, 299, 301, 303, 305, 307, 309, 311, 313,
//     315, 317, 319, 321, 323, 325, 327, 329, 331, 333, 335, 337, 339, 341,
//     343, 345, 347, 349, 351, 353, 355, 357, 359, 361, 363, 365, 367, 369,
//     371, 373, 375, 377, 379, 381, 383, 385, 387, 389, 391, 393, 395, 397,
//     399, 401, 403, 405, 407, 409, 411, 413, 415, 417, 419, 421, 423, 425,
//     427, 429, 431, 433, 435, 437, 439, 441, 443, 445, 447, 449, 451, 453,
//     455, 457, 459, 461, 463, 465, 467, 469, 471, 473, 475, 477, 479, 481,
//     483, 485, 487, 489, 491, 493, 495, 497, 499, 501, 503, 505, 507, 509,
//     511};

/**
 *
 * @param iterNum 迭代次数
 * @param K 信息位长度
 * @param N 信息比特长度
 * @return Polar编码的长度
 *
 */
int Task_nrPolarDecode(__v4096i8 in_LLR, __v2048i16 Index024, __v2048i16 Index135, __v2048i16 InfoIndex,
                       short_struct input_numIter, short_struct input_K, short_struct input_N) {
  short iterNum = input_numIter.data;
  short K       = input_K.data;
  short N       = input_N.data;
  // short      E       = input_E.data;
  // short nMax = 9;
  // uint16_t   N       = polarGetN(K, E, nMax);
  // __v2048i16 InfoIndex;
  // vclaim(InfoIndex);
  // InfoIndex = polarConstruct(K, E, N, 0, 0, nMax);

  //  生成L矩阵
  __v4096i8 L_Matrix_0;
  vclaim(L_Matrix_0);
  __v4096i8 L_Matrix_1;
  vclaim(L_Matrix_1);
  __v4096i8 L_Matrix_2;
  vclaim(L_Matrix_2);
  __v4096i8 L_Matrix_3;
  vclaim(L_Matrix_3);
  __v4096i8 L_Matrix_4;
  vclaim(L_Matrix_4);
  __v4096i8 L_Matrix_5;
  vclaim(L_Matrix_5);
  __v4096i8 L_Matrix_6;
  vclaim(L_Matrix_6);
  __v4096i8 L_Matrix_7;
  vclaim(L_Matrix_7);
  __v4096i8 L_Matrix_8;
  vclaim(L_Matrix_8);
  __v4096i8 L_Matrix_9;
  vclaim(L_Matrix_9);

  //  生成R矩阵
  __v4096i8 R_Matrix_0;
  vclaim(R_Matrix_0);
  __v4096i8 R_Matrix_1;
  vclaim(R_Matrix_1);
  __v4096i8 R_Matrix_2;
  vclaim(R_Matrix_2);
  __v4096i8 R_Matrix_3;
  vclaim(R_Matrix_3);
  __v4096i8 R_Matrix_4;
  vclaim(R_Matrix_4);
  __v4096i8 R_Matrix_5;
  vclaim(R_Matrix_5);
  __v4096i8 R_Matrix_6;
  vclaim(R_Matrix_6);
  __v4096i8 R_Matrix_7;
  vclaim(R_Matrix_7);
  __v4096i8 R_Matrix_8;
  vclaim(R_Matrix_8);
  __v4096i8 R_Matrix_9;
  vclaim(R_Matrix_9);

  __v2048i16 copy_index;
  vclaim(copy_index);
  vrange(copy_index, 2048);
  // copy_index = vadd(copy_index,0,MASKREAD_OFF,2048);

  __v4096i8 alpha;
  vclaim(alpha);
  vbrdcst(alpha, 60, MASKREAD_OFF, 256);

  __v4096i8 blank_vec;
  vclaim(blank_vec);
  vbrdcst(blank_vec, 0, MASKREAD_OFF, 2048);

  __v4096i8 temp_a;
  __v4096i8 temp_b;
  __v4096i8 temp_c;
  __v4096i8 temp_d;
  vclaim(temp_a);
  vclaim(temp_b);
  vclaim(temp_c);
  vclaim(temp_d);
  __v4096i8 temp;
  vclaim(temp);
  __v4096i8 temp1;
  vclaim(temp1);

  // __v4096i8 bugout;
  // vclaim(bugout);
  // bugout = vadd(bugout, in_LLR,MASKREAD_OFF,512);

  if (N == 512) {
    // 初始化R、L矩阵，如果向量定义后默认每个向量中的元素都为0，则可以省略
    L_Matrix_0 = vand(L_Matrix_0, 0, MASKREAD_OFF, 512);
    L_Matrix_1 = vand(L_Matrix_1, 0, MASKREAD_OFF, 512);
    L_Matrix_2 = vand(L_Matrix_2, 0, MASKREAD_OFF, 512);
    L_Matrix_3 = vand(L_Matrix_3, 0, MASKREAD_OFF, 512);
    L_Matrix_4 = vand(L_Matrix_4, 0, MASKREAD_OFF, 512);
    L_Matrix_5 = vand(L_Matrix_5, 0, MASKREAD_OFF, 512);
    L_Matrix_6 = vand(L_Matrix_6, 0, MASKREAD_OFF, 512);
    L_Matrix_7 = vand(L_Matrix_7, 0, MASKREAD_OFF, 512);
    L_Matrix_8 = vand(L_Matrix_8, 0, MASKREAD_OFF, 512);

    R_Matrix_1 = vand(R_Matrix_1, 0, MASKREAD_OFF, 512);
    R_Matrix_2 = vand(R_Matrix_2, 0, MASKREAD_OFF, 512);
    R_Matrix_3 = vand(R_Matrix_3, 0, MASKREAD_OFF, 512);
    R_Matrix_4 = vand(R_Matrix_4, 0, MASKREAD_OFF, 512);
    R_Matrix_5 = vand(R_Matrix_5, 0, MASKREAD_OFF, 512);
    R_Matrix_6 = vand(R_Matrix_6, 0, MASKREAD_OFF, 512);
    R_Matrix_7 = vand(R_Matrix_7, 0, MASKREAD_OFF, 512);
    R_Matrix_8 = vand(R_Matrix_8, 0, MASKREAD_OFF, 512);
    R_Matrix_9 = vand(R_Matrix_9, 0, MASKREAD_OFF, 512);

    vbrdcst(R_Matrix_0, 127, MASKREAD_OFF, 512);
    vshuffle(R_Matrix_0, InfoIndex, blank_vec, SHUFFLE_SCATTER, K);
    vshuffle(L_Matrix_9, copy_index, in_LLR, SHUFFLE_GATHER, 512);
    // vshuffle(L_Matrix_9, copy_index, in_LLR, SHUFFLE_GATHER, 512);

    __v2048i16 move512to256;
    vclaim(move512to256);
    move512to256 = vadd(copy_index, 256, MASKREAD_OFF, 256);

    __v4096i8 result_up_temp;
    vclaim(result_up_temp);
    __v4096i8 result_up_temp_a;
    vclaim(result_up_temp_a);

    //  迭代Polar译码
    for (int iter = 0; iter < iterNum; iter++) {
      //  L Matrix Calculation
      for (int column = 9; column > 0; column--) {
        if (column == 9) {
          vshuffle(temp_a, Index024, in_LLR, SHUFFLE_GATHER, 256);
          vshuffle(temp_b, Index135, in_LLR, SHUFFLE_GATHER, 256);
          vshuffle(temp_c, copy_index, R_Matrix_8, SHUFFLE_GATHER, 256);
          vshuffle(temp_d, move512to256, R_Matrix_8, SHUFFLE_GATHER, 256);
        } else if (column == 8) {
          vshuffle(temp_a, Index024, L_Matrix_8, SHUFFLE_GATHER, 256);
          vshuffle(temp_b, Index135, L_Matrix_8, SHUFFLE_GATHER, 256);
          vshuffle(temp_c, copy_index, R_Matrix_7, SHUFFLE_GATHER, 256);
          vshuffle(temp_d, move512to256, R_Matrix_7, SHUFFLE_GATHER, 256);
        } else if (column == 7) {
          vshuffle(temp_a, Index024, L_Matrix_7, SHUFFLE_GATHER, 256);
          vshuffle(temp_b, Index135, L_Matrix_7, SHUFFLE_GATHER, 256);
          vshuffle(temp_c, copy_index, R_Matrix_6, SHUFFLE_GATHER, 256);
          vshuffle(temp_d, move512to256, R_Matrix_6, SHUFFLE_GATHER, 256);
        } else if (column == 6) {
          vshuffle(temp_a, Index024, L_Matrix_6, SHUFFLE_GATHER, 256);
          vshuffle(temp_b, Index135, L_Matrix_6, SHUFFLE_GATHER, 256);
          vshuffle(temp_c, copy_index, R_Matrix_5, SHUFFLE_GATHER, 256);
          vshuffle(temp_d, move512to256, R_Matrix_5, SHUFFLE_GATHER, 256);
        } else if (column == 5) {
          vshuffle(temp_a, Index024, L_Matrix_5, SHUFFLE_GATHER, 256);
          vshuffle(temp_b, Index135, L_Matrix_5, SHUFFLE_GATHER, 256);
          vshuffle(temp_c, copy_index, R_Matrix_4, SHUFFLE_GATHER, 256);
          vshuffle(temp_d, move512to256, R_Matrix_4, SHUFFLE_GATHER, 256);
        } else if (column == 4) {
          vshuffle(temp_a, Index024, L_Matrix_4, SHUFFLE_GATHER, 256);
          vshuffle(temp_b, Index135, L_Matrix_4, SHUFFLE_GATHER, 256);
          vshuffle(temp_c, copy_index, R_Matrix_3, SHUFFLE_GATHER, 256);
          vshuffle(temp_d, move512to256, R_Matrix_3, SHUFFLE_GATHER, 256);
        } else if (column == 3) {
          vshuffle(temp_a, Index024, L_Matrix_3, SHUFFLE_GATHER, 256);
          vshuffle(temp_b, Index135, L_Matrix_3, SHUFFLE_GATHER, 256);
          vshuffle(temp_c, copy_index, R_Matrix_2, SHUFFLE_GATHER, 256);
          vshuffle(temp_d, move512to256, R_Matrix_2, SHUFFLE_GATHER, 256);
        } else if (column == 2) {
          vshuffle(temp_a, Index024, L_Matrix_2, SHUFFLE_GATHER, 256);
          vshuffle(temp_b, Index135, L_Matrix_2, SHUFFLE_GATHER, 256);
          vshuffle(temp_c, copy_index, R_Matrix_1, SHUFFLE_GATHER, 256);
          vshuffle(temp_d, move512to256, R_Matrix_1, SHUFFLE_GATHER, 256);
        } else if (column == 1) {
          vshuffle(temp_a, Index024, L_Matrix_1, SHUFFLE_GATHER, 256);
          vshuffle(temp_b, Index135, L_Matrix_1, SHUFFLE_GATHER, 256);
          vshuffle(temp_c, copy_index, R_Matrix_0, SHUFFLE_GATHER, 256);
          vshuffle(temp_d, move512to256, R_Matrix_0, SHUFFLE_GATHER, 256);
        }

        __v4096i8 b_add_d;
        b_add_d = vsadd(temp_b, temp_d, MASKREAD_OFF, 256);

        __v4096i8 temp_sign_larger_0;
        __v4096i8 temp_sign_smaller_0;
        temp_sign_smaller_0 = vsgt(b_add_d, 0, MASKREAD_OFF, MASKWRITE_OFF, 256);
        temp_sign_larger_0  = vslt(b_add_d, 0, MASKREAD_OFF, MASKWRITE_OFF, 256);

        __v4096i8 sign_b_add_d;
        sign_b_add_d = vadd(temp_sign_larger_0, 0, MASKREAD_OFF, 256);
        sign_b_add_d = vsub(temp_sign_smaller_0, sign_b_add_d, MASKREAD_OFF, 256);

        vbrdcst(temp_sign_larger_0, 0, MASKREAD_OFF, 256);
        vbrdcst(temp_sign_smaller_0, 0, MASKREAD_OFF, 256);

        temp_sign_smaller_0 = vsgt(temp_a, 0, MASKREAD_OFF, MASKWRITE_OFF, 256);
        temp_sign_larger_0  = vslt(temp_a, 0, MASKREAD_OFF, MASKWRITE_OFF, 256);

        __v4096i8 sign_a;
        sign_a = vadd(temp_sign_larger_0, 0, MASKREAD_OFF, 256);
        sign_a = vsub(temp_sign_smaller_0, sign_a, MASKREAD_OFF, 256);

        result_up_temp_a = vmul(sign_a, sign_b_add_d, MASKREAD_OFF, 256);
        result_up_temp   = vmul(result_up_temp_a, alpha, MASKREAD_OFF, 256);

        __v4096i8 abs_b_add_d;
        __v4096i8 abs_a;
        vsgt(b_add_d, 0, MASKREAD_OFF, MASKWRITE_ON, 256);
        vbrdcst(temp, 0xFF, MASKREAD_OFF, 256);
        vbrdcst(temp1, 0x1, MASKREAD_OFF, 256);
        abs_b_add_d = vxor(b_add_d, temp, MASKREAD_ON, 256);
        abs_b_add_d = vsadd(abs_b_add_d, temp1, MASKREAD_ON, 256);
        // abs_b_add_d = vmul(b_add_d, -1, MASKREAD_ON, MASKWRITE_OFF, 256);
        vsgt(temp_a, 0, MASKREAD_OFF, MASKWRITE_ON, 256);
        abs_a = vxor(temp_a, temp, MASKREAD_ON, 256);
        abs_a = vsadd(abs_a, temp1, MASKREAD_ON, 256);
        // abs_a = vmul(temp_a, -1, MASKREAD_ON, MASKWRITE_OFF, 256);

        __v4096i8 min_bd_a;
        min_bd_a = vmul(abs_a, 0, MASKREAD_OFF, 256); // 冗余
        vsle(abs_b_add_d, abs_a, MASKREAD_OFF, MASKWRITE_ON, 256);
        min_bd_a = vsadd(min_bd_a, abs_a, MASKREAD_ON, 256);
        vsgt(abs_b_add_d, abs_a, MASKREAD_OFF, MASKWRITE_ON, 256);
        min_bd_a = vsadd(min_bd_a, abs_b_add_d, MASKREAD_ON, MASKWRITE_OFF, 256);

        result_up_temp = MUL4096_8_FIXED(result_up_temp, min_bd_a, 6, 256);

        //  down
        temp_sign_smaller_0 = vsgt(temp_c, 0, MASKREAD_OFF, MASKWRITE_OFF, 256);
        temp_sign_larger_0  = vslt(temp_c, 0, MASKREAD_OFF, MASKWRITE_OFF, 256);

        __v4096i8 sign_c;
        sign_c = vadd(temp_sign_larger_0, 0, MASKREAD_OFF, 256);
        sign_c = vsub(temp_sign_smaller_0, sign_c, MASKREAD_OFF, 256);

        __v4096i8 result_down_temp;
        __v4096i8 result_down_temp_a;
        result_down_temp_a = vmul(sign_a, sign_c, MASKREAD_OFF, 256);
        result_down_temp   = vmul(result_down_temp_a, alpha, MASKREAD_OFF, 256);

        __v4096i8 abs_c;
        vsgt(temp_c, 0, MASKREAD_OFF, MASKWRITE_ON, 256);
        abs_c = vxor(temp_c, temp, MASKREAD_ON, 256);
        abs_c = vsadd(abs_c, temp1, MASKREAD_ON, 256);

        __v4096i8 min_a_c;
        min_a_c = vmul(abs_c, 0, MASKREAD_OFF, 256); //  冗余
        vsle(abs_a, abs_c, MASKREAD_OFF, MASKWRITE_ON, 256);
        min_a_c = vsadd(min_a_c, abs_c, MASKREAD_ON, MASKWRITE_OFF, 256);
        vsgt(abs_a, abs_c, MASKREAD_OFF, MASKWRITE_ON, 256);
        min_a_c = vsadd(min_a_c, abs_a, MASKREAD_ON, MASKWRITE_OFF, 256);

        result_down_temp = MUL4096_8_FIXED(result_down_temp, min_a_c, 6, 256);
        result_down_temp = vsadd(result_down_temp, temp_b, MASKREAD_OFF, 256);

        if (column == 9) {
          vshuffle(L_Matrix_8, copy_index, result_up_temp, SHUFFLE_GATHER, 256);
          vshuffle(L_Matrix_8, move512to256, result_down_temp, SHUFFLE_SCATTER, 256);
        } else if (column == 8) {
          vshuffle(L_Matrix_7, copy_index, result_up_temp, SHUFFLE_GATHER, 256);
          vshuffle(L_Matrix_7, move512to256, result_down_temp, SHUFFLE_SCATTER, 256);
        } else if (column == 7) {
          vshuffle(L_Matrix_6, copy_index, result_up_temp, SHUFFLE_GATHER, 256);
          vshuffle(L_Matrix_6, move512to256, result_down_temp, SHUFFLE_SCATTER, 256);
        } else if (column == 6) {
          vshuffle(L_Matrix_5, copy_index, result_up_temp, SHUFFLE_GATHER, 256);
          vshuffle(L_Matrix_5, move512to256, result_down_temp, SHUFFLE_SCATTER, 256);
        } else if (column == 5) {
          vshuffle(L_Matrix_4, copy_index, result_up_temp, SHUFFLE_GATHER, 256);
          vshuffle(L_Matrix_4, move512to256, result_down_temp, SHUFFLE_SCATTER, 256);
        } else if (column == 4) {
          vshuffle(L_Matrix_3, copy_index, result_up_temp, SHUFFLE_GATHER, 256);
          vshuffle(L_Matrix_3, move512to256, result_down_temp, SHUFFLE_SCATTER, 256);
        } else if (column == 3) {
          vshuffle(L_Matrix_2, copy_index, result_up_temp, SHUFFLE_GATHER, 256);
          vshuffle(L_Matrix_2, move512to256, result_down_temp, SHUFFLE_SCATTER, 256);
        } else if (column == 2) {
          vshuffle(L_Matrix_1, copy_index, result_up_temp, SHUFFLE_GATHER, 256);
          vshuffle(L_Matrix_1, move512to256, result_down_temp, SHUFFLE_SCATTER, 256);
        } else if (column == 1) {
          vshuffle(L_Matrix_0, copy_index, result_up_temp, SHUFFLE_GATHER, 256);
          vshuffle(L_Matrix_0, move512to256, result_down_temp, SHUFFLE_SCATTER, 256);
        }
      }

      //  R Matrix Calculation
      for (int column = 1; column < 10; column++) {
        if (column == 1) {
          vshuffle(temp_a, Index024, L_Matrix_1, SHUFFLE_GATHER, 256);
          vshuffle(temp_b, Index135, L_Matrix_1, SHUFFLE_GATHER, 256);
          vshuffle(temp_c, copy_index, R_Matrix_0, SHUFFLE_GATHER, 256);
          vshuffle(temp_d, move512to256, R_Matrix_0, SHUFFLE_GATHER, 256);
        } else if (column == 2) {
          vshuffle(temp_a, Index024, L_Matrix_2, SHUFFLE_GATHER, 256);
          vshuffle(temp_b, Index135, L_Matrix_2, SHUFFLE_GATHER, 256);
          vshuffle(temp_c, copy_index, R_Matrix_1, SHUFFLE_GATHER, 256);
          vshuffle(temp_d, move512to256, R_Matrix_1, SHUFFLE_GATHER, 256);
        } else if (column == 3) {
          vshuffle(temp_a, Index024, L_Matrix_3, SHUFFLE_GATHER, 256);
          vshuffle(temp_b, Index135, L_Matrix_3, SHUFFLE_GATHER, 256);
          vshuffle(temp_c, copy_index, R_Matrix_2, SHUFFLE_GATHER, 256);
          vshuffle(temp_d, move512to256, R_Matrix_2, SHUFFLE_GATHER, 256);
        } else if (column == 4) {
          vshuffle(temp_a, Index024, L_Matrix_4, SHUFFLE_GATHER, 256);
          vshuffle(temp_b, Index135, L_Matrix_4, SHUFFLE_GATHER, 256);
          vshuffle(temp_c, copy_index, R_Matrix_3, SHUFFLE_GATHER, 256);
          vshuffle(temp_d, move512to256, R_Matrix_3, SHUFFLE_GATHER, 256);
        } else if (column == 5) {
          vshuffle(temp_a, Index024, L_Matrix_5, SHUFFLE_GATHER, 256);
          vshuffle(temp_b, Index135, L_Matrix_5, SHUFFLE_GATHER, 256);
          vshuffle(temp_c, copy_index, R_Matrix_4, SHUFFLE_GATHER, 256);
          vshuffle(temp_d, move512to256, R_Matrix_4, SHUFFLE_GATHER, 256);
        } else if (column == 6) {
          vshuffle(temp_a, Index024, L_Matrix_6, SHUFFLE_GATHER, 256);
          vshuffle(temp_b, Index135, L_Matrix_6, SHUFFLE_GATHER, 256);
          vshuffle(temp_c, copy_index, R_Matrix_5, SHUFFLE_GATHER, 256);
          vshuffle(temp_d, move512to256, R_Matrix_5, SHUFFLE_GATHER, 256);
        } else if (column == 7) {
          vshuffle(temp_a, Index024, L_Matrix_7, SHUFFLE_GATHER, 256);
          vshuffle(temp_b, Index135, L_Matrix_7, SHUFFLE_GATHER, 256);
          vshuffle(temp_c, copy_index, R_Matrix_6, SHUFFLE_GATHER, 256);
          vshuffle(temp_d, move512to256, R_Matrix_6, SHUFFLE_GATHER, 256);
        } else if (column == 8) {
          vshuffle(temp_a, Index024, L_Matrix_8, SHUFFLE_GATHER, 256);
          vshuffle(temp_b, Index135, L_Matrix_8, SHUFFLE_GATHER, 256);
          vshuffle(temp_c, copy_index, R_Matrix_7, SHUFFLE_GATHER, 256);
          vshuffle(temp_d, move512to256, R_Matrix_7, SHUFFLE_GATHER, 256);
        } else if (column == 9) {
          vshuffle(temp_a, Index024, L_Matrix_9, SHUFFLE_GATHER, 256);
          vshuffle(temp_b, Index135, L_Matrix_9, SHUFFLE_GATHER, 256);
          vshuffle(temp_c, copy_index, R_Matrix_8, SHUFFLE_GATHER, 256);
          vshuffle(temp_d, move512to256, R_Matrix_8, SHUFFLE_GATHER, 256);
        }

        __v4096i8 b_add_d;
        b_add_d = vsadd(temp_b, temp_d, MASKREAD_OFF, 256);

        __v4096i8 temp_sign_larger_0;
        __v4096i8 temp_sign_smaller_0;
        temp_sign_smaller_0 = vsgt(b_add_d, 0, MASKREAD_OFF, MASKWRITE_OFF, 256);
        temp_sign_larger_0  = vslt(b_add_d, 0, MASKREAD_OFF, MASKWRITE_OFF, 256);

        __v4096i8 sign_b_add_d;
        sign_b_add_d = vadd(temp_sign_larger_0, 0, MASKREAD_OFF, 256);
        sign_b_add_d = vsub(temp_sign_smaller_0, sign_b_add_d, MASKREAD_OFF, 256);

        vbrdcst(temp_sign_larger_0, 0, MASKREAD_OFF, 256);
        vbrdcst(temp_sign_smaller_0, 0, MASKREAD_OFF, 256);

        temp_sign_smaller_0 = vsgt(temp_c, 0, MASKREAD_OFF, MASKWRITE_OFF, 256);
        temp_sign_larger_0  = vslt(temp_c, 0, MASKREAD_OFF, MASKWRITE_OFF, 256);

        __v4096i8 sign_c;
        sign_c = vadd(temp_sign_larger_0, 0, MASKREAD_OFF, 256);
        sign_c = vsub(temp_sign_smaller_0, sign_c, MASKREAD_OFF, 256);

        result_up_temp_a = vmul(sign_c, sign_b_add_d, MASKREAD_OFF, 256);
        result_up_temp   = vmul(result_up_temp_a, alpha, MASKREAD_OFF, 256);

        __v4096i8 abs_b_add_d;
        __v4096i8 abs_c;
        vsgt(b_add_d, 0, MASKREAD_OFF, MASKWRITE_ON, 256);
        abs_b_add_d = vxor(b_add_d, temp, MASKREAD_ON, 256);
        abs_b_add_d = vsadd(abs_b_add_d, temp1, MASKREAD_ON, 256);
        // abs_b_add_d = vmul(b_add_d, -1, MASKREAD_ON, MASKWRITE_OFF, 256);
        vsgt(temp_c, 0, MASKREAD_OFF, MASKWRITE_ON, 256);
        abs_c = vxor(temp_c, temp, MASKREAD_ON, 256);
        abs_c = vsadd(abs_c, temp1, MASKREAD_ON, 256);
        // abs_a = vmul(temp_a, -1, MASKREAD_ON, MASKWRITE_OFF, 256);

        __v4096i8 min_bd_c;
        min_bd_c = vmul(abs_c, 0, MASKREAD_OFF, 256); // 冗余
        vsle(abs_b_add_d, abs_c, MASKREAD_OFF, MASKWRITE_ON, 256);
        min_bd_c = vsadd(min_bd_c, abs_c, MASKREAD_ON, 256);
        vsgt(abs_b_add_d, abs_c, MASKREAD_OFF, MASKWRITE_ON, 256);
        min_bd_c = vsadd(min_bd_c, abs_b_add_d, MASKREAD_ON, MASKWRITE_OFF, 256);

        result_up_temp = MUL4096_8_FIXED(result_up_temp, min_bd_c, 6, 256);

        //  down
        temp_sign_smaller_0 = vsgt(temp_a, 0, MASKREAD_OFF, MASKWRITE_OFF, 256);
        temp_sign_larger_0  = vslt(temp_a, 0, MASKREAD_OFF, MASKWRITE_OFF, 256);

        __v4096i8 sign_a;
        sign_a = vadd(temp_sign_larger_0, 0, MASKREAD_OFF, 256);
        sign_a = vsub(temp_sign_smaller_0, sign_a, MASKREAD_OFF, 256);

        __v4096i8 result_down_temp;
        __v4096i8 result_down_temp_a;
        result_down_temp_a = vmul(sign_a, sign_c, MASKREAD_OFF, 256);
        result_down_temp   = vmul(result_down_temp_a, alpha, MASKREAD_OFF, 256);

        __v4096i8 abs_a;
        vsgt(temp_a, 0, MASKREAD_OFF, MASKWRITE_ON, 256);
        abs_a = vxor(temp_a, temp, MASKREAD_ON, 256);
        abs_a = vsadd(abs_a, temp1, MASKREAD_ON, 256);
        // abs_a = vmul(temp_a, -1, MASKREAD_ON, MASKWRITE_OFF, 256);

        __v4096i8 min_a_c;
        min_a_c = vmul(abs_c, 0, MASKREAD_OFF, 256); //  冗余
        vsle(abs_a, abs_c, MASKREAD_OFF, MASKWRITE_ON, 256);
        min_a_c = vsadd(min_a_c, abs_c, MASKREAD_ON, MASKWRITE_OFF, 256);
        vsgt(abs_a, abs_c, MASKREAD_OFF, MASKWRITE_ON, 256);
        min_a_c = vsadd(min_a_c, abs_a, MASKREAD_ON, MASKWRITE_OFF, 256);

        result_down_temp = MUL4096_8_FIXED(result_down_temp, min_a_c, 6, 256);
        result_down_temp = vsadd(result_down_temp, temp_d, MASKREAD_OFF, 256);

        if (column == 9) {
          vshuffle(R_Matrix_9, Index024, result_up_temp, SHUFFLE_SCATTER, 256);
          vshuffle(R_Matrix_9, Index135, result_down_temp, SHUFFLE_SCATTER, 256);
        } else if (column == 8) {
          vshuffle(R_Matrix_8, Index024, result_up_temp, SHUFFLE_SCATTER, 256);
          vshuffle(R_Matrix_8, Index135, result_down_temp, SHUFFLE_SCATTER, 256);
        } else if (column == 7) {
          vshuffle(R_Matrix_7, Index024, result_up_temp, SHUFFLE_SCATTER, 256);
          vshuffle(R_Matrix_7, Index135, result_down_temp, SHUFFLE_SCATTER, 256);
        } else if (column == 6) {
          vshuffle(R_Matrix_6, Index024, result_up_temp, SHUFFLE_SCATTER, 256);
          vshuffle(R_Matrix_6, Index135, result_down_temp, SHUFFLE_SCATTER, 256);
        } else if (column == 5) {
          vshuffle(R_Matrix_5, Index024, result_up_temp, SHUFFLE_SCATTER, 256);
          vshuffle(R_Matrix_5, Index135, result_down_temp, SHUFFLE_SCATTER, 256);
        } else if (column == 4) {
          vshuffle(R_Matrix_4, Index024, result_up_temp, SHUFFLE_SCATTER, 256);
          vshuffle(R_Matrix_4, Index135, result_down_temp, SHUFFLE_SCATTER, 256);
        } else if (column == 3) {
          vshuffle(R_Matrix_3, Index024, result_up_temp, SHUFFLE_SCATTER, 256);
          vshuffle(R_Matrix_3, Index135, result_down_temp, SHUFFLE_SCATTER, 256);
        } else if (column == 2) {
          vshuffle(R_Matrix_2, Index024, result_up_temp, SHUFFLE_SCATTER, 256);
          vshuffle(R_Matrix_2, Index135, result_down_temp, SHUFFLE_SCATTER, 256);
        } else if (column == 1) {
          vshuffle(R_Matrix_1, Index024, result_up_temp, SHUFFLE_SCATTER, 256);
          vshuffle(R_Matrix_1, Index135, result_down_temp, SHUFFLE_SCATTER, 256);
        }
      }
    }

  } else if (N == 256) {
    short caculation_length = N / 2;
    // 初始化R、L矩阵，如果向量定义后默认每个向量中的元素都为0，则可以省略
    L_Matrix_0 = vand(L_Matrix_0, 0, MASKREAD_OFF, N);
    L_Matrix_1 = vand(L_Matrix_1, 0, MASKREAD_OFF, N);
    L_Matrix_2 = vand(L_Matrix_2, 0, MASKREAD_OFF, N);
    L_Matrix_3 = vand(L_Matrix_3, 0, MASKREAD_OFF, N);
    L_Matrix_4 = vand(L_Matrix_4, 0, MASKREAD_OFF, N);
    L_Matrix_5 = vand(L_Matrix_5, 0, MASKREAD_OFF, N);
    L_Matrix_6 = vand(L_Matrix_6, 0, MASKREAD_OFF, N);
    L_Matrix_7 = vand(L_Matrix_7, 0, MASKREAD_OFF, N);

    R_Matrix_1 = vand(R_Matrix_1, 0, MASKREAD_OFF, N);
    R_Matrix_2 = vand(R_Matrix_2, 0, MASKREAD_OFF, N);
    R_Matrix_3 = vand(R_Matrix_3, 0, MASKREAD_OFF, N);
    R_Matrix_4 = vand(R_Matrix_4, 0, MASKREAD_OFF, N);
    R_Matrix_5 = vand(R_Matrix_5, 0, MASKREAD_OFF, N);
    R_Matrix_6 = vand(R_Matrix_6, 0, MASKREAD_OFF, N);
    R_Matrix_7 = vand(R_Matrix_7, 0, MASKREAD_OFF, N);
    R_Matrix_8 = vand(R_Matrix_8, 0, MASKREAD_OFF, N);

    vbrdcst(R_Matrix_0, 127, MASKREAD_OFF, N);
    vshuffle(R_Matrix_0, InfoIndex, blank_vec, SHUFFLE_SCATTER, K);
    vshuffle(L_Matrix_8, copy_index, in_LLR, SHUFFLE_GATHER, N);

    __v2048i16 move256to128;
    vclaim(move256to128);
    move256to128 = vadd(copy_index, 128, MASKREAD_OFF, 128);

    __v4096i8 result_up_temp;
    vclaim(result_up_temp);
    __v4096i8 result_up_temp_a;
    vclaim(result_up_temp_a);

    //  迭代Polar译码
    for (int iter = 0; iter < iterNum; iter++) {
      //  L Matrix Calculation
      for (int column = 8; column > 0; column--) {
        if (column == 8) {
          vshuffle(temp_a, Index024, L_Matrix_8, SHUFFLE_GATHER, caculation_length);
          vshuffle(temp_b, Index135, L_Matrix_8, SHUFFLE_GATHER, caculation_length);
          vshuffle(temp_c, copy_index, R_Matrix_7, SHUFFLE_GATHER, caculation_length);
          vshuffle(temp_d, move256to128, R_Matrix_7, SHUFFLE_GATHER, caculation_length);
        } else if (column == 7) {
          vshuffle(temp_a, Index024, L_Matrix_7, SHUFFLE_GATHER, caculation_length);
          vshuffle(temp_b, Index135, L_Matrix_7, SHUFFLE_GATHER, caculation_length);
          vshuffle(temp_c, copy_index, R_Matrix_6, SHUFFLE_GATHER, caculation_length);
          vshuffle(temp_d, move256to128, R_Matrix_6, SHUFFLE_GATHER, caculation_length);
        } else if (column == 6) {
          vshuffle(temp_a, Index024, L_Matrix_6, SHUFFLE_GATHER, caculation_length);
          vshuffle(temp_b, Index135, L_Matrix_6, SHUFFLE_GATHER, caculation_length);
          vshuffle(temp_c, copy_index, R_Matrix_5, SHUFFLE_GATHER, caculation_length);
          vshuffle(temp_d, move256to128, R_Matrix_5, SHUFFLE_GATHER, caculation_length);
        } else if (column == 5) {
          vshuffle(temp_a, Index024, L_Matrix_5, SHUFFLE_GATHER, caculation_length);
          vshuffle(temp_b, Index135, L_Matrix_5, SHUFFLE_GATHER, caculation_length);
          vshuffle(temp_c, copy_index, R_Matrix_4, SHUFFLE_GATHER, caculation_length);
          vshuffle(temp_d, move256to128, R_Matrix_4, SHUFFLE_GATHER, caculation_length);
        } else if (column == 4) {
          vshuffle(temp_a, Index024, L_Matrix_4, SHUFFLE_GATHER, caculation_length);
          vshuffle(temp_b, Index135, L_Matrix_4, SHUFFLE_GATHER, caculation_length);
          vshuffle(temp_c, copy_index, R_Matrix_3, SHUFFLE_GATHER, caculation_length);
          vshuffle(temp_d, move256to128, R_Matrix_3, SHUFFLE_GATHER, caculation_length);
        } else if (column == 3) {
          vshuffle(temp_a, Index024, L_Matrix_3, SHUFFLE_GATHER, caculation_length);
          vshuffle(temp_b, Index135, L_Matrix_3, SHUFFLE_GATHER, caculation_length);
          vshuffle(temp_c, copy_index, R_Matrix_2, SHUFFLE_GATHER, caculation_length);
          vshuffle(temp_d, move256to128, R_Matrix_2, SHUFFLE_GATHER, caculation_length);
        } else if (column == 2) {
          vshuffle(temp_a, Index024, L_Matrix_2, SHUFFLE_GATHER, caculation_length);
          vshuffle(temp_b, Index135, L_Matrix_2, SHUFFLE_GATHER, caculation_length);
          vshuffle(temp_c, copy_index, R_Matrix_1, SHUFFLE_GATHER, caculation_length);
          vshuffle(temp_d, move256to128, R_Matrix_1, SHUFFLE_GATHER, caculation_length);
        } else if (column == 1) {
          vshuffle(temp_a, Index024, L_Matrix_1, SHUFFLE_GATHER, caculation_length);
          vshuffle(temp_b, Index135, L_Matrix_1, SHUFFLE_GATHER, caculation_length);
          vshuffle(temp_c, copy_index, R_Matrix_0, SHUFFLE_GATHER, caculation_length);
          vshuffle(temp_d, move256to128, R_Matrix_0, SHUFFLE_GATHER, caculation_length);
        }

        __v4096i8 b_add_d;
        b_add_d = vsadd(temp_b, temp_d, MASKREAD_OFF, caculation_length);

        __v4096i8 temp_sign_larger_0;
        __v4096i8 temp_sign_smaller_0;
        temp_sign_smaller_0 = vsgt(b_add_d, 0, MASKREAD_OFF, MASKWRITE_OFF, caculation_length);
        temp_sign_larger_0  = vslt(b_add_d, 0, MASKREAD_OFF, MASKWRITE_OFF, caculation_length);

        __v4096i8 sign_b_add_d;
        sign_b_add_d = vadd(temp_sign_larger_0, 0, MASKREAD_OFF, caculation_length);
        sign_b_add_d = vsub(temp_sign_smaller_0, sign_b_add_d, MASKREAD_OFF, caculation_length);

        vbrdcst(temp_sign_larger_0, 0, MASKREAD_OFF, caculation_length);
        vbrdcst(temp_sign_smaller_0, 0, MASKREAD_OFF, caculation_length);

        temp_sign_smaller_0 = vsgt(temp_a, 0, MASKREAD_OFF, MASKWRITE_OFF, caculation_length);
        temp_sign_larger_0  = vslt(temp_a, 0, MASKREAD_OFF, MASKWRITE_OFF, caculation_length);

        __v4096i8 sign_a;
        sign_a = vadd(temp_sign_larger_0, 0, MASKREAD_OFF, caculation_length);
        sign_a = vsub(temp_sign_smaller_0, sign_a, MASKREAD_OFF, caculation_length);

        result_up_temp_a = vmul(sign_a, sign_b_add_d, MASKREAD_OFF, caculation_length);
        result_up_temp   = vmul(result_up_temp_a, alpha, MASKREAD_OFF, caculation_length);

        __v4096i8 abs_b_add_d;
        __v4096i8 abs_a;
        vsgt(b_add_d, 0, MASKREAD_OFF, MASKWRITE_ON, caculation_length);
        abs_b_add_d = vxor(b_add_d, 0xFF, MASKREAD_ON, caculation_length);
        abs_b_add_d = vsadd(abs_b_add_d, 1, MASKREAD_ON, caculation_length);

        vsgt(temp_a, 0, MASKREAD_OFF, MASKWRITE_ON, caculation_length);
        abs_a = vxor(temp_a, 0xFF, MASKREAD_ON, caculation_length);
        abs_a = vsadd(abs_a, 1, MASKREAD_ON, caculation_length);

        __v4096i8 min_bd_a;
        min_bd_a = vmul(abs_a, 0, MASKREAD_OFF, caculation_length); // 冗余
        vsle(abs_b_add_d, abs_a, MASKREAD_OFF, MASKWRITE_ON, caculation_length);
        min_bd_a = vadd(abs_a, min_bd_a, MASKREAD_ON, caculation_length);
        vsgt(abs_b_add_d, abs_a, MASKREAD_OFF, MASKWRITE_ON, caculation_length);
        min_bd_a = vadd(abs_b_add_d, min_bd_a, MASKREAD_ON, MASKWRITE_OFF, caculation_length);

        result_up_temp = MUL4096_8_FIXED(result_up_temp, min_bd_a, 6, caculation_length);

        //  down
        temp_sign_smaller_0 = vsgt(temp_c, 0, MASKREAD_OFF, MASKWRITE_OFF, caculation_length);
        temp_sign_larger_0  = vslt(temp_c, 0, MASKREAD_OFF, MASKWRITE_OFF, caculation_length);

        __v4096i8 sign_c;
        sign_c = vadd(temp_sign_larger_0, 0, MASKREAD_OFF, caculation_length);
        sign_c = vsub(temp_sign_smaller_0, sign_c, MASKREAD_OFF, caculation_length);

        __v4096i8 result_down_temp;
        __v4096i8 result_down_temp_a;
        result_down_temp_a = vmul(sign_a, sign_c, MASKREAD_OFF, caculation_length);
        result_down_temp   = vmul(result_down_temp_a, alpha, MASKREAD_OFF, caculation_length);

        __v4096i8 abs_c;
        vsgt(temp_c, 0, MASKREAD_OFF, MASKWRITE_ON, caculation_length);
        abs_c = vxor(temp_c, 0xFF, MASKREAD_ON, caculation_length);
        abs_c = vsadd(abs_c, 1, MASKREAD_ON, caculation_length);

        __v4096i8 min_a_c;
        min_a_c = vmul(abs_c, 0, MASKREAD_OFF, caculation_length); //  冗余
        vsle(abs_a, abs_c, MASKREAD_OFF, MASKWRITE_ON, caculation_length);
        min_a_c = vadd(abs_c, min_a_c, MASKREAD_ON, MASKWRITE_OFF, caculation_length);
        vsgt(abs_a, abs_c, MASKREAD_OFF, MASKWRITE_ON, caculation_length);
        min_a_c = vadd(abs_a, min_a_c, MASKREAD_ON, MASKWRITE_OFF, caculation_length);

        result_down_temp = MUL4096_8_FIXED(result_down_temp, min_a_c, 6, caculation_length);
        result_down_temp = vsadd(result_down_temp, temp_b, MASKREAD_OFF, caculation_length);

        if (column == 8) {
          vshuffle(L_Matrix_7, copy_index, result_up_temp, SHUFFLE_GATHER, caculation_length);
          vshuffle(L_Matrix_7, move256to128, result_down_temp, SHUFFLE_SCATTER, caculation_length);
        } else if (column == 7) {
          vshuffle(L_Matrix_6, copy_index, result_up_temp, SHUFFLE_GATHER, caculation_length);
          vshuffle(L_Matrix_6, move256to128, result_down_temp, SHUFFLE_SCATTER, caculation_length);
        } else if (column == 6) {
          vshuffle(L_Matrix_5, copy_index, result_up_temp, SHUFFLE_GATHER, caculation_length);
          vshuffle(L_Matrix_5, move256to128, result_down_temp, SHUFFLE_SCATTER, caculation_length);
        } else if (column == 5) {
          vshuffle(L_Matrix_4, copy_index, result_up_temp, SHUFFLE_GATHER, caculation_length);
          vshuffle(L_Matrix_4, move256to128, result_down_temp, SHUFFLE_SCATTER, caculation_length);
        } else if (column == 4) {
          vshuffle(L_Matrix_3, copy_index, result_up_temp, SHUFFLE_GATHER, caculation_length);
          vshuffle(L_Matrix_3, move256to128, result_down_temp, SHUFFLE_SCATTER, caculation_length);
        } else if (column == 3) {
          vshuffle(L_Matrix_2, copy_index, result_up_temp, SHUFFLE_GATHER, caculation_length);
          vshuffle(L_Matrix_2, move256to128, result_down_temp, SHUFFLE_SCATTER, caculation_length);
        } else if (column == 2) {
          vshuffle(L_Matrix_1, copy_index, result_up_temp, SHUFFLE_GATHER, caculation_length);
          vshuffle(L_Matrix_1, move256to128, result_down_temp, SHUFFLE_SCATTER, caculation_length);
        } else if (column == 1) {
          vshuffle(L_Matrix_0, copy_index, result_up_temp, SHUFFLE_GATHER, caculation_length);
          vshuffle(L_Matrix_0, move256to128, result_down_temp, SHUFFLE_SCATTER, caculation_length);
        }
      }

      //  R Matrix Calculation
      for (int column = 1; column < 9; column++) {
        if (column == 1) {
          vshuffle(temp_a, Index024, L_Matrix_1, SHUFFLE_GATHER, caculation_length);
          vshuffle(temp_b, Index135, L_Matrix_1, SHUFFLE_GATHER, caculation_length);
          vshuffle(temp_c, copy_index, R_Matrix_0, SHUFFLE_GATHER, caculation_length);
          vshuffle(temp_d, move256to128, R_Matrix_0, SHUFFLE_GATHER, caculation_length);
        } else if (column == 2) {
          vshuffle(temp_a, Index024, L_Matrix_2, SHUFFLE_GATHER, caculation_length);
          vshuffle(temp_b, Index135, L_Matrix_2, SHUFFLE_GATHER, caculation_length);
          vshuffle(temp_c, copy_index, R_Matrix_1, SHUFFLE_GATHER, caculation_length);
          vshuffle(temp_d, move256to128, R_Matrix_1, SHUFFLE_GATHER, caculation_length);
        } else if (column == 3) {
          vshuffle(temp_a, Index024, L_Matrix_3, SHUFFLE_GATHER, caculation_length);
          vshuffle(temp_b, Index135, L_Matrix_3, SHUFFLE_GATHER, caculation_length);
          vshuffle(temp_c, copy_index, R_Matrix_2, SHUFFLE_GATHER, caculation_length);
          vshuffle(temp_d, move256to128, R_Matrix_2, SHUFFLE_GATHER, caculation_length);
        } else if (column == 4) {
          vshuffle(temp_a, Index024, L_Matrix_4, SHUFFLE_GATHER, caculation_length);
          vshuffle(temp_b, Index135, L_Matrix_4, SHUFFLE_GATHER, caculation_length);
          vshuffle(temp_c, copy_index, R_Matrix_3, SHUFFLE_GATHER, caculation_length);
          vshuffle(temp_d, move256to128, R_Matrix_3, SHUFFLE_GATHER, caculation_length);
        } else if (column == 5) {
          vshuffle(temp_a, Index024, L_Matrix_5, SHUFFLE_GATHER, caculation_length);
          vshuffle(temp_b, Index135, L_Matrix_5, SHUFFLE_GATHER, caculation_length);
          vshuffle(temp_c, copy_index, R_Matrix_4, SHUFFLE_GATHER, caculation_length);
          vshuffle(temp_d, move256to128, R_Matrix_4, SHUFFLE_GATHER, caculation_length);
        } else if (column == 6) {
          vshuffle(temp_a, Index024, L_Matrix_6, SHUFFLE_GATHER, caculation_length);
          vshuffle(temp_b, Index135, L_Matrix_6, SHUFFLE_GATHER, caculation_length);
          vshuffle(temp_c, copy_index, R_Matrix_5, SHUFFLE_GATHER, caculation_length);
          vshuffle(temp_d, move256to128, R_Matrix_5, SHUFFLE_GATHER, caculation_length);
        } else if (column == 7) {
          vshuffle(temp_a, Index024, L_Matrix_7, SHUFFLE_GATHER, caculation_length);
          vshuffle(temp_b, Index135, L_Matrix_7, SHUFFLE_GATHER, caculation_length);
          vshuffle(temp_c, copy_index, R_Matrix_6, SHUFFLE_GATHER, caculation_length);
          vshuffle(temp_d, move256to128, R_Matrix_6, SHUFFLE_GATHER, caculation_length);
        } else if (column == 8) {
          vshuffle(temp_a, Index024, L_Matrix_8, SHUFFLE_GATHER, caculation_length);
          vshuffle(temp_b, Index135, L_Matrix_8, SHUFFLE_GATHER, caculation_length);
          vshuffle(temp_c, copy_index, R_Matrix_7, SHUFFLE_GATHER, caculation_length);
          vshuffle(temp_d, move256to128, R_Matrix_7, SHUFFLE_GATHER, caculation_length);
        }

        __v4096i8 b_add_d;
        b_add_d = vsadd(temp_b, temp_d, MASKREAD_OFF, caculation_length);

        __v4096i8 temp_sign_larger_0;
        __v4096i8 temp_sign_smaller_0;
        temp_sign_smaller_0 = vsgt(b_add_d, 0, MASKREAD_OFF, MASKWRITE_OFF, caculation_length);
        temp_sign_larger_0  = vslt(b_add_d, 0, MASKREAD_OFF, MASKWRITE_OFF, caculation_length);

        __v4096i8 sign_b_add_d;
        sign_b_add_d = vadd(temp_sign_larger_0, 0, MASKREAD_OFF, caculation_length);
        sign_b_add_d = vsub(temp_sign_smaller_0, sign_b_add_d, MASKREAD_OFF, caculation_length);

        vbrdcst(temp_sign_larger_0, 0, MASKREAD_OFF, caculation_length);
        vbrdcst(temp_sign_smaller_0, 0, MASKREAD_OFF, caculation_length);

        temp_sign_smaller_0 = vsgt(temp_c, 0, MASKREAD_OFF, MASKWRITE_OFF, caculation_length);
        temp_sign_larger_0  = vslt(temp_c, 0, MASKREAD_OFF, MASKWRITE_OFF, caculation_length);

        __v4096i8 sign_c;
        sign_c = vadd(temp_sign_larger_0, 0, MASKREAD_OFF, caculation_length);
        sign_c = vsub(temp_sign_smaller_0, sign_c, MASKREAD_OFF, caculation_length);

        result_up_temp_a = vmul(sign_c, sign_b_add_d, MASKREAD_OFF, caculation_length);
        result_up_temp   = vmul(result_up_temp_a, alpha, MASKREAD_OFF, caculation_length);

        __v4096i8 abs_b_add_d;
        __v4096i8 abs_c;
        vsgt(b_add_d, 0, MASKREAD_OFF, MASKWRITE_ON, caculation_length);
        abs_b_add_d = vxor(b_add_d, 0xFF, MASKREAD_ON, caculation_length);
        abs_b_add_d = vsadd(abs_b_add_d, 1, MASKREAD_ON, caculation_length);

        vsgt(temp_c, 0, MASKREAD_OFF, MASKWRITE_ON, caculation_length);
        abs_c = vxor(temp_c, 0xFF, MASKREAD_ON, caculation_length);
        abs_c = vsadd(abs_c, 1, MASKREAD_ON, caculation_length);

        __v4096i8 min_bd_c;
        min_bd_c = vmul(abs_c, 0, MASKREAD_OFF, caculation_length); // 冗余
        vsle(abs_b_add_d, abs_c, MASKREAD_OFF, MASKWRITE_ON, caculation_length);
        min_bd_c = vadd(abs_c, min_bd_c, MASKREAD_ON, caculation_length);
        vsgt(abs_b_add_d, abs_c, MASKREAD_OFF, MASKWRITE_ON, caculation_length);
        min_bd_c = vadd(abs_b_add_d, min_bd_c, MASKREAD_ON, MASKWRITE_OFF, caculation_length);

        result_up_temp = MUL4096_8_FIXED(result_up_temp, min_bd_c, 6, caculation_length);

        //  down
        temp_sign_smaller_0 = vsgt(temp_a, 0, MASKREAD_OFF, MASKWRITE_OFF, caculation_length);
        temp_sign_larger_0  = vslt(temp_a, 0, MASKREAD_OFF, MASKWRITE_OFF, caculation_length);

        __v4096i8 sign_a;
        sign_a = vadd(temp_sign_larger_0, 0, MASKREAD_OFF, caculation_length);
        sign_a = vsub(temp_sign_smaller_0, sign_a, MASKREAD_OFF, caculation_length);

        __v4096i8 result_down_temp;
        __v4096i8 result_down_temp_a;
        result_down_temp_a = vmul(sign_a, sign_c, MASKREAD_OFF, caculation_length);
        result_down_temp   = vmul(result_down_temp_a, alpha, MASKREAD_OFF, caculation_length);

        __v4096i8 abs_a;
        vsgt(temp_a, 0, MASKREAD_OFF, MASKWRITE_ON, caculation_length);
        abs_a = vxor(temp_a, 0xFF, MASKREAD_ON, caculation_length);
        abs_a = vsadd(abs_a, 1, MASKREAD_ON, caculation_length);

        __v4096i8 min_a_c;
        min_a_c = vmul(abs_c, 0, MASKREAD_OFF, caculation_length); //  冗余
        vsle(abs_a, abs_c, MASKREAD_OFF, MASKWRITE_ON, caculation_length);
        min_a_c = vadd(abs_c, min_a_c, MASKREAD_ON, MASKWRITE_OFF, caculation_length);
        vsgt(abs_a, abs_c, MASKREAD_OFF, MASKWRITE_ON, caculation_length);
        min_a_c = vadd(abs_a, min_a_c, MASKREAD_ON, MASKWRITE_OFF, caculation_length);

        result_down_temp = MUL4096_8_FIXED(result_down_temp, min_a_c, 6, caculation_length);
        result_down_temp = vsadd(result_down_temp, temp_d, MASKREAD_OFF, caculation_length);

        if (column == 8) {
          vshuffle(R_Matrix_8, Index024, result_up_temp, SHUFFLE_SCATTER, caculation_length);
          vshuffle(R_Matrix_8, Index135, result_down_temp, SHUFFLE_SCATTER, caculation_length);
        } else if (column == 7) {
          vshuffle(R_Matrix_7, Index024, result_up_temp, SHUFFLE_SCATTER, caculation_length);
          vshuffle(R_Matrix_7, Index135, result_down_temp, SHUFFLE_SCATTER, caculation_length);
        } else if (column == 6) {
          vshuffle(R_Matrix_6, Index024, result_up_temp, SHUFFLE_SCATTER, caculation_length);
          vshuffle(R_Matrix_6, Index135, result_down_temp, SHUFFLE_SCATTER, caculation_length);
        } else if (column == 5) {
          vshuffle(R_Matrix_5, Index024, result_up_temp, SHUFFLE_SCATTER, caculation_length);
          vshuffle(R_Matrix_5, Index135, result_down_temp, SHUFFLE_SCATTER, caculation_length);
        } else if (column == 4) {
          vshuffle(R_Matrix_4, Index024, result_up_temp, SHUFFLE_SCATTER, caculation_length);
          vshuffle(R_Matrix_4, Index135, result_down_temp, SHUFFLE_SCATTER, caculation_length);
        } else if (column == 3) {
          vshuffle(R_Matrix_3, Index024, result_up_temp, SHUFFLE_SCATTER, caculation_length);
          vshuffle(R_Matrix_3, Index135, result_down_temp, SHUFFLE_SCATTER, caculation_length);
        } else if (column == 2) {
          vshuffle(R_Matrix_2, Index024, result_up_temp, SHUFFLE_SCATTER, caculation_length);
          vshuffle(R_Matrix_2, Index135, result_down_temp, SHUFFLE_SCATTER, caculation_length);
        } else if (column == 1) {
          vshuffle(R_Matrix_1, Index024, result_up_temp, SHUFFLE_SCATTER, caculation_length);
          vshuffle(R_Matrix_1, Index135, result_down_temp, SHUFFLE_SCATTER, caculation_length);
        }
      }
    }
  } else if (N == 128) {
    short caculation_length = N / 2;
    // 初始化R、L矩阵，如果向量定义后默认每个向量中的元素都为0，则可以省略
    L_Matrix_0 = vand(L_Matrix_0, 0, MASKREAD_OFF, N);
    L_Matrix_1 = vand(L_Matrix_1, 0, MASKREAD_OFF, N);
    L_Matrix_2 = vand(L_Matrix_2, 0, MASKREAD_OFF, N);
    L_Matrix_3 = vand(L_Matrix_3, 0, MASKREAD_OFF, N);
    L_Matrix_4 = vand(L_Matrix_4, 0, MASKREAD_OFF, N);
    L_Matrix_5 = vand(L_Matrix_5, 0, MASKREAD_OFF, N);
    L_Matrix_6 = vand(L_Matrix_6, 0, MASKREAD_OFF, N);

    R_Matrix_1 = vand(R_Matrix_1, 0, MASKREAD_OFF, N);
    R_Matrix_2 = vand(R_Matrix_2, 0, MASKREAD_OFF, N);
    R_Matrix_3 = vand(R_Matrix_3, 0, MASKREAD_OFF, N);
    R_Matrix_4 = vand(R_Matrix_4, 0, MASKREAD_OFF, N);
    R_Matrix_5 = vand(R_Matrix_5, 0, MASKREAD_OFF, N);
    R_Matrix_6 = vand(R_Matrix_6, 0, MASKREAD_OFF, N);
    R_Matrix_7 = vand(R_Matrix_7, 0, MASKREAD_OFF, N);

    vbrdcst(R_Matrix_0, 127, MASKREAD_OFF, N);
    vshuffle(R_Matrix_0, InfoIndex, blank_vec, SHUFFLE_SCATTER, K);
    vshuffle(L_Matrix_7, copy_index, in_LLR, SHUFFLE_GATHER, N);

    __v2048i16 move128to64;
    vclaim(move128to64);
    move128to64 = vadd(copy_index, 64, MASKREAD_OFF, 64);

    __v4096i8 result_up_temp;
    vclaim(result_up_temp);
    __v4096i8 result_up_temp_a;
    vclaim(result_up_temp_a);

    //  迭代Polar译码
    for (int iter = 0; iter < iterNum; iter++) {
      //  L Matrix Calculation
      for (int column = 7; column > 0; column--) {
        if (column == 7) {
          vshuffle(temp_a, Index024, L_Matrix_7, SHUFFLE_GATHER, caculation_length);
          vshuffle(temp_b, Index135, L_Matrix_7, SHUFFLE_GATHER, caculation_length);
          vshuffle(temp_c, copy_index, R_Matrix_6, SHUFFLE_GATHER, caculation_length);
          vshuffle(temp_d, move128to64, R_Matrix_6, SHUFFLE_GATHER, caculation_length);
        } else if (column == 6) {
          vshuffle(temp_a, Index024, L_Matrix_6, SHUFFLE_GATHER, caculation_length);
          vshuffle(temp_b, Index135, L_Matrix_6, SHUFFLE_GATHER, caculation_length);
          vshuffle(temp_c, copy_index, R_Matrix_5, SHUFFLE_GATHER, caculation_length);
          vshuffle(temp_d, move128to64, R_Matrix_5, SHUFFLE_GATHER, caculation_length);
        } else if (column == 5) {
          vshuffle(temp_a, Index024, L_Matrix_5, SHUFFLE_GATHER, caculation_length);
          vshuffle(temp_b, Index135, L_Matrix_5, SHUFFLE_GATHER, caculation_length);
          vshuffle(temp_c, copy_index, R_Matrix_4, SHUFFLE_GATHER, caculation_length);
          vshuffle(temp_d, move128to64, R_Matrix_4, SHUFFLE_GATHER, caculation_length);
        } else if (column == 4) {
          vshuffle(temp_a, Index024, L_Matrix_4, SHUFFLE_GATHER, caculation_length);
          vshuffle(temp_b, Index135, L_Matrix_4, SHUFFLE_GATHER, caculation_length);
          vshuffle(temp_c, copy_index, R_Matrix_3, SHUFFLE_GATHER, caculation_length);
          vshuffle(temp_d, move128to64, R_Matrix_3, SHUFFLE_GATHER, caculation_length);
        } else if (column == 3) {
          vshuffle(temp_a, Index024, L_Matrix_3, SHUFFLE_GATHER, caculation_length);
          vshuffle(temp_b, Index135, L_Matrix_3, SHUFFLE_GATHER, caculation_length);
          vshuffle(temp_c, copy_index, R_Matrix_2, SHUFFLE_GATHER, caculation_length);
          vshuffle(temp_d, move128to64, R_Matrix_2, SHUFFLE_GATHER, caculation_length);
        } else if (column == 2) {
          vshuffle(temp_a, Index024, L_Matrix_2, SHUFFLE_GATHER, caculation_length);
          vshuffle(temp_b, Index135, L_Matrix_2, SHUFFLE_GATHER, caculation_length);
          vshuffle(temp_c, copy_index, R_Matrix_1, SHUFFLE_GATHER, caculation_length);
          vshuffle(temp_d, move128to64, R_Matrix_1, SHUFFLE_GATHER, caculation_length);
        } else if (column == 1) {
          vshuffle(temp_a, Index024, L_Matrix_1, SHUFFLE_GATHER, caculation_length);
          vshuffle(temp_b, Index135, L_Matrix_1, SHUFFLE_GATHER, caculation_length);
          vshuffle(temp_c, copy_index, R_Matrix_0, SHUFFLE_GATHER, caculation_length);
          vshuffle(temp_d, move128to64, R_Matrix_0, SHUFFLE_GATHER, caculation_length);
        }

        __v4096i8 b_add_d;
        b_add_d = vsadd(temp_b, temp_d, MASKREAD_OFF, caculation_length);

        __v4096i8 temp_sign_larger_0;
        __v4096i8 temp_sign_smaller_0;
        temp_sign_smaller_0 = vsgt(b_add_d, 0, MASKREAD_OFF, MASKWRITE_OFF, caculation_length);
        temp_sign_larger_0  = vslt(b_add_d, 0, MASKREAD_OFF, MASKWRITE_OFF, caculation_length);

        __v4096i8 sign_b_add_d;
        sign_b_add_d = vadd(temp_sign_larger_0, 0, MASKREAD_OFF, caculation_length);
        sign_b_add_d = vsub(temp_sign_smaller_0, sign_b_add_d, MASKREAD_OFF, caculation_length);

        vbrdcst(temp_sign_larger_0, 0, MASKREAD_OFF, caculation_length);
        vbrdcst(temp_sign_smaller_0, 0, MASKREAD_OFF, caculation_length);

        temp_sign_smaller_0 = vsgt(temp_a, 0, MASKREAD_OFF, MASKWRITE_OFF, caculation_length);
        temp_sign_larger_0  = vslt(temp_a, 0, MASKREAD_OFF, MASKWRITE_OFF, caculation_length);

        __v4096i8 sign_a;
        sign_a = vadd(temp_sign_larger_0, 0, MASKREAD_OFF, caculation_length);
        sign_a = vsub(temp_sign_smaller_0, sign_a, MASKREAD_OFF, caculation_length);

        result_up_temp_a = vmul(sign_a, sign_b_add_d, MASKREAD_OFF, caculation_length);
        result_up_temp   = vmul(result_up_temp_a, alpha, MASKREAD_OFF, caculation_length);

        __v4096i8 abs_b_add_d;
        __v4096i8 abs_a;
        vsgt(b_add_d, 0, MASKREAD_OFF, MASKWRITE_ON, caculation_length);
        abs_b_add_d = vxor(b_add_d, 0xFF, MASKREAD_ON, caculation_length);
        abs_b_add_d = vsadd(abs_b_add_d, 1, MASKREAD_ON, caculation_length);

        vsgt(temp_a, 0, MASKREAD_OFF, MASKWRITE_ON, caculation_length);
        abs_a = vxor(temp_a, 0xFF, MASKREAD_ON, caculation_length);
        abs_a = vsadd(abs_a, 1, MASKREAD_ON, caculation_length);

        __v4096i8 min_bd_a;
        min_bd_a = vmul(abs_a, 0, MASKREAD_OFF, caculation_length); // 冗余
        vsle(abs_b_add_d, abs_a, MASKREAD_OFF, MASKWRITE_ON, caculation_length);
        min_bd_a = vadd(abs_a, min_bd_a, MASKREAD_ON, caculation_length);
        vsgt(abs_b_add_d, abs_a, MASKREAD_OFF, MASKWRITE_ON, caculation_length);
        min_bd_a = vadd(abs_b_add_d, min_bd_a, MASKREAD_ON, MASKWRITE_OFF, caculation_length);

        result_up_temp = MUL4096_8_FIXED(result_up_temp, min_bd_a, 6, caculation_length);

        //  down
        temp_sign_smaller_0 = vsgt(temp_c, 0, MASKREAD_OFF, MASKWRITE_OFF, caculation_length);
        temp_sign_larger_0  = vslt(temp_c, 0, MASKREAD_OFF, MASKWRITE_OFF, caculation_length);

        __v4096i8 sign_c;
        sign_c = vadd(temp_sign_larger_0, 0, MASKREAD_OFF, caculation_length);
        sign_c = vsub(temp_sign_smaller_0, sign_c, MASKREAD_OFF, caculation_length);

        __v4096i8 result_down_temp;
        __v4096i8 result_down_temp_a;
        result_down_temp_a = vmul(sign_a, sign_c, MASKREAD_OFF, caculation_length);
        result_down_temp   = vmul(result_down_temp_a, alpha, MASKREAD_OFF, caculation_length);

        __v4096i8 abs_c;
        vsgt(temp_c, 0, MASKREAD_OFF, MASKWRITE_ON, caculation_length);
        abs_c = vxor(temp_c, 0xFF, MASKREAD_ON, caculation_length);
        abs_c = vsadd(abs_c, 1, MASKREAD_ON, caculation_length);

        __v4096i8 min_a_c;
        min_a_c = vmul(abs_c, 0, MASKREAD_OFF, caculation_length); //  冗余
        vsle(abs_a, abs_c, MASKREAD_OFF, MASKWRITE_ON, caculation_length);
        min_a_c = vadd(abs_c, min_a_c, MASKREAD_ON, MASKWRITE_OFF, caculation_length);
        vsgt(abs_a, abs_c, MASKREAD_OFF, MASKWRITE_ON, caculation_length);
        min_a_c = vadd(abs_a, min_a_c, MASKREAD_ON, MASKWRITE_OFF, caculation_length);

        result_down_temp = MUL4096_8_FIXED(result_down_temp, min_a_c, 6, caculation_length);
        result_down_temp = vsadd(result_down_temp, temp_b, MASKREAD_OFF, caculation_length);

        if (column == 7) {
          vshuffle(L_Matrix_6, copy_index, result_up_temp, SHUFFLE_GATHER, caculation_length);
          vshuffle(L_Matrix_6, move128to64, result_down_temp, SHUFFLE_SCATTER, caculation_length);
        } else if (column == 6) {
          vshuffle(L_Matrix_5, copy_index, result_up_temp, SHUFFLE_GATHER, caculation_length);
          vshuffle(L_Matrix_5, move128to64, result_down_temp, SHUFFLE_SCATTER, caculation_length);
        } else if (column == 5) {
          vshuffle(L_Matrix_4, copy_index, result_up_temp, SHUFFLE_GATHER, caculation_length);
          vshuffle(L_Matrix_4, move128to64, result_down_temp, SHUFFLE_SCATTER, caculation_length);
        } else if (column == 4) {
          vshuffle(L_Matrix_3, copy_index, result_up_temp, SHUFFLE_GATHER, caculation_length);
          vshuffle(L_Matrix_3, move128to64, result_down_temp, SHUFFLE_SCATTER, caculation_length);
        } else if (column == 3) {
          vshuffle(L_Matrix_2, copy_index, result_up_temp, SHUFFLE_GATHER, caculation_length);
          vshuffle(L_Matrix_2, move128to64, result_down_temp, SHUFFLE_SCATTER, caculation_length);
        } else if (column == 2) {
          vshuffle(L_Matrix_1, copy_index, result_up_temp, SHUFFLE_GATHER, caculation_length);
          vshuffle(L_Matrix_1, move128to64, result_down_temp, SHUFFLE_SCATTER, caculation_length);
        } else if (column == 1) {
          vshuffle(L_Matrix_0, copy_index, result_up_temp, SHUFFLE_GATHER, caculation_length);
          vshuffle(L_Matrix_0, move128to64, result_down_temp, SHUFFLE_SCATTER, caculation_length);
        }
      }

      //  R Matrix Calculation
      for (int column = 1; column < 8; column++) {
        if (column == 1) {
          vshuffle(temp_a, Index024, L_Matrix_1, SHUFFLE_GATHER, caculation_length);
          vshuffle(temp_b, Index135, L_Matrix_1, SHUFFLE_GATHER, caculation_length);
          vshuffle(temp_c, copy_index, R_Matrix_0, SHUFFLE_GATHER, caculation_length);
          vshuffle(temp_d, move128to64, R_Matrix_0, SHUFFLE_GATHER, caculation_length);
        } else if (column == 2) {
          vshuffle(temp_a, Index024, L_Matrix_2, SHUFFLE_GATHER, caculation_length);
          vshuffle(temp_b, Index135, L_Matrix_2, SHUFFLE_GATHER, caculation_length);
          vshuffle(temp_c, copy_index, R_Matrix_1, SHUFFLE_GATHER, caculation_length);
          vshuffle(temp_d, move128to64, R_Matrix_1, SHUFFLE_GATHER, caculation_length);
        } else if (column == 3) {
          vshuffle(temp_a, Index024, L_Matrix_3, SHUFFLE_GATHER, caculation_length);
          vshuffle(temp_b, Index135, L_Matrix_3, SHUFFLE_GATHER, caculation_length);
          vshuffle(temp_c, copy_index, R_Matrix_2, SHUFFLE_GATHER, caculation_length);
          vshuffle(temp_d, move128to64, R_Matrix_2, SHUFFLE_GATHER, caculation_length);
        } else if (column == 4) {
          vshuffle(temp_a, Index024, L_Matrix_4, SHUFFLE_GATHER, caculation_length);
          vshuffle(temp_b, Index135, L_Matrix_4, SHUFFLE_GATHER, caculation_length);
          vshuffle(temp_c, copy_index, R_Matrix_3, SHUFFLE_GATHER, caculation_length);
          vshuffle(temp_d, move128to64, R_Matrix_3, SHUFFLE_GATHER, caculation_length);
        } else if (column == 5) {
          vshuffle(temp_a, Index024, L_Matrix_5, SHUFFLE_GATHER, caculation_length);
          vshuffle(temp_b, Index135, L_Matrix_5, SHUFFLE_GATHER, caculation_length);
          vshuffle(temp_c, copy_index, R_Matrix_4, SHUFFLE_GATHER, caculation_length);
          vshuffle(temp_d, move128to64, R_Matrix_4, SHUFFLE_GATHER, caculation_length);
        } else if (column == 6) {
          vshuffle(temp_a, Index024, L_Matrix_6, SHUFFLE_GATHER, caculation_length);
          vshuffle(temp_b, Index135, L_Matrix_6, SHUFFLE_GATHER, caculation_length);
          vshuffle(temp_c, copy_index, R_Matrix_5, SHUFFLE_GATHER, caculation_length);
          vshuffle(temp_d, move128to64, R_Matrix_5, SHUFFLE_GATHER, caculation_length);
        } else if (column == 7) {
          vshuffle(temp_a, Index024, L_Matrix_7, SHUFFLE_GATHER, caculation_length);
          vshuffle(temp_b, Index135, L_Matrix_7, SHUFFLE_GATHER, caculation_length);
          vshuffle(temp_c, copy_index, R_Matrix_6, SHUFFLE_GATHER, caculation_length);
          vshuffle(temp_d, move128to64, R_Matrix_6, SHUFFLE_GATHER, caculation_length);
        }

        __v4096i8 b_add_d;
        b_add_d = vsadd(temp_b, temp_d, MASKREAD_OFF, caculation_length);

        __v4096i8 temp_sign_larger_0;
        __v4096i8 temp_sign_smaller_0;
        temp_sign_smaller_0 = vsgt(b_add_d, 0, MASKREAD_OFF, MASKWRITE_OFF, caculation_length);
        temp_sign_larger_0  = vslt(b_add_d, 0, MASKREAD_OFF, MASKWRITE_OFF, caculation_length);

        __v4096i8 sign_b_add_d;
        sign_b_add_d = vadd(temp_sign_larger_0, 0, MASKREAD_OFF, caculation_length);
        sign_b_add_d = vsub(temp_sign_smaller_0, sign_b_add_d, MASKREAD_OFF, caculation_length);

        vbrdcst(temp_sign_larger_0, 0, MASKREAD_OFF, caculation_length);
        vbrdcst(temp_sign_smaller_0, 0, MASKREAD_OFF, caculation_length);

        temp_sign_smaller_0 = vsgt(temp_c, 0, MASKREAD_OFF, MASKWRITE_OFF, caculation_length);
        temp_sign_larger_0  = vslt(temp_c, 0, MASKREAD_OFF, MASKWRITE_OFF, caculation_length);

        __v4096i8 sign_c;
        sign_c = vadd(temp_sign_larger_0, 0, MASKREAD_OFF, caculation_length);
        sign_c = vsub(temp_sign_smaller_0, sign_c, MASKREAD_OFF, caculation_length);

        result_up_temp_a = vmul(sign_c, sign_b_add_d, MASKREAD_OFF, caculation_length);
        result_up_temp   = vmul(result_up_temp_a, alpha, MASKREAD_OFF, caculation_length);

        __v4096i8 abs_b_add_d;
        __v4096i8 abs_c;
        vsgt(b_add_d, 0, MASKREAD_OFF, MASKWRITE_ON, caculation_length);
        abs_b_add_d = vxor(b_add_d, 0xFF, MASKREAD_ON, caculation_length);
        abs_b_add_d = vsadd(abs_b_add_d, 1, MASKREAD_ON, caculation_length);

        vsgt(temp_c, 0, MASKREAD_OFF, MASKWRITE_ON, caculation_length);
        abs_c = vxor(temp_c, 0xFF, MASKREAD_ON, caculation_length);
        abs_c = vsadd(abs_c, 1, MASKREAD_ON, caculation_length);

        __v4096i8 min_bd_c;
        min_bd_c = vmul(abs_c, 0, MASKREAD_OFF, caculation_length); // 冗余
        vsle(abs_b_add_d, abs_c, MASKREAD_OFF, MASKWRITE_ON, caculation_length);
        min_bd_c = vadd(abs_c, min_bd_c, MASKREAD_ON, caculation_length);
        vsgt(abs_b_add_d, abs_c, MASKREAD_OFF, MASKWRITE_ON, caculation_length);
        min_bd_c = vadd(abs_b_add_d, min_bd_c, MASKREAD_ON, MASKWRITE_OFF, caculation_length);

        result_up_temp = MUL4096_8_FIXED(result_up_temp, min_bd_c, 6, caculation_length);

        //  down
        temp_sign_smaller_0 = vsgt(temp_a, 0, MASKREAD_OFF, MASKWRITE_OFF, caculation_length);
        temp_sign_larger_0  = vslt(temp_a, 0, MASKREAD_OFF, MASKWRITE_OFF, caculation_length);

        __v4096i8 sign_a;
        sign_a = vadd(temp_sign_larger_0, 0, MASKREAD_OFF, caculation_length);
        sign_a = vsub(temp_sign_smaller_0, sign_a, MASKREAD_OFF, caculation_length);

        __v4096i8 result_down_temp;
        __v4096i8 result_down_temp_a;
        result_down_temp_a = vmul(sign_a, sign_c, MASKREAD_OFF, caculation_length);
        result_down_temp   = vmul(result_down_temp_a, alpha, MASKREAD_OFF, caculation_length);

        __v4096i8 abs_a;
        vsgt(temp_a, 0, MASKREAD_OFF, MASKWRITE_ON, caculation_length);
        abs_a = vxor(temp_a, 0xFF, MASKREAD_ON, caculation_length);
        abs_a = vsadd(abs_a, 1, MASKREAD_ON, caculation_length);

        __v4096i8 min_a_c;
        min_a_c = vmul(abs_c, 0, MASKREAD_OFF, caculation_length); //  冗余
        vsle(abs_a, abs_c, MASKREAD_OFF, MASKWRITE_ON, caculation_length);
        min_a_c = vadd(abs_c, min_a_c, MASKREAD_ON, MASKWRITE_OFF, caculation_length);
        vsgt(abs_a, abs_c, MASKREAD_OFF, MASKWRITE_ON, caculation_length);
        min_a_c = vadd(abs_a, min_a_c, MASKREAD_ON, MASKWRITE_OFF, caculation_length);

        result_down_temp = MUL4096_8_FIXED(result_down_temp, min_a_c, 6, caculation_length);
        result_down_temp = vsadd(result_down_temp, temp_d, MASKREAD_OFF, caculation_length);

        if (column == 7) {
          vshuffle(R_Matrix_7, Index024, result_up_temp, SHUFFLE_SCATTER, caculation_length);
          vshuffle(R_Matrix_7, Index135, result_down_temp, SHUFFLE_SCATTER, caculation_length);
        } else if (column == 6) {
          vshuffle(R_Matrix_6, Index024, result_up_temp, SHUFFLE_SCATTER, caculation_length);
          vshuffle(R_Matrix_6, Index135, result_down_temp, SHUFFLE_SCATTER, caculation_length);
        } else if (column == 5) {
          vshuffle(R_Matrix_5, Index024, result_up_temp, SHUFFLE_SCATTER, caculation_length);
          vshuffle(R_Matrix_5, Index135, result_down_temp, SHUFFLE_SCATTER, caculation_length);
        } else if (column == 4) {
          vshuffle(R_Matrix_4, Index024, result_up_temp, SHUFFLE_SCATTER, caculation_length);
          vshuffle(R_Matrix_4, Index135, result_down_temp, SHUFFLE_SCATTER, caculation_length);
        } else if (column == 3) {
          vshuffle(R_Matrix_3, Index024, result_up_temp, SHUFFLE_SCATTER, caculation_length);
          vshuffle(R_Matrix_3, Index135, result_down_temp, SHUFFLE_SCATTER, caculation_length);
        } else if (column == 2) {
          vshuffle(R_Matrix_2, Index024, result_up_temp, SHUFFLE_SCATTER, caculation_length);
          vshuffle(R_Matrix_2, Index135, result_down_temp, SHUFFLE_SCATTER, caculation_length);
        } else if (column == 1) {
          vshuffle(R_Matrix_1, Index024, result_up_temp, SHUFFLE_SCATTER, caculation_length);
          vshuffle(R_Matrix_1, Index135, result_down_temp, SHUFFLE_SCATTER, caculation_length);
        }
      }
    }
  } else if (N == 64) {
    short caculation_length = N / 2;
    // 初始化R、L矩阵，如果向量定义后默认每个向量中的元素都为0，则可以省略
    L_Matrix_0 = vand(L_Matrix_0, 0, MASKREAD_OFF, N);
    L_Matrix_1 = vand(L_Matrix_1, 0, MASKREAD_OFF, N);
    L_Matrix_2 = vand(L_Matrix_2, 0, MASKREAD_OFF, N);
    L_Matrix_3 = vand(L_Matrix_3, 0, MASKREAD_OFF, N);
    L_Matrix_4 = vand(L_Matrix_4, 0, MASKREAD_OFF, N);
    L_Matrix_5 = vand(L_Matrix_5, 0, MASKREAD_OFF, N);

    R_Matrix_1 = vand(R_Matrix_1, 0, MASKREAD_OFF, N);
    R_Matrix_2 = vand(R_Matrix_2, 0, MASKREAD_OFF, N);
    R_Matrix_3 = vand(R_Matrix_3, 0, MASKREAD_OFF, N);
    R_Matrix_4 = vand(R_Matrix_4, 0, MASKREAD_OFF, N);
    R_Matrix_5 = vand(R_Matrix_5, 0, MASKREAD_OFF, N);
    R_Matrix_6 = vand(R_Matrix_6, 0, MASKREAD_OFF, N);

    vbrdcst(R_Matrix_0, 127, MASKREAD_OFF, N);
    vshuffle(R_Matrix_0, InfoIndex, blank_vec, SHUFFLE_SCATTER, K);
    vshuffle(L_Matrix_6, copy_index, in_LLR, SHUFFLE_GATHER, N);

    __v2048i16 move64to32;
    vclaim(move64to32);
    move64to32 = vadd(copy_index, 32, MASKREAD_OFF, 32);

    __v4096i8 result_up_temp;
    vclaim(result_up_temp);
    __v4096i8 result_up_temp_a;
    vclaim(result_up_temp_a);

    //  迭代Polar译码
    for (int iter = 0; iter < iterNum; iter++) {
      //  L Matrix Calculation
      for (int column = 6; column > 0; column--) {
        if (column == 6) {
          vshuffle(temp_a, Index024, L_Matrix_6, SHUFFLE_GATHER, caculation_length);
          vshuffle(temp_b, Index135, L_Matrix_6, SHUFFLE_GATHER, caculation_length);
          vshuffle(temp_c, copy_index, R_Matrix_5, SHUFFLE_GATHER, caculation_length);
          vshuffle(temp_d, move64to32, R_Matrix_5, SHUFFLE_GATHER, caculation_length);
        } else if (column == 5) {
          vshuffle(temp_a, Index024, L_Matrix_5, SHUFFLE_GATHER, caculation_length);
          vshuffle(temp_b, Index135, L_Matrix_5, SHUFFLE_GATHER, caculation_length);
          vshuffle(temp_c, copy_index, R_Matrix_4, SHUFFLE_GATHER, caculation_length);
          vshuffle(temp_d, move64to32, R_Matrix_4, SHUFFLE_GATHER, caculation_length);
        } else if (column == 4) {
          vshuffle(temp_a, Index024, L_Matrix_4, SHUFFLE_GATHER, caculation_length);
          vshuffle(temp_b, Index135, L_Matrix_4, SHUFFLE_GATHER, caculation_length);
          vshuffle(temp_c, copy_index, R_Matrix_3, SHUFFLE_GATHER, caculation_length);
          vshuffle(temp_d, move64to32, R_Matrix_3, SHUFFLE_GATHER, caculation_length);
        } else if (column == 3) {
          vshuffle(temp_a, Index024, L_Matrix_3, SHUFFLE_GATHER, caculation_length);
          vshuffle(temp_b, Index135, L_Matrix_3, SHUFFLE_GATHER, caculation_length);
          vshuffle(temp_c, copy_index, R_Matrix_2, SHUFFLE_GATHER, caculation_length);
          vshuffle(temp_d, move64to32, R_Matrix_2, SHUFFLE_GATHER, caculation_length);
        } else if (column == 2) {
          vshuffle(temp_a, Index024, L_Matrix_2, SHUFFLE_GATHER, caculation_length);
          vshuffle(temp_b, Index135, L_Matrix_2, SHUFFLE_GATHER, caculation_length);
          vshuffle(temp_c, copy_index, R_Matrix_1, SHUFFLE_GATHER, caculation_length);
          vshuffle(temp_d, move64to32, R_Matrix_1, SHUFFLE_GATHER, caculation_length);
        } else if (column == 1) {
          vshuffle(temp_a, Index024, L_Matrix_1, SHUFFLE_GATHER, caculation_length);
          vshuffle(temp_b, Index135, L_Matrix_1, SHUFFLE_GATHER, caculation_length);
          vshuffle(temp_c, copy_index, R_Matrix_0, SHUFFLE_GATHER, caculation_length);
          vshuffle(temp_d, move64to32, R_Matrix_0, SHUFFLE_GATHER, caculation_length);
        }

        __v4096i8 b_add_d;
        b_add_d = vsadd(temp_b, temp_d, MASKREAD_OFF, caculation_length);

        __v4096i8 temp_sign_larger_0;
        __v4096i8 temp_sign_smaller_0;
        temp_sign_smaller_0 = vsgt(b_add_d, 0, MASKREAD_OFF, MASKWRITE_OFF, caculation_length);
        temp_sign_larger_0  = vslt(b_add_d, 0, MASKREAD_OFF, MASKWRITE_OFF, caculation_length);

        __v4096i8 sign_b_add_d;
        sign_b_add_d = vadd(temp_sign_larger_0, 0, MASKREAD_OFF, caculation_length);
        sign_b_add_d = vsub(temp_sign_smaller_0, sign_b_add_d, MASKREAD_OFF, caculation_length);

        vbrdcst(temp_sign_larger_0, 0, MASKREAD_OFF, caculation_length);
        vbrdcst(temp_sign_smaller_0, 0, MASKREAD_OFF, caculation_length);

        temp_sign_smaller_0 = vsgt(temp_a, 0, MASKREAD_OFF, MASKWRITE_OFF, caculation_length);
        temp_sign_larger_0  = vslt(temp_a, 0, MASKREAD_OFF, MASKWRITE_OFF, caculation_length);

        __v4096i8 sign_a;
        sign_a = vadd(temp_sign_larger_0, 0, MASKREAD_OFF, caculation_length);
        sign_a = vsub(temp_sign_smaller_0, sign_a, MASKREAD_OFF, caculation_length);

        result_up_temp_a = vmul(sign_a, sign_b_add_d, MASKREAD_OFF, caculation_length);
        result_up_temp   = vmul(result_up_temp_a, alpha, MASKREAD_OFF, caculation_length);

        __v4096i8 abs_b_add_d;
        __v4096i8 abs_a;
        vsgt(b_add_d, 0, MASKREAD_OFF, MASKWRITE_ON, caculation_length);
        abs_b_add_d = vxor(b_add_d, 0xFF, MASKREAD_ON, caculation_length);
        abs_b_add_d = vsadd(abs_b_add_d, 1, MASKREAD_ON, caculation_length);

        vsgt(temp_a, 0, MASKREAD_OFF, MASKWRITE_ON, caculation_length);
        abs_a = vxor(temp_a, 0xFF, MASKREAD_ON, caculation_length);
        abs_a = vsadd(abs_a, 1, MASKREAD_ON, caculation_length);

        __v4096i8 min_bd_a;
        min_bd_a = vmul(abs_a, 0, MASKREAD_OFF, caculation_length); // 冗余
        vsle(abs_b_add_d, abs_a, MASKREAD_OFF, MASKWRITE_ON, caculation_length);
        min_bd_a = vadd(abs_a, min_bd_a, MASKREAD_ON, caculation_length);
        vsgt(abs_b_add_d, abs_a, MASKREAD_OFF, MASKWRITE_ON, caculation_length);
        min_bd_a = vadd(abs_b_add_d, min_bd_a, MASKREAD_ON, MASKWRITE_OFF, caculation_length);

        result_up_temp = MUL4096_8_FIXED(result_up_temp, min_bd_a, 6, caculation_length);

        //  down
        temp_sign_smaller_0 = vsgt(temp_c, 0, MASKREAD_OFF, MASKWRITE_OFF, caculation_length);
        temp_sign_larger_0  = vslt(temp_c, 0, MASKREAD_OFF, MASKWRITE_OFF, caculation_length);

        __v4096i8 sign_c;
        sign_c = vadd(temp_sign_larger_0, 0, MASKREAD_OFF, caculation_length);
        sign_c = vsub(temp_sign_smaller_0, sign_c, MASKREAD_OFF, caculation_length);

        __v4096i8 result_down_temp;
        __v4096i8 result_down_temp_a;
        result_down_temp_a = vmul(sign_a, sign_c, MASKREAD_OFF, caculation_length);
        result_down_temp   = vmul(result_down_temp_a, alpha, MASKREAD_OFF, caculation_length);

        __v4096i8 abs_c;
        vsgt(temp_c, 0, MASKREAD_OFF, MASKWRITE_ON, caculation_length);
        abs_c = vxor(temp_c, 0xFF, MASKREAD_ON, caculation_length);
        abs_c = vsadd(abs_c, 1, MASKREAD_ON, caculation_length);

        __v4096i8 min_a_c;
        min_a_c = vmul(abs_c, 0, MASKREAD_OFF, caculation_length); //  冗余
        vsle(abs_a, abs_c, MASKREAD_OFF, MASKWRITE_ON, caculation_length);
        min_a_c = vadd(abs_c, min_a_c, MASKREAD_ON, MASKWRITE_OFF, caculation_length);
        vsgt(abs_a, abs_c, MASKREAD_OFF, MASKWRITE_ON, caculation_length);
        min_a_c = vadd(abs_a, min_a_c, MASKREAD_ON, MASKWRITE_OFF, caculation_length);

        result_down_temp = MUL4096_8_FIXED(result_down_temp, min_a_c, 6, caculation_length);
        result_down_temp = vsadd(result_down_temp, temp_b, MASKREAD_OFF, caculation_length);

        if (column == 6) {
          vshuffle(L_Matrix_5, copy_index, result_up_temp, SHUFFLE_GATHER, caculation_length);
          vshuffle(L_Matrix_5, move64to32, result_down_temp, SHUFFLE_SCATTER, caculation_length);
        } else if (column == 5) {
          vshuffle(L_Matrix_4, copy_index, result_up_temp, SHUFFLE_GATHER, caculation_length);
          vshuffle(L_Matrix_4, move64to32, result_down_temp, SHUFFLE_SCATTER, caculation_length);
        } else if (column == 4) {
          vshuffle(L_Matrix_3, copy_index, result_up_temp, SHUFFLE_GATHER, caculation_length);
          vshuffle(L_Matrix_3, move64to32, result_down_temp, SHUFFLE_SCATTER, caculation_length);
        } else if (column == 3) {
          vshuffle(L_Matrix_2, copy_index, result_up_temp, SHUFFLE_GATHER, caculation_length);
          vshuffle(L_Matrix_2, move64to32, result_down_temp, SHUFFLE_SCATTER, caculation_length);
        } else if (column == 2) {
          vshuffle(L_Matrix_1, copy_index, result_up_temp, SHUFFLE_GATHER, caculation_length);
          vshuffle(L_Matrix_1, move64to32, result_down_temp, SHUFFLE_SCATTER, caculation_length);
        } else if (column == 1) {
          vshuffle(L_Matrix_0, copy_index, result_up_temp, SHUFFLE_GATHER, caculation_length);
          vshuffle(L_Matrix_0, move64to32, result_down_temp, SHUFFLE_SCATTER, caculation_length);
        }
      }

      //  R Matrix Calculation
      for (int column = 1; column < 7; column++) {
        if (column == 1) {
          vshuffle(temp_a, Index024, L_Matrix_1, SHUFFLE_GATHER, caculation_length);
          vshuffle(temp_b, Index135, L_Matrix_1, SHUFFLE_GATHER, caculation_length);
          vshuffle(temp_c, copy_index, R_Matrix_0, SHUFFLE_GATHER, caculation_length);
          vshuffle(temp_d, move64to32, R_Matrix_0, SHUFFLE_GATHER, caculation_length);
        } else if (column == 2) {
          vshuffle(temp_a, Index024, L_Matrix_2, SHUFFLE_GATHER, caculation_length);
          vshuffle(temp_b, Index135, L_Matrix_2, SHUFFLE_GATHER, caculation_length);
          vshuffle(temp_c, copy_index, R_Matrix_1, SHUFFLE_GATHER, caculation_length);
          vshuffle(temp_d, move64to32, R_Matrix_1, SHUFFLE_GATHER, caculation_length);
        } else if (column == 3) {
          vshuffle(temp_a, Index024, L_Matrix_3, SHUFFLE_GATHER, caculation_length);
          vshuffle(temp_b, Index135, L_Matrix_3, SHUFFLE_GATHER, caculation_length);
          vshuffle(temp_c, copy_index, R_Matrix_2, SHUFFLE_GATHER, caculation_length);
          vshuffle(temp_d, move64to32, R_Matrix_2, SHUFFLE_GATHER, caculation_length);
        } else if (column == 4) {
          vshuffle(temp_a, Index024, L_Matrix_4, SHUFFLE_GATHER, caculation_length);
          vshuffle(temp_b, Index135, L_Matrix_4, SHUFFLE_GATHER, caculation_length);
          vshuffle(temp_c, copy_index, R_Matrix_3, SHUFFLE_GATHER, caculation_length);
          vshuffle(temp_d, move64to32, R_Matrix_3, SHUFFLE_GATHER, caculation_length);
        } else if (column == 5) {
          vshuffle(temp_a, Index024, L_Matrix_5, SHUFFLE_GATHER, caculation_length);
          vshuffle(temp_b, Index135, L_Matrix_5, SHUFFLE_GATHER, caculation_length);
          vshuffle(temp_c, copy_index, R_Matrix_4, SHUFFLE_GATHER, caculation_length);
          vshuffle(temp_d, move64to32, R_Matrix_4, SHUFFLE_GATHER, caculation_length);
        } else if (column == 6) {
          vshuffle(temp_a, Index024, L_Matrix_6, SHUFFLE_GATHER, caculation_length);
          vshuffle(temp_b, Index135, L_Matrix_6, SHUFFLE_GATHER, caculation_length);
          vshuffle(temp_c, copy_index, R_Matrix_5, SHUFFLE_GATHER, caculation_length);
          vshuffle(temp_d, move64to32, R_Matrix_5, SHUFFLE_GATHER, caculation_length);
        }

        __v4096i8 b_add_d;
        b_add_d = vsadd(temp_b, temp_d, MASKREAD_OFF, caculation_length);

        __v4096i8 temp_sign_larger_0;
        __v4096i8 temp_sign_smaller_0;
        temp_sign_smaller_0 = vsgt(b_add_d, 0, MASKREAD_OFF, MASKWRITE_OFF, caculation_length);
        temp_sign_larger_0  = vslt(b_add_d, 0, MASKREAD_OFF, MASKWRITE_OFF, caculation_length);

        __v4096i8 sign_b_add_d;
        sign_b_add_d = vadd(temp_sign_larger_0, 0, MASKREAD_OFF, caculation_length);
        sign_b_add_d = vsub(temp_sign_smaller_0, sign_b_add_d, MASKREAD_OFF, caculation_length);

        vbrdcst(temp_sign_larger_0, 0, MASKREAD_OFF, caculation_length);
        vbrdcst(temp_sign_smaller_0, 0, MASKREAD_OFF, caculation_length);

        temp_sign_smaller_0 = vsgt(temp_c, 0, MASKREAD_OFF, MASKWRITE_OFF, caculation_length);
        temp_sign_larger_0  = vslt(temp_c, 0, MASKREAD_OFF, MASKWRITE_OFF, caculation_length);

        __v4096i8 sign_c;
        sign_c = vadd(temp_sign_larger_0, 0, MASKREAD_OFF, caculation_length);
        sign_c = vsub(temp_sign_smaller_0, sign_c, MASKREAD_OFF, caculation_length);

        result_up_temp_a = vmul(sign_c, sign_b_add_d, MASKREAD_OFF, caculation_length);
        result_up_temp   = vmul(result_up_temp_a, alpha, MASKREAD_OFF, caculation_length);

        __v4096i8 abs_b_add_d;
        __v4096i8 abs_c;
        vsgt(b_add_d, 0, MASKREAD_OFF, MASKWRITE_ON, caculation_length);
        abs_b_add_d = vxor(b_add_d, 0xFF, MASKREAD_ON, caculation_length);
        abs_b_add_d = vsadd(abs_b_add_d, 1, MASKREAD_ON, caculation_length);

        vsgt(temp_c, 0, MASKREAD_OFF, MASKWRITE_ON, caculation_length);
        abs_c = vxor(temp_c, 0xFF, MASKREAD_ON, caculation_length);
        abs_c = vsadd(abs_c, 1, MASKREAD_ON, caculation_length);

        __v4096i8 min_bd_c;
        min_bd_c = vmul(abs_c, 0, MASKREAD_OFF, caculation_length); // 冗余
        vsle(abs_b_add_d, abs_c, MASKREAD_OFF, MASKWRITE_ON, caculation_length);
        min_bd_c = vadd(abs_c, min_bd_c, MASKREAD_ON, caculation_length);
        vsgt(abs_b_add_d, abs_c, MASKREAD_OFF, MASKWRITE_ON, caculation_length);
        min_bd_c = vadd(abs_b_add_d, min_bd_c, MASKREAD_ON, MASKWRITE_OFF, caculation_length);

        result_up_temp = MUL4096_8_FIXED(result_up_temp, min_bd_c, 6, caculation_length);

        //  down
        temp_sign_smaller_0 = vsgt(temp_a, 0, MASKREAD_OFF, MASKWRITE_OFF, caculation_length);
        temp_sign_larger_0  = vslt(temp_a, 0, MASKREAD_OFF, MASKWRITE_OFF, caculation_length);

        __v4096i8 sign_a;
        sign_a = vadd(temp_sign_larger_0, 0, MASKREAD_OFF, caculation_length);
        sign_a = vsub(temp_sign_smaller_0, sign_a, MASKREAD_OFF, caculation_length);

        __v4096i8 result_down_temp;
        __v4096i8 result_down_temp_a;
        result_down_temp_a = vmul(sign_a, sign_c, MASKREAD_OFF, caculation_length);
        result_down_temp   = vmul(result_down_temp_a, alpha, MASKREAD_OFF, caculation_length);

        __v4096i8 abs_a;
        vsgt(temp_a, 0, MASKREAD_OFF, MASKWRITE_ON, caculation_length);
        abs_a = vxor(temp_a, 0xFF, MASKREAD_ON, caculation_length);
        abs_a = vsadd(abs_a, 1, MASKREAD_ON, caculation_length);

        __v4096i8 min_a_c;
        min_a_c = vmul(abs_c, 0, MASKREAD_OFF, caculation_length); //  冗余
        vsle(abs_a, abs_c, MASKREAD_OFF, MASKWRITE_ON, caculation_length);
        min_a_c = vadd(abs_c, min_a_c, MASKREAD_ON, MASKWRITE_OFF, caculation_length);
        vsgt(abs_a, abs_c, MASKREAD_OFF, MASKWRITE_ON, caculation_length);
        min_a_c = vadd(abs_a, min_a_c, MASKREAD_ON, MASKWRITE_OFF, caculation_length);

        result_down_temp = MUL4096_8_FIXED(result_down_temp, min_a_c, 6, caculation_length);
        result_down_temp = vsadd(result_down_temp, temp_d, MASKREAD_OFF, caculation_length);

        if (column == 6) {
          vshuffle(R_Matrix_6, Index024, result_up_temp, SHUFFLE_SCATTER, caculation_length);
          vshuffle(R_Matrix_6, Index135, result_down_temp, SHUFFLE_SCATTER, caculation_length);
        } else if (column == 5) {
          vshuffle(R_Matrix_5, Index024, result_up_temp, SHUFFLE_SCATTER, caculation_length);
          vshuffle(R_Matrix_5, Index135, result_down_temp, SHUFFLE_SCATTER, caculation_length);
        } else if (column == 4) {
          vshuffle(R_Matrix_4, Index024, result_up_temp, SHUFFLE_SCATTER, caculation_length);
          vshuffle(R_Matrix_4, Index135, result_down_temp, SHUFFLE_SCATTER, caculation_length);
        } else if (column == 3) {
          vshuffle(R_Matrix_3, Index024, result_up_temp, SHUFFLE_SCATTER, caculation_length);
          vshuffle(R_Matrix_3, Index135, result_down_temp, SHUFFLE_SCATTER, caculation_length);
        } else if (column == 2) {
          vshuffle(R_Matrix_2, Index024, result_up_temp, SHUFFLE_SCATTER, caculation_length);
          vshuffle(R_Matrix_2, Index135, result_down_temp, SHUFFLE_SCATTER, caculation_length);
        } else if (column == 1) {
          vshuffle(R_Matrix_1, Index024, result_up_temp, SHUFFLE_SCATTER, caculation_length);
          vshuffle(R_Matrix_1, Index135, result_down_temp, SHUFFLE_SCATTER, caculation_length);
        }
      }
    }
  } else if (N == 32) {
    short caculation_length = N / 2;
    // 初始化R、L矩阵，如果向量定义后默认每个向量中的元素都为0，则可以省略
    L_Matrix_0 = vand(L_Matrix_0, 0, MASKREAD_OFF, N);
    L_Matrix_1 = vand(L_Matrix_1, 0, MASKREAD_OFF, N);
    L_Matrix_2 = vand(L_Matrix_2, 0, MASKREAD_OFF, N);
    L_Matrix_3 = vand(L_Matrix_3, 0, MASKREAD_OFF, N);
    L_Matrix_4 = vand(L_Matrix_4, 0, MASKREAD_OFF, N);
    L_Matrix_5 = vand(L_Matrix_5, 0, MASKREAD_OFF, N);

    R_Matrix_1 = vand(R_Matrix_1, 0, MASKREAD_OFF, N);
    R_Matrix_2 = vand(R_Matrix_2, 0, MASKREAD_OFF, N);
    R_Matrix_3 = vand(R_Matrix_3, 0, MASKREAD_OFF, N);
    R_Matrix_4 = vand(R_Matrix_4, 0, MASKREAD_OFF, N);
    R_Matrix_5 = vand(R_Matrix_5, 0, MASKREAD_OFF, N);
    R_Matrix_6 = vand(R_Matrix_6, 0, MASKREAD_OFF, N);

    vbrdcst(R_Matrix_0, 127, MASKREAD_OFF, N);
    vshuffle(R_Matrix_0, InfoIndex, blank_vec, SHUFFLE_SCATTER, K);
    vshuffle(L_Matrix_6, copy_index, in_LLR, SHUFFLE_GATHER, N);

    __v2048i16 move32to16;
    vclaim(move32to16);
    move32to16 = vadd(copy_index, 16, MASKREAD_OFF, 16);

    __v4096i8 result_up_temp;
    vclaim(result_up_temp);
    __v4096i8 result_up_temp_a;
    vclaim(result_up_temp_a);

    //  迭代Polar译码
    for (int iter = 0; iter < iterNum; iter++) {
      //  L Matrix Calculation
      for (int column = 5; column > 0; column--) {
        if (column == 5) {
          vshuffle(temp_a, Index024, L_Matrix_5, SHUFFLE_GATHER, caculation_length);
          vshuffle(temp_b, Index135, L_Matrix_5, SHUFFLE_GATHER, caculation_length);
          vshuffle(temp_c, copy_index, R_Matrix_4, SHUFFLE_GATHER, caculation_length);
          vshuffle(temp_d, move32to16, R_Matrix_4, SHUFFLE_GATHER, caculation_length);
        } else if (column == 4) {
          vshuffle(temp_a, Index024, L_Matrix_4, SHUFFLE_GATHER, caculation_length);
          vshuffle(temp_b, Index135, L_Matrix_4, SHUFFLE_GATHER, caculation_length);
          vshuffle(temp_c, copy_index, R_Matrix_3, SHUFFLE_GATHER, caculation_length);
          vshuffle(temp_d, move32to16, R_Matrix_3, SHUFFLE_GATHER, caculation_length);
        } else if (column == 3) {
          vshuffle(temp_a, Index024, L_Matrix_3, SHUFFLE_GATHER, caculation_length);
          vshuffle(temp_b, Index135, L_Matrix_3, SHUFFLE_GATHER, caculation_length);
          vshuffle(temp_c, copy_index, R_Matrix_2, SHUFFLE_GATHER, caculation_length);
          vshuffle(temp_d, move32to16, R_Matrix_2, SHUFFLE_GATHER, caculation_length);
        } else if (column == 2) {
          vshuffle(temp_a, Index024, L_Matrix_2, SHUFFLE_GATHER, caculation_length);
          vshuffle(temp_b, Index135, L_Matrix_2, SHUFFLE_GATHER, caculation_length);
          vshuffle(temp_c, copy_index, R_Matrix_1, SHUFFLE_GATHER, caculation_length);
          vshuffle(temp_d, move32to16, R_Matrix_1, SHUFFLE_GATHER, caculation_length);
        } else if (column == 1) {
          vshuffle(temp_a, Index024, L_Matrix_1, SHUFFLE_GATHER, caculation_length);
          vshuffle(temp_b, Index135, L_Matrix_1, SHUFFLE_GATHER, caculation_length);
          vshuffle(temp_c, copy_index, R_Matrix_0, SHUFFLE_GATHER, caculation_length);
          vshuffle(temp_d, move32to16, R_Matrix_0, SHUFFLE_GATHER, caculation_length);
        }

        __v4096i8 b_add_d;
        b_add_d = vsadd(temp_b, temp_d, MASKREAD_OFF, caculation_length);

        __v4096i8 temp_sign_larger_0;
        __v4096i8 temp_sign_smaller_0;
        temp_sign_smaller_0 = vsgt(b_add_d, 0, MASKREAD_OFF, MASKWRITE_OFF, caculation_length);
        temp_sign_larger_0  = vslt(b_add_d, 0, MASKREAD_OFF, MASKWRITE_OFF, caculation_length);

        __v4096i8 sign_b_add_d;
        sign_b_add_d = vadd(temp_sign_larger_0, 0, MASKREAD_OFF, caculation_length);
        sign_b_add_d = vsub(temp_sign_smaller_0, sign_b_add_d, MASKREAD_OFF, caculation_length);

        vbrdcst(temp_sign_larger_0, 0, MASKREAD_OFF, caculation_length);
        vbrdcst(temp_sign_smaller_0, 0, MASKREAD_OFF, caculation_length);

        temp_sign_smaller_0 = vsgt(temp_a, 0, MASKREAD_OFF, MASKWRITE_OFF, caculation_length);
        temp_sign_larger_0  = vslt(temp_a, 0, MASKREAD_OFF, MASKWRITE_OFF, caculation_length);

        __v4096i8 sign_a;
        sign_a = vadd(temp_sign_larger_0, 0, MASKREAD_OFF, caculation_length);
        sign_a = vsub(temp_sign_smaller_0, sign_a, MASKREAD_OFF, caculation_length);

        result_up_temp_a = vmul(sign_a, sign_b_add_d, MASKREAD_OFF, caculation_length);
        result_up_temp   = vmul(result_up_temp_a, alpha, MASKREAD_OFF, caculation_length);

        __v4096i8 abs_b_add_d;
        __v4096i8 abs_a;
        vsgt(b_add_d, 0, MASKREAD_OFF, MASKWRITE_ON, caculation_length);
        abs_b_add_d = vxor(b_add_d, 0xFF, MASKREAD_ON, caculation_length);
        abs_b_add_d = vsadd(abs_b_add_d, 1, MASKREAD_ON, caculation_length);

        vsgt(temp_a, 0, MASKREAD_OFF, MASKWRITE_ON, caculation_length);
        abs_a = vxor(temp_a, 0xFF, MASKREAD_ON, caculation_length);
        abs_a = vsadd(abs_a, 1, MASKREAD_ON, caculation_length);

        __v4096i8 min_bd_a;
        min_bd_a = vmul(abs_a, 0, MASKREAD_OFF, caculation_length); // 冗余
        vsle(abs_b_add_d, abs_a, MASKREAD_OFF, MASKWRITE_ON, caculation_length);
        min_bd_a = vadd(abs_a, min_bd_a, MASKREAD_ON, caculation_length);
        vsgt(abs_b_add_d, abs_a, MASKREAD_OFF, MASKWRITE_ON, caculation_length);
        min_bd_a = vadd(abs_b_add_d, min_bd_a, MASKREAD_ON, MASKWRITE_OFF, caculation_length);

        result_up_temp = MUL4096_8_FIXED(result_up_temp, min_bd_a, 6, caculation_length);

        //  down
        temp_sign_smaller_0 = vsgt(temp_c, 0, MASKREAD_OFF, MASKWRITE_OFF, caculation_length);
        temp_sign_larger_0  = vslt(temp_c, 0, MASKREAD_OFF, MASKWRITE_OFF, caculation_length);

        __v4096i8 sign_c;
        sign_c = vadd(temp_sign_larger_0, 0, MASKREAD_OFF, caculation_length);
        sign_c = vsub(temp_sign_smaller_0, sign_c, MASKREAD_OFF, caculation_length);

        __v4096i8 result_down_temp;
        __v4096i8 result_down_temp_a;
        result_down_temp_a = vmul(sign_a, sign_c, MASKREAD_OFF, caculation_length);
        result_down_temp   = vmul(result_down_temp_a, alpha, MASKREAD_OFF, caculation_length);

        __v4096i8 abs_c;
        vsgt(temp_c, 0, MASKREAD_OFF, MASKWRITE_ON, caculation_length);
        abs_c = vxor(temp_c, 0xFF, MASKREAD_ON, caculation_length);
        abs_c = vsadd(abs_c, 1, MASKREAD_ON, caculation_length);

        __v4096i8 min_a_c;
        min_a_c = vmul(abs_c, 0, MASKREAD_OFF, caculation_length); //  冗余
        vsle(abs_a, abs_c, MASKREAD_OFF, MASKWRITE_ON, caculation_length);
        min_a_c = vadd(abs_c, min_a_c, MASKREAD_ON, MASKWRITE_OFF, caculation_length);
        vsgt(abs_a, abs_c, MASKREAD_OFF, MASKWRITE_ON, caculation_length);
        min_a_c = vadd(abs_a, min_a_c, MASKREAD_ON, MASKWRITE_OFF, caculation_length);

        result_down_temp = MUL4096_8_FIXED(result_down_temp, min_a_c, 6, caculation_length);
        result_down_temp = vsadd(result_down_temp, temp_b, MASKREAD_OFF, caculation_length);

        if (column == 5) {
          vshuffle(L_Matrix_4, copy_index, result_up_temp, SHUFFLE_GATHER, caculation_length);
          vshuffle(L_Matrix_4, move32to16, result_down_temp, SHUFFLE_SCATTER, caculation_length);
        } else if (column == 4) {
          vshuffle(L_Matrix_3, copy_index, result_up_temp, SHUFFLE_GATHER, caculation_length);
          vshuffle(L_Matrix_3, move32to16, result_down_temp, SHUFFLE_SCATTER, caculation_length);
        } else if (column == 3) {
          vshuffle(L_Matrix_2, copy_index, result_up_temp, SHUFFLE_GATHER, caculation_length);
          vshuffle(L_Matrix_2, move32to16, result_down_temp, SHUFFLE_SCATTER, caculation_length);
        } else if (column == 2) {
          vshuffle(L_Matrix_1, copy_index, result_up_temp, SHUFFLE_GATHER, caculation_length);
          vshuffle(L_Matrix_1, move32to16, result_down_temp, SHUFFLE_SCATTER, caculation_length);
        } else if (column == 1) {
          vshuffle(L_Matrix_0, copy_index, result_up_temp, SHUFFLE_GATHER, caculation_length);
          vshuffle(L_Matrix_0, move32to16, result_down_temp, SHUFFLE_SCATTER, caculation_length);
        }
      }

      //  R Matrix Calculation
      for (int column = 1; column < 6; column++) {
        if (column == 1) {
          vshuffle(temp_a, Index024, L_Matrix_1, SHUFFLE_GATHER, caculation_length);
          vshuffle(temp_b, Index135, L_Matrix_1, SHUFFLE_GATHER, caculation_length);
          vshuffle(temp_c, copy_index, R_Matrix_0, SHUFFLE_GATHER, caculation_length);
          vshuffle(temp_d, move32to16, R_Matrix_0, SHUFFLE_GATHER, caculation_length);
        } else if (column == 2) {
          vshuffle(temp_a, Index024, L_Matrix_2, SHUFFLE_GATHER, caculation_length);
          vshuffle(temp_b, Index135, L_Matrix_2, SHUFFLE_GATHER, caculation_length);
          vshuffle(temp_c, copy_index, R_Matrix_1, SHUFFLE_GATHER, caculation_length);
          vshuffle(temp_d, move32to16, R_Matrix_1, SHUFFLE_GATHER, caculation_length);
        } else if (column == 3) {
          vshuffle(temp_a, Index024, L_Matrix_3, SHUFFLE_GATHER, caculation_length);
          vshuffle(temp_b, Index135, L_Matrix_3, SHUFFLE_GATHER, caculation_length);
          vshuffle(temp_c, copy_index, R_Matrix_2, SHUFFLE_GATHER, caculation_length);
          vshuffle(temp_d, move32to16, R_Matrix_2, SHUFFLE_GATHER, caculation_length);
        } else if (column == 4) {
          vshuffle(temp_a, Index024, L_Matrix_4, SHUFFLE_GATHER, caculation_length);
          vshuffle(temp_b, Index135, L_Matrix_4, SHUFFLE_GATHER, caculation_length);
          vshuffle(temp_c, copy_index, R_Matrix_3, SHUFFLE_GATHER, caculation_length);
          vshuffle(temp_d, move32to16, R_Matrix_3, SHUFFLE_GATHER, caculation_length);
        } else if (column == 5) {
          vshuffle(temp_a, Index024, L_Matrix_5, SHUFFLE_GATHER, caculation_length);
          vshuffle(temp_b, Index135, L_Matrix_5, SHUFFLE_GATHER, caculation_length);
          vshuffle(temp_c, copy_index, R_Matrix_4, SHUFFLE_GATHER, caculation_length);
          vshuffle(temp_d, move32to16, R_Matrix_4, SHUFFLE_GATHER, caculation_length);
        }
        __v4096i8 b_add_d;
        b_add_d = vsadd(temp_b, temp_d, MASKREAD_OFF, caculation_length);

        __v4096i8 temp_sign_larger_0;
        __v4096i8 temp_sign_smaller_0;
        temp_sign_smaller_0 = vsgt(b_add_d, 0, MASKREAD_OFF, MASKWRITE_OFF, caculation_length);
        temp_sign_larger_0  = vslt(b_add_d, 0, MASKREAD_OFF, MASKWRITE_OFF, caculation_length);

        __v4096i8 sign_b_add_d;
        sign_b_add_d = vadd(temp_sign_larger_0, 0, MASKREAD_OFF, caculation_length);
        sign_b_add_d = vsub(temp_sign_smaller_0, sign_b_add_d, MASKREAD_OFF, caculation_length);

        vbrdcst(temp_sign_larger_0, 0, MASKREAD_OFF, caculation_length);
        vbrdcst(temp_sign_smaller_0, 0, MASKREAD_OFF, caculation_length);

        temp_sign_smaller_0 = vsgt(temp_c, 0, MASKREAD_OFF, MASKWRITE_OFF, caculation_length);
        temp_sign_larger_0  = vslt(temp_c, 0, MASKREAD_OFF, MASKWRITE_OFF, caculation_length);

        __v4096i8 sign_c;
        sign_c = vadd(temp_sign_larger_0, 0, MASKREAD_OFF, caculation_length);
        sign_c = vsub(temp_sign_smaller_0, sign_c, MASKREAD_OFF, caculation_length);

        result_up_temp_a = vmul(sign_c, sign_b_add_d, MASKREAD_OFF, caculation_length);
        result_up_temp   = vmul(result_up_temp_a, alpha, MASKREAD_OFF, caculation_length);

        __v4096i8 abs_b_add_d;
        __v4096i8 abs_c;
        vsgt(b_add_d, 0, MASKREAD_OFF, MASKWRITE_ON, caculation_length);
        abs_b_add_d = vxor(b_add_d, 0xFF, MASKREAD_ON, caculation_length);
        abs_b_add_d = vsadd(abs_b_add_d, 1, MASKREAD_ON, caculation_length);

        vsgt(temp_c, 0, MASKREAD_OFF, MASKWRITE_ON, caculation_length);
        abs_c = vxor(temp_c, 0xFF, MASKREAD_ON, caculation_length);
        abs_c = vsadd(abs_c, 1, MASKREAD_ON, caculation_length);

        __v4096i8 min_bd_c;
        min_bd_c = vmul(abs_c, 0, MASKREAD_OFF, caculation_length); // 冗余
        vsle(abs_b_add_d, abs_c, MASKREAD_OFF, MASKWRITE_ON, caculation_length);
        min_bd_c = vadd(abs_c, min_bd_c, MASKREAD_ON, caculation_length);
        vsgt(abs_b_add_d, abs_c, MASKREAD_OFF, MASKWRITE_ON, caculation_length);
        min_bd_c = vadd(abs_b_add_d, min_bd_c, MASKREAD_ON, MASKWRITE_OFF, caculation_length);

        result_up_temp = MUL4096_8_FIXED(result_up_temp, min_bd_c, 6, caculation_length);

        //  down
        temp_sign_smaller_0 = vsgt(temp_a, 0, MASKREAD_OFF, MASKWRITE_OFF, caculation_length);
        temp_sign_larger_0  = vslt(temp_a, 0, MASKREAD_OFF, MASKWRITE_OFF, caculation_length);

        __v4096i8 sign_a;
        sign_a = vadd(temp_sign_larger_0, 0, MASKREAD_OFF, caculation_length);
        sign_a = vsub(temp_sign_smaller_0, sign_a, MASKREAD_OFF, caculation_length);

        __v4096i8 result_down_temp;
        __v4096i8 result_down_temp_a;
        result_down_temp_a = vmul(sign_a, sign_c, MASKREAD_OFF, caculation_length);
        result_down_temp   = vmul(result_down_temp_a, alpha, MASKREAD_OFF, caculation_length);

        __v4096i8 abs_a;
        vsgt(temp_a, 0, MASKREAD_OFF, MASKWRITE_ON, caculation_length);
        abs_a = vxor(temp_a, 0xFF, MASKREAD_ON, caculation_length);
        abs_a = vsadd(abs_a, 1, MASKREAD_ON, caculation_length);

        __v4096i8 min_a_c;
        min_a_c = vmul(abs_c, 0, MASKREAD_OFF, caculation_length); //  冗余
        vsle(abs_a, abs_c, MASKREAD_OFF, MASKWRITE_ON, caculation_length);
        min_a_c = vadd(abs_c, min_a_c, MASKREAD_ON, MASKWRITE_OFF, caculation_length);
        vsgt(abs_a, abs_c, MASKREAD_OFF, MASKWRITE_ON, caculation_length);
        min_a_c = vadd(abs_a, min_a_c, MASKREAD_ON, MASKWRITE_OFF, caculation_length);

        result_down_temp = MUL4096_8_FIXED(result_down_temp, min_a_c, 6, caculation_length);
        result_down_temp = vsadd(result_down_temp, temp_d, MASKREAD_OFF, caculation_length);

        if (column == 5) {
          vshuffle(R_Matrix_5, Index024, result_up_temp, SHUFFLE_SCATTER, caculation_length);
          vshuffle(R_Matrix_5, Index135, result_down_temp, SHUFFLE_SCATTER, caculation_length);
        } else if (column == 4) {
          vshuffle(R_Matrix_4, Index024, result_up_temp, SHUFFLE_SCATTER, caculation_length);
          vshuffle(R_Matrix_4, Index135, result_down_temp, SHUFFLE_SCATTER, caculation_length);
        } else if (column == 3) {
          vshuffle(R_Matrix_3, Index024, result_up_temp, SHUFFLE_SCATTER, caculation_length);
          vshuffle(R_Matrix_3, Index135, result_down_temp, SHUFFLE_SCATTER, caculation_length);
        } else if (column == 2) {
          vshuffle(R_Matrix_2, Index024, result_up_temp, SHUFFLE_SCATTER, caculation_length);
          vshuffle(R_Matrix_2, Index135, result_down_temp, SHUFFLE_SCATTER, caculation_length);
        } else if (column == 1) {
          vshuffle(R_Matrix_1, Index024, result_up_temp, SHUFFLE_SCATTER, caculation_length);
          vshuffle(R_Matrix_1, Index135, result_down_temp, SHUFFLE_SCATTER, caculation_length);
        }
      }
    }
  } else {
    // printf("\n\nERROR!Only 32-512 Length Polar Decoder is enable:%hd\n\n",
    // &N);
  }

  __v4096i8 final_llr;
  vclaim(final_llr);
  final_llr = vsadd(L_Matrix_0, R_Matrix_0, MASKREAD_OFF, N);

  __v4096i8 final_bit;
  final_bit = vsgt(final_llr, 0, MASKREAD_OFF, MASKWRITE_OFF, N);

  __v2048i16 Interleaving_Index;
  vclaim(Interleaving_Index);
  Interleaving_Index = nr_polar_interleaving_pattern(K, 1);
  __v2048i16 mask_0to255_16bit;
  vclaim(mask_0to255_16bit);
  vbrdcst(mask_0to255_16bit, 0, MASKREAD_OFF, N);
  vbrdcst(mask_0to255_16bit, 0xFFFF, MASKREAD_OFF, K);
  Interleaving_Index = vand(Interleaving_Index, mask_0to255_16bit, MASKREAD_OFF, N);
  InfoIndex          = vand(InfoIndex, mask_0to255_16bit, MASKREAD_OFF, N);

  __v4096i8 PDCCH_bits_out_temp;
  __v4096i8 PDCCH_bits_out;
  vclaim(PDCCH_bits_out_temp);
  vclaim(PDCCH_bits_out);
  vshuffle(PDCCH_bits_out_temp, InfoIndex, final_bit, SHUFFLE_GATHER, K);
  vshuffle(PDCCH_bits_out, Interleaving_Index, PDCCH_bits_out_temp, SHUFFLE_SCATTER, K);

  __v4096i8 mask_0to255_8bit;
  vbrdcst(mask_0to255_8bit, 0, MASKREAD_OFF, N);
  vbrdcst(mask_0to255_8bit, 0xFF, MASKREAD_OFF, K);
  PDCCH_bits_out = vand(PDCCH_bits_out, mask_0to255_8bit, MASKREAD_OFF, N);

  // printf("\n\ndebug_out:%hd\n\n", &N);
  vreturn(PDCCH_bits_out, sizeof(PDCCH_bits_out));
}