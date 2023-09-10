#include "sam.h"
#include "drv_spi.h"

static const struct drv_spi_channelConfig channels[DRV_SPI_CHANNEL_COUNT] = {
	[DRV_SPI_CHANNEL_SD] =
	{
		.sercom_id = 0,
		.module = &SERCOM0_REGS->SPIM, //spi master mode
		.do_pinmux = PINMUX_PA08C_SERCOM0_PAD0, // reference to actual pin
		.di_pinmux = PINMUX_PA09C_SERCOM0_PAD1, // actual pin
		.ss_pin = PIN_PA10, //slave select / chip select
		.ss_port = PORT_PA10,
		.sck_pinmux = PINMUX_PA11C_SERCOM0_PAD3,
		.do_po = 3, // use table in drv_spi.h
		.di_po = 1, // use table in drv_spi.h
		.baud = SERIAL_BAUD_SYNC(10000000),
	}
};

const struct drv_spi_config drv_spi_config = {
	.channelConfig = channels,
};
