/*
 * sd_task.c
 *
 * Created: 1/20/2020 1:29:56 PM
 *  Author: Connor
 */ 

#include "exports.h"

xTaskHandle sd_task_id;

xSemaphoreHandle new_data_semaphore;

Ctrl_status status;
FRESULT res;
FATFS fs;
FIL file_object;
FILINFO file_stat;
char filename[50];
int logno = 0;

#define WRITE_BUFLEN 2048
char writebuf[WRITE_BUFLEN];
char *writebufptr = writebuf;

#define LINE_BUFLEN 100
char linebuf[LINE_BUFLEN];


static int init_sd_card_and_filesystem(void)
{
	do
	{
		status = sd_mmc_test_unit_ready(0);

		if (CTRL_FAIL == status)
		{
			// SD card initialization failed
			// wait for the user to unplug the card before trying again
			while (SD_MMC_ERR_NO_CARD != sd_mmc_check(0)) {
			}
		}

	} while (CTRL_GOOD != status);
		
	memset(&fs, 0, sizeof(FATFS));
	res = f_mount(LUN_ID_SD_MMC_0_MEM, &fs);
	if (FR_INVALID_DRIVE == res)
	{
		return 1;
	}
	
	// find the latest filename available
	do
	{
		snprintf(filename, 50, "0:LOG%05d.CSV", logno++);
		bzero(&file_stat, sizeof(file_stat));
		res = f_stat(filename, &file_stat);
	} while (res == FR_OK);
	
	if (res != FR_NO_FILE)
	{
		return 2;
	}
	
	filename[0] = LUN_ID_SD_MMC_0_MEM + '0';
	res = f_open(&file_object, (char const *)filename, FA_CREATE_ALWAYS | FA_WRITE);
	if (res != FR_OK)
	{
		return 3;
	}
	
	strncpy(linebuf, "year,month,day,hour,min,sec,ms,id,data\n", LINE_BUFLEN);
	res = f_write(&file_object, linebuf, strlen(linebuf), NULL);
	if (res == -1)
	{
		return 4;
	}

	
	return 0;
}

static int buffer_data(const char *input)
{
	int res;
	if (*input == '\0') return 0; // base case
	
	// add characters from input to buffer as they fit
	while (*input != '\0' && writebufptr < (writebuf + WRITE_BUFLEN))
	{
		// write current input char to current buf position
		*(writebufptr++) = *(input++);
	}
	// check for buffer end
	if (writebufptr == (writebuf + WRITE_BUFLEN))
	{
		// sync buffers
		res = f_write(&file_object, writebuf, WRITE_BUFLEN, NULL);
		writebufptr = writebuf;
		if (res != 0) return res;
		res = f_sync(&file_object);
		if (res != 0) return res;
	}
	// did we finish writing the line in case of overflow?
	if (*input != '\0')
	{
		return buffer_data(input);
	}
	return 0;
}

void sd_task(void *pvParameters)
{
	enum read_message_status rms;
	struct can_message message;
	int hr, min, sec, ms;
	
	//vQueueAddToRegistry(new_data_semaphore, "Incoming CAN data");
    //xSemaphoreTake(notification_semaphore, 0);
sd_start:	
	// set up the SD card
	while (init_sd_card_and_filesystem() != 0);
	
	led0 = 1;

	while (1)
	{
		if (xSemaphoreTake(new_data_semaphore, 1000))
		{
			led1 = !led1;
			ms = xTaskGetTickCount(); // THIS ASSUMES KERNEL TICKS AT 1000Hz
			sec = ms / 1000; ms = ms % 1000;
			min = sec / 60; sec = sec % 60;
			hr = min / 60; min = min % 60;
			// read all new messages, just in case we swallowed an interrupt
			while ((rms = read_message(&message)) == READ_ONE)
			{
				// format message
				snprintf(linebuf, LINE_BUFLEN, "0,0,0,%d,%d,%d,%d,%08lx,%02x%02x%02x%02x%02x%02x%02x%02x\n",
					hr, min, sec, ms,
					message.id, message.data[0], message.data[1], message.data[2], message.data[3],
					message.data[4], message.data[5], message.data[6], message.data[7]
				);
				if (buffer_data(linebuf) != 0)
				{
					led0 = led1 = 0;
					goto sd_start;
				}
			}
			if (rms == READ_ERROR)
			{
				// error!
			}
		}
	}
	vTaskDelete(NULL);
}