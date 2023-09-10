/* 
 * File:   drv_rtc.h
 * Author: Connor
 *
 * Created on March 7, 2021, 11:57 AM
 *
 * Purpose: Implement real time clock interface (calendar, seconds resolution) over i2c with DS1307. Used by FatFs.
 */

#ifndef DRV_RTC_H
#define	DRV_RTC_H

void drv_rtc_init(void);
void drv_rtc_periodic(void);

#endif	/* DRV_RTC_H */

