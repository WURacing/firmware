#include "drv_tcc.h"

const struct drv_tcc_config drv_tcc_config = {
	.channelConfig = {
		[DRV_TCC_CHANNEL_ARB] = {
			.tccid = 0,
			.module = TCC0_REGS,
			.gclk = 4,
			.pinmux = {
				PINMUX_PA04E_TCC0_WO0, PINMUX_PA05E_TCC0_WO1, 0, 0, 0, 0, 0, 0
			},
			.per = 20000, // 50Hz from 1MHz GCLK4
			.cc = {
				SERVO_SAFETY, SERVO_SAFETY, 0, 0
			}
		},
		[DRV_TCC_CHANNEL_DRS] = {
			.tccid = 1,
			.module = TCC1_REGS,
			.gclk = 4,
			.pinmux = {
				PINMUX_PA06E_TCC1_WO0, 0, 0, 0, 0, 0, 0, 0
			},
			.per = 20000,
			.cc = {
				SERVO_SAFETY, 0, 0, 0
			}
		},
	}
};
