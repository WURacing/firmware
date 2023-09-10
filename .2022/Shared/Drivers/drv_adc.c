#include "drv_adc.h"
#include "sam.h"

#ifndef FEATURE_ADC1
#define FEATURE_ADC1 0
#endif

static enum drv_adc_channel reverse_adc0[32];
static enum drv_adc_channel reverse_adc1[32];

void drv_adc_init(void)
{
	
	// set up MCLK APB for ADC (synchronized peripheral clock)
#if FEATURE_ADC1
    MCLK_REGS->MCLK_APBCMASK |= MCLK_APBCMASK_ADC0(1) | MCLK_APBCMASK_ADC1(1);
#else
	MCLK_REGS->MCLK_APBCMASK |= MCLK_APBCMASK_ADC0(1);
#endif

	// set up ADC peripheral clocks
	GCLK_REGS->GCLK_PCHCTRL[33] = GCLK_PCHCTRL_CHEN(1) | GCLK_PCHCTRL_GEN_GCLK0;
	
	// enable interfaces, set up master-slave
    ADC0_REGS->ADC_CTRLA = ADC_CTRLA_ENABLE(0);
#if FEATURE_ADC1
	ADC1_REGS->ADC_CTRLA = ADC_CTRLA_SLAVEEN(1);
#endif
	
	// Calibrate ADC from factory values stored in flash
	uint32_t OTP5 = *(uint32_t *)OTP5_ADDR;
    ADC0_REGS->ADC_CALIB = ADC_CALIB_BIASREFBUF(OTP5) | ADC_CALIB_BIASCOMP(OTP5 >> 3);
    ADC1_REGS->ADC_CALIB = ADC_CALIB_BIASREFBUF(OTP5 >> 6) | ADC_CALIB_BIASCOMP(OTP5 >> 9);

	
	// CTRLA defaults: not on demand, not run standby, no ADC1 slave
	// CTRLB defaults: div2 prescaler
	// CTRLC defaults: no windows, no rail-to-rail, 12bit, no gain/offset correction, no free-running, right adjusted, single-ended
    ADC1_REGS->ADC_CTRLC = ADC0_REGS->ADC_CTRLC = ADC_CTRLC_RESSEL_16BIT; // enable averaging
	
	// EVCTRL defaults: all events disabled
	// INT* defaults: no interrupts
	
    ADC1_REGS->ADC_REFCTRL = ADC0_REGS->ADC_REFCTRL = ADC_REFCTRL_REFSEL_INTVCC2;
    ADC1_REGS->ADC_INPUTCTRL = ADC0_REGS->ADC_INPUTCTRL = ADC_INPUTCTRL_MUXPOS_AIN0 | ADC_INPUTCTRL_MUXNEG_GND;
	
    // accumulate samples. see datasheet page 875
    ADC1_REGS->ADC_AVGCTRL = ADC0_REGS->ADC_AVGCTRL = DRV_ADC_SAMPLENUM;
	
	// add additional sampling time. significantly increases precision
	ADC1_REGS->ADC_SAMPCTRL = ADC0_REGS->ADC_SAMPCTRL = ADC_SAMPCTRL_SAMPLEN(3); // sample for 3 cycles
	
	// set up sequencing. allows us to read every input in one command
	uint32_t adc0_seqctrl = 0, adc1_seqctrl = 0;
	for (enum drv_adc_channel i = (enum drv_adc_channel)0; i < DRV_ADC_CHANNEL_COUNT; ++i)
	{
		const struct drv_adc_channelConfig * channelConfig = &drv_adc_config.channelConfig[i];
		int mux = channelConfig->mux;
		uint32_t seqmask = 1 << mux;
		if (channelConfig->adc_id == 0)
		{
			adc0_seqctrl |= seqmask;
			reverse_adc0[mux] = i;
		}
		else /* if (channelConfig->adc_id == 1) */
		{
			adc1_seqctrl |= seqmask;
			reverse_adc1[mux] = i;
		}
		// set up PINMUX (only works for PA)
		int pin = channelConfig->pinmux >> 16;
		int pmux = channelConfig->pinmux & 0xF;
		int pin_eo = (pin % 32) / 2;
		int group = pin / 32;
		if ((pin % 2) == 0)
			PORT_REGS->GROUP[group].PORT_PMUX[pin_eo] |= PORT_PMUX_PMUXE(pmux);
		else
			PORT_REGS->GROUP[group].PORT_PMUX[pin_eo] |= PORT_PMUX_PMUXO(pmux);
		PORT_REGS->GROUP[group].PORT_PINCFG[pin % 32] = PORT_PINCFG_PMUXEN(1);
	}
	ADC0_REGS->ADC_SEQCTRL = adc0_seqctrl;
	ADC1_REGS->ADC_SEQCTRL = adc1_seqctrl;
		
	while (ADC0_REGS->ADC_SYNCBUSY) {}
	
	ADC0_REGS->ADC_CTRLA = ADC_CTRLA_ENABLE(1);
}

uint16_t drv_adc_read(int channel)
{
	return -1;
}

void drv_adc_read_sequence_sync(struct drv_adc_results * results)
{
	for (enum drv_adc_channel i = (enum drv_adc_channel)0; i < DRV_ADC_CHANNEL_COUNT; ++i)
	{
		results->results[i] = 0xFFFF;
	}
	results->error = 0;
	
#if FEATURE_ADC1
	while (ADC0_REGS->ADC_SYNCBUSY || ADC1_REGS->ADC_SYNCBUSY) {}
#else
	while (ADC0_REGS->ADC_SYNCBUSY) {}
#endif
    // start conversion
    ADC0_REGS->ADC_SWTRIG = ADC_SWTRIG_START(1);
    
#if FEATURE_ADC1
	while (ADC0_REGS->ADC_SYNCBUSY || ADC1_REGS->ADC_SYNCBUSY) {}
#else
	while (ADC0_REGS->ADC_SYNCBUSY) {}
#endif
    
    while ((ADC0_REGS->ADC_SEQSTATUS & ADC_SEQSTATUS_SEQBUSY_Msk) ||
#if FEATURE_ADC1
			(ADC1_REGS->ADC_SEQSTATUS & ADC_SEQSTATUS_SEQBUSY_Msk)
#else
			0
#endif
			)
    {
        if (ADC0_REGS->ADC_INTFLAG & ADC_INTFLAG_RESRDY_Msk)
        {
            int which = ADC0_REGS->ADC_SEQSTATUS & ADC_SEQSTATUS_SEQSTATE_Msk;
            unsigned result = ADC0_REGS->ADC_RESULT;
			enum drv_adc_channel channel = reverse_adc0[which];
			results->results[channel] = result;
        }
#if FEATURE_ADC1
        if (ADC1_REGS->ADC_INTFLAG & ADC_INTFLAG_RESRDY_Msk)
        {
            int which = ADC1_REGS->ADC_SEQSTATUS & ADC_SEQSTATUS_SEQSTATE_Msk;
            unsigned result = ADC1_REGS->ADC_RESULT;
			enum drv_adc_channel channel = reverse_adc1[which];
			results->results[channel] = result;
        }
#endif		
    }
}