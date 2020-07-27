#include "drv_adc.h"
#include "sam.h"
#include "samc21e18a.h"

void drv_adc_init(void)
{
	// set up MCLK APB for ADC (synchronized peripheral clock)
	MCLK->APBCMASK.bit.ADC0_ = 1;
	
	{ // set up ADC peripheral clocks
		GCLK_PCHCTRL_Type pclock = {
			.bit = {
				.CHEN = 1,
				.GEN = 0
			}
		};
		GCLK->PCHCTRL[ADC0_GCLK_ID] = pclock;
	}
	
	ADC0->CTRLA.bit.ENABLE = 0;
	
	{ // Calibrate ADC
		ADC0->CALIB.reg =
		ADC_CALIB_BIASREFBUF(
		(*(uint32_t *)NVMCTRL_OTP5 >> ADC0_FUSES_BIASREFBUF_Pos)
		) |
		ADC_CALIB_BIASCOMP(
		(*(uint32_t *)NVMCTRL_OTP5 >> ADC0_FUSES_BIASCOMP_Pos)
		);
	}
	
	// CTRLA defaults: not on demand, not run standby, no ADC1 slave
	// CTRLB defaults: div2 prescaler
	// CTRLC defaults: no windows, no rail-to-rail, 12bit, no gain/offset correction, no free-running, right adjusted, single-ended
	// EVCTRL defaults: all events disabled
	// INT* defaults: no interrupts
	
	{
		ADC_REFCTRL_Type refctrl = {
			.bit = {
				.REFCOMP = 0,
				.REFSEL = ADC_REFCTRL_REFSEL_INTVCC2_Val, // reference = VDDANA (5V)
			}
		};
		ADC0->REFCTRL.reg = refctrl.reg;
	}
	
	{
		ADC_INPUTCTRL_Type inputctrl = {
			.bit = {
				.MUXNEG = 0x18, /* ground */
				.MUXPOS = ADC_INPUTCTRL_MUXPOS_AIN0_Val
			}
		};
		ADC0->INPUTCTRL.reg = inputctrl.reg;
	}
	
	// AVGCTRL: no averaging
	// SAMPCTRL: no additional sampling time
	{
		ADC_SAMPCTRL_Type sampctrl = {
			.bit = {
				.SAMPLEN = 3
			}
		};
		ADC0->SAMPCTRL.reg = sampctrl.reg;
	}
		
	
	// set up sequencing
	ADC_SEQCTRL_Type seqctrl;
	for (enum drv_adc_sequence i = (enum drv_adc_sequence)0; i < DRV_ADC_SEQUENCE_COUNT; ++i)
	{
		seqctrl.reg |= (1 << drv_adc_config.sequence_map[i]);
	}
	ADC0->SEQCTRL.reg = seqctrl.reg;
		
	while (ADC0->SYNCBUSY.reg) {};
	
	ADC0->CTRLA.bit.ENABLE = 1;
	
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

void drv_adc_read_sequence_sync(uint16_t * results)
{
	// sync
	while (ADC0->SYNCBUSY.reg) {};
	// start ADC conversion
	ADC0->SWTRIG.bit.START = 1;
	// sync
	while (ADC0->SYNCBUSY.reg) {};
		
	int i = 0;
	while (ADC0->SEQSTATUS.bit.SEQBUSY)
	{
		// wait for result
		while (!ADC0->INTFLAG.bit.RESRDY) {};
		
		*(results++) = ADC0->RESULT.reg;
		++i;
	}
	// pick up the last result
	if (i < DRV_ADC_SEQUENCE_COUNT)
	{
		*(results++) = ADC0->RESULT.reg;
	}
}