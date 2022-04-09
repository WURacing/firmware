from machine import Pin
from time import sleep_ms
from rp2 import CAN, CANFrame, CANID, CANHack, MIN, CANIDFilter
import utime
import machine
import time
from struct import pack, unpack

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

id_filters = {0: CANIDFilter(filter="00000001011")}
can = CAN(id_filters=id_filters)

while True:
    frames = recv_wait(can)
    for frame in frames:
        data = unpack('>bbbb', frame.get_data())
        if data[0] == 2:
            print("upshift")
        elif data[0] == 1:
            print("downshift")
        else:
            print(frame.get_canid(), end ='\t')
            print(data)    