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

void app_init(void)
{
	// enable pull-ups on disconnected pins
	const uint8_t paUnused[] = {3, 4, 5, 6, 7, 12, 13, 14, 15, 16, 17, 18, 19, 28};
	const uint8_t pbUnused[] = {0, 1, 2, 3, 4, 5, 6, 7, 12, 13, 14, 22, 23, 30, 31};
	for (unsigned int i = 0; i < sizeof (paUnused) / sizeof (paUnused[0]); ++i)
		PORT_REGS->GROUP[0].PORT_PINCFG[paUnused[i]] = PORT_PINCFG_PULLEN(1);
	for (unsigned int i = 0; i < sizeof (pbUnused) / sizeof (pbUnused[0]); ++i)
		PORT_REGS->GROUP[1].PORT_PINCFG[pbUnused[i]] = PORT_PINCFG_PULLEN(1);
	
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
	
	result = f_open(&fp, "output.txt", FA_CREATE_ALWAYS | FA_WRITE);
	if (result == FR_OK)
	{
		puts("Opened file for writing");
		f_puts("Hey guys from uC!", &fp);
		f_close(&fp);
	}
	else
	{
		puts("Failed to open file for writing");
	}
}

