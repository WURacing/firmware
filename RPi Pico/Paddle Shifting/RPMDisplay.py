# Import all board pins.
import board
import busio

# Import the HT16K33 LED segment module.
from adafruit_ht16k33 import segments

from rp2 import CAN, CANIDFilter
from struct import unpack

# Create the I2C interface.
#                    (SCL, SDA)
i2c = busio.I2C(board.GP5, board.GP4)
print("I2C created!")

# Create the LED segment class.
# This creates a 7 segment 4 character display:
display = segments.Seg7x4(i2c, auto_write=False)
print("Display created!")

# id "10001100111111111111000001001000" too big, 33 chars
id_filters = {0: CANIDFilter(filter="00000001011")}
can = CAN(id_filters=id_filters)

# Wait for a frame
def recv_wait(can):
    can.recv()  # Throw away any left-over pending frames
    while True:
        frames = can.recv()
        if len(frames) > 0:
            return frames

def print_rpm(rpm):
    display.fill(0)
    display.print(str("{:.2f}".format(rpm/1000)))
    display.show()

while True:
    frames = recv_wait(can)
    for frame in frames:
        data = unpack('>HHHh', frame.get_data())
        print_rpm(data[0])