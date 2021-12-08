from ads79_SPI import ads79
from rp2 import CAN, CANFrame, CANID
from utime import sleep_ms

adc = ads79()
LED = Pin(25, Pin.OUT)
c = CAN()

LED.low()

adc.manual_select(cycle=[2,3,4]) # walks through channel 2, 3, then 4
adc.prog() # sends programming sequence to ads79, must be done before receiving data

ch=0

# adc.run() doesn't do CAN yet, so refactored here
while True:
    LED.high()
    frame_data = 0
    for sdi in adc.run_sequence:
        adc.send_recv(sdi) # sends channel select frame and recieves previous channel reading in adc.input
        ch,data = adc.ADC_val()
        frame_data = (frame_data << (12)) + data
        #print("Ch: " + str(ch) + ", data: " + str(data))
        sleep_ms(50)
    x = frame_data.to_bytes(8,'little')
    LED.low()
    try:
        c.send_frame(CANFrame(CANID(2),data=x))
        print(x)
    except:
        print("CAN Bus buffer full")
        sleep_ms(1000)
