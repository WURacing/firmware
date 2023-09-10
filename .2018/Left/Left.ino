#include <Wire.h>
#include <SoftwareSerial.h>
#include <Adafruit_GPS.h>
#include <mcp_can.h>
#include <SPI.h>


#include "CANBus18.h"
#include "PacketSender.h"

#define CAN0_INT 4
MCP_CAN CAN0(10);

unsigned long canDeltaTime = 4000;
unsigned long canAccumulator = 0;

unsigned long readDeltaTime = 3333;
unsigned long readAccumulator = 0;

const int topPin = A2;
const int bottomPin = A3;

const int gyroPin1 = A0;
const int gyroPin2 = A1;

const int leftPotPin = A5;
const int rightPotPin = A4;

void setup() {
  Serial.begin(9600);
  // put your setup code here, to run once:
  if (CAN0.begin(MCP_ANY, CAN_500KBPS, MCP_16MHZ) == CAN_OK) {
      Serial.println("MCP2515 Initialized Successfully!");
  }
  else {
      Serial.println("Error Initializing MCP2515...");
  }
  CAN0.setMode(MCP_NORMAL); // send acknowledgements to recieved data
  pinMode(CAN0_INT, INPUT);

  delay(500);

}

int topReading;
int bottomReading;
int gyroReading1;
int gyroReading2;
int leftPotReading;
int rightPotReading;
byte data[8];

void loop() {
  if (micros() > readAccumulator){
    topReading = analogRead(topPin);
    bottomReading = analogRead(bottomPin);
    gyroReading1 = analogRead(gyroPin1);
    gyroReading2 = analogRead(gyroPin2);
    leftPotReading = analogRead(leftPotPin);
    rightPotReading = analogRead(rightPotPin);
    
    readAccumulator += readDeltaTime;
  }

  if (micros() > canAccumulator){
    INIT_LeftController0(data);
    SET_LeftController0_RadiatorTopTemp(data, topReading);
    SET_LeftController0_RadiatorBottomTemp(data, bottomReading);
    SET_LeftController0_CGGyroscope(data, gyroReading1);
    SET_LeftController0_FrontGyroscope(data, gyroReading2);
    SET_LeftController0_LeftDamperTravel(data, leftPotReading);
    SET_LeftController0_RightDamperTravel(data, rightPotReading);

    byte sndStat = CAN0.sendMsgBuf(ID_LeftController0, 1, 8, data);
      if(sndStat == CAN_OK){
    Serial.println("Message Sent Successfully!");
  } else {
    Serial.println("Error Sending Message...");
  }
    
    canAccumulator += canDeltaTime;
  }
}
