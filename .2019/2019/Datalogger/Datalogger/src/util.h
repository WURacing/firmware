/*
 * util.h
 *
 * Created: 4/27/2019 7:42:13 AM
 *  Author: Wash U Racing
 */ 


#ifndef UTIL_H_
#define UTIL_H_

#include <asf.h>
#include <stdint.h>


void configure_usart_cdc(void);
void configure_i2c(void);
void initialize_rtc_calendar(void);
void read_time(struct rtc_calendar_time *ts);
void configure_can(void);


typedef union {
	uint8_t arr[8];
	uint64_t num;
} can_data_t;

typedef struct {
	uint32_t id;
	uint32_t ts;
	can_data_t data;
} can_message_t;

extern volatile can_message_t canline;
extern volatile int canline_updated;

extern struct rtc_module rtc_instance;
extern struct can_module can_instance;


#endif /* UTIL_H_ */