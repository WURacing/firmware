#pragma once

#define CAN0_STANDARD_FILTERS_NUM 0
#define CAN0_EXTENDED_FILTERS_NUM 0

#define CAN0_RX_FIFO_0_NUM 0 /* receive FIFO, used for messages not matched by a filter into a buffer */
#define CAN0_RX_FIFO_0_OPERATION_MODE 1 /* 1: overwrite mode */
#define CAN0_RX_FIFO_0_HIGH_WATER_INT_LEVEL 0 /* 0: interrupt disabled */
#define CAN0_RX_FIFO_0_DATA_SIZE CAN_RXESC_F0DS_DATA8_Val /* >8 if CAN_FD */

#define CAN0_RX_FIFO_1_NUM 0
#define CAN0_RX_FIFO_1_OPERATION_MODE 1 /* 1: overwrite mode */
#define CAN0_RX_FIFO_1_HIGH_WATER_INT_LEVEL 0 /* 0: interrupt disabled */
#define CAN0_RX_FIFO_1_DATA_SIZE CAN_RXESC_F1DS_DATA8_Val /* >8 if CAN_FD */

#define CAN0_RX_BUFFERS_NUM 0 /* dedicated receive buffers. should match DRV_CAN_RX_BUFFER_COUNT */
#define CAN0_RX_BUFFERS_DATA_SIZE CAN_RXESC_F1DS_DATA8_Val /* >8 if CAN_FD */

#define CAN0_TX_EVENT_FIFO_NUM 0
#define CAN0_TX_EVENT_FIFO_HIGH_WATER_INT_LEVEL 0 /* 0: interrupt disabled */

#define CAN0_TX_BUFFERS_NUM 2 /* dedicated transmit buffers. should match DRV_CAN_TX_BUFFER_COUNT */
#define CAN0_TX_FIFO_NUM 0  /* tx queue size, for non-dedicated messages */
#define CAN0_TX_DATA_SIZE CAN_TXESC_TBDS_DATA8_Val

enum drv_can_rx_buffer_table {	
	DRV_CAN_RX_BUFFER_COUNT
};

enum drv_can_tx_buffer_table {
	#if (PCBA_ID == PCBA_ID_SENSOR_BOARD_FRONT)
	DRV_CAN_TX_BUFFER_Front_Sensor,
	#elif (PCBA_ID == PCBA_ID_SENSOR_BOARD_REAR)
	DRV_CAN_TX_BUFFER_Rear_Sensor,
	#elif (PCBA_ID == PCBA_ID_SENSOR_BOARD_CG)
	DRV_CAN_TX_BUFFER_CG_Sensor,
	#elif (PCBA_ID == PCBA_ID_SENSOR_BOARD_FL_UPRIGHT)
	DRV_CAN_TX_BUFFER_FL_Upright,
	#elif (PCBA_ID == PCBA_ID_SENSOR_BOARD_FR_UPRIGHT)
	DRV_CAN_TX_BUFFER_FR_Upright,
	#elif (PCBA_ID == PCBA_ID_SENSOR_BOARD_RL_UPRIGHT)
	DRV_CAN_TX_BUFFER_RL_Upright,
	#elif (PCBA_ID == PCBA_ID_SENSOR_BOARD_RR_UPRIGHT)
	DRV_CAN_TX_BUFFER_RR_Upright,
	#endif
	
	DRV_CAN_TX_BUFFER_COUNT
};