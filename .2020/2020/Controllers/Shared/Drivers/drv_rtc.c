/* 
 * File:   drv_rtc.h
 * Author: Connor
 *
 * Created on March 7, 2021, 11:57 AM
 *
 * Purpose: Implement real time clock interface (calendar, seconds resolution) over i2c with DS1307. Used by FatFs.
 */

#include "drv_rtc.h"
#include <time.h>
#include "ff.h"
#include "diskio.h"
#include "drv_i2c.h"
#include "FreeRTOS.h"
#include "task.h"

static struct drv_rtc_data {
	struct tm local;
	uint8_t bcd[7];
} drv_rtc_data;

static void update_localtime(void)
{
	drv_rtc_data.local.tm_sec = (drv_rtc_data.bcd[0] & 0xF) + (drv_rtc_data.bcd[0] >> 4 & 0xF) * 10;
	drv_rtc_data.local.tm_min = (drv_rtc_data.bcd[1] & 0xF) + (drv_rtc_data.bcd[1] >> 4 & 0xF) * 10;
	drv_rtc_data.local.tm_hour = (drv_rtc_data.bcd[2] & 0xF) + (drv_rtc_data.bcd[2] >> 4 & 0xF) * 10;
	drv_rtc_data.local.tm_mday = (drv_rtc_data.bcd[4] & 0xF) + (drv_rtc_data.bcd[4] >> 4 & 0xF) * 10;
	drv_rtc_data.local.tm_mon = (drv_rtc_data.bcd[5] & 0xF) + (drv_rtc_data.bcd[5] >> 4 & 0xF) * 10 - 1;
	drv_rtc_data.local.tm_year = (drv_rtc_data.bcd[6] & 0xF) + (drv_rtc_data.bcd[6] >> 4 & 0xF) * 10 + 100;
	drv_rtc_data.local.tm_wday = drv_rtc_data.bcd[3] - 1;
	drv_rtc_data.local.tm_yday = 0;
	drv_rtc_data.local.tm_isdst = 0; /* CST */
}

void drv_rtc_init(void)
{
	int r = drv_i2c_read_register(DRV_I2C_CHANNEL_RTC, 0x68, 0x00, drv_rtc_data.bcd, 7);

	if (r == 7)
	{
		if (drv_rtc_data.bcd[0] & 0x80)
		{
			drv_rtc_data.bcd[0] = (0 << 7) | ((__TIME__[6] - '0') << 4) | (__TIME__[7] - '0'); //seconds
			drv_rtc_data.bcd[1] = ((__TIME__[3] - '0') << 4) | (__TIME__[4] - '0'); //minutes
			drv_rtc_data.bcd[2] = (0 << 6) | ((__TIME__[0] - '0') << 4) | (__TIME__[1] - '0'); //hours(24-hr)
			drv_rtc_data.bcd[3] = 1; // sunday
			drv_rtc_data.bcd[4] = 0x07;
			drv_rtc_data.bcd[5] = 0x03;
			drv_rtc_data.bcd[6] = 0x21;
			r = drv_i2c_write_register(DRV_I2C_CHANNEL_RTC, 0x68, 0x00, drv_rtc_data.bcd, 7);
		}
		update_localtime();
	}
}

void drv_rtc_periodic(void)
{
	if (xTaskGetTickCount() % 10) return;
	
	if (drv_i2c_read_register(DRV_I2C_CHANNEL_RTC, 0x68, 0x00, drv_rtc_data.bcd, 7) == 7)
	{
		update_localtime();
	}
}

DWORD get_fattime (void)
{
	return ((drv_rtc_data.local.tm_year - 80) << 25) |
			((drv_rtc_data.local.tm_mon + 1) << 21) |
			(drv_rtc_data.local.tm_mday << 16) |
			(drv_rtc_data.local.tm_hour << 11) |
			(drv_rtc_data.local.tm_min << 5) |
			(drv_rtc_data.local.tm_sec / 2);
}

struct tm *localtime (const time_t *_timer)
{
	if (_timer == NULL)
	{
		return &drv_rtc_data.local;
	}
	else
	{
		return NULL;
	}
}