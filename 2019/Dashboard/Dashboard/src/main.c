/**
 * \file
 *
 * \brief Empty user application template
 *
 */

/**
 * \mainpage User Application template doxygen documentation
 *
 * \par Empty user application template
 *
 * Bare minimum empty user application template
 *
 * \par Content
 *
 * -# Include the ASF header files (through asf.h)
 * -# "Insert system clock initialization code here" comment
 * -# Minimal main function that starts with a call to board_init()
 * -# "Insert application code here" comment
 *
 */

/*
 * Include header files for all drivers that have been imported from
 * Atmel Software Framework (ASF).
 */
/*
 * Support and FAQ: visit <a href="https://www.microchip.com/support/">Microchip Support</a>
 */
#include <asf.h>
#include <stdio.h>
#include "sevenseg.h"
#include "shift.h"
#include <mcp2515/mcp2515.h>
#include "CANBus18.h"



void die_error(int error);
void set_rpm_str(const char *rstr);
void set_gear_str(char gear);
void set_gear(int gear);
void set_rpm(int rpm);
void set_coolant(float degF);


int timer_state = 0;
char display[10] = "LOAD-";
int curdigit = 0;
int showdp = 0;
int shiftlevel = 0;
int oheat_status = 0;



void die_error(int error)
{
	while (1) {
		ioport_set_pin_level(MY_LED, 0);
		set_leds(error<<4);
	}
}

void set_rpm_str(const char *rstr)
{
	display[0] = rstr[0];
	display[1] = rstr[1];
	display[2] = rstr[2];
	display[3] = rstr[3];
}

void set_gear_str(char gear)
{
	display[4] = gear;
}

void set_gear(int gear)
{
	set_gear_str(gear + 0x30);
}

void set_rpm(int rpm)
{
	char rstr[5];
	showdp = 0;

	if (rpm > 20000 || rpm < 0)
	{
		set_rpm_str("EEEE");
		return;
	}
	
	snprintf(rstr, 5, "%4d", rpm/10);
	set_rpm_str(rstr);
	
	if (rpm >= 1000)
		showdp = 1;
	
	shiftlevel = rpm / 1084;
	
}

void set_coolant(float degF)
{
	oheat_status = (degF > 200);
}

int rx_msg_count = 0;

int main (void)
{
	int error;
	tCAN message;
	
	sysclk_init();
	// Enable clock for timers, I2C, and SPI
	sysclk_enable_module(POWER_RED_REG0, PRTIM0_bm);
	sysclk_enable_module(POWER_RED_REG0, PRTWI_bm);
	sysclk_enable_module(POWER_RED_REG0, PRSPI_bm);
	ioport_init();
	board_init();
	
	// Try communicating with seven seg
	error = initsevenseg();
	if (error) die_error(error);
	
	// Try communicating with CAN controller
	error = !mcp2515_init(1);
	if (error) die_error(0b10101010);
	
	// Initialize outputs
	delay_ms(1);
	set_leds(0);
	snprintf(display, sizeof(display), "%5lu", 12345LU);
	
	set_rpm_str("CONN");
	set_gear_str(' ');
	shiftlevel = 0;
	sei();
		TIMSK0 = (1<<OCIE0A);
	
	// Loop
	while (1) {
		// Read new CAN packets
		if (mcp2515_check_message()) {
			if (mcp2515_get_message(&message)) {
				if ((message.id & 0x1fffffff) == ID_AEMEngine0) {
					// Received an engine packet
					set_rpm((int) CALC_AEMEngine0_EngineSpeed(GET_AEMEngine0_EngineSpeed(message.data), 1.f));
					set_coolant(CALC_AEMEngine0_CoolantTemp(GET_AEMEngine0_CoolantTemp(message.data), 1.f));
				}
			}
		}
	}

}




SIGNAL(TIMER0_COMPA_vect)
{
	cli();
	++timer_state;

	// enable shift lights only for a fraction of time
	if ((timer_state & 0xF) == 0)
		set_shiftlight_level(shiftlevel);
	else
		set_leds(0);

	// overheat - inverse logic
	ioport_set_pin_level(MY_LED, !oheat_status);
	
	switch (timer_state & 0x2) {
		case 0:
		if (++curdigit >= 5) curdigit = 0;
		mcp23017_write(0, 0); // reset all transistors
		break;
		case 1:
		case 2:
		mcp23017_write(SevenSegmentASCII[display[curdigit] - 0x20] | ((showdp & (curdigit == 1))<<7), 1<<curdigit); // draw digit and decimal point after 2nd position
		break;
		case 3:
		mcp23017_write(0, 0xFF); // drain capacitance
		break;
	}
	sei();
}