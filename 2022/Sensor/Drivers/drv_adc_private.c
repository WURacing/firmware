/*
 * drv_adc_private.c
 *
 * Created: 7/26/2020 7:08:29 PM
 *  Author: connor
 */ 
#include "drv_adc.h"
#include "sam.h"

static const struct drv_adc_channelConfig channelConfig[DRV_ADC_CHANNEL_COUNT] = {
	// Items on ADC0
	[DRV_ADC_CHANNEL_SENSE1] = {
		.adc_id = 0,
		.mux = ADC_INPUTCTRL_MUXPOS_AIN0_Val,
	},
	[DRV_ADC_CHANNEL_SENSE2] = {
		.adc_id = 0,
		.mux = ADC_INPUTCTRL_MUXPOS_AIN1_Val,
	},
	[DRV_ADC_CHANNEL_SENSE3] = {
		.adc_id = 0,
		.mux = ADC_INPUTCTRL_MUXPOS_AIN4_Val,
	},
	[DRV_ADC_CHANNEL_SENSE4] = {
		.adc_id = 0,
		.mux = ADC_INPUTCTRL_MUXPOS_AIN5_Val,
	},
	[DRV_ADC_CHANNEL_SENSE5] = {
		.adc_id = 0,
		.mux = ADC_INPUTCTRL_MUXPOS_AIN6_Val,
	},
	[DRV_ADC_CHANNEL_SENSE6] = {
		.adc_id = 0,
		.mux = ADC_INPUTCTRL_MUXPOS_AIN7_Val,
	},
	[DRV_ADC_CHANNEL_ISENSE] = {
		.adc_id = 0,
		.mux = ADC_INPUTCTRL_MUXPOS_AIN9_Val,
	},
};

struct drv_adc_config drv_adc_config = {
	.channelConfig = channelConfig,
};