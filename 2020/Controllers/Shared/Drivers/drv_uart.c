#include "drv_uart.h"

#define RXBUFLEN 200
char rxbuf[RXBUFLEN] = {0};
int rxbufi = 0;

void drv_uart_init(void)
{
	// set up MCLK APB for SERCOM (synchronized peripheral clock)
	MCLK->APBCMASK.bit.SERCOM0_ = 1;
		
	{ // set up peripheral clocks
		GCLK_PCHCTRL_Type pclock = {
			.bit = {
				.CHEN = 1,
				.GEN = 0 // todo can we run this at 48MHz?
			}
		};
		GCLK->PCHCTRL[SERCOM0_GCLK_ID_CORE] = pclock;
	}
	
	// TODO  interrupts, DMA
	
	SERCOM0->USART.CTRLA.bit.ENABLE = 0;
	while (SERCOM0->USART.SYNCBUSY.reg) {};
		
	{
		SERCOM_USART_CTRLA_Type ctrla = {
			.bit = {
				.DORD = 1, // LSB first
				.CMODE = 0, // async only
				.FORM = 0, // uart (no parity, no LIN)
				.RXPO = 1, // rx = 1
				.TXPO = 0, // tx = 0, xck = 1 (disabled)
				.MODE = 1, // internal clock
			}
		};
		SERCOM0->USART.CTRLA.reg = ctrla.reg;
	}
	{
		SERCOM_USART_CTRLB_Type ctrlb = {
			.bit = {
				.RXEN = 1,
				.TXEN = 1,
				.ENC = 0,
				.SFDE = 0,
				.SBMODE = 0, // 1 stop bit
				.CHSIZE = 0, // 8 bit
			}
		};
		SERCOM0->USART.CTRLB.reg = ctrlb.reg;
	}
	
	SERCOM0->USART.BAUD.bit.BAUD = 63019;
	
	{ // Enable RX interrupt
		SERCOM_USART_INTENSET_Type intenset = {
			.bit = {
				.RXC = 1,
			}
		};
		SERCOM0->USART.INTENSET.reg = intenset.reg;
	}
	
	SERCOM0->USART.CTRLA.bit.ENABLE = 1;
	// Enable interrupt
	NVIC->ISER[0] = (1 << 9); 
}

void drv_uart_periodic(void)
{
	
}

void drv_uart_send_message(const char * msg)
{
	// Clear existing errors
	SERCOM0->USART.INTFLAG.reg = SERCOM_USART_INTFLAG_ERROR;
	while (msg)
	{
		// Wait til we good to write more
		while (! SERCOM0->USART.INTFLAG.bit.DRE) {};
		SERCOM0->USART.DATA.reg = *(msg++);
		// maybe wait until transmit complete
	}
	// maybe check errors HAHA
}

void SERCOM0_Handler(void)
{
	// New byte received
	if (rxbufi >= RXBUFLEN) return;
	rxbuf[rxbufi++] = SERCOM0->USART.DATA.reg;
}