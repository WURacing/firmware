import machine
import time

led = machine.Pin(25, machine.Pin.OUT, value=1)
shift_up_pin = machine.Pin(2, machine.Pin.OUT)
shift_down_pin = machine.Pin(1, machine.Pin.OUT)
shift_up_pin.low()
shift_down_pin.low()

#global used to call shift up or down
hot_key = False
default_pulse = 150

def shift_up(pulse=150):
    if hot_key:
       pulse=defualt_pulse
    shift_up_pin.high()
    time.sleep_ms(pulse) #sleep for pulse width in milliseconds
    shift_up_pin.low()

def shift_down(pulse=150):
    if hot_key:
       pulse=defualt_pulse
    shift_down_pin.high()
    time.sleep_ms(pulse)      # sleep for pulse width in milliseconds
    shift_down_pin.low()

def shift(sig, pulse=150):
    #override the hotkey with whatever is passed in by shift
    was_hot_key = false
    if hot_key:
        was_hot_key = True
        hot_key = False
        
    #do not trigger in the case the pulse width is too large or small
    if pulse > 750 or pulse < 50 :
        return
    
    # Sig shift signature, up = shift up, down = shift down
    if sig == "up":
        shift_up(pulse)
    elif sig == "down":
        shift_down(pulse)
    else: 
        print("Sig error: ", sig)
     
    #reset to the original hotkey value if it was never set should be false
    hot_key = was_hot_key

#call init(True, {pulse}) to set a defualt pulse to use for the shift up and shift down functions
def init(hot, pulse):
    hot_key = hot
    defualt_pulse = pulse