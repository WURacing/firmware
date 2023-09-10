from machine import Pin, PWM
from time import sleep
import uasyncio


red = PWM(Pin(16)) # TODO: Change pin number for real use
red.freq(1000)
blue = PWM(Pin(17)) # TODO: Change pin number for real use
blue.freq(1000)
green = PWM(Pin(18)) # TODO: Change pin number for real use
green.freq(1000)

# while True:
#     for duty in range(65025):
#         red.duty_u16(duty)
#         blue.duty_u16(65025-duty)
#         sleep(0.0001)
#     for duty in range(65025, 0, -1):
#         red.duty_u16(duty)
#         blue.duty_u16(65025-duty)
#         sleep(0.0001)
#     print("running")
async def StarterButtonRGB(LED, rate):
    while True:
        for duty in range(0, 65025, rate):
            LED.duty_u16(duty)
            await uasyncio.sleep(0.0001)
        for duty in range(65025, 0, -1 * rate):
            LED.duty_u16(duty)
            await uasyncio.sleep(0.0001)

loop=uasyncio.get_event_loop()
loop.create_task(StarterButtonRGB(red, 10))
loop.create_task(StarterButtonRGB(blue, 15))
loop.create_task(StarterButtonRGB(green, 25))
loop.run_forever() 