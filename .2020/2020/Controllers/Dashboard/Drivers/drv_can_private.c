#include "drv_can.h"
#include "2020.1.0.h"

// Filters for standard CAN frames
static const struct drv_can_standard_filter standardFilters[CAN0_STANDARD_FILTERS_NUM];

// Filters for extended CAN frames
static const struct drv_can_extended_filter extendedFilters[CAN0_EXTENDED_FILTERS_NUM] = {
	DRV_CAN_XTD_FILTER(CAN0, PE6),
};

const struct drv_can_config drv_can_config = {
	.standard_filters_can0 = standardFilters,
	.extended_filters_can0 = extendedFilters,
};