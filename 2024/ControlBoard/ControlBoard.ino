#include "ControlBoard.h"
#include <SPI.h>
#include <Servo.h>
// #include <CAN.h>

//Initializations

#define BAUD_RATE 1000000
#define PULSE 100           // ms
#define N_PULSE 12.5
#define BLINK_INTERVAL 1000 // ms
#define BLINK_FAST_INTERVAL 250 // ms
#define CAN_INTERVAL 1      // ms

#define ANLG_RES 4096
#define ANLG_VRANGE 3.3

#define UP_IN_PIN 1
#define DOWN_IN_PIN 4
#define UP_OUT_PIN 22
#define DOWN_OUT_PIN 5
#define CLUTCH_IN1_PIN A0
#define CLUTCH_IN2_PIN A1
#define CLUTCH_OUT_PIN 9
#define SMEET_PIN A3

#define CAN_ID 0x100

#define CLUTCH_PULLED 0.8
#define SAMPLE_SIZE 10

unsigned short gearPos, rpm, manAP, wheelSpeed;
unsigned long upData = 0;
unsigned long downData = 0;

bool shifting = false;
bool drsOpen = false;

const int BIT_NUM = sizeof(upData) * 8;
unsigned short dataCount = 0;
double position[SAMPLE_SIZE];

const int LED = 13;
int ledState = LOW;
//Delta Time Loop Setup
unsigned long blinkCurrentMillis = millis();
unsigned long blinkPreviousMillis = 0;
unsigned long canCurrentMillis = millis();
unsigned long canPreviousMillis = 0;
unsigned long runCount = 0;
double positionCommanded = 135;

Servo clutchServo;

void setup()
{
  // setting up CAN
  pinMode(UP_IN_PIN, INPUT);
  pinMode(DOWN_IN_PIN, INPUT);
  pinMode(CLUTCH_IN1_PIN, INPUT);
  pinMode(CLUTCH_IN2_PIN, INPUT);
  
  //setting up Shifting pins
  pinMode(UP_OUT_PIN, OUTPUT);
  pinMode(DOWN_OUT_PIN, OUTPUT);
  pinMode(SMEET_PIN,OUTPUT);

  digitalWrite(UP_OUT_PIN, LOW);
  digitalWrite(DOWN_OUT_PIN, LOW);
  digitalWrite(SMEET_PIN,LOW);
  analogReadResolution(12);

  //setting up Clutch pins
  clutchServo.attach(CLUTCH_OUT_PIN);

  //CAN setup
  pinMode(PIN_CAN_STANDBY, OUTPUT);
  digitalWrite(PIN_CAN_STANDBY, false);
  pinMode(PIN_CAN_BOOSTEN, OUTPUT);
  digitalWrite(PIN_CAN_BOOSTEN, true);

  Serial.begin(9600);
  // if (!CAN.begin(BAUD_RATE))
  // {
  //   Serial.println("Starting CAN failed!");
  // }
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

void blinkFast()
{
  blinkCurrentMillis = millis();
  if (blinkCurrentMillis - blinkPreviousMillis >= BLINK_FAST_INTERVAL)
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

void checkShiftPaddle(){
  while(digitalRead(UP_IN_PIN) == 0 || digitalRead(DOWN_IN_PIN) == 0){
    blinkFast();
  }
}

void checkClutchPaddle(){
  double clutch1 = (double)analogRead(CLUTCH_IN1_PIN) / 4096;
  double clutch2 = (double)analogRead(CLUTCH_IN2_PIN) / 4096;
  while(clutch1 >= CLUTCH_PULLED || clutch2 >= CLUTCH_PULLED){
    blinkFast();
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
  delay(pulse);
}

void downshift(int pulse)
{
  digitalWrite(SMEET_PIN,HIGH);
  digitalWrite(DOWN_OUT_PIN, HIGH);
  delay(pulse);
  digitalWrite(DOWN_OUT_PIN, LOW);
  digitalWrite(SMEET_PIN,LOW);
  delay(pulse);
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
  
  //FOR DEBUGGING
  blink();
  // checkShiftPaddle();
  // checkClutchPaddle();

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
    downshift(PULSE);
  }
  if (downData == 0 && upData == 0)
  {
    shifting = false;
  }

  if (clutch1 >= CLUTCH_PULLED && clutch2 >= CLUTCH_PULLED && downData == ULONG_MAX && !shifting)
  {
    shifting = true;
    downshift(N_PULSE);
  }
  if (clutch1 >= CLUTCH_PULLED && clutch2 >= CLUTCH_PULLED & upData == ULONG_MAX && !shifting)
  {
    shifting = true;
    upshift(N_PULSE);
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
    // positionCommand
    // ed = (-180 / 0.9)  * averagedPosition + 200;
    // positionCommanded = (-2 * sinh(10 * (averagedPosition - 0.35))) + 100;
    // positionCommanded = max(positionCommanded, 26);

    if (averagedPosition < 0.145)
    {
      positionCommanded = 165;
      // positionCommanded = 0;
    }
    else if (averagedPosition > 0.955)
    {
      positionCommanded = 26;
    }
    else
    {
      // positionCommanded = -12.3457 * averagedPosition + 121.79;
      // positionCommanded = -18.5185 * averagedPosition + 127.685;
      positionCommanded = -50.7697 * pow(averagedPosition, 3) + 120.513 * pow(averagedPosition, 2) + -96.6814 * averagedPosition + 136.64;
      // positionCommanded = -50.7697 * pow(averagedPosition, 3) + 120.513 * pow(averagedPosition, 2) + -96.6814 * averagedPosition + 75;
    }

    Serial.print("Clutch paddle: ");
    Serial.print(averagedPosition);
    Serial.print("\tClutch position: ");
    Serial.println(positionCommanded);
  }

  setClutchPosition(positionCommanded);

  // CAN
  // canCurrentMillis = millis();
  // if (canCurrentMillis - canPreviousMillis > CAN_INTERVAL)
  // {
  //   CAN.beginPacket(0x31);
  //   CAN.write(drsOpen);
  //   CAN.write((short)positionCommanded);
  //   CAN.endPacket();
  // }

  // Data count update
  dataCount++;
  if (dataCount == BIT_NUM)
  {
    dataCount = 0;
  }
}
