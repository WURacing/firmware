The Data Acquisition system is an essential subsystem of WashU Formula Racing. We gather, log, and distribute data around the car, allowing every other system to focus on sound, data-driven designs. To do this, we utilize custom boards for sensing, logging data, sending telemetry, and providing driver feedback via dashboard. This gives us full control over our data flow and allows us to easily adapt to changing demands from every system year-to-year. 

<img src="https://github.com/WURacing/DAQ-Project/blob/main/Docs/racecar.png"
     alt="racecar"
     width="500" />
     
Our current sensor board designs are based around the ATSAMC21 chip, which utilizes onboard ADCs for sensing and an onboard CAN protocol controller to send and receive data from across the car.

<img src="https://github.com/WURacing/DAQ-Project/blob/main/Docs/sensorboard.png"
     alt="sensor board"
     width="500" />

This design is functional and robust but has some major limitations. Limited clock speed on the ATSAMC21 boards and only 6 ADC channels means we have to deploy many sensor boards to achieve full coverage (as we've added more and more sensors to the car, this has become more of an issue - we have well over 20 sensors requiring 5 sensor boards). In addition, multiple years of adding features has led to an increasingly complex code base which has become difficult to effectively iterate on.

In addition, a telemetry system hasn't been implemented in multiple years. In this respect we are essentially starting from scratch.