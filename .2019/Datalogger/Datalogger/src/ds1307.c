/*
 * ds1307.c
 *
 * Created: 4/27/2019 6:04:19 PM
 *  Author: Connor
 */ 

#include <asf.h>
#include "util.h"


#define SLAVE_ADDRESS 0x12
#define SADDR 0xD0
#define DS1307_ADDRESS  0x68
#define DS1307_CONTROL  0x07
#define DS1307_NVRAM    0x08
#define I2C_TIMEOUT 1000

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
	config_i2c_master.pinmux_pad0    = RTC_PINMUX_PAD0;
	config_i2c_master.pinmux_pad1    = RTC_PINMUX_PAD1;
	/* Initialize and enable device with config */
	i2c_master_init(&i2c_master_instance, RTC_MODULE, &config_i2c_master);
	i2c_master_enable(&i2c_master_instance);
	delay_ms(1);
}

// Think I disabled callbacks

void i2c_write_complete_callback(struct i2c_master_module *const module)
{
}

void i2c_read_complete_callback(struct i2c_master_module *const module)
{
}

void configure_i2c_callbacks(void)
{
}

extern struct rtc_module rtc_instance;

static void configure_rtc_calendar(struct rtc_calendar_time *init_time)
{
	/* Initialize RTC in calendar mode. */
	struct rtc_calendar_config config_rtc_calendar;

	rtc_calendar_get_config_defaults(&config_rtc_calendar);

	config_rtc_calendar.clock_24h     = true;
	config_rtc_calendar.alarm[0].time = *init_time;
	config_rtc_calendar.alarm[0].mask = RTC_CALENDAR_ALARM_MASK_YEAR;

	rtc_calendar_init(&rtc_instance, RTC, &config_rtc_calendar);

	rtc_calendar_enable(&rtc_instance);
	
	rtc_calendar_set_time(&rtc_instance, init_time);
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


#define STR2INT(str) (((uint32_t)(str)[0] << 24) | ((uint32_t)(str)[1] << 16) | ((uint32_t)(str)[2] << 8) | ((uint32_t)(str)[3]))
#define Jan  1247899168
#define Feb  1181049376
#define Mar  1298231840
#define Apr  1097888288
#define May  1298233632
#define Jun  1249209888
#define Jul  1249209376
#define Aug  1098213152
#define Sep  1399156768
#define Oct  1331917856
#define Nov  1315927584
#define Dec  1147495200

static inline int tenmonth(void)
{
	switch (STR2INT(__DATE__ + 3))
	{
		case Jan:
		case Feb:
		case Mar:
		case Apr:
		case May:
		case Jun:
		case Jul:
		case Aug:
		case Sep:
			return 0;
		case Oct:
		case Nov:
		case Dec:
			return 1;
	}
}

static inline int onemonth(void)
{
	switch (STR2INT(__DATE__ + 3))
	{
		case Oct:
			return 0;
		case Jan:
		case Nov:
			return 1;
		case Feb:
		case Dec:
			return 2;
		case Mar:
			return 3;
		case Apr:
			return 4;
		case May:
			return 5;
		case Jun:
			return 6;
		case Jul:
			return 7;
		case Aug:
			return 8;
		case Sep:
			return 9;
	}
}

#define DS1307_UNCONFIGURED(packet) (((packet).data[0] >> 7) & 1)

static void parse_ds1307_data(uint8_t data[], struct rtc_calendar_time *ts)
{
	ts->year = ((data[6] >> 4) & 0xF) * 10 + (data[6] & 0xF)+2000;
	ts->month = ((data[5] >> 4) & 0xF) * 10 + (data[5] & 0xF);
	ts->day = ((data[4] >> 4) & 0xF) * 10 + (data[4] & 0xF);
	if ((data[2] >> 6) & 1) { // 12-hour mode
		ts->hour = ((data[2] >> 5) & 1) * 12 + ((data[2] >> 4) & 1) * 10 + ((data[2]) & 0xF);
	} else {
		ts->hour = ((data[2] >> 4) & 3) * 10 + ((data[2]) & 0xF);
	}
	ts->minute = ((data[1] >> 4) & 0xF) * 10 + (data[1] & 0xF);
	ts->second = ((data[0] >> 4) & 0x7) * 10 + (data[0] & 0xF);
}

void initialize_rtc_calendar(void)
{
	uint8_t rxbuf[50];
	uint8_t txbuf[10];

	bzero(rxbuf, sizeof(rxbuf));
	bzero(txbuf, sizeof(txbuf));
	
	/* Init i2c packet. */
	struct i2c_master_packet packet = {
		.address     = DS1307_ADDRESS,
		.data_length = 1,
		.data        = txbuf,
		.ten_bit_address = false,
		.high_speed      = false,
		.hs_master_code  = 0x0,
	};
	
	/* Write buffer to slave until success. */
	if (write_packet_with_timeout_no_stop(&packet, I2C_TIMEOUT) != STATUS_OK) {
		puts("DS1307 configuration failed");
		return;
	}
	
	/* Read from slave until success. */
	packet.data = rxbuf;
	packet.data_length = 7;
	if (read_packet_with_timeout(&packet, I2C_TIMEOUT) != STATUS_OK) {
		puts("DS1307 reading failed");
		return;
	}

	if (DS1307_UNCONFIGURED(packet)) {
		// write time
		printf("Initializing to %s %s\r\n", __DATE__, __TIME__);
		packet.data_length = 8;
		packet.data = txbuf;
		txbuf[0] = 0;
		txbuf[1] = (0<<7) | ((__TIME__[6]-'0') << 4) | (__TIME__[7]-'0'); //seconds
		txbuf[2] = ((__TIME__[3]-'0') << 4) | (__TIME__[4]-'0'); //minutes
		txbuf[3] = (0 << 6) | ((__TIME__[0]-'0') << 4) | (__TIME__[1]-'0'); //hours(24-hr)
		txbuf[4] = 6;
		txbuf[5] = 0x04;
		txbuf[6] = 0x05;
		txbuf[7] = 0x19;
		
		write_packet_with_timeout_no_stop(&packet, I2C_TIMEOUT);
		
		printf("Updated time on RTC\r\n");
		for (int i = 0; i < 7; ++i) {
			txbuf[i] = txbuf[i+1];
		}
	}
		
	i2c_master_disable(&i2c_master_instance);
	
	struct rtc_calendar_time ts;
	parse_ds1307_data(packet.data, &ts);
	configure_rtc_calendar(&ts);
	
	printf("RTC initialized at %02d/%02d/%04d %02d:%02d:%02d\n", ts.month, ts.day, ts.year, ts.hour, ts.minute, ts.second);
}

void read_time(struct rtc_calendar_time *ts)
{
	rtc_calendar_get_time(&rtc_instance, ts);
}