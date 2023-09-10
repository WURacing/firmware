/* 
 * File:   drv_i2c.h
 * Author: Connor
 *
 * Master mode I2C driver
 * Created on October 2, 2020, 1:05 PM
 */

#ifndef DRV_I2C_H
#define	DRV_I2C_H

#include "drv_i2c_private.h"
#include "drv_serial.h"
#include <sam.h>

// to do this accurately we need tRISE to be calculated from bus impedance
#define I2C_tRISE 0.f
// inverse of fSCL on SAM C21 datasheet page 563
#define I2C_BAUD(x) SERCOM_I2CM_BAUD_BAUD((int) ((CLOCK - 10.f*x - CLOCK*I2C_tRISE*x)/(2.f*x)))

struct drv_i2c_channelConfig {
	int sercom_id;
	sercom_i2cm_registers_t * module;
	// MUST BE PAD0. Only works on a very limited set of pins. Check dataset page 34!!!!!!!!!!!!
	unsigned sda_pinmux;
	// MUST BE PAD1.
	unsigned scl_pinmux;
	unsigned baud; // use SERIAL_BAUD macro
};

struct drv_i2c_config {
	const struct drv_i2c_channelConfig * channelConfig;
};
extern const struct drv_i2c_config drv_i2c_config;

void drv_i2c_init(void);
uint8_t drv_i2c_transfer(enum drv_i2c_channel channel, uint8_t out);

int drv_i2c_read_register(enum drv_i2c_channel channel, uint8_t address, uint8_t pointer, uint8_t results[], int length);
int drv_i2c_write_register(enum drv_i2c_channel channel, uint8_t address, uint8_t pointer, const uint8_t command[], int length);


#endif	/* DRV_I2C_H */

