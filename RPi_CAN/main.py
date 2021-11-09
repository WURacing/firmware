#pico controller to send messages over UART to a raspberry pi

from rp2 import *
import machine

def sendCanFrame():
    uart = UART(0, 9600) 
    c = CAN()
    while(c.recv_pending() > 0):
        frame = c.recv(as_bytes=True)
        frame = frame + b'\n'
        uart.write(frame)
