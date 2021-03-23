#include "drv_hsd.h"
#include <sam.h>

#define DIAG_EN_PORT PORT_PA08
#define HSD_SEL_PORT PORT_PA14
#define HSD_SEH_PORT PORT_PA15
#define HSD_FAULT_L_PORT PORT_PA10
#define HSD_FAULT_L_PIN PIN_PA10

void drv_hsd_init(void)
{
	// initialize output signals
	PORT_REGS->GROUP[0].PORT_DIRSET = DIAG_EN_PORT | HSD_SEL_PORT | HSD_SEH_PORT;
	// initialize input digital signals
	PORT_REGS->GROUP[0].PORT_DIRCLR = HSD_FAULT_L_PORT;
	// datasheet says this needs a pullup because it's open drain
	PORT_REGS->GROUP[0].PORT_PINCFG[HSD_FAULT_L_PIN] = PORT_PINCFG_PULLEN(1) | PORT_PINCFG_INEN(1);
	// enable fault and isense outputs
	PORT_REGS->GROUP[0].PORT_OUTSET = DIAG_EN_PORT;
	
	drv_hsd_setChannel((enum drv_hsd_channel)0U);
}
// Takes ~50uS to change on the external device
void drv_hsd_setChannel(enum drv_hsd_channel channel)
{
	// Calculate the 2-bit signal for this channel
	uint32_t set = 0, clr = 0;
	if (channel & 1) // least significant bit
		set |= HSD_SEL_PORT;
	else
		clr |= HSD_SEL_PORT;
	if (channel & 2) // most significant bit
		set |= HSD_SEH_PORT;
	else
		clr |= HSD_SEH_PORT;
	PORT_REGS->GROUP[0].PORT_OUTSET = set;
	PORT_REGS->GROUP[0].PORT_OUTCLR = clr;
}

bool drv_hsd_isFaulted(void)
{
	// return true if fault_l is low
	return !((PORT_REGS->GROUP[0].PORT_IN >> HSD_FAULT_L_PIN) & 1);
}