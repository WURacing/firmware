# The Project

Our goal is to get a clean start, revisiting previous design decisions and ensuring every part of our system aligns with the greater design goals of the car. The project will continue into the spring semester, with designs being incorporated into testing and potentially being brought to competition in May. 

## System Requirements

- sense values from sensors
  - most are 5V
  - need adequate resolution and sampling rate
- control actuators
  - digital outputs for things like shifter
  - PWM for servo control
- transmit messages over CAN
  - encoding can be offloaded to CAN controller

## Leaning on hardware, rather than software
 - Using a faster/multi-core CPU you can do more, or alternatively **do the same amount much easier**
   - This one's a no-brainer, but it may have taken a non-CS person to step back and realize it
   - Even ignoring potential performance gains, a faster CPU can easily accommodate C++/MicroPython rather than C, and much more readable OOP-style programming
 - Using a CAN controller, any CPU with SPI communications can easily send/receive CAN messages
   - don't have to worry about the low-level timing of CAN communication, buffering/filtering messages efficiently, etc
   - can filter CAN messages for you in real time, and only relay specific desired messages to the CPU
 - Using a ADC chip, any CPU with SPI communications can easily read an arbitrary number of analog inputs (the 12-16 channel range seems about right, which would cover ~half the car)
   - number of analog inputs, resolution, and sampling rate depends only on the quality of ADC and the ability of the CPU to read it

## Simplifying the code base
 - As mentioned above, a faster CPU opens up the possibility of utilizing programming paradigms that are easier to read, modify, prototype on, refactor, teach, etc
   - OOP embedded programming is really helpful for abstracting hardware functions while still leaving a clear understanding of what things are doing what operation
   - C++ supports object oriented programming, as well as abstracts some memory/pointer stuff from C
   - MicroPython (uPy) is a "pythonic" embedded language, ideal for rapid-prototyping, learning, and readability
     - uPy is significantly slower than C/C++, and also much less mature. This may not be a great option even on faster hardware if it's less reliable. Experimentation will be necessary.

# Proposed solution: Raspberry Pi Pico using MicroPython, with CANPico sock and custom ADC expansion hat

The RPi Pico is a low-cost, high-performance microcontroller device with flexible digital interfaces. Key features:

- Dual Cortex M0+ processor cores, up to 133 MHz
- 264 kB of embedded SRAM in 6 banks
- 30 multifunction GPIO
- Dedicated hardware for commonly used peripherals
  - 16 PWM channels
  - Programmable IO for extended peripheral support
- 4 channel ADC with internal temperature sensor, 0.5 MSa/s, 12-bit conversion
- USB 1.1 Host/Device

The CANPico is 

The custom ADC hat is
