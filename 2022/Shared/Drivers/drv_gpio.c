#include "drv_gpio.h"
#include "sam.h"

void drv_gpio_init(void)
{
	for (int pin_id = 0; pin_id < DRV_GPIO_PIN_COUNT; ++pin_id)
	{
		const struct drv_gpio_pinConfig * pinConfig = &drv_gpio_config.pinConfig[pin_id];
		
		int group = pinConfig->pin / 32; // the compiler will optimize these to shifts unless pigs fly
		int output = pinConfig->pin % 32;
		
		PORT_REGS->GROUP[group].PORT_PINCFG[output] = 0;
		if (pinConfig->input)
			PORT_REGS->GROUP[group].PORT_DIRCLR = (1 << output);
		else
			PORT_REGS->GROUP[group].PORT_DIRSET = (1 << output);
	}
}

void drv_gpio_setOutput(int pin_id, bool value)
{
	const struct drv_gpio_pinConfig * pinConfig = &drv_gpio_config.pinConfig[pin_id];
	
	int group = pinConfig->pin / 32;
	int port = pinConfig->pin % 32;
	
	if (value)
		PORT_REGS->GROUP[group].PORT_OUTSET = (1 << port);
	else
		PORT_REGS->GROUP[group].PORT_OUTCLR = (1 << port);
}

void drv_gpio_toggle(int pin_id)
{
	const struct drv_gpio_pinConfig * pinConfig = &drv_gpio_config.pinConfig[pin_id];
	
	int group = pinConfig->pin / 32;
	int port = pinConfig->pin % 32;
	
	PORT_REGS->GROUP[group].PORT_OUTTGL = (1 << port);
}

