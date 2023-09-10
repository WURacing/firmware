/*
 * drv_divas.c
 *
 * Created: 8/15/2020 12:29:35 PM
 *  Author: connor
 */ 
#include "sam.h"
#include "drv_divas.h"


void drv_divas_init(void)
{
	MCLK_REGS->MCLK_AHBMASK |= MCLK_AHBMASK_DIVAS(1);
}

int __aeabi_idiv(int numerator, int denominator)
{
	DIVAS_REGS->DIVAS_CTRLA = DIVAS_CTRLA_DLZ(0) | DIVAS_CTRLA_SIGNED(1);
	DIVAS_REGS->DIVAS_DIVIDEND = numerator;
	DIVAS_REGS->DIVAS_DIVISOR = denominator;
	while (DIVAS_REGS->DIVAS_STATUS & DIVAS_STATUS_BUSY_Msk) {}
	int result = DIVAS_REGS->DIVAS_RESULT;
	return result;
}

unsigned __aeabi_uidiv(unsigned numerator, unsigned denominator)
{
	DIVAS_REGS->DIVAS_CTRLA = DIVAS_CTRLA_DLZ(0) | DIVAS_CTRLA_SIGNED(0);
	DIVAS_REGS->DIVAS_DIVIDEND = numerator;
	DIVAS_REGS->DIVAS_DIVISOR = denominator;
	while (DIVAS_REGS->DIVAS_STATUS & DIVAS_STATUS_BUSY_Msk) {}
	unsigned result = DIVAS_REGS->DIVAS_RESULT;
	return result;
}

int __aeabi_idivmod(int numerator, int denominator)
{
	DIVAS_REGS->DIVAS_CTRLA = DIVAS_CTRLA_DLZ(0) | DIVAS_CTRLA_SIGNED(1);
	DIVAS_REGS->DIVAS_DIVIDEND = numerator;
	DIVAS_REGS->DIVAS_DIVISOR = denominator;
	while (DIVAS_REGS->DIVAS_STATUS & DIVAS_STATUS_BUSY_Msk) {}
	int result = DIVAS_REGS->DIVAS_REM;
	return result;
}

unsigned __aeabi_uidivmod(unsigned numerator, unsigned denominator)
{
	DIVAS_REGS->DIVAS_CTRLA = DIVAS_CTRLA_DLZ(0) | DIVAS_CTRLA_SIGNED(0);
	DIVAS_REGS->DIVAS_DIVIDEND = numerator;
	DIVAS_REGS->DIVAS_DIVISOR = denominator;
	while (DIVAS_REGS->DIVAS_STATUS & DIVAS_STATUS_BUSY_Msk) {}
	unsigned result = DIVAS_REGS->DIVAS_REM;
	return result;
}

struct drv_divas_quot_rem_u drv_divas_divide(unsigned numerator, unsigned denominator)
{
	DIVAS_REGS->DIVAS_CTRLA = DIVAS_CTRLA_DLZ(0) | DIVAS_CTRLA_SIGNED(0);
	DIVAS_REGS->DIVAS_DIVIDEND = numerator;
	DIVAS_REGS->DIVAS_DIVISOR = denominator;
	while (DIVAS_REGS->DIVAS_STATUS & DIVAS_STATUS_BUSY_Msk) {}
	
	struct drv_divas_quot_rem_u result = {
		.quotient = DIVAS_REGS->DIVAS_RESULT,
		.remainder = DIVAS_REGS->DIVAS_REM,
	};
	return result;
}