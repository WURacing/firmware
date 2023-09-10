/*
 * app_telemetry.h
 * Read received CAN messages and transmit them over LTE
 *
 * Created: 8/16/2020 11:40:36 AM
 *  Author: connor
 */ 


#ifndef APP_TELEMETRY_H_
#define APP_TELEMETRY_H_


void app_telemetry_init(void);
void app_telemetry_periodic(void);

int app_telemetry_sent_messages(void);


#endif /* APP_TELEMETRY_H_ */