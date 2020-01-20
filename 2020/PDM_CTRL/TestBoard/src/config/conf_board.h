/**
 * \file
 *
 * \brief User board configuration template
 *
 * Copyright (c) 2014-2018 Microchip Technology Inc. and its subsidiaries.
 *
 * \asf_license_start
 *
 * \page License
 *
 * Subject to your compliance with these terms, you may use Microchip
 * software and any derivatives exclusively with Microchip products.
 * It is your responsibility to comply with third party license terms applicable
 * to your use of third party software (including open source software) that
 * may accompany Microchip software.
 *
 * THIS SOFTWARE IS SUPPLIED BY MICROCHIP "AS IS". NO WARRANTIES,
 * WHETHER EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS SOFTWARE,
 * INCLUDING ANY IMPLIED WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY,
 * AND FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT WILL MICROCHIP BE
 * LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE, INCIDENTAL OR CONSEQUENTIAL
 * LOSS, DAMAGE, COST OR EXPENSE OF ANY KIND WHATSOEVER RELATED TO THE
 * SOFTWARE, HOWEVER CAUSED, EVEN IF MICROCHIP HAS BEEN ADVISED OF THE
 * POSSIBILITY OR THE DAMAGES ARE FORESEEABLE.  TO THE FULLEST EXTENT
 * ALLOWED BY LAW, MICROCHIP'S TOTAL LIABILITY ON ALL CLAIMS IN ANY WAY
 * RELATED TO THIS SOFTWARE WILL NOT EXCEED THE AMOUNT OF FEES, IF ANY,
 * THAT YOU HAVE PAID DIRECTLY TO MICROCHIP FOR THIS SOFTWARE.
 *
 * \asf_license_stop
 *
 */

#ifndef CONF_BOARD_H
#define CONF_BOARD_H
#define LED_0_PIN PIN_PB22
#define LED_1_PIN PIN_PB23
#define RAD_FAN_ENABLE_PIN PIN_PA17
#define FUEL_PUMP_ENABLE_PIN PIN_PA18

/*
CAN
*/
// CAN0 = CAN0 (On Schematic)
#define CAN0_TX_MUX_SETTING MUX_PA24G_CAN0_TX
#define CAN0_RX_MUX_SETTING MUX_PA25G_CAN0_RX
#define CAN0_TX_PIN PIN_PA24
#define CAN0_RX_PIN PIN_PA25
#define CAN0_MODULE CAN0
//#define CAN0_TX_BUFFER_INDEX 0

// CAN1 = CAN1 (On Schematic)
#define CAN1_TX_MUX_SETTING MUX_PB10G_CAN1_TX
#define CAN1_RX_MUX_SETTING MUX_PB11G_CAN1_RX
#define CAN1_TX_PIN PIN_PB10
#define CAN1_RX_PIN PIN_PB11
#define CAN1_MODULE CAN1
//#define CAN1_TX_BUFFER_INDEX 1

#endif // CONF_BOARD_H
