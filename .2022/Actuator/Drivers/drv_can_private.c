#include "drv_can.h"

// Filters for standard CAN frames
static const struct drv_can_standard_filter standardFilters[CAN0_STANDARD_FILTERS_NUM] = {
	DRV_CAN_STD_FILTER(VEHICLE, UI_INPUTS),
};

// Filters for extended CAN frames
static const struct drv_can_extended_filter extendedFilters[CAN0_EXTENDED_FILTERS_NUM] = {
	DRV_CAN_XTD_FILTER(VEHICLE, UI_INPUTS),
};

// Definitions for each transmit buffer
static const struct drv_can_tx_buffer_config transmitConfig[DRV_CAN_TX_BUFFER_COUNT];

const struct drv_can_config drv_can_config = {
	.standard_filters_can0 = standardFilters,
	.extended_filters_can0 = extendedFilters,
	.transmit_config = transmitConfig,
};
