/*
 * drv_lte.c
 *
 * Created: 8/8/2020 11:14:01 AM
 *  Author: connor
 */ 

#include "drv_lte.h"
#include "drv_uart.h"
#include "app_statusLight.h"
#include "sam.h"
#include "FreeRTOS.h"
#include "task.h"
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
	DRV_LTE_WAIT,
	
	DRV_LTE_COUNT,
	DRV_LTE_INVALID
} current_state;

struct drv_lte_location
{
	double latitude, longitude, speed, angle;
	int year, month, day, hour, minute, second;
};

static struct drv_lte_location last_location;
static TickType_t last_location_time;


static TickType_t timer;

static void drv_lte_state_entry(void);
static void drv_lte_state_action(void);
static enum drv_lte_state drv_lte_state_transition(void);
static void drv_lte_parse_nmea_rmc(const char *);

void drv_lte_init()
{
	last_location_time = 0;
	PORT->Group[LTE_POWER_GROUP].DIRSET.reg = LTE_POWER_PORT;
	
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
			drv_uart_send_message("ATE0\r\n");
			break;
			
		case DRV_LTE_CONFIGURE_GNSS_ENABLE:
			timer = xTaskGetTickCount() + 10000;
			drv_uart_clear_response();
			drv_uart_send_message("AT+UGPS=1,0,1\r\n");
			break;
			
		case DRV_LTE_CONFIGURE_GNSS_NMEA:
			timer = xTaskGetTickCount() + 10000;
			drv_uart_clear_response();
			drv_uart_send_message("AT+UGRMC=1\r\n");
			break;
			
		case DRV_LTE_READ_GNSS:
			timer = xTaskGetTickCount() + 10000;
			drv_uart_clear_response();
			drv_uart_send_message("AT+UGRMC?\r\n");
			break;
		
		case DRV_LTE_WAIT:
			timer = xTaskGetTickCount() + 1000;
			break;
			
		default:
			break;
	}
}

static void drv_lte_state_action()
{
	
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
				next_state = DRV_LTE_INVALID;
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
		
		case DRV_LTE_WAIT:
			// Check for new location information periodically
			if (xTaskGetTickCount() >= timer)
			{
				next_state = DRV_LTE_READ_GNSS;
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
		src = strchr(src, ','); // find next comma
		if (src == NULL) break;
		*src = '\0'; // terminate string there
		src++; // move to start char of next field
		num++; // inc fields
	}
	return num;
}

/************************************************************************/
/* Parses a string like either (lon=0) XXYY.Z+ or (lon=1) XXXYY.Z+      */
/************************************************************************/
static double parse_lat_lon(const char * s, bool lon)
{
	int degdigits = lon ? 3 : 2;
	char deg_field[4];
	char min_int_field[3];
	char min_frac_field[10];
	const char * decpt = strchr(s, '.');
	if (!decpt) return NAN;
	if ((decpt - s) != degdigits + 2) return NAN;
	int fracdigits = strlen(decpt + 1);
	strncpy(deg_field, s, degdigits);
	strncpy(min_int_field, s + degdigits, 2);
	strncpy(min_frac_field, decpt + 1, sizeof(min_frac_field));
	for (int i = fracdigits; i < 9; i++)
	{
		min_frac_field[i] = '0';
	}
	min_frac_field[9] = '\0';
	return atoi(deg_field) + (atoi(min_int_field) + atoi(min_frac_field) / 1000000000.f) / 60.f;
}

static void twotwotwo(const char *s, int *one, int *two, int *three)
{
	char digits[3];
	
	*one = *two = *three = 0;
	if (strlen(s) < 6) return;
	
	strncpy(digits, s, 2);
	*one = atoi(digits);
	strncpy(digits, s + 2, 2);
	*two = atoi(digits);
	strncpy(digits, s + 4, 2);
	*three = atoi(digits);
}

static void drv_lte_parse_nmea_rmc(const char * s)
{
	struct drv_lte_location current = {0};
	char src[100];
	
	strncpy(src, strstr(s, "$GPRMC") + 7, 100);
	
	int num = fields(src);
	if (num != 12) return;
	s = src;
	
	// Field 1: timestamp
	twotwotwo(s, &current.hour, &current.minute, &current.second);
	s += strlen(s) + 1;
	// Field 2: active/void
	if (*s != 'A') return;
	s += strlen(s) + 1;
	// Field 3: latitude
	current.latitude = parse_lat_lon(s, false);
	s += strlen(s) + 1;
	// Field 4: latitude direction
	if (*s == 'S')
	{
		current.latitude *= -1.f;
	}
	s += strlen(s) + 1;
	// Field 5: longitude
	current.longitude = parse_lat_lon(s, true);
	s += strlen(s) + 1;
	// Field 6: longitude direction
	if (*s == 'W')
	{
		current.longitude *= -1.f;
	}
	s += strlen(s) + 1;
	// Field 7: speed
	s += strlen(s) + 1;
	// Field 8: angle
	s += strlen(s) + 1;
	// Field 9: date
	twotwotwo(s, &current.day, &current.month, &current.year);
	
	last_location_time = xTaskGetTickCount();
	last_location = current;
}