#include <SPI.h>
#include <RH_RF95.h>
#include <CAN.h>

//CANSAME5x CAN;


#define RF95_FREQ 915.0
#define RFM95_RST 11  // "A"
#define RFM95_CS  10  // "B"
#define RFM95_INT  6  // "D"

//Delta Time Loop Setup
#define BLINK_INTERVAL 1000
#define LED 13
#define starter_val 50
unsigned long blinkCurrentMillis = millis();
unsigned long blinkPreviousMillis = 0;
bool LEDState = LOW;
int test = 0;
unsigned long current_millis = millis();
int starter = starter_val;
int value = 0;

RH_RF95 rf95(RFM95_CS, RFM95_INT);

#define Tel_Period 1000


void setup() {
  pinMode(RFM95_RST, OUTPUT);
  digitalWrite(RFM95_RST, HIGH);

  Serial.begin(115200);
//  while (!Serial) delay(1);
  delay(100);

  // manual reset
  digitalWrite(RFM95_RST,LOW);
  delay(10);
  digitalWrite(RFM95_RST, HIGH);
  delay(10);

  while (!rf95.init()) {
    Serial.println("LoRa radio init failed");
    Serial.println("Uncomment '#define SERIAL_DEBUG' in RH_RF95.cpp for detailed debug info");
    while (1);
  }

  Serial.println("LoRa radio init OK!");

  if (!rf95.setFrequency(RF95_FREQ)) {
    Serial.println("setFrequency failed");
    while (1);
  }

  Serial.print("Set Freq to: "); Serial.println(RF95_FREQ);

  rf95.setTxPower(23, false);

  pinMode(PIN_CAN_STANDBY, OUTPUT);
  digitalWrite(PIN_CAN_STANDBY, false);
  pinMode(PIN_CAN_BOOSTEN, OUTPUT);
  digitalWrite(PIN_CAN_BOOSTEN, true);

  if(!CAN.begin(1000000)){
    Serial.println("Starting CAN failed!");
    while(1);
  }
}

int16_t packetnum = 0;

//byte ANLG1[8];
//byte ANLG2[8];
//byte ANLG3[8];
//byte ANLG4[8];
//byte ANLG5[8];
//byte ANLG6[8];
//byte ANLG7[8];
//byte ANLG8[8];
//byte CTRLBD[8];

byte DBC_Matrix[72];
short s_DBC_Matrix[36];
byte Test[2];

unsigned long curTime = millis();
unsigned long prevTime = 0;

void loop() {
  blink();
  delay(10);
  curTime = millis();
  int packetSize = CAN.parsePacket();
  byte packetID = CAN.packetId();
  Serial.println(packetID);



  
  CANFiller(packetID,packetSize, DBC_Matrix);

  //Convert bytes to shorts for validation

  convertBytesToShorts(DBC_Matrix, s_DBC_Matrix, 72);

  
  Serial.print("Shorts: ");
  for (int i = 0; i < 36; i++) {
    Serial.print(s_DBC_Matrix[i]);
    Serial.print(",");
  }
  Serial.println(" ");
  if(curTime-prevTime >= Tel_Period){
    prevTime = curTime;
    //We sending Telemetry baby
    //does the last one have to be zero?
    //it will be for now although if we fill the frames idk

    //not sure if we need to typecast or not
    Serial.println("Sending...");
    rf95.send((uint8_t *)DBC_Matrix, 72);

//    Serial.println("Waiting for packet to complete...");
//    delay(10);
//    rf95.waitPacketSent();
//    uint8_t buf[RH_RF95_MAX_MESSAGE_LEN]; //251
//    uint8_t len = sizeof(buf);
//
//    Serial.println("Waiting for reply...");
//    if (rf95.waitAvailableTimeout(1000)) {
//      // Should be a reply message for us now
//      if (rf95.recv(buf, &len)) {
//        Serial.print("Got reply: ");
//        Serial.println((char*)buf);
//        Serial.print("RSSI: ");
//        Serial.println(rf95.lastRssi(), DEC);
//      } else {
//      Serial.println("Receive failed");
//      }
//    } else {
//      Serial.println("No reply, is there a listener around?");
//    }
  }

//  if (rf95.waitAvailableTimeout(1000)) {
//    //Should be a reply message for us now
//    if (rf95.recv(buf, &len)) {
//      Serial.print("Got reply: ");
//      Serial.println((char*)buf);
//      Serial.print("RSSI: ");
//      Serial.println(rf95.lastRssi(), DEC);
//    } 
//    else {
//      Serial.println("Receive failed");
//    }
//  } 
//  else {
//      Serial.println("No reply, is there a listener around?");
//  }
//}
  

  

  

//  curTime = millis();
//  Test[0] = packetID;
//  Test[1] = (byte)packetSize;
//  
//
//  rf95.send((uint8_t *)Test, 2);
//  delay(10);
//  rf95.waitPacketSent();

  

  
  

  


  

//  if(curTime-prevTime >= Tel_Period){
//    //We sending Telemetry baby
//    //does the last one have to be zero?
//    //it will be for now although if we fill the frames idk
//
//    //not sure if we need to typecast or not
//    Serial.println("Sending...");
//    rf95.send((uint8_t *)DBC_Matrix, 72);
//
//    Serial.println("Waiting for packet to complete...");
//    delay(10);
//    rf95.waitPacketSent();
//    uint8_t buf[RH_RF95_MAX_MESSAGE_LEN]; //251
//    uint8_t len = sizeof(buf);
//
//    Serial.println("Waiting for reply...");
//    if (rf95.waitAvailableTimeout(1000)) {
//      // Should be a reply message for us now
//      if (rf95.recv(buf, &len)) {
//        Serial.print("Got reply: ");
//        Serial.println((char*)buf);
//        Serial.print("RSSI: ");
//        Serial.println(rf95.lastRssi(), DEC);
//      } else {
//      Serial.println("Receive failed");
//      }
//    } else {
//      Serial.println("No reply, is there a listener around?");
//    }
//  }
}


void CANFiller(byte packetID, int packetSize, byte* DBC_Matrix){
  //Decimal values associated with the hex code for the CAN address
  if(packetID == 22){
    for(int i=0; i< packetSize; i=i+1){
//      ANLG1[i] = CAN.read();
       DBC_Matrix[i] = CAN.read();
    }
  }
  else if(packetID == 23){
    for(int i=0; i< packetSize; i=i+1){
//      ANLG2[i] = CAN.read();
       DBC_Matrix[i+8] = CAN.read();
    }
  }
  else if(packetID == 24){
    for(int i=0; i< packetSize; i=i+1){
//      ANLG3[i] = CAN.read();
       DBC_Matrix[i+16] = CAN.read();
    }
  }
  else if(packetID == 25){
    for(int i=0; i< packetSize; i=i+1){
//      ANLG4[i] = CAN.read();
       DBC_Matrix[i+24] = CAN.read();
    }
  }
  else if(packetID == 32){
    for(int i=0; i< packetSize; i=i+1){
//      ANLG5[i] = CAN.read();
       DBC_Matrix[i+32] = CAN.read();
    }
  }
  else if(packetID == 33){
    for(int i=0; i< packetSize; i=i+1){
//      ANLG6[i] = CAN.read();
       DBC_Matrix[i+40] = CAN.read();
    }
  }
  else if(packetID == 34){
    for(int i=0; i< packetSize; i=i+1){
//      ANLG7[i] = CAN.read();
       DBC_Matrix[i+48] = CAN.read();
    }
  }
  else if(packetID == 35){
    for(int i=0; i< packetSize; i=i+1){
//      ANLG8[i] = CAN.read();
        DBC_Matrix[i+56] = CAN.read();
    }
  }
  else if(packetID == 73){
    for(int i=0; i< packetSize; i=i+1){
//      CTRLBD[i] = CAN.read();
       DBC_Matrix[i+64]= CAN.read();
    }
  }
}

void convertBytesToShorts(byte* byteArray, short* shortArray, int byteArraySize) {
  for (int i = 0; i < byteArraySize / 2; i++) {
    shortArray[i] = (short(byteArray[2 * i + 1]) << 8) | byteArray[2 * i];
  }
}

void blink()
{
  blinkCurrentMillis = millis();
  if (blinkCurrentMillis - blinkPreviousMillis >= BLINK_INTERVAL)
  {
    blinkPreviousMillis = blinkCurrentMillis;
    if (LEDState == LOW)
    {
      LEDState = HIGH;
    }
    else
    {
      LEDState = LOW;
    }
    digitalWrite(LED, LEDState);
  }
}
