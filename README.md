## This repo contains the source code for our CSE 521S Wireless Sensor Networks Final Project

### WUFR Data Acquisition and Telemetry System

The Data Acquisition system is an essential subsystem of WashU Formula Racing. We gather, log, and distribute data around the car, allowing every other system to focus on sound, data-driven designs. To do this, we utilize custom boards for sensing, logging data, sending telemetry, and providing driver feedback via dashboard. This gives us full control over our data flow and allows us to easily adapt to changing demands from every system year-to-year. 

<img src="https://github.com/WURacing/DAQ-Project/blob/main/Docs/racecar.png"
     alt="racecar"
     width="500" />

Here you'll find some essential items:

RPi Pico (sensor board source code and documentation)
- code for interfacing with the ads79## ADCs by SPI
- code for interfacing with the CANPico CAN controllers
- main.py
- README.md, INSTALL.md, HOW-TO-RUN.md

RPi_CAN (data logger and telemetry source code and documentation)
- VEHICLE.dbc, PE3.dbc files for CAN decoding
- code for logging data and sending telemetry
- \_\_main__.py
- README.md, INSTALL.md, HOW-TO-RUN.md

client (website and telemetry dashboard source code and documentation)
- x
- README.md, INSTALL.md, HOW-TO-RUN.md

See the respective readme's for each section for specific information of each section.
