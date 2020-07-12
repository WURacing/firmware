/*
 * app_cantx.c
 *
 * Created: 7/26/2020 11:35:02 AM
 *  Author: connor
 */ 

#include "app_cantx.h"
#include "app_statusLight.h"
#include "drv_can.h"
#include "FreeRTOS.h"
#include "task.h"

static void app_cantx_populate_error(void);

void app_cantx_init(void)
{
}

void app_cantx_periodic(void)
{
	TickType_t time = xTaskGetTickCount();
	
	if ((time % 1000) == 0)
	{
		app_cantx_populate_error();
	}
}

static void app_cantx_populate_error(void)
{
	struct drv_can_tx_buffer_element * buffer = drv_can_get_tx_buffer(DRV_CAN_TX_BUFFER_ERROR);
	buffer->DB[0] = (uint8_t) globalError;
	drv_can_queue_tx_buffer(DRV_CAN_TX_BUFFER_ERROR);
}