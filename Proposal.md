# Peripheral Board

### System Requirements

- sense values from sensors
  - most are 5V
  - need adequate resolution and sampling rate
- control actuators
  - digital outputs for things like shifter
  - PWM for servo control
- transmit messages over CAN
  - encoding can be offloaded to CAN controller

### Solution 1: Raspberry Pi Pico with MicroPython

RP2040 (the chip for the Pico) is a low-cost, high-performance microcontroller device with flexible digital interfaces. Key features:

- Dual Cortex M0+ processor cores, up to 133 MHz
- 264 kB of embedded SRAM in 6 banks
- 30 multifunction GPIO
- Dedicated hardware for commonly used peripherals
  - 16 PWM channels
  - Programmable IO for extended peripheral support
- 4 channel ADC with internal temperature sensor, 0.5 MSa/s, 12-bit conversion
- USB 1.1 Host/Device

