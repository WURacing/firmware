#include "drv_main.h"
#include "drv_uart.h"
#include "drv_spi.h"
#include "drv_i2c.h"
#include "drv_divas.h"

void drv_init(void)
{
	drv_uart_init();
	drv_spi_init();
	drv_i2c_init();
	drv_divas_init();
	disk_initialize(0);
}

void drv_periodic(void)
{
	
}
