#include "ControlBoard.h"
#include <SPI.h>
#include <Servo.h>
#include <CAN.h>

#define BAUD_RATE 1000000
#define PULSE 100           // ms
#define BLINK_INTERVAL 1000 // ms
#define CAN_INTERVAL 1      // ms

#define ANLG_RES 4096
#define ANLG_VRANGE 3.3

#define UP_IN_PIN 5
#define DOWN_IN_PIN 4
#define UP_OUT_PIN 11
#define DOWN_OUT_PIN 10
#define CLUTCH_IN1_PIN A0
#define CLUTCH_IN2_PIN A1
#define CLUTCH_OUT_PIN 12
#define DRS_OPEN_PIN 6
#define DRS_CLOSE_PIN 9
#define DRS_OUT_PIN 13

#define CAN_ID 0x100

#define CLUTCH_PULLED 0.8
#define DRS_OPEN 30
#define DRS_CLOSE 100
#define SAMPLE_SIZE 10
#define CLUTCH_CLOSED 135

unsigned short gearPos, rpm, manAP, wheelSpeed;
unsigned long upData = 0;
unsigned long downData = 0;
unsigned long drsOpenData = 0;
unsigned long drsCloseData = 0;
bool shifting = false;
bool drsOpen = false;
bool drsChanging = false;
bool drsEnabled = true;
const int BIT_NUM = sizeof(upData) * 8;
unsigned short dataCount = 0;
double position[SAMPLE_SIZE];

const int LED = 13;
int ledState = LOW;
unsigned long blinkCurrentMillis = millis();
unsigned long blinkPreviousMillis = 0;
unsigned long canCurrentMillis = millis();
unsigned long canPreviousMillis = 0;
unsigned long runCount = 0;
double positionCommanded = 135;

Servo clutchServo;
Servo drsServo;

void setup()
{
  // setting up CAN
  pinMode(UP_IN_PIN, INPUT);
  pinMode(DOWN_IN_PIN, INPUT);
  pinMode(CLUTCH_IN1_PIN, INPUT);
  pinMode(CLUTCH_IN2_PIN, INPUT);
  pinMode(DRS_OPEN_PIN, INPUT);
  pinMode(UP_OUT_PIN, OUTPUT);
  pinMode(DOWN_OUT_PIN, OUTPUT);
  pinMode(DRS_CLOSE_PIN, INPUT);
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
  if (!CAN.begin(BAUD_RATE))
  {
    Serial.println("Starting CAN failed!");
  }
}

/* ----------------------- Methods ----------------------- */
void blink()
{
  blinkCurrentMillis = millis();
  if (blinkCurrentMillis - blinkPreviousMillis >= BLINK_INTERVAL)
  {
    blinkPreviousMillis = blinkCurrentMillis;
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

void checkShiftPaddles(unsigned long &upData, unsigned long &downData, unsigned short &dataCount)
{
  bool upStatus = !digitalRead(UP_IN_PIN);
  bool downStatus = !digitalRead(DOWN_IN_PIN);
  modifyBit(upData, dataCount, upStatus);
  modifyBit(downData, dataCount, downStatus);
}

void upshift(int pulse)
{
  digitalWrite(UP_OUT_PIN, HIGH);
  delay(pulse);
  digitalWrite(UP_OUT_PIN, LOW);
  delay(PULSE);
}

void downshift(int pulse)
{
  digitalWrite(DOWN_OUT_PIN, HIGH);
  delay(pulse);
  digitalWrite(DOWN_OUT_PIN, LOW);
  delay(PULSE);
}

double getClutchPaddlePosition()
{
  int clutch1 = analogRead(CLUTCH_IN1_PIN);
  int clutch2 = analogRead(CLUTCH_IN2_PIN);
  int value = max(clutch1, clutch2);
  return (double)value / 4096;
}

void getClutchPaddlePositions(double &clutch1, double &clutch2)
{
  clutch1 = (double)analogRead(CLUTCH_IN1_PIN) / 4096;
  clutch2 = (double)analogRead(CLUTCH_IN2_PIN) / 4096;
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

void getDRSState(unsigned long &drsOpenData, unsigned long &drsCloseData, unsigned short &dataCount)
{
  bool drsOpenStatus = !digitalRead(DRS_OPEN_PIN);
  bool drsCloseStatus = !digitalRead(DRS_CLOSE_PIN);
  // Serial.printf("drsOpenStatus: %d, drsCloseStatus: %d:\n", drsOpenStatus, drsCloseStatus);
  modifyBit(drsOpenData, dataCount, drsOpenStatus);
  modifyBit(drsCloseData, dataCount, drsCloseStatus);
}

void setDRS(bool drsOpen)
{
  if (drsOpen)
  {
    drsServo.write(DRS_OPEN);
  }
  else
  {
    drsServo.write(DRS_CLOSE);
  }
}

void enableDRS(bool enable)
{
  if (enable)
  {
    drsServo.attach(DRS_OUT_PIN);
  }
  else
  {
    drsServo.detach();
    pinMode(DRS_OUT_PIN, OUTPUT);
    digitalWrite(DRS_OUT_PIN, LOW);
  }
}

double sum(double *arr, int size)
{
  double sum = 0;
  for (int i = 0; i < size; i++)
  {
    sum += arr[i];
  }
  return sum;
}

/* ----------------------- Main Loop ----------------------- */
void loop()
{
  // Blink LED
  blink();

  // Shift Control
  double clutch1;
  double clutch2;
  getClutchPaddlePositions(clutch1, clutch2);
  checkShiftPaddles(upData, downData, dataCount);

  if (upData == ULONG_MAX && !shifting && !(clutch1 >= CLUTCH_PULLED && clutch2 >= CLUTCH_PULLED))
  {
    shifting = true;
    upshift(PULSE);
  }
  if (upData == 0 && downData == 0)
  {
    shifting = false;
  }

  if (downData == ULONG_MAX && !shifting && !(clutch1 >= CLUTCH_PULLED && clutch2 >= CLUTCH_PULLED))
  {
    shifting = true;
    setClutchPosition(35);
    downshift(PULSE);
  }
  if (downData == 0 && upData == 0)
  {
    shifting = false;
  }

  // Serial.printf("Clutch 1:%f\tClutch 2:%f\tshifting:%d\n", clutch1, clutch2, shifting);
  if (clutch1 >= CLUTCH_PULLED && clutch2 >= CLUTCH_PULLED && downData == ULONG_MAX && !shifting)
  {
    shifting = true;
    downshift(PULSE * 0.5);
  }
  if (clutch1 >= CLUTCH_PULLED && clutch2 >= CLUTCH_PULLED & upData == ULONG_MAX && !shifting)
  {
    shifting = true;
    upshift(PULSE * 0.5);
  }
  if (downData == 0 && upData == 0)
  {
    shifting = false;
  }

  // Clutch Control
  position[runCount % 10] = getClutchPaddlePosition();
  if (runCount < 10)
  {
    runCount++;
  }
  else
  {
    runCount = 0;
    double averagedPosition = (sum(position, SAMPLE_SIZE) / (double)SAMPLE_SIZE);
    // positionCommanded = (averagedPosition * -100) + CLUTCH_CLOSED; // Old function
    positionCommanded = (-1.5 * sinh(10 * (averagedPosition - 0.35))) + 100;
    positionCommanded = max(positionCommanded, 0);
    // Serial.printf("Paddle position: %f\tClutch postion: %f\n", averagedPosition, positionCommanded);
  }
  // Serial.println(position);
  setClutchPosition(positionCommanded);

  // Enable / Disable DRS
  getDRSState(drsOpenData, drsCloseData, dataCount);
  // if (drsOpenData == ULONG_MAX && !drsChanging && clutch1 >= CLUTCH_PULLED && clutch2 >= CLUTCH_PULLED)
  // {
  //   drsEnabled = !drsEnabled;
  //   enableDRS(drsEnabled);
  //   drsChanging = true;
  // }

  // DRS Control
  if (drsOpenData == ULONG_MAX && !drsChanging)
  {
    drsOpen = true;
    drsChanging = true;
  }
  if (drsCloseData == ULONG_MAX && !drsChanging)
  {
    drsOpen = false;
    drsChanging = true;
  }
  if (drsOpenData == 0 && drsCloseData == 0)
  {
    drsChanging = false;
  }
  setDRS(drsOpen);

  // CAN
  canCurrentMillis = millis();
  if (canCurrentMillis - canPreviousMillis > CAN_INTERVAL)
  {
    CAN.beginPacket(0x31);
    CAN.write(drsOpen);
    CAN.write((short)positionCommanded);
    CAN.endPacket();
  }

  // Data count update
  dataCount++;
  if (dataCount == BIT_NUM)
  {
    dataCount = 0;
  }
}
