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

static int flashesRemaining = 0;
static TickType_t nextFlash = 0;
static int lastError = 0;
static int lastTxbto = 0;
static TickType_t lastTransmission = 0;

void app_statusLight_init(void)
{	
	PORT->Group[1].DIRSET.reg = PORT_PB16 | PORT_PB17;
}

void app_statusLight_periodic(void)
{
	TickType_t time = xTaskGetTickCount();
	int error = CAN1->PSR.bit.LEC;
	if (error == 7) //  no change
	{
		error = lastError;
	}
	else
	{
		lastError = error;
	}
	
	if (nextFlash < time)
	{
		if (flashesRemaining > 0)
		{
			if (--flashesRemaining == 0)
			{
				nextFlash = time + 2000;
				
				PORT->Group[1].OUTCLR.reg = PORT_PB17;
			}
			else
			{
				nextFlash = time + 200;
				
				PORT->Group[1].OUTTGL.reg = PORT_PB17;
			}
		}
		else if (error != 0)
		{
			flashesRemaining = error * 2 + 1;
			PORT->Group[1].OUTCLR.reg = PORT_PB16;
		}
		else
		{
			PORT->Group[1].OUTTGL.reg = PORT_PB16;
		}
	}
	
	int txbto = CAN1->TXBTO.reg;
	if (txbto)
	{
		lastTransmission = time;
		if (nextFlash < time)
		{
			nextFlash = time + 20;
		}
	}
	lastTxbto = txbto;
}
