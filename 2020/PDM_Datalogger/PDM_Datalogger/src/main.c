/**
 * \file
 *
 * \brief Empty user application template
 *
 */

/**
 * \mainpage User Application template doxygen documentation
 *
 * \par Empty user application template
 *
 * Bare minimum empty user application template
 *
 * \par Content
 *
 * -# Include the ASF header files (through asf.h)
 * -# Minimal main function that starts with a call to system_init()
 * -# "Insert application code here" comment
 *
 */

/*
 * Include header files for all drivers that have been imported from
 * Atmel Software Framework (ASF).
 */
/*
 * Support and FAQ: visit <a href="https://www.microchip.com/support/">Microchip Support</a>
 */
#include <asf.h>
#include "exports.h"


int main (void)
{
	//sysclk_init();
	system_init();
	delay_init();
	
	vSemaphoreCreateBinary(new_data_semaphore);
	
	vQueueAddToRegistry(new_data_semaphore, "data");
	
	configure_can();
	
	sd_mmc_init();

	/* Insert application code here, after the board has been initialized. */
	
	xTaskCreate(sd_task, "SD", configMINIMAL_STACK_SIZE + 1000, NULL, tskIDLE_PRIORITY + 1, &sd_task_id);
	
	vTaskStartScheduler();
	while(1);
}
