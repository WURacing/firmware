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

};

const struct drv_spi_config drv_spi_config = {
	.channelConfig = channels,
};
