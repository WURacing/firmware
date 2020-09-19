#pragma once

#include "sam.h"

#define ENABLE_CAN0 0
#define ENABLE_CAN1 1
#define CAN1_BUS DAQ1
#define DAQ1_CAN CAN1
#define CAN1_TX_PIN PIN_PB10
#define CAN1_TX_MUX MUX_PB10G_CAN1_TX
#define CAN1_RX_PIN PIN_PB11
#define CAN1_RX_MUX MUX_PB11G_CAN1_RX

#define CAN1_STANDARD_FILTERS_NUM 0
#define CAN1_EXTENDED_FILTERS_NUM 0

#define CAN1_RX_FIFO_0_NUM 0 /* receive FIFO, used for messages not matched by a filter into a buffer */
#define CAN1_RX_FIFO_0_OPERATION_MODE 1 /* 1: overwrite mode */
#define CAN1_RX_FIFO_0_HIGH_WATER_INT_LEVEL 0 /* 0: interrupt disabled */
#define CAN_RX_FIFO_0_DATA_SIZE CAN_RXESC_F0DS_DATA8_Val /* >8 if CAN_FD */

#define CAN1_RX_FIFO_1_NUM 0
#define CAN1_RX_FIFO_1_OPERATION_MODE 1 /* 1: overwrite mode */
#define CAN1_RX_FIFO_1_HIGH_WATER_INT_LEVEL 0 /* 0: interrupt disabled */
#define CAN_RX_FIFO_1_DATA_SIZE CAN_RXESC_F1DS_DATA8_Val /* >8 if CAN_FD */

#define CAN_RX_BUFFERS_DATA_SIZE CAN_RXESC_F1DS_DATA8_Val /* >8 if CAN_FD */

#define CAN_TX_DATA_SIZE CAN_TXESC_TBDS_DATA8_Val

enum drv_can_rx_buffer_table {	
	DRV_CAN_RX_BUFFER_COUNT
};

enum drv_can_tx_buffer_table {
	#if (PCBA_ID == PCBA_ID_SENSOR_BOARD_FRONT)
	DRV_CAN_TX_BUFFER_CAN1_Front_Sensor,
	#elif (PCBA_ID == PCBA_ID_SENSOR_BOARD_REAR)
	DRV_CAN_TX_BUFFER_CAN1_Rear_Sensor,
	#elif (PCBA_ID == PCBA_ID_SENSOR_BOARD_CG)
	DRV_CAN_TX_BUFFER_CAN1_CG_Sensor,
	#elif (PCBA_ID == PCBA_ID_SENSOR_BOARD_FL_UPRIGHT)
	DRV_CAN_TX_BUFFER_CAN1_FL_Upright,
	#elif (PCBA_ID == PCBA_ID_SENSOR_BOARD_FR_UPRIGHT)
	DRV_CAN_TX_BUFFER_CAN1_FR_Upright,
	#elif (PCBA_ID == PCBA_ID_SENSOR_BOARD_RL_UPRIGHT)
	DRV_CAN_TX_BUFFER_CAN1_RL_Upright,
	#elif (PCBA_ID == PCBA_ID_SENSOR_BOARD_RR_UPRIGHT)
	DRV_CAN_TX_BUFFER_CAN1_RR_Upright,
	#endif
	
	DRV_CAN_TX_BUFFER_COUNT
};