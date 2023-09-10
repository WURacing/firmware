/**
 * \file
 *
 * \brief User board configuration template
 *
 */
/*
 * Support and FAQ: visit <a href="https://www.microchip.com/support/">Microchip Support</a>
 */

#ifndef CONF_BOARD_H
#define CONF_BOARD_H

#include <asf.h>

#define MY_LED    IOPORT_CREATE_PIN(PORTC, 0)

#define LED_SCLK  IOPORT_CREATE_PIN(PORTB, 0)
#define LED_SIN	  IOPORT_CREATE_PIN(PORTB, 7)
#define LED_LATCH IOPORT_CREATE_PIN(PORTD, 3)

#define SDA IOPORT_CREATE_PIN(PORTC, 4)
#define SCL IOPORT_CREATE_PIN(PORTC, 5)



#endif // CONF_BOARD_H
