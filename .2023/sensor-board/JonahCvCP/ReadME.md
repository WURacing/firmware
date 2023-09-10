Hello!
//written by Jonah Sachs

The goal of this project was to analyze the slower runtime differences present in the interpretive circuitpython compared to our usual method of code:
C in Arduino C

This project began by setting up a CircuitPython setup in Thonny with the help of a robotics guide from Chris. I then had to find the neccessary UF2
file for our board and import the necessary libraries directly onto the board (or in thonny file archive, but same thing). From there, I rewrote our
FLSB code in CircuitPython in Thonny. Once this was complete, I included simple code trackers at 2 distinct points in the code
1) SPI Tracker after all 3 SPI Accel values are taken in
2) CAN Tracker after the CAN frames are sent out

3) From there, I exported both the C arrays and the Python lists in a jupyter notebook and did relatively simple data analysis for both the CAN and SPI
4) setups. Included is also average differecnes between runs in an attempt to find an average runtime overall for both Python and C.

5) The results were as follows:

6) The average runtime for the FLSB in CircuitPython was about 26.5 ms
7) The average runtime for the FLSB in C was about 3.5 ms

8) It is clear massive runtime issues exist at the magnitude of about x10.

9) This data suggests that if we use python the max hertz rate may be about 40Hz
