/*
 * drv_lte.h
 *
 * Created: 8/8/2020 11:13:52 AM
 *  Author: connor
 */ 


#ifndef DRV_LTE_H_
#define DRV_LTE_H_

#include <stdint.h>
#include <stdbool.h>


struct drv_lte_location
{
	int latitude, longitude;
};
struct drv_lte_time
{
	int year, month, day, hour, minute, second;
};

void drv_lte_init(void);
bool drv_lte_configure(void);
bool drv_lte_is_network_registered(void);
bool drv_lte_is_logged_in(void);
bool drv_lte_mqtt_login(void);
bool drv_lte_mqtt_publish(const char *topic, const char *message);

const struct drv_lte_location * drv_lte_get_last_location(void);
const struct drv_lte_time * drv_lte_get_last_time(void);




#endif /* DRV_LTE_H_ */