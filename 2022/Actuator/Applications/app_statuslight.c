#include "app_statuslight.h"
#include "FreeRTOS.h"
#include "task.h"
#include "sam.h"
#include "drv_can.h"
#include "drv_ws2812b.h"
#include "app_actuator.h"

static const uint8_t red[3] = {0, 255, 0};
static const uint8_t green[3] = {255, 0, 0};
static const uint8_t yellow[3] = {255, 255, 0};
static const uint8_t purple[3] = {0, 255, 255};
static const uint8_t off[3] = {0, 0, 0};


#define DELAY_PERIOD 5U
xTaskHandle StatusTaskID;
static void StatusTask()
{
	TickType_t xLastWakeTime;
	xLastWakeTime = xTaskGetTickCount();
	
	// >83 us for ws2812b bringup
	vTaskDelayUntil(&xLastWakeTime, 1);
	// wait a bit for dashboard to come online
	for (int i = 0; i < 3; ++i)
	{
		drv_ws2812b_transmit(DRV_WS2812B_CHANNEL_STATUS_LED, yellow, 3);
		vTaskDelayUntil(&xLastWakeTime, 300);
		drv_ws2812b_transmit(DRV_WS2812B_CHANNEL_STATUS_LED, off, 3);
		vTaskDelayUntil(&xLastWakeTime, 300);
	}
	while (1)
	{
		// Check if not received
		if (app_actuator_is_signal_missing())
		{
			// Is it because of a CAN error?
			int error = drv_can_read_lec(CAN0_REGS);

			if (error != 0) {
				// Pulse to indicate CAN error (1-6)
				for (int i = 0; i < error; ++i)
				{
					drv_ws2812b_transmit(DRV_WS2812B_CHANNEL_STATUS_LED, red, 3);
					vTaskDelayUntil(&xLastWakeTime, 300);
					drv_ws2812b_transmit(DRV_WS2812B_CHANNEL_STATUS_LED, off, 3);
					vTaskDelayUntil(&xLastWakeTime, 300);
				}
				// Sleep to allow user to count reliably
				vTaskDelayUntil(&xLastWakeTime, 1500);
			}
			else
			{
				// solid red for other error
				drv_ws2812b_transmit(DRV_WS2812B_CHANNEL_STATUS_LED, red, 3);
				vTaskDelayUntil(&xLastWakeTime, 300);
			}
		}
		else
		{
			// Flash green if no error
			drv_ws2812b_transmit(DRV_WS2812B_CHANNEL_STATUS_LED, green, 3);
			vTaskDelayUntil(&xLastWakeTime, 500);
			drv_ws2812b_transmit(DRV_WS2812B_CHANNEL_STATUS_LED, off, 3);
			vTaskDelayUntil(&xLastWakeTime, 1500);
		}
	}
	
	vTaskDelete(NULL);
}

void app_statuslight_init(void)
{
	xTaskCreate(StatusTask, "STATUS", configMINIMAL_STACK_SIZE + 1000, NULL, 1, &StatusTaskID);
}

void vApplicationStackOverflowHook(TaskHandle_t xTask,
								   signed char *pcTaskName)
{
	(void)xTask;
	(void)pcTaskName;
	while (1)
	{
		drv_ws2812b_transmit(DRV_WS2812B_CHANNEL_STATUS_LED, purple, 3);
		for (int i = 0; i < (48000000/1000); ++i)
			asm("nop\r\n");
	}
}

__attribute__((naked))
void HardFault_Handler(void)
{
	while (1)
	{
		drv_ws2812b_transmit(DRV_WS2812B_CHANNEL_STATUS_LED, purple, 3);
		for (int i = 0; i < (48000000/1000); ++i)
			asm("nop\r\n");
	}
}