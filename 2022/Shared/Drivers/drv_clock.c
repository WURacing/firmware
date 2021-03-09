#include "drv_clock.h"
#include "sam.h"
#include "system_samc21.h"
#include "FreeRTOSConfig.h"

#if (configCPU_CLOCK_HZ != OSC48M_FREQUENCY)
#error FreeRTOS CPU frequency is not matching OSC48M, please check it
#endif

void drv_clock_init(void)
{
	// First, bring up OSC48M to 48MHz.
	// This requires increasing NVM read wait states for timing purposes.
#	if (OSC48M_FREQUENCY == 48000000U)
	{
		// increase flash wait states from 0 to 1. if this is not done, the
		// CPU has a seizure trying to run at 48MHz
		NVMCTRL_REGS->NVMCTRL_CTRLB = NVMCTRL_CTRLB_MANW(1) | NVMCTRL_CTRLB_RWS(1);
		// up speed to 48MHz
		OSCCTRL_REGS->OSCCTRL_OSC48MDIV = OSCCTRL_OSC48MDIV_DIV_DIV1;
		// wait for synchronization
		while (OSCCTRL_REGS->OSCCTRL_OSC48MSYNCBUSY) {}
	}
#	elif (OSC48M_FREQUENCY == 16000000U)
	{
		// up speed to 16MHz
		OSCCTRL_REGS->OSCCTRL_OSC48MDIV = OSCCTRL_OSC48MDIV_DIV_DIV3;
		// wait for synchronization
		while (OSCCTRL_REGS->OSCCTRL_OSC48MSYNCBUSY) {}
	}
#	else
	{
		// unsupported
		#error unsupported OSC48M_FREQUENCY
	}
#	endif
	SystemCoreClock = OSC48M_FREQUENCY;
	
	// Then, bring up the rest of the clocks.
	for (enum drv_clock_channel channel = (enum drv_clock_channel)0U; channel < DRV_CLOCK_CHANNEL_COUNT; ++channel) {
		const struct drv_clock_channelConfig * config = &drv_clock_config.channelConfig[channel];
		uint32_t target = (config->divisor << 24) | (config->division_mode << 12)
			|(1 << 8) | config->source;
		if (*config->genctrl != target)
		{
			*config->genctrl = target;
			while (GCLK_REGS->GCLK_SYNCBUSY) {}
		}
	}
}
