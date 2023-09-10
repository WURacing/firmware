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
#include "drv_eic.h"

static void app_cantx_populate_message(TimerHandle_t);

void app_cantx_init(void)
{
	int cycleTime;
#if (PCBA_ID == PCBA_ID_SENSOR_BOARD_FRONT)
	cycleTime = CYCLE_Front_Sensor;
#elif (PCBA_ID == PCBA_ID_SENSOR_BOARD_REAR)
	cycleTime = CYCLE_Rear_Sensor;
#elif (PCBA_ID == PCBA_ID_SENSOR_BOARD_CG)
	cycleTime = CYCLE_CG_Sensor;
#elif (PCBA_ID == PCBA_ID_SENSOR_BOARD_FL_UPRIGHT)
	cycleTime = CYCLE_FL_Upright;
#elif (PCBA_ID == PCBA_ID_SENSOR_BOARD_FR_UPRIGHT)
	cycleTime = CYCLE_FR_Upright;
#elif (PCBA_ID == PCBA_ID_SENSOR_BOARD_RL_UPRIGHT)
	cycleTime = CYCLE_RL_Upright;
#elif (PCBA_ID == PCBA_ID_SENSOR_BOARD_RR_UPRIGHT)
	cycleTime = CYCLE_RR_Upright;
#endif
	xTimerStart(xTimerCreate("CANTX", cycleTime, pdTRUE, NULL, app_cantx_populate_message), 0);
}

void app_cantx_periodic(void)
{
}

static void app_cantx_populate_message(TimerHandle_t xTimerHandle)
{
	(void)xTimerHandle;
	
	struct drv_can_tx_buffer_element * buffer = drv_can_get_tx_buffer((enum drv_can_tx_buffer_table)0U);
	#if (PCBA_ID == PCBA_ID_SENSOR_BOARD_FRONT)
	INIT_Front_Sensor(buffer->DB);
	SET_Front_Sensor_Brake_Pressure_Front(buffer->DB, sensorResults.results[DRV_ADC_CHANNEL_SENSE4]);
	SET_Front_Sensor_Pitot(buffer->DB, sensorResults.results[DRV_ADC_CHANNEL_SENSE5]);
	SET_Front_Sensor_Steer_Position(buffer->DB, sensorResults.results[DRV_ADC_CHANNEL_SENSE6]);
#elif (PCBA_ID == PCBA_ID_SENSOR_BOARD_REAR)
#elif (PCBA_ID == PCBA_ID_SENSOR_BOARD_CG)
#elif (PCBA_ID == PCBA_ID_SENSOR_BOARD_FL_UPRIGHT)
	INIT_FL_Upright(buffer->DB);
	SET_FL_Upright_Accel_X_FL(buffer->DB,		sensorResults.results[DRV_ADC_CHANNEL_ACCEL_X]);
	SET_FL_Upright_Accel_Y_FL(buffer->DB,		sensorResults.results[DRV_ADC_CHANNEL_ACCEL_Y]);
	SET_FL_Upright_Brake_Temp_FL(buffer->DB,	sensorResults.results[DRV_ADC_CHANNEL_BRK_TMP]);
	SET_FL_Upright_Wheel_Speed_FL(buffer->DB,	clicks_per_second);
#elif (PCBA_ID == PCBA_ID_SENSOR_BOARD_FR_UPRIGHT)
#elif (PCBA_ID == PCBA_ID_SENSOR_BOARD_RL_UPRIGHT)
#elif (PCBA_ID == PCBA_ID_SENSOR_BOARD_RR_UPRIGHT)
#endif
#if GENERAL_PURPOSE
	drv_can_queue_tx_buffer(CAN1_REGS, (enum drv_can_tx_buffer_table)0U);
#elif UPRIGHT
	drv_can_queue_tx_buffer(CAN0_REGS, (enum drv_can_tx_buffer_table)0U);
#endif
}
