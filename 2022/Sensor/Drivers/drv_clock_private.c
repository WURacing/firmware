#include "drv_clock.h"

const struct drv_clock_config drv_clock_config = {
	.channelConfig = {
		[DRV_CLOCK_CHANNEL_MAIN_CPU] = {
			.generator_id = 0,
			.genctrl = 0,
			.source = 6, // input from OSC48M
			.division_mode = 0,
			.divisor = 0, // output OSC48M / 1 = 48MHz
		},
	},
};
