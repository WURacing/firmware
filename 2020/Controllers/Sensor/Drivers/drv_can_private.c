#include "drv_can.h"

// Filters for standard CAN frames
static const struct drv_can_standard_filter standardFilters[CAN0_STANDARD_FILTERS_NUM];

// Filters for extended CAN frames
static const struct drv_can_extended_filter extendedFilters[CAN0_EXTENDED_FILTERS_NUM] = {
	{ // PE1 message filter
		.XIDFE_0 = {
			.bit = {
				.EFEC = CAN_XIDFE_0_EFEC_STRXBUF_Val, // store in dedicated RX buffer
				.EFID1 = 0x0CFFF048, // match this ID exactly
			}
		},
		.XIDFE_1 = {
			.bit = {
				.EFID2 = (0 << 9) | (0 << 6) | (DRV_CAN_RX_BUFFER_PE1) // store in buffer | don't set event pins | store at buffer PE1
			}
		}
	},
};

// Definitions for each transmit buffer
static const struct drv_can_tx_buffer_config transmitConfig[CAN0_TX_BUFFERS_NUM] = {
	[DRV_CAN_TX_BUFFER_ERROR] = {
		.TXBE_0 = {
			.bit = {
				.ID = 0x6969, // transmit this ID
				.RTR = 0, // not a remote request frame
				.XTD = 1, // this is an extended CAN frame
			}
		},
		.TXBE_1 = {
			.bit = {
				.DLC = 1, // send one byte
				.EFC = 0, // don't record events
			}
		}
	}
};

const struct drv_can_config drv_can_config = {
	.standard_filters = standardFilters,
	.extended_filters = extendedFilters,
	.transmit_config = transmitConfig,
};