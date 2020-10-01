/*
 * app_cantx.c
 *
 * Created: 7/26/2020 11:35:02 AM
 *  Author: connor
 */ 

#include "app_cantx.h"
#include "app_statusLight.h"
#include "app_sensorRead.h"
#include "drv_can.h"
#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#define ubyte uint8_t
#define uword uint16_t
#define ulong uint32_t
#include "2020.1.0.h"
#include "config.h"

static void app_cantx_populate_message(TimerHandle_t);

void app_cantx_init(void)
{
	int cycleTime;
	switch (PCBA_ID)
	{
		case PCBA_ID_SENSOR_BOARD_FRONT:
		{
			cycleTime = CYCLE_Front_Sensor;
			break;
		}
		default:
		{
			cycleTime = 10;
			break;
		}
	}
	xTimerStart(xTimerCreate("CANTX", cycleTime, pdTRUE, NULL, app_cantx_populate_message), 0);
}

void app_cantx_periodic(void)
{
}

static void app_cantx_populate_message(TimerHandle_t xTimerHandle)
{
	struct drv_can_tx_buffer_element * buffer = drv_can_get_tx_buffer((enum drv_can_tx_buffer_table)0U);
	switch (PCBA_ID)
	{
		case PCBA_ID_SENSOR_BOARD_FRONT:
		{
			INIT_Front_Sensor(buffer->DB);
			SET_Front_Sensor_Brake_Pressure_Front(buffer->DB,	sensorResults.results[DRV_ADC_CHANNEL_SENSE4]);
			SET_Front_Sensor_Pitot(buffer->DB,					sensorResults.results[DRV_ADC_CHANNEL_SENSE5]);
			SET_Front_Sensor_Steer_Position(buffer->DB,			sensorResults.results[DRV_ADC_CHANNEL_SENSE6]);
			break;
		}
		default:
		{
			break;
		}
	}
	drv_can_queue_tx_buffer(CAN1_REGS, (enum drv_can_tx_buffer_table)0U);
}
