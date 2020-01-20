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
	system_init();

	/* Insert application code here, after the board has been initialized. */
	
	xTaskCreate(sd_task, "SD writing", configMINIMAL_STACK_SIZE + 1000, NULL, tskIDLE_PRIORITY + 1, &sd_task_id);
	
	vTaskStartScheduler();
	while(1);
}
