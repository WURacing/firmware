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

struct adc_module adc_instance;

// TODO REMOVE
void configure_adc(void)
{
	struct adc_config config_adc;
	adc_get_config_defaults(&config_adc);
	config_adc.clock_prescaler = ADC_CLOCK_PRESCALER_DIV8;
	config_adc.reference       = ADC_REFERENCE_INTVCC2;
	config_adc.positive_input  = ADC_POSITIVE_INPUT_PIN0;
	config_adc.resolution      = ADC_RESOLUTION_16BIT;
	adc_init(&adc_instance, ADC0, &config_adc);
	adc_enable(&adc_instance);
}


void config_adc(int resolution, struct adc_module adc, int adc_index) {
	struct adc_config config_adc;
	adc_get_config_defaults(&config_adc);
	config_adc.clock_prescaler = ADC_CLOCK_PRESCALER_DIV8;
	config_adc.reference       = ADC_REFERENCE_INTVCC2;
	config_adc.positive_input  = ADC_POSITIVE_INPUT_PIN0;
	config_adc.resolution      = resolution;
	adc_init(&adc, adc_index, &config_adc);
	adc_enable(&adc);
}

config_adc(ADC_RESOLUTION_16BIT, adc_instance, ADC0);


// TODO REMOVE
void configure_gpio(void)
{
	struct system_pinmux_config config;
	
	system_pinmux_get_config_defaults(&config);
	config.direction = SYSTEM_PINMUX_PIN_DIR_INPUT;
	config.mux_position = 1;
	//system_pinmux_pin_set_config(PIN_PB08, &config);
	//system_pinmux_pin_set_config(PIN_PB09, &config);
	system_pinmux_pin_set_config(PIN_PA04, &config);
	system_pinmux_pin_set_config(PIN_PA05, &config);
	system_pinmux_pin_set_config(PIN_PA06, &config);
	system_pinmux_pin_set_config(PIN_PA07, &config);
	//system_pinmux_pin_set_config(PIN_PB06, &config);
	//system_pinmux_pin_set_config(PIN_PB07, &config);
}

// TODO MOVE TO OTHER FILE
struct system_pinmux_config config_gpio(void) {
	struct system_pinmux_config config;
	
	system_pinmux_get_config_defaults(&config);
	config.direction = SYSTEM_PINMUX_PIN_DIR_INPUT;
	config.mux_position = 1;
	return config;
}

void set_gpio(int pin, struct system_pinmux_config &config) {
	system_pinmux_pin_set_config(pin, config);
}

// TODO IMPLEMENT
struct system_pinmux_config c = config_gpio();
set_gpio(&c, PIN_PA04);
set_gpio(&c, PIN_PA05);
set_gpio(&c, PIN_PA06);
set_gpio(&c, PIN_PA07);

void adc_get_value(int pin, int adc_index, struct adc_module &adc, uint16_t* res) {
	int adc_pin = 0;
	if (pin == PIN_PA04 && adc_index == ADC0) {
		adc_pin = ADC_POSITIVE_INPUT_PIN3;
	}
	adc_set_positive_input(adc, adc_pin);
	adc_start_conversion(adc);
	while (adc_read(adc, res) == STATUS_BUSY) ;	
}

int main (void)
{
	system_init();
	delay_init();
	
	/*
	CAN
	*/
	configure_can();
	
	irq_initialize_vectors();
	cpu_irq_enable();
	
	// SD
	//initialize_rtc_calendar();
	
	delay_ms(1000);
	
	
	char test_file_name[50];
	Ctrl_status status;
	FRESULT res;
	FATFS fs;
	FIL file_object;
	FILINFO file_stat;
	char line[2200];
	int buf_len = 0;
	int logno = 0;
	int onetwentyeighths = 0;
	
	sd_mmc_init();

	
	
	/* CONFIG THE PINS */
	config_pins();
	configure_gpio();
	configure_adc();
	
	
	while (1) {
		
		sd_create_file();
		
		while(1) {
		
		system_interrupt_enable_global();
		
		// Main l00p
		//port_pin_set_output_level(LED_0_PIN, 1);
		int led_0_state = 0;
		int led_1_state = 0;
		int canline_ptr = 0;

		port_pin_set_output_level(LED_1_PIN, 1);
		int count = 0;
		
		uint16_t sensor_data[8];
		
		sensor_data[0] = sensor_data[1] = sensor_data[2] = sensor_data[3] =	sensor_data[4] = sensor_data[5] = sensor_data[6] = sensor_data[7] = 0;
		
		uint16_t currtime = 0;
		
		while (1) {
			/*
			if (rtc_calendar_is_periodic_interval(&rtc_instance, RTC_CALENDAR_PERIODIC_INTERVAL_0)) {
				rtc_calendar_clear_periodic_interval(&rtc_instance, RTC_CALENDAR_PERIODIC_INTERVAL_0);
				++onetwentyeighths;
			}
			if (rtc_calendar_is_periodic_interval(&rtc_instance, RTC_CALENDAR_PERIODIC_INTERVAL_7)) {
				rtc_calendar_clear_periodic_interval(&rtc_instance, RTC_CALENDAR_PERIODIC_INTERVAL_7);
				onetwentyeighths = 0;
				g_ul_ms_ticks = 0;
				read_time(&now);
			}
			*/
			adc_set_negative_input(&adc_instance, ADC_NEGATIVE_INPUT_GND);
			/*
			adc_set_positive_input(&adc_instance, ADC_POSITIVE_INPUT_PIN2);
			adc_start_conversion(&adc_instance);
			while (adc_read(&adc_instance, sensor_data + 0) == STATUS_BUSY) ;
			adc_set_positive_input(&adc_instance, ADC_POSITIVE_INPUT_PIN3);
			adc_start_conversion(&adc_instance);
			while (adc_read(&adc_instance, sensor_data + 1) == STATUS_BUSY) ;
			*/
			
			// TODO
			adc_get_value(PIN_PA04, ADC0, &adc_instance, sensor_data + 1);
			
			
			
			
			adc_set_positive_input(&adc_instance, ADC_POSITIVE_INPUT_PIN4);
			adc_start_conversion(&adc_instance);
			while (adc_read(&adc_instance, sensor_data + 2) == STATUS_BUSY) ;
			adc_set_positive_input(&adc_instance, ADC_POSITIVE_INPUT_PIN5);
			adc_start_conversion(&adc_instance);
			while (adc_read(&adc_instance, sensor_data + 3) == STATUS_BUSY) ;
			adc_set_positive_input(&adc_instance, ADC_POSITIVE_INPUT_PIN6);
			adc_start_conversion(&adc_instance);
			while (adc_read(&adc_instance, sensor_data + 4) == STATUS_BUSY) ;
			adc_set_positive_input(&adc_instance, ADC_POSITIVE_INPUT_PIN7);
			adc_start_conversion(&adc_instance);
			while (adc_read(&adc_instance, sensor_data + 5) == STATUS_BUSY) ;
			/*
			adc_set_positive_input(&adc_instance, ADC_POSITIVE_INPUT_PIN18);
			adc_start_conversion(&adc_instance);
			while (adc_read(&adc_instance, sensor_data + 6) == STATUS_BUSY) ;
			adc_set_positive_input(&adc_instance, ADC_POSITIVE_INPUT_PIN19);
			adc_start_conversion(&adc_instance);
			while (adc_read(&adc_instance, sensor_data + 7) == STATUS_BUSY) ;
			*/
			if (buf_len > 2048) {
				f_write(&file_object, line, 2048, NULL);
				buf_len -= 2048;
				
				f_sync(&file_object);
				
				shift_array_left(line, 2048, 2200 * 8);
			}
			
			//volatile can_message_t * cl = canline + (canline_ptr++);
			buf_len += sprintf(line + buf_len, "%i,%u,%u,%u,%u\n",
				currtime, sensor_data[2], sensor_data[3], sensor_data[4], sensor_data[5]
			);
			/*

			buf_len += sprintf(line + buf_len, "%08lx,%02x%02x%02x%02x%02x%02x%02x%02x\n",
			cl->id,
			cl->data.arr[0], cl->data.arr[1], cl->data.arr[2], cl->data.arr[3],
			cl->data.arr[4], cl->data.arr[5], cl->data.arr[6], cl->data.arr[7]);
			*/
			port_pin_set_output_level(LED_0_PIN, led_0_state);
			led_0_state = !led_0_state;
			
			currtime += 4;
			
			if (currtime % 10 == 0) {
				port_pin_set_output_level(LED_1_PIN, led_1_state);
				
				led_1_state = !led_1_state;
			}
			
			delay_ms(4);

			//canline_0_updated = 0;
			//if (--canline_i == 0) canline_ptr = 0;
				
			/*
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
			//}
			/*
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
			//}
			/* else {
				sprintf(line, "nobus\n");
				//printf(".", line);
				//port_pin_set_output_level(LED_0_PIN);

				// Write line
				if (f_puts(line, &file_object) == -1) goto sd_cleanup;
				// Flush
				f_sync(&file_object);
				count++;
				port_pin_set_output_level(LED_0_PIN, 1);
				delay_ms(100);
				port_pin_set_output_level(LED_0_PIN, 0);
				delay_ms(100);
			}
			*/
			/*
			while (canline_updated) {
				// Critical section
				can_disable_interrupt(&can_instance, CAN_RX_FIFO_1_NEW_MESSAGE);
				sprintf(line, "%d,%d,%d,%d,%d,%d,%d,%08lx,%02x%02x%02x%02x%02x%02x%02x%02x\n",
				now.year, now.month, now.day, now.hour, now.minute, now.second, Min(g_ul_ms_ticks, 999),
				canline.id,
				canline.data.arr[0], canline.data.arr[1], canline.data.arr[2], canline.data.arr[3],
				canline.data.arr[4], canline.data.arr[5], canline.data.arr[6], canline.data.arr[7]);
				printf(".", line);
				port_pin_set_output_level(LED_0_PIN, ledstate);
				ledstate = !ledstate;

				// Write line
				if (f_puts(line, &file_object) == -1) goto sd_cleanup;
				// Flush
				f_sync(&file_object);
				canline_updated = 0;
				can_enable_interrupt(&can_instance, CAN_RX_FIFO_1_NEW_MESSAGE);
			}
			*/
		}
		/*
		if (count > 50) {
			break;
		}
		*/
		
		}
		
		sd_cleanup:
		f_close(&file_object);
		
		port_pin_set_output_level(LED_1_PIN, 0);
		port_pin_set_output_level(LED_0_PIN, 1);
		delay_ms(1000);

		main_end_of_test:
		//printf("Please unplug the card.\r\n");
		port_pin_set_output_level(LED_0_PIN, 0);
		while (CTRL_NO_PRESENT != sd_mmc_check(0)) {
		}
	}
}
