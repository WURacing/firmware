/* 
 * File:   app_inputs.h
 * Author: Connor
 *
 * Created on April 17, 2021, 2:26 PM
 */

#ifndef APP_INPUTS_H
#define	APP_INPUTS_H

#include <stdbool.h>


enum app_inputs_analog {
	APP_INPUTS_SW1,
	APP_INPUTS_SW2,
	APP_INPUTS_SW3,
	APP_INPUTS_SW4,
	
	APP_INPUTS_ANALOG_COUNT
};
#define NUM_DIALS APP_INPUTS_ANALOG_COUNT

enum app_inputs_digital {
	APP_INPUTS_DRS_L,
	APP_INPUTS_DRS_R,
	APP_INPUTS_FUSE,
	APP_INPUTS_MISC_L,
	APP_INPUTS_MISC_R,
	APP_INPUTS_SHIFT_DOWN,
	APP_INPUTS_SHIFT_UP,
	
	APP_INPUTS_DIGITAL_COUNT
};

void app_inputs_init(void);

int app_inputs_get_dial(enum app_inputs_analog dial);
bool app_inputs_get_button(enum app_inputs_digital button);

#endif	/* APP_INPUTS_H */

