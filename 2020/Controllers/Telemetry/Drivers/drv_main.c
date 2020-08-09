/*
 * drv_main.c
 *
 * Connects all enabled drivers in this project.
 */ 

#include "drv_main.h"
#include "drv_uart.h"
#include "drv_can.h"
#include "drv_uart.h"
#include "sam.h"

void drv_init(void)
{
	// increase flash wait states from 0 to 1. if this is not done, the
	// CPU has a seizure trying to run at 48MHz
	NVMCTRL->CTRLB.bit.RWS = 1;
	// up speed to 48MHz
	OSCCTRL->OSC48MDIV.reg = OSCCTRL_OSC48MDIV_DIV(0b0000);
	// wait for synchronization
	while(OSCCTRL->OSC48MSYNCBUSY.reg) ;

	drv_can_init();
	drv_uart_init();
	drv_lte_init();
}

void drv_periodic(void)
{
	drv_uart_periodic();
	drv_lte_periodic();
}
