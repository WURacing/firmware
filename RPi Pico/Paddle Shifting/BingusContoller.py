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

filter = CANIDFilter(filter="10001100011000000000100000000000")

can = CAN(id_filters=filter)

while True:
    frames = recv_wait(can)
    for frame in frames:
        data = unpack('>HHHH', frame.get_data())[0] # Not sure why this [0] is here
        if data[2] == 255:
            print("upshift")
        if data[3] == 255:
            print("downshift")
