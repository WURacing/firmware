/* 
 * File:   drv_hsd.h
 * Author: Connor
 *
 * Created on March 9, 2021, 11:25 AM
 * 
 * Controls the high side drive chip to power the individual sensors
 * Chip is TPS4H000 ver B
 */

#ifndef DRV_HSD_H
#define	DRV_HSD_H

#include <stdbool.h>

// Power outputs from the high side drive chip
enum drv_hsd_channel {
	// Power for sensor 1
	DRV_HSD_CHANNEL_SENSOR_1,
	// Power for sensor 2
	DRV_HSD_CHANNEL_SENSOR_2,
	// Power for sensor 3
	DRV_HSD_CHANNEL_SENSOR_3,
	// Power for sensors 4, 5, and 6 are combined
	DRV_HSD_CHANNEL_SENSOR_456,
	
	DRV_HSD_CHANNEL_COUNT
};

// Set up inputs and outputs
void drv_hsd_init(void);
// Change the diagnostic channel on the HSD chip
// This controls which channel we're monitoring on the current-sense line and the faults line
// Takes ~50uS to change
void drv_hsd_setChannel(enum drv_hsd_channel channel);
// Check if any faults are reported
bool drv_hsd_isFaulted(void);

#endif	/* DRV_HSD_H */

