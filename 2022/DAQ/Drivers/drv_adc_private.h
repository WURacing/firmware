#pragma once

// Reduce sample averaging. We don't need a lot of precision here, just need to discern 12 values.
#define DRV_ADC_SAMPLENUM	(ADC_AVGCTRL_SAMPLENUM_4 | ADC_AVGCTRL_ADJRES(2))

enum drv_adc_channel {
	DRV_ADC_CHANNEL_SW1,
	DRV_ADC_CHANNEL_SW2,
	DRV_ADC_CHANNEL_SW3,
	DRV_ADC_CHANNEL_SW4,
	
	DRV_ADC_CHANNEL_COUNT
};
