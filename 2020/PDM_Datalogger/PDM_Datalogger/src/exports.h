/*
 * exports.h
 *
 * Created: 1/20/2020 1:43:53 PM
 *  Author: Connor
 */ 


#ifndef EXPORTS_H_
#define EXPORTS_H_

#include <asf.h>
#include <stdint.h>

extern xSemaphoreHandle new_data_semaphore;

void configure_can(void);

struct can_message {
	uint32_t id, ts;
	uint8_t data[8];
};

enum read_message_status {
	READ_ONE,
	READ_NONE,
	READ_ERROR
	};

enum read_message_status read_message(struct can_message *output);

extern xTaskHandle sd_task_id;

void sd_task(void *pvParameters);

#endif /* EXPORTS_H_ */