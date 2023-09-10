#include "drv_can.h"
#include "2020.1.0.h"
#include "config.h"

// Filters for standard CAN frames
static const struct drv_can_standard_filter standardFilters[CAN1_STANDARD_FILTERS_NUM];

// Filters for extended CAN frames
static const struct drv_can_extended_filter extendedFilters[CAN1_EXTENDED_FILTERS_NUM];

// Definitions for each transmit buffer
static const struct drv_can_tx_buffer_config transmitConfig[DRV_CAN_TX_BUFFER_COUNT] = {
	#if (PCBA_ID == PCBA_ID_SENSOR_BOARD_FRONT)
	DRV_CAN_TX_BUFFER(CAN1, Front_Sensor),
	#elif (PCBA_ID == PCBA_ID_SENSOR_BOARD_REAR)
	DRV_CAN_TX_BUFFER(CAN1, Rear_Sensor),
	#elif (PCBA_ID == PCBA_ID_SENSOR_BOARD_CG)
	DRV_CAN_TX_BUFFER(CAN1, CG_Sensor),
	#elif (PCBA_ID == PCBA_ID_SENSOR_BOARD_FL_UPRIGHT)
	DRV_CAN_TX_BUFFER(CAN0, FL_Upright),
	#elif (PCBA_ID == PCBA_ID_SENSOR_BOARD_FR_UPRIGHT)
	DRV_CAN_TX_BUFFER(CAN0, FR_Upright),
	#elif (PCBA_ID == PCBA_ID_SENSOR_BOARD_RL_UPRIGHT)
	DRV_CAN_TX_BUFFER(CAN0, RL_Upright),
	#elif (PCBA_ID == PCBA_ID_SENSOR_BOARD_RR_UPRIGHT)
	DRV_CAN_TX_BUFFER(CAN0, RR_Upright),
	#endif
};

const struct drv_can_config drv_can_config = {
#if UPRIGHT
	.standard_filters_can0 = standardFilters,
	.extended_filters_can0 = extendedFilters,
#elif GENERAL_PURPOSE
	.standard_filters_can1 = standardFilters,
	.extended_filters_can1 = extendedFilters,
#endif
	.transmit_config = transmitConfig,
};