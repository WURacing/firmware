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
# This is V2, the better optimized but hopefully more human
# readible. I am much less worried about RAM, this is likely
# and adequate program.

from machine import Pin, PWM, const
from gc import collect
from rp2 import CAN, CANIDFilter
from utime import sleep_ms as t_sleep
from uasyncio import sleep_ms as a_sleep, get_event_loop as a_loop
from ustruct import unpack

# CAN data

can = CAN(id_filters={0: CANIDFilter(filter="00000001011")})
data = ()
_FARB_data = const(1)
_RARB_data = const(2)
_DRS_data = const(3)
_shift_pulse

# ouputs

_shift_pulse = const(150) # ms
sdp = Pin('sdp', Pin.OUT)
sup = Pin('sup', Pin.OUT)
fp = PWM(Pin('fp', Pin.OUT))
rp = PWM(Pin('rp', Pin.OUT))
dp = PWM(Pin('dp', Pin.OUT))
redp = PWM(Pin('redp', Pin.OUT))
grnp = PWM(Pin('grnp', Pin.OUT))
blup = PWM(Pin('blup', Pin.OUT))

# inputs

UIsdp = Pin('UIsdp', Pin.IN)
UIsup = Pin('UIsup', Pin.IN)

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
    sdp.low()
    sup.low()
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

async def FARB():
    while True:
        set_servo(fp, data[_FARB_data])
        await a_sleep(0)

async def RARB():
    while True:
        set_servo(rp, data[_RARB_data])
        await a_sleep(0)
        
async def DRS():
    while True:
        set_servo(dp, data[_DRS_data])
        await a_sleep(0)

async def RGB(LED, rate):
    while True:
        for duty in range(0, 65025, 1000):
            LED.duty_u16(duty)
            await a_sleep(rate)
        for duty in range(65025, 0, -1000):
            LED.duty_u16(duty)
            await a_sleep(rate)
            
async def CANin():
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
loop.create_task(FARB())
loop.create_task(RARB())
loop.create_task(DRS())
loop.create_task(RGB(redp, 5))
loop.create_task(RGB(grnp, 6))
loop.create_task(RGB(blup, 7))
loop.run_forever() 
