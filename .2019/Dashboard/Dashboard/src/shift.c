/*
 * shift.c
 *
 * Created: 3/23/2019 2:30:54 PM
 *  Author: Wash U Racing
 */

#include <asf.h>
#include "shift.h"

void set_leds(uint16_t ledstate)
{
	int i;
	ioport_set_pin_level(LED_LATCH, 1);
	ioport_set_pin_level(LED_LATCH, 0);
	ioport_set_pin_level(LED_SCLK, 0);
	for (i = 0; i < 16; ++i) {
		ioport_set_pin_level(LED_SIN, (ledstate >> i) & 0x1);
		ioport_set_pin_level(LED_SCLK, 1);
		ioport_set_pin_level(LED_SCLK, 0);
	}
}

void set_shiftlight_level(int level) {
	uint16_t ledstate = 0;
	int i;
	for (i = 0; i < 16; ++i) {
		if (i < level) {
			ledstate |= (1 << (15-i));
			} else {
			ledstate &= ~(1 << (15-i));
		}
	}
	set_leds(ledstate);
}
