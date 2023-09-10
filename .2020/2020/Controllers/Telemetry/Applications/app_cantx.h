/*
 * app_cantx.h
 *
 * Created: 7/26/2020 11:34:53 AM
 *  Author: connor
 */ 


#ifndef APP_CANTX_H_
#define APP_CANTX_H_


void app_cantx_init(void);
void app_cantx_periodic(void);

#define APP_CANTX_START_TIMER(MSG) 	xTimerStart(xTimerCreate(#MSG, CYCLE_ ## MSG, pdTRUE, NULL, app_cantx_populate_ ## MSG), 0)


#endif /* APP_CANTX_H_ */