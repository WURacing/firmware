#include "drv_i2c.h"

static void rx_handler(int sercom, sercom_registers_t * module);

void drv_i2c_init(void)
{
	for (enum drv_i2c_channel channel = (enum drv_i2c_channel)0U; channel < DRV_I2C_CHANNEL_COUNT; ++channel)
	{
		const struct drv_i2c_channelConfig * config = &drv_i2c_config.channelConfig[channel];

		// Enable the bus clock, peripheral clock, and interrupts for the chosen SERCOM#
		drv_serial_enable_sercom(config->sercom_id);

		// Register the interrupt handler, for interoperability with drv_spi and drv_i2c.
		drv_serial_register_handler(config->sercom_id, rx_handler);

		// Set up SDA, SCL pins.
		drv_serial_set_pinmux(config->sda_pinmux);
		drv_serial_set_pinmux(config->scl_pinmux);

		// Disable SERCOM
		config->module->SERCOM_CTRLA = SERCOM_I2CM_CTRLA_ENABLE(0);
		// Wait for synchronization
		while (config->module->SERCOM_SYNCBUSY)
		{
		}

		config->module->SERCOM_CTRLA =
				SERCOM_I2CM_CTRLA_MODE_I2C_MASTER | SERCOM_I2CM_CTRLA_INACTOUT_55US;

		config->module->SERCOM_BAUD = config->baud;

		config->module->SERCOM_CTRLA |= SERCOM_I2CM_CTRLA_ENABLE(1);
		while (config->module->SERCOM_SYNCBUSY)
		{
		}
	}
}

static void rx_handler(int sercom, sercom_registers_t * module)
{
}

int drv_i2c_read_register(enum drv_i2c_channel channel, uint8_t address, uint8_t pointer, uint8_t results[], int length)
{
	sercom_i2cm_registers_t * module = drv_i2c_config.channelConfig[channel].module;
	
	// send address on bus, write mode
	module->SERCOM_ADDR = SERCOM_I2CM_ADDR_ADDR((address << 1) | 0);
	// wait
	while (!(module->SERCOM_INTFLAG & SERCOM_I2CM_INTFLAG_MB_Msk))
	{
	}
	// check errors
	if (module->SERCOM_INTFLAG & (SERCOM_I2CM_INTFLAG_ERROR(1)))
	{
		module->SERCOM_CTRLB = SERCOM_I2CM_CTRLB_CMD(3);
		module->SERCOM_INTFLAG = 0xFF;
		return 0;
	}
	// send pointer (to register in device) on bus
	module->SERCOM_DATA = SERCOM_I2CM_DATA_DATA(pointer);
	// wait
	while (!(module->SERCOM_INTFLAG & SERCOM_I2CM_INTFLAG_MB_Msk))
	{
	}
	// check errors
	if (module->SERCOM_INTFLAG & (SERCOM_I2CM_INTFLAG_ERROR(1)))
	{
		module->SERCOM_CTRLB = SERCOM_I2CM_CTRLB_CMD(3);
		module->SERCOM_INTFLAG = 0xFF;
		return 0;
	}
	// send a repeated start condition on bus, same address but read mode
	module->SERCOM_ADDR = SERCOM_I2CM_ADDR_ADDR((address << 1) | 1);

	// read all but the last byte
	for (int i = 0; i < length - 1; ++i)
	{
		// wait
		while (!(module->SERCOM_INTFLAG & SERCOM_I2CM_INTFLAG_SB_Msk))
		{
		}
		// check errors
		if (module->SERCOM_INTFLAG & (SERCOM_I2CM_INTFLAG_ERROR(1)))
		{
			module->SERCOM_CTRLB = SERCOM_I2CM_CTRLB_CMD(3);
			module->SERCOM_INTFLAG = 0xFF;
			return i;
		}
		// store received byte
		results[i] = module->SERCOM_DATA;
		// send ACK on bus (ask for more data)
		module->SERCOM_CTRLB = SERCOM_I2CM_CTRLB_CMD(2);
	}

	// wait
	while (!(module->SERCOM_INTFLAG & SERCOM_I2CM_INTFLAG_SB_Msk))
	{
	}
	// read last byte
	results[length - 1] = module->SERCOM_DATA;
	// send NAK / stop condition on bus
	module->SERCOM_CTRLB = SERCOM_I2CM_CTRLB_CMD(3) | SERCOM_I2CM_CTRLB_ACKACT(1);
	return length;
}

int drv_i2c_write_register(enum drv_i2c_channel channel, uint8_t address, uint8_t pointer, const uint8_t command[], int length)
{
	sercom_i2cm_registers_t * module = drv_i2c_config.channelConfig[channel].module;
	
	// send address on bus, write mode
	module->SERCOM_ADDR = SERCOM_I2CM_ADDR_ADDR((address << 1) | 0);
	// wait
	while (!(module->SERCOM_INTFLAG & SERCOM_I2CM_INTFLAG_MB_Msk))
	{
	}
	// check errors
	if (module->SERCOM_INTFLAG & (SERCOM_I2CM_INTFLAG_ERROR(1)))
	{
		module->SERCOM_CTRLB = SERCOM_I2CM_CTRLB_CMD(3);
		module->SERCOM_INTFLAG = 0xFF;
		return 0;
	}
	// send pointer (to register in device) on bus
	module->SERCOM_DATA = SERCOM_I2CM_DATA_DATA(pointer);

	// write all bytes
	for (int i = 0; i < length; ++i)
	{
		// wait
		while (!(module->SERCOM_INTFLAG & SERCOM_I2CM_INTFLAG_MB_Msk))
		{
		}
		// check errors
		if (module->SERCOM_INTFLAG & (SERCOM_I2CM_INTFLAG_ERROR(1)))
		{
			module->SERCOM_CTRLB = SERCOM_I2CM_CTRLB_CMD(3);
			module->SERCOM_INTFLAG = 0xFF;
			return i;
		}
		// write
		module->SERCOM_DATA = SERCOM_I2CM_DATA_DATA(command[i]);
	}

	// wait
	while (!(module->SERCOM_INTFLAG & SERCOM_I2CM_INTFLAG_MB_Msk))
	{
	}
	// send stop condition
	module->SERCOM_CTRLB = SERCOM_I2CM_CTRLB_CMD(3);
	module->SERCOM_INTFLAG = 0xFF;
	return length;
}