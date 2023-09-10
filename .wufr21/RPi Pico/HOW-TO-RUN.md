Assuming you've followed the instructions in INSTALL.md, then running the sensor board is very straightforward. Basically, just once CAN and power are connected the board should run automatically, sampling the channels specified in "main.py" and sending them as data packets over CAN.

To alter the sample rates and/or orders and combinations of channels, one can edit "main.py" and upload a new copy to the sensor board. Then after a power cycle it will begin transmitting the new order/rate of sensors.
