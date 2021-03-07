#include "app_main.h"
#include "drv_uart.h"
#include "drv_spi.h"
#include "drv_i2c.h"
#include "FreeRTOS.h"
#include "task.h"
#include <sam.h>
#include <stdio.h>
#include <time.h>
#include "ff.h"

static FATFS fs;
static FIL fp;

static uint8_t write_data_perf[4096];

void app_init(void)
{
	// enable pull-ups on disconnected pins
	const uint8_t paUnused[] = {3, 4, 5, 6, 7, 12, 13, 14, 15, 16, 17, 18, 19, 28};
	const uint8_t pbUnused[] = {0, 1, 2, 3, 4, 5, 6, 7, 12, 13, 14, 22, 23, 30, 31};
	for (unsigned int i = 0; i < sizeof (paUnused) / sizeof (paUnused[0]); ++i)
		PORT_REGS->GROUP[0].PORT_PINCFG[paUnused[i]] = PORT_PINCFG_PULLEN(1);
	for (unsigned int i = 0; i < sizeof (pbUnused) / sizeof (pbUnused[0]); ++i)
		PORT_REGS->GROUP[1].PORT_PINCFG[pbUnused[i]] = PORT_PINCFG_PULLEN(1);
	
	for (int i = 0; i < 4096; ++i)
	{
		write_data_perf[i] = 3*i;
	}
	
	f_mount(&fs, "", 0);
}

void app_periodic(void)
{
	if (xTaskGetTickCount() % 1000) return;
	
	struct tm * time = localtime(NULL);
	char timestr[100];
	strftime(timestr, 100, "%Y-%m-%d %H:%M:%S", time);
	puts(timestr);
	
	FRESULT result;
	result = f_open(&fp, "hello.txt", FA_READ);
	if (result == FR_OK)
	{
		puts("Opened file for reading");
		while (f_gets(timestr, sizeof timestr, &fp)) {
			printf(timestr);
		}
		f_close(&fp);
	}
	else
	{
		puts("Failed to open file for reading");
	}
	UINT bw;
	result = f_open(&fp, "perf.txt", FA_CREATE_ALWAYS | FA_WRITE);
	if (result == FR_OK)
	{
		puts("Opened file for writing");
#if 0
		puts("SD CARD WRITE PERFORMANCE TEST START"); // wonder what happens if we get preempted
		for (int i = 0; i < 1024; ++i)
		{
			// write 4MB
			FRESULT res = f_write(&fp, write_data_perf, 4096, &bw);
			if (res)
			{
				printf("\r\nDISK WRITE ERROR %d\r\n", res);
				break;
			}
			if (bw < 4096)
			{
				printf("\r\nDISK FULL ERROR %d\r\n", bw);
				break;
			}
			if ((i % 64) == 0) printf("%d\r\n", i);
		}
		puts("DONE WRITING 4MB");
		///f_puts("Hey guys from uC!", &fp);A
#endif
		f_close(&fp);
	}
	else
	{
		puts("Failed to open file for writing");
	}
}

