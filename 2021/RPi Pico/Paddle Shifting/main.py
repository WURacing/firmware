# Author Chris Donnell, with sections copied from Hayden 
# Schroeder's code
# 
# **This is run in uPython**

from micropython import const
from machine import Pin, PWM
from gc import collect
from utime import sleep_ms as t_sleep
from uasyncio import sleep_ms as a_sleep, get_event_loop

# outputs

_shift_down_pin = const(14)
_shift_up_pin = const(15)
_FARB_pin = const(12)
_RARB_pin = const(9)
_DRS_pin = const(13)
_button_red_pin = const(0)
_button_green_pin = const(1)
_button_blue_pin = const(7)

_FARB_val = const(30) # deg
_RARB_val = const(0) # deg 
_DRS_low = const(0) # deg
_DRS_high = const(30) # deg
_shift_pulse = const(150) # ms

# inputs

_UI_shift_down_pin = const(17)
_UI_shift_up_pin = const(16)
_UI_DRS_pin = const(18)

# util functions
        
def shift(pin, pulse=_shift_pulse):
    print('shift')
    pin.high()
    t_sleep(pulse)
    pin.low()

def set_servo(pwm, deg):
    print('set_servo')
    duty = int((2**16/180*deg)+2**15)
    pwm.duty_u16(duty)
    
# core functions

async def Bingus():
    sdp = Pin(_shift_down_pin, Pin.OUT, value=0)
    sup = Pin(_shift_up_pin, Pin.OUT, value=0)
    UIsdp = Pin(_UI_shift_down_pin, Pin.IN, Pin.PULL_UP)
    UIsup = Pin(_UI_shift_up_pin, Pin.IN, Pin.PULL_UP)
    d = 0
    u = 0
    while True:
        d = UIsdp.value()
        u = UIsup.value()
        if d ^ u:
            print('Bingus')
            if d:
                shift(sdp)
            if u:
                shift(sup)
            await a_sleep(500 - _shift_pulse)
        else:
            await a_sleep(0)

async def DRS():
    servo = PWM(Pin(_DRS_pin, Pin.OUT))
    servo.freq(333)
    button = Pin(_UI_DRS_pin, Pin.IN)
    status = 0
    while True:
        if button.value() != status:
            if button.value():
                set_servo(servo, _DRS_high)
                status = 1
            else:
                set_servo(servo, _DRS_low)
                status = 0
        await a_sleep(0)

async def RGB(pin_i, rate):
    LED = PWM(Pin(pin_i, Pin.OUT))
    while True:
#         print('RGB')
        for duty in range(0, 65025, 1000):
            LED.duty_u16(duty)
            await a_sleep(rate)
        for duty in range(65025, 0, -1000):
            LED.duty_u16(duty)
            await a_sleep(rate)
            
async def gar():
    LED = Pin(25, Pin.OUT) #onboard LED
    while True:
        print('gar')
        collect()
        LED.toggle()
        await a_sleep(2000)
        LED.toggle()
        await a_sleep(2000)
        LED.toggle()
        await a_sleep(2000)
        LED.toggle()
        await a_sleep(2000)
        LED.toggle()
        await a_sleep(2000)

fb = PWM(Pin(_FARB_pin, Pin.OUT))
fb.freq(333)
rb = PWM(Pin(_RARB_pin, Pin.OUT))
fb.freq(333)
set_servo(fb, _FARB_val)
set_servo(rb, _RARB_val)
            
# async program setup

loop=get_event_loop()
loop.create_task(gar()) # periodic garbage collection
loop.create_task(Bingus()) # 150 ms blocking code
#loop.create_task(DRS())
loop.create_task(RGB(_button_red_pin, 5))
# loop.create_task(RGB(_button_green_pin, 6))
# loop.create_task(RGB(_button_blue_pin, 7))
loop.run_forever()
