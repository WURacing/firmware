#include <pio/samc21j18a.h>

#include "drv_i2c.h"

static const struct drv_i2c_channelConfig channels[DRV_I2C_CHANNEL_COUNT] =
{
	[DRV_I2C_CHANNEL_RTC] = {
		.sercom_id = 1,
		.module = &SERCOM1_REGS->I2CM,
		.sda_pinmux = PINMUX_PA16C_SERCOM1_PAD0,
		.scl_pinmux = PINMUX_PA17C_SERCOM1_PAD1,
		.baud = I2C_BAUD(100000),
	},
	[DRV_I2C_CHANNEL_LED] = {
		.sercom_id = 3,
		.module = &SERCOM3_REGS->I2CM,
		.sda_pinmux = PINMUX_PA22C_SERCOM3_PAD0,
		.scl_pinmux = PINMUX_PA23C_SERCOM3_PAD1,
		.baud = I2C_BAUD(100000),
	},
};

const struct drv_i2c_config drv_i2c_config = {
	.channelConfig = channels,
};

