#include "drv_clock.h"

const struct drv_clock_config drv_clock_config = {
	.channelConfig = {
		[DRV_CLOCK_CHANNEL_MAIN_CPU] = {
			.genctrl = &GCLK_REGS->GCLK_GENCTRL[0],
			.reg = GCLK_GENCTRL_GENEN(1) | GCLK_GENCTRL_DIVSEL_DIV1 | GCLK_GENCTRL_DIV(0) | GCLK_GENCTRL_SRC_OSC48M,
		},
	},
};
