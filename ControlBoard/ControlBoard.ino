#include <CAN.h>
#include "ControlBoard.h"
#include <SPI.h>
#include <Servo.h>

#define BAUD_RATE 1000000
#define PULSE 100 //ms
#define BLINK_INTERVAL 1000 //ms

#define ANLG_RES 4096
#define ANLG_VRANGE 3.3

#define UP_IN_PIN 4
#define DOWN_IN_PIN 5
#define UP_OUT_PIN 11
#define DOWN_OUT_PIN 10
#define CLUTCH_IN1_PIN A0
#define CLUTCH_IN2_PIN A1
#define CLUTCH_OUT_PIN 13
#define DRS_IN_PIN 6
#define DRS_OUT_PIN 12

#define CAN_ID 0xCFFF248

#define SERVO_OPEN_POS 90
#define SERVO_CLOSE_POS 90

unsigned short gearPos = 3;
unsigned long upData = 0;
unsigned long downData = 0;
unsigned long drsData = 0;
bool drsState = false;
bool upShifting = false;
bool downShifting = false;
bool drsChanging = false;
const int BIT_NUM = sizeof(upData) * 8; 
unsigned short dataCount = 0;

const int LED = 13;
int ledState = LOW;
unsigned long blinkCurrentMillis = millis();
unsigned long blinkPreviousMillis = 0;

byte CanFrame[8];

Servo clutchServo;
Servo drsServo;

void setup() {

 

  // setting up CAN 
  pinMode(UP_IN_PIN, INPUT);
  pinMode(DOWN_IN_PIN, INPUT);
  pinMode(CLUTCH_IN1_PIN, INPUT);
  pinMode(CLUTCH_IN2_PIN, INPUT);
  pinMode(DRS_IN_PIN, INPUT);
  pinMode(UP_OUT_PIN, OUTPUT);
  pinMode(DOWN_OUT_PIN, OUTPUT);
  digitalWrite(UP_OUT_PIN, LOW);
  digitalWrite(DOWN_OUT_PIN, LOW);
  analogReadResolution(12);

  clutchServo.attach(CLUTCH_OUT_PIN);
  drsServo.attach(DRS_OUT_PIN);

  pinMode(PIN_CAN_STANDBY, OUTPUT);
  digitalWrite(PIN_CAN_STANDBY, false);
  pinMode(PIN_CAN_BOOSTEN, OUTPUT);
  digitalWrite(PIN_CAN_BOOSTEN, true);
  
  Serial.begin(9600);
  if (!CAN.begin(BAUD_RATE)) {
    Serial.println("Starting CAN failed!");
  }
}

/* ----------------------- Methods ----------------------- */
void blink() {
  blinkCurrentMillis = millis();
  if (blinkCurrentMillis - blinkPreviousMillis >= BLINK_INTERVAL) {
    blinkPreviousMillis = blinkCurrentMillis;
    if (ledState == LOW) {
      ledState = HIGH;
    } else {
      ledState = LOW;
    }
    digitalWrite(LED, ledState);
  }
}

void checkShiftPaddles(unsigned long &upData, unsigned long &downData, unsigned short &dataCount)
{
  bool upStatus = !digitalRead(UP_IN_PIN);
  bool downStatus = !digitalRead(DOWN_IN_PIN);
  // Serial.printf("%d,%d\n", upStatus, downStatus);
  modifyBit(upData, dataCount, upStatus);
  modifyBit(downData, dataCount, downStatus);
}

void upshift()
{
  // Serial.println("Upshift");
  digitalWrite(UP_OUT_PIN, HIGH);
  delay(PULSE);
  digitalWrite(UP_OUT_PIN, LOW);
  delay(PULSE);
}

void downshift()
{
  // Serial.println("Downshift");
  digitalWrite(DOWN_OUT_PIN, HIGH);
  delay(PULSE);
  digitalWrite(DOWN_OUT_PIN, LOW);
  delay(PULSE);
}

void updateGearPosition(unsigned short &gearPos)
{
  byte highByte = (byte)(gearPos >> 8);
  byte lowByte = (byte)(gearPos & 0xFF);
}


double getClutchPaddlePosition()
{
  int clutch1 = analogRead(CLUTCH_IN1_PIN);
  int clutch2 = analogRead(CLUTCH_IN2_PIN);
  int value = max(clutch1, clutch2);
  return (double)value / 4096;
}

void setClutchPosition(unsigned int position)
{
  clutchServo.write(position);
}

void modifyBit(unsigned long &d, unsigned short c, bool v)
{
    int mask = 1 << c;
    d = ((d & ~mask) | (v << c));
}

void getDRSButtonState(unsigned long &drsData, unsigned short &dataCount)
{
  bool drsStatus = !digitalRead(DRS_IN_PIN);
  modifyBit(drsData, dataCount, drsStatus);
}

/* ----------------------- Main Loop ----------------------- */
void loop() {
  // Blink LED
  blink();


  // Shift Control
  // updateGearPosition(gearPos);
  checkShiftPaddles(upData, downData, dataCount);

  // Serial.printf("%d,%d\n", upData, downData);

  if (upData == ULONG_MAX && gearPos < 6 && !upShifting && !downShifting)
  {
    upShifting = true;
    upshift();
  }
  if (upData == 0)
  {
    upShifting = false;
  }

  if (downData == ULONG_MAX && gearPos > 1 && !upShifting && !downShifting)
  {
    downShifting = true;
    downshift();
  }
  if (downData == 0)
  {
    downShifting = false;
  }
  

  // Clutch Control
  double position = getClutchPaddlePosition();
  position = (position * -100) + 135; 
  // Serial.println(position);
  setClutchPosition(position);

  // DRS Control
  if (drsData == ULONG_MAX && !drsChanging)
  {
    drsState = !drsState;
    drsChanging = true;
  }
  if (drsData == 0)
  {
    drsChanging = false;
  }
  // setDRSState(drsState);


  // Data count update
  dataCount++;
  if (dataCount == BIT_NUM)
  {
    dataCount = 0;
  }
}
