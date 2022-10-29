#include <CAN.h>

double x_H;
double y_H;
double z_H;


void setup(){
  Serial.begin(11520);

  pinMode(PIN_CAN_STANDBY, OUTPUT);
  digitalWrite(PIN_CAN_STANDBY, false);
  pinMode(PIN_CAN_BOOSTEN, OUTPUT);
  digitalWrite(PIN_CAN_BOOSTEN, true);

  if(!CAN.begin(500000)){
    Serial.println("Starting CAN failed!");
    while(1);
  }
}

void loop(){
  int packetSize = CAN.parsePacket();
  for (int i=0; i<packetSize; i=i+3){
    Serial.print(CAN.read());
    Serial.print(',');
    Serial.print(CAN.read());
    Serial.print(',');
    Serial.print(CAN.read());
  }
}
