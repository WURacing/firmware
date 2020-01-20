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

void config_pins(void)
{
	struct port_config config_port_pin;
	port_get_config_defaults(&config_port_pin);
	config_port_pin.direction = PORT_PIN_DIR_OUTPUT;
	port_pin_set_config(LED_0_PIN, &config_port_pin);
	port_pin_set_config(LED_1_PIN, &config_port_pin);
	port_pin_set_config(RAD_FAN_ENABLE_PIN, &config_port_pin);
	port_pin_set_config(FUEL_PUMP_ENABLE_PIN, &config_port_pin);

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
		int rad_fan_current_state = HC_CURRENT_ON;
		int fuel_pump_current_state = HC_CURRENT_ON;

		port_pin_set_output_level(RAD_FAN_ENABLE_PIN, rad_fan_current_state);
		port_pin_set_output_level(FUEL_PUMP_ENABLE_PIN, fuel_pump_current_state);

		port_pin_set_output_level(LED_1_PIN, 1);

		int halfmillis = 0;
		
		uint8_t tmp_data[8];

		uint8_t curr_data[8];

		// MAIN LOOP

		set_current_control_reg(14, LC_CURRENT_OFF);
		//set_current_control_reg(12, LC_CURRENT_OFF);
		//set_current_control_reg(10, LC_CURRENT_OFF);
		volatile uint16_t current_status = set_current_control();

		int should_change_current_control = 0;

		while (1)
		{
			if (halfmillis % 10 == 0)
			{
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
				
				if (halfmillis % 500 == 250)
				{

					can_send_extended_message(TMP_PKT_ID, tmp_data, 8);

					port_pin_set_output_level(LED_1_PIN, led_1_state);
					led_1_state = !led_1_state;
				}
				
				// ***************
				// CURRENT SECTION
				// ***************
				
				uint16_t current_reading;

				volatile struct current_readings power;
				read_current(&power);
				
				int i;
				for (i = 0; i < 17; ++i)
				{
					if (power.lc[i] > LC_CURR_LIMIT)
					{
						set_current_control_reg(i, LC_CURRENT_OFF);
						should_change_current_control = 1;
					}
				}
				if (power.fuel_pump > HC_CURR_LIMIT)
				{
					rad_fan_current_state = HC_CURRENT_OFF;
					should_change_current_control = 1;
				}
				if (power.rad_fan > HC_CURR_LIMIT)
				{
					rad_fan_current_state = HC_CURRENT_OFF;
					should_change_current_control = 1;
				}
				if (power.total > TOTAL_CURR_LIMIT)
				{
					// derp this is bad
				}
				
				if (should_change_current_control == 1)
				{
					should_change_current_control = 0;
					port_pin_set_output_level(RAD_FAN_ENABLE_PIN, rad_fan_current_state);
					port_pin_set_output_level(FUEL_PUMP_ENABLE_PIN, fuel_pump_current_state);

					current_status = set_current_control();

					char errStr[] = "____";

					if (rad_fan_current_state == HC_CURRENT_OFF)
					{
						errStr[1] = "L";
					}

					if (fuel_pump_current_state == HC_CURRENT_OFF)
					{
						errStr[2] = "L";
					}

					if (current_status != 0)
					{
						errStr[3] = "L";
					}

					screen_set_string(&errStr);
				}
				
				if (halfmillis % 500 == 0)
				{
					if (halfmillis % 1000 == 0)
					{
						curr_data[7] = (power.lc[6] >> 2) & 0xFF;
						curr_data[6] = (power.lc[8] >> 2) & 0xFF;
						curr_data[5] = (power.lc[10] >> 2) & 0xFF;
						curr_data[4] = (power.lc[12] >> 2) & 0xFF;
						curr_data[3] = (power.lc[14] >> 2) & 0xFF;
						curr_data[2] = (power.lc[16] >> 2) & 0xFF;
						curr_data[1] = 0;
						curr_data[0] = 0;

						can_send_extended_message(LC_PKT_ID, curr_data, 8);
					} else {
						curr_data[6] = power.fuel_pump >> 8;
						curr_data[7] = power.fuel_pump & 0xFF;
						curr_data[4] = power.rad_fan >> 8;
						curr_data[5] = power.rad_fan & 0xFF;
						curr_data[2] = power.total >> 8;
						curr_data[3] = power.total & 0xFF;

						can_send_extended_message(HC_PKT_ID, curr_data, 8);
					}

					port_pin_set_output_level(LED_0_PIN, led_0_state);
					led_0_state = !led_0_state;
				}

			}

			delay_us(495);
			screen_render();

			halfmillis++;
		}
	}
}