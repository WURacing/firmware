#include "drv_i2c.h"
#include "sevenseg.h"

const uint8_t SevenSegmentDigits[10] = {
	0x3F, /* 0 */
	0x06, /* 1 */
	0x5B, /* 2 */
	0x4F, /* 3 */
	0x66, /* 4 */
	0x6D, /* 5 */
	0x7D, /* 6 */
	0x07, /* 7 */
	0x7F, /* 8 */
	0x6F, /* 9 */
};

const uint8_t DisplayAddresses[5] = {
    0x25,
    0x26,
    0x26,
    0x27,
    0x27
};

#define TRIES 100
static int init_mcp23017(uint8_t address)
{
	const uint8_t tx[] = {0x00, 0x00};
	int length = 0;
	int remaining = TRIES;
	while ((length = drv_i2c_write_register(DRV_I2C_CHANNEL_EXPANDERS, address, 0x00, tx, 2)) < 2
			&& remaining > 0)
	{
		--remaining;
	}
	return length;
}

int sevenseg_init(void) {
	int failures = 0;
    for(uint8_t address = 0x20; address < 0x28; address++) {
        int length = init_mcp23017(address);
		if (length != 2)
			++failures;
    }
	return failures;
}


void set_digit(uint8_t display_index, uint8_t digit) {
    if(digit > 9) {
        // Invalid digit
        return; 
    }
    
    uint8_t code = ~SevenSegmentDigits[digit];
    uint8_t address = DisplayAddresses[display_index];
    uint8_t tx = code;
    
    // even index digit displays are at 0x13
    uint8_t byte_two = display_index % 2 == 0;    
    uint8_t pointer = 0x12 + byte_two;
    
    drv_i2c_write_register(DRV_I2C_CHANNEL_EXPANDERS, address, pointer, &tx, 1);
}

void set_rpm(uint16_t rpm) {
    if(rpm > 10000) {
        // invalid rpm
        return;
    }
    
    // Write the last four displays
    for(int i = 1; i < 5; i++) {
        int digit = rpm % 10;
        rpm /= 10;
        set_digit(i, digit);
    }
}

void set_gear(uint8_t gear) {
    set_digit(0, gear);
}

void set_rgb_one(uint8_t r, uint8_t g, uint8_t b) {
    uint8_t rg_data[] = {~(r | (g << 7)), ~((g >> 1) | (b << 6))};
    drv_i2c_write_register(DRV_I2C_CHANNEL_EXPANDERS, 0x20, 0x12, rg_data, 2);
    
    uint8_t b_data[] = {~((b >> 2) | (0xF << 5))};
    drv_i2c_write_register(DRV_I2C_CHANNEL_EXPANDERS, 0x21, 0x12, b_data, 1);
}

void set_rgb_one_digit(uint8_t digit, enum rgb_color color) {
    if(digit > 9) {
        // Invalid digit
        return;
    }
    uint8_t code = SevenSegmentDigits[digit];
    
    // Set the red, green, and blue data based on whether the specified color 
    // contains each component.
    uint8_t r_data = (color & RGB_RED) ? code : 0;
    uint8_t g_data = (color & RGB_GREEN) ? code : 0;
    uint8_t b_data = (color & RGB_BLUE) ? code : 0;
    
    set_rgb_one(r_data, g_data, b_data);
}
