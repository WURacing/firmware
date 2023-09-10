# Author Chris Donnell, with sections copied from Hayden 
# Schroeder's code
# 
# **This is run in uPython**
#
# This is a first try at implementing a full controller in
# uPython. This has the full suite of CAN control, and should
# more or less replicate the functionality of the existing
# controller board, minus shifting.
#
# This is V3, the most optimized. My fear with this program is
# that it's very difficult to read, and I'm not sure to what
# degree the optimizations are necessary. Nevertheless, I've 
# included it for the full spectrum of optimality. This might 
# be how a final deployment version looks, or similar.

from machine import Pin, PWM, const
from gc import collect
from rp2 import CAN, CANIDFilter
from utime import sleep_ms as t_sleep
from uasyncio import sleep_ms as a_sleep, get_event_loop as a_loop
from ustruct import unpack

# CAN data

data = ()
_FARB_data = const(1)
_RARB_data = const(2)
_DRS_data = const(3)

# outputs

_shift_down_pin = const('shift_down_pin')
_shift_up_pin = const('shift_up_pin')
_FARB_pin = const('FARB_pin')
_RARB_pin = const('RARB_pin')
_DRS_pin = const('DRS_pin')
_button_red_pin = const('button_red_pin')
_button_green_pin = const('button_green_pin')
_button_blue_pin = const('button_blue_pin')

_shift_pulse = const(150) # ms

# inputs

_UI_shift_down_pin = const('UI_shift_down_pin')
_UI_shift_up_pin = const('UI_shift_up_pin')

# util functions
        
def shift(pin, pulse=_shift_pulse):
    pin.high()
    t_sleep(pulse)
    pin.low()

def set_servo(pwm, deg):
    duty = (2**16/180*deg)+2**15
    pwm.duty_u16(duty)
    
# core functions

async def Bingus():
    sdp = Pin(_shift_down_pin, Pin.OUT, value=0)
    sup = Pin(_shift_up_pin, Pin.OUT, value=0)
    UIsdp = Pin(_UI_shift_down_pin, Pin.IN)
    UIsup = Pin(_UI_shift_up_pin, Pin.IN)
    d = 0
    u = 0
    while True:
        d = UIsdp.value()
        u = UIsup.value()
        if d ^ u:
            if d:
                shift(sdp)
            if u:
                shift(sup)
            await a_sleep(150)
        else:
            await a_sleep(0)

async def Servo(pin_i,data_i):
    global data
    fp = PWM(Pin(pin_i, Pin.OUT))
    while True:
        set_servo(fp, data[data_i])
        await a_sleep(0)

async def RGB(pin_i, rate):
    LED = PWM(Pin(pin_i, Pin.OUT))
    while True:
        for duty in range(0, 65025, 1000):
            LED.duty_u16(duty)
            await a_sleep(rate)
        for duty in range(65025, 0, -1000):
            LED.duty_u16(duty)
            await a_sleep(rate)
            
async def CANin():
    can = CAN(id_filters={0: CANIDFilter(filter="00000001011")})
    while True:
        collect()
        await a_sleep(0)
        frames = can.recv()
        global data = unpack('>bbbb', frames[-1])
        await a_sleep(0)
            
# async program setup
        
loop=a_loop()
loop.create_task(CANin()) # variable blocking code
loop.create_task(Bingus()) # 150 ms blocking code
loop.create_task(Servo(_FARB_pin,_FARB_data))
loop.create_task(Servo(_RARB_pin,_RARB_data))
loop.create_task(Servo(_DRS_pin,_DRS_data))
loop.create_task(RGB(_button_red_pin, 5))
loop.create_task(RGB(_button_green_pin, 6))
loop.create_task(RGB(_button_blue_pin, 7))
loop.run_forever() 
