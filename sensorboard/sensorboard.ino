#include <CAN.h>
#include <SPI.h>

#define BAUD_RATE 1000000

byte x_accel;
byte y_accel;
byte z_accel;

int datacount= 0;

const int ACCEL_PIN = 5;
const int LED = 13;

int ledState = LOW;
unsigned long currentMillis = millis();
unsigned long previousMillis = 0; 
const long interval = 1000;

// Method for status indicator
void blink()
{
  currentMillis = millis();
  if (currentMillis - previousMillis >= interval) 
  {
    previousMillis = currentMillis;
    if (ledState == LOW) 
    {
      ledState = HIGH;
    } 
    else 
    {
      ledState = LOW;
    }
    digitalWrite(LED, ledState);
  }
}

// Configured for + or - 2g
void setup() 
{
  pinMode(LED, OUTPUT);

  Serial.begin(11520);
  pinMode(ACCEL_PIN, OUTPUT);
  digitalWrite(ACCEL_PIN, HIGH);

  SPI.begin();
  SPI.beginTransaction(SPISettings(10000000, MSBFIRST, SPI_MODE3));

  digitalWrite(ACCEL_PIN,LOW);
  SPI.transfer(0x20);
  SPI.transfer(0x3F);
  digitalWrite(ACCEL_PIN,HIGH);


  pinMode(PIN_CAN_STANDBY, OUTPUT);
  digitalWrite(PIN_CAN_STANDBY, false);
  pinMode(PIN_CAN_BOOSTEN, OUTPUT);
  digitalWrite(PIN_CAN_BOOSTEN, true);

  if(!CAN.begin(BAUD_RATE))
  {
    Serial.println("Starting CAN failed!");
    while(1);
  }
}

void loop() 
{
  blink();

  digitalWrite(ACCEL_PIN,LOW);
  SPI.transfer(0xA9);
  x_accel= SPI.transfer(0x00);
  digitalWrite(ACCEL_PIN,HIGH);

  digitalWrite(ACCEL_PIN,LOW);
  SPI.transfer(0xAB);
  y_accel= SPI.transfer(0x00);
  digitalWrite(ACCEL_PIN,HIGH);

  digitalWrite(ACCEL_PIN,LOW);
  SPI.transfer(0xAD);
  z_accel= SPI.transfer(0x00);
  digitalWrite(ACCEL_PIN,HIGH);

  // Delta time
  int averageTime=0;
  if(millis()-averageTime>10)
  {
    averageTime=millis();
    CAN.beginPacket(0x1);
    Serial.printf("X:%d\tY:%d\tZ:%d\n", x_accel, y_accel, z_accel);
    CAN.write(x_accel);
    CAN.write(y_accel);
    CAN.write(z_accel);
    CAN.endPacket();
  }

}
  