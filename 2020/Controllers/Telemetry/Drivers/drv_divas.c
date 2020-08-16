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
	MCLK->AHBMASK.bit.DIVAS_ = 1;
}

int __aeabi_idiv(int numerator, int denominator)
{
	DIVAS_CTRLA_Type ctrla = {
		.bit = {
			.DLZ = 0,
			.SIGNED = 1,
		}
	};
	DIVAS->CTRLA.reg = ctrla.reg;
	DIVAS->DIVIDEND.reg = numerator;
	DIVAS->DIVISOR.reg = denominator;
	
	while (DIVAS->STATUS.bit.BUSY) {};
		
	int result = DIVAS->RESULT.reg;
	return result;
}

unsigned __aeabi_uidiv(unsigned numerator, unsigned denominator)
{
	DIVAS_CTRLA_Type ctrla = {
		.bit = {
			.DLZ = 0,
			.SIGNED = 0,
		}
	};
	DIVAS->CTRLA.reg = ctrla.reg;
	DIVAS->DIVIDEND.reg = numerator;
	DIVAS->DIVISOR.reg = denominator;
	
	while (DIVAS->STATUS.bit.BUSY) {};
	
	unsigned result = DIVAS->RESULT.reg;
	return result;
}

int __aeabi_idivmod(int numerator, int denominator)
{
	DIVAS_CTRLA_Type ctrla = {
		.bit = {
			.DLZ = 0,
			.SIGNED = 1,
		}
	};
	DIVAS->CTRLA.reg = ctrla.reg;
	DIVAS->DIVIDEND.reg = numerator;
	DIVAS->DIVISOR.reg = denominator;
	
	while (DIVAS->STATUS.bit.BUSY) {};
	
	int result = DIVAS->REM.reg;
	return result;
}

unsigned __aeabi_uidivmod(unsigned numerator, unsigned denominator)
{
	DIVAS_CTRLA_Type ctrla = {
		.bit = {
			.DLZ = 0,
			.SIGNED = 0,
		}
	};
	DIVAS->CTRLA.reg = ctrla.reg;
	DIVAS->DIVIDEND.reg = numerator;
	DIVAS->DIVISOR.reg = denominator;
	
	while (DIVAS->STATUS.bit.BUSY) {};
	
	unsigned result = DIVAS->RESULT.reg;
	return result;
}

struct drv_divas_quot_rem_u drv_divas_divide(unsigned numerator, unsigned denominator)
{
	DIVAS_CTRLA_Type ctrla = {
		.bit = {
			.DLZ = 0,
			.SIGNED = 0,
		}
	};
	DIVAS->CTRLA.reg = ctrla.reg;
	DIVAS->DIVIDEND.reg = numerator;
	DIVAS->DIVISOR.reg = denominator;
	
	while (DIVAS->STATUS.bit.BUSY) {};
		
	
	struct drv_divas_quot_rem_u result = {
		.quotient = DIVAS->RESULT.reg,
		.remainder = DIVAS->REM.reg,
	};
	return result;
}