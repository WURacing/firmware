#ifndef UTIL_H_
#define UTIL_H_

#include <asf.h>
#include <stdint.h>

#define TMP_PKT_ID 0x1f00aa0

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

extern volatile int canline_0_updated;

extern volatile int canline_1_updated;

extern volatile can_message_t canline[64];

extern volatile int canline_i;

extern struct can_module can_instance;

extern volatile int pkt_count;

void configure_i2c(void);

int configure_tmp75a(void);

uint16_t read_tmp75a(void);


#endif /* UTIL_H_ */