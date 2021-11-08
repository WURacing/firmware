The Data Acquisition system is an essential subsystem of WashU Formula Racing. We gather, log, and distribute data around the car, allowing every other system to focus on sound, data-driven designs. To do this, we utilize custom boards for sensing, logging data, sending telemetry, and providing driver feedback via dashboard. This gives us full control over our data flow and allows us to easily adapt to changing demands from every system year-to-year. 

<img src="https://github.com/WURacing/DAQ-Project/blob/main/Docs/racecar.png"
     alt="racecar"
     width="500" />
     
Our current sensor board designs are based around the ATSAMC21 chip, which utilizes onboard ADCs for sensing and an onboard CAN protocol controller to send and receive data from across the car.

<img src="https://github.com/WURacing/DAQ-Project/blob/main/Docs/sensorboard.png"
     alt="sensor board"
     width="500" />

This design is functional and robust but has some major limitations. Limited clock speed on the ATSAMC21 boards and only 6 ADC channels means we have to deploy many sensor boards to achieve full coverage (as we've added more and more sensors to the car, this has become more of an issue - we have well over 20 sensors requiring 5 sensor boards). In addition, multiple years of adding features has led to an increasingly complex code base which has become difficult to effectively iterate on.
 
## The Project

Our goal is to get a clean start, revisiting previous design decisions and ensuring every part of our system aligns with the greater design goals of the car. The project will continue into the spring semester, with designs being incorporated into testing and potentially being brought to competition in May. 

### Leaning on hardware, rather than software
 - Using a faster/multi-core CPU you can do more, or alternatively **do the same amount much easier**
   - This one's a no-brainer, but it may have taken a non-CS person to step back and realize it
   - Even ignoring potential performance gains, a faster CPU can easily accommodate C++/MicroPython rather than C, and much more readable OOP-style programming
 - Using a CAN controller, any CPU with SPI communications can easily send/receive CAN messages
   - don't have to worry about the low-level timing of CAN communication, buffering/filtering messages efficiently, etc
   - can filter CAN messages for you in real time, and only relay specific desired messages to the CPU
 - Using a ADC chip, any CPU with SPI communications can easily read an arbitrary number of analog inputs (the 12-16 channel range seems about right, which would cover ~half the car)
   - number of analog inputs, resolution, and sampling rate depends only on the quality of ADC and the ability of the CPU to read it

### Simplifying the code base
 - As mentioned above, a faster CPU opens up the possibility of utilizing programming paradigms that are easier to read, modify, prototype on, refactor, teach, etc
   - OOP embedded programming is really helpful for abstracting hardware functions while still leaving a clear understanding of what things are doing what operation
   - C++ supports object oriented programming, as well as abstracts some memory/pointer stuff from C
   - MicroPython (uPy) is a "pythonic" embedded language, ideal for rapid-prototyping, learning, and readability
     - uPy is significantly slower than C/C++, and also much less mature. This may not be a great option even on faster hardware if it's less reliable. Experimentation will be necessary.
