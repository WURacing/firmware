#include <CAN.h>
#include <SPI.h>


byte x_H;
byte y_H;
byte z_H;

int datacount= 0;


byte x_avg = 0;
byte y_avg = 0;
byte z_avg = 0;

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


  x_avg = (((x_H)*(0.1)) + x_avg)/1.1;
  y_avg = (((y_H)*(0.1)) + y_avg)/1.1;
  z_avg = (((z_H)*(0.1)) + z_avg)/1.1;

  


  int averageTime=0;
  if(millis()-averageTime>10){
    averageTime=millis();
    CAN.beginPacket(0x1);
    CAN.write(x_avg);
    CAN.write(y_avg);
    CAN.write(z_avg);
    CAN.endPacket();



//  Serial.print(x_avg);
//  Serial.print(',');
//  Serial.print(y_avg);
//  Serial.print(',');
//  Serial.print(z_avg);

//    delay(1);
    }

  









  
  }
