#pragma once
#include "sam.h"

void drv_uart_init(void);
void drv_uart_periodic(void);

void drv_uart_clear_response(void);
volatile const char * drv_uart_get_response_buffer(void);

void drv_uart_send_message(const char * msg);
void drv_uart_send_data(const uint8_t * msg, unsigned length);