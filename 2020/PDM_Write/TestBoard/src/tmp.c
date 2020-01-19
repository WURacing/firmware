#include <asf.h>
#include <stdint.h>

////////////// config //////////////////

#define TMP75A_PINMUX_PAD0 PINMUX_PA22C_SERCOM3_PAD0 // SDA
#define TMP75A_PINMUX_PAD1 PINMUX_PA23C_SERCOM3_PAD1 // SCL
#define TMP75A_MODULE SERCOM3
#define TMP75A_ADDR 0x48

#define CONF_OS 7
#define CONF_R1 6
#define CONF_R0 5
#define CONF_F1 4
#define CONF_F0 3
#define CONF_POL 2
#define CONF_TM 1
#define CONF_SD 0

// config: enable 12 bit temperature
#define TMP75A_CONF ((1 << CONF_R1) | (1 << CONF_R0) | (1 << CONF_TM))

#define REG_TEMP 0
#define REG_CONF 1

#define TIMEOUT 1000

////////////////////////////////////////

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

static enum status_code write_packet_with_timeout(struct i2c_master_packet *i2cpacket, int timeout)
{
	enum status_code res;
	while ((res = i2c_master_write_packet_wait(&i2c_master_instance, i2cpacket)) != STATUS_OK) {
		if (timeout-- <= 0)
		return res;
	}
	return res;
}

static enum status_code read_packet_with_timeout(struct i2c_master_packet *i2cpacket, int timeout)
{
	enum status_code res;
	while ((res = i2c_master_read_packet_wait(&i2c_master_instance, i2cpacket)) != STATUS_OK) {
		if (timeout-- <= 0)
		return res;
	}
	return res;
}

static enum status_code write_packet_with_timeout_no_stop(struct i2c_master_packet *i2cpacket, int timeout)
{
	enum status_code res;
	while ((res = i2c_master_write_packet_wait_no_stop(&i2c_master_instance, i2cpacket)) != STATUS_OK) {
		if (timeout-- <= 0)
		return res;
	}
	return res;
}

static enum status_code read_packet_with_timeout_no_stop(struct i2c_master_packet *i2cpacket, int timeout)
{
	enum status_code res;
	while ((res = i2c_master_read_packet_wait_no_stop(&i2c_master_instance, i2cpacket)) != STATUS_OK) {
		if (timeout-- <= 0)
		return res;
	}
	return res;
}

int configure_tmp75a(void)
{
	uint8_t buf[10];

	struct i2c_master_packet packet = {
		.address         = TMP75A_ADDR,
		.data_length     = 2,
		.data            = buf,
		.ten_bit_address = false,
		.high_speed      = false,
		.hs_master_code  = 0x0,
	};

	// write config
	buf[0] = REG_CONF;
	buf[1] = TMP75A_CONF;
	packet.data_length = 2;
	
	if (write_packet_with_timeout_no_stop(&packet, TIMEOUT) != STATUS_OK) {
		//puts("TMP75A configuration failed");
		return 1;
	}

	// reset pointer to temperature register so we read from the temperature register next time
	buf[0] = REG_TEMP;
	packet.data_length = 1;

	if (write_packet_with_timeout(&packet, TIMEOUT) != STATUS_OK) {
		//puts("TMP75A reset pointer failed");
		return 2;
	}
	return 0;
}

uint16_t read_tmp75a(void)
{
	
	uint8_t buf[10];

	struct i2c_master_packet packet = {
		.address         = TMP75A_ADDR,
		.data_length     = 2,
		.data            = buf,
		.ten_bit_address = false,
		.high_speed      = false,
		.hs_master_code  = 0x0,
	};

	// reads from temp reg into buf

	if (read_packet_with_timeout(&packet, TIMEOUT) != STATUS_OK) {
		//puts("TMP75A read failed");
		return -1; // not possibly a valid result
	}

	uint16_t result = (buf[0] << 8) | (buf[1]);

	return result;
}