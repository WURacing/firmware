#include "drv_uart.h"
#include <stdbool.h>
#include <string.h>

#define RXBUFLEN 200
volatile char rxbuf[RXBUFLEN] = {0};
volatile int rxbufi = 0;

#define BAUD 115200
#define CLOCK 48000000

void drv_uart_init(void)
{
	// set up MCLK APB for SERCOM (synchronized peripheral clock)
	MCLK_REGS->MCLK_APBCMASK |= MCLK_APBCMASK_SERCOM0(1);
		
	// set up peripheral clocks
	GCLK_REGS->GCLK_PCHCTRL[19] = GCLK_PCHCTRL_CHEN(1) | GCLK_PCHCTRL_GEN_GCLK0;
	
	// set up pins
	PORT_REGS->GROUP[0].PORT_PMUX[PIN_PA08] =
			PORT_PMUX_PMUXE(MUX_PA08C_SERCOM0_PAD0) | PORT_PMUX_PMUXO(MUX_PA09C_SERCOM0_PAD1);
	PORT_REGS->GROUP[0].PORT_PINCFG[PIN_PA08] = PORT_PINCFG_PMUXEN(1);
	PORT_REGS->GROUP[0].PORT_PINCFG[PIN_PA09] = PORT_PINCFG_PMUXEN(1);
	
	// Enable interrupt
	NVIC_EnableIRQ(SERCOM0_IRQn);
	
	SERCOM0_REGS->USART_INT.SERCOM_CTRLA = SERCOM_USART_INT_CTRLA_ENABLE(0);
	while (SERCOM0_REGS->USART_INT.SERCOM_SYNCBUSY) {}

	SERCOM0_REGS->USART_INT.SERCOM_CTRLA =
			SERCOM_USART_INT_CTRLA_DORD_LSB | SERCOM_USART_INT_CTRLA_CMODE_ASYNC |
			SERCOM_USART_INT_CTRLA_FORM_USART_FRAME_NO_PARITY | SERCOM_USART_INT_CTRLA_RXPO_PAD1 |
			SERCOM_USART_INT_CTRLA_TXPO_PAD0 | SERCOM_USART_INT_CTRLA_MODE_USART_INT_CLK;
	
	SERCOM0_REGS->USART_INT.SERCOM_CTRLB =
			SERCOM_USART_INT_CTRLB_RXEN(1) | SERCOM_USART_INT_CTRLB_TXEN(1) |
			SERCOM_USART_INT_CTRLB_ENC(0) | SERCOM_USART_INT_CTRLB_SFDE(0) |
			SERCOM_USART_INT_CTRLB_SBMODE_1_BIT | SERCOM_USART_INT_CTRLB_CHSIZE_8_BIT;
	
	// Set baud rate
	SERCOM0_REGS->USART_INT.SERCOM_BAUD = (int) 65535*(1-16*(((float)BAUD)/CLOCK)); // 115200, see datasheet page 489
	
	// Enable RX interrupt
	SERCOM0_REGS->USART_INT.SERCOM_INTENSET = SERCOM_USART_INT_INTENSET_RXC(1);
	
	// Start!
	SERCOM0_REGS->USART_INT.SERCOM_CTRLA |= SERCOM_USART_INT_CTRLA_ENABLE(1);
	while (SERCOM0_REGS->USART_INT.SERCOM_SYNCBUSY) {}
}

void drv_uart_periodic(void)
{
}

void drv_uart_send_message(const char * msg)
{
	// Clear existing errors
	SERCOM0_REGS->USART_INT.SERCOM_INTFLAG = SERCOM_USART_INT_INTFLAG_ERROR(1);
	while (*msg)
	{
		// Wait til we good to write more
		while (!(SERCOM0_REGS->USART_INT.SERCOM_INTFLAG & SERCOM_USART_INT_INTFLAG_DRE_Msk)) {};
		SERCOM0_REGS->USART_INT.SERCOM_DATA = *(msg++);
		// maybe wait until transmit complete
		while (!(SERCOM0_REGS->USART_INT.SERCOM_INTFLAG & SERCOM_USART_INT_INTFLAG_TXC_Msk)) {};
	}
	// maybe check errors HAHA
}

void drv_uart_send_data(const uint8_t * msg, unsigned length)
{
	const uint8_t * target = msg + length;
	while (msg < target)
	{
		// Wait til we good to write more
		while (!(SERCOM0_REGS->USART_INT.SERCOM_INTFLAG & SERCOM_USART_INT_INTFLAG_DRE_Msk)) {};
		SERCOM0_REGS->USART_INT.SERCOM_DATA = *(msg++);
		// maybe wait until transmit complete
		while (!(SERCOM0_REGS->USART_INT.SERCOM_INTFLAG & SERCOM_USART_INT_INTFLAG_TXC_Msk)) {};
	}
}


void drv_uart_clear_response(void)
{
	__disable_irq();
	rxbufi = 0;
	memset((char *)rxbuf, 0, RXBUFLEN);
	__enable_irq();
}

const char * drv_uart_get_response_buffer(void)
{
	// locally, it's not volatile
	return (const char *)rxbuf;
}

void SERCOM0_Handler(void)
{
	// New byte received
	uint8_t data = SERCOM0_REGS->USART_INT.SERCOM_DATA;
	if (rxbufi >= RXBUFLEN) return;
	rxbuf[rxbufi++] = data;
}