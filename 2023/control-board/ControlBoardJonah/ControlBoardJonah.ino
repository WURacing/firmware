
#include <CAN.h>
#define BAUD_RATE 1000000
#define PULSE 100 //ms
#define DEBOUNCE 10 //ms
#define UP_IN_PIN 4
#define DOWN_IN_PIN 5
#define UP_OUT_PIN 10
#define DOWN_OUT_PIN 11
#define BLINK_INTERVAL 1000 //ms
int gearPos = 3;
bool ready = true;
bool upIn = LOW;
bool downIn = LOW;
bool upCounter = false;
bool downCounter = false;
bool upstatus = false;
bool downstatus = false;
unsigned long upData = 0;
unsigned long downData = 0;
const int BIT_NUM = sizeof(upData) * 8;
int count = 0;
unsigned long up1time = 0;
unsigned long down1time = 0;
unsigned long up2time = 0;
unsigned long down2time = 0;
const int LED = 13;
int ledState = LOW;
unsigned long blinkCurrentMillis = millis();
unsigned long blinkPreviousMillis = 0;
//BLINK
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
//bit modifying method
unsigned long modifyBit(unsigned long d, int c, bool v)
{
    int mask = 1 << c;
    return ((d & ~mask) | (v << c));
}
void setup() {
  pinMode(UP_IN_PIN, INPUT);
  pinMode(DOWN_IN_PIN, INPUT);
  pinMode(UP_OUT_PIN, OUTPUT);
  pinMode(DOWN_OUT_PIN, OUTPUT);
  digitalWrite(UP_OUT_PIN, LOW);
  digitalWrite(DOWN_OUT_PIN, LOW);
  Serial.begin(9600);
  if (!CAN.begin(BAUD_RATE)) {
    Serial.println(“Starting CAN failed!“);
  }
}
void loop() {
  blink();
  //read gear position into CAN (not done yet)
  upData = modifyBit(upData,count, !digitalRead(UP_IN_PIN));
  downData = modifyBit(downData,count, !digitalRead(DOWN_IN_PIN));
  count++;
  if(count == BIT_NUM){
    count = 0;
  }
  upCounter = (upData == ULONG_MAX && downData != ULONG_MAX && gearPos < 6);
  downCounter = (downData == ULONG_MAX && upData != ULONG_MAX && gearPos > 1);
  //this is a parrallel approach involving 6 if statements
  //1 and 2 are the only ones that can be entered as upstatus/downstatus are not yet true
  //Once 1 is activated, 2 will not be, after 100 ms 3 will be.
  //4 won’t be because of downstatus. 5 will be entered eventually after 100 ms
  //If it loops, the conditionals involving up/downstatus will avoid entering 1/2
  //It will not enter alternates due to the condital, and the up1time has been
  //made irrationally large to p[revent entering 3/4 again before 5/6 has been completed.
  //Should work?
  if(upCounter == true && upstatus == false && downstatus == false){
    up1time = millis();
    upstatus = true;
    Serial.printf(“upIn: %d downIn: %d upshift\n”, upData, downData);
    digitalWrite(UP_OUT_PIN,HIGH);
  }
  if(downCounter == true && downstatus == false && upstatus == false){
    down1time = millis();
    downstatus = true;
    Serial.printf(“upIn: %d downIn: %d downshift\n”, upData, downData);
    digitalWrite(DOWN_OUT_PIN,HIGH);
  }
  if(upstatus==true && millis()>up1time+100){
    digitalWrite(UP_OUT_PIN,LOW);
    up2time = millis();
    up1time = ULONG_MAX-100;
    //this is made large so that this block is not entered until upstatus is reset
  }
  if(downstatus==true && millis()>down1time+100){
    digitalWrite(DOWN_OUT_PIN,LOW);
    down2time = millis();
    down1time = ULONG_MAX-100;
  }
  if(upstatus == true && millis()>up2time+100){
   //wait for release
   while(!digitalRead(UP_IN_PIN)){delay(1);}
   upstatus == false;
  }
  if(downstatus == true && millis()>down2time+100){
    while(!digitalRead(DOWN_IN_PIN)){delay(1);}
    downstatus == false;
  }
}