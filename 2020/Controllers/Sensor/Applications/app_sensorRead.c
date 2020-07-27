/*
 * app_sensorRead.c
 *
 * Created: 7/26/2020 7:38:57 PM
 *  Author: connor
 */ 
#include "app_sensorRead.h"
#include "drv_adc.h"

uint16_t sensorResults[DRV_ADC_SEQUENCE_COUNT];

void app_sensorRead_init(void)
{
	
}

void app_sensorRead_periodic(void)
{
	drv_adc_read_sequence_sync(sensorResults);
}