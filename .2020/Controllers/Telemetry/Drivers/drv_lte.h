/*
 * drv_lte.h
 *
 * Created: 8/8/2020 11:13:52 AM
 *  Author: connor
 */ 


#ifndef DRV_LTE_H_
#define DRV_LTE_H_

#include <stdint.h>

void drv_lte_init(void);
void drv_lte_periodic(void);

struct drv_lte_location
{
	int latitude, longitude;
};
struct drv_lte_time
{
	int year, month, day, hour, minute, second;
};

const struct drv_lte_location * drv_lte_get_last_location(void);
const struct drv_lte_time * drv_lte_get_last_time(void);

uint8_t * drv_lte_get_transmission_queue(void);
void drv_lte_queue_transmission(int length);
void drv_lte_cancel_transmission(void);



#endif /* DRV_LTE_H_ */