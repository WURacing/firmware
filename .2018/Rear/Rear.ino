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

const int frontBrakePin = A4;
const int rearBrakePin = A5;

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
int acc3
byte data[8];

void loop() {
  if (micros() > readAccumulator){
    frontReading = analogRead(frontBrakePin);
    rearReading = analogRead(rearBrakePin);
    readAccumulator += readDeltaTime;
  }

  if (micros() > canAccumulator){
    INIT_RearController0(data);
    SET_RearController0_RearBrakePressure(data, rearReading);
    SET_RearController0_FrontBrakePressure(data, frontReading);

    byte sndStat = CAN0.sendMsgBuf(ID_RearController0, 1, 8, data);
      if(sndStat == CAN_OK){
    Serial.println("Message Sent Successfully!");
  } else {
    Serial.println("Error Sending Message...");
  }
    
    canAccumulator += canDeltaTime;
  }
}
