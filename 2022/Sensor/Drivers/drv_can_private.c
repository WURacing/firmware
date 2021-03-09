#include "drv_can.h"

// Filters for standard CAN frames
static const struct drv_can_standard_filter standardFilters[CAN0_STANDARD_FILTERS_NUM];

// Filters for extended CAN frames
static const struct drv_can_extended_filter extendedFilters[CAN0_EXTENDED_FILTERS_NUM];

// Definitions for each transmit buffer
static const struct drv_can_tx_buffer_config transmitConfig[DRV_CAN_TX_BUFFER_COUNT] = {
	DRV_CAN_TX_BUFFER(CAN0, signals1),
	DRV_CAN_TX_BUFFER(CAN0, signals2),
};

const struct drv_can_config drv_can_config = {
	.standard_filters_can0 = standardFilters,
	.extended_filters_can0 = extendedFilters,
	.transmit_config = transmitConfig,
};
