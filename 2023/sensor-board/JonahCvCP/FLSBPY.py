#FLSB IN CIRCUITPYTHON
import board
import digitalio
from digitalio import DigitalInOut,Direction,Pull
import busio
import time
from analogio import AnalogIn
from adafruit_bus_device.spi_device import SPIDevice
import canio
import array
import struct


#efficiency tracker setup
SPITrackerTimes = []
OGSPITrackerCurrentTime = (float)(time.monotonic_ns() / 1000000)
SPITrackerTimes.append(OGSPITrackerCurrentTime)

CANTrackerTimes = []
OGCANTrackerCurrentTime = (float)(time.monotonic_ns() / 1000000)
CANTrackerTimes.append(OGCANTrackerCurrentTime)

def SPItracker():
    SPITrackerCurrentTime = (float)(time.monotonic_ns() / 1000000) #returns milliseconds for time.monotonic
    SPITrackerTimes.append(SPITrackerCurrentTime)
    
def CANtracker():
    CANTrackerCurrentTime = (float)(time.monotonic_ns() / 1000000)
    CANTrackerTimes.append(CANTrackerCurrentTime)


#variable setup
BlinkInterval = 1
datacount = 0
ROLLING_AVG = 64
x_avgs= bytearray(ROLLING_AVG)
y_avgs=bytearray(ROLLING_AVG)
z_avgs=bytearray(ROLLING_AVG)
ANLG_LEN = 6

#blink setup
led = digitalio.DigitalInOut(board.LED)
led.direction = digitalio.Direction.OUTPUT
BlinkCurrentTime = time.monotonic()
BlinkPreviousTime = 0
CanCurrentTime= time.monotonic_ns()
CanPreviousTime = 0

#blink method
def blink():
    global BlinkPreviousTime
    BlinkCurrentTime = time.monotonic()
    if(BlinkCurrentTime - BlinkPreviousTime >= BlinkInterval):
        BlinkPreviousTime = BlinkCurrentTime
        led.value = (not(led.value))

#analog read method
#analog_in = AnalogIn(board.A1)

A0 = AnalogIn(board.A0)
A1 = AnalogIn(board.A1)
A2 = AnalogIn(board.A2)
A3 = AnalogIn(board.A3)
A4 = AnalogIn(board.A4)
A5 = AnalogIn(board.A5)

def readAnalogs(analogs):
    analogs[0] = (int)((A2.value / 4096) * 1000 * 3.3)
    analogs[1] = (int)((A1.value / 4096) * 1000 * 3.3)
    analogs[2] = (int)((A0.value / 4096) * 1000 * 3.3)
    analogs[3] = (int)((A5.value / 4096) * 1000 * 3.3)
    analogs[4] = (int)((A4.value / 4096) * 1000 * 3.3)
    analogs[5] = (int)((A3.value / 4096) * 1000 * 3.3)

#SPI Setup
cs = digitalio.DigitalInOut(board.D5)
spidevice = SPIDevice(board.SPI(),cs,baudrate=10000000,polarity=1,phase=1)
cs.direction = Direction.OUTPUT
cs.value = True

#SPI Settings Setup
with spidevice as spi_dev:
    cs.value = False
    spi_dev.write(bytes([0x20,0x3F]))
    cs.value = True
    
with spidevice as spi_dev:
    cs.value = False
    spi_dev.write(bytes([0x23,0x10]))
    
#CAN Setup
if hasattr(board, 'CAN_STANDBY'):
    standby = digitalio.DigitalInOut(board.CAN_STANDBY)
    standby.direction = digitalio.Direction.OUTPUT
    standby.value = False
    
if hasattr(board, 'BOOST_ENABLE'):
    boost_enable = digitalio.DigitalInOut(board.BOOST_ENABLE)
    boost_enable.direction = digitalio.Direction.OUTPUT
    boost_enable.value=True
    
can = canio.CAN(rx=board.CAN_RX, tx=board.CAN_TX, baudrate=1000000, auto_restart=True)
if not can:
    print("Starting CAN failed!")   
#da looopppp    
while True:
    blink()
    
    #Receive SPI data
    with spidevice as spi_dev:
        cs.value = False
        spi_dev.write(bytes([0xA9]))
        buffer = bytearray(1)
        buffer[0] = 0x00
        spi_dev.write(buffer)
        x_acc = buffer[0]
        cs.value = True
            
    with spidevice as spi_dev:
        cs.value = False
        spi_dev.write(bytes([0xAB]))
        buffer[0] = 0x00
        spi_dev.write(buffer)
        y_acc = buffer[0]
        cs.value = True
        
    with spidevice as spi_dev:
        cs.value = False
        spi_dev.write(bytes([0xAD]))
        buffer[0] = 0x00
        spi_dev.write(buffer)
        z_acc = buffer[0]
        cs.value = True
        
    SPItracker()
    x_avgs[datacount%ROLLING_AVG] = x_acc
    y_avgs[datacount%ROLLING_AVG] = y_acc
    z_avgs[datacount%ROLLING_AVG] = z_acc       
    x_tot = 0
    y_tot = 0
    z_tot = 0
    
    for i in x_avgs:
       x_tot += x_avgs[i]
       y_tot += y_avgs[i]
       z_tot += z_avgs[i]
    
    x_send = (int)(x_tot / ROLLING_AVG)
    y_send = (int)(y_tot / ROLLING_AVG)
    z_send = (int)(z_tot / ROLLING_AVG)
    
    datacount+=1
    
    analogs = array.array('h', [0] * 6)
    readAnalogs(analogs)
    datasend = [x_send, y_send, z_send]
    datasend_bytes = bytearray()
    for value in datasend:
        datasend_bytes.extend(value.to_bytes(2, 'little'))
   
    analogone = [analogs[0],analogs[1],analogs[2],analogs[3]]
    analogtwo = [analogs[4],analogs[5]]
    analogone_bytes = bytearray()
    analogtwo_bytes = bytearray()
    
    for value in analogone:
        analogone_bytes.extend(value.to_bytes(2, 'little'))
   
    for value in analogtwo:
        analogtwo_bytes.extend(value.to_bytes(2, 'little')) 
    
    CanCurrentTime=time.monotonic_ns()
    if(CanCurrentTime-CanPreviousTime>1000000):
        CanPreviousTime=time.monotonic_ns()
        acc_message = canio.Message(id=0x21,data=datasend_bytes)
        can.send(acc_message)
        
        anlgone_message = canio.Message(id=0x22,data=analogone_bytes)
        can.send(anlgone_message)
        anlgtwo_message = canio.Message(id=0x23,data=analogtwo_bytes)
        can.send(anlgtwo_message)
        CANtracker()
    
    #print(datacount)
    
    
    
    
    if(datacount==999):
        for i in range(999):
            SPITrackerTimes[i] -= OGSPITrackerCurrentTime
        for i in range(999):
            CANTrackerTimes[i] -= OGCANTrackerCurrentTime
           
        print("SPI Tracker:")
        print(len(SPITrackerTimes))
        print(SPITrackerTimes)
        print("CAN Tracker:")
        print(len(CANTrackerTimes))
        print(CANTrackerTimes)
        while True:
            time.sleep(5)
    
    
    
    
    
        
        
