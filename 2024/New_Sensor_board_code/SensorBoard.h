#include <Adafruit_NeoPixel.h>

class SensorBoard{
  public:
    void blink(int r, int g, int b);
    void readAnalogsMan(short *analogs);
    void canWriteShort(short data);
};

#define BLINK_INTERVAL 1000
#define LEDPIN 8
#define ANLG_RES 4096
#define ANLG_VRANGE 3.3

unsigned long blinkCurrentMillis = millis();
unsigned long blinkPreviousMillis = 0;
unsigned long canCurrentMillis = millis();
unsigned long canPreviousMillis = 0;
bool LEDState = LOW;


Adafruit_NeoPixel strip = Adafruit_NeoPixel(2, LEDPIN, NEO_GRB + NEO_KHZ800);

//blink method for diagnostic testing
void blink(int r, int g, int b){
  blinkCurrentMillis = millis();
  if(blinkCurrentMillis-blinkPreviousMillis >= BLINK_INTERVAL){
    blinkPreviousMillis = blinkCurrentMillis;
    if(LEDState == LOW){
      LEDState = HIGH;
      strip.setPixelColor(0,r,g,b);
      strip.show();
    }
    else{
      LEDState = LOW;
      strip.clear();
      strip.show();
    } 
  }
}

//read analogs method for non-multiplexer analogs
//takes in 16,17,18,19
void readAnalogsMan(short *analogs){
  analogs[16] = (analogRead(A0) / (float)ANLG_RES) * 1000 * ANLG_VRANGE;
  analogs[17] = (analogRead(A1) / (float)ANLG_RES) * 1000 * ANLG_VRANGE;
  analogs[18] = (analogRead(A2) / (float)ANLG_RES) * 1000 * ANLG_VRANGE;
  analogs[19] = (analogRead(A3) / (float)ANLG_RES) * 1000 * ANLG_VRANGE;
}

//A short is two bytes in arduino. This is a simplified method to send shorts over CAN
void canWriteShort(short data){
  CAN.write(data & 0xFF);
  CAN.write(data >> 8);
}
