#include "drv_spi.h"

static const struct drv_spi_channelConfig channels[DRV_SPI_CHANNEL_COUNT] =
{
#if 0
	[DRV_SPI_CHANNEL_TEST] = {
		.sercom_id = 0,
		.module = &SERCOM0_REGS->SPIM,
		.do_pinmux = PINMUX_PA08C_SERCOM0_PAD0,
		.di_pinmux = PINMUX_PA09C_SERCOM0_PAD1,
		.ss_pin = PIN_PA10,
		.ss_port = PORT_PA10,
		.sck_pinmux = PINMUX_PA11C_SERCOM0_PAD3,
		.do_po = 3,
		.di_po = 1,
		.baud = SERIAL_BAUD_SYNC(400000)
	}
#endif

//TODO CHANGE VALUES
[DRV_SPI_CHANNEL_SD] = {
	.sercom_id = 5,
	.module = &SERCOM5_REGS->SPIM, //spi master mode
	.do_pinmux = PINMUX_PB16C_SERCOM5_PAD0, // reference to actual pin
	.di_pinmux = PINMUX_PB17C_SERCOM5_PAD1, // actual pin
	.ss_pin = PIN_PA20, //slave select / chip select
	.ss_port = PORT_PA20,
	.sck_pinmux = PINMUX_PA21C_SERCOM5_PAD3,
	.do_po = 3, // use table in drv_spi.h
	.di_po = 1, // use table in drv_spi.h
	.baud = SERIAL_BAUD_SYNC(1000000), //I'll need to set from research probably
}
};

const struct drv_spi_config drv_spi_config = {
	.channelConfig = channels,
};
