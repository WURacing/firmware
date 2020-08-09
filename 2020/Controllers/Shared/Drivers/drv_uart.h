#pragma once
#include "sam.h"

void drv_uart_init(void);
void drv_uart_periodic(void);

void drv_uart_clear_response(void);
volatile const char * drv_uart_get_response_buffer(void);