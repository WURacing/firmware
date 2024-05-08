/**
 * WURACING 2024 Sensor Board Iteration 1 Header File
 * Name: SensorBoard.h
 * Purpose: Organzing Functions and variables for the Sensor Board Code
 * 
 * @author Jonah Sachs
 * @version 1.8.19 9/30/23
 */



#include <Adafruit_NeoPixel.h>
#include <CAN.h>

class SensorBoard{
  public:
    void blink(int r, int g, int b);
    void readAnalogsMan(short *analogs);
    void canWriteShort(short data);
};



/**
 * Blink function for data diagnostic testing
 * 
 * @param r,g,b for color of inbuilt LED
 * @return void
 */
void blink(int r, int g, int b);

/**
 * Reading Manual Analog Values that aren't included in the Multiplexor
 * 
 * @param analogs array storing all 20 analog values
 * @return void
 */
void readAnalogsMan(short *analogs);

/**
 * CAN Write Short method: allowing SHORT transfer in the CAN network
 * 
 * @param the short value to be transferred
 * @return void
 */
void canWriteShort(short data);

/**
 * CAN write frame method which will write 4 sequential frames into a CAN frame
 * 
 * @param index i, array to output from, hex value for CAN address
 * @return void
 */
void canShortFrame(short *send, int  i, int Hex);