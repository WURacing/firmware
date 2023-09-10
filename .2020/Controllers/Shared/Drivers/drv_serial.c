#include "drv_serial.h"

/*
 * Serial handlers route from the appropriate interrupt, to the appropriate function in the higher
 * level interface (i2c/spi/uart)
 */

static drv_serial_handler_t handlers[6];

void drv_serial_register_handler(int sercom, drv_serial_handler_t handler)
{
	handlers[sercom] = handler;
}

void SERCOM0_Handler(void)
{
	handlers[0](0, SERCOM0_REGS);
}
void SERCOM1_Handler(void)
{
	handlers[1](1, SERCOM1_REGS);
}
void SERCOM2_Handler(void)
{
	handlers[2](2, SERCOM2_REGS);
}
void SERCOM3_Handler(void)
{
	handlers[3](3, SERCOM3_REGS);
}
void SERCOM4_Handler(void)
{
	handlers[4](4, SERCOM5_REGS);
}
void SERCOM5_Handler(void)
{
	handlers[5](5, SERCOM5_REGS);
}