#pragma once

#include "drv_can_private.h"
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include "sam.h"

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
void drv_can_queue_tx_buffer(Can * bus, int id);
bool drv_can_check_rx_buffer(Can * bus, int id);
void drv_can_clear_rx_buffer(Can * bus, int id);

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

