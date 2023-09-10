/*
 * drv_main.c
 *
 * Connects all enabled drivers in this project.
 */ 

#include "drv_main.h"
#include "drv_uart.h"
#include "drv_can.h"
#include "drv_uart.h"
#include "drv_divas.h"
#include "drv_lte.h"
#include "sam.h"
#include "system_samc21.h"

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

	drv_divas_init();
	drv_can_init();
	drv_uart_init();
	drv_lte_init();
}

void drv_periodic(void)
{
	drv_lte_periodic();
}
