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
	char line[256];
	int logno = 0;
	int onetwentyeighths = 0;
	
	sd_mmc_init();

	
	
	/* CONFIG THE PINS */
	config_pins();
	/*
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
	*/
	
	while (1) {
		//printf("Please plug an SD, MMC or SDIO card in slot.\n\r");

		/* Wait card present and ready */
		do {
			status = sd_mmc_test_unit_ready(0);
			
			if (CTRL_FAIL == status) {
				//printf("Failed to initialize SD card [%d], please re-insert the card.\r\n", status);
				//while (CTRL_NO_PRESENT != sd_mmc_check(0)) {
				//}
			}
		
		} while (CTRL_GOOD != status);
		
		port_pin_set_output_level(LED_0_PIN, 1);

		memset(&fs, 0, sizeof(FATFS));
		res = f_mount(LUN_ID_SD_MMC_0_MEM, &fs);
		if (FR_INVALID_DRIVE == res) {
			//printf("Failed to mount FAT32 filesystem on SD card [%d], please check that\r\n", res);
			goto main_end_of_test;
		}

		do {
			sprintf(test_file_name, "0:LOG%05d.CSV", logno++);
			bzero(&file_stat, sizeof(file_stat));
			res = f_stat(test_file_name, &file_stat);
		} while (res == FR_OK);
		
		if (res != FR_NO_FILE) {
			//printf("Failed to find new file on card [%d]\r\n", res);
			goto main_end_of_test;
		}
		
		test_file_name[0] = LUN_ID_SD_MMC_0_MEM + '0';
		res = f_open(&file_object, (char const *)test_file_name, FA_CREATE_ALWAYS | FA_WRITE);
		if (res != FR_OK) {
			//printf("Failed to create file on card [%d]\r\n", res);
			goto main_end_of_test;
		}

		//printf("Starting data logging...\r\n");
		res = f_puts("year,month,day,hour,min,sec,ms,id,data\n", &file_object);
		if (res == -1) goto sd_cleanup;
		
		while(1) {
		
		system_interrupt_enable_global();
		
		// Main l00p
		//port_pin_set_output_level(LED_0_PIN, 1);
		port_pin_set_output_level(LED_1_PIN, 1);
		int count = 0;
		while (count < 50) {
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
			if (canline_0_updated) {
				sprintf(line, "bus0\n");
				//printf(".", line);
				//port_pin_set_output_level(LED_0_PIN);

				// Write line
				if (f_puts(line, &file_object) == -1) goto sd_cleanup;
				// Flush
				f_sync(&file_object);
				canline_0_updated = 0;
				count++;
			}
			if (canline_1_updated) {
				sprintf(line, "bus1\n");
				//printf(".", line);
				//port_pin_set_output_level(LED_0_PIN);

				// Write line
				if (f_puts(line, &file_object) == -1) goto sd_cleanup;
				// Flush
				f_sync(&file_object);
				canline_1_updated = 0;
				count++;
			}
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
		if (count > 50) {
			break;
		}
		
		}
		
		sd_cleanup:
		f_close(&file_object);
		
		port_pin_set_output_level(LED_1_PIN, 1);
		port_pin_set_output_level(LED_0_PIN, 1);
		delay_ms(1000);

		main_end_of_test:
		//printf("Please unplug the card.\r\n");
		port_pin_set_output_level(LED_0_PIN, 0);
		while (CTRL_NO_PRESENT != sd_mmc_check(0)) {
		}
	}
}
