# Author Chris Donnell, with sections copied from Hayden 
# Schroeder's code
# 
# **This is run in uPython**
#
# Controller alt V4 but no CAN.

from machine import Pin, PWM, const
from gc import collect
from utime import sleep_ms as t_sleep
from uasyncio import sleep_ms as a_sleep, get_event_loop as a_loop

# outputs

_shift_down_pin = const('shift_down_pin')
_shift_up_pin = const('shift_up_pin')
_FARB_pin = const('FARB_pin')
_RARB_pin = const('RARB_pin')
_DRS_pin = const('DRS_pin')
_button_red_pin = const('button_red_pin')
_button_green_pin = const('button_green_pin')
_button_blue_pin = const('button_blue_pin')

_FARB_val = const(0) # deg
_RARB_val = const(0) # deg
_DRS_low = const(0) # deg
_DRS_high = const(30) # deg
_shift_pulse = const(150) # ms

# inputs

_UI_shift_down_pin = const('UI_shift_down_pin')
_UI_shift_up_pin = const('UI_shift_up_pin')
_UI_DRS_pin = const('UI_DRS_pin')

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

async def DRS():
    servo = PWM(Pin(_DRS_pin, Pin.OUT))
    button = Pin(_UI_DRS_pin, Pin.IN)
    while True:
        if button.value():
            set_servo(servo, _DRS_high)
        else:
            set_servo(servo, _DRS_low)
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
            
async def gar():
    while True:
        collect()
        await a_sleep(2000)
        
set_servo(fp, _FARB_val)
set_servo(rp, _RARB_val)
            
# async program setup
        
loop=a_loop()
loop.create_task(gar()) # periodic garbage collection
loop.create_task(Bingus()) # 150 ms blocking code
loop.create_task(DRS())
loop.create_task(RGB(_button_red_pin, 5))
loop.create_task(RGB(_button_green_pin, 6))
loop.create_task(RGB(_button_blue_pin, 7))
loop.run_forever() 
