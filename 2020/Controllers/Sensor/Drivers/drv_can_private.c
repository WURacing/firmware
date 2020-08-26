#include "drv_can.h"
#include "2020.1.0.h"
#include "config.h"

// Filters for standard CAN frames
static const struct drv_can_standard_filter standardFilters[CAN0_STANDARD_FILTERS_NUM];

// Filters for extended CAN frames
static const struct drv_can_extended_filter extendedFilters[CAN0_EXTENDED_FILTERS_NUM];

// Definitions for each transmit buffer
static const struct drv_can_tx_buffer_config transmitConfig[CAN0_TX_BUFFERS_NUM] = {
	#if (PCBA_ID == PCBA_ID_SENSOR_BOARD_FRONT)
	DRV_CAN_TX_BUFFER(Front_Sensor),
	#elif (PCBA_ID == PCBA_ID_SENSOR_BOARD_REAR)
	DRV_CAN_TX_BUFFER(Rear_Sensor),
	#elif (PCBA_ID == PCBA_ID_SENSOR_BOARD_CG)
	DRV_CAN_TX_BUFFER(CG_Sensor),
	#elif (PCBA_ID == PCBA_ID_SENSOR_BOARD_FL_UPRIGHT)
	DRV_CAN_TX_BUFFER(FL_Upright),
	#elif (PCBA_ID == PCBA_ID_SENSOR_BOARD_FR_UPRIGHT)
	DRV_CAN_TX_BUFFER(FR_Upright),
	#elif (PCBA_ID == PCBA_ID_SENSOR_BOARD_RL_UPRIGHT)
	DRV_CAN_TX_BUFFER(RL_Upright),
	#elif (PCBA_ID == PCBA_ID_SENSOR_BOARD_RR_UPRIGHT)
	DRV_CAN_TX_BUFFER(RR_Upright),
	#endif
};

const struct drv_can_config drv_can_config = {
	.standard_filters = standardFilters,
	.extended_filters = extendedFilters,
	.transmit_config = transmitConfig,
};