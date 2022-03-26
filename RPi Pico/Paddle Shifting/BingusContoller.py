from machine import Pin
from time import sleep_ms
from rp2 import CAN, CANFrame, CANID, CANHack, MIN, CANIDFilter
from utime import sleep, sleep_ms
import machine
import time
from struct import pack, unpack

led = machine.Pin(25, machine.Pin.OUT, value=1)
shift_up_pin = machine.Pin(2, machine.Pin.OUT)
shift_down_pin = machine.Pin(1, machine.Pin.OUT)
shift_up_pin.low()
shift_down_pin.low()

def shift_up(pulse=150):
    shift_up_pin.high()
    time.sleep_ms(pulse) #sleep for pulse width in milliseconds
    shift_up_pin.low()

def shift_down(pulse=150):
    shift_down_pin.high()
    time.sleep_ms(pulse)      # sleep for pulse width in milliseconds
    shift_down_pin.low()

# Wait for a frame
def recv_wait(can):
    can.recv()  # Throw away any left-over pending frames
    while True:
        frames = can.recv()
        if len(frames) > 0:
            return frames

# The filter is a character string that defines the ID matching to take place. It is composed
# of only ‘X’, ‘1’ or ‘0’ and must be 11 or 29 characters long. A ‘X’ character means “don’t
# care”, a ‘1’ means “Must be 1” and ‘0’ means “Must be 0”.
# This needs to be fixed, current filter is 32 characters long (remove trailing zeroes?)
id_filters = {0: CANIDFilter(filter="10001100011000000000100000000000")}
can = CAN(id_filters)

while True:
    frames = recv_wait(can)
    for frame in frames:
        # Indicies and format string here might need to change
        data = unpack('>HHHH', frame.get_data())
        if data[2] == 255:
            print("upshift")
        if data[3] == 255:
            print("downshift")
