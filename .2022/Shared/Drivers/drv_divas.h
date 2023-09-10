/*
 * drv_divas.h
 *
 * Created: 8/15/2020 12:29:28 PM
 *  Author: connor
 */ 


#ifndef DRV_DIVAS_H_
#define DRV_DIVAS_H_


void drv_divas_init(void);

int32_t __aeabi_idiv(int32_t numerator, int32_t denominator);
uint32_t __aeabi_uidiv(uint32_t numerator, uint32_t denominator);
uint64_t __aeabi_idivmod(int32_t numerator, int32_t denominator);
uint64_t __aeabi_uidivmod(uint32_t numerator, uint32_t denominator);

#endif /* DRV_DIVAS_H_ */