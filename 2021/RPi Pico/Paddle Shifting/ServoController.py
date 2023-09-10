from machine import Pin, PWM
from time import sleep_ms
from rp2 import CAN, CANFrame, CANID, CANHack, MIN, CANIDFilter
import utime
import machine
import time
from struct import pack, unpack

FRONT_ARB = 1
REAR_ARB = 2
DRS = 3

pwm = PWM(dest=Pin(1), freq=333, duty_u16=32768, duty_ns=1500000) #TODO: Change pin number for real use

# Wait for a frame
def recv_wait(can):
    can.recv()  # Throw away any left-over pending frames
    while True:
        frames = can.recv()
        if len(frames) > 0:
            return frames

# Set servo to degree in reference to operating travel
def set_servo(deg):
    duty_ns = (10000*deg)+1000000
    pwm.duty_ns(duty_ns)

id_filters = {0: CANIDFilter(filter="00000001011")}
can = CAN(id_filters=id_filters)

while True:
    frames = recv_wait(can)
    for frame in frames:
        data = unpack('>bbbb', frame.get_data())
        set_servo(data[FRONT_ARB])
        # set_servo(data[REAR_ARB])
        # set_servo(data[DRS])

