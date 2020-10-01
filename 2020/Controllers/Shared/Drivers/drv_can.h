#pragma once

#include "drv_can_private.h"
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include "sam.h"

/**
 * Using structure definitions from SAMC21_DFP version 1.2.176
 *
 * \brief Component description for CAN
 *
 * Copyright (c) 2018 Microchip Technology Inc.
 *
 * \asf_license_start
 *
 * \page License
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the Licence at
 * 
 * http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * \asf_license_stop
 *
 */

typedef union {
  struct {
    uint32_t ID:29;            /*!< bit:  0..28  Identifier                         */
    uint32_t RTR:1;            /*!< bit:     29  Remote Transmission Request        */
    uint32_t XTD:1;            /*!< bit:     30  Extended Identifier                */
    uint32_t ESI:1;            /*!< bit:     31  Error State Indicator              */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} CAN_RXBE_0_Type;

typedef union {
  struct {
    uint32_t RXTS:16;          /*!< bit:  0..15  Rx Timestamp                       */
    uint32_t DLC:4;            /*!< bit: 16..19  Data Length Code                   */
    uint32_t BRS:1;            /*!< bit:     20  Bit Rate Search                    */
    uint32_t FDF:1;            /*!< bit:     21  FD Format                          */
    uint32_t :2;               /*!< bit: 22..23  Reserved                           */
    uint32_t FIDX:7;           /*!< bit: 24..30  Filter Index                       */
    uint32_t ANMF:1;           /*!< bit:     31  Accepted Non-matching Frame        */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} CAN_RXBE_1_Type;

typedef union {
  struct {
    uint32_t ID:29;            /*!< bit:  0..28  Identifier                         */
    uint32_t RTR:1;            /*!< bit:     29  Remote Transmission Request        */
    uint32_t XTD:1;            /*!< bit:     30  Extended Identifier                */
    uint32_t ESI:1;            /*!< bit:     31  Error State Indicator              */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} CAN_RXF0E_0_Type;

typedef union {
  struct {
    uint32_t RXTS:16;          /*!< bit:  0..15  Rx Timestamp                       */
    uint32_t DLC:4;            /*!< bit: 16..19  Data Length Code                   */
    uint32_t BRS:1;            /*!< bit:     20  Bit Rate Search                    */
    uint32_t FDF:1;            /*!< bit:     21  FD Format                          */
    uint32_t :2;               /*!< bit: 22..23  Reserved                           */
    uint32_t FIDX:7;           /*!< bit: 24..30  Filter Index                       */
    uint32_t ANMF:1;           /*!< bit:     31  Accepted Non-matching Frame        */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} CAN_RXF0E_1_Type;

typedef union {
  struct {
    uint32_t DB0:8;            /*!< bit:  0.. 7  Data Byte 0                        */
    uint32_t DB1:8;            /*!< bit:  8..15  Data Byte 1                        */
    uint32_t DB2:8;            /*!< bit: 16..23  Data Byte 2                        */
    uint32_t DB3:8;            /*!< bit: 24..31  Data Byte 3                        */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} CAN_RXF0E_DATA_Type;

typedef union {
  struct {
    uint32_t ID:29;            /*!< bit:  0..28  Identifier                         */
    uint32_t RTR:1;            /*!< bit:     29  Remote Transmission Request        */
    uint32_t XTD:1;            /*!< bit:     30  Extended Identifier                */
    uint32_t ESI:1;            /*!< bit:     31  Error State Indicator              */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} CAN_RXF1E_0_Type;

typedef union {
  struct {
    uint32_t RXTS:16;          /*!< bit:  0..15  Rx Timestamp                       */
    uint32_t DLC:4;            /*!< bit: 16..19  Data Length Code                   */
    uint32_t BRS:1;            /*!< bit:     20  Bit Rate Search                    */
    uint32_t FDF:1;            /*!< bit:     21  FD Format                          */
    uint32_t :2;               /*!< bit: 22..23  Reserved                           */
    uint32_t FIDX:7;           /*!< bit: 24..30  Filter Index                       */
    uint32_t ANMF:1;           /*!< bit:     31  Accepted Non-matching Frame        */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} CAN_RXF1E_1_Type;

typedef union {
  struct {
    uint32_t SFID2:11;         /*!< bit:  0..10  Standard Filter ID 2               */
    uint32_t :5;               /*!< bit: 11..15  Reserved                           */
    uint32_t SFID1:11;         /*!< bit: 16..26  Standard Filter ID 1               */
    uint32_t SFEC:3;           /*!< bit: 27..29  Standard Filter Element Configuration */
    uint32_t SFT:2;            /*!< bit: 30..31  Standard Filter Type               */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} CAN_SIDFE_0_Type;

typedef union {
  struct {
    uint32_t ID:29;            /*!< bit:  0..28  Identifier                         */
    uint32_t RTR:1;            /*!< bit:     29  Remote Transmission Request        */
    uint32_t XTD:1;            /*!< bit:     30  Extended Identifier                */
    uint32_t ESI:1;            /*!< bit:     31  Error State Indicator              */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} CAN_TXBE_0_Type;

typedef union {
  struct {
    uint32_t :16;              /*!< bit:  0..15  Reserved                           */
    uint32_t DLC:4;            /*!< bit: 16..19  Identifier                         */
    uint32_t BRS:1;            /*!< bit:     20  Bit Rate Search                    */
    uint32_t FDF:1;            /*!< bit:     21  FD Format                          */
    uint32_t :1;               /*!< bit:     22  Reserved                           */
    uint32_t EFC:1;            /*!< bit:     23  Event FIFO Control                 */
    uint32_t MM:8;             /*!< bit: 24..31  Message Marker                     */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} CAN_TXBE_1_Type;

typedef union {
  struct {
    uint32_t EFID1:29;         /*!< bit:  0..28  Extended Filter ID 1               */
    uint32_t EFEC:3;           /*!< bit: 29..31  Extended Filter Element Configuration */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} CAN_XIDFE_0_Type;

typedef union {
  struct {
    uint32_t EFID2:29;         /*!< bit:  0..28  Extended Filter ID 2               */
    uint32_t :1;               /*!< bit:     29  Reserved                           */
    uint32_t EFT:2;            /*!< bit: 30..31  Extended Filter Type               */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} CAN_XIDFE_1_Type;

struct drv_can_rx_buffer_element {
	__IO CAN_RXBE_0_Type           RXBE_0;      /**< \brief Offset: 0x00 (R/W 32) Rx Buffer Element 0 */
	__IO CAN_RXBE_1_Type           RXBE_1;      /**< \brief Offset: 0x04 (R/W 32) Rx Buffer Element 1 */
	__IO uint8_t DB[8 + 4 * CAN_RX_BUFFERS_DATA_SIZE];
};

struct drv_can_rx_fifo_0_element {
	__IO CAN_RXF0E_0_Type          RXF0E_0;     /**< \brief Offset: 0x00 (R/W 32) Rx FIFO 0 Element 0 */
	__IO CAN_RXF0E_1_Type          RXF0E_1;     /**< \brief Offset: 0x04 (R/W 32) Rx FIFO 0 Element 1 */
	__IO uint8_t DB[8 + 4 * CAN_RX_FIFO_0_DATA_SIZE];
};

struct drv_can_rx_fifo_1_element {
	__IO CAN_RXF1E_0_Type          RXF1E_0;     /**< \brief Offset: 0x00 (R/W 32) Rx FIFO 1 Element 0 */
	__IO CAN_RXF1E_1_Type          RXF1E_1;     /**< \brief Offset: 0x04 (R/W 32) Rx FIFO 1 Element 1 */
	__IO uint8_t DB[8 + 4 * CAN_RX_FIFO_1_DATA_SIZE];
};

struct drv_can_tx_buffer_element {
	__IO CAN_TXBE_0_Type           TXBE_0;      /**< \brief Offset: 0x00 (R/W 32) Tx Buffer Element 0 */
	__IO CAN_TXBE_1_Type           TXBE_1;      /**< \brief Offset: 0x04 (R/W 32) Tx Buffer Element 1 */
	__IO uint8_t DB[8 + 4 * CAN_TX_DATA_SIZE];
};

struct drv_can_tx_buffer_config {
	__IO CAN_TXBE_0_Type           TXBE_0;      /**< \brief Offset: 0x00 (R/W 32) Tx Buffer Element 0 */
	__IO CAN_TXBE_1_Type           TXBE_1;      /**< \brief Offset: 0x04 (R/W 32) Tx Buffer Element 1 */
};

struct drv_can_standard_filter {
	__IO CAN_SIDFE_0_Type          SIDFE_0;     /**< \brief Offset: 0x00 (R/W 32) Standard Message ID Filter Element */
};

struct drv_can_extended_filter {
	__IO CAN_XIDFE_0_Type          XIDFE_0;     /**< \brief Offset: 0x00 (R/W 32) Extended Message ID Filter Element 0 */
	__IO CAN_XIDFE_1_Type          XIDFE_1;     /**< \brief Offset: 0x04 (R/W 32) Extended Message ID Filter Element 1 */
};

struct drv_can_config {
	#if ENABLE_CAN0
	const struct drv_can_standard_filter * standard_filters_can0;
	const struct drv_can_extended_filter * extended_filters_can0;
	#endif
	#if ENABLE_CAN1
	const struct drv_can_standard_filter * standard_filters_can1;
	const struct drv_can_extended_filter * extended_filters_can1;
	#endif
	const struct drv_can_tx_buffer_config * transmit_config;
};
extern const struct drv_can_config drv_can_config;

void drv_can_init(void);
struct drv_can_rx_buffer_element * drv_can_get_rx_buffer(int id);
struct drv_can_tx_buffer_element * drv_can_get_tx_buffer(int id);
void drv_can_queue_tx_buffer(can_registers_t * bus, int id);
bool drv_can_check_rx_buffer(can_registers_t * bus, int id);
void drv_can_clear_rx_buffer(can_registers_t * bus, int id);

#define DRV_CAN_XTD_FILTER(BUS, MSG) { \
	.XIDFE_0 = { \
		.bit = { \
			.EFEC = CAN_XIDFE_0_EFEC_STRXBUF_Val, \
			.EFID1 = ID_ ## MSG, \
		} \
	}, \
	.XIDFE_1 = { \
		.bit = { \
			.EFID2 = (0 << 9) | (0 << 6) | (DRV_CAN_RX_BUFFER_ ## BUS ## _ ## MSG) \
		} \
	} \
}

#define DRV_CAN_TX_BUFFER(BUS, MSG) 	[DRV_CAN_TX_BUFFER_ ## BUS ## _ ## MSG] = { \
	.TXBE_0 = { \
		.bit = { \
			.ID = ID_ ## MSG, \
			.RTR = 0, \
			.XTD = EXT_ ## MSG, \
		} \
	}, \
	.TXBE_1 = { \
		.bit = { \
			.DLC = DLC_ ## MSG, \
			.EFC = 0, \
		} \
	} \
}

