"""
Example code from SumoBot competition
Written by Chris Donnell prior to project start, not for use on DAQ system
"""

from machine import Pin, PWM, time_pulse_us
from time import sleep

class sensor:
    
    timeout = 100000
    weight = 1
    
    def __init__(self, ctrlPin,sensePin,bias):
        self.ctrlPin = Pin(ctrlPin, Pin.OUT)
        self.sensePin = Pin(sensePin, Pin.IN)
        self.bias = bias
        
    def __str__(self): 
        return str(self.sense())
    
    def calibrate(self):
        senseVal = self.sense()
        if senseVal == -2:
            return
        if senseVal > self.senseMax:
            self.senseMax = senseVal
        if senseVal < self.senseMin:
            self.senseMin = senseVal
    
    def guide(self):
        """converts sensor values to bias (-1 to 1) and intensity (-1 to 1)"""
        senseVal = self.sense()
        if senseVal == -2:
            return 0, 0
        return self.bias, min(1,max(-1,1-((senseVal-self.senseMin)/(self.senseMax-self.senseMin))))
    
class sonarSensor(sensor):
    
    senseMin = 0
    senseMax = 50
    weight = 10
    
    def sense(self):
        """sonar sensor algorithm"""
        self.ctrlPin(1)  # Set trig high
        sleep(0.00001)  # 10 micro seconds
        self.ctrlPin(0)  # Set trig low
        pulselen =  time_pulse_us(self.sensePin, 1, self.timeout)
        if pulselen == -2:
            return -2
        return pulselen * 0.017
    
class reflectSensor(sensor):
    
    senseMin = 0
    senseMax = 100
    
    def sense(self):
        """reflectance sensor algorithm"""
        self.ctrlPin(1)  # Set emitter high
        self.sensePin.init(mode=Pin.OUT)
        self.sensePin(1)
        sleep(0.00001)  # 10 micro seconds
        self.sensePin.init(mode=Pin.IN)
        pulselen =  time_pulse_us(self.sensePin, 1, self.timeout)
        return pulselen
    
class motor:
    
    dutyMin = 0
    dutyMax = 65025
    motor_freq = 50
    
    def __init__(self,fwdPin,revPin):
        self.fwdPin = PWM(Pin(fwdPin))
        self.revPin = PWM(Pin(revPin))
        self.fwdPin.freq(self.motor_freq)
        self.fwdPin.duty_u16(0)
        self.revPin.freq(self.motor_freq)
        self.revPin.duty_u16(0)
        
    def setSpeed(self,speed):
        """converts speed between 0 and 1 to duty cycle between 0 and 65025"""
        if speed > 0:
            self.fwdPin.duty_u16(int(speed*self.dutyMax))
            self.revPin.duty_u16(0)
        else:
            self.revPin.duty_u16(int(-speed*self.dutyMax))
            self.fwdPin.duty_u16(0)

class movement:
    
    def __init__(self, leftMotor, rightMotor, sleep):
        self.leftMotor = leftMotor
        self.rightMotor = rightMotor
        self.sleep = sleep
        self.sleep(1)
        
    def steering_func(self,steering):
        if steering < -1:
            left = 1
            right = -1
        elif steering < -0.5:
            left = 1
            right = (0.5 + steering)
        elif steering > 0.5:
            left = (0.5 - steering)
            right = 1
        elif steering > 1:
            left = -1
            right = 1
        else:
            left = (0.5 - steering)
            right = (0.5 + steering)
        return left, right
    
    def move(self, steering, throttle):
        """ convert steering (-1 to 1) and throttle values to individual motor speeds"""
        left, right = self.steering_func(steering)
        self.leftMotor.setSpeed(left*throttle)
        self.rightMotor.setSpeed(right*throttle)
        
class sensing:
    sensors = []
    
    def __str__(self): 
        return str([sens.sense() for sens in self.sensors])
        
    def addSensor(self,sensor):
        self.sensors.append(sensor)
        
    def calibrate(self):
        for x in range(10):
            for sensor in self.sensors:
                sensor.calibrate()
        for sensor in self.sensors:
            if type(sensor) == reflectSensor:
                sensor.weight = 100/(sensor.senseMax + sensor.senseMin)                
    
    def decision(self):
        count = 0
        steering = 0
        throttle = 0
        for sensor in self.sensors:
            count += sensor.weight
            bias, intensity = sensor.guide()
            steering += bias*intensity
            throttle += intensity*sensor.weight
        if count == 0:
            return 0, 0
        return steering/count, throttle/count
    
class robot:
    
    def __init__(self, movement, sensing):
        self.movement = movement
        self.sensing = sensing
        
    def run(self):
        while True:
            steering, throttle = self.sensing.decision()
            self.movement.move(steering, throttle)        
                
    @staticmethod
    def robot_initialization():
        print('intializing robot')
        s = sensing()
        sonar = sonarSensor(11,12,0)
        left_reflect = reflectSensor(7,8,-1)
        left_reflect.senseMax = 60
        right_reflect = reflectSensor(10,13,1)
        right_reflect.senseMax = 20
        s.addSensor(sonar)
        s.addSensor(left_reflect)
        s.addSensor(right_reflect)
        #s.calibrate()
        left_motor = motor(4,3) 
        right_motor = motor(0,1)
        sleep = Pin(2,Pin.OUT)
        m = movement(left_motor, right_motor, sleep)
        r = robot(m, s)
        print('robot initialized')
        return r

r = robot.robot_initialization()
#r.run()
while True:
     print(r.sensing)
     sleep(0.2)
#r.movement.move(0,1)
#r.movement.move(0,0)