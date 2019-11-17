
#include "util.h"

void shift_array_left(char *arr, const size_t size, const size_t bits) {
	const size_t chunks = bits / (8);

	if (chunks >= size) {
		return;
	}

	if (chunks) {
		memmove(arr, arr + chunks, size - chunks);
	}

	const size_t left = bits % (8);

	// If we have non directly addressable bits left we need to move the whole thing one by one.
	if (left) {
		const size_t right = (8) - left;
		const size_t l = size - chunks - 1;
		for (size_t i = 0; i < l; i++) {
			arr[i] = ((arr[i] << left) & ~left) | (arr[i+1] >> right);
		}
		arr[l] = (arr[l] << left) & ~left;
	}
}

void sd_create_file(void) {
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
	res = f_puts("timestamp,gpio_4,gpio_5,gpio_6,gpio_7\n", &file_object);
	if (res == -1) goto sd_cleanup;
}