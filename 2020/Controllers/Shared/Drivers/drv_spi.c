#include "drv_spi.h"

static void rx_handler(int sercom, sercom_registers_t * module);

void drv_spi_init(void)
{
	for (enum drv_spi_channel channel = (enum drv_spi_channel)0U; channel < DRV_SPI_CHANNEL_COUNT; ++channel)
	{
		const struct drv_spi_channelConfig * config = &drv_spi_config.channelConfig[channel];
		
		// Enable the bus clock, peripheral clock, and interrupts for the chosen SERCOM#
		drv_serial_enable_sercom(config->sercom_id);
		
		// Register the interrupt handler, for interoperability with drv_spi and drv_i2c.
		drv_serial_register_handler(config->sercom_id, rx_handler);
		
		// Set up DI, DO, SCK pins.
		drv_serial_set_pinmux(config->di_pinmux);
		drv_serial_set_pinmux(config->do_pinmux);
		drv_serial_set_pinmux(config->sck_pinmux);
		
		// Set up SS pin. Pull high to disable slave.
		PORT_REGS->GROUP[config->ss_pin / 32].PORT_DIRSET = config->ss_port;
		PORT_REGS->GROUP[config->ss_pin / 32].PORT_OUTSET = config->ss_port;
		
		
		// Disable SERCOM
		config->module->SERCOM_CTRLA = SERCOM_SPIM_CTRLA_ENABLE(0);
		// Wait for synchronization
		while (config->module->SERCOM_SYNCBUSY) {}

		config->module->SERCOM_CTRLA =
				SERCOM_SPIM_CTRLA_MODE_SPI_MASTER |
				SERCOM_SPIM_CTRLA_CPOL_IDLE_LOW | SERCOM_SPIM_CTRLA_CPHA_LEADING_EDGE |
				SERCOM_SPIM_CTRLA_FORM_SPI_FRAME |
				SERCOM_SPIM_CTRLA_DIPO(config->di_po) | SERCOM_SPIM_CTRLA_DOPO(config->do_po) |
				SERCOM_SPIM_CTRLA_DORD_MSB;
		
		config->module->SERCOM_CTRLB =
				SERCOM_SPIM_CTRLB_CHSIZE_8_BIT | SERCOM_SPIM_CTRLB_RXEN(1);
		
		config->module->SERCOM_BAUD = config->baud;
		
		config->module->SERCOM_CTRLA |= SERCOM_SPIM_CTRLA_ENABLE(1);
		while (config->module->SERCOM_SYNCBUSY) {}
	}
}

static void rx_handler(int sercom, sercom_registers_t * module)
{
}

uint8_t drv_spi_transfer(enum drv_spi_channel channel, uint8_t out)
{
	uint8_t result;
	if (channel < DRV_SPI_CHANNEL_COUNT)
	{
		const struct drv_spi_channelConfig * config = &drv_spi_config.channelConfig[channel];
		// Pull low to enable slave. TODO configurable
		PORT_REGS->GROUP[config->ss_pin / 32].PORT_OUTCLR = config->ss_port;
		
		// Wait until we can write
		while (!(config->module->SERCOM_INTFLAG & SERCOM_SPIM_INTFLAG_DRE_Msk)) {}
		// Write
		config->module->SERCOM_DATA = out;
		// Wait until done receiving
		while (!(config->module->SERCOM_INTFLAG & SERCOM_SPIM_INTFLAG_RXC_Msk)) {}
		// Read
		result = config->module->SERCOM_DATA;
		
		// We would then pull high to disable the slave again, only if necessary (multiple per bus)
		//PORT_REGS->GROUP[config->ss_pin / 32].PORT_OUTSET = config->ss_port;
	}
	else
	{
		result = 0xFF;
	}
	return result;
}