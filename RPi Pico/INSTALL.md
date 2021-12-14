(This guide is assuming your physical architecture is in parity with ours - A CANPico board purchased from CANIS Labs with our custom ADC hat connected on top.)

### Firmware and Software

Our sensor boards are programmed using primarily Micropython. In order to set up a sensor board you'll need some way of programming and uploading Micropython code to a microcontroller (my recommendation is using the [Thonny IDE], but you can view the full [MicroPython for Pico Software Guide] I've written for a full survey of options).

Now, the majority of the CANPico code is included in a modified Micropython firmware, found in the [CANPico Github]. This firmware is already flashed on the Pico if you purchased the CANPico, so nothing more needs to be done (unless you wish to alter the firmware further).

The ADC code is currently just a module, and should be uploaded directly. Using the Thonny IDE (or another program for uploading Micropython code), you should upload both files in ADC folder, "ads79_buffer.py" and "asd79_SPI.py". This module abstracts the ADC communication and makes reading ADC values simple and straighforward.

Finally, you should upload "main.py", found in this folder. This is where the core loop of the sensor board is run and can be adjusted to sample channels at different rates and in different orders and combinations.

### Physical Installation

The physical installation is relatively straighforward. On the car there is expected to be a 5V and ground line and a CAN bus (CAN high and CAN low in a twisted pair of wires). Both of these lines are meant to be daisy chained such that adding another board in the middle or end of the line doesn't affect the rest of the physical architecture. 

Future plans include adding a connector to simplify this process, but for now one should simply use the screw terminals on the end of the CANPico sock to connect these lines.

Finally, all sensors should be connected to the ADC hat. As above, future plans include adding a connector but for now this is simply headers on the ADC hat. The exact combination of connections depends on the sensors, but the ADC hat exposes all 16 ADC inputs as well as ground and 5V power for each sensor.
