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

#ifndef  __TRANSCEIVER_API_H
#define  __TRANSCEIVER_API_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************************************************************/
#include "transceiver_def.h"
#include "platform.h"
#include "limit_efuse.h"
/*-------------------------------------------------------------------------------------------------*/
// basic APIs for transceiver
/*-------------------------------------------------------------------------------------------------*/

/*!
 * \ingroup transceiverApiCmInit
 * \page transceiver_api_cm_init cm_init
 * \code
 * int32_t cm_init(rf_chip_phy_t *phy);
 * \endcode
 * @details Initializes the CM module.
 *
 * This API performs a write operation to a register using the hardware interface /dev/axi_fpga
 * when selecting axi_fpga. If necessary, the low-level interface can be changed to use the real SPI.
 *
 * @param[in] phy Structure instance of rf_chip_phy_t.
 * @note Refer to the user guide for detailed information.
 *
 * @return Result of the API execution status.
 * @retval 0 -> Success
 * @retval < 0 -> Fail
 */
int32_t cm_init(rf_chip_phy_t *phy);

/*!
 * \ingroup transceiverApiAnaManualOn
 * \page transceiver_api_ana_manual_on ana_manual_on
 * \code
 * int32_t ana_manual_on(rf_chip_phy_t *phy);
 * \endcode
 * @details Sets analog modules to manual mode.
 *
 * This API performs a write operation to a register using the hardware interface /dev/axi_fpga
 * when selecting axi_fpga. If necessary, the low-level interface can be changed to use the real SPI.
 *
 * @param[in] phy Structure instance of rf_chip_phy_t.
 * @note Refer to the user guide for detailed information.
 *
 * @return Result of the API execution status.
 * @retval 0 -> Success
 * @retval < 0 -> Fail
 */
int32_t ana_manual_on(rf_chip_phy_t *phy);

/*!
 * \ingroup transceiverApiAnaManualOff
 * \page transceiver_api_ana_manual_off ana_manual_off
 * \code
 * int32_t ana_manual_off(rf_chip_phy_t *phy);
 * \endcode
 * @details Sets analog modules to manual off.
 *
 * This API performs a write operation to a register using the hardware interface /dev/axi_fpga
 * when selecting axi_fpga. If necessary, the low-level interface can be changed to use the real SPI.
 *
 * @param[in] phy Structure instance of rf_chip_phy_t.
 * @note Refer to the user guide for detailed information.
 *
 * @return Result of the API execution status.
 * @retval 0 -> Success
 * @retval < 0 -> Fail
 */
int32_t ana_manual_off(rf_chip_phy_t *phy);

/*!
 * \ingroup transceiverApiLdoInit
 * \page transceiver_api_ldo_init ldo_init
 * \code
 * int32_t ldo_init(rf_chip_phy_t *phy);
 * \endcode
 * @details Initializes the LDOs.
 *
 * This API performs a write operation to a register using the hardware interface /dev/axi_fpga
 * when selecting axi_fpga. If necessary, the low-level interface can be changed to use the real SPI.
 *
 * @param[in] phy Structure instance of rf_chip_phy_t.
 * @note Refer to the user guide for detailed information.
 *
 * @return Result of the API execution status.
 * @retval 0 -> Success
 * @retval < 0 -> Fail
 */
int32_t ldo_init(rf_chip_phy_t *phy);

/*!
 * \ingroup transceiverApiXoSelt
 * \page transceiver_api_xo_selt xo_selt
 * \code
 * int32_t xo_selt(rf_chip_phy_t *phy, enum xo_type xo_type);
 * \endcode
 * @details Selects the XO mode between crystal and external clock source for the transceiver chip.
 *
 * This API performs a write operation to a register using the hardware interface /dev/axi_fpga
 * when selecting axi_fpga. If necessary, the low-level interface can be changed to use the real SPI.
 *
 * @param[in] phy Structure instance of rf_chip_phy_t.
 * @param[in] xo_sel Enum instance of xo_type.
 *                    - TCXO: Use the external clock source.
 *                    - XTAL: Use the plate-level crystal.
 * @note Refer to the user guide for detailed information.
 *
 * @return Result of the API execution status.
 * @retval 0 -> Success
 * @retval < 0 -> Fail
 */
int32_t xo_selt(rf_chip_phy_t *phy, xo_type xo_sel);

/*!
 * \ingroup transceiverApiX4Init
 * \page transceiver_api_x4_init x4_init
 * \code
 * int32_t x4_init(rf_chip_phy_t *phy);
 * \endcode
 * @details Initializes the reference clock X4 module for the transceiver chip.
 *
 * This API performs a write operation to a register using the hardware interface /dev/axi_fpga
 * when selecting axi_fpga. If necessary, the low-level interface can be changed to use the real SPI.
 *
 * @param[in] phy Structure instance of rf_chip_phy_t.
 * @note Refer to the user guide for detailed information.
 *
 * @return Result of the API execution status.
 * @retval 0 -> Success
 * @retval < 0 -> Fail
 */
int32_t x4_init(rf_chip_phy_t *phy);

/*!
 * \ingroup transceiverApiRefClkConfig
 * \page transceiver_api_refclk_config refclk_config
 * \code
 * int32_t refclk_config(rf_chip_phy_t *phy, bool x4_en);
 * \endcode
 * @details Configures the REFCLK of SX into X4 mode or X2 mode for the transceiver chip.
 *
 * This API performs a write operation to a register using the hardware interface /dev/axi_fpga
 * when selecting axi_fpga. If necessary, the low-level interface can be changed to use the real SPI.
 *
 * @param[in] phy Structure instance of rf_chip_phy_t.
 * @param[in] x4_en Boolean variable to select the configuration when x4 mode is enabled or not.
 *                  - true: Enable X4 mode
 *                  - false: Disable X4 mode
 *
 * @note Refer to the user guide for detailed information.
 *
 * @return Result of the API execution status.
 * @retval 0 -> Success
 * @retval < 0 -> Fail
 */
int32_t refclk_config(rf_chip_phy_t *phy, bool x4_en);

/*!
 * \ingroup transceiverApiSysPllInit
 * \page transceiver_api_syspll_init syspll_init
 * \code
 * int32_t syspll_init(rf_chip_phy_t *phy);
 * \endcode
 * @details Initializes the SYSPLL module.
 *
 * This API performs a write operation to a register using the hardware interface /dev/axi_fpga
 * when selecting axi_fpga. If necessary, the low-level interface can be changed to use the real SPI.
 *
 * @param[in] phy Structure instance of rf_chip_phy_t.
 * @note Refer to the user guide for detailed information.
 *
 * @return Result of the API execution status.
 * @retval 0 -> Success
 * @retval < 0 -> Fail
 */
int32_t syspll_init(rf_chip_phy_t *phy);

/*!
 * \ingroup transceiverApiSxInit
 * \page transceiver_api_sx_init sx_init
 * \code
 * int32_t sx_init(rf_chip_phy_t *phy);
 * \endcode
 * @details Initializes the SX BIAS module.
 *
 * This API performs a write operation to a register using the hardware interface /dev/axi_fpga
 * when selecting axi_fpga. If necessary, the low-level interface can be changed to use the real SPI.
 *
 * @param[in] phy Structure instance of rf_chip_phy_t.
 * @note Refer to the user guide for detailed information.
 *
 * @return Result of the API execution status.
 * @retval 0 -> Success
 * @retval < 0 -> Fail
 */
int32_t sx_init(rf_chip_phy_t *phy);

/*!
 * \ingroup transceiverApiSxRxInit
 * \page transceiver_api_sxrx_init sxrx_init
 * \code
 * int32_t sxrx_init(rf_chip_phy_t *phy);
 * \endcode
 * @details Initializes SX BIAS module.
 *
 * This API performs a write operation to a register using the hardware interface /dev/axi_fpga
 * when selecting axi_fpga. If necessary, the low-level interface can be changed to use the real SPI.
 *
 * @param[in] phy Structure instance of rf_chip_phy_t.
 * @param[in] fdd_en Boolean variable to enable FDD mode.
 *                   - true: Enable FDD mode.
 *                   - false: Disable FDD mode. 
 * @note Refer to the user guide for detailed information.
 *
 * @return Result of the API execution status.
 * @retval 0 -> Success
 * @retval < 0 -> Fail
 */
int32_t sxrx_init(rf_chip_phy_t *phy, bool fdd_en);
    
/*!
 * \ingroup transceiverApiSxTxInit
 * \page transceiver_api_sxtx_init sxtx_init
 * \code
 * int32_t sxtx_init(rf_chip_phy_t *phy);
 * \endcode
 * @details Initializes the SXTX module.
 *
 * This API performs a write operation to a register using the hardware interface /dev/axi_fpga
 * when selecting axi_fpga. If necessary, the low-level interface can be changed to use the real SPI.
 *
 * @param[in] phy Structure instance of rf_chip_phy_t.
 * @note Refer to the user guide for detailed information.
 *
 * @return Result of the API execution status.
 * @retval 0 -> Success
 * @retval < 0 -> Fail
 */
int32_t sxtx_init(rf_chip_phy_t *phy);

/*!
 * \ingroup transceiverApiManLoConfig
 * \page transceiver_api_man_lo_config man_lo_config
 * \code
 * int32_t man_lo_config(rf_chip_phy_t *phy, bool fdd_en);
 * \endcode
 * @details Configures LO into FDD or TDD mode in manual mode.
 *
 * This API performs a write operation to a register using the hardware interface /dev/axi_fpga
 * when selecting axi_fpga. If necessary, the low-level interface can be changed to use the real SPI.
 *
 * @param[in] phy Structure instance of rf_chip_phy_t.
 * @param[in] fdd_en Boolean variable to enable FDD mode.
 *                   - true: Enable FDD mode.
 *                   - false: Disable FDD mode.
 *
 * @note Refer to the user guide for detailed information.
 *
 * @return Result of the API execution status.
 * @retval 0 -> Success
 * @retval < 0 -> Fail
 */
int32_t man_lo_config(rf_chip_phy_t *phy, bool fdd_en);

/*!
 * \ingroup transceiverApiRx1Init
 * \page transceiver_api_rx1_init rx1_init
 * \code
 * int32_t rx1_init(rf_chip_phy_t *phy);
 * \endcode
 * @details Initializes RX1.
 *
 * This API performs a write operation to a register using the hardware interface /dev/axi_fpga
 * when selecting axi_fpga. If necessary, the low-level interface can be changed to use the real SPI.
 *
 * @param[in] phy Structure instance of rf_chip_phy_t.
 *
 * @note Refer to the user guide for detailed information.
 *
 * @return Result of the API execution status.
 * @retval 0 -> Success
 * @retval < 0 -> Fail
 */
int32_t rx1_init(rf_chip_phy_t *phy);

/*!
 * \ingroup transceiverApiRx2Init
 * \page transceiver_api_rx2_init rx2_init
 * \code
 * int32_t rx2_init(rf_chip_phy_t *phy);
 * \endcode
 * @details Initializes RX2.
 *
 * This API performs a write operation to a register using the hardware interface /dev/axi_fpga
 * when selecting axi_fpga. If necessary, the low-level interface can be changed to use the real SPI.
 *
 * @param[in] phy Structure instance of rf_chip_phy_t.
 *
 * @note Refer to the user guide for detailed information.
 *
 * @return Result of the API execution status.
 * @retval 0 -> Success
 * @retval < 0 -> Fail
 */
int32_t rx2_init(rf_chip_phy_t *phy);

/*!
 * \ingroup transceiverApiRxAdcInit
 * \page transceiver_api_rxadc_init rxadc_init
 * \code
 * int32_t rxadc_init(rf_chip_phy_t *phy);
 * \endcode
 * @details Initializes RXADC.
 *
 * This API performs a write operation to a register using the hardware interface /dev/axi_fpga
 * when selecting axi_fpga. If necessary, the low-level interface can be changed to use the real SPI.
 *
 * @param[in] phy Structure instance of rf_chip_phy_t.
 *
 * @note Refer to the user guide for detailed information.
 *
 * @return Result of the API execution status.
 * @retval 0 -> Success
 * @retval < 0 -> Fail
 */
int32_t rxadc_init(rf_chip_phy_t *phy);

/*!
 * \ingroup transceiverApiRx1AdcWeightInit
 * \page transceiver_api_rx1adc_weight_init rx1adc_weight_init
 * \code
 * int32_t rx1adc_weight_init(rf_chip_phy_t *phy);
 * \endcode
 * @details Initializes RX1ADC weight settings.
 *
 * This API performs a write operation to a register using the hardware interface /dev/axi_fpga
 * when selecting axi_fpga. If necessary, the low-level interface can be changed to use the real SPI.
 *
 * @param[in] phy Structure instance of rf_chip_phy_t.
 *
 * @note Refer to the user guide for detailed information.
 *
 * @return Result of the API execution status.
 * @retval 0 -> Success
 * @retval < 0 -> Fail
 */
int32_t rx1adc_weight_init(rf_chip_phy_t *phy);

/*!
 * \ingroup transceiverApiRx2AdcWeightInit
 * \page transceiver_api_rx2adc_weight_init rx2adc_weight_init
 * \code
 * int32_t rx2adc_weight_init(rf_chip_phy_t *phy);
 * \endcode
 * @details Initializes RX2ADC weight settings.
 *
 * This API performs a write operation to a register using the hardware interface /dev/axi_fpga
 * when selecting axi_fpga. If necessary, the low-level interface can be changed to use the real SPI.
 *
 * @param[in] phy Structure instance of rf_chip_phy_t.
 *
 * @note Refer to the user guide for detailed information.
 *
 * @return Result of the API execution status.
 * @retval 0 -> Success
 * @retval < 0 -> Fail
 */
int32_t rx2adc_weight_init(rf_chip_phy_t *phy);

/*!
 * \ingroup transceiverApiTx1Init
 * \page transceiver_api_tx1_init tx1_init
 * \code
 * int32_t tx1_init(rf_chip_phy_t *phy);
 * \endcode
 * @details Initializes TX1.
 *
 * This API performs a write operation to a register using the hardware interface /dev/axi_fpga
 * when selecting axi_fpga. If necessary, the low-level interface can be changed to use the real SPI.
 *
 * @param[in] phy Structure instance of rf_chip_phy_t.
 *
 * @note Refer to the user guide for detailed information.
 *
 * @return Result of the API execution status.
 * @retval 0 -> Success
 * @retval < 0 -> Fail
 */
int32_t tx1_init(rf_chip_phy_t *phy);

/*!
 * \ingroup transceiverApiTx2Init
 * \page transceiver_api_tx2_init tx2_init
 * \code
 * int32_t tx2_init(rf_chip_phy_t *phy);
 * \endcode
 * @details Initializes TX2.
 *
 * This API performs a write operation to a register using the hardware interface /dev/axi_fpga
 * when selecting axi_fpga. If necessary, the low-level interface can be changed to use the real SPI.
 *
 * @param[in] phy Structure instance of rf_chip_phy_t.
 *
 * @note Refer to the user guide for detailed information.
 *
 * @return Result of the API execution status.
 * @retval 0 -> Success
 * @retval < 0 -> Fail
 */
int32_t tx2_init(rf_chip_phy_t *phy);

/*!
 * \ingroup transceiverApiTx1DacInit
 * \page transceiver_api_tx1dac_init tx1dac_init
 * \code
 * int32_t tx1dac_init(rf_chip_phy_t *phy);
 * \endcode
 * @details Initializes TX1DAC.
 *
 * This API performs a write operation to a register using the hardware interface /dev/axi_fpga
 * when selecting axi_fpga. If necessary, the low-level interface can be changed to use the real SPI.
 *
 * @param[in] phy Structure instance of rf_chip_phy_t.
 *
 * @note Refer to the user guide for detailed information.
 *
 * @return Result of the API execution status.
 * @retval 0 -> Success
 * @retval < 0 -> Fail
 */
int32_t tx1dac_init(rf_chip_phy_t *phy);

/*!
 * \ingroup transceiverApiTx2DacInit
 * \page transceiver_api_tx2dac_init tx2dac_init
 * \code
 * int32_t tx2dac_init(rf_chip_phy_t *phy);
 * \endcode
 * @details Initializes TX2DAC.
 *
 * This API performs a write operation to a register using the hardware interface /dev/axi_fpga
 * when selecting axi_fpga. If necessary, the low-level interface can be changed to use the real SPI.
 *
 * @param[in] phy Structure instance of rf_chip_phy_t.
 *
 * @note Refer to the user guide for detailed information.
 *
 * @return Result of the API execution status.
 * @retval 0 -> Success
 * @retval < 0 -> Fail
 */
int32_t tx2dac_init(rf_chip_phy_t *phy);

/*!
 * \ingroup transceiverApiForceWait
 * \page transceiver_api_force_wait force_wait
 * \code
 * int32_t force_wait(rf_chip_phy_t *phy);
 * \endcode
 * @details Sets the transceiver chip to the wait state.
 *
 * This API performs a write operation to a register using the hardware interface /dev/axi_fpga
 * when selecting axi_fpga. If necessary, the low-level interface can be changed to use the real SPI.
 *
 * @param[in] phy Structure instance of rf_chip_phy_t.
 *
 * @note Refer to the user guide for detailed information.
 *
 * @return Result of the API execution status.
 * @retval 0 -> Success
 * @retval < 0 -> Fail
 */
int32_t force_wait(rf_chip_phy_t *phy);

/*!
 * \ingroup transceiverApiXoFcOff
 * \page transceiver_api_xo_fc_off xo_fc_off
 * \code
 * int32_t xo_fc_off(rf_chip_phy_t *phy);
 * \endcode
 * @details Disables XO fast charge.
 *
 * This API performs a write operation to a register using the hardware interface /dev/axi_fpga
 * when selecting axi_fpga. If necessary, the low-level interface can be changed to use the real SPI.
 *
 * @param[in] phy Structure instance of rf_chip_phy_t.
 *
 * @note Refer to the user guide for detailed information.
 *
 * @return Result of the API execution status.
 * @retval 0 -> Success
 * @retval < 0 -> Fail
 */
int32_t xo_fc_off(rf_chip_phy_t *phy);

/*!
 * \ingroup transceiverApiCmFcOff
 * \page transceiver_api_cm_fc_off cm_fc_off
 * \code
 * int32_t cm_fc_off(rf_chip_phy_t *phy);
 * \endcode
 * @details Disables CM MDIG LDO fast charge and CM IVREF fast charge.
 *
 * This API performs a write operation to a register using the hardware interface /dev/axi_fpga
 * when selecting axi_fpga. If necessary, the low-level interface can be changed to use the real SPI.
 *
 * @param[in] phy Structure instance of rf_chip_phy_t.
 *
 * @note Refer to the user guide for detailed information.
 *
 * @return Result of the API execution status.
 * @retval 0 -> Success
 * @retval < 0 -> Fail
 */
int32_t cm_fc_off(rf_chip_phy_t *phy);

/*!
 * \ingroup transceiverApiTransceiverCheckdevice
 * \page transceiver_api_transceiver_checkdevice transceiver_checkdevice
 * \code
 * int32_t transceiver_checkdevice(rf_chip_phy_t *phy);
 * \endcode
 * @details This API check whether the transceiver device exist or not.
 *
 * This API performs a write operation to a register using the hardware interface /dev/axi_fpga
 * when selecting axi_fpga. If necessary, the low-level interface can be changed to use the real SPI.
 *
 * @param[in] phy Structure instance of rf_chip_phy_t.
 *
 * @note Refer to the user guide for detailed information.
 *
 * @return Result of the API execution status.
 * @retval 0 -> Success
 * @retval < 0 -> Fail
 */
 
int32_t transceiver_checkdevice(rf_chip_phy_t * phy);

/*!
 * \ingroup transceiverApiPower_init
 * \page transceiver_api_power_init power_init
 * \code
 * int32_t power_init(rf_chip_phy_t *phy);
 * \endcode
 * @details Power up whole transceiver.
 *
 * This API performs a write operation to a register using the hardware interface /dev/axi_fpga
 * when selecting axi_fpga. If necessary, the low-level interface can be changed to use the real SPI.
 *
 * @param[in] phy Structure instance of rf_chip_phy_t.
 *
 * @note Refer to the user guide for detailed information.
 *
 * @return Result of the API execution status.
 * @retval 0 -> Success
 * @retval < 0 -> Fail
 */
int32_t power_init(rf_chip_phy_t *phy);

/*!
 * \ingroup transceiverApiAnalog_init
 * \page transceiver_api_analog_init analog_init
 * \code
 * int32_t analog_init(rf_chip_phy_t *phy);
 * \endcode
 * @details init all modules.
 *
 * This API performs a write operation to a register using the hardware interface /dev/axi_fpga
 * when selecting axi_fpga. If necessary, the low-level interface can be changed to use the real SPI.
 *
 * @param[in] phy Structure instance of rf_chip_phy_t.
 *
 * @note Refer to the user guide for detailed information.
 *
 * @return Result of the API execution status.
 * @retval 0 -> Success
 * @retval < 0 -> Fail
 */
int32_t analog_init(rf_chip_phy_t *phy);

/*!
 * \ingroup transceiverApiTransceiverInit
 * \page transceiver_api_transceiver_init transceiver_init
 * \code
 * int32_t transceiver_init(rf_chip_phy_t *phy);
 * \endcode
 * @details Initial whole transceiver.
 *
 * This API performs a write operation to a register using the hardware interface /dev/axi_fpga
 * when selecting axi_fpga. If necessary, the low-level interface can be changed to use the real SPI.
 *
 * @param[in] phy Structure instance of rf_chip_phy_t.
 *
 * @note Refer to the user guide for detailed information.
 *
 * @return Result of the API execution status.
 * @retval 0 -> Success
 * @retval < 0 -> Fail
 */
int32_t transceiver_init(rf_chip_phy_t *phy);

/*!
 * \ingroup transceiverApiAuxadcctrlBackup
 * \page transceiver_api_auxadcctrl_backup auxadcctrl_backup
 * \code
 * int32_t auxadcctrl_backup(rf_chip_phy_t *phy, MT_AUXADC_ENUM mt, auxadc_ctrl *ctrl);
 * \endcode
 * @details backup the related control registers for AUXADC.
 *  there are three task need to utilize the AUXADC. lmt_pdt (analog peak deltect), 
 *  voltage_dt (voltage detection) and temper_dt (temperature detection). And in the RX state, 
 *  AUXADC is basically all used for the detection of lmt_pdt. Once auxadc cut to voltage_t or 
 *  temper_t, we need to backup some control register for protection, then do voltage_t or temper_t.
 *  After the secondment is complete, we need to recover these control register.
 *
 * This API performs a write operation to a register using the hardware interface /dev/axi_fpga
 * when selecting axi_fpga. If necessary, the low-level interface can be changed to use the real SPI.
 *
 * @param[in] phy Structure instance of rf_chip_phy_t.
 * @param[in] mt  Enum instance of MT_AUXADC_ENUM.
 * @param[in, out] ctrl Structure instance of auxadc_ctrl, backup for control register value.
 * @note Refer to the user guide for detailed information.
 *
 * @return Result of the API execution status.
 * @retval 0 -> Success
 * @retval < 0 -> Fail
 */
int32_t auxadcctrl_backup(rf_chip_phy_t *phy, MT_AUXADC_ENUM mt, auxadc_ctrl *ctrl);

/*!
 * \ingroup transceiverApiAuxadcctrlRecover
 * \page transceiver_api_auxadcctrl_recover auxadcctrl_recover
 * \code
 * int32_t auxadcctrl_recover(rf_chip_phy_t *phy, MT_AUXADC_ENUM mt, auxadc_ctrl *ctrl);
 * \endcode
 * @details recover the related control registers for AUXADC.
 *  there are three task need to utilize the AUXADC. lmt_pdt (analog peak deltect), 
 *  voltage_dt (voltage detection) and temper_dt (temperature detection). And in the RX state, 
 *  AUXADC is basically all used for the detection of lmt_pdt. Once auxadc cut to voltage_t or 
 *  temper_t, we need to backup some control register for protection, then do voltage_t or temper_t.
 *  After the secondment is complete, we need to recover these control register.
 *
 * This API performs a write operation to a register using the hardware interface /dev/axi_fpga
 * when selecting axi_fpga. If necessary, the low-level interface can be changed to use the real SPI.
 *
 * @param[in] phy Structure instance of rf_chip_phy_t.
 * @param[in] mt  Enum instance of MT_AUXADC_ENUM.
 * @param[in, out] ctrl Structure instance of auxadc_ctrl, backup for control register value.
 * @note Refer to the user guide for detailed information.
 *
 * @return Result of the API execution status.
 * @retval 0 -> Success
 * @retval < 0 -> Fail
 */

int32_t auxadcctrl_recover(rf_chip_phy_t *phy, MT_AUXADC_ENUM mt, auxadc_ctrl *ctrl);

/*!
 * \ingroup transceiverApiAuxadc1Reset
 * \page transceiver_api_auxadc1_reset auxadc1_reset
 * \code
 * int32_t auxadc1_reset(rf_chip_phy_t *phy);
 * \endcode
 * @details Reset the AUXADC1.
 *
 * This API performs a write operation to a register using the hardware interface /dev/axi_fpga
 * when selecting axi_fpga. If necessary, the low-level interface can be changed to use the real SPI.
 *
 * @param[in] phy Structure instance of rf_chip_phy_t.
 *
 * @note Refer to the user guide for detailed information.
 *
 * @return Result of the API execution status.
 * @retval 0 -> Success
 * @retval < 0 -> Fail
 */
int32_t auxadc1_reset(rf_chip_phy_t *phy);

/*!
 * \ingroup transceiverApiAuxadc2Reset
 * \page transceiver_api_auxadc2_reset auxadc2_reset
 * \code
 * int32_t auxadc2_reset(rf_chip_phy_t *phy);
 * \endcode
 * @details Reset the AUXADC2.
 *
 * This API performs a write operation to a register using the hardware interface /dev/axi_fpga
 * when selecting axi_fpga. If necessary, the low-level interface can be changed to use the real SPI.
 *
 * @param[in] phy Structure instance of rf_chip_phy_t.
 *
 * @note Refer to the user guide for detailed information.
 *
 * @return Result of the API execution status.
 * @retval 0 -> Success
 * @retval < 0 -> Fail
 */
int32_t auxadc2_reset(rf_chip_phy_t *phy);

/*!
 * \ingroup transceiverApiGetAuxadcCalcVal
 * \page transceiver_api_get_auxadc_calc_val get_auxadc_calc_val
 * \code
 * int32_t get_auxadc_calc_val(rf_chip_phy_t *phy, TRX_CHN_ENUM chn, char swap_flag, int &value);
 * \endcode
 * @details Read tx dc calibration value.
 *
 * This API performs a write operation to a register using the hardware interface /dev/axi_fpga
 * when selecting axi_fpga. If necessary, the low-level interface can be changed to use the real SPI.
 *
 * @param[in] phy           :Structure instance of rf_chip_phy_t.
 * @param[in] chn           :Enum instance of TRX_CHN_ENUM.
 * @param[in] swap_flag     :swap flag, 0 --swap, 1--no swap.
 * @param[in out] value     :pointer to the adress of calc value.
 *
 * @note Refer to the user guide for detailed information.
 *
 * @return Result of the API execution status.
 * @retval 0 -> Success
 * @retval < 0 -> Fail
 */ 
int32_t get_auxadc_calc_val(rf_chip_phy_t *phy, TRX_CHN_ENUM chn, int8_t swap_flag, int32_t *value);

/*!
 * \ingroup transceiverApitx_dc_offset_cal_with_auxadc
 * \page transceiver_api_tx_dc_offset_cal_with_auxadc tx_dc_offset_cal_with_auxadc
 * \code
 * int32_t tx_dc_offset_cal_with_auxadc(rf_chip_phy_t *phy, TRX_CHN_ENUM chn, unsigned short tune_reg, int retry);
 * \endcode
 * @details Do tx dc offset cal with auxadc.
 *
 * This API performs a write operation to a register using the hardware interface /dev/axi_fpga
 * when selecting axi_fpga. If necessary, the low-level interface can be changed to use the real SPI.
 *
 * @param[in] phy           :Structure instance of rf_chip_phy_t.
 * @param[in] chn           :Enum instance of TRX_CHN_ENUM. 
 * @param[in] tune_reg      :the tune register.
 * @param[in] retry         :retry times. 1 --no retry, >1 --retry time when fail
 *
 * @note Refer to the user guide for detailed information.
 *
 * @return Result of the API execution status.
 * @retval 0 -> Success
 * @retval < 0 -> Fail
 */ 
int32_t tx_dc_offset_cal_with_auxadc(rf_chip_phy_t *phy, TRX_CHN_ENUM chn, uint16_t tune_reg, int32_t retry);

/*!
 * \ingroup transceiverApiRcal
 * \page transceiver_api_rcal rcal
 * \code
 * int32_t rcal(rf_chip_phy_t *phy);
 * \endcode
 * @details Do R cal.
 *
 * This API performs a write operation to a register using the hardware interface /dev/axi_fpga
 * when selecting axi_fpga. If necessary, the low-level interface can be changed to use the real SPI.
 *
 * @param[in] phy           :Structure instance of rf_chip_phy_t.
 *
 * @note Refer to the user guide for detailed information.
 *
 * @return Result of the API execution status.
 * @retval 0 -> Success
 * @retval < 0 -> Fail
 */  
int32_t rcal(rf_chip_phy_t *phy);

/*!
 * \ingroup transceiverApiAuxadcCal
 * \page transceiver_api_auxadc_cal auxadc_cal
 * \code
 * int32_t auxadc_cal(rf_chip_phy_t *phy);
 * \endcode
 * @details Read auxadc cal oridinate and ordinate from efuse in the chip..
 *
 * This API performs a write operation to a register using the hardware interface /dev/axi_fpga
 * when selecting axi_fpga. If necessary, the low-level interface can be changed to use the real SPI.
 *
 * @param[in] phy           :Structure instance of rf_chip_phy_t.
 *
 * @note Refer to the user guide for detailed information.
 *
 * @return Result of the API execution status.
 * @retval 0 -> Success
 * @retval < 0 -> Fail
 */  
int32_t auxadc_cal(rf_chip_phy_t *phy);

/*!
 * \ingroup transceiverApiAuxadccalTransfer
 * \page transceiver_api_auxadccal_transfer auxadccal_transfer
 * \code
 * int32_t auxadccal_transfer(rf_chip_phy_t *phy, int *value);
 * \endcode
 * @details Transfer original value reading from auxadc to target value by auxadc cal parameter.
 *
 * This API performs a write operation to a register using the hardware interface /dev/axi_fpga
 * when selecting axi_fpga. If necessary, the low-level interface can be changed to use the real SPI.
 *
 * @param[in] phy           :Structure instance of rf_chip_phy_t.
 * @param[in out] value     :pointer to the adress of calc value.
 *
 * @note Refer to the user guide for detailed information.
 *
 * @return Result of the API execution status.
 * @retval 0 -> Success
 * @retval < 0 -> Fail
 */
int32_t auxadccal_transfer(rf_chip_phy_t *phy, int *value);

/*!
 * \ingroup transceiverApiExtnRegInit
 * \page transceiver_api_extn_reg_init extn_reg_init
 * \code
 * int32_t extn_reg_init(rf_chip_phy_t *phy, char *file);
 * \endcode
 * @details Write data to the register by the list from the file. 
 *  In the file, in each line, it contains the register address string and value string, and they start with "0x" 
 *
 * This API performs a write operation to a register using the hardware interface /dev/axi_fpga
 * when selecting axi_fpga. If necessary, the low-level interface can be changed to use the real SPI.
 *
 * @param[in] phy           :Structure instance of rf_chip_phy_t.
 * @param[in] file          :pointer to the file store the register list.
 *
 * @note Refer to the user guide for detailed information.
 *
 * @return void.
 */
void extn_reg_init(rf_chip_phy_t *phy, char *file);

/*!
 * \ingroup transceiverApiExtnRegPrint
 * \page transceiver_api_extn_reg_print extn_reg_print
 * \code
 * int32_t extn_reg_print(rf_chip_phy_t *phy, char *file, char *outfile);
 * \endcode
 * @details Read the register list from the file and Write the result back to outfile.. 
 *  In the file, in each line, it contain the register address start with "0x" only.
 *  In the outfile, in each line, it contains the register address string and value string, and they start with "0x" 
 *
 * This API performs a write operation to a register using the hardware interface /dev/axi_fpga
 * when selecting axi_fpga. If necessary, the low-level interface can be changed to use the real SPI.
 *
 * @param[in] phy           :Structure instance of rf_chip_phy_t.
 * @param[in] file          :pointer to the file store the register list.
 * @param[out] outfile      :pointer to the file store the result by register list.
 *
 * @note Refer to the user guide for detailed information.
 *
 * @return void.
 */ 
void extn_reg_print(rf_chip_phy_t *phy, char *file, char *outfile);

#ifdef __cplusplus
}
#endif

#endif  /* __TRANSCEIVER_API_H */

