#include "drv_clock.h"

const struct drv_clock_config drv_clock_config = {
	.channelConfig = {
		[DRV_CLOCK_CHANNEL_MAIN_CPU] = {
			.generator_id = 0,
			.genctrl = 0,
			.source = 6,
			.division_mode = 0,
			.divisor = 0, // undivided
		},
		[DRV_CLOCK_CHANNEL_SERVO] = {
			.generator_id = 4,
			.genctrl = 0,
			.source = 6,
			.division_mode = 0,
			.divisor = 48, // run at 1MHz
		},
	},
};
