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

int32_t __aeabi_idiv(int32_t numerator, int32_t denominator)
{
	DIVAS_REGS->DIVAS_CTRLA = DIVAS_CTRLA_DLZ(0) | DIVAS_CTRLA_SIGNED(1);
	DIVAS_REGS->DIVAS_DIVIDEND = numerator;
	DIVAS_REGS->DIVAS_DIVISOR = denominator;
	while (DIVAS_REGS->DIVAS_STATUS & DIVAS_STATUS_BUSY_Msk) {}
	int32_t result = DIVAS_REGS->DIVAS_RESULT;
	return result;
}

uint32_t __aeabi_uidiv(uint32_t numerator, uint32_t denominator)
{
	DIVAS_REGS->DIVAS_CTRLA = DIVAS_CTRLA_DLZ(0) | DIVAS_CTRLA_SIGNED(0);
	DIVAS_REGS->DIVAS_DIVIDEND = numerator;
	DIVAS_REGS->DIVAS_DIVISOR = denominator;
	while (DIVAS_REGS->DIVAS_STATUS & DIVAS_STATUS_BUSY_Msk) {}
	uint32_t result = DIVAS_REGS->DIVAS_RESULT;
	return result;
}

uint64_t __aeabi_idivmod(int32_t numerator, int32_t denominator)
{
	DIVAS_REGS->DIVAS_CTRLA = DIVAS_CTRLA_DLZ(0) | DIVAS_CTRLA_SIGNED(1);
	DIVAS_REGS->DIVAS_DIVIDEND = numerator;
	DIVAS_REGS->DIVAS_DIVISOR = denominator;
	while (DIVAS_REGS->DIVAS_STATUS & DIVAS_STATUS_BUSY_Msk) {}
	uint64_t result = ((uint64_t)DIVAS_REGS->DIVAS_RESULT & 0x00000000FFFFFFFF ) | (((uint64_t)DIVAS_REGS->DIVAS_REM ) << 32);
	return result;
}

uint64_t __aeabi_uidivmod(uint32_t numerator, uint32_t denominator)
{
	DIVAS_REGS->DIVAS_CTRLA = DIVAS_CTRLA_DLZ(0) | DIVAS_CTRLA_SIGNED(0);
	DIVAS_REGS->DIVAS_DIVIDEND = numerator;
	DIVAS_REGS->DIVAS_DIVISOR = denominator;
	while (DIVAS_REGS->DIVAS_STATUS & DIVAS_STATUS_BUSY_Msk) {}
	uint64_t result = DIVAS_REGS->DIVAS_RESULT | (((uint64_t)DIVAS_REGS->DIVAS_REM) << 32);
	return result;
}

