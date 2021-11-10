# Author: Shane Warga
# Purpose: Create a new python base CAN recorder and parser to replace the current 2022
# version of the DAQ board. This is meant to run on a raspberry pi running a the raspian lite linux distro
# Configuration: 
#     NOTE THIS ONLY NEEDS TO BE DONE ONCE ON A NEW PI
#     -Flash and SD card with Raspian Lite Linux
#     -power the pi and plug it into a monitor
#     -configure login information I use username:pi password:pi, i'm lazy
#     -run the command 'sudo raspi-config'
#         -this will enable the gnu configuration tool
#         -go to the interfaces menu and enable SSH, UART, and I2C
#         -for development set up a connection to wifi labeled as wireless lan
#     -install the latest version of python3 and pip3 with the linux package manager
#     -install pyserial, cantools, and pandas with pip if no requirements file
#     -pySerial uses connections defined in the /dev/ folder on linux. The UART serial ports are serial0 and serial1
#     -Cantools needs a reference to a DBC file or multiple files with multiple cantool objects
 
import cantools
import serial
import pprint from pprint
import canRecorder


uart_connection = serial.Serial(
    port='/dev/serial0', 
    baudrate = 9600, 
)
vehicle_db = cantools.database.load_file("VEHICLE.dbc")
pe3_db = cantools.database.load_file("PE3.dbc")
recorder = canRecorder.CanRecorder()

def receiveMessages():
    if(uart_connection.is_open == False):
        print("Connection failed")
        return
    time = 0
    while True:
        frame = uart_connection.readline()
        time = time + 1
        recorder.writeCanFrame(frame, time)

if __name__ == "__main__":
    receiveMessages()