/* 
 * File:   drv_ws2812b.h
 * Author: connor
 *
 * Created on December 25, 2020, 4:33 PM
 */

#ifndef DRV_WS2812B_H
#define	DRV_WS2812B_H

#include "drv_ws2812b_private.h"
#include <stdint.h>

struct drv_ws2812b_config {
	const uint32_t pinMap[DRV_WS2812B_CHANNEL_COUNT];
};
extern struct drv_ws2812b_config drv_ws2812b_config;

void drv_ws2812b_init(void);
void drv_ws2812b_transmit(enum drv_ws2812b_channel led, const uint8_t *data, int length);

#endif	/* DRV_WS2812B_H */

