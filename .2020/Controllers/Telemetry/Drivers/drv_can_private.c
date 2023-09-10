#include "drv_can.h"
#include "2020.1.0.h"

// Filters for standard CAN frames
static const struct drv_can_standard_filter standardFilters[CAN0_STANDARD_FILTERS_NUM];

// Filters for extended CAN frames
static const struct drv_can_extended_filter extendedFilters[CAN0_EXTENDED_FILTERS_NUM] = {
	DRV_CAN_XTD_FILTER(CAN0, PE1),
	DRV_CAN_XTD_FILTER(CAN0, PE2),
	DRV_CAN_XTD_FILTER(CAN0, PE3),
	DRV_CAN_XTD_FILTER(CAN0, PE4),
	DRV_CAN_XTD_FILTER(CAN0, PE5),
	DRV_CAN_XTD_FILTER(CAN0, PE6),
	DRV_CAN_XTD_FILTER(CAN0, PE7),
	DRV_CAN_XTD_FILTER(CAN0, PE8),
	DRV_CAN_XTD_FILTER(CAN0, PE9),
	DRV_CAN_XTD_FILTER(CAN0, PE10),
	DRV_CAN_XTD_FILTER(CAN0, PE11),
	DRV_CAN_XTD_FILTER(CAN0, PE12),
	DRV_CAN_XTD_FILTER(CAN0, PE13),
	DRV_CAN_XTD_FILTER(CAN0, PE14),
	DRV_CAN_XTD_FILTER(CAN0, PE15),
	DRV_CAN_XTD_FILTER(CAN0, PE16),
	DRV_CAN_XTD_FILTER(CAN0, CG_Sensor),
	DRV_CAN_XTD_FILTER(CAN0, Dash_Stat),
	DRV_CAN_XTD_FILTER(CAN0, FL_Upright),
	DRV_CAN_XTD_FILTER(CAN0, FR_Upright),
	DRV_CAN_XTD_FILTER(CAN0, RL_Upright),
	DRV_CAN_XTD_FILTER(CAN0, RR_Upright),
	DRV_CAN_XTD_FILTER(CAN0, Front_Sensor),
	DRV_CAN_XTD_FILTER(CAN0, Rear_Sensor),
	DRV_CAN_XTD_FILTER(CAN0, GPS_POS),
	DRV_CAN_XTD_FILTER(CAN0, PDM_Current_A),
	DRV_CAN_XTD_FILTER(CAN0, PDM_Current_B),
	DRV_CAN_XTD_FILTER(CAN0, PDM_PWR),
};

// Definitions for each transmit buffer
static const struct drv_can_tx_buffer_config transmitConfig[DRV_CAN_TX_BUFFER_COUNT] = {
	DRV_CAN_TX_BUFFER(CAN0, GPS_time),
	DRV_CAN_TX_BUFFER(CAN0, GPS_POS),
	DRV_CAN_TX_BUFFER(CAN0, Telem_Stat),
};

const struct drv_can_config drv_can_config = {
	.standard_filters_can0 = standardFilters,
	.extended_filters_can0 = extendedFilters,
	.transmit_config = transmitConfig,
};