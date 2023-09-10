#pragma once

#include "drv_gpio_private.h"
#include <stdbool.h>

struct drv_gpio_pinConfig {
	int pin;
	bool input;
};

struct drv_gpio_config {
	const struct drv_gpio_pinConfig * pinConfig;
	int numPins;
};

extern const struct drv_gpio_config drv_gpio_config;

void drv_gpio_init(void);
void drv_gpio_setOutput(int pin_id, bool value);
void drv_gpio_toggle(int pin_id);