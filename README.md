The Data Acquisition system is an essential subsystem of WashU Formula Racing. We gather, log, and distribute data around the car, allowing every other system to focus on sound, data-driven designs. To do this, we utilize custom boards for sensing, logging data, sending telemetry, and providing driver feedback via dashboard. This gives us full control over our data flow and allows us to easily adapt to changing demands from every system year-to-year. 

Our current sensor board designs are based around the ATSAMC21 chip, which utilizes onboard ADCs for sensing and an onboard CAN protocol controller to send and receive data from across the car.

This design is functional and robust but has some major limitations. Limited clock speed on the ATSAMC21 boards and only 6 ADC channels means we have to deploy many sensor boards to achieve full coverage (we have well over 20 sensors across the car). In addition, multiple years of adding features has led to an increasingly complex code base which has become difficult to effectively iterate on.
 
## The Project

Our goal is to get a clean start, revisiting previous design decisions and ensuring every part of our system aligns with the greater design goals of the car. The project will continue into the spring semester, with designs being incorporated into the car and potentially being brought to competition in May. 
