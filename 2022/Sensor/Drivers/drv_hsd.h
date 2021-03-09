/* 
 * File:   drv_hsd.h
 * Author: Connor
 *
 * Created on March 9, 2021, 11:25 AM
 */

#ifndef DRV_HSD_H
#define	DRV_HSD_H

#include <stdbool.h>

enum drv_hsd_channel {
	DRV_HSD_CHANNEL_SENSOR_1,
	DRV_HSD_CHANNEL_SENSOR_2,
	DRV_HSD_CHANNEL_SENSOR_3,
	DRV_HSD_CHANNEL_SENSOR_456,
	
	DRV_HSD_CHANNEL_COUNT
};

void drv_hsd_init(void);
// Takes ~50uS to change
void drv_hsd_setChannel(enum drv_hsd_channel channel);
bool drv_hsd_isFaulted(void);

#endif	/* DRV_HSD_H */

