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
#include "util.h"

int main (void)
{
	system_init();
	delay_init();
	
	/*
	CAN
	*/
	configure_can();
	
	system_interrupt_enable_global();
	
	// need this??
	//irq_initialize_vectors();
	//cpu_irq_enable();
	
	struct port_config config_port_pin;
	port_get_config_defaults(&config_port_pin);
	config_port_pin.direction = PORT_PIN_DIR_OUTPUT;
	port_pin_set_config(LED_0_PIN, &config_port_pin);
	port_pin_set_config(LED_1_PIN, &config_port_pin);

	while (1) {
		if (canline_0_updated) {
			port_pin_set_output_level(LED_0_PIN, 1);
			delay_ms(500);
			port_pin_set_output_level(LED_0_PIN, 0);
			canline_0_updated = 0;
			delay_ms(500);
		}
		if (canline_1_updated) {
			port_pin_set_output_level(LED_1_PIN, 1);
			delay_ms(500);
			port_pin_set_output_level(LED_1_PIN, 0);
			canline_1_updated = 0;
			delay_ms(500);
		}
	}

	/* Insert application code here, after the board has been initialized. */
}
