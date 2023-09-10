import time

# Import all board pins.
import board
import busio

# Import the HT16K33 LED segment module.
from adafruit_ht16k33 import segments

from rp2 import CAN, CANIDFilter
from struct import unpack

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

id_filters = {0: CANIDFilter(filter="01100111111111111000001001000")} #Filter needs to change here 
can = CAN(profile=CAN.BITRATE_250K_75, id_filters=id_filters)

# Wait for a frame
def recv_wait(can):
    can.recv()  # Throw away any left-over pending frames
    while True:
        frames = can.recv()
        if len(frames) > 0:
            return frames
        
def print_gear_and_temp(gear, temp)
    

temp = 180
gear = 3
while True:
    
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
