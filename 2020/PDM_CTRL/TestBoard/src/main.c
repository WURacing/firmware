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
#include "current.h"
#include "screen.h"
#include "CANBus19.h"
#include "current_control.h"

#define TMP_PKT_ID 0x1f00111

void config_pins(void)
{
	struct port_config config_port_pin;
	port_get_config_defaults(&config_port_pin);
	config_port_pin.direction = PORT_PIN_DIR_OUTPUT;
	port_pin_set_config(LED_0_PIN, &config_port_pin);
	port_pin_set_config(LED_1_PIN, &config_port_pin);

	config_port_pin.direction = PORT_PIN_DIR_INPUT;
	config_port_pin.input_pull = PORT_PIN_PULL_DOWN;
	port_pin_set_config(PIN_PA12, &config_port_pin);
}

int main(void)
{
	system_init();
	delay_init();
	volatile enum status_code current_stat = configure_current();

	/*
	CAN
	*/
	configure_can();
	configure_i2c();
	volatile int conftmp = configure_tmp75a(TMP75A_ADDR_AIR);
	conftmp = configure_tmp75a(TMP75A_ADDR_FET);
	conftmp = configure_tmp75a(TMP75A_ADDR_MCU);
	configure_screen();
	
	configure_current_control();
	
	screen_set_string("FEED");

	irq_initialize_vectors();
	cpu_irq_enable();

	delay_ms(1000);

	/* CONFIG THE PINS */
	config_pins();

	while (1)
	{

		system_interrupt_enable_global();

		// Main l00p
		//port_pin_set_output_level(LED_0_PIN, 1);
		int led_0_state = 0;
		int led_1_state = 0;
		int canline_ptr = 0;

		port_pin_set_output_level(LED_1_PIN, 1);

		int count = 0;
		//uint16_t tmp_data[4];
		uint8_t tmp_data[8];

		// MAIN LOOP
		
		
		set_current_control_reg(14, CURRENT_ON);
		volatile uint16_t current_status = set_current_control();

		while (1)
		{


			if (count % 10 == 0) {
				volatile uint16_t tmp = read_tmp75a(TMP75A_ADDR_MCU) >> 4;

				// CONVERSION = (x >> 4) * 0.0625
				tmp_data[7] = tmp & 0x04;
				tmp_data[6] = tmp >> 4;
				
				tmp = read_tmp75a(TMP75A_ADDR_AIR) >> 4;
				
				tmp_data[5] = tmp & 0x04;
				tmp_data[4] = tmp >> 4;
				
				tmp = read_tmp75a(TMP75A_ADDR_FET) >> 4;
				
				tmp_data[3] = tmp & 0x04;
				tmp_data[2] = tmp >> 4;
				
				tmp_data[1] = current_status >> 8;
				tmp_data[0] = current_status & 0xFF;
				
				can_send_extended_message(TMP_PKT_ID, tmp_data, 8);
				
				port_pin_set_output_level(LED_1_PIN, led_1_state);
				led_1_state = !led_1_state;
			}
			
			
			if (count % 10 == 0)
			{
				
				uint16_t current_reading;
				
				volatile struct current_readings power;
				read_current(&power);
				
				port_pin_set_output_level(LED_0_PIN, led_0_state);
				led_0_state = !led_0_state;
			}
			
			delay_us(1000);
			screen_render();

			count++;
		}
	}
}