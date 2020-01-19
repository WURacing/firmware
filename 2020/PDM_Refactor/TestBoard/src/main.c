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
#include "sd.h"

void config_pins(void) {
	struct port_config config_port_pin;
	port_get_config_defaults(&config_port_pin);
	config_port_pin.direction = PORT_PIN_DIR_OUTPUT;
	port_pin_set_config(LED_0_PIN, &config_port_pin);
	port_pin_set_config(LED_1_PIN, &config_port_pin);	
	
	config_port_pin.direction = PORT_PIN_DIR_INPUT;
	config_port_pin.input_pull = PORT_PIN_PULL_DOWN;
	port_pin_set_config(PIN_PA12, &config_port_pin);
}

int main (void)
{
	system_init();
	delay_init();

	configure_can();
	
	irq_initialize_vectors();
	cpu_irq_enable();
	
	delay_ms(1000);
	
	
	int onetwentyeighths = 0;
	
	if (sd_create_file(&sd_inst) != 0) {
		// SD configuration is bad
	}

	/* CONFIG THE PINS */
	config_pins();
		
		while(1) {
		
		system_interrupt_enable_global();
		
		// Main l00p
		//port_pin_set_output_level(LED_0_PIN, 1);
		int led_0_state = 0;
		int led_1_state = 0;
		int canline_ptr = 0;

		port_pin_set_output_level(LED_1_PIN, 1);
		int count = 0;
		while (1) {
			
			if (canline_0_updated) {
				
				volatile can_message_t * cl = canline + (canline_ptr++);

				buf_len += sprintf(line + buf_len, "%08lx,%02x%02x%02x%02x%02x%02x%02x%02x\n",
				cl->id,
				cl->data.arr[0], cl->data.arr[1], cl->data.arr[2], cl->data.arr[3],
				cl->data.arr[4], cl->data.arr[5], cl->data.arr[6], cl->data.arr[7]);

				port_pin_set_output_level(LED_0_PIN, led_0_state);
				led_0_state = !led_0_state;

				canline_0_updated = 0;
				if (--canline_i == 0) canline_ptr = 0;
				/*
				sprintf(line + len, "bus0\n");
				//printf(".", line);
				//port_pin_set_output_level(LED_0_PIN);

				// Write line
				if (f_puts(line, &file_object) == -1) goto sd_cleanup;
				// Flush
				f_sync(&file_object);
				canline_0_updated = 0;
				count++;
				*/
			}
			if (canline_1_updated) {
				volatile can_message_t * cl = canline + (canline_ptr++);

				buf_len += sprintf(line + buf_len, "%08lx,%02x%02x%02x%02x%02x%02x%02x%02x\n",
				cl->id,
				cl->data.arr[0], cl->data.arr[1], cl->data.arr[2], cl->data.arr[3],
				cl->data.arr[4], cl->data.arr[5], cl->data.arr[6], cl->data.arr[7]);

				port_pin_set_output_level(LED_1_PIN, led_1_state);
				led_1_state = !led_1_state;

				canline_1_updated = 0;
				if (--canline_i == 0) canline_ptr = 0;
				/*
				sprintf(line + len, "bus1\n");
				//printf(".", line);
				//port_pin_set_output_level(LED_0_PIN);

				// Write line
				if (f_puts(line, &file_object) == -1) goto sd_cleanup;
				// Flush
				f_sync(&file_object);
				canline_1_updated = 0;
				count++;
				*/
			}
		}
		
		
	}
}
