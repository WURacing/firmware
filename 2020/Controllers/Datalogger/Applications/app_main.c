#include "app_main.h"
#include "drv_uart.h"
#include "drv_spi.h"
#include "drv_i2c.h"
#include "FreeRTOS.h"
#include "task.h"
#include <sam.h>
#include <stdio.h>

void app_init(void)
{
	// enable pull-ups on disconnected pins
	const uint8_t paUnused[] = {3, 4, 5, 6, 7, 12, 13, 14, 15, 16, 17, 18, 19, 28};
	const uint8_t pbUnused[] = {0, 1, 2, 3, 4, 5, 6, 7, 12, 13, 14, 22, 23, 30, 31};
	for (int i = 0; i < sizeof (paUnused) / sizeof (paUnused[0]); ++i)
		PORT_REGS->GROUP[0].PORT_PINCFG[paUnused[i]] = PORT_PINCFG_PULLEN(1);
	for (int i = 0; i < sizeof (pbUnused) / sizeof (pbUnused[0]); ++i)
		PORT_REGS->GROUP[1].PORT_PINCFG[pbUnused[i]] = PORT_PINCFG_PULLEN(1);
}

static uint8_t calenread[7];

void app_periodic(void)
{
	if (xTaskGetTickCount() % 1000) return;

	int r = drv_i2c_read_register(DRV_I2C_CHANNEL_RTC, 0x68, 0x00, calenread, 7);

	if (r == 7)
	{
		if (calenread[0] & 0x80)
		{
			printf("RTC is unconfigured, configuring...\r\n");
			calenread[0] = (0 << 7) | ((__TIME__[6] - '0') << 4) | (__TIME__[7] - '0'); //seconds
			calenread[1] = ((__TIME__[3] - '0') << 4) | (__TIME__[4] - '0'); //minutes
			calenread[2] = (0 << 6) | ((__TIME__[0] - '0') << 4) | (__TIME__[1] - '0'); //hours(24-hr)
			calenread[3] = 7;
			calenread[4] = 0x20;
			calenread[5] = 0x12;
			calenread[6] = 0x20;
			r = drv_i2c_write_register(DRV_I2C_CHANNEL_RTC, 0x68, 0x00, calenread, 7);
			
			if (r == 7)
				printf("Configuration success!\r\n");
			else
				printf("configuration fail...\r\n");
		}
		else
		{
			printf("20%02x-%02x-%02x %02x:%02x:%02x\r\n", calenread[6], calenread[5], calenread[4], calenread[2], calenread[1], calenread[0]);
		}
	}
	else
	{
		printf("RTC READ ERROR\r\n");
	}
}

