# CPython and uPy compatible
# see page 33, 36, 40 of ads79 datasheet

'''
When a call to SPI.write_readinto(output_buffer, input_buffer) is made to the ads79 chip, the SPI hardware will send all the data in output_buffer while simultaneously storing all the recieved data in input_buffer. Both buffers must be the same length since they are being sent and recieved syncronously. Both the input and output are expected to be 16 bit messages (we call these frames). 
The buffers are encoded with byte objects (bytes() or bytearray()), which can be unintuitive with respect to the actual meaning of the inidivdual bits. This module attempts to abstract the bit/byte manipulation and allow simple functional calls to create and decode messages for the ads79. 

The output message, (ie the Pico's Tx, the ads79's SDI) tells the adc which channel to sample and how, while the input message (ie the Pico's Rx, the ads79's SDO) shows the channel selected and the 12 bit reading. The formatting is fully documented in the datasheet, but a quick summary is shown below. Note that there is a 2 frame delay between sending new commands and recieving the corresponding values:

Output message:
 - bits 1-4: mode select (0: continue, 1: manual, 2: auto1, 3: auto2)
 - bit 5: programming enable (if 0, none of the following bits matter)
 - bits 6-9: in manual mode this is channel select, in auto modes the first bit set to 1 resets the channel sequence
 - bit 10: vref select (0: 2.5V range, 1: 5V range)
 - bit 11: power mode select (0: normal operation, 1: power saving)
 - bit 12-16: GPIO bits, don't really care about these (set all to 0)
Example output messages (dashes added for readability):
 - manual select, cycling through channels 0-3 (5V range, normal power):
 "0001-1-0000-1-0-00000", "0001-1-0001-1-0-00000", "0001-1-0010-1-0-00000", "0001-1-0011-1-0-00000"
 - auto1 select, followed by a continue message (5V range, normal power):
  "0010-1-0000-1-0-00000", "0000-0-0000-0-0-00000"

Input message:
 - bits 1-4: channel indicator, same number as in output message
 - bits 5-16: 12 bit data from channel (units of 2.5/4095 or 5V/4095)
Example input messages (dashes added for readability):
 - reading from channel 0 with data=1633 (with 5V range corresponds to 1.99V)
 "0000-011001100001"
 - reading from channel 1 with data=4095 (with 5V range corresponds to 5V)
 "0001-111111111111"
'''

# output bit shifts
_MODE_BIT = 12
_PROG_BIT = 11
_CHAN_BIT = 7
_VREF_BIT = 6
_POWER_BIT = 5
_GPIO_BIT = 0
# input bit shifts
_CHAN_IN_BIT = 12
_DATA_IN_BIT = 12

class ads79_SDI(bytearray):

    def __init__(self,mode=0,prog=False,chan=0,vref=False,power=False,GPIO=0):
        message = (
            (mode << _MODE_BIT)
            + (prog << _PROG_BIT)
            + (chan << _CHAN_BIT)
            + (vref << _VREF_BIT)
            + (power << _POWER_BIT)
            + (GPIO << _GPIO_BIT))
        super().__init__([message >> 8, message & 0b11111111])

    def __repr__(self):
        return self.__class__.__name__ + '(0b' + self.to_bin() + ')'

    def __str__(self):
        return self.__repr__()

    def to_bin(self):
        '''Returns sequence of bits for underlying bytearray.'''
        return "{:016b}".format(int.from_bytes(self, "big"))

    @classmethod
    def from_bytes(cls, byte_message,byteorder):
        message = int.from_bytes(byte_message, byteorder)
        mode = message >> _MODE_BIT & 0b1111
        prog = message >> _PROG_BIT & 0b1
        chan = message >> _CHAN_BIT & 0b1111
        vref = message >> _VREF_BIT & 0b1
        power = message >> _POWER_BIT & 0b1
        GPIO = message >> _GPIO_BIT & 0b11111
        return cls(mode=mode,prog=prog,chan=chan,vref=vref,power=power,GPIO=GPIO)

class ads79_SDO(bytearray):

    def __init__(self,bit_resolution=12):
        super().__init__(2)
        self.bit_resolution = bit_resolution
        self.data_mask=int('1'*bit_resolution,2)

    def __repr__(self):
        return self.__class__.__name__ + '(0b' + self.to_bin() + ')'

    def __str__(self):
        return self.__repr__()

    def to_bin(self):
        '''Returns sequence of bits for underlying bytearray.'''
        return "{:016b}".format(int.from_bytes(self, "big"))

    def ADC_val(self):
        '''Returns tuple of (channel, data).
       
        Channel is a 4 bit number, while data is 8 to 12 bits specified by self.bit_resolution.'''
        message = int.from_bytes(self, "big")
        chan = message >> _CHAN_IN_BIT & 0b1111
        data = message >> (_DATA_IN_BIT-self.bit_resolution) & self.data_mask
        return [chan,data]
