#include "drv_main.h"
#include "drv_uart.h"
#include "drv_spi.h"
#include "drv_i2c.h"
#include "drv_divas.h"
#include "drv_sd.h"
#include "drv_rtc.h"


void drv_init(void)
{
	drv_uart_init();
	drv_spi_init();
	drv_i2c_init();
	drv_divas_init();
	drv_sd_init();
	drv_rtc_init();
}

void drv_periodic(void)
{
	drv_sd_periodic();
	drv_rtc_periodic();
}
