/* 
 * File:   sevenseg.h
 * Author: bsiderowf
 *
 * Created on October 31, 2020, 12:56 PM
 */

#ifndef SEVENSEG_H
#define	SEVENSEG_H

enum rgb_color {
    RGB_RED = 1, 
    RGB_GREEN = 2, 
    RGB_BLUE = 4,
    RGB_YELLOW = RGB_RED | RGB_GREEN,
    RGB_MAGENTA = RGB_RED | RGB_BLUE,
    RGB_CYAN =  RGB_GREEN | RGB_BLUE,
    RGB_WHITE = RGB_RED | RGB_GREEN | RGB_BLUE
};

int sevenseg_init(void);
void set_digit(uint8_t display_index, uint8_t digit);
void set_rpm(uint16_t rpm);
void set_gear(uint8_t gear);
void set_rgb_one_digit(uint8_t digit, enum rgb_color color);

#endif	/* SEVENSEG_H */

