#pragma once
#include "drv_uart_private.h"
#include "drv_serial.h"
#include "sam.h"

void drv_uart_init(void);

void drv_uart_clear_response(enum drv_uart_channel channel);
const char * drv_uart_get_response_buffer(enum drv_uart_channel channel);

enum drv_uart_statusCode drv_uart_send_message(enum drv_uart_channel channel, const char * msg);
enum drv_uart_statusCode drv_uart_send_data(enum drv_uart_channel channel, const uint8_t * msg, unsigned length);

const char * drv_uart_read_line(enum drv_uart_channel channel, int timeout, const char * termination);

enum drv_uart_statusCode {
	DRV_UART_SUCCESS = 0,
	DRV_UART_ERROR,
	DRV_UART_TIMEOUT,
};

struct drv_uart_channelConfig {
	int sercom_id;
	sercom_usart_int_registers_t * module;
	unsigned rx_pinmux, tx_pinmux, rx_pad, tx_pad;
	unsigned baud;
};

struct drv_uart_config {
	const struct drv_uart_channelConfig * channelConfig;
};
extern const struct drv_uart_config drv_uart_config;