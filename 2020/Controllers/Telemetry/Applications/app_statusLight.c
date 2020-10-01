/*
 * app_statusLight.c
 *
 * Created: 7/11/2020 7:22:35 PM
 *  Author: connor
 */ 

#include "app_statusLight.h"
#include "drv_can.h"
#include "FreeRTOS.h"
#include "task.h"
#include "sam.h"

#define ERROR_LED_GROUP		1
#define ERROR_LED_PIN		PIN_PB12
#define ERROR_LED_PORT		PORT_PB12
#define STATUS_LED_GROUP	1
#define STATUS_LED_PIN		PIN_PB13
#define STATUS_LED_PORT		PORT_PB13

#define PERIOD_PER_TICK

int globalError;

static int nextLightPeriod;
static TickType_t nextLightExpiration;

void app_statusLight_init(void)
{
	globalError = 0;
	nextLightExpiration = -1;
	nextLightPeriod = 0;
	
	PORT_REGS->GROUP[ERROR_LED_GROUP].PORT_DIRSET  = ERROR_LED_PORT;
	PORT_REGS->GROUP[STATUS_LED_GROUP].PORT_DIRSET = STATUS_LED_PORT;
}

void app_statusLight_periodic(void)
{
	static int lastError = 7;
	TickType_t time = xTaskGetTickCount();
	
	if (globalError != lastError)
	{
		lastError = globalError;
		if (lastError == 0)
		{
			nextLightExpiration = -1;
			nextLightPeriod = 0;

			PORT_REGS->GROUP[STATUS_LED_GROUP].PORT_OUTSET = STATUS_LED_PORT;
			PORT_REGS->GROUP[ERROR_LED_GROUP].PORT_OUTCLR = ERROR_LED_PORT;
		}
		else
		{
			PORT_REGS->GROUP[ERROR_LED_GROUP].PORT_OUTSET = ERROR_LED_PORT;
			
			nextLightPeriod = 1000 >> (lastError - 1); // period = 1000 / 2^error
			nextLightExpiration = xTaskGetTickCount() + nextLightPeriod;
		}
	}
	
	if (time >= nextLightExpiration)
	{
		nextLightExpiration = time + nextLightPeriod;
		
		PORT_REGS->GROUP[STATUS_LED_GROUP].PORT_OUTTGL = STATUS_LED_PORT;
	}
}
