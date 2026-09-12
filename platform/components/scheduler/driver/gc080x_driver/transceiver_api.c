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
* @file       transceiver_api.h
* @date       2023-05-08
* @version    v1.0.0
*
*/

#include "transceiver_api.h"
#include "platform.h"
#include "transceiver_def.h"
/*-------------------------------------------------------------------------------------------------*/
// basic APIs for transceiver
/*-------------------------------------------------------------------------------------------------*/

/*!
 * @brief This API initializes the CM module.
 * 
 */
int32_t cm_init(rf_chip_phy_t* phy) {
  int32_t rslt = TRANSV_OK;

  rslt = hal_spi_write_reg(phy, 0x900, 0x07);
  rslt |= hal_spi_write_reg(phy, 0x904, 0xA4);

  return rslt;
}

/*!
 * @brief This API sets analog modules to manual mode.
 * 
 */
int32_t ana_manual_on(rf_chip_phy_t* phy) {
  int32_t rslt = TRANSV_OK;

  rslt = hal_spi_write_reg(phy, 0x816, 0x01);
  rslt |= hal_spi_write_reg(phy, 0x634, 0x04);
  rslt |= hal_spi_write_reg(phy, 0x64D, 0x04);
  rslt |= hal_spi_write_reg(phy, 0x650, 0x01);
  rslt |= hal_spi_write_reg(phy, 0x66E, 0x01);
  rslt |= hal_spi_write_reg(phy, 0x698, 0x80);
  rslt |= hal_spi_write_reg(phy, 0x6B6, 0x80);
  rslt |= hal_spi_write_reg(phy, 0x747, 0x01);
  rslt |= hal_spi_write_reg(phy, 0x78f, 0x01);
  rslt |= hal_spi_write_reg(phy, 0x790, 0x01);
  rslt |= hal_spi_write_reg(phy, 0x7A3, 0x01);

  return rslt;
}

/*!
 * @brief This API sets analog modules to manual off.
 * 
 */
int32_t ana_manual_off(rf_chip_phy_t* phy) {
  int32_t rslt = TRANSV_OK;

  rslt = hal_spi_write_reg(phy, 0x816, 0x00);
  rslt = hal_spi_write_reg(phy, 0x600, 0x02);
  rslt = hal_spi_write_reg(phy, 0x634, 0x00);
  rslt = hal_spi_write_reg(phy, 0x64D, 0x00);
  rslt = hal_spi_write_reg(phy, 0x650, 0x00);
  rslt = hal_spi_write_reg(phy, 0x66E, 0x00);
  rslt = hal_spi_write_reg(phy, 0x698, 0x00);
  rslt = hal_spi_write_reg(phy, 0x6B6, 0x00);
  rslt = hal_spi_write_reg(phy, 0x747, 0x00);
  rslt = hal_spi_write_reg(phy, 0x78F, 0x00);
  rslt = hal_spi_write_reg(phy, 0x790, 0x00);
  rslt = hal_spi_write_reg(phy, 0x7A3, 0x00);
  rslt = hal_spi_write_reg(phy, 0xD05, 0x63);

  return rslt;
}

/*!
 * @brief This API intializes LDOs.
 * 
 */
int32_t ldo_init(rf_chip_phy_t* phy) {
  int32_t rslt = TRANSV_OK;

  rslt = hal_spi_write_reg(phy, 0x802, 0xb9);
  rslt |= hal_spi_write_reg(phy, 0x80B, 0xFA);
  rslt |= hal_spi_write_reg(phy, 0x80C, 0x47);
  rslt |= hal_spi_write_reg(phy, 0x80D, 0xFF);
  rslt |= hal_spi_write_reg(phy, 0x80E, 0xAF);
  rslt |= hal_spi_write_reg(phy, 0x80F, 0x47);
  rslt |= hal_spi_write_reg(phy, 0x810, 0xF4);
  if (phy->config->chip_ver == CHIP_VERSION_E1)
    rslt |= hal_spi_write_reg(phy, 0x811, 0xAA);
  else
    rslt |= hal_spi_write_reg(phy, 0x811, 0x88);
  rslt |= hal_spi_write_reg(phy, 0x813, 0x43);
  rslt |= hal_spi_write_reg(phy, 0x814, 0x44);
  rslt |= hal_spi_write_reg(phy, 0x815, 0x43);
  rslt |= hal_spi_write_reg(phy, 0x807, 0x0f);
  rslt |= hal_spi_write_reg(phy, 0x801, 0xEF);
  rslt |= hal_spi_write_reg(phy, 0x804, 0x1F);
  rslt |= hal_spi_write_reg(phy, 0x806, 0xFF);
  rslt |= hal_spi_write_reg(phy, 0x803, 0x0F);
  rslt |= hal_spi_write_reg(phy, 0x805, 0x3F);

  rslt |= hal_spi_write_reg(phy, 0x817, 0x88);
  rslt |= hal_spi_write_reg(phy, 0x818, 0x48);
  rslt |= hal_spi_write_reg(phy, 0x819, 0x48);
  phy->config->chip_ver = confim_config_info(phy);
  if (phy->config->chip_ver < 0) {
    LOG_ERROR("config error!\n");
    rslt |= TRANSV_E_CFG_FAIL;
    goto error;
  }
  if (phy->config->chip_ver == CHIP_VERSION_D1)
    rslt |= hal_spi_write_reg(phy, 0x81A, 0x28);
  else if (phy->config->chip_ver == CHIP_VERSION_D2)
    rslt |= hal_spi_write_reg(phy, 0x81A, 0x88);
  else
    rslt |= hal_spi_write_reg(phy, 0x81A, 0x88);
  rslt |= hal_spi_write_reg(phy, 0x81B, 0xA8);

error:
  return rslt;
}

/*!
 * @brief This API selects XO mode between crystal and external clock source
 *  for the transceiver chip.
 */
int32_t xo_selt(rf_chip_phy_t* phy, xo_type xo_sel) {
  int32_t rslt = TRANSV_OK;
  uint8_t reg_data;

  reg_data = (xo_sel == TCXO) ? 0x09 : 0x0D;
  rslt     = hal_spi_write_reg(phy, 0x615, reg_data);

  return rslt;
}

/*!
 * @brief This API initializes reference clock X4 module for the transceiver chip.
 *
 */
int32_t x4_init(rf_chip_phy_t* phy) {
  int32_t rslt = TRANSV_OK;

  rslt = hal_spi_write_reg(phy, 0xd2c, 0x26);

  return rslt;
}

/*!
 * @brief This API configures REFCLK of SX into X4 mode or X2 mode
 *  for the transceiver chip.
 */
int32_t refclk_config(rf_chip_phy_t* phy, bool x4_en) {
  int32_t rslt = TRANSV_OK;

  if (x4_en) {
    rslt |= hal_spi_write_reg(phy, 0xD04, 0x4C);
    rslt |= hal_spi_write_reg(phy, 0xD08, 0x39);
    rslt |= hal_spi_write_reg(phy, 0xD05, 0xE3);
    rslt |= hal_spi_write_reg(phy, 0xD06, 0xBC);
    rslt |= hal_spi_write_reg(phy, 0xD07, 0xB3);
    rslt |= hal_spi_write_reg(phy, 0xD13, 0x96);
    rslt |= hal_spi_write_reg(phy, 0xD08, 0xB9);
    rslt |= hal_spi_write_reg(phy, 0xD08, 0x39);
    rslt |= hal_spi_write_reg(phy, 0xD00, 0xB6);
    rslt |= hal_spi_write_reg(phy, 0xD03, 0x8A);
    rslt |= hal_spi_write_reg(phy, 0xD2A, 0x3C);
    rslt |= hal_spi_write_reg(phy, 0xD13, 0xC7);
  } else {
    rslt |= hal_spi_write_reg(phy, 0xD2A, 0x14);
    rslt |= hal_spi_write_reg(phy, 0xD13, 0x16);
    rslt |= hal_spi_write_reg(phy, 0xD00, 0x16);
    rslt |= hal_spi_write_reg(phy, 0xD03, 0x80);
    rslt |= hal_spi_write_reg(phy, 0xD06, 0x3C);
    rslt |= hal_spi_write_reg(phy, 0xD07, 0x33);
    rslt |= hal_spi_write_reg(phy, 0xD05, 0x63);
  }

  return rslt;
}

/*!
 * @brief This API initializes SYSPLL module.
 * 
 */
int32_t syspll_init(rf_chip_phy_t* phy) {
  int32_t rslt = TRANSV_OK;

  rslt = hal_spi_write_reg(phy, 0x605, 0xE8);
  rslt |= hal_spi_write_reg(phy, 0x6f1, 0x15);
  rslt |= hal_spi_write_reg(phy, 0x613, 0x01);
  rslt |= hal_spi_write_reg(phy, 0x601, 0x8f);
  rslt |= hal_spi_write_reg(phy, 0x60b, 0x03);
  rslt |= hal_spi_write_reg(phy, 0x602, 0x00);
  rslt |= hal_spi_write_reg(phy, 0x600, 0x0B);
  rslt |= hal_spi_write_reg(phy, 0x604, 0x0B);
  rslt |= hal_spi_write_reg(phy, 0x60a, 0x30);

  return rslt;
}

/*!
 * @brief This API initializes SX BIAS module.
 * 
 */
int32_t sx_init(rf_chip_phy_t* phy) {
  int32_t rslt = TRANSV_OK;

  if (CHIP_VERSION_E1 == phy->config->chip_ver) {
    //SX cap bank turn of/off BISA
    //reg 0x61A<3:2> and 0x61A<1:0> to be define, set 0x61A<3:2>=0b00 and 0x61A<1:0>=0b11 temporarily
    rslt = hal_spi_write_reg(phy, 0x61A, 0x43);
  } else {
    rslt = hal_spi_write_reg(phy, 0x61A, 0x4C);
  }

  if (CHIP_VERSION_E1 == phy->config->chip_ver) {
    //SX bias RC filter R value to choose
    //reg 0x619<5> to be define, clear 0x619<5> temporarily
    rslt |= hal_spi_write_reg(phy, 0x619, 0x1C & 0xDF);
  } else {
    rslt |= hal_spi_write_reg(phy, 0x619, 0x1C);
  }

  return rslt;
}

/*!
 * @brief This API initializes SXRX module.
 * 
 */
int32_t sxrx_init(rf_chip_phy_t* phy, bool fdd_en) {
  int32_t rslt       = TRANSV_OK;
  uint8_t reg619_val = 0;
  uint8_t reg631_val = 0;
  uint8_t reg633_val = 0;

  rslt = hal_spi_write_reg(phy, 0x631, 0x83);

  if (fdd_en) {
    rslt |= hal_spi_write_reg(phy, 0x61F, 0x1f);
    rslt |= hal_spi_write_reg(phy, 0x61F, 0x9F);
  }

  if (CHIP_VERSION_E1 == phy->config->chip_ver) {
    //0x619<3:0> and 0x619<4> to be define, clear 0x619<3:0> and 0x619<4> temporarily
    reg619_val = hal_spi_read_reg(phy, 0x619);
    reg619_val = reg619_val & 0xc0;
    rslt |= hal_spi_write_reg(phy, 0x619, reg619_val);

    //0x631<7:6> to be define, clear 0x631<7:6> temporarily
    reg631_val = hal_spi_read_reg(phy, 0x631);
    reg631_val = reg631_val & 0x3F;
    rslt |= hal_spi_write_reg(phy, 0x631, reg631_val);

    //0x633<7> to be define, clear 0x633<7> temporarily
    reg633_val = hal_spi_read_reg(phy, 0x633);
    reg633_val = reg633_val & 0x7F;
    rslt |= hal_spi_write_reg(phy, 0x633, reg633_val);
  }

  return rslt;
}

/*!
 * @brief This API initializes SXTX module.
 * 
 */
int32_t sxtx_init(rf_chip_phy_t* phy) {
  int32_t rslt       = TRANSV_OK;
  uint8_t reg61B_val = 0;
  uint8_t reg64A_val = 0;
  uint8_t reg64C_val = 0;

  rslt |= hal_spi_write_reg(phy, 0x64A, 0x83);
  rslt |= hal_spi_write_reg(phy, 0x638, 0x1F);
  rslt |= hal_spi_write_reg(phy, 0x638, 0x9F);

  if (CHIP_VERSION_E1 == phy->config->chip_ver) {
    //0x61B<3:0> and 0x61B<4> to be define, clear 0x61B<3:0> and 0x61B<4> temporarily
    reg61B_val = hal_spi_read_reg(phy, 0x61B);
    reg61B_val = reg61B_val & 0xE0;
    rslt |= hal_spi_write_reg(phy, 0x61B, reg61B_val);

    //0x64A<7:6> to be define, clear 0x64A<7:6> temporarily
    reg64A_val = hal_spi_read_reg(phy, 0x64A);
    reg64A_val = reg64A_val & 0x3F;
    rslt |= hal_spi_write_reg(phy, 0x64A, reg64A_val);

    //0x64C<7> to be define, clear 0x64C<7> temporarily
    reg64C_val = hal_spi_read_reg(phy, 0x64C);
    reg64C_val = reg64C_val & 0x7F;
    rslt |= hal_spi_write_reg(phy, 0x64C, reg64C_val);
  }

  return rslt;
}

/*!
 * @brief This API configures LO into FDD or TDD mode in manual mode.
 * 
 */
int32_t man_lo_config(rf_chip_phy_t* phy, bool fdd_en) {
  int32_t rslt = TRANSV_OK;

  if (fdd_en) {
    rslt = hal_spi_write_reg(phy, 0x639, 0xCC);
    rslt |= hal_spi_write_reg(phy, 0x620, 0x3C);
    rslt |= hal_spi_write_reg(phy, 0x6D1, 0x7F);
    rslt |= hal_spi_write_reg(phy, 0x6E1, 0x7F);
    rslt |= hal_spi_write_reg(phy, 0x702, 0x0F);
    rslt |= hal_spi_write_reg(phy, 0x74A, 0x0F);
  } else {
    rslt = hal_spi_write_reg(phy, 0x6D1, 0x5F);
    rslt |= hal_spi_write_reg(phy, 0x6E1, 0x5F);
    rslt |= hal_spi_write_reg(phy, 0x702, 0x2F);
    rslt |= hal_spi_write_reg(phy, 0x74A, 0x2F);
    rslt |= hal_spi_write_reg(phy, 0x639, 0xFC);
    rslt |= hal_spi_write_reg(phy, 0x803, 0x07);
    rslt |= hal_spi_write_reg(phy, 0x620, 0x10);
    rslt |= hal_spi_write_reg(phy, 0x61F, 0x00);
  }

  return rslt;
}

/*!
 * @brief This API initializes RX1.
 * 
 */
int32_t rx1_init(rf_chip_phy_t* phy) {
  int32_t rslt = TRANSV_OK;

  rslt = hal_spi_write_reg(phy, 0x651, 0x7F);
  rslt |= hal_spi_write_reg(phy, 0x666, 0x27);
  rslt |= hal_spi_write_reg(phy, 0x667, 0x82);
  rslt |= hal_spi_write_reg(phy, 0x66D, 0x00);
  rslt |= hal_spi_write_reg(phy, 0x6D0, 0xEF);
  rslt |= hal_spi_write_reg(phy, 0x6D2, 0xC3);
  /* Rx mode init, to fix rx dc cal fail with unexpected value (0xff). */
  rslt |= hal_spi_write_reg(phy, 0xd34, 0xcc);
  return rslt;
}

/*!
 * @brief This API initializes RX2.
 * 
 */
int32_t rx2_init(rf_chip_phy_t* phy) {
  int32_t rslt = TRANSV_OK;

  rslt = hal_spi_write_reg(phy, 0x66f, 0x7f);
  rslt |= hal_spi_write_reg(phy, 0x684, 0x27);
  rslt |= hal_spi_write_reg(phy, 0x685, 0x82);
  rslt |= hal_spi_write_reg(phy, 0x68b, 0x00);
  rslt |= hal_spi_write_reg(phy, 0x6e0, 0xef);
  rslt |= hal_spi_write_reg(phy, 0x6e2, 0xC3);
  /* Rx mode init, to fix rx dc cal fail with unexpected value (0xff). */
  rslt |= hal_spi_write_reg(phy, 0xd34, 0xcc);
  return rslt;
}

/*!
 * @brief This API initializes RXADC.
 * 
 */
int32_t rxadc_init(rf_chip_phy_t* phy) {
  int32_t rslt = TRANSV_OK;

  rslt = hal_spi_write_reg(phy, 0x6b5, 0x00);
  rslt |= hal_spi_write_reg(phy, 0x6b4, 0x00);
  rslt |= hal_spi_write_reg(phy, 0x6b4, 0x30);
  rslt |= hal_spi_write_reg(phy, 0x6b5, 0xee);

  return rslt;
}

/*!
 * @brief This API initializes RX1ADC weight settings.
 * 
 */
int32_t rx1adc_weight_init(rf_chip_phy_t* phy) {
  int32_t rslt = TRANSV_OK;

  rslt = hal_spi_write_reg(phy, 0x699, 0x14);
  rslt |= hal_spi_write_reg(phy, 0x69A, 0x03);
  rslt |= hal_spi_write_reg(phy, 0x69B, 0xFA);
  rslt |= hal_spi_write_reg(phy, 0x699, 0xB4);
  rslt |= hal_spi_write_reg(phy, 0x699, 0x15);
  rslt |= hal_spi_write_reg(phy, 0x69A, 0xFC);
  rslt |= hal_spi_write_reg(phy, 0x69B, 0x06);
  rslt |= hal_spi_write_reg(phy, 0x699, 0xB5);
  rslt |= hal_spi_write_reg(phy, 0x699, 0x00);
  rslt |= hal_spi_write_reg(phy, 0x69C, 0x14);
  rslt |= hal_spi_write_reg(phy, 0x69D, 0x03);
  rslt |= hal_spi_write_reg(phy, 0x69E, 0xFA);
  rslt |= hal_spi_write_reg(phy, 0x69C, 0xB4);
  rslt |= hal_spi_write_reg(phy, 0x69C, 0x15);
  rslt |= hal_spi_write_reg(phy, 0x69D, 0xFC);
  rslt |= hal_spi_write_reg(phy, 0x69E, 0x06);
  rslt |= hal_spi_write_reg(phy, 0x69C, 0xB5);
  rslt |= hal_spi_write_reg(phy, 0x69C, 0x00);
  rslt |= hal_spi_write_reg(phy, 0x699, 0x14);
  rslt |= hal_spi_write_reg(phy, 0x69A, 0x03);
  rslt |= hal_spi_write_reg(phy, 0x69B, 0xFA);
  rslt |= hal_spi_write_reg(phy, 0x699, 0x74);
  rslt |= hal_spi_write_reg(phy, 0x699, 0x15);
  rslt |= hal_spi_write_reg(phy, 0x69A, 0xFC);
  rslt |= hal_spi_write_reg(phy, 0x69B, 0x06);
  rslt |= hal_spi_write_reg(phy, 0x699, 0x75);
  rslt |= hal_spi_write_reg(phy, 0x699, 0x00);
  rslt |= hal_spi_write_reg(phy, 0x69C, 0x14);
  rslt |= hal_spi_write_reg(phy, 0x69D, 0x03);
  rslt |= hal_spi_write_reg(phy, 0x69E, 0xFA);
  rslt |= hal_spi_write_reg(phy, 0x69C, 0x74);
  rslt |= hal_spi_write_reg(phy, 0x69C, 0x15);
  rslt |= hal_spi_write_reg(phy, 0x69D, 0xFC);
  rslt |= hal_spi_write_reg(phy, 0x69E, 0x06);
  rslt |= hal_spi_write_reg(phy, 0x69C, 0x75);
  rslt |= hal_spi_write_reg(phy, 0x69C, 0x00);

  return rslt;
}

/*!
 * @brief This API initializes RX2ADC weight settings.
 * 
 */
int32_t rx2adc_weight_init(rf_chip_phy_t* phy) {
  int32_t rslt = TRANSV_OK;

  rslt = hal_spi_write_reg(phy, 0x6B7, 0x14);
  rslt |= hal_spi_write_reg(phy, 0x6B8, 0x03);
  rslt |= hal_spi_write_reg(phy, 0x6B9, 0xFA);
  rslt |= hal_spi_write_reg(phy, 0x6B7, 0xB4);
  rslt |= hal_spi_write_reg(phy, 0x6B7, 0x15);
  rslt |= hal_spi_write_reg(phy, 0x6B8, 0xFC);
  rslt |= hal_spi_write_reg(phy, 0x6B9, 0x06);
  rslt |= hal_spi_write_reg(phy, 0x6B7, 0xB5);
  rslt |= hal_spi_write_reg(phy, 0x6B7, 0x00);
  rslt |= hal_spi_write_reg(phy, 0x6BA, 0x14);
  rslt |= hal_spi_write_reg(phy, 0x6BB, 0x03);
  rslt |= hal_spi_write_reg(phy, 0x6BC, 0xFA);
  rslt |= hal_spi_write_reg(phy, 0x6BA, 0xB4);
  rslt |= hal_spi_write_reg(phy, 0x6BA, 0x15);
  rslt |= hal_spi_write_reg(phy, 0x6BB, 0xFC);
  rslt |= hal_spi_write_reg(phy, 0x6BC, 0x06);
  rslt |= hal_spi_write_reg(phy, 0x6BA, 0xB5);
  rslt |= hal_spi_write_reg(phy, 0x6BA, 0x00);
  rslt |= hal_spi_write_reg(phy, 0x6B7, 0x14);
  rslt |= hal_spi_write_reg(phy, 0x6B8, 0x03);
  rslt |= hal_spi_write_reg(phy, 0x6B9, 0xFA);
  rslt |= hal_spi_write_reg(phy, 0x6B7, 0x74);
  rslt |= hal_spi_write_reg(phy, 0x6B7, 0x15);
  rslt |= hal_spi_write_reg(phy, 0x6B8, 0xFC);
  rslt |= hal_spi_write_reg(phy, 0x6B9, 0x06);
  rslt |= hal_spi_write_reg(phy, 0x6B7, 0x75);
  rslt |= hal_spi_write_reg(phy, 0x6B7, 0x00);
  rslt |= hal_spi_write_reg(phy, 0x6BA, 0x14);
  rslt |= hal_spi_write_reg(phy, 0x6BB, 0x03);
  rslt |= hal_spi_write_reg(phy, 0x6BC, 0xFA);
  rslt |= hal_spi_write_reg(phy, 0x6BA, 0x74);
  rslt |= hal_spi_write_reg(phy, 0x6BA, 0x15);
  rslt |= hal_spi_write_reg(phy, 0x6BB, 0xFC);
  rslt |= hal_spi_write_reg(phy, 0x6BC, 0x06);
  rslt |= hal_spi_write_reg(phy, 0x6BA, 0x75);
  rslt |= hal_spi_write_reg(phy, 0x6BA, 0x00);

  return rslt;
}

/*!
 * @brief This API initializes TX1.
 * 
 */
int32_t tx1_init(rf_chip_phy_t* phy) {
  int32_t rslt = TRANSV_OK;

  rslt = hal_spi_write_reg(phy, 0x747, 0x01);
  rslt |= hal_spi_write_reg(phy, 0x700, 0x00);
  rslt |= hal_spi_write_reg(phy, 0x703, 0x07);
  rslt |= hal_spi_write_reg(phy, 0x719, 0x00);
  rslt |= hal_spi_write_reg(phy, 0x738, 0x80);
  rslt |= hal_spi_write_reg(phy, 0x739, 0x84);
  rslt |= hal_spi_write_reg(phy, 0x71E, 0x00);
  rslt |= hal_spi_write_reg(phy, 0x71F, 0x40);
  rslt |= hal_spi_write_reg(phy, 0x722, 0x00);
  rslt |= hal_spi_write_reg(phy, 0x723, 0x00);
  rslt |= hal_spi_write_reg(phy, 0x724, 0x00);
  rslt |= hal_spi_write_reg(phy, 0x725, 0xFF);
  rslt |= hal_spi_write_reg(phy, 0x726, 0x32);
  rslt |= hal_spi_write_reg(phy, 0x727, 0x03);
  rslt |= hal_spi_write_reg(phy, 0x728, 0xC0);
  rslt |= hal_spi_write_reg(phy, 0x729, 0x40);
  rslt |= hal_spi_write_reg(phy, 0x701, 0x37);

  return rslt;
}

/*!
 * @brief This API initializes TX2.
 * 
 */
int32_t tx2_init(rf_chip_phy_t* phy) {
  int32_t rslt = TRANSV_OK;

  rslt = hal_spi_write_reg(phy, 0x78f, 0x01);
  rslt |= hal_spi_write_reg(phy, 0x748, 0x00);
  rslt |= hal_spi_write_reg(phy, 0x74b, 0x07);
  rslt |= hal_spi_write_reg(phy, 0x761, 0x00);
  rslt |= hal_spi_write_reg(phy, 0x780, 0x80);
  rslt |= hal_spi_write_reg(phy, 0x781, 0x84);
  rslt |= hal_spi_write_reg(phy, 0x766, 0x00);
  rslt |= hal_spi_write_reg(phy, 0x767, 0x40);
  rslt |= hal_spi_write_reg(phy, 0x76A, 0x00);
  rslt |= hal_spi_write_reg(phy, 0x76B, 0x00);
  rslt |= hal_spi_write_reg(phy, 0x76C, 0x00);
  rslt |= hal_spi_write_reg(phy, 0x76D, 0xFF);
  rslt |= hal_spi_write_reg(phy, 0x76E, 0x32);
  rslt |= hal_spi_write_reg(phy, 0x76F, 0x03);
  rslt |= hal_spi_write_reg(phy, 0x770, 0xC0);
  rslt |= hal_spi_write_reg(phy, 0x771, 0x40);
  rslt |= hal_spi_write_reg(phy, 0x749, 0x37);

  return rslt;
}

/*!
 * @brief This API initializes TX1DAC.
 * 
 */
int32_t tx1dac_init(rf_chip_phy_t* phy) {
  int32_t rslt = TRANSV_OK;

  rslt = hal_spi_write_reg(phy, 0x790, 0x01);
  rslt |= hal_spi_write_reg(phy, 0x791, 0x00);
  rslt |= hal_spi_write_reg(phy, 0x791, 0x21);
  rslt |= hal_spi_write_reg(phy, 0x791, 0x6F);
  rslt |= hal_spi_write_reg(phy, 0x791, 0x7F);

  return rslt;
}

/*!
 * @brief This API initializes TX2DAC.
 * 
 */
int32_t tx2dac_init(rf_chip_phy_t* phy) {
  int32_t rslt = TRANSV_OK;

  rslt = hal_spi_write_reg(phy, 0x7a3, 0x01);
  rslt |= hal_spi_write_reg(phy, 0x7a4, 0x00);
  rslt |= hal_spi_write_reg(phy, 0x7a4, 0x21);
  rslt |= hal_spi_write_reg(phy, 0x7a4, 0x6F);
  rslt |= hal_spi_write_reg(phy, 0x7a4, 0x7F);

  return rslt;
}

/*!
 * @brief This API sets the transceiver chip to wait state.
 * 
 */
int32_t force_wait(rf_chip_phy_t* phy) {
  int32_t rslt = TRANSV_OK;

  rslt = hal_spi_write_reg(phy, 0x0cc, 0x08);
  rslt |= hal_spi_write_reg(phy, 0x0cc, 0x09);
  rslt |= hal_spi_write_reg(phy, 0x0cc, 0x08);
  rslt |= hal_spi_write_reg(phy, 0x0cc, 0x08);
  rslt |= hal_spi_write_reg(phy, 0x0D1, 0xC0);

  CHIP_DELAY(5);

  return rslt;
}

/*!
 * @brief This API disables XO fast charge.
 * 
 */
int32_t xo_fc_off(rf_chip_phy_t* phy) {
  int32_t rslt = TRANSV_OK;

  rslt = hal_spi_write_reg(phy, 0x614, 0x00);

  return rslt;
}

/*!
 * @brief This API disables CM MDIG LDO fast charge 
 *  and CM IVREF fast charge.
 */
int32_t cm_fc_off(rf_chip_phy_t* phy) {
  int32_t rslt = TRANSV_OK;

  rslt = hal_spi_write_reg(phy, 0x901, 0xC0);

  return rslt;
}

/*!
 * @brief This API check whether the transceiver device exist or not.
 *
 */

int32_t transceiver_checkdevice(rf_chip_phy_t* phy) {
  short val1, val2;

  val1 = hal_spi_read_reg(phy, 0x65c);
  hal_spi_write_reg(phy, 0x65c, 0xd);
  val2 = hal_spi_read_reg(phy, 0x65c);
  if (val2 != 0xd) {
    LOG_WARN("%s in line %d SPI RW issue, write val 0xd via SPI, but read val 0x%0x via SPI\n",
             __FUNCTION__, __LINE__, val2);
    return TRANSV_E_COM_FAIL;
  }
  hal_spi_write_reg(phy, 0x65c, val1);
  return TRANSV_OK;
}

/*!
 * @brief This API power up whole transceiver 
 *
 */
int32_t power_init(rf_chip_phy_t* phy) {
  int32_t rslt = TRANSV_OK;
  bool fg_fdd  = false;

  phy->config->xo_sel = TCXO;

  rslt = cm_init(phy);
  rslt |= ana_manual_on(phy);
  rslt |= ldo_init(phy);
  rslt |= xo_selt(phy, phy->config->xo_sel);
  rslt |= x4_init(phy);
  rslt |= refclk_config(phy, phy->config->x4_enable);
  rslt |= syspll_init(phy);
  rslt |= sx_init(phy);

  if (rslt != TRANSV_OK) {
    LOG_ERROR("FAIL to run %s in line%d, rslt: %d\n", __FUNCTION__, __LINE__, rslt);
  }

  return rslt;
}

/*!
 * @brief This API init all analog modules
 *
 */
int32_t analog_init(rf_chip_phy_t* phy) {
  int32_t rslt = TRANSV_OK;
  bool fg_fdd  = false;

  /* config fd mode */
  if (phy->config->use_bybrid_mode) {
    fg_fdd = (phy->config->hybrid_mode == HYBRID_FDD_SXTX) ? false : true;
  } else {
    fg_fdd = (phy->config->mode <= CH1_CH2_FDD) ? true : false;
  }

  rslt |= sxrx_init(phy, fg_fdd);
  rslt |= sxtx_init(phy);
  rslt |= man_lo_config(phy, fg_fdd);
  rslt |= rx1_init(phy);
  rslt |= rx2_init(phy);
  rslt |= rxadc_init(phy);
  rslt |= rx1adc_weight_init(phy);
  rslt |= rx2adc_weight_init(phy);
  rslt |= tx1_init(phy);
  rslt |= tx2_init(phy);
  rslt |= tx1dac_init(phy);
  rslt |= tx2dac_init(phy);

  if (rslt != TRANSV_OK) {
    LOG_ERROR("FAIL to run %s in line%d, rslt: %d\n", __FUNCTION__, __LINE__, rslt);
  }

  return rslt;
}

/*!
 * @brief This API initial whole transceiver 
 *
 */
int32_t transceiver_init(rf_chip_phy_t* phy) {
  int32_t rslt = TRANSV_OK;
  bool fg_fdd  = false;

  phy->config->xo_sel = TCXO;

  rslt = cm_init(phy);
  rslt |= ana_manual_on(phy);
  rslt |= ldo_init(phy);
  rslt |= xo_selt(phy, phy->config->xo_sel);
  rslt |= x4_init(phy);
  rslt |= refclk_config(phy, phy->config->x4_enable);
  rslt |= syspll_init(phy);
  rslt |= sx_init(phy);

  /* config fd mode */
  if (phy->config->use_bybrid_mode) {
    fg_fdd = (phy->config->hybrid_mode == HYBRID_FDD_SXTX) ? false : true;
  } else {
    fg_fdd = (phy->config->mode <= CH1_CH2_FDD) ? true : false;
  }
  rslt |= sxrx_init(phy, fg_fdd);
  rslt |= sxtx_init(phy);
  rslt |= man_lo_config(phy, fg_fdd);
  rslt |= rx1_init(phy);
  rslt |= rx2_init(phy);
  rslt |= rxadc_init(phy);
  rslt |= rx1adc_weight_init(phy);
  rslt |= rx2adc_weight_init(phy);
  rslt |= tx1_init(phy);
  rslt |= tx2_init(phy);
  rslt |= tx1dac_init(phy);
  rslt |= tx2dac_init(phy);

  if (rslt != TRANSV_OK) {
    LOG_ERROR("FAIL to run %s in line%d, rslt: %d\n", __FUNCTION__, __LINE__, rslt);
  }

  return rslt;
}

/*!
 * @brief This API backup the related control registers for AUXADC
 *
 */

int32_t auxadcctrl_backup(rf_chip_phy_t* phy, MT_AUXADC_ENUM mt, auxadc_ctrl* ctrl) {
  int32_t rslt    = TRANSV_OK;
  uint8_t reg_val = 0;
  unsigned short reg_rxrstdetr; /* control to reset rx$ detector for auxadc */
  unsigned short reg_rxpowDetr; /* control to enable rx$ power dector for auxadc */

  if (ctrl == NULL) {
    rslt = TRANSV_E_NULL_PTR;
    LOG_ERROR("%s REPORTED ERR in line %d for access NULL PTR\n");
    return rslt;
  }

  if (mt <= MT1N_AUXADC) {
    reg_rxrstdetr = 0xE0D; /* The register to reset rx1 detector for auxadc1. */
    reg_rxpowDetr = 0x668; /* The register to enable rx1 power detector for auxadc1. */
  } else {
    reg_rxrstdetr = 0xE8D; /* The register to reset rx2 detector for auxadc2. */
    reg_rxpowDetr = 0x686; /* The register to enable rx2 power detector for auxadc2. */
  }

  ctrl->rgRxRstDetr_val = hal_spi_read_reg(phy, reg_rxrstdetr);
  ctrl->rg828_val       = hal_spi_read_reg(phy, 0x828);
  ctrl->rg829_val       = hal_spi_read_reg(phy, 0x829);
  ctrl->rg82A_val       = hal_spi_read_reg(phy, 0x82A);
  ctrl->rg830_val       = hal_spi_read_reg(phy, 0x830);
  ctrl->rg835_val       = hal_spi_read_reg(phy, 0x835);
  ctrl->rg841_val       = hal_spi_read_reg(phy, 0x841);
  ctrl->rg842_val       = hal_spi_read_reg(phy, 0x842);
  ctrl->rg843_val       = hal_spi_read_reg(phy, 0x843);
  ctrl->rgRxPowDetr_val = hal_spi_read_reg(phy, reg_rxpowDetr);

  if (mt <= MT1N_AUXADC) {
    reg_val = (uint8_t)(~(TRANSCV_RXPOWDTR_RXPDET_EN_MASK | TRANSCV_RXPOWDTR_AUXADC1_EN_MASK));
  } else {
    reg_val = (uint8_t)(~(TRANSCV_RXPOWDTR_RXPDET_EN_MASK | TRANSCV_RXPOWDTR_AUXADC2_EN_MASK));
  }

  rslt = hal_spi_write_reg(phy, reg_rxrstdetr, ctrl->rgRxRstDetr_val | 0x7F);

  rslt |= hal_spi_write_reg(phy, reg_rxpowDetr, ctrl->rgRxPowDetr_val & reg_val);

  return rslt;
}

/*!
 * @brief This API recover the related control registers for AUXADC
 *
 */

int32_t auxadcctrl_recover(rf_chip_phy_t* phy, MT_AUXADC_ENUM mt, auxadc_ctrl* ctrl) {
  int32_t rslt    = TRANSV_OK;
  uint8_t reg_val = 0;
  unsigned short reg_rxrstdetr; /* control to reset $rx detector for auxadc */
  unsigned short reg_rxpowDetr; /* control to enable $rx power dector for auxadc */

  if (ctrl == NULL) {
    rslt = TRANSV_E_NULL_PTR;
    LOG_ERROR("%s REPORTED ERR in line %d for access NULL PTR\n");
    return rslt;
  }

  if (mt <= MT1N_AUXADC) {
    reg_rxrstdetr = 0xE0D; /* The register to reset rx1 detector for auxadc1. */
    reg_rxpowDetr = 0x668; /* The register to enable rx1 power detector for auxadc. */
    ctrl->rgRxPowDetr_val &= (TRANSCV_RXPOWDTR_RXPDET_EN_MASK | TRANSCV_RXPOWDTR_AUXADC1_EN_MASK);
  } else {
    reg_rxrstdetr = 0xE8D; /* The register to reset rx2 detector for auxadc2. */
    reg_rxpowDetr = 0x686; /* The register to enable rx2 power detector for auxadc. */
    ctrl->rgRxPowDetr_val &= (TRANSCV_RXPOWDTR_RXPDET_EN_MASK | TRANSCV_RXPOWDTR_AUXADC2_EN_MASK);
  }

  rslt |= hal_spi_write_reg(phy, 0x828, ctrl->rg828_val);
  rslt |= hal_spi_write_reg(phy, 0x829, ctrl->rg829_val);
  rslt |= hal_spi_write_reg(phy, 0x82A, ctrl->rg82A_val);
  rslt |= hal_spi_write_reg(phy, 0x830, ctrl->rg830_val);
  rslt |= hal_spi_write_reg(phy, 0x835, ctrl->rg835_val);
  rslt |= hal_spi_write_reg(phy, 0x841, ctrl->rg841_val);
  rslt |= hal_spi_write_reg(phy, 0x842, ctrl->rg842_val);
  rslt |= hal_spi_write_reg(phy, 0x843, ctrl->rg843_val);

  reg_val = hal_spi_read_reg(phy, reg_rxpowDetr);
  if (mt <= MT1N_AUXADC) {
    reg_val &= (uint8_t)(~(TRANSCV_RXPOWDTR_RXPDET_EN_MASK | TRANSCV_RXPOWDTR_AUXADC1_EN_MASK));
  } else {
    reg_val &= (uint8_t)(~(TRANSCV_RXPOWDTR_RXPDET_EN_MASK | TRANSCV_RXPOWDTR_AUXADC2_EN_MASK));
  }
  rslt |= hal_spi_write_reg(phy, reg_rxpowDetr, ctrl->rgRxPowDetr_val | reg_val);

  rslt |= hal_spi_write_reg(phy, reg_rxrstdetr, ctrl->rgRxRstDetr_val);

  return rslt;
}

/*!
 * @brief This API reset the AUXADC1
 *
 */

int32_t auxadc1_reset(rf_chip_phy_t* phy) {
  int32_t rslt = TRANSV_OK;

  rslt |= hal_spi_write_reg(phy, 0x861, 0x30);
  rslt |= hal_spi_write_reg(phy, 0x861, 0x00);
  rslt |= hal_spi_write_reg(phy, 0x861, 0x00);
  rslt |= hal_spi_write_reg(phy, 0x861, 0x30);
  rslt |= hal_spi_write_reg(phy, 0x861, 0x31);

  return rslt;
}

/*!
 * @brief This API reset the AUXADC2
 *
 */

int32_t auxadc2_reset(rf_chip_phy_t* phy) {
  int32_t rslt = TRANSV_OK;

  rslt |= hal_spi_write_reg(phy, 0x86A, 0x30);
  rslt |= hal_spi_write_reg(phy, 0x86A, 0x00);
  rslt |= hal_spi_write_reg(phy, 0x86A, 0x00);
  rslt |= hal_spi_write_reg(phy, 0x86A, 0x30);
  rslt |= hal_spi_write_reg(phy, 0x86A, 0x31);

  return rslt;
}

/*!
 * @brief This API read tx dc calibration value
 *
 */
int32_t get_auxadc_calc_val(rf_chip_phy_t* phy, TRX_CHN_ENUM chn, int8_t swap_flag, int32_t* value) {
  int32_t rslt = TRANSV_OK;
  uint16_t auxadc_ctr_reg;
  uint16_t auxadc_read_reg1;
  uint16_t auxadc_read_reg2;
  uint8_t reg_val1;
  uint8_t reg_val2;

  if (value == NULL) {
    rslt = TRANSV_E_NULL_PTR;
    LOG_ERROR("%s REPORTED ERR in line %d for access NULL PTR\n");
    return rslt;
  }

  if (chn == TRX_CHN1) {
    auxadc_ctr_reg   = 0x860;
    auxadc_read_reg1 = 0x865;
    auxadc_read_reg2 = 0x866;
  } else {
    auxadc_ctr_reg   = 0x869;
    auxadc_read_reg1 = 0x86E;
    auxadc_read_reg2 = 0x86F;
  }

  rslt |= hal_spi_write_reg(phy, auxadc_ctr_reg, swap_flag ? 0x80 : 0);
  rslt |= hal_spi_write_reg(phy, auxadc_read_reg1, 0x80);  //enable read
  reg_val1 = hal_spi_read_reg(phy, auxadc_read_reg1);
  reg_val2 = hal_spi_read_reg(phy, auxadc_read_reg2);
  rslt |= hal_spi_write_reg(phy, auxadc_read_reg1, 0);  //disable read

  *value = ((reg_val1 & 0x7) << 8) | reg_val2;
  if (*value > 1023)
    *value = (*value - 2048) * 2;
  else
    *value = *value * 2;

  return rslt;
}

/*!
 * @brief This API do tx dc offset cal with auxadc
 *
 */
int32_t tx_dc_offset_cal_with_auxadc(rf_chip_phy_t* phy, TRX_CHN_ENUM chn, uint16_t tune_reg, int32_t retry) {
  int32_t rslt = TRANSV_OK;
  int32_t tmp1[TX_DC_OFFSET_CAL_AUXADC_READ_CNT];
  int32_t tmp2[TX_DC_OFFSET_CAL_AUXADC_READ_CNT];
  int32_t sum, average;
  int32_t i, flag = 0, read_cnt = TX_DC_OFFSET_CAL_AUXADC_READ_CNT;
  int32_t ci = 7;
  uint8_t reg_val, reg_val_bak;
  uint8_t cal_auxadc_buffer_tmp1[512] = {0};
  uint8_t cal_auxadc_buffer_tmp2[512] = {0};
//   printf("ruanbo: tx_dc_offset_cal_with_auxadc\n");
//   printf("ruanbo: phy->config->rx_dc_cal_flag: %d\n\r", phy->config->rx_dc_cal_flag);

  reg_val_bak = hal_spi_read_reg(phy, tune_reg);

  while (ci >= 0) {
    sum = 0;
    // printf("ruanbo: while\n");
    // printf("ruanbo: phy->config->rx_dc_cal_flag: %d\n\r", phy->config->rx_dc_cal_flag);

    for (i = 0; i < read_cnt; i++) {
      rslt |= get_auxadc_calc_val(phy, chn, 0, &tmp1[i]);

      if (phy->module_debug & TX_DC_CAL) {
        if (i % 5 == 0) {
          sprintf(cal_auxadc_buffer_tmp1 + strlen(cal_auxadc_buffer_tmp1), "REG[%02x:%02x]",
                  hal_spi_read_reg(phy, 0x842), hal_spi_read_reg(phy, 0x843));
        }
        sprintf(cal_auxadc_buffer_tmp1 + strlen(cal_auxadc_buffer_tmp1), "[%04d]", tmp1[i]);
      }
      //   printf("ruanbo: for index i = %d\n", i);
      //   printf("ruanbo: phy->config->rx_dc_cal_flag: %d\n\r", phy->config->rx_dc_cal_flag);
    }
    for (i = 0; i < read_cnt; i++) {
      rslt |= get_auxadc_calc_val(phy, chn, 1, &tmp2[i]);
      if (phy->module_debug & TX_DC_CAL) {
        if (i % 5 == 0) {
          sprintf(cal_auxadc_buffer_tmp2 + strlen(cal_auxadc_buffer_tmp2), "REG[%02x:%02x]",
                  hal_spi_read_reg(phy, 0x842), hal_spi_read_reg(phy, 0x843));
        }
        sprintf(cal_auxadc_buffer_tmp2 + strlen(cal_auxadc_buffer_tmp2), "[%04d]", tmp2[i]);
      }
      //   printf("ruanbo: for index i = %d\n", i);
      //   printf("ruanbo: phy->config->rx_dc_cal_flag: %d\n\r", phy->config->rx_dc_cal_flag);
    }

    if (phy->module_debug & TX_DC_CAL) {
      LOG_MDEBUG(phy, TX_DC_CAL, "%s cur ci =%d, tm1values: %s\n", __FUNCTION__, ci, cal_auxadc_buffer_tmp1);
      LOG_MDEBUG(phy, TX_DC_CAL, "%s cur ci =%d, tm2values: %s\n", __FUNCTION__, ci, cal_auxadc_buffer_tmp2);
      memset(cal_auxadc_buffer_tmp1, 0, 512);
      memset(cal_auxadc_buffer_tmp2, 0, 512);
    }

    for (i = 0; i < read_cnt; i++) {
      sum += (tmp1[i] - tmp2[i]) / 2;
    }

    //average = sum / read_cnt;
    average = sum; /* Average will lose the data sensitivity, so used sum instead. */

    if (ci == 7) {
      reg_val = 0; /* clear legacy reg val in tune_reg */
      if (average < 0) {
        reg_val = CLR_BIT(reg_val, ci);
        flag    = 0;
      } else if (average > 0) {
        reg_val = SET_BIT(reg_val, ci);
        flag    = 1;
      }

      reg_val = SET_BIT(reg_val, ci - 1);
      rslt |= hal_spi_write_reg(phy, tune_reg, reg_val);

    } else {
      if (average > 0) {
        if (flag == 0)
          HAL_REG_CLR_BIT(phy, (tune_reg), ci);
      } else if (average < 0) {
        if (flag == 1)
          HAL_REG_CLR_BIT(phy, (tune_reg), ci);
      }

      if (ci > 0)
        HAL_REG_SET_BIT(phy, (tune_reg), ci - 1);
    }

    reg_val = hal_spi_read_reg(phy, tune_reg);
    if (((reg_val & TRANSCV_TXDCOFFSETCAL_MAX_MASK) == TRANSCV_TXDCOFFSETCAL_MAX_MASK) && (retry > 1)) {
      LOG_WARN("%s REPORTED WRN in line %d cur ci =%d ,average = %d, tune_reg = 0x%x, reg_val = 0x%x, legacy reg_val = 0x%x\n",
               __FUNCTION__, __LINE__, ci, average, tune_reg, reg_val, reg_val_bak);
      return TRANSV_E_CAL_FAIL;
    } else {
      LOG_MDEBUG(phy, TX_DC_CAL, "%s in line %d cur ci =%d ,average = %d, tune_reg = 0x%x, reg_val = 0x%x\n",
                 __FUNCTION__, __LINE__, ci, average, tune_reg, reg_val);
    }

    ci--;
  }
  // printf("ruanbo: leave tx_dc_offset_cal_with_auxadc\n");
//   printf("ruanbo: phy->config->rx_dc_cal_flag: %d\n\r", phy->config->rx_dc_cal_flag);
//   printf("ruanbo: rslt = %d\n\r", rslt);

  return rslt;
}

/*!
 * @brief This API do R cal
 *
 */
int32_t rcal(rf_chip_phy_t* phy) {
  int32_t rslt = TRANSV_OK;
  uint8_t A    = 1;
  uint8_t b    = 63;
  uint8_t tmp;
  uint8_t reg_val;
  uint8_t i, cnt_one = 0;

  reg_val = hal_spi_read_reg(phy, 0x05c);
  reg_val = reg_val | 0x08;
  rslt |= hal_spi_write_reg(phy, 0x900, reg_val);

  if (1 != phy->r_cal_flag) {
    A = 1;
    b = 63;

    while (A <= b) {
      tmp = (A + b) / 2;
      rslt |= hal_spi_write_reg(phy, 0x902, tmp);

      cnt_one = 0;
      for (i = 0; i < phy->config->rcal_read; i++) {
        reg_val = hal_spi_read_reg(phy, 0x850);
        cnt_one += (reg_val & TRANSCV_REG_BIT0_MASK);
      }

      if (cnt_one > phy->config->rcal_read / 2)
        b = tmp - 1;
      else
        A = tmp + 1;
    }
    phy->r_cal_flag = 1;
    phy->r_cal      = tmp;
    LOG_MAIN("%s in line %d phy->r_cal result is 0x%02x\n", __FUNCTION__, __LINE__, phy->r_cal);
  } else {
    rslt |= hal_spi_write_reg(phy, 0x902, phy->r_cal);
  }

  reg_val = hal_spi_read_reg(phy, 0x05E);
  rslt |= hal_spi_write_reg(phy, 0x903, reg_val);

  if (phy->config->chip_ver == 1) {
    rslt |= hal_spi_write_reg(phy, 0x81a, 0x88);
    rslt |= hal_spi_write_reg(phy, 0x619, 0x1c);
    rslt |= hal_spi_write_reg(phy, 0x61a, 0x4e);
  }
  if (phy->config->chip_ver == 2) {
    rslt |= hal_spi_write_reg(phy, 0x81a, 0x88);
  } else {
    rslt |= hal_spi_write_reg(phy, 0x81a, 0x28);
  }

  reg_val = hal_spi_read_reg(phy, 0x05c);
  reg_val = reg_val & (~0x08);
  rslt |= hal_spi_write_reg(phy, 0x900, reg_val);

  return rslt;
}

/*!
 * @brief This API read auxadc cal oridinate and ordinate from efuse in the chip.
 *
 */
int32_t auxadc_cal(rf_chip_phy_t* phy) {
  int32_t rslt = TRANSV_OK;
  int8_t data;

  if (0 == phy->auxadc1_cal_flag) {
    rslt = efuse_read_opt(phy, &data, EFUSE_INFO_SECTION_OPT_AUXADC_H);
    if (rslt == TRANSV_OK) {
      phy->auxadccal_slope = (int32_t)data;
    } else {
      phy->auxadccal_slope = 0x0;
    }
    rslt |= efuse_read_opt(phy, &data, EFUSE_INFO_SECTION_OPT_AUXADC_L);
    if (rslt == TRANSV_OK) {
      phy->auxadccal_ordinate = (int32_t)data;
    } else {
      phy->auxadccal_ordinate = 0x0;
    }

    phy->auxadc1_cal_flag = 1;
  }
  LOG_MAIN("%s in line %d, auxadccal_slope is %d, and auxadccal_ordinate is %d\n", __FUNCTION__, __LINE__,
           phy->auxadccal_slope, phy->auxadccal_ordinate);

  return rslt;
}

/*!
 * @brief This API transfer original value reading from auxadc to target value by auxadc cal parameter.
 *
 */
int32_t auxadccal_transfer(rf_chip_phy_t* phy, int* value) {
  int32_t rslt   = TRANSV_OK;
  float outvalue = 0;
  float slope;

  if (value == NULL) {
    rslt = TRANSV_E_NULL_PTR;
    LOG_ERROR("%s REPORTED ERR in line %d for access NULL PTR\n");
    return rslt;
  }

  slope    = TRANSCV_AUXADC_CAL_RESOLUTION * (float)phy->auxadccal_slope + 1;
  outvalue = (*value - phy->auxadccal_ordinate) / slope;

  //LOG_MAIN("%s in line %d, value is %d, slope is %.8f , ordinate is %d, and output value is %d\n",
  //    __FUNCTION__, __LINE__, *value, slope, phy->auxadccal_ordinate, (int32_t)outvalue);

  *value = (int)outvalue;

  return rslt;
}

/*!
 * @brief This API Write data to the register by the list from the file.
 *
 */
void extn_reg_init(rf_chip_phy_t* phy, char* file) {
#if HAVE_FS
  FILE* fp = NULL;
  int rd, counter = 0;
  char buf[128] = {0};
  char *p, *q;
  unsigned short reg_addr;
  unsigned char reg_data;

  /* Read from $file for setting. */
  fp = fopen(file, "r");
  if (fp == NULL) {
    LOG_MAIN("[INFO], the %s not exist, go ahead!\r\n", file);
  } else {
    while (fgets(buf, 128, fp) != NULL) {
      reg_addr = 0;
      reg_data = 0;
      if (strlen(buf) >= 12) {
        p        = buf;
        reg_addr = (unsigned short)(strtol(p, &q, 16));
        p        = q;
        reg_data = (unsigned char)(strtol(p, &q, 16));
      }
      if (reg_addr != 0) {
        //LOG_MAIN("[INFO], the %d line, lenth[%d], reg_addr[%x], reg_data[%x]!\r\n", counter+1, strlen(buf), reg_addr, reg_data);
        hal_spi_write_reg(phy, reg_addr, reg_data);
        counter++;
      }
      memset(buf, 0, 128 * sizeof(char));
    }
    fclose(fp);
    LOG_MAIN("[INFO], total write %d lines command\r\n", counter);
  }
#endif
}

/*!
 * @brief This API read the register list from the file and write the result back to outfile.
 *
 */
void extn_reg_print(rf_chip_phy_t* phy, char* file, char* outfile) {
#if HAVE_FS
  FILE* fp    = NULL;
  FILE* fpOut = NULL;
  int rd, counter = 0;
  char buf[128] = {0};
  char *p, *q;
  unsigned short reg_addr;
  unsigned char reg_data;

  /* Read from $file for printing. */
  fp = fopen(file, "r");
  if (fp == NULL) {
    LOG_MAIN("[INFO], the %s not exist, go ahead!\r\n", file);
    return;
  }
  /* write the result to $outfile. */
  fpOut = fopen(outfile, "w");
  if (fpOut == NULL) {
    LOG_MAIN("[INFO], the %s open fail, go ahead!\r\n", outfile);
  } else {
    while (fgets(buf, 128, fp) != NULL) {
      reg_addr = 0;

      if (strlen(buf) >= 5) {
        p        = buf;
        reg_addr = (unsigned short)(strtol(p, &q, 16));
      }
      if (reg_addr != 0) {
        reg_data = hal_spi_read_reg(phy, reg_addr);
        counter++;
        //LOG_MAIN("[INFO], the %d line, lenth[%d], reg_addr[%x], reg_data[%x]!\r\n", counter+1, strlen(buf), reg_addr, reg_data);
        memset(buf, 0, 128 * sizeof(char));
        sprintf(buf + strlen(buf), "[0x%03x] = 0x%02x\r\n", reg_addr, reg_data);
      }

      fputs(buf, fpOut);
      memset(buf, 0, 128 * sizeof(char));
    }
    fclose(fp);
    fclose(fpOut);
    LOG_MAIN("[INFO], total read %d lines command\r\n", counter);
  }
#endif
}
