#pragma once
#define OSC48M_FREQUENCY 48000000U

enum drv_clock_channel {
	// Default clock, runs at 48MHz.
	DRV_CLOCK_CHANNEL_MAIN_CPU,
	// TODO: move CAN clock from drv_can to here

	DRV_CLOCK_CHANNEL_COUNT
};
