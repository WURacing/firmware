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
		.pinmux = PINMUX_PA02B_ADC0_AIN0,
	},
	[DRV_ADC_CHANNEL_SENSE2] = {
		.adc_id = 0,
		.mux = ADC_INPUTCTRL_MUXPOS_AIN1_Val,
		.pinmux = PINMUX_PA03B_ADC0_AIN1,
	},
	[DRV_ADC_CHANNEL_SENSE3] = {
		.adc_id = 0,
		.mux = ADC_INPUTCTRL_MUXPOS_AIN4_Val,
		.pinmux = PINMUX_PA04B_ADC0_AIN4,
	},
	[DRV_ADC_CHANNEL_SENSE4] = {
		.adc_id = 0,
		.mux = ADC_INPUTCTRL_MUXPOS_AIN5_Val,
		.pinmux = PINMUX_PA05B_ADC0_AIN5,
	},
	[DRV_ADC_CHANNEL_SENSE5] = {
		.adc_id = 0,
		.mux = ADC_INPUTCTRL_MUXPOS_AIN6_Val,
		.pinmux = PINMUX_PA06B_ADC0_AIN6,
	},
	[DRV_ADC_CHANNEL_SENSE6] = {
		.adc_id = 0,
		.mux = ADC_INPUTCTRL_MUXPOS_AIN7_Val,
		.pinmux = PINMUX_PA07B_ADC0_AIN7,
	},
	[DRV_ADC_CHANNEL_ISENSE] = {
		.adc_id = 0,
		.mux = ADC_INPUTCTRL_MUXPOS_AIN9_Val,
		.pinmux = PINMUX_PA09B_ADC0_AIN9,
	},
};

struct drv_adc_config drv_adc_config = {
	.channelConfig = channelConfig,
};