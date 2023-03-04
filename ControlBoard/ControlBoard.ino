

#include <CAN.h>

#define BAUD_RATE 1000000
#define PULSE 100 //ms
#define DEBOUNCE 10 //ms
#define UP_IN_PIN 4
#define DOWN_IN_PIN 5
#define UP_OUT_PIN 10
#define DOWN_OUT_PIN 11

int gearPos = 3;
bool ready = true;
bool upIn = LOW;
bool downIn = LOW;
unsigned long upData = 0;
unsigned long downData = 0;
const int BIT_NUM = sizeof(upData) * 8; 
int count = 0;


void setup() {
  pinMode(UP_IN_PIN, INPUT);
  pinMode(DOWN_IN_PIN, INPUT);
  pinMode(UP_OUT_PIN, OUTPUT);
  pinMode(DOWN_OUT_PIN, OUTPUT);
  digitalWrite(UP_OUT_PIN, LOW);
  digitalWrite(DOWN_OUT_PIN, LOW);
  Serial.begin(9600);
  if (!CAN.begin(BAUD_RATE)) {
    Serial.println("Starting CAN failed!");
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

/*
* Changes the bit at position c in data b to value v
*/
unsigned long modifyBit(unsigned long d, int c, bool v)
{
    int mask = 1 << c;
    return ((d & ~mask) | (v << c));
}

void loop() {
  // gearPos = CAN.read or something

  upData = modifyBit(upData, count, !digitalRead(UP_IN_PIN));
  downData = modifyBit(downData, count, !digitalRead(DOWN_IN_PIN));
  count++;
  if (count == BIT_NUM)
  {
    count = 0;
  }

  // if (upData != 0 || downData != 0)
  // {
  //       Serial.printf("upIn: %d downIn: %d\n", upData, downData);
  // }

  if (upData == ULONG_MAX && downData != ULONG_MAX && gearPos < 6)
  {
    Serial.printf("upIn: %d downIn: %d upshift\n", upData, downData);
    upshift();
    while(!digitalRead(UP_IN_PIN)){delay(1);} // Wait for release
  }
  if (downData == ULONG_MAX && upData != ULONG_MAX && gearPos > 1)
  {
    Serial.printf("upIn: %d downIn: %d downshift\n", upData, downData);
    downshift();
    while(!digitalRead(DOWN_IN_PIN)){delay(1);} // Wait for release
  }
}