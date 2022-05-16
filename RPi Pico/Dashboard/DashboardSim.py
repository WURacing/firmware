#Written by Hayden Schroeder WUFR2022

import board
import busio
from adafruit_ht16k33 import segments
from struct import unpack
import math
import random
import time

NO_WARNING = 0b00000000
COOLANT_WARNING = 0b00001000
BATTERY_WARNING = 0b00000001
BOTH_WARNINGS = 0b00001001

COOLANT_THRESHOLD = 200 #Celsius
BATTERY_THRESHOLD = 12 #Volts

print("Creating I2C.")
#                     (SCL, SDA)
i2c = busio.I2C(board.GP15, board.GP14)
print("I2C created!")

print("Creating left display.")
left_display = segments.Seg7x4(i2c, auto_write=False, address=0x71)
print("Left display created!")

print("Creating right display.")
right_display = segments.Seg7x4(i2c, auto_write=False)
print("Right display created!")
        
# Print data to dash
def print_to_dash(rpm, gear, temp, voltage):
    left_display.fill(0)
    left_display.print(str(math.floor(rpm / 1000)) + ".")
    left_display[0] = str(gear)
    left_display.show()
    
    text = str(rpm)
    right_display[0] = text[len(text) - 3]
    right_display[1] = text[len(text) - 2]
    if temp > COOLANT_THRESHOLD and voltage < BATTERY_THRESHOLD:
        right_display.set_digit_raw(3, BOTH_WARNINGS)
    elif temp > COOLANT_THRESHOLD:
        right_display.set_digit_raw(3, COOLANT_WARNING)
    elif voltage < BATTERY_THRESHOLD:
        right_display.set_digit_raw(3, BATTERY_WARNING)
    else:
        right_display.set_digit_raw(3, NO_WARNINGS)
    right_display.show()

#Global variables to keep track of data
rpm = 0
gear = 1
temp = 200
voltage = 12

while True:
    rpm_rand = random.randint(-100, 100)
    if rpm_rand > 0:
        rpm_rand = rpm_rand * 2;
    rpm = rpm + rpm_rand
    if rpm > 11000 or rpm < 0:
        rpm = 0
        
    gear_rand = random.randint(-100, 100) / 175
    gear = round(gear + gear_rand)
    gear = max(1, min(6, gear))
    
    temp_rand = random.randint(-100, 100) / 100
    temp = temp + temp_rand
    temp = max(-1000, min(1000, temp))
    
    voltage_rand = random.randint(-100, 100) / 100
    voltage = voltage + voltage_rand
    voltage = max(0, min(22, voltage))
    
    print("RPM: " + str(rpm) + "\tGear: " + str(gear) + "\tTemp: " + str(temp) + "\tVoltage: " + str(voltage))
    print_to_dash(rpm, gear, temp, voltage)
    time.sleep(.1)
