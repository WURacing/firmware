/*
 * drv_adc_private.c
 *
 * Created: 7/26/2020 7:08:29 PM
 *  Author: connor
 */ 
#include "drv_adc.h"
#include "sam.h"

static const struct drv_adc_channelConfig channelConfig[DRV_ADC_CHANNEL_COUNT] = {
	[DRV_ADC_CHANNEL_SW1] = {
		.adc_id = 0,
		.mux = ADC_INPUTCTRL_MUXPOS_AIN3_Val,
		.pinmux = PINMUX_PB09B_ADC0_AIN3,
	},
	[DRV_ADC_CHANNEL_SW2] = {
		.adc_id = 0,
		.mux = ADC_INPUTCTRL_MUXPOS_AIN2_Val,
		.pinmux = PINMUX_PB08B_ADC0_AIN2,
	},
	[DRV_ADC_CHANNEL_SW3] = {
		.adc_id = 0,
		.mux = ADC_INPUTCTRL_MUXPOS_AIN0_Val,
		.pinmux = PINMUX_PA02B_ADC0_AIN0,
	},
	[DRV_ADC_CHANNEL_SW4] = {
		.adc_id = 0,
		.mux = ADC_INPUTCTRL_MUXPOS_AIN1_Val,
		.pinmux = PINMUX_PA03B_ADC0_AIN1,
	},
};

struct drv_adc_config drv_adc_config = {
	.channelConfig = channelConfig,
};