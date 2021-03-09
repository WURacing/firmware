#pragma once
#define OSC48M_FREQUENCY 48000000U

enum drv_clock_channel {
	// Default clock, runs at 48MHz.
	DRV_CLOCK_CHANNEL_MAIN_CPU,
	// Clock for servo purposes, runs at 1MHz.
	// Why? Servo expects ~50Hz/20ms signaling. So the period of 20,000 clock
	// cycles fits into 16 bits.
	DRV_CLOCK_CHANNEL_SERVO,

	DRV_CLOCK_CHANNEL_COUNT
};
