from ads79_SPI import ads79
from rp2 import CAN, CANFrame, CANID
from utime import sleep, sleep_ms

adc = ads79.default_init()

adc.manual_cycle(cycle=[2,3,4])
adc.prog()

c = CAN()
ch=0

while True:
    x = adc.run().to_bytes(8,'little')
    print(bin(x))
    c.send_frame(CANFrame(CANID(2),data=x))
