/*
 * main.c
 * Initializes the RTOS and manages periodic tasks.
 *
 * Created: 7/11/2020 4:59:30 PM
 * Author : connor
 */ 


#include "sam.h"
#include "FreeRTOS.h"
#include "task.h"
#include "drv_main.h"
#include "app_main.h"

#define DELAY_PERIOD 1U

xTaskHandle DriverTaskID;
void DriverTask(void *pvParameters);
xTaskHandle ApplicationTaskID;
void ApplicationTask(void *pvParameters);

int main(void)
{
    SystemInit();

	xTaskCreate(DriverTask, "DRV", configMINIMAL_STACK_SIZE + 1000, NULL, 1, &DriverTaskID);
	xTaskCreate(ApplicationTask, "APP", configMINIMAL_STACK_SIZE + 1000, NULL, 1, &ApplicationTaskID);
	
	vTaskStartScheduler();
	while(1);
}

void DriverTask(void *pvParameters)
{
	TickType_t xLastWakeTime;

	xLastWakeTime = xTaskGetTickCount();
	
	drv_init();
	
	while (1)
	{
		vTaskDelayUntil(&xLastWakeTime, DELAY_PERIOD);
		
		drv_periodic();
	}
	
	vTaskDelete(NULL);
}


void ApplicationTask(void *pvParameters)
{
	TickType_t xLastWakeTime;

	xLastWakeTime = xTaskGetTickCount();
	
	app_init();
	
	while (1)
	{
		vTaskDelayUntil(&xLastWakeTime, DELAY_PERIOD);
		
		app_periodic();
	}
	
	vTaskDelete(NULL);
}
