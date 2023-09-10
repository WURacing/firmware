/* 
 * File:   app_datalogger.h
 * Author: Connor
 *
 * Created on April 18, 2021, 5:35 PM
 */

#ifndef APP_DATALOGGER_H
#define	APP_DATALOGGER_H

#include <stdbool.h>
#include <stdint.h>

#define SERVO_POSITIONS 12

struct servo_config
{
	int8_t eARBFrontPulses[SERVO_POSITIONS];
	int8_t eARBRearPulses[SERVO_POSITIONS];
	int8_t drsOpenPulse, drsClosedPulse;
};

void app_datalogger_init(void);
bool app_datalogger_okay(void);
bool app_datalogger_read_data(void);
const struct servo_config * app_datalogger_get_servo_positions(void);

#endif	/* APP_DATALOGGER_H */

