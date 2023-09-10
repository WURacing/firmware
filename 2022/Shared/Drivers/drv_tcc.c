#include "drv_tcc.h"


static inline void set_pinmux(uint32_t pinmux)
{
	//  Account for PORT register even/odd configuration.
	int pin = pinmux >> 16;
	int mux = pinmux & 0xFFFF;
	int group = pin / 32;
	int lpin = pin % 32;
	int even = (lpin % 2) == 0;
	PORT_REGS->GROUP[group].PORT_PMUX[lpin/2] |= even ? PORT_PMUX_PMUXE(mux) : PORT_PMUX_PMUXO(mux);
	PORT_REGS->GROUP[group].PORT_PINCFG[lpin] = PORT_PINCFG_PMUXEN(1);
	PORT_REGS->GROUP[group].PORT_DIRSET = (1 << lpin);
}

void drv_tcc_init(void)
{
	for (enum drv_tcc_channel channel = (enum drv_tcc_channel)0U; channel < DRV_TCC_CHANNEL_COUNT; ++channel) {
		const struct drv_tcc_channelConfig * config = &drv_tcc_config.channelConfig[channel];
		
		switch (config->tccid)
		{
		case 0:
			MCLK_REGS->MCLK_APBCMASK |= MCLK_APBCMASK_TCC0(1);
			GCLK_REGS->GCLK_PCHCTRL[28] = GCLK_PCHCTRL_CHEN(1) | GCLK_PCHCTRL_GEN(config->gclk);
			break;
		case 1:
			MCLK_REGS->MCLK_APBCMASK |= MCLK_APBCMASK_TCC1(1);
			GCLK_REGS->GCLK_PCHCTRL[28] = GCLK_PCHCTRL_CHEN(1) | GCLK_PCHCTRL_GEN(config->gclk);
			break;
		case 2:
			MCLK_REGS->MCLK_APBCMASK |= MCLK_APBCMASK_TCC2(1);
			GCLK_REGS->GCLK_PCHCTRL[29] = GCLK_PCHCTRL_CHEN(1) | GCLK_PCHCTRL_GEN(config->gclk);
			break;
		}
		
		// set PORTs
		for (int i = 0; i < DRV_TCC_NUM_PINMUX; ++i)
		{
			if (config->pinmux[i] != 0)
			{
				set_pinmux(config->pinmux[i]);
			}
		}
		
		
		config->module->TCC_CTRLA = TCC_CTRLA_ENABLE(0); // no prescaling, no capture mode
		//config->module->TCC_CTRLBSET = TCC_CTRLBSET_LUPD(1); // no downcounting, no double buffering
		//config->module.TCC_ = TCC_WAVE_WAVEGEN_NPWM; // POL = 0
		config->module->TCC_WAVE = TCC_WAVE_WAVEGEN_NPWM; // POL = 0
		// WEXT: default distribution of CCs to WOs
		
		config->module->TCC_PER = TCC_PER_PER(config->per);
		for (int i = 0; i < DRV_TCC_NUM_CC; ++i)
		{
			config->module->TCC_CC[i] = TCC_CC_CC(config->cc[i]);
		}
		
		while (config->module->TCC_SYNCBUSY);
		config->module->TCC_CTRLA |= TCC_CTRLA_ENABLE(1);
		while (config->module->TCC_SYNCBUSY);
		
	}
}

void drv_tcc_set_cc(enum drv_tcc_channel channel, int num_cc, int val)
{
	const struct drv_tcc_channelConfig * config = &drv_tcc_config.channelConfig[channel];
	config->module->TCC_CCBUF[num_cc] = TCC_CC_CC(val);
}