# Firmware architecture

Each type of circuit board / controller on the car is associated with one project.
e.g. the Sensor folder for the devices on the car designed to read from sensors and
transmit measurements periodically on the CAN bus.
There is a Shared folder for code that is reused between different controllers.
One example is the CAN driver which is used on every controller.

## Drivers

Microcontroller peripheral drivers:
- drv_adc
- drv_can
- drv_divas
- drv_eic
- drv_gpio
- drv_i2c
- drv_spi
- drv_uart

Utility drivers:
- drv_serial

Higher-level functional drivers:
- drv_sd
- drv_lte

### drv_adc
Uses: Sensor (analog sensors)

Interface to hardware analog-to-digital converter. Used to sample analog sensors.
### drv_can
Uses: everything

Interface to hardware CAN bus controller. Used for communication with other computers on the car. Our data acquisition system takes advantage of this to publish all data on the bus and collect it at one point.
### drv_divas
Interface to hardware division and square root accelerator. Just compensates for lack of appropriate CPU instructions or something. No config, just drop it in your project to make math faster (only if you need to do relevant math).
### drv_eic
Uses: Sensor (hall effect sensors)

Interface to hardware external interrupt controller. Lets us sense when a digital signal goes high or low. Has been used to measure wheel speed as hall effect sensors give a digital waveform as output.
### drv_gpio
Should be deleted probably, just use PORT directly its simple.
### drv_i2c
Dependency: drv_serial. Uses: Datalogger (ds1307), misc (mcp23017)

Implements i2c communication on top of hardware serial communication peripheral.
This driver is designed for "register-oriented" i2c communication. Why? The devices we use, like ds1307 and mcp23017 follow this approach. The idea is that the other device has a register file, and all communication happens as reads/writes to this. Communication structure goes by first sending out the device i2c address with the LSB indicating write, device acknowledges, then host sends a 1-byte register address, and then it either sends N bytes to write or sends a repeated start with LSB indicating read followed by requesting N bytes one at a time.
### drv_serial
Utility functions shared by i2c, spi, and uart drivers. The C source provides common interrupt handlers. The C header provides a general routine to set up the pins.
### drv_spi
Dependency: drv_serial. Uses: Datalogger (SD card)

Implements SPI communication on top of hardware serial communication peripheral.
Pretty simple, just has a busy/wait transfer function that reads and writes simultaneously.
### drv_uart
Dependency: drv_serial, FreeRTOS. Uses: Telemetry (ublox SARA-R4)

Implements UART communication on top of hardware serial communication peripheral. A bit more complicated than the others as it is interrupt driven: tx/rx data is buffered and sent/received in the ISR. read_line() and write() will block the calling RTOS Task but other RTOS tasks can continue running while waiting on UART I/O.

Why did I make this one so complicated? Since the SARA-R4 influences with the LTE network and the Internet, some of its procedures have very long timing guarantees (120 s in some cases) with no obvious mechanism to cancel or check status. We cannot let these potential stalls affect the rest of the system. 
