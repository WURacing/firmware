#include <drv_clock.h>
#include <drv_adc.h>
#include <drv_can.h>
#include <drv_ws2812b.h>
#include "drv_hsd.h"
#include "config.h"

#define SWDCLK_PIN PIN_PA30

// Warning: this is not running in an RTOS task
void drv_init(void)
{
#if (BOARD_ID == EL_04002_22)
	// Mitigate SWCLK errata
	PORT_REGS->GROUP[0].PORT_PINCFG[SWDCLK_PIN] = PORT_PINCFG_PULLEN(1) | PORT_PINCFG_PMUXEN(1);
#endif
	drv_clock_init();
	drv_adc_init();
	drv_can_init();
	drv_ws2812b_init();
	drv_hsd_init();
}