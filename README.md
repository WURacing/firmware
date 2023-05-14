# Code for sensor boards and AiM Configurations

## Description

This repository contains code for the sensor boards used in the WUFR23, as well as the AiM configurations used for the car.

Authors: Hayden Schroeder and Jonah Sachs

## Contents

### Sensor Boards (FLSB, FRSB, RSB)

The code for these boards is identical except for their CAN IDs. They read accelerometer, gyro, and temp data over SPI, as well as analog data over analog pins. They then send this data over CAN to DAQ.

### [`WUFR23.dbc`](WUFR23.dbc): DBC for custom data system

| CAN ID | Data                       |
| ------ | -------------------------- |
| 0x1    | RSB accelerometer (x,y,z)  |
| 0x2    | RSB analog 1, 2, 3, & 4    |
| 0x3    | RSB analog 5 & 6           |
| 0x11   | FRSB accelerometer (x,y,z) |
| 0x12   | FRSB analog 1, 2, 3, & 4   |
| 0x13   | FRSB analog 5 & 6          |

### [`PE3.dbc`](PE3.dbc): DBC for PE3 data system

This follows the standard PE3 DBC, but analog channels are mapped to the following CAN IDs:

| CAN ID     | Data                |
| ---------- | ------------------- |
| 0x0CFFF248 | Analog 1, 2, 3, & 4 |
| 0x0CFFF348 | Analog 5, 6, 7, & 8 |

## AiM Output

| CAN ID | Data                                          |
| ------ | --------------------------------------------- |
| 0x100  | Gear position, engine speed, MAP, wheel speed |

Data from analog inputs is recieved as a value between 0 and 5000, representing 0 and 5 volts respectively. These are linearzed accordingly in the DBC.

### [RacingLogo.png](RacingLogo.png): Logo for AiM Configurations

![Racing Logo](RacingLogo.png)

### Current AiM Configurations

- [Easy One](Easy%20One.zconfig): This configuration shows a few PE3 values on the dash.
- [MXS 1.x3A.zconfig](MXS%201.x3A.zconfig): This is the configuration used for the WUFR23.
