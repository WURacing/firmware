#include <drv_clock.h>
#include <drv_adc.h>
#include <drv_can.h>
#include <drv_i2c.h>
#include <drv_spi.h>
#include <drv_uart.h>
#include <drv_divas.h>
#include "drv_lte.h"
#include "drv_rtc.h"
#include <drv_sd.h>

// Warning: this is not running in an RTOS task
void drv_init(void)
{
	drv_clock_init();
	drv_divas_init();
	drv_adc_init();
	drv_can_init();
	drv_i2c_init();
	drv_spi_init();
	drv_uart_init();
	drv_rtc_init();
	drv_lte_init();
	drv_sd_init();
}