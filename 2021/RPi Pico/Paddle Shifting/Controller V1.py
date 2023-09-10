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
# This is V1, the least optimized. I worry a bit about RAM
# because of the number and length of global variables.

from machine import Pin, PWM
from gc import collect
from rp2 import CAN, CANIDFilter
from utime import sleep_ms as t_sleep
from uasyncio import sleep_ms as a_sleep, get_event_loop as a_loop
from ustruct import unpack

# CAN data

can = CAN(id_filters={0: CANIDFilter(filter="00000001011")})
data = ()
FARB_data = 1
RARB_data = 2
DRS_data = 3

# ouputs

_shift_pulse = const(150) # ms
shift_down_pin = Pin('shift_down_pin', Pin.OUT)
shift_up_pin = Pin('shift_up_pin', Pin.OUT)
FARB_pin = PWM(Pin('FARB_pin', Pin.OUT), freq=333)
RARB_pin = PWM(Pin('RARB_pin', Pin.OUT), freq=333)
DRS_pin = PWM(Pin('DRS_pin', Pin.OUT), freq=333)
button_red = PWM(Pin('button_red', Pin.OUT), freq=333)
button_green = PWM(Pin('button_green', Pin.OUT), freq=333)
button_blue = PWM(Pin('button_blue', Pin.OUT), freq=333)

# inputs

UI_shift_down_pin = Pin('UI_shift_down_pin', Pin.IN)
UI_shift_up_pin = Pin('UI_shift_up_pin', Pin.IN)

# util functions
        
def shift(pin, pulse=_shift_pulse):
    pin.high()
    t_sleep(pulse)
    pin.low()

def set_servo(pwm, deg):
    duty = (2**16/180*deg)+2**15
    pwm.duty_u16(duty)
    
# core functions

async def BingusContoller():
    shift_down_pin.low()
    shift_up_pin.low()
    d = 0
    u = 0
    while True:
        d = UI_shift_down_pin.value()
        u = UI_shift_up_pin.value()
        if d ^ u:
            if d:
                shift(shift_down_pin)
            if u:
                shift(shift_up_pin)
            await a_sleep(150)
        else:
            await a_sleep(0)

async def FARBController():
    while True:
        set_servo(FARB_pin, data[FARB_data])
        await a_sleep(0)

async def RARBController():
    while True:
        set_servo(RARB_pin, data[RARB_data])
        await a_sleep(0)
        
async def DRSController():
    while True:
        set_servo(DRS_pin, data[DRS_data])
        await a_sleep(0)

async def StarterButtonRGB(LED, rate):
    while True:
        for duty in range(0, 2**16-1, 2**8-1):
            LED.duty_u16(duty)
            await a_sleep(rate)
        for duty in range(2**16-1, 0, -2**8-1):
            LED.duty_u16(duty)
            await a_sleep(rate)
            
async def CAN_UI():
    while True:
        collect()
        await a_sleep(0)
        frames = can.recv()
        global data = unpack('>bbbb', frames[-1])
        await a_sleep(0)
            
# async program setup
        
loop=a_loop()
loop.create_task(CAN_UI()) # variable blocking code
loop.create_task(BingusContoller()) # 150 ms blocking code
loop.create_task(FARBController())
loop.create_task(RARBController())
loop.create_task(DRSController())
loop.create_task(StarterButtonRGB(button_red, 5))
loop.create_task(StarterButtonRGB(button_green, 6))
loop.create_task(StarterButtonRGB(button_blue, 7))
loop.run_forever() 
