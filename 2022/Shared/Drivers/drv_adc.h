#pragma once
#include "drv_adc_private.h"
#include <stdint.h>
#include "sam.h"

struct drv_adc_channelConfig {
	uint16_t adc_id:1;
	uint16_t mux:5;
	uint32_t pinmux;
};

struct drv_adc_config {
	const struct drv_adc_channelConfig * channelConfig;
};
extern struct drv_adc_config drv_adc_config;

struct drv_adc_results {
	uint16_t results[DRV_ADC_CHANNEL_COUNT];
	int error;
};

void drv_adc_init(void);
uint16_t drv_adc_read(int channel); // DOSN"T WORK IF A SEQ IS CONFIGURED
void drv_adc_read_sequence_sync(struct drv_adc_results * results);
