/*
 * drv_lte.c
 *
 * Created: 8/8/2020 11:14:01 AM
 *  Author: connor
 */ 

#include "drv_lte.h"
#include "drv_uart.h"
#include "drv_divas.h"
#include "sam.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include <time.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#define LTE_POWER_GROUP 0
#define LTE_POWER_PORT PORT_PA14

#define MQTT_CLIENT_ID "2022"
#define MQTT_HOST "3.tcp.ngrok.io"
#define MQTT_PORT "22463"


// Private data

static struct {
	bool logged_in;
	struct drv_lte_location last_location;
	TickType_t last_location_time;
	struct drv_lte_time last_time;
	TickType_t last_time_time;
	char cmd_buf[100];
} drv_lte_data = {0};


// Private functions

static const char * at_wait_resp(int timeout)
{
	int code;
	const char * response = drv_uart_read_line(DRV_UART_CHANNEL_LTE, timeout, "\r");
	if (response)
	{
		// Check whether we got information text, or error numeric response
		if (sscanf(response, "%d\r", &code) > 0)
		{
			return NULL;
		}
		else
		{
			// We got an information text response
			strncpy(drv_lte_data.cmd_buf, response, 100);
			if (strstr(drv_lte_data.cmd_buf, "$GPRMC"))
			{
				drv_uart_read_line(DRV_UART_CHANNEL_LTE, timeout, "\r\n\r");
			}
			response = drv_uart_read_line(DRV_UART_CHANNEL_LTE, timeout, "\r");
			if (response)
			{
				// We got a final result code
				if (sscanf(response, "\n%d\r", &code) > 0)
				{
					if (code == 0)
					{
						return drv_lte_data.cmd_buf;
					}
				}
			}
		}
	}
	return NULL;
}


static int at_cmd(const char * command, int timeout)
{
	const char * response;
	int code;
	drv_uart_clear_response(DRV_UART_CHANNEL_LTE);
	drv_uart_send_message(DRV_UART_CHANNEL_LTE, command);
	response = drv_uart_read_line(DRV_UART_CHANNEL_LTE, timeout, "\r");
	if (response)
	{
		// We got a final result code
		if (sscanf(response, "%d\r", &code) > 0)
		{
			return code == 0;
		}
	}
	return false;
}

static const char * at_cmd_resp(const char * command, int timeout)
{
	drv_uart_clear_response(DRV_UART_CHANNEL_LTE);
	drv_uart_send_message(DRV_UART_CHANNEL_LTE, command);
	return at_wait_resp(timeout);
}

static const char * at_cmd_custom(const char * command, const char * expected, int timeout)
{
	drv_uart_clear_response(DRV_UART_CHANNEL_LTE);
	drv_uart_send_message(DRV_UART_CHANNEL_LTE, command);
	return drv_uart_read_line(DRV_UART_CHANNEL_LTE, timeout, expected);
}

static inline int sara_check_network_registration()
{
	const char * response;
	if ((response = at_cmd_resp("AT+CREG?\r", 1000)) != NULL)
	{
		int n, stat;
		if (sscanf(response, "+CREG: %d,%d", &n, &stat) >= 2)
		{
			return (stat == 1 || stat == 5);
		}
	}
	return false;
}

static inline int sara_open_udp_socket()
{
	const char * response;
	if ((response = at_cmd_resp("AT+USOCR=17\r", 1000)) != NULL)
	{
		int sock;
		if (sscanf(response, "+USOCR: %d", &sock) >= 1)
		{
			return sock;
		}
	}
	return -1;
}

static inline int sara_close_socket(int sock)
{
	snprintf(drv_lte_data.cmd_buf, 100, "AT+USOCL=%d\r", sock);
	return at_cmd(drv_lte_data.cmd_buf, 120000);
}

static inline int sara_send_to_socket(int sock, const char * dest, int port, const uint8_t *data, int length)
{
	snprintf(drv_lte_data.cmd_buf, 100, "AT+USOST=%d,\"%s\",%d,%d\r", sock, dest, port, length);
	if (at_cmd_custom(drv_lte_data.cmd_buf, "@", 130000))
	{
		drv_uart_send_data(DRV_UART_CHANNEL_LTE, data, length);
		if (at_wait_resp(130000))
		{
			return true;
		}
	}
	return false;
}

static inline int sara_publish_topic_message(const char *topic, const char *message)
{
	snprintf(drv_lte_data.cmd_buf, 100, "AT+UMQTTC=2,0,0,\"%s\",\"%s\"\r", topic, message);
	const char * response = (at_cmd_custom(drv_lte_data.cmd_buf, "+UMQTTC: 2,1\r\r\n0\r", 2000));
	return response != NULL;
}

static void parse_nmea_rmc(char * s);

static inline int sara_update_rmc()
{
	const char * response;
	if ((response = at_cmd_resp("AT+UGRMC?\r", 100)) != NULL)
	{
		parse_nmea_rmc((char *)response);
	}
	return true;
}


static int fields(char * src)
{
	int num = 1;
	while (*src != '\0')
	{
		if (*src == ',')
		{
			*src = '\0'; // terminate string there
			num++;
		}
		src++; // move to start char of next field
	}
	return num;
}

/************************************************************************/
/* Parses a string like either (lon=0) XXYY.Z+ or (lon=1) XXXYY.Z+      */
/************************************************************************/
static int parse_lat_lon(const char * s, bool lon)
{
	const int numDegreeDigits = lon ? 3 : 2;
	const int minDigits = numDegreeDigits + 2 + 1 + 1; // Expect degree digits, plus minutes int digits, plus dp, plus at least one fractional digit
	const char * decimalPoint = s + numDegreeDigits + 2;
	int returnValue;
	const int inputStringLength = strlen(s);
	const char * minutesIntStart = s + numDegreeDigits;
	const char * minutesFracStart = s + numDegreeDigits + 2 + 1;
	// Check for expected decimal point and expected number of degree digits
	if ((inputStringLength >= minDigits) && (*decimalPoint == '.'))
	{
		int degrees;
		if (lon)
		{
			degrees = (s[0] - '0') * 100 + (s[1] - '0') * 10 + (s[2] - '0') * 1;
		}
		else
		{
			degrees = (s[0] - '0') * 10 + (s[1] - '0') * 1;
		}
		int minutesInt = (minutesIntStart[0] - '0') * 10 + (minutesIntStart[1] - '0') * 1;
		int minutesFrac = 0;
		for (int i = 0; i < 6; i++)
		{
			if (*minutesFracStart)
			{
				minutesFrac = (minutesFrac * 10) + (*minutesFracStart - '0'); 
				minutesFracStart++;
			}
			else
			{
				minutesFrac *= 10;
			}
		}
		returnValue = degrees * 1000000 + (minutesInt * 1000000 + minutesFrac) / 60;
	}
	else
	{
		returnValue = 0;
	}
	return returnValue;
}

static void twotwotwo(const char *s, int *one, int *two, int *three)
{
	*one = *two = *three = 0;
	if (strlen(s) < 6) return;
	
	*one   = (s[0] - '0') * 10 + (s[1] - '0');
	*two   = (s[2] - '0') * 10 + (s[3] - '0');
	*three = (s[4] - '0') * 10 + (s[5] - '0');
}

static void parse_nmea_rmc(char * s)
{
	struct drv_lte_location current = {0};
	struct drv_lte_time times = {0};
	bool location_valid;
	
	s = strstr(s, "$GPRMC") + 7;
	
	int num = fields(s);
	if (num != 12) return;
	
	// Field 1: timestamp
	twotwotwo(s, &times.hour, &times.minute, &times.second);
	s += strlen(s) + 1;
	// Field 2: active/void
	location_valid = (*s == 'A');
	s += strlen(s) + 1;
	// Field 3: latitude
	if (location_valid)
		current.latitude = parse_lat_lon(s, false);
	s += strlen(s) + 1;
	// Field 4: latitude direction
	if (location_valid && (*s == 'S'))
	{
		current.latitude *= -1;
	}
	s += strlen(s) + 1;
	// Field 5: longitude
	if (location_valid)
		current.longitude = parse_lat_lon(s, true);
	s += strlen(s) + 1;
	// Field 6: longitude direction
	if (location_valid && (*s == 'W'))
	{
		current.longitude *= -1;
	}
	s += strlen(s) + 1;
	// Field 7: speed
	s += strlen(s) + 1;
	// Field 8: angle
	s += strlen(s) + 1;
	// Field 9: date
	twotwotwo(s, &times.day, &times.month, &times.year);
	
	if (location_valid)
	{
		drv_lte_data.last_location_time = xTaskGetTickCount();
		drv_lte_data.last_location = current;
	}
	drv_lte_data.last_time_time = xTaskGetTickCount();
	drv_lte_data.last_time = times;
}

// PUBLIC API


void drv_lte_init()
{
	drv_lte_data.last_location_time = 0;
	PORT_REGS->GROUP[LTE_POWER_GROUP].PORT_DIRSET = LTE_POWER_PORT;
}


bool drv_lte_configure(void)
{
	const char * response;
	
	// Turn module on (if not already on)
	PORT_REGS->GROUP[LTE_POWER_GROUP].PORT_OUTCLR = LTE_POWER_PORT;
	vTaskDelay(500);
	PORT_REGS->GROUP[LTE_POWER_GROUP].PORT_OUTSET = LTE_POWER_PORT;
	vTaskDelay(1500);

	// Configure communications protocol (disable echo, disable verbose)
	if (!at_cmd_custom("ATE0V0\r", "0\r", 10)) return false;
	// disable CME errors (still reported as code=4)
	if (!at_cmd("AT+CMEE=0\r", 10)) return false;

	// Check whether the GPS is on
	response = at_cmd_resp("AT+UGPS?\r", 10000);
	if (!response) return false;
	if (strstr(response, "+UGPS: 0"))
	{
		// Try to power on GPS
		if (!at_cmd("AT+UGPS=1,0,3\r", 10000)) return false;
	}

	// Store GPS NMEA RMC strings (contains lat/lon, date/time, etc)
	if (!at_cmd("AT+UGRMC=1\r", 10000)) return false;

	// Clear stuff
	//at_cmd_custom("AT+USOCL=0;+USOCL=1;+USOCL=2;+USOCL=3;+USOCL=4;+USOCL=5;+USOCL=6\r", "\r", 10);
	vTaskDelay(1);

	// Set MQTT parameters
	at_cmd_custom("AT+UMQTT=0,\"" MQTT_CLIENT_ID "\"\r", "+UMQTT=0,1\r\n0\r", 1000);
	at_cmd_custom("AT+UMQTT=2,\"" MQTT_HOST "\"," MQTT_PORT "\r", "+UMQTT=2,1\r\n0\r", 1000);

	vTaskDelay(1);
	
	return true;
}


bool drv_lte_is_network_registered(void)
{
	return sara_check_network_registration();
}

bool drv_lte_is_logged_in(void)
{
	return drv_lte_data.logged_in;
}

bool drv_lte_mqtt_login(void)
{
	const char * response = at_cmd_custom("AT+UMQTTC=1\r", "+UUMQTTC: 1,0", 30000);
	drv_lte_data.logged_in = response != NULL;
	return drv_lte_data.logged_in;
}

bool drv_lte_mqtt_publish(const char *topic, const char *message)
{
#if 0
	if (!sara_publish_topic_message(topic, message))
		drv_lte_data.logged_in = false;
	return drv_lte_data.logged_in;
#else
	return sara_publish_topic_message(topic, message);
#endif
}

const struct drv_lte_location * drv_lte_get_last_location(void)
{
	if (drv_lte_data.last_location_time > 0)
	{
		return &drv_lte_data.last_location;
	}
	else
	{
		return NULL;
	}
}

const struct drv_lte_time * drv_lte_get_last_time(void)
{
	if (drv_lte_data.last_time_time > 0)
	{
		return &drv_lte_data.last_time;
	}
	else
	{
		return NULL;
	}
}

