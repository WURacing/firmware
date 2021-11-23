# uPy compatible

from machine import Pin, SPI, SoftSPI
from .ads79_bitcode import ads79_message
#from rp2 import ADC

# TODO:
#   __init__ to create own SPI dynamically


class ads79():
    # Initialize SPI_ADC with 4 wire SPI, SoftSPI or hard SPI are acceptable
    # Members:
    #   SPI_obj - machine.SPI or machine.SoftSPI object used for data transfer
    #   CSn - machine.Pin object (SPI chip select) used for ADC framing
    #   hard - bool for whether hard or soft SPI is being used
    #   channels - number of channels to iterate through
    #   bit_resolution - size of ADC samples
    def __init__(self):
        self.hard = None
        self.SPI_obj = None
        self.CSn = None
        self.channels = None
        self.bit_resolution = None

        self.TX_buffer = bytearray(2)
        self.RX_buffer = bytearray(2)
    
    @classmethod
    def default_init(cls):
        adc = SPI_ADC()
        adc.ADC_conf(16,12)
        adc.SPI_conf(SPI(0, 10000000, sck=Pin(18), mosi=Pin(19), miso=Pin(16)), Pin(17, Pin.OUT))
        return adc

    # Configure ADC settings
    def ADC_conf(self, channels, bit_resolution):
        self.channels = channels
        self.bitbit_resolution = bit_resolution

    # Configure SPI settings
    def SPI_conf(self, SPI_obj, CSn):
        if isinstance(SPI_obj, SPI):
            self.hard=True
        elif isinstance(SPI_obj, SoftSPI):
            self.hard=False
        self.SPI_obj = SPI_obj

        if isinstance(CSn, Pin):
            self.CSn = CSn
        else:
            self.CSn = Pin(CSn, Pin.OUT)

    def read(self):
        out_buffer = bytearray(2)
        self.CSn.low()
        self.SPI_obj.readinto(out_buffer)
        chip_select.high()
        return out_buffer

    def out_buffer_set(self, mode, ):
        return

    @staticmethod
    def TEST():
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