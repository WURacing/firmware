#pragma once

#include "sam.h"
#include "config.h"
#include "vehicle.h"

#define ENABLE_CAN0 1
#define ENABLE_CAN1 0

#define CAN0_TX_PIN PIN_PA24
#define CAN0_TX_MUX MUX_PA24G_CAN0_TX
#define CAN0_RX_PIN PIN_PA25
#define CAN0_RX_MUX MUX_PA25G_CAN0_RX

#define CAN0_RX_FIFO_0_NUM 0 /* receive FIFO, used for messages not matched by a filter into a buffer */
#define CAN0_RX_FIFO_0_OPERATION_MODE 1 /* 1: overwrite mode */
#define CAN0_RX_FIFO_0_HIGH_WATER_INT_LEVEL 0 /* 0: interrupt disabled */
#define CAN_RX_FIFO_0_DATA_SIZE CAN_RXESC_F0DS_DATA8_Val /* >8 if CAN_FD */

#define CAN0_RX_FIFO_1_NUM 0
#define CAN0_RX_FIFO_1_OPERATION_MODE 1 /* 1: overwrite mode */
#define CAN0_RX_FIFO_1_HIGH_WATER_INT_LEVEL 0 /* 0: interrupt disabled */
#define CAN_RX_FIFO_1_DATA_SIZE CAN_RXESC_F1DS_DATA8_Val /* >8 if CAN_FD */

#define CAN_RX_BUFFERS_DATA_SIZE CAN_RXESC_F1DS_DATA8_Val /* >8 if CAN_FD */
#define CAN_TX_DATA_SIZE CAN_TXESC_TBDS_DATA8_Val

enum drv_can_rx_buffer_table {		
	DRV_CAN_RX_BUFFER_VEHICLE_UI_INPUTS,
	
	DRV_CAN_RX_BUFFER_COUNT
};

enum drv_can_tx_buffer_table {
	DRV_CAN_TX_BUFFER_COUNT
};

#define CAN0_STANDARD_FILTERS_NUM DRV_CAN_RX_BUFFER_COUNT
#define CAN0_EXTENDED_FILTERS_NUM DRV_CAN_RX_BUFFER_COUNT
