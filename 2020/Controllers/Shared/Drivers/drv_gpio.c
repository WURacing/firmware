#include "drv_gpio.h"
#include "sam.h"

void drv_gpio_init(void)
{
	for (int pin_id = 0; pin_id < DRV_GPIO_PIN_COUNT; ++pin_id)
	{
		const struct drv_gpio_pinConfig * pinConfig = &drv_gpio_config.pinConfig[pin_id];
		
		int group = pinConfig->pin / 32; // the compiler will optimize these to shifts unless pigs fly
		int output = pinConfig->pin % 32;
		
		PORT->Group[group].PINCFG[output].reg = 0;
		if (pinConfig->input)
			PORT->Group[group].DIRCLR.reg = (1 << output);
		else
			PORT->Group[group].DIRSET.reg = (1 << output);
	}
}

void drv_gpio_setOutput(int pin_id, bool value)
{
	const struct drv_gpio_pinConfig * pinConfig = &drv_gpio_config.pinConfig[pin_id];
	
	int group = pinConfig->pin / 32;
	int port = pinConfig->pin % 32;
	
	if (value)
		PORT->Group[group].OUTSET.reg = (1 << port);
	else
		PORT->Group[group].OUTCLR.reg = (1 << port);
}

void drv_gpio_toggle(int pin_id)
{
	const struct drv_gpio_pinConfig * pinConfig = &drv_gpio_config.pinConfig[pin_id];
	
	int group = pinConfig->pin / 32;
	int port = pinConfig->pin % 32;
	
	PORT->Group[group].OUTTGL.reg = (1 << port);
}

