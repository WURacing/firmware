/*
 * usart.c
 *
 * Created: 4/27/2019 5:58:31 PM
 *  Author: Connor
 */ 

#include <asf.h>
#include "util.h"

void usart_read_callback(struct usart_module *const usart_module);
void usart_write_callback(struct usart_module *const usart_module);

struct usart_module usart_instance;

void configure_usart_cdc(void)
{
	struct usart_config config_cdc;
	usart_get_config_defaults(&config_cdc);
	config_cdc.baudrate  = USART_BAUD;
	config_cdc.mux_setting = USART_MUX_SETTING;
	config_cdc.pinmux_pad0 = USART_PINMUX_PAD0;
	config_cdc.pinmux_pad1 = USART_PINMUX_PAD1;
	config_cdc.pinmux_pad2 = USART_PINMUX_PAD2;
	config_cdc.pinmux_pad3 = USART_PINMUX_PAD3;
	stdio_serial_init(&usart_instance, USART_MODULE, &config_cdc);
	usart_enable(&usart_instance);
}

// Callbacks (not used directly due to stdio_serial)

void usart_read_callback(struct usart_module *const usart_module)
{
}

void usart_write_callback(struct usart_module *const usart_module)
{
}
