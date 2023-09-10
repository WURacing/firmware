#include <asf.h>
#include "util.h"

volatile uint32_t g_ul_ms_ticks = 0;
void SysTick_Handler(void)
{
	g_ul_ms_ticks++;
}

bool ledstate;

int main (void)
{
	system_init();
	delay_init();
	
	
	struct rtc_calendar_time now;

	
	/* Insert application code here, after the board has been initialized. */
	configure_usart_cdc();
	
	port_pin_set_output_level(LED_0_PIN, 1);
	
	configure_can();
	
	system_interrupt_enable_global();

	
	configure_i2c();
	initialize_rtc_calendar();
	SysTick_Config(48000000 / 1000);
	
	irq_initialize_vectors();
	cpu_irq_enable();

	
	


	char test_file_name[50];
	Ctrl_status status;
	FRESULT res;
	FATFS fs;
	FIL file_object;
	FILINFO file_stat;
	char line[256];
	int logno = 0;
	int onetwentyeighths = 0;


	/* Initialize SD MMC stack */
	sd_mmc_init();

	while (1) {
		printf("Please plug an SD, MMC or SDIO card in slot.\n\r");

		/* Wait card present and ready */
		do {
			status = sd_mmc_test_unit_ready(0);
			if (CTRL_FAIL == status) {
				printf("Failed to initialize SD card [%d], please re-insert the card.\r\n", status);
				while (CTRL_NO_PRESENT != sd_mmc_check(0)) {
				}
			}
		} while (CTRL_GOOD != status);

		memset(&fs, 0, sizeof(FATFS));
		res = f_mount(LUN_ID_SD_MMC_0_MEM, &fs);
		if (FR_INVALID_DRIVE == res) {
			printf("Failed to mount FAT32 filesystem on SD card [%d], please check that\r\n", res);
			goto main_end_of_test;
		}

		do {
			sprintf(test_file_name, "0:LOG%05d.CSV", logno++);
			bzero(&file_stat, sizeof(file_stat));
			res = f_stat(test_file_name, &file_stat);
		} while (res == FR_OK);
		
		if (res != FR_NO_FILE) {
			printf("Failed to find new file on card [%d]\r\n", res);
			goto main_end_of_test;
		}
		
		test_file_name[0] = LUN_ID_SD_MMC_0_MEM + '0';
		res = f_open(&file_object, (char const *)test_file_name, FA_CREATE_ALWAYS | FA_WRITE);
		if (res != FR_OK) {
			printf("Failed to create file on card [%d]\r\n", res);
			goto main_end_of_test;
		}

		printf("Starting data logging...\r\n");
		res = f_puts("year,month,day,hour,min,sec,ms,id,data\n", &file_object);
		if (res == -1) goto sd_cleanup;
		
		// Main l00p
		while (1) {
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
		}
		
		sd_cleanup:
		f_close(&file_object);

		main_end_of_test:
		printf("Please unplug the card.\r\n");
		port_pin_set_output_level(LED_0_PIN, 0);
		while (CTRL_NO_PRESENT != sd_mmc_check(0)) {
		}
	}

}
