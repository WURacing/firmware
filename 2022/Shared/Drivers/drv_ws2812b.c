#include "drv_ws2812b.h"
#include <sam.h>



static inline void blit0(__IO uint32_t * outset, __IO uint32_t * outclr, uint32_t port)
{
	// Critical timing, depends on 16MHz CPU clock
	asm(
	"str %[port], [%[outset], #0]\n\t"
	"nop\n\t"
	"nop\n\t"
	"nop\n\t"
	"str %[port], [%[outclr], #0]\n\t"
	"nop\n\t"
	"nop\n\t"
	"nop\n\t"
	"nop\n\t"
	"nop\n\t"
	"nop\n\t"
	"nop\n\t"
	"nop\n\t"
	"nop\n\t"
	"nop\n\t"
	"nop\n\t"
	"nop\n\t"
	"nop\n\t"
	"nop\n\t"
	:: [port] "r" (port), [outset] "r" (outset), [outclr] "r" (outclr));
}

static inline void blit1(__IO uint32_t * outset, __IO uint32_t * outclr, uint32_t port)
{
	// this is specific to WS2812B V5 timing -- need to change for regular WS2812B
	asm(
	"str %[port], [%[outset], #0]\n\t"
	"nop\n\t"
	"nop\n\t"
	"nop\n\t"
	"nop\n\t"
	"nop\n\t"
	"nop\n\t"
	"nop\n\t"
	"nop\n\t"
	"nop\n\t"
	"str %[port], [%[outclr], #0]\n\t"
	"nop\n\t"
	"nop\n\t"
	"nop\n\t"
	"nop\n\t"
	"nop\n\t"
	"nop\n\t"
	"nop\n\t"
	"nop\n\t"
	"nop\n\t"
	:: [port] "r" (port), [outset] "r" (outset), [outclr] "r" (outclr));
}

void drv_ws2812b_init(void)
{
	for (enum drv_ws2812b_channel led = (enum drv_ws2812b_channel)0U; led < DRV_WS2812B_CHANNEL_COUNT; ++led)
	{
		uint32_t pin = drv_ws2812b_config.pinMap[led];
		PORT_REGS->GROUP[pin / 32].PORT_DIRSET = 1 << (pin % 32);
	}
}

void drv_ws2812b_transmit(enum drv_ws2812b_channel led, const uint8_t *data, int length)
{
	uint32_t pin = drv_ws2812b_config.pinMap[led];
	__IO uint32_t * outset = &PORT_REGS->GROUP[pin / 32].PORT_OUTSET;
	__IO uint32_t * outclr = &PORT_REGS->GROUP[pin / 32].PORT_OUTCLR;
	uint32_t port = 1 << (pin % 32);

	__disable_irq();
	for (int i = 0; i < length; ++i)
	{
		uint8_t b = data[i];
		// heuristic: probably going to be outputting more 0's in general
		if (__builtin_expect(b & 0b10000000, 0))
		{
			blit1(outset, outclr, port);
		}
		else
		{
			blit0(outset, outclr, port);
		}
		if (__builtin_expect(b & 0b01000000, 0))
		{
			blit1(outset, outclr, port);
		}
		else
		{
			blit0(outset, outclr, port);
		}
		if (__builtin_expect(b & 0b00100000, 0))
		{
			blit1(outset, outclr, port);
		}
		else
		{
			blit0(outset, outclr, port);
		}
		if (__builtin_expect(b & 0b00010000, 0))
		{
			blit1(outset, outclr, port);
		}
		else
		{
			blit0(outset, outclr, port);
		}
		if (__builtin_expect(b & 0b00001000, 0))
		{
			blit1(outset, outclr, port);
		}
		else
		{
			blit0(outset, outclr, port);
		}
		if (__builtin_expect(b & 0b00000100, 0))
		{
			blit1(outset, outclr, port);
		}
		else
		{
			blit0(outset, outclr, port);
		}
		if (__builtin_expect(b & 0b00000010, 0))
		{
			blit1(outset, outclr, port);
		}
		else
		{
			blit0(outset, outclr, port);
		}
		if (__builtin_expect(b & 0b00000001, 0))
		{
			blit1(outset, outclr, port);
		}
		else
		{
			blit0(outset, outclr, port);
		}
	}
	__enable_irq();
}