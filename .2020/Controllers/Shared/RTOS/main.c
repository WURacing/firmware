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
#include "system_samc21.h"

#define DELAY_PERIOD 1U

xTaskHandle DriverTaskID;
void DriverTask(void *pvParameters);
xTaskHandle ApplicationTaskID;
void ApplicationTask(void *pvParameters);

int main(void)
{
    SystemInit();
	
	// increase flash wait states from 0 to 1. if this is not done, the
	// CPU has a seizure trying to run at 48MHz
	NVMCTRL_REGS->NVMCTRL_CTRLB = NVMCTRL_CTRLB_MANW(1) | NVMCTRL_CTRLB_RWS(1);
	// up speed to 48MHz
	OSCCTRL_REGS->OSCCTRL_OSC48MDIV = OSCCTRL_OSC48MDIV_DIV_DIV1;
	// wait for synchronization
    while (OSCCTRL_REGS->OSCCTRL_OSC48MSYNCBUSY) {}
    SystemCoreClock = 48000000;


// TODO: different priorities
	xTaskCreate(DriverTask, "DRV", configMINIMAL_STACK_SIZE + 1000, NULL, 1, &DriverTaskID);
	xTaskCreate(ApplicationTask, "APP", configMINIMAL_STACK_SIZE + 1000, NULL, 2, &ApplicationTaskID);
	
		drv_init();
			app_init();


	
	vTaskStartScheduler();
	while(1);
}

void DriverTask(void *pvParameters)
{
	TickType_t xLastWakeTime;

	xLastWakeTime = xTaskGetTickCount();
	
	
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
	
	
	while (1)
	{
		vTaskDelayUntil(&xLastWakeTime, DELAY_PERIOD);
		
		app_periodic();
	}
	
	vTaskDelete(NULL);
}


void vApplicationStackOverflowHook( TaskHandle_t xTask,
                                    signed char *pcTaskName )
{
	configASSERT(0);
}

__attribute__( (naked) )
void HardFault_Handler(void)
{
	configASSERT(0);
}