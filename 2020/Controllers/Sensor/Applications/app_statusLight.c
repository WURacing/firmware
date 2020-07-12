/*
 * app_statusLight.c
 *
 * Created: 7/11/2020 7:22:35 PM
 *  Author: connor
 */ 

#include "app_statusLight.h"
#include "drv_gpio.h"
#include "drv_can.h"
#include "FreeRTOS.h"
#include "task.h"

#define PERIOD_PER_TICK

int globalError;

static int nextLightPeriod;
static TickType_t nextLightExpiration;

void app_statusLight_init(void)
{
	globalError = 0;
	nextLightExpiration = -1;
	nextLightPeriod = 0;
}

void app_statusLight_periodic(void)
{
	static int lastError;
	TickType_t time = xTaskGetTickCount();
	
	if (globalError != lastError)
	{
		lastError = globalError;
		if (lastError == 0)
		{
			nextLightExpiration = -1;
			nextLightPeriod = 0;
			drv_gpio_setOutput(DRV_GPIO_PIN_LED, true);
		}
		else
		{
			nextLightPeriod = 1000 >> (lastError - 1); // period = 1000 / 2^error
			nextLightExpiration = xTaskGetTickCount() + nextLightPeriod;
		}
	}
	
	if (time >= nextLightExpiration)
	{
		nextLightExpiration = time + nextLightPeriod;
		
		drv_gpio_toggle(DRV_GPIO_PIN_LED);
	}
	
	if (time % 2000 == 0)
	{
		globalError += 1;
	}
}
