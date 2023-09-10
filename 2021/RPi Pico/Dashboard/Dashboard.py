#Written by Hayden Schroeder WUFR2022

import board
import busio
from adafruit_ht16k33 import segments
from rp2 import CAN, CANIDFilter, CANID
from struct import unpack
import math

DEBUG = False

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

rpm_canid = CANID(0x0CFFF048, extended=True)
batt_and_cool_canid = CANID(0x0CFFF548, extended=True)
gear_canid = CANID(0x0CFFF348, extended=True) #Analog Input 7

id_filters = {0: rpm_canid.get_id_filter(), 1: batt_and_cool_canid.get_id_filter(), 2: gear_canid.get_id_filter()}
can = CAN(profile=CAN.BITRATE_250K_75, id_filters=id_filters)

# Wait for a frame
def recv_wait(can):
    can.recv()  # Throw away any left-over pending frames
    while True:
        frames = can.recv()
        if len(frames) > 0:
            return frames
        
# Print data to dash
def print_to_dash(rpm, gear, temp, voltage):
    left_display.fill(0)
    left_display.print(str(math.floor(rpm / 1000)) + ".")
    left_display[0] = str(gear)
    left_display.show()
    
    right_display.fill(0)
    text = str(rpm)
    if(len(text) >= 3):
        right_display[0] = text[len(text) - 3]
    right_display[1] = text[len(text) - 2]
    if temp > COOLANT_THRESHOLD and voltage < BATTERY_THRESHOLD:
        right_display.set_digit_raw(3, BOTH_WARNINGS)
    elif temp > COOLANT_THRESHOLD:
        right_display.set_digit_raw(3, COOLANT_WARNING)
    elif voltage < BATTERY_THRESHOLD:
        right_display.set_digit_raw(3, BATTERY_WARNING)
    else:
        right_display.set_digit_raw(3, NO_WARNING)
    right_display.show()

def calculate_gear(analog):
    if analog > .49 and analog < .88:
        return 1
    elif analog > .98 and analog < 1.37:
        return 2
    elif analog > 1.47 and analog < 1.86:
        return 3
    elif analog > 1.96 and analog < 2.35:
        return 4
    elif analog > 2.45 and analog < 2.84:
        return 5
    else:
        return 0

#Global variables to keep track of data
rpm = 0
gear = 1
temp = 0
voltage = 12.1

while True:
    frames = recv_wait(can)
    #Unpack each frame and print data to dash
    for frame in frames:
        canid = frame.get_canid()
        if canid == rpm_canid:
            data = unpack('>HHHh', frame.get_data())
            rpm = data[0]
        if canid == batt_and_cool_canid:
            data = unpack('>hhhB', frame.get_data())
            voltage = data[0]/100
            temp_type = data[3]
            temp = data[2]/10
            #Convert from F to C if needed
            if temp_type == 1:
                temp = (temp - 32)*(5/9)
        if canid == gear_canid:
            data = unpack('>hhhh', frame.get_data())
            analog = data[2] #Analog Input 7
            gear = calculate_gear(analog)
            
        if(DEBUG):
            print("CANID: " + str(canid) + "\tRPM: " + str(rpm) + "\tGear: " + str(gear) + "\tTemp: " + str(temp) + "\tVoltage: " + str(voltage))
        print_to_dash(rpm, gear, temp, voltage)
