/*
 * sevenseg.h
 *
 * Created: 3/23/2019 5:03:03 PM
 *  Author: Wash U Racing
 */ 


#ifndef SEVENSEG_H_
#define SEVENSEG_H_

#include <stdint.h>

int mcp23017_configure(void);
int mcp23017_write(uint8_t number, uint8_t digit);

int display5digit(const char *str);

void i2c_init(void);
int i2c_start(void);
int i2c_send_addr(uint8_t data);
int i2c_send_data(uint8_t data);
void i2c_stop(void);
int i2c_restart(void);

extern const uint8_t SevenSegmentASCII[96];

int initsevenseg(void);

#endif /* SEVENSEG_H_ */