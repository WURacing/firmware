/*
 * current.h
 *
 * Created: 1/18/2020 3:32:37 PM
 *  Author: etshr
 */ 


#ifndef CURRENT_H_
#define CURRENT_H_

#include <asf.h>

struct current_readings {
	uint16_t lc[17];
	uint16_t fuel_pump, rad_fan, total;
};

enum status_code configure_current(void);

void read_current(struct current_readings * output);


#endif /* CURRENT_H_ */