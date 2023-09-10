#include <string.h>

#include "app_datalogger.h"
#include "app_data.h"
#include "FreeRTOS.h"
#include "task.h"
#include "drv_can.h"
#include <time.h>
#include <stdio.h>
#include "ff.h"
#include "diskio.h"

#define SYNC_INTERVAL 1000
#define DELAY_PERIOD 10
#define STACK_SIZE 1000
#define APP_DATALOGGER_PRIORITY 2

static struct {
	FATFS fs;
	FIL fp;
	bool file_opened;
	bool data_read;
	TickType_t last_sync;
	TickType_t last_write;
	struct servo_config servo_config;
	StaticTask_t rtos_task_id;
	StackType_t  rtos_stack[STACK_SIZE];
} app_datalogger_data = {
	.servo_config = {
		.eARBFrontPulses = {
			-128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, 
		},
		.eARBRearPulses = {
			-128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, 
		},
		.drsClosedPulse = -128,
		.drsOpenPulse = -128,
	}
};


static bool open_file(void)
{
	int i;
	FRESULT fresult;
	char name[15];
	app_datalogger_data.file_opened = false;
	
	for (i = 1; i < 99999; ++i)
	{
		snprintf(name, 15, "log%05d.csv", i);
		fresult = f_open(&app_datalogger_data.fp, name, FA_WRITE | FA_CREATE_NEW);
		if (fresult == FR_OK)
		{
			app_datalogger_data.file_opened = true;
			return true;
		}
		else if (fresult != FR_EXIST)
		{
			break;
		}
	}
	return false;
}

static bool read_data_files(void)
{
	FRESULT fresult;
	char line[10];
	int i;
	
	// ARB Front
	fresult = f_open(&app_datalogger_data.fp, "CONTROL/ARBFRONT.TXT", FA_READ);
	if (fresult != FR_OK) return false;
	i = 0;
	while (f_gets(line, sizeof line, &app_datalogger_data.fp) && i < SERVO_POSITIONS) {
		app_datalogger_data.servo_config.eARBFrontPulses[i++] = atoi(line);
    }
	f_close(&app_datalogger_data.fp);
	if (i < SERVO_POSITIONS) return false;

	// ARB Rear
	fresult = f_open(&app_datalogger_data.fp, "CONTROL/ARBREAR.TXT", FA_READ);
	if (fresult != FR_OK) return false;
	i = 0;
	while (f_gets(line, sizeof line, &app_datalogger_data.fp) && i < SERVO_POSITIONS) {
        app_datalogger_data.servo_config.eARBRearPulses[i++] = atoi(line);
    }
	f_close(&app_datalogger_data.fp);
	if (i < SERVO_POSITIONS) return false;
	
	// DRS
	fresult = f_open(&app_datalogger_data.fp, "CONTROL/DRS.TXT", FA_READ);
	if (fresult != FR_OK) return false;
	i = 0;
	if (f_gets(line, sizeof line, &app_datalogger_data.fp)) {
		app_datalogger_data.servo_config.drsClosedPulse = atoi(line); i++;
		if (f_gets(line, sizeof line, &app_datalogger_data.fp)) {
			app_datalogger_data.servo_config.drsOpenPulse = atoi(line); i++;
		}
	}
	f_close(&app_datalogger_data.fp);
	if (i < 2) return false;

	
	app_datalogger_data.data_read = true;
	return true;
}

static void app_datalogger_task()
{
	TickType_t xLastWakeTime = xTaskGetTickCount();
	f_mount(&app_datalogger_data.fs, "", 0);
	while (1)
	{	
		if (app_datalogger_data.file_opened)
		{
			// We are in data logging mode
			
			if ((xLastWakeTime - app_datalogger_data.last_sync) > SYNC_INTERVAL)
			{
				// Force writing out of data to the SD card periodically, to prevent loss
				app_datalogger_data.last_sync = xLastWakeTime;
				if (f_sync(&app_datalogger_data.fp) != FR_OK) goto handle_error;
			}
			
			// Keep reading messages from the FIFO
			int fres = 0;
			const struct app_data_message * message;
			while ((message = app_data_pop_fifo()) != NULL)
			{
				// Format and write to file
				fres = f_printf(&app_datalogger_data.fp, "%08d,%08x,%02x%02x%02x%02x%02x%02x%02x%02x\r\n",
							 message->timestamp_ms,
							 message->id,
							 message->data[0],
							 message->data[1],
							 message->data[2],
							 message->data[3],
							 message->data[4],
							 message->data[5],
							 message->data[6],
							 message->data[7]);
				if (fres < 0) goto handle_error;
				app_datalogger_data.last_write = xLastWakeTime;
			}
		}
		else
		{
			// We have not yet transitioned to data logging mode
			bool result;
			if (! app_datalogger_data.data_read)
			{
				// First, try to read the CONTROL files
				result = read_data_files();
			}
			else
			{
				// Second, try to open a data logging file
				result = open_file();
				if (result)
					f_puts("time,id,data\r\n", &app_datalogger_data.fp);
			}
			if (!result)
			{
				// could not open file, SD card might not be inserted
				// wait 100ms before trying again
				while (xTaskGetTickCount() - xLastWakeTime < 100)
				{
					// but keep pulling messages through the FIFO. this is necessary to update display
					while (app_data_pop_fifo() != NULL);
					vTaskDelay(10);
				}
			}
		}
		
		vTaskDelayUntil(&xLastWakeTime, DELAY_PERIOD);
		continue;
handle_error:
		app_datalogger_data.file_opened = false;
		f_close(&app_datalogger_data.fp);
	}
	vTaskDelete(NULL);
}

void app_datalogger_init(void)
{
	xTaskCreateStatic(app_datalogger_task, "DL", STACK_SIZE, NULL, APP_DATALOGGER_PRIORITY, app_datalogger_data.rtos_stack, &app_datalogger_data.rtos_task_id);
}

bool app_datalogger_okay(void)
{
	return app_datalogger_data.file_opened && (xTaskGetTickCount() - app_datalogger_data.last_write) < 1000;
}

bool app_datalogger_read_data(void)
{
	return app_datalogger_data.data_read;
}

const struct servo_config * app_datalogger_get_servo_positions(void)
{
	return &app_datalogger_data.servo_config;
}