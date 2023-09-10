/* 
 * File:   drv_tcc_private.h
 * Author: Connor
 *
 * Created on May 2, 2021, 11:29 AM
 */

#ifndef DRV_TCC_PRIVATE_H
#define	DRV_TCC_PRIVATE_H

// Set to 0 to send no command (keep position), 1500 to return to neutral.
#define SERVO_SAFETY 0

enum drv_tcc_channel
{
	DRV_TCC_CHANNEL_ARB,
	DRV_TCC_CHANNEL_DRS,
	
	DRV_TCC_CHANNEL_COUNT
};

enum drv_tcc_channel_arb_cc
{
	DRV_TCC_CHANNEL_ARB_CC_FRONT,
	DRV_TCC_CHANNEL_ARB_CC_REAR,
};

enum drv_tcc_channel_drs_cc
{
	DRV_TCC_CHANNEL_DRS_CC_REAR,
};

#endif	/* DRV_TCC_PRIVATE_H */

