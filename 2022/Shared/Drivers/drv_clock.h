#pragma once
#include "drv_clock_private.h"
#include <stdint.h>

struct drv_clock_channelConfig {
	int generator_id;
	uint32_t * genctrl;
	int source;
	int division_mode;
	int divisor;
};

struct drv_clock_config {
	const struct drv_clock_channelConfig channelConfig[DRV_CLOCK_CHANNEL_COUNT];
};

extern const struct drv_clock_config drv_clock_config;

void drv_clock_init(void);
