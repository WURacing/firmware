/* 
 * File:   app_data.h
 * Author: Connor
 *
 * Created on May 1, 2021, 1:35 PM
 */

#ifndef APP_DATA_H
#define	APP_DATA_H

#include <stdbool.h>
#include <stdint.h>
#include "drv_can.h"
#include "FreeRTOS.h"


struct app_data_message {
	int id;
	TickType_t timestamp_ms;
	uint8_t data[8];
};

void app_data_init(void);

bool app_data_is_missing(int frame_id);
bool app_data_read_message(int frame_id, struct app_data_message * output);
const struct app_data_message * app_data_pop_fifo(void);
void app_data_push_fifo(const struct drv_can_tx_buffer_element * element);
bool app_data_read_buffer(int i, struct app_data_message * output);


#endif	/* APP_DATA_H */

