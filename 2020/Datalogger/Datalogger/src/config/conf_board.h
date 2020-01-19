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
//#define LED_0_PIN PIN_PA28
#define LED_0_PIN PIN_PB12

//#define CAN_TX_MUX_SETTING MUX_PA24G_CAN0_TX
//#define CAN_RX_MUX_SETTING MUX_PA25G_CAN0_RX
//#define CAN_TX_PIN PIN_PA24
//#define CAN_RX_PIN PIN_PA25
#define CAN_TX_MUX_SETTING MUX_PA24G_CAN0_TX
#define CAN_RX_MUX_SETTING MUX_PA25G_CAN0_RX
#define CAN_TX_PIN PIN_PA24
#define CAN_RX_PIN PIN_PA25
#define CAN_MODULE CAN0

#define USART_BAUD 115200
#define USART_MUX_SETTING USART_RX_3_TX_2_XCK_3
#define USART_PINMUX_PAD0 PINMUX_UNUSED
#define USART_PINMUX_PAD1 PINMUX_UNUSED
#define USART_PINMUX_PAD2 PINMUX_PA18C_SERCOM1_PAD2
#define USART_PINMUX_PAD3 PINMUX_PA19C_SERCOM1_PAD3
#define USART_MODULE SERCOM1

#define RTC_PINMUX_PAD0 PINMUX_PA22C_SERCOM3_PAD0
#define RTC_PINMUX_PAD1 PINMUX_PA23C_SERCOM3_PAD1
#define RTC_MODULE SERCOM3




#endif // CONF_BOARD_H