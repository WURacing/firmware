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
	[DRV_ADC_CHANNEL_VIOUT1] = {
		.adc_id = 0,
		.mux = ADC_INPUTCTRL_MUXPOS_AIN0_Val,
	},
	[DRV_ADC_CHANNEL_ACCEL_X] = {
		.adc_id = 0,
		.mux = ADC_INPUTCTRL_MUXPOS_AIN1_Val,
	},
	[DRV_ADC_CHANNEL_ACCEL_Y] = {
		.adc_id = 0,
		.mux = ADC_INPUTCTRL_MUXPOS_AIN2_Val,
	},
	[DRV_ADC_CHANNEL_ACCEL_Z] = {
		.adc_id = 0,
		.mux = ADC_INPUTCTRL_MUXPOS_AIN3_Val,
	},
	[DRV_ADC_CHANNEL_VIOUT2] = {
		.adc_id = 0,
		.mux = ADC_INPUTCTRL_MUXPOS_AIN4_Val,
	},
	[DRV_ADC_CHANNEL_SENSE2] = {
		.adc_id = 0,
		.mux = ADC_INPUTCTRL_MUXPOS_AIN5_Val,
	},
	[DRV_ADC_CHANNEL_VIOUT3] = {
		.adc_id = 0,
		.mux = ADC_INPUTCTRL_MUXPOS_AIN6_Val,
	},
	[DRV_ADC_CHANNEL_SENSE3] = {
		.adc_id = 0,
		.mux = ADC_INPUTCTRL_MUXPOS_AIN7_Val,
	},
	// Items on ADC1
	[DRV_ADC_CHANNEL_VIOUT4] = {
		.adc_id = 1,
		.mux = ADC_INPUTCTRL_MUXPOS_AIN0_Val,
	},
	[DRV_ADC_CHANNEL_SENSE4] = {
		.adc_id = 1,
		.mux = ADC_INPUTCTRL_MUXPOS_AIN1_Val,
	},
	[DRV_ADC_CHANNEL_VIOUT5] = {
		.adc_id = 1,
		.mux = ADC_INPUTCTRL_MUXPOS_AIN2_Val,
	},
	[DRV_ADC_CHANNEL_SENSE5] = {
		.adc_id = 1,
		.mux = ADC_INPUTCTRL_MUXPOS_AIN3_Val,
	},
	[DRV_ADC_CHANNEL_VIOUT6] = {
		.adc_id = 1,
		.mux = ADC_INPUTCTRL_MUXPOS_AIN6_Val,
	},
	[DRV_ADC_CHANNEL_SENSE6] = {
		.adc_id = 1,
		.mux = ADC_INPUTCTRL_MUXPOS_AIN7_Val,
	},
};

struct drv_adc_config drv_adc_config = {
	.channelConfig = channelConfig,
};