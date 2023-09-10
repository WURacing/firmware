/*
 * app_cantx.c
 *
 * Created: 7/26/2020 11:35:02 AM
 *  Author: connor
 */ 

#include "app_cantx.h"
#include "app_statusLight.h"
#include "app_telemetry.h"
#include "drv_can.h"
#include "drv_lte.h"
#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"

#define ubyte uint8_t
#define uword uint16_t
#define ulong uint32_t
#include "2020.1.0.h"


static void app_cantx_populate_GPS_time(TimerHandle_t xTimer)
{
	const struct drv_lte_time * timestamp = drv_lte_get_last_time();
	if (timestamp != NULL)
	{
		struct drv_can_tx_buffer_element * buffer = drv_can_get_tx_buffer(DRV_CAN_TX_BUFFER_CAN0_GPS_time);
		INIT_GPS_time(buffer->DB);
		SET_GPS_time_YEAR(buffer->DB,	timestamp->year);
		SET_GPS_time_MONTH(buffer->DB,	timestamp->month);
		SET_GPS_time_DAY(buffer->DB,	timestamp->day);
		SET_GPS_time_HOUR(buffer->DB,	timestamp->hour);
		SET_GPS_time_MINUTE(buffer->DB, timestamp->minute);
		SET_GPS_time_SECOND(buffer->DB, timestamp->second);
		drv_can_queue_tx_buffer(CAN0_REGS, DRV_CAN_TX_BUFFER_CAN0_GPS_time);
	}
}

static void app_cantx_populate_GPS_POS(TimerHandle_t xTimer)
{
	const struct drv_lte_location * location = drv_lte_get_last_location();
	if (location != NULL)
	{
		struct drv_can_tx_buffer_element * buffer = drv_can_get_tx_buffer(DRV_CAN_TX_BUFFER_CAN0_GPS_POS);
		INIT_GPS_POS(buffer->DB);
		SET_GPS_POS_GPS_Lat(buffer->DB,		location->latitude);
		SET_GPS_POS_GPS_Long(buffer->DB,	location->longitude);
		drv_can_queue_tx_buffer(CAN0_REGS, DRV_CAN_TX_BUFFER_CAN0_GPS_POS);
	}
}

static void app_cantx_populate_Telem_Stat(TimerHandle_t xTimer)
{
	struct drv_can_tx_buffer_element * buffer = drv_can_get_tx_buffer(DRV_CAN_TX_BUFFER_CAN0_Telem_Stat);
	INIT_Telem_Stat(buffer->DB);
	SET_Telem_Stat_Telem_OK(buffer->DB,						(globalError == 0) ? Telem_Stat_Telem_OK_OK : Telem_Stat_Telem_OK_ERROR);
	SET_Telem_Stat_Telem_CAN_Packets_Sent(buffer->DB,		app_telemetry_sent_messages());
	SET_Telem_Stat_Telem_CAN_Unique_IDs_Sent(buffer->DB,	DRV_CAN_RX_BUFFER_COUNT);
	drv_can_queue_tx_buffer(CAN0_REGS, DRV_CAN_TX_BUFFER_CAN0_Telem_Stat);
}

void app_cantx_init(void)
{
	APP_CANTX_START_TIMER(GPS_time);
	APP_CANTX_START_TIMER(GPS_POS);
	APP_CANTX_START_TIMER(Telem_Stat);
}

void app_cantx_periodic(void)
{
}