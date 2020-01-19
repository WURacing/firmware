/*
 * current_control.h
 *
 * Created: 1/18/2020 6:52:02 PM
 *  Author: etshr
 */ 
#ifndef CURRENT_CONTROL_H_
#define CURRENT_CONTROL_H_

#define CURRENT_ON 0x00
#define CURRENT_OFF 0x01

#include <asf.h>

enum status_code configure_current_control(void);

enum status_code mcp23017_current_send_data(uint8_t dataA, uint8_t dataB);

uint16_t set_current_control(void);

void set_current_control_reg(uint8_t data_line, uint8_t STATE);

#endif /* CURRENT_CONTROL_H_ */