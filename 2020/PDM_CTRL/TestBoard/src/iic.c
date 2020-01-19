/*
 * iic.c
 *
 * Created: 1/18/2020 6:36:16 PM
 *  Author: etshr
 */ 
#include "iic.h"


struct i2c_master_module i2c_master_instance;

void configure_i2c(void)
{
	struct i2c_master_config config_i2c_master;
	i2c_master_get_config_defaults(&config_i2c_master);
	config_i2c_master.buffer_timeout = 10000;
	config_i2c_master.pinmux_pad0    = TMP75A_PINMUX_PAD0;
	config_i2c_master.pinmux_pad1    = TMP75A_PINMUX_PAD1;
	i2c_master_init(&i2c_master_instance, TMP75A_MODULE, &config_i2c_master);
	i2c_master_enable(&i2c_master_instance);
}

enum status_code write_packet_with_timeout(struct i2c_master_packet *i2cpacket, int timeout)
{
	enum status_code res;
	while ((res = i2c_master_write_packet_wait(&i2c_master_instance, i2cpacket)) != STATUS_OK) {
		if (timeout-- <= 0)
		return res;
	}
	return res;
}

enum status_code read_packet_with_timeout(struct i2c_master_packet *i2cpacket, int timeout)
{
	enum status_code res;
	while ((res = i2c_master_read_packet_wait(&i2c_master_instance, i2cpacket)) != STATUS_OK) {
		if (timeout-- <= 0)
		return res;
	}
	return res;
}

enum status_code write_packet_with_timeout_no_stop(struct i2c_master_packet *i2cpacket, int timeout)
{
	enum status_code res;
	while ((res = i2c_master_write_packet_wait_no_stop(&i2c_master_instance, i2cpacket)) != STATUS_OK) {
		if (timeout-- <= 0)
		return res;
	}
	return res;
}

enum status_code read_packet_with_timeout_no_stop(struct i2c_master_packet *i2cpacket, int timeout)
{
	enum status_code res;
	while ((res = i2c_master_read_packet_wait_no_stop(&i2c_master_instance, i2cpacket)) != STATUS_OK) {
		if (timeout-- <= 0)
		return res;
	}
	return res;
}
