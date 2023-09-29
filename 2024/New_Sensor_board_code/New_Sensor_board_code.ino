#include <Adafruit_NeoPixel.h>
#include <CAN.h>
#include <SPI.h>
#include <Mux.h>
#include <DFRobot_BMX160.h>
#include "SensorBoard.h"
//#include <GoblinMode.h>


#define LEDPIN 8
#define BLINK_INTERVAL 1000
#define BAUD_RATE 1000000
#define ANLG_RES 4096
#define ANLG_VRANGE 3.3
#define CAN_INTERVAL 1

unsigned long datacount = 0;

short analogs[20];
short accel[3];
short gyro[3];
short magn[3];
double average_matrix[29];


//accel/gyro sensor definition
DFRobot_BMX160 bmx160;

//to make using the mux space easier
using namespace admux;

void setup() {
  //Pin Definitions
  pinMode(PIN_NEOPIXEL_POWER, OUTPUT);
  pinMode(A0,INPUT);
  pinMode(A1,INPUT);
  pinMode(A2,INPUT);
  pinMode(A3,INPUT);
  pinMode(A4,INPUT);
  analogReadResolution(12);//for our board --> bit resolution

  Serial.begin(9600);
  
  
  
  //LED SETUP
  strip.begin();
  strip.clear();
  strip.setBrightness(20);
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
  blink(200,0,100);

  //Mux data processing
  int data;
  //A4 is voltage and we others are the identifiers
  Mux mux(Pin(A4,INPUT,PinType::Analog),Pinset(4,5,6,7));
  for(byte i=0; i<mux.channelCount(); i++){
    //taking in every value of the MUX
    data = mux.read();
    //adding each value with linearizations
    analogs[i]=(data / (float)ANLG_RES) * 1000 * ANLG_VRANGE;
  }

  //Manual Data (The hard wired Analog Inputs)
  readAnalogsMan(analogs);

  //New Sensor Event
  //Taking in Accel, Gyro, Magnetometer results
  sBmx160SensorData_t Omagn, Ogyro,Oaccel;
  //these are all objects with x,y,z attributes
  //we source this from the attached library

  bmx160.getAllData(&Omagn, &Ogyro, &Oaccel);
  accel[0] = Oaccel.x;
  accel[1] = Oaccel.y;
  accel[2] = Oaccel.z;

  gyro[0] = Ogyro.x;
  gyro[1] = Ogyro.y;
  gyro[2] = Ogyro.z;

  magn[0] = Omagn.x;
  magn[1] = Omagn.y;
  magn[2] = Omagn.z;

  //9 Sensor Values taken in here (3 seperate arrays)

  //each column of average_matrix will accumulate the average value over 10 entries

  for(int i=0; i<20; i++){
    average_matrix[i] = analogs[i];
  }

  for(int i=0; i<3; i++){
    average_matrix[i+20] = accel[i];
    average_matrix[i+23] = gyro[i];
    average_matrix[i+26] = magn[i];
  }

  short avg_send[29];
  if(datacount%10){
    //divide by averaging sampling size
    //convert to short for CAN frame
    for(int i=0; i<29;i++){
      average_matrix[i] = average_matrix[i]/10.0;
      avg_send[i] = (short)average_matrix[i];
    }

    

    

    //Send CAN Frame

    CAN.beginPacket(0x20);
    canWriteShort(avg_send[0]);
    canWriteShort(avg_send[1]);
    canWriteShort(avg_send[2]);
    canWriteShort(avg_send[3]);
    CAN.endPacket();

    CAN.beginPacket(0x21);
    canWriteShort(avg_send[4]);
    canWriteShort(avg_send[5]);
    canWriteShort(avg_send[6]);
    canWriteShort(avg_send[7]);
    CAN.endPacket();

    CAN.beginPacket(0x22);
    canWriteShort(avg_send[8]);
    canWriteShort(avg_send[9]);
    canWriteShort(avg_send[10]);
    canWriteShort(avg_send[11]);
    CAN.endPacket();

    CAN.beginPacket(0x23);
    canWriteShort(avg_send[12]);
    canWriteShort(avg_send[13]);
    canWriteShort(avg_send[14]);
    canWriteShort(avg_send[15]);
    CAN.endPacket();

    CAN.beginPacket(0x24);
    canWriteShort(avg_send[16]);
    canWriteShort(avg_send[17]);
    canWriteShort(avg_send[18]);
    canWriteShort(avg_send[19]);
    CAN.endPacket();

    CAN.beginPacket(0x25);
    canWriteShort(avg_send[20]);
    canWriteShort(avg_send[21]);
    canWriteShort(avg_send[22]);
    canWriteShort(avg_send[23]);
    CAN.endPacket();

    CAN.beginPacket(0x26);
    canWriteShort(avg_send[24]);
    canWriteShort(avg_send[25]);
    canWriteShort(avg_send[26]);
    canWriteShort(avg_send[27]);
    CAN.endPacket();

    CAN.beginPacket(0x27);
    canWriteShort(avg_send[28]);
    CAN.endPacket();

    //clear the avg_send and average_matrix arrays 
    for(int i=0; i<29; i++){
      avg_send[i]=0;
      average_matrix[i]=0;
    }
    
  } 

  datacount++;
}
