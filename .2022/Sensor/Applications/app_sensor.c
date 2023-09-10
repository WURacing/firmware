#include "app_sensor.h"
#include "FreeRTOS.h"
#include "task.h"
#include "sam.h"
#include "drv_adc.h"
#include "drv_hsd.h"
#include "drv_can.h"

static struct drv_adc_results results;
static enum drv_hsd_channel currentChannel;

#define NUM_ANALOG 6

static struct measurements {
	uint16_t analogSensors[NUM_ANALOG];
	uint16_t individualCurrent[DRV_HSD_CHANNEL_COUNT];
	uint16_t totalCurrent;
	bool individualFaults[DRV_HSD_CHANNEL_COUNT];
	bool totalFaults;
} measurements;

#define DELAY_PERIOD 5U
xTaskHandle MeasurementTaskID;
static void MeasurementTask()
{
	TickType_t xLastWakeTime;
	int i;
	struct drv_can_tx_buffer_element * buffer;
	struct signals1_layout * signals1;
	struct signals2_layout * signals2;
	xLastWakeTime = xTaskGetTickCount();
	currentChannel = (enum drv_hsd_channel)0U;
	
	while (1)
	{
		// Fault recovery
		if (drv_can_is_bus_off(CAN0_REGS))
		{
			drv_can_recover_from_bus_off(CAN0_REGS);
		}
		// Kick off ADC measurement and block
		// This takes 3.4ms to return with ADC_AVGCTRL at 512.
		drv_adc_read_sequence_sync(&results);
		
		// Copy results for analog sensors
		for (i = 0; i < NUM_ANALOG; ++i)
			measurements.analogSensors[i] = results.results[i];
		
		// Interpret current sense results
		if (results.results[DRV_ADC_CHANNEL_ISENSE] > 60000)
		{
			measurements.individualCurrent[currentChannel] = 0;
			measurements.individualFaults[currentChannel] = true;
		}
		else
		{
			measurements.individualCurrent[currentChannel] = results.results[DRV_ADC_CHANNEL_ISENSE];
			measurements.individualFaults[currentChannel] = false;
		}
		measurements.totalFaults = drv_hsd_isFaulted();
		
		// Calculate total current
		for (i = 0, measurements.totalCurrent = 0; i < DRV_HSD_CHANNEL_COUNT; ++i)
			measurements.totalCurrent += measurements.individualCurrent[i];
		
		// Read the next output channel next time
		if (++currentChannel >= DRV_HSD_CHANNEL_COUNT)
			currentChannel = (enum drv_hsd_channel)0U;
		drv_hsd_setChannel(currentChannel);
		
		// Queue tx
		buffer = drv_can_get_tx_buffer(0);
		if (buffer)
		{
			signals1 = (struct signals1_layout *)buffer->DB;
			signals1->analog1 = measurements.analogSensors[0];
			signals1->analog2 = measurements.analogSensors[1];
			signals1->analog3 = measurements.analogSensors[2];
			signals1->analog4 = measurements.analogSensors[3];
			drv_can_queue_tx_buffer(CAN0_REGS, 0);
		}
		buffer = drv_can_get_tx_buffer(1);
		if (buffer)
		{
			signals2 = (struct signals2_layout *)buffer->DB;
			signals2->analog5 = measurements.analogSensors[4];
			signals2->analog6 = measurements.analogSensors[5];
			signals2->fault1 = measurements.individualFaults[DRV_HSD_CHANNEL_SENSOR_1];
			signals2->fault2 = measurements.individualFaults[DRV_HSD_CHANNEL_SENSOR_2];
			signals2->fault3 = measurements.individualFaults[DRV_HSD_CHANNEL_SENSOR_3];
			signals2->fault4 = measurements.individualFaults[DRV_HSD_CHANNEL_SENSOR_456];
			signals2->allFaults = measurements.totalFaults;
			signals2->totalCurrent = measurements.totalCurrent;
			drv_can_queue_tx_buffer(CAN0_REGS, 1);
		}

		// Wait until next scheduled measurement time
		vTaskDelayUntil(&xLastWakeTime, DELAY_PERIOD);
	}
	
	vTaskDelete(NULL);
}

void app_sensor_init(void)
{
	xTaskCreate(MeasurementTask, "SENSOR", configMINIMAL_STACK_SIZE + 1000, NULL, 1, &MeasurementTaskID);
}

