from machine import Pin, PWM

red = PWM(dest=Pin(1), freq=1000) # TODO: Change pin number for real use

while True:
    for duty in range(65025):
        red.duty_u16(duty)
        sleep(0.0001)
    for duty in range(65025, 0, -1):
        red.duty_u16(duty)
        sleep(0.0001)