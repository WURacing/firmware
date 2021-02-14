#include "drv_main.h"
#include "drv_uart.h"
#include "drv_spi.h"
#include "drv_i2c.h"
#include "drv_divas.h"
#include "drv_sd.h"

static volatile char buff[2048];

void drv_init(void)
{
	drv_uart_init();
	drv_spi_init();
	drv_i2c_init();
	drv_divas_init();
	disk_initialize(0);
	volatile DRESULT result1, result2;
	result2 = disk_read(0, buff+512, 1, 2);
	result1 = disk_read(0, buff, 0, 1);
	asm volatile("nop\r\n");
}

void drv_periodic(void)
{
	
}
