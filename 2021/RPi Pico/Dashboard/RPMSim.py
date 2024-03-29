import time

# Import all board pins.
import board
import busio
import random

# Import the HT16K33 LED segment module.
from adafruit_ht16k33 import segments

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

while True:
    a = 0
    while a < 11000 and a > -1:
        print("Running!")
        rand = random.randint(-100,100)
        if rand > 0:
            rand = rand * 2;
        a = a + rand
        display.fill(0)
        display.print(str("{:.2f}".format(a/1000)))
        display.show()
        time.sleep(.1)