# uPy compatible
from machine import Pin, SPI
from rp2 import *
import time

def testSPI():
        spi_con = SPI(0, 10000000, sck=Pin(18), mosi=Pin(19), miso=Pin(16))
        #read manuel mode from channel 7 at 2x Vref
        chip_select = Pin(17, Pin.OUT)
        #b'0010100011000000'
        buffer=bytearray((0x28, 0xc0))
        print(buffer)
        i = 0
        results = bytearray(2)
        while(i < 100 ):
            chip_select.low()
            spi_con.write_readinto(buffer, results)
            chip_select.high()
            print(bin(int.from_bytes(results,'little')))
            i = i+1

def testCAN(user):
    c = CAN()
    i = 0
    while(True):
        c.send_frame(CANFrame(CANID(0x01 + i), data=user))
        time.sleep(0.01)