/*
 * drv_divas.h
 *
 * Created: 8/15/2020 12:29:28 PM
 *  Author: connor
 */ 


#ifndef DRV_DIVAS_H_
#define DRV_DIVAS_H_


struct drv_divas_quot_rem_i {
	int quotient;
	int remainder;
};
struct drv_divas_quot_rem_u {
	int quotient;
	int remainder;
};

void drv_divas_init(void);

struct drv_divas_quot_rem_u drv_divas_divide(unsigned numerator, unsigned denominator);

int __aeabi_idiv(int numerator, int denominator);
unsigned __aeabi_uidiv(unsigned numerator, unsigned denominator);
int __aeabi_idivmod(int numerator, int denominator);
unsigned __aeabi_uidivmod(unsigned numerator, unsigned denominator);

#endif /* DRV_DIVAS_H_ */