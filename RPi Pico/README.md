## Raspberry Pi Pico using MicroPython, with CANPico sock and custom ADC expansion hat

The RPi Pico is a low-cost, high-performance microcontroller device with flexible digital interfaces. Key features:

- RP2040 CPU: Dual Cortex M0+ processor cores, up to 133 MHz
- 264 kB of embedded SRAM in 6 banks
- 30 multifunction GPIO
- Dedicated hardware for commonly used peripherals
  - 16 PWM channels
  - Programmable IO for extended peripheral support
- 4 channel ADC with internal temperature sensor, 0.5 MSa/s, 12-bit conversion
- USB 1.1 Host/Device

The CANPico is a sock for the Raspberry Pi Pico which supplies a CAN controller (MCP2518FD) and CAN transceiver (MCP2562FD), as well as a Micropython SDK that allows for communication with said CAN controller. 
 - CAN-FD and CAN standard compatible
 - triggers can be set for certain CAN IDs or message expressions
 - SPI serial communication (very fast, synchronous communication))

The custom ADC hat is a board which contains an SPI controlled stand alone ADC (ADS7951). A custom Micropython module was written to control the ADC easily and effectively.
 - 12-Bit, 16-Channel input (supports our need for many sensors)
 - 1-MSPS sampling 
 - SPI serial communication (very fast, synchronous communication)
