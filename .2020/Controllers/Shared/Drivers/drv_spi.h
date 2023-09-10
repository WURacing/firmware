/* 
 * File:   drv_spi.h
 * Author: Connor
 *
 * Master mode SPI
 * Created on October 2, 2020, 11:38 AM
 */

#ifndef DRV_SPI_H
#define	DRV_SPI_H

#include "drv_spi_private.h"
#include "drv_serial.h"
#include <sam.h>
#include <stdint.h>



struct drv_spi_channelConfig {
	int sercom_id;
	sercom_spim_registers_t * module;
	unsigned do_pinmux, di_pinmux, sck_pinmux, ss_pin, ss_port;
	/* Valid configurations (for master mode):
	 * 
		+------+------+------+------+------+------+
		| DOPO | DIPO | PAD0 | PAD1 | PAD2 | PAD3 |
		+------+------+------+------+------+------+
		|    0 |    2 | DO   | SCK  | DI   |      |
		|    0 |    3 | DO   | SCK  |      | DI   |
		|    1 |    0 | DI   |      | DO   | SCK  |
		|    1 |    1 |      | DI   | DO   | SCK  |
		|    2 |    0 | DI   | SCK  |      | DO   |
		|    2 |    2 |      | SCK  | DI   | DO   |
		|    3 |    1 | DO   | DI   |      | SCK  |
		|    3 |    2 | DO   |      | DI   | SCK  |
		+------+------+------+------+------+------+
	 */
	unsigned do_po, di_po;
	unsigned baud; // use SERIAL_BAUD macro
};

struct drv_spi_config {
	const struct drv_spi_channelConfig * channelConfig;
};
extern const struct drv_spi_config drv_spi_config;

void drv_spi_init(void);
uint8_t drv_spi_transfer(enum drv_spi_channel channel, uint8_t out);

#endif	/* DRV_SPI_H */

