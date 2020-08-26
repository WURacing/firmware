#include "drv_can.h"
#include "2020.1.0.h"

// Filters for standard CAN frames
static const struct drv_can_standard_filter standardFilters[CAN0_STANDARD_FILTERS_NUM];

// Filters for extended CAN frames
static const struct drv_can_extended_filter extendedFilters[CAN0_EXTENDED_FILTERS_NUM] = {
	DRV_CAN_XTD_FILTER(PE1),
	DRV_CAN_XTD_FILTER(PE2),
	DRV_CAN_XTD_FILTER(PE3),
	DRV_CAN_XTD_FILTER(PE4),
	DRV_CAN_XTD_FILTER(PE5),
	DRV_CAN_XTD_FILTER(PE6),
	DRV_CAN_XTD_FILTER(PE7),
	DRV_CAN_XTD_FILTER(PE8),
	DRV_CAN_XTD_FILTER(PE9),
	DRV_CAN_XTD_FILTER(PE10),
	DRV_CAN_XTD_FILTER(PE11),
	DRV_CAN_XTD_FILTER(PE12),
	DRV_CAN_XTD_FILTER(PE13),
	DRV_CAN_XTD_FILTER(PE14),
	DRV_CAN_XTD_FILTER(PE15),
	DRV_CAN_XTD_FILTER(PE16),
	DRV_CAN_XTD_FILTER(CG_Sensor),
	DRV_CAN_XTD_FILTER(Dash_Stat),
	DRV_CAN_XTD_FILTER(FL_Upright),
	DRV_CAN_XTD_FILTER(FR_Upright),
	DRV_CAN_XTD_FILTER(RL_Upright),
	DRV_CAN_XTD_FILTER(RR_Upright),
	DRV_CAN_XTD_FILTER(Front_Sensor),
	DRV_CAN_XTD_FILTER(Rear_Sensor),
	DRV_CAN_XTD_FILTER(GPS_POS),
	DRV_CAN_XTD_FILTER(PDM_Current_A),
	DRV_CAN_XTD_FILTER(PDM_Current_B),
	DRV_CAN_XTD_FILTER(PDM_PWR),
};

// Definitions for each transmit buffer
static const struct drv_can_tx_buffer_config transmitConfig[CAN0_TX_BUFFERS_NUM] = {
	DRV_CAN_TX_BUFFER(GPS_time),
	DRV_CAN_TX_BUFFER(GPS_POS),
	DRV_CAN_TX_BUFFER(Telem_Stat),
};

const struct drv_can_config drv_can_config = {
	.standard_filters = standardFilters,
	.extended_filters = extendedFilters,
	.transmit_config = transmitConfig,
};