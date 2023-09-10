/**
 * \file
 *
 * \brief User board initialization template
 *
 */
/*
 * Support and FAQ: visit <a href="https://www.microchip.com/support/">Microchip Support</a>
 */

#include <asf.h>
#include <board.h>
#include <conf_board.h>
#include <sevenseg.h>

void board_init(void)
{
	/* This function is meant to contain board-specific initialization code
	 * for, e.g., the I/O pins. The initialization can rely on application-
	 * specific board configuration, found in conf_board.h.
	 */
	
	ioport_set_pin_dir(MY_LED, IOPORT_DIR_OUTPUT);
	ioport_set_pin_dir(LED_SCLK, IOPORT_DIR_OUTPUT);
	ioport_set_pin_dir(LED_SIN, IOPORT_DIR_OUTPUT);
	ioport_set_pin_dir(LED_LATCH, IOPORT_DIR_OUTPUT);
	ioport_set_pin_mode(SDA, IOPORT_MODE_PULLUP);
	ioport_set_pin_mode(SCL, IOPORT_MODE_PULLUP);
	
	ioport_set_pin_level(MY_LED, 1);

	cli();
	TCCR0A =  (1<<WGM01) | (1<<WGM00);
	TCCR0B = (1<<CS01) | (1<<CS00) | (1<<WGM02);
	OCR0A = 70;
	
}
