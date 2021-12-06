# uPy compatible
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
    def __init__(self,SPI_obj=None,CSn=None,channels=16, bit_resolution=12):
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

    def send_recv(self,command):
        self.CSn.low()
        self.SPI_obj.write_readinto(command, self.input)
        self.CSn.high()

    def ADC_val():
        return self.input.ADC_val()

    def prog(self):
        for sdi in self.prog_sequence:
            self.send_recv(sdi)

    def run(self):
        frame_data = 0
        for sdi in self.run_sequence:
            self.send_recv(sdi)
            ch,data = self.ADC_val()
            frame_data = (frame_data << (self.bit_resolution)) + data
        return frame_data
