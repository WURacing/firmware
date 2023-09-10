/* 
 * File:   drv_rtc.h
 * Author: Connor
 *
 * Created on March 7, 2021, 11:57 AM
 *
 * Purpose: Implement real time clock interface (calendar, seconds resolution).
 *	Primary source: CPU peripheral RTC. Loaded at boot time from battery-backup RTC,
 *  the DS1307 over I2C. If this source is uninitialized (battery swapped), use the GPS time.
 */

#include "drv_rtc.h"
#include <time.h>
#include <string.h>
#include "ff.h"
#include "diskio.h"
#include "drv_i2c.h"
#include "FreeRTOS.h"
#include "task.h"
#include "drv_lte.h"

// must be a leap year per datasheet
#define SAM_RTC_EPOCH 2000
#define TM_EPOCH 1900
#define DS1307_EPOCH 2000
#define LTE_EPOCH 2000

static struct drv_rtc_data {
	bool valid;
	bool ds1307_initialized;
	volatile struct tm local;
	uint8_t bcd[7];
	volatile uint32_t clock_reg;
	struct drv_lte_time lte;
	volatile TickType_t ms_epoch;
} drv_rtc_data = {0};

static void update_localtime_from_bcd(void)
{
	drv_rtc_data.local.tm_sec = (drv_rtc_data.bcd[0] & 0xF) + (drv_rtc_data.bcd[0] >> 4 & 0xF) * 10;
	drv_rtc_data.local.tm_min = (drv_rtc_data.bcd[1] & 0xF) + (drv_rtc_data.bcd[1] >> 4 & 0xF) * 10;
	drv_rtc_data.local.tm_hour = (drv_rtc_data.bcd[2] & 0xF) + (drv_rtc_data.bcd[2] >> 4 & 0xF) * 10;
	drv_rtc_data.local.tm_mday = (drv_rtc_data.bcd[4] & 0xF) + (drv_rtc_data.bcd[4] >> 4 & 0xF) * 10;
	drv_rtc_data.local.tm_mon = (drv_rtc_data.bcd[5] & 0xF) + (drv_rtc_data.bcd[5] >> 4 & 0xF) * 10 - 1;
	drv_rtc_data.local.tm_year = (drv_rtc_data.bcd[6] & 0xF) + (drv_rtc_data.bcd[6] >> 4 & 0xF) * 10 + (DS1307_EPOCH - TM_EPOCH);
	drv_rtc_data.local.tm_wday = drv_rtc_data.bcd[3] - 1;
	drv_rtc_data.local.tm_yday = 0;
	drv_rtc_data.local.tm_isdst = 0; /* CST */
}

static void update_bcd_from_localtime(void)
{
	drv_rtc_data.bcd[0] = (0 << 7) | ((drv_rtc_data.local.tm_sec / 10) << 4) | (drv_rtc_data.local.tm_sec % 10); //seconds
	drv_rtc_data.bcd[1] = ((drv_rtc_data.local.tm_min / 10) << 4) | (drv_rtc_data.local.tm_min % 10); //minutes
	drv_rtc_data.bcd[2] = (0 << 6) | ((drv_rtc_data.local.tm_hour / 10) << 4) | (drv_rtc_data.local.tm_hour % 10); //hours(24-hr)
	drv_rtc_data.bcd[3] = 1; // sunday
	drv_rtc_data.bcd[4] = ((drv_rtc_data.local.tm_mday / 10) << 4) | (drv_rtc_data.local.tm_mday % 10);
	drv_rtc_data.bcd[5] = (((drv_rtc_data.local.tm_mon + 1) / 10) << 4) | ((drv_rtc_data.local.tm_mon + 1) % 10);
	drv_rtc_data.bcd[6] = (((drv_rtc_data.local.tm_year + (TM_EPOCH - DS1307_EPOCH)) / 10) << 4)
			| ((drv_rtc_data.local.tm_year + (TM_EPOCH - DS1307_EPOCH)) % 10);
}

static void update_clockreg_from_local(void)
{
	drv_rtc_data.clock_reg = RTC_MODE2_CLOCK_YEAR(drv_rtc_data.local.tm_year + (TM_EPOCH - SAM_RTC_EPOCH))
			| RTC_MODE2_CLOCK_MONTH(drv_rtc_data.local.tm_mon + 1)
			| RTC_MODE2_CLOCK_DAY(drv_rtc_data.local.tm_mday)
			| RTC_MODE2_CLOCK_HOUR(drv_rtc_data.local.tm_hour)
			| RTC_MODE2_CLOCK_MINUTE(drv_rtc_data.local.tm_min)
			| RTC_MODE2_CLOCK_SECOND(drv_rtc_data.local.tm_sec);
}

static void update_localtime_from_clockreg(void)
{
	uint32_t clock_reg = drv_rtc_data.clock_reg;
	drv_rtc_data.local.tm_sec = (clock_reg & RTC_MODE2_CLOCK_SECOND_Msk) >> RTC_MODE2_CLOCK_SECOND_Pos;
	drv_rtc_data.local.tm_min = (clock_reg & RTC_MODE2_CLOCK_MINUTE_Msk) >> RTC_MODE2_CLOCK_MINUTE_Pos;
	drv_rtc_data.local.tm_hour = (clock_reg & RTC_MODE2_CLOCK_HOUR_Msk) >> RTC_MODE2_CLOCK_HOUR_Pos;
	drv_rtc_data.local.tm_mday = (clock_reg & RTC_MODE2_CLOCK_DAY_Msk) >> RTC_MODE2_CLOCK_DAY_Pos;
	drv_rtc_data.local.tm_mon = ((clock_reg & RTC_MODE2_CLOCK_MONTH_Msk) >> RTC_MODE2_CLOCK_MONTH_Pos) - 1;
	drv_rtc_data.local.tm_year = ((clock_reg & RTC_MODE2_CLOCK_YEAR_Msk) >> RTC_MODE2_CLOCK_YEAR_Pos) + (SAM_RTC_EPOCH - TM_EPOCH);
	drv_rtc_data.local.tm_wday = 0;
	drv_rtc_data.local.tm_yday = 0;
	drv_rtc_data.local.tm_isdst = 0; /* CST */
}

static void update_localtime_from_lte(void)
{
	drv_rtc_data.local.tm_sec = drv_rtc_data.lte.second;
	drv_rtc_data.local.tm_min = drv_rtc_data.lte.minute;
	drv_rtc_data.local.tm_hour = drv_rtc_data.lte.hour;
	drv_rtc_data.local.tm_mday = drv_rtc_data.lte.day;
	drv_rtc_data.local.tm_mon = drv_rtc_data.lte.month - 1;
	drv_rtc_data.local.tm_year = drv_rtc_data.lte.year + (LTE_EPOCH - TM_EPOCH);
	drv_rtc_data.local.tm_wday = 0;
	drv_rtc_data.local.tm_yday = 0;
	drv_rtc_data.local.tm_isdst = 0; /* CST */
}

static xTaskHandle RTCTaskId;
static void RTCTask()
{
	int r;
	const struct drv_lte_time *req_ltetime;
	// After boot, we want to set the CPU RTC based on the battery-backup RTC
	r = drv_i2c_read_register(DRV_I2C_CHANNEL_RTC, 0x68, 0x00, drv_rtc_data.bcd, 7);

	if (r == 7)
	{
		if (drv_rtc_data.bcd[0] & 0x80)
		{
			// Not yet initialized. Wait for GPS data
			asm("nop\r\n");
		}
		else
		{
			// Good data stored there, copy it over
			update_localtime_from_bcd();
			update_clockreg_from_local();
			RTC_REGS->MODE2.RTC_CLOCK = drv_rtc_data.clock_reg;
			drv_rtc_data.valid = true;
			drv_rtc_data.ds1307_initialized = true;
		}
	}
	// We need to wait for LTE/GPS time.
	// Heuristic: it often takes a while to stabilize so wait before trying. This delay shouldn't
	// be a large cause for concern as this only happens if the battery is changed
	vTaskDelay(30000);
	while (!drv_rtc_data.valid)
	{
		req_ltetime = drv_lte_get_last_time();
		if (req_ltetime) // LTE time has been updated
		{
			//memcpy(&drv_rtc_data.lte, req_ltetime, sizeof(struct drv_lte_time));
			drv_rtc_data.lte = *req_ltetime;
			
			// Check if LTE time makes sense
			if (drv_rtc_data.lte.year + LTE_EPOCH >= 2021 && drv_rtc_data.lte.month != 0 && drv_rtc_data.lte.day != 0)
			{
				// If so then load it 
				update_localtime_from_lte();
				update_clockreg_from_local();
				RTC_REGS->MODE2.RTC_CLOCK = drv_rtc_data.clock_reg;
				drv_rtc_data.valid = true;
			}
		}
		vTaskDelay(1000);
	}
	while (!drv_rtc_data.ds1307_initialized)
	{
		// Then save to battery backup
		update_bcd_from_localtime();
		r = drv_i2c_write_register(DRV_I2C_CHANNEL_RTC, 0x68, 0x00, drv_rtc_data.bcd, 7);
		if (r == 7) drv_rtc_data.ds1307_initialized = true;
		
		vTaskDelay(1000);
	}
	vTaskDelete(NULL);
}

// Called once a second, halfway between CLOCK increments.
void RTC_Handler()
{
	__disable_irq();
	RTC_REGS->MODE2.RTC_INTFLAG = 0xFFFF;
	// Update new time
	drv_rtc_data.clock_reg = RTC_REGS->MODE2.RTC_CLOCK;
	drv_rtc_data.ms_epoch = xTaskGetTickCountFromISR();
	update_localtime_from_clockreg();
	__enable_irq();
}


void drv_rtc_init(void)
{
	// Bring up RTC
	// Use default OSCULP32K at 1.024kHz
	MCLK_REGS->MCLK_APBAMASK |= MCLK_APBAMASK_RTC(1);
	// Clock mode, 1Hz clock, enable read, 24-hour
	RTC_REGS->MODE2.RTC_CTRLA = RTC_MODE2_CTRLA_MODE_CLOCK | RTC_MODE2_CTRLA_PRESCALER_DIV1024
			| RTC_MODE2_CTRLA_CLOCKSYNC(1) | RTC_MODE2_CTRLA_CLKREP(0);
	RTC_REGS->MODE2.RTC_INTENSET = RTC_MODE2_INTENSET_PER7(1);
	NVIC_EnableIRQ(RTC_IRQn);
	while (RTC_REGS->MODE2.RTC_SYNCBUSY);
	RTC_REGS->MODE2.RTC_CTRLA |= RTC_MODE2_CTRLA_ENABLE(1);
	while (RTC_REGS->MODE2.RTC_SYNCBUSY);

	xTaskCreate(RTCTask, "RTC", configMINIMAL_STACK_SIZE + 64, NULL, 3, &RTCTaskId);
}

int drv_rtc_get_ms(void)
{
	return xTaskGetTickCount() - drv_rtc_data.ms_epoch;
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