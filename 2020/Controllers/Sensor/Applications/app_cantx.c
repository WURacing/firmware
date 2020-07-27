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

static void app_cantx_populate_error(void);
static void app_cantx_populate_sens1(void);
static void app_cantx_populate_sens2(void);

void app_cantx_init(void)
{
}

void app_cantx_periodic(void)
{
	TickType_t time = xTaskGetTickCount();
	
	if ((time % 1000) == 0) //1Hz
	{
		app_cantx_populate_error();
	}
	if ((time % 10) == 0)   //100Hz
	{
		app_cantx_populate_sens1();
		app_cantx_populate_sens2();
	}
}

static void app_cantx_populate_error(void)
{
	struct drv_can_tx_buffer_element * buffer = drv_can_get_tx_buffer(DRV_CAN_TX_BUFFER_ERROR);
	buffer->DB[0] = (uint8_t) globalError;
	drv_can_queue_tx_buffer(DRV_CAN_TX_BUFFER_ERROR);
}

static void app_cantx_populate_sens1(void)
{
	struct drv_can_tx_buffer_element * buffer = drv_can_get_tx_buffer(DRV_CAN_TX_BUFFER_SENS_1);
	(*(uint16_t *) &buffer->DB[0]) = sensorResults[0];
	(*(uint16_t *) &buffer->DB[2]) = sensorResults[1];
	(*(uint16_t *) &buffer->DB[4]) = sensorResults[2];
	(*(uint16_t *) &buffer->DB[6]) = sensorResults[3];
	drv_can_queue_tx_buffer(DRV_CAN_TX_BUFFER_SENS_1);
}

static void app_cantx_populate_sens2(void)
{
	struct drv_can_tx_buffer_element * buffer = drv_can_get_tx_buffer(DRV_CAN_TX_BUFFER_SENS_2);
	(*(uint16_t *) &buffer->DB[0]) = sensorResults[4];
	(*(uint16_t *) &buffer->DB[2]) = sensorResults[5];
	drv_can_queue_tx_buffer(DRV_CAN_TX_BUFFER_SENS_2);
}