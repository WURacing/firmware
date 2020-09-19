#include "drv_adc.h"
#include "sam.h"

static enum drv_adc_channel reverse_adc0[32];
static enum drv_adc_channel reverse_adc1[32];

void drv_adc_init(void)
{
	// set up MCLK APB for ADC (synchronized peripheral clock)
	MCLK->APBCMASK.bit.ADC0_ = 1;
	MCLK->APBCMASK.bit.ADC1_ = 1;
	
	// set up ADC peripheral clocks
	GCLK_PCHCTRL_Type pclock = {
		.bit = {
			.CHEN = 1,
			.GEN = 0
		}
	};
	GCLK->PCHCTRL[ADC0_GCLK_ID] = pclock;
	
	ADC_CTRLA_Type adc0_ctrla = {
		.bit = {
			.ENABLE = 0,
		}
	};
	ADC_CTRLA_Type adc1_ctrla = {
		.bit = {
			.SLAVEEN = 1,
		}
	};
	ADC0->CTRLA = adc0_ctrla;
	ADC1->CTRLA = adc1_ctrla;
	
	// Calibrate ADC from factory values stored in flash
	ADC0->CALIB.reg =
	ADC_CALIB_BIASREFBUF(
		(*(uint32_t *)NVMCTRL_OTP5 >> ADC0_FUSES_BIASREFBUF_Pos)
	) |
	ADC_CALIB_BIASCOMP(
		(*(uint32_t *)NVMCTRL_OTP5 >> ADC0_FUSES_BIASCOMP_Pos)
	);
	ADC1->CALIB.reg =
	ADC_CALIB_BIASREFBUF(
	(*(uint32_t *)NVMCTRL_OTP5 >> ADC1_FUSES_BIASREFBUF_Pos)
	) |
	ADC_CALIB_BIASCOMP(
	(*(uint32_t *)NVMCTRL_OTP5 >> ADC1_FUSES_BIASCOMP_Pos)
	);

	
	// CTRLA defaults: not on demand, not run standby, no ADC1 slave
	// CTRLB defaults: div2 prescaler
	// CTRLC defaults: no windows, no rail-to-rail, 12bit, no gain/offset correction, no free-running, right adjusted, single-ended
	ADC_CTRLC_Type ctrlc = {
		.bit = {
			.RESSEL = 1, // enable averaging
		}
	};
	ADC0->CTRLC = ctrlc;
	ADC1->CTRLC = ctrlc;
	
	// EVCTRL defaults: all events disabled
	// INT* defaults: no interrupts
	
	ADC_REFCTRL_Type refctrl = {
		.bit = {
			.REFCOMP = 0,
			.REFSEL = ADC_REFCTRL_REFSEL_INTVCC2_Val, // reference = VDDANA (5V)
		}
	};
	ADC0->REFCTRL = refctrl;
	ADC1->REFCTRL = refctrl;
	
	ADC_INPUTCTRL_Type inputctrl = {
		.bit = {
			.MUXNEG = 0x18, /* ground */
			.MUXPOS = ADC_INPUTCTRL_MUXPOS_AIN0_Val
		}
	};
	ADC0->INPUTCTRL = inputctrl;
	ADC1->INPUTCTRL = inputctrl;
	
	// average four samples to increase accuracy
	ADC_AVGCTRL_Type avgctrl = {
		.bit = {
			.SAMPLENUM = 2,
			.ADJRES = 2,
		}
	};
	ADC0->AVGCTRL = avgctrl;
	ADC1->AVGCTRL = avgctrl;
	
	// add additional sampling time. significantly increases precision
	ADC_SAMPCTRL_Type sampctrl = {
		.bit = {
			.SAMPLEN = 3,
		}
	};
	ADC0->SAMPCTRL = sampctrl;
	ADC1->SAMPCTRL = sampctrl;
	
	// set up sequencing. allows us to read every input in one command
	ADC_SEQCTRL_Type adc0_seqctrl = { 0 };
	ADC_SEQCTRL_Type adc1_seqctrl = { 0 };
	for (enum drv_adc_channel i = (enum drv_adc_channel)0; i < DRV_ADC_CHANNEL_COUNT; ++i)
	{
		const struct drv_adc_channelConfig * channelConfig = &drv_adc_config.channelConfig[i];
		int mux = channelConfig->mux;
		uint32_t seqmask = 1 << mux;
		if (channelConfig->adc_id == 0)
		{
			adc0_seqctrl.reg |= seqmask;
			reverse_adc0[mux] = i;
		}
		else /* if (channelConfig->adc_id == 1) */
		{
			adc1_seqctrl.reg |= seqmask;
			reverse_adc1[mux] = i;
		}
	}
	ADC0->SEQCTRL = adc0_seqctrl;
	ADC1->SEQCTRL = adc1_seqctrl;
		
	while (ADC0->SYNCBUSY.reg) {};
	while (ADC1->SYNCBUSY.reg) {};
	
	adc0_ctrla.bit.ENABLE = 1;
	ADC0->CTRLA = adc0_ctrla;
	
	while (ADC0->SYNCBUSY.reg) {};
}

uint16_t drv_adc_read(int channel)
{
	// sync
	while (ADC0->SYNCBUSY.reg) {};
	// change ADC mux
	ADC0->INPUTCTRL.bit.MUXPOS = channel;
	// sync
	while (ADC0->SYNCBUSY.reg) {};
	// start ADC conversion
	ADC0->SWTRIG.bit.START = 1;
	// sync
	while (ADC0->SYNCBUSY.reg) {};
	// wait for result
	while (!ADC0->INTFLAG.bit.RESRDY) {};
	// read result
	return ADC0->RESULT.reg;
}

void drv_adc_read_sequence_sync(struct drv_adc_results * results)
{
	for (enum drv_adc_channel i = (enum drv_adc_channel)0; i < DRV_ADC_CHANNEL_COUNT; ++i)
	{
		results->results[i] = 0xFFFF;
	}
	results->error = 0;
	
	// sync
	while (ADC0->SYNCBUSY.reg) {};
	while (ADC1->SYNCBUSY.reg) {};
	// start ADC conversion, should kick off both ADCs
	ADC0->SWTRIG.bit.START = 1;
	// sync
	while (ADC0->SYNCBUSY.reg) {};
	while (ADC1->SYNCBUSY.reg) {};
		
	while (ADC0->SEQSTATUS.bit.SEQBUSY || ADC1->SEQSTATUS.bit.SEQBUSY)
	{
		if (ADC0->INTFLAG.bit.RESRDY)
		{
			uint16_t result = ADC0->RESULT.reg;
			int which = ADC0->SEQSTATUS.bit.SEQSTATE;
			enum drv_adc_channel channel = reverse_adc0[which];
			results->results[channel] = result;
		}
		if (ADC1->INTFLAG.bit.RESRDY)
		{
			uint16_t result = ADC1->RESULT.reg;
			int which = ADC1->SEQSTATUS.bit.SEQSTATE;
			enum drv_adc_channel channel = reverse_adc1[which];
			results->results[channel] = result;
		}
	}
	// pick up the last result(s), probably not needed
	if (ADC0->INTFLAG.bit.RESRDY)
		results->error |= 1;
	if (ADC1->INTFLAG.bit.RESRDY)
		results->error |= 2;
}