/*
 * drv_adc_private.c
 *
 * Created: 7/26/2020 7:08:29 PM
 *  Author: connor
 */ 
#include "drv_adc.h"
#include "sam.h"

// MUST be in increasing order of AIN0 -> 1,2,...
static const int sequenceMap[DRV_ADC_SEQUENCE_COUNT] = {
	[DRV_ADC_SEQUENCE_A0] = ADC_INPUTCTRL_MUXPOS_AIN0_Val,
	[DRV_ADC_SEQUENCE_A1] = ADC_INPUTCTRL_MUXPOS_AIN1_Val,
	[DRV_ADC_SEQUENCE_A4] = ADC_INPUTCTRL_MUXPOS_AIN4_Val,
	[DRV_ADC_SEQUENCE_A5] = ADC_INPUTCTRL_MUXPOS_AIN5_Val,
	[DRV_ADC_SEQUENCE_A6] = ADC_INPUTCTRL_MUXPOS_AIN6_Val,
	[DRV_ADC_SEQUENCE_A7] = ADC_INPUTCTRL_MUXPOS_AIN7_Val,
};

struct drv_adc_config drv_adc_config = {
	.sequence_map = sequenceMap,
};