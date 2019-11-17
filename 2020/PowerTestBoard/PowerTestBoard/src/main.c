/**
 * \file
 *
 * \brief Empty user application template
 *
 */

/**
 * \mainpage User Application template doxygen documentation
 *
 * \par Empty user application template
 *
 * Bare minimum empty user application template
 *
 * \par Content
 *
 * -# Include the ASF header files (through asf.h)
 * -# Minimal main function that starts with a call to system_init()
 * -# "Insert application code here" comment
 *
 */

/*
 * Include header files for all drivers that have been imported from
 * Atmel Software Framework (ASF).
 */
/*
 * Support and FAQ: visit <a href="https://www.microchip.com/support/">Microchip Support</a>
 */
#include <asf.h>
#include "mcp23017.h"

static void configure_gpio()
{
	struct system_pinmux_config config;
	
	system_pinmux_get_config_defaults(&config);
	config.direction = SYSTEM_PINMUX_PIN_DIR_OUTPUT;
	for (int i = 0; i < sizeof(TONY_PINS_1); ++i) {
		system_pinmux_pin_set_config(TONY_PINS_1[i], &config);
	}
}

int main (void)
{
	system_init();
	delay_init();
	configure_gpio();
	configure_i2c();
	initialize_mcp23017();

	/* Insert application code here, after the board has been initialized. */
	int clock_state = 0;
	while (1)
	{
		clock_state = !clock_state;
		for (int i = 0; i < sizeof(TONY_PINS_1); ++i) {
			port_pin_set_output_level(TONY_PINS_1[i], clock_state);
		}
		
		mcp23017_send_data(-clock_state, -clock_state);
		
		delay_s(5);
	}
}
