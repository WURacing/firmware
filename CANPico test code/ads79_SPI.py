# uPy compatible
import time
from machine import Pin, SPI, SoftSPI
from ads79_buffer import ads79_SDO, ads79_SDI

class ads79():
    # Initialize SPI_ADC with 4 wire SPI, SoftSPI or hard SPI are acceptable
    # Members:
    #   SPI_obj - machine.SPI or machine.SoftSPI object used for data transfer
    #   CSn - machine.Pin object (SPI chip select) used for ADC framing
    #   hard - bool for whether hard or soft SPI is being used
    #   channels - number of channels to iterate through
    #   bit_resolution - size of ADC samples
    def __init__(self,SPI_obj=None,CSn=None,channels=4, bit_resolution=12):
        self.SPI_obj = SPI_obj
        self.CSn = CSn
        self.channels = channels
        self.prog_sequence = []
        self.run_sequence = []
        self.input = ads79_SDO(bit_resolution)
    
    @classmethod
    def default_init(cls):
        return cls(SPI_obj=SPI(0, 10000000, sck=Pin(18), mosi=Pin(19), miso=Pin(16)), CSn=Pin(17, Pin.OUT))

    def manual_cycle(self,cycle=None):
        self.prog_sequence = [ads79_SDI(mode=1,prog=True,chan=0,vref=True,power=False,GPIO=0)]
        self.run_sequence = []
        if cycle == None:
            cycle = range(self.channels)
        for i in cycle:
            self.run_sequence.append(ads79_SDI(mode=1,chan=i))
    
    def auto1(self):
        pass

    def auto2(self):
        pass

    def write_readinto(self,command, results):
        self.CSn.low()
        self.SPI_obj.write_readinto(command, results)
        self.CSn.high()

    def run(self):
        for sdi in self.prog_sequence:
            self.write_readinto(sdi, self.input)
        while True:
            time.sleep(1)
            for sdi in self.run_sequence:
                self.write_readinto(sdi, self.input)
                print(self.input.ADC_val())