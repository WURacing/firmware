#include "drv_uart.h"
#include "drv_serial.h"
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include "FreeRTOS.h"
#include "task.h"

#define TXBUFLEN 100
#define RXBUFLEN 200
#define LINEBUFLEN 100
#define BAUD 115200

#define TX_DELAY (1 + 1000 / (BAUD / 10 / TXBUFLEN))

struct drv_uart_channelData {
	volatile char rxbuf[RXBUFLEN];
	volatile int rxbufwp, rxbufrp;
	char linebuf[LINEBUFLEN];
	volatile char txbuf[TXBUFLEN];
	volatile int txbufsp, txbufep;
	volatile TaskHandle_t txnotifytask;
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
		config->module->SERCOM_INTENSET = SERCOM_USART_INT_INTENSET_RXC(1) | SERCOM_USART_INT_INTENSET_TXC(1);

		// Start!
		config->module->SERCOM_CTRLA |= SERCOM_USART_INT_CTRLA_ENABLE(1);
		while (config->module->SERCOM_SYNCBUSY) {}

	}
}

enum drv_uart_statusCode drv_uart_send_message(enum drv_uart_channel channel, const char * msg)
{
	return drv_uart_send_data(channel, (const uint8_t *)msg, strlen(msg));
}

enum drv_uart_statusCode drv_uart_send_data(enum drv_uart_channel channel, const uint8_t * msg, unsigned length)
{
	if (channel < DRV_UART_CHANNEL_COUNT)
	{
		struct drv_uart_channelData * data = &channelData[channel];
		sercom_usart_int_registers_t * module = drv_uart_config.channelConfig[channel].module;

		// Load data to be sent by interrupt routines
		memcpy((uint8_t *)data->txbuf, msg, length);
		data->txbufsp = 1;
		data->txbufep = length;
		data->txnotifytask = xTaskGetCurrentTaskHandle();
		
		// Send the first byte to kick off the chain
		module->SERCOM_INTFLAG = 0xF;
		while (!(module->SERCOM_INTFLAG & SERCOM_USART_INT_INTFLAG_DRE_Msk)) {};
		module->SERCOM_DATA = *msg;
		
		// Wait for chain to finish
		uint32_t retval = ulTaskNotifyTake(pdTRUE, TX_DELAY);
		if (retval == 1)
		{
			return DRV_UART_SUCCESS;
		}
		else
		{
			return DRV_UART_TIMEOUT;
		}
	}
	else
	{
		return DRV_UART_ERROR;
	}
}


void drv_uart_clear_response(enum drv_uart_channel channel)
{
	if (channel < DRV_UART_CHANNEL_COUNT)
	{
		struct drv_uart_channelData * data = &channelData[channel];
		
		data->rxbufrp = data->rxbufwp;
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

// Return number of bytes read
int drv_uart_read(enum drv_uart_channel channel, char * buf, int nbyte)
{
	if (channel < DRV_UART_CHANNEL_COUNT)
	{
		struct drv_uart_channelData * data = &channelData[channel];
		
		// Read up to nbyte from FIFO structure into buf
		int bufi = 0;
		while ((data->rxbufrp != data->rxbufwp) && (bufi < nbyte - 1))
		{
			buf[bufi++] = data->rxbuf[data->rxbufrp++];
			if (data->rxbufrp >= RXBUFLEN)
			{
				data->rxbufrp = 0;
			}
		}
		buf[bufi] = '\0'; // null-terminate
		return bufi;
	}
	return -1;
}

static inline int drv_uart_getch(enum drv_uart_channel channel, char * ch)
{
	if (channel < DRV_UART_CHANNEL_COUNT)
	{
		struct drv_uart_channelData * data = &channelData[channel];
		
		// See if a character is available
		int rp = data->rxbufrp;
		if (rp != data->rxbufwp)
		{
			*ch = data->rxbuf[rp++];
			if (rp >= RXBUFLEN)
			{
				rp = 0;
			}
			data->rxbufrp = rp;
			return 1;
		}
		return 0;
	}
	return -1;
}


const char * drv_uart_read_line(enum drv_uart_channel channel, int timeout, const char * termination)
{
	if (channel < DRV_UART_CHANNEL_COUNT)
	{
		struct drv_uart_channelData * data = &channelData[channel];
		
		int li = 0;
		int ret;
		const int tlen = strlen(termination);
		do {
			// Read one character into the line buffer at available position
			ret = drv_uart_getch(channel, data->linebuf + li);
			if (ret == 1)
			{
				// Successful read
				li += 1;
				if (li >= LINEBUFLEN)
				{
					return NULL;
				}
				// Null-terminate and check last tlen characters
				data->linebuf[li] = '\0';
				if (strncmp(data->linebuf + li - tlen, termination, tlen) == 0)
				{
					return data->linebuf;
				}
			}
			else if (ret == 0)
			{
				// No data to read, so wait a tick
				vTaskDelay(1);
				timeout--;
			}
			else
			{
				// Should not happen
				return NULL;
			}
		} while (timeout > 0);
	}
	return NULL;
}

static void rx_handler(int sercom, sercom_registers_t * module)
{
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;
	struct drv_uart_channelData * data = &channelData[sercomToChannelMap[sercom]];
	
	// Check for incoming data
	if (module->USART_INT.SERCOM_INTFLAG & SERCOM_USART_INT_INTFLAG_RXC_Msk)
	{
		uint8_t incoming = module->USART_INT.SERCOM_DATA;
		// TODO: gracefully handle overflow conditions
		int wp = data->rxbufwp;
		data->rxbuf[wp++] = incoming;
		if (wp >= RXBUFLEN)
		{
			wp = 0;
		}
		data->rxbufwp = wp;
	}
	
	// Check if we can send more data
	if (module->USART_INT.SERCOM_INTFLAG & SERCOM_USART_INT_INTFLAG_TXC_Msk)
	{
		int sp = data->txbufsp; int ep = data->txbufep;
		if (sp < ep)
		{
			module->USART_INT.SERCOM_DATA = data->txbuf[sp];
			data->txbufsp = sp + 1;
		}
		else if (sp == ep)
		{
			module->USART_INT.SERCOM_INTFLAG = SERCOM_USART_INT_INTFLAG_TXC(1);
			vTaskNotifyGiveFromISR(data->txnotifytask, &xHigherPriorityTaskWoken);
			data->txnotifytask = NULL;
		}
	}
	
	portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

