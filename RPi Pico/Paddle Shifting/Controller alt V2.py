# Author Chris Donnell, with sections copied from Hayden 
# Schroeder's code
# 
# **This is run in uPython**
#
# This is the furthest shorted program - the only code is for
# the Bingus controller. There's 4 IO, 2 for paddle inputs and
# 2 for shifter outputs

from machine import Pin, PWM, const
from gc import collect
from utime import sleep_ms as t_sleep
from uasyncio import sleep_ms as a_sleep, get_event_loop as a_loop

# settings

_shift_pulse = const(150) # ms

# ouputs

sdp = Pin('sdp', Pin.OUT) # shift up pin
sup = Pin('sup', Pin.OUT) # shift down pin

# inputs

UIsdp = Pin('UIsdp', Pin.IN) # shift up paddle pin
UIsup = Pin('UIsup', Pin.IN) # shift down paddle pin

# util functions
        
def shift(pin, pulse=_shift_pulse):
    pin.high()
    t_sleep(pulse)
    pin.low()
    
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

# async program setup
        
loop=a_loop()
loop.create_task(Bingus()) # _shift_pulse length blocking code
loop.run_forever() 
