import time

# Import all board pins.
import board
import busio
import random
import math

# Import the HT16K33 LED segment module.
from adafruit_ht16k33 import segments

NOWARN = 0b00000000
LWARN = 0b00001000
MWARN = 0b01001000
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
gearTempDisplay = segments.Seg7x4(i2c, auto_write=False, address=0x71)
print("Display created!")

print("Creating display.")
rpmDisplay = segments.Seg7x4(i2c, auto_write=False)
print("Display created!")

temp = 180
gear = 3
a = 0
while True:
    rand1 = random.randint(-20,20)
    temp = temp + rand1
    gear = gear + rand1/40
    gear = max(1, min(gear, 6))
    temp = max(0, min(temp, 400))
    print("Rand: " + str(rand1) + "\tTemp: " + str(temp) + "\tGear: " + str(gear))
    gearTempDisplay.fill(0)
    if temp < LTHRESH:
        gearTempDisplay.set_digit_raw(2, NOWARN)
    elif temp < MTHRESH:
        gearTempDisplay.set_digit_raw(2, LWARN)
    elif temp < HTHRESH:
        gearTempDisplay.set_digit_raw(2, MWARN)
    else:
        gearTempDisplay.set_digit_raw(2, HWARN)
    gearTempDisplay[0] = str(math.floor(gear))
    gearTempDisplay.show()
    
    rand = random.randint(-100,100)
    if rand > 0:
        rand = rand * 2;
    a = a + rand
    if a > 11000 or a < 0:
        a = 0
    rpmDisplay.fill(0)
    rpmDisplay.print(str("{:.2f}".format(a/1000)))
    time.sleep(.1)
    rpmDisplay.show()
