/*
 * drv_gpio_private.c
 *
 * Created: 7/11/2020 6:42:47 PM
 *  Author: connor
 */ 

#include "drv_gpio.h"
#include "sam.h"

static const struct drv_gpio_pinConfig pinConfigs[DRV_GPIO_PIN_COUNT] = {
	[DRV_GPIO_PIN_LED] =
	{
		.pin = PIN_PA28,
		.input = false,
	}
};

const struct drv_gpio_config drv_gpio_config = {
	.pinConfig = pinConfigs,
	.numPins = DRV_GPIO_PIN_COUNT
};