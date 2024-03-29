# Import all board pins.
import board
import busio

# Import the HT16K33 LED segment module.
from adafruit_ht16k33 import segments

from rp2 import CAN, CANIDFilter
from struct import unpack

# Create the I2C interface.
#                    (SCL, SDA)
print("Creating I2C.")
i2c = busio.I2C(board.GP15, board.GP14)
print("I2C created!")

# Create the LED segment class.
# This creates a 7 segment 4 character display:
print("Creating display.")
display = segments.Seg7x4(i2c, auto_write=False)
print("Display created!")

id_filters = {0: CANIDFilter(filter="01100111111111111010101001000")}
can = CAN(profile=CAN.BITRATE_250K_75, id_filters=id_filters)

# Wait for a frame
def recv_wait(can):
    can.recv()  # Throw away any left-over pending frames
    while True:
        frames = can.recv()
        if len(frames) > 0:
            return frames

def print_voltage(voltage):
    display.fill(0)
    display.print(str("{:.2f}".format(voltage)))
    display.show()

while True:
    frames = recv_wait(can)
    for frame in frames:
        data = unpack('>hhhB', frame.get_data())
        voltage = data[0]/1000
        print_voltage(voltage)
        print(voltage)
