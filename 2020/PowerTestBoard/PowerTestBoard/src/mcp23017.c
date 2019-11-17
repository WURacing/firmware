/*
 * mcp23017.c
 *
 * Created: 11/9/2019 12:31:18 PM
 *  Author: connor
 */ 

#include <asf.h>


#define SLAVE_ADDRESS 0x12
#define SADDR 0xD0
#define DS1307_ADDRESS  0x68
#define DS1307_CONTROL  0x07
#define DS1307_NVRAM    0x08
#define I2C_TIMEOUT 1000

#define MCP23017_ADDRESS 0x20

void i2c_write_complete_callback(struct i2c_master_module *const module);
void i2c_read_complete_callback(struct i2c_master_module *const module);
void configure_i2c_callbacks(void);
enum status_code write_packet_with_timeout(struct i2c_master_packet *i2cpacket, int timeout);
enum status_code read_packet_with_timeout(struct i2c_master_packet *i2cpacket, int timeout);
enum status_code write_packet_with_timeout_no_stop(struct i2c_master_packet *i2cpacket, int timeout);
enum status_code read_packet_with_timeout_no_stop(struct i2c_master_packet *i2cpacket, int timeout);

struct i2c_master_module i2c_master_instance;



void configure_i2c(void)
{
	/* Initialize config structure and software module */
	struct i2c_master_config config_i2c_master;
	i2c_master_get_config_defaults(&config_i2c_master);
	/* Change buffer timeout to something longer */
	config_i2c_master.buffer_timeout = 10000;
	config_i2c_master.pinmux_pad0    = MCP23017_PINMUX_PAD0;
	config_i2c_master.pinmux_pad1    = MCP23017_PINMUX_PAD1;
	/* Initialize and enable device with config */
	i2c_master_init(&i2c_master_instance, MCP23017_MODULE, &config_i2c_master);
	i2c_master_enable(&i2c_master_instance);
	delay_ms(1);
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



#define DS1307_UNCONFIGURED(packet) (((packet).data[0] >> 7) & 1)


int initialize_mcp23017(void)
{
	uint8_t txbuf[3];
	int error;

	struct i2c_master_packet packet = {
		.address     = MCP23017_ADDRESS,
		.data_length = 3,
		.data        = txbuf,
		.ten_bit_address = false,
		.high_speed      = false,
		.hs_master_code  = 0x0,
	};
	txbuf[0] = 0x00;
	txbuf[1] = 0x00;
	txbuf[2] = 0x00;
	error = write_packet_with_timeout(&packet, I2C_TIMEOUT);
	if (error != STATUS_OK) {
		return error;
	}
	
	return 0;
	
}

int mcp23017_send_data(uint8_t dataA, uint8_t dataB)
{
	uint8_t txbuf[3];
	int error;

	struct i2c_master_packet packet = {
		.address     = MCP23017_ADDRESS,
		.data_length = 3,
		.data        = txbuf,
		.ten_bit_address = false,
		.high_speed      = false,
		.hs_master_code  = 0x0,
	};
	txbuf[0] = 0x12;
	txbuf[1] = dataA;
	txbuf[2] = dataB;
	error = write_packet_with_timeout(&packet, I2C_TIMEOUT);
	if (error != STATUS_OK) {
		return error;
	}
	
	return 0;
}