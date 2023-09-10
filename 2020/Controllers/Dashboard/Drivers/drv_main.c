#include "drv_main.h"
#include "drv_i2c.h"
#include "drv_can.h"

void drv_init(void)
{
	drv_i2c_init();
    drv_can_init();
}

void drv_periodic(void)
{
	
}
