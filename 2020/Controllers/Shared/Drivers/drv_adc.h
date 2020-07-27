#pragma once
#include "drv_adc_private.h"
#include <stdint.h>

struct drv_adc_config {
	const int * sequence_map;
};
extern struct drv_adc_config drv_adc_config;

void drv_adc_init(void);
uint16_t drv_adc_read(int channel); // DOSN"T WORK IF A SEQ IS CONFIGURED
void drv_adc_read_sequence_sync(uint16_t * results);