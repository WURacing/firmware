#include "drv_can.h"

// Filters for standard CAN frames
static const struct drv_can_standard_filter standardFilters[CAN0_STANDARD_FILTERS_NUM];

// Filters for extended CAN frames
static const struct drv_can_extended_filter extendedFilters[CAN0_EXTENDED_FILTERS_NUM];

// Definitions for each transmit buffer
static const struct drv_can_tx_buffer_config transmitConfig[CAN0_TX_BUFFERS_NUM] = {
	[DRV_CAN_TX_BUFFER_SENS_1] = {
		.TXBE_0 = {
			.bit = {
				.ID = 0x6942001, // transmit this ID
				.RTR = 0, // not a remote request frame
				.XTD = 1, // this is an extended CAN frame
			}
		},
		.TXBE_1 = {
			.bit = {
				.DLC = 8,
				.EFC = 0, // don't record events
			}
		}
	},
	[DRV_CAN_TX_BUFFER_SENS_2] = {
		.TXBE_0 = {
			.bit = {
				.ID = 0x6942002, // transmit this ID
				.RTR = 0, // not a remote request frame
				.XTD = 1, // this is an extended CAN frame
			}
		},
		.TXBE_1 = {
			.bit = {
				.DLC = 4,
				.EFC = 0, // don't record events
			}
		}
	},
};

const struct drv_can_config drv_can_config = {
	.standard_filters = standardFilters,
	.extended_filters = extendedFilters,
	.transmit_config = transmitConfig,
};