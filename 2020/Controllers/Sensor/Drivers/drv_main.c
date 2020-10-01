/*
 * drv_main.c
 *
 * Created: 7/11/2020 6:12:01 PM
 *  Author: connor
 */ 

#include "drv_main.h"
#include "drv_gpio.h"
#include "drv_can.h"
#include "drv_adc.h"
#include "sam.h"
#include "system_samc21.h"

/************************************************************************/
/* Initialize all drivers that will be used by this project.            */
/************************************************************************/
void drv_init(void)
{
	// increase flash wait states from 0 to 1. if this is not done, the
	// CPU has a seizure trying to run at 48MHz
	NVMCTRL_REGS->NVMCTRL_CTRLB = NVMCTRL_CTRLB_MANW(1) | NVMCTRL_CTRLB_RWS(1);
	// up speed to 48MHz
	OSCCTRL_REGS->OSCCTRL_OSC48MDIV = OSCCTRL_OSC48MDIV_DIV_DIV1;
	// wait for synchronization
    while (OSCCTRL_REGS->OSCCTRL_OSC48MSYNCBUSY) {}
    SystemCoreClock = 48000000;
	
	drv_gpio_init();
	drv_can_init();
	drv_adc_init();
}

/************************************************************************/
/* Periodic run function for all drivers in this project.               */
/************************************************************************/
void drv_periodic(void)
{
}