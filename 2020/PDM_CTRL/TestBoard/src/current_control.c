/*
 * current_control.c
 *
 * Created: 1/18/2020 6:51:47 PM
 *  Author: etshr
 */ 


#include "current_control.h"
#include "iic.h"

#define MCP23017_ADDRESS_CURRENT 0x21
#define I2C_TIMEOUT 1000

// DEFAULT TO ON
volatile uint8_t current_control_A, current_control_B = 0x00;


enum status_code configure_current_control(void)
{
	uint8_t txbuf[3];
	int error;

	struct i2c_master_packet packet = {
		.address     = MCP23017_ADDRESS_CURRENT,
		.data_length = 3,
		.data        = txbuf,
		.ten_bit_address = false,
		.high_speed      = false,
		.hs_master_code  = 0x0,
	};
	txbuf[0] = 0x00;
	txbuf[1] = 0x00;
	txbuf[2] = 0x00;
	return write_packet_with_timeout(&packet, I2C_TIMEOUT);
}

enum status_code mcp23017_current_send_data(uint8_t dataA, uint8_t dataB)
{
	uint8_t txbuf[3];
	int error;

	struct i2c_master_packet packet = {
		.address     = MCP23017_ADDRESS_CURRENT,
		.data_length = 3,
		.data        = txbuf,
		.ten_bit_address = false,
		.high_speed      = false,
		.hs_master_code  = 0x0,
	};
	txbuf[0] = 0x12;
	txbuf[1] = dataA;
	txbuf[2] = dataB;
	return write_packet_with_timeout(&packet, I2C_TIMEOUT);
}

uint16_t set_current_control(void)
{
	mcp23017_current_send_data(current_control_A, current_control_B);
	
	uint16_t data = 0x00;
	data |= current_control_B << 8;
	data |= current_control_A;
	return data;
}

void set_current_control_reg(uint8_t data_line, uint8_t STATE) {
	if (data_line > 8) {
		data_line -= 8;
		if (STATE == LC_CURRENT_OFF) {
			current_control_B |= (1 << data_line);
		} else {
			current_control_B &= ~(1 << data_line);
		}
	} else {
		if (STATE == LC_CURRENT_OFF) {
			current_control_A |= (1 << data_line);
			} else {
			current_control_A &= ~(1 << data_line);
		}
	}
}