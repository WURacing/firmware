#include "app_telemetry.h"
#include "app_data.h"
#include "FreeRTOS.h"
#include "task.h"
#include "drv_can.h"
#include "drv_lte.h"
#include <time.h>
#include <stdio.h>

#define DELAY_PERIOD 500
#define PACKET_SIZE 27
#define PACKETS_PER_MESSAGE 10
#define STR_SIZE (PACKETS_PER_MESSAGE * PACKET_SIZE + 1)
#define STACK_SIZE 512
#define APP_TELEMETRY_PRIORITY 1

static struct {
	struct app_data_message message;
	char str[STR_SIZE];
	StaticTask_t rtos_task_id;
	StackType_t  rtos_stack[STACK_SIZE];
} app_telemetry_data;

static void app_telemetry_task()
{
	while (!drv_lte_configure());
	
	TickType_t xLastWakeTime = xTaskGetTickCount();
	while (1)
	{	
		if (drv_lte_is_logged_in())
		{
			// send data
			int strpos = 0;
			int i = 0;
			while (app_data_read_buffer(i++, &app_telemetry_data.message))
			{
				if ((xLastWakeTime - app_telemetry_data.message.timestamp_ms) > DELAY_PERIOD)
				{
					// stale message
					continue;
				}
				strpos += snprintf(app_telemetry_data.str+strpos, STR_SIZE-strpos, "%08x=%02x%02x%02x%02x%02x%02x%02x%02x ",
							 (unsigned int)app_telemetry_data.message.id,
							 app_telemetry_data.message.data[0],
							 app_telemetry_data.message.data[1],
							 app_telemetry_data.message.data[2],
							 app_telemetry_data.message.data[3],
							 app_telemetry_data.message.data[4],
							 app_telemetry_data.message.data[5],
							 app_telemetry_data.message.data[6],
							 app_telemetry_data.message.data[7]);
				if (STR_SIZE - strpos - 1 <= PACKET_SIZE)
				{
					// out of space, break into multiple transmissions
					drv_lte_mqtt_publish("CAN", app_telemetry_data.str);
					strpos = 0;
				}

			}
			if (strpos>0) drv_lte_mqtt_publish("CAN", app_telemetry_data.str);
		}
		else
		{
			if (drv_lte_is_network_registered())
			{
				drv_lte_mqtt_login();
			}
		}
		vTaskDelayUntil(&xLastWakeTime, DELAY_PERIOD);
	}
	vTaskDelete(NULL);
}

void app_telemetry_init(void)
{
	xTaskCreateStatic(app_telemetry_task, "TEL", STACK_SIZE, NULL, APP_TELEMETRY_PRIORITY, app_telemetry_data.rtos_stack, &app_telemetry_data.rtos_task_id);
}
