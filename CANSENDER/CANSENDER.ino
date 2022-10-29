#include <CAN.h>
#include <SPI.h>
#include <arduinoFFT.h>


byte x_H;
byte y_H;
byte z_H;

int datacount= 0;

double x_countz[16];
double y_countz[16];
double z_countz[16];

double x_avg = 0.0;
double y_avg = 0.0;
double z_avg = 0.0;

int CS= 11;




void setup() {
  Serial.begin(11520);
  pinMode(CS, OUTPUT);
  digitalWrite(CS, HIGH);

  SPI.begin();
  SPI.beginTransaction(SPISettings(10000000, MSBFIRST, SPI_MODE3));

  digitalWrite(CS,LOW);
  SPI.transfer(0x20);
  SPI.transfer(0x3F);
  digitalWrite(CS,HIGH);

  while(!Serial);

  pinMode(PIN_CAN_STANDBY, OUTPUT);
  digitalWrite(PIN_CAN_STANDBY, false);
  pinMode(PIN_CAN_BOOSTEN, OUTPUT);
  digitalWrite(PIN_CAN_BOOSTEN, true);

  if(!CAN.begin(500000)){
    Serial.println("Starting CAN failed!");
    while(1);
  }
}
  


void loop() {
  digitalWrite(CS,LOW);
  SPI.transfer(0xA9);
  x_H= SPI.transfer(0x00);
  digitalWrite(CS,HIGH);

  digitalWrite(CS,LOW);
  SPI.transfer(0xAB);
  y_H= SPI.transfer(0x00);
  digitalWrite(CS,HIGH);

  digitalWrite(CS,LOW);
  SPI.transfer(0xAD);
  z_H= SPI.transfer(0x00);
  digitalWrite(CS,HIGH);


  x_countz[datacount%16]=(double)x_H;
  y_countz[datacount%16]=(double)y_H;
  z_countz[datacount%16]=(double)z_H;

  if(datacount>15){
    for(int i=0; i<16; i++){
      x_avg+=x_countz[i];
      y_avg+=y_countz[i];
      z_avg+=z_countz[i];
    }
    x_avg=x_avg/16.0;
    y_avg=y_avg/16.0;
    z_avg=z_avg/16.0;


    int averageTime=0;
    if(millis()-averageTime>10){
      averageTime=millis();
      CAN.beginPacket(0x1);
      CAN.write(x_avg);
      CAN.write(y_avg);
      CAN.write(z_avg);
      CAN.endPacket();
    }
  }

}
