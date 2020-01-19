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

void can_send_extended_message(uint32_t id_value, uint8_t *data,
uint32_t data_length);

void can_send_standard_message(uint32_t id_value, uint8_t *data, uint32_t data_length);

extern volatile int canline_0_updated;

extern volatile int canline_1_updated;

extern volatile can_message_t canline[64];

extern volatile int canline_i;

extern struct can_module can_instance;

void configure_i2c(void);

int configure_tmp75a(uint8_t ADDR);

uint16_t read_tmp75a(uint8_t ADDR);

#define TMP75A_ADDR_MCU 0x48

#define TMP75A_ADDR_FET 0x49

#define TMP75A_ADDR_AIR 0x4A


#endif /* UTIL_H_ */