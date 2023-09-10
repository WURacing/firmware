#pragma once
#include "sam.h"
#include "drv_clock_private.h"
#include <stdint.h>

struct drv_clock_channelConfig {
	__IO  uint32_t * genctrl;
	uint32_t reg;
};

struct drv_clock_config {
	const struct drv_clock_channelConfig channelConfig[DRV_CLOCK_CHANNEL_COUNT];
};

extern const struct drv_clock_config drv_clock_config;

void drv_clock_init(void);
