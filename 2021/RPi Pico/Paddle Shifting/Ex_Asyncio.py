# Author Chris Donnell, with sections copied from Hayden 
# Schroeder's code
# 
# **This is run in Python, not uPython**
# 
# This is a bastardization of the already created programs in 
# this folder (because I wanted to test/demo on just my 
# computer but the uPython machine library doesn't run in 
# standard Python). So I wrote fake classes to just print text 
# when a "pin" is changed. The meat of the example is found 
# where the "async def" lines start - these are meant to 
# replicate/replace the main loops of the other programs I 
# copied, in an asyncronous manner. 
# 
# This example is a bit more basic than the controller code 
# will probably be. Likely we'll encounter issues when we start 
# actually having IO processes occuring and not just randint 
# and print functions. We should also organize the final code a 
# bit better than this file.

import time # replace w/ utime
import random # replace with CAN
import asyncio # replace with uasyncio

class Pin: # dummy class for machine.Pin
    num = None
    def __init__(self,num):
        self.num = num
    def low(self):
        print('Pin %s low' %self.num)
    def high(self):
        print('Pin %s high' %self.num)
        
class PWM: # dummy class for machine.PWM
    pin = None
    def __init__(self,pin):
        self.pin = pin
    def duty_u16(self,duty_u16):
        duty = duty_u16/(2**16)
        print('Pin %s set to %.2f duty PWM' %(self.pin.num,duty))

def shift_up(pin, pulse=0.0150):
    pin.high()
    time.sleep(pulse)
    pin.low()

def shift_down(pin, pulse=0.0150):
    pin.high()
    time.sleep(pulse)
    pin.low()

def set_servo(pwm, deg):
    duty = (2**16/180*deg)+2**15
    pwm.duty_u16(duty)
    
shift_down_pin = Pin('shift_down_pin')
shift_up_pin = Pin('shift_up_pin')
FRONT_ARB = PWM(Pin('FRONT_ARB'))
REAR_ARB = PWM(Pin('REAR_ARB'))
DRS = PWM(Pin('DRS'))
button_red = PWM(Pin('button_red'))
button_green = PWM(Pin('button_green'))
button_blue = PWM(Pin('button_blue'))

async def BingusContoller():
    generator = random.Random()
    shift_down_pin.low()
    shift_up_pin.low()
    while True:
        await asyncio.sleep(0.05) # CAN
        data = random.randint(0, 100) # CAN data
        if data == 0:
            shift_up(shift_up_pin)
        elif data == 1:
            shift_down(shift_down_pin)

async def ServoController(servo):
    generator = random.Random() #ignore
    while True:
        await asyncio.sleep(0.05) # 
        data = random.randint(0, 100) # CAN data
        if data == 0:
            set_servo(servo, generator.randint(-90, 90))

async def StarterButtonRGB(LED, rate):
    while True:
        for duty in range(0, 65025, 1000):
            LED.duty_u16(duty)
            await asyncio.sleep(rate)
        for duty in range(65025, 0, -1000):
            LED.duty_u16(duty)
            await asyncio.sleep(rate)
        
loop=asyncio.get_event_loop()
loop.create_task(BingusContoller())
loop.create_task(ServoController(FRONT_ARB))
loop.create_task(ServoController(REAR_ARB))
loop.create_task(ServoController(DRS))
loop.create_task(StarterButtonRGB(button_red, 5))
loop.create_task(StarterButtonRGB(button_green, 5.1))
loop.create_task(StarterButtonRGB(button_blue, 5.2))
loop.run_forever() 
