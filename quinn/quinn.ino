#include <CAN.h>
#include <SPI.h>

// Declare 3 axis variables 
byte XH;
byte YH;
byte ZH;

// settings

void setup() {

  // Begin UART serial communication between computer and microcontroller (for optional viewing / debugging)
  // Serial.begin(11520);
  
  pinMode(11,OUTPUT); // enable the CS pin
  digitalWrite(11,HIGH); // de-select chip (active low)

  // enable SPI
  SPI.begin();
  SPI.beginTransaction(SPISettings(10000000, MSBFIRST, SPI_MODE3)); // SPI Library setup (specific to chip, see datasheet)

  // enable xyz accel data
  digitalWrite(11,LOW); // select chip
  SPI.transfer(0x20); 
  SPI.transfer(0x3F);
  digitalWrite(11,HIGH); // de-select chip

  // zero data? (this may not work / be necessary) (still figuring this part out / may not need)
  digitalWrite(11,LOW); // select chip
  SPI.transfer(0x26);
  SPI.transfer(0x00);
  digitalWrite(11,HIGH); // de-select chip

  // enable CAN functions
  pinMode(PIN_CAN_STANDBY, OUTPUT);
  digitalWrite(PIN_CAN_STANDBY, false); // turn off STANDBY
  pinMode(PIN_CAN_BOOSTEN, OUTPUT);
  digitalWrite(PIN_CAN_BOOSTEN, true); // turn on booster

  // start the CAN bus at 1000 kbps
  if (!CAN.begin(1000000)) {
    Serial.println("Starting CAN failed!");
    while (1);
  }

}

void loop() {

  // transfer x axis accel data
  digitalWrite(11,LOW); // select chip
  SPI.transfer(0xA9);
  XH = SPI.transfer(0x00);
  digitalWrite(11,HIGH); // de-select chip

  // transfer y axis accel data
  digitalWrite(11,LOW); // select chip
  SPI.transfer(0xAB);
  YH = SPI.transfer(0x00);
  digitalWrite(11,HIGH); // de-select chip

  // transfer z axis accel data
  digitalWrite(11,LOW); // select chip
  SPI.transfer(0xAD);
  ZH = SPI.transfer(0x00);
  digitalWrite(11,HIGH); // de-select chip

  // send CAN Packet
  CAN.beginPacket(0x1);
  CAN.write(XH);
  CAN.write(YH);
  CAN.write(ZH);
  CAN.endPacket();
  Serial.printf("X:%x\tY:%x\tZ:%x\n",XH,YH,ZH);


  // optional delay for serial viewing
  //delay(1);
  
}