import time

# Import all board pins.
import board
import busio
import random

# Import the HT16K33 LED segment module.
from adafruit_ht16k33 import segments

# Create the I2C interface.
#                    (SCL, SDA)
i2c = busio.I2C(board.GP5, board.GP4)
print("I2C created!")

# Create the LED segment class.
# This creates a 7 segment 4 character display:
display = segments.Seg7x4(i2c, auto_write=False)
print("Display created!")

while True:
#     a = 0
#     while a < 11000 and a > -1:
#         rand = random.randint(-100,100)
#         if rand > 0:
#             rand = rand * 2;
#         a = a + rand
#         display.fill(0)
#         display.print(str("{:.2f}".format(a/1000)))
#         display.show()
#         time.sleep(.1)
    display.print("1")
    display.show()
    print("Running!")