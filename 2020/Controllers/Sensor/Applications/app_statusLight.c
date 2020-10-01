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
	PORT_REGS->GROUP[1].PORT_DIRSET = PORT_PB16 | PORT_PB17;
}

void app_statusLight_periodic(void)
{
	TickType_t time = xTaskGetTickCount();
	int error = CAN1_REGS->CAN_PSR & CAN_PSR_LEC_Msk;
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
				
				PORT_REGS->GROUP[1].PORT_OUTCLR = PORT_PB17;
			}
			else
			{
				nextFlash = time + 200;
				
				PORT_REGS->GROUP[1].PORT_OUTTGL = PORT_PB17;
			}
		}
		else if (error != 0)
		{
			flashesRemaining = error * 2 + 1;
			PORT_REGS->GROUP[1].PORT_OUTCLR = PORT_PB16;
		}
		else
		{
			PORT_REGS->GROUP[1].PORT_OUTTGL = PORT_PB16;
		}
	}
	
	int txbto = CAN1_REGS->CAN_TXBTO;
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
