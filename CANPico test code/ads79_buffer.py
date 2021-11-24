# CPython and uPy compatible
# see page 33, 36, 40 of ads79 datasheet

_MODE_BIT = 12
_PROG_BIT = 11
_CHAN_BIT = 7
_VREF_BIT = 6
_POWER_BIT = 5
_GPIO_BIT = 0

class ads79_SDI(bytearray):
    def __init__(self,mode=1,prog=True,chan=0,vref=True,power=False,GPIO=0):
        super().__init__(2)
        self._mode=mode
        self._prog=prog
        self._chan=chan
        self._vref=vref
        self._power=power
        self._GPIO=GPIO

    @classmethod
    def from_bytes(cls, byte_message):
        message = int.from_bytes(byte_message, "little")
        if message == 0:
            return None
        mode = message >> _MODE_BIT & 0b1111
        prog = message >> _PROG_BIT & 0b1
        chan = message >> _CHAN_BIT & 0b1111
        vref = message >> _VREF_BIT & 0b1
        power = message >> _POWER_BIT & 0b1
        GPIO = message >> _GPIO_BIT & 0b11111
        return cls(mode=mode,prog=prog,chan=chan,vref=vref,power=power,GPIO=GPIO)
    
    # sets the message to 
    def update(self):
        message = (
            (self._mode << _MODE_BIT)
            + (self._prog << _PROG_BIT)
            + (self._chan << _CHAN_BIT)
            + (self._vref << _VREF_BIT)
            + (self._power << _POWER_BIT)
            + (self._GPIO << _GPIO_BIT))
        self.bit_message = format(self.message,"016b")

    def cont(self):
        pass

    def manual(self):
        pass

    def auto1(self):
        pass

    def auto2(self):
        pass

    def prog_en(self,bool):
        pass

    def manual_chan(self,chan):
        pass

    def auto_reset(self,bool):
        pass

    def power_saving(self,bool):
        pass

    def GPIO_set(self,GPIO):
        pass


adc = ads79_message()
print(adc)

adc.auto1()
print('>>> adc.auto1()')
print(adc)

adc.prog_en(False)
print('>>> adc.prog_en(False)')
print(adc)
