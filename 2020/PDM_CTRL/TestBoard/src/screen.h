/*
 * screen.h
 *
 * Created: 1/18/2020 6:19:48 PM
 *  Author: etshr
 */ 


#ifndef SCREEN_H_
#define SCREEN_H_

#include <asf.h>

enum status_code configure_screen(void);

enum status_code mcp23017_send_data(uint8_t dataA, uint8_t dataB);

enum status_code screen_set_string(const char * message);

void screen_render(void);


#endif /* SCREEN_H_ */