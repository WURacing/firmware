/*
 * app_statusLight.c
 *
 * Created: 7/11/2020 7:22:35 PM
 *  Author: connor
 */ 

#include "app_statusLight.h"
#include "drv_gpio.h"
#include "drv_can.h"
#include "FreeRTOS.h"
#include "task.h"
#include "core_cm0plus.h"

static int flashesRemaining = 0;
static TickType_t nextFlash = 0;
static int lastError = 0;
static int lastTxbto = 0;
static TickType_t lastTransmission = 0;
static int txToggleState = 0;

struct color {
	uint8_t g;
	uint8_t r;
	uint8_t b;
};

static volatile struct color tc2_txbuf[2]; 
static volatile int tc2_cur_bit;


void app_statusLight_init(void)
{	
#if GENERAL_PURPOSE
	PORT_REGS->GROUP[1].PORT_DIRSET = PORT_PB16 | PORT_PB17;
#elif UPRIGHT
	/*
	PORT_REGS->GROUP[1].PORT_DIRSET = PORT_PB17;
	PORT_REGS->GROUP[1].PORT_OUTCLR = PORT_PB17;
	 * */
	PORT_REGS->GROUP[(PIN_PB17 / 32)].PORT_PMUX[(PIN_PB17 % 32)/2] |= PORT_PMUX_PMUXO(MUX_PB17E_TC2_WO1);
	PORT_REGS->GROUP[(PIN_PB17 / 32)].PORT_PINCFG[(PIN_PB17 % 32)] = PORT_PINCFG_PMUXEN(1);
	
	GCLK_REGS->GCLK_PCHCTRL[31] = GCLK_PCHCTRL_CHEN(1) | GCLK_PCHCTRL_GEN_GCLK0;
	MCLK_REGS->MCLK_APBCMASK |= MCLK_APBCMASK_TC2(1);
	NVIC_EnableIRQ(TC2_IRQn);
	tc2_cur_bit = sizeof(tc2_txbuf)*8;
	// TC2/WO1
	TC2_REGS->COUNT16.TC_CTRLA = TC_CTRLA_ENABLE(0);
	while (TC2_REGS->COUNT16.TC_SYNCBUSY) {}
	TC2_REGS->COUNT16.TC_CTRLA = TC_CTRLA_MODE_COUNT16;
	TC2_REGS->COUNT16.TC_WAVE = TC_WAVE_WAVEGEN_MPWM;
	TC2_REGS->COUNT16.TC_CC[0] = 60;
	TC2_REGS->COUNT16.TC_CC[1] = 17;
	TC2_REGS->COUNT16.TC_CCBUF[0] = 60;
	TC2_REGS->COUNT16.TC_CCBUF[1] = 17;
	TC2_REGS->COUNT16.TC_INTENSET = TC_INTENSET_OVF(1);
	TC2_REGS->COUNT16.TC_CTRLA |= TC_CTRLA_ENABLE(1);
	while (TC2_REGS->COUNT16.TC_SYNCBUSY) {}
#endif
}


void TC2_Handler(void)
{
	int curbit = tc2_cur_bit;
	if (__builtin_expect(curbit < sizeof(tc2_txbuf)*8, 1))
	{
		const uint8_t *tx = (const uint8_t *)tc2_txbuf;
		int byte = tx[curbit / 8];
		int b = (byte << (curbit % 8)) & 0x80;
		TC2_REGS->COUNT16.TC_CCBUF[1] = b ? 34 : 17;
	}
	else
	{
		TC2_REGS->COUNT16.TC_CCBUF[1] = 0;
		NVIC_DisableIRQ(TC2_IRQn);
	}
	tc2_cur_bit = curbit + 1;
}

const struct color off = {.r = 0, .g = 0, .b = 0};
const struct color blue = {.r = 0, .g = 0, .b = 255};
const struct color green = {.r = 0, .g = 255, .b = 0};
const struct color cyan = {.r = 0, .g = 255, .b = 255};
const struct color red = {.r = 255, .g = 0, .b = 0};
const struct color magenta = {.r = 255, .g = 0, .b = 255};
const struct color yellow = {.r = 255, .g = 255, .b = 0};
const struct color white = {.r = 255, .g = 255, .b = 255};

void app_statusLight_periodic(void)
{
	TickType_t time = xTaskGetTickCount();
	int error = DAQ_CAN_REGS->CAN_PSR & CAN_PSR_LEC_Msk;
	if (error == 7) //  no change
	{
		error = lastError;
	}
	else
	{
		lastError = error;
	}
	
#if GENERAL_PURPOSE
	if (nextFlash < time)
	{
		if (flashesRemaining > 0)
		{
			if (--flashesRemaining == 0)
			{
				nextFlash = time + 2000;
				
				PORT_REGS->GROUP[1].PORT_OUTCLR = PORT_PB17;
			}
			else
			{
				nextFlash = time + 200;
				
				PORT_REGS->GROUP[1].PORT_OUTTGL = PORT_PB17;
			}
		}
		else if (error != 0)
		{
			flashesRemaining = error * 2 + 1;
			PORT_REGS->GROUP[1].PORT_OUTCLR = PORT_PB16;
		}
		else
		{
			PORT_REGS->GROUP[1].PORT_OUTTGL = PORT_PB16;
		}
	}
#elif UPRIGHT
	switch (error)
	{
	case 0:
		tc2_txbuf[0] = green;
		if (nextFlash < time && (time - lastTransmission < 50))
			txToggleState = !txToggleState;
		tc2_txbuf[1] = txToggleState ? blue : off;
		break;
	case 1: //STUFF ERROR
		tc2_txbuf[0] = red;
		tc2_txbuf[1] = blue;
		break;
	case 2: //RECV FORM ERROR
		tc2_txbuf[0] = red;
		tc2_txbuf[1] = yellow;
		break;
	case 3: //NO ACK ERROR
		tc2_txbuf[0] = red;
		tc2_txbuf[1] = red;
		break;
	case 4: //BIT1 TX ERROR
		tc2_txbuf[0] = red;
		tc2_txbuf[1] = cyan;
		break;
	case 5: //BIT0 TX ERROR
		tc2_txbuf[0] = red;
		tc2_txbuf[1] = magenta;
		break;
	case 6: //CRC ERROR
		tc2_txbuf[0] = red;
		tc2_txbuf[1] = white;
		break;
	default:
		tc2_txbuf[0] = off;
		tc2_txbuf[1] = off;
		break;
	}
	tc2_cur_bit = 0;
	NVIC_EnableIRQ(TC2_IRQn);
#endif
	
	int txbto = DAQ_CAN_REGS->CAN_TXBTO;
	if (txbto)
	{
		lastTransmission = time;
		if (nextFlash < time)
		{
			nextFlash = time + 20;
		}
	}
	lastTxbto = txbto;
}
