# uPy compatible

'''
When a call to SPI.write_readinto(output_buffer, input_buffer) is made to the ads79 chip, the SPI hardware will send all the data in output_buffer while simultaneously storing all the recieved data in input_buffer. Both buffers must be the same length since they are being sent and recieved syncronously. Both the input and output are expected to be 16 bit messages (we call these frames). 
The ads79_buffer module abstracts the bit/byte manipulation needed to create and read these frames. This module attempts to abstract the sequence of frames needed to program and run the ads79 ADCs. 

In order to initialize the ads79 class, an SPI object and CSn (chip select) pin object should be provided. You will also need to specify the number of channels and bit resolution of your particular ads79 ADC. The default values will use the default SPI pins for the RPi Pico's first SPI channel.
The select methods set the sequence of bytes to send while programming and running the ADC. 
'''

from machine import Pin, SPI, SoftSPI
from ads79_buffer import ads79_SDO, ads79_SDI

class ads79():

    def __init__(self,SPI_obj=SPI(0, 10000000, sck=Pin(18), mosi=Pin(19), miso=Pin(16)), CSn=Pin(17, Pin.OUT), channels=16, bit_resolution=12):
        '''Initialize SPI_ADC with 4 wire SPI. SoftSPI or hard SPI are acceptable.'''
        self.SPI_obj = SPI_obj
        self.CSn = CSn
        self.channels = channels
        self.prog_sequence = []
        self.run_sequence = []
        self.input = ads79_SDO(bit_resolution)

    def manual_select(self,cycle=None):
        '''Sets sequences for manual channel selection.

        Programming sequence: one frame with mode=1
        Running sequence: one frame per channel specified by cycle. If cycle isn't provided, go through each channel in range(self.channels)'''
        self.prog_sequence = [ads79_SDI(mode=1,prog=True,chan=0,vref=True,power=False,GPIO=0)]
        self.run_sequence = []
        if cycle == None:
            cycle = range(self.channels)
        for i in cycle:
            self.run_sequence.append(ads79_SDI(mode=1,chan=i))
    
    def auto1_select(self):
        pass

    def auto2_select(self):
        pass

    def send_recv(self,command):
        '''Send the command over SPI, and recieve ADC reading in self.input'''
        self.CSn.low()
        self.SPI_obj.write_readinto(command, self.input)
        self.CSn.high()

    def ADC_val():
        '''Returns a tuple of the current self.input's (channel, data).
       
        Channel is a 4 bit number, while data is 8 to 12 bits specified by bit_resolution.'''
        return self.input.ADC_val()

    def prog(self):
        '''Sends self.prog_sequence over SPI.
        
        This only needs to be run once, unless the mode or settings are changing.'''
        for sdi in self.prog_sequence:
            self.send_recv(sdi)

    def run(self):
        '''Sends self.run_sequence over SPI.
        
        TODO: what should this function output?'''
        frame_data = 0
        for sdi in self.run_sequence:
            self.send_recv(sdi)
            ch,data = self.ADC_val()
            print("Ch: " + ch + ", data: " + data)
