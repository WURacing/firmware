#include <CAN.h>
#include "ControlBoard.h"

#define BAUD_RATE 1000000
#define PULSE 100 //ms
#define BLINK_INTERVAL 1000 //ms
#define UP_IN_PIN 4
#define DOWN_IN_PIN 5
#define UP_OUT_PIN 10
#define DOWN_OUT_PIN 11
// TODO: Figure out pins for clutch
#define CLUTCH_IN_PIN 100
#define CLUTCH_OUT_PIN 100


unsigned short gearPos = 3;
unsigned long upData = 0;
unsigned long downData = 0;
bool upShifting = false;
bool downShifting = false;
const int BIT_NUM = sizeof(upData) * 8; 
unsigned short dataCount = 0;

const int LED = 13;
int ledState = LOW;
unsigned long blinkCurrentMillis = millis();
unsigned long blinkPreviousMillis = 0;

void setup() {
  pinMode(UP_IN_PIN, INPUT);
  pinMode(DOWN_IN_PIN, INPUT);
  pinMode(CLUTCH_IN_PIN, INPUT);
  pinMode(UP_OUT_PIN, OUTPUT);
  pinMode(DOWN_OUT_PIN, OUTPUT);
  digitalWrite(UP_OUT_PIN, LOW);
  digitalWrite(DOWN_OUT_PIN, LOW);
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
  modifyBit(upData, dataCount, !digitalRead(UP_IN_PIN));
  modifyBit(downData, dataCount, !digitalRead(DOWN_IN_PIN));
  dataCount++;
  if (dataCount == BIT_NUM)
  {
    dataCount = 0;
  }
}

void upshift()
{
  digitalWrite(UP_OUT_PIN, HIGH);
  delay(PULSE);
  digitalWrite(UP_OUT_PIN, LOW);
  delay(PULSE);
}

void downshift()
{
  digitalWrite(DOWN_OUT_PIN, HIGH);
  delay(PULSE);
  digitalWrite(DOWN_OUT_PIN, LOW);
  delay(PULSE);
}

void updateGearPosition(unsigned short &gearPos)
{
  // TODO: gearPos = CAN.read or something
}

unsigned int getClutchPaddlePosition()
{
  int value = analogRead(CLUTCH_IN_PIN);
  // TODO: Convert value to position
  return value;
}

void setClutchPosition(unsigned int position)
{
  // TODO: Control clutch servo
}

void modifyBit(unsigned long &d, unsigned short c, bool v)
{
    int mask = 1 << c;
    d = ((d & ~mask) | (v << c));
}

/* ----------------------- Main Loop ----------------------- */
void loop() {
  // Blink LED
  blink();


  // Shift Control
  updateGearPosition(gearPos);
  checkShiftPaddles(upData, downData, dataCount);

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
  setClutchPosition(getClutchPaddlePosition());
}