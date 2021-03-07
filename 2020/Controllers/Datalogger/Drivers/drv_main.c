#include "drv_main.h"
#include "drv_uart.h"
#include "drv_spi.h"
#include "drv_i2c.h"
#include "drv_divas.h"
#include "drv_sd.h"

static volatile char buff_write[2048];
static volatile char buff_read[2048];

void drv_init(void)
{
	for (int i = 0; i < 2048; i++) {
		buff_write[i] = 0x47;
	}
	
	drv_uart_init();
	drv_spi_init();
	drv_i2c_init();
	drv_divas_init();
	disk_initialize(0);
	volatile DRESULT result1, result2;
	disk_write(0, buff_write, 2, 4);
	result1 = disk_read(0, buff_read, 2, 4);
	asm volatile("nop\r\n");
}

void drv_periodic(void)
{
	
}
