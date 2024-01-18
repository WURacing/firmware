#include <Adafruit_NeoPixel.h>
#include <CAN.h>
#include <SPI.h>
#include <Mux.h>
#include <DFRobot_BMX160.h>

#include "SB_CODE.h"
//#include "GoblinMode.h"

#define LEDPIN 8
#define BLINK_INTERVAL 1000
#define BAUD_RATE 1000000
#define ANLG_RES 4096
#define ANLG_VRANGE 3.3
#define CAN_INTERVAL 1

#define BIT_RESOLUTION 12
#define BRIGHTNESS 20
#define DIMENSIONS 3

unsigned long datacount = 0;

short analogs[20];
short accel[DIMENSIONS];
short gyro[DIMENSIONS];
short magn[DIMENSIONS];
double average_matrix[29];

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


//accel/gyro sensor definition
DFRobot_BMX160 bmx160;

//to make using the mux space easier
//using namespace admux;

using namespace admux;


void setup() {
  //Pin Definitions
  pinMode(PIN_NEOPIXEL_POWER, OUTPUT);
  pinMode(A0,INPUT);
  pinMode(A1,INPUT);
  pinMode(A2,INPUT);
  pinMode(A3,INPUT);
  pinMode(A4,INPUT);
  analogReadResolution(BIT_RESOLUTION);//for our board --> bit resolution

  Serial.begin(9600);
  
  
  
  //LED SETUP
  strip.begin();
  strip.clear();
  strip.setBrightness(BRIGHTNESS);
  strip.show();

  //MUX SETUP
  //A4 is voltage output
  //4,5,6,7 are the output identifiers (4 binary codes)
  Mux mux(Pin(A4,INPUT,PinType::Analog),Pinset(4,5,6,7));
  

  //CAN SETUP
  pinMode(PIN_CAN_STANDBY, OUTPUT);
  digitalWrite(PIN_CAN_STANDBY,false);
  pinMode(PIN_CAN_BOOSTEN,OUTPUT);
  digitalWrite(PIN_CAN_BOOSTEN,true);

  //CAN error message
  if (!CAN.begin(BAUD_RATE)) {
    Serial.println("Starting CAN failed!");
  }

  //Accel error message
  if(bmx160.begin() != true){
    Serial.println("init false");
    while(1);
  }

}

void loop() {
  //inputs are R,G,B
  blink(200,0,100,strip);

  //MUX Update
  mux_update(analogs);

  //Manual Data (The hard wired Analog Inputs)
  readAnalogsMan(analogs);

  //New Sensor Event
  //Taking in Accel, Gyro, Magnetometer results
  sBmx160SensorData_t Omagn, Ogyro,Oaccel;
  //these are all objects with x,y,z attributes
  //we source this from the attached library

  bmx160.getAllData(&Omagn, &Ogyro, &Oaccel);


  //updating accel, gyro, magn arrays
  accel_update(accel, Oaccel);
  accel_update(gyro, Ogyro);
  accel_update(magn, Omagn);

  //9 Sensor Values taken in here (3 seperate arrays)

  //each column of average_matrix will accumulate the average value over 10 entries

  for(int i=0; i<20; i++){
    average_matrix[i] += analogs[i];
  }

  for(int i=0; i<3; i++){
    average_matrix[i+20] += accel[i];
    average_matrix[i+23] += gyro[i];
    average_matrix[i+26] += magn[i];
  }

  short avg_send[29];
  if(datacount%10){
    //divide by averaging sampling size
    //convert to short for CAN frame
    for(int i=0; i<29;i++){
      average_matrix[i] = average_matrix[i]/10.0;
      avg_send[i] = (short)average_matrix[i];
    }

    //clear average_matrix for next summation. All other arrays don't have to be cleared since they are set using '=' declarations and not '+='

    //Send CAN Frame
    canShortFrame(avg_send,0,0x20);
    canShortFrame(avg_send,4,0x21);
    canShortFrame(avg_send,8,0x22);
    canShortFrame(avg_send,12,0x23);
    canShortFrame(avg_send,16,0x24);
    canShortFrame(avg_send,20,0x25);
    canShortFrame(avg_send,24,0x26);

    CAN.beginPacket(0x27);
    canWriteShort(avg_send[28]);
    CAN.endPacket();

    //clear the avg_send and average_matrix arrays 
    for(int i=0; i<29; i++){
      avg_send[i]=0;
      average_matrix[i]=0;
    }
    
  } 

  ++datacount;
}

void mux_update(short *analogs){
  Mux mux(Pin(A4,INPUT,PinType::Analog),Pinset(4,5,6,7));
  int data;
  for(byte i=0; i<mux.channelCount(); i++){
    mux.channel(i);
    data = mux.read();
    analogs[i]=(data / (float)ANLG_RES) * 1000 * ANLG_VRANGE;
  }
}

void accel_update(short *accel, sBmx160SensorData_t Oaccel){
  accel[0] = Oaccel.x;
  accel[1] = Oaccel.y;
  accel[2] = Oaccel.z;
}




void blink(int r, int g, int b, Adafruit_NeoPixel &strip){
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


void readAnalogsMan(short *analogs){
  analogs[16] = (analogRead(A0) / (float)ANLG_RES) * 1000 * ANLG_VRANGE;
  analogs[17] = (analogRead(A1) / (float)ANLG_RES) * 1000 * ANLG_VRANGE;
  analogs[18] = (analogRead(A2) / (float)ANLG_RES) * 1000 * ANLG_VRANGE;
  analogs[19] = (analogRead(A3) / (float)ANLG_RES) * 1000 * ANLG_VRANGE;
}

void canWriteShort(short data){
  CAN.write(data & 0xFF);
  CAN.write(data >> 8);
}


void canShortFrame(short *send, int  i, int Hex){
  CAN.beginPacket(Hex);
  for(int j=i; j<i+4; i++){
    canWriteShort(send[i]);
  }
  CAN.endPacket();
}
