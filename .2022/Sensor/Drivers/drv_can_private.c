#include "drv_can.h"

// Filters for standard CAN frames
static const struct drv_can_standard_filter standardFilters[CAN0_STANDARD_FILTERS_NUM];

// Filters for extended CAN frames
static const struct drv_can_extended_filter extendedFilters[CAN0_EXTENDED_FILTERS_NUM];

// Definitions for each transmit buffer
static const struct drv_can_tx_buffer_config transmitConfig[DRV_CAN_TX_BUFFER_COUNT] = {
#if BOARD_USAGE == SBFront1
	DRV_CAN_TX_BUFFER(VEHICLE, SB_FRONT1_SIGNALS1),
	DRV_CAN_TX_BUFFER(VEHICLE, SB_FRONT1_SIGNALS2),
#elif BOARD_USAGE == SBFront2
	DRV_CAN_TX_BUFFER(VEHICLE, SB_FRONT2_SIGNALS1),
	DRV_CAN_TX_BUFFER(VEHICLE, SB_FRONT2_SIGNALS2),
#elif BOARD_USAGE == SBRear1
	DRV_CAN_TX_BUFFER(VEHICLE, SB_REAR1_SIGNALS1),
	DRV_CAN_TX_BUFFER(VEHICLE, SB_REAR1_SIGNALS2),
#elif BOARD_USAGE == SBRear2
	DRV_CAN_TX_BUFFER(VEHICLE, SB_REAR2_SIGNALS1),
	DRV_CAN_TX_BUFFER(VEHICLE, SB_REAR2_SIGNALS2),
#elif BOARD_USAGE == SBCG
	DRV_CAN_TX_BUFFER(VEHICLE, SBCG_SIGNALS1),
	DRV_CAN_TX_BUFFER(VEHICLE, SBCG_SIGNALS2),
#else
#error "Unknown BOARD_USAGE"
#endif
};

const struct drv_can_config drv_can_config = {
	.standard_filters_can0 = standardFilters,
	.extended_filters_can0 = extendedFilters,
	.transmit_config = transmitConfig,
};
