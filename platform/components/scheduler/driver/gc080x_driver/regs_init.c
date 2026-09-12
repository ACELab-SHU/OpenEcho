#include "regs_init.h"

/* scripts update date: 2023/4/20 17:25 */

/****************************************************************************************/
// old x4 setting
/****************************************************************************************/
const reg_t G_REG_ARRAY_ANA_7B_MAN_X4_EN[] = 
{
	////CALPLL lock state check from test
	{FLG_WRITE, 0xD04, 0x4C},
	//
	////X4 calibration config:
	{FLG_WRITE, 0xD08, 0x39},
	{FLG_WRITE, 0xD05, 0xE3},
	{FLG_WRITE, 0xD06, 0xBC},
	{FLG_WRITE, 0xD13, 0x96},
	{FLG_WRITE, 0xD13, 0xD6},
	{FLG_WRITE, 0xD08, 0xB9},
	{FLG_WRITE, 0xD08, 0x39},
	//
	//
	////For better PN
	{FLG_WRITE, 0xD00, 0xB6},
	{FLG_WRITE, 0xD03, 0x8A},
	//
	////Calibration state check:
	//#wait  500
	{FLG_DELAY, 0x5, 0x0},
	{FLG_READ, 0xD3E, 0x0},
	{FLG_READ, 0xD3F, 0x0},
	{FLG_READ, 0xD34, 0x0},
	{FLG_READ, 0xD36, 0x0},
	{FLG_READ, 0xD37, 0x0},
	{FLG_READ, 0xD3B, 0x0},
	{FLG_READ, 0xD3C, 0x0},
	//#wait  500
	{FLG_DELAY, 0x5, 0x0},
	//
	////change SXTX/SXRX reference clock frequency from x2 to x4
	////SYSPLL remains X2
	////./ad9361 w 0xD2A 0x3D
	{FLG_WRITE, 0xD2A, 0x3C},
	//
	//
	//
	//
};
const int G_REG_ARRAY_ANA_7B_MAN_X4_EN_LEN = sizeof(G_REG_ARRAY_ANA_7B_MAN_X4_EN)/sizeof(G_REG_ARRAY_ANA_7B_MAN_X4_EN[0]);

const reg_t G_REG_ARRAY_ANA_7A_MAN_X2_EN[] = 
{
	////X4 bypass mode
	{FLG_WRITE, 0xD2C, 0x32},
	{FLG_WRITE, 0xD2A, 0x14},
	{FLG_WRITE, 0xD13, 0x16},
};
const int G_REG_ARRAY_ANA_7A_MAN_X2_EN_LEN = sizeof(G_REG_ARRAY_ANA_7A_MAN_X2_EN)/sizeof(G_REG_ARRAY_ANA_7A_MAN_X2_EN[0]);

/****************************************************************************************/
// rx adc cal
/****************************************************************************************/
const reg_t G_REG_ARRAY_for_IIP3_RX1_c10_SHA_ibias[] = 
{
	//
	////sub1 c10p
	{FLG_WRITE, 0x699, 0x14},
	{FLG_WRITE, 0x69A, 0x03},
	{FLG_WRITE, 0x69B, 0xFA},
	{FLG_WRITE, 0x699, 0xB4},
	//
	////sub1 c10n
	{FLG_WRITE, 0x699, 0x15},
	{FLG_WRITE, 0x69A, 0xFC},
	{FLG_WRITE, 0x69B, 0x06},
	{FLG_WRITE, 0x699, 0xB5},
	//
	{FLG_WRITE, 0x699, 0x00},
	//
	//
	////sub2 c10p
	{FLG_WRITE, 0x69C, 0x14},
	{FLG_WRITE, 0x69D, 0x03},
	{FLG_WRITE, 0x69E, 0xFA},
	{FLG_WRITE, 0x69C, 0xB4},
	//
	////sub2 c10n
	{FLG_WRITE, 0x69C, 0x15},
	{FLG_WRITE, 0x69D, 0xFC},
	{FLG_WRITE, 0x69E, 0x06},
	{FLG_WRITE, 0x69C, 0xB5},
	//
	{FLG_WRITE, 0x69C, 0x00},
	//
	//
	//
	//
	//
	////sub1 c10p
	{FLG_WRITE, 0x699, 0x14},
	{FLG_WRITE, 0x69A, 0x03},
	{FLG_WRITE, 0x69B, 0xFA},
	{FLG_WRITE, 0x699, 0x74},
	//
	////sub1 c10n
	{FLG_WRITE, 0x699, 0x15},
	{FLG_WRITE, 0x69A, 0xFC},
	{FLG_WRITE, 0x69B, 0x06},
	{FLG_WRITE, 0x699, 0x75},
	//
	{FLG_WRITE, 0x699, 0x00},
	//
	//
	////sub2 c10p
	{FLG_WRITE, 0x69C, 0x14},
	{FLG_WRITE, 0x69D, 0x03},
	{FLG_WRITE, 0x69E, 0xFA},
	{FLG_WRITE, 0x69C, 0x74},
	//
	////sub2 c10n
	{FLG_WRITE, 0x69C, 0x15},
	{FLG_WRITE, 0x69D, 0xFC},
	{FLG_WRITE, 0x69E, 0x06},
	{FLG_WRITE, 0x69C, 0x75},
	//
	{FLG_WRITE, 0x69C, 0x00},
	//
	//
	//
	//{FLG_WRITE, 0x666, 0x0F},
	//{FLG_WRITE, 0x668, 0x02},
	//
	//
};
const int G_REG_ARRAY_for_IIP3_RX1_c10_SHA_ibias_LEN = sizeof(G_REG_ARRAY_for_IIP3_RX1_c10_SHA_ibias)/sizeof(G_REG_ARRAY_for_IIP3_RX1_c10_SHA_ibias[0]);

const reg_t G_REG_ARRAY_for_IIP3_RX2_c10_SHA_ibias[] = 
{
	//
	////sub1 c10p
	{FLG_WRITE, 0x6B7, 0x14},
	{FLG_WRITE, 0x6B8, 0x03},
	{FLG_WRITE, 0x6B9, 0xFA},
	{FLG_WRITE, 0x6B7, 0xB4},
	//
	////sub1 c10n
	{FLG_WRITE, 0x6B7, 0x15},
	{FLG_WRITE, 0x6B8, 0xFC},
	{FLG_WRITE, 0x6B9, 0x06},
	{FLG_WRITE, 0x6B7, 0xB5},
	//
	{FLG_WRITE, 0x6B7, 0x00},
	//
	//
	////sub2 c10p
	{FLG_WRITE, 0x6BA, 0x14},
	{FLG_WRITE, 0x6BB, 0x03},
	{FLG_WRITE, 0x6BC, 0xFA},
	{FLG_WRITE, 0x6BA, 0xB4},
	//
	////sub2 c10n
	{FLG_WRITE, 0x6BA, 0x15},
	{FLG_WRITE, 0x6BB, 0xFC},
	{FLG_WRITE, 0x6BC, 0x06},
	{FLG_WRITE, 0x6BA, 0xB5},
	//
	{FLG_WRITE, 0x6BA, 0x00},
	//
	//
	//
	//
	//
	////sub1 c10p
	{FLG_WRITE, 0x6B7, 0x14},
	{FLG_WRITE, 0x6B8, 0x03},
	{FLG_WRITE, 0x6B9, 0xFA},
	{FLG_WRITE, 0x6B7, 0x74},
	//
	////sub1 c10n
	{FLG_WRITE, 0x6B7, 0x15},
	{FLG_WRITE, 0x6B8, 0xFC},
	{FLG_WRITE, 0x6B9, 0x06},
	{FLG_WRITE, 0x6B7, 0x75},
	//
	{FLG_WRITE, 0x6B7, 0x00},
	//
	//
	////sub2 c10p
	{FLG_WRITE, 0x6BA, 0x14},
	{FLG_WRITE, 0x6BB, 0x03},
	{FLG_WRITE, 0x6BC, 0xFA},
	{FLG_WRITE, 0x6BA, 0x74},
	//
	////sub2 c10n
	{FLG_WRITE, 0x6BA, 0x15},
	{FLG_WRITE, 0x6BB, 0xFC},
	{FLG_WRITE, 0x6BC, 0x06},
	{FLG_WRITE, 0x6BA, 0x75},
	//
	{FLG_WRITE, 0x6BA, 0x00},
	//
	//
	//
	//
	//{FLG_WRITE, 0x684, 0x0F},
	//{FLG_WRITE, 0x686, 0x02},
	//
	//
};
const int G_REG_ARRAY_for_IIP3_RX2_c10_SHA_ibias_LEN = sizeof(G_REG_ARRAY_for_IIP3_RX2_c10_SHA_ibias)/sizeof(G_REG_ARRAY_for_IIP3_RX2_c10_SHA_ibias[0]);

/****************************************************************************************/
// cal: analog setting
/****************************************************************************************/
const reg_t G_REG_ARRAY_tx1_dc[] =
{
	////Enable DC offset test path
	{FLG_WRITE, 0x83A, 0x01},
	////TXDAC DC = 0
	//{FLG_WRITE, 0x790, 0x01},
	//{FLG_WRITE, 0x791, 0x2F},
	//{FLG_WRITE, 0x7A1, 0x00},
	////CHI
	{FLG_WRITE, 0x7C4, 0x80},
	////CHQ
	{FLG_WRITE, 0x7C2, 0x80},
	////TX chain
	//{FLG_WRITE, 0x747, 0x01},
	{FLG_WRITE, 0x700, 0x00},
	//{FLG_WRITE, 0x702, 0x0F},
	//{FLG_WRITE, 0x703, 0x07},
	//
	//{FLG_WRITE, 0x705, 0xFF},
	//{FLG_WRITE, 0x706, 0xFF},
	//{FLG_WRITE, 0x707, 0xFF},
	{FLG_WRITE, 0x705, 0x80},
	{FLG_WRITE, 0x706, 0x80},
	{FLG_WRITE, 0x707, 0x80},
	//{FLG_WRITE, 0x708, 0x6E},
	//{FLG_WRITE, 0x70B, 0x80},
	//{FLG_WRITE, 0x70F, 0x80},
	//{FLG_WRITE, 0x739, 0x88},
	//
	//{FLG_WRITE, 0x72A, 0x01},
};
const int G_REG_ARRAY_tx1_dc_LEN = sizeof(G_REG_ARRAY_tx1_dc)/sizeof(G_REG_ARRAY_tx1_dc[0]);


const reg_t G_REG_ARRAY_tx2_dc[] =
{
	////Enable DC offset test path
	{FLG_WRITE, 0x83A, 0x10},
	////TXDAC DC = 0
	//{FLG_WRITE, 0x7A3, 0x01},
	//{FLG_WRITE, 0x7A4, 0x2F},
	//
	//{FLG_WRITE, 0x7B4, 0x00},  ///  RG_TX2DAC_17
	////CHI
	{FLG_WRITE, 0x7D4, 0x80},
	////CHQ
	{FLG_WRITE, 0x7D2, 0x80},
	////TX chain
	//{FLG_WRITE, 0x78F, 0x01},
	{FLG_WRITE, 0x748, 0x00},
	//{FLG_WRITE, 0x74A, 0x0F},
	//{FLG_WRITE, 0x74B, 0x07},
	//
	{FLG_WRITE, 0x74D, 0x80},
	{FLG_WRITE, 0x74E, 0x80},
	{FLG_WRITE, 0x74F, 0x80},
	//{FLG_WRITE, 0x750, 0x6E},
	//{FLG_WRITE, 0x753, 0x80},
	//{FLG_WRITE, 0x757, 0x80},
	//{FLG_WRITE, 0x781, 0x88},
	//
	//{FLG_WRITE, 0x772, 0x01},
	//
};
const int G_REG_ARRAY_tx2_dc_LEN = sizeof(G_REG_ARRAY_tx2_dc)/sizeof(G_REG_ARRAY_tx2_dc[0]);


const reg_t G_REG_ARRAY_rx1_bw_cal1[] =
{
	////For LPBK, DA gain 3 dB lower.
	{FLG_WRITE, 0x725, 0x33},
	//
	////------start----hjt del
	////TXDAC Init and calibration
	//
	////./ad9361 w 0x790 0x01
	////./ad9361 w 0x791 0x0F
	////./ad9361 w 0x791 0x2F
	////./ad9361 w 0x604 0xB
	//
	////TXDAC CAL
	////./ad9361 w 790 7
	//
	////#wait
	{FLG_DELAY, 0x1, 0x0},
	//
	////./ad9361 w 790 1
	////-------end-----hjt del
	//
	////TX2 RVDD18 OFF, for LO spurs
	////./ad9361 w 0x801 0xED
	//
	////TX chain
	//
	{FLG_WRITE, 0x747, 0x01},
	{FLG_WRITE, 0x700, 0x04},
	{FLG_WRITE, 0x702, 0x0F},
	{FLG_WRITE, 0x703, 0x07},
	//
	{FLG_WRITE, 0x705, 0x46},
	{FLG_WRITE, 0x706, 0x46},
	{FLG_WRITE, 0x707, 0x46},
	{FLG_WRITE, 0x708, 0x6E},
	{FLG_WRITE, 0x70B, 0x10},
	{FLG_WRITE, 0x70F, 0x10},
	{FLG_WRITE, 0x739, 0x88},
	//
	{FLG_WRITE, 0x72A, 0x01},
};
const int G_REG_ARRAY_rx1_bw_cal1_LEN = sizeof(G_REG_ARRAY_rx1_bw_cal1)/sizeof(G_REG_ARRAY_rx1_bw_cal1[0]);


const reg_t G_REG_ARRAY_rx1_bw_cal2[] =
{
	////TX related LDO enable
	////./ad9361 w 0x804 0x1F
	////./ad9361 w 0x806 0x0F
	//
	{FLG_WRITE, 0x600, 0x03},
	{FLG_WRITE, 0x604, 0x0B},
	//
	//
	{FLG_WRITE, 0x747, 0x01},
	{FLG_WRITE, 0x790, 0x01},
	//
	//
	////TX2 RVDD18 OFF, for LO spurs
	////./ad9361 w 0x801 0x29
	//
	////------start----hjt del
	////TXDAC DC
	////./ad9361 w 0x790 0x01
	////./ad9361 w 0x791 0x0F
	////./ad9361 w 0x791 0x2F
	////------end----hjt del
	{FLG_WRITE, 0x7A1, 0x00},
	////CHI
	{FLG_WRITE, 0x7C4, 0x90},
	////CHQ
	{FLG_WRITE, 0x7C2, 0x90},
	//
	//// TX1B
	////./ad9361 w 0x700 0x04
	//
	//// TX1A
	{FLG_WRITE, 0x700, 0x02},
	//
	{FLG_WRITE, 0x702, 0x0F},
	{FLG_WRITE, 0x703, 0x07},
	{FLG_WRITE, 0x739, 0x88},
	//
	//
	{FLG_WRITE, 0x72A, 0x01},
};
const int G_REG_ARRAY_rx1_bw_cal2_LEN = sizeof(G_REG_ARRAY_rx1_bw_cal2)/sizeof(G_REG_ARRAY_rx1_bw_cal2[0]);


const reg_t G_REG_ARRAY_rx1_bw_cal3[] =
{
	//
	////RFLB 
	////TX1to RX1C
	{FLG_WRITE, 0x847, 0x44},
	{FLG_WRITE, 0x848, 0x0C},
	////RXBBF to TP
	{FLG_WRITE, 0x837, 0x03},
	//
	//
	////TX1port off
	{FLG_WRITE, 0x700, 0x00},
	//
};
const int G_REG_ARRAY_rx1_bw_cal3_LEN = sizeof(G_REG_ARRAY_rx1_bw_cal3)/sizeof(G_REG_ARRAY_rx1_bw_cal3[0]);


const reg_t G_REG_ARRAY_rx2_bw_cal1[] =
{
	////For LPBK, DA gain 3 dB lower.
	{FLG_WRITE, 0x76D, 0x33},
	//
	////------start----hjt del
	////TXDAC Init and calibration
	//
	////./ad9361 w 0x790 0x01
	////./ad9361 w 0x791 0x0F
	////./ad9361 w 0x791 0x2F
	////./ad9361 w 0x604 0xB
	//
	////TXDAC CAL
	////./ad9361 w 790 7
	//
	////#wait
	//
	{FLG_DELAY, 0x1, 0x0},
	////./ad9361 w 790 1
	////-------end-----hjt del
	//
	////TX2 RVDD18 OFF, for LO spurs
	////./ad9361 w 0x801 0xED
	//
	////TX chain
	//
	{FLG_WRITE, 0x78F, 0x01},
	{FLG_WRITE, 0x748, 0x04},
	{FLG_WRITE, 0x74A, 0x0F},
	{FLG_WRITE, 0x74B, 0x07},
	//
	{FLG_WRITE, 0x74D, 0x46},
	{FLG_WRITE, 0x74E, 0x46},
	{FLG_WRITE, 0x74F, 0x46},
	{FLG_WRITE, 0x750, 0x6E},
	{FLG_WRITE, 0x753, 0x10},
	{FLG_WRITE, 0x757, 0x10},
	{FLG_WRITE, 0x781, 0x88},
	//
	{FLG_WRITE, 0x772, 0x01},
};
const int G_REG_ARRAY_rx2_bw_cal1_LEN = sizeof(G_REG_ARRAY_rx2_bw_cal1)/sizeof(G_REG_ARRAY_rx2_bw_cal1[0]);


const reg_t G_REG_ARRAY_rx2_bw_cal2[] =
{
	////TX related LDO enable
	////./ad9361 w 0x804 0x1F
	////./ad9361 w 0x806 0x0F
	//
	{FLG_WRITE, 0x600, 0x03},
	{FLG_WRITE, 0x604, 0x0B},
	//
	//
	{FLG_WRITE, 0x78F, 0x01},
	{FLG_WRITE, 0x7A3, 0x01},
	//
	//
	////TX2 RVDD18 OFF, for LO spurs
	////./ad9361 w 0x801 0x29
	//
	////------start----hjt del
	////TXDAC DC
	////./ad9361 w 0x790 0x01
	////./ad9361 w 0x791 0x0F
	////./ad9361 w 0x791 0x2F
	////------end----hjt del
	{FLG_WRITE, 0x7B4, 0x00},
	////CHI
	{FLG_WRITE, 0x7D4, 0x90},
	////CHQ
	{FLG_WRITE, 0x7D2, 0x90},
	//
	//// TX1B
	////./ad9361 w 0x700 0x04
	//
	//// TX1A
	{FLG_WRITE, 0x748, 0x02},
	//
	{FLG_WRITE, 0x74A, 0x0F},
	{FLG_WRITE, 0x74B, 0x07},
	{FLG_WRITE, 0x781, 0x88},
	//
	//
	{FLG_WRITE, 0x772, 0x01},
};
const int G_REG_ARRAY_rx2_bw_cal2_LEN = sizeof(G_REG_ARRAY_rx2_bw_cal2)/sizeof(G_REG_ARRAY_rx2_bw_cal2[0]);


const reg_t G_REG_ARRAY_rx2_bw_cal3[] =
{
	//
	////RFLB 
	////TX1to RX1C
	{FLG_WRITE, 0x846, 0x44},
	{FLG_WRITE, 0x848, 0x03},
	////RXBBF to TP
	{FLG_WRITE, 0x836, 0x03},
	//
	//
	////TX1port off
	{FLG_WRITE, 0x748, 0x00},
	//
};
const int G_REG_ARRAY_rx2_bw_cal3_LEN = sizeof(G_REG_ARRAY_rx2_bw_cal3)/sizeof(G_REG_ARRAY_rx2_bw_cal3[0]);

const reg_t G_REG_ARRAY_6A_TXDAC_ON[] = 
{	
	#if 0
	//
	{FLG_WRITE, 0x790, 0x01},
	{FLG_WRITE, 0x791, 0x0F},
	{FLG_WRITE, 0x791, 0x2F},
	////./ad9361 w 0x790 0x07
	//
	//
	//#wait   1000
	{FLG_DELAY, 0x5, 0x0},
	//
	{FLG_WRITE, 0x790, 0x1},
	//
	//
	////Increase DAC bias for ACLR
	{FLG_WRITE, 0x799, 0x70},
	{FLG_WRITE, 0x79A, 0x74},
	//
	//
	////Decrease RDAC for ACLR
	{FLG_WRITE, 0x737, 0xF0},
	#else
	{FLG_WRITE, 0x790, 0x01},
	{FLG_WRITE, 0x7A3, 0x01},
	{FLG_WRITE, 0x791, 0x00},
	{FLG_WRITE, 0x7A4, 0x00},	
	{FLG_WRITE, 0x791, 0x2E},
	{FLG_WRITE, 0x7A4, 0x2E},	
	{FLG_WRITE, 0x791, 0x6F},
	{FLG_WRITE, 0x7A4, 0x6F},	
	{FLG_WRITE, 0x791, 0x7F},
	{FLG_WRITE, 0x7A4, 0x7F},	
	{FLG_WRITE, 0x790, 0x01},
	#endif
};
const int G_REG_ARRAY_6A_TXDAC_ON_LEN = sizeof(G_REG_ARRAY_6A_TXDAC_ON)/sizeof(G_REG_ARRAY_6A_TXDAC_ON[0]);


const reg_t G_REG_ARRAY_6A_TXDAC_ON_2[] = 
{
	#if 0
	//
	{FLG_WRITE, 0x7a3, 0x01},
	{FLG_WRITE, 0x7a4, 0x0f},
	{FLG_WRITE, 0x7a4, 0x2f},
	{FLG_WRITE, 0x7a3, 0x07},
	//
	//
	//#wait
	{FLG_DELAY, 0x5, 0x0},
	//
	{FLG_WRITE, 0x7a3, 0x01},
	//
	//
	////Increase DAC bias for ACLR
	{FLG_WRITE, 0x7AC, 0x70},
	{FLG_WRITE, 0x7AD, 0x74},
	//
	////Decrease RDAC for ACLR
	//{FLG_WRITE, 0x77F, 0xF0},
	#else
	{FLG_WRITE, 0x790, 0x01},
	{FLG_WRITE, 0x7A3, 0x01},
	{FLG_WRITE, 0x791, 0x00},
	{FLG_WRITE, 0x7A4, 0x00},	
	{FLG_WRITE, 0x791, 0x2E},
	{FLG_WRITE, 0x7A4, 0x2E},	
	{FLG_WRITE, 0x791, 0x6F},
	{FLG_WRITE, 0x7A4, 0x6F},	
	{FLG_WRITE, 0x791, 0x7F},
	{FLG_WRITE, 0x7A4, 0x7F},	
	{FLG_WRITE, 0x790, 0x01},
	#endif
};
const int G_REG_ARRAY_6A_TXDAC_ON_2_LEN = sizeof(G_REG_ARRAY_6A_TXDAC_ON_2)/sizeof(G_REG_ARRAY_6A_TXDAC_ON_2[0]);

/****************************************************************************************/
// power up a: mbias && xo syspll on
/****************************************************************************************/
const reg_t G_REG_ARRAY_Power_Up_A_1_SYSPLL_LTE20[] = 
{
	{FLG_WRITE, 0x900, 0x07},
	{FLG_WRITE, 0x901, 0x4f},
	////D1 IVREF OP RSEL, keep DEFAULT MDIG_LDO_OUT (current setup in C1 different from DEFAULT), added 10172022 byYN
	////./ad9361 w 0x904 0x88
	{FLG_WRITE, 0x904, 0xA4},
	{FLG_DELAY, 0x1, 0x00},
	{FLG_WRITE, 0x816, 0x01},
	{FLG_WRITE, 0x810, 0xc6},
	{FLG_WRITE, 0x802, 0xb9},
	{FLG_WRITE, 0x807, 0x00},
	{FLG_WRITE, 0x615, 0x04},
	//
	//
	////X4 bypass mode
	////./ad9361 w 0xD2C 0x32 change to 26 for lower spur @ 122.88MHz
	{FLG_WRITE, 0xD2C, 0x26},
	{FLG_WRITE, 0xD2A, 0x14},
	{FLG_WRITE, 0xD13, 0x16},
	//
	////SYSPLL on
	{FLG_WRITE, 0x600, 0x00},
	//
	////IB 3
	{FLG_WRITE, 0x605, 0xE8},
	//
	{FLG_WRITE, 0x606, 0x08},
	{FLG_WRITE, 0x607, 0x88},
	{FLG_WRITE, 0x608, 0x08},
	{FLG_WRITE, 0x609, 0x08},
	//
	////RSEL 1
	{FLG_WRITE, 0x6f1, 0x15},
	////SCAP 101010
	{FLG_WRITE, 0x60a, 0x1c},
	//
	{FLG_WRITE, 0x613, 0x01},
	//
	////LOOP DIV
	{FLG_WRITE, 0x611, 0x60},
	//
	{FLG_WRITE, 0x601, 0x8f},
	{FLG_WRITE, 0x60b, 0x03},
	//
	////SYSPLL MANUAL and scap from DA_SCAP
	{FLG_WRITE, 0x600, 0x0B},
	{FLG_WRITE, 0x600, 0x0F},
	{FLG_WRITE, 0x600, 0x0B},
	//
	////Rx mode init
	{FLG_WRITE, 0xd34, 0xcc},
	//
	//
	////Add TXDAC CLK EN by xiaoshuangz @ 20221102
	{FLG_WRITE, 0x790, 0x01},
	{FLG_WRITE, 0x7a3, 0x01},
	{FLG_WRITE, 0x791, 0x00},
	{FLG_WRITE, 0x7a4, 0x00},
	{FLG_WRITE, 0x791, 0x21},
	{FLG_WRITE, 0x7a4, 0x21},
	//Add LDO size setting
	{FLG_WRITE, 0x80b, 0xfa},
	{FLG_WRITE, 0x80c, 0x47},
	{FLG_WRITE, 0x80d, 0xff},
	{FLG_WRITE, 0x80e, 0xaf},
	{FLG_WRITE, 0x80f, 0x47},
	{FLG_WRITE, 0x810, 0xf4},
	{FLG_WRITE, 0x811, 0x88},
	{FLG_WRITE, 0x813, 0x43},
	{FLG_WRITE, 0x814, 0x44},
	{FLG_WRITE, 0x815, 0x43},
};
const int G_REG_ARRAY_Power_Up_A_1_SYSPLL_LTE20_LEN = sizeof(G_REG_ARRAY_Power_Up_A_1_SYSPLL_LTE20)/sizeof(G_REG_ARRAY_Power_Up_A_1_SYSPLL_LTE20[0]);


const reg_t G_REG_ARRAY_Power_Up_A_2_ANA_MANUAL_ON[] = 
{
	////LDO 
	{FLG_WRITE, 0x816, 0x01},
	//
	////SXRX
	{FLG_WRITE, 0x634, 0x04},
	//
	////SXTX
	{FLG_WRITE, 0x64D, 0x04},
	//
	////RX1
	{FLG_WRITE, 0x650, 0x01},
	//
	////RX2
	{FLG_WRITE, 0x66E, 0x01},
	//
	////RX1_ADC
	{FLG_WRITE, 0x698, 0x80},
	//
	////RX2_ADC
	{FLG_WRITE, 0x6B6, 0x80},
	//
	////TX1
	{FLG_WRITE, 0x747, 0x01},
	//
	////TX2
	{FLG_WRITE, 0x78F, 0x01},
	//
	////TX1_DAC
	{FLG_WRITE, 0x790, 0x01},
	//
	////TX2_DAC
	{FLG_WRITE, 0x7A3, 0x01},
};
const int G_REG_ARRAY_Power_Up_A_2_ANA_MANUAL_ON_LEN = sizeof(G_REG_ARRAY_Power_Up_A_2_ANA_MANUAL_ON)/sizeof(G_REG_ARRAY_Power_Up_A_2_ANA_MANUAL_ON[0]);


const reg_t G_REG_ARRAY_Power_Up_B_1_SXTXRX_INIT_LTE20[] = 
{
	////SXTXRX initialization
	////./ad9361 w 0x802 0xBF
	////./ad9361 w 0x803 0x0F
	////./ad9361 w 0x804 0x1F
	////./ad9361 w 0x805 0x3F
	////./ad9361 w 0x806 0x3F
	//
	////LDO FC enable
	{FLG_WRITE, 0x807, 0xDF},
	//
	////SX VCO EN, VCO/XO FC Disabled
	////./ad9361 w 0x615 0x0F
	//// change for D1, 0F to 0D
	{FLG_WRITE, 0x615, 0x09},
	{FLG_WRITE, 0x614, 0x06},
	{FLG_WRITE, 0x619, 0x5C},
	////./ad9361 w 0x619 0x1C
	//
	////RX1ABB, RX2ABB Startup Disabled
	{FLG_WRITE, 0x650, 0x01},
	{FLG_WRITE, 0x66E, 0x01},
	//
	////RX1, RX2 ADC RSTN toggling
	////./ad9361 w 0x698 0x80
	////./ad9361 w 0x696 0x00
	////./ad9361 w 0x696 0x30
	////./ad9361 w 0x6B6 0x80
	////./ad9361 w 0x6B4 0x00
	////./ad9361 w 0x6B4 0x30
	//
	////RX1, RX2 DAC RSTN toggling
	////./ad9361 w 0x790 0x01
	////./ad9361 w 0x791 0x00
	////./ad9361 w 0x791 0x20
	////./ad9361 w 0x7A3 0x01
	////./ad9361 w 0x7A4 0x00
	////./ad9361 w 0x7A4 0x20
	//
	////LDO,RVDD18 bypass
	////./ad9361 w 0x901 0xDF
	{FLG_WRITE, 0x902, 0x38},
	{FLG_WRITE, 0x903, 0x38},
	////MDIG changed from 1d2V to 1d1V special
	////size 10 used for IVREF_OP_R_SEL,changed on 10172022 byYN
	////./ad9361 w 0x904 0x88
	{FLG_WRITE, 0x904, 0xA4},
	////ori
	////./ad9361 w 0x904 0xBF
	////./ad9361 w 0x904 0xB4
	//
	//
	//
	{FLG_WRITE, 0x817, 0x88},
	{FLG_WRITE, 0x818, 0x48},
	{FLG_WRITE, 0x819, 0x38},
	////2022_11_04, decrease LO +/- 4MHz spurs by YP
	////2022_11_08, increase LDO from 08 to 88 for better PN by YP
	{FLG_WRITE, 0x81A, 0x28},
	{FLG_WRITE, 0x81B, 0xA8},
	//
	////add SX MBIAS setting, remove by YP
	////./ad9361 w 0x619 0x9C
	{FLG_WRITE, 0x61A, 0x4C},
	{FLG_WRITE, 0x619, 0x1C},
	//
	////SYSPLL HW FCAL
	////SYSPLL_FC_off
	{FLG_WRITE, 0x600, 0x03},
	{FLG_WRITE, 0x807, 0x0f},
	{FLG_WRITE, 0x602, 0x00},
	//
	////target counter
	{FLG_WRITE, 0x024, 0x01},
	{FLG_WRITE, 0x026, 0x00},
	{FLG_WRITE, 0x027, 0xC0},
	//
	////open fcal
	{FLG_WRITE, 0x600, 0x13},
	//
	////read fcal results   bits5 from 0 to 1
	////#wait  16000
	{FLG_READ, 0x028, 0x0},
	{FLG_READ, 0x02d, 0x0},
	//
	////fcal off
	{FLG_WRITE, 0x600, 0x03},
	//
	////Turn off RF SYNC
	//// (add for D1)
	{FLG_WRITE, 0x631, 0x83},
	{FLG_WRITE, 0x64A, 0x83},
	//
	////set RCAL result
	{FLG_WRITE, 0x902, 0x27},
	{FLG_WRITE, 0x903, 0x27},
	//
	//
	//
	//
	//
};
const int G_REG_ARRAY_Power_Up_B_1_SXTXRX_INIT_LTE20_LEN = sizeof(G_REG_ARRAY_Power_Up_B_1_SXTXRX_INIT_LTE20)/sizeof(G_REG_ARRAY_Power_Up_B_1_SXTXRX_INIT_LTE20[0]);


const reg_t G_REG_ARRAY_Power_Up_B_2_MAN_RVDD18[] = 
{
	{FLG_WRITE, 0x816, 0x01},
	{FLG_WRITE, 0x801, 0xEF},
	//
	////SXTX related LDO enable
	{FLG_WRITE, 0x804, 0x18},
	{FLG_WRITE, 0x806, 0x04},
	{FLG_WRITE, 0x64D, 0x00},
	//
	////TX1TX2 related LDO enable
	{FLG_WRITE, 0x804, 0x1F},
	{FLG_WRITE, 0x806, 0xFF},
	//
	////SXRX related LDO enable
	{FLG_WRITE, 0x803, 0x0C},
	{FLG_WRITE, 0x805, 0x02},
	{FLG_WRITE, 0x634, 0x00},
	//
	////RX1RX2 LDO ENable
	{FLG_WRITE, 0x803, 0x0F},
	{FLG_WRITE, 0x805, 0x3F},
	//
};
const int G_REG_ARRAY_Power_Up_B_2_MAN_RVDD18_LEN = sizeof(G_REG_ARRAY_Power_Up_B_2_MAN_RVDD18)/sizeof(G_REG_ARRAY_Power_Up_B_2_MAN_RVDD18[0]);

/****************************************************************************************/
// power up e: sx x4 selection
/****************************************************************************************/
const reg_t G_REG_ARRAY_Power_Up_F_1_MAN_X2_EN[] = 
{
	////X4 bypass mode
	////./ad9361 w 0xD2C 0x32 change to 26 for lower spur @ 122.88MHz
	{FLG_WRITE, 0xD2C, 0x26},
	{FLG_WRITE, 0xD2A, 0x14},
	{FLG_WRITE, 0xD13, 0x16},

	{FLG_WRITE, 0xD00, 0x16},
	{FLG_WRITE, 0xD03, 0x80},
	{FLG_WRITE, 0xD06, 0x3c},
	{FLG_WRITE, 0xD07, 0x33},
	{FLG_WRITE, 0xD05, 0x63},
};
const int G_REG_ARRAY_Power_Up_F_1_MAN_X2_EN_LEN = sizeof(G_REG_ARRAY_Power_Up_F_1_MAN_X2_EN)/sizeof(G_REG_ARRAY_Power_Up_F_1_MAN_X2_EN[0]);


const reg_t G_REG_ARRAY_Power_Up_F_2_MAN_X4_EN[] = 
{
	////CALPLL lock state check from test   
	{FLG_WRITE, 0xD04, 0x4C},
	//   
	////X4 calibration config   
	{FLG_WRITE, 0xD08, 0x39},
	{FLG_WRITE, 0xD05, 0xe3},
	{FLG_WRITE, 0xD00, 0xB6},
	{FLG_WRITE, 0xD03, 0x8A},
	{FLG_WRITE, 0xD06, 0xBC},
	{FLG_WRITE, 0xD07, 0xB3},
	{FLG_WRITE, 0xD13, 0x96},
	{FLG_WRITE, 0xD13, 0xD7},
	{FLG_WRITE, 0xD08, 0xB9},
	{FLG_WRITE, 0xD08, 0x39},
	//   
	//   
	////For better PN   
	{FLG_WRITE, 0xD00, 0xB6},
	{FLG_WRITE, 0xD03, 0x8A},
	//   
	////Calibration state check: 
	{FLG_DELAY, 5, 0x0},
	{FLG_READ, 0xD3E, 0x0},
	{FLG_READ, 0xD3F, 0x0},
	{FLG_READ, 0xD34, 0x0},
	{FLG_READ, 0xD36, 0x0},
	{FLG_READ, 0xD37, 0x0},
	{FLG_READ, 0xD3B, 0x0},
	{FLG_READ, 0xD3C, 0x0},
	{FLG_DELAY, 5, 0x0},
	//   
	//   
	////change SXTX/SXRX reference   
	//// clock frequency from x2 to x4   
	////change SXTX/SXRX reference clock frequency from x2 to x4 remains X2 
	////./ad9361 w 0xD2A 0x3D
	{FLG_WRITE, 0xD2A, 0x3C},
	{FLG_WRITE, 0xD13, 0xC7},
};
const int G_REG_ARRAY_Power_Up_F_2_MAN_X4_EN_LEN = sizeof(G_REG_ARRAY_Power_Up_F_2_MAN_X4_EN)/sizeof(G_REG_ARRAY_Power_Up_F_2_MAN_X4_EN[0]);

/****************************************************************************************/
// power up f: initialization(LUT,SX,TX,RX,ADC,DAC...)
/****************************************************************************************/
const reg_t G_REG_ARRAY_Power_Up_E_0_1_SXTX_wFCAL_ON_VCO10380_XO30p72_X4[] = 
{
	////SXTX related LDO enable
	////./ad9361 w 0x804 0x18
	////./ad9361 w 0x806 0x04
	//
	////SX FCAL
	{FLG_WRITE, 0x64D, 0x04},
	//
	////C1 RX FCAL do not need use RXLO 
	////./ad9361 w 0x747 0x01
	////./ad9361 w 0x78F 0x01
	//
	//
	{FLG_WRITE, 0x637, 0x12},
	{FLG_WRITE, 0x638, 0x1F},
	////./ad9361 w 0x639 0xCF
	{FLG_WRITE, 0x639, 0x44},
	//
	//
	////FCAL WIN128
	{FLG_WRITE, 0x019, 0xCE},
	//
	////./ad9361 w 0x702 0x00
	////./ad9361 w 0x703 0xEC
	////./ad9361 w 0x704 0x08
	//
	////LO1_DIV
	////./ad9361 w 0x72A 0x04
	//
	//// IVCO, LDO, TCOMP_VAR, SX_MBIAS
	////./ad9361 w 0x81E 0x04
	////./ad9361 w 0x81D 0x80
	////./ad9361 w 0x81A 0xDD
	//
	//
	{FLG_WRITE, 0x645, 0x1F},
	////./ad9361 w 0x619 0x69
	{FLG_WRITE, 0x649, 0xBA},
	//
	////SX Performance setting
	{FLG_WRITE, 0x645, 0x20},
	{FLG_WRITE, 0x646, 0x00},
	{FLG_WRITE, 0x63A, 0x19},
	{FLG_WRITE, 0x63B, 0xD0},
	{FLG_WRITE, 0x641, 0xCC},
	{FLG_WRITE, 0x642, 0x0F},
	//
	////62A high 3bit set-up FCAL DIV: 001-DIV4, 010-DIV2, 100-DIV1
	{FLG_WRITE, 0x643, 0x29},
	{FLG_WRITE, 0x644, 0x19},
	{FLG_WRITE, 0x649, 0xBF},
	{FLG_WRITE, 0x64E, 0x00},
	//
	////SX HW Calibration Digital Settings
	{FLG_WRITE, 0x63C, 0x1E},
	{FLG_WRITE, 0x63D, 0x40},
	{FLG_WRITE, 0x63E, 0x00},
	{FLG_WRITE, 0x63F, 0x00},
	{FLG_WRITE, 0x640, 0x2A},
	{FLG_WRITE, 0x01B, 0x2A},
	{FLG_WRITE, 0x01C, 0x3D},
	{FLG_WRITE, 0x019, 0xDE},
	//
	////SX DSM Toggling
	{FLG_WRITE, 0x637, 0x16},
	{FLG_WRITE, 0x637, 0x12},
	//
	////SX FCAL done
	{FLG_WRITE, 0x638, 0x9F},
	////./ad9361 w 0x704 0x00
	////./ad9361 w 0x7ec 0x00
	//
	////SX ON
	{FLG_WRITE, 0x019, 0xCE},
	{FLG_WRITE, 0x637, 0x02},
	//
	//
	//
};
const int G_REG_ARRAY_Power_Up_E_0_1_SXTX_wFCAL_ON_VCO10380_XO30p72_X4_LEN = sizeof(G_REG_ARRAY_Power_Up_E_0_1_SXTX_wFCAL_ON_VCO10380_XO30p72_X4)/sizeof(G_REG_ARRAY_Power_Up_E_0_1_SXTX_wFCAL_ON_VCO10380_XO30p72_X4[0]);


const reg_t G_REG_ARRAY_Power_Up_E_0_2_SXRX_wFCAL_ON_VCO9600_XO30p72_X4[] = 
{
	////SXRX related LDO enable
	////./ad9361 w 0x803 0x0C
	////./ad9361 w 0x805 0x02
	//
	////SX FCAL
	{FLG_WRITE, 0x634, 0x04},
	//
	////C1 RX FCAL do not need use RXLO 
	////./ad9361 w 0x650 0x01
	////./ad9361 w 0x66E 0x01
	//
	{FLG_WRITE, 0x61E, 0x12},
	//
	////FCAL WIN128
	{FLG_WRITE, 0x00E, 0xCE},
	//
	{FLG_WRITE, 0x61F, 0x1f},
	//
	{FLG_WRITE, 0x620, 0x14},
	//
	////C1 RX FCAL do not need use RXLO 
	////./ad9361 w 0x652 0x83
	//
	//
	////./ad9361 w 0x6D1 0x3F
	//
	////LO1_DIV, C1 RX FCAL do not need use RXLO 
	////./ad9361 w 0x665 0x04
	//
	//
	//// IVCO, LDO, TCOMP_VAR, SX_MBIAS
	////./ad9361 w 0x81D 0x40
	////./ad9361 w 0x81A 0xFD
	{FLG_WRITE, 0x62C, 0x1F},
	////./ad9361 w 0x619 0x69
	{FLG_WRITE, 0x630, 0xBA},
	//
	////SX Performance setting
	////SX LDO Control bit from 5bit to 6bit
	{FLG_WRITE, 0x62C, 0x20},
	{FLG_WRITE, 0x62D, 0x00},
	{FLG_WRITE, 0x621, 0x1A},
	{FLG_WRITE, 0x622, 0xD0},
	{FLG_WRITE, 0x628, 0xCC},
	{FLG_WRITE, 0x629, 0x0F},
	//
	////62A high 3bit set-up FCAL DIV: 001-DIV4, 010-DIV2, 100-DIV1
	{FLG_WRITE, 0x62A, 0x29},
	{FLG_WRITE, 0x62B, 0x19},
	{FLG_WRITE, 0x630, 0xBF},
	{FLG_WRITE, 0x635, 0x00},
	//
	////SX HW Calibration Digital Settings
	{FLG_WRITE, 0x623, 0x08},
	{FLG_WRITE, 0x624, 0x00},
	{FLG_WRITE, 0x625, 0x00},
	{FLG_WRITE, 0x626, 0x00},
	{FLG_WRITE, 0x627, 0x27},
	{FLG_WRITE, 0x010, 0x27},
	{FLG_WRITE, 0x011, 0x10},
	{FLG_WRITE, 0x00E, 0xDE},
	//
	////SX DSM Toggling
	{FLG_WRITE, 0x61E, 0x16},
	{FLG_WRITE, 0x61E, 0x12},
	//
	////SX FCAL done
	{FLG_WRITE, 0x61F, 0x9F},
	////./ad9361 w 0x652 0x82
	////./ad9361 w 0x670 0x82
	//
	////SX ON
	{FLG_WRITE, 0x00E, 0xCE},
	{FLG_WRITE, 0x61E, 0x02},
	//
	////RX1, RX2 Manual off
	////./ad9361 w 0x650 0x00
	////./ad9361 w 0x66E 0x00
	//
	//
	//
};
const int G_REG_ARRAY_Power_Up_E_0_2_SXRX_wFCAL_ON_VCO9600_XO30p72_X4_LEN = sizeof(G_REG_ARRAY_Power_Up_E_0_2_SXRX_wFCAL_ON_VCO9600_XO30p72_X4)/sizeof(G_REG_ARRAY_Power_Up_E_0_2_SXRX_wFCAL_ON_VCO9600_XO30p72_X4[0]);


const reg_t G_REG_ARRAY_Power_Up_E_0_3_LO_CONFIG_2T2R[] = 
{
	////SXRX LO Output config
	{FLG_WRITE, 0x639, 0xCC},
	{FLG_WRITE, 0x620, 0x3C},
	//
	{FLG_WRITE, 0x6D1, 0x7F},
	{FLG_WRITE, 0x6E1, 0x7F},
	{FLG_WRITE, 0x702, 0x0F},
	{FLG_WRITE, 0x74A, 0x0F},
	//
};
const int G_REG_ARRAY_Power_Up_E_0_3_LO_CONFIG_2T2R_LEN = sizeof(G_REG_ARRAY_Power_Up_E_0_3_LO_CONFIG_2T2R)/sizeof(G_REG_ARRAY_Power_Up_E_0_3_LO_CONFIG_2T2R[0]);


const reg_t G_REG_ARRAY_Power_Up_E_1_RX1FE_BBF_ON_DiffInput_CPCN_PORTA_Remove_6D1[] = 
{
	////SXRX_RX1LDO ENable
	////./ad9361 w 0x803 0x0E
	////./ad9361 w 0x805 0x03
	//
	{FLG_WRITE, 0x650, 0x01},
	{FLG_WRITE, 0x651, 0x7F},
	{FLG_WRITE, 0x653, 0x3B},
	{FLG_WRITE, 0x654, 0x3D},
	{FLG_WRITE, 0x655, 0x3D},
	{FLG_WRITE, 0x656, 0x3D},
	{FLG_WRITE, 0x657, 0x3D},
	//
	{FLG_WRITE, 0x653, 0x45},
	{FLG_WRITE, 0x654, 0x38},
	{FLG_WRITE, 0x655, 0x38},
	{FLG_WRITE, 0x656, 0x38},
	{FLG_WRITE, 0x657, 0x38},
	//
	{FLG_WRITE, 0x658, 0x01},
	{FLG_WRITE, 0x659, 0x81},
	{FLG_WRITE, 0x65A, 0x01},
	{FLG_WRITE, 0x65B, 0x50},
	{FLG_WRITE, 0x65C, 0x00},
	{FLG_WRITE, 0x65D, 0x01},
	{FLG_WRITE, 0x65E, 0x01},
	{FLG_WRITE, 0x65F, 0x00},
	{FLG_WRITE, 0x660, 0x05},
	{FLG_WRITE, 0x661, 0x00},
	{FLG_WRITE, 0x662, 0x38},
	//
	//
	{FLG_WRITE, 0x665, 0x01},
	//
	{FLG_WRITE, 0x666, 0x27},
	{FLG_WRITE, 0x667, 0x82},
	//
	//
	//
	{FLG_WRITE, 0x668, 0x00},
	{FLG_WRITE, 0x669, 0x88},
	{FLG_WRITE, 0x66A, 0x0C},
	{FLG_WRITE, 0x66B, 0x20},
	{FLG_WRITE, 0x66C, 0x60},
	{FLG_WRITE, 0x66D, 0x00},
	////./ad9361 w 0x6D0 0xFF
	{FLG_WRITE, 0x6D0, 0xEF},
	//
	////./ad9361 w 0x6D1 0x7F
	////20221114 Gain C
	////./ad9361 w 0x6D2 0x03
	{FLG_WRITE, 0x6D2, 0xC3},
	//
	{FLG_WRITE, 0x6D3, 0x80},
	{FLG_WRITE, 0x6D4, 0x80},
	{FLG_WRITE, 0x6D5, 0x80},
	{FLG_WRITE, 0x6D6, 0x80},
	{FLG_WRITE, 0x6D7, 0x20},
	{FLG_WRITE, 0x6D8, 0x07},
	{FLG_WRITE, 0x6D9, 0x08},
	//
	////RXBBFI offset cal
	{FLG_WRITE, 0x6D2, 0xC3},
	{FLG_WRITE, 0x6D5, 0x80},
	{FLG_WRITE, 0x6D6, 0x80},
	////RXBBFQ offset cal
	{FLG_WRITE, 0x6D2, 0xC3},
	{FLG_WRITE, 0x6D3, 0x80},
	{FLG_WRITE, 0x6D4, 0x80},
	//
	//
};
const int G_REG_ARRAY_Power_Up_E_1_RX1FE_BBF_ON_DiffInput_CPCN_PORTA_Remove_6D1_LEN = sizeof(G_REG_ARRAY_Power_Up_E_1_RX1FE_BBF_ON_DiffInput_CPCN_PORTA_Remove_6D1)/sizeof(G_REG_ARRAY_Power_Up_E_1_RX1FE_BBF_ON_DiffInput_CPCN_PORTA_Remove_6D1[0]);


const reg_t G_REG_ARRAY_Power_Up_E_2_RX2FE_BBF_ON_DiffInput_CPCN_PORTA_Remove_6E1[] = 
{
	////SXRX_RX1LDO ENable
	////./ad9361 w 0x803 0x0E
	////./ad9361 w 0x805 0x03
	//
	{FLG_WRITE, 0x66e, 0x01},
	{FLG_WRITE, 0x66f, 0x7f},
	{FLG_WRITE, 0x671, 0x3b},
	{FLG_WRITE, 0x672, 0x3d},
	{FLG_WRITE, 0x673, 0x3d},
	{FLG_WRITE, 0x674, 0x3d},
	{FLG_WRITE, 0x675, 0x3d},
	//
	{FLG_WRITE, 0x671, 0x45},
	{FLG_WRITE, 0x672, 0x38},
	{FLG_WRITE, 0x673, 0x38},
	{FLG_WRITE, 0x674, 0x38},
	{FLG_WRITE, 0x675, 0x38},
	//
	//
	{FLG_WRITE, 0x676, 0x01},

	{FLG_WRITE, 0x678, 0x01},
	{FLG_WRITE, 0x679, 0x50},
	{FLG_WRITE, 0x67a, 0x00},
	{FLG_WRITE, 0x67b, 0x01},
	{FLG_WRITE, 0x67c, 0x01},
	{FLG_WRITE, 0x67d, 0x00},
	{FLG_WRITE, 0x67e, 0x05},
	{FLG_WRITE, 0x67f, 0x00},
	{FLG_WRITE, 0x680, 0x38},
	//
	//
	{FLG_WRITE, 0x683, 0x01},
	//
	{FLG_WRITE, 0x684, 0x27},
	{FLG_WRITE, 0x685, 0x82},
	//
	{FLG_WRITE, 0x686, 0x00},
	{FLG_WRITE, 0x687, 0x88},
	{FLG_WRITE, 0x688, 0x0c},
	{FLG_WRITE, 0x689, 0x20},
	{FLG_WRITE, 0x68a, 0x60},
	{FLG_WRITE, 0x68b, 0x00},
	////./ad9361 w 0x6e0 0xff
	{FLG_WRITE, 0x6e0, 0xef},
	//
	////./ad9361 w 0x6e1 0x7f
	////./ad9361 w 0x6e2 0x03
	{FLG_WRITE, 0x6e2, 0xC3},
	//
	{FLG_WRITE, 0x6e3, 0x80},
	{FLG_WRITE, 0x6e4, 0x80},
	{FLG_WRITE, 0x6e5, 0x80},
	{FLG_WRITE, 0x6e6, 0x80},
	{FLG_WRITE, 0x6e7, 0x20},
	{FLG_WRITE, 0x6e8, 0x07},
	{FLG_WRITE, 0x6e9, 0x08},
	//
	////RXBBFI offset cal
	{FLG_WRITE, 0x6e2, 0xC3},
	{FLG_WRITE, 0x6e5, 0x80},
	{FLG_WRITE, 0x6e6, 0x80},
	////RXBBFQ offset cal
	{FLG_WRITE, 0x6e2, 0xC3},
	{FLG_WRITE, 0x6e3, 0x80},
	{FLG_WRITE, 0x6e4, 0x80},
	//
	//
};
const int G_REG_ARRAY_Power_Up_E_2_RX2FE_BBF_ON_DiffInput_CPCN_PORTA_Remove_6E1_LEN = sizeof(G_REG_ARRAY_Power_Up_E_2_RX2FE_BBF_ON_DiffInput_CPCN_PORTA_Remove_6E1)/sizeof(G_REG_ARRAY_Power_Up_E_2_RX2FE_BBF_ON_DiffInput_CPCN_PORTA_Remove_6E1[0]);


const reg_t G_REG_ARRAY_Power_Up_E_3_RX1_ADC_with_ofst_gain_cal[] = 
{
	////ADC LDO on
	////./ad9361 w 0x803 0x0F
	////./ad9361 w 0x805 0x07
	//
	//
	////ADC LDO BYPASS
	////./ad9361 w 0x81d 0x01
	//
	//
	////ADC OFF I&Q
	{FLG_WRITE, 0x6b6, 0x80},
	{FLG_WRITE, 0x6b5, 0x00},
	//
	////SYSPLL to RXADC
	////./ad9361 w 0x600 0x03
	////./ad9361 w 0x6f0 0x03
	//
	////ADC RST
	{FLG_WRITE, 0x6b4, 0x30},
	{FLG_WRITE, 0x6b4, 0x00},
	{FLG_WRITE, 0x6b4, 0x00},
	{FLG_WRITE, 0x6b4, 0x00},
	{FLG_WRITE, 0x6b4, 0x30},
	//
	//
	//
	////ADC EN I&Q
	{FLG_WRITE, 0x6b5, 0xee},
	////ADC DIV=3
	//
	//////////////////////////////////////////ADC OFST_CAL
	////./ad9361 w 0x6b6 0x84
	//
	{FLG_DELAY, 5, 0x0},
	//
	////./ad9361 r 0x856
	////./ad9361 r 0x856
	////./ad9361 r 0x856
	////./ad9361 r 0x856
	//
	////./ad9361 w 0x6b6 0x80
	//
	//
	///////////////////////////////////////////////GAIN_CAL
	////./ad9361 w 0x6b6 0x82
	//
	////spi read to take up time for postive cal
	////#wait  500
	////./ad9361 r 0x856
	////./ad9361 r 0x856
	////./ad9361 r 0x856
	////./ad9361 r 0x856
	////#wait  500
	//
	////wait until postive cal done
	////negtive cal begin
	////./ad9361 w 0x6b7 0xdf
	{FLG_DELAY, 5, 0x0},
	//
	////spi read to take up time for negtive cal
	////./ad9361 r 0x856
	////./ad9361 r 0x856
	////./ad9361 r 0x856
	////./ad9361 r 0x856
	//
	////./ad9361 w 0x6b6 0x80
	////./ad9361 w 0x6b7 0x00
	//
	//
	//
	//// sel sub1 data out
	////./ad9361 w 0x83b 0x01
	//
	////CTRL   ADC_I_DATA_OUT
	////./ad9361 w 0x0E2 0x01
	////./ad9361 w 0x0E3 0x06
	//
	////CTRL   ADC_Q_DATA_OUT
	////./ad9361 w 0x0E2 0x02
	////./ad9361 w 0x0E3 0x06
	//
	//
	//
	//
};
const int G_REG_ARRAY_Power_Up_E_3_RX1_ADC_with_ofst_gain_cal_LEN = sizeof(G_REG_ARRAY_Power_Up_E_3_RX1_ADC_with_ofst_gain_cal)/sizeof(G_REG_ARRAY_Power_Up_E_3_RX1_ADC_with_ofst_gain_cal[0]);


const reg_t G_REG_ARRAY_Power_Up_E_4_RX2_ADC_with_ofst_gain_cal[] = 
{
	////ADC LDO on
	////./ad9361 w 0x803 0x0F
	////./ad9361 w 0x805 0x07
	//
	//
	////ADC LDO BYPASS
	////./ad9361 w 0x81d 0x01
	//
	//
	////ADC OFF I&Q
	{FLG_WRITE, 0x6b6, 0x80},
	{FLG_WRITE, 0x6b5, 0x00},
	//
	////SYSPLL to RXADC
	////./ad9361 w 0x600 0x03
	////./ad9361 w 0x6f0 0x03
	//
	////ADC RST
	{FLG_WRITE, 0x6b4, 0x30},
	{FLG_WRITE, 0x6b4, 0x00},
	{FLG_WRITE, 0x6b4, 0x00},
	{FLG_WRITE, 0x6b4, 0x00},
	{FLG_WRITE, 0x6b4, 0x30},
	//
	//
	//
	////ADC EN I&Q
	{FLG_WRITE, 0x6b5, 0xee},
	////ADC DIV=3
	//
	//////////////////////////////////////////ADC OFST_CAL
	////./ad9361 w 0x6b6 0x84
	//
	{FLG_DELAY, 5, 0x0},
	//
	////./ad9361 r 0x856
	////./ad9361 r 0x856
	////./ad9361 r 0x856
	////./ad9361 r 0x856
	//
	////./ad9361 w 0x6b6 0x80
	//
	//
	///////////////////////////////////////////////GAIN_CAL
	////./ad9361 w 0x6b6 0x82
	//
	////spi read to take up time for postive cal
	////#wait  500
	////./ad9361 r 0x856
	////./ad9361 r 0x856
	////./ad9361 r 0x856
	////./ad9361 r 0x856
	////#wait  500
	//
	////wait until postive cal done
	////negtive cal begin
	////./ad9361 w 0x6b7 0xdf
	////#wait  500
	//
	////spi read to take up time for negtive cal
	////./ad9361 r 0x856
	////./ad9361 r 0x856
	////./ad9361 r 0x856
	////./ad9361 r 0x856
	//
	////./ad9361 w 0x6b6 0x80
	////./ad9361 w 0x6b7 0x00
	//
	//
	//
	//// sel sub1 data out
	////./ad9361 w 0x83b 0x01
	//
	////CTRL   ADC_I_DATA_OUT
	////./ad9361 w 0x0E2 0x01
	////./ad9361 w 0x0E3 0x06
	//
	////CTRL   ADC_Q_DATA_OUT
	////./ad9361 w 0x0E2 0x02
	////./ad9361 w 0x0E3 0x06
	//
	//
	//
	//
};
const int G_REG_ARRAY_Power_Up_E_4_RX2_ADC_with_ofst_gain_cal_LEN = sizeof(G_REG_ARRAY_Power_Up_E_4_RX2_ADC_with_ofst_gain_cal)/sizeof(G_REG_ARRAY_Power_Up_E_4_RX2_ADC_with_ofst_gain_cal[0]);


const reg_t G_REG_ARRAY_Power_Up_E_5_TXDAC_TX1[] = 
{
	{FLG_WRITE, 0x791, 0x6F},
	{FLG_WRITE, 0x791, 0x7F},
};
const int G_REG_ARRAY_Power_Up_E_5_TXDAC_TX1_LEN = sizeof(G_REG_ARRAY_Power_Up_E_5_TXDAC_TX1)/sizeof(G_REG_ARRAY_Power_Up_E_5_TXDAC_TX1[0]);


const reg_t G_REG_ARRAY_Power_Up_E_6_TXDAC_TX2[] = 
{
	{FLG_WRITE, 0x7a4, 0x6F},
	{FLG_WRITE, 0x7a4, 0x7F},
};
const int G_REG_ARRAY_Power_Up_E_6_TXDAC_TX2_LEN = sizeof(G_REG_ARRAY_Power_Up_E_6_TXDAC_TX2)/sizeof(G_REG_ARRAY_Power_Up_E_6_TXDAC_TX2[0]);


const reg_t G_REG_ARRAY_Power_Up_E_7_TX1_BBF_L20_IQM_HRMDIV4_PA_PORTA_IFLPBK_Remove_702[] = 
{
	////SXTX & TX1 related LDO enable
	////./ad9361 w 0x804 0x1F
	////./ad9361 w 0x806 0x0F
	//
	//
	////TXDAC Init and calibration
	//
	////./ad9361 w 0x790 0x01
	////./ad9361 w 0x791 0x0F
	////./ad9361 w 0x791 0x2F
	//
	////changed for GSIIC1
	//{FLG_WRITE, 0x604, 0xB},
	//
	////TXDAC CAL
	////./ad9361 w 790 7
	//
	{FLG_DELAY, 50, 0x0},
	//
	{FLG_WRITE, 0x790, 0x1},
	//
	////TX2 RVDD18 OFF, for LO spurs
	////./ad9361 w 0x801 0xED
	//
	////TX chain
	//
	{FLG_WRITE, 0x747, 0x01},
	{FLG_WRITE, 0x700, 0x00},
	////./ad9361 w 0x702 0x0F
	{FLG_WRITE, 0x703, 0x07},
	//
	//
	{FLG_WRITE, 0x706, 0x10},
	{FLG_WRITE, 0x707, 0x10},
	//
	{FLG_WRITE, 0x72A, 0x01},
	//
	////D1
	{FLG_WRITE, 0x708, 0x00},
	{FLG_WRITE, 0x709, 0x18},
	{FLG_WRITE, 0x70A, 0x18},
	{FLG_WRITE, 0x70B, 0xC0},
	{FLG_WRITE, 0x70C, 0x10},
	{FLG_WRITE, 0x70F, 0x00},
	{FLG_WRITE, 0x710, 0x08},
	////Disable RBQ/CBQ
	{FLG_WRITE, 0x719, 0x00},
	{FLG_WRITE, 0x705, 0x00},
	{FLG_WRITE, 0x738, 0x80},
	{FLG_WRITE, 0x739, 0x84},
	////band LUT 2.6GHz
	{FLG_WRITE, 0x714, 0xD0},
	{FLG_WRITE, 0x734, 0x80},
	{FLG_WRITE, 0x735, 0x80},
	{FLG_WRITE, 0x736, 0x80},
	{FLG_WRITE, 0x708, 0x00},
	{FLG_WRITE, 0x709, 0x18},
	{FLG_WRITE, 0x70A, 0x18},
	{FLG_WRITE, 0x71D, 0x1A},
	{FLG_WRITE, 0x73D, 0x70},
	{FLG_WRITE, 0x73E, 0x70},
	{FLG_WRITE, 0x73F, 0x70},
	{FLG_WRITE, 0x71B, 0x23},
	{FLG_WRITE, 0x71C, 0x23},
	{FLG_WRITE, 0x720, 0x23},
	{FLG_WRITE, 0x721, 0x23},
	{FLG_WRITE, 0x73B, 0x11},
	{FLG_WRITE, 0x73C, 0x88},
	{FLG_WRITE, 0x740, 0x11},
	////PA gain LUT 2.6GHz
	{FLG_WRITE, 0x71E, 0x0},
	{FLG_WRITE, 0x71F, 0x40},
	{FLG_WRITE, 0x722, 0x0},
	{FLG_WRITE, 0x723, 0x0},
	{FLG_WRITE, 0x724, 0x0},
	{FLG_WRITE, 0x725, 0xFF},
	{FLG_WRITE, 0x726, 0x32},
	{FLG_WRITE, 0x727, 0x3},
	{FLG_WRITE, 0x728, 0xC0},
	{FLG_WRITE, 0x729, 0x40},
	//
	//
	////Turn off core2 DLL
	{FLG_WRITE, 0x701, 0x37},
	//
	//
	////OP2 IOUT
	{FLG_WRITE, 0x711, 0x37},
	{FLG_WRITE, 0x712, 0x37},
	//
	//
};
const int G_REG_ARRAY_Power_Up_E_7_TX1_BBF_L20_IQM_HRMDIV4_PA_PORTA_IFLPBK_Remove_702_LEN = sizeof(G_REG_ARRAY_Power_Up_E_7_TX1_BBF_L20_IQM_HRMDIV4_PA_PORTA_IFLPBK_Remove_702)/sizeof(G_REG_ARRAY_Power_Up_E_7_TX1_BBF_L20_IQM_HRMDIV4_PA_PORTA_IFLPBK_Remove_702[0]);


const reg_t G_REG_ARRAY_Power_Up_E_8_TX2_BBF_L20_IQM_HRMDIV4_PA_PORTA_IFLPBK_Remove_74A[] = 
{
	////SXTX & TX1 related LDO enable
	////./ad9361 w 0x804 0x1f
	////./ad9361 w 0x806 0xf0
	//
	//
	////TXDAC
	//
	////./ad9361 w 0x7a3 0x01
	////./ad9361 w 0x7a4 0x0f
	////./ad9361 w 0x7a4 0x2f
	//{FLG_WRITE, 0x604, 0x0b},
	//
	////TXDAC CAL
	////./ad9361 w 0x7a3 0x07
	//
	{FLG_DELAY, 50, 0x0},
	//
	{FLG_WRITE, 0x7a3, 0x01},
	//
	////TX2 RVDD18 OFF, for LO spurs
	////./ad9361 w 0x801 0xed
	//
	////TX chain
	//
	{FLG_WRITE, 0x78f, 0x01},
	{FLG_WRITE, 0x748, 0x00},
	////./ad9361 w 0x74a 0x0f
	{FLG_WRITE, 0x74b, 0x07},
	//
	//
	{FLG_WRITE, 0x74e, 0x10},
	{FLG_WRITE, 0x74f, 0x10},
	//
	{FLG_WRITE, 0x772, 0x01},
	//
	////D1
	{FLG_WRITE, 0x750, 0x00},
	{FLG_WRITE, 0x751, 0x18},
	{FLG_WRITE, 0x752, 0x18},
	{FLG_WRITE, 0x753, 0xC0},
	{FLG_WRITE, 0x754, 0x10},
	{FLG_WRITE, 0x757, 0x00},
	{FLG_WRITE, 0x758, 0x08},
	////Disable RBQ/CBQ
	{FLG_WRITE, 0x761, 0x00},
	{FLG_WRITE, 0x74D, 0x00},
	{FLG_WRITE, 0x780, 0x80},
	{FLG_WRITE, 0x781, 0x84},
	////band LUT 2.6GHz
	{FLG_WRITE, 0x75C, 0xD0},
	{FLG_WRITE, 0x77C, 0x80},
	{FLG_WRITE, 0x77D, 0x80},
	{FLG_WRITE, 0x77E, 0x80},
	{FLG_WRITE, 0x750, 0x00},
	{FLG_WRITE, 0x751, 0x18},
	{FLG_WRITE, 0x752, 0x18},
	{FLG_WRITE, 0x765, 0x1A},
	{FLG_WRITE, 0x785, 0x70},
	{FLG_WRITE, 0x786, 0x70},
	{FLG_WRITE, 0x787, 0x70},
	{FLG_WRITE, 0x763, 0x23},
	{FLG_WRITE, 0x764, 0x23},
	{FLG_WRITE, 0x768, 0x23},
	{FLG_WRITE, 0x769, 0x23},
	{FLG_WRITE, 0x783, 0x11},
	{FLG_WRITE, 0x784, 0x88},
	{FLG_WRITE, 0x788, 0x11},
	////PA gain LUT 2.6GHz
	{FLG_WRITE, 0x766, 0x0},
	{FLG_WRITE, 0x767, 0x40},
	{FLG_WRITE, 0x76A, 0x0},
	{FLG_WRITE, 0x76B, 0x0},
	{FLG_WRITE, 0x76C, 0x0},
	{FLG_WRITE, 0x76D, 0xFF},
	{FLG_WRITE, 0x76E, 0x32},
	{FLG_WRITE, 0x76F, 0x3},
	{FLG_WRITE, 0x770, 0x0},
	{FLG_WRITE, 0x771, 0x40},
	//
	////Turn off core2 DLL
	{FLG_WRITE, 0x749, 0x37},
	//
	//
	////OP2 IOUT
	{FLG_WRITE, 0x759, 0x37},
	{FLG_WRITE, 0x75A, 0x37},
};
const int G_REG_ARRAY_Power_Up_E_8_TX2_BBF_L20_IQM_HRMDIV4_PA_PORTA_IFLPBK_Remove_74A_LEN = sizeof(G_REG_ARRAY_Power_Up_E_8_TX2_BBF_L20_IQM_HRMDIV4_PA_PORTA_IFLPBK_Remove_74A)/sizeof(G_REG_ARRAY_Power_Up_E_8_TX2_BBF_L20_IQM_HRMDIV4_PA_PORTA_IFLPBK_Remove_74A[0]);


const reg_t G_REG_ARRAY_Power_Up_E_0_4_LO_CONFIG_2T2R_TDD[] = 
{
	//
	////SXTX to RX1RX2, TX1TX2
	{FLG_WRITE, 0x6D1, 0x5F},
	{FLG_WRITE, 0x6E1, 0x5F},
	{FLG_WRITE, 0x702, 0x2F},
	{FLG_WRITE, 0x74A, 0x2F},
	{FLG_WRITE, 0x639, 0xFC},
	//
	////turn off SXRX LDO and SXRX
	{FLG_WRITE, 0x803, 0x07},
	{FLG_WRITE, 0x620, 0x10},
	{FLG_WRITE, 0x61F, 0x00},
};
const int G_REG_ARRAY_Power_Up_E_0_4_LO_CONFIG_2T2R_TDD_LEN = sizeof(G_REG_ARRAY_Power_Up_E_0_4_LO_CONFIG_2T2R_TDD)/sizeof(G_REG_ARRAY_Power_Up_E_0_4_LO_CONFIG_2T2R_TDD[0]);


/****************************************************************************************/
// wait a: FSM Wait State Entry and Manual-Control Enable
/****************************************************************************************/
const reg_t G_REG_ARRAY_Wait_A_1_FORECE_WAIT[] = 
{
	////LDO_FC_LOAD
	//{FLG_WRITE, 0x0cc, 0x00},
	//{FLG_WRITE, 0x0cc, 0x01},
	//{FLG_WRITE, 0x0cc, 0x00},
	//
	//
	////FDD -> WAIT
	{FLG_WRITE, 0x0cc, 0x08},
	{FLG_WRITE, 0x0cc, 0x09},
	{FLG_WRITE, 0x0cc, 0x08},
	{FLG_WRITE, 0x0cc, 0x08},
	//
	////Change State1 time to 11
	{FLG_WRITE, 0x0D1, 0xC0},
	//
	////RX LO protection
	////RX1 Manual on + DIV2 off
	//{FLG_WRITE, 0x650, 0x01},
	//{FLG_WRITE, 0x660, 0x00},
	//{FLG_WRITE, 0x661, 0x00},
	////RX2 Manual on + DIV2 off
	//{FLG_WRITE, 0x66E, 0x01},
	//{FLG_WRITE, 0x67E, 0x00},
	//{FLG_WRITE, 0x67F, 0x00},
	//
	{FLG_DELAY, 5, 0x0},
	//
	////All LDO on
	{FLG_WRITE, 0x805, 0xFF},
	{FLG_WRITE, 0x806, 0xFF},
};
const int G_REG_ARRAY_Wait_A_1_FORECE_WAIT_LEN = sizeof(G_REG_ARRAY_Wait_A_1_FORECE_WAIT)/sizeof(G_REG_ARRAY_Wait_A_1_FORECE_WAIT[0]);


const reg_t G_REG_ARRAY_Wait_A_5_RG_FC_TURN_OFF[] = 
{
	////./ad9361 w 0x901 0xC0
	{FLG_WRITE, 0x901, 0xC0},
	//
	{FLG_WRITE, 0x614, 0x00},
	////./ad9361 w 0x614 0x04
};
const int G_REG_ARRAY_Wait_A_5_RG_FC_TURN_OFF_LEN = sizeof(G_REG_ARRAY_Wait_A_5_RG_FC_TURN_OFF)/sizeof(G_REG_ARRAY_Wait_A_5_RG_FC_TURN_OFF[0]);

/****************************************************************************************/
// wait h: digital config
/****************************************************************************************/
const reg_t G_REG_ARRAY_Wait_H_1_DIG_MANUAL_OFF_RSNT[] = 
{
	//
	//
	////gate bb intf clk & state
	{FLG_WRITE, 0x0de, 0x00},
	{FLG_WRITE, 0x0dc, 0x60},
	//
	//// quit manual 
	{FLG_WRITE, 0x0dd, 0xf0},
	//
	////bb_intf soft  rstn only
	{FLG_WRITE, 0x0f7, 0x07},
	{FLG_WRITE, 0x0f7, 0x05},
	{FLG_WRITE, 0x0f7, 0x07},
	//
	////fsm state control 
	{FLG_WRITE, 0x0cd, 0x00},
	{FLG_WRITE, 0x0ce, 0x00},
	//{FLG_WRITE, 0x0cf, 0x00},
	//
	//
	////d7 bit[7] cnt_flush_mode_lock_spi
	////./ad9361 w 0x0d7 0x43
	{FLG_WRITE, 0x0d7, 0xC3},
	//
	//////ensm soft rstn only
	////./ad9361 w 0x0f7 07
	////./ad9361 w 0x0f7 06
	////./ad9361 w 0x0f7 07
};
const int G_REG_ARRAY_Wait_H_1_DIG_MANUAL_OFF_RSNT_LEN = sizeof(G_REG_ARRAY_Wait_H_1_DIG_MANUAL_OFF_RSNT)/sizeof(G_REG_ARRAY_Wait_H_1_DIG_MANUAL_OFF_RSNT[0]);


/****************************************************************************************/
// wait i: manual-control turn-off
/****************************************************************************************/
const reg_t G_REG_ARRAY_Wait_I_1_ANA_MANUAL_OFF[] = 
{
	////LDO 
	{FLG_WRITE, 0x816, 0x00},
	//
	////SYSPLL
	{FLG_WRITE, 0x600, 0x02},
	//
	////SXRX
	{FLG_WRITE, 0x634, 0x00},
	//
	////SXTX
	{FLG_WRITE, 0x64D, 0x00},
	//
	////RX1
	{FLG_WRITE, 0x650, 0x00},
	//
	////RX2
	{FLG_WRITE, 0x66E, 0x00},
	//
	////RX1_ADC
	{FLG_WRITE, 0x698, 0x00},
	//
	////RX2_ADC
	{FLG_WRITE, 0x6B6, 0x00},
	//
	////TX1
	{FLG_WRITE, 0x747, 0x00},
	//
	////TX2
	{FLG_WRITE, 0x78F, 0x00},
	//
	////TX1_DAC
	{FLG_WRITE, 0x790, 0x00},
	//
	////TX2_DAC
	{FLG_WRITE, 0x7A3, 0x00},
	//
	//
	////Turn off DAC1 DC
	{FLG_WRITE, 0x7C2, 0x00},
	{FLG_WRITE, 0x7C3, 0x00},
	{FLG_WRITE, 0x7C4, 0x00},
	{FLG_WRITE, 0x7C5, 0x00},
	//
	////Turn off DAC2 DC
	{FLG_WRITE, 0x7D2, 0x00},
	{FLG_WRITE, 0x7D3, 0x00},
	{FLG_WRITE, 0x7D4, 0x00},
	{FLG_WRITE, 0x7D5, 0x00},
	//
	//
	////RX LO protection
	////RX1 Manual on + DIV2 off
	//{FLG_WRITE, 0x650, 0x01},
	//{FLG_WRITE, 0x660, 0x00},
	//{FLG_WRITE, 0x661, 0x00},
	//
	////RX2 Manual on + DIV2 off
	//{FLG_WRITE, 0x66E, 0x01},
	//{FLG_WRITE, 0x67E, 0x00},
	//{FLG_WRITE, 0x67F, 0x00},
	//
	{FLG_WRITE, 0xD05, 0x63},
};
const int G_REG_ARRAY_Wait_I_1_ANA_MANUAL_OFF_LEN = sizeof(G_REG_ARRAY_Wait_I_1_ANA_MANUAL_OFF)/sizeof(G_REG_ARRAY_Wait_I_1_ANA_MANUAL_OFF[0]);

/****************************************************************************************/
// ENSM Control: fdd
/****************************************************************************************/
const reg_t G_REG_ARRAY_DIG_2_FORCE_AlERT[] = 
{
	//
	//
	////WAIT -> FDD ALERT
	////./ad9361 w 0x0cc  0x09
	//
	{FLG_WRITE, 0x0cc, 0x48},
	//
	//
	//
	//{FLG_DELAY, 500, 0x0},
	//
	//
	//
	//
	//
	//
	//
	//
	//
	//
};
const int G_REG_ARRAY_DIG_2_FORCE_AlERT_LEN = sizeof(G_REG_ARRAY_DIG_2_FORCE_AlERT)/sizeof(G_REG_ARRAY_DIG_2_FORCE_AlERT[0]);


const reg_t G_REG_ARRAY_DIG_3A_FSM_STATE_EN_FDD_1T1R[] = 
{
	////swap port=1 sdr cmos fdd dual_port full_port 1r1t   tx upsampleration 8 rx downsampleration 8  clk div 4
	////delay to make sure Alert_state is ready
	//
	//{FLG_DELAY, 500, 0x0},
	////delay
	////FDD Enable
	{FLG_WRITE, 0x0cc, 0x4a},
	//{FLG_DELAY, 500, 0x0},
	{FLG_READ, 0x0da, 0x0},
	//
	//
	////FDD -> WAIT
	////./ad9361 w 0x0cc  0x08
	//
	////FDD -> ALERT
	////./ad9361 w 0x0cc  0x28
	//
	//
	////WAIT -> TDD ALERT
	////./ad9361 w 0x0cc  0x40
	//
	////TDD_TX Enable
	////./ad9361 w 0x0cc  0x02
	//
	////TDD_RX Enable
	////./ad9361 w 0x0cc  0x04
	//
	////TDD -> WAIT
	////./ad9361 w 0x0cc  0x00
	//
	////TDD -> ALERT
	////./ad9361 w 0x0cc  0x20
	//
	////RX LO protection: RX manual off
	//{FLG_WRITE, 0x650, 0x00},
	//{FLG_WRITE, 0x66E, 0x00},
	//
	//{FLG_DELAY, 500, 0x0},
	//
	{FLG_FPGA_W, 0x533, 0x34},
};
const int G_REG_ARRAY_DIG_3A_FSM_STATE_EN_FDD_1T1R_LEN = sizeof(G_REG_ARRAY_DIG_3A_FSM_STATE_EN_FDD_1T1R)/sizeof(G_REG_ARRAY_DIG_3A_FSM_STATE_EN_FDD_1T1R[0]);


const reg_t G_REG_ARRAY_DIG_RX1_mgc_start_maxium_gain[] = 
{
	{FLG_WRITE, 0xE27, 0x0C},
	{FLG_WRITE, 0xE00, 0x14},
	{FLG_WRITE, 0xE01, 0x03},
	{FLG_WRITE, 0xE02, 0x80},
	//{FLG_WRITE, 0xE03, 0x00},//fdd del here, but under tdisk tdd use the config
	//{FLG_WRITE, 0xE04, 0x00},//fdd del here, but under tdisk tdd use the config
	{FLG_WRITE, 0xE04, 0x00},
	{FLG_WRITE, 0xE05, 0x00},
	{FLG_WRITE, 0xE06, 0x0D},
	{FLG_WRITE, 0xE28, 0x5C},
	{FLG_WRITE, 0xE0B, 0x2A},
	{FLG_WRITE, 0xE2A, 0x00},
	{FLG_WRITE, 0xE0C, 0x64},
	{FLG_WRITE, 0xE29, 0x00},
	{FLG_WRITE, 0xE27, 0x0C},
	{FLG_WRITE, 0xE0D, 0x09},
	{FLG_WRITE, 0xE19, 0xFA},
	{FLG_WRITE, 0xE1A, 0xF0},
	{FLG_WRITE, 0xE1B, 0xC8},
	{FLG_WRITE, 0xE1C, 0x14},
	{FLG_WRITE, 0xE1D, 0xFA},
	{FLG_WRITE, 0xE1E, 0xF0},
	{FLG_WRITE, 0xE1F, 0xC8},
	{FLG_WRITE, 0xE20, 0x14},
	{FLG_WRITE, 0xE21, 0x00},
	{FLG_WRITE, 0xE0D, 0x09},
	{FLG_WRITE, 0xE0F, 0x18},
	{FLG_WRITE, 0xE10, 0xFA},
	{FLG_WRITE, 0xE11, 0xA0},
	{FLG_WRITE, 0xE12, 0x14},
	{FLG_WRITE, 0xE13, 0x14},
	{FLG_WRITE, 0xE14, 0x14},
	{FLG_WRITE, 0xE15, 0x14},
	{FLG_WRITE, 0xE16, 0x00},
	{FLG_WRITE, 0xE17, 0x00},
	{FLG_WRITE, 0xE18, 0x00},
	{FLG_WRITE, 0xE0E, 0x0C},
	{FLG_WRITE, 0xE22, 0x14},
	{FLG_WRITE, 0xE23, 0x3C},
	{FLG_WRITE, 0xE24, 0x14},
	{FLG_WRITE, 0xE25, 0x45},
	{FLG_WRITE, 0xE53, 0x00},
	{FLG_WRITE, 0xE54, 0x00},
	{FLG_WRITE, 0xE55, 0x00},
	{FLG_WRITE, 0xE56, 0x00},
	{FLG_WRITE, 0xE57, 0x00},
	{FLG_WRITE, 0xE53, 0x40},
	{FLG_WRITE, 0xE55, 0x5C},
	{FLG_WRITE, 0xE56, 0x00},
	{FLG_WRITE, 0xE53, 0x40},
	{FLG_WRITE, 0xE53, 0xC0},
	{FLG_WRITE, 0xE53, 0x40},
};
const int G_REG_ARRAY_DIG_RX1_mgc_start_maxium_gain_LEN = sizeof(G_REG_ARRAY_DIG_RX1_mgc_start_maxium_gain)/sizeof(G_REG_ARRAY_DIG_RX1_mgc_start_maxium_gain[0]);


const reg_t G_REG_ARRAY_17C_MGC_DIGITAL_CFG_MAX_GAIN[] = 
{
	{FLG_WRITE, 0xEA7, 0x0C},
	{FLG_WRITE, 0xE80, 0x14},
	{FLG_WRITE, 0xE81, 0x03},
	{FLG_WRITE, 0xE82, 0x80},
	//{FLG_WRITE, 0xE83, 0x00},//fdd del here, but under tdisk tdd use the config
	//{FLG_WRITE, 0xE84, 0x00},//fdd del here, but under tdisk tdd use the config
	{FLG_WRITE, 0xE84, 0x00},
	{FLG_WRITE, 0xE85, 0x00},
	{FLG_WRITE, 0xE86, 0x0D},
	{FLG_WRITE, 0xEA8, 0x5C},
	{FLG_WRITE, 0xE8B, 0x2A},
	{FLG_WRITE, 0xEAA, 0x00},
	{FLG_WRITE, 0xE8C, 0x64},
	{FLG_WRITE, 0xEA9, 0x00},
	{FLG_WRITE, 0xEA7, 0x0C},
	{FLG_WRITE, 0xE8D, 0x09},
	{FLG_WRITE, 0xE99, 0xFA},
	{FLG_WRITE, 0xE9A, 0xF0},
	{FLG_WRITE, 0xE9B, 0xC8},
	{FLG_WRITE, 0xE9C, 0x14},
	{FLG_WRITE, 0xE9D, 0xFA},
	{FLG_WRITE, 0xE9E, 0xF0},
	{FLG_WRITE, 0xE9F, 0xC8},
	{FLG_WRITE, 0xEA0, 0x14},
	{FLG_WRITE, 0xEA1, 0x00},
	{FLG_WRITE, 0xE8D, 0x09},
	{FLG_WRITE, 0xE8F, 0x18},
	{FLG_WRITE, 0xE90, 0xFA},
	{FLG_WRITE, 0xE91, 0xA0},
	{FLG_WRITE, 0xE92, 0x14},
	{FLG_WRITE, 0xE93, 0x14},
	{FLG_WRITE, 0xE94, 0x14},
	{FLG_WRITE, 0xE95, 0x14},
	{FLG_WRITE, 0xE96, 0x00},
	{FLG_WRITE, 0xE97, 0x00},
	{FLG_WRITE, 0xE98, 0x00},
	{FLG_WRITE, 0xE8E, 0x0C},
	{FLG_WRITE, 0xEA2, 0x14},
	{FLG_WRITE, 0xEA3, 0x3C},
	{FLG_WRITE, 0xEA4, 0x14},
	{FLG_WRITE, 0xEA5, 0x45},
	{FLG_WRITE, 0xED3, 0x00},
	{FLG_WRITE, 0xED4, 0x00},
	{FLG_WRITE, 0xED5, 0x00},
	{FLG_WRITE, 0xED6, 0x00},
	{FLG_WRITE, 0xED7, 0x00},
	{FLG_WRITE, 0xED3, 0x40},
	{FLG_WRITE, 0xED5, 0x5C},
	{FLG_WRITE, 0xED6, 0x00},
	{FLG_WRITE, 0xED3, 0x40},
	{FLG_WRITE, 0xED3, 0xc0},
	{FLG_WRITE, 0xED3, 0x40},
};
const int G_REG_ARRAY_17C_MGC_DIGITAL_CFG_MAX_GAIN_LEN = sizeof(G_REG_ARRAY_17C_MGC_DIGITAL_CFG_MAX_GAIN)/sizeof(G_REG_ARRAY_17C_MGC_DIGITAL_CFG_MAX_GAIN[0]);


const reg_t G_REG_ARRAY_DIG_2_2_FDD_FORCE_AlERT[] = 
{
	////WAIT -> ALERT WITH FORCE_ALERT
	{FLG_FPGA_W, 0x100, 0x34},
	{FLG_WRITE, 0x0cc, 0x68},
	//
	////ALERT -> FDD
	{FLG_WRITE, 0x0cc, 0x6a},
	//
	////FDD -> ALERT
	{FLG_WRITE, 0x0cc, 0x68},
	//
	////RX LO protection
	////RX1 Manual on + DIV2 off
	//{FLG_WRITE, 0x650, 0x01},
	//{FLG_WRITE, 0x660, 0x00},
	//{FLG_WRITE, 0x661, 0x00},
	//
	////RX2 Manual on + DIV2 off
	//{FLG_WRITE, 0x66E, 0x01},
	//{FLG_WRITE, 0x67E, 0x00},
	//{FLG_WRITE, 0x67F, 0x00},
	//
	//
	//{FLG_DELAY, 500, 0x0},
};
const int G_REG_ARRAY_DIG_2_2_FDD_FORCE_AlERT_LEN = sizeof(G_REG_ARRAY_DIG_2_2_FDD_FORCE_AlERT)/sizeof(G_REG_ARRAY_DIG_2_2_FDD_FORCE_AlERT[0]);



/****************************************************************************************/
// ENSM Control: tdd
/****************************************************************************************/
const reg_t G_REG_ARRAY_TDD_wirecontrol_ENABLE[] = 
{
	////pin control =1 & to alert = 0
	{FLG_WRITE, 0x0cc, 0x80},
	//
	//
	//
};
const int G_REG_ARRAY_TDD_wirecontrol_ENABLE_LEN = sizeof(G_REG_ARRAY_TDD_wirecontrol_ENABLE)/sizeof(G_REG_ARRAY_TDD_wirecontrol_ENABLE[0]);


const reg_t G_REG_ARRAY_TDD_wirecontrol_WAIT_TO_ALERT[] = 
{
	////WAIT -> TDD ALERT
	////./ad9361 w 0x0cc  0x40
	{FLG_FPGA_W, 0x00, 0x0c},
	////enable rise
	{FLG_FPGA_W, 0x10, 0x0c},
	{FLG_FPGA_W, 0x00, 0x0c},
	//
	{FLG_READ, 0x0da, 0x0},
	{FLG_READ, 0x0da, 0x0},
	{FLG_READ, 0x0da, 0x0},
	{FLG_READ, 0x0da, 0x0},
	//
};
const int G_REG_ARRAY_TDD_wirecontrol_WAIT_TO_ALERT_LEN = sizeof(G_REG_ARRAY_TDD_wirecontrol_WAIT_TO_ALERT)/sizeof(G_REG_ARRAY_TDD_wirecontrol_WAIT_TO_ALERT[0]);


const reg_t G_REG_ARRAY_TDD_wirecontrol_ALERT_TO_RX[] = 
{
	////TDD ALERT -> TDD RX
	////./ad9361 w 0x0cc  0x04
	{FLG_FPGA_W, 0x00, 0x0c},
	////enable rise
	{FLG_FPGA_W, 0x10, 0x0c},
	{FLG_FPGA_W, 0x00, 0x0c},
	//
	//
	////RX LO protection: RX manual off
	//{FLG_WRITE, 0x650, 0x00},
	//{FLG_WRITE, 0x66E, 0x00},
};
const int G_REG_ARRAY_TDD_wirecontrol_ALERT_TO_RX_LEN = sizeof(G_REG_ARRAY_TDD_wirecontrol_ALERT_TO_RX)/sizeof(G_REG_ARRAY_TDD_wirecontrol_ALERT_TO_RX[0]);


const reg_t G_REG_ARRAY_TDD_wirecontrol_RX_TO_WAIT[] = 
{
	////TDD RX -> TDD WAIT
	////./ad9361 w 0x0cc  0x00
	{FLG_FPGA_W, 0x00, 0x0c},
	////enable rise
	{FLG_FPGA_W, 0x10, 0x0c},
	{FLG_FPGA_W, 0x00, 0x0c},
	//
	////RX LO protection
	////RX1 Manual on + DIV2 off
	//{FLG_WRITE, 0x650, 0x01},
	//{FLG_WRITE, 0x660, 0x00},
	//{FLG_WRITE, 0x661, 0x00},
	//
	////RX2 Manual on + DIV2 off
	//{FLG_WRITE, 0x66E, 0x01},
	//{FLG_WRITE, 0x67E, 0x00},
	//{FLG_WRITE, 0x67F, 0x00},
	//
	//{FLG_DELAY, 500, 0x0},
	//
};
const int G_REG_ARRAY_TDD_wirecontrol_RX_TO_WAIT_LEN = sizeof(G_REG_ARRAY_TDD_wirecontrol_RX_TO_WAIT)/sizeof(G_REG_ARRAY_TDD_wirecontrol_RX_TO_WAIT[0]);


const reg_t G_REG_ARRAY_TDD_wirecontrol_ALERT_TO_TX[] = 
{
	////TDD ALERT -> TDD TX
	////./ad9361 w 0x0cc  0x02
	{FLG_FPGA_W, 0x20, 0x0c},
	////enable rise
	{FLG_FPGA_W, 0x30, 0x0c},
	{FLG_FPGA_W, 0x20, 0x0c},
};
const int G_REG_ARRAY_TDD_wirecontrol_ALERT_TO_TX_LEN = sizeof(G_REG_ARRAY_TDD_wirecontrol_ALERT_TO_TX)/sizeof(G_REG_ARRAY_TDD_wirecontrol_ALERT_TO_TX[0]);


const reg_t G_REG_ARRAY_TDD_wirecontrol_TX_TO_WAIT[] = 
{
	////TDD TX -> TDD WAIT
	////./ad9361 w 0x0cc  0x00
	{FLG_FPGA_W, 0x20, 0x0c},
	////enable rise
	{FLG_FPGA_W, 0x30, 0x0c},
	{FLG_FPGA_W, 0x20, 0x0c},
	//
	//
	//
	////RX LO protection
	////RX1 Manual on + DIV2 off
	//{FLG_WRITE, 0x650, 0x01},
	//{FLG_WRITE, 0x660, 0x00},
	//{FLG_WRITE, 0x661, 0x00},
	//
	////RX2 Manual on + DIV2 off
	//{FLG_WRITE, 0x66E, 0x01},
	//{FLG_WRITE, 0x67E, 0x00},
	//{FLG_WRITE, 0x67F, 0x00},
};
const int G_REG_ARRAY_TDD_wirecontrol_TX_TO_WAIT_LEN = sizeof(G_REG_ARRAY_TDD_wirecontrol_TX_TO_WAIT)/sizeof(G_REG_ARRAY_TDD_wirecontrol_TX_TO_WAIT[0]);

const reg_t G_REG_ANA_PEAK_RX1_ENABLE[] =
{
	{FLG_WRITE, 0xd2e, 0x0},
	{FLG_WRITE, 0x803, 0x0f},
	{FLG_WRITE, 0x6f1, 0x1b},
	{FLG_WRITE, 0x861, 0x30},
	{FLG_WRITE, 0x861, 0x0},
	{FLG_WRITE, 0x861, 0x0},
	{FLG_WRITE, 0x861, 0x0},
	{FLG_WRITE, 0x861, 0x30},
	{FLG_WRITE, 0x861, 0x71},
	{FLG_WRITE, 0x860, 0x0},
	{FLG_WRITE, 0x865, 0x80},
	{FLG_READ,  0x865, 0x0},
	{FLG_READ,  0x866, 0x0},
	{FLG_WRITE, 0x865, 0x0},
	{FLG_WRITE, 0x82c, 0x84},
	{FLG_WRITE, 0x835, 0x0},
	{FLG_WRITE, 0x843, 0x13},
	{FLG_WRITE, 0x668, 0x90},
};
const int G_REG_ANA_PEAK_RX1_ENABLE_LEN = sizeof(G_REG_ANA_PEAK_RX1_ENABLE)/sizeof(G_REG_ANA_PEAK_RX1_ENABLE[0]);

const reg_t G_REG_ANA_PEAK_RX2_ENABLE[] =
{
	{FLG_WRITE, 0xd2e, 0x0},
	{FLG_WRITE, 0x803, 0x0f},
	{FLG_WRITE, 0x6f1, 0x1b},
	{FLG_WRITE, 0x86a, 0x30},
	{FLG_WRITE, 0x86a, 0x0},
	{FLG_WRITE, 0x86a, 0x0},
	{FLG_WRITE, 0x86a, 0x0},
	{FLG_WRITE, 0x86a, 0x30},
	{FLG_WRITE, 0x86a, 0x71},
	{FLG_WRITE, 0x869, 0x0},
	{FLG_WRITE, 0x86e, 0x80},
	{FLG_READ,  0x86e, 0x0},
	{FLG_READ,  0x86e, 0x0},
	{FLG_WRITE, 0x86e, 0x0},
	{FLG_WRITE, 0x82c, 0x28},
	{FLG_WRITE, 0x835, 0x0},
	{FLG_WRITE, 0x843, 0x1c},
	{FLG_WRITE, 0x686, 0x90},
};
const int G_REG_ANA_PEAK_RX2_ENABLE_LEN = sizeof(G_REG_ANA_PEAK_RX2_ENABLE)/sizeof(G_REG_ANA_PEAK_RX2_ENABLE[0]);
