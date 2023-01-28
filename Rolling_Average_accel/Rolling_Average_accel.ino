#include <CAN.h>
#include <SPI.h>

//#include <goblin mode>

byte x_acc;
byte y_acc;
byte z_acc;

byte x_send;
byte y_send;
byte z_send;

int datacount=0;
const int rollingAverage=128;

byte x_avgs[rollingAverage];
byte y_avgs[rollingAverage];
byte z_avgs[rollingAverage];

long x_tot = 0;
long y_tot = 0;
long z_tot = 0;

const int CS = 5;
const int LED = 13;

int ledState = LOW;
unsigned long currentMillis = millis();
unsigned long previousMillis = 0;
const long interval = 1000;

void blink(){
  currentMillis = millis();
  if (currentMillis - previousMillis >= interval){
    previousMillis = currentMillis;
    if (ledState == LOW){
      ledState = HIGH;
      //Serial.printf("Led High!\n");
    }
    else{
      ledState = LOW;
      //Serial.printf("Led Low!\n");
    }
    digitalWrite(LED,ledState);
  }
}







void setup() {

  

  pinMode(LED,OUTPUT);

  Serial.begin(11520);
  pinMode(CS,OUTPUT);
  digitalWrite(CS,HIGH);

  SPI.begin();
  SPI.beginTransaction(SPISettings(10000000,MSBFIRST,SPI_MODE3));

  digitalWrite(CS,LOW);
  SPI.transfer(0x20);
  SPI.transfer(0x3F);
  digitalWrite(CS,HIGH);

  pinMode(PIN_CAN_STANDBY,OUTPUT);
  digitalWrite(PIN_CAN_STANDBY,false);
  pinMode(PIN_CAN_BOOSTEN,OUTPUT);
  digitalWrite(PIN_CAN_BOOSTEN,true);

  if(!CAN.begin(500000)){
    Serial.println("Starting CAN failed!");
    while(1);
  }
}

void loop() {

  blink();

  digitalWrite(CS,LOW);
  SPI.transfer(0xA9);
  x_acc = SPI.transfer(0x00);
  digitalWrite(CS,HIGH);

  digitalWrite(CS,LOW);
  SPI.transfer(0xAB);
  y_acc = SPI.transfer(0x00);
  digitalWrite(CS,HIGH);

  digitalWrite(CS,LOW);
  SPI.transfer(0xAD);
  z_acc = SPI.transfer(0x00);
  digitalWrite(CS,HIGH);

  x_avgs[datacount%rollingAverage]=x_acc;
  y_avgs[datacount%rollingAverage]=y_acc;
  z_avgs[datacount%rollingAverage]=z_acc;

  x_tot += x_acc;
  y_tot += y_acc;
  z_tot += z_acc;

  
  if(datacount>=(rollingAverage-1)){
    //once this condition is met, the array is filled
    //in order to be more effiecent and not include nested loops,
    ///the code will always subtract off the 0 value in the array 
    //thus all values will be eventually subtracted and we maintain a total 
    //of 128 values
    //the total is then divided by 127 instead of 128

    //when at 127 --> 128 data points and thus we take off first value
    //(rollingAverage-1) used to avoid overflow in x_tot

    x_tot -= x_avgs[datacount%(rollingAverage-1)]; //this is 0 when full, 1 on next go, etc
    y_tot -= y_avgs[datacount%(rollingAverage-1)];
    z_tot -= z_avgs[datacount%(rollingAverage-1)];

    x_send = (byte)(x_tot/127);
    y_send = (byte)(y_tot/127);
    z_send = (byte)(z_tot/127);

    
    unsigned long averageTime=0;
    if(millis()-averageTime>10){
      averageTime = millis();

      CAN.beginPacket(0x1);
      CAN.write(x_send);
      CAN.write(y_send);
      CAN.write(z_send);
      CAN.endPacket();
    }
  }
  Serial.printf("X:%d\tY:%d\tZ:%d\n",x_tot,y_tot,z_tot);
}
