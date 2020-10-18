#include "drv_uart.h"
#include "drv_serial.h"
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

#define RXBUFLEN 200
volatile char rxbuf[RXBUFLEN] = {0};
volatile int rxbufi = 0;

#define BAUD 115200

struct drv_uart_channelData {
	volatile char rxbuf[RXBUFLEN];
	volatile int rxbufi;
};

static struct drv_uart_channelData channelData[DRV_UART_CHANNEL_COUNT];
static enum drv_uart_channel sercomToChannelMap[6];

static void rx_handler(int sercom, sercom_registers_t * module);

void drv_uart_init(void)
{
	for (enum drv_uart_channel channel = (enum drv_uart_channel)0U; channel < DRV_UART_CHANNEL_COUNT; ++channel)
	{
		const struct drv_uart_channelConfig * config = &drv_uart_config.channelConfig[channel];
		
		memset(&channelData[channel], 0, sizeof(struct drv_uart_channelData));
		sercomToChannelMap[config->sercom_id] = channel;
		
		// Enable the bus clock, peripheral clock, and interrupts for the chosen SERCOM#
		drv_serial_enable_sercom(config->sercom_id);
		
		// Register the interrupt handler, for interoperability with drv_spi and drv_i2c.
		drv_serial_register_handler(config->sercom_id, rx_handler);
		
		// Set up TX and RX pins.
		drv_serial_set_pinmux(config->tx_pinmux);
		drv_serial_set_pinmux(config->rx_pinmux);
		
		// Disable SERCOM
		config->module->SERCOM_CTRLA = SERCOM_USART_INT_CTRLA_ENABLE(0);
		// Wait for synchronization
		while (config->module->SERCOM_SYNCBUSY) {}

		// Default UART settings (async, no LIN, LSB first, no parity)
		config->module->SERCOM_CTRLA =
				SERCOM_USART_INT_CTRLA_DORD_LSB | SERCOM_USART_INT_CTRLA_CMODE_ASYNC |
				SERCOM_USART_INT_CTRLA_FORM_USART_FRAME_NO_PARITY | SERCOM_USART_INT_CTRLA_MODE_USART_INT_CLK |
				config->tx_pad | config->rx_pad;

		// Default UART settings (enable sending/receiving, 1 stop bit, 8 data bits)
		config->module->SERCOM_CTRLB =
				SERCOM_USART_INT_CTRLB_RXEN(1) | SERCOM_USART_INT_CTRLB_TXEN(1) |
				SERCOM_USART_INT_CTRLB_ENC(0) | SERCOM_USART_INT_CTRLB_SFDE(0) |
				SERCOM_USART_INT_CTRLB_SBMODE_1_BIT | SERCOM_USART_INT_CTRLB_CHSIZE_8_BIT;

		// Set baud rate
		config->module->SERCOM_BAUD = config->baud;  

		// Enable RX interrupt
		config->module->SERCOM_INTENSET = SERCOM_USART_INT_INTENSET_RXC(1);

		// Start!
		config->module->SERCOM_CTRLA |= SERCOM_USART_INT_CTRLA_ENABLE(1);
		while (config->module->SERCOM_SYNCBUSY) {}

	}
}

void drv_uart_periodic(void)
{
}

void drv_uart_send_message(enum drv_uart_channel channel, const char * msg)
{
	if (channel < DRV_UART_CHANNEL_COUNT)
	{
		sercom_usart_int_registers_t * module = drv_uart_config.channelConfig[channel].module;
		// Clear existing errors
		module->SERCOM_INTFLAG = SERCOM_USART_INT_INTFLAG_ERROR(1);
		while (*msg)
		{
			// Wait for data buffer to empty
			while (!(module->SERCOM_INTFLAG & SERCOM_USART_INT_INTFLAG_DRE_Msk)) {};
			module->SERCOM_DATA = *(msg++);
			// Wait for transmit complete
			while (!(module->SERCOM_INTFLAG & SERCOM_USART_INT_INTFLAG_TXC_Msk)) {};
		}
		// Error checking not included because I can't rationalize what errors we could possibly
		// have (especially because we're not using CTS/DTS/etc)
	}
}

void drv_uart_send_data(enum drv_uart_channel channel, const uint8_t * msg, unsigned length)
{
	if (channel < DRV_UART_CHANNEL_COUNT)
	{
		sercom_usart_int_registers_t * module = drv_uart_config.channelConfig[channel].module;
		const uint8_t * target = msg + length;
		while (msg < target)
		{
			// Wait for data buffer to empty
			while (!(module->SERCOM_INTFLAG & SERCOM_USART_INT_INTFLAG_DRE_Msk)) {};
			module->SERCOM_DATA = *(msg++);
			// Wait for transmit complete
			while (!(module->SERCOM_INTFLAG & SERCOM_USART_INT_INTFLAG_TXC_Msk)) {};
		}
	}
}


void drv_uart_clear_response(enum drv_uart_channel channel)
{
	if (channel < DRV_UART_CHANNEL_COUNT)
	{
		__disable_irq();
		channelData[channel].rxbufi = 0;
		memset((char *)channelData[channel].rxbuf, 0, RXBUFLEN);
		__enable_irq();
	}
}

const char * drv_uart_get_response_buffer(enum drv_uart_channel channel)
{
	if (channel < DRV_UART_CHANNEL_COUNT)
	{
		// locally, it's not volatile
		return (const char *)channelData[channel].rxbuf;
	}
	else
	{
		return NULL;
	}
}

static void rx_handler(int sercom, sercom_registers_t * module)
{
	uint8_t incoming = module->USART_INT.SERCOM_DATA;
	struct drv_uart_channelData * data = &channelData[sercomToChannelMap[sercom]];
	if (data->rxbufi < RXBUFLEN)
	{
		data->rxbuf[data->rxbufi++] = incoming;
	}
}

