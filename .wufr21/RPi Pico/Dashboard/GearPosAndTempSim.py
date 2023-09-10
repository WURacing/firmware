import time

# Import all board pins.
import board
import busio
import random
import math

# Import the HT16K33 LED segment module.
from adafruit_ht16k33 import segments

NOWARN = 0b00000000
LWARN = 0b00000001
MWARN = 0b00001001
HWARN = 0b01001001

LTHRESH = 100
MTHRESH = 200
HTHRESH = 300

# Create the I2C interface.
#                    (SCL, SDA)
print("Creating I2C.")
i2c = busio.I2C(board.GP15, board.GP14)
print("I2C created!")

# Create the LED segment class.
# This creates a 7 segment 4 character display:
print("Creating display.")
display = segments.Seg7x4(i2c, auto_write=False, address=0x71)
print("Display created!")

temp = 180
gear = 3
while True:
    rand = random.randint(-20,20)
    temp = temp + rand
    gear = gear + rand/40
    gear = max(1, min(gear, 6))
    temp = max(0, min(temp, 400))
    print("Rand: " + str(rand) + "\tTemp: " + str(temp) + "\tGear: " + str(gear))
    display.fill(0)
    if temp < LTHRESH:
        display.set_digit_raw(2, NOWARN)
    elif temp < MTHRESH:
        display.set_digit_raw(2, LWARN)
    elif temp < HTHRESH:
        display.set_digit_raw(2, MWARN)
    else:
        display.set_digit_raw(2, HWARN)
    display[0] = str(math.floor(gear))
    time.sleep(.1)
    display.show()