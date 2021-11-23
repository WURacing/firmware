# CPython and uPy compatible
#
# see page 33, 36, 40 of ads79 datasheet

_MODE_BIT = 12
_PROG_BIT = 11
_CHAN_BIT = 7
_VREF_BIT = 6
_POWER_BIT = 5
_GPIO_BIT = 0

class ads79_message():
    def __init__(self,mode=1,prog=True,chan=0,vref=True,power=False,GPIO=0):
        self._mode=mode
        self._prog=prog
        self._chan=chan
        self._vref=vref
        self._power=power
        self._GPIO=GPIO
        self.message = 0
        self.bit_message = ''
        self.update()

    def __repr__(self):
        return bin(self.message)

    def __str__(self):
        return self.bit_message

    def __int__(self):
        return self.message

    def __eq__(self, other):
        if isinstance(other, self.__class__):
            return self.__dict__ == other.__dict__
        else:
            return self.__int__ == other

    def to_bytes(self):
        return self.message.to_bytes(2, 'little')

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
        self.message = (
            (self._mode << _MODE_BIT)
            + (self._prog << _PROG_BIT)
            + (self._chan << _CHAN_BIT)
            + (self._vref << _VREF_BIT)
            + (self._power << _POWER_BIT)
            + (self._GPIO << _GPIO_BIT))
        self.bit_message = format(self.message,"016b")

    def cont(self):
        self.message = 0
        self.bit_message = format(self.message,"016b")

    def manual(self):
        self._mode = 1
        self.update()

    def auto1(self):
        self._mode = 2
        self.update()

    def auto2(self):
        self._mode = 3
        self.update()

    def prog_en(self,bool):
        self._prog = bool
        self.update()

    def manual_chan(self,chan):
        self._chan = chan
        self.update()

    def auto_reset(self,bool):
        self._chan = bool << 3
        self.update()

    def power_saving(self,bool):
        self._power = bool
        self.update()

    def GPIO_set(self,GPIO):
        self._GPIO = GPIO
        self.update()


adc = ads79_message()
print(adc)

adc.auto1()
print('>>> adc.auto1()')
print(adc)

adc.prog_en(False)
print('>>> adc.prog_en(False)')
print(adc)
