#include "drv_uart_private.h"
#include "drv_uart.h"

static const struct drv_uart_channelConfig channels[DRV_UART_CHANNEL_COUNT] =
{
	[DRV_UART_CHANNEL_LTE] = {
		.sercom_id = 0,
		.module = &SERCOM0_REGS->USART_INT,
		.tx_pinmux = PINMUX_PA08C_SERCOM0_PAD0,
		.tx_pad = SERCOM_USART_INT_CTRLA_TXPO_PAD0,
		.rx_pinmux = PINMUX_PA09C_SERCOM0_PAD1,
		.rx_pad = SERCOM_USART_INT_CTRLA_RXPO_PAD1,
		.baud = SERIAL_BAUD_ASYNC(115200)
	}
};

const struct drv_uart_config drv_uart_config = {
	.channelConfig = channels,
};