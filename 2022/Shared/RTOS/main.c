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
#include "system_samc21.h"

#define DELAY_PERIOD 1U

void drv_init(void);
void app_init(void);

int main(void)
{
	SystemInit();
	
	drv_init();
	app_init();

	vTaskStartScheduler();
	while (1);
}

void vApplicationStackOverflowHook(TaskHandle_t xTask,
								   signed char *pcTaskName)
{
	(void)xTask;
	(void)pcTaskName;
	configASSERT(0);
}

__attribute__((naked))
void HardFault_Handler(void)
{
	configASSERT(0);
}