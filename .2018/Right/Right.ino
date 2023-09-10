#include <Wire.h>
#include <SoftwareSerial.h>
#include <Adafruit_GPS.h>
#include <mcp_can.h>
#include <SPI.h>


#include "CANBus18.h"
#include "PacketSender.h"

#define CAN0_INT 4
MCP_CAN CAN0(10);

unsigned long canDeltaTime = 50;
unsigned long canAccumulator = 0;

unsigned long readDeltaTime = 4000;
unsigned long readAccumulator = 0;

const int frontBrakePin = A1;
const int rearBrakePin = A0;

const int acc1Pin = A2;
const int acc2Pin = A3;
const int acc3Pin = A5;

const int mapPin = A4;

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

int frontReading;
int rearReading;
int acc1;
int acc2;
int acc3;
int innerMap;
byte data[8];

void loop() {
  if (micros() > readAccumulator){
    frontReading = analogRead(frontBrakePin);
    rearReading = analogRead(rearBrakePin);
    acc1 = analogRead(acc1Pin);
    acc2 = analogRead(acc2Pin);
    acc3 = analogRead(acc3Pin);
    innerMap = analogRead(mapPin);
    readAccumulator += readDeltaTime;
  }

  if (micros() > canAccumulator){
    INIT_RightController0(data);
    SET_RightController0_RearBrakePressure(data, rearReading);
    SET_RightController0_FrontBrakePressure(data, frontReading);
    SET_RightController0_CGAcc1(data, acc1);
    SET_RightController0_CGAcc2(data, acc2);
    SET_RightController0_CGAcc3(data, acc3);
    SET_RightController0_InnerManifoldPressure(data, innerMap);

    byte sndStat = CAN0.sendMsgBuf(ID_RightController0, 1, 8, data);
      if(sndStat == CAN_OK){
    Serial.println("Message Sent Successfully!");
  } else {
    Serial.println("Error Sending Message...");
  }
    
    canAccumulator += canDeltaTime;
  }
}
