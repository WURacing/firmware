/*
 * sd.c
 *
 * Created: 1/8/2020 3:24:41 PM
 *  Author: etshr
 */

#include "sd.h"

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

int sd_create_file(struct sd_instance *const sd_inst)
{
	
	char test_file_name[50];
	Ctrl_status status;
	FRESULT res;
	FATFS fs;
	FILINFO file_stat;
	int logno = 0;
	
	sd_mmc_init();
	
	while (1) {
		//printf("Please plug an SD, MMC or SDIO card in slot.\n\r");

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
			printf("Failed to mount FAT32 file system on SD card [%d], please check that\r\n", res);
			return 1;
		}

		do {
			sprintf(test_file_name, "0:LOG%05d.CSV", logno++);
			bzero(&file_stat, sizeof(file_stat));
			res = f_stat(test_file_name, &file_stat);
		} while (res == FR_OK);
		
		if (res != FR_NO_FILE) {
			//printf("Failed to find new file on card [%d]\r\n", res);
			return 2;
		}
		
		test_file_name[0] = LUN_ID_SD_MMC_0_MEM + '0';
		res = f_open(&sd_inst->file, (char const *)test_file_name, FA_CREATE_ALWAYS | FA_WRITE);
		if (res != FR_OK) {
			//printf("Failed to create file on card [%d]\r\n", res);
			return 3;
		}

		//printf("Starting data logging...\r\n");
		res = f_puts("year,month,day,hour,min,sec,ms,id,data\n", &sd_inst->file);
		if (res == -1) return 4;

		return 0;
}

int sd_write(char *data, struct sd_instance *const sd_inst) {
		
			sd_inst->buf_len += sprintf(&sd_inst->buf + sd_inst->buf_len, data); 
		
			if (sd_inst->buf_len > 2048) {
				f_write(&sd_inst->file, sd_inst->buf, 2048, NULL);
				sd_inst->buf -= 2048;
				
				f_sync(&sd_inst->file);
				
				shift_array_left(sd_inst->buf, 2048, 2200 * 8);
			}
}

void sd_close(struct sd_instance *const sd_inst) {
	f_close(&sd_inst->file);
}
