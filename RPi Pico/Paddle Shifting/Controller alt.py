# Author Chris Donnell, with sections copied from Hayden 
# Schroeder's code
# 
# **This is run in uPython**
# 
# This is a pure direct IO program, abandoning CAN completely.
# The ARB servos are statically set, as well as the DRS limits.
# There's inputs for DRS activation and paddle shifter signals.
# There's PWM outputs for the 3 servos and 2 digital outputs
# for the shifter.

from machine import Pin, PWM, const
from gc import collect
from utime import sleep_ms as t_sleep
from uasyncio import sleep_ms as a_sleep, get_event_loop as a_loop

# settings

_FARB_val = const(0) # deg
_RARB_val = const(0) # deg
_DRS_low = const(0) # deg
_DRS_high = const(30) # deg
_shift_pulse = const(150) # ms

# ouputs

sdp = Pin('sdp', Pin.OUT) # shift up pin
sup = Pin('sup', Pin.OUT) # shift down pin
fp = PWM(Pin('fp', Pin.OUT)) # FARB pin
rp = PWM(Pin('rp', Pin.OUT)) # RARB pin
dp = PWM(Pin('dp', Pin.OUT)) # DRS pin
redp = PWM(Pin('redp', Pin.OUT)) # red LED pin
grnp = PWM(Pin('grnp', Pin.OUT)) # green LED pin
blup = PWM(Pin('blup', Pin.OUT)) # blue LED pin

# inputs

UIsdp = Pin('UIsdp', Pin.IN) # shift up paddle pin
UIsup = Pin('UIsup', Pin.IN) # shift down paddle pin
UIDRSp = Pin('UIDSRp', Pin.IN) # DRS button pin

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
        
async def DRS():
    while True:
        if UIDRSp.value():
            set_servo(dp, _DRS_high)
        else:
            set_servo(dp, _DRS_low)
        await a_sleep(0)

async def RGB(LED, rate):
    while True:
        for duty in range(0, 65025, 1000):
            LED.duty_u16(duty)
            await a_sleep(rate)
        for duty in range(65025, 0, -1000):
            LED.duty_u16(duty)
            await a_sleep(rate)
            
async def gar():
    while True:
        collect()
        await a_sleep(2000)
        
set_servo(fp, _FARB_val)
set_servo(rp, _RARB_val)

# async program setup
        
loop=a_loop()
loop.create_task(gar()) # garbage collection, variable blocking code
loop.create_task(Bingus()) # _shift_pulse length blocking code
loop.create_task(DRS())
loop.create_task(RGB(redp, 5))
loop.create_task(RGB(grnp, 6))
loop.create_task(RGB(blup, 7))
loop.run_forever() 
