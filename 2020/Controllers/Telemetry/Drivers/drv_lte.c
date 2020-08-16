/*
 * drv_lte.c
 *
 * Created: 8/8/2020 11:14:01 AM
 *  Author: connor
 */ 

#include "drv_lte.h"
#include "drv_uart.h"
#include "drv_divas.h"
#include "app_statusLight.h"
#include "sam.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include <time.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>

#define LTE_POWER_GROUP 0
#define LTE_POWER_PORT PORT_PA10

static enum drv_lte_state {
	DRV_LTE_POWER_TOGGLE_1,
	DRV_LTE_POWER_TOGGLE_2,
	DRV_LTE_CONFIGURE_CMDECHO,
	DRV_LTE_CONFIGURE_GNSS_ENABLE,
	DRV_LTE_CONFIGURE_GNSS_NMEA,
	DRV_LTE_READ_GNSS,
	DRV_LTE_OPEN_SOCKET,
	DRV_LTE_CLOSE_SOCKET,
	DRV_LTE_TRANSMIT_UDP_META,
	DRV_LTE_TRANSMIT_UDP_DATA,
	DRV_LTE_WAIT,
	
	DRV_LTE_COUNT,
	DRV_LTE_INVALID
} current_state;

static struct drv_lte_location last_location;
static TickType_t last_location_time = 0;
static struct drv_lte_time last_time;
static TickType_t last_time_time = 0;
static bool network_registered = false;
static volatile bool message_to_send = false;
static int udp_socket_id = -1;
static const char udp_message_destination[50] = "3.18.176.160";
static const unsigned udp_port = 9999;
static uint8_t udp_message_data[256];
static unsigned udp_message_length = 0;
static SemaphoreHandle_t transmission_semaphore;

uint8_t * drv_lte_get_transmission_queue(void)
{
	uint8_t * queue = NULL;
	if (message_to_send == false)
	{
		if (xSemaphoreTake(transmission_semaphore, 0))
		{
			queue = udp_message_data;
		}
	}
	return queue;
}

void drv_lte_queue_transmission(int length)
{
	udp_message_length = length;
	xSemaphoreGive(transmission_semaphore);
	message_to_send = true;
}

void drv_lte_cancel_transmission(void)
{
	message_to_send = false;
	udp_message_length = 0;
	xSemaphoreGive(transmission_semaphore);
}


static TickType_t timer;

static void drv_lte_state_entry(void);
static void drv_lte_state_action(void);
static enum drv_lte_state drv_lte_state_transition(void);
static void drv_lte_parse_nmea_rmc(const char *);
static void drv_lte_parse_creg(const char *);
static void drv_lte_parse_usocr(const char *);
static void drv_lte_parse_usost(const char *);

static const char * drv_lte_utoa(unsigned i)
{
	static char intbuf[13] = {0}; // not reentrant really :)
	char *s = intbuf + 11;
	
	*s = '0';
	if (i == 0) return s;
	while (i > 0)
	{
		struct drv_divas_quot_rem_u result = drv_divas_divide(i, 10);
		//*(s--) = (i % 10) + '0';
		//i /= 10;
		*(s--) = result.remainder + '0';
		i = result.quotient;
	}
	return s + 1;
}

void drv_lte_init()
{
	last_location_time = 0;
	PORT->Group[LTE_POWER_GROUP].DIRSET.reg = LTE_POWER_PORT;
	
	transmission_semaphore = xSemaphoreCreateMutex();
	vQueueAddToRegistry(transmission_semaphore, "LTETX");
			
	current_state = DRV_LTE_POWER_TOGGLE_1;
	drv_lte_state_entry();
	drv_lte_state_action();
	
}

void drv_lte_periodic()
{
	enum drv_lte_state next_state = drv_lte_state_transition();
	if (current_state != next_state)
	{
		if (next_state < DRV_LTE_COUNT)
		{
			current_state = next_state;
		}
		drv_lte_state_entry();
	}
	drv_lte_state_action();
}

static void drv_lte_state_entry()
{
	switch (current_state)
	{
		case DRV_LTE_POWER_TOGGLE_1:
			PORT->Group[LTE_POWER_GROUP].OUTCLR.reg = LTE_POWER_PORT;
			timer = xTaskGetTickCount() + 500;
			break;
			
		case DRV_LTE_POWER_TOGGLE_2:
			PORT->Group[LTE_POWER_GROUP].OUTSET.reg = LTE_POWER_PORT;
			timer = xTaskGetTickCount() + 1500;
			break;
			
		case DRV_LTE_CONFIGURE_CMDECHO:
			timer = xTaskGetTickCount() + 100;
			drv_uart_clear_response();
			drv_uart_send_message("ATE0\r");
			break;
			
		case DRV_LTE_CONFIGURE_GNSS_ENABLE:
			timer = xTaskGetTickCount() + 10000;
			drv_uart_clear_response();
			drv_uart_send_message("AT+UGPS=1,0,3\r");
			break;
			
		case DRV_LTE_CONFIGURE_GNSS_NMEA:
			timer = xTaskGetTickCount() + 10000;
			drv_uart_clear_response();
			drv_uart_send_message("AT+UGRMC=1\r");
			break;
			
		case DRV_LTE_READ_GNSS:
			timer = xTaskGetTickCount() + 10000;
			drv_uart_clear_response();
			drv_uart_send_message("AT+UGRMC?\r");
			break;
		
		case DRV_LTE_WAIT:
			timer = xTaskGetTickCount() + 1000;
			network_registered = false;
			drv_uart_clear_response();
			drv_uart_send_message("AT+CREG?\r");
			break;
		
		case DRV_LTE_OPEN_SOCKET:
			timer = xTaskGetTickCount() + 1000;
			drv_uart_clear_response();
			drv_uart_send_message("AT+USOCR=17\r");
			break;
		
		case DRV_LTE_CLOSE_SOCKET:
		{
			timer = xTaskGetTickCount() + 120000;
			drv_uart_clear_response();
			drv_uart_send_message("AT+USOCL=");
			drv_uart_send_message(drv_lte_utoa(udp_socket_id));
			drv_uart_send_message("\r");
			udp_socket_id = -1;
			break;
		}
		
		case DRV_LTE_TRANSMIT_UDP_META:
		{
			timer = xTaskGetTickCount() + 10000;
			xSemaphoreTake(transmission_semaphore, 9999);
			drv_uart_clear_response();
			drv_uart_send_message("AT+USOST=");
			drv_uart_send_message(drv_lte_utoa(udp_socket_id));
			drv_uart_send_message(",\"");
			drv_uart_send_message(udp_message_destination);
			drv_uart_send_message("\",");
			drv_uart_send_message(drv_lte_utoa(udp_port));
			drv_uart_send_message(",");
			drv_uart_send_message(drv_lte_utoa(udp_message_length));
			drv_uart_send_message("\r");
			break;
		}
		
		case DRV_LTE_TRANSMIT_UDP_DATA:
		{
			timer = xTaskGetTickCount() + 10000;
			drv_uart_clear_response();
			drv_uart_send_data(udp_message_data, udp_message_length);
			break;
		}
			
		default:
			break;
	}
}

static void drv_lte_state_action()
{
	switch (current_state)
	{
		case DRV_LTE_WAIT:
			if (strstr(drv_uart_get_response_buffer(), "\r\nOK\r\n"))
			{
				drv_lte_parse_creg(drv_uart_get_response_buffer());
				drv_uart_clear_response();
				if (!network_registered)
					globalError = 4;
			}
			break;
		default:
			break;
	}
}

static enum drv_lte_state drv_lte_state_transition()
{
	enum drv_lte_state next_state = current_state;
	switch (current_state)
	{
		case DRV_LTE_POWER_TOGGLE_1:
			if (xTaskGetTickCount() >= timer)
			{
				next_state = DRV_LTE_POWER_TOGGLE_2;
			}
			break;
			
		case DRV_LTE_POWER_TOGGLE_2:
			if (xTaskGetTickCount() >= timer)
			{
				next_state = DRV_LTE_CONFIGURE_CMDECHO;
			}
			break;
			
		case DRV_LTE_CONFIGURE_CMDECHO:
			if (strstr(drv_uart_get_response_buffer(), "\r\nOK\r\n"))
			{
				next_state = DRV_LTE_CONFIGURE_GNSS_ENABLE;
				globalError = 0;
			}
			else if (xTaskGetTickCount() >= timer)
			{
				next_state = DRV_LTE_POWER_TOGGLE_1;
				globalError = 1;
			}
			break;
			
		case DRV_LTE_CONFIGURE_GNSS_ENABLE:
			if (strstr(drv_uart_get_response_buffer(), "\r\nOK\r\n")
				|| strstr(drv_uart_get_response_buffer(), "GPS aiding mode already set\r\n"))
			{
				next_state = DRV_LTE_CONFIGURE_GNSS_NMEA;
				globalError = 0;
			}
			else if (xTaskGetTickCount() >= timer)
			{
				next_state = DRV_LTE_INVALID;
				globalError = 1;
			}
			break;
		
		case DRV_LTE_CONFIGURE_GNSS_NMEA:
			if (strstr(drv_uart_get_response_buffer(), "\r\nOK\r\n"))
			{
				next_state = DRV_LTE_READ_GNSS;
				globalError = 0;
			}
			else if (strstr(drv_uart_get_response_buffer(), "ERROR"))
			{
				next_state = DRV_LTE_WAIT;
				globalError = 2;
			}
			else if (xTaskGetTickCount() >= timer)
			{
				next_state = DRV_LTE_INVALID;
				globalError = 1;
			}
			break;
		
		case DRV_LTE_READ_GNSS:
			if (strstr(drv_uart_get_response_buffer(), "Not available"))
			{
				next_state = DRV_LTE_WAIT;
				globalError = 3;
			}
			else if (strstr(drv_uart_get_response_buffer(), "\r\nOK\r\n"))
			{
				next_state = DRV_LTE_WAIT;
				globalError = 0;
				
				drv_lte_parse_nmea_rmc(drv_uart_get_response_buffer());
			}
			else if (strstr(drv_uart_get_response_buffer(), "ERROR"))
			{
				next_state = DRV_LTE_WAIT;
				globalError = 2;
			}
			else if (xTaskGetTickCount() >= timer)
			{
				next_state = DRV_LTE_INVALID;
				globalError = 1;
			}
			break;
		
		case DRV_LTE_WAIT:
			// Check for new location information periodically
			if (xTaskGetTickCount() >= timer)
			{
				next_state = DRV_LTE_READ_GNSS;
			}
			else if (message_to_send && network_registered)
			{
				if (udp_socket_id == -1)
				{
					next_state = DRV_LTE_OPEN_SOCKET;
				}
				else
				{
					next_state = DRV_LTE_TRANSMIT_UDP_META;
				}
			}
			break;
		
		case DRV_LTE_OPEN_SOCKET:
			if (strstr(drv_uart_get_response_buffer(), "\r\nOK\r\n"))
			{
				next_state = DRV_LTE_WAIT;
				globalError = 0;
				
				drv_lte_parse_usocr(drv_uart_get_response_buffer());
			}
			else if (xTaskGetTickCount() >= timer)
			{
				next_state = DRV_LTE_WAIT;
				globalError = 1;
			}
			break;
		
		case DRV_LTE_CLOSE_SOCKET:
		{
			if (strstr(drv_uart_get_response_buffer(), "\r\nOK\r\n"))
			{
				next_state = DRV_LTE_WAIT;
			}
			else if (strstr(drv_uart_get_response_buffer(), "ERROR"))
			{
				next_state = DRV_LTE_WAIT;
			}
			else if (xTaskGetTickCount() >= timer)
			{
				next_state = DRV_LTE_WAIT;
			}
			break;
		}
		
		case DRV_LTE_TRANSMIT_UDP_META:
		{
			if (strstr(drv_uart_get_response_buffer(), "@"))
			{
				next_state = DRV_LTE_TRANSMIT_UDP_DATA;
			}
			else if (strstr(drv_uart_get_response_buffer(), "ERROR") && strstr(drv_uart_get_response_buffer(), "\r\n\r\n"))
			{
				xSemaphoreGive(transmission_semaphore);
				next_state = DRV_LTE_WAIT;
				globalError = 6;
			}
			else if (xTaskGetTickCount() >= timer)
			{
				// need to reset the LTE module, can get stuck without @ response
				globalError = 1;
			}
			break;
		}
		
		case DRV_LTE_TRANSMIT_UDP_DATA:
		{
			if (strstr(drv_uart_get_response_buffer(), "\r\nOK\r\n"))
			{
				next_state = DRV_LTE_WAIT;
				globalError = 0;
				xSemaphoreGive(transmission_semaphore);
				message_to_send = false;
				
				drv_lte_parse_usost(drv_uart_get_response_buffer());
			}
			else if (strstr(drv_uart_get_response_buffer(), "ERROR"))
			{
				next_state = DRV_LTE_CLOSE_SOCKET;
				globalError = 2;
				xSemaphoreGive(transmission_semaphore);
				message_to_send = false;
			}
			else if (xTaskGetTickCount() >= timer)
			{
				next_state = DRV_LTE_CLOSE_SOCKET;
				globalError = 1;
				xSemaphoreGive(transmission_semaphore);
				message_to_send = false;
			}
			break;
		}

		default:
			break;
	}
	return next_state;
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
int parse_lat_lon(const char * s, bool lon)
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

static void drv_lte_parse_nmea_rmc(const char * s)
{
	struct drv_lte_location current = {0};
	struct drv_lte_time times = {0};
	char src[100];
	bool location_valid;
	
	strncpy(src, strstr(s, "$GPRMC") + 7, sizeof(src));
	
	int num = fields(src);
	if (num != 12) return;
	s = src;
	
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
		last_location_time = xTaskGetTickCount();
		last_location = current;
	}
	last_time_time = xTaskGetTickCount();
	last_time = times;
}

const struct drv_lte_location * drv_lte_get_last_location(void)
{
	if (last_location_time > 0)
	{
		return &last_location;
	}
	else
	{
		return NULL;
	}
}

const struct drv_lte_time * drv_lte_get_last_time(void)
{
	if (last_time_time > 0)
	{
		return &last_time;
	}
	else
	{
		return NULL;
	}
}

static void drv_lte_parse_creg(const char * s)
{
	s = strstr(s, "+CREG: ");
	if (!s) return;
	int n, stat;
	char src[20];
	
	strncpy(src, s + 7, sizeof(src));
	
	int num = fields(src);
	if (num != 2) return;
	s = src;
	
	n = atoi(s);
	s += strlen(s) + 1;
	stat = atoi(s);

	network_registered = (stat == 1 || stat == 5);
}

static void drv_lte_parse_usocr(const char * s)
{
	s = strstr(s, "+USOCR: ");
	if (!s) return;
	
	udp_socket_id = atoi(s + 8);
}

static void drv_lte_parse_usost(const char * s)
{
	s = strstr(s, "+USOST: ");
	if (!s) return;
	
	int sock, len;
	char src[20];
	
	strncpy(src, s + 8, sizeof(src));
	
	int num = fields(src);
	if (num != 2) return;
	s = src;
	
	sock = atoi(s);
	s += strlen(s) + 1;
	len = atoi(s);
	
	if (len != udp_message_length)
	{
		globalError = 5;
	}
}