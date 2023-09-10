/* 
 * File:   drv_tcc.h
 * Author: Connor
 *
 * Created on May 2, 2021, 11:28 AM
 */

#ifndef DRV_TCC_H
#define	DRV_TCC_H

#include "sam.h"
#include "drv_tcc_private.h"

#define DRV_TCC_NUM_PINMUX 8
#define DRV_TCC_NUM_CC 4

struct drv_tcc_channelConfig {
	int tccid;
	tcc_registers_t * module;
	int gclk;
	uint32_t pinmux[DRV_TCC_NUM_PINMUX];
	uint16_t per;
	uint16_t cc[DRV_TCC_NUM_CC];
};

struct drv_tcc_config {
	const struct drv_tcc_channelConfig channelConfig[DRV_TCC_CHANNEL_COUNT];
};

extern const struct drv_tcc_config drv_tcc_config;

void drv_tcc_init(void);
void drv_tcc_set_cc(enum drv_tcc_channel channel, int num_cc, int val);


#endif	/* DRV_TCC_H */

