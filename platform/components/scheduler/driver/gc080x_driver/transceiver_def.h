/**
* Copyright (c) 2023 Geo CHIP INC. All rights reserved.
*
* BSD-3-Clause
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*
* 1. Redistributions of source code must retain the above copyright
*    notice, this list of conditions and the following disclaimer.
*
* 2. Redistributions in binary form must reproduce the above copyright
*    notice, this list of conditions and the following disclaimer in the
*    documentation and/or other materials provided with the distribution.
*
* 3. Neither the name of the copyright holder nor the names of its
*    contributors may be used to endorse or promote products derived from
*    this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
* "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
* LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
* FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
* COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
* INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
* (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
* SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
* HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
* STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
* IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
* POSSIBILITY OF SUCH DAMAGE.
*
* @file       transciver_def.h
* @date       2023-05-08
* @version    v1.0.0
*
*/

#ifndef  __TRANSCIVER_DEF_H
#define  __TRANSCIVER_DEF_H


/*********************************************************************/
/**\ header files */
#ifdef __KERNEL__
#include <linux/types.h>
#include <linux/kernel.h>
#else
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#endif
#include <stdio.h>
#include <stdlib.h>
// #include <string.h>

/*********************************************************************/
/** \name       Common macros                   */
/*********************************************************************/

#if !defined(UINT8_C) && !defined(INT8_C)
#define INT8_C(x)               S8_C(x)
#define UINT8_C(x)              U8_C(x)
#endif

#if !defined(UINT16_C) && !defined(INT16_C)
#define INT16_C(x)              S16_C(x)
#define UINT16_C(x)             U16_C(x)
#endif

#if !defined(INT32_C) && !defined(UINT32_C)
#define INT32_C(x)              S32_C(x)
#define UINT32_C(x)             U32_C(x)
#endif

#if !defined(INT64_C) && !defined(UINT64_C)
#define INT64_C(x)              S64_C(x)
#define UINT64_C(x)             U64_C(x)
#endif


/**\name API success code */
#define TRANSV_OK                               INT32_C(0)

/**\name API error codes */
#define TRANSV_E_NULL_PTR                       INT32_C(-1)
#define TRANSV_E_COM_FAIL                       INT32_C(-2)
#define TRANSV_E_CAL_FAIL                       INT32_C(-3)
#define TRANSV_E_CFG_FAIL                       INT32_C(-4)

/**
 * TRANSV_INTF_RET_TYPE is the read/write interface return type 
 * which can be overwritten by the curent build system.
 */
#ifndef TRANSV_INTF_RET_TYPE
#define TRANSV_INTF_RET_TYPE     int8_t
#endif

/**
 * The last error code from read/write interface is stored 
 * in the device structure as intf_rslt.
 */
#ifndef TRANSV_INTF_RET_SUCCESS
#define TRANSV_INTF_RET_SUCCESS  INT8_C(0)
#endif

/** Mask definitions for enable $RX power dector for $AUXADC */
#define TRANSCV_RXPOWDTR_RXPDET_EN_MASK         UINT8_C(1<<7)
#define TRANSCV_RXPOWDTR_AUXADC2_EN_MASK        UINT8_C(1<<5)
#define TRANSCV_RXPOWDTR_AUXADC1_EN_MASK        UINT8_C(1<<4)

#define TRANSCV_BQOPISEL_BQ20P2_MASK            UINT8_C(7<<1)
#define TRANSCV_BQOPISEL_BQ20P2_SET             UINT8_C(1<<1)

#define TRANSCV_TXDCOFFSETCAL_MAX_MASK          UINT8_C(0x78)

/** Mask definitions for BIT MASK */
#define TRANSCV_REG_BIT0_MASK                   UINT8_C(1<<0)
#define TRANSCV_REG_BIT1_MASK                   UINT8_C(1<<1)
#define TRANSCV_REG_BIT2_MASK                   UINT8_C(1<<2)
#define TRANSCV_REG_BIT3_MASK                   UINT8_C(1<<3)
#define TRANSCV_REG_BIT4_MASK                   UINT8_C(1<<4)
#define TRANSCV_REG_BIT5_MASK                   UINT8_C(1<<5)
#define TRANSCV_REG_BIT6_MASK                   UINT8_C(1<<6)
#define TRANSCV_REG_BIT7_MASK                   UINT8_C(1<<7)

#define TRANSCV_LUT_MAIN_ADDR_START             UINT16_C(0)
#define TRANSCV_LUT_MAIN_ADDR_LENGTH            UINT16_C(280)
#define TRANSCV_LUT_RXIP2CAL_ADDR_START         UINT16_C(0)
#define TRANSCV_LUT_RXIP2CAL_ADDR_LENGTH        UINT16_C(8)
#define TRANSCV_LUT_SXCONFIG_ADDR_START         UINT16_C(0)
#define TRANSCV_LUT_SXCONFIG_ADDR_LENGTH        UINT16_C(432)
#define TRANSCV_LUT_SXCAL_ADDR_START            UINT16_C(0)
#define TRANSCV_LUT_SXCAL_ADDR_LENGTH           UINT16_C(256)
#define TRANSCV_LUT_TXLOCAL_ADDR_START          UINT16_C(0)
#define TRANSCV_LUT_TXLOCAL_ADDR_LENGTH         UINT16_C(32)
#define TRANSCV_LUT_TRXBAND_ADDR_START          UINT16_C(0)
#define TRANSCV_LUT_TRXBAND_ADDR_LENGTH         UINT16_C(32)
#define TRANSCV_LUT_TRXBW_ADDR_START            UINT16_C(0)
#define TRANSCV_LUT_TRXBW_ADDR_LENGTH           UINT16_C(1441)
#define TRANSCV_LUT_TXGAIN_ADDR_START           UINT16_C(0)
#define TRANSCV_LUT_TXGAIN_ADDR_LENGTH          UINT16_C(295)
#define TRANSCV_LUT_TXRSB_ADDR_START            UINT16_C(0)
#define TRANSCV_LUT_TXRSB_ADDR_LENGTH           UINT16_C(1000)



/**
 * Define the constant macro
 */
#define TX_DC_OFFSET_CAL_AUXADC_READ_CNT           (10)
#define R_CAL_CNT_BIT0_IN_REG850                   (33)

/* Auxadc resolution for cal slope */
#define TRANSCV_AUXADC_CAL_RESOLUTION               0.002f

/*!
 * @brief Interface selection enums
 */
enum transceiver_intf {
    /*! I2C interface */
    TRANSV_FPGA_INTF,
    /*! SPI interface */
    TRANSV_SPI_INTF
};


/*!
 * @brief Interface selection enums
 */
typedef enum _xo_type {
    /*! Use the external clock source. */
    TCXO,
    /*! Use the plate-level crystal. */
    XTAL
}xo_type;

/*!
 * @brief AUXADC1 control register structure.
 */
typedef struct _auxadc_ctrl
{
    uint8_t rgRxRstDetr_val; 
    uint8_t rgRxPowDetr_val;     
    uint8_t rg828_val;
    uint8_t rg829_val;
    uint8_t rg82A_val;
    uint8_t rg830_val;
    uint8_t rg835_val;
    uint8_t rg841_val;
    uint8_t rg842_val;
    uint8_t rg843_val;   
}auxadc_ctrl;

#endif  /* __TRANSCIVER_DEF_H */
