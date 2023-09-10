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

// No CAN messages to receive for now
enum drv_can_rx_buffer_table {		
	DRV_CAN_RX_BUFFER_COUNT
};

// Send two messages with sensor signal information
enum drv_can_tx_buffer_table {
#if BOARD_USAGE == SBFront1
	DRV_CAN_TX_BUFFER_VEHICLE_SB_FRONT1_SIGNALS1,
	DRV_CAN_TX_BUFFER_VEHICLE_SB_FRONT1_SIGNALS2,
#elif BOARD_USAGE == SBFront2
	DRV_CAN_TX_BUFFER_VEHICLE_SB_FRONT2_SIGNALS1,
	DRV_CAN_TX_BUFFER_VEHICLE_SB_FRONT2_SIGNALS2,
#elif BOARD_USAGE == SBRear1
	DRV_CAN_TX_BUFFER_VEHICLE_SB_REAR1_SIGNALS1,
	DRV_CAN_TX_BUFFER_VEHICLE_SB_REAR1_SIGNALS2,
#elif BOARD_USAGE == SBRear2
	DRV_CAN_TX_BUFFER_VEHICLE_SB_REAR2_SIGNALS1,
	DRV_CAN_TX_BUFFER_VEHICLE_SB_REAR2_SIGNALS2,
#elif BOARD_USAGE == SBCG
	DRV_CAN_TX_BUFFER_VEHICLE_SBCG_SIGNALS1,
	DRV_CAN_TX_BUFFER_VEHICLE_SBCG_SIGNALS2,
#else
#error "Unknown BOARD_USAGE"
#endif
	
	DRV_CAN_TX_BUFFER_COUNT
};

#define CAN0_STANDARD_FILTERS_NUM 0
#define CAN0_EXTENDED_FILTERS_NUM DRV_CAN_RX_BUFFER_COUNT

struct signals1_layout {
	uint16_t analog1, analog2, analog3, analog4;
};
struct signals2_layout {
	uint16_t analog5, analog6;
	uint8_t fault1:1, fault2:1, fault3:1, fault4:1, allFaults:1;
	uint8_t :3;
	uint16_t totalCurrent;
};