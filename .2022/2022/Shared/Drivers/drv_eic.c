#include "drv_eic.h"
#include "sam.h"

int clicks_per_second;
int total_clicks;
int seconds_passed;
int last_time;

void drv_eic_init(void) {
    //Sets configuration to disable eic
    EIC_REGS->EIC_CTRLA = EIC_CTRLA_ENABLE(0);
    
    //Enable CLK_EIC_APB Clock
    MCLK_REGS->MCLK_APBAMASK |= MCLK_APBAMASK_EIC(1);
    
    //Set up Global Clock
	GCLK_REGS->GCLK_PCHCTRL[2] = GCLK_PCHCTRL_CHEN(1) | GCLK_PCHCTRL_GEN_GCLK0;
    
    //Set up the pin for input 
    //Sets up edge detection for falling edge
    EIC_REGS->EIC_CONFIG[1] = EIC_CONFIG_SENSE1_FALL;
            
            
    //Sets configuration to enable EIC
    EIC_REGS->EIC_CTRLA = EIC_CTRLA_ENABLE(1);
    
    //Enable interrupt EXTINT[9] 
    EIC_REGS->EIC_INTENSET = EIC_INTENSET_EXTINT((1<<9));
            
    //Enable EIC interrupt to make CPU listen
    NVIC_EnableIRQ(EIC_IRQn);
    
    //Configure the PORT for EIC and Turn On
    PORT_REGS->GROUP[1].PORT_PMUX[9/2] |= PORT_PMUX_PMUXO(MUX_PB09A_EIC_EXTINT9);
    PORT_REGS->GROUP[1].PORT_PINCFG[9] |= PORT_PINCFG_PMUXEN(1);
	
	last_time = 0;
	clicks_per_second = 0;
}

void EIC_Handler() {
    int currentTime = xTaskGetTickCount();
	
	if (currentTime - last_time > 1000)
	{
		clicks_per_second = total_clicks;
		total_clicks = 0;
		last_time = currentTime;
	}
    total_clicks++;
	
    EIC_REGS->EIC_INTFLAG = 0xFFFFFFFF;
}
