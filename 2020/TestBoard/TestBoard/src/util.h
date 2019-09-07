#ifndef UTIL_H_
#define UTIL_H_

#include <asf.h>
#include <stdint.h>

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

extern volatile int canline_0_updated;

extern volatile int canline_1_updated;

extern struct can_module can_instance;


#endif /* UTIL_H_ */